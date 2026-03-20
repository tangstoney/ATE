# ATE UI软件架构设计文档

## 一、文档概述

### 1.1 架构负责人角色设定

本文档由嵌入式系统架构负责人视角编写，具备 30 年团队技术管理经验、20 年以上嵌入式软件开发经验，长期从事工业自动化设备 / ATE 自动测试设备 UI 架构设计，精通以下技术领域：

- **MCU 平台**：ESP32-P4、ESP-IDF、FreeRTOS、多核任务调度、硬件接口（SPI/I2C/UART/RS485/USB）

- **GUI 框架**：LVGL 9、LVGL Adapter 设计、LVGL 线程安全模型、LVGL 渲染机制、LVGL Timer/Event

- **UI 工具链**：EEZ Studio、EEZ UI 代码生成结构、LVGL+EEZ 集成、EEZ Screen/Component/Action

- **工业触摸屏 UI**：工业设备 HMI、ATE 自动测试设备 UI、触摸屏交互设计、工业设备操作逻辑

- **软件架构**：任务划分、状态机设计、事件总线、模块化架构、前后端解耦

### 1.2 项目背景

当前项目为 ATE 自动测试设备 UI 系统，技术栈与设备信息如下：

- **MCU**：ESP32-P4

- **RTOS**：FreeRTOS（ESP-IDF）

- **GUI**：LVGL 9

- **UI 工具**：EEZ Studio

- **设备类型**：工业 ATE 自动测试设备

- **UI 运行载体**：触摸屏 HMI

- **设备组成**：测试模块 slot1~slot8、仪表设备（串口 / RS485/USB）、NFC 打卡、服务器数据上传、OTA 升级

### 1.3 文档编写原则

本文档聚焦于软件架构、页面逻辑、状态机、数据流、任务模型、LVGL 线程安全、UI 与业务逻辑解耦，不关注 UI 视觉设计、动画、复杂布局，输出符合工程规范的架构文档，可直接用于团队开发、架构评审与代码实现。

## 二、系统架构

### 2.1 三层架构设计

ATE UI 系统采用分层架构，严格实现 UI 与业务逻辑解耦，架构层级如下：

```Plain Text

ATE Application Layer
        ↓
UI Service Layer
        ↓
LVGL Layer
```

- **ATE Application Layer**：负责所有业务逻辑，包含 ATE Test Manager、ATE Config Manager、ATE Module Manager、ATE Upload Manager、ATE Operator Manager

- **UI Service Layer**：负责页面调度、状态管理、事件适配，包含 ui_page_manager、ui_status_bar、ui_voice_manager、ui_navigation_manager、ui_event_adapter

- **LVGL Layer**：负责 UI 绘制与渲染，包含 EEZ UI Screens、LVGL Widgets、Display Driver

### 2.2 层级职责划分

|层级|核心职责|EEZ UI 定位|
|---|---|---|
|ATE Application|测试逻辑执行、设备管理、配置存储|不涉及，仅处理业务逻辑|
|UI Service|页面调度、状态订阅、事件转发|负责页面调度与 UI 事件适配|
|LVGL|UI 绘制、控件渲染、显示驱动|仅负责 UI 布局、页面、控件、事件|
### 2.3 LVGL Adapter 设计

针对 ESP32-P4 平台，LVGL Adapter 需完成以下适配：

1. **显示驱动适配**：适配 ESP32-P4 的 LCD 接口（如 SPI/Parallel），实现 LVGL 的显示刷新回调

2. **输入驱动适配**：适配触摸屏的输入接口（如 I2C），实现 LVGL 的输入设备回调

3. **内存管理适配**：使用 ESP-IDF 的内存分配函数替代 LVGL 默认的 malloc/free，避免内存碎片

4. **EEZ 集成适配**：将 EEZ Studio 生成的 UI 资源（如图片、字体）集成到 LVGL 的资源管理器中

## 三、任务架构

### 3.1 双核任务划分

基于 ESP32-P4 双核特性，采用如下任务分配策略，避免 UI 与业务任务冲突：

