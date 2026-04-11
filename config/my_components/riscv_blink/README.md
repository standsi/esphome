# riscv_blink ESPHome component

This component targets the ESP32-S2 and ESP32-S3 ULP RISC-V coprocessor.

The runtime code is in place, but the generated ULP artifacts are still placeholders. To finish the component:

1. Build the sibling riscv_blink ESP-IDF project for esp32s2.
2. Run `pwsh ./esphome/update_riscv_ulp_artifacts.ps1 -Target esp32s2`.
3. Build for esp32s3.
4. Run `pwsh ./esphome/update_riscv_ulp_artifacts.ps1 -Target esp32s3`.

The component will log an explicit runtime error until those target-specific artifacts are populated.
