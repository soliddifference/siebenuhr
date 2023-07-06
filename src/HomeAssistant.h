#ifndef _7U_HOMEASSISTANT_H
#define _7U_HOMEASSISTANT_H

#include <Arduino.h>
#include "SiebenUhr.h"

#include <ArduinoHA.h>

#include <WiFi.h>

#include "DisplayDriver.h"


namespace siebenuhr {

class HomeAssistant {

public:
  HomeAssistant();
  ~HomeAssistant()  = default;

  static void onBrightnessCommand(uint8_t brightness, HALight* sender);
  static void onStateCommand(bool state, HALight* sender);
  static void onColorTemperatureCommand(uint16_t temperature, HALight* sender);
  static void onRGBColorCommand(HALight::RGBColor color, HALight* sender);

  void setup();
  void update();
  void init();

private:
    WiFiClient client;
    HADevice device;
    HAMqtt *_mqtt;
    HALight *_light;
};

}

#endif