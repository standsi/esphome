#include "lp_blink_component.h"

#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_err.h"
#include "esphome/core/log.h"
#include "ulp_lp_core.h"

#include "ulp_main_binary.h"
#include "ulp_main_shared.h"

namespace esphome {
namespace lp_blink {

static const char *const TAG = "lp_blink";
static constexpr uint32_t RTC_STATE_MAGIC = 0x4C50424BU;

struct RTC_DATA_ATTR RTCBlinkState {
  uint32_t magic;
  bool running;
};

static RTCBlinkState s_rtc_blink_state = {
    .magic = 0,
    .running = true,
};

bool LPBlinkComponent::should_start_on_boot_() const {
  switch (this->init_state_) {
    case InitState::RUNNING:
      return true;
    case InitState::STOPPED:
      return false;
    case InitState::LAST:
      if (s_rtc_blink_state.magic == RTC_STATE_MAGIC) {
        return s_rtc_blink_state.running;
      }
      ESP_LOGW(TAG, "No saved LP state in RTC memory, defaulting to running");
      return true;
  }

  return true;
}

void LPBlinkComponent::save_last_state_(bool running) {
  s_rtc_blink_state.magic = RTC_STATE_MAGIC;
  s_rtc_blink_state.running = running;
}

bool LPBlinkComponent::start_lp_core_() {
  gpio_num_t gpio_num = static_cast<gpio_num_t>(this->gpio_num_);

  if (!rtc_gpio_is_valid_gpio(gpio_num)) {
    ESP_LOGE(TAG, "GPIO %u is not LP/RTC capable", this->gpio_num_);
    return false;
  }

  if (this->running_) {
    ulp_lp_core_stop();
    this->running_ = false;
  }

  esp_err_t err = ulp_lp_core_load_binary(kUlpMainBinary, kUlpMainBinarySize);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to load LP binary: %d", err);
    return false;
  }

  ulp_main_shared::flash_lp_io() = static_cast<uint32_t>(rtc_io_number_get(gpio_num));
  ulp_main_shared::pulse_width_us() = this->pulse_width_us_;
  ulp_main_shared::run_count() = 0;

  ulp_lp_core_cfg_t cfg = {
      .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER,
      .lp_timer_sleep_duration_us = this->wakeup_period_us_,
  };

  ESP_LOGI(TAG, "Starting LP core: gpio=%u pulse=%u us period=%u us", this->gpio_num_, this->pulse_width_us_,
           this->wakeup_period_us_);
  err = ulp_lp_core_run(&cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start LP core: %d", err);
    return false;
  }

  this->running_ = true;
  this->save_last_state_(true);
  ESP_LOGI(TAG, "LP core running (pulse+halt per wake)");
  return true;
}

void LPBlinkComponent::setup() {
  if (this->should_start_on_boot_()) {
    this->start_lp_core_();
    this->running_ = true;
    return;
  }

  this->running_ = false;
  this->save_last_state_(false);
  ulp_lp_core_stop();
  ESP_LOGI(TAG, "LP blink initialized in stopped state");
}

void LPBlinkComponent::start_blink() {
  if (this->running_) {
    ESP_LOGI(TAG, "LP blink is already running");
    return;
  }

  this->start_lp_core_();
}

void LPBlinkComponent::stop_blink() {
  if (!this->running_) {
    this->save_last_state_(false);
    ESP_LOGI(TAG, "LP blink is already stopped");
    return;
  }

  ulp_lp_core_stop();
  this->running_ = false;
  this->save_last_state_(false);
  ESP_LOGI(TAG, "Stopped LP core timer/wakeups");
}

void LPBlinkComponent::set_pulse_width_us(uint32_t pulse_width_us) {
  this->pulse_width_us_ = pulse_width_us;

  if (this->running_) {
    ulp_main_shared::pulse_width_us() = this->pulse_width_us_;
  }

  ESP_LOGI(TAG, "Updated LP pulse width to %u us", this->pulse_width_us_);
}

void LPBlinkComponent::set_wakeup_period_ms(uint32_t wakeup_period_ms) {
  this->wakeup_period_us_ = wakeup_period_ms * 1000U;
  ESP_LOGI(TAG, "Updating LP wake period to %u ms", wakeup_period_ms);

  if (this->running_) {
    this->start_lp_core_();
  }
}

}  // namespace lp_blink
}  // namespace esphome
