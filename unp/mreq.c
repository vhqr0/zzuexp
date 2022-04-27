#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFSIZE 4096
#define ADDRSTRLEN                                                             \
  INET_ADDRSTRLEN > INET6_ADDRSTRLEN ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN

char buf[BUFSIZE], ntopbuf[ADDRSTRLEN];

int main(int argc, char **argv) {
  int sockfd, domain, brd = 0, timeout = 0, on = 1, nread, ret;
  short port = 13;
  const char *address = NULL;
  union {
    struct sockaddr_in a4;
    struct sockaddr_in6 a6;
  } addr;
  socklen_t socklen;
  time_t ticks;

  while ((ret = getopt(argc, argv, "h4:6:p:bt:")) > 0) {
    switch (ret) {
    case 'h':
      printf("usage: %s [-h] [-46 [ADDRESS]] [-p PORT] [-b] [t TIMEOUT]\n",
             argv[0]);
      exit(1);
      break;
    case '4':
      domain = AF_INET;
      address = optarg;
      break;
    case '6':
      domain = AF_INET6;
      address = optarg;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'b':
      brd = 1;
      break;
    case 't':
      timeout = atoi(optarg);
      break;
    default:
      fprintf(stderr, "unknown option: %c\n", ret);
      exit(-1);
    }
  }

  if (!address) {
    fprintf(stderr, "you must specify an address\n");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  switch (domain) {
  case AF_INET:
    addr.a4.sin_family = AF_INET;
    addr.a4.sin_port = htons(port);
    socklen = sizeof(struct sockaddr_in);
    ret = inet_pton(AF_INET, address, &addr.a4.sin_addr);
    break;
  case AF_INET6:
    addr.a6.sin6_family = AF_INET6;
    addr.a6.sin6_port = htons(port);
    socklen = sizeof(struct sockaddr_in6);
    ret = inet_pton(AF_INET6, address, &addr.a6.sin6_addr);
    break;
  }
  if (ret < 0) {
    perror("inet_pton failed");
    exit(-1);
  }
  if (!ret) {
    fprintf(stderr, "inet_pton invalid data\n");
    exit(-1);
  }

  if ((sockfd = socket(domain, SOCK_DGRAM, 0)) < 0) {
    perror("socket failed");
    exit(-1);
  }

  if (brd && setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
    perror("setsockopt SO_BROADCAST failed");
    exit(-1);
  }

  if (sendto(sockfd, "", 1, 0, (struct sockaddr *)&addr, socklen) < 0) {
    perror("sendto failed");
    exit(-1);
  }

  if (timeout > 0)
    alarm(timeout);

  for (;;) {
    if ((nread = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addr,
                          &socklen)) < 0) {
      if (errno == EINTR)
        exit(0);
      perror("recvfrom failed");
      exit(-1);
    }
    switch (domain) {
    case AF_INET:
      if (!inet_ntop(AF_INET, &addr.a4.sin_addr, ntopbuf, sizeof(ntopbuf))) {
        perror("inet_ntop failed");
        exit(-1);
      }
      break;
    case AF_INET6:
      if (!inet_ntop(AF_INET6, &addr.a6.sin6_addr, ntopbuf, sizeof(ntopbuf))) {
        perror("inet_ntop failed");
        exit(-1);
      }
      break;
    }
    if (!nread)
      continue;
    printf("recvfrom %s: ", ntopbuf);
    fflush(stdout);
    if (write(STDOUT_FILENO, buf, nread) < 0) {
      perror("write failed");
      exit(-1);
    }
  }
}