```Plain Text

Core0
 ├ UI Task
 ├ Voice Task

Core1
 ├ ATE Test Task
 ├ Module Monitor Task
 ├ Instrument Monitor Task
 ├ Upload Task
```

### 3.2 任务优先级与职责

|任务名称|核心职责|优先级|运行核心|
|---|---|---|---|
|UI Task|执行 lv_timer_handler ()、处理 UI 消息、更新 UI 控件|3|Core0|
|Voice Task|管理语音播报、响应页面切换语音触发事件|3|Core0|
|ATE Test Task|执行测试逻辑、发送测试进度消息、处理测试流程控制|5|Core1|
|Module Monitor Task|监控测试模块 slot1~slot8 热插拔、检测模块状态、发送模块状态事件|4|Core1|
|Instrument Monitor Task|监控仪表设备热插拔、检测仪表通信状态、发送仪表状态事件|4|Core1|
|Upload Task|处理测试数据上传服务器、发送上传状态消息|4|Core1|
### 3.3 任务通信机制

所有业务任务（ATE Test Task、Module Monitor Task 等）禁止直接调用 LVGL API（如 lv_label_set_text、lv_bar_set_value），必须通过 Queue/Event/Message 的方式通知 UI Task，确保 LVGL 线程安全。

### 3.4 任务通信具体实现

1. **队列配置**：

    - UI 消息队列：长度为 32，消息大小为`sizeof(ate_event_msg_t)`

    - 测试进度队列：长度为 16，消息大小为`sizeof(ate_progress_msg_t)`

    - 模块状态队列：长度为 8，消息大小为`sizeof(module_info_t)`

    - 仪表状态队列：长度为 4，消息大小为`sizeof(instrument_info_t)`

2. **队列创建**：

    ```c
    
    QueueHandle_t ui_queue = xQueueCreate(32, sizeof(ate_event_msg_t));
    QueueHandle_t test_progress_queue = xQueueCreate(16, sizeof(ate_progress_msg_t));
    ```

3. **消息处理**：UI Task 在`lv_timer_handler()`之后处理队列消息，确保 UI 更新在 LVGL 线程中执行

### 3.5 多任务冲突处理

1. **共享资源访问**：使用互斥量保护共享资源（如配置数据、模块信息），避免多任务同时访问导致的数据冲突

    ```c
    
    SemaphoreHandle_t config_mutex = xSemaphoreCreateMutex();
    // 访问配置数据时获取互斥量
    xSemaphoreTake(config_mutex, portMAX_DELAY);
    // 访问配置数据
    xSemaphoreGive(config_mutex);
    ```

2. **任务优先级调度**：业务任务优先级高于 UI 任务，确保测试逻辑优先执行，UI 任务仅在空闲时处理 UI 更新，避免 UI 抢 CPU 导致测试变慢

### 3.6 Upload Task 实现

Upload Task 负责测试数据上传至服务器，具体实现如下：

1. **上传协议**：采用 HTTPS 协议上传数据，数据格式为 JSON，包含测试结果、模块信息、仪表信息等

2. **重试机制**：上传失败时，最多重试 3 次，每次重试间隔 5 秒，重试失败时将数据存储于 eMMC 中，等待下次上传

3. **状态更新**：上传过程中，通过队列发送上传进度至 UI Task，由 UI Task 更新 Upload 页面的进度条和状态信息

## 四、页面结构

### 4.1 页面分类与整体结构

ATE UI 分为三类页面，整体结构如下：

```Plain Text

Boot
│
▼
Home
├ ATE Flow
│
│  NFC
│  ATE Setting
│  ATE Prepare
│  ATE Running
│  ATE Result
│  Upload
│
├ Module Manager
├ Instrument Config
├ History Log
└ OTA
```

### 4.2 页面生命周期设计

为避免 LVGL 内存碎片与 UI 卡顿，采用**页面一次创建、长期存在、show/hide 切换**的生命周期模型：

