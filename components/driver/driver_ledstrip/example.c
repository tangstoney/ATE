#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver_ledstrip.h"

// Reference-only validation example.
// This file is intentionally not added to SRCS in CMakeLists.txt.

static const char *TAG = "driver_ledstrip_example";

void driver_ledstrip_example_run(void)
{
    esp_err_t ret = ESP_OK;
    driver_ledstrip_handle_t led = NULL;

    ESP_GOTO_ON_ERROR(driver_ledstrip_create(&led), err, TAG, "driver_ledstrip_create failed");

    ESP_GOTO_ON_ERROR(driver_ledstrip_fill(led, 255, 255, 255), err, TAG, "driver_ledstrip_fill failed");
    // ESP_GOTO_ON_ERROR(driver_ledstrip_refresh(led), err, TAG, "driver_ledstrip_refresh failed");
    ESP_LOGI(TAG, "LED strip: all white");
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_GOTO_ON_ERROR(driver_ledstrip_fill(led, 11, 22, 33), err, TAG, "driver_ledstrip_fill failed");
    ESP_LOGI(TAG, "LED strip: test pattern updated");
    vTaskDelay(pdMS_TO_TICKS(2000));

    ret = driver_ledstrip_destroy(led);
    led = NULL;
    ESP_GOTO_ON_ERROR(ret, err, TAG, "driver_ledstrip_destroy failed");
    ESP_LOGI(TAG, "driver_ledstrip validation done");
    return;

err:
    if (led) {
        driver_ledstrip_destroy(led);
    }
}
