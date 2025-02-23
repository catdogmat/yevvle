/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include "esp_sleep.h"
#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "esp_rom_sys.h"
#include "esp_wake_stub.h"
#include "sdkconfig.h"

// For touch detection and Light on/off
#include "hal/touch_sensor_ll.h"
// For WDT feeding
#include "hal/wdt_hal.h"

#include "hardware.h"
#include "power.h"
#include "light.h"
#include "uspi.h"
#include "deep_sleep.h"

#include "esp_attr.h"
#include "soc/rtc_periph.h"
#include "rom/gpio.h"

RTC_DATA_ATTR DeepSleepState kDSState;

void RTC_IRAM_ATTR turnOffGpio() {
  using D = HW::Display;
  for (auto& pin : std::array{D::Cs, D::Dc, D::Res, D::Mosi, D::Sck}) {
    GPIO_DIS_OUTPUT(pin);
  }
#if (HW_VERSION < 10)
  GPIO_MODE_INPUT(19); // TODO: Make it using the variable HW::Display::Busy
#else
  GPIO_MODE_INPUT(7); // TODO: Make it using the variable HW::Display::Busy
#endif
}

void RTC_IRAM_ATTR feed_wdt() {
  // More than 500ms it is better to reset the MWDT0
#if(HW_VERSION < 10)
  TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
#else
  TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_V;
#endif
  TIMERG0.wdtfeed.wdt_feed = 1;
  TIMERG0.wdtwprotect.wdt_wkey = 0;
}

#if (HW_VERSION >= 10)
const RTC_DATA_ATTR rtc_io_desc_t descRes = rtc_io_desc[rtc_io_num_map[HW::Display::Res]];
#endif

void RTC_IRAM_ATTR microSleep(uint32_t micros) {
  constexpr auto step = 400'000;
  feed_wdt();
  while (micros > step) {
    micros -= step;
    esp_rom_delay_us(step);
    feed_wdt();
  }
  esp_rom_delay_us(micros);
  feed_wdt();
}

