#pragma once

#include "siebenuhr_controller.h"
#include "configuration.h"
#include <ezTime.h>

namespace siebenuhr {

    class Controller : public siebenuhr_core::BaseController {
    public:
        void loadConfiguration(bool forceFirstTimeSetup = false);
        bool initializeWifi(bool enable);
        bool initializeNTP(bool enable, int timezoneId = -1);

        void update() override;

    protected: 
        bool handleLongPressReset() override;
        void onBrightnessChange(int brightness) override;
        void onColorChange(CRGB color) override;

    private:
        Configuration m_configuration;

        bool m_wifiEnabled = false;
        bool m_NTPEnabled = false;

        Timezone m_ezTimezone;
        int m_currentHours = -1;
        int m_currentMinutes = -1; 
    };

};