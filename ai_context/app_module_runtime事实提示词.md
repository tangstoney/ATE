# `app_module_runtime` 事实提示词

## 目的

这份文档用于把 `app_module_runtime` **当前已经成立的实现事实**整理清楚，方便直接发给 ChatGPT 讨论：

- 今天加协议之后，App 层应该接什么输入
- 需要补哪些 `get` 接口
- 白盒测试应该围绕哪些状态与事件做
- 和 Tiny MCU 联调后，这个 App 层如何承接 System 输出

---

## 模块名

`app_module_runtime`

中文名：

热插拔模组运行态管理模块

---

## 当前模块本质

`app_module_runtime` 当前不是协议层，也不是驱动层，而是一个**业务侧运行态聚合器**。

它当前只负责：

- 按 `module_id` 维护模块基础信息
- 维护在线 / 离线 / 故障 等运行状态
- 输出 snapshot
- 通过 `esp_event` 发布运行态事件

它当前**不负责**：

- 直接访问 I2C / UART / USB 等硬件
- 协议解析
- 模组底层帧结构解释
- 创建 `system_module`
- 白盒测试驱动

一句话：

> `app_module_runtime = 模组运行态业务聚合器，当前输入是上层语义事件，不是底层通信事实`

---

## 当前已经成立的代码事实

### 1. 当前是 App 层，不碰硬件

组件依赖只有：

- `esp_event`

没有直接依赖：

- `driver_i2c_module`
- `system_module`
- `system_module_events`

所以当前 `app_module_runtime` 仍然是一个比较“纯”的业务层容器。

### 2. 当前内部核心状态只有一个 snapshot

当前 handle 内部只有：

```c
struct app_module_runtime {
    app_module_runtime_snapshot_t snapshot;
    bool started;
};
```

也就是说当前状态中心很简单：

- 一个模块数组
- `module_count`
- `online_count`
- `started`

### 3. 当前模块上限为 16

编译期上限：

- `APP_MODULE_RUNTIME_MAX_MODULES = 16`

每个模块当前持有的基础字段包括：

- `module_id`
- `module_type`
- `display_name`
- `revision`
- `online`
- `status`

### 4. 当前公开输入是“业务语义输入”，不是硬件输入

当前输入接口是：

- `on_rx_frame`
- `on_timeout`
- `on_port_detected`
- `on_port_lost`
- `on_manual_rescan`

注意：

这里的 `rx_frame` 已经不是 UART/I2C 原始帧，而是 App 层抽象后的：

```c
typedef struct {
    char source_tag[...];
    uint8_t payload[64];
    size_t len;
} app_module_runtime_rx_frame_t;
```

### 5. 当前事件总线已经存在，但还没有看到订阅者

当前定义了：

- `APP_MODULE_RUNTIME_EVENT`

并会通过默认 `esp_event` loop 发布：

- `APP_MODULE_RUNTIME_BUS_EVENT_NOTIFY`
- `APP_MODULE_RUNTIME_BUS_EVENT_SNAPSHOT`
- `APP_MODULE_RUNTIME_BUS_EVENT_ONLINE_CHANGED`
- `APP_MODULE_RUNTIME_BUS_EVENT_INFO_UPDATED`

但当前仓库里还没看到实际订阅者。

### 6. 当前事件 ID 已经定义，但业务语义还比较粗

事件 ID：

- `MODULE_ONLINE_CHANGED`
- `MODULE_INFO_UPDATED`
- `MODULE_PORT_DETECTED`
- `MODULE_PORT_LOST`
- `MODULE_TIMEOUT`
- `MODULE_MANUAL_RESCAN`

这说明当前已经定下来的方向是：

> App 层靠语义事件驱动，而不是靠回调函数指针和 emit/bind 风格

### 7. `on_rx_frame()` 当前实现还是占位逻辑

当前 `app_module_runtime_on_rx_frame()` 的行为是：

- 用 `frame->source_tag` 找或创建 slot
- 直接标记 `online = true`
- 直接标记 `status = ONLINE`
- 用 `"frame_len_%u"` 填 `revision`
- 发布 `info updated / online changed / notify / snapshot`

这明显还是一个占位实现，不是真正的协议适配结果。

### 8. `on_port_detected()` 当前也比较早期

当前 `on_port_detected()`：

- 找或创建 slot
- 直接置 `online = true`
- `status = DETECTED`
- 发布在线变化和 snapshot

这意味着当前“port detected”被当成了“已在线”的近似信号，后面是否要继续细分要讨论。

### 9. 当前只有一个 get 接口

当前唯一只读查询接口是：

```c
esp_err_t app_module_runtime_get_snapshot(...);
```

没有：

- `get_module_count`
- `get_module_by_index`
- `get_module_by_id`
- `get_online_count`
- `get_module_status`
- `get_module_basic_info`

### 10. 当前 `init()` 里已经留了待办

源码里已经有一句注释：

```c
// codex todo 一键初始化system_module
```

这说明当前作者思路已经开始往：

> `app_module_runtime` 与 `system_module` 自动接线

这个方向考虑，但还没有真正落地。

---

## 当前公开接口事实

