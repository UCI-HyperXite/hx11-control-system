#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "esp_task_wdt.h"

/* 
  Data Structures 
*/
enum class PodState:uint8_t {
  GUI_OK,
	INIT,
	LOAD,
	PRECHARGE,
	START,
	STOP,
	FAULT,
	HALT
};

enum class GUICommand:uint8_t {
  NONE,
  OK,
  INIT,
  LOAD,
  START,
  STOP
};

typedef struct __attribute__((packed)) SensorData {
  uint8_t start_marker;
	uint32_t lidar_distance;
  float roll, pitch;
	float thermistors[8];
	float pt_up, pt_down;
	float lv_batt;
  float hv_batt_temp, hv_batt;
  float batt_soc;
  float lim_volt, lim_curr;
  float imd;             
  uint8_t pod_state;
	char message[100];
} SensorData;

/* 
  Variables 
*/
// 68:fe:71:90:76:88  <-- Laptop
// 68:fe:71:90:76:90  <-- STM32
uint8_t broadcastAddress[] = {0x68, 0xFE, 0x71, 0x90, 0x76, 0x88};

esp_now_peer_info_t peerInfo;
const uint8_t START_BYTE = 0xAA;

SensorData telemetryData;
GUICommand command = GUICommand::NONE;

unsigned long lastHeartbeatSTM = 0;
unsigned long lastHeartbeatESP = 0;
const unsigned long timeoutMs = 2000;
unsigned long lastLoop = 0;

bool stmTimedOut = false;
bool espTimedOut = false;

/* 
  Function Definitions 
*/

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  static bool senderLocked = false;
  static uint8_t knownSender[6];

  if (len != sizeof(uint8_t)) {
    Serial.println("Invalid packet size");
    return;
  }

  if (!senderLocked) {
    memcpy(knownSender, mac, 6);
    senderLocked = true;
    Serial.print("Sender locked: ");
    for (int i = 0; i < 6; i++) {
      if (mac[i] < 16) Serial.print("0");
      Serial.print(mac[i], HEX);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }

  // Filter for only our ESP's messages
  if (memcmp(mac, knownSender, 6) != 0) {
    Serial.println("Data received but not the correct address");
    return;
  }

  lastHeartbeatESP = millis();
  espTimedOut = false;
  uint8_t receivedByte = *incomingData;

  if (receivedByte > static_cast<uint8_t>(GUICommand::STOP)) {
    Serial.println("Invalid command value");
    return;
  }

  command = static_cast<GUICommand>(receivedByte);
  Serial2.write(receivedByte);

  Serial.print("Command received: ");
  Serial.println(receivedByte);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Fail");
  }
}

bool readPacket() {
  while (Serial2.available() > 0 && Serial2.peek() != START_BYTE) {
    Serial2.read();
  }

  if (Serial2.available() == 0) return false;
  if (Serial2.available() < sizeof(SensorData)) return false;

  Serial2.readBytes((uint8_t*)&telemetryData, sizeof(SensorData));

  if (telemetryData.start_marker != START_BYTE) {
    Serial.println("Bad packet alignment");
    return false;
  }
  return true;
}

void setup() {
  /* ESP32 - STM32 */
  // Initialize Serial Monitor
  Serial.begin(115200);  // TODO: Remove this later
  Serial2.begin(115200, SERIAL_8N1, 16, 17);  // (rx, tx)
  delay(100);

  // Set device as a Wi-Fi Station  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setTxPower(WIFI_POWER_11dBm);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    Serial2.write(-1);  // ERROR WITH ESP // FLASH ERROR LED LIGHTS
    ESP.restart();
    return;
  }
  
  esp_wifi_set_protocol(WIFI_IF_STA , WIFI_PROTOCOL_LR);

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  
  // Register peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 11;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_is_peer_exist(broadcastAddress)) {
    esp_now_del_peer(broadcastAddress);
  }
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    Serial2.write(-1);  // ERROR WITH ESP // FLASH ERROR LED LIGHTS
    ESP.restart();
    return;
  }

  // TODO: esp_task_wdt_init(2, true);
  // TODO: esp_task_wdt_add(NULL);

}

void loop() {
  /* ESP32 - STM32 */
  // TODO: esp_task_wdt_reset();

  if (millis() - lastLoop < 50) {
    return;
  }
  lastLoop = millis();

  if (readPacket()) {
    lastHeartbeatSTM = millis();
    stmTimedOut = false;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&telemetryData, sizeof(SensorData));

    if (result != ESP_OK) {
      // TODO: count number of fails before sending -1 to STM and restarting ESP
      Serial.println("Send queue failed");
    }
  } else {
    Serial.println("No serial data available.");
  }

  if ((millis()-lastHeartbeatSTM) > timeoutMs) {
    // TODO: What to do when STM doesn't send anything
    // Cases: GUI not connected  || STM frozen || Telemetry task blocked
    if (!stmTimedOut) {
      stmTimedOut = true;
      Serial.println("STM32 TIMEOUT");

      if (command != GUICommand::NONE) {
        SensorData eStopPacket = {};
        strcpy(eStopPacket.message, "ESTOP");
        eStopPacket.start_marker = START_BYTE;
        esp_now_send(broadcastAddress, (uint8_t *)&eStopPacket, sizeof(SensorData));
      }
    }
  } else {
    stmTimedOut = false;
  }

  if ((millis()-lastHeartbeatESP) > timeoutMs) {
    if (!espTimedOut) {
      espTimedOut = true;
      Serial.println("ESP-NOW TIMEOUT");
      command = GUICommand::NONE;
      Serial2.write((uint8_t)command);
    }
  } else { 
    espTimedOut = false;
  }
}
