/**
 * @file config.h
 * @brief Configurações do nó cliente (ESP32-S3 + SX1262) para envio binário (protocol.h).
 *
 * Observações:
 * - Todos os valores podem ser sobrescritos por -D no platformio.ini (ex.: -DCLIENT_ID=2).
 * - Mantido o mapeamento de pinos para XIAO ESP32-S3 + Wio SX1262.
 * - Layout de payload definido em protocol.h (little-endian, 16 bytes).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// Seção: Defaults sobrescrevíveis (permitem -DCHAVE=valor no platformio.ini)
// ============================================================================

// ---------- Identidade do nó ----------
#ifndef CLIENT_ID
  #define CLIENT_ID 1
#endif

// ---------- Telemetria e energia ----------
#ifndef TX_INTERVAL_MS
  #define TX_INTERVAL_MS 10000   // 10s (teste)
#endif
#ifndef ENABLE_DEEP_SLEEP
  #define ENABLE_DEEP_SLEEP false
#endif
#ifndef SLEEP_TIME_US
  #define SLEEP_TIME_US (TX_INTERVAL_MS * 1000ULL)
#endif

// ---------- Sensores ----------
#ifndef USE_REAL_SENSORS
  #define USE_REAL_SENSORS true  // true: usa HC-SR04 + umidade; false: simula
#endif

// Ultrassom (HC-SR04)
#ifndef ULTRASONIC_TRIG_PIN
  #define ULTRASONIC_TRIG_PIN 1
#endif
#ifndef ULTRASONIC_ECHO_PIN
  #define ULTRASONIC_ECHO_PIN 2
#endif
#ifndef ULTRASONIC_TIMEOUT_US
  #define ULTRASONIC_TIMEOUT_US 30000  // ~5 m
#endif
#ifndef PRESENCE_THRESHOLD_CM
  #define PRESENCE_THRESHOLD_CM 100    // <100 cm => presença
#endif

// Umidade de solo (ADC 12 bits)
#ifndef MOISTURE_SENSOR_PIN
  #define MOISTURE_SENSOR_PIN 3       // A2
#endif
#ifndef MOISTURE_SAMPLES
  #define MOISTURE_SAMPLES 10
#endif
#ifndef MOISTURE_DRY_VALUE
  #define MOISTURE_DRY_VALUE 4095     // seco (calibrar)
#endif
#ifndef MOISTURE_WET_VALUE
  #define MOISTURE_WET_VALUE 1500     // molhado (calibrar)
#endif

// Parâmetros de simulação (quando USE_REAL_SENSORS=false)
#ifndef HUMID_BASE
  #define HUMID_BASE 60.0
#endif
#ifndef HUMID_VARIATION
  #define HUMID_VARIATION 20.0
#endif
#ifndef DISTANCE_BASE
  #define DISTANCE_BASE 100.0
#endif
#ifndef DISTANCE_VARIATION
  #define DISTANCE_VARIATION 80.0
#endif

// ---------- Otimização de tráfego ----------
#ifndef ENABLE_ADAPTIVE_TX
  #define ENABLE_ADAPTIVE_TX false
#endif
#ifndef HUMID_THRESHOLD
  #define HUMID_THRESHOLD 2.0   // %
#endif
#ifndef DISTANCE_THRESHOLD
  #define DISTANCE_THRESHOLD 10.0 // cm
#endif

#ifndef MAX_TX_RETRIES
  #define MAX_TX_RETRIES 3
#endif

// ---------- Debug ----------
#ifndef DEBUG_MODE
  #define DEBUG_MODE true
#endif
#ifndef SERIAL_BAUD
  #define SERIAL_BAUD 115200
#endif

#if DEBUG_MODE
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ============================================================================
// Seção: LoRa SX1262 (pinos + rádio) — XIAO ESP32-S3 + Wio SX1262
// ============================================================================

// SPI
#ifndef LORA_MOSI
  #define LORA_MOSI 9
#endif
#ifndef LORA_MISO
  #define LORA_MISO 8
#endif
#ifndef LORA_SCK
  #define LORA_SCK  7
#endif

// Controle SX1262
#ifndef LORA_NSS
  #define LORA_NSS  41   // CS
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

// Parâmetros de enlace
#ifndef LORA_FREQUENCY_MHZ
  #define LORA_FREQUENCY_MHZ 915.0  // 915 Américas | 868 EU | 433 Ásia (ajuste regional)
#endif
#ifndef LORA_BANDWIDTH_KHZ
  #define LORA_BANDWIDTH_KHZ 125.0
#endif
#ifndef LORA_SPREADING_FACTOR
  #define LORA_SPREADING_FACTOR 9   // 7..12
#endif
#ifndef LORA_CODING_RATE
  #define LORA_CODING_RATE 7        // 5..8 (RadioLib usa enum/inteiro compatível)
#endif
#ifndef LORA_SYNC_WORD
  #define LORA_SYNC_WORD 0x12       // rede privada
#endif
#ifndef LORA_TX_POWER_DBM
  #define LORA_TX_POWER_DBM 20      // máx ~22
#endif
#ifndef LORA_PREAMBLE_LEN
  #define LORA_PREAMBLE_LEN 8
#endif

// ============================================================================
// Seção: Namespaces C++ (somente leitura no código) — organiza sem duplicar estilo
// ============================================================================

namespace NodeCfg {
  constexpr uint8_t  kClientId        = static_cast<uint8_t>(CLIENT_ID);
  constexpr uint32_t kTxIntervalMs    = static_cast<uint32_t>(TX_INTERVAL_MS);
  constexpr bool     kDeepSleep       = (ENABLE_DEEP_SLEEP);
  constexpr uint64_t kSleepTimeUs     = static_cast<uint64_t>(SLEEP_TIME_US);
}

namespace SensorCfg {
  constexpr bool     kUseReal         = (USE_REAL_SENSORS);
  // Ultrassom
  constexpr uint8_t  kTrigPin         = static_cast<uint8_t>(ULTRASONIC_TRIG_PIN);
  constexpr uint8_t  kEchoPin         = static_cast<uint8_t>(ULTRASONIC_ECHO_PIN);
  constexpr uint32_t kEchoTimeoutUs   = static_cast<uint32_t>(ULTRASONIC_TIMEOUT_US);
  constexpr uint16_t kPresenceThresh  = static_cast<uint16_t>(PRESENCE_THRESHOLD_CM);
  // Umidade solo
  constexpr uint8_t  kMoistPin        = static_cast<uint8_t>(MOISTURE_SENSOR_PIN);
  constexpr uint8_t  kMoistSamples    = static_cast<uint8_t>(MOISTURE_SAMPLES);
  constexpr uint16_t kDryRaw          = static_cast<uint16_t>(MOISTURE_DRY_VALUE);
  constexpr uint16_t kWetRaw          = static_cast<uint16_t>(MOISTURE_WET_VALUE);
  // Simulação
  constexpr float    kHumBase         = static_cast<float>(HUMID_BASE);
  constexpr float    kHumVar          = static_cast<float>(HUMID_VARIATION);
  constexpr float    kDistBase        = static_cast<float>(DISTANCE_BASE);
  constexpr float    kDistVar         = static_cast<float>(DISTANCE_VARIATION);
}

namespace LinkCfg {
  // SPI/pinos
  constexpr uint8_t kMosi = static_cast<uint8_t>(LORA_MOSI);
  constexpr uint8_t kMiso = static_cast<uint8_t>(LORA_MISO);
  constexpr uint8_t kSck  = static_cast<uint8_t>(LORA_SCK);
  constexpr uint8_t kNss  = static_cast<uint8_t>(LORA_NSS);
  constexpr uint8_t kRst  = static_cast<uint8_t>(LORA_RST);
  constexpr uint8_t kDio1 = static_cast<uint8_t>(LORA_DIO1);
  constexpr uint8_t kBusy = static_cast<uint8_t>(LORA_BUSY);

  // Rádio
  constexpr float    kFreqMHz   = static_cast<float>(LORA_FREQUENCY_MHZ);
  constexpr float    kBwKHz     = static_cast<float>(LORA_BANDWIDTH_KHZ);
  constexpr uint8_t  kSf        = static_cast<uint8_t>(LORA_SPREADING_FACTOR);
  constexpr uint8_t  kCr        = static_cast<uint8_t>(LORA_CODING_RATE);
  constexpr uint8_t  kSyncWord  = static_cast<uint8_t>(LORA_SYNC_WORD);
  constexpr int8_t   kTxPowerDb = static_cast<int8_t>(LORA_TX_POWER_DBM);
  constexpr uint16_t kPreamble  = static_cast<uint16_t>(LORA_PREAMBLE_LEN);
}

namespace TxPolicy {
  constexpr bool   kAdaptive       = (ENABLE_ADAPTIVE_TX);
  constexpr float  kHumThreshPct   = static_cast<float>(HUMID_THRESHOLD);
  constexpr float  kDistThreshCm   = static_cast<float>(DISTANCE_THRESHOLD);
  constexpr uint8_t kMaxRetries    = static_cast<uint8_t>(MAX_TX_RETRIES);
}

namespace DebugCfg {
  constexpr bool kDebug = (DEBUG_MODE);
  constexpr uint32_t kBaud = static_cast<uint32_t>(SERIAL_BAUD);
}

// ============================================================================
// Seção: Checks em tempo de compilação
// ============================================================================

static_assert(NodeCfg::kClientId <= 255, "CLIENT_ID deve caber em uint8_t (0..255).");
static_assert(LinkCfg::kSf >= 7 && LinkCfg::kSf <= 12, "LORA_SPREADING_FACTOR deve estar entre 7..12.");
static_assert(LinkCfg::kTxPowerDb >= -9 && LinkCfg::kTxPowerDb <= 22, "Potência fora do intervalo típico SX1262.");
static_assert(SensorCfg::kDryRaw > SensorCfg::kWetRaw, "MOISTURE_DRY_VALUE deve ser maior que MOISTURE_WET_VALUE.");

// ============================================================================

#endif // CONFIG_H
