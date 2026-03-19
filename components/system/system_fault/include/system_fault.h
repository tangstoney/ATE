#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_FAULT_NONE = 0,
    SYSTEM_FAULT_TIMEOUT,
    SYSTEM_FAULT_CRC_ERROR,
    SYSTEM_FAULT_NACK,
    SYSTEM_FAULT_DISCONNECT,
    SYSTEM_FAULT_PORT_BUSY,
    SYSTEM_FAULT_BUS_STUCK,
    SYSTEM_FAULT_UNSUPPORTED_PROTOCOL,
    SYSTEM_FAULT_MALFORMED_FRAME,
    SYSTEM_FAULT_RESOURCE_BUSY,
    SYSTEM_FAULT_INVALID_RESPONSE,
    SYSTEM_FAULT_DRIVER_ERROR,
    SYSTEM_FAULT_NOT_FOUND,
} system_fault_code_t;

typedef enum {
    SYSTEM_FAULT_SOURCE_UNKNOWN = 0,
    SYSTEM_FAULT_SOURCE_UART,
    SYSTEM_FAULT_SOURCE_I2C,
    SYSTEM_FAULT_SOURCE_PROTOCOL,
    SYSTEM_FAULT_SOURCE_COMM_MGR,
    SYSTEM_FAULT_SOURCE_MODULE_SERVICE,
    SYSTEM_FAULT_SOURCE_INSTRUMENT_SERVICE,
    SYSTEM_FAULT_SOURCE_ROUTER,
} system_fault_source_t;

typedef struct {
    system_fault_code_t code;
    system_fault_source_t source;
    esp_err_t esp_err;
    uint32_t detail0;
    uint32_t detail1;
    int64_t timestamp_us;
} system_fault_record_t;

esp_err_t system_fault_init(void);
esp_err_t system_fault_clear(void);
esp_err_t system_fault_get_last(system_fault_record_t *out_record);

system_fault_code_t system_fault_from_esp_err(esp_err_t err);
system_fault_code_t system_fault_from_protocol_status(uint8_t status);
const char *system_fault_code_to_name(system_fault_code_t code);

esp_err_t system_fault_report(system_fault_source_t source,
                              system_fault_code_t code,
                              esp_err_t err,
                              uint32_t detail0,
                              uint32_t detail1);

#ifdef __cplusplus
}
#endif
