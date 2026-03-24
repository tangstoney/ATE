#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_ate_p4.h"
#include "app_ate_console.h"


static const char *TAG = "app_main";

void app_main(void)
{
    esp_err_t ret = ESP_OK;
 
    ESP_GOTO_ON_ERROR(nvs_flash_init(), err, TAG, "nvs_flash_init failed");
    ESP_GOTO_ON_ERROR(board_init(), err, TAG, "board_init failed");
    ESP_GOTO_ON_ERROR(app_ate_console_start(), err, TAG, "app_ate_console_start failed");
   
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

err:


    return;
}
