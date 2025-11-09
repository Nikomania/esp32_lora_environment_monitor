# gateway/lora_serial_bridge.py
import json, socket, threading, time, struct
import serial  # pip install pyserial

SERVER_HOST = "127.0.0.1"   # onde roda seu server.py
SERVER_PORT = 5000          # porta que seu server.py já escuta
SERIAL_PORT = "COM5"        # ajuste p/ sua máquina (/dev/ttyUSB0 no Linux)
BAUD = 115200

# === Decodificador do payload binário de 16 bytes do colega ===
# Exemplo de layout: | u8 client_id | i16 temp_cx100 | u16 rh_x100 | i8 rssi | u32 ts | reserva... |
# ATENÇÃO: ajuste o 'struct.unpack' para o layout real do firmware.
def decode_lora_payload(pkt: bytes):
    # Exemplo de 1 byte + 2 + 2 + 1 + 4 + 6 = 16 (ajuste conforme necessário)
    client_id, t_raw, h_raw, rssi, ts = struct.unpack(">BhhbI", pkt[:10])
    temperature = t_raw / 100.0
    humidity = h_raw / 100.0
    return {
        "device_id": str(client_id),
        "temperature": temperature,
        "humidity": humidity,
        "rssi": int(rssi),
        "ts": int(ts) if ts else int(time.time() * 1000)
    }

def tx_to_server(sock, msg: dict):
    wire = (json.dumps(msg) + "\n").encode("utf-8")
    sock.sendall(wire)

def serial_reader():
    ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
    sock = socket.create_connection((SERVER_HOST, SERVER_PORT), timeout=5)
    buffer = b""
    try:
        while True:
            # Estratégia 1: firmware do gateway imprime cada pacote como linha HEX/base64
            line = ser.readline().strip()
            if not line:
                continue

            # Se o firmware mandar JSON por linha:
            # msg = json.loads(line.decode())
            # tx_to_server(sock, msg); continue

            # Se mandar binário como HEX por linha:
            try:
                raw = bytes.fromhex(line.decode())
            except Exception:
                # fallback: se vier CSV "id;temp;hum", parse aqui
                parts = line.decode().split(";")
                if len(parts) >= 3:
                    msg = {
                        "device_id": parts[0],
                        "temperature": float(parts[1]),
                        "humidity": float(parts[2]),
                        "ts": int(time.time()*1000)
                    }
                    tx_to_server(sock, msg)
                continue

            msg = decode_lora_payload(raw)
            tx_to_server(sock, msg)

    except KeyboardInterrupt:
        pass
    finally:
        try: sock.close()
        except: pass
        try: ser.close()
        except: pass

if __name__ == "__main__":
    # rode: python gateway/lora_serial_bridge.py
    serial_reader()
