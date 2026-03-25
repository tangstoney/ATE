#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_ATE_GATEWAY_PAYLOAD_MAX_LEN 128

typedef struct app_ate_gateway *app_ate_gateway_handle_t;

ESP_EVENT_DECLARE_BASE(APP_ATE_GATEWAY_EVENT);

typedef enum {
    APP_ATE_GATEWAY_CONNECTION_STATE_DISCONNECTED = 0,
    APP_ATE_GATEWAY_CONNECTION_STATE_CONNECTING,
    APP_ATE_GATEWAY_CONNECTION_STATE_CONNECTED,
    APP_ATE_GATEWAY_CONNECTION_STATE_READY,
    APP_ATE_GATEWAY_CONNECTION_STATE_FAULT,
} app_ate_gateway_connection_state_t;

typedef enum {
    APP_ATE_GATEWAY_EVENT_STARTED = 0,
    APP_ATE_GATEWAY_EVENT_CONNECTION_CHANGED,
    APP_ATE_GATEWAY_EVENT_SERVER_COMMAND_ROUTED,
    APP_ATE_GATEWAY_EVENT_DEVICE_DATA_FORWARDED,
} app_ate_gateway_event_id_t;

typedef enum {
    APP_ATE_GATEWAY_BUS_EVENT_NOTIFY = 0,
    APP_ATE_GATEWAY_BUS_EVENT_DEVICE_COMMAND,
    APP_ATE_GATEWAY_BUS_EVENT_SERVER_DATA,
} app_ate_gateway_bus_event_id_t;

typedef struct {
    uint16_t command_id;
    size_t len;
    uint8_t payload[APP_ATE_GATEWAY_PAYLOAD_MAX_LEN];
} app_ate_gateway_server_command_t;

typedef struct {
    uint16_t source_id;
    size_t len;
    uint8_t payload[APP_ATE_GATEWAY_PAYLOAD_MAX_LEN];
} app_ate_gateway_device_data_t;

typedef struct {
    uint32_t session_id;
    uint16_t command_id;
    size_t len;
    uint8_t payload[APP_ATE_GATEWAY_PAYLOAD_MAX_LEN];
} app_ate_gateway_device_command_t;

typedef struct {
    uint32_t session_id;
    size_t len;
    uint8_t payload[APP_ATE_GATEWAY_PAYLOAD_MAX_LEN];
} app_ate_gateway_server_data_t;

typedef struct {
    app_ate_gateway_connection_state_t state;
    bool server_online;
    bool device_online;
    esp_err_t result;
} app_ate_gateway_connection_event_t;

typedef struct {
    app_ate_gateway_event_id_t event_id;
    app_ate_gateway_connection_state_t connection_state;
    uint32_t session_id;
    bool server_online;
    bool device_online;
    esp_err_t result;
    size_t payload_len;
} app_ate_gateway_event_t;

esp_err_t app_ate_gateway_init(app_ate_gateway_handle_t *out_handle);
esp_err_t app_ate_gateway_deinit(app_ate_gateway_handle_t handle);

esp_err_t app_ate_gateway_on_server_command(app_ate_gateway_handle_t handle,
                                            const app_ate_gateway_server_command_t *command);
esp_err_t app_ate_gateway_on_device_data(app_ate_gateway_handle_t handle,
                                         const app_ate_gateway_device_data_t *device_data);
esp_err_t app_ate_gateway_on_connection_event(app_ate_gateway_handle_t handle,
                                              const app_ate_gateway_connection_event_t *event);
esp_err_t app_ate_gateway_get_connection_state(app_ate_gateway_handle_t handle,
                                               app_ate_gateway_connection_state_t *out_state);

#ifdef __cplusplus
}
#endif
