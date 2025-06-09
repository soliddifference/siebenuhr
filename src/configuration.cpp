#include "configuration.h"

namespace siebenuhr {

    Configuration::Configuration() 
    : deferredWriteCount(0)
    , lastFlushTime(0) 
    {
        // Initialize EEPROM
        EEPROM.begin(512);  // Adjust size based on your EEPROM size
    }

    void Configuration::reset()
    {
        for (int i = 0; i < EEPROM.length(); i++) {
            EEPROM.write(i, 0x00);
        }        
    }

    uint8_t Configuration::read(uint8_t EEPROM_address) {
        return EEPROM.read(EEPROM_address);
    }

    String Configuration::readString(uint8_t EEPROM_address, int maxLength) {
        String result = "";
        for (int i = 0; i < maxLength; i++) {
            char c = EEPROM.read(EEPROM_address + i);
            if (c == 0) break;  // Null terminator
            result += c;
        }
        return result;
    }

    void Configuration::performWrite(uint8_t address, uint8_t value) {
        EEPROM.write(address, value);
        EEPROM.commit();
    }

    void Configuration::write(uint8_t EEPROM_address, uint8_t value, uint32_t delay) {
        // Check if we should defer this write
        if (delay > 0) {
            // Add to deferred writes if there's space
            if (deferredWriteCount < MAX_DEFERRED_WRITES) {
                for (int i=0;i<deferredWriteCount;i++)
                {
                    if (deferredWrites[i].address == EEPROM_address) 
                    {
                        deferredWrites[i].address = EEPROM_address;
                        deferredWrites[i].value = value;
                        deferredWrites[i].timestamp = millis();
                        return;
                    }
                }

                deferredWrites[deferredWriteCount].address = EEPROM_address;
                deferredWrites[deferredWriteCount].value = value;
                deferredWrites[deferredWriteCount].timestamp = millis();
                deferredWriteCount++;
            } else {
                // If buffer is full, force a flush
                flushDeferredSaving(true);
                // Then add this write
                deferredWrites[0].address = EEPROM_address;
                deferredWrites[0].value = value;
                deferredWrites[0].timestamp = millis();
                deferredWriteCount = 1;
            }
        } else {
            // If no delay specified, write immediately
            performWrite(EEPROM_address, value);
        }
    }

    void Configuration::writeString(uint8_t EEPROM_address, String data, int maxLength) {
        // Ensure we don't write beyond maxLength
        int length = min(data.length(), (unsigned int)(maxLength - 1));
        
        // Write the string
        for (int i = 0; i < length; i++) {
            write(EEPROM_address + i, data[i]);
        }
        
        // Write null terminator
        write(EEPROM_address + length, 0);
    }

    void Configuration::flushDeferredSaving(bool forceFlush) {
        if (deferredWriteCount == 0) return;

        uint32_t currentTime = millis();
        
        // Check if it's time to flush
        if (!forceFlush && (currentTime - lastFlushTime < 1000)) {
            return;  // Don't flush too frequently
        }

        // Perform all pending writes
        for (uint8_t i = 0; i < deferredWriteCount; i++) {
            performWrite(deferredWrites[i].address, deferredWrites[i].value);
        }

        // Reset the deferred write buffer
        deferredWriteCount = 0;
        lastFlushTime = currentTime;
    } 

}