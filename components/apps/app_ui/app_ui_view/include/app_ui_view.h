#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_ui_view_init(void);
esp_err_t app_ui_view_deinit(void);

#ifdef __cplusplus
}
#endif
