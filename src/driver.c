#include <stdio.h>
#include "constants.h"
#include "goodix_init_seq.h"
#include "goodix_usb.h"
#include "goodix_tls.h"


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
