from http.server import SimpleHTTPRequestHandler
from socketserver import TCPServer
import json
import sqlite3
from pathlib import Path
from typing import Any, Dict, List
import os
import time

class DBController:
    """Controller that encapsulates database operations with resiliency for SQLite locks."""
    def __init__(self, db_path: Path, timeout: float = 30.0, retries: int = 5):
        self.db_path = db_path
        self.timeout = timeout
        self.retries = retries

    def initialize(self) -> None:
        with sqlite3.connect(self.db_path, timeout=self.timeout) as conn:
            cur = conn.cursor()
            cur.execute('''
                CREATE TABLE IF NOT EXISTS sensor_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    node_id TEXT NOT NULL,
                    timestamp TEXT NOT NULL,
                    temperature_celsius REAL,
                    humidity_percent REAL,
                    luminosity_lux REAL,
                    presence_detected BOOLEAN,
                    power_on BOOLEAN
                )
            ''')
            cur.execute('PRAGMA journal_mode = WAL;')
            cur.execute('PRAGMA synchronous = NORMAL;')
            conn.commit()

    def _connect(self):
        return sqlite3.connect(self.db_path, timeout=self.timeout)

    def save(self, payload: Dict[str, Any]) -> None:
        sensors = payload.get('sensors')
        if sensors is None:
            sqlite3.OperationalError('No sensor data provided')
            return
        sql = '''
            INSERT INTO sensor_data (
                node_id, timestamp, temperature_celsius, humidity_percent,
                luminosity_lux, presence_detected, power_on
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        '''
        params = (
            payload.get('node_id'),
            payload.get('timestamp'),
            sensors.get('temperature_celsius'),
            sensors.get('humidity_percent'),
            sensors.get('luminosity_lux'),
            1 if sensors.get('presence_detected') else 0,
            1 if sensors.get('power_on') else 0
        )

        for attempt in range(1, self.retries + 1):
            try:
                with self._connect() as conn:
                    cur = conn.cursor()
                    cur.execute(sql, params)
                    conn.commit()
                return
            except sqlite3.OperationalError as e:
                msg = str(e).lower()
                if 'locked' in msg:
                    sleep_time = 0.05 * (2 ** (attempt - 1))
                    time.sleep(sleep_time)
                    continue
                else:
                    raise

        raise sqlite3.OperationalError(f'database is locked after {self.retries} retries')

    def fetch_recent(self, limit: int = 100) -> List[Dict[str, Any]]:
        sql = '''
            SELECT node_id, timestamp, temperature_celsius, humidity_percent,
                   luminosity_lux, presence_detected, power_on
            FROM sensor_data
            ORDER BY timestamp DESC
            LIMIT ?
        '''
        for attempt in range(1, self.retries + 1):
            try:
                with self._connect() as conn:
                    cur = conn.cursor()
                    cur.execute(sql, (limit,))
                    rows = cur.fetchall()
                out: List[Dict[str, Any]] = []
                for r in rows:
                    out.append({
                        'node_id': r[0],
                        'timestamp': r[1],
                        'sensors': {
                            'temperature_celsius': r[2],
                            'humidity_percent': r[3],
                            'luminosity_lux': r[4],
                            'presence_detected': bool(r[5]),
                            'power_on': bool(r[6])
                        }
                    })
                return out
            except sqlite3.OperationalError as e:
                msg = str(e).lower()
                if 'locked' in msg:
                    sleep_time = 0.02 * (2 ** (attempt - 1))
                    time.sleep(sleep_time)
                    continue
                else:
                    raise

        raise sqlite3.OperationalError('database is locked after retries')


class RequestHandler(SimpleHTTPRequestHandler):
    db_controller: DBController = None

    def do_POST(self) -> None:
        if self.path != '/data':
            self.send_response(404)
            self.end_headers()
            return

        try:
            length = int(self.headers.get('Content-Length', 0))
            raw = self.rfile.read(length)
            payload = json.loads(raw)
            print(payload)
            self.db_controller.save(payload)
            self.send_response(202)
            self.end_headers()
            self.wfile.write(b'Data accepted and stored.')
        except Exception as exc:
            self.send_response(500)
            self.end_headers()
            self.wfile.write(f'Error: {exc}'.encode())

    def do_GET(self) -> None:
        if self.path == '/':
            self.path = 'public/index.html'
            return SimpleHTTPRequestHandler.do_GET(self)

        if self.path == '/data':
            try:
                rows = self.db_controller.fetch_recent()
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps(rows).encode())
            except Exception as exc:
                self.send_response(500)
                self.end_headers()
                self.wfile.write(f'Error: {exc}'.encode())
            return

        super().do_GET()


if __name__ == '__main__':
    BASE_FOLDER = Path(__file__).resolve().parent
    DB_NAME = 'telemetry.db'
    DB_PATH = BASE_FOLDER / DB_NAME
    BIND_ADDR = '0.0.0.0'
    PORT = 8000

    db_controller = DBController(DB_PATH, timeout=30.0, retries=6)
    db_controller.initialize()

    RequestHandler.db_controller = db_controller

    os.chdir(BASE_FOLDER)

    # if great race for connections is expected, use ThreadingHTTPServer:
    # server = ThreadingHTTPServer((BIND_ADDR, PORT), RequestHandler)

    with TCPServer((BIND_ADDR, PORT), RequestHandler) as srv:
        print(f"Server running at http://{BIND_ADDR}:{PORT}")
        print(f"Open the dashboard at http://localhost:{PORT}/")
        try:
            srv.serve_forever()
        except KeyboardInterrupt:
            print('\nStopping server')
