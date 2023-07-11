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

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

APController* APController::getInstance(){
	if (APController::_pInstance == nullptr) {
		APController::_pInstance = new APController();
	}
	return APController::_pInstance;
}

APController::APController() {
	_nSelectedTimeZoneID = -1;
	_pCustomTZDropdown = nullptr;
	_pCustomTZHidden = nullptr;
    _pCustomMQTTServer = nullptr;
    _pCustomMQTTUser = nullptr;
    _pCustomMQTTPassword = nullptr;
}

bool APController::begin() {
	Controller::getInstance()->debugMessage("Starting Access Point...");
	return setupAPCaptivePortal();
}

String APController::buildTimezoneCheckboxOption(int default_tz) {
	String checkboxTimeZone = R"(
	<label for='timezone'>Timezone</label>
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
	// siebenuhr::Controller::getInstance()->debugMessage(checkboxTimeZone);

	return checkboxTimeZone;
}

bool APController::setupAPCaptivePortal() {
	siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience
    if (_inst == nullptr)
        return false;

	sprintf(_sIdentifier, "SiebenuhrAP");

	WiFi.disconnect(true, true);
    wifiManager.resetSettings();
	wifiManager.setDebugOutput(false);

	int curTimezoneID = (int)_inst->readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_ID);
	if (_pCustomTZHidden == nullptr) {
		wifiManager.addParameter(new AsyncWiFiManagerParameter("<br/>NTP config:"));

		char convertedValue[6];
		sprintf(convertedValue, "%d", curTimezoneID); // Need to convert to string to display a default value.
		_pCustomTZHidden = new AsyncWiFiManagerParameterExt("key_custom", "Will be hidden", convertedValue, 3);
		wifiManager.addParameter(_pCustomTZHidden); // Needs to be added before the javascript that hides it
	} 

	if (_pCustomTZDropdown == nullptr) {
		_sDropDownTimeZoneHTML = buildTimezoneCheckboxOption(curTimezoneID);
		_pCustomTZDropdown = new AsyncWiFiManagerParameterExt(_sDropDownTimeZoneHTML.c_str());
		wifiManager.addParameter(_pCustomTZDropdown);
	} else {
		// if control already exists, just update the custom HTML (initial selection of timeszone might have changed since first initialization)
		_sDropDownTimeZoneHTML = buildTimezoneCheckboxOption(curTimezoneID);
		_pCustomTZDropdown->setCustomHTML(_sDropDownTimeZoneHTML.c_str());
	}

	if (_pCustomMQTTServer == nullptr) {
		wifiManager.addParameter(new AsyncWiFiManagerParameter("<br/><br/>MQTT config:"));

		String mqtt_server = _inst->readStringFromEEPROM(EEPROM_ADDRESS_HA_MQTT_IP, EEPROM_ADDRESS_MAX_LENGTH);
		_pCustomMQTTServer = new AsyncWiFiManagerParameter("MQTT IP", "mqtt server", mqtt_server.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);
 		wifiManager.addParameter(_pCustomMQTTServer);

		String mqtt_user = _inst->readStringFromEEPROM(EEPROM_ADDRESS_HA_MQTT_USERNAME, EEPROM_ADDRESS_MAX_LENGTH);
		_pCustomMQTTUser = new AsyncWiFiManagerParameter("MQTT User", "mqtt user", mqtt_user.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);
 		wifiManager.addParameter(_pCustomMQTTUser);

		String mqtt_password = _inst->readStringFromEEPROM(EEPROM_ADDRESS_HA_MQTT_PASSWORD, EEPROM_ADDRESS_MAX_LENGTH);
		_pCustomMQTTPassword = new AsyncWiFiManagerParameter("MQTT Password", "mqtt password", mqtt_password.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);
 		wifiManager.addParameter(_pCustomMQTTPassword);
	}
	
    if (wifiManager.startConfigPortal(_sIdentifier)) {

  		String ssid = wifiManager.getConfiguredSTASSID();
		_inst->debugMessage("SSID: %s", ssid.c_str());
		_inst->writeStringToEEPROM(EEPROM_ADDRESS_WIFI_SSID, ssid.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

		String pwd = wifiManager.getConfiguredSTAPassword();
		_inst->debugMessage("PSWD: %s", pwd.c_str());
		_inst->writeStringToEEPROM(EEPROM_ADDRESS_WIFI_PSWD, pwd.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

		if (_pCustomTZHidden != nullptr) {
			_nSelectedTimeZoneID = String(_pCustomTZHidden->getValue()).toInt();
			_inst->debugMessage("Timezone select: %s", __timezones[_nSelectedTimeZoneID].name);
			_inst->writeToEEPROM(EEPROM_ADDRESS_TIMEZONE_ID, _nSelectedTimeZoneID, 0);
		}

  		String mqtt_server = String(_pCustomMQTTServer->getValue());
		_inst->debugMessage("MQTT IP: %s", mqtt_server.c_str());
		_inst->writeStringToEEPROM(EEPROM_ADDRESS_HA_MQTT_IP, mqtt_server.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

  		String mqtt_user = String(_pCustomMQTTUser->getValue());
		_inst->debugMessage("MQTT USER: %s", mqtt_user.c_str());
		_inst->writeStringToEEPROM(EEPROM_ADDRESS_HA_MQTT_USERNAME, mqtt_user.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

  		String mqtt_password = String(_pCustomMQTTPassword->getValue());
		_inst->debugMessage("MQTT PSWD: %s", mqtt_password.c_str());
		_inst->writeStringToEEPROM(EEPROM_ADDRESS_HA_MQTT_PASSWORD, mqtt_password.c_str(), EEPROM_ADDRESS_MAX_LENGTH-1);

		// save and reboot ESP
		_inst->debugMessage("New config from captive portal! Rebooting ESP now....");
		_inst->flushDeferredSavingToEEPROM(true);
		ESP.restart();

		return true;
	}

	server.reset();
	server.end();

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