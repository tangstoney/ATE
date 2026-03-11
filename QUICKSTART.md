# 快速开始指南

## 目标

本模板展示如何用 ESP-IDF 构建一个**组件化、松耦合、事件驱动**的网络应用。目标是：

- ✅ **参考已跑通的源码** (`以太网应用源码.md`) 进行模块化重构
- ✅ **隐藏细节**：驱动层、网络层、业务层清晰分离
- ✅ **事件解耦**：组件间通过 ESP-IDF `esp_event` 通信，不直接调用
- ✅ **易于扩展**：添加新业务只需新建组件 + 订阅事件，无需改动现有代码

---

## 五分钟快速启动

### 1. 确认环境

```bash
# 检查 ESP-IDF 是否已安装
idf.py --version
# 应输出类似：ESP-IDF v5.0 或更高

# 如未安装，参考: https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html
```

### 2. 进入项目目录

```bash
cd /Users/yanfa-tangshi/Johnson/AutoTestPlatform/ATE
```

### 3. 配置目标芯片 (首次)

```bash
idf.py set-target esp32p4
```

### 4. 验证 GPIO 配置 (关键!)

**打开 `main/bsp.h`，检查以下 GPIO 是否与你的硬件匹配**：

```c
#define BSP_ETH_RMII_TX_EN    GPIO_NUM_49
#define BSP_ETH_RMII_TXD0     GPIO_NUM_34
// ... 等其他 RMII 引脚

#define BSP_ETH_MDC_GPIO      GPIO_NUM_23
#define BSP_ETH_MDIO_GPIO     GPIO_NUM_18

#define BSP_ETH_PHY_ADDR      0
#define BSP_ETH_PHY_RST_GPIO  GPIO_NUM_51
```

**参考 `以太网应用源码.md` 中的配置，根据你的 PCB 修改这些宏**。

### 5. 构建

```bash
idf.py build
```

**预期输出**（最后几行）：
```
[100%] Linking CXX executable ATE.elf
[100%] Built target app-update
Project build complete. The built files can be found at /path/to/build
```

### 6. 烧录

```bash
# 查看可用串口
ls /dev/tty.* | grep -i usb

# 假设是 /dev/tty.usbserial-XXXX
idf.py -p /dev/tty.usbserial-XXXX flash
```

### 7. 监测输出

```bash
idf.py -p /dev/tty.usbserial-XXXX monitor
```

**预期看到的日志**：

```
I (263) main: ==========================================
I (263) main: ESP-IDF Component-Based Network App Start
I (263) main: ==========================================
I (274) nvs: NVS Flash initialized
I (275) main: NVS Flash initialized
I (276) main: Default event loop created
I (277) driver_eth: Ethernet PHY Address: 0
I (278) net_manager: Ethernet driver created
I (279) net_manager: esp_netif initialized
I (281) net_manager: Ethernet driver started
I (283) net_manager: Event handlers registered
I (284) main: Network manager initialized
I (286) app_tcp_client: TCP client application initialized
I (290) app_tcp_client: TCP client started
I (291) main: Application main loop running...

# 等待链接...
I (5294) net_manager: Ethernet event: CONNECTED
I (6295) net_manager: Ethernet event: DISCONNECTED
# 或
I (7295) net_manager: Got IPv4 address: 192.168.x.x
I (7298) net_manager: Ethernet event: CONNECTED
I (8301) app_tcp_client: Network ready, initiating connection...
I (8302) app_tcp_client: Resolving hostname: 192.168.1.100
I (8304) app_tcp_client: Connecting to 192.168.1.100:5000
```

---

## 项目文件说明

### 核心组件

| 文件 | 说明 |
|------|------|
| `components/driver_eth/**` | 驱动层：ESP-IDF 以太网驱动封装 |
| `components/net_manager/**` | 网络层：esp_netif + DHCP + 事件广播 |
| `components/app_tcp_client/**` | 业务层：TCP 客户端示例 |
| `main/main.c` | 应用启动入口 |
| `main/bsp.h` | 板级硬件配置（GPIO 等） |

### 如何添加新业务组件？

假设要添加文件传输功能 `app_file_transfer`：

```
components/app_file_transfer/
├── CMakeLists.txt           # 声明依赖：net_manager, freertos
├── include/
│   └── app_file_transfer.h  # 公开 API
└── app_file_transfer.c      # 实现
```

关键 API：

```c
// app_file_transfer.h
esp_err_t app_file_transfer_init(void);
esp_err_t app_file_transfer_start(void);
```

实现思路：

1. `init()` 中注册 `NET_MGR_EVENT` 事件处理器
2. 事件处理器收到 `NET_MGR_EVENT_GOT_IP` 时，创建任务开始传输
3. 收到 `NET_MGR_EVENT_LOST_IP` 时，关闭连接等待重连

**这样就完全不需要改动 driver_eth、net_manager 或 main.c！**

---

## 工作流程图

