#include "system_module.h"

#include <inttypes.h>
#include <stdio.h>
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
    bool event_seeded;
    TickType_t last_bus_activity_tick;
    module_status_t status;
};

#define SYSTEM_MODULE_RESPONSE_WINDOW_LEN 32U
#define SYSTEM_MODULE_PACKET_INTERVAL_MS 50U
#define SYSTEM_MODULE_RESPONSE_POLL_RETRY_COUNT 4U

static const char *TAG = "system_module";
static const char *SYSTEM_MODULE_DEFAULT_TYPE = "tested_module";
static const char *SYSTEM_MODULE_DEFAULT_REVISION = "unknown";

static void copy_text(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) {
        return;
    }

    if (!src) {
        dst[0] = '\0';
        return;
    }

    snprintf(dst, dst_size, "%s", src);
}

static void log_frame_hex(const char *label, const uint8_t *buf, size_t len)
{
    char line[(SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN +
               SYSTEM_MODULE_PROTOCOL_MAX_PAYLOAD_LEN) * 3U + 1U] = {0};
    size_t pos = 0;

    if (!buf || len == 0U) {
        ESP_LOGI(TAG, "%s: <empty>", label);
        return;
    }

    for (size_t i = 0; i < len && (pos + 4U) < sizeof(line); ++i) {
        int written = snprintf(&line[pos], sizeof(line) - pos, "%02X%s", buf[i], (i + 1U) < len ? " " : "");
        if (written <= 0) {
            break;
        }
        pos += (size_t)written;
    }

    ESP_LOGI(TAG, "%s (%uB): %s", label, (unsigned int)len, line);
}

static TickType_t system_module_packet_interval_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SYSTEM_MODULE_PACKET_INTERVAL_MS);

    if (ticks == 0) {
        ticks = 1;
    }

    return ticks;
}

static void wait_min_packet_interval(TickType_t *last_bus_activity_tick)
{
    TickType_t min_gap = system_module_packet_interval_ticks();
    TickType_t now = 0;
    TickType_t elapsed = 0;

    if (!last_bus_activity_tick || *last_bus_activity_tick == 0) {
        return;
    }

    now = xTaskGetTickCount();
    elapsed = now - *last_bus_activity_tick;
    if (elapsed < min_gap) {
        vTaskDelay(min_gap - elapsed);
    }
}

static void mark_bus_activity(TickType_t *last_bus_activity_tick)
{
    if (!last_bus_activity_tick) {
        return;
    }

    *last_bus_activity_tick = xTaskGetTickCount();
}

static esp_err_t read_status_frame_with_poll(driver_i2c_module_handle_t driver_handle,
                                             TickType_t *last_bus_activity_tick,
                                             uint8_t *out_status_frame,
                                             size_t out_status_frame_len,
                                             size_t *out_frame_len)
{
    uint8_t response_window[SYSTEM_MODULE_RESPONSE_WINDOW_LEN] = {0};
    esp_err_t err = ESP_ERR_INVALID_RESPONSE;

    ESP_RETURN_ON_FALSE(driver_handle, ESP_ERR_INVALID_ARG, TAG, "driver_handle is NULL");
    ESP_RETURN_ON_FALSE(out_status_frame, ESP_ERR_INVALID_ARG, TAG, "out_status_frame is NULL");
    ESP_RETURN_ON_FALSE(out_status_frame_len >= SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN,
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "response frame buffer too small");
    ESP_RETURN_ON_FALSE(out_frame_len, ESP_ERR_INVALID_ARG, TAG, "out_frame_len is NULL");

    for (uint32_t attempt = 0; attempt < SYSTEM_MODULE_RESPONSE_POLL_RETRY_COUNT; ++attempt) {
        wait_min_packet_interval(last_bus_activity_tick);

        memset(response_window, 0, sizeof(response_window));
        err = driver_i2c_module_read(driver_handle, response_window, sizeof(response_window));
        mark_bus_activity(last_bus_activity_tick);
        if (err != ESP_OK) {
            ESP_LOGW(TAG,
                     "module response read failed: attempt=%u err=%s",
                     (unsigned int)(attempt + 1U),
                     esp_err_to_name(err));
            continue;
        }

        log_frame_hex("module response window", response_window, sizeof(response_window));

        for (size_t offset = 0; offset + SYSTEM_MODULE_PROTOCOL_STATUS_FRAME_LEN <= sizeof(response_window); ++offset) {
            system_module_protocol_frame_t frame = {0};
            size_t frame_len = 0U;

            if (response_window[offset] != (uint8_t)(SYSTEM_MODULE_PROTOCOL_FRAME_HEADER >> 8) ||
                response_window[offset + 1U] != (uint8_t)(SYSTEM_MODULE_PROTOCOL_FRAME_HEADER & 0xFF)) {
                continue;
            }

            err = system_module_protocol_parse(&response_window[offset],
                                               sizeof(response_window) - offset,
                                               &frame,
                                               &frame_len);
            if (err != ESP_OK) {
                continue;
            }

            ESP_RETURN_ON_FALSE(frame_len <= out_status_frame_len,
                                ESP_ERR_INVALID_SIZE,
                                TAG,
                                "parsed frame exceeds output buffer");
            memcpy(out_status_frame, &response_window[offset], frame_len);
            *out_frame_len = frame_len;
            log_frame_hex("module response frame", out_status_frame, frame_len);
            return ESP_OK;
        }
    }

    return err;
}

