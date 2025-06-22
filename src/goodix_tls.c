#include "goodix_tls.h"

#include "tls.h"
#include "goodix_protocol.h"
#include "goodix_usb.h"
#include "buffers.h"

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

