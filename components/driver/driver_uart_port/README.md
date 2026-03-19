# driver_uart_port

ATE UART port driver wrapper for ESP-IDF v5.5.3 on ESP32-P4.

Responsibilities:
- Create and delete UART ports
- Support normal UART / RS422 style links
- Support RS485 half-duplex through `UART_MODE_RS485_HALF_DUPLEX`
- Provide baudrate switch, flush and recover helpers

Backend:
- `driver/uart.h`
- Component dependency: `esp_driver_uart`

Out of scope:
- Device scan policy
- Protocol parsing
- UI or server forwarding logic

Basic usage:

```c
driver_uart_port_handle_t uart = NULL;

driver_uart_port_config_t cfg = {
    .port = UART_NUM_1,
    .tx_io = 43,
    .rx_io = 44,
    .rts_io = -1,
    .baud_rate = 115200,
    .mode = DRIVER_UART_MODE_NORMAL,
    .rx_buf_size = 2048,
    .tx_buf_size = 2048,
    .timeout_ms = 1000,
};

ESP_ERROR_CHECK(driver_uart_port_create(&cfg, &uart));
ESP_ERROR_CHECK(driver_uart_write(uart, tx_buf, tx_len));
ESP_ERROR_CHECK(driver_uart_port_delete(uart));
```
