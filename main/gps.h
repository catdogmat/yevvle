#include <optional>
#include <TimeLib.h>

#include "hardware.h"

/* Helper class to read/configure GPS as well as
 * turning it on/off and keeping it in deepsleep
 */
class Gps {
public:
  static struct Data {
    struct Location {
      double mLat;
      double mLon;
    };
    struct DateTime {
      tmElements_t mElements;
      uint8_t mCentiSeconds;
    };
    std::optional<Location> mLocation;
    std::optional<DateTime> mDateTime;
    std::optional<float> mSpeed;
    std::optional<float> mDirection;
  } mData;

  void on() const { set(true); }
  void off() const { set(false); }
  void set(bool high) const;
  bool isOn() const;

  bool read() const;
};
