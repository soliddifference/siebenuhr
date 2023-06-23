#include <Arduino.h>
#include "SiebenUhr.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ezTime.h>

//needed for library
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPmDNS.h>

#include "Controller.h"
#include "DisplayDriver.h"

// NTP Servers:
static const char ntpServerName[] = "0.ch.pool.ntp.org"; // us.pool.ntp.org

int8_t minutesTimeZone = 0;

DisplayDriver Display;

//WebDriver webdriver(&Display);
AsyncWebServer server(80);
DNSServer dns;

AsyncWiFiManager wifiManager(&server, &dns);

void setup() {
    siebenuhr::Controller *_cntrl = siebenuhr::Controller::getInstance(); // just for convinience
    if (_cntrl == nullptr)
        return;

    _cntrl->initializeDebug(true);
    _cntrl->initializeEEPROM();

    Display.setup(_cntrl->getFirstTimeSetup());

    _cntrl->setResetButton(RESET_BUTTON);
    _cntrl->setKnob(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN);

    if (!_cntrl->initializeDisplay(&Display)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr display setup failed.");
        return;
    }

    if (!_cntrl->initializeWifi(false, &wifiManager)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr wifi setup failed.");
        return;
    };

    if (!_cntrl->initializeNTP(false)) {
        _cntrl->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _cntrl->debugMessage("7Uhr NTP setup failed.");
        return;
    };

    _cntrl->begin();
}

void _setup() {
    siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
    if (_inst == nullptr)
        return;

    // setup wifi
    char connectName[100];
    sprintf(connectName, "Siebenuhr_%04d WiFi setup", _inst->getSerialNumber());

    String _cTimezone;
    #ifdef _WIFITEST
        Serial.println("Demo mode enabled. Will connect to Justin Bieber's iPhone.");
        WiFi.begin("Justin Bieber's iPhone", "stalldrang");
        _cTimezone = "Europe/Zurich";
        delay(1000);
    #else
        AsyncWiFiManagerParameter custom_text("<br><b>Timezone</b> of this SIEBENUHR (e.g. \"Europe/Zurich\")<BR>Please find <a href=\"https://en.wikipedia.org/wiki/List_of_tz_database_time_zones\">a complete list on Wikipedia</a>");
        wifiManager.addParameter(&custom_text);
        char provided_timezone[EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH] = "Europe/Zurich";
        AsyncWiFiManagerParameter captive_portal_timezone("Timezone", "Timezone e.g. Europe/Zurich", provided_timezone, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH);
        wifiManager.addParameter(&captive_portal_timezone);
        wifiManager.autoConnect(connectName);
        // FIXME move EEPROM part to better place, once refactoring of the DisplayDriver is done
        _cTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
        if(!findTimezoneMatch(_cTimezone)) {
            _inst->debugMessage(formatString("EEPROM timezone didn't match any existing timezone. The value searched for was: %s", _cTimezone.c_str()));
            _inst->debugMessage(formatString("Saving timezone from captive portal or default value to EEPROM: %s", captive_portal_timezone.getValue()));
           EEPROMWriteString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, captive_portal_timezone.getValue(), EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
        }
    #endif

    _cTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH);
    _inst->debugMessage(formatString("Timezone (EEPROM) : %s", _cTimezone.c_str()));
    _inst->debugMessage(formatString("SSID              : %s", WiFi.SSID()));
    _inst->debugMessage(formatString("IP address (DHCP) : %s", WiFi.localIP()));
    _inst->debugMessage(formatString("MAC address is    : %s", WiFi.macAddress()));

    //initialize mDNS service
    const char * instanceName = "siebenuhr";
    if (!MDNS.begin(instanceName)) {
        _inst->debugMessage("Error setting up MDNS responder!");
        delay(1000);
    }
    mdns_hostname_set(instanceName);
    MDNS.addService("http", "tcp", 80);
    mdns_instance_name_set(instanceName);

    // Display.setNotification("Sync");
    // Display.update();

    setDebug(INFO);
    while(timeStatus()==timeNotSet) {
      updateNTP();
      _inst->debugMessage("Waiting for time sync");
      delay(100);
    }
    waitForSync();

    Display.setNewDefaultTimezone(_cTimezone);
    Display.disableNotification();
    Display.setOperationMode(OPERATION_MODE_CLOCK_HOURS);

    _inst->debugMessage("7Uhr controller setup complete.");
}

void loop() {
  siebenuhr::Controller::getInstance()->update();
}
