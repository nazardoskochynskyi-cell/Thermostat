#include <SPI.h>
#include <TFT_eSPI.h>
#include <DHT.h>
#include <ESP32Encoder.h>
#include "Valve_Comms.h"

#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#include "Montserrat_Regular18.h"
#include "Montserrat_Regular30.h"

#define FONT NotoSansBold36
#define FONT_ NotoSansBold15
#define FONTT Montserrat_Regular18
#define FONT__ Montserrat_Regular30
#define DHTPIN 17
#define DHTTYPE DHT11
#define ENC_CLK 21
#define ENC_DT 19
#define ENC_SW 5

TFT_eSPI tft = TFT_eSPI();
DHT dht(DHTPIN, DHTTYPE);
ESP32Encoder encoder;

float setpointTemperature = 20.0;
float previousSetpoint = -999.0;
static uint32_t dhtTimer = 0;

float lastReadTemperature = -999.0;
float lastReadHumidity = -999.0;
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

  initCommunication();

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
  tft.setCursor(35, 150);
  tft.print("У приміщенні:");
  tft.setCursor(120, 150);
  tft.print("Вологість:");
  tft.setCursor(100, 80);
  tft.print("Задана:");

  tft.loadFont(FONT_);
  tft.setTextPadding(150);
  char setpointString[10];
  dtostrf(setpointTemperature, 4, 1, setpointString);
  strcat(setpointString, "°");
  tft.drawString(setpointString, 75, 90);
  previousSetpoint = setpointTemperature;
}

void loop() {
  bool currentButtonState = digitalRead(ENC_SW);
  if (currentButtonState == LOW && lastButtonState == HIGH && (millis() - lastButtonPressTime > debounceDelay)) {
    isEditMode = !isEditMode;
    if (isEditMode) {
      lastEditTime = millis();
    } else {
      sendValveCommand(setpointTemperature);
    }
    lastButtonPressTime = millis();
  }
  lastButtonState = currentButtonState;

  if (isEditMode && (millis() - lastEditTime > EDIT_TIMEOUT)) {
    isEditMode = false;
    sendValveCommand(setpointTemperature);
  }

  if (isEditMode) {
    long currentCount = encoder.getCount();
    const long minCount = 36;
    const long maxCount = 64;

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
    }
  }

  if (millis() - dhtTimer > 2000) {
    dhtTimer = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      isTempError = true;
    } else if (t == lastReadTemperature) {
      if (millis() - lastTempChangeTime > TEMP_TIMEOUT) {
        isTempError = true;
      }
    } else {
      isTempError = false;
      lastReadTemperature = t;
      lastTempChangeTime = millis();
      lastReadHumidity = h; 
    }

    if (isTempError) {
      tft.loadFont(FONTT);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setTextPadding(150);
      tft.drawString("DHT ERROR", 80, 260);
    } else {
      tft.loadFont(FONT__);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(150);
      char tempString[10];
      dtostrf(lastReadTemperature, 4, 1, tempString);
      strcat(tempString, "°");
      tft.drawString(tempString, 35, 160);

      tft.loadFont(FONT__);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(150);
      char humString[10];
      dtostrf(lastReadHumidity, 4, 1, humString);
      strcat(humString, "%");
      tft.drawString(humString, 120, 160);
    }
  }

  char setpointString[10];
  dtostrf(setpointTemperature, 4, 1, setpointString);
  strcat(setpointString, "°");

  if (isEditMode) {
    if ((millis() / 400) % 2) {
      tft.loadFont(FONT_);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(150);
      tft.drawString(setpointString, 75, 90);
    } else {
      tft.fillRect(75, 90, 150, 40, TFT_BLACK);
    }
  } else {
    tft.loadFont(FONT_);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextPadding(150);
    tft.drawString(setpointString, 75, 90);
  }
}