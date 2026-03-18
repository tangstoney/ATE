#include "driver_display.h"

#include <stdio.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_touch.h"

#include "driver_display_lcd.h"
#include "driver_display_touch.h"


typedef struct driver_display {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_touch_handle_t touch_handle;
    bool touch_available;
    driver_display_info_t info;
} driver_display_t;

static const char *TAG = "driver_display";

esp_err_t driver_display_create(driver_display_handle_t *out_handle)
{
    lcd_display_bsp_gpio_init();

    esp_err_t ret = ESP_OK;           // ← 必须加这一行
    driver_display_t *handle = NULL;
    ESP_GOTO_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, err, TAG, "out_handle is NULL");
    
    handle = calloc(1, sizeof(driver_display_t));
    ESP_GOTO_ON_FALSE(handle, ESP_ERR_NO_MEM, err, TAG, "no memory");

    driver_display_lcd_handle_t *disp = driver_display_lcd_init();
    ESP_GOTO_ON_FALSE(disp, ESP_FAIL, err, TAG, "driver_display_lcd_init failed");

    handle->panel = disp->panel;
    handle->io = disp->io;
    handle->touch_handle = driver_display_touch_init(disp->hor_res, disp->ver_res);
    handle->touch_available = (handle->touch_handle != NULL);
    handle->info.width = disp->hor_res;
    handle->info.height = disp->ver_res;
    handle->info.touch_available = handle->touch_available;

    *out_handle = handle;
    return ESP_OK;

err:
    free(handle);
    return ESP_FAIL;
}

esp_err_t driver_display_destroy(driver_display_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}

esp_err_t driver_display_get_info(driver_display_handle_t handle, driver_display_info_t *out_info)
{
    ESP_RETURN_ON_FALSE(handle && out_info, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_info = handle->info;
    return ESP_OK;
}

esp_err_t driver_display_get_panel_handle(driver_display_handle_t handle,
                                          esp_lcd_panel_handle_t *out_panel,
                                          esp_lcd_panel_io_handle_t *out_io)
{
    ESP_RETURN_ON_FALSE(handle && out_panel && out_io, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_panel = handle->panel;
    *out_io = handle->io;
    return ESP_OK;
}

esp_err_t driver_display_get_resolution(driver_display_handle_t handle, uint16_t *w, uint16_t *h)
{
    ESP_RETURN_ON_FALSE(handle && w && h, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *w = handle->info.width;
    *h = handle->info.height;
    return ESP_OK;
}

esp_err_t driver_display_get_touch_handle(driver_display_handle_t handle, esp_lcd_touch_handle_t *out_touch)
{
    ESP_RETURN_ON_FALSE(handle && out_touch, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_touch = handle->touch_handle;
    return ESP_OK;
}

esp_err_t driver_display_read_touch(driver_display_handle_t handle, bool *pressed, uint16_t *x, uint16_t *y)
{
    ESP_RETURN_ON_FALSE(handle && pressed && x && y, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(handle->touch_handle, ESP_ERR_INVALID_STATE, TAG, "touch unavailable");

    ESP_RETURN_ON_ERROR(esp_lcd_touch_read_data(handle->touch_handle), TAG, "read touch failed");

    esp_lcd_touch_point_data_t points[1] = {0};
    uint8_t count = 0;
    ESP_RETURN_ON_ERROR(esp_lcd_touch_get_data(handle->touch_handle, points, &count, 1), TAG,
                        "get touch data failed");

    *pressed = (count > 0);
    *x = (count > 0) ? points[0].x : 0;
    *y = (count > 0) ? points[0].y : 0;
    return ESP_OK;
}

/*
 * 板型切换位置:
 * 当前按参考工程里可工作的显示板参数对齐。
 * 如果后面切回另一块板，优先改这里的 EN/PWM/RST/POWER。
 */
#define LCD_DISPLAY_BSP_EN_GPIO         (GPIO_NUM_53)
#define LCD_DISPLAY_BSP_PWM_GPIO        (GPIO_NUM_21)
#define LCD_DISPLAY_BSP_RST_GPIO        (GPIO_NUM_27)
#define POWER_EN_GPIO                   (GPIO_NUM_45)
// 10.1 lcd bsp 的gpio初始化，比如rst en等需要置高电平的
esp_err_t lcd_display_bsp_gpio_init(void)
{
    esp_err_t ret;

    gpio_config_t En_GPIO_Config =
    {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LCD_DISPLAY_BSP_EN_GPIO),
        .pull_down_en = 0,                  //disable pull-down mode
        .pull_up_en = 0,                    //disable pull-up mode
    };

    ret = gpio_config(&En_GPIO_Config);
    if (ret != ESP_OK) {
        // 处理错误
        printf("GPIO配置失败: %d\n", ret);
    }

    ret = gpio_set_level(LCD_DISPLAY_BSP_EN_GPIO, 1);
    if (ret != ESP_OK) {
        // 处理错误
        printf("GPIO设置电平失败: %d\n", ret);
    }

    gpio_config_t PWM_GPIO_Config =
    {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LCD_DISPLAY_BSP_PWM_GPIO),
        .pull_down_en = 0,                  //disable pull-down mode
        .pull_up_en = 0,                    //disable pull-up mode
    };

    ret = gpio_config(&PWM_GPIO_Config);
    if (ret != ESP_OK) {
        // 处理错误
        printf("GPIO配置失败: %d\n", ret);
    }

    ret = gpio_set_level(LCD_DISPLAY_BSP_PWM_GPIO, 1);
    if (ret != ESP_OK) {
        // 处理错误
        printf("GPIO设置电平失败: %d\n", ret);
    }

    gpio_config_t RST_GPIO_Config =
    {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LCD_DISPLAY_BSP_RST_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };

    ret = gpio_config(&RST_GPIO_Config);
    if (ret != ESP_OK) {
        printf("GPIO配置失败: %d\n", ret);
    }

    ret = gpio_set_level(LCD_DISPLAY_BSP_RST_GPIO, 1);
    if (ret != ESP_OK) {
        printf("GPIO设置电平失败: %d\n", ret);
    }

    gpio_config_t POWER_EN_GPIO_Config =
    {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << POWER_EN_GPIO),
        .pull_down_en = 0,                  //disable pull-down mode
        .pull_up_en = 0,                    //disable pull-up mode
    };

    ret = gpio_config(&POWER_EN_GPIO_Config);
    if (ret != ESP_OK) {
        // 处理错误
        printf("GPIO配置失败: %d\n", ret);
    }

    ret = gpio_set_level(POWER_EN_GPIO, 1);
    if (ret != ESP_OK) {
        // 处理错误
        printf("GPIO设置电平失败: %d\n", ret);
    }

    return ret;
}

