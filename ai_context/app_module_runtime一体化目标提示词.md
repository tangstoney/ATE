# `app_module_runtime` 一体化目标提示词

## 使用目的

这份文档用于直接发给 Codex / ChatGPT，要求其基于当前 ATE 仓库真实代码，把 `app_module_runtime` 收口成一个**内聚的一体化 App 运行态模块**。

这份提示词强调的不是“只订阅 system event”这么简单，而是进一步明确：

1. `app_ate_console` 不应该同时持有 `system_module_handle_t` 和 `app_module_runtime_handle_t`
2. `system_module_handle_t` 应隐藏在 `app_module_runtime` 内部
3. `app_module_runtime_init()` / `start()` 负责把 `system_module` 生命周期和事件订阅接起来
4. `SYSTEM_MODULE_EVENT` 必须带上模组身份信息，不能继续只有单一匿名状态
5. 协议实现必须继续留在 `system_module` 内部，不能再泄漏到 App 层

如果要讨论“当前已经实现了什么”，请同时参考：

- [app_module_runtime事实提示词.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/ai_context/app_module_runtime事实提示词.md)
- [app_module_runtime目标提示词.md](/Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE/ai_context/app_module_runtime目标提示词.md)

---

## 任务目标

实现 `app_module_runtime` 组件（Application 层），让它成为：

> 模组运行态唯一业务入口 + system_module 的上层封装 + UI / network / log 的统一状态出口

它的职责不是协议解析，不是驱动访问，而是：

1. 内部持有并管理 `system_module`
2. 订阅 `system_module` 发布的 `SYSTEM_MODULE_EVENT`
3. 按 `module_id` 收敛状态到 `module_registry`
4. 生成统一 `snapshot`
5. 发布 `APP_MODULE_RUNTIME_EVENT`
6. 对外提供只读 `get` 接口

---

## 核心架构定位

目标链路应为：

```text
driver_i2c_module
    ↓
system_module（内部含私有协议实现）
    ↓ SYSTEM_MODULE_EVENT（带 module identity）
app_module_runtime（持有 system_module + 状态收敛）
    ↓ APP_MODULE_RUNTIME_EVENT / snapshot
UI / network / log / console
```

关键原则：

- `app_ate_console` 只持有 `app_module_runtime_handle_t`
- `app_ate_console` 不直接持有 `system_module_handle_t`
- `system_module` 句柄藏在 `app_module_runtime` 内部

---

## 强制架构约束

### 1. `app_ate_console` 不允许同时持有两个句柄

禁止：

```c
system_module_handle_t system_module;
app_module_runtime_handle_t module_runtime;
```

出现在同一个 app 启动聚合器上下文里。

目标做法：

```c
typedef struct app_module_runtime *app_module_runtime_handle_t;
```

由 `app_module_runtime` 内部隐藏：

```c
system_module_handle_t system_module;
```

---

### 2. `app_module_runtime_init()` 必须负责底层接线

至少要做到：

- 创建 `system_module`
- 初始化自己的 registry / snapshot / started 标志

可选但推荐：

- `init()` 做 create
- `start()` 做 event subscribe

也就是说，上层只需要：

```c
app_module_runtime_init(...)
app_module_runtime_start(...)
```

而不是再额外手动：

```c
system_module_create(...)
```

---

### 3. `SYSTEM_MODULE_EVENT` 必须带模组身份信息

当前如果 event payload 只有：

- `state`
- `error_code`
- `cmd`
- `success`

这是不够的。

必须补充最少这些身份信息：

- `module_id`
- `module_name`

建议同时具备：

- `module_type`

原因：

1. `app_module_runtime` 必须按 `module_id` 更新 slot
2. 后续会有不止 8 个模组，必须支持继续扩展
3. 不能再使用“单匿名默认槽位”的临时方案

---

### 4. `system_module` 的 public event contract 允许公开，但协议实现必须私有

允许公开：

- `SYSTEM_MODULE_EVENT`
- `SYSTEM_MODULE_EVENT_*`
- `system_module_*_event_t`

并放在：

- `components/system/system_module/include/system_module.h`

不允许公开：

- 协议 frame 格式
- CRC 细节
- 私有 parse/build 结构体
- `system_module_protocol_*` 内部类型

私有协议实现应留在：

- `components/system/system_module/src/system_module_protocol.h`
- `components/system/system_module/src/system_module_protocol.c`

---

### 5. App 层禁止协议语义

`app_module_runtime` 中禁止出现：

- frame
- raw_data
- payload parser
- crc
- 帧头
- build_request / parse_response

App 层只能吃：

- `SYSTEM_MODULE_EVENT_*`
- `system_module` 的只读 get / 动作接口

---

### 6. 必须保留 App 层事件总线

