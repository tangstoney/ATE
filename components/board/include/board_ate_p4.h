/**
 * @file board_ate_p4.h
 * @brief ATE ESP32-P4 board description layer.
 *
 * This file centralizes hardware-default descriptions for the current ATE
 * ESP32-P4 board: GPIO assignments, bus numbers, default parameters,
 * board capabilities, and thin accessors for board configuration data.
 *
 * Usage:
 * - Driver layer: use board_get_*_config() accessors as default parameters
 * - System layer: use board_has_*() helpers to guard capability-specific paths
 * - App layer: do not consume GPIO, bus, or address details directly
 *
 * To adapt the board to a new PCB revision, update the macro values and
 * static default config initializers while keeping symbol names unchanged.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_eth_mac_esp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Board Identification
 *===========================================================================*/

#define BOARD_NAME                  "ATE_ESP32_P4"
#define BOARD_ATE_P4                1
#define BOARD_HW_REV_MAJOR          1
#define BOARD_HW_REV_MINOR          0

/*===========================================================================
 * Capability Macros (BOARD_CAPS_*)
 * 1 = hardware present on this PCB
 * 0 = hardware not present
 *===========================================================================*/

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_LCD
#define BOARD_CAPS_LCD              1
#else
#define BOARD_CAPS_LCD              0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_TOUCH
#define BOARD_CAPS_TOUCH            1
#else
#define BOARD_CAPS_TOUCH            0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_LED_STRIP
#define BOARD_CAPS_STATUS_LED       1
#define BOARD_CAPS_LED_STRIP        1
#else
#define BOARD_CAPS_STATUS_LED       0
#define BOARD_CAPS_LED_STRIP        0
#endif

#define BOARD_CAPS_BUZZER           0

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_USB_DEVICE
#define BOARD_CAPS_USB_DEVICE       1
#else
#define BOARD_CAPS_USB_DEVICE       0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_USB_HOST
#define BOARD_CAPS_USB_HOST         1
#else
#define BOARD_CAPS_USB_HOST         0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_ETHERNET
#define BOARD_CAPS_ETHERNET         1
#else
#define BOARD_CAPS_ETHERNET         0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_MODULE_I2C
#define BOARD_CAPS_MODULE_I2C       1
#else
#define BOARD_CAPS_MODULE_I2C       0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_INSTRUMENT_UART
#define BOARD_CAPS_INSTRUMENT_UART  1
#else
#define BOARD_CAPS_INSTRUMENT_UART  0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_RS422
#define BOARD_CAPS_RS422            1
#define BOARD_INSTR_LINK_MODE_RS422 1
#else
#define BOARD_CAPS_RS422            0
#define BOARD_INSTR_LINK_MODE_RS422 0
#endif

#ifdef CONFIG_BOARD_ATE_P4_ENABLE_RS485_HALF
#define BOARD_CAPS_RS485            1
#define BOARD_INSTR_LINK_MODE_RS485_HALF 1
#else
#define BOARD_CAPS_RS485            0
#define BOARD_INSTR_LINK_MODE_RS485_HALF 0
#endif

#define BOARD_CAPS_NFC              0
#define BOARD_CAPS_SD_CARD          0

/*===========================================================================
 * Common Sentinel / Default Values
 *===========================================================================*/

#define BOARD_GPIO_NONE             GPIO_NUM_NC
#define BOARD_I2C_ADDR_INVALID      (0xFFFFU)
#define BOARD_UART_BAUD_DEFAULT     (115200U)
#define BOARD_I2C_CLK_DEFAULT_HZ    (400000U)

/*===========================================================================
 * Power and Reset GPIO
 *===========================================================================*/

#define BOARD_GPIO_PWR_EN_3V3       BOARD_GPIO_NONE
#define BOARD_GPIO_PWR_EN_5V        BOARD_GPIO_NONE

#define BOARD_GPIO_MODULE_PWR_EN    BOARD_GPIO_NONE
#define BOARD_GPIO_MODULE_RESET     BOARD_GPIO_NONE

#define BOARD_GPIO_LCD_POWER_EN     GPIO_NUM_45
#define BOARD_GPIO_LCD_ENABLE       GPIO_NUM_53
#define BOARD_GPIO_LCD_BACKLIGHT    GPIO_NUM_21
#define BOARD_GPIO_LCD_RESET        GPIO_NUM_27

#define BOARD_GPIO_TOUCH_RESET      BOARD_GPIO_NONE
#define BOARD_GPIO_TOUCH_INT        BOARD_GPIO_NONE

#define BOARD_GPIO_ETH_RESET        BOARD_GPIO_NONE
#define BOARD_GPIO_USB_HUB_RESET    BOARD_GPIO_NONE

/*===========================================================================
 * Status Indicator GPIO
 *===========================================================================*/

#define BOARD_GPIO_STATUS_LED_R     BOARD_GPIO_NONE
#define BOARD_GPIO_STATUS_LED_G     BOARD_GPIO_NONE
#define BOARD_GPIO_STATUS_LED_B     BOARD_GPIO_NONE
#define BOARD_GPIO_BUZZER           BOARD_GPIO_NONE

