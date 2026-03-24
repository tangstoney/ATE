## ✅ 完善版 Prompt（ESP-IDF Driver 层设计模板 · Kconfig 宏定义版）

---

### 任务目标

根据给定的硬件器件或外设能力，基于 **ESP-IDF 框架**，抽象出 Driver Layer（设备驱动层）设计。

Driver 层只负责：设备能力抽象（读 / 写 / 控制 / 状态）

---

### 架构约束（必须遵守）

1. 本次设计只针对 Driver Layer
2. Driver **必须直接使用 ESP-IDF 组件或乐鑫官方组件**，例如：
   - `driver/i2c_master.h`、`driver/spi_master.h`、`driver/uart.h` 等外设驱动
   - `esp_err.h` 错误码体系
   - FreeRTOS 原语（`SemaphoreHandle_t`、`QueueHandle_t`）用于并发控制
   - `esp_log.h` 用于日志输出
3. **编译期参数全部通过 Kconfig 宏 board_ate_p4.h 的（`BOARD_XXX_XXX`）提供，禁止使用运行时 config 结构体**
4. Driver **不允许**包含：
   - 业务语义（测试 / UI / OTA 等）
   - 状态机流程（Running / PASS / FAIL）
   - 策略逻辑
5. Driver **不负责**：
   - 板级连接（GPIO / pin / bus 号）——这属于 BSP 层
   - 复杂业务协议解析
6. Driver 只表达：**"设备能做什么"**

---

### 命名规范（必须遵守）

```
driver_xxx_xxx
```

要求：
- 必须体现设备或能力
- 禁止使用：`manager`、`service`、`business`
- 示例：`driver_uart_port`、`driver_i2c_sensor`、`driver_eth`、`driver_touch_gt911`

---

### 输出结构（必须按以下格式）

---

#### 1. 模块命名

```
driver_xxx_xxx
```

中文名：设备驱动（描述设备能力）

---

#### 2. 模块职责（一句话）

该模块基于 **ESP-IDF** 对 xxx 设备进行抽象，提供初始化、控制、读写与状态查询能力；编译期参数由 Kconfig 宏固定。

---

#### 3. 输入（Input）

要求：
- 必须是"驱动级输入"
- **禁止传入 config 结构体**
- 编译期参数通过 Kconfig 宏 `CONFIG_DRIVER_XXX_XXX` 提供，由 `sdkconfig.h` 自动生成
- 运行时只允许传入：设备句柄、读写参数

格式：
```
输入：
    handle（driver_xxx_handle_t）
    read/write 参数

（编译期参数由 Kconfig 宏 CONFIG_DRIVER_XXX_XXX 提供，不通过接口传入）
```

