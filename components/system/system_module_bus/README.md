# system_module_bus

## 目的

`system_module_bus` 是给后续“模组总线选择器 / I2C 复用器”预留的 System 层占位组件。

当前阶段它还没有接入 `system_module`，也没有真正驱动任何硬件，只保留了最小的软件接口：

- `system_module_bus_init()`
- `system_module_bus_deinit()`
- `system_module_bus_select_channel()`
- `system_module_bus_get_selected_channel()`
- `system_module_bus_is_initialized()`

对应代码：

- [include/system_module_bus.h](include/system_module_bus.h)
- [src/system_module_bus.c](src/system_module_bus.c)

---

## 当前仓库里的已知事实

仓库里已经有一条明确的硬件映射线索：

- [ai_context/Driver硬件映射.md](../../../ai_context/Driver硬件映射.md)

其中写到：

- 模组 I2C 总线 -> `NCA9545` 后级各槽位模组

另外在能力清单里也明确写了，如果前端存在 I2C 选择器 / mux，模组表应增加 `channel` 维度：

- [ai_context/System能力清单.md](../../../ai_context/System能力清单.md)

所以可以确认两件事：

1. 仓库设计层面已经预期“模组总线前面可能存在 I2C 复用 / 选择器”
2. `system_module_bus` 这个组件的职责应当是“总线选择”，而不是协议解析或业务管理

---

## 对器件功能的推断

下面这部分是**推断**，不是当前仓库已经完全实现的事实。

由于仓库内部文档写的是 `NCA9545`，而公开容易查到的是同类 4 通道 I2C 开关器件资料，因此当前 README 先按“`NCA9545 / PCA9545A / TCA9545A` 这一类 4-channel I2C switch”来理解功能边界。

可参考的公开资料：

- NXP PCA9545A 产品页：
  https://www.nxp.com/products/interfaces/ic-spi-i3c-interface-devices/ic-i3c-multiplexers-switches/four-channel-ic-bus-switch-with-interrupt-logic-and-reset:PCA9545A_45B_45C
- TI TCA9545A 产品页：
  https://www.ti.com/product/TCA9545A

从这些资料可以得到一组很稳定的“同族器件行为特征”：

- 上游只有一组 `SCL/SDA`
- 下游分成 `4` 个通道
- 通道选择通过 I2C 控制寄存器完成
- 上电默认所有通道不选中
- 器件通常带 `RESET`
- 同类器件通常带中断汇总逻辑
- 支持不同电压域之间的 I2C 电平转换

这意味着 `system_module_bus` 后面更合理的角色是：

- 在访问某个模组槽位前，先切到对应 `channel`
- 切换完成后，再把后续 I2C 收发交给 `driver_i2c_module` / `system_module`
- 保证一次原子收发期间不发生通道切换

---

## 职责边界

`system_module_bus` 未来建议只负责下面这些事情：

- 初始化总线选择器
- 切换当前活动通道
- 查询当前活动通道
- 必要时执行总线 / 选择器复位

不应该负责：

- 下位模组协议打包 / 解析
- 模组在线状态聚合
- App 层业务事件
- UI 或测试流程

也就是说：

- `system_module_bus` 负责“切哪一路”
- `system_module` 负责“在这一路上怎么和下位机通信”
- `app_module_runtime` 负责“把 system 状态收敛成业务状态”

---

## 未来接入方式

比较合理的演进路径是：

1. `system_module_bus` 先落真实器件驱动接线
2. `system_module` 引入 `channel` 维度
3. 在单次 probe / command 前先调用 `system_module_bus_select_channel(channel)`
4. 在 `system_module` 内部保证“选通 + 收发 + 状态更新”是同一段串行临界区

示意流程：

```text
system_module
  -> system_module_bus_select_channel(channel)
  -> driver_i2c_module_write_read(...)
  -> 更新 status / 发布 SYSTEM_MODULE_EVENT
```

---

## 当前实现状态

当前这个组件还只是占位：

- 只保存了一个软件侧 `selected_channel`
- 没有真实写寄存器
- 没有中断处理
- 没有 reset 控制
- 没有和 `system_module` 接线

所以它现在的价值主要是：

- 先把命名和职责边界立住
- 给后续“总线选择器”功能预留稳定入口

---

## TODO

- 确认实际器件型号与地址配置
- 确认通道数量是否固定为 `4`
- 确认是否需要 `INT` 聚合输入支持
- 确认是否需要 `RESET` 管脚控制
- 把软件占位实现替换成真实 I2C 选择器访问
- 在 `system_module` 中引入 `channel` 维度并完成串行化接入
