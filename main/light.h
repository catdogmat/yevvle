
#pragma once

#include "esp_attr.h"

#include <atomic>

/* Helper class to handle light on/off and keeping it in deepsleep
 */
struct Light {
    static void RTC_SLOW_ATTR on() { set(true); }
    static void RTC_SLOW_ATTR off() { set(false); }
    static bool RTC_SLOW_ATTR toggle();
    static void RTC_SLOW_ATTR set(bool high);
    static void onFor(uint32_t ms);
};