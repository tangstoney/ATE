#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 典型 ESP-IDF 风格的不透明句柄
// typedef struct driver_ledstrip *driver_ledstrip_handle_t;
typedef struct driver_ledstrip *driver_ledstrip_handle_t;

/**
 * @brief 创建 LED 灯带驱动实例
 * 
 * 所有硬件相关参数（GPIO、数量、RMT 分辨率等）都从 board_ate_p4.h 宏读取，
 * 上层完全不用管。
 */
esp_err_t driver_ledstrip_create(driver_ledstrip_handle_t *out_handle);

/**
 * @brief 销毁实例
 */
esp_err_t driver_ledstrip_destroy(driver_ledstrip_handle_t handle);

/**
 * @brief 可选锁（System 层多任务并发访问时用）
 */
esp_err_t driver_ledstrip_lock(driver_ledstrip_handle_t handle, int timeout_ms);
esp_err_t driver_ledstrip_unlock(driver_ledstrip_handle_t handle);

/**
 * @brief 设置某一颗 LED 的 RGB 颜色（0-based）
 *        只写缓冲区，不立即输出
 */
esp_err_t driver_ledstrip_set_pixel(driver_ledstrip_handle_t handle,
                                    uint16_t index,
                                    uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 将全部 LED 设置为同一颜色（只写缓冲区）
 */
esp_err_t driver_ledstrip_fill(driver_ledstrip_handle_t handle,
                               uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 刷新输出：把缓冲区真正发到灯带
 */
esp_err_t driver_ledstrip_refresh(driver_ledstrip_handle_t handle);

/**
 * @brief 全部熄灭（会立即刷新）
 */
esp_err_t driver_ledstrip_clear(driver_ledstrip_handle_t handle);

#ifdef __cplusplus
}
#endif