|生命周期阶段|对应 LVGL 操作|适用场景|
|---|---|---|
|PAGE_CREATE|lv_obj_create 创建页面根对象|系统启动时一次性创建所有页面|
|PAGE_ENTER|lv_obj_clear_flag (hidden) 显示页面|页面切换进入时触发|
|PAGE_LEAVE|lv_obj_add_flag (hidden) 隐藏页面|页面切换离开时触发|
|PAGE_DESTROY|lv_obj_del 删除页面对象|极少使用，仅系统销毁时触发|
### 4.3 页面模板规范

所有页面统一采用如下结构，确保 UI 结构一致性：

```Plain Text

base_page
│
├ status_bar
│
├ title_bar
│   ├ back_button
│   ├ page_title
│   └ nav_buttons
│
└ content_area
```

- **status_bar**：永远存在，显示 WiFi 状态、服务器连接、模块数量、仪表数量、当前操作员、时间

- **title_bar**：包含返回按钮、页面标题、导航按钮（左右翻页）

- **content_area**：页面核心内容展示区域

### 4.4 EEZ Studio 生成代码适配

EEZ Studio 生成的页面代码需适配页面管理器的生命周期模型，具体实现如下：

1. **EEZ 页面代码集成**：EEZ 生成的每个页面对应一个`ui_page_t`结构体，EEZ 生成的页面根对象作为`ui_page_t`的`root`成员

2. **事件转发**：EEZ 生成的控件事件（如按钮点击）需通过`ui_event_adapter`转发至 ATE Application 层，禁止在 EEZ 代码中直接处理业务逻辑

    ```c
    
    // EEZ按钮点击事件示例
    void eez_button_start_test_click(lv_event_t *e)
    {
        // 转发事件至ATE Test Manager
        ate_event_msg_t msg = {EVT_TEST_START, NULL};
        xQueueSend(ate_event_queue, &msg, pdMS_TO_TICKS(10));
    }
    ```

3. **生命周期适配**：EEZ 生成的页面需实现`on_enter`和`on_leave`回调，在页面切换时更新 EEZ 页面的状态

### 4.5 页面管理器设计

#### 4.5.1 页面枚举与结构

```c

typedef enum
{
UI_PAGE_BOOT,
UI_PAGE_HOME,
UI_PAGE_NFC,
UI_PAGE_ATE_SETTING,
UI_PAGE_ATE_PREPARE,
UI_PAGE_ATE_RUNNING,
UI_PAGE_ATE_RESULT,
UI_PAGE_UPLOAD,
UI_PAGE_MODULE_MANAGER,
UI_PAGE_INSTRUMENT_CONFIG,
UI_PAGE_HISTORY_LOG,
UI_PAGE_OTA
} ui_page_id_t;

typedef struct
{
ui_page_id_t id;
lv_obj_t *root;
void (*on_enter)(void);
void (*on_leave)(void);
void (*on_update)(void);
} ui_page_t;
```

#### 4.5.2 页面切换逻辑

核心函数`ui_page_show(page_id)`流程：

1. 调用旧页面的`on_leave()`方法

2. 隐藏旧页面根对象

3. 显示新页面根对象

4. 调用新页面的`on_enter()`方法

5. 更新当前页面指针

### 4.6 语音系统实现

语音系统由`ui_voice_manager`管理，具体实现如下：

1. **语音资源存储**：语音文件存储于 SPI Flash 中，采用 WAV 格式，每个页面切换对应一个语音文件

2. **语音播放触发**：在页面的`on_enter`回调中触发语音播放，通过 ESP32-P4 的 I2S 接口播放语音

    ```c
    
    void ui_page_ate_running_on_enter(void)
    {
        ui_voice_manager_play(VOICE_ATE_RUNNING);
    }
    ```

3. **语音管理**：`ui_voice_manager`负责语音资源的加载、播放和停止，避免同时播放多个语音

### 4.7 OTA 升级页面实现

OTA 升级页面的具体流程如下：