如果可行，`app_module_runtime` 应继续保留自己的事件总线：

- `APP_MODULE_RUNTIME_EVENT`

原因：

1. UI 不应该直接订阅 `SYSTEM_MODULE_EVENT`
2. network / log / console 也不应该直接依赖 system 细粒度事件
3. App 层应该输出“业务收敛结果”

所以建议继续发布：

- `APP_MODULE_RUNTIME_BUS_EVENT_SNAPSHOT`
- `APP_MODULE_RUNTIME_BUS_EVENT_ONLINE_CHANGED`
- `APP_MODULE_RUNTIME_BUS_EVENT_INFO_UPDATED`
- `APP_MODULE_RUNTIME_BUS_EVENT_NOTIFY`

---

## 目标句柄结构

目标内部结构建议至少包含：

```c
struct app_module_runtime {
    system_module_handle_t system_module;
    app_module_runtime_snapshot_t snapshot;
    bool started;
    esp_event_handler_instance_t event_instance;
};
```

如有必要可补充：

- mutex
- module_registry 子结构
- last_command cache

但不要过度设计。

---

## `system_module` 事件目标设计

你需要推动 `system_module` 的 event payload 变成“可按模组识别”的结构。

建议参考：

```c
typedef struct {
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    char module_name[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    char module_type[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    uint8_t state;
    uint16_t error_code;
} system_module_status_event_t;

typedef struct {
    char module_id[APP_MODULE_RUNTIME_ID_MAX_LEN];
    char module_name[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    char module_type[APP_MODULE_RUNTIME_NAME_MAX_LEN];
    uint8_t cmd;
    bool success;
} system_module_cmd_done_event_t;
```

要求：

1. `module_id` 必须是稳定身份标识
2. `module_name` 是便于 UI / log 展示的显示名
3. `module_type` 可选但推荐，便于后续扩展
4. 这些字段必须由 `system_module` 发布时写入，而不是让 App 层猜

---

## `app_module_runtime` 目标职责

### 1. 生命周期

对外接口建议至少保留：

```c
esp_err_t app_module_runtime_init(app_module_runtime_handle_t *out_handle);
esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_stop(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_deinit(app_module_runtime_handle_t handle);
```

语义要求：

- `init()`：创建 `app_module_runtime` 自身资源，并 `system_module_create()`
- `start()`：注册 `SYSTEM_MODULE_EVENT`
- `stop()`：解绑事件
- `deinit()`：销毁 `system_module` 和自身资源

---

### 2. 状态收敛

`app_module_runtime` 负责：

- 按 `module_id` 查找或创建 slot
- 维护：
  - `online`
  - `status`
  - `error_code`
  - `module_name`
  - `module_type`
  - `revision`（若 system 侧能提供）
- 去重，避免重复发布

---

### 3. Snapshot

建议结构：

```c
typedef struct {
    size_t module_count;
    size_t online_count;
    app_module_runtime_basic_info_t modules[MAX];
} app_module_runtime_snapshot_t;
```

其中每个模块基础信息至少有：

```c
typedef struct {
    char module_id[...];
    char module_type[...];
    char display_name[...];
    char revision[...];
    bool online;
    app_module_runtime_status_t status;
    uint16_t error_code;
} app_module_runtime_basic_info_t;
```

限制：

snapshot 只允许包含：

- 生命周期
- 基本标识
- 状态
- 错误码

禁止包含：

- 原始 payload
- 实时测量流
- 协议帧

---

### 4. 只读查询接口

建议补齐：

```c
esp_err_t app_module_runtime_get_snapshot(...);
esp_err_t app_module_runtime_get_module_count(...);
esp_err_t app_module_runtime_get_online_count(...);
esp_err_t app_module_runtime_get_module_by_index(...);
esp_err_t app_module_runtime_get_module_by_id(...);
esp_err_t app_module_runtime_is_module_online(...);
```

这些接口是给：

- UI
- console
- network
- 白盒测试

使用的。

---

### 5. system_module 动作入口

因为 `system_module` 被藏在 `app_module_runtime` 内部，上层如果要驱动模组行为，应该通过 `app_module_runtime` 暴露语义入口，而不是直接拿 system 句柄。

可按需要补这类接口：

```c
esp_err_t app_module_runtime_probe_all(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_send_command(app_module_runtime_handle_t handle,
                                          const char *module_id,
                                          uint8_t command_id,
                                          const uint8_t *payload,
                                          size_t len);
```

注意：

- 这里仍然应是“业务/模块运行态语义”
- 不要把协议 build/parse 暴露给 App 层

如果当前阶段不做完整命令路由，也至少保留 TODO 接口设计。

---

## 事件处理目标逻辑

