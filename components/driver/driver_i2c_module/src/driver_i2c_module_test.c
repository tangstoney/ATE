#include "driver_i2c_module_test.h"

#include <stddef.h>
#include <stdint.h>

#include "board_ate_p4.h"
#include "driver_i2c_module.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "driver_i2c_test";
static const uint8_t s_default_pattern[] = {0xA5, 0x5A, 0x00, 0xFF};

static void log_bus_context(const char *op_name)
{
    ESP_LOGI(TAG,
             "%s: port=%d sda=%d scl=%d addr=0x%02X clk_hz=%u",
             op_name,
             (int)BOARD_I2C_MASTER_NUM,
             (int)BOARD_I2C_MASTER_SDA,
             (int)BOARD_I2C_MASTER_SCL,
             (unsigned)BOARD_MODULE_LINK_I2C_ADDR,
             (unsigned)BOARD_I2C_MASTER_CLK_HZ);
}

esp_err_t driver_i2c_module_test_probe_once(void)
{
    driver_i2c_module_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    log_bus_context("probe_once");
    ESP_RETURN_ON_ERROR(driver_i2c_module_create(&handle), TAG, "driver_i2c_module_create failed");

    ret = driver_i2c_module_probe(handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "probe ack received from addr=0x%02X", (unsigned)BOARD_MODULE_LINK_I2C_ADDR);
    } else {
        ESP_LOGW(TAG,
                 "probe result=%s (logic analyzer can still confirm address phase)",
                 esp_err_to_name(ret));
    }

    ESP_RETURN_ON_ERROR(driver_i2c_module_destroy(handle), TAG, "driver_i2c_module_destroy failed");
    return ret;
}

esp_err_t driver_i2c_module_test_write_default_pattern(void)
{
    driver_i2c_module_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    log_bus_context("write_default_pattern");
    ESP_LOGI(TAG,
             "pattern bytes: %02X %02X %02X %02X",
             s_default_pattern[0],
             s_default_pattern[1],
             s_default_pattern[2],
             s_default_pattern[3]);

    ESP_RETURN_ON_ERROR(driver_i2c_module_create(&handle), TAG, "driver_i2c_module_create failed");

    ret = driver_i2c_module_write(handle, s_default_pattern, sizeof(s_default_pattern));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "write pattern completed");
    } else {
        ESP_LOGW(TAG,
                 "write result=%s (logic analyzer can still confirm address/data phase)",
                 esp_err_to_name(ret));
    }

    ESP_RETURN_ON_ERROR(driver_i2c_module_destroy(handle), TAG, "driver_i2c_module_destroy failed");
    return ret;
}
