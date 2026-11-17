#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void format_addr(const struct sockaddr_in *addr, char *buf, size_t len) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    snprintf(buf, len, "%s:%d", ip, ntohs(addr->sin_port));
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bind_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *bind_ip = argv[1];
    int port = atoi(argv[2]);

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, bind_ip, &srv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        close(sock_fd);
        return 1;
    }

    if (bind(sock_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return 1;
    }

    struct sockaddr_in local_addr;
    socklen_t local_len = sizeof(local_addr);
    getsockname(sock_fd, (struct sockaddr *)&local_addr, &local_len);

    char local_str[32];
    format_addr(&local_addr, local_str, sizeof(local_str));
    printf("[UDP Server] Bound on %s | Socket FD = %d\n", local_str, sock_fd);

    while (1) {
        char buf[1024];
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);

        ssize_t n = recvfrom(sock_fd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&cli_addr, &cli_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buf[n] = '\0';
        char msg[1024];
        strncpy(msg, buf, sizeof(msg));
        msg[sizeof(msg) - 1] = '\0';
        size_t len = strlen(msg);
        while (len > 0 && (msg[len - 1] == '\n' || msg[len - 1] == '\r')) {
            msg[--len] = '\0';
        }

        char cli_str[32];
        format_addr(&cli_addr, cli_str, sizeof(cli_str));
        printf("[UDP Recv] Server FD=%d | Local=%s <- Source(Peer)=%s | Msg: \"%s\"\n",
               sock_fd, local_str, cli_str, msg);

        sendto(sock_fd, buf, n, 0, (struct sockaddr *)&cli_addr, cli_len);
    }

    close(sock_fd);
    return 0;
}