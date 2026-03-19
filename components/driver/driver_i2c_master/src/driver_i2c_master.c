#include "driver_i2c_master.h"

#include <stdlib.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct driver_i2c_device {
    struct driver_i2c_master *master;
    i2c_master_dev_handle_t dev_handle;
    uint16_t dev_addr;
    uint32_t scl_speed_hz;
    int timeout_ms;
    struct driver_i2c_device *next;
} driver_i2c_device_t;

typedef struct driver_i2c_master {
    i2c_master_bus_handle_t bus_handle;
    driver_i2c_master_config_t config;
    SemaphoreHandle_t list_mutex;
    driver_i2c_device_t *devices;
} driver_i2c_master_t;

static const char *TAG = "driver_i2c_master";

static uint32_t normalize_clk_speed(uint32_t hz)
{
    return hz ? hz : 400000;
}

static int normalize_timeout_ms(int timeout_ms)
{
    return timeout_ms > 0 ? timeout_ms : 1000;
}

static uint8_t normalize_glitch_ignore_cnt(uint8_t glitch_ignore_cnt)
{
    return glitch_ignore_cnt ? glitch_ignore_cnt : 7;
}

static esp_err_t lock_device_list(driver_i2c_master_t *master)
{
    ESP_RETURN_ON_FALSE(master && master->list_mutex, ESP_ERR_INVALID_ARG, TAG, "invalid master");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(master->list_mutex, portMAX_DELAY) == pdTRUE,
                        ESP_ERR_TIMEOUT, TAG, "take list mutex failed");
    return ESP_OK;
}

static void unlock_device_list(driver_i2c_master_t *master)
{
    if (!master || !master->list_mutex) {
        return;
    }

    if (xSemaphoreGive(master->list_mutex) != pdTRUE) {
        ESP_LOGE(TAG, "give list mutex failed");
    }
}

