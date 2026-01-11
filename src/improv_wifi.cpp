#include "improv_wifi.h"
#include "Controller.h"

#include <ImprovWiFiLibrary.h>
#include <WiFi.h>

namespace siebenuhr {

static ImprovWiFi improvSerial(&Serial);
static Configuration* g_config = nullptr;
static bool g_improvProvisioned = false;
static bool g_improvInitialized = false;

// Device name based on build variant
#if defined(BUILD_CLOCK_MINI)
static const char* DEVICE_NAME = "SIEBENUHR Mini";
#else
static const char* DEVICE_NAME = "Siebenuhr Mk1";
#endif

static const char* FIRMWARE_NAME = "siebenuhr";

// Callback when Improv successfully connects to Wi-Fi
static void onImprovConnectedCallback(const char* ssid, const char* password) {
    LOG_I("Improv: Connected to %s", ssid);
    
    // Store credentials for future boots
    if (g_config != nullptr) {
        g_config->writeString(ConfigKey::WIFI_SSID, ssid);
        g_config->writeString(ConfigKey::WIFI_PSWD, password);
        g_config->flushDeferredSaving(true);
        LOG_I("Improv: Credentials saved, restarting...");
    }
    
    g_improvProvisioned = true;
    
    // Restart to apply new credentials through normal boot flow
    delay(500);
    ESP.restart();
}

// Callback for Improv errors
static void onImprovErrorCallback(ImprovTypes::Error error) {
    LOG_E("Improv error: %d", error);
}

void initImprov(Configuration* config) {
    g_config = config;

    // Set device info shown in ESP Web Tools
    // Parameters: chipFamily, firmwareName, firmwareVersion, deviceName
    improvSerial.setDeviceInfo(
        ImprovTypes::ChipFamily::CF_ESP32,
        FIRMWARE_NAME,
        SIEBENUHR_VERSION,
        DEVICE_NAME
    );

    // Set callbacks
    improvSerial.onImprovConnected(onImprovConnectedCallback);
    improvSerial.onImprovError(onImprovErrorCallback);

    g_improvInitialized = true;
    LOG_I("Improv: Initialized (%s v%s)", DEVICE_NAME, SIEBENUHR_VERSION);
}

void handleImprov() {
    if (g_improvInitialized) {
        improvSerial.handleSerial();
    }
}

bool isImprovProvisioned() {
    return g_improvProvisioned;
}

bool hasValidWifiCredentials() {
    if (g_config == nullptr) return false;
    
    String ssid = g_config->readString(ConfigKey::WIFI_SSID);
    // "undefined1" is the default/unconfigured value
    return ssid.length() > 0 && ssid != "undefined1";
}

}  // namespace siebenuhr
