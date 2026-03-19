# ATE 通信架构约束文档（RS422 / RS485 / I2C）

> 平台约束：ESP-IDF **v5.5.3**  
> 芯片上下文：ESP32-P4  
> 目标：为 Cursor / Codex 生成稳定、一致、可落地的通信层代码约束，避免生成过时 API、错误分层和不必要的自定义框架。

---

## 1. 架构目标

本项目中，ESP32-P4 属于**测试控制终端**，不负责完整测试业务逻辑，不直接承载所有被测设备协议细节。

ESP32-P4 的职责限定为：

- UI / HMI
- 通信链路管理（UART / RS422 / RS485 / I2C / Ethernet）
- 测试流程状态机驱动
- 配置管理
- 关键状态采集
- 数据转发 / 路由
- 基础容错与故障上报

ESP32-P4 **不是**“完整业务解释器”，也**不是**“所有下位机协议的全量实现端”。

---

## 2. 分层原则

### 2.1 App Layer

只允许调用 **System API**，禁止直接访问：

- ESP-IDF 驱动 API
- UART/I2C 原始读写接口
- GPIO 翻转控制收发方向
- FreeRTOS 队列细节
- `esp_event` 原生发布细节

App 层只表达业务语义，例如：

- 扫描仪表
- 扫描模组
- 获取在线状态
- 执行测试动作
- 请求转发数据
- 刷新界面

### 2.2 System Layer

System 层是通信系统的核心层，负责：

- 通信资源管理
- 协议编解码
- 设备服务抽象
- 数据路由与转发
- 事件封装
- 容错与故障码映射
- 线程/任务边界管理

System 层向上提供“设备语义接口”，向下调用 Driver 层。

### 2.3 Driver Layer

Driver 层负责**硬件能力抽象 + 链路级能力封装**，而不只是“最薄的一层 API 转抄”。

Driver 层允许封装以下**链路级能力**：

- UART 安装与参数配置
- I2C bus / device 创建与销毁
- 原始收发
- 超时控制
- 缓冲区管理
- 运行时波特率切换
- 运行时链路重建（delete / recreate）
- 端口探测 / 设备探测所需的基础能力
- 必要的硬件能力启用

Driver 层**禁止**包含：

- 业务状态机
- 仪表扫描策略本身
- 模组识别策略本身
- UI 逻辑
- 服务器转发策略
- 复杂协议解释

说明：

- Driver 层可以提供“热插拔探测”“波特率切换”“端口可用性检查”“链路恢复”这类**通用链路能力**。
- 但“何时扫描、扫描哪些波特率、识别成功后绑定到哪个业务对象”仍属于 System 层。

---

## 3. 模块划分

### 3.1 Driver Layer

- `driver_uart_port`：UART 端口驱动封装
- `driver_i2c_master`：I2C 主机驱动封装（新驱动）
- `driver_gpio_irq`：必要 GPIO 中断封装
- `driver_timer`：必要定时器封装

> 仅当硬件设计必须使用应用层手动方向控制时，才允许存在 `driver_rs485_dir`。  
> 若使用 ESP-IDF RS485 半双工模式，则**不创建**该模块。

### 3.2 System Layer

- `system_comm_mgr`：通信资源统一管理
- `system_uart_link`：串口链路对象管理
- `system_i2c_link`：I2C 链路对象管理
- `system_protocol`：协议帧编解码
- `system_router`：路由 / 转发
- `system_fault`：故障映射与故障上报
- `system_event`：系统事件封装（基于 `esp_event`）
- `system_instrument_service`：仪表服务（RS422/RS485）
- `system_module_service`：模组服务（I2C）
- `system_config`：端口/地址/协议配置

### 3.3 App Layer

- `app_test_flow`
- `app_instrument_manager`
- `app_module_manager`
- `app_setting`
- `app_ui`

---

## 4. 总线与设备抽象原则

### 4.1 不在 Driver 层强行统一 UART 和 I2C

UART/RS422/RS485 与 I2C 的底层模型不同：

- UART/RS422/RS485：**字节流 / 流式链路**
- I2C：**主从事务 / 地址化访问**

因此：

- Driver 层分别抽象：
  - `driver_uart_*`
  - `driver_i2c_*`
