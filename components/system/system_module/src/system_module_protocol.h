#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_MODULE_PROTOCOL_DEFAULT_DEVICE_ID    0x02U
#define SYSTEM_MODULE_PROTOCOL_FRAME_HEADER         0xAA55U
#define SYSTEM_MODULE_PROTOCOL_MAX_PAYLOAD_LEN      10U
#define SYSTEM_MODULE_PROTOCOL_STATUS_FRAME_LEN     8U
#define SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN 9U

typedef enum {
    SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_REQUEST = 0x01,
    SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_RESPONSE = 0x02,
    SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_EVENT = 0x03,
    SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_STATUS = 0x04,
} system_module_protocol_frame_type_t;

typedef enum {
    SYSTEM_MODULE_PROTOCOL_STATUS_SUCCESS = 0x00,
    SYSTEM_MODULE_PROTOCOL_STATUS_FAILURE = 0x01,
    SYSTEM_MODULE_PROTOCOL_STATUS_HEADER_ERROR = 0x02,
    SYSTEM_MODULE_PROTOCOL_STATUS_TYPE_ERROR = 0x03,
    SYSTEM_MODULE_PROTOCOL_STATUS_COMMAND_ERROR = 0x04,
    SYSTEM_MODULE_PROTOCOL_STATUS_LENGTH_ERROR = 0x05,
    SYSTEM_MODULE_PROTOCOL_STATUS_DATA_ERROR = 0x06,
    SYSTEM_MODULE_PROTOCOL_STATUS_CRC_ERROR = 0x07,
    SYSTEM_MODULE_PROTOCOL_STATUS_BUSY = 0x08,
    SYSTEM_MODULE_PROTOCOL_STATUS_PROCESSING = 0x09,
    SYSTEM_MODULE_PROTOCOL_STATUS_RESOURCE_BUSY = 0x0A,
} system_module_protocol_status_t;

typedef struct {
    uint8_t device_id;
    system_module_protocol_frame_type_t frame_type;
    uint8_t command;
    uint16_t data_len;
    const uint8_t *data;
    system_module_protocol_status_t status;
    bool is_status_frame;
} system_module_protocol_frame_t;

esp_err_t system_module_protocol_build_request(uint8_t device_id,
                                               uint8_t command,
                                               const uint8_t *payload,
                                               uint16_t payload_len,
                                               uint8_t *out_buf,
                                               size_t out_buf_size,
                                               size_t *out_frame_len);

esp_err_t system_module_protocol_parse(const uint8_t *buf,
                                       size_t buf_len,
                                       system_module_protocol_frame_t *out_frame,
                                       size_t *out_frame_len);

#ifdef __cplusplus
}
#endif
