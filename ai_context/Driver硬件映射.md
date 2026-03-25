以下是整理后的 `Driver硬件映射.md`，按你要求的风格：规则 → 映射表 → 禁止项 → 示例，同时对 AI/Codex 给出硬性约束。

---

# Driver 硬件映射

## 1. 文档目的

本文档明确以下四件事：

- 当前硬件器件应由哪个 Driver 负责抽象
- Board / BSP 与 Driver 的职责边界
- Driver 对上层暴露的接口形式
- 禁止 AI / 开发者在 Driver 层混入板级信息

**本文档既给人看，也给 AI / Codex 看。生成或修改 Driver 代码时，必须优先遵守本文档约束。**

---

## 2. 核心边界

### 2.1 Board / BSP 层负责

Board / BSP 层负责"硬件怎么连接"：

- GPIO 编号
- pinmux 配置
- reset 引脚
- power enable 引脚
- 中断引脚连接
- 外设挂载到哪条总线
- bus 实例选择（如 I2C0 / SPI2 / UART1）

> **一句话：BSP 负责物理连接与板级差异。**

### 2.2 Driver 层负责

Driver 层负责"硬件能力怎么使用"：

- 设备初始化流程
- 协议读写
- 设备控制
- 状态查询
- 错误处理
- 对上层提供统一 API

> **一句话：Driver 负责外设能力抽象，不负责板级连线定义。**

---

## 3. Driver 设计约束

### 3.1 头文件优先暴露 handle 和函数声明

Driver 层 `.h` 文件优先暴露：

- `xxx_handle_t`（opaque 指针类型）
- 枚举 / 常量
- 对外 API 函数声明
- 仅在确实需要多实例或运行时传参时，才暴露 `xxx_config_t`

ATE 当前很多固定板载设备已经采用"Board/BSP 宏 + Driver 直接消费"的方式，例如 `driver_eth_create(driver_eth_handle_t *out_handle)`、`driver_ledstrip_create(driver_ledstrip_handle_t *out_handle)`。这类 Driver 不需要为了形式统一而强行引入 `xxx_config_t`。

**禁止在 `.h` 中暴露内部结构体完整定义。**

