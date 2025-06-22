#include "goodix_init_seq.h"

#include "goodix_usb.h"
#include "goodix_protocol.h"
#include "buffers.h"

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


int get_first_image(struct goodix_device *dev) {

   if (goodix_mcu_switch_to_fdt_mode(dev, COMMAND_MCU_SWITCH_TO_FDT_MODE)) return -1;

   if (goodix_mcu_switch_to_fdt_mode(dev, COMMAND_MCU_SWITCH_TO_FDT_MODE)) return -1;

   prebulktransfer(dev);
   if (goodix_send_command(dev, COMMAND_MCU_GET_IMAGE, MCU_GET_IMAGE_BUF, MCU_GET_IMAGE_BUF_LEN, SEED_MCU_GET_IMAGE, 1, 1, NULL)) return -1;
   
   return 0;
}

int goodix_mcu_switch_to_fdt_mode(struct goodix_device *dev, int cmd_id) {
   if (goodix_send_command(dev, cmd_id, MCU_SWITCH_TO_FDT_MODE_BUF, MCU_SWITCH_TO_FDT_MODE_BUF_LEN, SEED_MCU_SWITCH_TO_FDT_MODE, 1, 1, MCU_SWITCH_TO_FDT_MODE_HELP)) return -1;
   fdt_mode_construct_payload(MCU_SWITCH_TO_FDT_MODE_BUF + 2, MCU_SWITCH_TO_FDT_MODE_HELP + 11, 12);
   return 0;
}
