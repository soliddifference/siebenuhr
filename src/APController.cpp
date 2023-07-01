 /* Access Point Controller.
	v1.0 ys 2020123 setup

	Class to manage the access point and wifi setup of siebenuhr
*/

#include <Arduino.h>
#include "Controller.h"
#include "APController.h"
#include "TimeZones.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>

using namespace siebenuhr;

APController* APController::_pInstance = nullptr;

APController* APController::getInstance(){
	if (APController::_pInstance == nullptr) {
		APController::_pInstance = new APController();
	}
	return APController::_pInstance;
}

APController::APController() {
	_bUIConfigured = false;
}

bool APController::begin() {
	Controller::getInstance()->debugMessage("Starting Access Point...");
	return setupAPCaptivePortal();
}

void saveConfigCallback() {
    siebenuhr::Controller::getInstance()->debugMessage("Save Wifi Config now...");
}

String APController::buildTimezoneCheckboxOption(int default_tz) {
	String checkboxTimeZone = R"(
	<br/><label for='timezone'>Timezone</label>
	<select name="dayOfWeek" id="timezone" onchange="document.getElementById('key_custom').value = this.value">)";

	const char *timezone_option = R"(<option value="%d">%s</option>)";
	for (int i=0; i<__timezones.size(); i++) {
		char tzOption[200];
		sprintf(tzOption, timezone_option, i, __timezones[i].name);	
		checkboxTimeZone += String(tzOption);
	}

	checkboxTimeZone += R"(
	</select>
	<script>
	document.getElementById('timezone').value = "%d";
	document.getElementById('key_custom').hidden = true;
	</script>
	)";
	// document.querySelector("[for='key_custom']").hidden = true;

	checkboxTimeZone.replace(String("%d"), String(default_tz));

	return checkboxTimeZone;
}

bool APController::setupAPCaptivePortal() {
	siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
    if (_inst == nullptr)
        return false;

	sprintf(_sIdentifier, "SiebenuhrAP");

	AsyncWebServer server(80);
	DNSServer dns;
	AsyncWiFiManager wifiManager(&server, &dns);

    wifiManager.resetSettings();
	// WiFi.disconnect(true, true);
    // pWiFiManager->setTimeout(180);
	wifiManager.setDebugOutput(false);
    // pWiFiManager->setSaveConfigCallback(saveConfigCallback);

	int curTimezoneID = (int)_inst->readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_ID);
	String checkboxTimeZone = buildTimezoneCheckboxOption(curTimezoneID);
	AsyncWiFiManagerParameter custom_tz_dropdown(checkboxTimeZone.c_str());

	char convertedValue[6];
	sprintf(convertedValue, "%d", curTimezoneID); // Need to convert to string to display a default value.
	AsyncWiFiManagerParameter custom_tz_hidden("key_custom", "Will be hidden", convertedValue, 3);

	wifiManager.addParameter(&custom_tz_hidden); // Needs to be added before the javascript that hides it
	wifiManager.addParameter(&custom_tz_dropdown);

    if (wifiManager.startConfigPortal(_sIdentifier)) {

  		String ssid = wifiManager.getConfiguredSTASSID();
		_inst->debugMessage("SSID: %s", ssid.c_str());
		EEPROMWriteString(EEPROM_ADDRESS_WIFI_SSID, ssid.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

		String pwd = wifiManager.getConfiguredSTAPassword();
		_inst->debugMessage("PSWD: %s", pwd.c_str());
		EEPROMWriteString(EEPROM_ADDRESS_WIFI_PSWD, pwd.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

		int newTimezoneID = String(custom_tz_hidden.getValue()).toInt();
		_inst->debugMessage("Timezone select: %s", __timezones[newTimezoneID].name);
		_inst->writeToEEPROM(EEPROM_ADDRESS_TIMEZONE_ID, newTimezoneID, 0);
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

		_inst->debugMessage("Network connected.");
		_inst->debugMessage("SSID              : %s", WiFi.SSID().c_str());
		_inst->debugMessage("BSSID             : %s", WiFi.BSSIDstr().c_str());
		_inst->debugMessage("Gateway IP        : %s", WiFi.gatewayIP().toString().c_str());
		_inst->debugMessage("Subnet Mask       : %s", WiFi.subnetMask().toString().c_str());
		_inst->debugMessage("IP address (DHCP) : %s", WiFi.localIP().toString().c_str());
		_inst->debugMessage("MAC address is    : %s", WiFi.macAddress().c_str());
		_inst->debugMessage("Hostename         : %s", WiFi.getHostname());
    }
}