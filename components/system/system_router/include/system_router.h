#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_ROUTER_TARGET_UI             (1U << 0)
#define SYSTEM_ROUTER_TARGET_LOG            (1U << 1)
#define SYSTEM_ROUTER_TARGET_NETWORK        (1U << 2)
#define SYSTEM_ROUTER_TARGET_STATE_MACHINE  (1U << 3)

typedef struct system_router_subscription *system_router_subscription_handle_t;

typedef void (*system_router_handler_t)(void *handler_arg,
                                        uint32_t route_mask,
                                        const void *payload,
                                        size_t payload_size,
                                        const void *metadata,
                                        size_t metadata_size);

esp_err_t system_router_init(void);
esp_err_t system_router_deinit(void);
esp_err_t system_router_subscribe(uint32_t route_mask,
                                  system_router_handler_t handler,
                                  void *handler_arg,
                                  system_router_subscription_handle_t *out_handle);
esp_err_t system_router_unsubscribe(system_router_subscription_handle_t handle);
esp_err_t system_router_route(uint32_t route_mask,
                              const void *payload,
                              size_t payload_size,
                              const void *metadata,
                              size_t metadata_size);

#ifdef __cplusplus
}
#endif
