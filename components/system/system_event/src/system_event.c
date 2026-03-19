#include "system_event.h"

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

ESP_EVENT_DEFINE_BASE(SYSTEM_EVENT_BASE);

typedef struct {
    size_t data_size;
    uint8_t data[];
} system_event_envelope_t;

typedef struct system_event_subscription {
    system_event_id_t event_id;
    system_event_handler_t handler;
    void *handler_arg;
    esp_event_handler_instance_t instance;
    struct system_event_subscription *next;
} system_event_subscription_t;

static const char *TAG = "system_event";
static esp_event_loop_handle_t s_loop;
static system_event_subscription_t *s_subscriptions;

static void system_event_dispatcher(void *handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void *event_data)
{
    system_event_subscription_t *subscription = (system_event_subscription_t *)handler_arg;
    const system_event_envelope_t *envelope = (const system_event_envelope_t *)event_data;
    const void *payload = NULL;
    size_t payload_size = 0;

    (void)event_base;

    if (!subscription || !subscription->handler) {
        return;
    }

    if (envelope) {
        payload = envelope->data;
        payload_size = envelope->data_size;
    }

    subscription->handler(subscription->handler_arg, (system_event_id_t)event_id, payload, payload_size);
}

esp_err_t system_event_init(void)
{
    esp_err_t ret = ESP_OK;

    if (s_loop) {
        return ESP_OK;
    }

    esp_event_loop_args_t loop_args = {
        .queue_size = 32,
        .task_name = "ate_evt",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY,
    };

    ret = esp_event_loop_create(&loop_args, &s_loop);
    ESP_RETURN_ON_ERROR(ret, TAG, "esp_event_loop_create failed");
    return ESP_OK;
}

esp_err_t system_event_deinit(void)
{
    system_event_subscription_t *cursor = NULL;
    system_event_subscription_t *next = NULL;

    if (!s_loop) {
        return ESP_OK;
    }

    cursor = s_subscriptions;
    while (cursor) {
        next = cursor->next;
        esp_event_handler_instance_unregister_with(s_loop,
                                                   SYSTEM_EVENT_BASE,
                                                   cursor->event_id == SYSTEM_EVENT_ANY ? ESP_EVENT_ANY_ID : cursor->event_id,
                                                   cursor->instance);
        free(cursor);
        cursor = next;
    }
    s_subscriptions = NULL;

    ESP_RETURN_ON_ERROR(esp_event_loop_delete(s_loop), TAG, "esp_event_loop_delete failed");
    s_loop = NULL;
    return ESP_OK;
}

esp_err_t system_event_subscribe(system_event_id_t event_id,
                                 system_event_handler_t handler,
                                 void *handler_arg,
                                 system_event_subscription_handle_t *out_handle)
{
    system_event_subscription_t *subscription = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(handler && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_event_init(), TAG, "system_event_init failed");

    subscription = calloc(1, sizeof(*subscription));
    ESP_RETURN_ON_FALSE(subscription, ESP_ERR_NO_MEM, TAG, "no memory");

    subscription->event_id = event_id;
    subscription->handler = handler;
    subscription->handler_arg = handler_arg;

    ret = esp_event_handler_instance_register_with(s_loop,
                                                   SYSTEM_EVENT_BASE,
                                                   event_id == SYSTEM_EVENT_ANY ? ESP_EVENT_ANY_ID : event_id,
                                                   system_event_dispatcher,
                                                   subscription,
                                                   &subscription->instance);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "esp_event_handler_instance_register_with failed");

    subscription->next = s_subscriptions;
    s_subscriptions = subscription;

    *out_handle = subscription;
    return ESP_OK;

err:
    if (subscription && subscription->instance) {
        esp_event_handler_instance_unregister_with(s_loop,
                                                   SYSTEM_EVENT_BASE,
                                                   event_id == SYSTEM_EVENT_ANY ? ESP_EVENT_ANY_ID : event_id,
                                                   subscription->instance);
    }
    free(subscription);
    return ret;
}

esp_err_t system_event_unsubscribe(system_event_subscription_handle_t handle)
{
    system_event_subscription_t **cursor = NULL;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(s_loop, ESP_ERR_INVALID_STATE, TAG, "event loop not ready");

    cursor = &s_subscriptions;
    while (*cursor && *cursor != handle) {
        cursor = &(*cursor)->next;
    }

    if (*cursor != handle) {
        return ESP_ERR_NOT_FOUND;
    }

    *cursor = handle->next;

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister_with(s_loop,
                                                                   SYSTEM_EVENT_BASE,
                                                                   handle->event_id == SYSTEM_EVENT_ANY ? ESP_EVENT_ANY_ID : handle->event_id,
                                                                   handle->instance),
                        TAG, "esp_event_handler_unregister_with failed");

    free(handle);
    return ESP_OK;
}

esp_err_t system_event_publish(system_event_id_t event_id,
                               const void *event_data,
                               size_t event_data_size,
                               int timeout_ms)
{
    system_event_envelope_t *envelope = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_ERROR(system_event_init(), TAG, "system_event_init failed");
    ESP_RETURN_ON_FALSE(event_id > SYSTEM_EVENT_ANY, ESP_ERR_INVALID_ARG, TAG, "invalid event id");
    if (event_data_size > 0) {
        ESP_RETURN_ON_FALSE(event_data, ESP_ERR_INVALID_ARG, TAG, "event_data is NULL");
    }

    if (event_data_size > 0) {
        envelope = malloc(sizeof(*envelope) + event_data_size);
        ESP_RETURN_ON_FALSE(envelope, ESP_ERR_NO_MEM, TAG, "no memory");
        envelope->data_size = event_data_size;
        memcpy(envelope->data, event_data, event_data_size);
    }

    err = esp_event_post_to(s_loop,
                            SYSTEM_EVENT_BASE,
                            event_id,
                            envelope,
                            envelope ? sizeof(*envelope) + event_data_size : 0,
                            timeout_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms));
    free(envelope);
    ESP_RETURN_ON_ERROR(err, TAG, "esp_event_post_to failed");
    return ESP_OK;
}
