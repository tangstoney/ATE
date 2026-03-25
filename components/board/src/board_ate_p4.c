#include "board_ate_p4.h"

#include "driver/gpio.h"
#include "esp_check.h"

static const char *TAG = "board_ate_p4";
static bool s_initialized;

static const board_i2c_master_config_t s_i2c_master_config = {
    .port = BOARD_I2C_MASTER_NUM,
    .scl_io = BOARD_I2C_MASTER_SCL,
    .sda_io = BOARD_I2C_MASTER_SDA,
    .clk_speed_hz = BOARD_I2C_MASTER_CLK_HZ,
    .timeout_ms = BOARD_I2C_MASTER_TIMEOUT_MS,
    .enable_internal_pullup = BOARD_I2C_MASTER_USE_INTERNAL_PULLUP,
    .glitch_ignore_cnt = BOARD_I2C_MASTER_GLITCH_IGNORE_CNT,
};

static const board_module_link_config_t s_module_link_config = {
    .i2c = {
        .port = BOARD_I2C_MASTER_NUM,
        .scl_io = BOARD_I2C_MASTER_SCL,
        .sda_io = BOARD_I2C_MASTER_SDA,
        .clk_speed_hz = BOARD_I2C_MASTER_CLK_HZ,
        .timeout_ms = BOARD_I2C_MASTER_TIMEOUT_MS,
        .enable_internal_pullup = BOARD_I2C_MASTER_USE_INTERNAL_PULLUP,
        .glitch_ignore_cnt = BOARD_I2C_MASTER_GLITCH_IGNORE_CNT,
    },
    .device_address = BOARD_MODULE_LINK_I2C_ADDR,
    .timeout_ms = BOARD_MODULE_LINK_DEFAULT_TIMEOUT_MS,
};

static const board_instrument_link_config_t s_instrument_link_config = {
    .uart = {
        .port = BOARD_UART_INSTR_PORT,
        .tx_io = BOARD_UART_INSTR_TX,
        .rx_io = BOARD_UART_INSTR_RX,
        .rts_io = BOARD_UART_INSTR_RTS,
        .cts_io = BOARD_UART_INSTR_CTS,
        .baud_rate = BOARD_UART_INSTR_BAUD_DEFAULT,
        .rx_buf_size = BOARD_UART_INSTR_RX_BUF_SIZE,
        .tx_buf_size = BOARD_UART_INSTR_TX_BUF_SIZE,
        .timeout_ms = BOARD_UART_INSTR_TIMEOUT_MS,
    },
    .is_rs422 = BOARD_INSTR_LINK_MODE_RS422 == 1,
    .is_rs485_half = BOARD_INSTR_LINK_MODE_RS485_HALF == 1,
    .has_dir_ctrl = BOARD_INSTR_LINK_HAS_DIR_CTRL == 1,
    .dir_io = BOARD_GPIO_RS485_DIR,
    .link_count = BOARD_INSTRUMENT_LINK_COUNT,
};

static const board_usb_port_config_t s_usb_port_config = {
    .dp_io = BOARD_USB_DP,
    .dm_io = BOARD_USB_DM,
    .vbus_en_io = BOARD_GPIO_USB_VBUS_EN,
    .fault_io = BOARD_GPIO_USB_FAULT,
    .use_internal_phy = BOARD_USB_USE_INTERNAL_PHY == 1,
};

static const board_eth_config_t s_eth_config = {
    .mdc_io = BOARD_ETH_MDC,
    .mdio_io = BOARD_ETH_MDIO,
    .reset_io = BOARD_ETH_PHY_RST,
    .phy_addr = BOARD_ETH_PHY_ADDR,
};

static const board_display_config_t s_display_config = {
    .hor_res = BOARD_LCD_H_RES,
    .ver_res = BOARD_LCD_V_RES,
    .power_en_io = BOARD_GPIO_LCD_POWER_EN,
    .enable_io = BOARD_GPIO_LCD_ENABLE,
    .backlight_io = BOARD_GPIO_LCD_BACKLIGHT,
    .reset_io = BOARD_GPIO_LCD_RESET,
};

static const board_touch_config_t s_touch_config = {
    .port = BOARD_TOUCH_I2C_PORT,
    .scl_io = BOARD_TOUCH_I2C_SCL,
    .sda_io = BOARD_TOUCH_I2C_SDA,
    .int_io = BOARD_GPIO_TOUCH_INT,
    .reset_io = BOARD_GPIO_TOUCH_RESET,
    .device_address = BOARD_TOUCH_I2C_ADDR,
    .clk_speed_hz = BOARD_TOUCH_I2C_CLK_HZ,
    .enable_internal_pullup = BOARD_TOUCH_USE_INTERNAL_PULLUP == 1,
};

static const board_led_strip_config_t s_led_strip_config = {
    .data_io = BOARD_LED_STRIP_DATA_GPIO,
    .led_count = BOARD_LED_STRIP_LED_COUNT,
    .rmt_resolution_hz = BOARD_LED_STRIP_RMT_RES_HZ,
    .mem_block_symbols = BOARD_LED_STRIP_MEM_WORDS,
    .use_dma = BOARD_LED_STRIP_USE_DMA == 1,
};

static esp_err_t configure_output_gpio(gpio_num_t io, int default_level)
{
    if (io == BOARD_GPIO_NONE) {
        return ESP_OK;
    }

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << io,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "gpio_config failed for io=%d", io);
    ESP_RETURN_ON_ERROR(gpio_set_level(io, default_level), TAG, "gpio_set_level failed for io=%d", io);
    return ESP_OK;
}

esp_err_t board_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_PWR_EN_3V3, 0), TAG, "3v3 power gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_PWR_EN_5V, 0), TAG, "5v power gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_MODULE_PWR_EN, 0), TAG, "module power gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_LCD_POWER_EN, 0), TAG, "lcd power gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_LCD_ENABLE, 0), TAG, "lcd enable gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_LCD_BACKLIGHT, 0), TAG, "lcd backlight gpio failed");

    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_MODULE_RESET, 0), TAG, "module reset gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_LCD_RESET, 0), TAG, "lcd reset gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_TOUCH_RESET, 0), TAG, "touch reset gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_ETH_RESET, 0), TAG, "eth reset gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_USB_HUB_RESET, 0), TAG, "usb hub reset gpio failed");
    ESP_RETURN_ON_ERROR(configure_output_gpio(BOARD_GPIO_USB_VBUS_EN, 0), TAG, "usb vbus gpio failed");

    s_initialized = true;
    return ESP_OK;
}

const board_i2c_master_config_t *board_get_i2c_master_config(void)
{
    return &s_i2c_master_config;
}

const board_module_link_config_t *board_get_module_link_config(void)
{
    return &s_module_link_config;
}

const board_instrument_link_config_t *board_get_instrument_link_config(void)
{
    return &s_instrument_link_config;
}

const board_usb_port_config_t *board_get_usb_port_config(void)
{
    return &s_usb_port_config;
}

const board_eth_config_t *board_get_eth_config(void)
{
    return &s_eth_config;
}

const board_display_config_t *board_get_display_config(void)
{
    return &s_display_config;
}

const board_touch_config_t *board_get_touch_config(void)
{
    return &s_touch_config;
}

const board_led_strip_config_t *board_get_led_strip_config(void)
{
    return &s_led_strip_config;
}
