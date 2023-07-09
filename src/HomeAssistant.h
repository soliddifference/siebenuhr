#ifndef _7U_HOMEASSISTANT_H
#define _7U_HOMEASSISTANT_H

#include <Arduino.h>
#include "SiebenUhr.h"

#include <ArduinoHA.h>
#include "HATextExt.h"



#include <WiFi.h>

#include "DisplayDriver.h"


namespace siebenuhr {

class HomeAssistant {

public:
  HomeAssistant(IPAddress ipAddress, String mqttBrokerUsername,String mqttBrokerPassword);

  ~HomeAssistant()  = default;

  static void onBrightnessCommand(uint8_t brightness, HALight* sender);
  static void onStateCommand(bool state, HALight* sender);
  static void onColorTemperatureCommand(uint16_t temperature, HALight* sender);
  static void onRGBColorCommand(HALight::RGBColor color, HALight* sender);
  static void onSelectCommand(int8_t index, HASelect* sender);
  static void onTextCommand(String text, HATextExt* sender);

  bool setup();
  void update();

private:
    WiFiClient client;
    HADevice *_haDevice;
    HAMqtt *_mqtt;
    HALight *_light;
    HASelect *_color_mode; 
    HATextExt *_text;

    IPAddress _iMQTTBrokerIPAddress;
    char _sMQTTBrokerUsername[EEPROM_ADDRESS_MAX_LENGTH];
    char _sMQTTBrokerPassword[EEPROM_ADDRESS_MAX_LENGTH];
};

}

#endif