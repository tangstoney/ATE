#include "app_ui_controller.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_check.h"
#include "esp_log.h"
#include "app_ui_view.h"

// TODO: app_ui_controller and app_ui_view still share EEZ generated ui.h directly.
// Split the render boundary later after the UI architecture settles.
#include "ui.h"

struct app_ui_controller {
    app_ui_page_t current_page;
    app_ui_controller_test_config_t last_test_config;
    uint8_t usb_ota_progress_percent;
    esp_event_handler_instance_t ui_event_handler;
    bool initialized;
};

static const char *TAG = "app_ui_controller";
static struct app_ui_controller s_controller;

ESP_EVENT_DEFINE_BASE(APP_UI_CTRL_EVENT);
ESP_EVENT_DEFINE_BASE(APP_UI_EVENT);

static esp_err_t app_ui_controller_require_initialized(void)
{
    ESP_RETURN_ON_FALSE(s_controller.initialized, ESP_ERR_INVALID_STATE, TAG, "controller not initialized");
    return ESP_OK;
}

static esp_err_t app_ui_controller_render_page(app_ui_page_t page)
{
    switch (page) {
    case APP_UI_PAGE_MAIN:
        loadScreen(SCREEN_ID_MAIN);
        return ESP_OK;
    case APP_UI_PAGE_PAGE1:
        loadScreen(SCREEN_ID_PAGE1);
        return ESP_OK;
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

static esp_err_t app_ui_controller_post_ctrl_event(int32_t event_id,
                                                   const void *event_data,
                                                   size_t event_data_size)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_initialized(), TAG, "controller not ready");
    return esp_event_post(APP_UI_CTRL_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_ui_controller_post_ui_event(int32_t event_id,
                                                 const void *event_data,
                                                 size_t event_data_size)
{
    ESP_RETURN_ON_ERROR(app_ui_controller_require_initialized(), TAG, "controller not ready");
    return esp_event_post(APP_UI_EVENT,
                          event_id,
                          event_data,
                          event_data_size,
                          pdMS_TO_TICKS(100));
}

static esp_err_t app_ui_controller_dispatch_ui_event(int32_t event_id, const void *event_data)
{
    switch (event_id) {
    case APP_UI_EVENT_SHOW_DEFAULT:
        s_controller.current_page = APP_UI_PAGE_MAIN;
        return app_ui_controller_render_page(APP_UI_PAGE_MAIN);
    case APP_UI_EVENT_SHOW_PAGE: {
        const app_ui_page_event_t *page_event = event_data;
        ESP_RETURN_ON_FALSE(page_event, ESP_ERR_INVALID_ARG, TAG, "page_event is NULL");
        s_controller.current_page = page_event->page;
        return app_ui_controller_render_page(page_event->page);
    }
    case APP_UI_EVENT_USB_OTA_PROGRESS: {
        const app_ui_usb_ota_progress_event_t *progress_event = event_data;
        ESP_RETURN_ON_FALSE(progress_event, ESP_ERR_INVALID_ARG, TAG, "progress_event is NULL");
        s_controller.usb_ota_progress_percent = progress_event->progress_percent;
        return ESP_OK;
    }
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

static void app_ui_controller_ui_event_handler(void *handler_arg,
                                               esp_event_base_t event_base,
                                               int32_t event_id,
                                               void *event_data)
{
    esp_err_t ret = ESP_OK;

    (void)handler_arg;
    (void)event_base;

    ret = app_ui_controller_dispatch_ui_event(event_id, event_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "dispatch ui event %ld failed: %s", (long)event_id, esp_err_to_name(ret));
    }
}

esp_err_t app_ui_controller_init(app_ui_controller_handle_t *out_handle)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");

    if (s_controller.initialized) {
        *out_handle = &s_controller;
        return ESP_OK;
    }

    memset(&s_controller, 0, sizeof(s_controller));

    ESP_RETURN_ON_ERROR(app_ui_view_init(), TAG, "ui view init failed");

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(APP_UI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            app_ui_controller_ui_event_handler,
                                                            NULL,
                                                            &s_controller.ui_event_handler),
                        TAG,
                        "register ui event handler failed");

    s_controller.initialized = true;
    *out_handle = &s_controller;
    ret = app_ui_controller_post_ui_event(APP_UI_EVENT_SHOW_DEFAULT, NULL, 0);
    if (ret != ESP_OK) {
        (void)esp_event_handler_instance_unregister(APP_UI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    s_controller.ui_event_handler);
        (void)app_ui_view_deinit();
        memset(&s_controller, 0, sizeof(s_controller));
        return ret;
    }

    return ESP_OK;
}

esp_err_t app_ui_controller_deinit(app_ui_controller_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(handle == &s_controller, ESP_ERR_INVALID_ARG, TAG, "handle is invalid");

    if (!s_controller.initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(APP_UI_EVENT,
                                                              ESP_EVENT_ANY_ID,
                                                              s_controller.ui_event_handler),
                        TAG,
                        "unregister ui event handler failed");
    ESP_RETURN_ON_ERROR(app_ui_view_deinit(), TAG, "ui view deinit failed");

    memset(&s_controller, 0, sizeof(s_controller));
    return ESP_OK;
}

esp_err_t app_ui_controller_on_start_test_request(void)
{
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_START_TEST, NULL, 0);
}

esp_err_t app_ui_controller_on_stop_test_request(void)
{
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_STOP_TEST, NULL, 0);
}

esp_err_t app_ui_controller_on_manual_rescan_instrument(void)
{
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_MANUAL_RESCAN_INSTRUMENT, NULL, 0);
}

esp_err_t app_ui_controller_on_manual_rescan_module(void)
{
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_MANUAL_RESCAN_MODULE, NULL, 0);
}

esp_err_t app_ui_controller_on_clear_instrument_binding(void)
{
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_CLEAR_INSTRUMENT_BINDING, NULL, 0);
}

esp_err_t app_ui_controller_on_trigger_vision_check(void)
{
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_TRIGGER_VISION_CHECK, NULL, 0);
}

esp_err_t app_ui_controller_on_start_usb_ota(const char *update_file_path)
{
    app_ui_controller_start_usb_ota_event_t event = {0};

    if (update_file_path) {
        snprintf(event.update_file_path,
                 sizeof(event.update_file_path),
                 "%s",
                 update_file_path);
    }

    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_START_USB_OTA, &event, sizeof(event));
}

esp_err_t app_ui_controller_on_enable_usb_msc(bool enable)
{
    app_ui_controller_enable_usb_msc_event_t event = {
        .enable = enable,
    };

    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_ENABLE_USB_MSC, &event, sizeof(event));
}

esp_err_t app_ui_controller_on_page_changed(app_ui_page_t page)
{
    app_ui_page_event_t event = {
        .page = page,
    };

    return app_ui_controller_post_ui_event(APP_UI_EVENT_SHOW_PAGE, &event, sizeof(event));
}

esp_err_t app_ui_controller_on_test_config_changed(const app_ui_controller_test_config_t *config)
{
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_ERROR(app_ui_controller_require_initialized(), TAG, "controller not ready");

    s_controller.last_test_config = *config;
    return app_ui_controller_post_ctrl_event(APP_UI_CTRL_EVENT_TEST_CONFIG_CHANGED,
                                             config,
                                             sizeof(*config));
}
