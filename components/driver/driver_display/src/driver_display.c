#include "driver_display.h"

#include <stdlib.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "sdkconfig.h"

#include "driver_lcd_panel.h"

#if CONFIG_DRIVER_DISPLAY_TOUCH_ENABLED
#include "esp_lcd_touch.h"
#include "driver_touch_gt911.h"
#endif

typedef struct driver_display {
    driver_lcd_panel_handle_t lcd_handle;
#if CONFIG_DRIVER_DISPLAY_TOUCH_ENABLED
    driver_touch_gt911_handle_t touch_handle;
    esp_lcd_touch_handle_t touch_native_handle;
#endif
    bool touch_available;
    driver_display_info_t info;
} driver_display_t;

static const char *TAG = "driver_display";
static lv_display_t *s_lvgl_display;
static lv_indev_t *s_lvgl_touch;
static bool s_lvgl_started;

static esp_lv_adapter_rotation_t driver_display_get_rotation(void)
{
    // This is the LCD render rotation used by esp_lvgl_adapter only.
    // GT911 touch mapping is calibrated separately in driver_touch_gt911.c.
    return ESP_LV_ADAPTER_ROTATE_270;// 这个其实也属于 board_ate_p4.h 需要配置的部分， codex todo
}

static esp_lv_adapter_tear_avoid_mode_t driver_display_get_tear_avoid_mode(void)
{
    return ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_FULL;  // 这个其实也属于 board_ate_p4.h 需要配置的部分， codex todo
}

static uint8_t driver_display_get_required_panel_frame_buffer_count(void)
{
    // RGB/MIPI DSI panel creation needs num_fbs up front, so derive it from the
    // same adapter mode/rotation that will be used during display registration.
    return esp_lv_adapter_get_required_frame_buffer_count(driver_display_get_tear_avoid_mode(),
                                                          driver_display_get_rotation());
}

static esp_err_t driver_display_init_lvgl_adapter(void)
{
    if (esp_lv_adapter_is_initialized()) {
        return ESP_OK;
    }

    esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    cfg.task_stack_size = 32 * 1024;
    cfg.task_core_id = 1;
    cfg.stack_in_psram = true;
    cfg.task_priority = 5;

    ESP_LOGI(TAG, "esp_lv_adapter_init: task_stack_size=%u task_priority=%u task_core_id=%d",
             (unsigned)cfg.task_stack_size, (unsigned)cfg.task_priority, (int)cfg.task_core_id);
    return esp_lv_adapter_init(&cfg);
}

esp_err_t driver_display_create(driver_display_handle_t *out_handle)
{
    driver_display_t *handle = NULL;
    esp_err_t ret = ESP_OK;
    uint8_t panel_num_fbs = driver_display_get_required_panel_frame_buffer_count();

    ESP_GOTO_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, err, TAG, "out_handle is NULL");

    handle = calloc(1, sizeof(driver_display_t));
    ESP_GOTO_ON_FALSE(handle, ESP_ERR_NO_MEM, err, TAG, "no memory");

    ESP_LOGI(TAG, "create display: rotation=%d tear_mode=%d required_num_fbs=%u",
             driver_display_get_rotation(), driver_display_get_tear_avoid_mode(), (unsigned)panel_num_fbs);
    ESP_GOTO_ON_ERROR(driver_lcd_panel_create_with_frame_buffers(&handle->lcd_handle, panel_num_fbs),
                      err, TAG, "driver_lcd_panel_create_with_frame_buffers failed");
    ESP_GOTO_ON_ERROR(driver_lcd_panel_get_resolution(handle->lcd_handle,
                                                      &handle->info.hor_res,
                                                      &handle->info.ver_res),
                      err, TAG, "driver_lcd_panel_get_resolution failed");

#if CONFIG_DRIVER_DISPLAY_TOUCH_ENABLED
    if (driver_touch_gt911_create(&handle->touch_handle) == ESP_OK) {
        void *native_touch = NULL;

        if (driver_touch_gt911_get_native_handle(handle->touch_handle, &native_touch) == ESP_OK) {
            handle->touch_native_handle = (esp_lcd_touch_handle_t)native_touch;
            handle->touch_available = true;
        } else {
            ESP_LOGW(TAG, "touch init skipped: failed to fetch native GT911 handle");
            driver_touch_gt911_destroy(handle->touch_handle);
            handle->touch_handle = NULL;
        }
    } else {
        ESP_LOGW(TAG, "touch init skipped: GT911 create failed");
    }
#endif
    handle->info.touch_available = handle->touch_available;

    *out_handle = handle;
    return ESP_OK;

err:
    if (handle && handle->lcd_handle) {
        driver_lcd_panel_destroy(handle->lcd_handle);
    }
    free(handle);
    return ret;
}

esp_err_t driver_display_destroy(driver_display_handle_t handle)
{
    esp_err_t ret = ESP_OK;

    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

#if CONFIG_DRIVER_DISPLAY_TOUCH_ENABLED
    if (handle->touch_handle) {
        esp_err_t err = driver_touch_gt911_destroy(handle->touch_handle);
        if (ret == ESP_OK) {
            ret = err;
        }
    }
#endif

    if (handle->lcd_handle) {
        esp_err_t err = driver_lcd_panel_destroy(handle->lcd_handle);
        if (ret == ESP_OK) {
            ret = err;
        }
    }

    free(handle);
    return ret;
}

