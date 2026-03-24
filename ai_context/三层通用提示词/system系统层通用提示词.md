
✅ System 层通用 Prompt

很好，这一块我直接按你 App + Driver 的提示词风格统一，给你一份可直接复用的 System 层提示词模板（重点是约束 AI/Codex）。

不会写废话，直接工程可用版本👇

⸻

任务目标

根据给定的业务需求与已有 Driver 能力，抽象出 System Layer（系统服务层）设计。

System 层只负责：

能力组合 + 资源管理 + 状态收敛


⸻

架构约束（必须遵守）
	1.	本次设计只针对 System Layer
	2.	System 不允许包含：
	•	UI 逻辑
	•	测试流程（PASS / FAIL / RUNNING）
	•	页面跳转
	3.	System 不负责：
	•	直接访问硬件（必须通过 Driver）
	•	业务策略与业务判定
	4.	System 只表达：

“系统能提供什么服务”


⸻

命名规范（必须遵守）

system_xxx_xxx

要求：
	•	必须体现“服务能力”
	•	禁止使用：
	•	manager（慎用）
	•	business
	•	ui
	•	示例：

system_module_service
system_instrument_service
system_network
system_storage
system_usb
system_event
system_router


⸻

输出结构（必须按以下格式）

⸻

1. 模块命名

system_xxx_xxx

中文名：

系统服务（描述系统能力）


⸻

2. 模块职责（一句话）

该模块负责对多个驱动能力进行组合与管理，对外提供稳定的系统服务接口。


⸻

3. 输入（Input）

要求：
	•	必须是“系统级输入”
	•	来源只能是：

Driver输出
配置数据
上层调用

格式：

输入：
    driver_data
    config
    control_command


⸻

4. 输出（Output）

要求：
	•	必须是“系统语义输出”
	•	不能直接是硬件数据

格式：

输出：
    system_state
    system_event
    service_result


⸻

5. 黑盒视角（Blackbox）

Driver能力
    ↓
system_xxx_xxx
    ↓
系统服务能力


⸻

6. 白盒结构（Whitebox）

要求：
	•	体现“组合 + 管理”
	•	不出现 App

格式：

system_xxx_xxx
    ├── resource_manager     ← 资源管理
    ├── state_manager        ← 状态收敛
    ├── service_adapter      ← 对外接口
    ├── driver_adapter       ← Driver封装


⸻

7. 接口设计约束（必须包含）

- 对外接口使用语义化命名（如 start / stop / get_state）
- 不暴露 driver handle
- 不透出底层协议细节
- 生命周期清晰（init / start / stop）


架构约束补充：ESP-IDF System 层常用组件声明
当前模板未说明 System 层允许使用哪些 ESP-IDF 组件，建议在架构约束中补充：

System 层可以使用以下 ESP-IDF 组件进行能力组合：

esp_event：事件循环，用于系统事件发布与订阅
esp_timer：高精度定时器，用于周期任务
FreeRTOS 原语（xTaskCreate、xQueueCreate、xSemaphoreCreateMutex）：用于任务与并发管理
nvs_flash：非易失性存储，用于系统配置持久化
esp_log.h：日志输出


禁止事项补充：禁止使用 ESP_ERROR_CHECK 做异常中止
System 层面向稳定服务，不应使用 ESP_ERROR_CHECK（该宏在错误时直接 abort()，只适合示例代码或初始化阶段的顶层），应显式处理并向上传播错误。[错误处理宏说明]

补充到禁止事项：

- 禁止在 System 服务接口中使用 ESP_ERROR_CHECK（会导致 abort，不可恢复）
- 应显式判断 esp_err_t 并向调用方返回错误码

⸻

8. 禁止事项（必须列出）

- 禁止直接调用 GPIO / I2C / UART 等硬件接口
- 禁止出现 UI / 页面逻辑
- 禁止写业务流程（测试流程）
- 禁止跨层调用 App


⸻

推荐接口风格（必须遵守）

esp_err_t system_xxx_init(void);
esp_err_t system_xxx_start(void);
esp_err_t system_xxx_stop(void);

xxx_state_t system_xxx_get_state(void);


⸻

核心原则（必须遵守）

Driver = 设备能力
System = 能力组合
App = 业务语义


⸻

判断标准（AI必须自检）

如果一个函数在回答：

“如何访问硬件？” → ❌（属于Driver）
“什么时候执行？” → ❌（属于App）
“系统提供什么能力？” → ✔（属于System）


⸻

一句话总结（必须输出）

System层负责组合Driver能力并对外提供稳定服务，不包含业务语义与硬件细节。

:::

⸻

🔥 这一层我帮你额外强调一句（非常关键）

你现在三层关系已经可以定死了：

App：决定“做什么”
System：提供“能做什么”
Driver：实现“怎么做”


⸻



