#pragma once

#include "constants.h"

void format_header(uint8_t *header, uint16_t data_length);
unsigned char GetCheckSum(unsigned char *buffer, int buffer_len, int seed, int target);
int goodix_send_command(struct goodix_device *dev, uint8_t cmd_id, const uint8_t *data, uint16_t data_len, int seed, int expect_ack, int expect_reply, uint8_t *save_to_buffer);
