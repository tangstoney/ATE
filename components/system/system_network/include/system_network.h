/*
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 系统网络事件基
 */
ESP_EVENT_DECLARE_BASE(SYSTEM_NET_EVENT);

typedef enum {
    SYSTEM_NET_EVENT_CONNECTED = 0,  // 以太网链接建立
    SYSTEM_NET_EVENT_DISCONNECTED,  // 以太网链接断开
    SYSTEM_NET_EVENT_GOT_IP,         // 获取到 IP
    SYSTEM_NET_EVENT_LOST_IP,        // 失去 IP
    SYSTEM_NET_EVENT_READY,          // 网络就绪（链接 + IP）
} system_net_event_id_t;

/**
 * @brief 初始化网络系统（管理 driver_eth + esp_netif + DHCP）
 */
esp_err_t system_network_init(void);

/**
 * @brief 反初始化网络系统
 */
esp_err_t system_network_deinit(void);

/**
 * @brief 查询网络是否就绪
 */
bool system_network_is_ready(void);

/**
 * @brief 获取 IP 地址字符串
 */
esp_err_t system_network_get_ip(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif
