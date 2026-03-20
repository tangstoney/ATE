#include "system_comm_mgr.h"

#include <stdbool.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "system_comm_mgr";
static SemaphoreHandle_t s_mutex;
static driver_i2c_master_handle_t s_i2c_links[SYSTEM_COMM_MGR_MAX_I2C_LINKS];
static uint32_t s_i2c_link_ids[SYSTEM_COMM_MGR_MAX_I2C_LINKS];
static system_uart_link_handle_t s_uart_links[SYSTEM_COMM_MGR_MAX_UART_LINKS];
static uint32_t s_uart_link_ids[SYSTEM_COMM_MGR_MAX_UART_LINKS];

static int find_free_i2c_slot(void)
{
    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_I2C_LINKS; ++i) {
        if (!s_i2c_links[i]) {
            return i;
        }
    }
    return -1;
}

static int find_i2c_slot_by_id(uint32_t link_id)
{
    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_I2C_LINKS; ++i) {
        if (s_i2c_links[i] && s_i2c_link_ids[i] == link_id) {
            return i;
        }
    }
    return -1;
}

static int find_i2c_slot_by_handle(driver_i2c_master_handle_t handle)
{
    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_I2C_LINKS; ++i) {
        if (s_i2c_links[i] == handle) {
            return i;
        }
    }
    return -1;
}

static int find_free_uart_slot(void)
{
    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_UART_LINKS; ++i) {
        if (!s_uart_links[i]) {
            return i;
        }
    }
    return -1;
}

static int find_uart_slot_by_id(uint32_t link_id)
{
    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_UART_LINKS; ++i) {
        if (s_uart_links[i] && s_uart_link_ids[i] == link_id) {
            return i;
        }
    }
    return -1;
}

static int find_uart_slot_by_handle(system_uart_link_handle_t handle)
{
    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_UART_LINKS; ++i) {
        if (s_uart_links[i] == handle) {
            return i;
        }
    }
    return -1;
}

esp_err_t system_comm_mgr_init(void)
{
    if (s_mutex) {
        return ESP_OK;
    }
    s_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_mutex, ESP_ERR_NO_MEM, TAG, "xSemaphoreCreateMutex failed");
    return ESP_OK;
}

esp_err_t system_comm_mgr_deinit(void)
{
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_I2C_LINKS; ++i) {
        if (s_i2c_links[i]) {
            (void)driver_i2c_master_delete(s_i2c_links[i]);
            s_i2c_links[i] = NULL;
            s_i2c_link_ids[i] = 0;
        }
    }

    for (int i = 0; i < SYSTEM_COMM_MGR_MAX_UART_LINKS; ++i) {
        if (s_uart_links[i]) {
            (void)system_uart_link_delete(s_uart_links[i]);
            s_uart_links[i] = NULL;
            s_uart_link_ids[i] = 0;
        }
    }

    xSemaphoreGive(s_mutex);
    vSemaphoreDelete(s_mutex);
    s_mutex = NULL;
    return ESP_OK;
}

esp_err_t system_comm_mgr_register_i2c_link(const system_comm_mgr_i2c_link_config_t *config,
                                            driver_i2c_master_handle_t *out_handle)
{
    int slot = -1;
    driver_i2c_master_handle_t handle = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(config && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    ESP_GOTO_ON_FALSE(find_i2c_slot_by_id(config->link_id) < 0,
                      ESP_ERR_INVALID_STATE, err_unlock, TAG, "duplicate I2C link id");

    slot = find_free_i2c_slot();
    ESP_GOTO_ON_FALSE(slot >= 0, ESP_ERR_NO_MEM, err_unlock, TAG, "no free I2C slot");
    xSemaphoreGive(s_mutex);

    err = driver_i2c_master_create(&config->bus_config, &handle);
    if (err != ESP_OK) {
        return err;
    }

    ESP_GOTO_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, err_delete, TAG, "lock failed");
    s_i2c_links[slot] = handle;
    s_i2c_link_ids[slot] = config->link_id;
    xSemaphoreGive(s_mutex);

    *out_handle = handle;
    return ESP_OK;

err_delete:
    (void)driver_i2c_master_delete(handle);
    return ESP_FAIL;

err_unlock:
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t system_comm_mgr_unregister_i2c_link(driver_i2c_master_handle_t handle)
{
    int slot = -1;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    slot = find_i2c_slot_by_handle(handle);
    if (slot < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    s_i2c_links[slot] = NULL;
    s_i2c_link_ids[slot] = 0;
    xSemaphoreGive(s_mutex);

    return driver_i2c_master_delete(handle);
}

esp_err_t system_comm_mgr_get_i2c_link(uint32_t link_id, driver_i2c_master_handle_t *out_handle)
{
    int slot = -1;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    slot = find_i2c_slot_by_id(link_id);
    if (slot < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    *out_handle = s_i2c_links[slot];
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t system_comm_mgr_register_uart_link(const system_uart_link_config_t *config,
                                             system_uart_link_handle_t *out_handle)
{
    int slot = -1;
    system_uart_link_handle_t handle = NULL;
    esp_err_t err = ESP_OK;

    ESP_RETURN_ON_FALSE(config && out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    ESP_GOTO_ON_FALSE(find_uart_slot_by_id(config->link_id) < 0,
                      ESP_ERR_INVALID_STATE, err_unlock, TAG, "duplicate UART link id");

    slot = find_free_uart_slot();
    ESP_GOTO_ON_FALSE(slot >= 0, ESP_ERR_NO_MEM, err_unlock, TAG, "no free UART slot");
    xSemaphoreGive(s_mutex);

    err = system_uart_link_create(config, &handle);
    if (err != ESP_OK) {
        return err;
    }

    ESP_GOTO_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, err_delete, TAG, "lock failed");
    s_uart_links[slot] = handle;
    s_uart_link_ids[slot] = config->link_id;
    xSemaphoreGive(s_mutex);

    *out_handle = handle;
    return ESP_OK;

err_delete:
    (void)system_uart_link_delete(handle);
    return ESP_FAIL;

err_unlock:
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t system_comm_mgr_unregister_uart_link(system_uart_link_handle_t handle)
{
    int slot = -1;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid handle");
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    slot = find_uart_slot_by_handle(handle);
    if (slot < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    s_uart_links[slot] = NULL;
    s_uart_link_ids[slot] = 0;
    xSemaphoreGive(s_mutex);

    return system_uart_link_delete(handle);
}

esp_err_t system_comm_mgr_get_uart_link(uint32_t link_id, system_uart_link_handle_t *out_handle)
{
    int slot = -1;

    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_ERROR(system_comm_mgr_init(), TAG, "system_comm_mgr_init failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE, ESP_FAIL, TAG, "lock failed");

    slot = find_uart_slot_by_id(link_id);
    if (slot < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    *out_handle = s_uart_links[slot];
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}
