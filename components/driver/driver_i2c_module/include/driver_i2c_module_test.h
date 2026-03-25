#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t driver_i2c_module_test_probe_once(void);
esp_err_t driver_i2c_module_test_write_default_pattern(void);

#ifdef __cplusplus
}
#endif
