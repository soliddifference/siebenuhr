#include <Arduino.h>
#include "HomeAssistant.h"

#include <WiFi.h>
#include <Controller.h>

using namespace siebenuhr;

HomeAssistant::HomeAssistant(const String mqttBrokerIPAddress, const String mqttBrokerUsername, const String mqttBrokerPassword) {
    _haDeviceSerial = formatString("Siebenuhr-%06d", Controller::getInstance()->getSerialNumber()); 

    strcpy(_sMQTTBrokerUsername, mqttBrokerUsername.c_str());
    strcpy(_sMQTTBrokerPassword, mqttBrokerPassword.c_str());   
    _iMQTTBrokerIPAddress.fromString(mqttBrokerIPAddress.c_str());
    if(!_iMQTTBrokerIPAddress) {
        siebenuhr::Controller::getInstance()->debugMessage("Homeassistant address empty!");
    }

    _haMQTT = nullptr;
    _haDevice = nullptr;

    _light = nullptr;
    _color_mode = nullptr; 
    _text = nullptr;
}
  
void HomeAssistant::onStateCommand(bool state, HALight* sender) {
    siebenuhr::Controller::getInstance()->debugMessage("Turning Siebenuhr through HA to %3d", state);
    Controller::getInstance()->getDisplayDriver()->setPower(state);
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
    siebenuhr::Controller::getInstance()->getDisplayDriver()->setColorRGB(_newColor, 0);
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

void HomeAssistant::onTextCommand(String text, HATextExt* sender) {
    text = text.substring(0,4);
    Controller::getInstance()->getDisplayDriver()->setNotification(text, 5000);
    sender->setState(text); // report the selected option back to the HA panel
}

bool HomeAssistant::setup() {
    if(!_iMQTTBrokerIPAddress) {
        return false;
    }

    siebenuhr::Controller::getInstance()->debugMessage("Registering MQTT device: %s", _haDeviceSerial.c_str());
    if (_haDevice == nullptr) {
        _haDevice = new HADevice(_haDeviceSerial.c_str());
    }
    if (_haMQTT == nullptr) {
        _haMQTT = new HAMqtt(client, *_haDevice, 16);   
    }

    _haDevice->setName(_haDeviceSerial.c_str());
    _haDevice->setSoftwareVersion("1.0.0");
    _haDevice->setModel("Siebenuhr");
    _haDevice->setManufacturer("Solid Difference");

    _haMQTT->begin(_iMQTTBrokerIPAddress, _sMQTTBrokerUsername, _sMQTTBrokerPassword);
    
    _haDisplayIdentifier = formatString("siebenuhr-%06d-display", Controller::getInstance()->getSerialNumber()); 
    _light = new HALight(_haDisplayIdentifier.c_str(), HALight::BrightnessFeature | HALight::RGBFeature);
    _light->setName("Display");
    _light->onStateCommand(onStateCommand);
    _light->onBrightnessCommand(onBrightnessCommand); 
    _light->onRGBColorCommand(onRGBColorCommand); 
    _light->setIcon("mdi:clock-digital");
    
    _haColorModeIdentifier = formatString("siebenuhr-%06d-colormode", Controller::getInstance()->getSerialNumber()); 
    _color_mode = new HASelect(_haColorModeIdentifier.c_str());
    _color_mode->setName("ColorMode"); // optional
    _color_mode->onCommand(onSelectCommand);
    _color_mode->setOptions("Color Wheel;Fixed Color;Random Color"); 
    _color_mode->setCurrentState(siebenuhr::Controller::getInstance()->getDisplayDriver()->getDisplayEffect());
    _color_mode->setIcon("mdi:apple-keyboard-option"); // optional

    _haNotificationIdentifier = formatString("siebenuhr-%06d-notification", Controller::getInstance()->getSerialNumber()); 
    _text = new HATextExt(_haNotificationIdentifier.c_str());
    _text->setName("Notification");
    _text->onTextCommand(onTextCommand);

    // setup complete, let's start the mqtt-client
    _haMQTT->loop();
    _light->setState(1);
    _light->setBrightness(Controller::getInstance()->getDisplayDriver()->getBrightness());
    
    _color_mode->setState(Controller::getInstance()->getDisplayDriver()->getDisplayEffect());

    return _haMQTT->isConnected(); 
}

void HomeAssistant::update() {
    if(_iMQTTBrokerIPAddress && _haMQTT) {
        _haMQTT->loop();
    }
}