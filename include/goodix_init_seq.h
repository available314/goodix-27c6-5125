#pragma once

#include "constants.h"

int windows_initialization(struct goodix_device *dev);
int goodix_mcu_switch_to_fdt_mode(struct goodix_device *dev, int cmd_id);
int get_first_image(struct goodix_device *dev);
