#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int tls_connect(const char *host, int port); 

int send_all(int sock, const uint8_t *data, size_t len); 

int recv_some(int sock, uint8_t *buf, size_t maxlen);

void close_connection(int sock); 
