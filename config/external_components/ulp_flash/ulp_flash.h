#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ulp_flash {

enum class FlashPulseWidth : uint8_t {
  NARROW = 0,
  MEDIUM = 1,
  WIDE = 2,
};

class ULPFlash : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void set_interval(uint32_t interval) { interval_ = interval; }
  void set_pulse_width(FlashPulseWidth width) { pulse_width_ = width; }

 protected:
  InternalGPIOPin *pin_{nullptr};
  uint32_t interval_{1000};  // default 1s
  uint32_t last_toggle_{0};
  bool state_{false};
  int rtc_bit_{-1};                                       // store computed bit here
  FlashPulseWidth pulse_width_{FlashPulseWidth::NARROW};  // default is about 8ms
};

}  // namespace ulp_flash
}  // namespace esphome