static void fill_identity(module_status_t *status, uint8_t protocol_device_id)
{
    char module_name[SYSTEM_MODULE_NAME_MAX_LEN] = {0};

    if (!status) {
        return;
    }

    snprintf(status->module_id, sizeof(status->module_id), "module_%02X", protocol_device_id);
    snprintf(module_name, sizeof(module_name), "Test Module %02X", protocol_device_id);
    copy_text(status->module_name, sizeof(status->module_name), module_name);
    copy_text(status->module_type, sizeof(status->module_type), SYSTEM_MODULE_DEFAULT_TYPE);
    copy_text(status->revision, sizeof(status->revision), SYSTEM_MODULE_DEFAULT_REVISION);
}

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
    return strncmp(lhs->module_id, rhs->module_id, sizeof(lhs->module_id)) == 0 &&
           strncmp(lhs->module_name, rhs->module_name, sizeof(lhs->module_name)) == 0 &&
           strncmp(lhs->module_type, rhs->module_type, sizeof(lhs->module_type)) == 0 &&
           strncmp(lhs->revision, rhs->revision, sizeof(lhs->revision)) == 0 &&
           lhs->state == rhs->state &&
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
        .online = status->online,
        .error_code = status->error_code,
        .detail_status_code = status->detail_status_code,
    };

    copy_text(event.module_id, sizeof(event.module_id), status->module_id);
    copy_text(event.module_name, sizeof(event.module_name), status->module_name);
    copy_text(event.module_type, sizeof(event.module_type), status->module_type);
    copy_text(event.revision, sizeof(event.revision), status->revision);
    (void)esp_event_post(SYSTEM_MODULE_EVENT, event_id, &event, sizeof(event), portMAX_DELAY);
}

static void publish_command_done_event(const module_status_t *status, uint8_t cmd, bool success)
{
    system_module_cmd_done_event_t event = {
        .cmd = cmd,
        .success = success,
        .detail_status_code = status ? status->detail_status_code : 0,
    };

    if (status) {
        copy_text(event.module_id, sizeof(event.module_id), status->module_id);
        copy_text(event.module_name, sizeof(event.module_name), status->module_name);
        copy_text(event.module_type, sizeof(event.module_type), status->module_type);
        copy_text(event.revision, sizeof(event.revision), status->revision);
    }

    (void)esp_event_post(SYSTEM_MODULE_EVENT, SYSTEM_MODULE_EVENT_COMMAND_DONE, &event, sizeof(event), portMAX_DELAY);
}

