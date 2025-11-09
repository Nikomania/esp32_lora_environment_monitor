/**
 * @file main.cpp
 * @brief LoRa Client Node - Environmental Monitoring System
 *
 * Features:
 * - Reads environmental sensors (soil moisture, ultrasonic distance)
 * - Transmits data via LoRa using compact binary protocol
 * - Implements adaptive transmission (only sends on significant changes)
 * - Uses deep sleep for power efficiency
 * - Supports ESP32-S3 XIAO with SX1262 LoRa module
 */

#include "config.h"
#include "protocol.h"
#include <Arduino.h>
#include <RadioLib.h>

// =====================================================
// Global Variables
// =====================================================

// LoRa radio instance (SX1262)
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

// Previous sensor readings (for adaptive transmission)
float prev_humidity = 0;
float prev_distance = 0;

// Transmission statistics
uint32_t tx_count = 0;
uint32_t tx_success = 0;
uint32_t tx_failed = 0;
uint32_t tx_skipped = 0; // Skipped due to no significant change

// LoRa status flag
bool lora_initialized = false;

// Boot count for deep sleep
RTC_DATA_ATTR uint32_t boot_count = 0;

// =====================================================
// Function Declarations
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

void setup()
{
    // Increment boot count
    boot_count++;

// Initialize serial for debugging
#if DEBUG_MODE
    Serial.begin(SERIAL_BAUD);
    delay(1000); // Wait for serial to initialize
    DEBUG_PRINTLN("\n===========================================");
    DEBUG_PRINTLN("LoRa Client Node - Environmental Monitor");
    DEBUG_PRINTLN("===========================================");
    DEBUG_PRINTF("Boot count: %u\n", boot_count);
    DEBUG_PRINTF("Client ID: %d\n", CLIENT_ID);
#endif

    // Setup LoRa radio
    setup_lora();

    // Setup sensors
    setup_sensors();

    // Initialize sensor readings with current values
    if (boot_count == 1) {
        read_sensors(prev_humidity, prev_distance);
    }
}

// =====================================================
// Main Loop
// =====================================================

void loop()
{
    DEBUG_PRINTLN("\n--- Measurement Cycle ---");

    // Read current sensor values
    float humidity, distance;
    read_sensors(humidity, distance);

    // Display readings
    DEBUG_PRINTF("Humidity: %.2f %%\n", humidity);
    DEBUG_PRINTF("Distance: %.2f cm\n", distance);
    DEBUG_PRINTF("Presence: %s\n", (distance < 100.0) ? "DETECTED" : "No");

    // Check if transmission is needed (adaptive mode)
    bool should_send = true;

#if ENABLE_ADAPTIVE_TX
    should_send = should_transmit(humidity, distance);

    if (!should_send) {
        DEBUG_PRINTLN("No significant change detected - skipping transmission");
        tx_skipped++;
    }
#endif

    // Transmit data if needed
    if (should_send) {
        bool success = transmit_sensor_data(humidity, distance);

        if (success) {
            DEBUG_PRINTLN("✓ Transmission successful");
            tx_success++;

            // Update previous values
            prev_humidity = humidity;
            prev_distance = distance;
        } else {
            DEBUG_PRINTLN("✗ Transmission failed");
            tx_failed++;
        }
    }

    tx_count++;

    // Print statistics
    print_stats();

// Enter deep sleep to save power
#if ENABLE_DEEP_SLEEP
    DEBUG_PRINTLN("\nEntering deep sleep...");
    delay(100); // Give time for serial to flush
    enter_deep_sleep();
#else
    // If deep sleep is disabled, just delay
    DEBUG_PRINTF("\nWaiting %d seconds...\n", TX_INTERVAL_MS / 1000);
    delay(TX_INTERVAL_MS);
#endif
}

// =====================================================
// LoRa Setup
// =====================================================