- System 层统一为“设备服务”：
  - `system_instrument_service_*`
  - `system_module_service_*`

### 4.2 上层面向设备语义，不面向裸总线

App 层看到的接口必须是：

- `scan()`
- `attach()`
- `get_info()`
- `send_cmd()`
- `exec_action()`
- `get_state()`

而不是：

- `uart_read_bytes()`
- `uart_write_bytes()`
- `i2c_master_transmit()`
- `i2c_master_receive()`

### 4.3 Driver 层允许“链路级能力”，但不承载业务语义

Driver 层适合放置：

- `driver_uart_port_set_baudrate()`
- `driver_uart_port_reconfigure()`
- `driver_uart_port_check_alive()`
- `driver_uart_port_recover()`
- `driver_i2c_master_probe()`
- `driver_i2c_master_recover()`

System 层适合放置：

- `system_instrument_scan_baudset()`
- `system_instrument_identify_device()`
- `system_module_scan()`
- `system_comm_mgr_bind_device()`

原则：

- Driver 负责“能不能做”与“怎么安全地做”。
- System 负责“什么时候做”“按什么顺序做”“结果绑定给谁”。

---

## 5. ESP-IDF v5.5.3 平台硬约束

### 5.1 文档与平台基线

本约束文档只参考 ESP-IDF **stable v5.5.3** 与 **ESP32-P4** 官方编程指南。

### 5.2 I2C：必须使用新驱动，不允许 legacy `driver/i2c.h`

ESP-IDF v5.5.3 的 I2C 文档明确区分：

- `i2c.h`：legacy I2C APIs
- `i2c_master.h`：new driver, master mode
- `i2c_slave.h`：new driver, slave mode

并明确指出：

- legacy driver 与 new driver **不能共存**
- legacy driver **已经 deprecated**，未来会移除

因此本项目 I2C 统一采用**新驱动模型**：

- 头文件：`driver/i2c_master.h`
- bus 句柄：`i2c_master_bus_handle_t`
- device 句柄：`i2c_master_dev_handle_t`
- 组件依赖：`esp_driver_i2c`

必须遵循的接口方向：

- `i2c_new_master_bus()`
- `i2c_master_bus_add_device()`
- `i2c_master_transmit()`
- `i2c_master_receive()`
- `i2c_master_transmit_receive()`
- `i2c_master_bus_rm_device()`
- `i2c_del_master_bus()`
- `i2c_master_probe()`（用于总线探测时）

禁止生成或使用以下内容：

- `#include "driver/i2c.h"`
- `i2c_param_config()`
- `i2c_driver_install()`
- `i2c_cmd_link_create()`
- `i2c_master_cmd_begin()`
- 任何 legacy I2C command link 写法

`driver_i2c_master` 的职责是：

- 创建 bus
- 为不同从设备创建设备句柄
- 封装同步收发事务
- 提供 `probe / recover / remove / recreate` 这类链路级能力
- 统一 timeout
- 映射底层错误码

`driver_i2c_master` **不负责**：

- 模组枚举策略
- 业务命令解释
- UI 刷新

### 5.3 RS422 与 RS485 必须显式区分

虽然两者都基于 UART，但在软件配置上**不可混用**。

#### RS422

- 全双工
- 不需要方向控制
- 使用普通 UART 模式：`UART_MODE_UART`

适用约束：

- 不创建 `driver_rs485_dir`
- 不调用 `uart_set_mode(..., UART_MODE_RS485_*)`
- 按普通 UART 端口处理

#### RS485

- 典型半双工
- 需要 DE / ~RE 方向控制
- 优先使用 ESP-IDF 内置模式：`UART_MODE_RS485_HALF_DUPLEX`

适用约束：

- 通过 `uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX)` 启用
- 通过 `uart_set_pin()` 绑定 RTS 到收发器 DE / ~RE 控制脚
- **必须禁用硬件流控**
- 不再额外实现软件层 GPIO 翻转方向控制

只有在硬件拓扑不兼容 `UART_MODE_RS485_HALF_DUPLEX` 时，才允许引入 `driver_rs485_dir` 做手动方向控制。

### 5.4 UART 驱动约束

UART 头文件与组件依赖必须遵循官方定义：