这与 ESP-IDF 官方的 handle 抽象风格一致，例如 `esp_lcd_panel_handle_t`、`spi_device_handle_t` 均采用这种"对上层暴露句柄、隐藏底层实现"的接口思路。[[OOP in C](https://developer.espressif.com/blog/2025/10/oop_with_c/#handles-in-esp-idf)]

推荐头文件形式（固定板载设备）：

```c
// driver_touch.h

typedef struct driver_touch_t *driver_touch_handle_t;

esp_err_t driver_touch_create(driver_touch_handle_t *out_handle);
esp_err_t driver_touch_destroy(driver_touch_handle_t handle);
esp_err_t driver_touch_read(driver_touch_handle_t handle,
                            int *out_x, int *out_y, bool *out_pressed);
```

若该 Driver 需要支持多实例、不同端口切换或运行时传参，再使用 `xxx_config_t`。ATE 当前的 `driver_uart_instrument` 不属于这种场景，应直接消费 Board 默认 UART 资源。

内部结构体定义必须放在 `.c` 文件中：

```c
// driver_touch.c（仅此文件可见）

struct driver_touch_t {
    i2c_master_bus_handle_t bus;
    uint8_t                 addr;
    gpio_num_t              rst_gpio;
    gpio_num_t              int_gpio;
    int                     last_x;
    int                     last_y;
};
```

### 3.2 若使用配置结构体，必须完整初始化

并不是所有 Driver 都需要 `xxx_config_t`；但一旦使用，配置结构体的所有字段都必须被显式初始化，推荐使用 C99 指定初始化器；若组件提供默认宏，优先使用默认宏再覆盖特定字段。未初始化字段会导致未定义行为。[[API 约定](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32p4/api-reference/api-conventions.html)]

```c
// 正确：指定初始化器，所有字段明确赋值
const driver_uart_bridge_config_t cfg = {
    .port = BOARD_UART_INSTR_PORT,
    .tx_io = BOARD_UART_INSTR_TX,
    .rx_io = BOARD_UART_INSTR_RX,
    .rts_io = BOARD_UART_INSTR_RTS,
    .baud_rate = BOARD_UART_INSTR_BAUD_DEFAULT,
    .rx_buf_size = BOARD_UART_INSTR_RX_BUF_SIZE,
    .tx_buf_size = BOARD_UART_INSTR_TX_BUF_SIZE,
    .timeout_ms = BOARD_UART_INSTR_TIMEOUT_MS,
};
```

---

## 4. 明确禁止事项

**Driver 层禁止直接写入以下板级信息：**

| 禁止内容 | 示例（不允许出现） |
|---|---|
| GPIO 编号 | `GPIO_NUM_21`、`GPIO_NUM_22` |
| 固定 pin 配置 | `.sda_io_num = 21` |
| 固定 reset / power 引脚 | `gpio_set_level(GPIO_NUM_5, 0)` |
| 固定 bus 号 | `I2C_NUM_0`、`SPI2_HOST` |
| 固定中断引脚 | `gpio_isr_handler_add(GPIO_NUM_4, ...)` |
| 任何板卡绑定的硬编码连接关系 | 一切与具体 PCB 版本耦合的常量 |

> 除非该 Driver 明确标注为"Board 专用 Driver"，否则一律不允许出现上述内容。

这里禁止的是 Driver 自己写死裸常量或私自定义板级连接关系；**不禁止** Driver 通过 `board_ate_p4.h` 这类 Board/BSP 头文件统一暴露的 `BOARD_*` 宏读取板级事实。ATE 当前固定板载设备优先采用这种方式。

---

## 5. Driver 初始化输入来源

Driver 初始化所需的资源，**必须来自 Board / BSP 的统一定义**，包括但不限于：

- `BOARD_*` 宏（当前 ATE 固定板载设备的主要输入方式）
- Board 默认外设实例和初始化支撑
- bus handle（`i2c_master_bus_handle_t` / `spi_device_handle_t` 等）
- 设备地址、port / channel 号、reset / int / power GPIO
- 对于通用多实例 Driver，再通过 `xxx_config_t` 由上层传入

**Driver 只消费这些板级事实，不在 Driver 内重新发明另一套板级常量。**

---

## 6. 硬件器件 → Driver 映射表

| 功能域 | 硬件器件 / 接口 | 板级资源提供方 | Driver |
|---|---|---|---|
| 显示 | LCD / MIPI-DSI Panel | `board_display` | `driver_display` |
| 触摸 | GT911 | `board_i2c` / `board_touch` | `driver_touch_gt911` |
| 以太网 | IP101 / RMII | `board_eth` | `driver_eth` |
| 灯条 | WS2812 / RMT LED | `board_led` | `driver_ledstrip` |
| 音频功放 / 播报 | AMP / Codec / I2S | `board_audio` | `driver_audio` |
| NFC | NFC Reader | `board_i2c` / `board_nfc` | `driver_nfc` |
| 模组 I2C 总线 | NCA9545 后级各槽位模组 | `board_module_bus` | `driver_module_bus` |
| Console 串口 | RS422 / RS485 通道 | `board_console_uart` | `driver_console_uart` |
| USB Host 外设 | AI Camera / U Disk | `board_usb` | `driver_usb_host` |
| 风扇控制 | PWM Fan | `board_fan` | `driver_fan` |
| 存储 | SD / eMMC | `board_storage` | `driver_storage` |

> 本表为当前已知器件的归属声明，不代表组件清单已冻结。新增器件应在此表登记后再实现 Driver。

---

## 7. 正确与错误示例

### 7.1 错误写法

```c
// ❌ Driver 直接绑定 GPIO，与具体板卡耦合
esp_err_t driver_touch_init(void)
{
    i2c_master_bus_config_t cfg = {
        .sda_io_num = GPIO_NUM_21,   // 禁止：板级 GPIO 写死在 Driver
        .scl_io_num = GPIO_NUM_22,   // 禁止：同上
    };
    // ...
}
```

问题：
- Driver 与具体板卡绑定，更换 PCB 版本后必须改 Driver 代码
- 违反 Driver 层"只描述能力、不定义连接"的职责边界

### 7.2 正确写法

```c
// ✅ Driver 直接消费 Board/BSP 已定义好的宏，不写死裸板级常量
esp_err_t driver_touch_create(driver_touch_handle_t *out_handle)
{
    if (!out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    struct driver_touch_t *dev = calloc(1, sizeof(*dev));
    if (!dev) return ESP_ERR_NO_MEM;

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = BOARD_TOUCH_I2C_PORT,
        .scl_io_num = BOARD_TOUCH_I2C_SCL,
        .sda_io_num = BOARD_TOUCH_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = BOARD_TOUCH_USE_INTERNAL_PULLUP,
    };

    dev->addr = BOARD_TOUCH_I2C_ADDR;
    dev->rst_gpio = BOARD_GPIO_TOUCH_RESET;
    dev->int_gpio = BOARD_GPIO_TOUCH_INT;

    // 执行设备初始化流程（所有板级事实都来自 Board/BSP 宏）
    // i2c_new_master_bus(&bus_cfg, &dev->bus);
    // gpio_set_level(dev->rst_gpio, 0);
    // gpio_set_level(dev->rst_gpio, 1);
    // ...

    *out_handle = dev;
    return ESP_OK;
}
```

同时补上对应的销毁函数，保证生命周期完整：

```c
esp_err_t driver_touch_destroy(driver_touch_handle_t handle)
{
    if (!handle) return ESP_ERR_INVALID_ARG;

    // 释放设备资源（从 bus 注销、关闭中断等）
    // ...

    free(handle);
    return ESP_OK;
}
```

这两个函数共同构成一对完整的 `create / destroy` 生命周期，与 ESP-IDF 官方的 handle 风格（如 `i2c_new_master_bus` / `i2c_del_master_bus`）保持一致。[[OOP in C](https://developer.espressif.com/blog/2025/10/oop_with_c/#examples-of-oop-in-esp-idf)]




