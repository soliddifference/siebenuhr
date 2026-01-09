#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace siebenuhr {

    // Key names for Preferences storage
    namespace ConfigKey {
        constexpr const char* INITIALIZED = "init";
        constexpr const char* TIMEZONE_ID = "tz";
        constexpr const char* BRIGHTNESS = "bright";
        constexpr const char* PERSONALITY = "pers";
        constexpr const char* COLOR_R = "col_r";
        constexpr const char* COLOR_G = "col_g";
        constexpr const char* COLOR_B = "col_b";
        constexpr const char* WIFI_SSID = "ssid";
        constexpr const char* WIFI_PSWD = "pswd";
    }

    class Configuration {
    private:
        Preferences prefs;
        static const uint8_t MAX_DEFERRED_WRITES = 10;
        struct DeferredWrite {
            const char* key;
            uint8_t value;
            uint32_t timestamp;
        };
        DeferredWrite deferredWrites[MAX_DEFERRED_WRITES];
        uint8_t deferredWriteCount;
        uint32_t lastFlushTime;

        void performWrite(const char* key, uint8_t value);

    public:
        Configuration();
        ~Configuration();

        void reset();
        
        // Read operations
        uint8_t read(const char* key);
        String readString(const char* key);
        
        // Write operations
        void write(const char* key, uint8_t value, uint32_t delay = 10000);
        void writeString(const char* key, const String& data);
        void flushDeferredSaving(bool forceFlush = false);
    };

}
