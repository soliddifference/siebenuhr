#include <Arduino.h>
#include "controller.h"

siebenuhr::Controller *g_controller = nullptr;

void setup() {
    Serial.begin(115200);

    g_controller = new siebenuhr::Controller();

    g_controller->setLogLevel(static_cast<int>(siebenuhr_core::CoreLogLevel::DEBUG));
    g_controller->initialize(siebenuhr_core::ClockType::CLOCK_TYPE_MINI);
    g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_MOSAIK);   

    if (!g_controller->initializeWifi(true)) 
    {
        LOG_E("7Uhr wifi setup failed.");
        return;
    }

    if (!g_controller->initializeNTP(true))
    {
        LOG_E("7Uhr NTP setup failed.");    
        return;
    }        
}

void loop() {
    if (g_controller != nullptr) {
        g_controller->update();
    }
}
