#include <Arduino.h>
#include "SiebenUhr.h"

#include <WiFi.h>
#include <ezTime.h>

#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>

#include "Controller.h" 
#include "DisplayDriver.h"

DisplayDriver Display;

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

void setup() {
    siebenuhr::Controller *_cntrl = siebenuhr::Controller::getInstance(); // just for convinience
    if (_cntrl == nullptr)
        return;

    _cntrl->initializeDebug(true);
    _cntrl->initializeEEPROM(/*true*/);

    Display.setup(_cntrl->getFirstTimeSetup());

    // _cntrl->setResetButton(RESET_BUTTON);
    _cntrl->setKnob(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN);

    if (!_cntrl->initializeDisplay(&Display)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr display setup failed.");
        return;
    }

    if (!_cntrl->initializeWifi(false, &wifiManager)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr wifi setup failed.");
        return;
    };

    if (!_cntrl->initializeNTP(false)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr NTP setup failed.");
        return;
    };

    _cntrl->begin();
}

void loop() {
  siebenuhr::Controller::getInstance()->update();
}