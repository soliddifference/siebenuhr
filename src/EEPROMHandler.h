#pragma once

#include <Arduino.h>
#include <EEPROM.h>

namespace siebenuhr {

    class EEPROMHandler {
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
        EEPROMHandler();
        
        // EEPROM read operations
        uint8_t readFromEEPROM(uint8_t EEPROM_address);
        String readStringFromEEPROM(uint8_t EEPROM_address, int maxLength);
        
        // EEPROM write operations
        void writeToEEPROM(uint8_t EEPROM_address, uint8_t value, uint32_t delay=10000);
        void writeStringToEEPROM(uint8_t EEPROM_address, String data, int maxLength);
        void flushDeferredSavingToEEPROM(bool forceFlush=false);
    };

}