```c
esp_err_t app_module_runtime_init(app_module_runtime_handle_t *out_handle);
esp_err_t app_module_runtime_start(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_stop(app_module_runtime_handle_t handle);
esp_err_t app_module_runtime_deinit(app_module_runtime_handle_t handle);

esp_err_t app_module_runtime_on_rx_frame(app_module_runtime_handle_t handle,
                                         const app_module_runtime_rx_frame_t *frame);
esp_err_t app_module_runtime_on_timeout(app_module_runtime_handle_t handle, const char *module_id);
esp_err_t app_module_runtime_on_port_detected(app_module_runtime_handle_t handle, const char *module_id);
esp_err_t app_module_runtime_on_port_lost(app_module_runtime_handle_t handle, const char *module_id);
esp_err_t app_module_runtime_on_manual_rescan(app_module_runtime_handle_t handle);

esp_err_t app_module_runtime_get_snapshot(app_module_runtime_handle_t handle,
                                          app_module_runtime_snapshot_t *out_snapshot);
```

---

## 当前白盒数据流事实

当前实现更像：

```text
上层语义输入
    ├── module_rx_frame
    ├── module_timeout
    ├── module_port_detected
    ├── module_port_lost
    └── manual_rescan
            ↓
app_module_runtime
    ├── module registry（snapshot）
    ├── online count refresh
    └── event publish
            ↓
APP_MODULE_RUNTIME_EVENT
    ├── SNAPSHOT
    ├── ONLINE_CHANGED
    ├── INFO_UPDATED
    └── NOTIFY
```

---

## 已知缺口 / 讨论重点

如果今天要加协议、做基础白盒测试、再和 Tiny MCU 联调，当前值得重点讨论这些：

### 1. 当前 get 接口明显不够

现在只有 `get_snapshot()`。

如果后面要做白盒测试、调试控制台、或者给 UI / Gateway / Console 查询，会很可能需要补：

- `get_module_count`
- `get_online_count`
- `get_module_by_index`
- `get_module_basic_info_by_id`
- `get_module_status_by_id`
- `is_module_online`

但这些接口要保持 App 层语义，不要回到 Driver/System 细节。

### 2. 当前 `rx_frame -> revision=frame_len_x` 明显只是占位

后面和 Tiny MCU 联调后，真正应该讨论的是：

- 谁把协议解析成业务对象
- 是在 `system_module` 里做，还是先有 parser adapter
- `app_module_runtime` 是否只吃已经解析好的结构

当前实现显然还没收敛到最终形态。

### 3. 当前 `port_detected` 与 `online=true` 绑定得太早

如果热插拔业务想更严谨，可能需要区分：

- `detected`
- `identified`
- `online`
- `offline`
- `fault`

当前实现偏简化。

### 4. 当前没有 timestamp / stale / last_seen

如果之后要做更稳定的在线管理，可能还需要：

- 最后一次帧到达时间
- 最后一次探测时间
- 超时窗口
- 数据陈旧度

当前一个都没有。

### 5. 当前没有和 `system_module` 真正接线

这意味着现在 `app_module_runtime` 还更像“空心业务容器”。

联调之后应重点讨论：

- 它应该订阅哪个事件源
- 是不是要直接吃 `system_module_events`
- 是否需要一个 `module_parser_adapter`

---

## 可直接发给 ChatGPT 的提示词

```text
我在做一个 ESP-IDF 工程里的 app_module_runtime，下面是当前已经成立的实现事实。请不要按理想化方式重写整个架构，而是在这些事实基础上讨论应该怎么补“get 接口”和今天的白盒测试方案。

当前事实如下：

1. 这是 App 层，不允许直接访问 I2C/UART/USB/Driver，不负责协议解析。
2. 它当前只依赖 esp_event，本质上是一个“模组运行态业务聚合器”。
3. 内部核心状态只有：
   - snapshot
   - started
4. snapshot 里维护：
   - module_count
   - online_count
   - modules[16]
5. 每个 module 当前字段只有：
   - module_id
   - module_type
   - display_name
   - revision
   - online
   - status
6. 当前输入接口只有：
   - on_rx_frame
   - on_timeout
   - on_port_detected
   - on_port_lost
   - on_manual_rescan
7. 当前唯一 get 接口只有 get_snapshot()。
8. 当前已经定义了 APP_MODULE_RUNTIME_EVENT，并会发布：
   - SNAPSHOT
   - ONLINE_CHANGED
   - INFO_UPDATED
   - NOTIFY
9. 当前 on_rx_frame() 还是占位实现：收到 frame 后直接把 revision 写成 frame_len_x，并标记 online/status。
10. 当前 on_port_detected() 也较早期：检测到端口后直接 online=true。
11. 当前源码里已经有“后续自动接 system_module”的 TODO，但还没真正接线。

我现在要讨论的是：

- 在这些既有事实下，app_module_runtime 应该补哪些 get 接口，才能支持白盒测试、UI 查询、联调调试？
- 这些 get 接口应该如何命名和分层，才能保持 App 语义而不是下沉到 System/Driver？
- 如果今天要加协议、做基础白盒测试、然后和 Tiny MCU 联调，app_module_runtime 最合理的输入输出边界应该是什么？

请按以下结构回答：

1. 已成立事实
2. 建议保留
3. 建议新增的 get 接口
4. 当前实现中明显只是占位、联调后应替换的部分
5. 明确不要做的事
```

