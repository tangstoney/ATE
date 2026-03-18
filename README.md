# ATE demo

出展用 App — Auto Test Equipment (ATE)

## 簡介
本倉庫為 ATE（Auto Test Equipment）展示用範例，包含最小的主工程與示例配置建議。此 README 將說明目錄結構、編譯/配置建議與後續要補充的 TODO 项目。

## 版本訊息
查看 `VERSION.md` 獲取詳細的版本更新訊息。

## 目前目錄詳盡清單
下面為專案目前的實際目錄與源檔位置（已排除自動產生的 LVGL UI 內容）。此清單會隨工程演進更新：

```
├── .clangd
├── .devcontainer/
├── .vscode/
├── ATE.code-workspace
├── CMakeLists.txt
├── README.md
├── VERSION.md
├── sdkconfig.ci
├── pytest_hello_world.py
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                 # 主程序（存在）
│   ├── bsp.h.bak             # 已迁移到 components/bsp_manager/include/bsp.h
│   └── Config/                # 配置範例（GUI_Config.*, System_Config.*）
├── components/                # ATE components
│   ├── driver/                 # Driver layer
│   │   ├── driver_display/
│   │   ├── driver_input/
│   │   ├── driver_ledstrip/
│   │   ├── driver_network/
│   │   └── driver_usb/
│   ├── system/                 # System layer
│   │   ├── system_config/
│   │   ├── system_device_manager/
│   │   ├── system_display/
│   │   ├── system_event_bus/
│   │   ├── system_led/
│   │   ├── system_log/
│   │   ├── system_network/
│   │   ├── system_storage/
│   │   ├── system_ui/
│   │   └── system_usb/
│   ├── apps/                   # Application layer
│   │   ├── app_ate_console/
│   │   ├── app_eez_ui/
│   │   ├── app_tcp_client/
│   │   ├── app_ui_manager/
│   │   ├── app_usb/
│   │   └── app_wifi_manager/
│   └── driver/                 # Ethernet driver component now in driver/driver_eth/
├── managed_components/
├── eez/
├── sdkconfig
├── sdkconfig.old
└── 以太网应用源码.md
```

注意：`components/` 與 `main/Config/` 已建立為範例骨架，內含簡單的 header/c 檔與 `CMakeLists.txt`。
## 開發者注意事項
如果您希望本專案成為完整展示包，請補上以下檔案/資源：
	- `main/Config/` 目錄與相關 `GUI_Config.*`、`System_Config.*`。
	- `VERSION.md`（版本歷史）。
	- 流程圖（Mermaid 或 PNG）放於 `docs/`。

## ATE App 功能說明
- 展示 LVGL + ESP-IDF 應用的系統配置與 GUI 範例。
- 作為展示/測試平台（demo）之用，用於驗證顯示、按鍵、與儲存（SD 卡）等子系統整合。

## 軟體流程圖
（TODO: 建議新增 Mermaid 流程圖或圖片，放置於 `docs/` 或倉庫根目錄）

## 依賴核心組件
- `core.gui` — 請使用對應分支或子模組（如需要，請把此模組加入子模組或在 README 中註明來源）。

## 建置說明（快速指南）
以下為一般化的建置說明。具體流程依您採用的框架（純 CMake、或 ESP-IDF 專案）略有不同。

1. 使用通用 CMake（若您在 ESP-IDF 環境，請使用 `idf.py`）：

```bash
# 建議在專案根目錄建立 build 目錄
mkdir -p build && cd build
cmake ..
cmake --build .
```

2. 如果是 ESP-IDF 專案（推薦）：

```bash
# 在專案根目錄 (含 CMakeLists.txt 與 sdkconfig) 下
idf.py set-target esp32p4
idf.py menuconfig   # 設定 SDK 選項
idf.py build
idf.py flash
```

## sdkconfig 建議
- Component config LVGL configuration: 勾選 Enable Montserrat 14-48。
- Serial flasher config Size: 32MB。
- partition 选自定义:
- Offset of partition table（0xf0000 boot大小直接拉满）
- 啟用 ESP PSRAM（若硬體支援）。
- 勾選 `CONFIG_IDF_EXPERIMENTAL_FEATURES`（若使用實驗性功能）。
- Memory Settings: 將 Malloc functions source 選為標準 C 分配以利用 PSRAM。
- 對於 IDF 5.5.x，注意區分 chip rev 3.1 / 1.1 的差異。

Menuconfig 推薦設定：
- RTOS: `configTICK_RATE_HZ` 設為 `1000`。
- LVGL:
	- Default refresh period (ms): `15`
	- Operating System (OS): `FREERTOS`
	- Number of draw units: `2`
	- Use cache to speed up getting object style properties: 勾選

## sd 卡文件目录（示例）
- /sdcard/images/ — UI 圖片
- /sdcard/fonts/ — 字體文件
- /sdcard/config/ — 運行時配置（如 JSON）

（TODO: 根據實際使用的 SD 卡結構補充具體範例與讀取程式）

## 開發者注意事項
- 若您希望本專案成為完整展示包，請補上以下檔案/資源：
	- `main/Config/` 目錄與相關 `GUI_Config.*`、`System_Config.*`。
	- `VERSION.md`（版本歷史）。
	- 流程圖（Mermaid 或 PNG）放於 `docs/`。

## 許可證
Copyright © 2026 Johnson Health Tech. All rights reserved.

# TODO List
1. 需要验证移植后的代码是否都正常运行
2. 每个组件的设计约束，使用约束，设计说明需要添加
3. 该项目的架构与文件目录需要说明
4. 每个组件的readme待补充
5. 每个层的每个组件的文件处在该层是否合理也待说明
6. 每个层的每个组件的设计是否合理，需要给gpt进行审核和提意见

