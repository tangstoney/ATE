#include "system_i2c_link.h"

#include <stdlib.h>

#include "esp_check.h"

typedef struct system_i2c_link {
    uint32_t link_id;
    driver_i2c_master_handle_t driver_handle;
} system_i2c_link_t;

static const char *TAG = "system_i2c_link";

esp_err_t system_i2c_link_create(const system_i2c_link_config_t *config,
                                 system_i2c_link_handle_t *out_handle)
{
    system_i2c_link_t *handle = NULL;

    ESP_RETURN_ON_FALSE(config && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    handle->link_id = config->link_id;
    esp_err_t err = driver_i2c_master_create(&config->bus_config, &handle->driver_handle);
    if (err != ESP_OK) {
        free(handle);
        return err;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t system_i2c_link_delete(system_i2c_link_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(driver_i2c_master_delete(handle->driver_handle), TAG, "driver_i2c_master_delete failed");
    free(handle);
    return ESP_OK;
}

esp_err_t system_i2c_link_get_id(system_i2c_link_handle_t handle, uint32_t *out_link_id)
{
    ESP_RETURN_ON_FALSE(handle && out_link_id, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_link_id = handle->link_id;
    return ESP_OK;
}

esp_err_t system_i2c_link_probe(system_i2c_link_handle_t handle,
                                uint16_t dev_addr,
                                int timeout_ms)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_i2c_master_probe(handle->driver_handle, dev_addr, timeout_ms);
}

esp_err_t system_i2c_link_recover(system_i2c_link_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_i2c_master_recover(handle->driver_handle);
}

esp_err_t system_i2c_link_device_add(system_i2c_link_handle_t handle,
                                     const driver_i2c_device_config_t *config,
                                     driver_i2c_device_handle_t *out_dev)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return driver_i2c_device_add(handle->driver_handle, config, out_dev);
}

esp_err_t system_i2c_link_device_remove(driver_i2c_device_handle_t dev)
{
    return driver_i2c_device_remove(dev);
}

esp_err_t system_i2c_link_write(driver_i2c_device_handle_t dev,
                                const uint8_t *data,
                                size_t len)
{
    return driver_i2c_write(dev, data, len);
}

esp_err_t system_i2c_link_read(driver_i2c_device_handle_t dev,
                               uint8_t *buf,
                               size_t len)
{
    return driver_i2c_read(dev, buf, len);
}

esp_err_t system_i2c_link_write_read(driver_i2c_device_handle_t dev,
                                     const uint8_t *write_buf,
                                     size_t write_len,
                                     uint8_t *read_buf,
                                     size_t read_len)
{
    return driver_i2c_write_read(dev, write_buf, write_len, read_buf, read_len);
}
