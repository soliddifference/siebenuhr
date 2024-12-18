#include <Arduino.h>
#include "SiebenUhr.h"

#include <WiFi.h>
#include <ezTime.h>

#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>

#include "Controller.h" 
#include "DisplayDriver.h"

// #include <siebenuhr/controller.h>
#include <siebenuhr_controller.h>

DisplayDriver Display;

void setup() {
    siebenuhr_core::Controller *pController = siebenuhr_core::Controller::getInstance();
    if (pController != nullptr) 
    {
        #ifdef SIEBENUHR_MINI
            pController->initialize(1);
        #else
            pController->initialize(0);
        #endif            
    }

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

    if (!_cntrl->initializeWifi(true)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr wifi setup failed.");
        return;
    };

    if (!_cntrl->initializeNTP(true)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr NTP setup failed.");
        return;
    };

    _cntrl->begin();
}

void loop() {
    siebenuhr_core::Controller *pController = siebenuhr_core::Controller::getInstance();
    if (pController != nullptr) 
    {
        pController->update();
    }

    siebenuhr::Controller::getInstance()->update();
}