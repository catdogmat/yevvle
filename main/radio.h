#include <optional>
#include <string>
#include <vector>
#include <variant>

#include <Arduino.h>
#include <RadioLib.h>

#include "hardware.h"

namespace Signal {
  struct BasicOOK {
    float mFrequency;
    uint32_t mBitMicros;
    std::vector<uint8_t> mPattern;
    uint8_t mRepetitions = 1;
    uint32_t mDelayRepetitions = 0;
    void send() const;
  };
  using Sequence = std::variant<BasicOOK>;
  struct Group {
    std::string mName;
    std::vector<std::pair<std::string, Sequence>> mSequences;
  };
}
extern const std::vector<Signal::Group> kSignals;


/* Helper class to use the Radio HW module 
 */
class Radio {
public:
  struct Pck {
    std::string mData;
    float mSNR;
    float mRSSI;
    float mFreqError;
  };

  std::optional<Pck> mPck;

  Radio();

  static void startReceive();
  static void sleep();
  static void sendSignal(const Signal::Sequence& seq);

  bool sendLoraPck(const std::string& pck);
  void readLoraPck();
};

class OOK {
  uint32_t mBitUsDuration;
  int8_t mMinPower, mMaxPower;
  int16_t setOutputPowerFast(int8_t power);
public:
  OOK(uint32_t bitUsDuration, float freq, int8_t minPower = -9, int8_t maxPower = 22);
  ~OOK();
  void transmit(std::vector<uint8_t> seq);
};