- 头文件：`driver/uart.h`
- 组件依赖：`esp_driver_uart`

串口驱动允许使用的典型 ESP-IDF 接口：

- `uart_driver_install()`
- `uart_driver_delete()`
- `uart_param_config()`
- `uart_set_pin()`
- `uart_set_mode()`
- `uart_set_baudrate()`
- `uart_write_bytes()`
- `uart_read_bytes()`
- `uart_wait_tx_done()`
- `uart_flush()`
- `uart_get_buffered_data_len()`

禁止在 App 层直接调用上述接口。

### 5.5 事件系统：统一基于 `esp_event`

本项目事件机制**不自创底层框架**，统一建立在 ESP-IDF `esp_event` 上。

System 层封装方式：

- 使用 `ESP_EVENT_DECLARE_BASE()` / `ESP_EVENT_DEFINE_BASE()` 定义事件域
- 使用 `esp_event_loop_create()` 创建 ATE 专用事件循环
- 使用 `esp_event_handler_register_with()` 注册事件处理器
- 使用 `esp_event_post_to()` 投递事件

约束：

- App 层只能调用 `system_event_subscribe()` / `system_event_unsubscribe()` / `system_event_publish()` 这类 System 封装接口
- App 层不直接依赖 `esp_event` 原生细节
- 默认 Wi-Fi / IP / Ethernet 系统事件与 ATE 自定义事件循环分离
- Driver 层不直接把 `esp_event` 暴露给 App 层

### 5.6 错误处理：底层返回 `esp_err_t`，上层使用领域故障码

Driver 与 System 的底层函数统一返回：

- `esp_err_t`

日志必须允许输出：

- `esp_err_to_name(err)`

`system_fault` 负责：

- 将底层 `esp_err_t` 映射为领域故障码
- 将协议错误映射为统一 fault code
- 将故障推送给 UI / log / network

规则：

- Driver 层和 System 内部可以传递 `esp_err_t`
- App 层**不直接处理原始 `esp_err_t`**
- App 层只接收：
  - 域内 fault code
  - 状态枚举
  - 语义化结果结构体

### 5.7 Debug 构建约束

开发阶段建议在 `sdkconfig.defaults` 中启用：

```ini
CONFIG_ESP_SYSTEM_USE_FRAME_POINTER=y
```

用途：

- 增强 panic / WDT 场景下的回溯可读性
- 便于定位阻塞点和任务栈调用链

补充要求：

- `CONFIG_ESP_SYSTEM_USE_FRAME_POINTER` 会带来大约 **+5~6%** 的 binary size 增长与约 **1%** 的性能下降
- `CONFIG_ESP_SYSTEM_USE_EH_FRAME` 不作为默认量产配置
- 开发版与量产版的 `sdkconfig.defaults` 可分离维护

---

## 6. System 层职责细化

### 6.1 `system_comm_mgr`

负责：

- 注册 UART / I2C 资源
- 创建链路对象
- 统一管理端口占用状态
- 管理链路生命周期
- 为上层服务提供句柄查询

不负责：

- 业务命令解释
- UI 联动

### 6.2 `system_protocol`

负责：

- 帧封包
- 帧解析
- CRC / checksum
- 长度字段校验
- 命令字/响应字识别
- 原始字节流转结构体

不负责：

- 物理收发
- 设备热插拔
- 转发策略

### 6.3 `system_instrument_service`

负责：

- 仪表扫描
- 在线识别
- 型号探测
- 端口绑定
- 命令收发
- 会话状态管理
- 掉线检测
- 波特率扫描策略

依赖：

- `system_uart_link`
- `system_protocol`
- `system_router`
- `system_fault`

### 6.4 `system_module_service`

负责：

- I2C 模组扫描
- 地址表管理
- 版本读取
- 模组命令事务执行
- 模组状态缓存
- 异常检测

依赖：

- `system_i2c_link`
- `system_protocol`
- `system_fault`

### 6.5 `system_router`

负责：

- 数据在多个消费者之间分发
- UI / log / network / state machine 的路由选择
- 原始数据透传与本地解析结果并存

原则：

- 只解析控制面所需最小字段
- 数据面尽量透传

### 6.6 `system_fault`

