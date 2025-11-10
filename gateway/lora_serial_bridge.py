import json
import time
import serial
import requests

# =====================================================
# CONFIGURAÇÕES
# =====================================================
SERIAL_PORT = "/dev/ttyUSB0"   # Linux: /dev/ttyUSBx ou /dev/ttyACMx | Windows: COMx
BAUD = 115200
SERVER_URL = "http://127.0.0.1:8000/data"   # Endpoint do server.py

# =====================================================
# FUNÇÕES AUXILIARES
# =====================================================

def post_to_server(msg: dict):
    """Envia o JSON recebido do gateway para o servidor via HTTP POST."""
    try:
        r = requests.post(SERVER_URL, json=msg, timeout=5)
        r.raise_for_status()
        print(f"[OK] Enviado para o servidor ({r.status_code})")
    except requests.RequestException as e:
        print(f"[ERRO HTTP] {e}")

# =====================================================
# LOOP PRINCIPAL
# =====================================================

def main():
    print(f"\n[Bridge] Lendo Serial {SERIAL_PORT} @ {BAUD}")
    print("[Bridge] Enviando dados para:", SERVER_URL)
    print("-------------------------------------------")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"[ERRO] Não foi possível abrir {SERIAL_PORT}: {e}")
        return

    try:
        while True:
            line = ser.readline().strip()
            if not line:
                continue

            try:
                decoded = json.loads(line.decode("utf-8"))
                # Se o gateway imprimir JSON no formato completo, basta enviar
                post_to_server(decoded)
                print("[Bridge] Linha JSON encaminhada com sucesso.")
            except json.JSONDecodeError:
                # Ignorar linhas de debug do gateway
                text = line.decode(errors="ignore")
                if not text.startswith("{"):
                    print("[DBG]", text)
                else:
                    print("[ERRO] JSON malformado:", text[:100])
                continue

    except KeyboardInterrupt:
        print("\n[Bridge] Encerrando...")
    finally:
        ser.close()

# =====================================================
# ENTRADA
# =====================================================

if __name__ == "__main__":
    main()
