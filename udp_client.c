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

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    /* Q5 提示：在 sendto() 前呼叫 getsockname() */
    struct sockaddr_in local_before;
    socklen_t len_before = sizeof(local_before);
    getsockname(sock_fd, (struct sockaddr *)&local_before, &len_before);

    char srv_str[32], before_str[32];
    format_addr(&srv_addr, srv_str, sizeof(srv_str));
    format_addr(&local_before, before_str, sizeof(before_str));

    printf("[UDP Client] Socket FD = %d | Target Peer = %s\n", sock_fd, srv_str);
    printf("[Q5 Check] Before sendto() -> Local = %s\n", before_str);
    printf("Type message and press Enter (Ctrl+D or 'quit' to exit):\n");

    int first_send = 1;
    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strcmp(line, "quit\n") == 0 || strcmp(line, "quit\r\n") == 0) {
            break;
        }

        sendto(sock_fd, line, strlen(line), 0,
               (struct sockaddr *)&srv_addr, sizeof(srv_addr));

        if (first_send) {
            struct sockaddr_in local_after;
            socklen_t len_after = sizeof(local_after);
            getsockname(sock_fd, (struct sockaddr *)&local_after, &len_after);

            char after_str[32];
            format_addr(&local_after, after_str, sizeof(after_str));
            printf("[Q5 Check] After sendto()  -> Local = %s\n", after_str);
            first_send = 0;
        }

        char buf[1024];
        struct sockaddr_in peer_addr;
        socklen_t peer_len = sizeof(peer_addr);
        ssize_t n = recvfrom(sock_fd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&peer_addr, &peer_len);
        if (n > 0) {
            buf[n] = '\0';
            char peer_str[32];
            format_addr(&peer_addr, peer_str, sizeof(peer_str));
            printf("[Echo from %s] %s", peer_str, buf);
            fflush(stdout);
        }
    }

    close(sock_fd);
    return 0;
}