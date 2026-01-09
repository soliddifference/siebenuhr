// Tests for timezone.h
// Focus: data integrity and bounds
#include <unity.h>
#include <cstring>
#include "timezone.h"

using namespace siebenuhr;

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Data Integrity
// ============================================================================

void test_timezone_array_not_empty() {
    TEST_ASSERT_TRUE(timezones.size() > 0);
}

void test_default_timezone_in_bounds() {
    TEST_ASSERT_TRUE(DEFAULT_TIMEZONE >= 0);
    TEST_ASSERT_TRUE(DEFAULT_TIMEZONE < (int)timezones.size());
}

void test_all_timezone_names_valid() {
    for (size_t i = 0; i < timezones.size(); i++) {
        TEST_ASSERT_NOT_NULL(timezones[i].name);
        TEST_ASSERT_TRUE_MESSAGE(strlen(timezones[i].name) > 0, 
            "Timezone name should not be empty");
    }
}

void test_timezone_offsets_reasonable() {
    // UTC offsets range from -12 to +14
    for (size_t i = 0; i < timezones.size(); i++) {
        TEST_ASSERT_TRUE(timezones[i].offset >= -12);
        TEST_ASSERT_TRUE(timezones[i].offset <= 14);
    }
}

void test_default_is_zurich() {
    // Verify DEFAULT_TIMEZONE points to expected value
    TEST_ASSERT_EQUAL_STRING("Europe/Zurich", timezones[DEFAULT_TIMEZONE].name);
}

void test_expected_timezones_present() {
    // Spot check some expected entries exist
    bool found_london = false, found_tokyo = false, found_la = false;
    
    for (const auto& tz : timezones) {
        if (strcmp(tz.name, "Europe/London") == 0) found_london = true;
        if (strcmp(tz.name, "Asia/Tokyo") == 0) found_tokyo = true;
        if (strcmp(tz.name, "America/Los_Angeles") == 0) found_la = true;
    }
    
    TEST_ASSERT_TRUE(found_london);
    TEST_ASSERT_TRUE(found_tokyo);
    TEST_ASSERT_TRUE(found_la);
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_timezone_array_not_empty);
    RUN_TEST(test_default_timezone_in_bounds);
    RUN_TEST(test_all_timezone_names_valid);
    RUN_TEST(test_timezone_offsets_reasonable);
    RUN_TEST(test_default_is_zurich);
    RUN_TEST(test_expected_timezones_present);
    
    return UNITY_END();
}
