/**
 * @file main.cpp
 * @brief LoRa Client Node - Environmental Monitoring System (Refatorado)
 *
 * @details
 * - Lê sensores ambientais (umidade e distância)
 * - Transmite dados via LoRa em formato binário compacto (protocol.h)
 * - Transmissão adaptativa e modo de baixo consumo
 * - Suporte para ESP32-S3 XIAO + SX1262
 */

#include "config.h"
#include "protocol.h"
#include <Arduino.h>
#include <RadioLib.h>

// =====================================================
// Instâncias globais
// =====================================================

SX1262 radio = new Module(
    LinkCfg::kNss,
    LinkCfg::kDio1,
    LinkCfg::kRst,
    LinkCfg::kBusy
);

float prev_humidity = 0;
float prev_distance = 0;

uint32_t tx_count = 0;
uint32_t tx_success = 0;
uint32_t tx_failed = 0;
uint32_t tx_skipped = 0;

bool lora_initialized = false;

RTC_DATA_ATTR uint32_t boot_count = 0;

// =====================================================
// Declarações
// =====================================================

void setup_lora();
void setup_sensors();
float read_moisture_sensor();
float read_ultrasonic_distance();
void read_sensors(float& humid, float& distance);
bool should_transmit(float humid, float distance);
bool transmit_sensor_data(float humid, float distance);
void enter_deep_sleep();
void print_stats();
float simulate_sensor_reading(float base, float variation);

// =====================================================
// Setup
// =====================================================

void setup() {
    boot_count++;

#if DEBUG_MODE
    Serial.begin(DebugCfg::kBaud);
    delay(1000);
    DEBUG_PRINTLN("\n===========================================");
    DEBUG_PRINTLN("LoRa Client Node - Environmental Monitor");
    DEBUG_PRINTLN("===========================================");
    DEBUG_PRINTF("Boot count: %u\n", boot_count);
    DEBUG_PRINTF("Client ID: %d\n", NodeCfg::kClientId);
#endif

    setup_lora();
    setup_sensors();

    if (boot_count == 1)
        read_sensors(prev_humidity, prev_distance);
}

// =====================================================
// Loop principal
// =====================================================

void loop() {
    DEBUG_PRINTLN("\n--- Measurement Cycle ---");

    float humidity, distance;
    read_sensors(humidity, distance);

    DEBUG_PRINTF("Humidity: %.2f %%\n", humidity);
    DEBUG_PRINTF("Distance: %.2f cm\n", distance);
    DEBUG_PRINTF("Presence: %s\n", (distance < SensorCfg::kPresenceThresh) ? "DETECTED" : "No");

    bool should_send = true;

#if ENABLE_ADAPTIVE_TX
    should_send = should_transmit(humidity, distance);
    if (!should_send) {
        DEBUG_PRINTLN("No significant change detected - skipping transmission");
        tx_skipped++;
    }
#endif

    if (should_send) {
        bool success = transmit_sensor_data(humidity, distance);
        if (success) {
            tx_success++;
            prev_humidity = humidity;
            prev_distance = distance;
        } else {
            tx_failed++;
        }
    }

    tx_count++;
    print_stats();

#if ENABLE_DEEP_SLEEP
    DEBUG_PRINTLN("\nEntering deep sleep...");
    delay(100);
    enter_deep_sleep();
#else
    DEBUG_PRINTF("\nWaiting %d seconds...\n", NodeCfg::kTxIntervalMs / 1000);
    delay(NodeCfg::kTxIntervalMs);
#endif
}

// =====================================================
// LoRa Setup
// =====================================================

void setup_lora() {
    DEBUG_PRINTLN("\n========================================");
    DEBUG_PRINTLN("LoRa Radio Initialization");
    DEBUG_PRINTLN("========================================");

    DEBUG_PRINTF("   MOSI:%d, MISO:%d, SCK:%d\n",
        LinkCfg::kMosi, LinkCfg::kMiso, LinkCfg::kSck);
    DEBUG_PRINTF("   NSS:%d, RST:%d, DIO1:%d, BUSY:%d\n",
        LinkCfg::kNss, LinkCfg::kRst, LinkCfg::kDio1, LinkCfg::kBusy);

    pinMode(LinkCfg::kRst, OUTPUT);
    digitalWrite(LinkCfg::kRst, LOW);
    delay(10);
    digitalWrite(LinkCfg::kRst, HIGH);
    delay(10);
    DEBUG_PRINTLN("✓ Reset pulse sent");

    SPI.begin(LinkCfg::kSck, LinkCfg::kMiso, LinkCfg::kMosi, LinkCfg::kNss);
    SPI.setFrequency(2'000'000);
    delay(100);

    int state = radio.begin(
        LinkCfg::kFreqMHz,
        LinkCfg::kBwKHz,
        LinkCfg::kSf,
        LinkCfg::kCr,
        LinkCfg::kSyncWord,
        LinkCfg::kTxPowerDb,
        LinkCfg::kPreamble
    );

    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN("✓✓✓ LoRa initialization SUCCESS ✓✓✓");
        lora_initialized = true;
        radio.setCurrentLimit(140);
    } else {
        DEBUG_PRINTF("✗✗✗ LoRa init failed (code %d)\n", state);
        lora_initialized = false;
    }
}

