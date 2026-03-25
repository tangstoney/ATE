#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t driver_uart_instrument_test_loopback_run(uint8_t slot_index);
esp_err_t driver_uart_instrument_test_loopback_run_internal(uint8_t slot_index);

#ifdef __cplusplus
}
#endif
