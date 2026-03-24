
✅ 通用 Prompt（App 层业务抽象模板）


任务目标

根据给定的软件功能描述，抽象出 Application Layer（业务层）架构设计，仅关注业务语义，不涉及驱动实现、System实现或底层协议细节。

⸻

架构约束（必须遵守）
	1.	本次设计只针对 Application Layer
	2.	不允许出现：
	•	GPIO / I2C / UART / USB API
	•	ESP-IDF / HAL / 驱动调用
	•	协议解析细节（帧结构、CRC等）
	3.	默认：
	•	Driver 层已实现
	•	System 层已提供标准接口
	4.	Application 只表达：
“业务意图 / 状态 / 行为”

⸻

输出要求（必须按结构输出）

对每一个业务模块，必须包含以下内容：

⸻

1. 模块命名
	•	格式：

app_xxx_xxx

	•	要求：
	•	使用业务语义命名
	•	禁止包含 driver / protocol / hardware
	•	命名体现“能力”，而不是实现

同时提供：
	•	中文名

⸻

2. 模块职责（简述）

用一句话说明：

该模块负责什么业务能力


⸻

3. 输入（Input）

要求：
	•	必须是“业务语义输入”，例如：
	•	状态变化
	•	用户操作
	•	上层触发

禁止：
	•	中断
	•	硬件信号
	•	协议帧

格式：

输入：
    xxx_event
    xxx_trigger
    xxx_command


⸻

4. 输出（Output）

要求：
	•	必须是“业务语义输出”
	•	不直接输出硬件行为

格式：

输出：
    xxx_result
    xxx_event
    xxx_snapshot


⸻

5. 黑盒视角（Blackbox）

描述：

输入 → 模块 → 输出

不展开内部细节

⸻

6. 白盒结构（Whitebox）

要求：
	•	只拆“业务内部结构”
	•	不涉及 driver / system

格式：

app_xxx_xxx
    ├── 子模块A
    ├── 子模块B


⸻

7. 数据流方向（必须有）

格式：

输入
    ↓
业务处理
    ↓
输出


⸻

8. 设计约束（必须列出）

至少包含：

- 不做硬件访问
- 不做协议解析
- 不做策略（除非特别说明）
- 仅执行业务语义


⸻

推荐业务模块类型（参考）

根据场景可包含：

app_xxx_runtime          ← 运行态管理
app_xxx_device           ← 设备能力暴露
app_xxx_feedback         ← 声光反馈
app_xxx_binding          ← UI状态绑定
app_xxx_controller       ← UI行为控制
app_xxx_view             ← UI结构（EEZ等）


⸻

示例输入（供参考）

例如：
	•	模组检测
	•	仪表检测
	•	AI视觉检测
	•	声光反馈
	•	USB设备模拟

⸻

输出风格要求
	•	结构清晰
	•	模块边界明确
	•	不展开实现细节
	•	不写代码（除非接口定义）
	•	重点体现：
	•	模块职责
	•	输入输出
	•	数据流
	•	解耦关系

⸻

核心原则（必须遵守）

Driver → 设备能力
System → 服务能力
App → 业务语义

Application 层只表达：

“做什么”，不表达“怎么做”

:::

⸻

✅ 你这套 Prompt 的价值（给你一个判断）

这份模板已经具备：
	•	✔ AI 可直接生成结构化架构
	•	✔ 防止 AI 下沉到 driver 层
	•	✔ 强制输出模块边界
	•	✔ 强制数据流清晰

