#include "driver_uart_instrument.h"

#include "board_ate_p4.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

struct driver_uart_instrument_t {
    uint8_t slot_index;
    uart_port_t port;
    uint32_t default_timeout_ms;
};

static const char *TAG = "driver_uart_instr";
typedef struct {
    uart_port_t port;
    gpio_num_t tx_io;
    gpio_num_t rx_io;
    gpio_num_t rts_io;
    gpio_num_t cts_io;
    uint32_t baud_rate;
    uint32_t rx_buf_size;
    uint32_t tx_buf_size;
    uint32_t timeout_ms;
} driver_uart_instrument_slot_config_t;

static const driver_uart_instrument_slot_config_t s_slot_configs[DRIVER_UART_INSTRUMENT_MAX] = {
    [0] = {
        .port = BOARD_UART_INSTR_0_PORT,
        .tx_io = BOARD_UART_INSTR_0_TX,
        .rx_io = BOARD_UART_INSTR_0_RX,
        .rts_io = BOARD_UART_INSTR_0_RTS,
        .cts_io = BOARD_UART_INSTR_0_CTS,
        .baud_rate = BOARD_UART_INSTR_0_BAUD_DEFAULT,
        .rx_buf_size = BOARD_UART_INSTR_0_RX_BUF_SIZE,
        .tx_buf_size = BOARD_UART_INSTR_0_TX_BUF_SIZE,
        .timeout_ms = BOARD_UART_INSTR_0_TIMEOUT_MS,
    },
    [1] = {
        .port = BOARD_UART_INSTR_1_PORT,
        .tx_io = BOARD_UART_INSTR_1_TX,
        .rx_io = BOARD_UART_INSTR_1_RX,
        .rts_io = BOARD_UART_INSTR_1_RTS,
        .cts_io = BOARD_UART_INSTR_1_CTS,
        .baud_rate = BOARD_UART_INSTR_1_BAUD_DEFAULT,
        .rx_buf_size = BOARD_UART_INSTR_1_RX_BUF_SIZE,
        .tx_buf_size = BOARD_UART_INSTR_1_TX_BUF_SIZE,
        .timeout_ms = BOARD_UART_INSTR_1_TIMEOUT_MS,
    },
    [2] = {
        .port = BOARD_UART_INSTR_2_PORT,
        .tx_io = BOARD_UART_INSTR_2_TX,
        .rx_io = BOARD_UART_INSTR_2_RX,
        .rts_io = BOARD_UART_INSTR_2_RTS,
        .cts_io = BOARD_UART_INSTR_2_CTS,
        .baud_rate = BOARD_UART_INSTR_2_BAUD_DEFAULT,
        .rx_buf_size = BOARD_UART_INSTR_2_RX_BUF_SIZE,
        .tx_buf_size = BOARD_UART_INSTR_2_TX_BUF_SIZE,
        .timeout_ms = BOARD_UART_INSTR_2_TIMEOUT_MS,
    },
    [3] = {
        .port = BOARD_UART_INSTR_3_PORT,
        .tx_io = BOARD_UART_INSTR_3_TX,
        .rx_io = BOARD_UART_INSTR_3_RX,
        .rts_io = BOARD_UART_INSTR_3_RTS,
        .cts_io = BOARD_UART_INSTR_3_CTS,
        .baud_rate = BOARD_UART_INSTR_3_BAUD_DEFAULT,
        .rx_buf_size = BOARD_UART_INSTR_3_RX_BUF_SIZE,
        .tx_buf_size = BOARD_UART_INSTR_3_TX_BUF_SIZE,
        .timeout_ms = BOARD_UART_INSTR_3_TIMEOUT_MS,
    },
};

static struct driver_uart_instrument_t s_handles[DRIVER_UART_INSTRUMENT_MAX] = {
    [0] = {
        .slot_index = 0,
        .port = BOARD_UART_INSTR_0_PORT,
        .default_timeout_ms = BOARD_UART_INSTR_0_TIMEOUT_MS > 0 ? BOARD_UART_INSTR_0_TIMEOUT_MS : 100U,
    },
    [1] = {
        .slot_index = 1,
        .port = BOARD_UART_INSTR_1_PORT,
        .default_timeout_ms = BOARD_UART_INSTR_1_TIMEOUT_MS > 0 ? BOARD_UART_INSTR_1_TIMEOUT_MS : 100U,
    },
    [2] = {
        .slot_index = 2,
        .port = BOARD_UART_INSTR_2_PORT,
        .default_timeout_ms = BOARD_UART_INSTR_2_TIMEOUT_MS > 0 ? BOARD_UART_INSTR_2_TIMEOUT_MS : 100U,
    },
    [3] = {
        .slot_index = 3,
        .port = BOARD_UART_INSTR_3_PORT,
        .default_timeout_ms = BOARD_UART_INSTR_3_TIMEOUT_MS > 0 ? BOARD_UART_INSTR_3_TIMEOUT_MS : 100U,
    },
};

