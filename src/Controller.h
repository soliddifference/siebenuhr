#pragma once

#include "siebenuhr_controller.h"
#include "configuration.h"
#include <ezTime.h>

#define SIEBENUHR_VERSION "1.1.1"

namespace siebenuhr {

    enum class RenderState : int {
        SPLASH = 0,
        WIFI,
        NTP,
        CLOCK
    };

    class Controller : public siebenuhr_core::BaseController {
    public:
        void loadConfiguration(bool forceFirstTimeSetup = false);
        bool initializeWifi(bool enable);
        bool initializeNTP(bool enable, int timezoneId = -1);

        void update(bool doHandleUserInput = true) override;
        
        // Override to add logarithmic scaling and rate limiting
        void setBrightness(int value);

        void setRenderState(RenderState state, const std::string& text = "")
        {
            m_renderState = state;
            m_renderStateChange = millis();
            if (text.length() != 0)
            {
                m_display->setText(text);
            }
        }

        Configuration* getConfiguration() { return &m_configuration; }

    protected:
        void onButtonLongPress() override;
        void onBrightnessChange(int brightness) override;
        void onColorChange(CRGB color) override;
        void onPersonalityChange(siebenuhr_core::PersonalityType personality) override;

    private:
        // Logarithmic brightness mapping (volume-knob feel)
        int applyLogBrightnessMapping(int linearInput);
        
        Configuration m_configuration;

        RenderState m_renderState = RenderState::SPLASH;
        unsigned long m_renderStateChange = 0;
        unsigned long m_lastBrightnessChange = 0;

        bool m_wifiEnabled = false;
        bool m_NTPEnabled = false;

        Timezone m_ezTimezone;
        int m_currentHours = -1;
        int m_currentMinutes = -1;
    };

};
