/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "system_network.h"

#include "driver_eth.h"
#include "esp_event.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "system_network";

ESP_EVENT_DEFINE_BASE(SYSTEM_NET_EVENT);

typedef struct {
    driver_eth_handle_t eth_driver;
    esp_netif_t *eth_netif;
    bool link_up;
    bool has_ip;
} system_network_ctx_t;

static system_network_ctx_t g_net_ctx = {
    .eth_driver = NULL,
    .eth_netif = NULL,
    .link_up = false,
    .has_ip = false,
};

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            g_net_ctx.link_up = true;
            esp_event_post(SYSTEM_NET_EVENT, SYSTEM_NET_EVENT_CONNECTED, NULL, 0, portMAX_DELAY);
            ESP_LOGI(TAG, "Ethernet connected");
            break;

        case ETHERNET_EVENT_DISCONNECTED:
            g_net_ctx.link_up = false;
            g_net_ctx.has_ip = false;
            esp_event_post(SYSTEM_NET_EVENT, SYSTEM_NET_EVENT_DISCONNECTED, NULL, 0, portMAX_DELAY);
            esp_event_post(SYSTEM_NET_EVENT, SYSTEM_NET_EVENT_LOST_IP, NULL, 0, portMAX_DELAY);
            ESP_LOGI(TAG, "Ethernet disconnected");
            break;

        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet started");
            break;

        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet stopped");
            break;

        default:
            break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    (void)arg;

    switch (event_id) {
        case IP_EVENT_ETH_GOT_IP: {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            g_net_ctx.has_ip = true;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            esp_event_post(SYSTEM_NET_EVENT, SYSTEM_NET_EVENT_GOT_IP, NULL, 0, portMAX_DELAY);
            break;
        }

        case IP_EVENT_ETH_LOST_IP:
            g_net_ctx.has_ip = false;
            esp_event_post(SYSTEM_NET_EVENT, SYSTEM_NET_EVENT_LOST_IP, NULL, 0, portMAX_DELAY);
            ESP_LOGI(TAG, "Lost IP");
            break;

        default:
            break;
    }
}

esp_err_t system_network_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing system network...");

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return ret;
    }

    ret = driver_eth_create(&g_net_ctx.eth_driver);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "driver_eth_create failed");
        return ret;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init failed");
        driver_eth_destroy(g_net_ctx.eth_driver);
        g_net_ctx.eth_driver = NULL;
        return ret;
    }

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    g_net_ctx.eth_netif = esp_netif_new(&cfg);
    if (!g_net_ctx.eth_netif) {
        ESP_LOGE(TAG, "esp_netif_new failed");
        driver_eth_destroy(g_net_ctx.eth_driver);
        g_net_ctx.eth_driver = NULL;
        return ESP_FAIL;
    }

    esp_eth_handle_t eth_handle = NULL;
    ret = driver_eth_get_raw_handle(g_net_ctx.eth_driver, &eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "driver_eth_get_raw_handle failed");
        goto err;
    }

    ret = esp_netif_attach(g_net_ctx.eth_netif, esp_eth_new_netif_glue(eth_handle));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_attach failed");
        goto err;
    }

    ret = driver_eth_start(g_net_ctx.eth_driver);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "driver_eth_start failed");
        goto err;
    }

    esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, ip_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, ip_event_handler, NULL);

    ESP_LOGI(TAG, "System network initialized");
    return ESP_OK;

err:
    if (g_net_ctx.eth_netif != NULL) {
        esp_netif_destroy(g_net_ctx.eth_netif);
        g_net_ctx.eth_netif = NULL;
    }
    if (g_net_ctx.eth_driver != NULL) {
        driver_eth_destroy(g_net_ctx.eth_driver);
        g_net_ctx.eth_driver = NULL;
    }
    return ret;
}

esp_err_t system_network_deinit(void)
{
    esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, ip_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_LOST_IP, ip_event_handler);

    if (g_net_ctx.eth_driver != NULL) {
        driver_eth_stop(g_net_ctx.eth_driver);
        driver_eth_destroy(g_net_ctx.eth_driver);
        g_net_ctx.eth_driver = NULL;
    }

    if (g_net_ctx.eth_netif != NULL) {
        esp_netif_destroy(g_net_ctx.eth_netif);
        g_net_ctx.eth_netif = NULL;
    }

    g_net_ctx.link_up = false;
    g_net_ctx.has_ip = false;

    return ESP_OK;
}

bool system_network_is_ready(void)
{
    return g_net_ctx.link_up && g_net_ctx.has_ip;
}

bool system_network_is_link_up(void)
{
    return g_net_ctx.link_up;
}

bool system_network_has_ip(void)
{
    return g_net_ctx.has_ip;
}
