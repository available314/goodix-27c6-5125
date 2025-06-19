#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "constants.h"
#include "tls.h"

int goodix_init(struct goodix_device *dev) {
   int r = libusb_init(NULL);

   if (r < 0) return r;

   dev->handle = libusb_open_device_with_vid_pid(NULL, GOODIX_VID, GOODIX_PID);
   if (!dev->handle) return -1;

   if (libusb_kernel_driver_active(dev->handle, 0)) {
      libusb_detach_kernel_driver(dev->handle, 0);
   }

   r = libusb_claim_interface(dev->handle, 0);
   if (r < 0) {
      libusb_close(dev->handle);

      return r;
   }
   
   pthread_mutex_init(&dev->usb_mutex, NULL);
   dev->timeout = TIMEOUT_MS;
   dev->last_cmd_type = 0;

   dev->tls_sockfd = tls_connect(TLS_SERVER_HOST, TLS_SERVER_PORT);

   if (dev->tls_sockfd <= 0) {
      return -1;
   }

   return 0;
}

int get_string_descriptor(libusb_device_handle *dev_handle, uint8_t index, uint16_t langid, uint8_t *buf, int size) {
    int ret = libusb_control_transfer(
        dev_handle,
        LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_DESCRIPTOR,
        (LIBUSB_DT_STRING << 8) | index,
        langid,
        buf,
        size,
        TIMEOUT_MS);
    
    if (ret < 0) {
        fprintf(stderr, "Failed to get string descriptor %d: %s\n", 
                index, libusb_error_name(ret));
        return ret;
    }
    return ret;
}

int get_all_string_descriptors(struct goodix_device *dev) {
   uint8_t buf[256];
   uint16_t langids[32];
   int i, ret;

   ret = get_string_descriptor(dev->handle, 0x0, 0, (uint8_t*)langids, sizeof(langids));
   if (ret < 0) return ret;

   int num_langs = (ret - 2) / 2;
   printf("Supported languages: %d\n", num_langs);

   uint16_t langid = langids[1];

   ret = get_string_descriptor(dev->handle, 0x1, langid, buf, sizeof(buf));
   if (ret > 0) {
      printf("Manufacturer: %s\n", buf+2); 
   }

   ret = get_string_descriptor(dev->handle, 0x2, langid, buf, sizeof(buf));
   if (ret > 0) {
      printf("Product: %s\n", buf+2);
   }

   return 0;
}

void goodix_disconnect(struct goodix_device *dev) {
   libusb_release_interface(dev->handle, 0);
   libusb_close(dev->handle);
   close_connection(dev->tls_sockfd);
   pthread_mutex_destroy(&dev->usb_mutex);
   libusb_exit(NULL);
}



void format_header(uint8_t *header, uint16_t data_length) {
    header[0] = (header[0] & 0x0F) | 0xA0;      
    header[1] = (uint8_t)(data_length);    
    header[2] = (uint8_t)(data_length >> 8);   
    header[3] = header[0] + header[1] + header[2];
}

unsigned char GetCheckSum(unsigned char *buffer, int buffer_len, int seed, int target) {
   if (buffer_len <= 0) {
      printf("Error, wrong buffer_size\n");

      return 0x88;
   }

   unsigned char sum = seed;
   for (int i = 0; i < buffer_len; i++) {
      sum += buffer[i];
   }
   sum = target - sum;
   return sum; 
}

int goodix_send_packet(struct goodix_device *dev, const uint8_t *packet, int packet_len) {
   static uint8_t buffer[MAX_PACKET_SIZE];
   memset(buffer, 0, sizeof(buffer));

   int sent = 0, ret;
   while (sent < packet_len) {
      int chunk_size = (packet_len - sent) > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : (packet_len - sent);
      memcpy(buffer, packet + sent, chunk_size);

      int actual;
      ret = libusb_bulk_transfer(dev->handle, USB_BULK_EP_OUT, buffer, MAX_PACKET_SIZE, &actual, dev->timeout);

      if (ret != LIBUSB_SUCCESS || actual != MAX_PACKET_SIZE) {
         return -1;
      }
      sent += chunk_size;
   }
   return 0;
}

