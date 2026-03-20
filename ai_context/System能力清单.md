# System 能力清单

## 文档定位

本文档定义 ATE 项目当前阶段建议保留的 System Layer 能力边界。

- 只记录当前工程里已经存在、且确实值得作为 System 能力维护的模块
- 不为了分层而分层，不引入 Linux 风格的大统一配置中心、消息路由中心、存储中心
- 对小型 SoC / MPU / MCU 没有明确复用价值的抽象，不强行做成 `system_xxx`
- 本文档不展开 API 细节，不展开初始化顺序，只说明职责边界

---

## System Layer 总原则

System Layer 位于 App 层之下、Driver 层之上，但它不是“所有东西都往中间收”的总包层。

- `board` / `bsp` 解决“板子怎么连、资源怎么映射”
- `driver` 解决“某个外设或协议栈怎么用”
- `system` 解决“少量需要跨 App 复用的系统级能力怎么提供”
- `app` 解决“测试流程、业务判断、页面逻辑怎么组织”

ATE 当前是一个小平台，因此 System 层只保留真正有必要统一的能力，不保留空壳和过度抽象。

---

## 当前纳入能力清单的模块

## system_event

### 1. 模块定位
`system_event` 是项目级事件发布与订阅能力，用于 System / App 模块之间的松耦合协作。

### 2. 核心职责
- 提供统一的事件发布与订阅接口
- 承载项目内公共事件流转
- 避免每个模块各自直接建一套事件分发机制

### 3. 允许依赖
- ESP-IDF `esp_event`

### 4. 禁止依赖
- `app_xxx` 业务模块
- 具体硬件驱动实现
- 具体 UI 页面对象

### 5. 不负责内容
- 业务流程编排
- 事件语义设计本身
- 页面跳转与界面状态管理

### 6. 边界说明
项目内需要公共事件协作时，优先通过 `system_event`，而不是各模块直接裸用 `esp_event` 形成多套边界。

---

## system_comm_mgr

### 1. 模块定位
`system_comm_mgr` 是轻量级通信资源注册与查找能力，用于管理 I2C 主机句柄与 UART 链路句柄。

### 2. 核心职责
- 注册、注销 I2C / UART 链路
- 通过 `link_id` 查找对应链路
- 为上层 System Service 提供统一的链路获取入口

### 3. 允许依赖
- `driver_i2c_master`
- `system_uart_link`
- `freertos`

### 4. 禁止依赖
- `app_xxx` 业务模块
- 具体协议帧格式
- 业务消息语义

### 5. 不负责内容
- 不负责协议解析与收发
- 不负责业务调度
- 不负责统一消息路由

### 6. 边界说明
`system_comm_mgr` 只是资源登记处，不是“大统一通信管理层”，更不是 `system_router` 那种跨模块总线。

---

## system_protocol

### 1. 模块定位
`system_protocol` 是 ATE 自动化测试通信协议的编解码能力，当前与根目录的 [自动化测试I2C通信协议.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/自动化测试I2C通信协议.md) 强绑定。

### 2. 核心职责
- 定义协议帧头、帧类型、状态码与命令码枚举
- 负责请求帧、响应帧、事件帧、状态帧构造
- 负责 CRC16 计算
- 负责收到数据后的协议解析与合法性校验

### 3. 允许依赖
- `esp_err`
- 标准整数与内存操作

### 4. 禁止依赖
- `app_xxx`
- 具体 I2C / UART / USB 传输实现
- 具体板级资源定义

### 5. 不负责内容
- 不负责物理链路收发
- 不负责设备扫描与上线管理
- 不负责测试流程组织

### 6. 边界说明
协议文档一旦变更，`system_protocol` 必须同步更新。它是协议语义边界，不是传输边界。

---

## system_module_service

### 1. 模块定位
`system_module_service` 是下位模组管理与协议通信服务，也是模组共享 I2C 总线的唯一 owner，负责模组发现、挂接、协议收发与状态维护。

### 2. 核心职责
- 扫描 I2C 地址范围，发现在线模组
- 维护模组表与状态信息
- 模组表至少包含 `i2c_addr + protocol_device_id`；若前端存在 I2C 选择器 / mux，还应纳入 `channel` 维度
- 负责模组总线的串行化访问，保证单次原子收发未完成前不切换到下一次访问
- 基于 `system_protocol` 构造请求帧并完成收发
- 解析回包并更新模组状态
- 发布模组上线/掉线、I2C 传输、协议异常等事件
- 在错误场景下向 `system_fault` 上报故障

### 3. 允许依赖
- `system_comm_mgr`
- `driver_i2c_master`
- `driver_i2c_mux` / `driver_i2c_selector`（如硬件存在选择器）
- `system_protocol`
- `system_event`
- `system_fault`
- `freertos`

### 4. 禁止依赖
- `app_xxx` 业务模块
- UI 页面逻辑
- 板级 pin / I2C 实例硬编码

### 5. 不负责内容
- 测试流程组织
- 测试项选择与业务判定
- UI 列表展示

### 6. 边界说明
模组通信不是单纯“扫描一下 I2C 设备”，而是和 [自动化测试I2C通信协议.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/自动化测试I2C通信协议.md) 强绑定的系统服务。App 层如需访问下位模组，应通过 `system_module_service_*` 使用语义接口，不直接知道 mux 通道、I2C 地址、命令码与收发细节。

---

## system_fault

### 1. 模块定位
`system_fault` 是系统级故障收敛、记录与分发能力。

