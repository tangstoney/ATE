# `app_module_runtime` 目标提示词

## 使用目的

这份文档不是描述当前代码事实，而是描述 **`app_module_runtime` 期望演进到的目标形态**。

适合直接发给 ChatGPT / Codex 讨论：

- `app_module_runtime` 最终应该如何从 `system_module` 事件收敛业务状态
- 应该补哪些只读 `get` 接口
- 白盒测试应该围绕哪些状态变化和事件输出设计

如果要讨论“当前已经实现了什么”，请同时参考：

- [app_module_runtime事实提示词.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/ai_context/app_module_runtime事实提示词.md)

---

## 任务目标

实现 `app_module_runtime` 组件（Application 层），
该组件是系统的“状态中心（State Aggregator）”。

职责：

1. 订阅 `system_module` 产生的设备层事件（`esp_event`）
2. 对事件进行状态收敛（`module_registry`）
3. 生成统一 `snapshot`（只读数据出口）
4. 向 UI / network / log 发布业务层事件（`APP_MODULE_RUNTIME_EVENT`）

---

## 核心架构定位

System 层事件：

```text
SYSTEM_MODULE_EVENT_*
→ 表示“设备状态变化”（细粒度）
```

App 层事件：

```text
APP_MODULE_RUNTIME_EVENT_*
→ 表示“业务状态变化”（收敛后结果）
```

这两层事件不是重复，而是：

```text
System Event = 输入信号
App Event    = 状态机输出
```

---

## 必须输出文件

- `components/apps/app_module_runtime/CMakeLists.txt`
- `components/apps/app_module_runtime/idf_component.yml`
- `components/apps/app_module_runtime/include/app_module_runtime.h`
- `components/apps/app_module_runtime/src/app_module_runtime.c`

---

## 架构约束

### 1. 禁止处理协议数据

禁止出现：

- `frame`
- `raw_data`
- `i2c / uart payload`
- `parser`

### 2. 数据来源必须来自 System 层事件

只允许接收：

- `SYSTEM_MODULE_EVENT_*`

### 3. 禁止调用 driver 层

禁止：

- `driver_i2c_module_*`

### 4. 禁止 UI 直接订阅 System 事件

UI 只能订阅：

- `APP_MODULE_RUNTIME_EVENT_*`

### 5. 必须使用 instance API

使用：

- `esp_event_handler_instance_register()`
- `esp_event_handler_instance_unregister()`

原因：

- 生命周期更安全
- 能精确解绑

### 6. 禁止创建 event loop

事件循环只允许在 `main` 初始化一次：

```c
esp_event_loop_create_default();
```

---

## 核心结构

```c
struct app_module_runtime {
    app_module_runtime_snapshot_t snapshot;
    bool started;
    esp_event_handler_instance_t event_instance;
};
```

---

## `module_registry` 目标职责

职责：

- 查找 / 创建 module slot
- 保存状态（`online / status / info`）
- 去重（避免重复更新）

---

## `snapshot` 目标约束

```c
typedef struct {
    size_t module_count;
    size_t online_count;
    app_module_runtime_basic_info_t modules[MAX];
} app_module_runtime_snapshot_t;
```

限制：

`snapshot` 只允许包含：

- 生命周期信息
- 基本标识信息

禁止包含：

- `rpm`
- 按键状态
- 实时测量数据

---

## 事件订阅要求

### `start()`

```c
esp_event_handler_instance_register(
    SYSTEM_MODULE_EVENT,
    ESP_EVENT_ANY_ID,
    system_module_event_handler,
    handle,
    &handle->event_instance);
```

### `stop()`

```c
esp_event_handler_instance_unregister(
    SYSTEM_MODULE_EVENT,
    ESP_EVENT_ANY_ID,
    handle->event_instance);
```

---

## 事件处理目标逻辑

### CASE: `SYSTEM_MODULE_EVENT_ONLINE`

若状态变化：

- `slot->online = true`
- `slot->status = ONLINE`
- 发布 `ONLINE_CHANGED`

### CASE: `SYSTEM_MODULE_EVENT_OFFLINE`

若状态变化：

- `slot->online = false`
- `slot->status = OFFLINE`
- 发布 `ONLINE_CHANGED`

### CASE: `SYSTEM_MODULE_EVENT_STATUS_UPDATED`

更新：

- `status`
- `revision`
- `error_code`

若信息变化：

- 发布 `INFO_UPDATED`

### CASE: `SYSTEM_MODULE_EVENT_COMMAND_DONE`

更新 command 状态：

- 发布 `NOTIFY`

---

## 关键规则

1. 必须检测状态变化，避免重复更新
2. 每次状态变化后必须重新计算 `online_count`
3. 每次状态变化后必须发布最新 `snapshot`

---

## `snapshot` 发布要求

```c
static void app_module_runtime_publish_snapshot(app_module_runtime_handle_t handle)
{
    esp_event_post(APP_MODULE_RUNTIME_EVENT,
                   APP_MODULE_RUNTIME_BUS_EVENT_SNAPSHOT,
                   &handle->snapshot,
                   sizeof(handle->snapshot),
                   portMAX_DELAY);
}
```

