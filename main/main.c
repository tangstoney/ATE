#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system_init.h"
#include "app_ate_test.h"

void app_main(void)
{
    nvs_flash_init();
    system_init();
    app_ate_test_start();
    while (1) vTaskDelay(pdMS_TO_TICKS(10000));
}



