
#include <fmt/format.h>

#include "core.h"
#include "deep_sleep.h"
#include "power.h"
#include "light.h"
#include "touch.h"
#include "peripherals.h"
#include "hardware.h"
#include "settings.h"

#include "watchface_default.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"

RTC_DATA_ATTR Settings kSettings;

Core::Core()
: mTime{kSettings.mTime}
, mFirstTimeBoot{[&]{
    static RTC_DATA_ATTR bool sDone = false;
    if (sDone)
        return false;
    sDone = true;

#if HW_VERSION < 10
    // Set all GPIOs to input that we are not using to avoid leaking power
    // This is NEEDED
    const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
    for (int i = 0; i < GPIO_NUM_MAX; i++) {
        if ((ignore >> i) & 0b1)
            continue;
        // ESP_LOGE("", "%d input", i);
        pinMode(i, INPUT);
    }
#else
    // // Set all GPIOs to input that we are not using to avoid leaking power
    // // This is NEEDED
    // const uint64_t ignore = 0b111111111110000000000000000000010001; // Ignore some GPIOs due to resets
    // for (int i = 0; i < GPIO_NUM_MAX; i++) {
    //     if ((ignore >> i) & 0b1)
    //         continue;
    //     // ESP_LOGE("", "%d input", i);
    //     pinMode(i, INPUT);
    // }
#endif

    // Recover Settings from Disk // TODO
    // load NVS and load settings
    // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    // Select default voltage 2.6V
    Power::low();
    Light::off();
    // HACK: Set a fixed time to start with
    struct timeval tv{.tv_sec=1723194200, .tv_usec=0};
    // struct timezone tz{.tz_minuteswest=60, .tz_dsttime=1};
    mTime.setTime(tv);
    // reset calibration to the ESP32
    mTime.calReset();

    return true;
}()}
, mDisplay{}
, mBattery{kSettings.mBattery}
, mTouch{kSettings.mTouch}
, mNow{mTime.getElements()}
, mUi{createMainMenu()}
{
    // Wake up reason affects how to proceed
    auto wakeup_reason = esp_sleep_get_wakeup_cause();
    // ESP_LOGE("", "boot %lu reason %d", micros(), wakeup_reason);
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TOUCHPAD: { // Touch!
        handleTouch();
    } break;
    case ESP_SLEEP_WAKEUP_TIMER: // Internal Timer
    case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm ?
        // Check alarms, vibration, beeps, etc.
        // Check watchdog -> reset to watchFace
        if (kSettings.mTouchWatchDog) {
            kSettings.mUi.mDepth = -1; // Reset to watchFace
        }
        kSettings.mTouchWatchDog = true;
        break;
    default: // Lets assume first time boot ?
        // ESP_LOGE("ev", "%d", (int)wakeup_reason);
        break;
    }

    // Beep conditions
    if (mNow.Minute == 0
        && wakeup_reason == ESP_SLEEP_WAKEUP_TIMER
        && mNow.Hour >= kSettings.mHourly.mFirst
        && mNow.Hour <= kSettings.mHourly.mLast)
    {
        if (kSettings.mHourly.mBeep) {
            mTasks.emplace_back(std::async(std::launch::async, []{
                Peripherals::speaker(
                    std::vector<std::pair<int,int>>{
                    {3200,100},{0,100},{3200,500}
                });
            }));
        }
        if (kSettings.mHourly.mVib) {
            mTasks.emplace_back(std::async(std::launch::async, []{
                Peripherals::vibrator(std::vector<int>{50,75,50});
            }));
        }
    }

    // Common display preparations, post UI Events processing
    auto& disp = kSettings.mDisplay;
    mDisplay.setRotation(disp.mRotation);
    mDisplay.setDarkBorder(disp.mDarkBorder ^ disp.mInvert);
    mDisplay.setInverted(!disp.mInvert);
    mDisplay.setTextColor(0xFF);

    // Show watch face or menu ?
    if (kSettings.mUi.mDepth < 0) {
        mDisplay.setRefreshMode(kSettings.mDisplay.mWatchLut);
        #define ARGS kSettings.mDisplay, kSettings.mWatchface, mDisplay, mBattery, mTime, mNow
        // Instantiate the watchface type we are using
        switch(kSettings.mWatchface.mType) {
            default: DefaultWatchface(ARGS).draw(); break;
            // case 0: break;
            // case 1: break;
            // case 2: break;
            // case 3: break;
        }
        #undef ARGS
        Light::off(); // Always turn off light exiting the Menus
    } else {
        mDisplay.setRefreshMode(kSettings.mDisplay.mMenuLut);
        kSettings.mWatchface.mLastDraw.mValid = false;
        std::visit([&](auto& e){
            if constexpr (has_render<decltype(e), Display&>::value) {
                e.render(mDisplay);
            } else {
                mDisplay.setTextSize(3);
                mDisplay.setTextColor(1, 0);
                mDisplay.println("UNIMPLEMENTED");
                mDisplay.println("PRESS BACK");
            }
        }, findUi());
    }
    
    // bool inverted = false;
    // while (true) {
    //     mDisplay.setInverted(inverted = !inverted);
    //     mDisplay.writeAllAndRefresh();
    //     if (mTouch.readAndClear() != Touch::Btn::NONE)
    //     break;
    // }

    // mDisplay.invertDisplay(true);
    // mDisplay.refresh();

    // Finish display, setup touch and finish pending tasks
    mDisplay.hibernate();
    mTouch.setUp(kSettings.mUi.mDepth < 0);
    mTasks.clear();
    mTouch.clear(); // Clear it again in case the tasks took too long

    
    // rtc_gpio_pullup_en((gpio_num_t)HW::Display::Res);
    // rtc_gpio_hold_en((gpio_num_t)HW::Display::Res);

    // gpio_dump_io_configuration(stdout, (15ULL << 4) | (3ULL << 19));
    // ESP_LOGE("deepSleep", "%ld", micros());

    // Calculate stepsize based on battery level or on battery save mode
    auto stepSize = [&] {
        if (kSettings.mPowerSave.mNight && mNow.Hour < 7)
            return 5;
        if (!kSettings.mPowerSave.mAuto)
            return 1;
        if (mBattery.mCurPercent < 100) {
            return 5;
        } else if (mBattery.mCurPercent < 200) {
            return 4;
        } else if (mBattery.mCurPercent < 500) {
            return 2;
        }
        return 1;
    }();

    // TODO: When there is an alarm, we need to wake up earlier
    auto nextFullWake = 60;
    auto firstMinutesSleep = stepSize - mNow.Minute % stepSize;
    auto nextPartialWake = firstMinutesSleep + mNow.Minute;
    // In case the step overflows, we need to chop it, and wake up earlier
    // kDSState.minutes will be exactly 0 after this trim
    if (nextPartialWake > nextFullWake)
        firstMinutesSleep -= nextPartialWake - nextFullWake;

    // ESP_LOGE("", "nextFullWake %d firstMinutesSleep %d nextPartialWake %d", nextFullWake, firstMinutesSleep, nextPartialWake);

    // We can only run wakeupstub when on watchface mode
    if (kSettings.mUi.mDepth < 0) {
        kDSState.currentMinutes = mNow.Minute + firstMinutesSleep;
        kDSState.minutes = nextFullWake - mNow.Minute - firstMinutesSleep;
        // ESP_LOGE("", "min %d step %d wait %ld", kDSState.minutes, stepSize, kDSState.updateWait);
        kDSState.stepSize = stepSize;
        // Only trigger the wakeupstub if there is any minute left
        if (kDSState.minutes > 0)
            esp_set_deep_sleep_wake_stub(&wake_stub_example);
    }

    auto nextMinute = (60 - mNow.Second) * 1'000'000 - mTime.getTimeval().tv_usec;
    esp_sleep_enable_timer_wakeup(nextMinute + (firstMinutesSleep - 1) * 60'000'000);
    esp_deep_sleep_start();
    ESP_LOGE("deepSleep", "never reach!");
}