1. **U 盘检测**：OTA 页面在`on_enter`回调中检测 U 盘是否插入，通过 ESP32-P4 的 USB Host 接口检测 U 盘

2. **bin 文件检测**：扫描 U 盘中的 bin 文件，校验文件的签名和版本号

3. **升级过程**：

    - 显示升级进度（通过 lv_bar 控件）

    - 擦除 Flash 的 OTA 分区

    - 写入 bin 文件

    - 重启设备

4. **升级结果**：升级完成后，显示升级成功或失败的信息，自动返回 Home 页面

### 4.8 历史日志页面实现

历史日志页面的具体实现如下：

1. **日志存储**：测试日志存储于 eMMC 或 SD 卡中，采用 CSV 格式，每条日志包含测试时间、仪表型号、测试结果、操作员等信息

2. **日志加载**：在 History Log 页面的`on_enter`回调中，加载最近的 100 条日志到 lv_table 控件中

3. **日志查询**：支持按时间、操作员、测试结果筛选日志，通过 lv_dropdown 和 lv_input 控件实现

## 五、状态机设计

### 5.1 ATE 核心状态机

ATE UI 绑定统一的测试状态机，所有测试状态采用`ATE_`前缀（`TEST_`仅用于研发调试），状态枚举如下：

```c

typedef enum
{
ATE_STATE_IDLE,
ATE_STATE_NFC,
ATE_STATE_SETTING,
ATE_STATE_PREPARE,
ATE_STATE_RUNNING,
ATE_STATE_RESULT,
ATE_STATE_UPLOAD
} ate_state_t;
```

### 5.2 状态机驱动与 UI 订阅

- 状态机由`ate_test_manager`驱动，负责业务逻辑层面的状态流转

- UI Service 层的`ui_page_manager`订阅状态变化，通过`ui_page_manager_on_state_change()`方法触发页面切换：

    - `ATE_STATE_RUNNING` → 切换至 ATE Running 页面

    - `ATE_STATE_RESULT` → 切换至 ATE Result 页面

### 5.3 状态流转触发条件

|状态流转|触发条件|
|---|---|
|ATE_STATE_IDLE → ATE_STATE_NFC|用户点击 Home 页面的 "开始测试" 按钮，或需要更换操作员时触发|
|ATE_STATE_NFC → ATE_STATE_SETTING|NFC 打卡成功，或用户选择跳过 NFC 打卡时触发|
|ATE_STATE_SETTING → ATE_STATE_PREPARE|用户点击 ATE Setting 页面的 "开始测试" 按钮，配置验证通过时触发|
|ATE_STATE_PREPARE → ATE_STATE_RUNNING|设备自检完成、模块初始化完成、仪表握手成功时触发|
|ATE_STATE_RUNNING → ATE_STATE_RESULT|测试完成（达到测试次数或超时），或用户终止测试时触发|
|ATE_STATE_RESULT → ATE_STATE_UPLOAD|用户点击 ATE Result 页面的 "上传服务器" 按钮，或自动上传开启时触发|
|ATE_STATE_UPLOAD → ATE_STATE_IDLE|上传完成，或用户点击 "返回 Home" 按钮时触发|
## 六、事件系统

### 6.1 统一事件总线设计

采用事件总线实现任务间通信，定义统一事件类型与消息结构：

```c

typedef enum
{
EVT_MODULE_INSERT,
EVT_MODULE_REMOVE,
EVT_INSTRUMENT_CONNECT,
EVT_INSTRUMENT_DISCONNECT,
EVT_TEST_PROGRESS,
EVT_TEST_FINISH,
EVT_UPLOAD_COMPLETE,
EVT_TEST_START
} ate_event_t;

typedef struct
{
ate_event_t type;
void *data;
} ate_event_msg_t;
```

### 6.2 事件发送与订阅

- 所有业务任务（Module Monitor Task、Instrument Monitor Task 等）通过事件总线发送状态事件

- UI Task 订阅相关事件，仅在 UI 线程中处理 UI 更新操作，确保 LVGL 线程安全