```
启动程序
   ↓
 nvs_flash_init()
   ↓
esp_event_loop_create_default()  ← 创建事件队列
   ↓
 driver_eth_create()  ← 初始化 MAC/PHY
   ↓
net_manager_init()
   ├─ esp_netif_init()  ← 创建网络接口
   ├─ esp_netif_attach()  ← 绑定驱动
   ├─ driver_eth_start()  ← 启动以太网
   ├─ 注册 ETH_EVENT/IP_EVENT handlers  ← 监听原生事件
   └─ (事件处理器转化为 NET_MGR_EVENT)
   ↓
app_tcp_client_init()
   └─ 注册 NET_MGR_EVENT 处理器  ← 订阅网络事件
   ↓
app_tcp_client_start()
   └─ 创建 TCP 客户端任务  ← 后台运行

等待网络连接...
   ↓
[链接 UP] ─→ ETH_EVENT_CONNECTED
   ↓
esp_event_post(NET_MGR_EVENT_CONNECTED)
   ↓
[DHCP 获得 IP] ─→ IP_EVENT_ETH_GOT_IP
   ↓
esp_event_post(NET_MGR_EVENT_GOT_IP)
   ↓
  app_tcp_client 的事件处理器被触发
             ↓
          连接服务器
             ↓
         发送/接收数据

等待网络断开...
   ↓
[链接 DOWN] ─→ ETH_EVENT_DISCONNECTED
   ↓
esp_event_post(NET_MGR_EVENT_DISCONNECTED)
   ↓
  app_tcp_client 清理连接，等待重连
```

---

## 关键设计要点

### 1. 为什么使用 Handle 模式？

```c
typedef struct driver_eth_t *driver_eth_handle_t;
```

**优点**：
- ✅ 隐藏实现细节 (应用不需知道内部字段)
- ✅ 便于版本控制 (可改变内部结构而不影响 API)
- ✅ 易于多实例 (可创建多个驱动实例)

参考：[ESP-IDF Handle 模式说明](https://developer.espressif.com/blog/2025/10/oop_with_c/#handles-in-esp-idf)

### 2. 为什么使用事件而不是直接调用？

**直接调用**（紧耦合）：
```c
// ❌ 不好：app_tcp_client 必须知道 net_manager 的所有 API
app_tcp_client_on_network_ready() {
    net_manager_get_netif();
    net_manager_get_driver();
    // ...
}
```

**事件驱动**（松耦合）：
```c
// ✅ 好：app_tcp_client 只需订阅事件
void net_event_handler(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data) {
    if (event_id == NET_MGR_EVENT_GOT_IP) {
        tcp_client_connect();
    }
}
```

**优点**：
- 组件独立性强
- 易于添加新业务组件（无需改动现有代码）
- 事件处理是异步的（效率高）

### 3. 参考已跑通的源码

`以太网应用源码.md` 中的 `example_eth_init()` 已在硬件上验证通过，所以：

- ✅ `driver_eth.c` 中的 `eth_esp32_emac_config_t` 配置复用原样
- ✅ `driver_eth.c` 中的 GPIO 配置也复用，只是改用 `bsp.h` 宏
- ✅ `net_manager.c` 中的 DHCP + IP 事件处理参考原始流程

这样可以最大程度复用已验证的代码。

---

## 常见问题

### Q: 如何改用静态 IP 而非 DHCP？

**A**: 编辑 `components/net_manager/net_manager.c`，在 `net_manager_init()` 中：

```c
// esp_netif_dhcpc_start(g_net_mgr.eth_netif);  ← 注释掉 DHCP

// 改为静态 IP
esp_netif_ip_info_t ip_info;
IP4_ADDR(&ip_info.ip, 192, 168, 1, 100);
IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
esp_netif_set_ip_info(g_net_mgr.eth_netif, &ip_info);
```

### Q: 如何改变 TCP 服务器地址？

**A**: 编辑 `components/app_tcp_client/app_tcp_client.c`：

```c
#define APP_TCP_SERVER_HOST "your-server-ip"
#define APP_TCP_SERVER_PORT 5000
```

### Q: 如何添加自己的业务组件？

**A**: 参考"快速启动"一节中的"添加新业务组件"部分。

### Q: 如何查看详细日志？

**A**: 在 `idf.py menuconfig` 中调整日志级别：

```
Component config → Log output
  → Default log verbosity
    → [*] Debug
```

### Q: 烧录后无反应？

**A**: 
1. 检查 USB 连接和 `bsp.h` 中的 GPIO 配置
2. 打开监测输出查看日志：`idf.py -p /dev/tty.* monitor`
3. 如仍无输出，尝试强制重启：按住 BOOT + RST 按钮

---

## 下一步

- 📖 阅读 [ARCHITECTURE.md](./ARCHITECTURE.md) 了解完整设计
- 🔧 根据硬件修改 `main/bsp.h`
- 🚀 编译、烧录、测试
- 🧩 添加自己的业务组件
- 📡 集成更多协议 (MQTT、HTTP、CoAP 等)

---

**祝编码愉快！ 🎉**
