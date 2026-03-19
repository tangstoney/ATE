#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_EVENT_ANY = -1,
    SYSTEM_EVENT_EVT_UART_RX = 1,
    SYSTEM_EVENT_EVT_UART_TX_DONE,
    SYSTEM_EVENT_EVT_UART_TIMEOUT,
    SYSTEM_EVENT_EVT_I2C_XFER_DONE,
    SYSTEM_EVENT_EVT_I2C_TIMEOUT,
    SYSTEM_EVENT_EVT_DEVICE_ONLINE,
    SYSTEM_EVENT_EVT_DEVICE_OFFLINE,
    SYSTEM_EVENT_EVT_MODULE_DISCOVERED,
    SYSTEM_EVENT_EVT_PROTOCOL_ERROR,
    SYSTEM_EVENT_EVT_FORWARD_REQUEST,
    SYSTEM_EVENT_EVT_FORWARD_DONE,
    SYSTEM_EVENT_EVT_FAULT,
} system_event_id_t;

typedef struct system_event_subscription *system_event_subscription_handle_t;

typedef void (*system_event_handler_t)(void *handler_arg,
                                       system_event_id_t event_id,
                                       const void *event_data,
                                       size_t event_data_size);

esp_err_t system_event_init(void);
esp_err_t system_event_deinit(void);

esp_err_t system_event_subscribe(system_event_id_t event_id,
                                 system_event_handler_t handler,
                                 void *handler_arg,
                                 system_event_subscription_handle_t *out_handle);
esp_err_t system_event_unsubscribe(system_event_subscription_handle_t handle);

esp_err_t system_event_publish(system_event_id_t event_id,
                               const void *event_data,
                               size_t event_data_size,
                               int timeout_ms);

#ifdef __cplusplus
}
#endif
