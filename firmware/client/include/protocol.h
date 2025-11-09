/**
 * @file protocol.h
 * @brief Compact binary protocol for sensor data transmission
 *
 * Protocol Design:
 * - Minimal payload size to reduce transmission time and power consumption
 * - Binary format instead of JSON for efficiency
 * - Fixed-size messages for easy parsing
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// =====================================================
// Message Types
// =====================================================

#define MSG_TYPE_SENSOR_DATA 0x01
#define MSG_TYPE_HEARTBEAT 0x02
#define MSG_TYPE_ALERT 0x03
#define MSG_TYPE_ACK 0xAA

// =====================================================
// Message Structure
// =====================================================

/**
 * @brief Sensor data message (16 bytes total)
 *
 * Optimized for minimal size while maintaining precision
 */
struct __attribute__((packed)) SensorDataMessage {
    uint8_t msg_type; // Message type (1 byte)
    uint8_t client_id; // Client identifier (1 byte, supports up to 255 clients)
    uint32_t timestamp; // Unix timestamp or millis() (4 bytes)
    int16_t temperature; // Temperature * 100 (2 bytes, range: -327.68°C to +327.67°C)
    uint16_t humidity; // Humidity * 100 (2 bytes, range: 0-655.35%)
    uint16_t distance_cm; // Distance in cm from ultrasonic sensor (2 bytes, range: 0-65535)
    uint8_t battery; // Battery level % (1 byte, 0-100)
    uint8_t checksum; // Simple checksum for error detection (1 byte)
    uint16_t reserved; // Reserved for future use (2 bytes)
};

/**
 * @brief Heartbeat message (8 bytes total)
 *
 * Sent periodically when no significant data changes
 */
struct __attribute__((packed)) HeartbeatMessage {
    uint8_t msg_type; // Message type = MSG_TYPE_HEARTBEAT
    uint8_t client_id; // Client identifier
    uint32_t timestamp; // Unix timestamp or millis()
    uint8_t status; // Status flags (bit field)
    uint8_t checksum; // Simple checksum
};

/**
 * @brief Alert message (12 bytes total)
 *
 * Sent when thresholds are exceeded
 */
struct __attribute__((packed)) AlertMessage {
    uint8_t msg_type; // Message type = MSG_TYPE_ALERT
    uint8_t client_id; // Client identifier
    uint32_t timestamp; // Unix timestamp or millis()
    uint8_t alert_code; // Alert type code
    int16_t alert_value; // Value that triggered alert
    uint8_t severity; // Severity level (0-255)
    uint8_t checksum; // Simple checksum
    uint8_t reserved; // Reserved
};

// =====================================================
// Status Flags (for HeartbeatMessage)
// =====================================================

#define STATUS_OK 0x00
#define STATUS_LOW_BATTERY 0x01
#define STATUS_SENSOR_ERROR 0x02
#define STATUS_LORA_ERROR 0x04

// =====================================================
// Alert Codes
// =====================================================

#define ALERT_TEMP_HIGH 0x10
#define ALERT_TEMP_LOW 0x11
#define ALERT_HUMIDITY_HIGH 0x20
#define ALERT_HUMIDITY_LOW 0x21
#define ALERT_DISTANCE_LOW 0x30  // Distance too close (presence detected)

// =====================================================
// Helper Functions
// =====================================================

/**
 * @brief Calculate simple checksum for message integrity
 */
inline uint8_t calculate_checksum(const uint8_t* data, size_t length)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < length - 1; i++) { // -1 to exclude checksum byte
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Verify message checksum
 */
inline bool verify_checksum(const uint8_t* data, size_t length)
{
    uint8_t calculated = calculate_checksum(data, length);
    return calculated == data[length - 1];
}

/**
 * @brief Convert float temperature to int16_t (scaled by 100)
 */
inline int16_t encode_temperature(float temp)
{
    return (int16_t)(temp * 100);
}

/**
 * @brief Convert int16_t back to float temperature
 */
inline float decode_temperature(int16_t temp)
{
    return temp / 100.0f;
}

/**
 * @brief Convert float humidity to uint16_t (scaled by 100)
 */
inline uint16_t encode_humidity(float humid)
{
    return (uint16_t)(humid * 100);
}

/**
 * @brief Convert uint16_t back to float humidity
 */
inline float decode_humidity(uint16_t humid)
{
    return humid / 100.0f;
}

#endif // PROTOCOL_H
