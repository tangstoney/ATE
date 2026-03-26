#include "system_module.h"

#include <stdlib.h>
#include <string.h>

#include "board_ate_p4.h"
#include "driver_i2c_module.h"
#include "esp_check.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "system_module_protocol.h"

struct system_module_t {
    driver_i2c_module_handle_t driver_handle;
    SemaphoreHandle_t mutex;
    uint8_t protocol_device_id;
    module_status_t status;
};

static const char *TAG = "system_module";

static uint16_t status_error_code(const module_status_t *status)
{
    if (status->last_err != ESP_OK) {
        return (uint16_t)(status->last_err & 0xFFFF);
    }
    if (status->detail_status_code != SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS) {
        return status->detail_status_code;
    }
    return 0;
}

static system_module_state_t resolve_state(const module_status_t *status)
{
    if (!status->attached) {
        return SYSTEM_MODULE_STATE_UNKNOWN;
    }
    if (!status->online) {
        return SYSTEM_MODULE_STATE_OFFLINE;
    }
    if (status_error_code(status) != 0) {
        return SYSTEM_MODULE_STATE_ERROR;
    }
    return SYSTEM_MODULE_STATE_ONLINE;
}

static bool status_snapshot_equal(const module_status_t *lhs, const module_status_t *rhs)
{
    return lhs->state == rhs->state &&
           lhs->error_code == rhs->error_code &&
           lhs->online == rhs->online &&
           lhs->last_command_id == rhs->last_command_id &&
           lhs->detail_status_code == rhs->detail_status_code &&
           lhs->tx_count == rhs->tx_count &&
           lhs->rx_count == rhs->rx_count &&
           lhs->last_err == rhs->last_err;
}

static void publish_status_event(system_module_event_id_t event_id, const module_status_t *status)
{
    system_module_status_event_t event = {
        .state = (uint8_t)status->state,
        .error_code = status->error_code,
    };

    (void)esp_event_post(SYSTEM_MODULE_EVENT, event_id, &event, sizeof(event), portMAX_DELAY);
}

static void publish_command_done_event(uint8_t cmd, bool success)
{
    system_module_cmd_done_event_t event = {
        .cmd = cmd,
        .success = success,
    };

    (void)esp_event_post(SYSTEM_MODULE_EVENT, SYSTEM_MODULE_EVENT_COMMAND_DONE, &event, sizeof(event), portMAX_DELAY);
}

static void publish_status_transitions(const module_status_t *before, const module_status_t *after)
{
    if (!before->online && after->online) {
        publish_status_event(SYSTEM_MODULE_EVENT_ONLINE, after);
    } else if (before->online && !after->online) {
        publish_status_event(SYSTEM_MODULE_EVENT_OFFLINE, after);
    }

    if (!status_snapshot_equal(before, after)) {
        publish_status_event(SYSTEM_MODULE_EVENT_STATUS_UPDATED, after);
    }
}

static void finalize_status(system_module_handle_t handle)
{
    handle->status.error_code = status_error_code(&handle->status);
    handle->status.state = resolve_state(&handle->status);
}

static esp_err_t validate_board_module_config(void)
{
    const board_module_link_config_t *config = board_get_module_link_config();

    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_STATE, TAG, "module board config missing");
    ESP_RETURN_ON_FALSE(config->device_address != BOARD_I2C_ADDR_INVALID,
                        ESP_ERR_INVALID_STATE, TAG, "module i2c address not configured");
    ESP_RETURN_ON_FALSE(config->device_address <= 0x7F,
                        ESP_ERR_INVALID_STATE, TAG, "invalid module i2c address");
    return ESP_OK;
}

static esp_err_t write_request_frame(uint8_t protocol_device_id,
                                     uint8_t cmd,
                                     const uint8_t *payload,
                                     size_t len,
                                     uint8_t *out_buf,
                                     size_t out_buf_size,
                                     size_t *out_len)
{
    ESP_RETURN_ON_FALSE(len <= UINT16_MAX, ESP_ERR_INVALID_SIZE, TAG, "payload too large");
    return system_module_protocol_build_request(protocol_device_id,
                                                cmd,
                                                payload,
                                                (uint16_t)len,
                                                out_buf,
                                                out_buf_size,
                                                out_len);
}

