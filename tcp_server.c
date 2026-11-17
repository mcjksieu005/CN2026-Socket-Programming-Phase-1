#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
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

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, bind_ip, &srv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        close(listen_fd);
        return 1;
    }

    if (bind(listen_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    struct sockaddr_in actual_srv;
    socklen_t srv_len = sizeof(actual_srv);
    getsockname(listen_fd, (struct sockaddr *)&actual_srv, &srv_len);

    char srv_str[32];
    format_addr(&actual_srv, srv_str, sizeof(srv_str));
    printf("[TCP Server] Listening on %s | Listening FD = %d\n", srv_str, listen_fd);

    int clients[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; i++) {
        clients[i] = -1;
    }

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);
        int max_fd = listen_fd;

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clients[i] >= 0) {
                FD_SET(clients[i], &read_fds);
                if (clients[i] > max_fd) {
                    max_fd = clients[i];
                }
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        /* 1. 處理新連線 */
        if (FD_ISSET(listen_fd, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int conn_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
            if (conn_fd >= 0) {
                int added = 0;
                for (int i = 0; i < FD_SETSIZE; i++) {
                    if (clients[i] < 0) {
                        clients[i] = conn_fd;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    fprintf(stderr, "Too many clients\n");
                    close(conn_fd);
                } else {
                    struct sockaddr_in local_addr;
                    socklen_t local_len = sizeof(local_addr);
                    getsockname(conn_fd, (struct sockaddr *)&local_addr, &local_len);

                    char local_str[32], peer_str[32];
                    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
                    format_addr(&local_addr, local_str, sizeof(local_str));
                    format_addr(&cli_addr, peer_str, sizeof(peer_str));
                    inet_ntop(AF_INET, &cli_addr.sin_addr, src_ip, sizeof(src_ip));
                    inet_ntop(AF_INET, &local_addr.sin_addr, dst_ip, sizeof(dst_ip));

                    printf("[Connect] Listening FD=%d -> New Connected FD=%d | Local=%s | Peer=%s | Four-tuple=(%s, %d, %s, %d)\n",
                           listen_fd, conn_fd, local_str, peer_str,
                           src_ip, ntohs(cli_addr.sin_port),
                           dst_ip, ntohs(local_addr.sin_port));
                }
            }
        }

        /* 2. 處理既有連線的資料或斷線 */
        for (int i = 0; i < FD_SETSIZE; i++) {
            int cfd = clients[i];
            if (cfd >= 0 && FD_ISSET(cfd, &read_fds)) {
                char buf[1024];
                ssize_t n = recv(cfd, buf, sizeof(buf) - 1, 0);

                struct sockaddr_in cli_addr, local_addr;
                socklen_t cli_len = sizeof(cli_addr), local_len = sizeof(local_addr);
                getpeername(cfd, (struct sockaddr *)&cli_addr, &cli_len);
                getsockname(cfd, (struct sockaddr *)&local_addr, &local_len);

                char peer_str[32];
                format_addr(&cli_addr, peer_str, sizeof(peer_str));

                if (n <= 0) {
                    printf("[Disconnect] Connected FD=%d | Peer=%s closed connection.\n",
                           cfd, peer_str);
                    close(cfd);
                    clients[i] = -1;
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

                char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cli_addr.sin_addr, src_ip, sizeof(src_ip));
                inet_ntop(AF_INET, &local_addr.sin_addr, dst_ip, sizeof(dst_ip));

                printf("[Recv] Connected FD=%d | Four-tuple=(%s, %d, %s, %d) | Msg: \"%s\"\n",
                       cfd, src_ip, ntohs(cli_addr.sin_port),
                       dst_ip, ntohs(local_addr.sin_port), msg);

                send(cfd, buf, n, 0);
            }
        }
    }

    close(listen_fd);
    return 0;
}