int goodix_get_reply(struct goodix_device *dev, uint8_t *save_to_buffer) {
   uint8_t reply_buf[8000];

   int reply_ret = -1;
   int total_wait = 0;
   while (total_wait < MAX_WAIT_TIME) {
      int actual;
      int ret = libusb_bulk_transfer(dev->handle, USB_BULK_EP_IN, reply_buf, sizeof(reply_buf), &actual, ACK_TIMEOUT_MS);

      if (ret == LIBUSB_SUCCESS && actual >= 10) {
         if (save_to_buffer != NULL) {
            memcpy(save_to_buffer, reply_buf, actual);
         }
         return actual;
      }
      total_wait += ACK_TIMEOUT_MS;
      usleep(ACK_TIMEOUT_MS * 1000);
   }
   return -1;
}

int goodix_send_command(struct goodix_device *dev, uint8_t cmd_id, const uint8_t *data, uint16_t data_len, int seed, int expect_ack, int expect_reply, uint8_t *save_to_buffer) {
   int ret;
   uint8_t *packet;
   uint16_t packet_len = 0;

   if (cmd_id != 0xff) {
      packet_len = data_len + 8;
      uint8_t checksum = 0x88;

      packet = calloc(1, packet_len);
      if (!packet) {
         return -1;
      }

      format_header(packet, data_len + 4);

      uint8_t *payload = packet + 4;
      payload[0] = cmd_id;
      payload[1] = (uint8_t)(data_len + 1);
      payload[2] = (uint8_t)((data_len + 1) >> 8);

      if (data_len > 0) {
         memcpy(payload + 3, data, data_len);
      }

      if (seed != -1) {
         checksum = GetCheckSum(payload + 3, data_len, seed, 0xaa);
      }
      payload[3 + data_len] = checksum;
   } else {
      packet_len = data_len;
      packet = calloc(1, packet_len);

      memcpy(packet, data, packet_len);
   }

   /*if (expect_ack || expect_reply) {
      uint8_t dummy_buffer[64];
      pthread_mutex_lock(&dev->usb_mutex);
      int tmp;
      libusb_bulk_transfer(dev->handle, USB_BULK_EP_IN, dummy_buffer, 64, &tmp, 1);
      pthread_mutex_unlock(&dev->usb_mutex);
   }*/

   pthread_mutex_lock(&dev->usb_mutex);

   ret = goodix_send_packet(dev, packet, packet_len);

   if (ret == -1) {
      pthread_mutex_unlock(&dev->usb_mutex);
      free(packet);

      return -1;
   }
   
   free(packet);

   dev->last_cmd_type = cmd_id;

   if (expect_ack) {
      uint8_t ack_buf[500];

      int ack_ret = -1;
      int total_wait = 0;

      while (total_wait < MAX_WAIT_TIME) {
         int actual;
         ret = libusb_bulk_transfer(dev->handle, USB_BULK_EP_IN, ack_buf, sizeof(ack_buf), &actual, ACK_TIMEOUT_MS);

         if (ret == LIBUSB_SUCCESS && actual == 10 && ack_buf[4] == 0xb0) {    
            ret = 0;
            break;
         }
         total_wait += ACK_TIMEOUT_MS;
         usleep(ACK_TIMEOUT_MS * 1000);
      }

      if (ret) {
         pthread_mutex_unlock(&dev->usb_mutex);
         return -1;
      }
   }

   if (expect_reply) {
      if (goodix_get_reply(dev, save_to_buffer) == -1) {
         pthread_mutex_unlock(&dev->usb_mutex);
         return -1;
      }
   }

   pthread_mutex_unlock(&dev->usb_mutex);
   return 0;
}

uint8_t* set_mcu_state_buffer() {
   struct timespec ts;
   uint16_t timestamp;
    
   clock_gettime(CLOCK_REALTIME, &ts);
   timestamp = (ts.tv_sec % 60) * 1000 + (ts.tv_nsec / 1000000);

   memcpy(MCU_QUERY_STATE_BUF + 1, &timestamp, sizeof(timestamp));

   return MCU_QUERY_STATE_BUF;
}

