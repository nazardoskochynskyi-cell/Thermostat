#ifndef VALVE_COMMS_H
#define VALVE_COMMS_H

#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  float temperature;
} struct_message;

struct_message valveData;
esp_now_peer_info_t peerInfo;

void initCommunication() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    return;
  }
}

void sendValveCommand(float temp) {
  valveData.temperature = temp;
  esp_now_send(broadcastAddress, (uint8_t *) &valveData, sizeof(valveData));
}

#endif