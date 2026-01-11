#include <Arduino.h>

#include "Controller.h"

#include "accesspoint.h"
#include "timezone.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>

namespace siebenuhr {

	APController* APController::_pInstance = nullptr;

	// These must be created fresh each time to avoid stale state
	AsyncWebServer* pServer = nullptr;
	DNSServer* pDns = nullptr;
	AsyncWiFiManager* pWifiManager = nullptr;

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

		// Clean up any previous instances to ensure fresh state
		if (pWifiManager != nullptr) {
			delete pWifiManager;
			pWifiManager = nullptr;
		}
		if (pServer != nullptr) {
			pServer->reset();
			pServer->end();
			delete pServer;
			pServer = nullptr;
		}
		if (pDns != nullptr) {
			pDns->stop();
			delete pDns;
			pDns = nullptr;
		}

		// Reset parameter tracking
		_pCustomTZDropdown = nullptr;
		_pCustomTZHidden = nullptr;

		// Ensure WiFi is completely stopped
		WiFi.disconnect(true, true);  // disconnect and erase credentials
		WiFi.mode(WIFI_OFF);
		delay(500);  // longer delay to ensure clean state

		// Create fresh server and WiFiManager instances
		pServer = new AsyncWebServer(80);
		pDns = new DNSServer();
		pWifiManager = new AsyncWiFiManager(pServer, pDns);

		// Configure WiFiManager BEFORE starting portal
		pWifiManager->setTryConnectDuringConfigPortal(false);  // Don't try to connect while portal is open
		pWifiManager->setConfigPortalTimeout(300);  // 5 minute timeout
		pWifiManager->setBreakAfterConfig(true);  // Exit portal after config saved (even if connection fails)
		pWifiManager->setDebugOutput(true);
		
		// Set static IP for the AP (helps with captive portal detection)
		pWifiManager->setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

		// Captive portal detection handlers - redirect common OS check URLs to portal
		pServer->on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});
		pServer->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});
		pServer->on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});
		pServer->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});
		pServer->on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});
		pServer->on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});
		pServer->on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});

		// Add parameters (only once per WiFiManager instance)
		int curTimezoneID = config->read(ConfigKey::TIMEZONE_ID);
		
		pWifiManager->addParameter(new AsyncWiFiManagerParameter("<br/>NTP config:"));

		char convertedValue[6];
		sprintf(convertedValue, "%d", curTimezoneID);
		_pCustomTZHidden = new AsyncWiFiManagerParameter("key_custom", "Will be hidden", convertedValue, 3);
		pWifiManager->addParameter(_pCustomTZHidden);

		_sDropDownTimeZoneHTML = buildTimezoneCheckboxOption(curTimezoneID);
		_pCustomTZDropdown = new AsyncWiFiManagerParameter(_sDropDownTimeZoneHTML.c_str());
		pWifiManager->addParameter(_pCustomTZDropdown);

		// Catch-all: redirect unknown requests to portal
		pServer->onNotFound([](AsyncWebServerRequest *request) {
			request->redirect("http://192.168.4.1");
		});

		LOG_I("Starting config portal: %s", _sIdentifier);
		LOG_I("Connect to AP and browse to http://192.168.4.1");
		
		bool portalResult = pWifiManager->startConfigPortal(_sIdentifier);
		
		// Check if user submitted config (SSID will be set even if connection failed)
		String ssid = pWifiManager->getConfiguredSTASSID();
		
		if (ssid.length() > 0) {
			// User submitted config - save it regardless of connection result
			config->writeString(ConfigKey::WIFI_SSID, ssid.c_str());
			LOG_I("SSID: %s", ssid.c_str());

			String pwd = pWifiManager->getConfiguredSTAPassword();
			config->writeString(ConfigKey::WIFI_PSWD, pwd.c_str());
			LOG_I("PSWD: (saved, %d chars)", pwd.length());

			if (_pCustomTZHidden != nullptr) {
				_nSelectedTimeZoneID = String(_pCustomTZHidden->getValue()).toInt();
				LOG_I("Timezone select: %s", timezones[_nSelectedTimeZoneID].name);
				config->write(ConfigKey::TIMEZONE_ID, _nSelectedTimeZoneID);
			}

			config->flushDeferredSaving(true);
			
			LOG_I("Configuration saved, restarting...");
			delay(500);  // Allow log to flush
			ESP.restart();

			return true;
		}

		if (portalResult) {
			LOG_I("Portal closed successfully but no SSID configured");
		} else {
			LOG_I("Config portal timed out or closed without config");
		}
		return false;
	}

	void APController::resetWifiSettingsAndReboot(AsyncWiFiManager* wifiMgr) {
		if (wifiMgr != nullptr) {
			wifiMgr->resetSettings();
		}
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