# driver_i2c_master

ATE I2C master driver wrapper for ESP-IDF v5.5.3 on ESP32-P4.

Responsibilities:
- Create and delete I2C master buses
- Add and remove I2C devices
- Perform synchronous transmit / receive / transmit-receive operations
- Provide probe and bus recovery helpers

Backend:
- `driver/i2c_master.h`
- Component dependency: `esp_driver_i2c`

Out of scope:
- Module enumeration policy
- Business command decoding
- UI or routing logic

Basic usage:

```c
driver_i2c_master_handle_t bus = NULL;
driver_i2c_device_handle_t dev = NULL;

driver_i2c_master_config_t bus_cfg = {
    .port = I2C_NUM_0,
    .sda_io = 7,
    .scl_io = 8,
    .clk_speed_hz = 400000,
    .timeout_ms = 1000,
};

driver_i2c_device_config_t dev_cfg = {
    .dev_addr = 0x2A,
};

ESP_ERROR_CHECK(driver_i2c_master_create(&bus_cfg, &bus));
ESP_ERROR_CHECK(driver_i2c_device_add(bus, &dev_cfg, &dev));
ESP_ERROR_CHECK(driver_i2c_write(dev, tx_buf, tx_len));
ESP_ERROR_CHECK(driver_i2c_device_remove(dev));
ESP_ERROR_CHECK(driver_i2c_master_delete(bus));
```
