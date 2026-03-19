#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_ate_p4.h"
#include "app_ate_console.h"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(board_init());
    ESP_ERROR_CHECK(app_ate_console_start());
    while (1) vTaskDelay(pdMS_TO_TICKS(10000));
}


