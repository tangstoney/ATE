#include "system_uart_link.h"

#include <stdlib.h>

#include "esp_check.h"

typedef struct system_uart_link {
    uint32_t link_id;
    driver_uart_port_handle_t driver_handle;
} system_uart_link_t;

static const char *TAG = "system_uart_link";

esp_err_t system_uart_link_create(const system_uart_link_config_t *config,
                                  system_uart_link_handle_t *out_handle)
{
    system_uart_link_t *handle = NULL;

    ESP_RETURN_ON_FALSE(config && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    handle->link_id = config->link_id;
    esp_err_t err = driver_uart_port_create(&config->port_config, &handle->driver_handle);
    if (err != ESP_OK) {
        free(handle);
        return err;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t system_uart_link_delete(system_uart_link_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(driver_uart_port_delete(handle->driver_handle), TAG, "driver_uart_port_delete failed");
    free(handle);
    return ESP_OK;
}

esp_err_t system_uart_link_get_id(system_uart_link_handle_t handle, uint32_t *out_link_id)
{
    ESP_RETURN_ON_FALSE(handle && out_link_id, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_link_id = handle->link_id;
    return ESP_OK;
}

esp_err_t system_uart_link_set_baudrate(system_uart_link_handle_t handle, uint32_t baud_rate)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_uart_port_set_baudrate(handle->driver_handle, baud_rate);
}

esp_err_t system_uart_link_recover(system_uart_link_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_uart_port_recover(handle->driver_handle);
}

esp_err_t system_uart_link_write(system_uart_link_handle_t handle,
                                 const uint8_t *data,
                                 size_t len)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_uart_write(handle->driver_handle, data, len);
}

esp_err_t system_uart_link_read(system_uart_link_handle_t handle,
                                uint8_t *buf,
                                size_t len,
                                size_t *out_len)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_uart_read(handle->driver_handle, buf, len, out_len);
}
