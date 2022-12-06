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
    siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
    if (_inst == nullptr)
        return;

    uint16_t serialNumber_ = Display.setup(_inst->getFirstTimeSetup());
    _inst->setResetButton(RESET_BUTTON);
    _inst->setKnob(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN);

    if (!_inst->initializeDebug(true) || !_inst->initializeWifi(true, &wifiManager) || !_inst->initializeNTP(true) || !_inst->initializeDisplay(&Display)) {
        _inst->debugMessage(siebenuhr::Controller::getInstance()->getLastErrorDesc());
        _inst->debugMessage("7Uhr controller setup failed.");
        return;
    };

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
           Serial.println("EEPROM timezone didn't match any existing timezone. The value searched for was: ");
           Serial.println(_cTimezone);
           Serial.print("Saving timezone from captive portal or default value to EEPROM: '");
           Serial.print(captive_portal_timezone.getValue());
           Serial.println("'");
           EEPROMWriteString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, captive_portal_timezone.getValue(), EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
        }
    #endif

    _cTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH);
    Serial.print("Timezone (EEPROM) : ");
    Serial.println(_cTimezone);
    Serial.print("SSID              : ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address (DHCP) : ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address is    : ");
    Serial.println(WiFi.macAddress());

    //initialize mDNS service
    const char * instanceName = "siebenuhr";
    if (!MDNS.begin(instanceName)) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    mdns_hostname_set(instanceName);
    MDNS.addService("http", "tcp", 80);
    mdns_instance_name_set(instanceName);

    Display.set_notification("Sync");
    Display.update();

    setDebug(INFO);
    while(timeStatus()==timeNotSet) {
      updateNTP();
      Serial.println("waiting for time sync");
      delay(1000);
    }
    waitForSync();

    Display.set_new_default_timezone(_cTimezone);
    Display.disable_notification();
    Display.set_operations_mode(OPERATION_MODE_CLOCK_HOURS);

    _inst->debugMessage("7Uhr controller setup complete.");
}

void loop() {
  siebenuhr::Controller::getInstance()->update();
}
