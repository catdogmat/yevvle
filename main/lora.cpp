#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "lora.h"

namespace {
  RTC_DATA_ATTR std::optional<SPISettings> kSpi;
  RTC_DATA_ATTR std::optional<ArduinoHal> kHal;
  RTC_DATA_ATTR std::optional<Module> kModule;
  RTC_DATA_ATTR std::optional<SX1262> kRadio;
}

Lora::Lora() {
  if constexpr (!HW::kHasLora) {
    return;
  }
  // if (rtc_gpio_is_valid_gpio((gpio_num_t)HW::Lora::Dio1))
  //    rtc_gpio_pulldown_en((gpio_num_t)HW::Lora::Dio1);
  // if (rtc_gpio_is_valid_gpio((gpio_num_t)HW::Lora::Busy))
  //    rtc_gpio_pullup_en((gpio_num_t)HW::Lora::Busy);
  if (!kRadio) {
    // ESP_LOGE("lora", "initialize");
    // Construct the RadioLib objects just once in RTC mem
    kSpi.emplace(2'000'000, MSBFIRST, SPI_MODE0);
    kHal.emplace(SPI, *kSpi);
    kModule.emplace(&kHal.value(), HW::Lora::Cs, HW::Lora::Dio1, HW::Lora::Res, HW::Lora::Busy);
    return; // FIX ME, if the module is not present should not crash the chip
    kRadio.emplace(&kModule.value());
    // kRadio.XTAL = false;
    // kRadio.standbyXOSC = false;
    kRadio->begin(434.0, 125.0, 9, 7, 0x12, 10, 8, 0, false);
    kRadio->sleep();
  }

  kHal->pinMode(kModule->getIrq(), kHal->GpioModeInput);
  kHal->pinMode(kModule->getGpio(), kHal->GpioModeInput);
  kModule->init();
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
}