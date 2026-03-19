#include "system_router.h"

#include <string.h>
#include <stdlib.h>

#include "esp_check.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"

typedef struct system_router_subscription {
    uint32_t route_mask;
    system_router_handler_t handler;
    void *handler_arg;
    esp_event_handler_instance_t instance;
    struct system_router_subscription *next;
} system_router_subscription_t;

ESP_EVENT_DEFINE_BASE(SYSTEM_ROUTER_EVENT_BASE);

typedef enum {
    SYSTEM_ROUTER_EVENT_ROUTE = 1,
} system_router_event_id_t;

typedef struct {
    uint32_t route_mask;
    size_t payload_size;
    size_t metadata_size;
    uint8_t data[];
} system_router_event_envelope_t;

static const char *TAG = "system_router";
static esp_event_loop_handle_t s_loop;
static system_router_subscription_t *s_subscriptions;

static void system_router_dispatcher(void *handler_arg,
                                     esp_event_base_t event_base,
                                     int32_t event_id,
                                     void *event_data)
{
    system_router_subscription_t *subscription = (system_router_subscription_t *)handler_arg;
    const system_router_event_envelope_t *envelope = (const system_router_event_envelope_t *)event_data;
    const void *payload = NULL;
    const void *metadata = NULL;

    (void)event_base;

    if (!subscription || !subscription->handler || event_id != SYSTEM_ROUTER_EVENT_ROUTE || !envelope) {
        return;
    }

    if ((subscription->route_mask & envelope->route_mask) == 0) {
        return;
    }

    if (envelope->payload_size > 0) {
        payload = envelope->data;
    }
    if (envelope->metadata_size > 0) {
        metadata = envelope->data + envelope->payload_size;
    }

    subscription->handler(subscription->handler_arg,
                          envelope->route_mask,
                          payload,
                          envelope->payload_size,
                          metadata,
                          envelope->metadata_size);
}

esp_err_t system_router_init(void)
{
    esp_err_t ret = ESP_OK;

    if (s_loop) {
        return ESP_OK;
    }

    esp_event_loop_args_t loop_args = {
        .queue_size = 32,
        .task_name = "ate_router",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY,
    };

    ret = esp_event_loop_create(&loop_args, &s_loop);
    ESP_RETURN_ON_ERROR(ret, TAG, "esp_event_loop_create failed");
    return ESP_OK;
}

esp_err_t system_router_deinit(void)
{
    system_router_subscription_t *cursor = NULL;
    system_router_subscription_t *next = NULL;

    if (!s_loop) {
        return ESP_OK;
    }

    cursor = s_subscriptions;
    while (cursor) {
        next = cursor->next;
        esp_event_handler_instance_unregister_with(s_loop,
                                                   SYSTEM_ROUTER_EVENT_BASE,
                                                   SYSTEM_ROUTER_EVENT_ROUTE,
                                                   cursor->instance);
        free(cursor);
        cursor = next;
    }
    s_subscriptions = NULL;

    ESP_RETURN_ON_ERROR(esp_event_loop_delete(s_loop), TAG, "esp_event_loop_delete failed");
    s_loop = NULL;
    return ESP_OK;
}

esp_err_t system_router_subscribe(uint32_t route_mask,
                                  system_router_handler_t handler,
                                  void *handler_arg,
                                  system_router_subscription_handle_t *out_handle)
{
    system_router_subscription_t *subscription = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(route_mask != 0 && handler && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_router_init(), TAG, "system_router_init failed");

    subscription = calloc(1, sizeof(*subscription));
    ESP_RETURN_ON_FALSE(subscription, ESP_ERR_NO_MEM, TAG, "no memory");

    subscription->route_mask = route_mask;
    subscription->handler = handler;
    subscription->handler_arg = handler_arg;

    ret = esp_event_handler_instance_register_with(s_loop,
                                                   SYSTEM_ROUTER_EVENT_BASE,
                                                   SYSTEM_ROUTER_EVENT_ROUTE,
                                                   system_router_dispatcher,
                                                   subscription,
                                                   &subscription->instance);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "esp_event_handler_instance_register_with failed");

    subscription->next = s_subscriptions;
    s_subscriptions = subscription;

    *out_handle = subscription;
    return ESP_OK;

err:
    free(subscription);
    return ret;
}

esp_err_t system_router_unsubscribe(system_router_subscription_handle_t handle)
{
    system_router_subscription_t **cursor = NULL;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(s_loop, ESP_ERR_INVALID_STATE, TAG, "router not initialized");

    cursor = &s_subscriptions;
    while (*cursor && *cursor != handle) {
        cursor = &(*cursor)->next;
    }

    if (*cursor != handle) {
        return ESP_ERR_NOT_FOUND;
    }

    *cursor = handle->next;

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister_with(s_loop,
                                                                   SYSTEM_ROUTER_EVENT_BASE,
                                                                   SYSTEM_ROUTER_EVENT_ROUTE,
                                                                   handle->instance),
                        TAG, "esp_event_handler_instance_unregister_with failed");

    free(handle);
    return ESP_OK;
}

esp_err_t system_router_route(uint32_t route_mask,
                              const void *payload,
                              size_t payload_size,
                              const void *metadata,
                              size_t metadata_size)
{
    system_router_event_envelope_t *envelope = NULL;
    size_t event_size = sizeof(*envelope) + payload_size + metadata_size;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(route_mask != 0, ESP_ERR_INVALID_ARG, TAG, "route_mask must not be zero");
    ESP_RETURN_ON_ERROR(system_router_init(), TAG, "system_router_init failed");
    if (payload_size > 0) {
        ESP_RETURN_ON_FALSE(payload, ESP_ERR_INVALID_ARG, TAG, "payload is NULL");
    }
    if (metadata_size > 0) {
        ESP_RETURN_ON_FALSE(metadata, ESP_ERR_INVALID_ARG, TAG, "metadata is NULL");
    }

    envelope = calloc(1, event_size);
    ESP_RETURN_ON_FALSE(envelope, ESP_ERR_NO_MEM, TAG, "no memory");

    envelope->route_mask = route_mask;
    envelope->payload_size = payload_size;
    envelope->metadata_size = metadata_size;
    if (payload_size > 0) {
        memcpy(envelope->data, payload, payload_size);
    }
    if (metadata_size > 0) {
        memcpy(envelope->data + payload_size, metadata, metadata_size);
    }

    ret = esp_event_post_to(s_loop,
                            SYSTEM_ROUTER_EVENT_BASE,
                            SYSTEM_ROUTER_EVENT_ROUTE,
                            envelope,
                            event_size,
                            portMAX_DELAY);
    free(envelope);
    ESP_RETURN_ON_ERROR(ret, TAG, "esp_event_post_to failed");
    return ESP_OK;
}
