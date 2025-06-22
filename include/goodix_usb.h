#pragma once

#include "constants.h"

int goodix_init(struct goodix_device *dev);
int goodix_send_packet(struct goodix_device *dev, const uint8_t *packet, int packet_len); 
int goodix_get_reply(struct goodix_device *dev, uint8_t *save_to_buffer);
void goodix_disconnect(struct goodix_device *dev);
int get_string_descriptor(libusb_device_handle *dev_handle, uint8_t index, uint16_t langid, uint8_t *buf, int size);
int get_all_string_descriptors(struct goodix_device *dev);
void prebulktransfer(struct goodix_device *dev);
