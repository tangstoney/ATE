#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_LED_STATE_OFF = 0,
    SYSTEM_LED_STATE_OK,
    SYSTEM_LED_STATE_BUSY,
    SYSTEM_LED_STATE_ERROR,
} system_led_state_t;
// todo 业务上可以加入 指示 ，比如 fail提示（红灯呼吸），pass 绿色，小问题 yellow黄色等呼吸，业务只管抽象的业务，具体的实现方式就是调用这里的函数
// todo set_state + set_mode 合并

typedef enum {
    SYSTEM_LED_MODE_STATIC = 0,
    SYSTEM_LED_MODE_BREATH,
    SYSTEM_LED_MODE_CHASE,
    SYSTEM_LED_MODE_RAINBOW,
} system_led_mode_t;

esp_err_t system_led_init(void);
esp_err_t system_led_set_state(system_led_state_t state);
esp_err_t system_led_set_mode(system_led_mode_t mode);
esp_err_t system_led_update(void);

#ifdef __cplusplus
}
#endif
