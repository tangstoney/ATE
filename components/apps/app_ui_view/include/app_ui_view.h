#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_UI_VIEW_PAGE_MAIN = 0,
    APP_UI_VIEW_PAGE_PAGE1,
} app_ui_view_page_t;

esp_err_t app_ui_view_init(void);
esp_err_t app_ui_view_start(void);
esp_err_t app_ui_view_stop(void);
esp_err_t app_ui_view_deinit(void);

esp_err_t app_ui_view_show_default(void);
esp_err_t app_ui_view_show_page(app_ui_view_page_t page);
esp_err_t app_ui_view_show_usb_ota_progress(uint8_t progress_percent);

#ifdef __cplusplus
}
#endif
