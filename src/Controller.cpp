#include "Controller.h"

#include "timezone.h"
#include "accesspoint.h"
#include "siebenuhr_color.h"

#include <WiFi.h>
#include <cmath>

namespace siebenuhr {

    void Controller::loadConfiguration(bool forceFirstTimeSetup)
    {
        int initialized = m_configuration.read(to_addr(EEPROMAddress::INITIALISED));
        if (forceFirstTimeSetup || initialized != 1)
        {
            LOG_I("Initializing default configuration settings.");
            m_configuration.reset();
            // INITIALIZED must be written immediately (delay=0) - it's critical
            m_configuration.write(to_addr(EEPROMAddress::INITIALISED), 1, 0);
            m_configuration.write(to_addr(EEPROMAddress::TIMEZONE_ID), DEFAULT_TIMEZONE);
            m_configuration.write(to_addr(EEPROMAddress::BRIGHTNESS), siebenuhr_core::constants::DefaultBrightness);
            m_configuration.write(to_addr(EEPROMAddress::PERSONALITY), siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_R), siebenuhr_core::constants::DEFAULT_COLOR.r);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_G), siebenuhr_core::constants::DEFAULT_COLOR.g);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_B), siebenuhr_core::constants::DEFAULT_COLOR.b);
            m_configuration.writeString(to_addr(EEPROMAddress::WIFI_SSID), "undefined1");
            m_configuration.writeString(to_addr(EEPROMAddress::WIFI_PSWD), "undefined2");
            m_configuration.flushDeferredSaving(true);
        }

        int brightness = m_configuration.read(to_addr(EEPROMAddress::BRIGHTNESS));
        CRGB color = CRGB(m_configuration.read(to_addr(EEPROMAddress::COLOR_R)), m_configuration.read(to_addr(EEPROMAddress::COLOR_G)), m_configuration.read(to_addr(EEPROMAddress::COLOR_B)));
        LOG_I("Configuration:");
        LOG_I("- cololor = RGB(%d, %d, %d)", color.r, color.g, color.b);
        LOG_I("- brightness = %d", brightness);

        setBrightness(brightness);

        int personality = m_configuration.read(to_addr(EEPROMAddress::PERSONALITY));
        if (personality == siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL)
        {
            LOG_I("- personality = COLORWHEEL");
            // setColor(siebenuhr_core::Color::fromCRGB(color));
            setColor(siebenuhr_core::Color::fromCRGB(siebenuhr_core::constants::DEFAULT_COLOR));
        }
        else
        {
            LOG_I("- personality = FIXED COLOR");
            // setColor(siebenuhr_core::Color::fromCRGB(color));
            setColor(siebenuhr_core::Color::fromCRGB(siebenuhr_core::constants::DEFAULT_COLOR));
        }
    }

    bool Controller::initializeWifi(bool enable)
    {
        WiFi.setHostname("7uhr");
        if (enable) {
            if (WiFi.status() == WL_CONNECTED) {
                return true;
            }

            WiFi.mode(WIFI_STA);

            String SSID = m_configuration.readString(to_addr(EEPROMAddress::WIFI_SSID));
            String PSWD = m_configuration.readString(to_addr(EEPROMAddress::WIFI_PSWD));
            LOG_I("ZU? %s", SSID.c_str());
            LOG_I("ZP? %s", PSWD.c_str());

            if (SSID.length() != 0) {
                WiFi.begin(SSID.c_str(), PSWD.c_str());
                LOG_I("Connecting to WiFi (%s)..", SSID.c_str());

                int ConnectRetries = 0;
                while (WiFi.status() != WL_CONNECTED && ConnectRetries < 20) {
                    ConnectRetries++;
                    LOG_I(".. retry #%d", ConnectRetries);
                    delay(500);
                }

                if (WiFi.status() == WL_CONNECTED) {
                    // APController::getInstance()->getNetworkInfo();
                    m_wifiEnabled = true;
                    LOG_I("Wifi connected.");
                    return true;
                }
            }
        }

        LOG_E("Wifi setup failed!");

        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        m_wifiEnabled = false;

        setRenderState(RenderState::WIFI, "uiFi");

        return false;
    }

    bool Controller::initializeNTP(bool enable, int timezoneId)
    {
        if (m_wifiEnabled && enable) {
            if (timezoneId == -1) {
                timezoneId = m_configuration.read(to_addr(EEPROMAddress::TIMEZONE_ID));
            }
            String sTimezone = timezones[timezoneId].name;
            LOG_I("Timezone(%d) : %s", timezoneId, sTimezone.c_str());

            setDebug(INFO);
            while(timeStatus()==timeNotSet) {
                updateNTP();
                LOG_I("Waiting for time sync...");
                delay(100);
            }
            waitForSync();

            m_ezTimezone.setLocation(sTimezone);
            m_ezTimezone.setDefault();

            m_NTPEnabled = enable;
        }

        return true;
    }

    void Controller::update(bool doHandleUserInput)
    {
        if (m_wifiEnabled && m_NTPEnabled) {
            events(); // give the ezTime-lib it's processing cycle
        }

        if (m_renderState == RenderState::SPLASH)
        {
            if (millis()-m_renderStateChange > 2000)
            {
                LOG_I("Initializing 7Uhr...");
                if (initializeWifi(true))
                {
                    LOG_I("7Uhr wifi setup successful.");
                    setRenderState(RenderState::NTP, "ntp");
                }
            }
        }
        else if (m_renderState == RenderState::WIFI)
        {
            // Only start AP once when entering WIFI state
            // The AP will restart the device on success, or stay in portal mode
            static bool apStarted = false;
            if (!apStarted && millis()-m_renderStateChange > 2000)
            {
                apStarted = true;
                if (APController::getInstance()->begin(&m_configuration)) {
                    // Config saved and device will restart (this line won't be reached)
                } else {
                    // Portal timed out without config - restart device to retry
                    LOG_I("AP portal closed without config, restarting...");
                    delay(500);
                    ESP.restart();
                }
            }
        }
        else if (m_renderState == RenderState::NTP)
        {
            if (millis() - m_renderStateChange > 2000)
            {
                if (initializeNTP(true))
                {
                    LOG_I("7Uhr NTP setup successful.");
                    setRenderState(RenderState::CLOCK);

                    // switch to personatlity from the configuration
                    // siebenuhr_core::PersonalityType personality = (siebenuhr_core::PersonalityType)m_configuration.read(to_addr(EEPROMAddress::PERSONALITY));

                    // Update: always set to COLORWHEEL on startup
                    siebenuhr_core::PersonalityType personality = siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL;
                    setPersonality(personality);
                }
            }
        }
        else
        {
            if (false)
            {
                // fast version (minutes and seconds) for testing
                if (m_currentHours != minute() || m_currentMinutes != second())
                {
                    m_currentHours = minute();
                    m_currentMinutes = second();
                    setTime(m_currentHours, m_currentMinutes);
                }
            }
            else
            {
                if (m_currentHours != hour() || m_currentMinutes != minute())
                {
                    m_currentHours = hour();
                    m_currentMinutes = minute();
                    setTime(m_currentHours, m_currentMinutes);
                }
            }
        }

        // Allow button input in WIFI state so user can trigger long-press reset
        doHandleUserInput = (m_renderState != RenderState::SPLASH);

        m_configuration.flushDeferredSaving();
        BaseController::update(doHandleUserInput);
    }

    void Controller::onButtonLongPress()
    {
        // reset configuration of the clock
        LOG_I("!!!!! Long press detected - resetting configuration to defaults !!!!!");
        loadConfiguration(true);
        setRenderState(RenderState::WIFI, "uiFi");
    }

    void Controller::onBrightnessChange(int brightness)
    {
        m_configuration.write(to_addr(EEPROMAddress::BRIGHTNESS), brightness, 1000);
    }

    void Controller::onColorChange(CRGB color)
    {
        LOG_I("Color change... %d %d %d", color.r, color.g, color.b);
        m_configuration.write(to_addr(EEPROMAddress::COLOR_R), color.r, 1000);
        m_configuration.write(to_addr(EEPROMAddress::COLOR_G), color.g, 1000);
        m_configuration.write(to_addr(EEPROMAddress::COLOR_B), color.b, 1000);
    }

    void Controller::onPersonalityChange(siebenuhr_core::PersonalityType personality)
    {
        LOG_I("Personality change... %d", personality);
        m_configuration.write(to_addr(EEPROMAddress::PERSONALITY), siebenuhr_core::PersonalityType::PERSONALITY_SOLIDCOLOR);
    }

    // Logarithmic mapping for volume-knob feel
    // More resolution at low brightness, compressed at high brightness
    int Controller::applyLogBrightnessMapping(int linearInput)
    {
        if (linearInput <= 1) return 1;
        if (linearInput >= 255) return 255;
        
        // Use log curve: more input range dedicated to low output values
        // Formula: output = 255 * (log(input) / log(255))
        // This gives ~50% of input range to first 16 brightness levels
        float normalized = (float)linearInput / 255.0f;
        float logged = log10(1.0f + normalized * 9.0f) / log10(10.0f);  // log scale 1-10
        int output = (int)(logged * 255.0f);
        
        return siebenuhr_core::clamp(output, 1, 255);
    }

    void Controller::setBrightness(int value)
    {
        // Rate limit: minimum 50ms between brightness changes for smoother control
        unsigned long now = millis();
        if (now - m_lastBrightnessChange < 50) {
            return;  // Skip this update, too fast
        }
        m_lastBrightnessChange = now;

        // Apply logarithmic mapping for better low-end control
        int mappedValue = applyLogBrightnessMapping(value);
        
        // Call base class with mapped value
        BaseController::setBrightness(mappedValue);
    }
}
