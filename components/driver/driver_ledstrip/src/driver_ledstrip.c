#include "driver_ledstrip.h"

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "led_strip.h"

#include "board_ate_p4.h"

static const char *TAG = "driver_ledstrip";

// 真实结构体只在本文件可见
struct driver_ledstrip {
    led_strip_handle_t strip;   // led_strip 组件的句柄
    uint16_t           led_count;
    SemaphoreHandle_t  mutex;
};

esp_err_t driver_ledstrip_create(driver_ledstrip_handle_t *out_handle)
{
    if (!out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    driver_ledstrip_handle_t handle = calloc(1, sizeof(*handle));
    if (!handle) {
        return ESP_ERR_NO_MEM;
    }

    handle->led_count = BOARD_LED_STRIP_LED_COUNT;
    handle->mutex = xSemaphoreCreateMutex();
    if (!handle->mutex) {
        free(handle);
        return ESP_ERR_NO_MEM;
    }

    // --- 配置 led_strip（来自官方示例的模式） ---
    led_strip_config_t strip_config = {
        .strip_gpio_num = BOARD_LED_STRIP_DATA_GPIO,
        .max_leds       = BOARD_LED_STRIP_LED_COUNT,
        .led_model      = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src        = RMT_CLK_SRC_DEFAULT,
        .resolution_hz  = BOARD_LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = BOARD_LED_STRIP_MEM_WORDS,
        .flags = {
            .with_dma = BOARD_LED_STRIP_USE_DMA,
        },
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &handle->strip); // [[LED strip driver](https://components.espressif.com/components/espressif/led_strip)]
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %s", esp_err_to_name(err));
        vSemaphoreDelete(handle->mutex);
        free(handle);
        return err;
    }

    // 上电先清一次
    led_strip_clear(handle->strip);  // 官方示例同样做法 [[完整示例代码](https://developer.espressif.com/workshops/esp-idf-with-esp32-c6/assignment-2/#complete-code)]

    *out_handle = handle;
    ESP_LOGI(TAG, "Created LED strip driver: gpio=%d, count=%d",
             BOARD_LED_STRIP_DATA_GPIO, BOARD_LED_STRIP_LED_COUNT);
    return ESP_OK;
}

esp_err_t driver_ledstrip_destroy(driver_ledstrip_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->strip) {
        led_strip_clear(handle->strip);
        // 目前 led_strip 组件没有显式 delete API，丢弃句柄即可 [[LED strip driver](https://components.espressif.com/components/espressif/led_strip)]
        handle->strip = NULL;
    }

    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }

    free(handle);
    return ESP_OK;
}

esp_err_t driver_ledstrip_lock(driver_ledstrip_handle_t handle, int timeout_ms)
{
    if (!handle || !handle->mutex) {
        return ESP_ERR_INVALID_ARG;
    }
    TickType_t ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(handle->mutex, ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t driver_ledstrip_unlock(driver_ledstrip_handle_t handle)
{
    if (!handle || !handle->mutex) {
        return ESP_ERR_INVALID_ARG;
    }
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

esp_err_t driver_ledstrip_set_pixel(driver_ledstrip_handle_t handle,
                                    uint16_t index,
                                    uint8_t r, uint8_t g, uint8_t b)
{
    if (!handle || !handle->strip) {
        return ESP_ERR_INVALID_ARG;
    }
    if (index >= handle->led_count) {
        return ESP_ERR_INVALID_ARG;
    }

    return led_strip_set_pixel(handle->strip, index, r, g, b); //  [[LED strip driver](https://components.espressif.com/components/espressif/led_strip)]
}

esp_err_t driver_ledstrip_fill(driver_ledstrip_handle_t handle,
                               uint8_t r, uint8_t g, uint8_t b)
{
    if (!handle || !handle->strip) {
        return ESP_ERR_INVALID_ARG;
    }

    for (uint16_t i = 0; i < handle->led_count; ++i) {
        esp_err_t err = led_strip_set_pixel(handle->strip, i, r, g, b);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;   // 只写缓冲区，不自动 refresh
}

esp_err_t driver_ledstrip_refresh(driver_ledstrip_handle_t handle)
{
    if (!handle || !handle->strip) {
        return ESP_ERR_INVALID_ARG;
    }
    return led_strip_refresh(handle->strip);
}

esp_err_t driver_ledstrip_clear(driver_ledstrip_handle_t handle)
{
    if (!handle || !handle->strip) {
        return ESP_ERR_INVALID_ARG;
    }
    return led_strip_clear(handle->strip);
}
