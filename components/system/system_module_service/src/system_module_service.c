#include "system_module_service.h"

#include <string.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "system_comm_mgr.h"
#include "system_event.h"
#include "system_fault.h"

typedef struct {
    bool used;
    driver_i2c_device_handle_t dev_handle;
    system_module_info_t info;
} system_module_slot_t;

typedef struct {
    bool initialized;
    system_module_service_config_t config;
    driver_i2c_master_handle_t bus;
    // Serialize a full module-bus transaction so the active path cannot change mid-transfer.
    SemaphoreHandle_t mutex;
    system_module_slot_t slots[SYSTEM_MODULE_SERVICE_MAX_MODULES];
} system_module_service_ctx_t;

static const char *TAG = "system_module_service";
static system_module_service_ctx_t s_ctx;

static uint16_t normalize_scan_start(uint16_t addr)
{
    return addr ? addr : 0x08;
}

static uint16_t normalize_scan_end(uint16_t addr)
{
    return addr ? addr : 0x77;
}

static int normalize_io_timeout_ms(int timeout_ms)
{
    return timeout_ms > 0 ? timeout_ms : 1000;
}

static system_module_slot_t *find_slot_by_addr(uint16_t i2c_addr)
{
    for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
        if (s_ctx.slots[i].used && s_ctx.slots[i].info.i2c_addr == i2c_addr) {
            return &s_ctx.slots[i];
        }
    }
    return NULL;
}

static system_module_slot_t *alloc_slot(uint16_t i2c_addr)
{
    system_module_slot_t *slot = find_slot_by_addr(i2c_addr);
    if (slot) {
        return slot;
    }

    for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
        if (!s_ctx.slots[i].used) {
            memset(&s_ctx.slots[i], 0, sizeof(s_ctx.slots[i]));
            s_ctx.slots[i].used = true;
            s_ctx.slots[i].info.i2c_addr = i2c_addr;
            s_ctx.slots[i].info.last_status = SYSTEM_PROTOCOL_STATUS_SUCCESS;
            return &s_ctx.slots[i];
        }
    }
    return NULL;
}

static uint8_t resolve_protocol_device_id(system_module_slot_t *slot, uint8_t protocol_device_id)
{
    if (protocol_device_id != 0) {
        return protocol_device_id;
    }
    if (slot && slot->info.protocol_device_id != 0) {
        return slot->info.protocol_device_id;
    }
    return s_ctx.config.default_protocol_device_id;
}