const UI::Any& Core::findUi() {
    // Find current UI element in view by recursively finding deeper elements
    const UI::Any* item = &mUi;
    for (auto i=0; i < kSettings.mUi.mDepth; i++) {
        auto& index = kSettings.mUi.mState[i];
        std::visit([&](auto& e) {
            if constexpr (has_sub<decltype(e), uint8_t>::value) {
                item = &e.sub(index);
            }
        }, *item);
    }
    return *item;
}

void Core::handleTouch() {
    // Clear WatchDog since we received a valid touch
    kSettings.mTouchWatchDog = false;

    // Convert to what button was it
    Touch::Btn btn = mTouch.readAndClear();
    ESP_LOGE("parsed", "%d, %s", btn, std::string(magic_enum::enum_name(btn)).c_str());

    auto& ui = kSettings.mUi;
    // ESP_LOGE("ui", "depth%d st%d", ui.mDepth, ui.mState[ui.mDepth]);

    // Button press on the watchface
    if (ui.mDepth < 0) {
        if (btn == Touch::LIGHT) {
            Light::toggle();
        } else if (btn == Touch::MENU) {
            ui.mDepth = 0;
        }
        return;
    }

    // Button press on the UI is sent to the current selected item
    auto& item = findUi();

    // Send the touch event to the UI element to handle it, or fallback
    std::visit([&]<typename E>(const E& e) {
        if constexpr (has_button<E>::value) {
            e.button(btn);
        } else {
            // If a generic button handler is not implemented, try the specific ones
            if (btn & Touch::BACK) {
                if constexpr (has_button_back<E>::value) {
                    e.button_back();
                } else {
                    // Default option is to go back in the UI
                    ui.mDepth--;
                }
            } else if (btn & Touch::MENU) {
                if constexpr (has_button_menu<E>::value) {
                    e.button_menu();
                }
            } else {
                if constexpr (has_button_updown<E, int>::value) {
                    e.button_updown(btn == Touch::DOWN ? -1 : 1);
                }
            }
        }
    }, item);
}

#include <Arduino.h>
#include <WiFiManager.h>

void Core::NTPSync() {
  // Select default voltage 2.9V/3.3V for WiFi
  // We need Arduino for this (WiFi + NTP)
  Power::lock();
//   initArduino();

//   WiFi.waitForConnectResult();

//   settimeofday_cb([]() { // set callback to execute after time is retrieved
//     time_t now = time(nullptr);
//     setTime(now); // update time to TimeLib
//   });
//   configTime(0, 0, "pool.ntp.org");

//   // get network time
//   struct tm timeinfo;
//   if(!getLocalTime(&timeinfo)){
//     ESP_LOGE("NTP", "Failed to obtain time");
//     return;
//   }
//   delay(5'000);

  ESP_LOGE("NTP", "done");

  Power::unlock();
}