uint8_t* set_read_sensor_register_buffer(uint8_t base_address, uint8_t len) {
   READ_SENSOR_REGISTER_BUF[1] = base_address;
   READ_SENSOR_REGISTER_BUF[3] = len;
   return READ_SENSOR_REGISTER_BUF;
}

uint8_t* set_reg_buffer(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
   REG_BUF[1] = byte0;
   REG_BUF[2] = byte1;
   REG_BUF[3] = byte2;
   REG_BUF[4] = byte3;

   return REG_BUF;
}

void prebulktransfer(struct goodix_device *dev) { 
   uint8_t lolbuf[1];
   int loltrans;
   libusb_bulk_transfer(dev->handle, USB_BULK_EP_IN, lolbuf, 1, &loltrans, 500);
}

int windows_initialization(struct goodix_device *dev) {
   if (get_all_string_descriptors(dev)) return -1;

   if (goodix_send_command(dev, COMMAND_NOP, NOP_BUF, NOP_BUF_LEN, SEED_NOP, 0, 0, NULL)) return -1;


   if (goodix_send_command(dev, 
            COMMAND_TLS_SUCCESSFULLY_ESTABLISHED, 
            TLS_SUCCESSFULLY_ESTABLISHED_BUF, 
            TLS_SUCCESSFULLY_ESTABLISHED_BUF_LEN, 
            SEED_TLS_SUCCESSFULLY_ESTABLISHED, 0, 0, NULL)) return -1;


   //prebulktransfer(dev);
   if (goodix_send_command(dev, COMMAND_NOP, NOP_BUF, NOP_BUF_LEN, SEED_NOP, 0, 0, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_FIRMWARE_VERSION, FIRMWARE_VERSION_BUF, FIRMWARE_VERSION_BUF_LEN, SEED_FIRMWARE_VERSION, 1, 1, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_ENABLE_CHIP, ENABLE_CHIP_BUF, ENABLE_CHIP_BUF_LEN, SEED_ENABLE_CHIP, 0, 0, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_QUERY_MCU_STATE, set_mcu_state_buffer(), MCU_QUERY_STATE_BUF_LEN, SEED_MCU_QUERY_STATE, 0, 1, NULL)) return -1;


   //prebulktransfer(dev);
   if (goodix_send_command(dev, COMMAND_NOP, NOP_BUF, NOP_BUF_LEN, SEED_NOP, 0, 0, NULL)) return -1;
   
   if (goodix_send_command(dev, COMMAND_FIRMWARE_VERSION, FIRMWARE_VERSION_BUF, FIRMWARE_VERSION_BUF_LEN, SEED_FIRMWARE_VERSION, 1, 1, NULL)) return -1;

   //prebulktransfer(dev);
   if (goodix_send_command(dev, COMMAND_PRESET_PSK_READ_R, PRESET_PSK_READ_R_BUF, PRESET_PSK_READ_R_BUF_LEN, SEED_PRESET_PSK_READ_R, 1, 1, NULL)) return -1;

   //prebulktransfer(dev);
   if (goodix_send_command(dev, 0xff, PRESET_PSK_WRITE_R_BUF, PRESET_PSK_WRITE_R_BUF_LEN, SEED_NOP, 1, 1, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_PRESET_PSK_READ_R, PRESET_PSK_READ_R_BUF, PRESET_PSK_READ_R_BUF_LEN, SEED_PRESET_PSK_READ_R, 1, 1, NULL)) return -1;

   //prebulktransfer(dev);
   if (goodix_send_command(dev, COMMAND_RESET, RESET_BUF, RESET_BUF_LEN, SEED_RESET, 1, 1, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_READ_SENSOR_REGISTER, set_read_sensor_register_buffer(0x0, 0x4), READ_SENSOR_REGISTER_BUF_LEN, SEED_READ_SENSOR_REGISTER, 1, 1, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_READ_OTP, READ_OTP_BUF, READ_OTP_BUF_LEN, SEED_READ_OTP, 1, 1, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_RESET, RESET_BUF, RESET_BUF_LEN, SEED_RESET, 1, 1, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_MCU_SWITCH_TO_IDLE_MODE, MCU_SWITCH_TO_IDLE_MODE_BUF, MCU_SWITCH_TO_IDLE_MODE_BUF_LEN, SEED_MCU_SWITCH_TO_IDLE_MODE, 1, 0, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_REG, set_reg_buffer(0x20, 0x2, 0x48, 0x0b), REG_BUF_LEN, SEED_REG, 1, 0, NULL)) return -1;
   if (goodix_send_command(dev, COMMAND_REG, set_reg_buffer(0x36, 0x2, 0xb6, 0x0), REG_BUF_LEN, SEED_REG, 1, 0, NULL)) return -1;
   if (goodix_send_command(dev, COMMAND_REG, set_reg_buffer(0x38, 0x2, 0xb4, 0x0), REG_BUF_LEN, SEED_REG, 1, 0, NULL)) return -1;
   if (goodix_send_command(dev, COMMAND_REG, set_reg_buffer(0x3a, 0x2, 0xb4, 0x0), REG_BUF_LEN, SEED_REG, 1, 0, NULL)) return -1;

   if (goodix_send_command(dev, 0xff, MCU_DOWNLOAD_CHIP_CONFIG_BUF, MCU_DOWNLOAD_CHIP_CONFIG_BUF_LEN, SEED_NOP, 1, 1, NULL)) return -1;

   return 0;
}

