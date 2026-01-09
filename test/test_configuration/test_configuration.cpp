// Tests for configuration.cpp
// Focus: address mapping, deferred write buffer logic
#include <unity.h>

// Include mocks and source directly for native test linking
#include "../mocks/mocks.cpp"
#include "../../src/configuration.cpp"

using namespace siebenuhr;

static Configuration* config;

void setUp(void) {
    _set_mock_millis(0);
    Preferences::_reset_all();
    config = new Configuration();
}

void tearDown(void) {
    delete config;
    config = nullptr;
}

// ============================================================================
// Address-to-Key Mapping (tested via read/write)
// ============================================================================

void test_known_addresses_readable() {
    // All known addresses should read without error (return 0 default)
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::INITIALISED)));
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::TIMEZONE_ID)));
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::BRIGHTNESS)));
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::PERSONALITY)));
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::COLOR_R)));
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::COLOR_G)));
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::COLOR_B)));
}

void test_write_immediate_then_read() {
    // Write with delay=0 should be immediate
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 128, 0);
    TEST_ASSERT_EQUAL(128, config->read(to_addr(EEPROMAddress::BRIGHTNESS)));
}

void test_string_roundtrip() {
    config->writeString(to_addr(EEPROMAddress::WIFI_SSID), "TestNetwork");
    String result = config->readString(to_addr(EEPROMAddress::WIFI_SSID));
    TEST_ASSERT_EQUAL_STRING("TestNetwork", result.c_str());
}

void test_unknown_address_returns_zero() {
    // Address 99 is not mapped
    TEST_ASSERT_EQUAL(0, config->read(99));
}

// ============================================================================
// Deferred Write Buffer
// ============================================================================

void test_deferred_write_not_immediate() {
    // Write with delay>0 should not be visible until flush
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 200, 1000);
    
    // Value not yet persisted (mock Preferences won't have it)
    Preferences prefs;
    prefs.begin("siebenuhr");
    TEST_ASSERT_EQUAL(0, prefs.getUChar("bright", 0));
}

void test_deferred_write_updates_same_address() {
    // Multiple writes to same address should update, not accumulate
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 100, 1000);
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 150, 1000);
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 200, 1000);
    
    // Force flush
    config->flushDeferredSaving(true);
    
    // Should have final value
    TEST_ASSERT_EQUAL(200, config->read(to_addr(EEPROMAddress::BRIGHTNESS)));
}

void test_flush_rate_limited() {
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 100, 1000);
    
    // First flush should work
    _set_mock_millis(100);
    config->flushDeferredSaving(false);
    
    // Add another write
    config->write(to_addr(EEPROMAddress::PERSONALITY), 2, 1000);
    
    // Second flush within 1 second should be skipped (rate limited)
    _set_mock_millis(500);
    config->flushDeferredSaving(false);
    
    // Personality should not be persisted yet
    Preferences prefs;
    prefs.begin("siebenuhr");
    TEST_ASSERT_EQUAL(0, prefs.getUChar("pers", 0));
    
    // After 1 second, flush should work
    _set_mock_millis(1200);
    config->flushDeferredSaving(false);
    TEST_ASSERT_EQUAL(2, config->read(to_addr(EEPROMAddress::PERSONALITY)));
}

void test_force_flush_ignores_rate_limit() {
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 100, 1000);
    config->flushDeferredSaving(true);
    
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 200, 1000);
    _set_mock_millis(10);  // Only 10ms later
    config->flushDeferredSaving(true);  // Force should work
    
    TEST_ASSERT_EQUAL(200, config->read(to_addr(EEPROMAddress::BRIGHTNESS)));
}

void test_reset_clears_all() {
    config->write(to_addr(EEPROMAddress::BRIGHTNESS), 128, 0);
    config->writeString(to_addr(EEPROMAddress::WIFI_SSID), "Network");
    
    config->reset();
    
    TEST_ASSERT_EQUAL(0, config->read(to_addr(EEPROMAddress::BRIGHTNESS)));
    TEST_ASSERT_EQUAL_STRING("", config->readString(to_addr(EEPROMAddress::WIFI_SSID)).c_str());
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Address mapping
    RUN_TEST(test_known_addresses_readable);
    RUN_TEST(test_write_immediate_then_read);
    RUN_TEST(test_string_roundtrip);
    RUN_TEST(test_unknown_address_returns_zero);
    
    // Deferred writes
    RUN_TEST(test_deferred_write_not_immediate);
    RUN_TEST(test_deferred_write_updates_same_address);
    RUN_TEST(test_flush_rate_limited);
    RUN_TEST(test_force_flush_ignores_rate_limit);
    RUN_TEST(test_reset_clears_all);
    
    return UNITY_END();
}
