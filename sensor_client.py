import json
import random
import time
from datetime import datetime, timezone
from typing import Any, Dict

import requests

class SensorClient:
    def __init__(self, api_endpoint: str, device_id: str):
        self.api_endpoint = api_endpoint
        self.device_id = device_id

    def gen_payload(self) -> Dict[str, Any]:
        ts: str = datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
        return {
            "node_id": self.device_id,
            "timestamp": ts,
            "sensors": {
                "temperature_celsius": round(random.uniform(30.0, 50.0), 2),
                "humidity_percent": round(random.uniform(30.0, 60.0), 2),
                "luminosity_lux": round(random.uniform(5.0, 50.0), 2),
                "presence_detected": random.choices([True, False], weights=[5, 95], k=1)[0],
                "power_on": random.choices([True, False], weights=[999, 1], k=1)[0],
            },
        }


    def send(self, payload: Dict[str, Any]) -> None:
        headers = {"Content-Type": "application/json"}
        try:
            requests.post(self.api_endpoint, data=json.dumps(payload), headers=headers, timeout=5)
        except requests.exceptions.RequestException:
            pass
        
    def run_test(self, debug=False) -> None:
        p = self.gen_payload()
        self.send(p)
        if debug:
            print(p)


if __name__ == '__main__':
    API_ENDPOINT: str = "http://localhost:8000/data"
    DEVICE_ID: str = "rack-servers-01"
    INTERVAL_SEC: int = 10
    
    client = SensorClient(API_ENDPOINT, DEVICE_ID)
    
    while True:
        try:
            client.run_test(debug=True)
            print("Payload sent.")
            time.sleep(INTERVAL_SEC)
        except KeyboardInterrupt:
            break