void setup_lora()
{
    DEBUG_PRINTLN("\n========================================");
    DEBUG_PRINTLN("LoRa Radio Initialization");
    DEBUG_PRINTLN("========================================");
    
    DEBUG_PRINTLN("\n1. Pin Configuration:");
    DEBUG_PRINTF("   MOSI: D%d, MISO: D%d, SCK: D%d\n", LORA_MOSI, LORA_MISO, LORA_SCK);
    DEBUG_PRINTF("   NSS:  D%d, RST: D%d, DIO1: D%d, BUSY: D%d\n", 
                 LORA_NSS, LORA_RST, LORA_DIO1, LORA_BUSY);
    
    DEBUG_PRINTLN("\n2. Radio Parameters:");
    DEBUG_PRINTF("   Frequency: %.1f MHz\n", LORA_FREQUENCY);
    DEBUG_PRINTF("   Bandwidth: %.1f kHz\n", LORA_BANDWIDTH);
    DEBUG_PRINTF("   Spreading Factor: %d\n", LORA_SPREADING);
    DEBUG_PRINTF("   TX Power: %d dBm\n", LORA_TX_POWER);

    DEBUG_PRINTLN("\n3. Testing hardware reset...");
    // Manual reset sequence
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    DEBUG_PRINTLN("   ✓ Reset pulse sent");

    DEBUG_PRINTLN("\n4. Initializing SPI bus...");
    // Initialize SPI with custom pins
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    SPI.setFrequency(2000000); // 2 MHz for initial communication
    DEBUG_PRINTLN("   ✓ SPI initialized at 2 MHz");
    
    // Small delay to ensure SPI is ready
    delay(100);

    DEBUG_PRINTLN("\n5. Attempting to initialize SX1262...");
    // Initialize SX1262
    int state = radio.begin(
        LORA_FREQUENCY,
        LORA_BANDWIDTH,
        LORA_SPREADING,
        LORA_CODING_RATE,
        LORA_SYNC_WORD,
        LORA_TX_POWER,
        LORA_PREAMBLE);

    DEBUG_PRINTLN("\n========================================");
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN("✓✓✓ LoRa initialization SUCCESS ✓✓✓");
        DEBUG_PRINTLN("========================================\n");
        lora_initialized = true;
        
        // Configure LoRa for low power
        radio.setCurrentLimit(140); // Limit current to 140 mA
        DEBUG_PRINTLN("LoRa ready for transmission");
    } else {
        DEBUG_PRINTLN("✗✗✗ LoRa initialization FAILED ✗✗✗");
        DEBUG_PRINTLN("========================================");
        DEBUG_PRINTF("\nError code: %d\n", state);
        DEBUG_PRINTLN("\nError meanings:");
        DEBUG_PRINTLN("  -2 = CHIP_NOT_FOUND (no response from module)");
        DEBUG_PRINTLN("  -5 = INVALID_FREQUENCY");
        DEBUG_PRINTLN("  -6 = INVALID_BANDWIDTH");
        DEBUG_PRINTLN("\nTroubleshooting steps:");
        DEBUG_PRINTLN("  1. Verify physical connections");
        DEBUG_PRINTLN("  2. Check if module has power (3.3V)");
        DEBUG_PRINTLN("  3. Measure voltage on BUSY pin during init");
        DEBUG_PRINTLN("  4. Try different pin configuration");
        DEBUG_PRINTLN("  5. Check if SPI bus is shared with other devices");
        DEBUG_PRINTLN("\nSystem will continue in simulation mode...\n");
        lora_initialized = false;
        return;
    }
}

// =====================================================
// Sensor Setup
// =====================================================

void setup_sensors()
{
    DEBUG_PRINTLN("\nInitializing sensors...");

#if USE_REAL_SENSORS
    // Setup HC-SR04 Ultrasonic sensor
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    DEBUG_PRINTF("✓ HC-SR04 initialized (TRIG: GPIO%d, ECHO: GPIO%d)\n", 
                 ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN);

    // Setup MH-RD Moisture sensor (ADC)
    pinMode(MOISTURE_SENSOR_PIN, INPUT);
    analogReadResolution(12); // 12-bit ADC (0-4095)
    DEBUG_PRINTF("✓ MH-RD Moisture sensor initialized (ADC: GPIO%d - A2, 3.3V)\n", 
                 MOISTURE_SENSOR_PIN);

    // Test read
    float test_humid = read_moisture_sensor();
    float test_dist = read_ultrasonic_distance();
    DEBUG_PRINTF("Test readings: Moisture=%.1f%%, Distance=%.1fcm\n", 
                 test_humid, test_dist);
#else
    DEBUG_PRINTLN("✓ Using simulated sensors");
#endif

    DEBUG_PRINTLN("Sensors ready\n");
}

