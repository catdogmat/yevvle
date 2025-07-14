#pragma once

#include <future>
#include <vector>

#include "spi.h"
#include "gps.h"
#include "lora.h"
#include "display.h"
#include "watchface.h"
#include "battery.h"
#include "touch.h"
#include "ui.h"
#include "time.h"

/* This is the primary class of the project.
 * It has the entry point from deepsleep as well as all
 * the code that handles the setup/menus/misc.
 */
class Core {
    std::vector<std::future<void>> mTasks;
    std::optional<uint32_t> mNextUpdate;

public:
    Core();

    void handleTouch();
    const UI::Any& findUi();
    UI::Any generateMenus();
    void regenerateMenus() { mUi.emplace(generateMenus()); };
    void setNextUpdate(uint32_t seconds);
    void finishTasks();

    void NTPSync();

    Time mTime;
    Battery mBattery;
    const bool mFirstTimeBoot;
    Spi mSpi;
    Display mDisplay;
    Gps mGps;
    Lora mLora;
    Touch mTouch;

    const tmElements_t& mNow;
    std::optional<UI::Any> mUi; // optional to allow re-generation of complex type

};