esp_err_t driver_display_start(driver_display_handle_t handle)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lv_adapter_rotation_t rotation = driver_display_get_rotation();
    esp_lv_adapter_tear_avoid_mode_t tear_mode = driver_display_get_tear_avoid_mode();

    ESP_RETURN_ON_FALSE(handle && handle->lcd_handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(driver_display_init_lvgl_adapter(), TAG, "esp_lv_adapter_init failed");

    if (!s_lvgl_display) {
        ESP_RETURN_ON_ERROR(driver_lcd_panel_get_handles(handle->lcd_handle, &panel, &io), TAG,
                            "driver_lcd_panel_get_handles failed");

        esp_lv_adapter_display_config_t disp_cfg =
            ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
                panel,
                io,
                handle->info.hor_res,
                handle->info.ver_res,
                rotation);

        disp_cfg.tear_avoid_mode = tear_mode;
        disp_cfg.profile.buffer_height = handle->info.hor_res;
        disp_cfg.profile.use_psram = true;
        disp_cfg.profile.require_double_buffer = true;
        // PPA acceleration requires LV_DRAW_SW_DRAW_UNIT_CNT == 1 (current=2)
        disp_cfg.profile.enable_ppa_accel = false;

        ESP_LOGI(TAG,
                 "register display: panel=%p io=%p hor_res=%u ver_res=%u rotation=%d tear_mode=%d buffer_height=%u use_psram=%d ppa=%d double_buffer=%d",
                 panel, io, handle->info.hor_res, handle->info.ver_res, rotation,
                 disp_cfg.tear_avoid_mode, (unsigned)disp_cfg.profile.buffer_height,
                 disp_cfg.profile.use_psram, disp_cfg.profile.enable_ppa_accel,
                 disp_cfg.profile.require_double_buffer);

        s_lvgl_display = esp_lv_adapter_register_display(&disp_cfg);
        ESP_RETURN_ON_FALSE(s_lvgl_display, ESP_FAIL, TAG, "register display failed");
    }

#if CONFIG_DRIVER_DISPLAY_TOUCH_ENABLED
    if (handle->touch_available && handle->touch_handle && handle->touch_native_handle && !s_lvgl_touch) {
        esp_lv_adapter_touch_config_t touch_cfg =
            ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_lvgl_display, handle->touch_native_handle);
        s_lvgl_touch = esp_lv_adapter_register_touch(&touch_cfg);
        ESP_RETURN_ON_FALSE(s_lvgl_touch, ESP_FAIL, TAG, "register touch failed");
    }
#endif

    if (!s_lvgl_started) {
        ESP_RETURN_ON_ERROR(esp_lv_adapter_start(), TAG, "esp_lv_adapter_start failed");
        s_lvgl_started = true;
    }

    return ESP_OK;
}

esp_err_t driver_display_get_info(driver_display_handle_t handle, driver_display_info_t *out_info)
{
    ESP_RETURN_ON_FALSE(handle && out_info, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_info = handle->info;
    return ESP_OK;
}

esp_err_t driver_display_get_resolution(driver_display_handle_t handle, uint16_t *out_hor_res, uint16_t *out_ver_res)
{
    ESP_RETURN_ON_FALSE(handle && out_hor_res && out_ver_res, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_hor_res = handle->info.hor_res;
    *out_ver_res = handle->info.ver_res;
    return ESP_OK;
}

esp_err_t driver_display_read_touch(driver_display_handle_t handle, bool *pressed, uint16_t *x, uint16_t *y)
{
    ESP_RETURN_ON_FALSE(handle && pressed && x && y, ESP_ERR_INVALID_ARG, TAG, "invalid args");

#if CONFIG_DRIVER_DISPLAY_TOUCH_ENABLED
    ESP_RETURN_ON_FALSE(handle->touch_handle, ESP_ERR_INVALID_STATE, TAG, "touch unavailable");
    ESP_RETURN_ON_ERROR(driver_touch_gt911_read_data(handle->touch_handle), TAG, "read touch failed");
    return driver_touch_gt911_get_data(handle->touch_handle, pressed, x, y);
#else
    (void)handle;
    *pressed = false;
    *x = 0;
    *y = 0;
    return ESP_ERR_INVALID_STATE;
#endif
}

esp_err_t driver_display_lock(driver_display_handle_t handle, int32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return esp_lv_adapter_lock(timeout_ms);
}

esp_err_t driver_display_unlock(driver_display_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    esp_lv_adapter_unlock();
    return ESP_OK;
}

esp_err_t driver_display_set_hw_pattern(driver_display_handle_t handle, mipi_dsi_pattern_type_t pattern)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io = NULL;

    ESP_RETURN_ON_FALSE(handle && handle->lcd_handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(driver_lcd_panel_get_handles(handle->lcd_handle, &panel, &io), TAG,
                        "driver_lcd_panel_get_handles failed");
    return esp_lcd_dpi_panel_set_pattern(panel, pattern);
}