int goodix_connect_tls(struct goodix_device *dev) {
   uint8_t client_hello_message[500];
   ///prebulktransfer(dev);
 
   if (goodix_send_command(dev, 
            COMMAND_REQUEST_TLS_CONNECTION, 
            REQUEST_TLS_CONNECTION_BUF, 
            REQUEST_TLS_CONNECTION_BUF_LEN, 
            SEED_REQUEST_TLS_CONNECTION, 0, 1, client_hello_message)) return -1;

   for (int i = 0; i < 56; i++) {
      printf("%02x ", client_hello_message[i]);
      if ((i + 1) % 16 == 0) printf("\n");
   }
   printf("\n");

   if (send_all(dev->tls_sockfd, client_hello_message + 4, 52)) {
      return -1;
   }
   
   uint8_t tls_server_response[4096];
   
   tls_server_response[0] = 0xb0;
   tls_server_response[1] = 0x56;
   tls_server_response[2] = 0x00;
   tls_server_response[3] = 0x06;

   int recv_len = 4 + recv_some(dev->tls_sockfd, tls_server_response + 4, sizeof(tls_server_response) - 4);
   printf("Got %d bytes from openssl server:\n", recv_len);
   for (int i = 0; i < recv_len; i++) {
      printf("%02x ", tls_server_response[i]);
      if ((i + 1) % 16 == 0) printf("\n");
   }
   printf("\n");

   if (goodix_send_command(dev, 0xff, tls_server_response, recv_len - 9, SEED_NOP, 0, 0, NULL)) return -1;
   
   // very important transfer
   prebulktransfer(dev);
   // for now idk why

   if (goodix_send_command(dev, 0xff, TLS_SERVER_HELLO_DONE_BUF, TLS_SERVER_HELLO_DONE_BUF_LEN, SEED_NOP, 0, 0, NULL)) return -1;

   usleep(100000);

   int padding = 0;
   uint8_t tls_handshake_payload[4096], tls_tmp_buffer[500];
   
   int len = goodix_get_reply(dev, tls_tmp_buffer) - 4;
   memcpy(tls_handshake_payload + padding, tls_tmp_buffer + 4, len);
   padding += len;

   len = goodix_get_reply(dev, tls_tmp_buffer) - 4;
   memcpy(tls_handshake_payload + padding, tls_tmp_buffer + 4, len);
   padding += len;

   len = goodix_get_reply(dev, tls_tmp_buffer) - 4;
   memcpy(tls_handshake_payload + padding, tls_tmp_buffer + 4, len);
   padding += len;

   printf("Got 3 message %d bytes from openssl server:\n", recv_len);
   for (int i = 0; i < padding; i++) {
      printf("%02x ", tls_handshake_payload[i]);
      if ((i + 1) % 16 == 0) printf("\n");
   }
   printf("\n");

   if (send_all(dev->tls_sockfd, tls_handshake_payload, padding)) return -1;
   recv_len = recv_some(dev->tls_sockfd, tls_server_response, sizeof(tls_server_response));

   if (recv_len < 0) {
      return -1;
   }

   printf("Recieved %d for handshake from tls\n", recv_len);
 
   memset(tls_handshake_payload, 0, sizeof(tls_handshake_payload));
  
   tls_handshake_payload[0] = 0xb0;
   tls_handshake_payload[1] = 0x06;
   tls_handshake_payload[2] = 0x00;
   tls_handshake_payload[3] = 0xb6;
   memcpy(tls_handshake_payload + 4, tls_server_response, 6);
   if (goodix_send_command(dev, 0xff, tls_handshake_payload, 10, SEED_NOP, 0, 0, NULL)) return -1;

   tls_handshake_payload[0] = 0xb0;
   tls_handshake_payload[1] = 0x2d;
   tls_handshake_payload[2] = 0x00;
   tls_handshake_payload[3] = 0xdd;
   memcpy(tls_handshake_payload + 4, tls_server_response + 6, recv_len - 6);
   if (goodix_send_command(dev, 0xff, tls_handshake_payload, recv_len - 2, SEED_NOP, 0, 0, NULL)) return -1;

   prebulktransfer(dev);
   if (goodix_send_command(dev, 
            0xd4, 
            TLS_SUCCESSFULLY_ESTABLISHED_BUF, 
            TLS_SUCCESSFULLY_ESTABLISHED_BUF_LEN, 
            SEED_TLS_SUCCESSFULLY_ESTABLISHED, 1, 0, NULL)) return -1;

   if (goodix_send_command(dev, COMMAND_QUERY_MCU_STATE, set_mcu_state_buffer(), MCU_QUERY_STATE_BUF_LEN, SEED_MCU_QUERY_STATE, 0, 1, NULL)) return -1;

   return 0;
}

