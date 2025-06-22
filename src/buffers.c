#include "buffers.h"
#include "constants.h"

#include <string.h>
#include <time.h>

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

void fdt_mode_construct_payload(uint8_t *main_buffer, uint8_t *help_buffer, int len) {
   for (int i = 0; i < len; i += 2) {
     uint16_t value = help_buffer[i] | (help_buffer[i + 1] << 8);

     value >>= 1;

     main_buffer[i] = 0x80;
     main_buffer[i + 1] = value & 0xFF;
   }
}