负责：

- timeout
- crc error
- nack
- disconnect
- port busy
- bus stuck
- unsupported protocol
- malformed frame

输出目标：

- UI
- 本地日志
- 网络上报

---

## 7. 事件模型约束

建议至少定义以下 ATE 自定义事件：

- `EVT_UART_RX`
- `EVT_UART_TX_DONE`
- `EVT_UART_TIMEOUT`
- `EVT_I2C_XFER_DONE`
- `EVT_I2C_TIMEOUT`
- `EVT_DEVICE_ONLINE`
- `EVT_DEVICE_OFFLINE`
- `EVT_MODULE_DISCOVERED`
- `EVT_PROTOCOL_ERROR`
- `EVT_FORWARD_REQUEST`
- `EVT_FORWARD_DONE`
- `EVT_FAULT`

规则：

- ISR 不直接执行业务逻辑
- ISR 只做最小唤醒 / 通知
- 复杂处理统一下沉到任务上下文或 `esp_event` 事件处理器

---

## 8. 接口风格约束

### 8.1 App 层允许调用的接口风格

```c
system_instrument_scan();
system_instrument_attach(port_id);
system_instrument_send(dev_id, cmd, payload, len);
system_instrument_get_state(dev_id, &state);

system_module_scan();
system_module_exec(module_id, action, arg);
system_module_get_info(module_id, &info);

system_event_subscribe(...);
system_fault_get_last(...);
```

### 8.2 App 层禁止直接调用

```c
uart_write_bytes(...);
uart_read_bytes(...);
uart_set_mode(...);
i2c_new_master_bus(...);
i2c_master_transmit(...);
i2c_master_receive(...);
esp_event_post(...);
esp_event_handler_register(...);
```

---

## 9. 数据处理边界

### 9.1 ESP32-P4 必须本地解析的内容（控制面）

- 帧头 / 帧尾
- 长度字段
- CRC / checksum
- ACK / NACK
- 在线 / 离线状态
- 设备识别结果
- 模组版本号
- 必要状态机字段
- UI 关键展示字段
- 转发时必须带的元数据

### 9.2 ESP32-P4 可透传的内容（数据面）

- 大块日志正文
- 长测量数据流
- 服务器脚本 payload
- 详细诊断报文
- 不参与本地状态机判断的扩展字段

原则：

- ESP32-P4 只做最小必要解析
- 避免把 ESP32-P4 变成完整业务解释器

---

## 10. 状态与上下文设计

所有通信模块必须使用 **context / handle** 设计，禁止大量裸全局变量。

推荐对象：

- `driver_uart_port_ctx_t`
- `system_uart_link_ctx_t`
- `driver_i2c_bus_ctx_t`
- `driver_i2c_dev_ctx_t`
- `system_instrument_ctx_t`
- `system_module_ctx_t`
- `system_protocol_session_t`

规则：

- 上下文由创建函数初始化
- 生命周期由所属 manager 管理
- 业务层不得直接改写 driver context 内部字段

### 10.1 handle 使用判断标准

- 全局唯一系统服务通常不使用 handle
- 多实例设备、链路、会话对象允许使用 `*_handle_t`
- 是否使用 handle，以“是否存在多实例 + 生命周期 + 独立上下文”为判断标准

### 10.2 Driver 层的 handle 使用原则

Driver 层对“多实例 + 生命周期 + 独立上下文”的对象，优先使用 opaque handle 模式。

推荐使用 handle 的对象：

- `driver_uart_port`
- `driver_i2c_master`
- `driver_i2c_device`

不强制使用 handle 的对象：

- 全局唯一的轻量设施
- 无实例状态的纯辅助模块

---

## 11. 前缀命名规范

- Driver 层函数统一使用 `driver_` 前缀
- System 层函数统一使用 `system_` 前缀
- App 层函数统一使用 `app_` 前缀
- Driver/System 对外接口优先返回 `esp_err_t`

示例：

```c
esp_err_t driver_uart_port_create(...);
esp_err_t driver_uart_port_set_baudrate(...);
esp_err_t driver_i2c_master_probe(...);

esp_err_t system_comm_mgr_init(void);
esp_err_t system_instrument_scan(void);
esp_err_t system_module_exec(...);

esp_err_t app_test_flow_start(void);
esp_err_t app_ui_refresh_home(void);
```

