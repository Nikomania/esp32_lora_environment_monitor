/**
 * @file protocol.h
 * @brief Compact binary protocol for sensor data transmission
 *
 * This file is shared between client and gateway
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
 */
struct __attribute__((packed)) SensorDataMessage {
    uint8_t msg_type; // Message type (1 byte)
    uint8_t client_id; // Client identifier (1 byte)
    uint32_t timestamp; // Unix timestamp or millis() (4 bytes)
    int16_t temperature; // Temperature * 100 (2 bytes)
    uint16_t humidity; // Humidity * 100 (2 bytes)
    uint16_t distance_cm; // Distance in cm from ultrasonic sensor (2 bytes)
    uint8_t battery; // Battery level % (1 byte)
    uint16_t reserved; // Reserved (2 bytes) - MOVED BEFORE CHECKSUM
    uint8_t checksum; // Simple checksum (1 byte) - MUST BE LAST
};

/**
 * @brief Heartbeat message (8 bytes total)
 */
struct __attribute__((packed)) HeartbeatMessage {
    uint8_t msg_type; // Message type
    uint8_t client_id; // Client identifier
    uint32_t timestamp; // Unix timestamp or millis()
    uint8_t status; // Status flags
    uint8_t checksum; // Simple checksum - MUST BE LAST
};

/**
 * @brief Alert message (12 bytes total)
 */
struct __attribute__((packed)) AlertMessage {
    uint8_t msg_type; // Message type
    uint8_t client_id; // Client identifier
    uint32_t timestamp; // Unix timestamp or millis()
    uint8_t alert_code; // Alert type code
    int16_t alert_value; // Value that triggered alert
    uint8_t severity; // Severity level
    uint8_t reserved; // Reserved
    uint8_t checksum; // Simple checksum - MUST BE LAST
};

// =====================================================
// Status Flags
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

inline uint8_t calculate_checksum(const uint8_t* data, size_t length)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < length - 1; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

inline bool verify_checksum(const uint8_t* data, size_t length)
{
    uint8_t calculated = calculate_checksum(data, length);
    return calculated == data[length - 1];
}

inline int16_t encode_temperature(float temp)
{
    return (int16_t)(temp * 100);
}

inline float decode_temperature(int16_t temp)
{
    return temp / 100.0f;
}

inline uint16_t encode_humidity(float humid)
{
    return (uint16_t)(humid * 100);
}

inline float decode_humidity(uint16_t humid)
{
    return humid / 100.0f;
}

#endif // PROTOCOL_H
