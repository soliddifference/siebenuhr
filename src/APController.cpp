 /* Access Point Controller.
	v1.0 ys 2020123 setup

	Class to manage the access point and wifi setup of siebenuhr
*/

#include <Arduino.h>
#include "Controller.h"
#include "APController.h"

#include <WiFi.h>

using namespace siebenuhr;

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

bool APController::begin(AsyncWiFiManager* pWiFiManager) {
	Controller::getInstance()->debugMessage("Starting Access Point...");
	return setupWifi(pWiFiManager);
}

void saveConfigCallback() {
    siebenuhr::Controller::getInstance()->debugMessage("Save Wifi Config now...");
}

bool APController::setupWifi(AsyncWiFiManager* pWiFiManager) {
	siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
    if (_inst == nullptr)
        return false;

	sprintf(_sIdentifier, "Siebenuhr_%04d WiFi setup", _inst->getSerialNumber());

    // pWiFiManager->resetSettings();
	WiFi.disconnect(true, true);
    pWiFiManager->setTimeout(180);
	pWiFiManager->setDebugOutput(false);
    pWiFiManager->setSaveConfigCallback(saveConfigCallback);

	pWiFiManager->addParameter(&custom_ap_text);
	pWiFiManager->addParameter(&custom_ap_timezone);

    // WiFi.hostname(_sIdentifier);
	// pWiFiManager->autoConnect(_sIdentifier);

    if (pWiFiManager->startConfigPortal(_sIdentifier)) {
		// FIXME move EEPROM part to better place, once refactoring of the DisplayDriver is done
		_cTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
		if(!findTimezoneMatch(_cTimezone)) {
			_inst->debugMessage("EEPROM timezone didn't match any existing timezone. The value searched for was: %s", _cTimezone.c_str());
			_inst->debugMessage("Saving timezone from captive portal or default value to EEPROM: %s", custom_ap_timezone.getValue());
			EEPROMWriteString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, custom_ap_timezone.getValue(), EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
		}
		// _inst->writeToEEPROM(EEPROM_ADDRESS_WIFI_ENABLED, 1);
		return true;
	}
	return false;
}

void APController::resetWifiSettingsAndReboot(AsyncWiFiManager* pWiFiManager) {
    pWiFiManager->resetSettings();
    delay(3000);
    ESP.restart();
}

void APController::getNetworkInfo() {
    if(WiFi.status() == WL_CONNECTED) {
		siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
		if (_inst == nullptr)
			return;

		// _cTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH);
		// _inst->debugMessage(formatString("Timezone (EEPROM) : %s", _cTimezone.c_str()));

		_inst->debugMessage(formatString("Network connected."));
		_inst->debugMessage(formatString("SSID              : %s", WiFi.SSID().c_str()));
		_inst->debugMessage(formatString("BSSID             : %s", WiFi.BSSIDstr().c_str()));
		_inst->debugMessage(formatString("Gateway IP        : %s", WiFi.gatewayIP().toString().c_str()));
		_inst->debugMessage(formatString("Subnet Mask       : %s", WiFi.subnetMask().toString().c_str()));
		_inst->debugMessage(formatString("IP address (DHCP) : %s", WiFi.localIP().toString().c_str()));
		_inst->debugMessage(formatString("MAC address is    : %s", WiFi.macAddress().c_str()));
    }
}