// =====================================================
// Read Moisture Sensor (MH-RD)
// =====================================================

float read_moisture_sensor()
{
#if USE_REAL_SENSORS
    uint32_t sum = 0;
    
    // Take multiple samples and average
    for (int i = 0; i < MOISTURE_SAMPLES; i++) {
        sum += analogRead(MOISTURE_SENSOR_PIN);
        delay(10); // Small delay between readings
    }
    
    uint16_t avg_value = sum / MOISTURE_SAMPLES;
    
    // Convert ADC value to percentage (inverted: lower ADC = more wet)
    // Map: DRY_VALUE (4095) -> 0%, WET_VALUE (1500) -> 100%
    float humidity_percent = 100.0 - ((float)(avg_value - MOISTURE_WET_VALUE) / 
                             (MOISTURE_DRY_VALUE - MOISTURE_WET_VALUE) * 100.0);
    
    // Constrain to 0-100%
    humidity_percent = constrain(humidity_percent, 0, 100);
    
    DEBUG_PRINTF("[SENSOR] MH-RD ADC: %d, Humidity: %.1f%%\n", avg_value, humidity_percent);
    
    return humidity_percent;
#else
    return simulate_sensor_reading(HUMID_BASE, HUMID_VARIATION);
#endif
}

// =====================================================
// Read Ultrasonic Distance (HC-SR04)
// =====================================================

float read_ultrasonic_distance()
{
#if USE_REAL_SENSORS
    // Send trigger pulse
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    
    // Measure echo pulse duration
    long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);
    
    // Calculate distance in cm (speed of sound = 343 m/s = 0.0343 cm/µs)
    // Distance = (duration / 2) * 0.0343
    float distance_cm = (duration * 0.0343) / 2.0;
    
    // If timeout or invalid reading, return max range
    if (duration == 0 || distance_cm > 400) {
        distance_cm = 400; // Out of range
        DEBUG_PRINTLN("[SENSOR] HC-SR04: Out of range");
    } else if (distance_cm < 2) {
        distance_cm = 2; // Too close, sensor limitation
        DEBUG_PRINTLN("[SENSOR] HC-SR04: Too close");
    } else {
        DEBUG_PRINTF("[SENSOR] HC-SR04: %.1f cm\n", distance_cm);
    }
    
    return distance_cm;
#else
    return simulate_sensor_reading(DISTANCE_BASE, DISTANCE_VARIATION);
#endif
}

// =====================================================
// Sensor Reading (Simulated or Real)
// =====================================================

void read_sensors(float& humid, float& distance)
{
#if USE_REAL_SENSORS
    // Read from real sensors
    humid = read_moisture_sensor();
    distance = read_ultrasonic_distance();
#else
    // Simulate sensor readings with some variation
    humid = simulate_sensor_reading(HUMID_BASE, HUMID_VARIATION);
    distance = simulate_sensor_reading(DISTANCE_BASE, DISTANCE_VARIATION);

    // Add some temporal correlation (readings don't change drastically)
    if (boot_count > 1) {
        humid = prev_humidity + (humid - HUMID_BASE) * 0.3;
        distance = prev_distance + (distance - DISTANCE_BASE) * 0.3;
    }
#endif

    // Ensure values are within reasonable ranges
    humid = constrain(humid, 0, 100);
    distance = constrain(distance, 5, 400); // Ultrasonic sensor range: 5-400cm
}

// =====================================================
// Adaptive Transmission Decision
// =====================================================

