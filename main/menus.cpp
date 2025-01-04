#include "core.h"
#include "light.h"
#include "touch.h"
#include "peripherals.h"
#include "hardware.h"
#include "settings.h"
#include "ble.h"

UI::Any Core::createMainMenu() {
  return UI::Menu{"Main Menu", {
      UI::Menu{"Clock", {
        UI::DateTime{"Set DateTime", mTime},
        UI::Menu{"Calibration", {
          UI::Action{"Sync", [&]{ mTime.calUpdate(); }},
          UI::Action{"Reset", [&]{ mTime.calReset(); }},
          UI::Text{[&] -> std::string {
            if (!kSettings.mTime.mSync)
              return "\n Not calibrated\n Set Date/Time\n then press Sync";
            auto& sync = *kSettings.mTime.mSync;
            tmElements_t last;
            breakTime(sync.mTime.tv_sec, last);
            char lastTime[32];
            std::sprintf(lastTime, "\n  %02d:%02d:%02d\n  %02d/%02d/%04d", last.Hour, last.Minute, last.Second, last.Day, last.Month, last.Year + 1970);
            auto elapsed = mTime.getTimeval().tv_sec - sync.mTime.tv_sec;
            auto actuallyElapsed = elapsed - kSettings.mTime.mSync->mDrift.tv_sec;
            bool megasec = actuallyElapsed > 1'000'000;
            char ppm[10];
            std::sprintf(ppm, "%+.2f", mTime.getPpm());

            return "\n Last sync on:" + std::string(lastTime) +
              "\n Err: " + std::to_string(kSettings.mTime.mSync->mDrift.tv_sec) +
              "/" + std::to_string(actuallyElapsed / (megasec ? 1'000'000 : 1)) + (megasec ? "M" : "") +
              "\n PPM:" + std::string(ppm) +
              "\n Cal:" + std::to_string(kSettings.mTime.mCalibration);
          }},
        }},
      UI::Menu{"Hour Beep", {
        UI::Bool{"Beep", kSettings.mHourly.mBeep },
        UI::Bool{"Vibrate", kSettings.mHourly.mVib },
        UI::Loop<uint8_t>{"First Hour", kSettings.mHourly.mFirst, 24 },
        UI::Loop<uint8_t>{"Last Hour", kSettings.mHourly.mLast, 24 },
      }},
      UI::Menu{"Alarms", {
      }},
      UI::Action{"NTP", [&]{
        NTPSync();
      }},
    }},
    UI::Menu{"Watchface", {
      // UI::Loop<int>{"Style",
      //     []() -> int { return kSettings.mWatchface.mType; },
      //     [](){ kSettings.mWatchface.mType = (kSettings.mWatchface.mType + 1) % 4; }
      // },
      UI::Bool{"Show Battery %", kSettings.mWatchface.mConfig.mBattery},
      UI::Bool{"Moon Phases", kSettings.mWatchface.mConfig.mMoon},
      UI::Bool{"Sunset/Sunrise", kSettings.mWatchface.mConfig.mSun},
      UI::Bool{"Tides", kSettings.mWatchface.mConfig.mTides},
    }},
    UI::Menu{"Display", {
      UI::Bool{"Invert", kSettings.mDisplay.mInvert },
      UI::Bool{"Border", kSettings.mDisplay.mDarkBorder },
      UI::Loop<uint8_t>{"Rotation", kSettings.mDisplay.mRotation, 4},
      UI::Loop<DisplayMode>{"Menu Lut", kSettings.mDisplay.mMenuLut },
      UI::Loop<DisplayMode>{"Watch Lut", kSettings.mDisplay.mWatchLut },
    }},
    UI::Menu{"Power Save", {
      UI::Bool{"Night (0-6am)", kSettings.mPowerSave.mNight },
      UI::Bool{"Auto (bat <25%)", kSettings.mPowerSave.mAuto },
    }},
    UI::Menu{"Touch", {
      UI::Loop<MeasureRate>{"Menu Rate", kSettings.mTouch.mRate[0]},
      UI::Loop<MeasureRate>{"Watch Rate", kSettings.mTouch.mRate[1]},
      // UI::Loop<MeasureCycles>{"Menu", kSettings.mTouch.mCycles[0]},
      // UI::Loop<MeasureCycles>{"Watch", kSettings.mTouch.mCycles[1]},
    }},
    UI::Menu{"Test", {
      UI::Action{"Vib 2x75ms", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Peripherals::vibrator(std::vector<int>{75,75,75});
        }));
      }},
      UI::Action{"Vib 1x75ms", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Peripherals::vibrator(std::vector<int>{75});
        }));
      }},
      UI::Action{"Vib 200ms", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Peripherals::vibrator(std::vector<int>{200});
        }));
      }},
      UI::Action{"Beep", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Peripherals::speaker(
            std::vector<std::pair<int,int>>{
            {3200,100},{0,100},{3200,100}});
        }));
      }},
      UI::Action{"Tetris Sound", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Peripherals::tetris();
        }));
      }},
      UI::Action{"BLE Test", [&]{
        extern void ble_main(void);
        mTasks.emplace_back(std::async(std::launch::async, []{
          ble_main();
          delay(50'000);
        }));
      }},
      UI::Action{"Light toggle", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Light::toggle();
        }));
      }},
      UI::Action{"Display Restore", [&]{
        mDisplay.setRefreshMode(DisplayMode::FULL);
        bool inverted = false;
        while (true) {
          mDisplay.setInverted(inverted = !inverted);
          mDisplay.writeAllAndRefresh();
          if (mTouch.readAndClear() != Touch::Btn::NONE)
            break;
        }
      }},
      UI::Action{"Parallel All", [&]{
        mTasks.emplace_back(std::async(std::launch::async, []{
          Peripherals::tetris();
        }));
        mTasks.emplace_back(std::async(std::launch::async, []{
          for (auto i=0; i<10; i++) {
            delay(1300);
            Peripherals::vibrator(std::vector<int>{70});
          }
        }));
        mTasks.emplace_back(std::async(std::launch::async, []{
          for (auto i=0; i<30; i++) {
            delay(1000);
            Light::toggle();
          }
        }));
      }},
    }}
  }};
};
