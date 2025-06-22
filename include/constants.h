#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>

#define GOODIX_VID 0x27c6
#define GOODIX_PID 0x5125
#define TIMEOUT_MS 1000
#define MAX_PACKET_SIZE 64
#define USB_BULK_EP_OUT 0x01
#define MAX_WAIT_TIME 1000
#define USB_BULK_EP_IN 0x81
#define ACK_TIMEOUT_MS 500
#define TLS_SERVER_HOST "127.0.0.1"
#define TLS_SERVER_PORT 5353

#define NOP_BUF_LEN 4
#define TLS_SUCCESSFULLY_ESTABLISHED_BUF_LEN 2
#define FIRMWARE_VERSION_BUF_LEN 2
#define ENABLE_CHIP_BUF_LEN 2
#define MCU_QUERY_STATE_BUF_LEN 5
#define PRESET_PSK_READ_R_BUF_LEN 8
#define RESET_BUF_LEN 2
#define READ_SENSOR_REGISTER_BUF_LEN 5
#define READ_OTP_BUF_LEN 2
#define MCU_SWITCH_TO_IDLE_MODE_BUF_LEN 2
#define REG_BUF_LEN 5
#define MCU_DOWNLOAD_CHIP_CONFIG_BUF_LEN 256
#define REQUEST_TLS_CONNECTION_BUF_LEN 2
#define TLS_SERVER_HELLO_DONE_BUF_LEN 13
#define PRESET_PSK_WRITE_R_BUF_LEN 512
#define MCU_SWITCH_TO_FDT_MODE_BUF_LEN 14
#define MCU_GET_IMAGE_BUF_LEN 2

enum {
   COMMAND_NOP = 0x01,
   COMMAND_FIRMWARE_VERSION = 0xa8,
   COMMAND_ENABLE_CHIP = 0x97,
   COMMAND_QUERY_MCU_STATE = 0xaf,
   COMMAND_PRESET_PSK_READ_R = 0xe4,
   COMMAND_TLS_SUCCESSFULLY_ESTABLISHED = 0xd5,
   COMMAND_RESET = 0xa2,
   COMMAND_READ_SENSOR_REGISTER = 0x82,
   COMMAND_READ_OTP = 0xa6,
   COMMAND_MCU_SWITCH_TO_IDLE_MODE = 0x70,
   COMMAND_REG = 0x80,
   COMMAND_REQUEST_TLS_CONNECTION = 0xd1,
   COMMAND_MCU_SWITCH_TO_FDT_MODE = 0x36,
   COMMAND_MCU_GET_IMAGE = 0x20,
};

enum {
   SEED_NOP = -1,
   SEED_MCU_SWITCH_TO_FDT_MODE = 69,
   SEED_MCU_QUERY_STATE = 180,
   SEED_RESET = 165,
   SEED_ENABLE_CHIP = 153,
   SEED_FIRMWARE_VERSION = 171,
   SEED_TLS_SUCCESSFULLY_ESTABLISHED = 215,
   SEED_PRESET_PSK_READ_R = 237,
   SEED_READ_SENSOR_REGISTER = 136,
   SEED_READ_OTP = 169,
   SEED_MCU_SWITCH_TO_IDLE_MODE = 115,
   SEED_REG = 134,
   SEED_REQUEST_TLS_CONNECTION = 211,
   SEED_MCU_GET_IMAGE = 35,
};

struct goodix_device {
   libusb_device_handle *handle;
   pthread_mutex_t usb_mutex;
   int timeout;
   uint8_t last_cmd_type;
   int tls_sockfd;
};

#pragma pack(push, 1)
struct goodix_response {
   uint8_t goodix_message[4]; // including header checksum
   uint8_t command_id;
   uint8_t data[0];
};
#pragma pack(pop)


extern const uint8_t NOP_BUF[4];
extern const uint8_t TLS_SUCCESSFULLY_ESTABLISHED_BUF[2];
extern const uint8_t FIRMWARE_VERSION_BUF[2];
extern const uint8_t ENABLE_CHIP_BUF[2];
extern const uint8_t PRESET_PSK_READ_R_BUF[8];
extern const uint8_t RESET_BUF[2];
extern const uint8_t READ_OTP_BUF[2];
extern const uint8_t MCU_SWITCH_TO_IDLE_MODE_BUF[2];
extern const uint8_t MCU_DOWNLOAD_CHIP_CONFIG_BUF[256];
extern const uint8_t REQUEST_TLS_CONNECTION_BUF[2];
extern const uint8_t TLS_SERVER_HELLO_DONE_BUF[13];
extern const uint8_t PRESET_PSK_WRITE_R_BUF[512];
extern const uint8_t MCU_GET_IMAGE_BUF[2];

extern uint8_t MCU_QUERY_STATE_BUF[5];
extern uint8_t READ_SENSOR_REGISTER_BUF[5];
extern uint8_t REG_BUF[5];


extern uint8_t MCU_SWITCH_TO_FDT_MODE_BUF[14];
extern uint8_t MCU_SWITCH_TO_FDT_MODE_HELP[24];

#endif
