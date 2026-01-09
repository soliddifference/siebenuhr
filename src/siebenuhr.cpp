#include <Arduino.h>
#include "controller.h"

siebenuhr::Controller *g_controller = nullptr;

// Build flags for optional features (set in platformio.ini)
#ifndef AUTO_BRIGHTNESS_ENABLED
#define AUTO_BRIGHTNESS_ENABLED 0
#endif

#ifndef POWER_MONITORING_ENABLED
#define POWER_MONITORING_ENABLED 0
#endif

#ifndef VERBOSE_LOGGING
#define VERBOSE_LOGGING 0
#endif

void setup() {
    Serial.begin(115200);

    g_controller = new siebenuhr::Controller();
    g_controller->initialize(siebenuhr_core::ClockType::CLOCK_TYPE_MINI);

    // Set log level after initialize() since Logger::init() resets it to INFO
    #if VERBOSE_LOGGING
    g_controller->setLogLevel(static_cast<int>(siebenuhr_core::CoreLogLevel::DEBUG));
    #endif

    LOG_I("Siebenuhr v%s (core v%s)", SIEBENUHR_VERSION, SIEBENUHR_CORE_VERSION);

    g_controller->loadConfiguration();
    
    // Enable sensors based on build flags
    g_controller->setAutoBrightnessEnabled(AUTO_BRIGHTNESS_ENABLED);
    g_controller->setPowerMonitoringEnabled(POWER_MONITORING_ENABLED);
    
    g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_SOLIDCOLOR);   
    g_controller->setRenderState(siebenuhr::RenderState::SPLASH, "7Uhr");
}

void loop() {
    if (g_controller != nullptr) {
        g_controller->update();
    }
}
