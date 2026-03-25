#include "driver_i2c_module.h"

#include <stdlib.h>

#include "board_ate_p4.h"
#include "esp_check.h"
#include "esp_log.h"

struct driver_i2c_module_t {
    i2c_master_dev_handle_t dev_handle;
    uint16_t dev_addr;
    int timeout_ms;
};

static const char *TAG = "driver_i2c_module";
static i2c_master_bus_handle_t s_bus_handle;

static i2c_port_num_t resolve_bus_port(void)
{
    return BOARD_I2C_MASTER_NUM >= 0 ? BOARD_I2C_MASTER_NUM : (i2c_port_num_t)CONFIG_DRIVER_I2C_MODULE_PORT;
}

static gpio_num_t resolve_scl_gpio(void)
{
    return BOARD_I2C_MASTER_SCL != BOARD_GPIO_NONE ? BOARD_I2C_MASTER_SCL :
                                                     (gpio_num_t)CONFIG_DRIVER_I2C_MODULE_SCL_GPIO;
}

static gpio_num_t resolve_sda_gpio(void)
{
    return BOARD_I2C_MASTER_SDA != BOARD_GPIO_NONE ? BOARD_I2C_MASTER_SDA :
                                                     (gpio_num_t)CONFIG_DRIVER_I2C_MODULE_SDA_GPIO;
}

static bool resolve_internal_pullup(void)
{
    return (BOARD_I2C_MASTER_USE_INTERNAL_PULLUP == 1) || CONFIG_DRIVER_I2C_MODULE_USE_INTERNAL_PULLUP;
}

static uint8_t resolve_glitch_ignore_cnt(void)
{
    return BOARD_I2C_MASTER_GLITCH_IGNORE_CNT ? BOARD_I2C_MASTER_GLITCH_IGNORE_CNT : 7U;
}

static uint16_t resolve_device_addr(void)
{
    return BOARD_MODULE_LINK_I2C_ADDR;
}

static uint32_t resolve_device_speed_hz(void)
{
    return BOARD_I2C_MASTER_CLK_HZ;
}

static int resolve_timeout_ms(void)
{
    return BOARD_I2C_MASTER_TIMEOUT_MS > 0 ? (int)BOARD_I2C_MASTER_TIMEOUT_MS : 100;
}

static esp_err_t validate_bus_gpio(gpio_num_t io_num, const char *io_name)
{
    ESP_RETURN_ON_FALSE(io_num != BOARD_GPIO_NONE, ESP_ERR_INVALID_STATE, TAG, "%s gpio not configured", io_name);
    ESP_RETURN_ON_FALSE(io_num >= 0, ESP_ERR_INVALID_ARG, TAG, "%s gpio invalid", io_name);
    return ESP_OK;
}

static esp_err_t validate_device_config(uint16_t dev_addr, uint32_t scl_speed_hz)
{
    ESP_RETURN_ON_FALSE(dev_addr != BOARD_I2C_ADDR_INVALID, ESP_ERR_INVALID_STATE, TAG, "module i2c address not configured");
    ESP_RETURN_ON_FALSE(dev_addr <= 0x7F, ESP_ERR_INVALID_ARG, TAG, "invalid device address");
    ESP_RETURN_ON_FALSE(scl_speed_hz > 0, ESP_ERR_INVALID_STATE, TAG, "invalid device speed");
    return ESP_OK;
}

esp_err_t driver_i2c_module_bus_create(void)
{
    i2c_master_bus_config_t cfg = {0};
    i2c_port_num_t bus_port = resolve_bus_port();
    gpio_num_t scl_gpio = resolve_scl_gpio();
    gpio_num_t sda_gpio = resolve_sda_gpio();
    esp_err_t err = ESP_OK;

    err = i2c_master_get_bus_handle(bus_port, &s_bus_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus port %d already initialized, reusing handle", bus_port);
        return ESP_OK;
    }
    ESP_RETURN_ON_FALSE(err == ESP_ERR_INVALID_STATE, err, TAG, "i2c_master_get_bus_handle failed");

    ESP_RETURN_ON_ERROR(validate_bus_gpio(scl_gpio, "scl"), TAG, "invalid scl gpio");
    ESP_RETURN_ON_ERROR(validate_bus_gpio(sda_gpio, "sda"), TAG, "invalid sda gpio");

    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.i2c_port = bus_port;
    cfg.scl_io_num = scl_gpio;
    cfg.sda_io_num = sda_gpio;
    cfg.glitch_ignore_cnt = resolve_glitch_ignore_cnt();
    cfg.flags.enable_internal_pullup = resolve_internal_pullup();

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&cfg, &s_bus_handle), TAG, "i2c_new_master_bus failed");

    ESP_LOGI(TAG, "I2C bus created: port=%d scl=%d sda=%d", bus_port, (int)scl_gpio, (int)sda_gpio);
    return ESP_OK;
}

esp_err_t driver_i2c_module_bus_destroy(void)
{
    s_bus_handle = NULL;
    return ESP_OK;
}

esp_err_t driver_i2c_module_create(driver_i2c_module_handle_t *out_handle)
{
    driver_i2c_module_handle_t handle = NULL;
    i2c_device_config_t dev_cfg = {0};
    uint16_t dev_addr = resolve_device_addr();
    uint32_t scl_speed_hz = resolve_device_speed_hz();
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    ESP_RETURN_ON_ERROR(driver_i2c_module_bus_create(), TAG, "driver_i2c_module_bus_create failed");
    ESP_RETURN_ON_ERROR(validate_device_config(dev_addr, scl_speed_hz), TAG, "invalid device config");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "alloc handle failed");

    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = dev_addr;
    dev_cfg.scl_speed_hz = scl_speed_hz;

    err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &handle->dev_handle);
    if (err != ESP_OK) {
        free(handle);
        return err;
    }

    handle->dev_addr = dev_addr;
    handle->timeout_ms = resolve_timeout_ms();
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_i2c_module_destroy(driver_i2c_module_handle_t handle)
{
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    err = i2c_master_bus_rm_device(handle->dev_handle);
    free(handle);
    return err;
}

esp_err_t driver_i2c_module_write(driver_i2c_module_handle_t handle,
                                  const uint8_t *data,
                                  size_t len)
{
    ESP_RETURN_ON_FALSE(handle && data && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return i2c_master_transmit(handle->dev_handle, data, len, handle->timeout_ms);
}

esp_err_t driver_i2c_module_read(driver_i2c_module_handle_t handle,
                                 uint8_t *out_data,
                                 size_t len)
{
    ESP_RETURN_ON_FALSE(handle && out_data && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return i2c_master_receive(handle->dev_handle, out_data, len, handle->timeout_ms);
}

esp_err_t driver_i2c_module_write_read(driver_i2c_module_handle_t handle,
                                       const uint8_t *write_data,
                                       size_t write_len,
                                       uint8_t *read_data,
                                       size_t read_len)
{
    ESP_RETURN_ON_FALSE(handle && write_data && write_len > 0 && read_data && read_len > 0,
                        ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return i2c_master_transmit_receive(handle->dev_handle,
                                       write_data,
                                       write_len,
                                       read_data,
                                       read_len,
                                       handle->timeout_ms);
}

esp_err_t driver_i2c_module_probe(driver_i2c_module_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(s_bus_handle, ESP_ERR_INVALID_STATE, TAG, "bus not initialized");
    return i2c_master_probe(s_bus_handle, handle->dev_addr, handle->timeout_ms);
}