/*===========================================================================
 * LED Strip Defaults
 *===========================================================================*/

#define BOARD_LED_STRIP_DATA_GPIO   GPIO_NUM_4
#define BOARD_LED_STRIP_LED_COUNT   (6U)
#define BOARD_LED_STRIP_RMT_RES_HZ  (10U * 1000U * 1000U)
#define BOARD_LED_STRIP_MEM_WORDS   (0U)
#define BOARD_LED_STRIP_USE_DMA     0

/*===========================================================================
 * I2C Master Bus Defaults
 * Used by the downstream module link.
 * TODO: Confirm module bus GPIO mapping from the final schematic.
 *===========================================================================*/

#define BOARD_I2C_MASTER_NUM                CONFIG_BOARD_ATE_P4_MODULE_I2C_PORT
#define BOARD_I2C_MASTER_SCL                BOARD_GPIO_NONE
#define BOARD_I2C_MASTER_SDA                BOARD_GPIO_NONE
#define BOARD_I2C_MASTER_CLK_HZ             CONFIG_BOARD_ATE_P4_MODULE_I2C_CLK_HZ
#define BOARD_I2C_MASTER_TIMEOUT_MS         (100U)
#define BOARD_I2C_MASTER_GLITCH_IGNORE_CNT  (7U)
#define BOARD_I2C_MASTER_USE_INTERNAL_PULLUP 0

/*===========================================================================
 * Touch I2C Defaults
 *===========================================================================*/

#define BOARD_TOUCH_I2C_PORT          I2C_NUM_0
#define BOARD_TOUCH_I2C_SCL           GPIO_NUM_8
#define BOARD_TOUCH_I2C_SDA           GPIO_NUM_7
#define BOARD_TOUCH_I2C_CLK_HZ        (400000U)
#define BOARD_TOUCH_I2C_ADDR          CONFIG_BOARD_ATE_P4_TOUCH_I2C_ADDR
#define BOARD_TOUCH_USE_INTERNAL_PULLUP 0

/*===========================================================================
 * Test Module Link Defaults
 *===========================================================================*/

#define BOARD_MODULE_LINK_I2C_ADDR_MIN        (0x08U)
#define BOARD_MODULE_LINK_I2C_ADDR_MAX        (0x77U)
#define BOARD_MODULE_LINK_DEFAULT_TIMEOUT_MS  (100U)
#define BOARD_MODULE_LINK_DEFAULT_RETRY       (2U)

/*===========================================================================
 * Instrument UART Link Defaults
 * TODO: Confirm TX/RX/RTS/CTS GPIO mapping from the final schematic.
 *===========================================================================*/

#define BOARD_INSTRUMENT_LINK_COUNT          (4U)
#define BOARD_UART_INSTR_PORT                CONFIG_BOARD_ATE_P4_INSTRUMENT_UART_PORT
#define BOARD_UART_INSTR_TX                  BOARD_GPIO_NONE
#define BOARD_UART_INSTR_RX                  BOARD_GPIO_NONE
#define BOARD_UART_INSTR_RTS                 BOARD_GPIO_NONE
#define BOARD_UART_INSTR_CTS                 BOARD_GPIO_NONE
#define BOARD_UART_INSTR_BAUD_DEFAULT        CONFIG_BOARD_ATE_P4_INSTRUMENT_UART_BAUD_DEFAULT
#define BOARD_UART_INSTR_RX_BUF_SIZE         (1024U)
#define BOARD_UART_INSTR_TX_BUF_SIZE         (1024U)
#define BOARD_UART_INSTR_TIMEOUT_MS          (100U)
#define BOARD_INSTR_LINK_HAS_DIR_CTRL        0
#define BOARD_GPIO_RS485_DIR                 BOARD_GPIO_NONE

#define BOARD_UART_DEBUG_PORT                UART_NUM_0
#define BOARD_UART_DEBUG_TX                  BOARD_GPIO_NONE
#define BOARD_UART_DEBUG_RX                  BOARD_GPIO_NONE
#define BOARD_UART_DEBUG_BAUD                (115200U)

/*===========================================================================
 * USB Port Defaults
 *===========================================================================*/

#define BOARD_USB_DP                         BOARD_GPIO_NONE
#define BOARD_USB_DM                         BOARD_GPIO_NONE
#define BOARD_USB_USE_INTERNAL_PHY           1
#define BOARD_GPIO_USB_VBUS_EN               BOARD_GPIO_NONE
#define BOARD_GPIO_USB_FAULT                 BOARD_GPIO_NONE

/*===========================================================================
 * Ethernet Defaults
 * TODO: Extend with full RMII data pin description when driver_eth migrates.
 *===========================================================================*/

