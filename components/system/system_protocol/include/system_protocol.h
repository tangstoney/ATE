#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_PROTOCOL_FRAME_HEADER               0xAA55U
#define SYSTEM_PROTOCOL_MAX_PAYLOAD_LEN            10U
#define SYSTEM_PROTOCOL_STATUS_FRAME_LEN           8U
#define SYSTEM_PROTOCOL_GENERAL_FRAME_BASE_LEN     9U

typedef enum {
    SYSTEM_PROTOCOL_FRAME_TYPE_REQUEST = 0x01,
    SYSTEM_PROTOCOL_FRAME_TYPE_RESPONSE = 0x02,
    SYSTEM_PROTOCOL_FRAME_TYPE_EVENT = 0x03,
    SYSTEM_PROTOCOL_FRAME_TYPE_STATUS = 0x04,
} system_protocol_frame_type_t;

typedef enum {
    SYSTEM_PROTOCOL_CMD_KEY_GPIO_TEST = 0x20,
    SYSTEM_PROTOCOL_CMD_LEFT_KNOB_TEST = 0x21,
    SYSTEM_PROTOCOL_CMD_RIGHT_KNOB_TEST = 0x22,
    SYSTEM_PROTOCOL_CMD_CENTER_KNOB_TEST = 0x23,
} system_protocol_command_t;

typedef enum {
    SYSTEM_PROTOCOL_STATUS_SUCCESS = 0x00,
    SYSTEM_PROTOCOL_STATUS_FAILURE = 0x01,
    SYSTEM_PROTOCOL_STATUS_HEADER_ERROR = 0x02,
    SYSTEM_PROTOCOL_STATUS_TYPE_ERROR = 0x03,
    SYSTEM_PROTOCOL_STATUS_COMMAND_ERROR = 0x04,
    SYSTEM_PROTOCOL_STATUS_LENGTH_ERROR = 0x05,
    SYSTEM_PROTOCOL_STATUS_DATA_ERROR = 0x06,
    SYSTEM_PROTOCOL_STATUS_CRC_ERROR = 0x07,
    SYSTEM_PROTOCOL_STATUS_BUSY = 0x08,
    SYSTEM_PROTOCOL_STATUS_PROCESSING = 0x09,
    SYSTEM_PROTOCOL_STATUS_RESOURCE_BUSY = 0x0A,
} system_protocol_status_t;

typedef struct {
    uint8_t device_id;
    system_protocol_frame_type_t frame_type;
    uint8_t command;
    uint16_t data_len;
    const uint8_t *data;
    system_protocol_status_t status;
    bool is_status_frame;
} system_protocol_frame_t;

esp_err_t system_protocol_crc16_ccitt(const uint8_t *data,
                                      size_t len,
                                      uint16_t *out_crc);

esp_err_t system_protocol_build_request(uint8_t device_id,
                                        uint8_t command,
                                        const uint8_t *payload,
                                        uint16_t payload_len,
                                        uint8_t *out_buf,
                                        size_t out_buf_size,
                                        size_t *out_frame_len);
esp_err_t system_protocol_build_response(uint8_t device_id,
                                         uint8_t command,
                                         const uint8_t *payload,
                                         uint16_t payload_len,
                                         uint8_t *out_buf,
                                         size_t out_buf_size,
                                         size_t *out_frame_len);
esp_err_t system_protocol_build_event(uint8_t device_id,
                                      uint8_t command,
                                      const uint8_t *payload,
                                      uint16_t payload_len,
                                      uint8_t *out_buf,
                                      size_t out_buf_size,
                                      size_t *out_frame_len);
esp_err_t system_protocol_build_status(uint8_t device_id,
                                       uint8_t command,
                                       system_protocol_status_t status,
                                       uint8_t *out_buf,
                                       size_t out_buf_size,
                                       size_t *out_frame_len);

esp_err_t system_protocol_parse(const uint8_t *buf,
                                size_t buf_len,
                                system_protocol_frame_t *out_frame,
                                size_t *out_frame_len);

#ifdef __cplusplus
}
#endif
