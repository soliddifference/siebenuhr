// Mock implementations
#include "Arduino.h"
#include "Preferences.h"

MockSerial Serial;

std::map<std::string, uint8_t> Preferences::_uchar_store;
std::map<std::string, std::string> Preferences::_string_store;
