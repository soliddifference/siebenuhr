#include <Arduino.h>

#include "controller.h"

#include "accesspoint.h"
#include "timezone.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>

namespace siebenuhr {

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
	}

	bool APController::begin(Configuration *config) {
		LOG_I("Starting Access Point...");
		return setupAPCaptivePortal(config);
	}

	String APController::buildTimezoneCheckboxOption(int default_tz) {
		String checkboxTimeZone = R"(
		<label for='timezone'>Timezone</label>
		<select name="dayOfWeek" id="timezone" onchange="document.getElementById('key_custom').value = this.value">)";

		const char *timezone_option = R"(<option value="%d">%s</option>)";
		for (int i=0; i<timezones.size(); i++) {
			char tzOption[200];
			sprintf(tzOption, timezone_option, i, timezones[i].name);	
			checkboxTimeZone += String(tzOption);
		}

		checkboxTimeZone += R"(
		</select>
		<script>
		document.getElementById('timezone').value = "%d";
		document.getElementById('key_custom').hidden = true;
		</script>
		)";

		checkboxTimeZone.replace(String("%d"), String(default_tz));

		return checkboxTimeZone;
	}

	bool APController::setupAPCaptivePortal(Configuration *config) {
		sprintf(_sIdentifier, "SiebenuhrAP");

		WiFi.disconnect(true, true);
		wifiManager.resetSettings();
		wifiManager.setDebugOutput(false);

		int curTimezoneID = config->read(to_addr(EEPROMAddress::TIMEZONE_ID));
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
		
		if (wifiManager.startConfigPortal(_sIdentifier)) {

			String ssid = wifiManager.getConfiguredSTASSID();
			config->writeString(to_addr(EEPROMAddress::WIFI_SSID), ssid.c_str());
			LOG_I("SSID: %s", ssid.c_str());

			String pwd = wifiManager.getConfiguredSTAPassword();
			config->writeString(to_addr(EEPROMAddress::WIFI_PSWD), pwd.c_str());
			LOG_I("PSWD: %s", pwd.c_str());

			if (_pCustomTZHidden != nullptr) {
				_nSelectedTimeZoneID = String(_pCustomTZHidden->getValue()).toInt();
				LOG_I("Timezone select: %s", timezones[_nSelectedTimeZoneID].name);
	            config->write(to_addr(EEPROMAddress::TIMEZONE_ID), _nSelectedTimeZoneID);
			}

			config->flushDeferredSaving(true);

			// save and reboot ESP
			// LOG_I("New config from captive portal! Rebooting ESP now....");
			// // _inst->flushDeferredSaving(true);
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
			LOG_I("Network connected.");
			LOG_I("SSID              : %s", WiFi.SSID().c_str());
			LOG_I("BSSID             : %s", WiFi.BSSIDstr().c_str());
			LOG_I("Gateway IP        : %s", WiFi.gatewayIP().toString().c_str());
			LOG_I("Subnet Mask       : %s", WiFi.subnetMask().toString().c_str());
			LOG_I("IP address (DHCP) : %s", WiFi.localIP().toString().c_str());
			LOG_I("MAC address is    : %s", WiFi.macAddress().c_str());
			LOG_I("Hostename         : %s", WiFi.getHostname());
		}
	}

}