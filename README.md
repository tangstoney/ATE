# ATE Demo

Auto Test Equipment (ATE) demo project based on ESP-IDF / ESP32-P4.

## Overview

This repository is the current ATE main project. The codebase is organized by layer:

- `board`: board-level resources and hardware mapping
- `driver`: low-level hardware / peripheral drivers
- `system`: reusable system services built on top of drivers
- `apps`: business logic and UI-facing application modules
- `main`: project entry

Detailed system boundary notes are maintained in [ai_context/System能力清单.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/ai_context/System能力清单.md). The I2C module communication protocol is maintained in [自动化测试I2C通信协议.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/自动化测试I2C通信协议.md).

## Current Directory Snapshot

The following list reflects the current repository structure that is actually present in the workspace.

```text
.
├── ATE.code-workspace
├── CMakeLists.txt
├── README.md
├── VERSION.md
├── ai_context/
├── components/
│   ├── apps/
│   │   ├── app_ate_console/
│   │   ├── app_eez_ui/
│   │   ├── app_hmi_business/        # reserved directory, not wired into build
│   │   ├── app_tcp_client/
│   │   ├── app_ui_manager/
│   │   ├── app_usb/
│   │   └── app_wifi_manager/
│   ├── board/
│   ├── driver/
│   │   ├── driver_display/
│   │   ├── driver_eth/
│   │   ├── driver_i2c_master/
│   │   ├── driver_input/
│   │   ├── driver_ledstrip/
│   │   ├── driver_network/
│   │   ├── driver_uart_port/
│   │   └── driver_usb/
│   └── system/
│       ├── system_comm_mgr/
│       ├── system_display/
│       ├── system_event/
│       ├── system_fault/
│       ├── system_instrument_service/
│       ├── system_led/
│       ├── system_log/
│       ├── system_module_service/
│       ├── system_network/
│       ├── system_protocol/
│       ├── system_uart_link/
│       └── system_usb/
├── dependencies.lock
├── main/
├── partitions.csv
├── sdkconfig
├── sdkconfig.ci
├── sdkconfig.defaults
├── sdkconfig.old
└── 自动化测试I2C通信协议.md
```

## Current Build-Relevant Components

### Apps

- `app_ate_console`
- `app_eez_ui`
- `app_tcp_client`
- `app_ui_manager`
- `app_usb`
- `app_wifi_manager`

`app_hmi_business/` currently exists only as a reserved directory and is not part of the build.

### System

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

Removed legacy system components such as `system_config`, `system_storage`, `system_event_bus`, `system_i2c_link`, `system_router`, `system_ui`, and `system_device_manager` are no longer part of the current structure.

### Driver

- `driver_display`
- `driver_eth`
- `driver_i2c_master`
- `driver_input`
- `driver_ledstrip`
- `driver_network`
- `driver_uart_port`
- `driver_usb`

## Build

Use ESP-IDF to build.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py flash
```

If `idf.py` is unavailable in the shell, source the ESP-IDF environment first.

## Notes

- `system_module_service` is the current owner of the shared module I2C bus.
- `system_protocol` contains the protocol encode / decode logic bound to the module communication document.
- Board mapping and architecture notes under `ai_context/` are design references, not build inputs.

## License

Copyright © 2026 Johnson Health Tech. All rights reserved.
