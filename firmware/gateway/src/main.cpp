/**
 * @file main.cpp
 * @brief LoRa Gateway Node - Ponte LoRa → Serial → (Python Bridge → Server)
 *
 * @details
 * - Recebe pacotes binários LoRa de múltiplos nós sensores (clients)
 * - Valida e converte para JSON
 * - Envia pela porta serial (para o script Python lora_serial_bridge.py)
 * - Opcionalmente envia por HTTP direto (desativado por padrão)
 * - Modo debug detalhado exibe bytes, checksum, RSSI e SNR
 */

#include "config.h"
#include "protocol.h"

#include <Arduino.h>
#include <RadioLib.h>
#if ENABLE_WIFI
  #include <WiFi.h>
#endif
#if USE_HTTP
  #include <HTTPClient.h>
#endif

// =====================================================
// Instâncias globais e estado
// =====================================================

SX1262 radio = new Module(
    LinkCfg::kNss,
    LinkCfg::kDio1,
    LinkCfg::kRst,
    LinkCfg::kBusy
);

uint32_t packets_ok       = 0;
uint32_t packets_invalid  = 0;
uint32_t packets_checksum = 0;
uint32_t last_stat_time   = 0;

bool lora_ready = false;

// =====================================================
// Funções auxiliares
// =====================================================

void setup_lora();
void setup_wifi();
void print_stats();
void process_packet(uint8_t* buf, size_t len);
void send_json(const String& json_line);
String packet_to_json(const SensorDataMessage& msg);
void print_hex(const uint8_t* data, size_t len);

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(IoCfg::kSerialBaud);
  delay(500);
  Serial.println("\n===============================");
  Serial.println("LoRa Gateway - ESP32-S3 XIAO");
  Serial.println("===============================");

#if ENABLE_WIFI
  setup_wifi();
#endif

  setup_lora();

  if (!lora_ready) {
    Serial.println("LoRa init failed, switching to passive mode.");
  }
}


// =====================================================
// Loop
// =====================================================

void loop() {
#if TEST_MODE
  static unsigned long last = 0;
  if (millis() - last > GwCfg::kTestEveryMs) {
      last = millis();

      // Pacote simulado
      SensorDataMessage msg{};
      msg.msg_type   = MSG_TYPE_SENSOR_DATA;
      msg.client_id  = 1;
      msg.timestamp  = millis();
      msg.temperature= encode_temperature(25.4f);
      msg.humidity   = encode_humidity(58.3f);
      msg.distance_cm= 90;
      msg.battery    = 97;
      msg.checksum   = calculate_checksum((uint8_t*)&msg, sizeof(msg));

      String json_line = packet_to_json(msg);
      Serial.println(json_line); // gateway envia o JSON simulado
  }
#else
  if (!lora_ready) {
    delay(2000);
    return;
  }

  uint8_t buf[GwCfg::kMaxPkt] = {0};
  int len = radio.receive(buf, sizeof(buf));

  if (len > 0) {
    process_packet(buf, len);
  }

  // Estatísticas periódicas
  if (millis() - last_stat_time > GwCfg::kStatsEveryMs) {
    print_stats();
    last_stat_time = millis();
  }
#endif
}


// =====================================================
// LoRa setup
// =====================================================

void setup_lora() {
  Serial.println("\n[LoRa Setup]");
  Serial.printf("  MOSI:%d  MISO:%d  SCK:%d  NSS:%d\n",
      LinkCfg::kMosi, LinkCfg::kMiso, LinkCfg::kSck, LinkCfg::kNss);
  Serial.printf("  DIO1:%d  RST:%d  BUSY:%d\n",
      LinkCfg::kDio1, LinkCfg::kRst, LinkCfg::kBusy);

  pinMode(LinkCfg::kRst, OUTPUT);
  digitalWrite(LinkCfg::kRst, LOW);
  delay(10);
  digitalWrite(LinkCfg::kRst, HIGH);
  delay(10);

  SPI.begin(LinkCfg::kSck, LinkCfg::kMiso, LinkCfg::kMosi, LinkCfg::kNss);
  delay(50);

  int state = radio.begin(
      LinkCfg::kFreqMHz,
      LinkCfg::kBwKHz,
      LinkCfg::kSf,
      LinkCfg::kCr,
      LinkCfg::kSyncWord,
      14,
      LinkCfg::kPreamble
  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ SX1262 iniciado com sucesso!");
    lora_ready = true;
  } else {
    Serial.printf("✗ Falha na inicialização LoRa (erro %d)\n", state);
    lora_ready = false;
  }
}

// =====================================================
// Wi-Fi (opcional)
// =====================================================

