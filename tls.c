#include "tls.h"

int tls_connect(const char *host, int port) {
    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket()");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, host, &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect()");
        close(sock);
        return -1;
    }

    return sock;
}

int send_all(int sock, const uint8_t *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = send(sock, data + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

int recv_some(int sock, uint8_t *buf, size_t maxlen) {
    int n = recv(sock, buf, maxlen, 0);
    if (n <= 0) return -1;
    return n;
}

void close_connection(int sock) {
   close(sock);
}
