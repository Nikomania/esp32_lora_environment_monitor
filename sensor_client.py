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