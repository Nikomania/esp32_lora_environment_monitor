/**
 * @file protocol.h
 * @brief Define o protocolo binário compacto utilizado na comunicação LoRa
 *        entre os nós sensores (clients) e o gateway.
 *
 * @details
 * Este protocolo define diferentes tipos de mensagens, todas transmitidas
 * no formato LITTLE-ENDIAN, otimizadas para redes LoRa de baixa taxa de dados.
 * O principal pacote (SensorDataMessage) contém medições de temperatura,
 * umidade e distância, sendo o formato padrão utilizado no projeto.
 *
 * Layout (LITTLE-ENDIAN on ESP32)
 *  Offset  Size  Field
 *  0       1     msg_type
 *  1       1     client_id
 *  2       4     timestamp (millis)
 *  6       2     temperature (°C x100)
 *  8       2     humidity (% x100)
 *  10      2     distance_cm (uint16)
 *  12      1     battery (%)
 *  13      1     checksum (XOR of bytes [0..12])
 *  14      2     reserved
 *
 *  Total = 16 bytes
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// =====================================================
// Tipos de mensagem
// =====================================================

#define MSG_TYPE_SENSOR_DATA 0x01  ///< Dados de sensores (mensagem principal)
#define MSG_TYPE_HEARTBEAT   0x02  ///< Sinal periódico de vida do dispositivo
#define MSG_TYPE_ALERT       0x03  ///< Alerta de evento crítico
#define MSG_TYPE_ACK         0xAA  ///< Confirmação de recebimento (ACK)

// =====================================================
// Estruturas de mensagens
// =====================================================

/**
 * @struct SensorDataMessage
 * @brief Estrutura base de 16 bytes para envio de medições ambientais.
 *
 * @details
 * Usada pelo nó sensor para transmitir temperatura, umidade, distância e
 * nível de bateria. O checksum é calculado com XOR sobre os 13 primeiros bytes.
 */
struct __attribute__((packed)) SensorDataMessage {
    uint8_t  msg_type;     ///< Tipo de mensagem (MSG_TYPE_SENSOR_DATA)
    uint8_t  client_id;    ///< Identificador do nó (0–255)
    uint32_t timestamp;    ///< Tempo em milissegundos desde o boot (millis)
    int16_t  temperature;  ///< Temperatura em °C × 100
    uint16_t humidity;     ///< Umidade relativa em % × 100
    uint16_t distance_cm;  ///< Distância em centímetros
    uint8_t  battery;      ///< Percentual de bateria (0–100)
    uint8_t  checksum;     ///< XOR dos bytes [0..12]
    uint16_t reserved;     ///< Reservado para uso futuro (alinhamento)
};

/**
 * @struct HeartbeatMessage
 * @brief Mensagem curta de status geral do nó (8 bytes).
 *
 * @details
 * Enviada periodicamente para indicar que o nó continua ativo,
 * mesmo quando não há alterações significativas nas leituras.
 */
struct __attribute__((packed)) HeartbeatMessage {
    uint8_t  msg_type;    ///< Tipo de mensagem = MSG_TYPE_HEARTBEAT
    uint8_t  client_id;   ///< Identificador do nó
    uint32_t timestamp;   ///< Tempo em milissegundos desde o boot
    uint8_t  status;      ///< Flags de status (bitfield)
    uint8_t  checksum;    ///< Checksum simples (XOR)
};

/**
 * @struct AlertMessage
 * @brief Mensagem de evento crítico (12 bytes).
 *
 * @details
 * Enviada quando um valor medido ultrapassa limites definidos,
 * como temperatura alta, baixa umidade ou presença detectada.
 */
struct __attribute__((packed)) AlertMessage {
    uint8_t  msg_type;    ///< Tipo de mensagem = MSG_TYPE_ALERT
    uint8_t  client_id;   ///< Identificador do nó
    uint32_t timestamp;   ///< Tempo em milissegundos desde o boot
    uint8_t  alert_code;  ///< Código do tipo de alerta
    int16_t  alert_value; ///< Valor que gerou o alerta
    uint8_t  severity;    ///< Nível de severidade (0–255)
    uint8_t  checksum;    ///< Checksum simples (XOR)
    uint8_t  reserved;    ///< Reservado (para alinhamento futuro)
};

// =====================================================
// Status Flags (para HeartbeatMessage)
// =====================================================

#define STATUS_OK            0x00  ///< Operação normal
#define STATUS_LOW_BATTERY   0x01  ///< Bateria baixa
#define STATUS_SENSOR_ERROR  0x02  ///< Falha em sensor
#define STATUS_LORA_ERROR    0x04  ///< Falha de transmissão LoRa

// =====================================================
// Alert Codes (para AlertMessage)
// =====================================================

#define ALERT_TEMP_HIGH      0x10  ///< Temperatura acima do limite
#define ALERT_TEMP_LOW       0x11  ///< Temperatura abaixo do limite
#define ALERT_HUMIDITY_HIGH  0x20  ///< Umidade acima do limite
#define ALERT_HUMIDITY_LOW   0x21  ///< Umidade abaixo do limite
#define ALERT_DISTANCE_LOW   0x30  ///< Distância muito próxima (presença detectada)

// =====================================================
// Funções auxiliares
// =====================================================

/**
 * @brief Calcula o checksum XOR de um buffer.
 * @param data Ponteiro para os dados.
 * @param length Tamanho total do buffer.
 * @return Checksum de 8 bits.
 */
inline uint8_t calculate_checksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length - 1; i++) checksum ^= data[i];
    return checksum;
}

/**
 * @brief Verifica se o checksum de um pacote é válido.
 * @param data Ponteiro para os dados.
 * @param length Tamanho total do buffer.
 * @return true se o checksum for válido.
 */
inline bool verify_checksum(const uint8_t* data, size_t length) {
    return calculate_checksum(data, length) == data[length - 1];
}

/**
 * @brief Converte temperatura em °C para o formato codificado (×100).
 */
inline int16_t encode_temperature(float t) { return (int16_t)(t * 100); }

/**
 * @brief Decodifica temperatura codificada (×100) para °C.
 */
inline float decode_temperature(int16_t t) { return t / 100.0f; }

/**
 * @brief Converte umidade em % para o formato codificado (×100).
 */
inline uint16_t encode_humidity(float h) { return (uint16_t)(h * 100); }

/**
 * @brief Decodifica umidade codificada (×100) para %.
 */
inline float decode_humidity(uint16_t h) { return h / 100.0f; }

#endif // PROTOCOL_H