static esp_err_t ensure_attached(system_module_slot_t *slot, uint8_t protocol_device_id)
{
    driver_i2c_device_config_t dev_cfg = {
        .dev_addr = slot->info.i2c_addr,
        .scl_speed_hz = s_ctx.config.device_speed_hz,
        .timeout_ms = normalize_io_timeout_ms(s_ctx.config.io_timeout_ms),
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
    };

    if (slot->dev_handle) {
        slot->info.attached = true;
        if (protocol_device_id != 0) {
            slot->info.protocol_device_id = protocol_device_id;
        }
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(driver_i2c_device_add(s_ctx.bus, &dev_cfg, &slot->dev_handle),
                        TAG, "driver_i2c_device_add failed");
    slot->info.attached = true;
    slot->info.protocol_device_id = resolve_protocol_device_id(slot, protocol_device_id);
    return ESP_OK;
}

static void publish_module_presence_event(system_event_id_t event_id, const system_module_info_t *info)
{
    (void)system_event_publish(event_id, info, sizeof(*info), 0);
}

static void report_link_error(system_fault_source_t source, esp_err_t err, uint32_t detail0, uint32_t detail1)
{
    (void)system_fault_report(source, system_fault_from_esp_err(err), err, detail0, detail1);
}

esp_err_t system_module_service_init(const system_module_service_config_t *config)
{
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    if (s_ctx.initialized) {
        return ESP_OK;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.config = *config;
    s_ctx.config.scan_start_addr = normalize_scan_start(config->scan_start_addr);
    s_ctx.config.scan_end_addr = normalize_scan_end(config->scan_end_addr);
    s_ctx.config.io_timeout_ms = normalize_io_timeout_ms(config->io_timeout_ms);
    ESP_RETURN_ON_FALSE(s_ctx.config.scan_start_addr <= s_ctx.config.scan_end_addr,
                        ESP_ERR_INVALID_ARG, TAG, "invalid scan range");

    s_ctx.mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_ctx.mutex, ESP_ERR_NO_MEM, TAG, "xSemaphoreCreateMutex failed");

    err = system_comm_mgr_get_i2c_link(config->link_id, &s_ctx.bus);
    if (err != ESP_OK) {
        vSemaphoreDelete(s_ctx.mutex);
        memset(&s_ctx, 0, sizeof(s_ctx));
        return err;
    }
    s_ctx.initialized = true;
    return ESP_OK;
}

esp_err_t system_module_service_deinit(void)
{
    if (!s_ctx.initialized) {
        return ESP_OK;
    }

    if (s_ctx.mutex && xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE) {
        for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
            if (s_ctx.slots[i].dev_handle) {
                (void)driver_i2c_device_remove(s_ctx.slots[i].dev_handle);
            }
        }
        xSemaphoreGive(s_ctx.mutex);
        vSemaphoreDelete(s_ctx.mutex);
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    return ESP_OK;
}

esp_err_t system_module_scan(void)
{
    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
        if (s_ctx.slots[i].used) {
            s_ctx.slots[i].info.online = false;
        }
    }

    for (uint16_t addr = s_ctx.config.scan_start_addr; addr <= s_ctx.config.scan_end_addr; ++addr) {
        esp_err_t err = driver_i2c_master_probe(s_ctx.bus, addr, s_ctx.config.io_timeout_ms);
        if (err == ESP_OK) {
            system_module_slot_t *slot = alloc_slot(addr);
            if (!slot) {
                xSemaphoreGive(s_ctx.mutex);
                return ESP_ERR_NO_MEM;
            }
            bool was_online = slot->info.online;
            slot->info.online = true;
            slot->info.last_err = ESP_OK;
            if (!was_online) {
                publish_module_presence_event(SYSTEM_EVENT_EVT_MODULE_DISCOVERED, &slot->info);
                publish_module_presence_event(SYSTEM_EVENT_EVT_DEVICE_ONLINE, &slot->info);
            }
        }
    }

    for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
        if (s_ctx.slots[i].used && !s_ctx.slots[i].info.online) {
            publish_module_presence_event(SYSTEM_EVENT_EVT_DEVICE_OFFLINE, &s_ctx.slots[i].info);
        }
    }

    xSemaphoreGive(s_ctx.mutex);
    return ESP_OK;
}

esp_err_t system_module_get_count(size_t *out_count)
{
    size_t count = 0;

    ESP_RETURN_ON_FALSE(out_count, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
        if (s_ctx.slots[i].used) {
            ++count;
        }
    }
    xSemaphoreGive(s_ctx.mutex);

    *out_count = count;
    return ESP_OK;
}

