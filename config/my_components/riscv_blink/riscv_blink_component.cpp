#include "riscv_blink_component.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esphome/core/log.h"
#include "ulp_riscv.h"

#include "ulp_artifacts.h"

namespace esphome {
namespace riscv_blink {

static const char *const TAG = "riscv_blink";
static constexpr uint32_t RTC_STATE_MAGIC = 0x5256424BU;

struct RTCBlinkState {
  uint32_t magic;
  bool running;
};

static RTC_DATA_ATTR RTCBlinkState s_rtc_blink_state = {
    .magic = 0,
    .running = true,
};

static volatile uint32_t &shared_u32_(uintptr_t address) { return *reinterpret_cast<volatile uint32_t *>(address); }

bool RISCVBlinkComponent::should_start_on_boot_() const {
  switch (this->init_state_) {
    case InitState::RUNNING:
      return true;
    case InitState::STOPPED:
      return false;
    case InitState::LAST:
      if (s_rtc_blink_state.magic == RTC_STATE_MAGIC) {
        return s_rtc_blink_state.running;
      }
      ESP_LOGW(TAG, "No saved ULP RISC-V state in RTC memory, defaulting to stopped");
      return false;
  }

  return true;
}

void RISCVBlinkComponent::save_last_state_(bool running) {
  s_rtc_blink_state.magic = RTC_STATE_MAGIC;
  s_rtc_blink_state.running = running;
}

void RISCVBlinkComponent::drive_pin_to_inactive_() {
  gpio_num_t gpio_num = static_cast<gpio_num_t>(this->gpio_num_);

  if (!rtc_gpio_is_valid_gpio(gpio_num)) {
    ESP_LOGW(TAG, "Cannot drive GPIO %u inactive, pin is not RTC capable", this->gpio_num_);
    return;
  }

  esp_err_t err = rtc_gpio_hold_dis(gpio_num);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to release RTC hold on GPIO %u: %d", this->gpio_num_, err);
  }

  err = rtc_gpio_deinit(gpio_num);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to deinit RTC GPIO %u: %d", this->gpio_num_, err);
  }

  err = rtc_gpio_init(gpio_num);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to route GPIO %u back to RTC IO: %d", this->gpio_num_, err);
    return;
  }

  err = rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_OUTPUT_ONLY);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to set GPIO %u as RTC output: %d", this->gpio_num_, err);
    return;
  }

  err = rtc_gpio_set_level(gpio_num, this->flash_lp_io_inverted_ ? 1 : 0);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to drive GPIO %u inactive: %d", this->gpio_num_, err);
    return;
  }

  ESP_LOGD(TAG, "Drove GPIO %u inactive after stopping ULP RISC-V", this->gpio_num_);
}

