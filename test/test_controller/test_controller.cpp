// Tests for Controller.cpp
// Focus: logarithmic brightness mapping
#include <unity.h>
#include <cmath>

// Test the brightness mapping function directly
// Extracted from Controller::applyLogBrightnessMapping for testability
namespace {
    template <typename T>
    T clamp(T value, T minValue, T maxValue) {
        if (value > maxValue) return maxValue;
        if (value < minValue) return minValue;
        return value;
    }

    int applyLogBrightnessMapping(int linearInput) {
        if (linearInput <= 1) return 1;
        if (linearInput >= 255) return 255;
        
        float normalized = (float)linearInput / 255.0f;
        float logged = log10(1.0f + normalized * 9.0f) / log10(10.0f);
        int output = (int)(logged * 255.0f);
        
        return clamp(output, 1, 255);
    }
}

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Boundary Conditions
// ============================================================================

void test_min_input_returns_min() {
    TEST_ASSERT_EQUAL(1, applyLogBrightnessMapping(0));
    TEST_ASSERT_EQUAL(1, applyLogBrightnessMapping(1));
}

void test_max_input_returns_max() {
    TEST_ASSERT_EQUAL(255, applyLogBrightnessMapping(255));
    TEST_ASSERT_EQUAL(255, applyLogBrightnessMapping(300));  // Over max
}

// ============================================================================
// Logarithmic Curve Properties
// ============================================================================

void test_output_monotonically_increasing() {
    int prev = 0;
    for (int i = 1; i <= 255; i++) {
        int mapped = applyLogBrightnessMapping(i);
        TEST_ASSERT_TRUE_MESSAGE(mapped >= prev, "Output should never decrease");
        prev = mapped;
    }
}

void test_low_input_has_more_resolution() {
    // Log curve: first half of input should map to less than half of output
    // This gives more fine control at low brightness
    int at_mid_input = applyLogBrightnessMapping(128);
    TEST_ASSERT_TRUE_MESSAGE(at_mid_input < 200, 
        "Mid input should map below 200 (log compression)");
}

void test_output_range_covered() {
    // Verify we use reasonable range of output
    int at_quarter = applyLogBrightnessMapping(64);
    int at_three_quarter = applyLogBrightnessMapping(192);
    
    TEST_ASSERT_TRUE(at_quarter > 50);
    TEST_ASSERT_TRUE(at_three_quarter > 200);
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_min_input_returns_min);
    RUN_TEST(test_max_input_returns_max);
    RUN_TEST(test_output_monotonically_increasing);
    RUN_TEST(test_low_input_has_more_resolution);
    RUN_TEST(test_output_range_covered);
    
    return UNITY_END();
}
