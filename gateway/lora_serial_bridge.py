import json, time, struct
import serial          # pip install pyserial
import requests        # pip install requests

# === CONFIGURAÇÕES ===
SERIAL_PORT = "COM5"         # Windows: COMx | Linux: /dev/ttyUSB0 ou /dev/ttyACM0
BAUD = 115200
SERVER_URL = "http://127.0.0.1:8000/data"   # seu server.py: POST /data

# --- ajuste para o layout real do payload binário vindo do firmware ---
# Exemplo genérico: | u8 client_id | i16 temp_x100 | u16 hum_x100 | i8 rssi | u32 ts |
def decode_lora_payload(pkt: bytes):
    # Ajuste o unpack conforme o seu protocol.h
    client_id, t_raw, h_raw, rssi, ts = struct.unpack(">BhhbI", pkt[:10])
    temperature = t_raw / 100.0
    humidity = h_raw / 100.0
    return {
        "device_id": str(client_id),
        "temperature": temperature,
        "humidity": humidity,
        "rssi": int(rssi),
        "ts": int(ts) if ts else int(time.time()*1000)
    }

def build_payload_for_server(decoded: dict) -> dict:
    """
    Transforma o que veio do LoRa no JSON que seu server.py espera.
    server.py -> DBController.save(payload) espera:
    {
      "node_id": str,
      "timestamp": str,   # texto
      "sensors": {
        "temperature_celsius": float|None,
        "humidity_percent": float|None,
        "luminosity_lux": float|None,
        "presence_detected": bool,
        "power_on": bool
      }
    }
    """
    iso_ts = time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime(decoded.get("ts", time.time()*1000)/1000))
    return {
        "node_id": decoded.get("device_id", "unknown"),
        "timestamp": iso_ts,
        "sensors": {
            "temperature_celsius": decoded.get("temperature"),
            "humidity_percent": decoded.get("humidity"),
            "luminosity_lux": None,         # opcional, pode preencher se tiver
            "presence_detected": False,     # ajuste se seu firmware enviar isso
            "power_on": True                # ajuste se seu firmware enviar isso
        }
    }

def post_to_server(msg: dict):
    r = requests.post(SERVER_URL, json=msg, timeout=5)
    r.raise_for_status()

def main():
    print(f"Abrindo serial {SERIAL_PORT} @ {BAUD}")
    ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
    try:
        while True:
            line = ser.readline().strip()
            if not line:
                continue

            # 1) Se o firmware do gateway imprimir JSON por linha:
            try:
                as_json = json.loads(line.decode())
                decoded = {
                    "device_id": str(as_json.get("device_id", "unknown")),
                    "temperature": float(as_json.get("temperature", 0.0)),
                    "humidity": float(as_json.get("humidity", 0.0)),
                    "ts": int(as_json.get("ts", int(time.time()*1000)))
                }
                payload = build_payload_for_server(decoded)
                post_to_server(payload)
                continue
            except Exception:
                pass

            # 2) Se o firmware imprimir HEX por linha (binário codificado):
            try:
                raw = bytes.fromhex(line.decode())
                decoded = decode_lora_payload(raw)
                payload = build_payload_for_server(decoded)
                post_to_server(payload)
                continue
            except Exception:
                pass

            # 3) Fallback CSV: "id;temp;hum"
            try:
                parts = line.decode().split(";")
                if len(parts) >= 3:
                    decoded = {
                        "device_id": parts[0],
                        "temperature": float(parts[1]),
                        "humidity": float(parts[2]),
                        "ts": int(time.time()*1000)
                    }
                    payload = build_payload_for_server(decoded)
                    post_to_server(payload)
                    continue
            except Exception:
                pass

            # Se chegou aqui, a linha não casou com nenhum formato esperado
            print("Linha ignorada:", line[:80])
    except KeyboardInterrupt:
        pass
    finally:
        try: ser.close()
        except: pass

if __name__ == "__main__":
    main()
