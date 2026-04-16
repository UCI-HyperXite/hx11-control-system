#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "esp_task_wdt.h"

/* 
  Data Structures 
*/
enum class PodState:uint8_t {
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
  uint8_t pod_state;
	char message[100];
} SensorData;


/* 
  Variables 
*/
uint8_t broadcastAddress[] = {0x68, 0xFE, 0x71, 0x90, 0x76, 0x90};
// 68:fe:71:90:76:88  <-- Laptop
// 68:fe:71:90:76:90  <-- STM32
esp_now_peer_info_t peerInfo;
const uint8_t START_BYTE = 0xAA;

SensorData buffer[5];
int head = 0;

PodState podStatus = PodState::INIT;
SensorData telemetryData;  // Assign from ESP32 (other)
uint8_t command = 10;  // Assign from Laptop
unsigned long lastHeartbeatESP = 0;
unsigned long lastSendTime = 0;
const unsigned long timeoutMs = 2000;  // TODO: change from 2 seconds

unsigned long lastLoop = 0;

/* 
  Function Definitions 
*/
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // if (memcmp(mac, broadcastAddress, 6) != 0) {
  //   return;
  // }

  // Check signal strength
  // wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)(incomingData - sizeof(wifi_pkt_rx_ctrl_t) - 12);
  // int rssi = pkt->rx_ctrl.rssi;

  if (len == sizeof(SensorData)) {
    // Serial.print("RSSI: ");
    // Serial.println(rssi);
    memcpy(&telemetryData, incomingData, sizeof(telemetryData));
    lastHeartbeatESP = millis();
    
    // Web Serial printing (read as json in gui code)
    Serial.print("{");
    // Serial.print("\"RSSI\":"); Serial.print(rssi);
    Serial.print("\"lidar\":"); Serial.print(telemetryData.lidar_distance);
    Serial.print(",\"pod_state\":"); Serial.print(telemetryData.pod_state);
    Serial.print(",\"roll\":"); Serial.print(telemetryData.roll, 2);
    Serial.print(",\"pitch\":"); Serial.print(telemetryData.pitch, 2);
    
    // Add thermistors
    Serial.print(",\"therms\":["); 
    for(int i = 0; i < 8; i++) {
      Serial.print(telemetryData.thermistors[i], 2);
      if (i < 7) Serial.print(",");
    }
    Serial.print("]");

    // Pressure sensors
    Serial.print(",\"pt_up\":"); Serial.print(telemetryData.pt_up, 2);
    Serial.print(",\"pt_down\":"); Serial.print(telemetryData.pt_down, 2);

    // Batteries
    Serial.print(",\"lv_batt\":"); Serial.print(telemetryData.lv_batt, 2);
    Serial.print(",\"hv_batt_temp\":"); Serial.print(telemetryData.hv_batt_temp, 2);
    Serial.print(",\"hv_batt\":"); Serial.print(telemetryData.hv_batt, 2);

    Serial.print(",\"msg\":\"");
    telemetryData.message[99] = '\0';

    for (int i = 0; i < 100 && telemetryData.message[i] != '\0'; i++) {
      if (telemetryData.message[i] == '"') {
        Serial.print("\\\"");
      } else {
        Serial.print(telemetryData.message[i]);
      }
    }
    Serial.print("\"");

    Serial.println("}");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
      Serial.println("Delivery Fail");
  }
}

bool readPacket() {
  if (Serial2.available() == 0) return false;

  if (Serial2.peek() != START_BYTE) {
    Serial2.read();
    return false;
  }

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
  Serial.begin(115200);
  delay(100);

  // Set device as a Wi-Fi Station  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setTxPower(WIFI_POWER_11dBm);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);


  for (int i = 0; i < 3; i++) {
    if (esp_now_init() == ESP_OK) break;
    delay(100);
  }

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
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
  
  int retry = 3;
  while (retry--) {
    if (esp_now_add_peer(&peerInfo) == ESP_OK) break;
    delay(50);
  }
  if (retry < 0) {
    Serial.println("Cannot add ESP-NOW peer. Restarting...");
    ESP.restart();
  }
 
  // esp_task_wdt_init(2, true);
  // esp_task_wdt_add(NULL);

}

bool stmTimedOut = false;
void loop() {
  // esp_task_wdt_reset();

  if (millis() - lastLoop < 50) {
    return;
  }

  lastLoop = millis();

  // CONNECT to laptop serial
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      int parsed = input.toInt();
      if (parsed != 0 || input == "0") {
        command = (uint8_t)parsed;
      }
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&command, sizeof(command));
      if (result != ESP_OK) {
        Serial.println("Message was not queued for transmission");
      }
      lastSendTime = millis();
    }
  }

  unsigned long now = millis();
  if (now - lastSendTime >= 1000) {
    lastSendTime = now;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&command, sizeof(command));
    if (result != ESP_OK) {
      Serial.println("Message was not queued for transmission");
    }
  }

  if ((millis()-lastHeartbeatESP) > timeoutMs) {
    // TODO: Close the breaks somehow
    // Send message to STM32 to close the breaks bcuz lost connection
    // Serial.println("ESP32 has not responded in " + String(millis()-lastHeartbeatESP) + " ms");  // TODO: Remove this
  }

  
}
