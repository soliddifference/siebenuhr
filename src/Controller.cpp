#include "controller.h"
#include "timezone.h"

#include <WiFi.h>

namespace siebenuhr {

    bool Controller::initializeWifi(bool enable) 
    {
        WiFi.setHostname("7uhr");
        if (enable) {
            if (WiFi.status() == WL_CONNECTED) {
                return true;
            }

            WiFi.mode(WIFI_STA);

            String SSID = "tulporium"; //readStringFromEEPROM(EEPROM_ADDRESS_WIFI_SSID, EEPROM_ADDRESS_MAX_LENGTH);
            String PSWD = "barneb33"; //readStringFromEEPROM(EEPROM_ADDRESS_WIFI_PSWD, EEPROM_ADDRESS_MAX_LENGTH);

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
                timezoneId = 11; //(int)readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_ID);
            }
            String sTimezone = timezones[timezoneId].name;
            LOG_I("Timezone (EEPROM) : %s", sTimezone.c_str());

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
        
        // Call the base class update method
        BaseController::update();
    }

}
