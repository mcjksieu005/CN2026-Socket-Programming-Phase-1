#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in make_addr(const char *ip, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return addr;
}

void format_addr(const struct sockaddr_in *addr, char *buf, size_t len) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    snprintf(buf, len, "%s:%d", ip, ntohs(addr->sin_port));
}

int main(void) {
    /* 3A. bind(port=0): display assigned port */
    printf("=== Task 3A: bind(port=0) ===\n");
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = make_addr("127.0.0.1", 0);
        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            struct sockaddr_in assigned;
            socklen_t len = sizeof(assigned);
            getsockname(fd, (struct sockaddr *)&assigned, &len);

            char assigned_str[32];
            format_addr(&assigned, assigned_str, sizeof(assigned_str));
            printf("FD=%d requested 127.0.0.1:0 -> OS assigned: %s\n", fd, assigned_str);
        } else {
            printf("bind failed: %s\n", strerror(errno));
        }
        close(fd);
    }

    /* 3B. Same local IP and port, two TCP sockets (no SO_REUSEADDR) */
    printf("\n=== Task 3B: Two TCP sockets on same IP:Port ===\n");
    {
        int tcp1 = socket(AF_INET, SOCK_STREAM, 0);
        int tcp2 = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = make_addr("127.0.0.1", 9000);

        int r1 = bind(tcp1, (struct sockaddr *)&addr, sizeof(addr));
        if (r1 == 0) {
            printf("TCP Socket 1 (FD=%d) bind(127.0.0.1:9000) -> SUCCESS\n", tcp1);
        } else {
            printf("TCP Socket 1 (FD=%d) bind(127.0.0.1:9000) -> FAILED: %s\n", tcp1, strerror(errno));
        }

        int r2 = bind(tcp2, (struct sockaddr *)&addr, sizeof(addr));
        if (r2 == 0) {
            printf("TCP Socket 2 (FD=%d) bind(127.0.0.1:9000) -> SUCCESS\n", tcp2);
        } else {
            printf("TCP Socket 2 (FD=%d) bind(127.0.0.1:9000) -> FAILED (errno=%d): %s\n",
                   tcp2, errno, strerror(errno));
        }

        close(tcp1);
        close(tcp2);
    }

    /* 3C. TCP and UDP bound to the same local IP and port */
    printf("\n=== Task 3C: TCP & UDP on same IP:Port ===\n");
    {
        int tcp_s = socket(AF_INET, SOCK_STREAM, 0);
        int udp_s = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr = make_addr("127.0.0.1", 9001);

        int r1 = bind(tcp_s, (struct sockaddr *)&addr, sizeof(addr));
        printf("TCP Socket (FD=%d) bind(127.0.0.1:9001) -> %s\n",
               tcp_s, r1 == 0 ? "SUCCESS" : strerror(errno));

        int r2 = bind(udp_s, (struct sockaddr *)&addr, sizeof(addr));
        printf("UDP Socket (FD=%d) bind(127.0.0.1:9001) -> %s\n",
               udp_s, r2 == 0 ? "SUCCESS" : strerror(errno));

        close(tcp_s);
        close(udp_s);
    }

    /* 3D. Two TCP sockets, different local IPs but same port */
    printf("\n=== Task 3D: Two TCP sockets on different local IPs, same Port ===\n");
    {
        int tcp_a = socket(AF_INET, SOCK_STREAM, 0);
        int tcp_b = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr_a = make_addr("127.0.0.1", 9002);
        struct sockaddr_in addr_b = make_addr("127.0.0.2", 9002);

        int r1 = bind(tcp_a, (struct sockaddr *)&addr_a, sizeof(addr_a));
        printf("TCP Socket A (FD=%d) bind(127.0.0.1:9002) -> %s\n",
               tcp_a, r1 == 0 ? "SUCCESS" : strerror(errno));

        int r2 = bind(tcp_b, (struct sockaddr *)&addr_b, sizeof(addr_b));
        printf("TCP Socket B (FD=%d) bind(127.0.0.2:9002) -> %s\n",
               tcp_b, r2 == 0 ? "SUCCESS" : strerror(errno));

        close(tcp_a);
        close(tcp_b);
    }

    return 0;
}