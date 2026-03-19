/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "driver_eth.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "board_ate_p4.h"

static const char *TAG = "driver_eth";

typedef struct driver_eth_t {
    esp_eth_handle_t eth_handle;
    esp_eth_mac_t *mac;
    esp_eth_phy_t *phy;
} driver_eth_t;

esp_err_t driver_eth_create(driver_eth_handle_t *out_handle)
{
    esp_err_t ret = ESP_OK;
    driver_eth_t *driver = NULL;
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_mac_t *mac = NULL;
    esp_eth_phy_t *phy = NULL;

    ESP_RETURN_ON_FALSE(out_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "out_handle cannot be NULL");

    driver = calloc(1, sizeof(driver_eth_t));
    ESP_RETURN_ON_FALSE(driver != NULL, ESP_ERR_NO_MEM, TAG, "failed to allocate driver handle");

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.phy_addr = BOARD_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = BOARD_ETH_PHY_RST;

    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    // 硬件表示，直接按照默认的io进行配置，这样就与乐鑫官方原理图一致了
    // esp32_emac_config.emac_dataif_gpio.rmii.tx_en_num = BOARD_ETH_RMII_TX_EN;
    // esp32_emac_config.emac_dataif_gpio.rmii.txd0_num = BOARD_ETH_RMII_TXD0;
    // esp32_emac_config.emac_dataif_gpio.rmii.txd1_num = BOARD_ETH_RMII_TXD1;
    // esp32_emac_config.emac_dataif_gpio.rmii.crs_dv_num = BOARD_ETH_RMII_CRS_DV;
    // esp32_emac_config.emac_dataif_gpio.rmii.rxd0_num = BOARD_ETH_RMII_RXD0;
    // esp32_emac_config.emac_dataif_gpio.rmii.rxd1_num = BOARD_ETH_RMII_RXD1;

    // esp32_emac_config.smi_gpio.mdc_num = BOARD_ETH_MDC;
    // esp32_emac_config.smi_gpio.mdio_num = BOARD_ETH_MDIO;
    // esp32_emac_config.clock_config.rmii.clock_mode = BOARD_ETH_CLK_MODE;
    // esp32_emac_config.clock_config.rmii.clock_gpio = BOARD_ETH_CLK_GPIO;

    mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    ESP_GOTO_ON_FALSE(mac != NULL, ESP_FAIL, err, TAG, "Failed to create MAC instance");

    phy = esp_eth_phy_new_generic(&phy_config);
    ESP_GOTO_ON_FALSE(phy != NULL, ESP_FAIL, err, TAG, "Failed to create PHY instance");

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_GOTO_ON_ERROR(esp_eth_driver_install(&eth_config, &eth_handle), err, TAG,
                      "Ethernet driver install failed");

    int phy_addr = -1;
    esp_eth_ioctl(eth_handle, ETH_CMD_G_PHY_ADDR, &phy_addr);
    ESP_LOGI(TAG, "Ethernet PHY Address: %d", phy_addr);

    driver->eth_handle = eth_handle;
    driver->mac = mac;
    driver->phy = phy;

    *out_handle = driver;
    return ESP_OK;

err:
    if (eth_handle != NULL) {
        esp_eth_driver_uninstall(eth_handle);
    }
    if (phy != NULL) {
        phy->del(phy);
    }
    if (mac != NULL) {
        mac->del(mac);
    }
    free(driver);
    return ret;
}

esp_err_t driver_eth_destroy(driver_eth_handle_t handle)
{
    driver_eth_t *driver = (driver_eth_t *)handle;
    ESP_RETURN_ON_FALSE(driver != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid handle");

    if (driver->eth_handle != NULL) {
        ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(driver->eth_handle), TAG,
                           "failed to uninstall driver");
    }

    if (driver->phy != NULL) {
        driver->phy->del(driver->phy);
    }

    if (driver->mac != NULL) {
        driver->mac->del(driver->mac);
    }

    free(driver);
    return ESP_OK;
}

esp_err_t driver_eth_start(driver_eth_handle_t handle)
{
    driver_eth_t *driver = (driver_eth_t *)handle;
    ESP_RETURN_ON_FALSE(driver != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(driver->eth_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "driver not initialized");

    return esp_eth_start(driver->eth_handle);
}

esp_err_t driver_eth_stop(driver_eth_handle_t handle)
{
    driver_eth_t *driver = (driver_eth_t *)handle;
    ESP_RETURN_ON_FALSE(driver != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_FALSE(driver->eth_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "driver not initialized");

    return esp_eth_stop(driver->eth_handle);
}

esp_err_t driver_eth_get_raw_handle(driver_eth_handle_t handle, esp_eth_handle_t *out_eth)
{
    driver_eth_t *driver = (driver_eth_t *)handle;
    ESP_RETURN_ON_FALSE(driver != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid driver handle");
    ESP_RETURN_ON_FALSE(out_eth != NULL, ESP_ERR_INVALID_ARG, TAG, "out_eth cannot be NULL");
    ESP_RETURN_ON_FALSE(driver->eth_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "driver not initialized");

    *out_eth = driver->eth_handle;
    return ESP_OK;
}
