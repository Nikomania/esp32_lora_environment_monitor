# Refactored project — HTML, Server (controller), Client mock

Below are the three refactored files. All variable names and comments were translated to English and simplified. Styling in the dashboard has been removed (no colors) and retains only functional layout (table). The server was refactored to a controller-style structure while preserving the same endpoints and functionality. The client mock is adjusted with similar variable names.

---

## File: `dashboard.html`

```html
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Monitoring Dashboard</title>
    <style>
      /* Minimal, neutral styling for functional table layout */
      html,
      body {
        margin: 0;
        padding: 0;
        font-family: system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue",
          Arial;
        color: #000;
        background: #fff;
      }
      .wrap {
        max-width: 1100px;
        margin: 24px auto;
        padding: 16px;
      }
      header {
        margin-bottom: 12px;
      }
      h1 {
        margin: 0 0 4px 0;
        font-size: 20px;
      }
      p {
        margin: 0;
        font-size: 13px;
        color: #222;
      }

      .card {
        border: 1px solid #ccc;
        border-radius: 6px;
        overflow: hidden;
      }
      .table-container {
        width: 100%;
        overflow-x: auto;
      }
      table {
        width: 100%;
        border-collapse: collapse;
      }
      thead th {
        text-align: left;
        padding: 10px;
        border-bottom: 1px solid #ccc;
        font-weight: 600;
      }
      tbody td {
        padding: 10px;
        border-bottom: 1px solid #e6e6e6;
      }
      tr:hover td {
        background: #fafafa;
      }
      .center {
        text-align: center;
      }
      .mono {
        font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, "Roboto Mono",
          monospace;
      }
      footer {
        padding: 10px;
        font-size: 12px;
        color: #333;
        border-top: 1px solid #eee;
      }

      /* No color semantics applied. Keep only structural classes. */
    </style>
  </head>
  <body>
    <div class="wrap">
      <header>
        <h1>Monitoring Dashboard</h1>
        <p>Live sensor data from the server room</p>
      </header>

      <div class="card">
        <div class="table-container">
          <table>
            <thead>
              <tr>
                <th>Sensor Node (ID)</th>
                <th>Timestamp</th>
                <th class="center">Temperature</th>
                <th class="center">Humidity</th>
                <th class="center">Luminosity</th>
                <th class="center">Presence</th>
                <th class="center">Power</th>
              </tr>
            </thead>
            <tbody id="table-body">
              <tr>
                <td colspan="7" class="center mono">Waiting for data...</td>
              </tr>
            </tbody>
          </table>
        </div>
        <footer>
          Refreshing every 5 seconds. Last update:
          <span id="last-update">Never</span>
        </footer>
      </div>
    </div>

    <script>
      // Minimal, clear names. Comments kept short.
      const DATA_ENDPOINT = "/data";
      const POLL_INTERVAL = 5000; // ms

      function classifyTemperature(value) {
        if (value == null || Number.isNaN(value)) return "normal";
        if (value >= 45.0) return "critical";
        if (value >= 38.0) return "hot";
        return "normal";
      }

      async function fetchAndRender() {
        try {
          const res = await fetch(DATA_ENDPOINT);
          if (!res.ok) throw new Error(`status ${res.status}`);
          const payload = await res.json();
          renderTable(payload);
        } catch (err) {
          const el = document.getElementById("table-body");
          el.innerHTML = `<tr><td colspan="7" class="center mono">Failed to load data. Is the server running?</td></tr>`;
          console.error("Fetch error:", err);
        } finally {
          document.getElementById("last-update").textContent =
            new Date().toLocaleTimeString();
        }
      }

      function renderTable(list) {
        const body = document.getElementById("table-body");
        if (!Array.isArray(list) || list.length === 0) {
          body.innerHTML = `<tr><td colspan="7" class="center mono">No data received yet...</td></tr>`;
          return;
        }

        body.innerHTML = "";

        list.forEach((entry) => {
          const node = entry.node_id ?? "N/A";
          const ts = entry.timestamp
            ? new Date(entry.timestamp).toLocaleString()
            : "N/A";
          const s = entry.sensors || {};

          const tempVal =
            typeof s.temperature_celsius === "number"
              ? s.temperature_celsius.toFixed(2) + " °C"
              : "N/A";
          const humVal =
            typeof s.humidity_percent === "number"
              ? s.humidity_percent.toFixed(2) + " %"
              : "N/A";
          const lumVal =
            typeof s.luminosity_lux === "number"
              ? s.luminosity_lux.toFixed(2) + " lux"
              : "N/A";
          const presenceVal = s.presence_detected ? "Detected" : "No";
          const powerVal = s.power_on ? "ON" : "OFF";

          const tr = document.createElement("tr");
          tr.innerHTML = `
                    <td class="mono">${node}</td>
                    <td>${ts}</td>
                    <td class="center">${tempVal}</td>
                    <td class="center">${humVal}</td>
                    <td class="center">${lumVal}</td>
                    <td class="center">${presenceVal}</td>
                    <td class="center">${powerVal}</td>
                `;
          body.appendChild(tr);
        });
      }

      document.addEventListener("DOMContentLoaded", () => {
        fetchAndRender();
        setInterval(fetchAndRender, POLL_INTERVAL);
      });
    </script>
  </body>
</html>
```

---

## File: `server.py` (controller-style)

```python
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
            self.path = '/dashboard.html'
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
```

---

## File: `sensor_client.py` (small renames)

```python
# sensor_client.py
import json
import random
import time
from datetime import datetime, timezone
from typing import Any, Dict

import requests

API_ENDPOINT: str = "http://localhost:8000/data"
DEVICE_ID: str = "rack-servers-01"
INTERVAL_SEC: int = 10


def gen_payload(device_id: str) -> Dict[str, Any]:
    ts: str = datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
    return {
        "node_id": device_id,
        "timestamp": ts,
        "sensors": {
            "temperature_celsius": round(random.uniform(30.0, 50.0), 2),
            "humidity_percent": round(random.uniform(30.0, 60.0), 2),
            "luminosity_lux": round(random.uniform(5.0, 50.0), 2),
            "presence_detected": random.choices([True, False], weights=[5, 95], k=1)[0],
            "power_on": random.choices([True, False], weights=[999, 1], k=1)[0],
        },
    }


def send(payload: Dict[str, Any]) -> None:
    headers = {"Content-Type": "application/json"}
    try:
        requests.post(API_ENDPOINT, data=json.dumps(payload), headers=headers, timeout=5)
    except requests.exceptions.RequestException:
        pass


if __name__ == '__main__':
    while True:
        try:
            p = gen_payload(DEVICE_ID)
            send(p)
            time.sleep(INTERVAL_SEC)
        except KeyboardInterrupt:
            break
```

---

### Notes

- Files should be placed in the same directory. Run `python server.py` then open `http://localhost:8000`.
- I kept comments short and in English, and renamed variables to similar readable names.
- If you want, I can also split these into separate files in a zip or produce a single combined file — tell me which format you prefer.
