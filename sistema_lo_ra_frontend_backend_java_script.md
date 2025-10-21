# Sistema de Monitoramento via LoRa — Frontend & Backend (JavaScript)

Este documento contém um projeto mínimo funcional para **backend (Node.js + SQLite)** e **frontend (HTML/CSS/JS)** que define as interfaces e payloads esperados do gateway/ESP32 via LoRa. Foi pensado para cumprir as especificações do trabalho: nós sensores enviam payloads compactos, gateway encaminha por HTTP, servidor decodifica e registra, dashboard mostra dados.

---

## Estrutura de arquivos (proposta)

```
project/
├─ package.json
├─ server.js               # backend Express + SQLite
├─ README.md
└─ public/
   ├─ index.html
   ├─ app.js
   └─ style.css
```

---

## Especificação de payload (compacto — 8 bytes)

Formato (ordem e tamanho):

- `node_id` — 1 byte unsigned (0..255)
- `seq` — 1 byte unsigned (contador de pacotes)
- `temp` — 2 bytes signed (int16) — temperatura em **centésimos de °C** (ex.: 2350 = 23.50°C)
- `hum` — 1 byte unsigned — umidade relativa em % (0..100)
- `dust` — 2 bytes unsigned (uint16) — concentração de poeira em µg/m³ (valor inteiro)
- `batt` — 1 byte unsigned — nível da bateria em % (0..100)

Total: **8 bytes**.

Exemplo: `Buffer.from([0x01, 0x05, 0x09, 0x2E, 0x40, 0x01, 0x2C, 0x64])`.

Motivação: payload binário pequeno (sem JSON) para minimizar airtime.

---

## API do servidor

### `POST /gateway` (Ingestão do gateway)

Recebe `application/json` com corpo mínimo:

```json
{
  "payload_base64": "...",
  "gateway_id": "gw01",
  "rssi": -42,
  "snr": 7.5,
  "room": "Sala A",
  "timestamp": "2025-10-20T20:00:00Z"
}
```

O servidor: decodifica `payload_base64`, interpreta os 8 bytes, persiste no banco.

### `GET /api/latest`

Retorna os últimos valores por nó/room (JSON) para o dashboard.

### `GET /api/telemetry?room=Sala%20A&limit=100`

Retorna histórico para gráfico.

---

## Banco de dados

Usamos SQLite (arquivo `telemetry.db`) com tabela `telemetry`:

- id (integer pk autoincrement)
- ts (timestamp)
- node_id (integer)
- seq (integer)
- temp_c (real) — temperatura em °C
- hum (integer)
- dust (integer)
- batt (integer)
- rssi (real)
- snr (real)
- room (text)
- raw_base64 (text)

---

## Exemplo de payload montagem (ESP32 / Arduino C++)

```cpp
// pseudocódigo: monte um array de 8 bytes conforme especificação e envie via LoRa
uint8_t payload[8];
uint8_t node_id = 1;
uint8_t seq = 5;
int16_t temp_centi = (int16_t)round(23.57 * 100); // 2357
uint8_t hum = 45;
uint16_t dust = 300; // ug/m3
uint8_t batt = 98;

payload[0] = node_id;
payload[1] = seq;
payload[2] = (uint8_t)((temp_centi >> 8) & 0xFF);
payload[3] = (uint8_t)(temp_centi & 0xFF);
payload[4] = hum;
payload[5] = (uint8_t)((dust >> 8) & 0xFF);
payload[6] = (uint8_t)(dust & 0xFF);
payload[7] = batt;

// enviar payload via LoRa (biblioteca do módulo)
```

O **gateway** deve receber esse payload LoRa, e então fazer um `POST` HTTP para o servidor com o `payload_base64` e metadados (rssi, snr, room, timestamp). Exemplo de corpo já mostrado na seção de API.

---

## Observações e próximos passos

- O projeto entregue aqui é um esqueleto: pronto para testes locais e demonstração de ponta-a-ponta.
- Para produção/entrega do trabalho, adicione: autenticação leve, paginação, filtros por nó, alertas por limite, gráficos para poeira/bateria, e investigação experimental (RTO, perdas, consumo de energia).
- Se quiser, eu adapto para usar MQTT em vez de HTTP, ou crio um exemplo de gateway em Node/LoRa (se você já tiver o hardware e a biblioteca do gateway).

---

_Fim do documento._
