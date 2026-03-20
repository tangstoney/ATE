#include "app_ate_console.h"

#include "esp_check.h"

#include "app_eez_ui.h"

// codex todo ，这里完全没有正常用起来，这里是app的业务中心，所有业务都在这里跑，业务内部通过注册的方式使用，没注册的直接返回或者跑空函数
// codex todo 每个app的模块使用时，才会一层层的下调用初始化函数使用，而不是一次性全部初始化，没初始化的不跑

static const char *TAG = "app_ate_console";

static esp_err_t app_network_xx(void)
{
    return ESP_OK;
}

static esp_err_t app_ledstrip_xx(void)
{
    return ESP_OK;
}

static esp_err_t app_usb_xx(void)
{
    return ESP_OK;
}

/*
 * @brief Start the peripheral module business module.
 *
 * App-layer entry point for managing downstream peripheral modules
 * that are multiplexed over the shared I2C communication path.
 *
 * Current stage: placeholder stub only.
 * The following capabilities will be added in later iterations:
 *   - Module discovery and attach flow via system layer
 *   - Time-sliced transaction scheduling on the shared I2C bus
 *   - Module state cache and lifecycle management
 *   - Fault detection and reporting
 *
 * @note Do not add peripheral-layer calls directly here.
 *       All communication must go through system_module_service_*.
 *
 * @return ESP_OK  Always returns OK at current stub stage.
 */
static esp_err_t app_peripheral_module_start(void)
{
    return ESP_OK;
}

/*
 * @brief Start the instrument manager business module.
 *
 * App-layer entry point for managing downstream instruments connected
 * to ESP32-P4. Up to 4 instrument links may be managed simultaneously
 * via UART channels.
 *
 * Current stage: placeholder stub only.
 * The following capabilities will be added in later iterations:
 *   - Port assignment and link initialization via system layer
 *   - Instrument online detection and identification
 *   - Protocol session management
 *   - Fault detection and reporting
 *
 * @note Do not add peripheral-layer calls directly here.
 *       All communication must go through system_instrument_service_*.
 *
 * @return ESP_OK  Always returns OK at current stub stage.
 */
static esp_err_t app_instrument_mgr_start(void)
{
    return ESP_OK;
}

esp_err_t app_ate_console_start(void)
{
    // codex todo ，这些也应该用线程包起来跑吧我感觉，初始化不要直接用函数调用，这样很奇怪
    ESP_RETURN_ON_ERROR(app_eez_ui_start(), TAG, "EEZ UI start failed"); // 跑eez ui业务，已成功

    ESP_RETURN_ON_ERROR(app_network_xx(), TAG, "network start failed"); // 跑以太网业务，todo，3月底开始

    ESP_RETURN_ON_ERROR(app_ledstrip_xx(), TAG, "network start failed"); // 跑幻彩灯条提示灯业务，下周可以 用 逻辑分析仪开始

    ESP_RETURN_ON_ERROR(app_usb_xx(), TAG, "network start failed"); // 跑 usb hub的业务，里面会分比如连其他仪表，p4端也就是本工程的p4，会变成usb从机，变成usb，fw更新下位机等等
    
    ESP_RETURN_ON_ERROR(app_peripheral_module_start(), TAG, "peripheral module start failed"); // 跑 被测模组，主要是和下面的 cs32l0的下位机进行通信 
    
    ESP_RETURN_ON_ERROR(app_instrument_mgr_start(), TAG, "instrument manager start failed"); // 跑 被测仪表，主要是和下面esp32s3的下位机进行通信 

    return ESP_OK;
}
