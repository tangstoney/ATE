## Driver 驱动层进度盘点提示词

---

### 任务目标

基于当前 ATE 仓库实际代码，盘点 Driver Layer 已有模块、模块名称、实现进度、边界是否清晰，以及哪些模块适合直接用于 `boardmix` 画图。

本提示词不是让 AI 设计新 Driver，而是让 AI 识别“当前已经存在的 Driver 现状”。

---

### 使用约束（必须遵守）

1. 必须严格基于当前仓库现状回答
2. 不允许把规划项、README 草案、注释中的设想当成“已实现模块”
3. 不允许臆造不存在的组件目录、接口或器件
4. 盘点对象仅限 Driver Layer，不输出 App / System 作为结果主体
5. 如果模块只有目录、接口、桩实现或半成品，必须明确标注
6. 如果模块本质上不适合单独作为 Driver 画图，必须明确指出

---

### 仓库背景

- 框架：ESP-IDF
- 板级头文件：`components/board/include/board_ate_p4.h`
- Driver 根目录：`components/driver`
- 目标：统计当前已经存在的 Driver 模块、模块名称、职责、完成度、是否符合“Driver = 设备能力抽象”的边界，方便我在 `boardmix` 画三层架构图

---

### 当前仓库已发现的 Driver 组件目录

- `driver_display`
- `driver_eth`
- `driver_i2c_master`
- `driver_input`
- `driver_ledstrip`
- `driver_network`
- `driver_uart_port`
- `driver_usb`

补充说明：

- `driver_usb` 当前对外暴露的子能力包括：
  - `driver_usb_host`
  - `driver_usb_device`
- `driver_display` 当前覆盖的器件能力包括：
  - LCD panel
  - Touch panel（GT911）

---

### 输出要求（必须按以下顺序输出）

#### 1. Driver 总体进度总结

要求：

- 从两个维度总结：
  - 维度 A：仓库里“已经有代码/组件目录”的 Driver 进度
  - 维度 B：相对于“Driver 层设计模板（Kconfig 宏版）”的规范达标进度
- 用一句话说明当前 Driver 层处于“已起步 / 初步成型 / 基本完成 / 待统一重构”中的哪一档，并解释原因

---

#### 2. 已有 Driver 模块清单

请输出表格，列为：

- 模块名
- 中文名
- 类型（设备驱动 / 总线驱动 / 包装层 / 桩实现）
- 当前状态（已落地 / 半成品 / 桩 / 待整改）
- 主要能力
- 主要依赖的 ESP-IDF / 官方组件
- 是否适合直接画进 Driver 架构图（是 / 否 / 需要标注“规划中”）

---

#### 3. 建议用于 boardmix 画图的 Driver 层模块名

要求：

- 只列“建议画进图里的名字”
- 分成三组：
  - 已实现
  - 已有目录但不建议当独立 Driver 画
  - 后续建议拆分 / 重命名
- 如果某模块内部混合了多个器件能力，要明确指出

---

#### 4. 特别指出当前不符合规范的点

至少检查以下问题：

- 是否没有组件级 Kconfig
- 是否仍然暴露 `xxx_config_t`
- 是否把 BSP / GPIO 初始化放进 Driver
- 是否跨 Driver 调用，导致边界不清
- 是否只有接口没有实现
- 是否把 USB / Network 这种系统能力误放在 Driver

---

#### 5. 最终给我一个“适合画图”的精简版模块列表

格式：

```md
Driver Layer
- xxx
- xxx
- xxx
```

---

### 最终输出约束

- 不要输出 App / System 模块
- 不要臆造不存在的目录
- 不要把 README 里的规划项当成已实现
- 如果某模块只是桩实现，要明确写“桩”
- 如果某模块本质上不应单独作为 Driver 画图，要明确写“建议并入 xxx”或“建议改名为 xxx”
