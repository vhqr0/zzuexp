#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUF_SIZE 4096

#define ADDRSTRLEN                                                             \
  (INET_ADDRSTRLEN < INET6_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN)

int main(int argc, char **argv) {
  uint16_t port;
  struct sockaddr *laddr = NULL, *raddr = NULL;
  struct sockaddr_in laddr4, raddr4;
  struct sockaddr_in6 laddr6, raddr6;
  void *raddr_sin_addr;
  socklen_t len, len4 = sizeof(struct sockaddr_in),
                 len6 = sizeof(struct sockaddr_in6);
  int domain, sock;
  ssize_t n;
  char buf[BUF_SIZE], inetpbuf[ADDRSTRLEN];

  if (argc != 4) {
    fprintf(stderr, "usage: echoc IP PORT MSG\n");
    return -1;
  }
  port = htons(atoi(argv[2]));
  memset(&laddr4, 0, len4);
  memset(&raddr4, 0, len4);
  memset(&laddr6, 0, len6);
  memset(&raddr6, 0, len6);
  laddr4.sin_family = AF_INET;
  raddr4.sin_family = AF_INET;
  laddr6.sin6_family = AF_INET6;
  raddr6.sin6_family = AF_INET6;
  raddr4.sin_port = port;
  raddr6.sin6_port = port;
  laddr4.sin_addr.s_addr = INADDR_ANY;
  inet_pton(AF_INET6, "", &laddr6.sin6_addr);
  if (inet_pton(AF_INET, argv[1], &raddr4.sin_addr) == 1) {
    laddr = (struct sockaddr *)&laddr4;
    raddr = (struct sockaddr *)&raddr4;
    len = len4;
    domain = AF_INET;
    raddr_sin_addr = &raddr4.sin_addr;
    puts("use ipv4");
  } else if (inet_pton(AF_INET6, argv[1], &raddr6.sin6_addr) == 1) {
    laddr = (struct sockaddr *)&laddr6;
    raddr = (struct sockaddr *)&raddr6;
    len = len6;
    domain = AF_INET6;
    raddr_sin_addr = &raddr6.sin6_addr;
    puts("use ipv6");
  } else {
    fprintf(stderr, "unrecognized address: %s\n", argv[1]);
    return -1;
  }
  if ((sock = socket(domain, SOCK_DGRAM, 0)) == -1) {
    perror("socket failed");
    return -1;
  }
  if (bind(sock, laddr, len) == -1) {
    perror("bind failed");
    return -1;
  }
  if (sendto(sock, argv[3], strlen(argv[3]), 0, raddr, len) == -1) {
    perror("sendto failed");
    return -1;
  }
  if ((n = recvfrom(sock, buf, BUF_SIZE - 1, 0, raddr, &len)) == -1) {
    perror("recvfrom failed");
    return -1;
  }
  buf[n] = 0;
  if (!inet_ntop(domain, raddr_sin_addr, inetpbuf, ADDRSTRLEN)) {
    perror("inet_ntop failed");
    return -1;
  }
  printf("receive from %s: %s\n", inetpbuf, buf);
}
