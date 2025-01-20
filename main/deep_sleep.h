#include <inttypes.h>

#include "display.h"

struct DeepSleepState {
#if(HW_VERSION < 3)
  // This is needed because the HW pin is not properly wired to a RTC GPIO
  struct BusyWait {
    constexpr static auto kStartWait = 25'000u; // Super low value to start with
    constexpr static auto kReduce = 500u;
    constexpr static auto kWaitStep = 100u;

    uint32_t currentWait {kStartWait};
    uint8_t missedTimes {0};
  } busyWait[magic_enum::enum_count<DisplayMode>()];
#endif

  // Display minute update variables
  uint8_t currentMinutes {0};
  uint8_t stepSize {1};
  int8_t minutes {10};
  bool redrawDec {false};
  bool displayBusy {false};

  // Light
  uint8_t lightPad {0};
};

extern struct DeepSleepState kDSState;

extern void wake_stub_example(void);