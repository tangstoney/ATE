#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_UI_CONTROLLER_OTA_FILE_PATH_MAX_LEN 128

typedef struct app_ui_controller *app_ui_controller_handle_t;

typedef enum {
    APP_UI_PAGE_MAIN = 0,
    APP_UI_PAGE_PAGE1,
} app_ui_page_t;

typedef struct {
    uint32_t recipe_id;
    uint32_t plan_id;
    uint32_t step_id;
    bool auto_start_enabled;
} app_ui_controller_test_config_t;

ESP_EVENT_DECLARE_BASE(APP_UI_CTRL_EVENT);
ESP_EVENT_DECLARE_BASE(APP_UI_EVENT);

typedef enum {
    APP_UI_CTRL_EVENT_START_TEST = 0,
    APP_UI_CTRL_EVENT_STOP_TEST,
    APP_UI_CTRL_EVENT_MANUAL_RESCAN_INSTRUMENT,
    APP_UI_CTRL_EVENT_MANUAL_RESCAN_MODULE,
    APP_UI_CTRL_EVENT_CLEAR_INSTRUMENT_BINDING,
    APP_UI_CTRL_EVENT_TRIGGER_VISION_CHECK,
    APP_UI_CTRL_EVENT_START_USB_OTA,
    APP_UI_CTRL_EVENT_ENABLE_USB_MSC,
    APP_UI_CTRL_EVENT_TEST_CONFIG_CHANGED,
} app_ui_controller_event_id_t;

typedef struct {
    char update_file_path[APP_UI_CONTROLLER_OTA_FILE_PATH_MAX_LEN];
} app_ui_controller_start_usb_ota_event_t;

typedef struct {
    bool enable;
} app_ui_controller_enable_usb_msc_event_t;

typedef enum {
    APP_UI_EVENT_SHOW_DEFAULT = 0,
    APP_UI_EVENT_SHOW_PAGE,
    APP_UI_EVENT_USB_OTA_PROGRESS,
} app_ui_event_id_t;

typedef struct {
    app_ui_page_t page;
} app_ui_page_event_t;

typedef struct {
    uint8_t progress_percent;
} app_ui_usb_ota_progress_event_t;

esp_err_t app_ui_controller_init(app_ui_controller_handle_t *out_handle);
esp_err_t app_ui_controller_deinit(app_ui_controller_handle_t handle);

esp_err_t app_ui_controller_on_start_test_request(void);
esp_err_t app_ui_controller_on_stop_test_request(void);
esp_err_t app_ui_controller_on_manual_rescan_instrument(void);
esp_err_t app_ui_controller_on_manual_rescan_module(void);
esp_err_t app_ui_controller_on_clear_instrument_binding(void);
esp_err_t app_ui_controller_on_trigger_vision_check(void);
esp_err_t app_ui_controller_on_start_usb_ota(const char *update_file_path);
esp_err_t app_ui_controller_on_enable_usb_msc(bool enable);
esp_err_t app_ui_controller_on_page_changed(app_ui_page_t page);
esp_err_t app_ui_controller_on_test_config_changed(const app_ui_controller_test_config_t *config);

#ifdef __cplusplus
}
#endif