// =====================================================
// Sensor Setup
// =====================================================

void setup_sensors() {
    DEBUG_PRINTLN("\nInitializing sensors...");

#if USE_REAL_SENSORS
    pinMode(SensorCfg::kTrigPin, OUTPUT);
    pinMode(SensorCfg::kEchoPin, INPUT);
    digitalWrite(SensorCfg::kTrigPin, LOW);
    pinMode(SensorCfg::kMoistPin, INPUT);
    analogReadResolution(12);

    DEBUG_PRINTF("✓ HC-SR04 ready (TRIG: %d, ECHO: %d)\n",
        SensorCfg::kTrigPin, SensorCfg::kEchoPin);
    DEBUG_PRINTF("✓ MH-RD ready (ADC: %d)\n", SensorCfg::kMoistPin);

    float test_humid = read_moisture_sensor();
    float test_dist = read_ultrasonic_distance();
    DEBUG_PRINTF("Test readings: Humidity=%.1f%%, Distance=%.1fcm\n",
        test_humid, test_dist);
#else
    DEBUG_PRINTLN("✓ Using simulated sensors");
#endif
}

// =====================================================
// Leituras dos sensores
// =====================================================

float read_moisture_sensor() {
#if USE_REAL_SENSORS
    uint32_t sum = 0;
    for (int i = 0; i < SensorCfg::kMoistSamples; i++) {
        sum += analogRead(SensorCfg::kMoistPin);
        delay(10);
    }
    uint16_t avg = sum / SensorCfg::kMoistSamples;
    float humid = 100.0 - ((float)(avg - SensorCfg::kWetRaw) /
                           (SensorCfg::kDryRaw - SensorCfg::kWetRaw) * 100.0);
    return constrain(humid, 0, 100);
#else
    return simulate_sensor_reading(SensorCfg::kHumBase, SensorCfg::kHumVar);
#endif
}

float read_ultrasonic_distance() {
#if USE_REAL_SENSORS
    digitalWrite(SensorCfg::kTrigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(SensorCfg::kTrigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(SensorCfg::kTrigPin, LOW);

    long dur = pulseIn(SensorCfg::kEchoPin, HIGH, SensorCfg::kEchoTimeoutUs);
    float dist = (dur * 0.0343f) / 2.0f;
    if (dur == 0 || dist > 400) dist = 400;
    return constrain(dist, 2, 400);
#else
    return simulate_sensor_reading(SensorCfg::kDistBase, SensorCfg::kDistVar);
#endif
}

// =====================================================
// Lógica de envio
// =====================================================

bool should_transmit(float humid, float dist) {
    bool hchg = abs(humid - prev_humidity) > TxPolicy::kHumThreshPct;
    bool dchg = abs(dist - prev_distance) > TxPolicy::kDistThreshCm;
    if (boot_count == 1 || hchg || dchg) return true;
    return (boot_count % 10 == 0);
}

bool transmit_sensor_data(float humid, float dist) {
    if (!lora_initialized) return false;

    SensorDataMessage msg{};
    msg.msg_type   = MSG_TYPE_SENSOR_DATA;
    msg.client_id  = NodeCfg::kClientId;
    msg.timestamp  = millis();
    msg.temperature= 0;
    msg.humidity   = encode_humidity(humid);
    msg.distance_cm= (uint16_t)dist;
    msg.battery    = 100;
    msg.reserved   = 0;
    msg.checksum   = calculate_checksum((uint8_t*)&msg, sizeof(msg));

    DEBUG_PRINTF("TX attempt (%d bytes): humid=%.2f dist=%.1f\n",
        sizeof(msg), humid, dist);

    for (int i = 0; i < TxPolicy::kMaxRetries; i++) {
        int state = radio.transmit((uint8_t*)&msg, sizeof(msg));
        if (state == RADIOLIB_ERR_NONE) return true;
        delay(100);
    }
    return false;
}

// =====================================================
// Energia e estatísticas
// =====================================================

void enter_deep_sleep() {
    esp_sleep_enable_timer_wakeup(NodeCfg::kSleepTimeUs);
    esp_deep_sleep_start();
}

void print_stats() {
    DEBUG_PRINTF("\nCycles:%u  Success:%u  Fail:%u  Skip:%u\n",
        tx_count, tx_success, tx_failed, tx_skipped);
    if (tx_count > 0) {
        float eff = (float)tx_success / tx_count * 100.0f;
        DEBUG_PRINTF("Efficiency: %.1f%%\n", eff);
    }
}

// =====================================================
// Simulação
// =====================================================

float simulate_sensor_reading(float base, float var) {
    float r = (float)random(-1000, 1000) / 1000.0f;
    return base + r * var;
}