bool RISCVBlinkComponent::start_ulp_riscv_() {
  gpio_num_t gpio_num = static_cast<gpio_num_t>(this->gpio_num_);
  const auto &artifact = get_ulp_main_artifact();

  if (!rtc_gpio_is_valid_gpio(gpio_num)) {
    ESP_LOGE(TAG, "GPIO %u is not RTC capable", this->gpio_num_);
    return false;
  }

  if (!artifact.valid) {
    ESP_LOGE(TAG,
             "No generated ULP artifact is available for %s. Build the riscv_blink IDF project for this target and "
             "update the ESPHome artifact headers.",
             artifact.target_name);
    return false;
  }

  if (this->running_) {
    ulp_riscv_timer_stop();
    ulp_riscv_halt();
    ulp_riscv_reset();
    this->running_ = false;
  }

  esp_err_t err = rtc_gpio_deinit(gpio_num);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to deinit RTC GPIO %u before restart: %d", this->gpio_num_, err);
  }

  err = rtc_gpio_init(gpio_num);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init RTC GPIO %u: %d", this->gpio_num_, err);
    return false;
  }

  err = rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_OUTPUT);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set RTC GPIO %u direction: %d", this->gpio_num_, err);
    return false;
  }

  rtc_gpio_pulldown_dis(gpio_num);
  rtc_gpio_pullup_dis(gpio_num);

  err = ulp_riscv_load_binary(artifact.binary, artifact.binary_size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to load ULP RISC-V binary for %s: %d", artifact.target_name, err);
    return false;
  }

  shared_u32_(artifact.flash_lp_io_addr) = this->gpio_num_;
  shared_u32_(artifact.pulse_width_us_addr) = this->pulse_width_us_;
  shared_u32_(artifact.run_count_addr) = 0;
  shared_u32_(artifact.flash_lp_io_inverted_addr) = this->flash_lp_io_inverted_ ? 1U : 0U;

  err = ulp_set_wakeup_period(0, this->wakeup_period_us_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure ULP wake period: %d", err);
    return false;
  }

  ESP_LOGI(TAG, "Starting ULP RISC-V: target=%s gpio=%u pulse=%u us period=%u us", artifact.target_name,
           this->gpio_num_, this->pulse_width_us_, this->wakeup_period_us_);
  err = ulp_riscv_run();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start ULP RISC-V: %d", err);
    return false;
  }

  this->running_ = true;
  this->save_last_state_(true);
  return true;
}

void RISCVBlinkComponent::setup() {
  if (this->should_start_on_boot_()) {
    ESP_LOGI(TAG, "RISC-V blink initialized in running state");
    this->start_ulp_riscv_();
    return;
  }

  this->running_ = false;
  this->save_last_state_(false);
  ulp_riscv_timer_stop();
  ulp_riscv_halt();
  ulp_riscv_reset();
  this->drive_pin_to_inactive_();
  ESP_LOGI(TAG, "RISC-V blink initialized in stopped state");
}

void RISCVBlinkComponent::start_blink() {
  if (this->running_) {
    ESP_LOGI(TAG, "RISC-V blink is already running");
    return;
  }

  this->start_ulp_riscv_();
}

void RISCVBlinkComponent::stop_blink() {
  if (!this->running_) {
    this->drive_pin_to_inactive_();
    this->save_last_state_(false);
    ESP_LOGI(TAG, "RISC-V blink is already stopped");
    return;
  }

  ulp_riscv_timer_stop();
  ulp_riscv_halt();
  ulp_riscv_reset();
  this->drive_pin_to_inactive_();
  this->running_ = false;
  this->save_last_state_(false);
  ESP_LOGI(TAG, "Stopped ULP RISC-V timer/wakeups");
}

void RISCVBlinkComponent::set_pulse_width_us(uint32_t pulse_width_us) {
  this->pulse_width_us_ = pulse_width_us;

  if (this->running_) {
    const auto &artifact = get_ulp_main_artifact();
    if (artifact.valid) {
      shared_u32_(artifact.pulse_width_us_addr) = this->pulse_width_us_;
    }
  }

  ESP_LOGI(TAG, "Updated ULP pulse width to %u us", this->pulse_width_us_);
}

void RISCVBlinkComponent::set_wakeup_period_ms(uint32_t wakeup_period_ms) {
  this->wakeup_period_us_ = wakeup_period_ms * 1000U;
  ESP_LOGI(TAG, "Updating ULP wake period to %u ms", wakeup_period_ms);

  if (this->running_) {
    this->start_ulp_riscv_();
  }
}

void RISCVBlinkComponent::set_flash_lp_io_inverted(bool inverted) {
  this->flash_lp_io_inverted_ = inverted;
  ESP_LOGI(TAG, "Setting flash RTC IO inverted to %s", inverted ? "true" : "false");

  if (this->running_) {
    const auto &artifact = get_ulp_main_artifact();
    if (artifact.valid) {
      shared_u32_(artifact.flash_lp_io_inverted_addr) = this->flash_lp_io_inverted_ ? 1U : 0U;
    }
  }
}

}  // namespace riscv_blink
}  // namespace esphome
