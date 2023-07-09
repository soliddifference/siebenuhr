#include <Arduino.h>
#include "HomeAssistant.h"

#include <WiFi.h>
#include <Controller.h>

using namespace siebenuhr;

HomeAssistant::HomeAssistant(IPAddress ipAddress, String mqttBrokerUsername,String mqttBrokerPassword) 
{
    //mqttBrokerUsername.c_str(_sMQTTBrokerUsername);
    strcpy(_sMQTTBrokerUsername, mqttBrokerUsername.c_str());
    strcpy(_sMQTTBrokerPassword, mqttBrokerPassword.c_str());
    
    // _sMQTTBrokerPassword* = mqttBrokerPassword;
    _iMQTTBrokerIPAddress = ipAddress;
    if(!_iMQTTBrokerIPAddress) {
         Serial.println("Address empty!");
    }
}


HomeAssistant::HomeAssistant()
{

}
  
void HomeAssistant::onStateCommand(bool state, HALight* sender) {
    //Serial.print("State: ");
    //Serial.println(state);
    // FIXME: We still need to implement an 'Off' mode for the clock which should be triggered from here
    // Controller::getInstance()->getDisplayDriver()->setPower(state);
    sender->setState(state); // report state back to the Home Assistant
}

void HomeAssistant::onBrightnessCommand(uint8_t brightness, HALight* sender) {
    Serial.print("Brightness: ");
    Serial.println(brightness);
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

void HomeAssistant::onSelectCommand(int8_t index, HASelect* sender)
{
    switch (index) {
    case 0:
        // Option "Color Wheel" was selected
        siebenuhr::Controller::getInstance()->debugMessage("Display effect switched to Color Wheel");
        if(siebenuhr::Controller::getInstance()->getDisplayDriver()->getDisplayEffect()!=DISPLAY_EFFECT_DAYLIGHT_WHEEL) {
            siebenuhr::Controller::getInstance()->getDisplayDriver()->setDisplayEffect(DISPLAY_EFFECT_DAYLIGHT_WHEEL);
        };
        break;

    case 1:
        // Option "Fixed color" was selected
        siebenuhr::Controller::getInstance()->debugMessage("Display effect switched to Fixed Color");
        if(siebenuhr::Controller::getInstance()->getDisplayDriver()->getDisplayEffect()!=DISPLAY_EFFECT_SOLID_COLOR) {
            siebenuhr::Controller::getInstance()->getDisplayDriver()->setDisplayEffect(DISPLAY_EFFECT_SOLID_COLOR);
        };
        break;

    case 2:
        // Option "Random Color" was selected
        siebenuhr::Controller::getInstance()->debugMessage("Display effect switched to Random Color");
        if(siebenuhr::Controller::getInstance()->getDisplayDriver()->getDisplayEffect()!=DISPLAY_EFFECT_RANDOM_COLOR) {
            siebenuhr::Controller::getInstance()->getDisplayDriver()->setDisplayEffect(DISPLAY_EFFECT_RANDOM_COLOR);
        };
        break;

    default:
        // unknown option
        return;
    }

    sender->setState(siebenuhr::Controller::getInstance()->getDisplayDriver()->getDisplayEffect()); // report the selected option back to the HA panel
}

void HomeAssistant::onTextCommand(String text, HATextExt* sender)
{
    text = text.substring(0,4);
    sender->setState(text); // report the selected option back to the HA panel
    Serial.print("Text changed to: ");
    
    Serial.println(text);
    Controller::getInstance()->getDisplayDriver()->setNotification(text, 5000);
}

bool HomeAssistant::setup() 
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

    _mqtt->begin(_iMQTTBrokerIPAddress, _sMQTTBrokerUsername, _sMQTTBrokerPassword);

    _light = new HALight("siebenuhr-0001", HALight::BrightnessFeature | HALight::RGBFeature);
    _light->setName("siebenuhr-0001");
    _light->onStateCommand(onStateCommand);
    _light->onBrightnessCommand(onBrightnessCommand); // optional
    _light->onRGBColorCommand(onRGBColorCommand); // optional
    
    _color_mode = new HASelect("SiebenuhrColorMode");
    _color_mode->setName("SiebenuhrColorMode"); // optional
    _color_mode->onCommand(onSelectCommand);
    _color_mode->setOptions("Color Wheel;Fixed Color;Random Color"); 
    _color_mode->setCurrentState(siebenuhr::Controller::getInstance()->getDisplayDriver()->getDisplayEffect());
    _color_mode->setIcon("mdi:apple-keyboard-option"); // optional

    _text = new HATextExt("Notification");
    _text->setName("Notification");
    _text->onTextCommand(onTextCommand);

    // setup complete, let's start the mqtt-client
    _mqtt->loop();
    _light->setState(1);
    _light->setBrightness(Controller::getInstance()->getDisplayDriver()->getBrightness());
    
    _color_mode->setState(1);

    return _mqtt->isConnected(); 
}

void HomeAssistant::update() 
{
    _mqtt->loop();
}