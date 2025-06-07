
#pragma once

#include "esp_attr.h"

#include <atomic>

/* Helper class to handle extra power needed for WiFi/Display operations
 */
struct Power {
    static void RTC_SLOW_ATTR high() { set(true); }
    static void RTC_SLOW_ATTR low() { set(false); }
    static void RTC_SLOW_ATTR set(bool high);

    static void RTC_SLOW_ATTR lock();
    static void RTC_SLOW_ATTR unlock();

    static bool RTC_SLOW_ATTR current();
};