---

## 12. Driver 层文件结构生成约束（给 Cursor / Codex）

### 12.1 每个 Driver 模块必须独立为一个 ESP-IDF 组件

每个 Driver 模块独立存放在 `components/` 下的子目录中，结构如下：

```text
components/
├── driver_uart_port/
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   ├── README.md
│   ├── include/
│   │   └── driver_uart_port.h
│   └── src/
│       └── driver_uart_port.c
├── driver_i2c_master/
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   ├── README.md
│   ├── include/
│   │   └── driver_i2c_master.h
│   └── src/
│       └── driver_i2c_master.c
```

规则：

- 每个组件有独立的 `CMakeLists.txt`
- 头文件统一放在 `include/` 目录，由 `idf_component_register` 的 `INCLUDE_DIRS` 自动暴露给依赖方
- 实现文件放在 `src/`
- 不允许把多个 Driver 合并为一个组件

### 12.2 CMakeLists.txt 模板

#### `driver_i2c_master` 的 CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "src/driver_i2c_master.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_i2c
)
```

#### `driver_uart_port` 的 CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "src/driver_uart_port.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_driver_uart
)
```

约束：

- I2C Driver 声明 `REQUIRES esp_driver_i2c`
- UART Driver 声明 `REQUIRES esp_driver_uart`
- 不允许 `REQUIRES driver`（all-in-one 形式）
- 不额外引入无必要组件；其他依赖按需添加

### 12.3 `idf_component.yml` 模板

```yaml
## driver_i2c_master
version: "0.1.0"
description: "I2C master driver wrapper for ATE system. Wraps esp_driver_i2c new API."
dependencies:
  idf: ">=5.5.0"
```

```yaml
## driver_uart_port
version: "0.1.0"
description: "UART port driver wrapper for ATE system. Supports RS422 and RS485 half-duplex."
dependencies:
  idf: ">=5.5.0"
```

### 12.4 头文件设计约束：opaque handle 的适用范围

Driver 层对“多实例 + 生命周期 + 独立上下文”的对象，优先使用 opaque handle 模式，禁止在头文件中暴露内部 struct 字段。

适用对象：

- `driver_i2c_master`
- `driver_i2c_device`
- `driver_uart_port`

不强制对象：

- 全局唯一的轻量设施
- 无实例状态的纯辅助模块

### 12.5 `driver_i2c_master.h` 模板

```c
#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_i2c_master *driver_i2c_master_handle_t;
typedef struct driver_i2c_device *driver_i2c_device_handle_t;

typedef struct {
    int sda_io;
    int scl_io;
    uint32_t clk_speed_hz;
    int timeout_ms;
} driver_i2c_master_config_t;

typedef struct {
    uint16_t dev_addr;
    uint32_t scl_speed_hz;    /* 0 = use bus default */
    int timeout_ms;
} driver_i2c_device_config_t;

esp_err_t driver_i2c_master_create(const driver_i2c_master_config_t *config,
                                   driver_i2c_master_handle_t *out_handle);
esp_err_t driver_i2c_master_delete(driver_i2c_master_handle_t handle);

esp_err_t driver_i2c_master_probe(driver_i2c_master_handle_t handle,
                                  uint16_t dev_addr,
                                  int timeout_ms);
esp_err_t driver_i2c_master_recover(driver_i2c_master_handle_t handle);

esp_err_t driver_i2c_device_add(driver_i2c_master_handle_t bus,
                                const driver_i2c_device_config_t *config,
                                driver_i2c_device_handle_t *out_dev);
esp_err_t driver_i2c_device_remove(driver_i2c_device_handle_t dev);

esp_err_t driver_i2c_write(driver_i2c_device_handle_t dev,
                           const uint8_t *data,
                           size_t len);
esp_err_t driver_i2c_read(driver_i2c_device_handle_t dev,
                          uint8_t *buf,
                          size_t len);
esp_err_t driver_i2c_write_read(driver_i2c_device_handle_t dev,
                                const uint8_t *write_buf,
                                size_t write_len,
                                uint8_t *read_buf,
                                size_t read_len);

#ifdef __cplusplus
}
#endif
```