esp_err_t system_module_get_info(size_t index, system_module_info_t *out_info)
{
    size_t seen = 0;

    ESP_RETURN_ON_FALSE(out_info, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    for (size_t i = 0; i < SYSTEM_MODULE_SERVICE_MAX_MODULES; ++i) {
        if (!s_ctx.slots[i].used) {
            continue;
        }
        if (seen == index) {
            *out_info = s_ctx.slots[i].info;
            xSemaphoreGive(s_ctx.mutex);
            return ESP_OK;
        }
        ++seen;
    }

    xSemaphoreGive(s_ctx.mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t system_module_attach(uint16_t i2c_addr, uint8_t protocol_device_id)
{
    system_module_slot_t *slot = NULL;

    ESP_RETURN_ON_FALSE(i2c_addr <= 0x7F, ESP_ERR_INVALID_ARG, TAG, "invalid I2C address");
    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    slot = alloc_slot(i2c_addr);
    if (!slot) {
        xSemaphoreGive(s_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = ensure_attached(slot, protocol_device_id);
    slot->info.last_err = err;
    if (err == ESP_OK) {
        slot->info.online = true;
    }
    xSemaphoreGive(s_ctx.mutex);

    if (err != ESP_OK) {
        report_link_error(SYSTEM_FAULT_SOURCE_MODULE_SERVICE, err, i2c_addr, protocol_device_id);
    }
    return err;
}

esp_err_t system_module_send_cmd(uint16_t i2c_addr,
                                 uint8_t protocol_device_id,
                                 uint8_t command,
                                 const uint8_t *payload,
                                 uint16_t payload_len)
{
    uint8_t frame_buf[SYSTEM_PROTOCOL_GENERAL_FRAME_BASE_LEN + SYSTEM_PROTOCOL_MAX_PAYLOAD_LEN] = {0};
    size_t frame_len = 0;
    system_module_slot_t *slot = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    if (payload_len > 0) {
        ESP_RETURN_ON_FALSE(payload, ESP_ERR_INVALID_ARG, TAG, "payload is NULL");
    }

    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
    slot = alloc_slot(i2c_addr);
    if (!slot) {
        xSemaphoreGive(s_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }

    protocol_device_id = resolve_protocol_device_id(slot, protocol_device_id);
    if (protocol_device_id == 0) {
        err = ESP_ERR_INVALID_ARG;
        goto err_unlock;
    }

    err = ensure_attached(slot, protocol_device_id);
    if (err != ESP_OK) {
        goto err_unlock;
    }

    err = system_protocol_build_request(protocol_device_id,
                                        command,
                                        payload,
                                        payload_len,
                                        frame_buf,
                                        sizeof(frame_buf),
                                        &frame_len);
    if (err != ESP_OK) {
        goto err_unlock;
    }

    err = driver_i2c_write(slot->dev_handle, frame_buf, frame_len);
    slot->info.last_command = command;
    slot->info.last_err = err;
    if (err == ESP_OK) {
        slot->info.tx_count++;
        slot->info.online = true;
    }
    xSemaphoreGive(s_ctx.mutex);

    system_module_xfer_event_t evt = {
        .i2c_addr = i2c_addr,
        .protocol_device_id = protocol_device_id,
        .command = command,
        .tx_len = frame_len,
        .rx_len = 0,
        .result = err,
    };
    (void)system_event_publish(err == ESP_ERR_TIMEOUT ? SYSTEM_EVENT_EVT_I2C_TIMEOUT : SYSTEM_EVENT_EVT_I2C_XFER_DONE,
                               &evt, sizeof(evt), 0);

    if (err != ESP_OK) {
        report_link_error(SYSTEM_FAULT_SOURCE_MODULE_SERVICE, err, i2c_addr, command);
    }
    return err;

err_unlock:
    slot->info.last_err = err;
    xSemaphoreGive(s_ctx.mutex);
    return err;
}

esp_err_t system_module_exec_action(uint16_t i2c_addr,
                                    uint8_t protocol_device_id,
                                    uint8_t command,
                                    const uint8_t *payload,
                                    uint16_t payload_len,
                                    uint8_t *response_buf,
                                    size_t response_buf_size,
                                    size_t expected_response_len,
                                    system_protocol_frame_t *out_frame)
{
    uint8_t frame_buf[SYSTEM_PROTOCOL_GENERAL_FRAME_BASE_LEN + SYSTEM_PROTOCOL_MAX_PAYLOAD_LEN] = {0};
    size_t frame_len = 0;
    size_t parsed_len = 0;
    system_module_slot_t *slot = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    ESP_RETURN_ON_FALSE(response_buf && out_frame, ESP_ERR_INVALID_ARG, TAG, "invalid output args");
    ESP_RETURN_ON_FALSE(expected_response_len > 0 && expected_response_len <= response_buf_size,
                        ESP_ERR_INVALID_SIZE, TAG, "invalid response length");
    if (payload_len > 0) {
        ESP_RETURN_ON_FALSE(payload, ESP_ERR_INVALID_ARG, TAG, "payload is NULL");
    }

    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
    slot = alloc_slot(i2c_addr);
    if (!slot) {
        xSemaphoreGive(s_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }

    protocol_device_id = resolve_protocol_device_id(slot, protocol_device_id);
    if (protocol_device_id == 0) {
        err = ESP_ERR_INVALID_ARG;
        goto err_unlock;
    }

    err = ensure_attached(slot, protocol_device_id);
    if (err != ESP_OK) {
        goto err_unlock;
    }

    err = system_protocol_build_request(protocol_device_id,
                                        command,
                                        payload,
                                        payload_len,
                                        frame_buf,
                                        sizeof(frame_buf),
                                        &frame_len);
    if (err != ESP_OK) {
        goto err_unlock;
    }

    err = driver_i2c_write_read(slot->dev_handle, frame_buf, frame_len, response_buf, expected_response_len);
    slot->info.last_command = command;
    slot->info.last_err = err;
    if (err == ESP_OK) {
        slot->info.tx_count++;
        slot->info.rx_count++;
        slot->info.online = true;
    }
    xSemaphoreGive(s_ctx.mutex);

    if (err != ESP_OK) {
        system_module_xfer_event_t evt = {
            .i2c_addr = i2c_addr,
            .protocol_device_id = protocol_device_id,
            .command = command,
            .tx_len = frame_len,
            .rx_len = expected_response_len,
            .result = err,
        };
        (void)system_event_publish(err == ESP_ERR_TIMEOUT ? SYSTEM_EVENT_EVT_I2C_TIMEOUT : SYSTEM_EVENT_EVT_I2C_XFER_DONE,
                                   &evt, sizeof(evt), 0);
        report_link_error(SYSTEM_FAULT_SOURCE_MODULE_SERVICE, err, i2c_addr, command);
        return err;
    }

    err = system_protocol_parse(response_buf, expected_response_len, out_frame, &parsed_len);
    if (err != ESP_OK) {
        ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
        slot = find_slot_by_addr(i2c_addr);
        if (slot) {
            slot->info.last_err = err;
        }
        xSemaphoreGive(s_ctx.mutex);
        (void)system_event_publish(SYSTEM_EVENT_EVT_PROTOCOL_ERROR, response_buf, expected_response_len, 0);
        (void)system_fault_report(SYSTEM_FAULT_SOURCE_PROTOCOL,
                                  SYSTEM_FAULT_MALFORMED_FRAME,
                                  err,
                                  i2c_addr,
                                  command);
        return err;
    }

    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
    slot = find_slot_by_addr(i2c_addr);
    if (slot) {
        slot->info.last_status = out_frame->is_status_frame ? out_frame->status : SYSTEM_PROTOCOL_STATUS_SUCCESS;
        slot->info.last_err = ESP_OK;
    }
    xSemaphoreGive(s_ctx.mutex);

    system_module_xfer_event_t evt = {
        .i2c_addr = i2c_addr,
        .protocol_device_id = protocol_device_id,
        .command = command,
        .tx_len = frame_len,
        .rx_len = parsed_len,
        .result = ESP_OK,
    };
    (void)system_event_publish(SYSTEM_EVENT_EVT_I2C_XFER_DONE, &evt, sizeof(evt), 0);

    if (out_frame->is_status_frame && out_frame->status != SYSTEM_PROTOCOL_STATUS_SUCCESS) {
        (void)system_fault_report(SYSTEM_FAULT_SOURCE_MODULE_SERVICE,
                                  system_fault_from_protocol_status((uint8_t)out_frame->status),
                                  ESP_ERR_INVALID_RESPONSE,
                                  i2c_addr,
                                  command);
    }
    return ESP_OK;

err_unlock:
    slot->info.last_err = err;
    xSemaphoreGive(s_ctx.mutex);
    return err;
}

esp_err_t system_module_get_state(uint16_t i2c_addr, system_module_state_t *out_state)
{
    system_module_slot_t *slot = NULL;

    ESP_RETURN_ON_FALSE(out_state, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_FALSE(s_ctx.initialized, ESP_ERR_INVALID_STATE, TAG, "service not initialized");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_ctx.mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    slot = find_slot_by_addr(i2c_addr);
    if (!slot) {
        xSemaphoreGive(s_ctx.mutex);
        return ESP_ERR_NOT_FOUND;
    }

    out_state->online = slot->info.online;
    out_state->last_command = slot->info.last_command;
    out_state->last_status = slot->info.last_status;
    out_state->tx_count = slot->info.tx_count;
    out_state->rx_count = slot->info.rx_count;
    out_state->last_err = slot->info.last_err;

    xSemaphoreGive(s_ctx.mutex);
    return ESP_OK;
}
