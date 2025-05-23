#include "EEPROMHandler.h"

namespace siebenuhr {

    EEPROMHandler::EEPROMHandler() : deferredWriteCount(0), lastFlushTime(0) {
        // Initialize EEPROM
        EEPROM.begin(512);  // Adjust size based on your EEPROM size
    }

    uint8_t EEPROMHandler::readFromEEPROM(uint8_t EEPROM_address) {
        return EEPROM.read(EEPROM_address);
    }

    String EEPROMHandler::readStringFromEEPROM(uint8_t EEPROM_address, int maxLength) {
        String result = "";
        for (int i = 0; i < maxLength; i++) {
            char c = EEPROM.read(EEPROM_address + i);
            if (c == 0) break;  // Null terminator
            result += c;
        }
        return result;
    }

    void EEPROMHandler::performWrite(uint8_t address, uint8_t value) {
        EEPROM.write(address, value);
        EEPROM.commit();
    }

    void EEPROMHandler::writeToEEPROM(uint8_t EEPROM_address, uint8_t value, uint32_t delay) {
        // Check if we should defer this write
        if (delay > 0) {
            // Add to deferred writes if there's space
            if (deferredWriteCount < MAX_DEFERRED_WRITES) {
                deferredWrites[deferredWriteCount].address = EEPROM_address;
                deferredWrites[deferredWriteCount].value = value;
                deferredWrites[deferredWriteCount].timestamp = millis();
                deferredWriteCount++;
            } else {
                // If buffer is full, force a flush
                flushDeferredSavingToEEPROM(true);
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

    void EEPROMHandler::writeStringToEEPROM(uint8_t EEPROM_address, String data, int maxLength) {
        // Ensure we don't write beyond maxLength
        int length = min(data.length(), (unsigned int)(maxLength - 1));
        
        // Write the string
        for (int i = 0; i < length; i++) {
            writeToEEPROM(EEPROM_address + i, data[i]);
        }
        
        // Write null terminator
        writeToEEPROM(EEPROM_address + length, 0);
    }

    void EEPROMHandler::flushDeferredSavingToEEPROM(bool forceFlush) {
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