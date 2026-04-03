#pragma once

#include <cstdint>

// These RTC_SLOW_MEM addresses are synchronized to the current LP binary build.
// Source of truth: lp_blink/build/esp-idf/ulp_blink/ulp_main/ulp_main.ld
namespace esphome {
namespace lp_blink {
namespace ulp_main_shared {

constexpr uintptr_t kRunCountAddr = 0x500004fcu;
constexpr uintptr_t kPulseWidthUsAddr = 0x50000500u;
constexpr uintptr_t kFlashLpIoAddr = 0x50000504u;

inline volatile uint32_t &run_count() { return *reinterpret_cast<volatile uint32_t *>(kRunCountAddr); }

inline volatile uint32_t &pulse_width_us() { return *reinterpret_cast<volatile uint32_t *>(kPulseWidthUsAddr); }

inline volatile uint32_t &flash_lp_io() { return *reinterpret_cast<volatile uint32_t *>(kFlashLpIoAddr); }

}  // namespace ulp_main_shared
}  // namespace lp_blink
}  // namespace esphome
