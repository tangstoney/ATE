#include "system_module_protocol.h"

#include <string.h>

#include "esp_check.h"

static const char *TAG = "sys_mod_protocol";

static void write_u16_be(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
}

static uint16_t read_u16_be(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

static esp_err_t system_module_protocol_crc16_ccitt(const uint8_t *data,
                                                    size_t len,
                                                    uint16_t *out_crc)
{
    uint16_t crc = 0xFFFF;

    ESP_RETURN_ON_FALSE(data && out_crc, ESP_ERR_INVALID_ARG, TAG, "invalid args");

    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000U) {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            } else {
                crc <<= 1;
            }
        }
    }

    *out_crc = crc;
    return ESP_OK;
}

static esp_err_t build_general_frame(system_module_protocol_frame_type_t frame_type,
                                     uint8_t device_id,
                                     uint8_t command,
                                     const uint8_t *payload,
                                     uint16_t payload_len,
                                     uint8_t *out_buf,
                                     size_t out_buf_size,
                                     size_t *out_frame_len)
{
    uint16_t crc = 0;
    size_t frame_len = SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN + payload_len;

    ESP_RETURN_ON_FALSE(out_buf && out_frame_len, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(payload_len <= SYSTEM_MODULE_PROTOCOL_MAX_PAYLOAD_LEN,
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "payload too large");
    ESP_RETURN_ON_FALSE(frame_len <= out_buf_size, ESP_ERR_INVALID_SIZE, TAG, "buffer too small");
    ESP_RETURN_ON_FALSE(device_id != 0, ESP_ERR_INVALID_ARG, TAG, "device id must not be zero");
    if (payload_len > 0) {
        ESP_RETURN_ON_FALSE(payload, ESP_ERR_INVALID_ARG, TAG, "payload is NULL");
    }

    out_buf[0] = (uint8_t)(SYSTEM_MODULE_PROTOCOL_FRAME_HEADER >> 8);
    out_buf[1] = (uint8_t)(SYSTEM_MODULE_PROTOCOL_FRAME_HEADER & 0xFF);
    out_buf[2] = device_id;
    out_buf[3] = (uint8_t)frame_type;
    out_buf[4] = command;
    write_u16_be(&out_buf[5], payload_len);
    if (payload_len > 0) {
        memcpy(&out_buf[7], payload, payload_len);
    }

    ESP_RETURN_ON_ERROR(system_module_protocol_crc16_ccitt(&out_buf[2], 5U + payload_len, &crc),
                        TAG,
                        "crc calc failed");
    write_u16_be(&out_buf[7 + payload_len], crc);
    *out_frame_len = frame_len;
    return ESP_OK;
}

esp_err_t system_module_protocol_build_request(uint8_t device_id,
                                               uint8_t command,
                                               const uint8_t *payload,
                                               uint16_t payload_len,
                                               uint8_t *out_buf,
                                               size_t out_buf_size,
                                               size_t *out_frame_len)
{
    return build_general_frame(SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_REQUEST,
                               device_id,
                               command,
                               payload,
                               payload_len,
                               out_buf,
                               out_buf_size,
                               out_frame_len);
}

esp_err_t system_module_protocol_parse(const uint8_t *buf,
                                       size_t buf_len,
                                       system_module_protocol_frame_t *out_frame,
                                       size_t *out_frame_len)
{
    uint16_t expected_crc = 0;
    uint16_t actual_crc = 0;
    size_t frame_len = 0;

    ESP_RETURN_ON_FALSE(buf && out_frame && out_frame_len, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(buf_len >= SYSTEM_MODULE_PROTOCOL_STATUS_FRAME_LEN,
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "frame too short");
    ESP_RETURN_ON_FALSE(read_u16_be(buf) == SYSTEM_MODULE_PROTOCOL_FRAME_HEADER,
                        ESP_ERR_INVALID_RESPONSE,
                        TAG,
                        "bad header");

    memset(out_frame, 0, sizeof(*out_frame));
    out_frame->device_id = buf[2];
    out_frame->frame_type = (system_module_protocol_frame_type_t)buf[3];
    out_frame->command = buf[4];

    if (out_frame->frame_type == SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_STATUS) {
        frame_len = SYSTEM_MODULE_PROTOCOL_STATUS_FRAME_LEN;
        ESP_RETURN_ON_FALSE(buf_len >= frame_len, ESP_ERR_INVALID_SIZE, TAG, "status frame too short");

        expected_crc = read_u16_be(&buf[6]);
        ESP_RETURN_ON_ERROR(system_module_protocol_crc16_ccitt(&buf[2], 4, &actual_crc),
                            TAG,
                            "crc calc failed");
        ESP_RETURN_ON_FALSE(expected_crc == actual_crc, ESP_ERR_INVALID_RESPONSE, TAG, "crc mismatch");

        out_frame->is_status_frame = true;
        out_frame->status = (system_module_protocol_status_t)buf[5];
        *out_frame_len = frame_len;
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(out_frame->frame_type == SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_REQUEST ||
                        out_frame->frame_type == SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_RESPONSE ||
                        out_frame->frame_type == SYSTEM_MODULE_PROTOCOL_FRAME_TYPE_EVENT,
                        ESP_ERR_INVALID_RESPONSE,
                        TAG,
                        "unsupported frame type");

    out_frame->data_len = read_u16_be(&buf[5]);
    ESP_RETURN_ON_FALSE(out_frame->data_len <= SYSTEM_MODULE_PROTOCOL_MAX_PAYLOAD_LEN,
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "payload too large");

    frame_len = SYSTEM_MODULE_PROTOCOL_GENERAL_FRAME_BASE_LEN + out_frame->data_len;
    ESP_RETURN_ON_FALSE(buf_len >= frame_len, ESP_ERR_INVALID_SIZE, TAG, "general frame too short");

    expected_crc = read_u16_be(&buf[7 + out_frame->data_len]);
    ESP_RETURN_ON_ERROR(system_module_protocol_crc16_ccitt(&buf[2], 5U + out_frame->data_len, &actual_crc),
                        TAG,
                        "crc calc failed");
    ESP_RETURN_ON_FALSE(expected_crc == actual_crc, ESP_ERR_INVALID_RESPONSE, TAG, "crc mismatch");

    out_frame->is_status_frame = false;
    out_frame->data = out_frame->data_len > 0 ? &buf[7] : NULL;
    *out_frame_len = frame_len;
    return ESP_OK;
}
