#include "controller.h"

#include "timezone.h"
#include "accesspoint.h"
#include "siebenuhr_color.h"

#include <WiFi.h>

namespace siebenuhr {

    void Controller::loadConfiguration(bool forceFirstTimeSetup) 
    {
        int initialized = m_configuration.read(to_addr(EEPROMAddress::INITIALISED));
        if (forceFirstTimeSetup || initialized != 1)
        {
            m_configuration.reset();

            LOG_I("Initializing default configuration settings.");
            m_configuration.write(to_addr(EEPROMAddress::INITIALISED), 1);
            m_configuration.write(to_addr(EEPROMAddress::TIMEZONE_ID), DEFAULT_TIMEZONE);
            m_configuration.write(to_addr(EEPROMAddress::BRIGHTNESS), siebenuhr_core::constants::DefaultBrightness);
            m_configuration.write(to_addr(EEPROMAddress::PERSONALITY), siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_G), siebenuhr_core::constants::DEFAULT_COLOR.g);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_B), siebenuhr_core::constants::DEFAULT_COLOR.b);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_B), siebenuhr_core::constants::DEFAULT_COLOR.b);
            m_configuration.writeString(to_addr(EEPROMAddress::WIFI_SSID), "undefined1");
            m_configuration.writeString(to_addr(EEPROMAddress::WIFI_PSWD), "undefined2");
            m_configuration.flushDeferredSaving(true);
        }

        int brightness = m_configuration.read(to_addr(EEPROMAddress::BRIGHTNESS));
        CRGB color = CRGB(m_configuration.read(to_addr(EEPROMAddress::COLOR_R)), m_configuration.read(to_addr(EEPROMAddress::COLOR_G)), m_configuration.read(to_addr(EEPROMAddress::COLOR_B)));
        LOG_I("Clock settings: col=RGB(%d, %d, %d) brightness=%d", color.r, color.g, color.b, brightness);

        setBrightness(brightness);

        int personality = m_configuration.read(to_addr(EEPROMAddress::PERSONALITY));
        if (personality == siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL)
        {
            setColor(siebenuhr_core::Color::fromCRGB(color));
        }
        else
        {
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
                    delay(200);
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

    void Controller::update()
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
                    if (initializeNTP(true))
                    {
                        LOG_I("7Uhr NTP setup successful.");    
                        setRenderState(RenderState::CLOCK);

                        // switch to personatlity from the configuration
                        siebenuhr_core::PersonalityType personality = (siebenuhr_core::PersonalityType)m_configuration.read(to_addr(EEPROMAddress::PERSONALITY));
                        setPersonality(personality);
                    }        
                }
            }
        }
        else if (m_renderState == RenderState::WIFI)
        {
            if (millis()-m_renderStateChange > 2000) 
            {
                APController::getInstance()->begin(&m_configuration);
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

        m_configuration.flushDeferredSaving();
        BaseController::update();
    }

    void Controller::onButtonLongPress()
    { 
        // reset configuration of the clock
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
}