// wake up stub function stored in RTC memory
void RTC_IRAM_ATTR wake_stub_example(void)
{
  // This sets up the delay to work properly
#if(HW_VERSION < 10)
  auto& busyWait = kDSState.busyWait[getSetDisplayMode()];
  ets_update_cpu_frequency_rom(ets_get_detected_xtal_freq() / 1'000'000);
#else
  ets_update_cpu_frequency(ets_get_xtal_freq() / 1'000'000);
#endif

  // If we were waiting for a display finish, we need to complete it first
  if (kDSState.displayBusy) {
    kDSState.displayBusy = false;

    // Go back to low power mode
    Power::unlock();

    uSpi::init();

#if(HW_VERSION < 10)
    // Wait until display busy goes off (this will not be needed in next HW version
    GPIO_MODE_INPUT(19); // TODO: Make it using the variable HW::Display::Busy
    while(GPIO_INPUT_GET(19) != 0) {
      microSleep(busyWait.kWaitStep);
      busyWait.currentWait += busyWait.kWaitStep;
      busyWait.missedTimes = 0; // Reset it
    }
    busyWait.missedTimes = std::min(busyWait.missedTimes, uint8_t(16));
    uint32_t reduceAmount = busyWait.kReduce << ++busyWait.missedTimes;
    busyWait.currentWait -= std::min(busyWait.currentWait / 2, reduceAmount);
#else
    gpio_pin_wakeup_disable();
#endif

    if (kDSState.redrawDec) {
      kDSState.redrawDec = false;
      const auto& dec = kSettings.mWatchface.mCache.mDecimal;
      uSpi::writeArea(dec.data + dec.coord.size()*kSettings.mWatchface.mLastDraw.mMinuteD, dec.coord.x, dec.coord.y, dec.coord.w, dec.coord.h);
    }

    // Set display to sleep and go to sleep
    uSpi::hibernate();
    turnOffGpio();

    // Guess the amount to sleep until next one and advance counters
    kDSState.currentMinutes += kDSState.stepSize;
    kDSState.minutes -= kDSState.stepSize;
    auto minutes = kDSState.stepSize + (kDSState.minutes < 0 ? kDSState.minutes : 0);
    esp_wake_stub_set_wakeup_time(
      minutes * 60'000'000
#if(HW_VERSION < 10)
      - busyWait.currentWait
#endif
      );

    // Set stub entry, then going to deep sleep again.
    esp_wake_stub_sleep(&wake_stub_example);
  }

  // Light on press button?
  // 8 for timer // 256 for touch
  if (esp_wake_stub_get_wakeup_cause() == 256) {
    uint32_t mask;
    touch_ll_read_trigger_status_mask(&mask);

    // If it is the current set light pad button
    if ((mask >> kDSState.lightPad) & 1){
      touch_ll_clear_trigger_status_mask(); // This will consume the touch

      // LED is powered from VDD, and we need to rise it to 3.3V,
      // because 1.9V is too low for the LED to light up
      Light::toggle();

      // Go back to sleep, don´t touch the timer, if the user enters menu, then light will stay on
      esp_wake_stub_sleep(&wake_stub_example);
    }
    // Wake up, touch needs to handle by the Main code
    esp_default_wake_deep_sleep();
    return;
  }

  // Check if we should just do normal wakeup
  // If it is not a timer wakeup, return to handle on the Main code
  // 8 for timer // 256 for touch
  if (esp_wake_stub_get_wakeup_cause() != 8 || kDSState.minutes <= 0) {
    esp_default_wake_deep_sleep();
    return;
  }

  // Turn off the light if it is on when we do a normal update
  Light::off();

  // Reset display to wake it up
#if (HW_VERSION < 10)
  GPIO_MODE_OUTPUT(9); // TODO: Make it using the variable HW::Display::Res
#else
  CLEAR_PERI_REG_MASK(RTC_CNTL_PAD_HOLD_REG, descRes.hold_force);
  GPIO_MODE_OUTPUT(6); // TODO: Make it using the variable HW::Display::Res
#endif
  GPIO_OUTPUT_SET(HW::Display::Res, 0);
#if (HW_VERSION < 10)
  esp_rom_delay_us(1'000);
#else
  esp_rom_delay_us(500); // HW 3 suspiciusly is 2X this time
#endif
  GPIO_OUTPUT_SET(HW::Display::Res, 1);
#if (HW_VERSION >= 10)
  SET_PERI_REG_MASK(RTC_CNTL_PAD_HOLD_REG, descRes.hold_force);
#endif

  // Turn on high power mode since it makes display use less power (not sure why)
  // It will take around 125us * 0.1V, for 1.5V = 1.6ms
  Power::lock();

  // Calculate the areas to update based on time and watchface states
  const auto u = kDSState.currentMinutes % 10;
  const auto d = kDSState.currentMinutes / 10;

  auto& last = kSettings.mWatchface.mLastDraw;

  uSpi::init();
  if (u != last.mMinuteU[0]) {
    // Write minute U
    const auto& uni = kSettings.mWatchface.mCache.mUnits;
    uSpi::writeArea(uni.data + uni.coord.size()*u, uni.coord.x, uni.coord.y, uni.coord.w, uni.coord.h);
    last.mMinuteU[0] = last.mMinuteU[1];
    last.mMinuteU[1] = u;
  }
  if (d != last.mMinuteD) {
    // Write minute D + repeat it in the display hibernate
    const auto& dec = kSettings.mWatchface.mCache.mDecimal;
    uSpi::writeArea(dec.data + dec.coord.size()*d, dec.coord.x, dec.coord.y, dec.coord.w, dec.coord.h);
    last.mMinuteD = d;
    kDSState.redrawDec = true;
  }
  uSpi::refresh();
  turnOffGpio();
  kDSState.displayBusy = true;

#if (HW_VERSION < 10)
  // Set wakeup timer when we guess display will finish refreshing, to put display to hibernation
  esp_wake_stub_set_wakeup_time(busyWait.currentWait);
#else
  // Set the wakeup on busy of the display
  gpio_pin_wakeup_enable(HW::Display::Busy, GPIO_PIN_INTR_LOLEVEL);
#endif

  // Set stub entry, then going to deep sleep again.
  esp_wake_stub_sleep(&wake_stub_example);
}