### 12.6 `driver_uart_port.h` 模板

```c
#pragma once

#include "esp_err.h"
#include "driver/uart.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_uart_port *driver_uart_port_handle_t;

typedef enum {
    DRIVER_UART_MODE_NORMAL,
    DRIVER_UART_MODE_RS485_HALF,
} driver_uart_mode_t;

typedef struct {
    uart_port_t port;
    int tx_io;
    int rx_io;
    int rts_io;          /* Required for RS485 half-duplex auto direction; set -1 for normal UART/RS422 */
    uint32_t baud_rate;
    driver_uart_mode_t mode;
    int rx_buf_size;
    int tx_buf_size;
    int timeout_ms;
} driver_uart_port_config_t;

esp_err_t driver_uart_port_create(const driver_uart_port_config_t *config,
                                  driver_uart_port_handle_t *out_handle);
esp_err_t driver_uart_port_delete(driver_uart_port_handle_t handle);

esp_err_t driver_uart_port_set_baudrate(driver_uart_port_handle_t handle,
                                        uint32_t baud_rate);
esp_err_t driver_uart_port_reconfigure(driver_uart_port_handle_t handle,
                                       const driver_uart_port_config_t *config);
esp_err_t driver_uart_port_recover(driver_uart_port_handle_t handle);

esp_err_t driver_uart_write(driver_uart_port_handle_t handle,
                            const uint8_t *data,
                            size_t len);
esp_err_t driver_uart_read(driver_uart_port_handle_t handle,
                           uint8_t *buf,
                           size_t len,
                           size_t *out_len);
esp_err_t driver_uart_flush(driver_uart_port_handle_t handle);

#ifdef __cplusplus
}
#endif
```

### 12.7 README.md 约束

每个 Driver 组件必须带最小 README，至少说明：

- 模块职责
- 目标平台（ESP-IDF v5.5.3 / ESP32-P4）
- backend 依赖的官方驱动
- 不负责的内容
- 基础使用示例

---

## 13. 扩展原则

新增仪表或模组时，优先新增：

- 设备描述表
- 协议适配器
- service binding
- fault mapping

禁止：

- 复制已有业务流程代码后大面积修改
- 在 App 层新增裸 UART/I2C 调用
- 在 Driver 层加入业务判断

---

## 14. 代码生成硬约束（给 Cursor / Codex）

生成代码时必须满足以下要求：

1. 目标平台固定为 **ESP-IDF v5.5.3**。
2. I2C 只能使用**新驱动** `driver/i2c_master.h`，禁止 legacy `driver/i2c.h`。
3. RS422 与 RS485 必须区分：
   - RS422 → `UART_MODE_UART`
   - RS485 半双工 → `UART_MODE_RS485_HALF_DUPLEX`
4. 使用 RS485 半双工时：
   - 必须通过 `uart_set_mode()` 启用
   - 必须禁用硬件流控
   - 优先复用 UART RTS 自动方向控制
5. 事件系统必须基于 `esp_event`，不得自创底层事件轮子。
6. 所有底层/系统函数返回 `esp_err_t`，领域故障通过 `system_fault` 统一映射。
7. App 层不得直接调用 UART/I2C/`esp_event` 原生接口。
8. Driver 层允许封装“热插拔探测 / probe / recover / 波特率切换 / 端口重建”等链路级能力。
9. 代码优先生成：
   - 清晰的 `.h/.c` 分层
   - 语义化 API
   - `context + handle` 设计
   - 明确错误返回路径
10. 禁止生成“大而全单文件”实现。
11. 禁止为了抽象而把 UART 与 I2C 在 Driver 层做成同构 `read/write/ioctl` 万能接口。
12. Driver 组件优先按独立 ESP-IDF component 生成。

---

## 15. 推荐落地顺序

建议优先实现以下骨架模块：

1. `driver_uart_port`
2. `driver_i2c_master`
3. `system_event`
4. `system_fault`
5. `system_comm_mgr`
6. `system_protocol`
7. `system_instrument_service`
8. `system_module_service`
9. `system_router`

只有这些骨架稳定后，再继续接入：

- UI
- 测试流程状态机
- 网络转发
- 批量设备适配

