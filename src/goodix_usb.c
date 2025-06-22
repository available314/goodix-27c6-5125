#include "goodix_usb.h"

#include "tls.h"

#include <pthread.h>
#include <string.h>

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


void goodix_disconnect(struct goodix_device *dev) {
   libusb_release_interface(dev->handle, 0);
   libusb_close(dev->handle);
   close_connection(dev->tls_sockfd);
   pthread_mutex_destroy(&dev->usb_mutex);
   libusb_exit(NULL);
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


void prebulktransfer(struct goodix_device *dev) { 
   uint8_t lolbuf[1];
   int loltrans;
   libusb_bulk_transfer(dev->handle, USB_BULK_EP_IN, lolbuf, 1, &loltrans, 500);
}