ESP-IDF Kconfig 系统会将 `Kconfig` 文件中定义的选项自动生成到 `sdkconfig.h`，以 `#define CONFIG_XXX` 形式供 C 代码使用。 [[配置用法](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/kconfig/project-configuration-guide.html#how-to-use-configuration-variables-in-c-code-and-cmake)]

---

#### 4. 输出（Output）

要求：
- 必须是"设备事实"
- 禁止业务语义
- **错误码必须使用 `esp_err_t`**，成功返回 `ESP_OK`

格式：
```
输出：
    esp_err_t（ESP_OK / ESP_ERR_INVALID_ARG / ESP_ERR_NO_MEM / ESP_FAIL）
    read_data（out 参数指针）
    device_state
```

---

#### 5. 黑盒视角（Blackbox）

```
输入（handle / 操作参数）
    ↓
driver_xxx_xxx（基于 ESP-IDF 组件封装，编译期由 Kconfig 宏配置）
    ↓
输出（esp_err_t + out 参数）
```

---

#### 6. 白盒结构（Whitebox）

要求：
- 只体现驱动内部结构
- **必须体现 ESP-IDF HAL/Driver 调用链**
- 不出现 App / System

格式：
```
driver_xxx_xxx
    ├── device_context        ← 内部状态（opaque struct）
    ├── kconfig_params        ← 编译期宏 CONFIG_DRIVER_XXX_XXX（来自 sdkconfig.h）
    ├── idf_interface         ← 直接调用 ESP-IDF driver/xxx.h 接口
    │     ├── i2c_master_transmit / spi_device_transmit / ...
    │     └── FreeRTOS 原语（mutex / semaphore）
    ├── device_control        ← 控制逻辑（封装 IDF 调用）
    └── device_io             ← 数据读写（封装 IDF 调用）
```

---

#### 7. 接口设计约束（必须包含）

- 使用 **opaque handle**（`driver_xxx_handle_t`），内部为指向结构体的指针
- **不暴露内部结构体**（私有实现放 `.c` 文件）
- **使用 `esp_err_t` 作为返回值**，out 参数用指针传出
- `create` / `destroy` 生命周期必须成对
- **`create` 不接收任何 config 参数**，所有编译期参数通过 Kconfig 宏读取
- **禁止提供 `DEFAULT_CONFIG` 宏**（本阶段无 config 结构体）

---

#### 8. 禁止事项（必须列出）

- 禁止写死 GPIO / pin / bus 号
- 禁止包含 BOARD 宏以外的硬件常量
- 禁止出现业务逻辑（如测试、UI）
- 禁止跨层调用 App / System
- **禁止定义或使用 `driver_xxx_config_t` 结构体**（本阶段统一用 Kconfig 宏）
- **禁止直接调用 HAL/LL 层**（`hal/xxx_hal.h`、`hal/xxx_ll.h`）
- **禁止使用已弃用的旧版 IDF 驱动**（如 `driver/i2c.h` 旧接口）

---

### 推荐接口风格（必须遵守）

```c
/* 不透明句柄，内部为 IDF 资源的封装 */
typedef struct driver_xxx_t *driver_xxx_handle_t;

/* 生命周期（无 config 参数，编译期由 Kconfig 宏固定） */
esp_err_t driver_xxx_create(driver_xxx_handle_t *out_handle);
esp_err_t driver_xxx_destroy(driver_xxx_handle_t handle);

/* 能力接口（返回 esp_err_t，out 参数用指针） */
esp_err_t driver_xxx_read(driver_xxx_handle_t handle,
                          uint8_t *out_data, size_t len);
esp_err_t driver_xxx_write(driver_xxx_handle_t handle,
                           const uint8_t *data, size_t len);
esp_err_t driver_xxx_control(driver_xxx_handle_t handle,
                             uint32_t cmd, void *arg);
```

`.c` 内部实现中使用 Kconfig 宏：

```c
#include "sdkconfig.h"

esp_err_t driver_xxx_create(driver_xxx_handle_t *out_handle)
{
    int param_a = CONFIG_DRIVER_XXX_PARAM_A;  // 来自 Kconfig
    int param_b = CONFIG_DRIVER_XXX_PARAM_B;  // 来自 Kconfig
    // ... 初始化逻辑
    return ESP_OK;
}
```

---

### Kconfig 文件（必须提供）

AI 生成 Driver 时，必须同步输出对应的 `Kconfig` 文件，放于组件根目录：

```kconfig
menu "Driver XXX Configuration"

    config DRIVER_XXX_PARAM_A
        int "Param A value"
        default 100
        help
            Configure param A for driver_xxx.

    config DRIVER_XXX_PARAM_B
        int "Param B value"
        default 200
        help
            Configure param B for driver_xxx.

endmenu
```

- Kconfig 文件命名必须为 `Kconfig` 或 `Kconfig.projbuild`，不可自定义文件名 [[Kconfig 文件](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/kconfig/configuration_structure.html#kconfig-files)]
- `Kconfig`：选项显示在 menuconfig 的 `Component configuration` 下
- `Kconfig.projbuild`：选项显示在 menuconfig 顶层菜单 [[文件差异](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/kconfig/component-configuration-guide.html#how-to-define-new-configuration-options-for-your-component)]

---

### ESP-IDF 组件依赖（AI 必须在 CMakeLists.txt 中声明）

```cmake
idf_component_register(
    SRCS "driver_xxx.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c   # 或 esp_driver_spi / esp_driver_uart 等
)
```

---

### 核心原则（必须遵守）

```
Driver = ESP-IDF 能力封装（编译期参数由 Kconfig 宏提供）
System = 能力组合
App    = 业务语义
BSP    = 板级绑定（GPIO/pin/bus）
```