esp_err_t system_module_create(system_module_handle_t *out_handle)
{
    system_module_handle_t handle = NULL;
    const board_module_link_config_t *config = board_get_module_link_config();
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    ESP_RETURN_ON_ERROR(validate_board_module_config(), TAG, "invalid module board config");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc handle failed");

    handle->mutex = xSemaphoreCreateMutex();
    if (!handle->mutex) {
        free(handle);
        return ESP_ERR_NO_MEM;
    }

    err = driver_i2c_module_bus_create();
    if (err != ESP_OK) {
        vSemaphoreDelete(handle->mutex);
        free(handle);
        return err;
    }

    err = driver_i2c_module_create(&handle->driver_handle);
    if (err != ESP_OK) {
        vSemaphoreDelete(handle->mutex);
        free(handle);
        return err;
    }

    handle->status.i2c_addr = config->device_address;
    handle->protocol_device_id = SYSTEM_MODULE_PROTOCOL_DEFAULT_DEVICE_ID;
    handle->status.attached = true;
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = ESP_OK;
    finalize_status(handle);

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t system_module_destroy(system_module_handle_t handle)
{
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    err = driver_i2c_module_destroy(handle->driver_handle);
    (void)driver_i2c_module_bus_destroy();

    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }
    free(handle);
    return err;
}

esp_err_t system_module_get_status(system_module_handle_t handle,
                                   module_status_t *out_status)
{
    ESP_RETURN_ON_FALSE(handle && out_status, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    *out_status = handle->status;

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

esp_err_t system_module_send_command(system_module_handle_t handle,
                                     uint8_t cmd,
                                     const uint8_t *payload,
                                     size_t len)
{
    uint8_t request_buf[SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN + SYSTEM_MODULE_PROTOCOL_MAX_PAYLOAD_LEN] = {0};
    uint8_t response_buf[SYSTEM_MODULE_PROTOCOL_STATUS_FRAME_LEN] = {0};
    size_t request_len = 0;
    size_t parsed_len = 0;
    system_module_protocol_frame_t frame = {0};
    module_status_t before = {0};
    module_status_t after = {0};
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    if (len > 0) {
        ESP_RETURN_ON_FALSE(payload, ESP_ERR_INVALID_ARG, TAG, "payload is NULL");
    }

    ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    before = handle->status;
    handle->status.last_command_id = cmd;

    err = write_request_frame(handle->protocol_device_id,
                              cmd,
                              payload,
                              len,
                              request_buf,
                              sizeof(request_buf),
                              &request_len);
    if (err != ESP_OK) {
        handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
        handle->status.last_err = err;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(&before, &after);
        publish_command_done_event(cmd, false);
        return err;
    }

    err = driver_i2c_module_write_read(handle->driver_handle,
                                       request_buf,
                                       request_len,
                                       response_buf,
                                       sizeof(response_buf));
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = err;
    if (err != ESP_OK) {
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(&before, &after);
        publish_command_done_event(cmd, false);
        return err;
    }

    err = system_module_protocol_parse(response_buf, sizeof(response_buf), &frame, &parsed_len);
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = err;
    if (err != ESP_OK) {
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(&before, &after);
        publish_command_done_event(cmd, false);
        return err;
    }

    if (!frame.is_status_frame || frame.command != cmd) {
        handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
        handle->status.last_err = ESP_ERR_INVALID_RESPONSE;
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(&before, &after);
        publish_command_done_event(cmd, false);
        return ESP_ERR_INVALID_RESPONSE;
    }

    handle->status.online = true;
    handle->status.tx_count++;
    handle->status.rx_count++;
    handle->status.detail_status_code = (uint16_t)frame.status;
    handle->status.last_err = ESP_OK;
    finalize_status(handle);
    after = handle->status;

    xSemaphoreGive(handle->mutex);

    publish_status_transitions(&before, &after);
    publish_command_done_event(cmd, frame.status == SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS);
    return ESP_OK;
}

esp_err_t system_module_is_online(system_module_handle_t handle,
                                  bool *out_online)
{
    module_status_t before = {0};
    module_status_t after = {0};
    esp_err_t probe_err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle && out_online, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    before = handle->status;
    probe_err = driver_i2c_module_probe(handle->driver_handle);
    handle->status.online = (probe_err == ESP_OK);
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = (probe_err == ESP_ERR_NOT_FOUND) ? ESP_OK : probe_err;
    finalize_status(handle);
    after = handle->status;
    *out_online = after.online;

    xSemaphoreGive(handle->mutex);

    publish_status_transitions(&before, &after);

    if (probe_err == ESP_ERR_NOT_FOUND) {
        return ESP_OK;
    }
    return probe_err;
}
