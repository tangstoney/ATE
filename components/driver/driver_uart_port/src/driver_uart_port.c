#include "driver_uart_port.h"

#include <stdbool.h>
#include <stdlib.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"

typedef struct driver_uart_port {
    driver_uart_port_config_t config;
    bool installed;
} driver_uart_port_t;

static const char *TAG = "driver_uart_port";

static int normalize_pin(int io)
{
    return io >= 0 ? io : UART_PIN_NO_CHANGE;
}

static uint32_t normalize_baudrate(uint32_t baud_rate)
{
    return baud_rate ? baud_rate : 115200;
}

static int normalize_timeout_ms(int timeout_ms)
{
    return timeout_ms > 0 ? timeout_ms : 1000;
}

static int normalize_rx_buf_size(int rx_buf_size)
{
    return rx_buf_size > 0 ? rx_buf_size : 1024;
}

static int normalize_tx_buf_size(int tx_buf_size)
{
    return tx_buf_size > 0 ? tx_buf_size : 1024;
}

static esp_err_t driver_uart_port_open(driver_uart_port_t *handle,
                                       const driver_uart_port_config_t *config)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(handle && config, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(config->tx_io >= 0 && config->rx_io >= 0, ESP_ERR_INVALID_ARG, TAG, "invalid io");

    if (config->mode == DRIVER_UART_MODE_RS485_HALF) {
        ESP_RETURN_ON_FALSE(config->rts_io >= 0, ESP_ERR_INVALID_ARG, TAG, "RS485 requires RTS");
    }

    uart_config_t uart_cfg = {
        .baud_rate = (int)normalize_baudrate(config->baud_rate),
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ret = uart_driver_install(config->port,
                              normalize_rx_buf_size(config->rx_buf_size),
                              normalize_tx_buf_size(config->tx_buf_size),
                              0,
                              NULL,
                              0);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "uart_driver_install failed");

    ret = uart_param_config(config->port, &uart_cfg);
    ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_param_config failed");
    ret = uart_set_pin(config->port,
                       normalize_pin(config->tx_io),
                       normalize_pin(config->rx_io),
                       normalize_pin(config->rts_io),
                       UART_PIN_NO_CHANGE);
    ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_set_pin failed");

    if (config->mode == DRIVER_UART_MODE_RS485_HALF) {
        ret = uart_set_mode(config->port, UART_MODE_RS485_HALF_DUPLEX);
        ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_set_mode RS485 failed");
    } else {
        ret = uart_set_mode(config->port, UART_MODE_UART);
        ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_set_mode UART failed");
    }

    handle->config = *config;
    handle->config.baud_rate = normalize_baudrate(config->baud_rate);
    handle->config.timeout_ms = normalize_timeout_ms(config->timeout_ms);
    handle->config.rx_buf_size = normalize_rx_buf_size(config->rx_buf_size);
    handle->config.tx_buf_size = normalize_tx_buf_size(config->tx_buf_size);
    handle->installed = true;
    return ESP_OK;

err_delete:
    uart_driver_delete(config->port);
err:
    return ret;
}

static esp_err_t driver_uart_port_close(driver_uart_port_t *handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    if (!handle->installed) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(uart_driver_delete(handle->config.port), TAG, "uart_driver_delete failed");
    handle->installed = false;
    return ESP_OK;
}

esp_err_t driver_uart_port_create(const driver_uart_port_config_t *config,
                                  driver_uart_port_handle_t *out_handle)
{
    driver_uart_port_t *handle = NULL;

    ESP_RETURN_ON_FALSE(config && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    handle = calloc(1, sizeof(*handle));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");

    esp_err_t err = driver_uart_port_open(handle, config);
    if (err != ESP_OK) {
        free(handle);
        return err;
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_uart_port_delete(driver_uart_port_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(driver_uart_port_close(handle), TAG, "close failed");
    free(handle);
    return ESP_OK;
}

esp_err_t driver_uart_port_set_baudrate(driver_uart_port_handle_t handle,
                                        uint32_t baud_rate)
{
    ESP_RETURN_ON_FALSE(handle && handle->installed, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(baud_rate > 0, ESP_ERR_INVALID_ARG, TAG, "invalid baudrate");
    ESP_RETURN_ON_ERROR(uart_set_baudrate(handle->config.port, baud_rate), TAG, "uart_set_baudrate failed");
    handle->config.baud_rate = baud_rate;
    return ESP_OK;
}

esp_err_t driver_uart_port_reconfigure(driver_uart_port_handle_t handle,
                                       const driver_uart_port_config_t *config)
{
    ESP_RETURN_ON_FALSE(handle && config, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(driver_uart_port_close(handle), TAG, "close failed");
    return driver_uart_port_open(handle, config);
}

esp_err_t driver_uart_port_recover(driver_uart_port_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(driver_uart_port_close(handle), TAG, "close failed");
    return driver_uart_port_open(handle, &handle->config);
}

esp_err_t driver_uart_write(driver_uart_port_handle_t handle,
                            const uint8_t *data,
                            size_t len)
{
    int written = 0;

    ESP_RETURN_ON_FALSE(handle && handle->installed && data && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    written = uart_write_bytes(handle->config.port, data, len);
    ESP_RETURN_ON_FALSE(written >= 0, ESP_FAIL, TAG, "uart_write_bytes failed");
    ESP_RETURN_ON_FALSE((size_t)written == len, ESP_FAIL, TAG, "short write");
    return uart_wait_tx_done(handle->config.port, pdMS_TO_TICKS(handle->config.timeout_ms));
}

esp_err_t driver_uart_read(driver_uart_port_handle_t handle,
                           uint8_t *buf,
                           size_t len,
                           size_t *out_len)
{
    int read_len = 0;

    ESP_RETURN_ON_FALSE(handle && handle->installed && buf && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    read_len = uart_read_bytes(handle->config.port,
                               buf,
                               len,
                               pdMS_TO_TICKS(handle->config.timeout_ms));
    ESP_RETURN_ON_FALSE(read_len >= 0, ESP_FAIL, TAG, "uart_read_bytes failed");

    if (out_len) {
        *out_len = (size_t)read_len;
    }

    return read_len > 0 ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t driver_uart_flush(driver_uart_port_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle && handle->installed, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return uart_flush(handle->config.port);
}