void setup_wifi() {
#if ENABLE_WIFI
  if (!NetCfg::kWifiEnabled) return;

  Serial.printf("\n[Wi-Fi] Conectando a %s ...\n", NetCfg::WifiSsid());
  WiFi.begin(NetCfg::WifiSsid(), NetCfg::WifiPass());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < NetCfg::WifiTimeoutMs) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[Wi-Fi] Conectado, IP: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[Wi-Fi] Falha ao conectar, modo offline");
  }
#endif
}

// =====================================================
// Processamento de pacotes
// =====================================================

void process_packet(uint8_t* buf, size_t len) {
  Serial.println("\n[LoRa] Pacote recebido!");

  // Mostrar RSSI/SNR
  float rssi = radio.getRSSI();
  float snr  = radio.getSNR();
  Serial.printf("  RSSI: %.1f dBm | SNR: %.1f dB | Len: %d bytes\n", rssi, snr, len);

  // Mostrar bytes em HEX
  Serial.print("  Data HEX: ");
  print_hex(buf, len);
  Serial.println();

  if (len < sizeof(SensorDataMessage)) {
    packets_invalid++;
    Serial.println("  ⚠ Pacote muito pequeno, ignorado.");
    return;
  }

  SensorDataMessage msg;
  memcpy(&msg, buf, sizeof(msg));

  // Validar checksum
  uint8_t expected = calculate_checksum((uint8_t*)&msg, sizeof(msg));
  bool ok = verify_checksum((uint8_t*)&msg, sizeof(msg));

  if (!ok) {
    packets_checksum++;
    Serial.printf("  ⚠ Checksum inválido. Recebido: 0x%02X, Esperado: 0x%02X\n",
                  msg.checksum, expected);
    return;
  }

  // Exibir conteúdo decodificado
  Serial.printf("  ✓ Client ID: %u\n", msg.client_id);
  Serial.printf("  ✓ Temp: %.2f °C\n", decode_temperature(msg.temperature));
  Serial.printf("  ✓ Humid: %.2f %%\n", decode_humidity(msg.humidity));
  Serial.printf("  ✓ Dist: %u cm\n", msg.distance_cm);
  Serial.printf("  ✓ Batt: %u %%\n", msg.battery);

  String json_line = packet_to_json(msg);
  send_json(json_line);
  packets_ok++;
}

// =====================================================
// Conversão para JSON
// =====================================================

String packet_to_json(const SensorDataMessage& msg) {
  char time_buf[32];
  unsigned long ts_ms = msg.timestamp;
  time_t sec = ts_ms / 1000;
  struct tm t;
  gmtime_r(&sec, &t);
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &t);

  bool presence = msg.distance_cm < 100;

  String json = "{";
  json += "\"node_id\":\"" + String(msg.client_id) + "\",";
  json += "\"timestamp\":\"" + String(time_buf) + "\",";
  json += "\"sensors\":{";
  json += "\"temperature_celsius\":" + String(decode_temperature(msg.temperature), 2) + ",";
  json += "\"humidity_percent\":" + String(decode_humidity(msg.humidity), 2) + ",";
  json += "\"luminosity_lux\":null,";
  json += "\"presence_detected\":" + String(presence ? "true" : "false") + ",";
  json += "\"power_on\":true";
  json += "}}";
  return json;
}

// =====================================================
// Saída (Serial ou HTTP)
// =====================================================

void send_json(const String& json_line) {
#if USE_HTTP
  if (NetCfg::kUseHttp && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("http://") + NetCfg::Host() + ":" + NetCfg::Port + NetCfg::Path();
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json_line);
    Serial.printf("[HTTP] POST %s (%d bytes) → code %d\n",
                  url.c_str(), json_line.length(), code);
    http.end();
  } else
#endif
  if (IoCfg::kUseSerial) {
    // Linha JSON pura — o bridge Python lê exatamente isso
    Serial.print(IoCfg::Prefix());
    Serial.println(json_line);
  }
}

// =====================================================
// Utilitários e estatísticas
// =====================================================

void print_hex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (i > 0) Serial.print(" ");
    Serial.printf("%02X", data[i]);
  }
}

void print_stats() {
  Serial.printf("\n--- Gateway Stats ---\n");
  Serial.printf("  Packets OK:       %lu\n", packets_ok);
  Serial.printf("  Invalid length:   %lu\n", packets_invalid);
  Serial.printf("  Bad checksum:     %lu\n", packets_checksum);
  Serial.printf("  RSSI last: %.1f dBm  SNR last: %.1f dB\n",
                radio.getRSSI(), radio.getSNR());
  Serial.println("----------------------");
}