### CASE: `SYSTEM_MODULE_EVENT_ONLINE`

根据 `module_id` 找 slot：

- 若 online 状态变化：
  - `online = true`
  - `status = ONLINE`
  - 发布 `ONLINE_CHANGED`
  - 发布 snapshot

---

### CASE: `SYSTEM_MODULE_EVENT_OFFLINE`

根据 `module_id` 找 slot：

- 若 online 状态变化：
  - `online = false`
  - `status = OFFLINE`
  - 发布 `ONLINE_CHANGED`
  - 发布 snapshot

---

### CASE: `SYSTEM_MODULE_EVENT_STATUS_UPDATED`

根据 `module_id` 找 slot：

- 更新：
  - `status`
  - `error_code`
  - `module_name`
  - `module_type`
  - `revision`（若有）
- 若信息变化：
  - 发布 `INFO_UPDATED`
  - 发布 snapshot

---

### CASE: `SYSTEM_MODULE_EVENT_COMMAND_DONE`

根据 `module_id` 找 slot：

- 更新最近命令状态（若需要缓存）
- 发布 `NOTIFY`

---

## 关键规则

1. 必须用 `module_id` 做 registry key
2. 不允许再使用单默认槽位方案
3. 必须检测状态变化，避免重复更新
4. 每次有效变化后必须重新计算 `online_count`
5. 每次有效变化后必须发布 snapshot
6. 必须使用 `esp_event_handler_instance_register()` / `unregister()`
7. 禁止创建新的 event loop

---

## 与 `app_ate_console` 的关系

目标态下：

`app_ate_console` 只应做：

- `app_module_runtime_init()`
- `app_module_runtime_start()`

而不是：

- `system_module_create()`
- `system_module_get_status()`
- `system_module_send_command()`

这些都不应直接出现在 `app_ate_console` 里。

如果 `app_ate_console` 需要读取模组信息，应通过：

- `app_module_runtime_get_snapshot()`
- 或订阅 `APP_MODULE_RUNTIME_EVENT`

---

## 必须输出的文件

最少涉及：

- `components/apps/app_module_runtime/CMakeLists.txt`
- `components/apps/app_module_runtime/idf_component.yml`
- `components/apps/app_module_runtime/include/app_module_runtime.h`
- `components/apps/app_module_runtime/src/app_module_runtime.c`
- `components/system/system_module/include/system_module.h`
- `components/system/system_module/src/system_module.c`
- `components/system/system_module/src/system_module_events.c`
- `components/system/system_module/src/system_module_protocol.h`
- `components/system/system_module/src/system_module_protocol.c`

如有必要，可再更新：

- `components/apps/app_ate_console/src/app_ate_console.c`

但只允许把它收口到“只持有 app_module_runtime 句柄”，不允许继续扩散 system 细节。

---

## 禁止事项

禁止：

1. 让 `app_ate_console` 同时持有 `system_module_handle_t` 和 `app_module_runtime_handle_t`
2. 让 App 层继续处理 frame / payload / parser
3. 把 `system_protocol` 再单独拆回 public 组件
4. 在 `system_module.h` 再次暴露协议内部类型
5. 继续使用“单匿名默认模组槽位”来假装支持多模组
6. UI 直接订阅 `SYSTEM_MODULE_EVENT`
7. 为了“优雅”一次性推翻整个 System / Driver

---

## 执行顺序（必须按顺序）

第一阶段：现状盘点

1. 扫描当前 `app_module_runtime`
2. 扫描当前 `system_module`
3. 确认 `system_module.h` 中哪些字段仍泄漏协议语义
4. 确认 `SYSTEM_MODULE_EVENT` 当前 payload 缺哪些身份字段

第二阶段：接口收口设计

1. 先确定 `app_module_runtime` 内部持有 `system_module`
2. 再确定 `SYSTEM_MODULE_EVENT` payload 新结构
3. 再确定 `app_module_runtime` 的 get 接口和 app 事件接口

第三阶段：落地代码

1. 先改 `system_module` event payload
2. 再改 `app_module_runtime` registry / event handling
3. 最后才改 `app_ate_console` 的装配方式

第四阶段：最终输出

必须输出：

1. 修改文件清单
2. 新增文件清单
3. `app_module_runtime` 最终对外接口
4. `SYSTEM_MODULE_EVENT` 最终 payload 结构
5. `app_ate_console` 最终应只持有哪些句柄
6. 多模组扩展是如何支持的
7. 还未实现的 TODO

---

## 一句话总结

目标不是简单“让 app 订阅一下 system 事件”，而是要把模组运行态真正收成：

> `app_module_runtime = 内含 system_module 的 App 运行态总入口`

让上层只看 `app_module_runtime`，不再直接碰 `system_module` 句柄和协议细节。