static const driver_uart_instrument_slot_config_t *get_slot_config(uint8_t slot_index)
{
    return slot_index < DRIVER_UART_INSTRUMENT_MAX ? &s_slot_configs[slot_index] : NULL;
}

static uint32_t resolve_default_timeout_ms(const driver_uart_instrument_slot_config_t *config)
{
    return config->timeout_ms > 0 ? config->timeout_ms : 100U;
}

static uint32_t resolve_timeout_ms(driver_uart_instrument_handle_t handle, uint32_t timeout_ms)
{
    return timeout_ms > 0 ? timeout_ms : handle->default_timeout_ms;
}

static int normalize_pin(gpio_num_t io)
{
    return io != BOARD_GPIO_NONE ? (int)io : UART_PIN_NO_CHANGE;
}

static uart_mode_t resolve_uart_mode(void)
{
    return BOARD_INSTR_LINK_MODE_RS485_HALF == 1 ? UART_MODE_RS485_HALF_DUPLEX : UART_MODE_UART;
}

static esp_err_t validate_board_capability(void)
{
    ESP_RETURN_ON_FALSE(board_has_instrument_uart(), ESP_ERR_NOT_SUPPORTED, TAG, "instrument uart not supported");
    return ESP_OK;
}

static esp_err_t validate_slot_index(uint8_t slot_index)
{
    ESP_RETURN_ON_FALSE(slot_index < DRIVER_UART_INSTRUMENT_MAX,
                        ESP_ERR_INVALID_ARG, TAG, "invalid uart slot");
    return ESP_OK;
}

static bool slot_is_configured(const driver_uart_instrument_slot_config_t *config)
{
    return config->tx_io != BOARD_GPIO_NONE && config->rx_io != BOARD_GPIO_NONE;
}

static esp_err_t validate_common_config(const driver_uart_instrument_slot_config_t *config)
{
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "slot config is NULL");
    ESP_RETURN_ON_FALSE(config->port >= UART_NUM_0 && config->port < UART_NUM_MAX,
                        ESP_ERR_INVALID_ARG, TAG, "invalid uart port");
    ESP_RETURN_ON_FALSE(config->baud_rate > 0, ESP_ERR_INVALID_STATE, TAG, "invalid baud rate");
    ESP_RETURN_ON_FALSE(config->rx_buf_size > CONFIG_SOC_UART_FIFO_LEN,
                        ESP_ERR_INVALID_ARG, TAG, "rx buffer must be > fifo len");
    ESP_RETURN_ON_FALSE(config->tx_buf_size == 0 || config->tx_buf_size > CONFIG_SOC_UART_FIFO_LEN,
                        ESP_ERR_INVALID_ARG, TAG, "tx buffer must be 0 or > fifo len");
    return ESP_OK;
}

static esp_err_t validate_install_pins(const driver_uart_instrument_slot_config_t *config)
{
    ESP_RETURN_ON_FALSE(config->tx_io != BOARD_GPIO_NONE, ESP_ERR_INVALID_STATE, TAG, "tx gpio not configured");
    ESP_RETURN_ON_FALSE(config->rx_io != BOARD_GPIO_NONE, ESP_ERR_INVALID_STATE, TAG, "rx gpio not configured");

    if (BOARD_INSTR_LINK_MODE_RS485_HALF == 1) {
        ESP_RETURN_ON_FALSE(config->rts_io != BOARD_GPIO_NONE,
                            ESP_ERR_INVALID_STATE, TAG, "rs485-half requires rts gpio");
    }

    return ESP_OK;
}

