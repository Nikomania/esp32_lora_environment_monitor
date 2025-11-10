/**
 * @file config.h
 * @brief Configurações do LoRa Gateway (ESP32-S3 XIAO + SX1262).
 *
 * Observações:
 * - Todos os valores podem ser sobrescritos via -D no platformio.ini (ex.: -DUSE_HTTP=false).
 * - Fluxo recomendado neste projeto: LoRa -> Serial -> (bridge Python) -> HTTP /data.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// Flags de build (podem ser sobrescritas por -DCHAVE=valor)
// ============================================================================

#ifndef USE_HTTP
  #define USE_HTTP false     // Gateway envia direto por HTTP? (normalmente NÃO — usamos o bridge)
#endif

#ifndef ENABLE_WIFI
  #define ENABLE_WIFI false  // Só precisa se USE_HTTP=true
#endif

#ifndef USE_SERIAL
  #define USE_SERIAL true    // Imprime JSON puro em uma linha para o bridge ler
#endif

#ifndef SERIAL_BAUD
  #define SERIAL_BAUD 115200
#endif

// Se quiser prefixar a linha serial (eu recomendo string vazia para JSON puro):
#ifndef SERIAL_PREFIX
  #define SERIAL_PREFIX ""   // "" => linha é exatamente o JSON
#endif

// ============================================================================
// Identidade do gateway e estatísticas
// ============================================================================
#ifndef GATEWAY_ID
  #define GATEWAY_ID 1
#endif

#ifndef MAX_PACKET_SIZE
  #define MAX_PACKET_SIZE 256
#endif

#ifndef STATS_INTERVAL_MS
  #define STATS_INTERVAL_MS 60000
#endif

// Modo de teste (injeta pacotes fake)
#ifndef TEST_MODE
  #define TEST_MODE true
#endif

#ifndef TEST_INTERVAL_MS
  #define TEST_INTERVAL_MS 15000
#endif

// ============================================================================
// LoRa SX1262 — XIAO ESP32-S3 + Wio SX1262 (pinos e parâmetros)
// ============================================================================
#ifndef LORA_MOSI
  #define LORA_MOSI 9
#endif
#ifndef LORA_MISO
  #define LORA_MISO 8
#endif
#ifndef LORA_SCK
  #define LORA_SCK  7
#endif
#ifndef LORA_NSS
  #define LORA_NSS  41
#endif
#ifndef LORA_RST
  #define LORA_RST  42
#endif
#ifndef LORA_DIO1
  #define LORA_DIO1 39
#endif
#ifndef LORA_BUSY
  #define LORA_BUSY 40
#endif

#ifndef LORA_FREQUENCY_MHZ
  #define LORA_FREQUENCY_MHZ 915.0   // 915 Américas | 868 EU | 433 Ásia
#endif
#ifndef LORA_BW_KHZ
  #define LORA_BW_KHZ 125.0
#endif
#ifndef LORA_SF
  #define LORA_SF 9                 // 7..12
#endif
#ifndef LORA_CR
  #define LORA_CR 7                 // 5..8 (conforme RadioLib)
#endif
#ifndef LORA_SYNC_WORD
  #define LORA_SYNC_WORD 0x12
#endif
#ifndef LORA_PREAMBLE
  #define LORA_PREAMBLE 8
#endif

// ============================================================================
// (Opcional) HTTP: só use se for enviar direto ao servidor (sem bridge).
// Recomendo manter desativado neste projeto.
// ============================================================================
#ifndef SERVER_HOST
  #define SERVER_HOST "127.0.0.1"
#endif
#ifndef SERVER_PORT
  #define SERVER_PORT 8000
#endif
#ifndef SERVER_PATH
  #define SERVER_PATH "/data"
#endif

// ============================================================================
// (Opcional) Wi-Fi: só precisa se USE_HTTP=true
// ============================================================================
#ifndef WIFI_SSID
  #define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
  #define WIFI_PASSWORD ""
#endif
#ifndef WIFI_TIMEOUT_MS
  #define WIFI_TIMEOUT_MS 10000
#endif

// ============================================================================
// Namespaces (apenas leitura) — evitam macros no código-fonte
// ============================================================================
namespace LinkCfg {
  constexpr uint8_t  kMosi = LORA_MOSI;
  constexpr uint8_t  kMiso = LORA_MISO;
  constexpr uint8_t  kSck  = LORA_SCK;
  constexpr uint8_t  kNss  = LORA_NSS;
  constexpr uint8_t  kRst  = LORA_RST;
  constexpr uint8_t  kDio1 = LORA_DIO1;
  constexpr uint8_t  kBusy = LORA_BUSY;
  constexpr float    kFreqMHz  = LORA_FREQUENCY_MHZ;
  constexpr float    kBwKHz    = LORA_BW_KHZ;
  constexpr uint8_t  kSf       = LORA_SF;
  constexpr uint8_t  kCr       = LORA_CR;
  constexpr uint8_t  kSyncWord = LORA_SYNC_WORD;
  constexpr uint16_t kPreamble = LORA_PREAMBLE;
}

namespace GwCfg {
  constexpr uint8_t   kGatewayId   = GATEWAY_ID;
  constexpr uint32_t  kStatsEveryMs= STATS_INTERVAL_MS;
  constexpr uint16_t  kMaxPkt      = MAX_PACKET_SIZE;
  constexpr bool      kTestMode    = TEST_MODE;
  constexpr uint32_t  kTestEveryMs = TEST_INTERVAL_MS;
}

namespace IoCfg {
  constexpr bool     kUseSerial = USE_SERIAL;
  constexpr uint32_t kSerialBaud= SERIAL_BAUD;
  // prefixo para a linha — mantenha "" para o bridge ler JSON puro
  inline const char*  Prefix() { return SERIAL_PREFIX; }
}

namespace NetCfg {
  constexpr bool      kUseHttp     = USE_HTTP;
  constexpr bool      kWifiEnabled = ENABLE_WIFI;
  inline const char*  Host() { return SERVER_HOST; }
  constexpr uint16_t  Port        = SERVER_PORT;
  inline const char*  Path() { return SERVER_PATH; }
  inline const char*  WifiSsid() { return WIFI_SSID; }
  inline const char*  WifiPass() { return WIFI_PASSWORD; }
  constexpr uint32_t  WifiTimeoutMs = WIFI_TIMEOUT_MS;
}

namespace DebugCfg {
  constexpr bool     kDebug = true;
  constexpr uint32_t kBaud  = SERIAL_BAUD;
}

#endif // CONFIG_H
