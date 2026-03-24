#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "app_ui_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_ui_controller *app_ui_controller_handle_t;

typedef struct {
    uint32_t recipe_id;
    uint32_t plan_id;
    uint32_t step_id;
    bool auto_start_enabled;
} app_ui_controller_test_config_t;

typedef enum {
    APP_UI_CONTROLLER_COMMAND_START_TEST = 0,
    APP_UI_CONTROLLER_COMMAND_STOP_TEST,
    APP_UI_CONTROLLER_COMMAND_MANUAL_RESCAN_INSTRUMENT,
    APP_UI_CONTROLLER_COMMAND_MANUAL_RESCAN_MODULE,
    APP_UI_CONTROLLER_COMMAND_CLEAR_INSTRUMENT_BINDING,
    APP_UI_CONTROLLER_COMMAND_TRIGGER_VISION_CHECK,
    APP_UI_CONTROLLER_COMMAND_START_USB_OTA,
    APP_UI_CONTROLLER_COMMAND_ENABLE_USB_MSC,
    APP_UI_CONTROLLER_COMMAND_PAGE_CHANGED,
    APP_UI_CONTROLLER_COMMAND_TEST_CONFIG_CHANGED,
} app_ui_controller_command_id_t;

typedef struct {
    app_ui_controller_command_id_t command_id;
    union {
        struct {
            char update_file_path[128];
        } start_usb_ota;
        struct {
            bool enable;
        } enable_usb_msc;
        struct {
            app_ui_view_page_t page;
        } page_changed;
        struct {
            app_ui_controller_test_config_t config;
        } test_config_changed;
    } payload;
} app_ui_controller_command_t;

typedef esp_err_t (*app_ui_controller_on_command_fn_t)(void *user_context,
                                                       const app_ui_controller_command_t *command);

typedef struct {
    app_ui_controller_on_command_fn_t on_command;
    void *user_context;
} app_ui_controller_config_t;

esp_err_t app_ui_controller_init(const app_ui_controller_config_t *config,
                                 app_ui_controller_handle_t *out_handle);
esp_err_t app_ui_controller_start(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_stop(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_deinit(app_ui_controller_handle_t handle);

esp_err_t app_ui_controller_on_start_test_request(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_on_stop_test_request(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_on_manual_rescan_instrument(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_on_manual_rescan_module(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_on_clear_instrument_binding(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_on_trigger_vision_check(app_ui_controller_handle_t handle);
esp_err_t app_ui_controller_on_start_usb_ota(app_ui_controller_handle_t handle, const char *update_file_path);
esp_err_t app_ui_controller_on_enable_usb_msc(app_ui_controller_handle_t handle, bool enable);
esp_err_t app_ui_controller_on_page_changed(app_ui_controller_handle_t handle, app_ui_view_page_t page);
esp_err_t app_ui_controller_on_test_config_changed(app_ui_controller_handle_t handle,
                                                    const app_ui_controller_test_config_t *config);

#ifdef __cplusplus
}
#endif