### 6.3 EEZ 事件转发机制

EEZ Studio 生成的控件事件需通过`ui_event_adapter`转发至 ATE Application 层，禁止在 UI 层处理业务逻辑：

1. **事件注册**：在页面创建时，将 EEZ 控件的事件回调注册为转发函数

2. **事件转发**：转发函数将事件封装为`ate_event_msg_t`，通过事件总线发送至对应的业务任务

3. **事件响应**：业务任务处理事件后，通过队列发送状态更新至 UI Task，由 UI Task 更新 UI

## 七、配置管理

### 7.1 参数输入策略

为减少工程师输入操作，采用三层参数输入策略：

1. **默认配置**：系统启动时读取 NVS/eMMC/SD 中的默认配置

2. **最近配置**：自动加载上一次测试的参数配置

3. **配置模板**：支持 Test Profile 模板，工程师仅需选择模板即可完成配置

### 7.2 配置存储结构

定义统一的配置数据结构，存储于 NVS 中：

```c

typedef struct
{
uint32_t test_count;
uint32_t timeout;
uint32_t instrument_type;
uint32_t module_mask;
uint32_t test_profile;
} ate_config_t;
```

配置更新策略：用户点击 "开始测试" 时，自动保存当前配置至 NVS。

### 7.3 Profile 模板实现

1. **Profile 存储**：Profile 模板存储于 eMMC 或 SD 卡中，每个 Profile 对应一个 JSON 文件，包含测试参数、仪表配置、模块配置等

2. **Profile 加载**：在 ATE Setting 页面，通过 lv_dropdown 选择 Profile 后，读取 JSON 文件并加载到`ate_config_t`结构体中

3. **Profile 导出**：用户可将当前配置导出为 Profile 模板，存储到 eMMC 或 SD 卡中

## 八、模块管理

### 8.1 模块监控任务

Module Monitor Task 运行于 Core1，周期为 500ms，负责：

- 扫描测试模块 slot1~slot8 的热插拔状态

- 识别模块 ID、类型、固件版本

- 检测模块在线状态与插拔时间

### 8.2 模块状态模型

```c

typedef enum
{
MODULE_STATE_EMPTY,
MODULE_STATE_PRESENT,
MODULE_STATE_READY,
MODULE_STATE_ERROR
} module_state_t;

typedef struct
{
uint8_t slot_id;
uint32_t module_id;
uint8_t state;
uint32_t firmware;
uint64_t plug_time;
} module_info_t;
```

### 8.3 UI 更新机制

Module Monitor Task 通过事件总线发送模块状态事件，UI Task 接收事件后更新 Module Manager 页面的 lv_table 控件，实时展示模块状态：

- 绿色：MODULE_STATE_READY

- 灰色：MODULE_STATE_EMPTY

- 红色：MODULE_STATE_ERROR

### 8.4 热插拔处理细节

1. **模块热插拔检测**：Module Monitor Task 通过扫描 SPI/I2C 总线检测模块插拔，检测周期为 500ms

2. **事件发送**：检测到模块插拔时，发送`EVT_MODULE_INSERT`或`EVT_MODULE_REMOVE`事件，携带模块信息

3. **UI 刷新**：UI Task 接收事件后，在`on_update`回调中更新 Module Manager 页面的 lv_table 控件，使用`lv_table_set_cell_value`更新模块状态，避免直接操作 LVGL 对象树导致的线程安全问题

## 九、仪表管理

### 9.1 仪表监控任务

Instrument Monitor Task 运行于 Core1，负责：

- 监控仪表设备（串口 / RS485/USB）的热插拔状态

- 检测仪表通信参数（波特率、设备型号）

- 发送仪表连接 / 断开事件

### 9.2 仪表状态模型

```c

typedef enum
{
INSTRUMENT_STATE_DISCONNECTED,
INSTRUMENT_STATE_CONNECTED,
INSTRUMENT_STATE_READY,
INSTRUMENT_STATE_ERROR
} instrument_state_t;

typedef struct
{
uint8_t port;
uint32_t baudrate;
uint32_t type;
uint8_t state;
} instrument_info_t;
```

