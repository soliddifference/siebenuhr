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
        siebenuhr::Controller::getInstance()->debugMessage("Homeassistant address empty!");
    }
}
  
void HomeAssistant::onStateCommand(bool state, HALight* sender) {
    siebenuhr::Controller::getInstance()->debugMessage("Turning Siebenuhr through HA to %3d", state);

    //Serial.print("State: ");
    //Serial.println(state);
    // FIXME: We still need to implement an 'Off' mode for the clock which should be triggered from here
    //if(!state) {
    Controller::getInstance()->getDisplayDriver()->setPower(state);
    //}
    sender->setState(state); // report state back to the Home Assistant
}

void HomeAssistant::onBrightnessCommand(uint8_t brightness, HALight* sender) {
    siebenuhr::Controller::getInstance()->debugMessage(formatString("Setting brightness through HA to %3d", brightness));
    Controller::getInstance()->getDisplayDriver()->setBrightness(brightness);
    sender->setBrightness(brightness); // report brightness back to the Home Assistant
}

void HomeAssistant::onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
	siebenuhr::Controller::getInstance()->debugMessage(formatString("Setting RGB color through HA to R:%3d G:%3d B:%3d", color.red, color.green, color.blue));
    CRGB _newColor;
    _newColor.red   = color.red;
    _newColor.green = color.green;
    _newColor.blue  = color.blue;
    siebenuhr::Controller::getInstance()->getDisplayDriver()->setColor(_newColor, 0);
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

    sender->setState(""); // report the selected option back to the HA panel
    //Serial.print("Text changed to: ");
    
    //Serial.println(text);
    Controller::getInstance()->getDisplayDriver()->setNotification(text, 5000);
}

bool HomeAssistant::setup() {
    if(!_iMQTTBrokerIPAddress) {
        return false;
    }

    // Unique ID must be set!

    // FIXME: For some odd reason mqtt / HA doesn't work if we provide the
    // serial number this way, even I don't spot the difference between manually 
    // providing it or through the serial number function...
    // 
    // uint16_t intserial = siebenuhr::Controller::getInstance()->getSerialNumber();
    // Serial.print("Serial: ");
    // Serial.println(intserial);

    // const char* d = "30279";

    // const uint16_t serial = 30279;
    // char buffer[16];  // Adjust the buffer size as per your requirements
    // sprintf(buffer, "%hu", serial);
    // const char* charPtr = buffer;


    // Serial.print("Serial: ");
    // Serial.print(strlen(charPtr));
    // Serial.print(" ");
    // Serial.println(charPtr);
    // Serial.print("Serial: ");
    // Serial.print(strlen(d));
    // Serial.print(" ");
    // Serial.println(d);

    const char* _serial = "30279";

    _haDevice = new HADevice(_serial);
    _mqtt = new HAMqtt(client, *_haDevice);   

    _haDevice->setName("siebuhr-30279");
    _haDevice->setSoftwareVersion("1.0.0");
    _haDevice->setModel("Siebenuhr");
    _haDevice->setManufacturer("Solid Difference");

    _mqtt->begin(_iMQTTBrokerIPAddress, _sMQTTBrokerUsername, _sMQTTBrokerPassword);

    _light = new HALight("siebenuhr", HALight::BrightnessFeature | HALight::RGBFeature);
   
    _light->setName("siebenuhr");
    _light->onStateCommand(onStateCommand);
    _light->onBrightnessCommand(onBrightnessCommand); 
    _light->onRGBColorCommand(onRGBColorCommand); 
    
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
    
    _color_mode->setState(Controller::getInstance()->getDisplayDriver()->getDisplayEffect());

    return _mqtt->isConnected(); 
}

void HomeAssistant::update() {
    if(_iMQTTBrokerIPAddress) {
        _mqtt->loop();
    }
}