#include "system_led.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver_ledstrip.h"
#include "esp_check.h"

#define BREATH_STEP 8
#define RAINBOW_STEP 4

static const char *TAG = "system_led";

static driver_ledstrip_handle_t s_led;
static uint16_t s_led_count = 0;
static system_led_state_t s_state = SYSTEM_LED_STATE_OFF;
static system_led_mode_t s_mode = SYSTEM_LED_MODE_STATIC;
static uint16_t s_chase_pos = 0;
static uint16_t s_rainbow_hue = 0;
static uint8_t s_breath = 0;
static int s_breath_dir = 1;

static void state_to_rgb(system_led_state_t state, uint8_t *r, uint8_t *g, uint8_t *b)
{
    switch (state) {
    case SYSTEM_LED_STATE_OK:
        *r = 0; *g = 255; *b = 0;
        break;
    case SYSTEM_LED_STATE_BUSY:
        *r = 255; *g = 180; *b = 0;
        break;
    case SYSTEM_LED_STATE_ERROR:
        *r = 255; *g = 0; *b = 0;
        break;
    case SYSTEM_LED_STATE_OFF:
    default:
        *r = 0; *g = 0; *b = 0;
        break;
    }
}

static void hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region = hue / 60;
    uint16_t remainder = (hue - (region * 60)) * 255 / 60;

    uint8_t p = (uint16_t)val * (255 - sat) / 255;
    uint8_t q = (uint16_t)val * (255 - ((uint16_t)sat * remainder / 255)) / 255;
    uint8_t t = (uint16_t)val * (255 - ((uint16_t)sat * (255 - remainder) / 255)) / 255;

    switch (region) {
    default:
    case 0: *r = val; *g = t; *b = p; break;
    case 1: *r = q; *g = val; *b = p; break;
    case 2: *r = p; *g = val; *b = t; break;
    case 3: *r = p; *g = q; *b = val; break;
    case 4: *r = t; *g = p; *b = val; break;
    case 5: *r = val; *g = p; *b = q; break;
    }
}

static esp_err_t apply_static(void)
{
    uint8_t r, g, b;
    state_to_rgb(s_state, &r, &g, &b);
    ESP_RETURN_ON_ERROR(driver_ledstrip_fill(s_led, r, g, b), TAG, "fill failed");
    return driver_ledstrip_refresh(s_led);
}

static esp_err_t apply_breath(void)
{
    uint8_t r, g, b;
    state_to_rgb(s_state, &r, &g, &b);
    uint8_t br = (uint16_t)r * s_breath / 255;
    uint8_t bg = (uint16_t)g * s_breath / 255;
    uint8_t bb = (uint16_t)b * s_breath / 255;

    ESP_RETURN_ON_ERROR(driver_ledstrip_fill(s_led, br, bg, bb), TAG, "fill failed");
    ESP_RETURN_ON_ERROR(driver_ledstrip_refresh(s_led), TAG, "refresh failed");

    int next = (int)s_breath + (s_breath_dir * BREATH_STEP);
    if (next >= 255) {
        s_breath = 255;
        s_breath_dir = -1;
    } else if (next <= 0) {
        s_breath = 0;
        s_breath_dir = 1;
    } else {
        s_breath = (uint8_t)next;
    }

    return ESP_OK;
}

static esp_err_t apply_chase(void)
{
    uint8_t r, g, b;
    state_to_rgb(s_state, &r, &g, &b);

    ESP_RETURN_ON_FALSE(s_led_count > 0, ESP_ERR_INVALID_STATE, TAG, "invalid led count");

    ESP_RETURN_ON_ERROR(driver_ledstrip_clear(s_led), TAG, "clear failed");
    ESP_RETURN_ON_ERROR(driver_ledstrip_set_pixel(s_led, s_chase_pos, r, g, b), TAG, "set pixel failed");
    ESP_RETURN_ON_ERROR(driver_ledstrip_refresh(s_led), TAG, "refresh failed");

    s_chase_pos = (s_chase_pos + 1) % s_led_count;
    return ESP_OK;
}

static esp_err_t apply_rainbow(void)
{
    ESP_RETURN_ON_FALSE(s_led_count > 0, ESP_ERR_INVALID_STATE, TAG, "invalid led count");

    for (uint16_t i = 0; i < s_led_count; ++i) {
        uint16_t hue = (s_rainbow_hue + (i * 360 / s_led_count)) % 360;
        uint8_t r, g, b;
        hsv_to_rgb(hue, 255, 255, &r, &g, &b);
        ESP_RETURN_ON_ERROR(driver_ledstrip_set_pixel(s_led, i, r, g, b), TAG, "set pixel failed");
    }
    ESP_RETURN_ON_ERROR(driver_ledstrip_refresh(s_led), TAG, "refresh failed");

    s_rainbow_hue = (s_rainbow_hue + RAINBOW_STEP) % 360;
    return ESP_OK;
}

static esp_err_t apply_frame(void)
{
    switch (s_mode) {
    case SYSTEM_LED_MODE_STATIC:
        return apply_static();
    case SYSTEM_LED_MODE_BREATH:
        return apply_breath();
    case SYSTEM_LED_MODE_CHASE:
        return apply_chase();
    case SYSTEM_LED_MODE_RAINBOW:
        return apply_rainbow();
    default:
        return apply_static();
    }
}

esp_err_t system_led_init(void)
{
    if (s_led) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(driver_ledstrip_create(&s_led), TAG, "driver_ledstrip_create failed");
    s_led_count = driver_ledstrip_get_count(s_led);
    ESP_RETURN_ON_FALSE(s_led_count != UINT16_MAX, ESP_ERR_INVALID_STATE, TAG, "driver_ledstrip_get_count failed");
    ESP_RETURN_ON_FALSE(s_led_count > 0, ESP_ERR_INVALID_STATE, TAG, "led count must be > 0");
    return driver_ledstrip_clear(s_led);
}

esp_err_t system_led_set_state(system_led_state_t state)
{
    s_state = state;
    s_breath = 0;
    s_breath_dir = 1;
    s_chase_pos = 0;
    return system_led_update();
}

esp_err_t system_led_set_mode(system_led_mode_t mode)
{
    s_mode = mode;
    s_breath = 0;
    s_breath_dir = 1;
    s_chase_pos = 0;
    return system_led_update();
}

esp_err_t system_led_update(void)
{
    ESP_RETURN_ON_FALSE(s_led, ESP_ERR_INVALID_STATE, TAG, "system_led_init not called");

    esp_err_t ret = driver_ledstrip_lock(s_led, -1);
    ESP_RETURN_ON_ERROR(ret, TAG, "lock failed");

    ret = apply_frame();

    esp_err_t unlock_ret = driver_ledstrip_unlock(s_led);
    if (ret == ESP_OK) {
        ret = unlock_ret;
    }
    return ret;
}