### 9.3 仪表配置页面

仪表配置页面支持自动检测、手动选择、导入 / 导出配置，通过 lv_dropdown、lv_switch、lv_button 控件实现配置操作，配置数据存储于 NVS 中。

### 9.4 仪表热插拔处理细节

1. **仪表热插拔检测**：Instrument Monitor Task 通过检测串口 / USB 的设备连接状态（如 USB 的枚举事件、串口的 DTR 信号）检测仪表插拔

2. **事件发送**：检测到仪表连接 / 断开时，发送`EVT_INSTRUMENT_CONNECT`或`EVT_INSTRUMENT_DISCONNECT`事件

3. **UI 刷新**：UI Task 接收事件后，更新 status_bar 的仪表数量显示，以及 Instrument Config 页面的仪表列表

## 十、UI 更新机制

### 10.1 LVGL 线程安全模型

LVGL 仅允许在 UI Task 中调用 API，所有业务任务禁止直接操作 LVGL 控件，必须通过 Queue/Event 的方式将数据传递至 UI Task，由 UI Task 统一更新 UI。

### 10.2 UI 任务实现

UI Task 运行于 Core0，唯一职责为执行 lv_timer_handler () 与处理 UI 消息：

```c

void ui_task(void *arg)
{
while (1)
{
lv_timer_handler();
// 处理UI消息队列
ate_event_msg_t msg;
if (xQueueReceive(ui_queue, &msg, pdMS_TO_TICKS(5)) == pdPASS)
{
process_ui_event(&msg);
}
vTaskDelay(pdMS_TO_TICKS(5));
}
}
```

### 10.3 测试进度更新

ATE Test Task 通过队列发送测试进度消息，UI Task 接收消息后更新 ATE Running 页面的进度条、PASS/FAIL 计数、剩余时间等控件：

```c

typedef struct
{
uint8_t progress;
uint32_t pass;
uint32_t fail;
uint32_t remain_time;
} ate_progress_msg_t;

// ATE Test Task发送消息
ate_progress_msg_t progress = {45, 12, 1, 154};
xQueueSend(ui_queue, &progress, pdMS_TO_TICKS(10));
```

### 10.4 LVGL 渲染适配

1. **双核渲染配合**：Core0 负责 LVGL 的渲染和 UI 更新，Core1 负责业务逻辑，通过队列传递数据，避免 Core1 阻塞 Core0 的渲染

2. **渲染优化**：使用 LVGL 的`lv_disp_flush_ready`回调优化显示刷新，避免屏幕撕裂

3. **内存优化**：使用 LVGL 的`lv_mem_pool`分配 UI 对象内存，减少内存碎片

## 十一、关键设计规范与注意事项

### 11.1 工业 UI 原则

- 优先级：信息清晰 > 动画效果；稳定可靠 > 花哨设计；操作简单 > 功能复杂

- 工程师核心关注点：设备当前状态、是否存在错误、测试完成时间

### 11.2 页面管理规范

- 禁止频繁创建 / 销毁页面，必须采用 show/hide 切换方式

- 所有页面继承 base_page 模板，确保 UI 结构一致性

### 11.3 任务优先级规范

- UI Task 优先级设为 3，业务任务优先级设为 4~5，避免 UI 抢 CPU 导致测试变慢

- 禁止在 UI Task 中执行耗时操作，确保 UI 流畅性

### 11.4 热插拔支持

系统必须支持测试模块与仪表设备的热插拔，Module Monitor Task 与 Instrument Monitor Task 需实时检测状态变化，并通过事件总线通知 UI 更新，确保 UI 实时刷新且不卡顿。

### 11.5 线程安全规范

- 禁止非 UI 线程调用 LVGL API，所有 UI 更新必须通过 UI Task 执行

- 共享资源必须通过互斥量保护，避免多任务同时访问导致的数据冲突


