#pragma once

#include "siebenuhr_controller.h"

#include <ezTime.h>

namespace siebenuhr {

    class Controller : public siebenuhr_core::BaseController {
    public:
        bool initializeWifi(bool enable);
        bool initializeNTP(bool enable, int timezoneId = -1);

        void update() override;

    protected: 
        bool sendBrightnessToHomeAssistant(int brightness) override { return false; };
        bool sendColorToHomeAssistant(CRGB color) override { return false; };

    private:
        bool m_wifiEnabled = false;
        bool m_NTPEnabled = false;

        Timezone  m_ezTimezone;
        int m_currentHours = -1;
        int m_currentMinutes = -1; 
    };

};