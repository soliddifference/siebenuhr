#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace siebenuhr {

    // Key names for Preferences storage
    namespace ConfigKeys {
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

    // Legacy address mapping (for compatibility with existing code)
    enum class EEPROMAddress : uint8_t {
        INITIALISED = 0,
        TIMEZONE_ID,
        BRIGHTNESS,
        PERSONALITY,
        COLOR_R,
        COLOR_G,
        COLOR_B,
        WIFI_SSID = 20,
        WIFI_PSWD = 60,
    };

    constexpr uint8_t to_addr(EEPROMAddress addr) {
        return static_cast<uint8_t>(addr);
    }

    class Configuration {
    private:
        Preferences prefs;
        static const uint8_t MAX_DEFERRED_WRITES = 10;
        struct DeferredWrite {
            uint8_t address;
            uint8_t value;
            uint32_t timestamp;
        };
        DeferredWrite deferredWrites[MAX_DEFERRED_WRITES];
        uint8_t deferredWriteCount;
        uint32_t lastFlushTime;

        const char* addressToKey(uint8_t address);
        void performWrite(uint8_t address, uint8_t value);

    public:
        Configuration();
        ~Configuration();

        void reset();
        
        // Read operations
        uint8_t read(uint8_t address);
        String readString(uint8_t address, int maxLength = 40);
        
        // Write operations
        void write(uint8_t address, uint8_t value, uint32_t delay=10000);
        void writeString(uint8_t address, String data, int maxLength = 40);
        void flushDeferredSaving(bool forceFlush=false);
    };

}
