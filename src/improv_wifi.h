#pragma once

#include "configuration.h"

namespace siebenuhr {

/**
 * Improv Wi-Fi Serial handler for web flasher provisioning.
 * 
 * When firmware is flashed via ESP Web Tools, Improv allows the user
 * to configure Wi-Fi credentials directly through the browser without
 * needing to connect to a captive portal.
 * 
 * Usage:
 *   - Call initImprov() in setup() after Serial.begin()
 *   - Call handleImprov() in loop() - this handles Improv commands
 *   - Improv runs alongside normal operation (non-blocking)
 */

// Initialize Improv Wi-Fi (non-blocking, just sets up callbacks)
void initImprov(Configuration* config);

// Process incoming Improv Serial commands (call every loop iteration)
void handleImprov();

// Check if device was provisioned via Improv during this boot
bool isImprovProvisioned();

// Check if we have valid Wi-Fi credentials stored
bool hasValidWifiCredentials();

}  // namespace siebenuhr
