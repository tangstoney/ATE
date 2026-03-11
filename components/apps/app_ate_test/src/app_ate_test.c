#include "app_ate_test.h"

#include "esp_check.h"

#include "app_eez_ui.h"


static const char *TAG = "app_ate_test";

static esp_err_t app_network_xx(void)
{
    return ESP_OK;
}

static esp_err_t app_ledstrip_xx(void)
{
    return ESP_OK;
}

static esp_err_t app_usb_xx(void)
{
    return ESP_OK;
}

static esp_err_t app_i2c_xx(void)
{
    return ESP_OK;
}

static esp_err_t app_uart_xx(void)
{
    return ESP_OK;
}

esp_err_t app_ate_test_start(void)
{
    // codex todo ，这些也应该用线程包起来跑吧我感觉，初始化不要直接用函数调用，这样很奇怪
    ESP_RETURN_ON_ERROR(app_eez_ui_start(), TAG, "EEZ UI start failed"); // 跑eez ui业务
    ESP_RETURN_ON_ERROR(app_network_xx(), TAG, "network start failed"); // 跑以太网业务 
    ESP_RETURN_ON_ERROR(app_ledstrip_xx(), TAG, "network start failed"); // 跑幻彩灯条提示灯业务
    ESP_RETURN_ON_ERROR(app_usb_xx(), TAG, "network start failed"); // 跑 usb hub的业务，里面会分比如连其他仪表变成usb从机，变成usb fw更新等等
    ESP_RETURN_ON_ERROR(app_i2c_xx(), TAG, "network start failed"); // 跑 被测模组，主要是和下面的 cs32l0的下位机进行通信 
    ESP_RETURN_ON_ERROR(app_uart_xx(), TAG, "network start failed"); // 跑 被测仪表，主要是和下面esp32s3的下位机进行通信 

    return ESP_OK;
}
