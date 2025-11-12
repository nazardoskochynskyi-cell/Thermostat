#include <SPI.h>
#include <TFT_eSPI.h>
#include <DHT.h>
#include <ESP32Encoder.h>

#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#include "Montserrat_Medium48pt7b.h"

#define FONT  NotoSansBold36
#define FONT_ NotoSansBold15
#define FONTT Montserrat_Medium48pt7b
#define DHTPIN 17
#define DHTTYPE DHT11
#define ENC_CLK 21
#define ENC_DT 19
#define ENC_SW 5

// Ініціалізація
TFT_eSPI tft = TFT_eSPI();
DHT dht(DHTPIN, DHTTYPE);
ESP32Encoder encoder;

float setpointTemperature = 20.0;
float previousSetpoint = -999.0;
static uint32_t dhtTimer = 0;


float lastReadTemperature = -999.0;
unsigned long lastTempChangeTime = 0;
const unsigned long TEMP_TIMEOUT = 10000; 
bool isTempError = false;


bool isEditMode = false;
unsigned long lastEditTime = 0;
const unsigned long EDIT_TIMEOUT = 5000;
unsigned long lastButtonPressTime = 0;
bool lastButtonState = HIGH;
const long debounceDelay = 50;

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  dht.begin();

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  
  encoder.attachSingleEdge(ENC_CLK, ENC_DT);
  encoder.setCount((long)(setpointTemperature * 2.0));

  tft.loadFont(FONTT);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);


  tft.setCursor(65, 180);
  tft.print("Температура:");
  tft.setCursor(80, 100);
  tft.print("Setpoint:");

  tft.loadFont(FONT);
  tft.setTextPadding(150);
  char setpointString[10];
  dtostrf(setpointTemperature, 4, 1, setpointString); 
  strcat(setpointString, "\xB0C"); 
  tft.drawString(setpointString, 80, 120);
  previousSetpoint = setpointTemperature;
}

void loop() {
  

  bool currentButtonState = digitalRead(ENC_SW);
  if (currentButtonState == LOW && lastButtonState == HIGH && (millis() - lastButtonPressTime > debounceDelay)) {

    isEditMode = !isEditMode;
    if (isEditMode) {

      lastEditTime = millis();
    }
    lastButtonPressTime = millis();
  }
  lastButtonState = currentButtonState;

  if (isEditMode && (millis() - lastEditTime > EDIT_TIMEOUT)) {
    isEditMode = false;
  }

  if (isEditMode) {
    long currentCount = encoder.getCount();
    const long minCount = 36; // 18.0 * 2
    const long maxCount = 64; // 32.0 * 2

    if (currentCount < minCount) {
      currentCount = minCount;
      encoder.setCount(minCount);
    } else if (currentCount > maxCount) {
      currentCount = maxCount;
      encoder.setCount(maxCount);
    }
    setpointTemperature = currentCount / 2.0; 

    if (setpointTemperature != previousSetpoint) {
      lastEditTime = millis();
      previousSetpoint = setpointTemperature;
      Serial.print("New setpoint: ");
      Serial.println(setpointTemperature);
    }
  }

  if (millis() - dhtTimer > 2000) {
    dhtTimer = millis();
    float t = dht.readTemperature();

    if (isnan(t)) {
      isTempError = true;
    } else if (t == lastReadTemperature) {
      if (millis() - lastTempChangeTime > TEMP_TIMEOUT) {
        isTempError = true;
      }
    } else {
      isTempError = false;
      lastReadTemperature = t;
      lastTempChangeTime = millis();
    }
    
    if (isTempError) {
      tft.loadFont(FONT_); 
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setTextPadding(150); 
      tft.drawString("ERROR", 80, 200);
    } else {
      tft.loadFont(FONT); 
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(150); 
      char tempString[10];
      dtostrf(lastReadTemperature, 4, 1, tempString);
      strcat(tempString, ""); 
      tft.drawString(tempString, 80, 200);
    }
  }

  char setpointString[10]; 
  dtostrf(setpointTemperature, 4, 1, setpointString); 
  strcat(setpointString, ""); 

  if (isEditMode) {
    if ((millis() / 400) % 2) {
      tft.loadFont(FONT);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(150);
      tft.drawString(setpointString, 80, 120);
    } else {
      tft.fillRect(80, 120, 150, 40, TFT_BLACK); 
    }
  } else {
    tft.loadFont(FONT);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextPadding(150);
    tft.drawString(setpointString, 80, 120);
  }
}