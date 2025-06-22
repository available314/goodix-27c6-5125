#include "goodix_protocol.h"

#include "goodix_usb.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
