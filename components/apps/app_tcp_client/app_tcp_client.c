/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "app_tcp_client.h"
#include "system_network.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>

static const char *TAG = "app_tcp_client";

// Configuration (can be customized)
#define APP_TCP_SERVER_HOST "192.168.1.100"
#define APP_TCP_SERVER_PORT 5000
#define APP_TCP_CONNECT_TIMEOUT_MS 10000
#define APP_TCP_TASK_STACK_SIZE 4096
#define APP_TCP_TASK_PRIORITY 5

/**
 * @brief Commands for TCP client task
 */
typedef enum {
    APP_TCP_CMD_START,
    APP_TCP_CMD_STOP,
    APP_TCP_CMD_RECONNECT,
} app_tcp_cmd_t;

/**
 * @brief TCP client context
 */
typedef struct {
    TaskHandle_t task_handle;
    QueueHandle_t cmd_queue;
    int socket_fd;
    bool running;
} app_tcp_client_ctx_t;

static app_tcp_client_ctx_t g_tcp_ctx = {
    .task_handle = NULL,
    .cmd_queue = NULL,
    .socket_fd = -1,
    .running = false,
};

/**
 * @brief Close TCP socket if open
 */
static void tcp_socket_close(void)
{
    if (g_tcp_ctx.socket_fd >= 0) {
        shutdown(g_tcp_ctx.socket_fd, SHUT_RDWR);
        close(g_tcp_ctx.socket_fd);
        g_tcp_ctx.socket_fd = -1;
        ESP_LOGI(TAG, "Socket closed");
    }
}

/**
 * @brief Connect to TCP server
 *
 * @return true if connection successful, false otherwise
 */
static bool tcp_connect_to_server(void)
{
    struct sockaddr_in server_addr = {0};
    struct hostent *he = NULL;

    ESP_LOGI(TAG, "Resolving hostname: %s", APP_TCP_SERVER_HOST);
    
    // DNS lookup
    he = gethostbyname(APP_TCP_SERVER_HOST);
    if (he == NULL) {
        ESP_LOGW(TAG, "DNS resolution failed");
        return false;
    }

    // Create socket
    g_tcp_ctx.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_tcp_ctx.socket_fd < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return false;
    }

    // Set socket timeouts
    struct timeval tv = {
        .tv_sec = APP_TCP_CONNECT_TIMEOUT_MS / 1000,
        .tv_usec = (APP_TCP_CONNECT_TIMEOUT_MS % 1000) * 1000,
    };
    setsockopt(g_tcp_ctx.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Connect to server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(APP_TCP_SERVER_PORT);
    server_addr.sin_addr = *(struct in_addr *)he->h_addr;

    ESP_LOGI(TAG, "Connecting to %s:%d", APP_TCP_SERVER_HOST, APP_TCP_SERVER_PORT);
    
    if (connect(g_tcp_ctx.socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGW(TAG, "Connection failed");
        tcp_socket_close();
        return false;
    }

    ESP_LOGI(TAG, "Connected to server successfully");
    return true;
}

/**
 * @brief Process data from TCP server
 */
static void tcp_process_data(void)
{
    char rx_buffer[128] = {0};
    int len = 0;

    len = recv(g_tcp_ctx.socket_fd, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout, no data received (normal)
            return;
        }
        ESP_LOGI(TAG, "Socket receive error, reconnecting...");
        tcp_socket_close();
        return;
    } else if (len == 0) {
        // Server closed connection
        ESP_LOGI(TAG, "Server closed connection");
        tcp_socket_close();
        return;
    }

    rx_buffer[len] = '\0';
    ESP_LOGI(TAG, "Received from server: %s", (char *)rx_buffer);

    // Echo response back to server (optional)
    const char *msg = "ACK\r\n";
    send(g_tcp_ctx.socket_fd, msg, strlen(msg), 0);
}

/**
 * @brief Send keep-alive message to server
 */
static void tcp_send_keepalive(void)
{
    if (g_tcp_ctx.socket_fd < 0) {
        return;
    }

    const char *msg = "PING\r\n";
    if (send(g_tcp_ctx.socket_fd, msg, strlen(msg), 0) < 0) {
        ESP_LOGI(TAG, "Send failed, reconnecting...");
        tcp_socket_close();
    }
}

/**
 * @brief TCP client main task
 */
