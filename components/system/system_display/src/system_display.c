#include "system_display.h"

#include "esp_check.h"
#include "esp_lv_adapter.h"
#include "esp_lcd_mipi_dsi.h"
#include "lvgl.h"

#include "driver_display.h"
#include "system_display_internal.h"

static const char *TAG = "system_display";

static driver_display_handle_t s_display = NULL;
static lv_display_t *s_disp = NULL;
static bool s_lvgl_started = false;
static system_display_ui_init_fn_t s_ui_init_cb = NULL;

esp_err_t system_display_init(void)
{
    if (s_disp) {
        ESP_LOGI(TAG, "display already initialized: s_display=%p s_disp=%p lvgl_started=%d",
                 s_display, s_disp, s_lvgl_started);
        return ESP_OK;
    }

    if (!s_display) {
        ESP_RETURN_ON_ERROR(driver_display_create(&s_display), TAG, "driver_display_create failed");
        ESP_LOGI(TAG, "driver_display_create ok: s_display=%p", s_display);
    }

    if (!s_lvgl_started) {
        esp_lcd_panel_handle_t panel = NULL;
        esp_lcd_panel_io_handle_t io = NULL;
        uint16_t hor_res = 0;
        uint16_t ver_res = 0;
        esp_lcd_touch_handle_t touch = NULL;
        esp_err_t touch_ret = ESP_FAIL;

        ESP_RETURN_ON_ERROR(driver_display_get_panel_handle(s_display, &panel, &io), TAG, "get panel failed");
        ESP_LOGI(TAG, "driver_display_get_panel_handle: s_display=%p panel=%p io=%p", s_display, panel, io);
        ESP_RETURN_ON_ERROR(driver_display_get_resolution(s_display, &hor_res, &ver_res), TAG, "get res failed");
        ESP_LOGI(TAG, "driver_display_get_resolution: hor_res=%u ver_res=%u", hor_res, ver_res);
        touch_ret = driver_display_get_touch_handle(s_display, &touch);
        ESP_LOGI(TAG, "driver_display_get_touch_handle: ret=%s(%d) touch=%p",
                 esp_err_to_name(touch_ret), touch_ret, touch);

        esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
        cfg.task_stack_size = 32 * 1024;
        cfg.task_core_id = 1;   // 固定在核心 1 运行（视具体芯片而定，P4/S3 建议固定）
        cfg.stack_in_psram = true;       // 将任务栈放入 PSRAM        
        cfg.task_priority = 5;           // 保持默认或根据系统负载适当调整

        ESP_LOGI(TAG, "esp_lv_adapter_init: task_stack_size=%u task_priority=%u task_core_id=%d",
                 (unsigned)cfg.task_stack_size, (unsigned)cfg.task_priority, (int)cfg.task_core_id);
        ESP_RETURN_ON_ERROR(esp_lv_adapter_init(&cfg), TAG, "esp_lv_adapter_init failed");

        esp_lv_adapter_display_config_t disp_cfg =
            ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
                panel,
                io,
                hor_res,
                ver_res,
                ESP_LV_ADAPTER_ROTATE_90);

        // 参考工程在横屏下使用全屏双缓冲，避免仍按面板原生 800x1280 方向显示。
        disp_cfg.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_FULL;
        disp_cfg.profile.buffer_height = hor_res;
        disp_cfg.profile.use_psram = true;
        disp_cfg.profile.require_double_buffer = true;
        ESP_LOGI(TAG,
                 "register display: panel=%p io=%p hor_res=%u ver_res=%u rotation=%d tear_mode=%d buffer_height=%u use_psram=%d double_buffer=%d",
                 panel, io, hor_res, ver_res, ESP_LV_ADAPTER_ROTATE_90, disp_cfg.tear_avoid_mode,
                 (unsigned)disp_cfg.profile.buffer_height, disp_cfg.profile.use_psram,
                 disp_cfg.profile.require_double_buffer);

        s_disp = esp_lv_adapter_register_display(&disp_cfg);
        ESP_RETURN_ON_FALSE(s_disp, ESP_FAIL, TAG, "register display failed");
        ESP_LOGI(TAG, "esp_lv_adapter_register_display ok: s_disp=%p", s_disp);

        if (touch_ret == ESP_OK && touch) {
            esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_disp, touch);
            ESP_LOGI(TAG, "register touch: s_disp=%p touch=%p", s_disp, touch);
            lv_indev_t *indev = esp_lv_adapter_register_touch(&touch_cfg);
            ESP_RETURN_ON_FALSE(indev, ESP_FAIL, TAG, "register touch failed");
            ESP_LOGI(TAG, "esp_lv_adapter_register_touch ok: indev=%p", indev);
        } else {
            ESP_LOGW(TAG, "touch registration skipped: ret=%s(%d) touch=%p",
                     esp_err_to_name(touch_ret), touch_ret, touch);
        }

        ESP_RETURN_ON_ERROR(esp_lv_adapter_start(), TAG, "esp_lv_adapter_start failed");
        s_lvgl_started = true;
        ESP_LOGI(TAG, "esp_lv_adapter_start ok: lvgl_started=%d", s_lvgl_started);
    }

    return ESP_OK;
}

esp_err_t system_display_register_ui_init_cb(system_display_ui_init_fn_t cb)
{
    ESP_RETURN_ON_FALSE(cb, ESP_ERR_INVALID_ARG, TAG, "callback is NULL");
    s_ui_init_cb = cb;
    return ESP_OK;
}

esp_err_t system_ui_init(void)
{
    ESP_RETURN_ON_FALSE(s_ui_init_cb, ESP_ERR_INVALID_STATE, TAG, "ui init callback not registered");
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");
    ESP_RETURN_ON_ERROR(system_display_lock(), TAG, "system_display_lock failed");
    s_ui_init_cb();
    ESP_RETURN_ON_ERROR(system_display_unlock(), TAG, "system_display_unlock failed");


    // system_display_set_hw_pattern(MIPI_DSI_PATTERN_BAR_HORIZONTAL);
    printf("Minimum free heap size: %" PRIu32 " Mbytes\n", esp_get_minimum_free_heap_size()/(1024*1024));
    return ESP_OK;
}

esp_err_t system_display_lock(void)
{
    return esp_lv_adapter_lock(-1);
}

esp_err_t system_display_unlock(void)
{
    esp_lv_adapter_unlock();
    return ESP_OK;
}

esp_err_t system_display_get_resolution(uint16_t *out_hor_res, uint16_t *out_ver_res)
{
    ESP_RETURN_ON_FALSE(out_hor_res && out_ver_res, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_get_resolution(s_display, out_hor_res, out_ver_res);
}

esp_err_t system_display_get_panel_handles(esp_lcd_panel_handle_t *out_panel,
                                           esp_lcd_panel_io_handle_t *out_io)
{
    ESP_RETURN_ON_FALSE(s_display, ESP_ERR_INVALID_STATE, TAG, "display not initialized");
    return driver_display_get_panel_handle(s_display, out_panel, out_io);
}

esp_err_t system_display_set_hw_pattern(mipi_dsi_pattern_type_t pattern)
{
    ESP_RETURN_ON_ERROR(system_display_init(), TAG, "system_display_init failed");

    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io = NULL;
    ESP_RETURN_ON_ERROR(system_display_get_panel_handles(&panel, &io), TAG, "get panel failed");
    ESP_LOGI(TAG, "set hw pattern: panel=%p io=%p pattern=%d", panel, io, pattern);

    return esp_lcd_dpi_panel_set_pattern(panel, pattern);
}
