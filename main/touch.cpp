#include "touch.h"
#include "deep_sleep.h"

#include "hal/touch_sensor_hal.h"
#include "driver/touch_sensor.h"
#include "esp_sleep.h"

void Touch::setUp(bool onlyMenuLight) {
#if(HW_VERSION >= 10)
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
#endif

  // This takes around 0.4ms, so it is better to cache it
  if (mSettings.mSetup && mSettings.mSetupMode == onlyMenuLight) {
    // Settings are valid, Clear the flags & touch masks and return
    esp_sleep_enable_touchpad_wakeup();
    clear();
    return;
  }
  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V);
#if(HW_VERSION < 10)
  touch_ll_set_meas_time(mSettings.mCycles[0] * 1024);
  touch_pad_set_measurement_interval((32 * 1024) * mSettings.mRate[0] / MeasureRate::_1s / (2 << !onlyMenuLight));
#else
  touch_ll_set_meas_times(mSettings.mCycles[0] * 1024);
  touch_pad_set_measurement_interval((8 * 1024) * mSettings.mRate[0] / MeasureRate::_1s / (2 << !onlyMenuLight));
#endif
  // touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
  // touch_pad_intr_enable();
  esp_sleep_enable_touchpad_wakeup();
  // touch_pad_denoise_disable();

#if(HW_VERSION < 10)
  uint16_t mask {};
#endif

  auto setTouchPad = [&](auto&& v) {
    auto pad = (touch_pad_t)HW::Touch::Pad[mSettings.mMap[v]];
#if(HW_VERSION < 10)
    touch_pad_config(pad, mSettings.mThresholds[v]);
    mask |= 1 << pad;
#else
    touch_pad_config(pad);
    touch_ll_set_threshold(pad, mSettings.mThresholds[v]);
    touch_pad_sleep_channel_enable(pad, false); // We should not set sleep pads
#endif
  };
  if (!onlyMenuLight) {
    setTouchPad(HW::Touch::BotR);
    setTouchPad(HW::Touch::TopR);
  }
  setTouchPad(HW::Touch::TopL);
  setTouchPad(HW::Touch::BotL);

  // Touch Sensor Timer initiated
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
#if (HW_VERSION < 10)
  touch_pad_set_group_mask(mask, mask, mask); // Need to reset the mask after FSM on
#else
  touch_pad_fsm_start();
#endif
  kDSState.lightPad = HW::Touch::Pad[mSettings.mMap[HW::Touch::BotL]];

  mSettings.mSetup = true;
  mSettings.mSetupMode = onlyMenuLight;
}

Touch::Btn Touch::read() const {
  uint32_t mask;
  touch_ll_read_trigger_status_mask(&mask);
  // ESP_LOGE("mask", "%lu", mask);

  // Strange, but For S3 if the mask is unset, it is the last-pad
  if constexpr (HW::kChipType == HW_chips::ESP_32_S3) {
    if (mask == 0) {
      mask = 1 << HW::Touch::Pad.back();
    }
  }

  uint8_t bitmask {};
  for (auto i = 0; i < HW::Touch::Pad.size(); i++)
  {
    // ESP_LOGE("val", "%d: %lu", HW::Touch::Pad[mSettings.mMap[i]], touch_ll_read_raw_data(HW::Touch::Pad[mSettings.mMap[i]]));
    bitmask |= ((mask >> HW::Touch::Pad[mSettings.mMap[i]]) & 1) << i;
  }
  return (Touch::Btn)bitmask;
}

void Touch::clear() const {
  touch_ll_clear_trigger_status_mask();
}

Touch::Btn Touch::readAndClear() const {
  auto btn = read();
  clear();
  return btn;
}