11 UI页面设计

本章描述 UI 页面结构与示意图。

⸻

11.1 Boot 页面

页面职责

设备启动展示。

页面事件

on_boot_complete → Home

页面数据来源

system_version
init_state

UI示意图

+--------------------------------+
|           ATE Tester           |
|                                |
|             LOGO               |
|                                |
|   Firmware Version: v1.0.3     |
|                                |
|   System Initializing...       |
|   [###########-----]           |
|                                |
+--------------------------------+


⸻

11.2 Home 页面

页面职责

设备总体状态展示。

页面事件

btn_start_test → NFC
btn_module → Module Manager

数据来源

module_manager
instrument_manager
operator_manager

UI示意图

+--------------------------------+
| WiFi  Modules:8  Inst:2  Time  |
+--------------------------------+
|           HOME                 |
|                                |
| [ Start Test ]                 |
|                                |
| [ Module Manager ]             |
| [ Instrument Config ]          |
| [ History Log ]                |
| [ OTA Upgrade ]                |
+--------------------------------+


⸻

11.3 NFC 页面

职责：

操作员身份确认。

UI：

+------------------------------+
|           NFC Login          |
|                              |
|        Please Tap Card       |
|                              |
|        Operator: ---         |
+------------------------------+


⸻

11.4 ATE Setting 页面

职责：

配置测试参数。

UI：

+------------------------------+
|         Test Setting         |
|                              |
| Test Count: 10               |
| Timeout: 30s                 |
| Instrument: DMM34465A       |
| Module Mask: 1 2 3           |
|                              |
|   [Start Test]               |
+------------------------------+


⸻

11.5 ATE Prepare 页面

职责：

测试准备。

UI：

+------------------------------+
|         Test Prepare         |
|                              |
| Checking Modules...          |
| Connecting Instruments...    |
| Initializing Scripts...      |
|                              |
| Status: OK                   |
+------------------------------+


⸻

11.6 ATE Running 页面

核心页面。

UI：

+------------------------------+
|        Test Running          |
|                              |
| Script: VoltageTest_v2       |
| Step: Measure Voltage        |
|                              |
| Progress: [#######-----]45%  |
|                              |
| PASS:12   FAIL:1             |
| Remaining: 02:34             |
|                              |
| [Pause]     [Stop]           |
+------------------------------+


⸻

11.7 ATE Result 页面

+------------------------------+
|          Test Result         |
|                              |
| RESULT: PASS                 |
|                              |
| Fail List:                   |
| - Channel 3 Voltage          |
|                              |
| Operator: Alice              |
| Time: 2026-03-12             |
|                              |
| [Upload]  [Back]             |
+------------------------------+


⸻

11.8 Upload 页面

+------------------------------+
|         Upload Server        |
|                              |
| Uploading Results...         |
|                              |
| Progress [########----]      |
|                              |
| Status: Success              |
+------------------------------+


⸻

11.9 Module Manager 页面

+--------------------------------+
| Slot | Module | FW | State     |
|--------------------------------|
| 1    | A101   |1.2 | READY     |
| 2    | A102   |1.2 | READY     |
| 3    | ----   |--  | EMPTY     |
+--------------------------------+


⸻

11.10 Instrument Config 页面

+--------------------------------+
| Instrument Config              |
|                                |
| Port: RS485                    |
| Baud: 115200                   |
| Model: Keysight34465A          |
|                                |
| [Auto Detect]                  |
| [Save]                         |
+--------------------------------+


⸻

11.11 History Log 页面

+--------------------------------+
| Time       | Result | Operator |
|--------------------------------|
|10:12:02    | PASS   | Alice    |
|10:30:14    | FAIL   | Bob      |
+--------------------------------+


⸻

11.12 OTA 页面

+--------------------------------+
|         OTA Upgrade            |
|                                |
| Current: v1.0.3                |
| New: v1.0.4                    |
|                                |
| [Upgrade]                      |
|                                |
| Progress [#####------]         |
+--------------------------------+


⸻

