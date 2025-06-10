#include <Arduino.h>
#include "controller.h"

siebenuhr::Controller *g_controller = nullptr;

void setup() {
    Serial.begin(115200);

    g_controller = new siebenuhr::Controller();

    g_controller->setLogLevel(static_cast<int>(siebenuhr_core::CoreLogLevel::INFO));
    g_controller->initialize(siebenuhr_core::ClockType::CLOCK_TYPE_MINI);
    g_controller->loadConfiguration();
    g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_SOLIDCOLOR);   
    g_controller->setRenderState(siebenuhr::RenderState::SPLASH, "7Uhr");
}

void loop() {
    if (g_controller != nullptr) {
        g_controller->update();
    }
}
