#include <Arduino.h>
#include <unity.h>

// ฟังก์ชันที่เราจะทดสอบ
float celsiusToFahrenheit(float c) {
    return c * 1.8 + 32;
}

void test_conversion() {
    TEST_ASSERT_EQUAL_FLOAT(32.0, celsiusToFahrenheit(0));
    TEST_ASSERT_EQUAL_FLOAT(212.0, celsiusToFahrenheit(100));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_conversion);
    UNITY_END();
}

void loop() {
    // ไม่ต้องใช้ loop
}
