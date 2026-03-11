#include "driver_usb_host.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "usb/usb_host.h"

static const char *TAG = "driver_usb_host";
static bool s_host_installed = false;
static TaskHandle_t s_event_task = NULL;

static void usb_host_event_task(void *arg)
{
    (void)arg;

    while (s_host_installed) {
        uint32_t event_flags = 0;
        usb_host_lib_handle_events(pdMS_TO_TICKS(100), &event_flags);
        if (!s_host_installed) {
            break;
        }
    }

    s_event_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t driver_usb_host_init(void)
{
    if (s_host_installed) {
        return ESP_OK;
    }

    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .root_port_unpowered = false,
        .intr_flags = 0,
        .enum_filter_cb = NULL,
        .fifo_settings_custom = {0},
        .peripheral_map = 0,
    };

    esp_err_t ret = usb_host_install(&host_config);
    ESP_RETURN_ON_ERROR(ret, TAG, "usb_host_install failed");

    s_host_installed = true;
    BaseType_t ok = xTaskCreate(usb_host_event_task, "usb_host_evt", 4096, NULL, 5, &s_event_task);
    if (ok != pdPASS) {
        s_host_installed = false;
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t driver_usb_host_deinit(void)
{
    if (!s_host_installed) {
        return ESP_OK;
    }

    s_host_installed = false;
    for (int i = 0; i < 50 && s_event_task != NULL; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp_err_t ret = usb_host_uninstall();
    ESP_RETURN_ON_ERROR(ret, TAG, "usb_host_uninstall failed");

    return ESP_OK;
}

esp_err_t driver_usb_host_msc_mount(const char *mount_point)
{
    (void)mount_point;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_host_msc_unmount(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_host_cdc_open(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t driver_usb_host_cdc_close(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
