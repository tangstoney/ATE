#include "driver_uart_instrument_test.h"

#include <string.h>

#include "board_ate_p4.h"
#include "driver/uart.h"
#include "driver_uart_instrument.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "uart_instr_test";

typedef struct {
    uart_port_t port;
    gpio_num_t tx_io;
    gpio_num_t rx_io;
} driver_uart_instrument_test_slot_info_t;

static const driver_uart_instrument_test_slot_info_t s_slot_info[DRIVER_UART_INSTRUMENT_MAX] = {
    [0] = {
        .port = BOARD_UART_INSTR_0_PORT,
        .tx_io = BOARD_UART_INSTR_0_TX,
        .rx_io = BOARD_UART_INSTR_0_RX,
    },
    [1] = {
        .port = BOARD_UART_INSTR_1_PORT,
        .tx_io = BOARD_UART_INSTR_1_TX,
        .rx_io = BOARD_UART_INSTR_1_RX,
    },
    [2] = {
        .port = BOARD_UART_INSTR_2_PORT,
        .tx_io = BOARD_UART_INSTR_2_TX,
        .rx_io = BOARD_UART_INSTR_2_RX,
    },
    [3] = {
        .port = BOARD_UART_INSTR_3_PORT,
        .tx_io = BOARD_UART_INSTR_3_TX,
        .rx_io = BOARD_UART_INSTR_3_RX,
    },
};

static const driver_uart_instrument_test_slot_info_t *get_slot_info(uint8_t slot_index)
{
    return slot_index < DRIVER_UART_INSTRUMENT_MAX ? &s_slot_info[slot_index] : NULL;
}

static void fill_pattern(uint8_t *buffer, size_t len, uint8_t pattern)
{
    memset(buffer, pattern, len);
}

static esp_err_t run_one_case(uint8_t slot_index,
                              driver_uart_instrument_handle_t handle,
                              uint8_t pattern,
                              size_t len)
{
    uint8_t tx[64] = {0};
    uint8_t rx[64] = {0};
    size_t buffered = 0;
    const driver_uart_instrument_test_slot_info_t *slot_info = get_slot_info(slot_index);

    ESP_RETURN_ON_FALSE(len <= sizeof(tx), ESP_ERR_INVALID_ARG, TAG, "len too large");
    ESP_RETURN_ON_FALSE(slot_info, ESP_ERR_INVALID_ARG, TAG, "invalid slot index");

    fill_pattern(tx, len, pattern);
    ESP_RETURN_ON_ERROR(driver_uart_instrument_flush_rx(handle), TAG, "flush rx failed");
    ESP_RETURN_ON_ERROR(driver_uart_instrument_write(handle, tx, len), TAG, "write failed");
    ESP_RETURN_ON_ERROR(uart_get_buffered_data_len(slot_info->port, &buffered),
                        TAG, "uart_get_buffered_data_len failed");
    ESP_LOGI(TAG,
             "slot=%u port=%d tx_gpio=%d rx_gpio=%d rs485_half=%d rx_buffered=%u len=%u pattern=0x%02X",
             (unsigned)slot_index,
             (int)slot_info->port,
             (int)slot_info->tx_io,
             (int)slot_info->rx_io,
             BOARD_INSTR_LINK_MODE_RS485_HALF,
             (unsigned)buffered,
             (unsigned)len,
             pattern);
    ESP_RETURN_ON_ERROR(driver_uart_instrument_read(handle, rx, len, 200), TAG, "read failed");

    if (memcmp(tx, rx, len) != 0) {
        for (size_t i = 0; i < len; ++i) {
            if (tx[i] != rx[i]) {
                ESP_LOGE(TAG,
                         "mismatch: slot=%u pattern=0x%02X len=%u index=%u tx=0x%02X rx=0x%02X",
                         (unsigned)slot_index,
                         pattern,
                         (unsigned)len,
                         (unsigned)i,
                         tx[i],
                         rx[i]);
                break;
            }
        }
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "loopback pass: slot=%u pattern=0x%02X len=%u",
             (unsigned)slot_index,
             pattern,
             (unsigned)len);
    return ESP_OK;
}

static esp_err_t run_all_cases(uint8_t slot_index,
                               driver_uart_instrument_handle_t handle)
{
    static const uint8_t patterns[] = {0x00, 0x55, 0xAA, 0xFF};
    static const size_t lengths[] = {1U, 16U, 64U};

    for (size_t p = 0; p < sizeof(patterns); ++p) {
        for (size_t l = 0; l < (sizeof(lengths) / sizeof(lengths[0])); ++l) {
            ESP_RETURN_ON_ERROR(run_one_case(slot_index, handle, patterns[p], lengths[l]),
                                TAG, "loopback case failed");
        }
    }

    ESP_LOGI(TAG, "all loopback cases passed for slot=%u", (unsigned)slot_index);
    return ESP_OK;
}

esp_err_t driver_uart_instrument_test_loopback_run(uint8_t slot_index)
{
    driver_uart_instrument_handle_t handle = NULL;

    ESP_RETURN_ON_ERROR(driver_uart_instrument_install(slot_index, &handle),
                        TAG, "driver_uart_instrument_install failed");
    return run_all_cases(slot_index, handle);
}

esp_err_t driver_uart_instrument_test_loopback_run_internal(uint8_t slot_index)
{
    driver_uart_instrument_handle_t handle = NULL;
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_ERROR(driver_uart_instrument_install(slot_index, &handle),
                        TAG, "driver_uart_instrument_install failed");
    ESP_RETURN_ON_ERROR(driver_uart_instrument_set_loopback(handle, true),
                        TAG, "enable internal loopback failed");
    ESP_LOGI(TAG, "internal loopback enabled for slot=%u", (unsigned)slot_index);

    ret = run_all_cases(slot_index, handle);

    esp_err_t disable_ret = driver_uart_instrument_set_loopback(handle, false);
    if (disable_ret != ESP_OK) {
        ESP_LOGE(TAG, "disable internal loopback failed for slot=%u: %s",
                 (unsigned)slot_index, esp_err_to_name(disable_ret));
        if (ret == ESP_OK) {
            ret = disable_ret;
        }
    }

    return ret;
}