### 2. 核心职责
- 统一记录最近一次系统故障
- 提供故障码映射与故障查询能力
- 对外发布故障事件，供 App 决策处理

### 3. 允许依赖
- `system_event`
- `esp_timer`
- `freertos`

### 4. 禁止依赖
- `app_xxx`
- 板级资源
- 具体业务恢复策略

### 5. 不负责内容
- 自动恢复策略
- UI 告警展示
- 驱动内部错误处理

### 6. 边界说明
`system_fault` 负责“故障被统一看到”，不负责“故障应该怎么处理”。

---

## system_usb

### 1. 模块定位
`system_usb` 是 USB 子系统能力边界，当前应同时覆盖 USB Host 与 USB Device 两类场景。

### 2. 核心职责
- 提供 USB Host 生命周期管理
- 提供 U 盘挂载、USB OTA、CDC 串口打开等 Host 能力
- 提供 USB Device 栈启停能力
- 提供 USB Device CDC / MSC 从机能力启用

### 3. 允许依赖
- `driver_usb_host`
- `driver_usb_device`
- `esp_msc_ota`
- USB Host / TinyUSB 等底层基础设施

### 4. 禁止依赖
- `app_xxx`
- 板级业务逻辑
- 上层 USB 业务协议语义

### 5. 不负责内容
- PC 侧业务流程
- 上层业务协议解析
- 测试流程如何使用 USB 的决策

### 6. 边界说明
这里不应只定义“USB Host 管理”。当前工程既有主机 USB，也存在把本机模拟成 USB 从机的需求，因此它们都应属于同一个 `system_usb` 能力边界，内部再拆 `system_usb_host` / `system_usb_device` 即可。

---

## system_display

### 1. 模块定位
`system_display` 是显示子系统能力，负责显示驱动、LVGL 运行环境与 UI 初始化入口的系统级封装。

### 2. 核心职责
- 初始化显示硬件与 LVGL 适配层
- 提供 UI 初始化回调注册入口
- 提供显示锁与解锁能力
- 提供分辨率查询与硬件测试图能力

### 3. 允许依赖
- `driver_display`
- `esp_lv_adapter`
- `lvgl`
- `esp_lcd`

### 4. 禁止依赖
- `app_xxx` 页面业务逻辑
- 板级资源细节
- 具体页面对象组织

### 5. 不负责内容
- 页面结构设计
- 页面跳转逻辑
- 业务状态机

### 6. 边界说明
`system_display` 管的是显示运行时与显示基础设施，不负责页面业务本身。

---

## system_led

### 1. 模块定位
`system_led` 是系统级状态指示能力，向上暴露抽象状态和效果模式，向下调用灯带驱动实现。

### 2. 核心职责
- 统一管理状态灯抽象状态
- 统一管理静态、呼吸、追逐、彩虹等效果模式
- 将系统状态映射为具体 LED 效果

### 3. 允许依赖
- `driver_ledstrip`

### 4. 禁止依赖
- `app_xxx` 业务判断逻辑
- 板级灯珠排布细节向上泄漏
- 具体测试结果策略

### 5. 不负责内容
- 业务是否判定为 pass / fail
- 页面联动逻辑
- 上层测试策略

### 6. 边界说明
App 层应使用抽象状态与模式，不直接控制像素级细节。

---

## 当前不纳入正式能力清单的模块

以下组件当前存在于工程中，但不建议写成正式 System 能力承诺：

- `system_storage`
  现在基本是空壳；对小型 SoC / MPU / MCU 也没有独立抽象价值。当前需要持久化时，直接用乐鑫现成能力即可。若后续确实出现统一管理价值，再新增事件驱动式存储服务。


- `system_router`
  当前实现只是基于 `esp_event` 的局部分发器，不应拔高成“大统一消息路由层”。

- `system_instrument_service`
  现在还是占位实现，`scan/attach/send` 都返回 `ESP_ERR_NOT_SUPPORTED`，尚未形成正式能力边界。

- `system_network`
  头文件描述的是较完整的网络状态管理，但当前实现只有最小初始化包装，能力尚未闭环。

- `system_uart_link`
  更适合作为 `system_comm_mgr` 的底层支撑组件，而不是顶层对 App 宣称的独立 System 能力。

- `system_log`
  当前仅做日志初始化，不需要单列为正式 System 能力。


---

## 附：当前建议保留的依赖方向一览

| 模块 | 允许依赖 Driver | 允许依赖 System | 允许依赖基础设施 |
|---|---|---|---|
| `system_event` | — | — | `esp_event` |
| `system_comm_mgr` | `driver_i2c_master` | `system_uart_link` | `freertos` |
| `system_protocol` | — | — | `esp_err` |
| `system_module_service` | `driver_i2c_master`、`driver_i2c_mux` / `driver_i2c_selector`（可选） | `system_comm_mgr`、`system_protocol`、`system_event`、`system_fault` | `freertos` |
| `system_fault` | — | `system_event` | `esp_timer`、`freertos` |
| `system_usb` | `driver_usb_host`、`driver_usb_device` | — | USB Host / TinyUSB / `esp_msc_ota` |
| `system_display` | `driver_display` | — | `esp_lv_adapter`、`lvgl`、`esp_lcd` |
| `system_led` | `driver_ledstrip` | — | — |

> 新增 System 模块前，先回答一件事：这个能力是不是跨 App 复用、并且值得在不同芯片/不同业务之间持续维护。如果答案不明确，就不要先做成 `system_xxx`。
