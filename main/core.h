#pragma once

#include <future>
#include <vector>

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
public:
    void boot(); // Called when ESP32 starts up

    Core();

private:
    void firstTimeBoot();

    void handleTouch();
    const UI::Any& findUi();
    UI::Any createMainMenu();

    void NTPSync();

    Display mDisplay;
    Time mTime;
    Battery mBattery;
    Touch mTouch;

    std::vector<std::future<void>> mTasks;

    const tmElements_t& mNow;
    const UI::Any mUi;
};