static void publish_status_transitions(system_module_handle_t handle,
                                       const module_status_t *before,
                                       const module_status_t *after)
{
    bool seeded_now = false;

    if (!handle->event_seeded) {
        publish_status_event(SYSTEM_MODULE_EVENT_STATUS_UPDATED, after);
        handle->event_seeded = true;
        seeded_now = true;
    }

    if (!before->online && after->online) {
        publish_status_event(SYSTEM_MODULE_EVENT_ONLINE, after);
    } else if (before->online && !after->online) {
        publish_status_event(SYSTEM_MODULE_EVENT_OFFLINE, after);
    }

    if (!seeded_now && !status_snapshot_equal(before, after)) {
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
        (void)driver_i2c_module_bus_destroy();
        vSemaphoreDelete(handle->mutex);
        free(handle);
        return err;
    }

    handle->status.i2c_addr = config->device_address;
    handle->protocol_device_id = SYSTEM_MODULE_PROTOCOL_DEFAULT_DEVICE_ID;
    fill_identity(&handle->status, handle->protocol_device_id);
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
    uint8_t response_buf[SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN + SYSTEM_MODULE_PROTOCOL_MAX_PAYLOAD_LEN] = {0};
    size_t request_len = 0;
    size_t parsed_len = 0;
    size_t response_len = 0;
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
        publish_status_transitions(handle, &before, &after);
        publish_command_done_event(&after, cmd, false);
        return err;
    }

    ESP_LOGI(TAG,
             "send business command: cmd=0x%02X i2c_addr=0x%02X payload_len=%u",
             cmd,
             (unsigned int)handle->status.i2c_addr,
             (unsigned int)len);
    log_frame_hex("module request frame", request_buf, request_len);

    wait_min_packet_interval(&handle->last_bus_activity_tick);
    err = driver_i2c_module_write(handle->driver_handle, request_buf, request_len);
    mark_bus_activity(&handle->last_bus_activity_tick);
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = err;
    if (err != ESP_OK) {
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(handle, &before, &after);
        publish_command_done_event(&after, cmd, false);
        return err;
    }

    err = read_status_frame_with_poll(handle->driver_handle,
                                      &handle->last_bus_activity_tick,
                                      response_buf,
                                      sizeof(response_buf),
                                      &response_len);
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = err;
    if (err != ESP_OK) {
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(handle, &before, &after);
        publish_command_done_event(&after, cmd, false);
        return err;
    }

    err = system_module_protocol_parse(response_buf, response_len, &frame, &parsed_len);
    handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    handle->status.last_err = err;
    if (err != ESP_OK) {
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(handle, &before, &after);
        publish_command_done_event(&after, cmd, false);
        return err;
    }

    if (frame.command != cmd) {
        handle->status.last_err = ESP_ERR_INVALID_RESPONSE;
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(handle, &before, &after);
        publish_command_done_event(&after, cmd, false);
        return ESP_ERR_INVALID_RESPONSE;
    }

    handle->status.online = true;
    handle->status.tx_count++;
    handle->status.rx_count++;
    if (frame.is_status_frame) {
        handle->status.detail_status_code = (uint16_t)frame.status;
    } else if (frame.frame_type == SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_RESPONSE) {
        handle->status.detail_status_code = SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS;
    } else {
        handle->status.last_err = ESP_ERR_INVALID_RESPONSE;
        handle->status.online = false;
        finalize_status(handle);
        after = handle->status;
        xSemaphoreGive(handle->mutex);
        publish_status_transitions(handle, &before, &after);
        publish_command_done_event(&after, cmd, false);
        return ESP_ERR_INVALID_RESPONSE;
    }
    handle->status.last_err = ESP_OK;
    finalize_status(handle);
    after = handle->status;

    xSemaphoreGive(handle->mutex);

    publish_status_transitions(handle, &before, &after);
    publish_command_done_event(&after,
                               cmd,
                               frame.is_status_frame ?
                                   (frame.status == SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS) :
                                   true);
    if (!frame.is_status_frame && frame.data_len > 0U && frame.data) {
        log_frame_hex("module response payload", frame.data, frame.data_len);
    }
    ESP_LOGI(TAG,
             "command result: cmd=0x%02X frame_type=0x%02X status=0x%02X success=%d online=%d error_code=0x%04X detail_status=0x%04X response_len=%u",
             cmd,
             (unsigned int)frame.frame_type,
             (unsigned int)(frame.is_status_frame ? frame.status : SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS),
             frame.is_status_frame ? (frame.status == SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS) : true,
             after.online,
             after.error_code,
             after.detail_status_code,
             (unsigned int)response_len);
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

    ESP_LOGI(TAG,
             "module probe result: i2c_addr=0x%02X speed_hz=%" PRIu32 " probe_err=%s online=%d state=%d error_code=0x%04X detail_status=0x%04X",
             (unsigned int)after.i2c_addr,
             (uint32_t)BOARD_I2C_MASTER_CLK_HZ,
             esp_err_to_name(probe_err),
             after.online,
             after.state,
             after.error_code,
             after.detail_status_code);

    publish_status_transitions(handle, &before, &after);

    if (probe_err == ESP_ERR_NOT_FOUND) {
        return ESP_OK;
    }
    return probe_err;
}