---

## 16. 一句话总原则

**Driver 层面向外设能力与链路能力，System 层面向通信服务，App 层面向业务语义。**  
**统一发生在 System 层，不发生在裸总线层。**



以下是完整的三部分内容：

---

## 一、给 Codex 的教程级提示词

````markdown
# Task: Rewrite system_router using ESP-IDF esp_event

## Background (read carefully before generating any code)

The existing `system_router.c` implementation is **incorrect** because it:
1. Uses a hand-written linked list + mutex for pub/sub — this is forbidden by project constraints.
2. Calls `handler()` **while holding a mutex**, which will deadlock if any handler tries to subscribe/unsubscribe.
3. Does NOT use ESP-IDF's built-in `esp_event` library.

Your task is to rewrite `system_router` entirely using `esp_event`.

---

## Platform constraints
- ESP-IDF: v5.5.3
- Chip: ESP32-P4
- Language: C (C17)
- FreeRTOS is available

---

## Required ESP-IDF APIs (use ONLY these for the event system)

### Step 1 — Define an event base
In the header file, declare the event base:
```c
// system_router.h
#include "esp_event.h"
ESP_EVENT_DECLARE_BASE(SYSTEM_ROUTER_EVENT);
```

In the source file, define it:
```c
// system_router.c
ESP_EVENT_DEFINE_BASE(SYSTEM_ROUTER_EVENT);
```

The event base is a `const char *` global variable.
Event IDs are `int32_t` values — use `uint32_t route_mask` cast to `int32_t` as the event ID.

---

### Step 2 — Create a dedicated event loop (NOT the default loop)

ATE router events must run on a **dedicated loop**, isolated from system Wi-Fi/IP events:

```c
esp_event_loop_args_t loop_args = {
    .queue_size      = 16,
    .task_name       = "sys_router_task",
    .task_priority   = 5,
    .task_stack_size = 4096,
    .task_core_id    = tskNO_AFFINITY,
};
esp_event_loop_handle_t s_router_loop = NULL;
esp_err_t err = esp_event_loop_create(&loop_args, &s_router_loop);
```

---

### Step 3 — Subscribe (register handler)

```c
esp_event_handler_instance_t instance;
esp_err_t err = esp_event_handler_instance_register_with(
    s_router_loop,           // your dedicated loop
    SYSTEM_ROUTER_EVENT,     // event base
    (int32_t)route_mask,     // event ID = route_mask
    handler_func,            // void (*)(void*, esp_event_base_t, int32_t, void*)
    handler_arg,             // user context
    &instance                // output: used for unregistering
);
```

Store `instance` in the handle returned to the caller.

**Handler signature must be:**
```c
void my_handler(void *handler_arg,
                esp_event_base_t event_base,
                int32_t event_id,
                void *event_data);
// event_data points to a heap copy managed by esp_event — always valid in handler
// event_id == (int32_t)route_mask
```

---

### Step 4 — Unsubscribe (unregister handler)

```c
esp_err_t err = esp_event_handler_instance_unregister_with(
    s_router_loop,
    SYSTEM_ROUTER_EVENT,
    (int32_t)route_mask,
    instance
);
```

---

### Step 5 — Publish (post event)

```c
// esp_event makes a COPY of event_data — caller does not need to keep buffer alive
esp_err_t err = esp_event_post_to(
    s_router_loop,
    SYSTEM_ROUTER_EVENT,
    (int32_t)route_mask,
    payload,             // const void* — copied by esp_event
    payload_size,        // size_t
    pdMS_TO_TICKS(100)   // timeout if queue full
);
```

**Key property**: `esp_event_post_to()` enqueues the event; the handler is invoked asynchronously in the loop's dedicated task — NOT in the caller's task. This means there is NO mutex needed, NO deadlock risk.

---

## Output file structure required

```
components/system_router/
├── CMakeLists.txt
├── idf_component.yml
├── README.md
├── include/
│   └── system_router.h
└── src/
    └── system_router.c
```

### CMakeLists.txt
```cmake
idf_component_register(
    SRCS "src/system_router.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_event
        esp_common
        freertos
)
```

