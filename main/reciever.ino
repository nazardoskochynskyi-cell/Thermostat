#include <ESP8266WiFi.h>
#include <espnow.h>
#include <AccelStepper.h>

#define IN1 5 // D1
#define IN2 4 // D2
#define IN3 0 // D3
#define IN4 2 // D4
#define HALFSTEP 8

AccelStepper stepper(HALFSTEP, IN1, IN3, IN2, IN4);

typedef struct struct_message {
  float temperature;
} struct_message;

struct_message myData;

float currentTemp = 20.0; 
float targetTemp = 20.0;
const long STEPS_PER_DEGREE = 1000; 

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  targetTemp = myData.temperature;
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  stepper.setMaxSpeed(1000.0);
  stepper.setAcceleration(500.0);
}

void loop() {
  if (currentTemp != targetTemp) {
    long targetPosition = (long)((targetTemp - 20.0) * STEPS_PER_DEGREE);
    stepper.moveTo(targetPosition);
    
    if (stepper.distanceToGo() == 0) {
        currentTemp = targetTemp;
    }
  }
  stepper.run();
}