bool should_transmit(float humid, float distance)
{
    // Check if any value changed significantly
    bool humid_changed = abs(humid - prev_humidity) > HUMID_THRESHOLD;
    bool distance_changed = abs(distance - prev_distance) > DISTANCE_THRESHOLD;

    // Always transmit on first boot or if any value changed significantly
    if (boot_count == 1 || humid_changed || distance_changed) {
        if (humid_changed)
            DEBUG_PRINTLN("  → Humidity changed");
        if (distance_changed)
            DEBUG_PRINTLN("  → Distance changed (movement detected)");
        return true;
    }

    // Also transmit periodically even if no change (every 10 cycles as heartbeat)
    if (boot_count % 10 == 0) {
        DEBUG_PRINTLN("  → Periodic heartbeat");
        return true;
    }

    return false;
}

// =====================================================
// Transmit Sensor Data
// =====================================================

bool transmit_sensor_data(float humid, float distance)
{
    DEBUG_PRINTLN("\nPreparing transmission...");

    // Check if LoRa is initialized
    if (!lora_initialized) {
        DEBUG_PRINTLN("✗ LoRa not initialized - skipping transmission");
        return false;
    }

    // Create sensor data message
    SensorDataMessage msg;
    msg.msg_type = MSG_TYPE_SENSOR_DATA;
    msg.client_id = CLIENT_ID;
    msg.timestamp = millis();
    msg.temperature = 0; // Not used - set to 0
    msg.humidity = encode_humidity(humid);
    msg.distance_cm = (uint16_t)distance; // Distance from ultrasonic sensor in cm
    msg.battery = 100; // Simulated - replace with actual battery reading
    msg.reserved = 0;

    // Calculate and add checksum
    msg.checksum = calculate_checksum((uint8_t*)&msg, sizeof(msg));

    DEBUG_PRINTF("Message size: %d bytes\n", sizeof(msg));
    DEBUG_PRINTF("Encoded humid: %u (%.2f %%)\n", msg.humidity, decode_humidity(msg.humidity));
    DEBUG_PRINTF("Distance: %u cm\n", msg.distance_cm);
    DEBUG_PRINTF("Presence: %s\n", (msg.distance_cm < 100) ? "DETECTED" : "No");

    // Transmit with retries
    int attempts = 0;
    int state = RADIOLIB_ERR_NONE;

    while (attempts < MAX_TX_RETRIES) {
        DEBUG_PRINTF("Transmission attempt %d/%d...\n", attempts + 1, MAX_TX_RETRIES);

        state = radio.transmit((uint8_t*)&msg, sizeof(msg));

        if (state == RADIOLIB_ERR_NONE) {
            DEBUG_PRINTLN("✓ Packet transmitted successfully");
            return true;
        } else {
            DEBUG_PRINTF("✗ Transmission failed, code: %d\n", state);
            attempts++;

            if (attempts < MAX_TX_RETRIES) {
                delay(100); // Brief delay before retry
            }
        }
    }

    return false;
}

// =====================================================
// Power Management - Deep Sleep
// =====================================================

void enter_deep_sleep()
{
    // Configure wake-up timer
    esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);

    // Enter deep sleep
    esp_deep_sleep_start();
}

// =====================================================
// Statistics
// =====================================================

void print_stats()
{
    DEBUG_PRINTLN("\n--- Statistics ---");
    DEBUG_PRINTF("Total cycles: %u\n", tx_count);
    DEBUG_PRINTF("Successful TX: %u\n", tx_success);
    DEBUG_PRINTF("Failed TX: %u\n", tx_failed);
    DEBUG_PRINTF("Skipped TX: %u\n", tx_skipped);

    if (tx_count > 0) {
        float efficiency = (float)(tx_success) / tx_count * 100;
        float reduction = (float)(tx_skipped) / tx_count * 100;
        DEBUG_PRINTF("Success rate: %.1f%%\n", efficiency);
        DEBUG_PRINTF("TX reduction: %.1f%%\n", reduction);
    }
}

// =====================================================
// Helper Functions
// =====================================================

float simulate_sensor_reading(float base, float variation)
{
    // Generate pseudo-random value around base with given variation
    float random_factor = (float)random(-1000, 1000) / 1000.0; // -1.0 to 1.0
    return base + (random_factor * variation);
}