### idf_component.yml
```yaml
version: "0.1.0"
description: "ATE system router: publish/subscribe via ESP-IDF esp_event."
dependencies:
  idf: ">=5.5.0"
```

---

## Required public API (do not change signatures)

```c
// system_router.h

#pragma once
#include "esp_err.h"
#include "esp_event.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(SYSTEM_ROUTER_EVENT);

/* Opaque handle representing one subscription */
typedef struct system_router_subscription_t *system_router_subscription_handle_t;

/* Handler called in the router's dedicated task context */
typedef void (*system_router_handler_t)(void *handler_arg,
                                        uint32_t route_mask,
                                        const void *payload,
                                        size_t payload_size);

esp_err_t system_router_init(void);
esp_err_t system_router_deinit(void);

esp_err_t system_router_subscribe(uint32_t route_mask,
                                  system_router_handler_t handler,
                                  void *handler_arg,
                                  system_router_subscription_handle_t *out_handle);

esp_err_t system_router_unsubscribe(system_router_subscription_handle_t handle);

esp_err_t system_router_route(uint32_t route_mask,
                               const void *payload,
                               size_t payload_size);

#ifdef __cplusplus
}
#endif
```





---

## Hard rules

1. Do NOT use `xSemaphoreCreateMutex` / `xSemaphoreTake` for protecting the subscription list — `esp_event` handles thread safety internally.
2. Do NOT call the handler synchronously inside `system_router_route`. Use `esp_event_post_to`.
3. Do NOT use the default event loop (`esp_event_loop_create_default`). Use a dedicated loop.
4. Do NOT expose `esp_event_loop_handle_t`, `esp_event_handler_instance_t`, or `SYSTEM_ROUTER_EVENT` in the App layer. Those are System layer internals.
5. The `system_router_subscription_t` struct (containing `esp_event_handler_instance_t` + `route_mask`) must be opaque — defined only in `.c`.
6. Return `esp_err_t` from every function. Log errors using `ESP_LOGE` / `ESP_RETURN_ON_ERROR`.
````

---

## 二、数据流文档

```markdown
# system_router 数据流说明

## 1. 组件定位

system_router 属于 System 层，是 ATE 系统内部的事件分发中心。
它基于 ESP-IDF esp_event 构建，对上层屏蔽 esp_event 细节。

## 2. 核心原则

- 发布者（Publisher）：调用 system_router_route()，不关心谁在监听
- 订阅者（Subscriber）：调用 system_router_subscribe()，不关心谁在发布
- 解耦：发布与处理运行在不同 FreeRTOS 任务中，不阻塞发布者

## 3. 事件路由键：route_mask

route_mask 是 uint32_t 位掩码，每个比特代表一类数据通道：

| 比特位 | 含义（示例） |
|--------|-------------|
| bit 0  | UART_RX_RAW |
| bit 1  | I2C_XFER_DONE |
| bit 2  | INSTRUMENT_DATA |
| bit 3  | MODULE_STATE |
| bit 4  | FAULT_EVENT |
| ...    | 按需扩展    |

订阅者声明自己感兴趣的 mask，只有 route_mask 匹配时才被回调。

## 4. 数据流动路径

### 上行路径（Driver → System → App）

1. Driver 层完成收发后，System 层解析帧
2. sys_instrument_service / sys_module_service 调用 system_router_route()
3. esp_event 将 payload 复制入队列
4. router 专属任务从队列取出，调用已注册 handler
5. App 层 handler 收到语义化数据，更新 UI / 状态机

### 下行路径（App → System → Driver）

下行路径不走 router，直接通过 sys_instrument_service / sys_module_service 的
send/exec 接口调用 Driver 层。

## 5. 生命周期

- system_router_init()：创建专属 esp_event loop 和 FreeRTOS 任务
- system_router_subscribe()：注册 handler 到 loop
- system_router_route()：post 事件到 loop 队列
- system_router_unsubscribe()：注销 handler
- system_router_deinit()：删除 loop 和任务
```

---

## 三、Mermaid 代码

```mermaid
flowchart TD
    subgraph DRIVER["Driver Layer"]
        D_UART["drv_uart_port\n(收到原始字节)"]
        D_I2C["drv_i2c_master\n(完成 I2C 事务)"]
    end

    举例如上，剩下的自己补全



    