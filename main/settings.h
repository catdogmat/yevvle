
#pragma once

#include "battery.h"
#include "time.h"
#include "touch.h"
#include "watchface.h"
#include "display.h"

struct Settings {
    bool mTouchWatchDog : 1 {false};
    bool mLeakPinsSet : 1 {false};

    struct Hourly {
        bool mBeep {false};
        bool mVib {false};
        uint8_t mFirst {23};
        uint8_t mLast {0};
    } mHourly;

    UiSettings mUi;
    TimeSettings mTime;
    TouchSettings mTouch;
    BatterySettings mBattery;
    DisplaySettings mDisplay;
    WatchfaceSettings mWatchface;

    struct PowerSave {
        bool mNight {false};
        bool mAuto {true};
    } mPowerSave;
};
extern struct Settings kSettings;