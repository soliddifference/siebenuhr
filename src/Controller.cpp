#include "controller.h"

#include "timezone.h"
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
            m_configuration.write(to_addr(EEPROMAddress::COLOR_R), siebenuhr_core::constants::DEFAULT_COLOR.r);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_G), siebenuhr_core::constants::DEFAULT_COLOR.g);
            m_configuration.write(to_addr(EEPROMAddress::COLOR_B), siebenuhr_core::constants::DEFAULT_COLOR.b);
            m_configuration.writeString(to_addr(EEPROMAddress::WIFI_SSID), "undefined1");
            m_configuration.writeString(to_addr(EEPROMAddress::WIFI_PSWD), "undefined2");
            m_configuration.flushDeferredSaving(true);
        }

        int brightness = m_configuration.read(to_addr(EEPROMAddress::BRIGHTNESS));
        CRGB color = CRGB(m_configuration.read(to_addr(EEPROMAddress::COLOR_R)), m_configuration.read(to_addr(EEPROMAddress::COLOR_G)), m_configuration.read(to_addr(EEPROMAddress::COLOR_B)));
        LOG_I("Clock settings: col=RGB(%d, %d, %d) brightness=%d", color.r, color.g, color.b, brightness);

        setBrightness(brightness);
        setColor(siebenuhr_core::Color::fromCRGB(color));
    } 

    bool Controller::initializeWifi(bool enable) 
    {
        WiFi.setHostname("7uhr");
        if (enable) {
            if (WiFi.status() == WL_CONNECTED) {
                return true;
            }

            WiFi.mode(WIFI_STA);

            String s = m_configuration.readString(to_addr(EEPROMAddress::WIFI_SSID));
            String p = m_configuration.readString(to_addr(EEPROMAddress::WIFI_PSWD));
            LOG_I("ZU? %s", s.c_str());
            LOG_I("ZP? %s", p.c_str());


            String SSID = "tulporium"; //readString(EEPROM_ADDRESS_WIFI_SSID, EEPROM_ADDRESS_MAX_LENGTH);
            String PSWD = "barneb33"; //readString(EEPROM_ADDRESS_WIFI_PSWD, EEPROM_ADDRESS_MAX_LENGTH);

            if (SSID.length() != 0) {
                WiFi.begin(SSID.c_str(), PSWD.c_str());
                LOG_I("Connecting to WiFi (%s)..", SSID.c_str());
                
                int ConnectRetries = 0;
                while (WiFi.status() != WL_CONNECTED && ConnectRetries < 50) {
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

        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        m_wifiEnabled = false;

        return true;
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
        
        m_configuration.flushDeferredSaving();

        // Call the base class update method
        BaseController::update();
    }

    bool Controller::handleLongPressReset()
    { 
        return false; 
    }

    void Controller::onBrightnessChange(int brightness)
    { 
        m_configuration.write(to_addr(EEPROMAddress::BRIGHTNESS), brightness);
    }

    void Controller::onColorChange(CRGB color)
    { 
        m_configuration.write(to_addr(EEPROMAddress::COLOR_R), color.r);
        m_configuration.write(to_addr(EEPROMAddress::COLOR_G), color.g);
        m_configuration.write(to_addr(EEPROMAddress::COLOR_B), color.b);
    }
}
