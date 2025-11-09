/**
 * @file config.h
 * @brief Configuration file for LoRa Client Node
 *
 * Hardware: ESP32-S3 XIAO + SX1262 LoRa Module
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =====================================================
// LoRa Module Configuration (SX1262)
// =====================================================

// Pin Configuration for XIAO ESP32-S3 + Wio-SX1262 Module
// Based on official Seeed Studio One Channel Hub firmware
// Reference: https://github.com/Seeed-Studio/one_channel_hub

#define LORA_MOSI 9    // GPIO9 (SPI MOSI)
#define LORA_MISO 8    // GPIO8 (SPI MISO)
#define LORA_SCK 7     // GPIO7 (SPI SCK)
#define LORA_NSS 41    // GPIO41 (Chip Select/NSS)
#define LORA_RST 42    // GPIO42 (Reset)
#define LORA_DIO1 39   // GPIO39 (DIO1/IRQ)
#define LORA_BUSY 40   // GPIO40 (BUSY)

// LoRa Radio Parameters
#define LORA_FREQUENCY 915.0 // MHz (adjust for your region: 915 for Americas, 868 for EU, 433 for Asia)
#define LORA_BANDWIDTH 125.0 // kHz
#define LORA_SPREADING 9 // Spreading Factor (7-12, higher = longer range, slower)
#define LORA_CODING_RATE 7 // Coding Rate (5-8)
#define LORA_SYNC_WORD 0x12 // Private network sync word
#define LORA_TX_POWER 20 // dBm (max 22)
#define LORA_PREAMBLE 8 // Preamble length

// =====================================================
// Client Configuration
// =====================================================

// Unique client ID (should be different for each node)
#define CLIENT_ID 1

// Transmission interval (milliseconds)
#define TX_INTERVAL_MS 10000 // 10 seconds for testing

// Sleep mode settings
#define ENABLE_DEEP_SLEEP false // Disabled for testing
#define SLEEP_TIME_US (TX_INTERVAL_MS * 1000) // Convert to microseconds

// =====================================================
// Sensor Configuration
// =====================================================

// Enable/disable real sensors (set to false for simulation)
#define USE_REAL_SENSORS true

// HC-SR04 Ultrasonic Sensor Pins (Distance/Presence)
#define ULTRASONIC_TRIG_PIN 1  // GPIO1 (D0)
#define ULTRASONIC_ECHO_PIN 2  // GPIO2 (D1)
#define ULTRASONIC_TIMEOUT_US 30000  // 30ms timeout = ~5m max range
#define PRESENCE_THRESHOLD_CM 100  // Distance < 100cm = presence detected

// MH-RD Soil Moisture Sensor (Works with 3.3V!)
#define MOISTURE_SENSOR_PIN 3  // GPIO3 (A2) - ADC pin
#define MOISTURE_SAMPLES 10    // Number of samples to average
#define MOISTURE_DRY_VALUE 4095   // ADC value when dry (12-bit ADC max)
#define MOISTURE_WET_VALUE 1500   // ADC value when wet (calibrate this!)

// Sensor simulation parameters (used when USE_REAL_SENSORS = false)
#define HUMID_BASE 60.0 // Base humidity in %
#define HUMID_VARIATION 20.0 // Humidity variation range (30-80%)
#define DISTANCE_BASE 100.0 // Base distance in cm (ultrasonic sensor)
#define DISTANCE_VARIATION 80.0 // Distance variation range (20-180cm)
// Note: Distance < 100cm = presence detected

// =====================================================
// Data Optimization
// =====================================================

// Adaptive transmission: only send if value changed significantly
#define ENABLE_ADAPTIVE_TX false // Changed to false for testing
#define HUMID_THRESHOLD 2.0 // Percent
#define DISTANCE_THRESHOLD 10.0 // cm (significant movement detected)

// Maximum retries for failed transmissions
#define MAX_TX_RETRIES 3

// =====================================================
// Debug Configuration
// =====================================================

#define DEBUG_MODE true
#define SERIAL_BAUD 115200

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
