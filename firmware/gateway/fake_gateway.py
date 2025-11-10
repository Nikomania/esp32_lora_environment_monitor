import time, json, random

while True:
    data = {
        "node_id": "1",
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "sensors": {
            "temperature_celsius": round(random.uniform(24, 28), 2),
            "humidity_percent": round(random.uniform(55, 65), 2),
            "luminosity_lux": None,
            "presence_detected": random.choice([True, False]),
            "power_on": True
        }
    }
    print(json.dumps(data))
    time.sleep(10)
