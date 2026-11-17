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
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    const char *srv_ip = argv[1];
    int srv_port = atoi(argv[2]);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(srv_port);
    if (inet_pton(AF_INET, srv_ip, &srv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        close(sock_fd);
        return 1;
    }

    if (connect(sock_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    struct sockaddr_in local_addr, peer_addr;
    socklen_t local_len = sizeof(local_addr), peer_len = sizeof(peer_addr);
    getsockname(sock_fd, (struct sockaddr *)&local_addr, &local_len);
    getpeername(sock_fd, (struct sockaddr *)&peer_addr, &peer_len);

    char local_str[32], peer_str[32];
    format_addr(&local_addr, local_str, sizeof(local_str));
    format_addr(&peer_addr, peer_str, sizeof(peer_str));

    printf("[TCP Client] Connected! Socket FD = %d | Local = %s | Peer = %s\n",
           sock_fd, local_str, peer_str);
    printf("Type message and press Enter (Ctrl+D or 'quit' to exit):\n");

    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strcmp(line, "quit\n") == 0 || strcmp(line, "quit\r\n") == 0) {
            break;
        }
        send(sock_fd, line, strlen(line), 0);

        char buf[1024];
        ssize_t n = recv(sock_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("[TCP Client] Server disconnected.\n");
            break;
        }
        buf[n] = '\0';
        printf("[Echo from %s] %s", peer_str, buf);
        fflush(stdout);
    }

    close(sock_fd);
    printf("[TCP Client] Closed FD=%d\n", sock_fd);
    return 0;
}