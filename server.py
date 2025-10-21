# server.py
from http.server import SimpleHTTPRequestHandler
import socketserver
import json
import sqlite3
from pathlib import Path
from typing import Any, Dict, List

BASE_DIR = Path(__file__).resolve().parent
DB_NAME = 'telemetry.db'
DB_PATH = BASE_DIR / DB_NAME
BIND_ADDR = '0.0.0.0'
PORT = 8000


class MonitoringController:
    """Controller that encapsulates database operations."""
    def __init__(self, db_path: Path):
        self.db_path = db_path

    def initialize(self) -> None:
        conn = sqlite3.connect(self.db_path)
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
        conn.commit()
        conn.close()

    def save(self, payload: Dict[str, Any]) -> None:
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        sensors = payload.get('sensors', {})
        cur.execute('''
            INSERT INTO sensor_data (
                node_id, timestamp, temperature_celsius, humidity_percent,
                luminosity_lux, presence_detected, power_on
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            payload.get('node_id'), payload.get('timestamp'),
            sensors.get('temperature_celsius'), sensors.get('humidity_percent'),
            sensors.get('luminosity_lux'), sensors.get('presence_detected'),
            sensors.get('power_on')
        ))
        conn.commit()
        conn.close()

    def fetch_recent(self, limit: int = 100) -> List[Dict[str, Any]]:
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute('''
            SELECT node_id, timestamp, temperature_celsius, humidity_percent,
                   luminosity_lux, presence_detected, power_on
            FROM sensor_data
            ORDER BY timestamp DESC
            LIMIT ?
        ''', (limit,))
        rows = cur.fetchall()
        conn.close()

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


class RequestHandler(SimpleHTTPRequestHandler):
    controller: MonitoringController = None  # type: ignore

    def do_POST(self) -> None:
        if self.path != '/data':
            self.send_response(404)
            self.end_headers()
            return

        try:
            length = int(self.headers.get('Content-Length', 0))
            raw = self.rfile.read(length)
            payload = json.loads(raw)
            self.controller.save(payload)
            self.send_response(202)
            self.end_headers()
            self.wfile.write(b'Data accepted and stored.')
        except Exception as exc:
            self.send_response(500)
            self.end_headers()
            self.wfile.write(f'Error: {exc}'.encode())

    def do_GET(self) -> None:
        if self.path == '/':
            self.path = '/index.html'
            return SimpleHTTPRequestHandler.do_GET(self)

        if self.path == '/data':
            try:
                rows = self.controller.fetch_recent()
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


def start_server() -> None:
    controller = MonitoringController(DB_PATH)
    controller.initialize()

    RequestHandler.controller = controller

    # serve files from project directory
    import os
    os.chdir(BASE_DIR)

    with socketserver.TCPServer((BIND_ADDR, PORT), RequestHandler) as srv:
        print(f"Server running at http://{BIND_ADDR}:{PORT}")
        print(f"Open the dashboard at http://localhost:{PORT}/")
        try:
            srv.serve_forever()
        except KeyboardInterrupt:
            print('\nStopping server')


if __name__ == '__main__':
    start_server()