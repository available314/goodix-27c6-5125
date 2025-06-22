#pragma once

#include <stdint.h>


uint8_t* set_mcu_state_buffer();
uint8_t* set_read_sensor_register_buffer(uint8_t base_address, uint8_t len);
uint8_t* set_reg_buffer(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
void fdt_mode_construct_payload(uint8_t *main_buffer, uint8_t *help_buffer, int len);
