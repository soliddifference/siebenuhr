#include "configuration.h"
#include "siebenuhr_logger.h"

namespace siebenuhr {

    Configuration::Configuration() 
    : deferredWriteCount(0)
    , lastFlushTime(0) 
    {
        // Open Preferences with namespace "siebenuhr"
        // false = read/write mode
        prefs.begin("siebenuhr", false);
        LOG_I("Preferences storage initialized");
    }

    Configuration::~Configuration() {
        prefs.end();
    }

    const char* Configuration::addressToKey(uint8_t address) {
        // Map legacy EEPROM addresses to Preferences keys
        switch (address) {
            case 0:  return ConfigKeys::INITIALIZED;
            case 1:  return ConfigKeys::TIMEZONE_ID;
            case 2:  return ConfigKeys::BRIGHTNESS;
            case 3:  return ConfigKeys::PERSONALITY;
            case 4:  return ConfigKeys::COLOR_R;
            case 5:  return ConfigKeys::COLOR_G;
            case 6:  return ConfigKeys::COLOR_B;
            case 20: return ConfigKeys::WIFI_SSID;
            case 60: return ConfigKeys::WIFI_PSWD;
            default: return nullptr;
        }
    }

    void Configuration::reset()
    {
        // Clear all stored preferences
        prefs.clear();
        LOG_I("Preferences cleared");
    }

    uint8_t Configuration::read(uint8_t address) {
        const char* key = addressToKey(address);
        if (key == nullptr) {
            LOG_E("Unknown address for read: %d", address);
            return 0;
        }
        return prefs.getUChar(key, 0);  // Default to 0 if not found
    }

    String Configuration::readString(uint8_t address, int maxLength) {
        const char* key = addressToKey(address);
        if (key == nullptr) {
            LOG_E("Unknown address for readString: %d", address);
            return "";
        }
        return prefs.getString(key, "");  // Default to empty string
    }

    void Configuration::performWrite(uint8_t address, uint8_t value) {
        const char* key = addressToKey(address);
        if (key == nullptr) {
            LOG_E("Unknown address for write: %d", address);
            return;
        }
        
        size_t written = prefs.putUChar(key, value);
        if (written == 0) {
            LOG_E("Preferences write FAILED for %s=%d", key, value);
        }
    }

    void Configuration::write(uint8_t address, uint8_t value, uint32_t delay) {
        // Check if we should defer this write
        if (delay > 0) {
            // Add to deferred writes if there's space
            if (deferredWriteCount < MAX_DEFERRED_WRITES) {
                // Check if we already have a pending write for this address
                for (int i = 0; i < deferredWriteCount; i++) {
                    if (deferredWrites[i].address == address) {
                        deferredWrites[i].value = value;
                        deferredWrites[i].timestamp = millis();
                        return;
                    }
                }

                deferredWrites[deferredWriteCount].address = address;
                deferredWrites[deferredWriteCount].value = value;
                deferredWrites[deferredWriteCount].timestamp = millis();
                deferredWriteCount++;
            } else {
                // If buffer is full, force a flush
                flushDeferredSaving(true);
                // Then add this write
                deferredWrites[0].address = address;
                deferredWrites[0].value = value;
                deferredWrites[0].timestamp = millis();
                deferredWriteCount = 1;
            }
        } else {
            // If no delay specified, write immediately
            performWrite(address, value);
        }
    }

    void Configuration::writeString(uint8_t address, String data, int maxLength) {
        const char* key = addressToKey(address);
        if (key == nullptr) {
            LOG_E("Unknown address for writeString: %d", address);
            return;
        }
        
        // Truncate if needed
        String truncated = data.substring(0, maxLength - 1);
        
        size_t written = prefs.putString(key, truncated);
        if (written == 0) {
            LOG_E("Preferences writeString FAILED for %s", key);
        }
    }

    void Configuration::flushDeferredSaving(bool forceFlush) {
        if (deferredWriteCount == 0) return;

        uint32_t currentTime = millis();
        
        // Check if it's time to flush (rate limit to 1 second)
        if (!forceFlush && (currentTime - lastFlushTime < 1000)) {
            return;
        }

        // Perform all pending writes
        for (uint8_t i = 0; i < deferredWriteCount; i++) {
            performWrite(deferredWrites[i].address, deferredWrites[i].value);
        }

        // Reset the deferred write buffer
        deferredWriteCount = 0;
        lastFlushTime = currentTime;
        LOG_I("Configuration flushed to Preferences.");
    } 
}