说明：

- `esp_event_post()` 会复制事件数据
- 适合把 snapshot 作为只读状态出口

---

## App 层事件定义目标

必须发布：

- `APP_MODULE_RUNTIME_EVENT_SNAPSHOT`
- `APP_MODULE_RUNTIME_EVENT_ONLINE_CHANGED`
- `APP_MODULE_RUNTIME_EVENT_INFO_UPDATED`
- `APP_MODULE_RUNTIME_EVENT_NOTIFY`

---

## 数据流要求

```text
driver
  ↓
system_module（协议 + 状态机）
  ↓ event（细粒度）
app_module_runtime（状态收敛）
  ↓ event（粗粒度）
UI / network / log
```

---

## 设计原则

1. App 层只处理“状态”，不处理“数据包”
2. System event ≠ App event
3. App event 是“处理结果”，不是透传

---

## 禁止事项

禁止：

- 解析协议
- 使用 `frame`
- 调用 driver
- UI 直接订阅 system event
- 创建 event loop

---

## 设计目标

`app_module_runtime` 最终应成为：

- UI 唯一数据源
- 网络唯一数据源
- 模组状态统一入口

---

## 一句话总结

`app_module_runtime` 只做：

> “接收 system 事件 → 状态收敛 → 发布业务状态”

---

## 与当前代码的主要差异

当前仓库中的 `app_module_runtime` 还没有完全达到这个目标，主要差异有：

### 1. 当前仍然暴露 `on_rx_frame()`

目标态要求 App 层不处理 `frame`，但当前实现还保留：

- `app_module_runtime_on_rx_frame()`

这是一个明显的过渡接口。

### 2. 当前还没有订阅 `SYSTEM_MODULE_EVENT`

目标态要求：

- `start()` 内部订阅 `SYSTEM_MODULE_EVENT`

但当前实现尚未接入 `system_module` 事件。

### 3. 当前还没有 `esp_event_handler_instance_t`

目标态结构里应包含：

- `esp_event_handler_instance_t event_instance`

当前实现内部还没有这个字段。

### 4. 当前 `snapshot` 字段还比较简化

目标态里提到：

- `revision`
- `error_code`

当前实现还没有真正稳定地从 System 层收敛这些信息。

---

## 可直接发给 ChatGPT 的版本

```text
我想实现一个 ESP-IDF 工程里的 app_module_runtime 组件，请按 Application 层来设计，不要下沉到协议层、System 层或 Driver 层。

目标定位：

- app_module_runtime 是模组运行态的状态中心（State Aggregator）
- 它不处理协议帧，不处理 raw data，不调用 driver
- 它只订阅 SYSTEM_MODULE_EVENT_*，做状态收敛，再发布 APP_MODULE_RUNTIME_EVENT_*

职责：

1. 订阅 system_module 的 esp_event 事件
2. 对模块状态做收敛（module_registry）
3. 生成统一 snapshot
4. 向 UI / network / log 发布 APP_MODULE_RUNTIME_EVENT

强制约束：

1. 禁止出现 frame / raw_data / i2c / uart payload / parser
2. 数据来源只能是 SYSTEM_MODULE_EVENT_*
3. 禁止调用 driver_i2c_module_* 等 driver API
4. UI 不能直接订阅 SYSTEM_MODULE_EVENT，只能订阅 APP_MODULE_RUNTIME_EVENT
5. 必须使用 esp_event_handler_instance_register / unregister
6. 禁止创建 event loop，默认事件循环只允许 main 初始化一次

目标核心结构：

struct app_module_runtime {
    app_module_runtime_snapshot_t snapshot;
    bool started;
    esp_event_handler_instance_t event_instance;
};

snapshot 只允许包含：

- module_count
- online_count
- module basic info

禁止包含：

- rpm
- 按键状态
- 实时测量数据

目标事件处理逻辑：

- SYSTEM_MODULE_EVENT_ONLINE
  - 若状态变化：online=true, status=ONLINE
  - 发布 ONLINE_CHANGED

- SYSTEM_MODULE_EVENT_OFFLINE
  - 若状态变化：online=false, status=OFFLINE
  - 发布 ONLINE_CHANGED

- SYSTEM_MODULE_EVENT_STATUS_UPDATED
  - 更新 status / revision / error_code
  - 若信息变化：发布 INFO_UPDATED

- SYSTEM_MODULE_EVENT_COMMAND_DONE
  - 更新 command 状态
  - 发布 NOTIFY

关键规则：

1. 必须检测状态变化，避免重复更新
2. 每次状态变化后重新计算 online_count
3. 每次状态变化后发布 snapshot

目标数据流：

driver
  ↓
system_module（协议 + 状态机）
  ↓ event（细粒度）
app_module_runtime（状态收敛）
  ↓ event（粗粒度）
UI / network / log

请按以下结构回答：

1. 模块职责
2. 头文件接口设计
3. 内部白盒结构
4. 事件处理流程
5. 需要的 get 接口
6. 白盒测试点
7. 明确禁止做的事
```

