#include "touch.h"
#include "hardware.h"
#include "deep_sleep.h"

#include "hal/touch_sensor_hal.h"
#include "driver/touch_sensor.h"
#include "esp_sleep.h"

void Touch::setUp(bool onlyMenuLight) {
  // This takes around 0.4ms, so it is better to cache it
  if (mSettings.mSetup && mSettings.mSetupMode == onlyMenuLight) {
    // Settings are valid, Clear the flags & touch masks and return
    esp_sleep_enable_touchpad_wakeup();
    clear();
    return;
  }
  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V);
  //touch_pad_set_cnt_mode(); 
  touch_pad_set_measurement_clock_cycles(mSettings.mCycles[0] * 1024);
  touch_pad_set_measurement_interval((32 * 1024) * mSettings.mRate[0] / MeasureRate::_1s);
  //touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
  //touch_pad_intr_enable();
  esp_sleep_enable_touchpad_wakeup();
  //touch_pad_denoise_disable();
  uint16_t mask {};
  auto setTouchPad = [&](auto&& v) {
    touch_pad_config((touch_pad_t)HW::Touch::Pad[mSettings.mMap[v]], mSettings.mThresholds[v]);
    mask |= 1 << HW::Touch::Pad[mSettings.mMap[v]];
  };
  if (!onlyMenuLight) {
    setTouchPad(HW::Touch::BotR);
    setTouchPad(HW::Touch::TopR);
  }
  setTouchPad(HW::Touch::TopL);
  setTouchPad(HW::Touch::BotL);
  // Touch Sensor Timer initiated
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_set_group_mask(mask, mask, mask); // Need to reset the mask after FSM on


  mSettings.mSetup = true;
  mSettings.mSetupMode = onlyMenuLight;
}

Touch::Btn Touch::read() const {
  uint32_t mask;
  touch_ll_read_trigger_status_mask(&mask);

  uint8_t bitmask {};
  for (auto i = 0; i < HW::Touch::Pad.size(); i++)
  {
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