#include <Arduino.h>
#include "HomeAssistant.h"

#include <WiFi.h>
#include <Controller.h>

using namespace siebenuhr;

HomeAssistant::HomeAssistant()
{

}
  
void HomeAssistant::onStateCommand(bool state, HALight* sender) {
    //Serial.print("State: ");
    //Serial.println(state);
    Controller::getInstance()->getDisplayDriver()->setPower(state);
    sender->setState(state); // report state back to the Home Assistant
}

void HomeAssistant::onBrightnessCommand(uint8_t brightness, HALight* sender) {
    //Serial.print("Brightness: ");
    //Serial.println(brightness);
    Controller::getInstance()->getDisplayDriver()->setBrightness(brightness);
    sender->setBrightness(brightness); // report brightness back to the Home Assistant
}

void HomeAssistant::onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
    Serial.print("Red: ");
    Serial.println(color.red);
    Serial.print("Green: ");
    Serial.println(color.green);
    Serial.print("Blue: ");
    Serial.println(color.blue);

    sender->setRGBColor(color); // report color back to the Home Assistant
}


void HomeAssistant::setup() 
{
    _mqtt = new HAMqtt(client, device);   
    // Unique ID must be set!
    byte mac[8];
    WiFi.macAddress(mac);
    device.setUniqueId(mac, sizeof(mac));

    device.setName("siebuhr-F2:A3");
    device.setSoftwareVersion("1.0.0");
    device.setModel("Siebenuhr");
    device.setManufacturer("Solid Difference");

    #define BROKER_ADDR     IPAddress(10,0,0,10)
    #define BROKER_USERNAME     "" // replace with your credentials
    #define BROKER_PASSWORD     ""

    _mqtt->begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);

    _light = new HALight("siebenuhr-0001", HALight::BrightnessFeature | HALight::RGBFeature);
    _light->setName("siebenuhr-0001");
    _light->onStateCommand(onStateCommand);
    _light->onBrightnessCommand(onBrightnessCommand); // optional
    _light->onRGBColorCommand(onRGBColorCommand); // optional

}

void HomeAssistant::init() {
    _mqtt->loop();
    _light->setState(1);
    _light->setBrightness(Controller::getInstance()->getDisplayDriver()->getBrightness());
}



void HomeAssistant::update() 
{
    _mqtt->loop();

}