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
  STOP,
  FAULT
};

typedef struct __attribute__((packed)) SensorData {
  uint8_t start_marker;
	uint32_t lidar_distance;
  float roll, pitch;
	float thermistors[8];
	float pt_up, pt_down;
  float lv_batt;

  // VFD
	uint8_t drive_direction,
	error_code;

	double encoder_speed,
	batt_voltage,
	motor_curr,
	motor_temp,
	controller_temp;

	// BMS
	uint8_t relay_status, bms_test_counter;
	double lowest_cell_volt,
	highest_cell_volt,
	pack_soc,
	highest_temp,
	pack_volt,
	lowest_temp;

	uint8_t dis_en_status;
         
  uint8_t pod_state;
	char message[100];
} SensorData;


/* 
  Variables 
*/
// 68:fe:71:90:76:88  <-- Laptop
// 68:fe:71:90:76:90  <-- STM32
uint8_t broadcastAddress[] = {0x68, 0xFE, 0x71, 0x90, 0x76, 0x90};

esp_now_peer_info_t peerInfo;
const uint8_t START_BYTE = 0xAA;

SensorData telemetryData;
GUICommand command = GUICommand::NONE;

unsigned long lastHeartbeatESP = 0;
unsigned long lastSendTime = 0;
const unsigned long timeoutMs = 2000;
unsigned long lastLoop = 0;

bool sentESTOP = false;

/* 
  Function Definitions 
*/
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Check signal strength
  // wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)(incomingData - sizeof(wifi_pkt_rx_ctrl_t) - 12);
  // int rssi = pkt->rx_ctrl.rssi;
  static bool senderLocked = false;
  static uint8_t knownSender[6];

  if (len != sizeof(SensorData)) return;

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

  // Serial.print("RSSI: ");
  // Serial.println(rssi);
  memcpy(&telemetryData, incomingData, sizeof(telemetryData));
  lastHeartbeatESP = millis();

  if (strcmp(telemetryData.message, "ESTOP") == 0) {
    Serial.println("{\"msg\":\"ESTOP1\"}");
    sentESTOP = true;
    return;
  }

  sentESTOP = false;
  
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
  
  //VFD
  Serial.print(",\"drive_direction\":"); Serial.print(telemetryData.drive_direction);
  Serial.print(",\"error_code\":"); Serial.print(telemetryData.error_code);

  Serial.print(",\"encoder_speed\":"); Serial.print(telemetryData.encoder_speed, 2);
  Serial.print(",\"batt_voltage\":"); Serial.print(telemetryData.batt_voltage, 2);
  Serial.print(",\"motor_curr\":"); Serial.print(telemetryData.motor_curr, 2);
  Serial.print(",\"motor_temp\":"); Serial.print(telemetryData.motor_temp, 2);
  Serial.print(",\"controller_temp\":"); Serial.print(telemetryData.controller_temp, 2);

  // BMS
  Serial.print(",\"relay_status\":"); Serial.print(telemetryData.relay_status);
  Serial.print(",\"bms_test_counter\":"); Serial.print(telemetryData.bms_test_counter);
  Serial.print(",\"lowest_cell_volt\":"); Serial.print(telemetryData.lowest_cell_volt, 2);
  Serial.print(",\"highest_cell_volt\":"); Serial.print(telemetryData.highest_cell_volt, 2);
  Serial.print(",\"pack_soc\":"); Serial.print(telemetryData.pack_soc, 2);
  Serial.print(",\"highest_temp\":"); Serial.print(telemetryData.highest_temp, 2);
  Serial.print(",\"pack_volt\":"); Serial.print(telemetryData.pack_volt, 2);
  Serial.print(",\"lowest_temp\":"); Serial.print(telemetryData.lowest_temp, 2);

  Serial.print(",\"dis_en_status\":"); Serial.print(telemetryData.dis_en_status);

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
    Serial.println("Error initializing ESP-NOW. Restarting now.");
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
 
  // TODO: esp_task_wdt_init(2, true);
  // TODO: esp_task_wdt_add(NULL);
}

void loop() {
  // TODO: esp_task_wdt_reset();
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
        command = (GUICommand)parsed;
      }
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&command, sizeof(command));
      if (result != ESP_OK) {
        Serial.println("Message was not queued for transmission");
      }
      lastSendTime = millis();
    }
  }

  // THIS heartbeat
  unsigned long now = millis();
  if (now - lastSendTime >= 1500) {
    lastSendTime = now;
    command = GUICommand::NONE;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&command, sizeof(command));
    if (result != ESP_OK) {
      Serial.println("Message was not queued for transmission");
    }
  }

  if (sentESTOP != true && (millis()-lastHeartbeatESP) > timeoutMs) {
    Serial.println("{\"msg\":\"ESTOP2\"}");
    sentESTOP = true;
    return;
  }
}
