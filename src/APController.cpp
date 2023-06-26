 /* Access Point Controller.
	v1.0 ys 2020123 setup

	Class to manage the access point and wifi setup of siebenuhr
*/

#include <Arduino.h>
#include "Controller.h"
#include "APController.h"

#include <WiFi.h>
#include <WiFiManager.h>

using namespace siebenuhr;

WiFiManager wifiManager;

APController* APController::_pInstance = nullptr;

char default_timezone[EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH] = "Europe/Zurich";
AsyncWiFiManagerParameter custom_ap_text("<br><b>Timezone</b> of this SIEBENUHR (e.g. \"Europe/Zurich\")<BR>Please find <a href=\"https://en.wikipedia.org/wiki/List_of_tz_database_time_zones\">a complete list on Wikipedia</a>");
AsyncWiFiManagerParameter custom_ap_timezone("Timezone", "Timezone e.g. Europe/Zurich", default_timezone, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH);

APController* APController::getInstance(){
	if (APController::_pInstance == nullptr) {
		APController::_pInstance = new APController();
	}
	return APController::_pInstance;
}

APController::APController() {
    // todo
}

void APController::begin() {
	Controller::getInstance()->debugMessage("Starting Access Point...");
	setupWifi();
}

void saveConfigCallback() {
    // shouldSaveConfig = true;
}

void APController::setupWifi() {
	siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
    if (_inst == nullptr)
        return;

	sprintf(_sIdentifier, "Siebenuhr_%04d WiFi setup", _inst->getSerialNumber());

    wifiManager.setDebugOutput(false);
    wifiManager.setSaveConfigCallback(saveConfigCallback);

	wifiManager.addParameter(&custom_ap_text);
	wifiManager.addParameter(&custom_ap_timezone);

    WiFi.hostname(_sIdentifier);
	wifiManager.autoConnect(_sIdentifier);

	// FIXME move EEPROM part to better place, once refactoring of the DisplayDriver is done
	_cTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
	if(!findTimezoneMatch(_cTimezone)) {
		_inst->debugMessage(formatString("EEPROM timezone didn't match any existing timezone. The value searched for was: %s", _cTimezone.c_str()));
		_inst->debugMessage(formatString("Saving timezone from captive portal or default value to EEPROM: %s", captive_portal_timezone.getValue()));
		EEPROMWriteString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, captive_portal_timezone.getValue(), EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
	}
}

void APController::resetWifiSettingsAndReboot() {
    wifiManager.resetSettings();
    delay(3000);
    ESP.restart();
}