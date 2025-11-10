/**
 * @file protocol.h
 * @brief Protocolo binário compacto compartilhado (client <-> gateway).
 *
 * Layout (LITTLE-ENDIAN no ESP32)
 *  Offset  Size  Field
 *  0       1     msg_type
 *  1       1     client_id
 *  2       4     timestamp (millis)
 *  6       2     temperature (°C x100)
 *  8       2     humidity (% x100)
 *  10      2     distance_cm (uint16)
 *  12      1     battery (%)
 *  13      2     reserved
 *  15      1     checksum (XOR de bytes [0..14])  ← ÚLTIMO BYTE
 *
 * Total = 16 bytes
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// =====================================================
// Tipos de mensagem
// =====================================================
#define MSG_TYPE_SENSOR_DATA 0x01
#define MSG_TYPE_HEARTBEAT   0x02
#define MSG_TYPE_ALERT       0x03
#define MSG_TYPE_ACK         0xAA

// =====================================================
// Estruturas
// =====================================================

/**
 * @brief Pacote de dados de sensores (16 bytes).
 *
 * Checksum é XOR de todos os bytes exceto o próprio checksum
 * (ou seja, XOR de bytes [0..14]).
 */
struct __attribute__((packed)) SensorDataMessage {
    uint8_t  msg_type;
    uint8_t  client_id;
    uint32_t timestamp;     // millis()
    int16_t  temperature;   // °C ×100
    uint16_t humidity;      // % ×100
    uint16_t distance_cm;   // cm
    uint8_t  battery;       // 0..100
    uint16_t reserved;      // alinhamento / uso futuro
    uint8_t  checksum;      // ÚLTIMO BYTE
};

/**
 * @brief Heartbeat (8 bytes).
 */
struct __attribute__((packed)) HeartbeatMessage {
    uint8_t  msg_type;
    uint8_t  client_id;
    uint32_t timestamp;     // millis()
    uint8_t  status;        // flags
    uint8_t  checksum;      // último
};

/**
 * @brief Alerta (12 bytes).
 */
struct __attribute__((packed)) AlertMessage {
    uint8_t  msg_type;
    uint8_t  client_id;
    uint32_t timestamp;     // millis()
    uint8_t  alert_code;    // tipo
    int16_t  alert_value;   // valor
    uint8_t  severity;      // 0..255
    uint8_t  reserved;
    uint8_t  checksum;      // último
};

// =====================================================
// Helpers
// =====================================================

inline uint8_t calculate_checksum(const uint8_t* data, size_t length) {
    // XOR de todos os bytes exceto o último (checksum)
    uint8_t c = 0;
    for (size_t i = 0; i + 1 < length; ++i) c ^= data[i];
    return c;
}

inline bool verify_checksum(const uint8_t* data, size_t length) {
    if (length == 0) return false;
    return calculate_checksum(data, length) == data[length - 1];
}

inline int16_t  encode_temperature(float t) { return (int16_t)(t * 100); }
inline float    decode_temperature(int16_t t){ return t / 100.0f; }
inline uint16_t encode_humidity(float h)    { return (uint16_t)(h * 100); }
inline float    decode_humidity(uint16_t h) { return h / 100.0f; }

#endif // PROTOCOL_H
