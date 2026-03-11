#include "driver_network.h"

#include <stdlib.h>

#include "esp_check.h"
#include "esp_netif.h"

typedef struct driver_network {
    esp_netif_t *netif;
} driver_network_t;

static const char *TAG = "driver_network";

esp_err_t driver_network_create(driver_network_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "out_handle is NULL");
    driver_network_t *handle = calloc(1, sizeof(driver_network_t));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "no memory");
    *out_handle = handle;
    return ESP_OK;
}

esp_err_t driver_network_destroy(driver_network_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}

esp_err_t driver_network_send(driver_network_handle_t handle, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(handle && data && len, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_network_recv(driver_network_handle_t handle, uint8_t *buf, size_t buf_len, size_t *out_len)
{
    ESP_RETURN_ON_FALSE(handle && buf && buf_len && out_len, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    *out_len = 0;
    return ESP_ERR_NOT_SUPPORTED;
}
