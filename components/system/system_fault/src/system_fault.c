#include "system_fault.h"

#include <string.h>

#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "system_fault";
static SemaphoreHandle_t s_mutex;
static system_fault_record_t s_last_fault;

esp_err_t system_fault_init(void)
{
    if (s_mutex) {
        return ESP_OK;
    }
    s_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_mutex, ESP_ERR_NO_MEM, TAG, "xSemaphoreCreateMutex failed");
    return ESP_OK;
}

esp_err_t system_fault_clear(void)
{
    ESP_RETURN_ON_ERROR(system_fault_init(), TAG, "system_fault_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
    memset(&s_last_fault, 0, sizeof(s_last_fault));
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t system_fault_get_last(system_fault_record_t *out_record)
{
    ESP_RETURN_ON_FALSE(out_record, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_ERROR(system_fault_init(), TAG, "system_fault_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
    *out_record = s_last_fault;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

system_fault_code_t system_fault_from_esp_err(esp_err_t err)
{
    switch (err) {
    case ESP_OK:
        return SYSTEM_FAULT_NONE;
    case ESP_ERR_TIMEOUT:
        return SYSTEM_FAULT_TIMEOUT;
    case ESP_ERR_NOT_FOUND:
        return SYSTEM_FAULT_NOT_FOUND;
    case ESP_ERR_NOT_SUPPORTED:
        return SYSTEM_FAULT_UNSUPPORTED_PROTOCOL;
    case ESP_ERR_INVALID_RESPONSE:
        return SYSTEM_FAULT_INVALID_RESPONSE;
    default:
        return SYSTEM_FAULT_DRIVER_ERROR;
    }
}

system_fault_code_t system_fault_from_protocol_status(uint8_t status)
{
    switch (status) {
    case 0x00:
        return SYSTEM_FAULT_NONE;
    case 0x07:
        return SYSTEM_FAULT_CRC_ERROR;
    case 0x08:
        return SYSTEM_FAULT_RESOURCE_BUSY;
    case 0x09:
    case 0x0A:
        return SYSTEM_FAULT_PORT_BUSY;
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
        return SYSTEM_FAULT_MALFORMED_FRAME;
    default:
        return SYSTEM_FAULT_INVALID_RESPONSE;
    }
}

const char *system_fault_code_to_name(system_fault_code_t code)
{
    switch (code) {
    case SYSTEM_FAULT_NONE:
        return "NONE";
    case SYSTEM_FAULT_TIMEOUT:
        return "TIMEOUT";
    case SYSTEM_FAULT_CRC_ERROR:
        return "CRC_ERROR";
    case SYSTEM_FAULT_NACK:
        return "NACK";
    case SYSTEM_FAULT_DISCONNECT:
        return "DISCONNECT";
    case SYSTEM_FAULT_PORT_BUSY:
        return "PORT_BUSY";
    case SYSTEM_FAULT_BUS_STUCK:
        return "BUS_STUCK";
    case SYSTEM_FAULT_UNSUPPORTED_PROTOCOL:
        return "UNSUPPORTED_PROTOCOL";
    case SYSTEM_FAULT_MALFORMED_FRAME:
        return "MALFORMED_FRAME";
    case SYSTEM_FAULT_RESOURCE_BUSY:
        return "RESOURCE_BUSY";
    case SYSTEM_FAULT_INVALID_RESPONSE:
        return "INVALID_RESPONSE";
    case SYSTEM_FAULT_DRIVER_ERROR:
        return "DRIVER_ERROR";
    case SYSTEM_FAULT_NOT_FOUND:
        return "NOT_FOUND";
    default:
        return "UNKNOWN";
    }
}

esp_err_t system_fault_report(system_fault_source_t source,
                              system_fault_code_t code,
                              esp_err_t err,
                              uint32_t detail0,
                              uint32_t detail1)
{
    system_fault_record_t record = {
        .code = code,
        .source = source,
        .esp_err = err,
        .detail0 = detail0,
        .detail1 = detail1,
        .timestamp_us = esp_timer_get_time(),
    };

    ESP_RETURN_ON_ERROR(system_fault_init(), TAG, "system_fault_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");
    s_last_fault = record;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}
