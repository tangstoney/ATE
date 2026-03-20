# Version

## Current Snapshot

- Project: `ATE Demo`
- Version: `0.1.0`
- Target: `ESP32-P4`
- ESP-IDF: `5.5.3`
- Status: `in progress`

## 2026-03-20

- Updated `README.md` to match the current repository structure.
- Updated system architecture notes to reflect the current System layer.
- Removed legacy system components from the active project structure:
  - `system_config`
  - `system_storage`
  - `system_event_bus`
  - `system_i2c_link`
  - `system_router`
  - `system_ui`
  - `system_device_manager`
- Adjusted the I2C dependency chain:
  - `system_comm_mgr` now manages `driver_i2c_master`
  - `system_module_service` now accesses the module bus directly via `driver_i2c_master`
- Kept the shared module bus ownership inside `system_module_service`.

## Current System Components

- `system_comm_mgr`
- `system_display`
- `system_event`
- `system_fault`
- `system_instrument_service`
- `system_led`
- `system_log`
- `system_module_service`
- `system_network`
- `system_protocol`
- `system_uart_link`
- `system_usb`
