#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "lora.h"

namespace {
  RTC_DATA_ATTR std::optional<SPISettings> kSpi;
  RTC_DATA_ATTR std::optional<ArduinoHal> kHal;
  RTC_DATA_ATTR std::optional<Module> kModule;
  RTC_DATA_ATTR std::optional<SX1262> kRadio;
}

std::vector<uint8_t> closeSeq = {0x88, 0x8e, 0xee, 0xee, 0x88, 0x8e, 0x88, 0xe8, 0x8e, 0x88, 0x8e, 0x88, 0x80};
std::vector<uint8_t> openSeq  = {0x88, 0x8e, 0xee, 0xee, 0x88, 0x8e, 0x88, 0xe8, 0x8e, 0x88, 0xe8, 0x88, 0x80};

class OOK {
  uint32_t mBitUsDuration;
  int8_t mMinPower, mMaxPower;

public:
  OOK(uint32_t bitUsDuration, float freq, int8_t minPower = -9, int8_t maxPower = 22)
  : mBitUsDuration(bitUsDuration)
  , mMinPower(minPower)
  , mMaxPower(maxPower)
  {
    if (int16_t err = kRadio->beginFSK(freq)) {
      ESP_LOGE("Lora", "beginFSK error: %d", err);
    }
    kRadio->setFrequency(freq);
    kRadio->transmitDirect();
    kRadio->setOutputPower(mMinPower);  // Initially set to low
  }
  ~OOK() {
    // TODO: How to fix the sleep power
    // kRadio->sleep();
    kRadio->sleep(false); // ~0.3 uA @ 3.3V
    // Hold some pins to some values to avoid leaking current
    rtc_gpio_hold_en((gpio_num_t)HW::Lora::Cs);
  }

  int16_t setOutputPowerFast(int8_t power) {
    if (auto state = kRadio->checkOutputPower(power, NULL))
      return state;
    const uint8_t data[] = { static_cast<uint8_t>(power), RADIOLIB_SX126X_PA_RAMP_10U };
    return kModule->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_TX_PARAMS, data, 2);
  }

  void transmit(std::vector<uint8_t> seq){
    RadioLibTime_t start = kHal->micros();
    for (size_t byteIdx = 0; byteIdx < seq.size(); ++byteIdx) {
      uint8_t byte = seq[byteIdx];
      for (int bitIdx = 7; bitIdx >= 0; --bitIdx) {
        bool bit = (byte >> bitIdx) & 0x01;
        setOutputPowerFast(bit ? mMaxPower : mMinPower);
        kModule->waitForMicroseconds(start, mBitUsDuration);
        start += mBitUsDuration;
      }
    }
    kRadio->setOutputPower(mMinPower);
  }
};

void Lora::sendClose() {
  if (!kRadio)
    return;
  ESP_LOGE("Send", "close");
  auto ook = OOK(355, 433.98);
  for(auto i=0; i<5; i++) {
    ook.transmit(closeSeq);
    delayMicroseconds(8130);
  }
}
void Lora::sendOpen() {
  if (!kRadio)
    return;
  ESP_LOGE("Send", "open");
  auto ook = OOK(355, 433.98);
  for(auto i=0; i<5; i++) {
    ook.transmit(openSeq);
    delayMicroseconds(8130);
  }
}
Lora::Lora() {
  if constexpr (!HW::kHasLora) {
    return;
  }
  if (!kRadio) {
    // Construct the RadioLib objects just once in RTC mem
    kSpi.emplace(10'000'000, MSBFIRST, SPI_MODE0);
    kHal.emplace(SPI, *kSpi);
    kModule.emplace(&kHal.value(), HW::Lora::Cs, HW::Lora::Dio1, HW::Lora::Res, HW::Lora::Busy);
    kRadio.emplace(&kModule.value());
    kRadio->XTAL = true;
    kRadio->begin();
    sleep();
  } else {
    kHal->pinMode(kModule->getIrq(), kHal->GpioModeInput);
    kHal->pinMode(kModule->getGpio(), kHal->GpioModeInput);
    kModule->init();
  }
}

void Lora::startReceive()
{
  if (!kRadio)
    return;

  // ESP_LOGE("lora", "listening");
  rtc_gpio_hold_dis((gpio_num_t)HW::Lora::Cs);

  // If we did a deep sleep, we need to call again begin() to reset the module
  kRadio->begin(434.0, 125.0, 9, 7, 0x12, 10, 8, 0, false);

  kRadio->startReceive(); // 4.65mA @ 3.3V
}

void Lora::sleep()
{
  if (!kRadio)
    return;

  kRadio->sleep(false); // ~0.3 uA @ 3.3V
  // kRadio->sleep(); // ~0.7 uA @3.3V

  // Hold some pins to some values to avoid leaking current
  rtc_gpio_hold_en((gpio_num_t)HW::Lora::Cs);
}

void Lora::receive() {
  if (!kRadio)
    return;

  String data;
  auto status = kRadio->readData(data);
  if (status == RADIOLIB_ERR_NONE) {
    // Fill the rest of parameters
    mPck = Lora::Pck{.mData = data.c_str(),
                     .mSNR = kRadio->getSNR(),
                     .mRSSI = kRadio->getRSSI(),
                     .mFreqError = kRadio->getFrequencyError()};
    sleep();
    return;
  }
  ESP_LOGE("Lora", "receive error: %d", status);
  sleep();
}