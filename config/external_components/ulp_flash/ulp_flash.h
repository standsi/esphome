#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace ulp_flash {

enum class FlashPulseWidth : uint8_t {
  NARROW = 0,
  MEDIUM = 1,
  WIDE = 2,
};

const extern bool FLASH_OFF;
const extern bool FLASH_ON;

class ULPFlash;

class ULPFlashCall {
 public:
  ULPFlashCall(ULPFlash *parent);

  // set the command as a string "on" or "off"
  ULPFlashCall &set_command(const char *command);

  ULPFlashCall &set_command_on();

  ULPFlashCall &set_command_off();

  void perform();

 protected:
  void validate_();
  ULPFlash *parent_;
  bool flash_state_;
};

class ULPFlash : public Component, public EntityBase {
 public:
  explicit ULPFlash();

  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void set_interval(uint32_t interval) { interval_ = interval; }
  void set_pulse_width(FlashPulseWidth width) { pulse_width_ = width; }

  bool flash_state;

  ULPFlashCall make_call();
  void add_on_state_callback(std::function<void()> &&f);

  /** Publish the current state of the valve.
   *
   * First set the .position, etc. values and then call this method
   * to publish the state of the valve.
   *
   * @param save Whether to save the updated values in RTC area.
   */
  void publish_state(bool save = true);

 protected:
  InternalGPIOPin *pin_{nullptr};
  uint32_t interval_{1000};  // default 1s
  uint32_t last_toggle_{0};
  bool state_{false};
  int rtc_bit_{-1};                                       // store computed bit here
  FlashPulseWidth pulse_width_{FlashPulseWidth::NARROW};  // default is about 8ms
  friend ULPFlashCall;
  LazyCallbackManager<void()> state_callback_{};
  ESPPreferenceObject rtc_;
};

}  // namespace ulp_flash
}  // namespace esphome