#define BOARD_ETH_MDC                        BOARD_GPIO_NONE
#define BOARD_ETH_MDIO                       BOARD_GPIO_NONE
#define BOARD_ETH_RMII_TX_EN                 BOARD_GPIO_NONE
#define BOARD_ETH_RMII_TXD0                  BOARD_GPIO_NONE
#define BOARD_ETH_RMII_TXD1                  BOARD_GPIO_NONE
#define BOARD_ETH_RMII_CRS_DV                BOARD_GPIO_NONE
#define BOARD_ETH_RMII_RXD0                  BOARD_GPIO_NONE
#define BOARD_ETH_RMII_RXD1                  BOARD_GPIO_NONE
#define BOARD_ETH_PHY_RST                    BOARD_GPIO_NONE
#define BOARD_ETH_PHY_ADDR                   (1U)
#define BOARD_ETH_CLK_MODE                   EMAC_CLK_EXT_IN
#define BOARD_ETH_CLK_GPIO                   BOARD_GPIO_NONE

/*===========================================================================
 * LCD and Touch Defaults
 *===========================================================================*/

#define BOARD_LCD_H_RES                      (800U)
#define BOARD_LCD_V_RES                      (1280U)
#define BOARD_LCD_MIPI_LDO_CHAN             (3U)
#define BOARD_LCD_MIPI_LDO_MV               (2500U)

typedef struct {
    i2c_port_num_t port;
    gpio_num_t scl_io;
    gpio_num_t sda_io;
    uint32_t clk_speed_hz;
    uint32_t timeout_ms;
    bool enable_internal_pullup;
    uint8_t glitch_ignore_cnt;
} board_i2c_master_config_t;

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
} board_uart_link_config_t;

typedef struct {
    board_i2c_master_config_t i2c;
    uint8_t addr_min;
    uint8_t addr_max;
    uint32_t timeout_ms;
    uint8_t retry_count;
} board_module_link_config_t;

typedef struct {
    board_uart_link_config_t uart;
    bool is_rs422;
    bool is_rs485_half;
    bool has_dir_ctrl;
    gpio_num_t dir_io;
    uint8_t link_count;
} board_instrument_link_config_t;

typedef struct {
    gpio_num_t dp_io;
    gpio_num_t dm_io;
    gpio_num_t vbus_en_io;
    gpio_num_t fault_io;
    bool use_internal_phy;
} board_usb_port_config_t;

typedef struct {
    gpio_num_t mdc_io;
    gpio_num_t mdio_io;
    gpio_num_t reset_io;
    uint8_t phy_addr;
} board_eth_config_t;

typedef struct {
    uint16_t hor_res;
    uint16_t ver_res;
    gpio_num_t power_en_io;
    gpio_num_t enable_io;
    gpio_num_t backlight_io;
    gpio_num_t reset_io;
} board_display_config_t;

typedef struct {
    i2c_port_num_t port;
    gpio_num_t scl_io;
    gpio_num_t sda_io;
    gpio_num_t int_io;
    gpio_num_t reset_io;
    uint16_t device_address;
    uint32_t clk_speed_hz;
    bool enable_internal_pullup;
} board_touch_config_t;

typedef struct {
    gpio_num_t data_io;
    uint16_t led_count;
    uint32_t rmt_resolution_hz;
    uint16_t mem_block_symbols;
    bool use_dma;
} board_led_strip_config_t;

/**
 * @brief Initialize the board to a safe default hardware state.
 *
 * Configures known power-enable, backlight-enable, and reset GPIOs to a safe
 * default state. This function does not create any driver, task, event loop,
 * or service instance.
 *
 * @return ESP_OK on success.
 */
esp_err_t board_init(void);

const board_i2c_master_config_t *board_get_i2c_master_config(void);
const board_module_link_config_t *board_get_module_link_config(void);
const board_instrument_link_config_t *board_get_instrument_link_config(void);
const board_usb_port_config_t *board_get_usb_port_config(void);
const board_eth_config_t *board_get_eth_config(void);
const board_display_config_t *board_get_display_config(void);
const board_touch_config_t *board_get_touch_config(void);
const board_led_strip_config_t *board_get_led_strip_config(void);

static inline bool board_has_lcd(void)
{
    return BOARD_CAPS_LCD == 1;
}

static inline bool board_has_touch(void)
{
    return BOARD_CAPS_TOUCH == 1;
}

static inline bool board_has_eth(void)
{
    return BOARD_CAPS_ETHERNET == 1;
}

static inline bool board_has_module_i2c(void)
{
    return BOARD_CAPS_MODULE_I2C == 1;
}

static inline bool board_has_instrument_uart(void)
{
    return BOARD_CAPS_INSTRUMENT_UART == 1;
}

static inline bool board_has_usb_device(void)
{
    return BOARD_CAPS_USB_DEVICE == 1;
}

static inline bool board_has_status_led(void)
{
    return BOARD_CAPS_STATUS_LED == 1;
}

static inline bool board_has_led_strip(void)
{
    return BOARD_CAPS_LED_STRIP == 1;
}

#ifdef __cplusplus
}
#endif