void fdt_mode_construct_payload(uint8_t *main_buffer, uint8_t *help_buffer, int len) {
   for (int i = 0; i < len; i += 2) {
     uint16_t value = help_buffer[i] | (help_buffer[i + 1] << 8);

     value >>= 1;

     main_buffer[i] = 0x80;
     main_buffer[i + 1] = value & 0xFF;
   }
}

int goodix_mcu_switch_to_fdt_mode(struct goodix_device *dev, int cmd_id) {
   if (goodix_send_command(dev, cmd_id, MCU_SWITCH_TO_FDT_MODE_BUF, MCU_SWITCH_TO_FDT_MODE_BUF_LEN, SEED_MCU_SWITCH_TO_FDT_MODE, 1, 1, MCU_SWITCH_TO_FDT_MODE_HELP)) return -1;
   fdt_mode_construct_payload(MCU_SWITCH_TO_FDT_MODE_BUF + 2, MCU_SWITCH_TO_FDT_MODE_HELP + 11, 12);
   return 0;
}

int get_first_image(struct goodix_device *dev) {

   if (goodix_mcu_switch_to_fdt_mode(dev, COMMAND_MCU_SWITCH_TO_FDT_MODE)) return -1;

   if (goodix_mcu_switch_to_fdt_mode(dev, COMMAND_MCU_SWITCH_TO_FDT_MODE)) return -1;

   prebulktransfer(dev);
   if (goodix_send_command(dev, COMMAND_MCU_GET_IMAGE, MCU_GET_IMAGE_BUF, MCU_GET_IMAGE_BUF_LEN, SEED_MCU_GET_IMAGE, 1, 1, NULL)) return -1;
   
   return 0;
}

int main(int argc, char *argv[]) {

   struct goodix_device dev;

   if (goodix_init(&dev)) {
      fprintf(stderr, "Device initialization failed!\n");
      return 1;
   }

   int status = windows_initialization(&dev);

   if (status) {
      printf("Failed to proccess all queries\n");
   }

   
   status = goodix_connect_tls(&dev);

   if (status) {
      printf("Failed to establish tls\n");
   }


   status = get_first_image(&dev);

   if (status) {
      printf("Failed to get image\n");
   }

   goodix_disconnect(&dev);

   return 0;
}
