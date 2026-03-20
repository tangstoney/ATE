# ATE 项目架构总纲

## 1. 文档定位

本文档只定义 ATE 项目的顶层软件架构约束，用于统一后续各子系统设计文档、代码实现和 AI 审查口径。

本文档负责回答四个问题：

- ATE 项目整体采用什么架构
- 每一层的职责边界在哪里
- 仓库中的模块应如何归属
- 哪些内容属于总纲，哪些内容必须下沉到专题文档

本文档**不负责**展开以下内容：

- 任务优先级、任务绑核、队列长度、消息结构细节
- UI 页面树、LVGL/EEZ 生命周期、页面跳转逻辑
- 通信协议帧格式、寄存器级时序、驱动实现细节
- 具体测试流程、工站逻辑、判定规则、上传策略

以上内容应分别写入对应的子系统设计文档、接口文档或专题文档。

## 2. 项目基础信息

- 项目名称：ATE（Auto Test Equipment）
- 主控平台：乐鑫 ESP32-P4
- SDK：ESP-IDF v5.5.3
- 开发语言：C
- 开发辅助工具：ChatGPT 5.4 + Codex
- 架构原则：严格三层结构，业务语义归 App，通用服务归 System，IDF 使用与硬件适配归 Driver

参考基线：

- ESP-IDF 官方文档  
  https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32p4/index.html
- ESP-IDF 组件管理与组件化思想  
  https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/tools/idf-component-manager.html
- USB Host / Class Driver 分层示例  
  https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-reference/peripherals/usb_host.html#id22
- ESP-IDF Opaque Handle 设计风格  
  https://developer.espressif.com/blog/2025/10/oop_with_c/#handles-in-esp-idf

## 3. 架构目标

ATE 项目架构的核心目标如下：

- 保证业务逻辑与硬件实现解耦，避免应用层直接依赖 ESP-IDF 外设接口
- 保证硬件接口可替换，便于后续扩展不同模块、不同仪表和不同板级版本
- 保证模块职责单一，方便多人协作、多AI 辅助开发和后续重构
- 保证系统层对上提供稳定语义，对下屏蔽设备与总线差异
- 保证文档、目录、代码接口三者一致，降低后续审查成本

## 4. 总体架构

ATE 采用严格三层软件架构：

```text
Application Layer
        ↓
System Layer
        ↓
Driver Layer
        ↓
ESP-IDF / Board BSP / Hardware
```

说明：

- `Application Layer` 是业务入口层，只表达业务意图，不关心具体硬件访问方式
- `System Layer` 是系统服务层，负责把多个驱动组合成稳定的业务可调用能力
- `Driver Layer` 是设备驱动层，直接面向 ESP-IDF、官方组件和板级资源
- `Board/BSP` 是板级适配支撑，不单独视为业务架构层；其职责是提供引脚、外设实例、板级宏和初始化支撑，供 Driver 使用

## 5. 分层职责定义

### 5.1 Application Layer

Application Layer 负责：

- 测试业务流程编排
- 业务状态推进与结果判定
- 操作员交互入口，如 UI、CLI、网络命令入口
- 基于系统层能力完成产测、上传、控制等业务行为

Application Layer 不负责：

- 直接调用 `gpio_xxx`、`i2c_xxx`、`uart_xxx`、`usb_host_xxx` 等 ESP-IDF/HAL API
- 管理底层设备生命周期
- 承担板级资源适配

### 5.2 System Layer

System Layer 负责：

- 对多个 Driver 进行封装、组合和统一管理
- 对外暴露稳定的系统服务接口
- 维护资源生命周期、连接状态、系统级状态机和故障收敛
- 提供事件、路由、配置、存储、通信链路等通用服务

System Layer 不负责：

- 实现具体业务流程文案和工艺规则
- 直接依赖 ESP-IDF HAL 头文件完成硬件访问
- 承载具体业务语义，测试流程、判定规则或UI逻辑（防止 system 被写成 app-lite，防止后期架构腐化）

### 5.3 Driver Layer

Driver Layer 负责：

- 直接对接 ESP-IDF 驱动、官方组件和板级资源
- 封装外设访问细节，向上提供稳定的设备能力接口，状态原始信息
- 管理单一设备或单一总线的底层读写、初始化、关闭和错误返回
- 使用 opaque handle、配置结构体、`esp_err_t` 等官方风格接口
- 配置结构体必须完整初始化，推荐使用 C99 指定初始化器（= { .field = val }），避免未初始化字段导致未定义行为
Driver Layer 不负责：

- 业务流程编排
- 测试通过/失败等业务判定
- UI 页面逻辑
- 做策略判断与业务语义封装 （避免后期“驱动写成半系统层”）

## 6. 目录与架构映射

结合当前仓库，顶层目录映射关系如下：

