// Mock Preferences.h for native testing
// Simulates ESP32 Preferences API with in-memory storage
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include "Arduino.h"

class Preferences {
public:
    bool begin(const char* name, bool readOnly = false) {
        (void)name; (void)readOnly;
        return true;
    }
    
    void end() {}
    
    bool clear() {
        _uchar_store.clear();
        _string_store.clear();
        return true;
    }
    
    // UChar storage
    size_t putUChar(const char* key, uint8_t value) {
        _uchar_store[key] = value;
        return 1;
    }
    
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
        auto it = _uchar_store.find(key);
        return (it != _uchar_store.end()) ? it->second : defaultValue;
    }
    
    // String storage
    size_t putString(const char* key, const String& value) {
        _string_store[key] = value.c_str();
        return value.length();
    }
    
    String getString(const char* key, const String& defaultValue = "") {
        auto it = _string_store.find(key);
        return (it != _string_store.end()) ? String(it->second.c_str()) : defaultValue;
    }
    
    // Test helpers
    static void _reset_all() {
        _uchar_store.clear();
        _string_store.clear();
    }
    
    static size_t _write_count() { return _uchar_store.size() + _string_store.size(); }
    
private:
    static std::map<std::string, uint8_t> _uchar_store;
    static std::map<std::string, std::string> _string_store;
};
