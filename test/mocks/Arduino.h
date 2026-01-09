// Mock Arduino.h for native testing
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

typedef uint8_t byte;
typedef bool boolean;

#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0

inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline void delay(unsigned long) {}

// Controllable millis() for timing tests
static unsigned long _mock_millis = 0;
inline unsigned long millis() { return _mock_millis; }
inline void _set_mock_millis(unsigned long ms) { _mock_millis = ms; }
inline void _advance_mock_millis(unsigned long ms) { _mock_millis += ms; }

// Minimal String class
class String {
public:
    String() : _str() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(int val) : _str(std::to_string(val)) {}
    
    const char* c_str() const { return _str.c_str(); }
    size_t length() const { return _str.length(); }
    int toInt() const { return std::stoi(_str); }
    String substring(size_t from, size_t to) const { 
        return String(_str.substr(from, to - from)); 
    }
    
    bool operator==(const String& o) const { return _str == o._str; }
    String& operator+=(const String& o) { _str += o._str; return *this; }
    
private:
    std::string _str;
};

class MockSerial {
public:
    void begin(unsigned long) {}
    void print(const char* s) { printf("%s", s); }
    void println(const char* s = "") { printf("%s\n", s); }
};

extern MockSerial Serial;