|目录|架构归属|说明|
|---|---|---|
|`components/apps`|Application Layer|业务入口、UI 编排、业务控制|
|`components/system`|System Layer|系统服务、资源管理、状态收敛|
|`components/driver`|Driver Layer|设备驱动、总线封装、IDF 适配|
|`components/board`|Board/BSP 支撑|板级资源定义与初始化支撑|
|`components/bsp_manager`|Board/BSP 支撑|板级管理与统一入口|

当前项目中，具有代表性的系统层能力包括：

- `system_module_service`
- `system_instrument_service`
- `system_network`
- `system_storage`
- `system_usb`
- `system_event`
- `system_router`
- `system_fault`
- `system_config`
系统内跨模块通信应优先通过统一的事件/消息机制实现，避免多套并行通信模型（codex todo，直接用esp原生的esp_event，使用默认事件循环）


当前项目中，具有代表性的驱动层能力包括：

- `driver_display`
- `driver_input`
- `driver_usb`
- `driver_network`
- `driver_eth`
- `driver_i2c_master`
- `driver_uart_port`
- `driver_ledstrip`

以上列举用于说明归属关系，不代表最终组件清单已经冻结。

## 7. 依赖规则

ATE 项目必须遵守单向依赖原则：

```text
App  -> System
System -> Driver
Driver -> ESP-IDF / Board BSP
```

禁止的依赖关系：

- App 直接依赖 Driver
- App 直接依赖 ESP-IDF 外设接口或板级宏
- System 直接跳过 Driver 访问硬件
- Driver 反向依赖 App 或 System 业务模块

补充约束：

- 跨层调用只能自上而下
- 反向通知必须通过事件、回调、消息或状态查询等受控接口完成，不能形成实现层面的反向依赖
- 公共类型若被多层复用，应定义在明确的公共接口中，避免上层直接包含下层私有头文件

## 8. 架构裁剪原则

为避免模块语义漂移，ATE 架构按以下规则裁剪：

### 8.1 什么属于 App

以下内容属于 App：

- 测试项目选择
- 测试流程推进
- 测试结果判定与业务状态切换
- 面向操作员的交互语义
- 上传动作的业务触发条件

### 8.2 什么属于 System

以下内容属于 System：

- 模块扫描服务
- 仪表接入服务
- 通信链路管理
- 配置、存储、事件、路由、故障等通用能力
- 多驱动资源组合后的统一服务接口

### 8.3 什么属于 Driver

以下内容属于 Driver：

- I2C/UART/USB/Display/Input/Network 等具体设备访问
- 对 ESP-IDF 驱动与官方组件的直接封装
- 设备句柄、底层缓冲区、同步原语和硬件初始化细节

### 8.4 什么属于 BSP

以下内容属于 Board/BSP：

- 引脚定义
- 面板分辨率
- 板级供电/复位控制资源
- 板级默认外设实例和初始化支撑

Board/BSP 不承担测试逻辑、服务编排和设备业务状态判断。

## 9. 接口+命名风格总原则

本项目接口设计遵循以下总原则：

- 对外接口优先使用 `esp_err_t`
- get 对外接口优先直接返回对应变量或地址
- Driver 层优先使用 opaque handle，避免暴露内部结构体
- 配置输入与运行时句柄分离
- 生命周期明确，初始化、启停、销毁语义清晰
- 接口命名优先体现能力而不是业务故事
- Driver 输出“设备事实”，System 输出“系统服务语义”，App 输出“业务语义”

这里仅定义总原则，不在本总纲中展开具体 API 设计与头文件示例，可以去约束-接口命名规则里查看

## 10. 文档分工

为避免总纲与专题文档重复，文档按如下方式分工：

|文档|建议承载内容|
|---|---|
|ATE项目架构总纲.md|顶层架构、分层边界、目录映射、依赖规则、文档分工|
|ATE UI软件子系统设计文档.md|UI 子系统架构、页面体系、LVGL/EEZ 集成、UI 线程模型|
|项目事实卡.md|平台事实、硬件资源、设备清单、外部依赖 +（硬件→driver映射）|
|约束-接口命名规则.md|命名规范、接口命名、文件命名约束|
|审核-当前阶段边界.md|当前阶段范围、冻结内容|
|审核-测试验证规则.md|接口验收、联调规则|
|System能力清单.md（新增）|system_xxx 对外能力定义 + 依赖driver|
|Driver硬件映射.md（新增）|硬件器件 → driver 映射关系|
|系统数据流.md（新增）|核心数据/事件流向（简图级）|

总纲中只保留“原则与边界”，不展开“实现与参数”。

## 11. 当前阶段的顶层结论

当前 ATE 项目的顶层架构结论如下：

- 三层结构方向正确，仓库目录也已基本按该结构展开
- 后续新增模块应优先判断其归属，再决定放入 `apps`、`system`、`driver` 还是 `board`
- 任何实现细节文档都不应反向修改总纲边界
- 如果某一模块难以归类，优先按“是否含业务语义、是否直接访问硬件、是否承担资源编排”三个问题进行判定

该总纲作为后续各专题设计文档和 AI 审查提示词的统一上位约束使用。
