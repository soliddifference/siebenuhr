#include "configuration.h"
#include "siebenuhr_logger.h"
#include <cstring>

namespace siebenuhr {

    Configuration::Configuration() 
    : deferredWriteCount(0)
    , lastFlushTime(0) 
    {
        prefs.begin("siebenuhr", false);
        LOG_I("Preferences storage initialized");
    }

    Configuration::~Configuration() {
        prefs.end();
    }

    void Configuration::reset() {
        prefs.clear();
        LOG_I("Preferences cleared");
    }

    uint8_t Configuration::read(const char* key) {
        return prefs.getUChar(key, 0);
    }

    String Configuration::readString(const char* key) {
        return prefs.getString(key, "");
    }

    void Configuration::performWrite(const char* key, uint8_t value) {
        size_t written = prefs.putUChar(key, value);
        if (written == 0) {
            LOG_E("Preferences write FAILED for %s=%d", key, value);
        }
    }

    void Configuration::write(const char* key, uint8_t value, uint32_t delay) {
        if (delay > 0) {
            // Check if we already have a pending write for this key
            for (int i = 0; i < deferredWriteCount; i++) {
                if (strcmp(deferredWrites[i].key, key) == 0) {
                    deferredWrites[i].value = value;
                    deferredWrites[i].timestamp = millis();
                    return;
                }
            }

            // Add to deferred writes if there's space
            if (deferredWriteCount < MAX_DEFERRED_WRITES) {
                deferredWrites[deferredWriteCount].key = key;
                deferredWrites[deferredWriteCount].value = value;
                deferredWrites[deferredWriteCount].timestamp = millis();
                deferredWriteCount++;
            } else {
                // If buffer is full, force a flush then add this write
                flushDeferredSaving(true);
                deferredWrites[0].key = key;
                deferredWrites[0].value = value;
                deferredWrites[0].timestamp = millis();
                deferredWriteCount = 1;
            }
        } else {
            // No delay - write immediately
            performWrite(key, value);
        }
    }

    void Configuration::writeString(const char* key, const String& data) {
        size_t written = prefs.putString(key, data);
        if (written == 0) {
            LOG_E("Preferences writeString FAILED for %s", key);
        }
    }

    void Configuration::flushDeferredSaving(bool forceFlush) {
        if (deferredWriteCount == 0) return;

        uint32_t currentTime = millis();
        
        // Rate limit to 1 second between flushes
        if (!forceFlush && (currentTime - lastFlushTime < 1000)) {
            return;
        }

        for (uint8_t i = 0; i < deferredWriteCount; i++) {
            performWrite(deferredWrites[i].key, deferredWrites[i].value);
        }

        deferredWriteCount = 0;
        lastFlushTime = currentTime;
        LOG_I("Configuration flushed to Preferences.");
    } 
}
