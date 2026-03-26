# `driver_i2c_module` 事实提示词

## 目的

这份文档不是重新设计 `driver_i2c_module`，而是把**当前工程中已经成立的事实**整理出来，方便直接发给 ChatGPT 讨论：

- 今天要加协议时，Driver 层该保持什么边界
- 白盒测试应该怎么做
- 是否需要补一些 `get` 接口
- 和 Tiny MCU 联调时，哪些事实不能改成想当然

---

## 模块名

`driver_i2c_module`

中文名：

热插拔模组 I2C 字节通信驱动

---

## 当前模块本质

`driver_i2c_module` 是一个**固定板级资源、固定从机地址、固定通信速率**的 I2C Master Driver。

它当前只负责：

- 创建 / 复用 I2C Master bus
- 将固定地址的从机加入总线
- 提供字节级 `write / read / write_read / probe`

它**不负责**：

- 协议封包 / 解包
- 业务状态
- PASS / FAIL 判断
- 设备信息解析
- 重试策略

一句话：

> `driver_i2c_module = 基于 ESP-IDF 新版 i2c_master API 的固定板级、固定地址的字节通信驱动`

---

## 当前已经成立的代码事实

### 1. 使用的是 ESP-IDF 新版 I2C Master API

当前实现直接基于：

- `driver/i2c_master.h`
- `i2c_new_master_bus`
- `i2c_master_get_bus_handle`
- `i2c_master_bus_add_device`
- `i2c_master_transmit`
- `i2c_master_receive`
- `i2c_master_transmit_receive`
- `i2c_master_probe`

旧版 `driver/i2c.h` 没有使用。

### 2. bus 是静态单例

当前实现里：

- 文件内静态保存 `s_bus_handle`
- `driver_i2c_module_bus_create()` 先尝试 `i2c_master_get_bus_handle()`
- 若端口未初始化才新建总线

也就是说：

> 当前设计默认整个工程只复用一条固定的模组 I2C bus

### 3. 地址、端口、速率都来自 Board

运行时**不传地址和速率**，而是直接吃板级宏：

- `BOARD_I2C_MASTER_NUM`
- `BOARD_I2C_MASTER_SCL`
- `BOARD_I2C_MASTER_SDA`
- `BOARD_I2C_MASTER_CLK_HZ`
- `BOARD_MODULE_LINK_I2C_ADDR`
- `BOARD_I2C_MASTER_TIMEOUT_MS`

### 4. 当前地址是 7 位地址

当前板级地址是：

- `BOARD_MODULE_LINK_I2C_ADDR`
- 现阶段配置为 `0x11`

这个是 **7 位从机地址**。

因此逻辑分析仪上如果看的是**地址字节**，写方向通常会看到：

```text
0x22
```

因为：

```text
(0x11 << 1) | 0 = 0x22
```

这不是地址错，而是 I2C 地址字节显示方式不同。

### 5. 当前 Driver 默认只对应一个固定 device

虽然内部仍然有 `driver_i2c_module_create()` / `destroy()`，但当前实际上：

- 不支持运行时切换地址
- 不支持多实例不同地址
- 不支持同 bus 上挂多个不同测试模组

当前工程语义已经收敛成：

> 一个固定的模组地址加入固定 bus

### 6. `bus_destroy()` 当前只是清空静态缓存

当前 `driver_i2c_module_bus_destroy()` 不真正 `i2c_del_master_bus()`，只清空静态指针。

这表示当前设计是：

- bus 生命周期按“全局固定资源”看待
- 不准备做完整的 bus 释放 / 重建管理

### 7. 当前已带有 Driver 级测试 helper

当前已经在 Driver 组件里加入测试辅助文件：

- `driver_i2c_module_test.h`
- `driver_i2c_module_test.c`

已有测试入口：

- `driver_i2c_module_test_bus_signal_run()`
- `driver_i2c_module_test_write_default_pattern()`

用途：

- 用逻辑分析仪确认地址帧有没有发出去
- 用固定字节 `A5 5A 00 FF` 确认数据阶段有没有发出去

对于“逻辑分析仪只看信号”的测试场景：

- `ACK` 算通过
- `NACK` 也算通过
- `TIMEOUT` 才更像总线没起来或上拉有问题

---

## 当前公开接口事实

