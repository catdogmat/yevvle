#include <optional>
#include <string>

#include <Arduino.h>
#include <RadioLib.h>

#include "hardware.h"

/* Helper class to use the lora HW module 
 */
class Lora {
public:
  struct Pck {
    std::string mData;
    float mSNR;
    float mRSSI;
    float mFreqError;
  };

  std::optional<Pck> mPck;

  Lora();

  void startReceive();
  void sleep();

  bool send(const std::string& pck);
  void receive();
};
