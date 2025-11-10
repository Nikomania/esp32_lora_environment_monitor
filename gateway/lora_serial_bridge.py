import json
import time
import sys
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
        
def run_from_stdin():
    print("[Bridge] Lendo do STDIN (pipe). Enviando para:", SERVER_URL)
    for line in sys.stdin.buffer:
        line = line.strip()
        if not line:
            continue
        try:
            payload = json.loads(line.decode("utf-8", errors="ignore"))
            post_to_server(payload)
            print("[Bridge] Linha JSON encaminhada.")
        except json.JSONDecodeError:
            print("[DBG] ignorado (não é JSON):", line[:80])
            
def run_from_serial(port: str, baud: int = 115200):
    print(f"[Bridge] Lendo Serial {port} @ {baud}")
    print("[Bridge] Enviando dados para:", SERVER_URL)
    print("-------------------------------------------")
    try:
        ser = serial.Serial(port, baud, timeout=1)
    except serial.SerialException as e:
        print(f"[ERRO] Não foi possível abrir {port}: {e}")
        print("[DICA] Use --stdin para ler via pipe.")
        return

    try:
        while True:
            line = ser.readline().strip()
            if not line:
                continue
            try:
                payload = json.loads(line.decode("utf-8", errors="ignore"))
                post_to_server(payload)
                print("[Bridge] Linha JSON encaminhada.")
            except json.JSONDecodeError:
                txt = line.decode(errors="ignore")
                if not txt.startswith("{"):
                    print("[DBG]", txt)
                else:
                    print("[ERRO] JSON malformado:", txt[:120])
    except KeyboardInterrupt:
        pass
    finally:
        try: ser.close()
        except: pass

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
    # Uso:
    #   python lora_serial_bridge.py --stdin
    #   python lora_serial_bridge.py --port /dev/ttyACM0 --baud 115200
    if "--stdin" in sys.argv:
        run_from_stdin()
    else:
        port = "/dev/ttyUSB0"
        if "--port" in sys.argv:
            i = sys.argv.index("--port")
            if i + 1 < len(sys.argv):
                port = sys.argv[i+1]
        baud = 115200
        if "--baud" in sys.argv:
            i = sys.argv.index("--baud")
            if i + 1 < len(sys.argv):
                baud = int(sys.argv[i+1])
        run_from_serial(port, baud)