static esp_err_t install_uart_driver(const driver_uart_instrument_slot_config_t *config)
{
    esp_err_t ret = ESP_OK;
    uart_config_t uart_config = {
        .baud_rate = (int)config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(validate_install_pins(config), TAG, "invalid uart pins");
    ESP_RETURN_ON_ERROR(uart_driver_install(config->port,
                                            config->rx_buf_size,
                                            config->tx_buf_size,
                                            0,
                                            NULL,
                                            0),
                        TAG, "uart_driver_install failed");
    ret = uart_param_config(config->port, &uart_config);
    ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_param_config failed");
    ret = uart_set_pin(config->port,
                       normalize_pin(config->tx_io),
                       normalize_pin(config->rx_io),
                       normalize_pin(config->rts_io),
                       normalize_pin(config->cts_io));
    ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_set_pin failed");
    ret = uart_set_mode(config->port, resolve_uart_mode());
    ESP_GOTO_ON_ERROR(ret, err_delete, TAG, "uart_set_mode failed");

    return ESP_OK;

err_delete:
    (void)uart_driver_delete(config->port);
    return ret;
}

esp_err_t driver_uart_instrument_install_all(void)
{
    ESP_RETURN_ON_ERROR(validate_board_capability(), TAG, "instrument uart capability check failed");

    for (uint8_t slot_index = 0; slot_index < DRIVER_UART_INSTRUMENT_MAX; ++slot_index) {
        const driver_uart_instrument_slot_config_t *config = get_slot_config(slot_index);

        if (!config || !slot_is_configured(config)) {
            continue;
        }

        ESP_RETURN_ON_ERROR(validate_common_config(config), TAG, "invalid board uart config for slot %u", slot_index);
        if (!uart_is_driver_installed(config->port)) {
            ESP_RETURN_ON_ERROR(install_uart_driver(config), TAG, "install uart driver failed for slot %u", slot_index);
        }

        s_handles[slot_index].default_timeout_ms = resolve_default_timeout_ms(config);
    }

    return ESP_OK;
}

esp_err_t driver_uart_instrument_install(uint8_t slot_index,
                                         driver_uart_instrument_handle_t *out_handle)
{
    const driver_uart_instrument_slot_config_t *config = NULL;
    driver_uart_instrument_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    ESP_RETURN_ON_ERROR(validate_slot_index(slot_index), TAG, "invalid slot index");
    ESP_RETURN_ON_ERROR(validate_board_capability(), TAG, "instrument uart capability check failed");
    config = get_slot_config(slot_index);
    ESP_RETURN_ON_ERROR(validate_common_config(config), TAG, "invalid board uart config");

    if (!slot_is_configured(config)) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    handle = &s_handles[slot_index];
    handle->default_timeout_ms = resolve_default_timeout_ms(config);

    if (!uart_is_driver_installed(config->port)) {
        ESP_RETURN_ON_ERROR(install_uart_driver(config), TAG, "install uart driver failed");
    }

    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_uart_instrument_uninstall(driver_uart_instrument_handle_t handle)
{
    (void)handle;
    /*
     * This driver models a fixed board-level UART resource.
     * Keep the driver installed for the process lifetime instead of pretending
     * it has a meaningful per-handle uninstall lifecycle.
     */
    return ESP_OK;
}

esp_err_t driver_uart_instrument_write(driver_uart_instrument_handle_t handle,
                                       const uint8_t *data,
                                       size_t len)
{
    int written = 0;

    ESP_RETURN_ON_FALSE(handle && data && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    written = uart_write_bytes(handle->port, data, len);
    ESP_RETURN_ON_FALSE(written >= 0, ESP_FAIL, TAG, "uart_write_bytes failed");
    ESP_RETURN_ON_FALSE((size_t)written == len, ESP_FAIL, TAG, "short write");
    return driver_uart_instrument_wait_tx_done(handle, 0);
}

esp_err_t driver_uart_instrument_read(driver_uart_instrument_handle_t handle,
                                      uint8_t *out_data,
                                      size_t len,
                                      uint32_t timeout_ms)
{
    int read_len = 0;
    TickType_t timeout_ticks = pdMS_TO_TICKS(resolve_timeout_ms(handle, timeout_ms));

    ESP_RETURN_ON_FALSE(handle && out_data && len > 0, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    read_len = uart_read_bytes(handle->port, out_data, len, timeout_ticks);
    ESP_RETURN_ON_FALSE(read_len >= 0, ESP_FAIL, TAG, "uart_read_bytes failed");
    ESP_RETURN_ON_FALSE(read_len > 0, ESP_ERR_TIMEOUT, TAG, "uart read timeout");
    ESP_RETURN_ON_FALSE((size_t)read_len == len, ESP_ERR_TIMEOUT, TAG, "partial uart read");
    return ESP_OK;
}

esp_err_t driver_uart_instrument_wait_tx_done(driver_uart_instrument_handle_t handle,
                                              uint32_t timeout_ms)
{
    TickType_t timeout_ticks = pdMS_TO_TICKS(resolve_timeout_ms(handle, timeout_ms));

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return uart_wait_tx_done(handle->port, timeout_ticks);
}

esp_err_t driver_uart_instrument_flush_rx(driver_uart_instrument_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return uart_flush_input(handle->port);
}

esp_err_t driver_uart_instrument_set_loopback(driver_uart_instrument_handle_t handle,
                                              bool enable)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    return uart_set_loop_back(handle->port, enable);
}