```c
typedef struct driver_i2c_module_t *driver_i2c_module_handle_t;

esp_err_t driver_i2c_module_bus_create(void);
esp_err_t driver_i2c_module_bus_destroy(void);

esp_err_t driver_i2c_module_create(driver_i2c_module_handle_t *out_handle);
esp_err_t driver_i2c_module_destroy(driver_i2c_module_handle_t handle);

esp_err_t driver_i2c_module_write(driver_i2c_module_handle_t handle,
                                  const uint8_t *data,
                                  size_t len);
esp_err_t driver_i2c_module_read(driver_i2c_module_handle_t handle,
                                 uint8_t *out_data,
                                 size_t len);
esp_err_t driver_i2c_module_write_read(driver_i2c_module_handle_t handle,
                                       const uint8_t *write_data,
                                       size_t write_len,
                                       uint8_t *read_data,
                                       size_t read_len);

esp_err_t driver_i2c_module_probe(driver_i2c_module_handle_t handle);
```

---

## 当前白盒边界

```text
board_ate_p4.h
    ↓
driver_i2c_module
    ├── bus_create / bus singleton
    ├── create / destroy fixed device handle
    ├── write
    ├── read
    ├── write_read
    └── probe
```

不允许跨到：

- 协议层
- 状态机
- 业务层

---

## 已知缺口 / 讨论重点

如果今天要加协议、做白盒测试、和 Tiny MCU 联调，当前值得讨论的点主要是这些：

### 1. 是否需要补 `get` 接口

当前 Driver 没有对外暴露任何事实查询接口。

如果为了调试 / 白盒测试 / 联调日志，可能会考虑补以下只读接口：

- `get_bus_port`
- `get_scl_gpio`
- `get_sda_gpio`
- `get_device_addr`
- `get_clk_hz`
- `get_timeout_ms`

但要注意：

- 这些接口只能暴露**硬件事实**
- 不能把 Driver 变成状态管理器
- 不应该为了测试把内部 `i2c_master_* handle` 直接暴露出去

### 2. 是否还需要保留 `create/destroy`

由于当前地址已经固定，`create/destroy` 的存在有一点历史遗留味道。

可讨论两条路线：

- 保持现状，不动接口，最稳
- 继续收平成更固定的板级设备驱动

### 3. 白盒测试应该停留在哪一层

当前建议：

- Driver 白盒测试只测 bus/create/probe/write/read/write_read 的硬件事实
- 协议正确性放到 `system_module`
- 联调时协议字段语义不要回灌到 Driver

### 4. 和 Tiny MCU 联调时的稳定前提

当前已经固定的联调前提：

- I2C 7 位地址：`0x11`
- 逻辑分析仪写地址字节：`0x22`
- 默认时钟：`400kHz`
- 端口：`I2C0`
- GPIO：`SDA=7`, `SCL=8`

Tiny MCU 侧需要按这个事实对齐，不能把地址当 `0x22` 配进去。

---

## 可直接发给 ChatGPT 的提示词

```text
我在做一个 ESP-IDF 工程里的 driver_i2c_module，下面是当前已经成立的事实，请不要按理想化方式重写，而是在这些事实基础上讨论：

1. 这是 Driver 层，不允许包含协议解析、状态机、业务判断、PASS/FAIL、重试策略。
2. 它基于 ESP-IDF 新版 driver/i2c_master.h。
3. 当前 bus 是静态单例，driver_i2c_module_bus_create() 先用 i2c_master_get_bus_handle() 复用，再决定是否 i2c_new_master_bus()。
4. 当前地址、端口、速率都来自 board_ate_p4.h，不通过运行时传参：
   - I2C port = 0
   - SDA = GPIO7
   - SCL = GPIO8
   - speed = 400kHz
   - 7-bit device addr = 0x11
5. 逻辑分析仪如果看到写地址字节 0x22 是正常的，因为 (0x11 << 1) | 0 = 0x22。
6. 当前公开接口只有：
   - bus_create / bus_destroy
   - create / destroy
   - write / read / write_read / probe
7. 当前 bus_destroy 不真正删除 bus，只清空静态缓存。
8. 当前已经有 Driver 级测试 helper：
   - driver_i2c_module_test_bus_signal_run()
   - driver_i2c_module_test_write_default_pattern()
   主要服务逻辑分析仪抓波形。

我现在想讨论的不是“重写一个新的 I2C driver”，而是：

- 在这些既有事实下，是否需要增加一些 get 接口来服务白盒测试和联调？
- 如果要加，哪些 get 接口是合理的，哪些会破坏 Driver 边界？
- 今天准备加协议、做基础白盒测试、再和 Tiny MCU 联调，这个 Driver 层应该稳定在什么边界上？

请按“已成立事实 / 建议保留 / 建议新增 / 明确不要做”的结构回答。
```