static esp_err_t validate_device_address(uint16_t dev_addr, i2c_addr_bit_len_t addr_bit_len)
{
    switch (addr_bit_len) {
    case I2C_ADDR_BIT_LEN_7:
        return dev_addr <= 0x7F ? ESP_OK : ESP_ERR_INVALID_ARG;
    case I2C_ADDR_BIT_LEN_10:
        return dev_addr <= 0x3FF ? ESP_OK : ESP_ERR_INVALID_ARG;
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t driver_i2c_master_create(const driver_i2c_master_config_t *config,
                                   driver_i2c_master_handle_t *out_handle)
{
    driver_i2c_master_t *handle = NULL;

    ESP_RETURN_ON_FALSE(config && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(config->sda_io >= 0 && config->scl_io >= 0, ESP_ERR_INVALID_ARG, TAG, "invalid io");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    handle->config = *config;
    handle->config.clk_speed_hz = normalize_clk_speed(config->clk_speed_hz);
    handle->config.timeout_ms = normalize_timeout_ms(config->timeout_ms);
    handle->config.glitch_ignore_cnt = normalize_glitch_ignore_cnt(config->glitch_ignore_cnt);
    handle->list_mutex = xSemaphoreCreateMutex();
    if (!handle->list_mutex) {
        free(handle);
        ESP_LOGE(TAG, "create list mutex failed");
        return ESP_ERR_NO_MEM;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = config->clk_source,
        .i2c_port = config->port,
        .sda_io_num = config->sda_io,
        .scl_io_num = config->scl_io,
        .glitch_ignore_cnt = handle->config.glitch_ignore_cnt,
        .flags.enable_internal_pullup = config->enable_internal_pullup,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &handle->bus_handle);
    if (err != ESP_OK) {
        vSemaphoreDelete(handle->list_mutex);
        free(handle);
        return err;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_i2c_master_delete(driver_i2c_master_handle_t handle)
{
    driver_i2c_device_t *dev = NULL;
    driver_i2c_device_t *next = NULL;
    esp_err_t ret = ESP_OK;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(lock_device_list(handle), TAG, "lock device list failed");

    dev = handle->devices;
    handle->devices = NULL;
    unlock_device_list(handle);

    while (dev) {
        next = dev->next;
        err = i2c_master_bus_rm_device(dev->dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "rm device failed: %s", esp_err_to_name(err));
            if (ret == ESP_OK) {
                ret = err;
            }
        }
        free(dev);
        dev = next;
    }

    err = i2c_del_master_bus(handle->bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "del bus failed: %s", esp_err_to_name(err));
        if (ret == ESP_OK) {
            ret = err;
        }
    }

    vSemaphoreDelete(handle->list_mutex);
    free(handle);
    return ret;
}

esp_err_t driver_i2c_master_probe(driver_i2c_master_handle_t handle,
                                  uint16_t dev_addr,
                                  int timeout_ms)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return i2c_master_probe(handle->bus_handle, dev_addr, normalize_timeout_ms(timeout_ms));
}

esp_err_t driver_i2c_master_recover(driver_i2c_master_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return i2c_master_bus_reset(handle->bus_handle);
}

esp_err_t driver_i2c_device_add(driver_i2c_master_handle_t bus,
                                const driver_i2c_device_config_t *config,
                                driver_i2c_device_handle_t *out_dev)
{
    driver_i2c_device_t *dev = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(bus && config && out_dev, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(validate_device_address(config->dev_addr, config->addr_bit_len), TAG, "invalid address");

    dev = calloc(1, sizeof(*dev));
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_NO_MEM, TAG, "no memory");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = config->addr_bit_len,
        .device_address = config->dev_addr,
        .scl_speed_hz = config->scl_speed_hz ? config->scl_speed_hz : bus->config.clk_speed_hz,
    };

    err = i2c_master_bus_add_device(bus->bus_handle, &dev_cfg, &dev->dev_handle);
    if (err != ESP_OK) {
        free(dev);
        return err;
    }

    dev->master = bus;
    dev->dev_addr = config->dev_addr;
    dev->scl_speed_hz = dev_cfg.scl_speed_hz;
    dev->timeout_ms = normalize_timeout_ms(config->timeout_ms ? config->timeout_ms : bus->config.timeout_ms);

    err = lock_device_list(bus);
    if (err != ESP_OK) {
        esp_err_t cleanup_err = i2c_master_bus_rm_device(dev->dev_handle);
        if (cleanup_err != ESP_OK) {
            ESP_LOGE(TAG, "rollback add device failed: %s", esp_err_to_name(cleanup_err));
        }
        free(dev);
        return err;
    }
    dev->next = bus->devices;
    bus->devices = dev;
    unlock_device_list(bus);

    *out_dev = dev;
    return ESP_OK;
}

esp_err_t driver_i2c_device_remove(driver_i2c_device_handle_t dev)
{
    driver_i2c_master_t *master = NULL;
    driver_i2c_device_t **cursor = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(dev && dev->master, ESP_ERR_INVALID_ARG, TAG, "invalid device");

    master = dev->master;
    ESP_RETURN_ON_ERROR(lock_device_list(master), TAG, "lock device list failed");
    cursor = &master->devices;
    while (*cursor && *cursor != dev) {
        cursor = &(*cursor)->next;
    }
    if (*cursor != dev) {
        unlock_device_list(master);
        return ESP_ERR_NOT_FOUND;
    }

    err = i2c_master_bus_rm_device(dev->dev_handle);
    if (err != ESP_OK) {
        unlock_device_list(master);
        return err;
    }

    *cursor = dev->next;
    unlock_device_list(master);
    free(dev);
    return ESP_OK;
}

esp_err_t driver_i2c_write(driver_i2c_device_handle_t dev,
                           const uint8_t *data,
                           size_t len)
{
    ESP_RETURN_ON_FALSE(dev && data && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return i2c_master_transmit(dev->dev_handle, data, len, dev->timeout_ms);
}

esp_err_t driver_i2c_read(driver_i2c_device_handle_t dev,
                          uint8_t *buf,
                          size_t len)
{
    ESP_RETURN_ON_FALSE(dev && buf && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return i2c_master_receive(dev->dev_handle, buf, len, dev->timeout_ms);
}

esp_err_t driver_i2c_write_read(driver_i2c_device_handle_t dev,
                                const uint8_t *write_buf,
                                size_t write_len,
                                uint8_t *read_buf,
                                size_t read_len)
{
    ESP_RETURN_ON_FALSE(dev && write_buf && write_len > 0 && read_buf && read_len > 0,
                        ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return i2c_master_transmit_receive(dev->dev_handle,
                                       write_buf,
                                       write_len,
                                       read_buf,
                                       read_len,
                                       dev->timeout_ms);
}
