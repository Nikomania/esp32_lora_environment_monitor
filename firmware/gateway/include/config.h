/**
 * @file config.h
 * @brief Configuration file for LoRa Gateway
 *
 * Hardware: ESP32-S3 XIAO + SX1262 LoRa Module
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =====================================================
// LoRa Module Configuration (SX1262)
// =====================================================

// SPI Pins for ESP32-S3 XIAO with Wio-SX1262
// Based on official Seeed Studio configuration
#define LORA_MOSI 9  // GPIO9  - D9
#define LORA_MISO 8  // GPIO8  - D8
#define LORA_SCK 7   // GPIO7  - D7
#define LORA_NSS 41  // GPIO41 - D4  - Chip Select
#define LORA_RST 42  // GPIO42 - D3  - Reset
#define LORA_DIO1 39 // GPIO39 - D1  - DIO1 (IRQ)
#define LORA_BUSY 40 // GPIO40 - D2  - BUSY

// LoRa Radio Parameters (MUST match client settings)
#define LORA_FREQUENCY 915.0 // MHz
#define LORA_BANDWIDTH 125.0 // kHz
#define LORA_SPREADING 9 // Spreading Factor
#define LORA_CODING_RATE 7 // Coding Rate
#define LORA_SYNC_WORD 0x12 // Private network sync word
#define LORA_PREAMBLE 8 // Preamble length

// =====================================================
// WiFi Configuration
// =====================================================

#define ENABLE_WIFI true  // WiFi enabled for server communication
#define WIFI_SSID "MORAES_MATOS_2.4G" // Change this to your WiFi name
#define WIFI_PASSWORD "Abzx@02468" // Change this to your WiFi password
#define WIFI_TIMEOUT_MS 10000 // 10 seconds

// =====================================================
// Server Configuration
// =====================================================

// Server communication method
#define USE_HTTP true // If true, use HTTP POST; if false, use Serial
#define USE_SERIAL true // Enable serial output (always useful for debugging)

// HTTP Server settings
#define SERVER_HOST "192.168.1.11" // Your server IP
#define SERVER_PORT 8080
#define SERVER_ENDPOINT "/api/sensor-data"

// Serial settings for forwarding (if using Serial to PC/Raspberry Pi)
#define SERIAL_BAUD 115200

// =====================================================
// Gateway Configuration
// =====================================================

// Gateway ID
#define GATEWAY_ID 1

// Buffer settings
#define MAX_PACKET_SIZE 256

// Statistics interval (ms)
#define STATS_INTERVAL_MS 60000 // Print stats every 60 seconds

// =====================================================
// Debug Configuration
// =====================================================

#define DEBUG_MODE true

// Test mode - simulates receiving LoRa packets every 15 seconds
// Useful for testing without physical LoRa client
#define TEST_MODE true // Set to true to enable test mode

#define TEST_INTERVAL_MS 15000 // Send test packet every 15 seconds

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
