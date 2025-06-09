#pragma once

#include <Arduino.h>
#include <EEPROM.h>

namespace siebenuhr {

    static const int EEPROM_ADDRESS_INITIALIZED = 0;

    enum class EEPROMAddress : uint8_t {
        // Basic configuration
        INITIALISED = 0,
        TIMEZONE_ID,
        BRIGHTNESS,
        COLOR_R,
        COLOR_G,
        COLOR_B,

        // String storage 
        WIFI_SSID = 20,        // 40 bytes
        WIFI_PSWD = 60,        // 40 bytes
    };

    constexpr uint8_t to_addr(EEPROMAddress addr) {
        return static_cast<uint8_t>(addr);
    }

    class Configuration {
    private:
        static const uint8_t MAX_DEFERRED_WRITES = 10;
        struct DeferredWrite {
            uint8_t address;
            uint8_t value;
            uint32_t timestamp;
        };
        DeferredWrite deferredWrites[MAX_DEFERRED_WRITES];
        uint8_t deferredWriteCount;
        uint32_t lastFlushTime;

        void performWrite(uint8_t address, uint8_t value);

    public:
        Configuration();

        void reset();
        
        // EEPROM read operations
        uint8_t read(uint8_t EEPROM_address);
        String readString(uint8_t EEPROM_address, int maxLength = 40);
        
        // EEPROM write operations
        void write(uint8_t EEPROM_address, uint8_t value, uint32_t delay=10000);
        void writeString(uint8_t EEPROM_address, String data, int maxLength = 40);
        void flushDeferredSaving(bool forceFlush=false);
    };

}