#ifndef TIMEZONES_H
#define TIMEZONES_H

#include <Arduino.h>
#include <vector>

// Timezone structure
struct STimezone {
    const char* name;
    int offset;
};

// List of time zones
const std::vector<STimezone> __timezones = {
    {"Pacific/Pago_Pago", -11},
    {"Pacific/Honolulu", -10},
    {"America/Anchorage", -9},
    {"America/Los_Angeles", -8},
    {"America/Denver", -7},
    {"America/Chicago", -6},
    {"America/New_York", -5},
    {"America/Argentina/Buenos_Aires", -3},
    {"Europe/London", 0},
    {"Europe/Paris", 1},
    {"Europe/Berlin", 2},
    {"Europe/Zurich", 2},
    {"Europe/Moscow", 3},
    {"Asia/Dubai", 4},
    {"Asia/Kolkata", 5},
    {"Asia/Jakarta", 7},
    {"Asia/Tokyo", 9},
    {"Australia/Sydney", 10},
    {"Pacific/Auckland", 12}
    // Add more time zones as needed
};

#endif  // TIMEZONES_H