static void tcp_client_task(void *arg)
{
    app_tcp_cmd_t cmd;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    ESP_LOGI(TAG, "TCP client task started");

    while (1) {
        // Check for commands with timeout
        if (xQueueReceive(g_tcp_ctx.cmd_queue, &cmd, pdMS_TO_TICKS(5000)) == pdTRUE) {
            switch (cmd) {
                case APP_TCP_CMD_START:
                    ESP_LOGI(TAG, "Received START command");
                    g_tcp_ctx.running = true;
                    break;

                case APP_TCP_CMD_STOP:
                    ESP_LOGI(TAG, "Received STOP command");
                    tcp_socket_close();
                    g_tcp_ctx.running = false;
                    break;

                case APP_TCP_CMD_RECONNECT:
                    ESP_LOGI(TAG, "Received RECONNECT command");
                    tcp_socket_close();
                    break;

                default:
                    break;
            }
        }

        // If running and network is ready, maintain connection
        if (g_tcp_ctx.running && system_network_is_ready()) {
            if (g_tcp_ctx.socket_fd < 0) {
                // Try to connect
                if (!tcp_connect_to_server()) {
                    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait before retry
                    continue;
                }
            }

            // Process any incoming data
            tcp_process_data();

            // Send keep-alive every ~10 seconds
            if ((xTaskGetTickCount() - xLastWakeTime) > pdMS_TO_TICKS(10000)) {
                tcp_send_keepalive();
                xLastWakeTime = xTaskGetTickCount();
            }
        } else if (g_tcp_ctx.running && !system_network_is_ready()) {
            // Network not ready, close socket and wait
            tcp_socket_close();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/**
 * @brief Event handler for network manager events
 */
static void net_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == SYSTEM_NET_EVENT) {
        switch (event_id) {
            case SYSTEM_NET_EVENT_GOT_IP:
                ESP_LOGI(TAG, "Network ready, initiating connection...");
                if (g_tcp_ctx.running) {
                    app_tcp_cmd_t cmd = APP_TCP_CMD_RECONNECT;
                    xQueueSend(g_tcp_ctx.cmd_queue, &cmd, portMAX_DELAY);
                }
                break;

            case SYSTEM_NET_EVENT_LOST_IP:
            case SYSTEM_NET_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "Network lost");
                if (g_tcp_ctx.socket_fd >= 0) {
                    tcp_socket_close();
                }
                break;

            default:
                break;
        }
    }
}

esp_err_t app_tcp_client_init(void)
{
    ESP_LOGI(TAG, "Initializing TCP client application...");

    if (g_tcp_ctx.cmd_queue != NULL) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Create command queue
    g_tcp_ctx.cmd_queue = xQueueCreate(4, sizeof(app_tcp_cmd_t));
    if (g_tcp_ctx.cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create command queue");
        return ESP_FAIL;
    }

    // Register event handler for network events
    esp_event_handler_register(SYSTEM_NET_EVENT, ESP_EVENT_ANY_ID, net_event_handler, NULL);

    ESP_LOGI(TAG, "TCP client application initialized");
    return ESP_OK;
}

esp_err_t app_tcp_client_start(void)
{
    ESP_LOGI(TAG, "Starting TCP client...");

    if (g_tcp_ctx.cmd_queue == NULL) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_FAIL;
    }

    // Create task if not already created
    if (g_tcp_ctx.task_handle == NULL) {
        xTaskCreate(tcp_client_task, "tcp_client", APP_TCP_TASK_STACK_SIZE,
                   NULL, APP_TCP_TASK_PRIORITY, &g_tcp_ctx.task_handle);
        if (g_tcp_ctx.task_handle == NULL) {
            ESP_LOGE(TAG, "Failed to create TCP client task");
            return ESP_FAIL;
        }
    }

    // Send START command
    app_tcp_cmd_t cmd = APP_TCP_CMD_START;
    if (xQueueSend(g_tcp_ctx.cmd_queue, &cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send START command");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TCP client started");
    return ESP_OK;
}

esp_err_t app_tcp_client_stop(void)
{
    ESP_LOGI(TAG, "Stopping TCP client...");

    if (g_tcp_ctx.cmd_queue == NULL) {
        ESP_LOGW(TAG, "Not initialized");
        return ESP_OK;
    }

    // Send STOP command
    app_tcp_cmd_t cmd = APP_TCP_CMD_STOP;
    xQueueSend(g_tcp_ctx.cmd_queue, &cmd, pdMS_TO_TICKS(1000));

    // Wait a bit for task to finish
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "TCP client stopped");
    return ESP_OK;
}
