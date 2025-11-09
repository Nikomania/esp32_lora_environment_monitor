/**
 * @file main.cpp
 * @brief LoRa Gateway - Receives sensor data and forwards to server
 *
 * Features:
 * - Continuously listens for LoRa messages from client nodes
 * - Decodes binary protocol messages
 * - Forwards data to server via HTTP POST or Serial
 * - Provides statistics on message reception
 * - Supports WiFi connectivity
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <RadioLib.h>
#include <time.h>
#include <sys/time.h>

#include "config.h"
#include "protocol.h"

#if ENABLE_WIFI
#include <HTTPClient.h>
#include <WiFi.h>
#endif

// =====================================================
// Global Variables
// =====================================================

// LoRa radio instance (SX1262)
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

// Reception statistics
uint32_t rx_count = 0;
uint32_t rx_valid = 0;
uint32_t rx_invalid = 0;
uint32_t rx_checksum_error = 0;

// Timing
uint32_t last_stats_time = 0;
uint32_t last_rx_time = 0;

#if TEST_MODE
uint32_t last_test_time = 0;
#endif

// WiFi status
bool wifi_connected = false;

// Time synchronization
bool time_synced = false;

// Receive buffer
uint8_t rx_buffer[MAX_PACKET_SIZE];

// =====================================================
// Function Declarations
// =====================================================

String get_iso8601_timestamp();

void setup_lora();
void setup_wifi();
void listen_for_messages();
void process_message(uint8_t* data, size_t length, float rssi, float snr);
void process_sensor_data(SensorDataMessage* msg, float rssi, float snr);
void forward_to_server(const char* json_data);
void print_statistics();
String message_to_json(SensorDataMessage* msg, float rssi, float snr);

#if TEST_MODE
void send_test_packet();
#endif

// =====================================================
// Setup
// =====================================================

void setup()
{
    // Initialize serial
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    DEBUG_PRINTLN("\n===========================================");
    DEBUG_PRINTLN("LoRa Gateway - Environmental Monitor");
    DEBUG_PRINTLN("===========================================");
    DEBUG_PRINTF("Gateway ID: %d\n", GATEWAY_ID);

    // Setup LoRa radio
    setup_lora();

// Setup WiFi if enabled
#if ENABLE_WIFI
    setup_wifi();
#endif

    DEBUG_PRINTLN("\nGateway ready - listening for messages...\n");
    last_stats_time = millis();
}

// =====================================================
// Main Loop
// =====================================================

void loop()
{
#if TEST_MODE
    // Send test packet periodically
    if (millis() - last_test_time >= TEST_INTERVAL_MS) {
        send_test_packet();
        last_test_time = millis();
    }
#else
    // Listen for incoming LoRa messages
    listen_for_messages();
#endif

    // Print statistics periodically
    if (millis() - last_stats_time >= STATS_INTERVAL_MS) {
        print_statistics();
        last_stats_time = millis();
    }

// Check WiFi connection periodically
#if ENABLE_WIFI
    if (!WiFi.isConnected() && wifi_connected) {
        DEBUG_PRINTLN("WiFi disconnected! Attempting to reconnect...");
        wifi_connected = false;
        setup_wifi();
    }
#endif
}

// =====================================================
// LoRa Setup
// =====================================================

void setup_lora()
{
    DEBUG_PRINTLN("\nInitializing LoRa radio...");
    DEBUG_PRINTF("Frequency: %.1f MHz\n", LORA_FREQUENCY);
    DEBUG_PRINTF("Bandwidth: %.1f kHz\n", LORA_BANDWIDTH);
    DEBUG_PRINTF("Spreading Factor: %d\n", LORA_SPREADING);

    // Initialize SX1262
    int state = radio.begin(
        LORA_FREQUENCY,
        LORA_BANDWIDTH,
        LORA_SPREADING,
        LORA_CODING_RATE,
        LORA_SYNC_WORD,
        10, // TX power (not used much in gateway)
        LORA_PREAMBLE);

    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN("✓ LoRa initialization successful");
    } else {
        DEBUG_PRINTF("✗ LoRa initialization failed, code: %d\n", state);
        DEBUG_PRINTLN("Check wiring and restart device");
        while (true) {
            delay(1000);
        }
    }

    // Start listening (non-blocking mode)
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("✗ Failed to start receive mode, code: %d\n", state);
    } else {
        DEBUG_PRINTLN("✓ Receive mode active");
    }
}

// =====================================================
// WiFi Setup
// =====================================================

void setup_wifi()
{
#if ENABLE_WIFI
    DEBUG_PRINTLN("\nConnecting to WiFi...");
    DEBUG_PRINTF("SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < WIFI_TIMEOUT_MS) {
        delay(500);
        DEBUG_PRINT(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        DEBUG_PRINTLN("\n✓ WiFi connected");
        DEBUG_PRINTF("IP Address: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("Server: http://%s:%d%s\n", SERVER_HOST, SERVER_PORT, SERVER_ENDPOINT);
        
        // Configure NTP for time synchronization
        DEBUG_PRINTLN("Synchronizing time with NTP...");
        configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // GMT-3 (Brasília)
        
        // Wait for time sync (max 5 seconds)
        int retry = 0;
        while (time(nullptr) < 100000 && retry < 10) {
            delay(500);
            DEBUG_PRINT(".");
            retry++;
        }
        
        if (time(nullptr) > 100000) {
            time_synced = true;
            DEBUG_PRINTLN("\n✓ Time synchronized");
            DEBUG_PRINTLN(get_iso8601_timestamp());
        } else {
            DEBUG_PRINTLN("\n✗ Time sync failed, using relative timestamps");
        }
    } else {
        wifi_connected = false;
        DEBUG_PRINTLN("\n✗ WiFi connection failed");
        DEBUG_PRINTLN("Continuing in Serial-only mode");
    }
#endif
}

// =====================================================
// Listen for LoRa Messages
// =====================================================

void listen_for_messages()
{
    // Check if a packet was received
    if (radio.getPacketLength() > 0) {
        int state = radio.readData(rx_buffer, MAX_PACKET_SIZE);

        if (state == RADIOLIB_ERR_NONE) {
            // Packet received successfully
            size_t length = radio.getPacketLength();
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();

            rx_count++;
            last_rx_time = millis();

            DEBUG_PRINTLN("\n╔═══════════════════════════════════════");
            DEBUG_PRINTF("║ Packet received (#%u)\n", rx_count);
            DEBUG_PRINTLN("╠═══════════════════════════════════════");
            DEBUG_PRINTF("║ Length: %d bytes\n", length);
            DEBUG_PRINTF("║ RSSI: %.2f dBm\n", rssi);
            DEBUG_PRINTF("║ SNR: %.2f dB\n", snr);
            DEBUG_PRINTLN("╚═══════════════════════════════════════");

            // Process the received message
            process_message(rx_buffer, length, rssi, snr);

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            DEBUG_PRINTLN("✗ CRC error in received packet");
            rx_invalid++;
        }

        // Restart listening for next packet
        radio.startReceive();
    }
}

// =====================================================
// Process Received Message
// =====================================================

void process_message(uint8_t* data, size_t length, float rssi, float snr)
{
    // Check minimum message length
    if (length < 1) {
        DEBUG_PRINTLN("✗ Message too short");
        rx_invalid++;
        return;
    }

    // Get message type
    uint8_t msg_type = data[0];

    DEBUG_PRINTF("Message type: 0x%02X\n", msg_type);

    // Process based on message type
    switch (msg_type) {
    case MSG_TYPE_SENSOR_DATA: {
        if (length == sizeof(SensorDataMessage)) {
            SensorDataMessage* msg = (SensorDataMessage*)data;

            // Calculate expected checksum for debugging
            uint8_t calculated = 0;
            for (size_t i = 0; i < length - 1; i++) {
                calculated ^= data[i];
            }
            uint8_t received = data[length - 1];
            
            DEBUG_PRINTF("Checksum - Calculated: 0x%02X, Received: 0x%02X\n", calculated, received);

            // Verify checksum
            if (verify_checksum(data, length)) {
                process_sensor_data(msg, rssi, snr);
                rx_valid++;
            } else {
                DEBUG_PRINTLN("✗ Checksum verification failed");
                rx_checksum_error++;
                rx_invalid++;
            }
        } else {
            DEBUG_PRINTF("✗ Invalid sensor data length: %d (expected %d)\n",
                length, sizeof(SensorDataMessage));
            rx_invalid++;
        }
        break;
    }

    case MSG_TYPE_HEARTBEAT: {
        if (length == sizeof(HeartbeatMessage)) {
            HeartbeatMessage* msg = (HeartbeatMessage*)data;
            if (verify_checksum(data, length)) {
                DEBUG_PRINTF("Heartbeat from client %d\n", msg->client_id);
                rx_valid++;
            } else {
                rx_checksum_error++;
                rx_invalid++;
            }
        }
        break;
    }

    case MSG_TYPE_ALERT: {
        if (length == sizeof(AlertMessage)) {
            AlertMessage* msg = (AlertMessage*)data;
            if (verify_checksum(data, length)) {
                DEBUG_PRINTF("⚠ Alert from client %d, code: 0x%02X\n",
                    msg->client_id, msg->alert_code);
                rx_valid++;
            } else {
                rx_checksum_error++;
                rx_invalid++;
            }
        }
        break;
    }

    default:
        DEBUG_PRINTF("✗ Unknown message type: 0x%02X\n", msg_type);
        rx_invalid++;
        break;
    }
}

// =====================================================
// Process Sensor Data
// =====================================================

void process_sensor_data(SensorDataMessage* msg, float rssi, float snr)
{
    // Decode sensor values
    float humidity = decode_humidity(msg->humidity);
    uint16_t distance = msg->distance_cm; // Distance in cm from ultrasonic sensor
    bool presence = (distance < 100); // Presence if distance < 100cm

    DEBUG_PRINTLN("\n┌─ Sensor Data ──────────────────────");
    DEBUG_PRINTF("│ Node ID: node-%d\n", msg->client_id);
    DEBUG_PRINTF("│ Timestamp: %u ms\n", msg->timestamp);
    DEBUG_PRINTF("│ Humidity: %.2f %%\n", humidity);
    DEBUG_PRINTF("│ Distance: %u cm\n", distance);
    DEBUG_PRINTF("│ Presence: %s\n", presence ? "DETECTED" : "No");
    DEBUG_PRINTF("│ Battery: %d %%\n", msg->battery);
    DEBUG_PRINTF("│ RSSI: %.1f dBm | SNR: %.1f dB\n", rssi, snr);
    DEBUG_PRINTLN("└────────────────────────────────────");

    // Convert to JSON and forward
    String json = message_to_json(msg, rssi, snr);

    DEBUG_PRINTLN("\nJSON payload:");
    DEBUG_PRINTLN(json);

// Forward to server
#if USE_SERIAL
    Serial.println("DATA:" + json); // Prefix for easy parsing
#endif

#if USE_HTTP && ENABLE_WIFI
    DEBUG_PRINTF("\n[HTTP] WiFi connected: %s\n", wifi_connected ? "YES" : "NO");
    if (wifi_connected) {
        forward_to_server(json.c_str());
    } else {
        DEBUG_PRINTLN("[HTTP] Skipping server forward - WiFi not connected");
    }
#else
    DEBUG_PRINTLN("[HTTP] HTTP or WiFi disabled in config");
#endif
}

// =====================================================
// Get ISO 8601 Timestamp
// =====================================================

String get_iso8601_timestamp()
{
    if (time_synced) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        
        struct tm timeinfo;
        localtime_r(&tv.tv_sec, &timeinfo);
        
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
        
        // Add milliseconds and timezone
        char timestamp[40];
        snprintf(timestamp, sizeof(timestamp), "%s.%03ldZ", buffer, tv.tv_usec / 1000);
        
        return String(timestamp);
    } else {
        // Fallback to relative timestamp if NTP not synced
        char buffer[30];
        snprintf(buffer, sizeof(buffer), "boot+%lu", millis());
        return String(buffer);
    }
}

// =====================================================
// Convert Message to JSON
// =====================================================

String message_to_json(SensorDataMessage* msg, float rssi, float snr)
{
    // Create JSON document
    JsonDocument doc;

    // Node identification
    char node_id[20];
    snprintf(node_id, sizeof(node_id), "node-%d", msg->client_id);
    doc["node_id"] = node_id;
    
    doc["gateway_id"] = GATEWAY_ID;
    
    // Use gateway's timestamp (ISO 8601 format when NTP synced)
    doc["timestamp"] = get_iso8601_timestamp();
    doc["client_timestamp"] = msg->timestamp; // Keep original client timestamp for reference

    // Sensor data - only humidity and presence (ultrasonic distance)
    JsonObject sensors = doc["sensors"].to<JsonObject>();
    sensors["humidity_percent"] = decode_humidity(msg->humidity);
    sensors["distance_cm"] = msg->distance_cm; // Distance from ultrasonic sensor
    
    // Determine presence based on distance (< 100cm = presence detected)
    sensors["presence_detected"] = (msg->distance_cm < 100);

    // Device status
    doc["battery_percent"] = msg->battery;

    // Radio metrics
    JsonObject radio = doc["radio"].to<JsonObject>();
    radio["rssi_dbm"] = rssi;
    radio["snr_db"] = snr;

    // Serialize to string
    String json;
    serializeJson(doc, json);

    return json;
}

// =====================================================
// Forward to Server via HTTP
// =====================================================

void forward_to_server(const char* json_data)
{
#if USE_HTTP && ENABLE_WIFI
    if (!wifi_connected) {
        DEBUG_PRINTLN("✗ Cannot forward: WiFi not connected");
        return;
    }

    HTTPClient http;

    String url = String("http://") + SERVER_HOST + ":" + SERVER_PORT + SERVER_ENDPOINT;

    DEBUG_PRINTF("\nSending to server: %s\n", url.c_str());
    DEBUG_PRINTF("Data: %s\n", json_data);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(json_data);

    if (httpCode > 0) {
        DEBUG_PRINTF("✓ Server response: %d ", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_ACCEPTED) {
            DEBUG_PRINTLN("OK");
            String response = http.getString();
            DEBUG_PRINTF("Response body: %s\n", response.c_str());
        } else {
            DEBUG_PRINTLN("ERROR");
        }
    } else {
        DEBUG_PRINTF("✗ HTTP POST failed: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
#endif
}

// =====================================================
// Print Statistics
// =====================================================

void print_statistics()
{
    DEBUG_PRINTLN("\n╔═══════════════════════════════════════");
    DEBUG_PRINTLN("║ Gateway Statistics");
    DEBUG_PRINTLN("╠═══════════════════════════════════════");
    DEBUG_PRINTF("║ Total packets: %u\n", rx_count);
    DEBUG_PRINTF("║ Valid packets: %u\n", rx_valid);
    DEBUG_PRINTF("║ Invalid packets: %u\n", rx_invalid);
    DEBUG_PRINTF("║ Checksum errors: %u\n", rx_checksum_error);

    if (rx_count > 0) {
        float success_rate = (float)rx_valid / rx_count * 100;
        DEBUG_PRINTF("║ Success rate: %.1f%%\n", success_rate);
    }

    if (last_rx_time > 0) {
        uint32_t time_since_rx = (millis() - last_rx_time) / 1000;
        DEBUG_PRINTF("║ Last RX: %u seconds ago\n", time_since_rx);
    } else {
        DEBUG_PRINTLN("║ No packets received yet");
    }

#if ENABLE_WIFI
    DEBUG_PRINTF("║ WiFi: %s\n", wifi_connected ? "Connected" : "Disconnected");
    if (wifi_connected) {
        DEBUG_PRINTF("║ IP: %s\n", WiFi.localIP().toString().c_str());
    }
#endif

    DEBUG_PRINTLN("╚═══════════════════════════════════════\n");
}

// =====================================================
// Test Mode - Simulate LoRa Packet Reception
// =====================================================

#if TEST_MODE
void send_test_packet()
{
    DEBUG_PRINTLN("\n[TEST MODE] Simulating LoRa packet reception...");
    
    // Create a simulated sensor data message
    SensorDataMessage test_msg;
    test_msg.msg_type = MSG_TYPE_SENSOR_DATA;
    test_msg.client_id = 42; // Test client ID
    test_msg.timestamp = millis() / 1000;
    
    // Generate random sensor values
    // Humidity: 30% to 80%
    test_msg.humidity = random(3000, 8000);
    
    // Distance: 5cm to 200cm (ultrasonic sensor range)
    // Values < 100cm will trigger presence detection
    test_msg.distance_cm = random(5, 200);
    
    // Temperature field not used, but set to 0 for consistency
    test_msg.temperature = 0;
    
    // Battery: 60% to 100%
    test_msg.battery = random(60, 100);
    test_msg.reserved = 0;
    
    // Calculate checksum using XOR (same as protocol.h)
    uint8_t* data = (uint8_t*)&test_msg;
    uint8_t checksum = 0;
    for (size_t i = 0; i < sizeof(SensorDataMessage) - 1; i++) {
        checksum ^= data[i];  // XOR, not addition
    }
    test_msg.checksum = checksum;
    
    // Simulate RSSI and SNR values
    float fake_rssi = random(-120, -30) / 1.0; // -120 to -30 dBm
    float fake_snr = random(-10, 10) / 1.0;    // -10 to 10 dB
    
    DEBUG_PRINTF("Test packet: Client=%d, Humidity=%.2f%%, Distance=%d cm\n",
                 test_msg.client_id,
                 test_msg.humidity / 100.0,
                 test_msg.distance_cm);
    DEBUG_PRINTF("Presence: %s\n", (test_msg.distance_cm < 100) ? "DETECTED" : "No");
    DEBUG_PRINTF("Checksum: 0x%02X | Message size: %d bytes\n", checksum, sizeof(SensorDataMessage));
    
    // Process the test message as if it was received via LoRa
    process_message((uint8_t*)&test_msg, sizeof(SensorDataMessage), fake_rssi, fake_snr);
}
#endif
