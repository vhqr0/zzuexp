#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFSIZE 4096

char buf[BUFSIZE], ntopbuf[INET_ADDRSTRLEN];

int main(int argc, char **argv) {
  int sockfd, timeout = 1, nread, opt;
  short port = 13;
  const char *address = "255.255.255.255";
  struct sockaddr_in addr;
  union {
    int intval;
  } val;
  socklen_t socklen;

  while ((opt = getopt(argc, argv, "ht:p:a:")) > 0) {
    switch (opt) {
    case 'h':
      printf("usage: %s [-h] [-t TIMEOUT] [-p PORT] [-a ADDRESS]\n", argv[0]);
      exit(1);
      break;
    case 't':
      timeout = atoi(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'a':
      address = optarg;
      break;
    }
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  switch (inet_pton(AF_INET, address, &addr.sin_addr)) {
  case -1:
    perror("inet_pton failed");
    exit(-1);
    break;
  case 0:
    fprintf(stderr, "inet_pton invalid data\n");
    exit(-1);
    break;
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket failed");
    exit(-1);
  }

  memset(&val, 0, sizeof(val));
  val.intval = 1;
  socklen = sizeof(int);
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &val, socklen) < 0) {
    perror("setsockopt failed");
    exit(-1);
  }

  socklen = sizeof(addr);
  if (sendto(sockfd, "", 1, 0, (struct sockaddr *)&addr, socklen) < 0) {
    perror("sendto failed");
    exit(-1);
  }

  alarm(timeout);

  for (;;) {
    if ((nread = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &socklen)) < 0) {
      if (errno == EINTR)
        exit(0);
      perror("recvfrom failed");
      exit(-1);
    }
    if (!nread)
      continue;
    if (!inet_ntop(AF_INET, &addr.sin_addr, ntopbuf, sizeof(ntopbuf))) {
      perror("inet_ntop failed");
      exit(-1);
    }
    printf("recv from %s: ", ntopbuf);
    fflush(stdout);
    if (write(STDOUT_FILENO, buf, nread) < 0) {
      perror("write failed");
      exit(-1);
    }
  }
}
