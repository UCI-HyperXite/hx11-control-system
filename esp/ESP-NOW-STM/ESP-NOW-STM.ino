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
  uint8_t pod_state;
	char message[100];
} SensorData;


/* ESP32 - STM32 */
// receives stm32 data from stm32
  // normal priority
// send gui command to stm32
  // high priority
// send stm32 data to esp32
  // normal priority
// receives gui command from esp32
  // high priority


/* 
  Variables 
*/
uint8_t broadcastAddress[] = {0x68, 0xFE, 0x71, 0x90, 0x76, 0x88};
// 68:fe:71:90:76:88  <-- Laptop
// 68:fe:71:90:76:90  <-- STM32
esp_now_peer_info_t peerInfo;
const uint8_t START_BYTE = 0xAA;

SensorData buffer[5];
int head = 0;

unsigned long lastHeartbeatSTM = 0;
unsigned long lastHeartbeatESP = 0;
const unsigned long timeoutMs = 2000;
GUICommand cmd = GUICommand::NONE;
SensorData telemetryData;
GUICommand guiCMD = GUICommand::NONE;

unsigned long lastLoop = 0;

/* 
  Function Definitions 
*/

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  Serial.println(">>> PACKET RECEIVED <<<");
  lastHeartbeatESP = millis();
  if (len == sizeof(uint8_t)) {
    uint8_t receivedByte = *incomingData;
    Serial2.write(receivedByte);
    Serial.print("Was sent this command: ");
    Serial.println(receivedByte);
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
    // TODO: Return fatal error to GUI
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
  }

  // esp_task_wdt_init(2, true);
  // esp_task_wdt_add(NULL);

}

bool stmTimedOut = false;
int counter = 0;
void loop() {
  /* ESP32 - STM32 */
  // esp_task_wdt_reset();

  if (millis() - lastLoop < 50) {
    return;
  }

  lastLoop = millis();

  if (readPacket()) {
    lastHeartbeatSTM = millis();
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&telemetryData, sizeof(SensorData));

    if (result != ESP_OK) {
      Serial.println("Send queue failed");
    }

    Serial.println("Packet received from STM32!");

    Serial.print("Lidar: ");
    Serial.print(telemetryData.lidar_distance);
    Serial.print(" | State: ");
    Serial.println(telemetryData.pod_state);
    
    Serial.print("Temps: ");
    for(int i=0; i<8; i++) {
      Serial.print(telemetryData.thermistors[i], 2); 
      Serial.print(" ");
    }
    Serial.println("\n-------------------------");

    // // Serial2.write((uint8_t)GUICommand::LOAD);
    // Serial2.write(2);
  } else {
    counter += 1;
    if (counter %5 == 0) {
      Serial.println("No serial available");
    }
  }

  if ((millis()-lastHeartbeatSTM) > timeoutMs) {
    Serial.println("STM32 TIMEOUT");

    if (!stmTimedOut) {
      stmTimedOut = true;
      Serial.println("STM32 TIMEOUT");
      uint8_t stopCmd = (uint8_t)GUICommand::STOP;
      Serial2.write(stopCmd);
    }

    // TODO: Close the breaks somehow
    // TODO: Inform the GUI
    // Serial.println("STM32 has not responded in " + String(millis()-lastHeartbeatSTM) + " ms");  // TODO: Remove this
  } else {
    stmTimedOut = false;
  }

  
  // if (result != ESP_OK) {
  //   // TODO: ERROR HANDLING
  //   Serial.println("Message was not queued for transmission");
  // }

  if ((millis()-lastHeartbeatESP) > timeoutMs) {
    // Serial.println("ESP-NOW TIMEOUT");
    uint8_t stopCmd = (uint8_t)GUICommand::STOP;
    Serial2.write(stopCmd);

    // TODO: Close the breaks somehow
    // Send message to STM32 to close the breaks bcuz lost connection
    Serial.println("ESP32 has not responded in " + String(millis()-lastHeartbeatESP) + " ms");  // TODO: Remove this
  }
}
