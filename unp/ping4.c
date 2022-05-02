#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define BUFSIZE 1500

char buf[BUFSIZE], cbuf[BUFSIZE], ntopbuf[INET_ADDRSTRLEN];
struct icmphdr *icmphdr = (struct icmphdr *)buf;

int sockfd, id, seq, length = 64;

struct sockaddr_in dst;

uint16_t cksum(uint16_t *addr, int len) {
  int nleft = len;
  uint32_t sum = 0;
  uint16_t *w = addr;
  uint16_t answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft) {
    *(unsigned char *)(&answer) = *(unsigned char *)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

void send_echoreq() {
  memset(buf, 0, sizeof(buf));
  icmphdr->type = ICMP_ECHO;
  icmphdr->un.echo.id = id;
  icmphdr->un.echo.sequence = seq++;
  if (gettimeofday((struct timeval *)(icmphdr + 1), NULL) < 0) {
    perror("gettimeofday failed");
    exit(-1);
  }
  if (length)
    memset(buf + sizeof(struct icmphdr) + sizeof(struct timeval), 0xa5, length);
  icmphdr->checksum =
      cksum((uint16_t *)buf,
            sizeof(struct icmphdr) + sizeof(struct timeval) + length);
  if (sendto(sockfd, buf,
             sizeof(struct icmphdr) + sizeof(struct timeval) + length, 0,
             (struct sockaddr *)&dst, sizeof(dst)) < 0) {
    perror("sendto failed");
    exit(-1);
  }
}

void recv_echorep() {
  int nread, hlen, ttl;
  double rtt;
  struct timeval tv;
  struct sockaddr_in src;
  socklen_t socklen;
  struct iphdr *iphdr = (struct iphdr *)buf;
  struct icmphdr *icmphdr;

  socklen = sizeof(src);
  if ((nread = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&src,
                        &socklen)) < 0) {
    perror("recvfrom failed");
  }
  hlen = iphdr->ihl << 2;
  icmphdr = (struct icmphdr *)(buf + hlen);
  if (nread !=
          hlen + sizeof(struct icmphdr) + sizeof(struct timeval) + length ||
      icmphdr->type != ICMP_ECHOREPLY || icmphdr->un.echo.id != id)
    return;
  if (!inet_ntop(AF_INET, &src.sin_addr, ntopbuf, sizeof(ntopbuf))) {
    perror("inet_ntop failed");
    exit(-1);
  }
  ttl = iphdr->ttl;
  if (gettimeofday(&tv, NULL) < 0) {
    perror("gettimeofday failed");
    exit(-1);
  }
  rtt = (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0 -
        ((struct timeval *)(icmphdr + 1))->tv_sec * 1000.0 -
        ((struct timeval *)(icmphdr + 1))->tv_usec / 1000.0;
  printf("recvfrom %s, %d bytes, seq: %d, rtt: %lfms, ttl: %d\n", ntopbuf,
         (int)sizeof(struct timeval) + length, icmphdr->un.echo.sequence, rtt,
         ttl);
}

void sig_alrm(int signo) {
  send_echoreq();
  alarm(1);
}

int main(int argc, char **argv) {
  int ret;
  struct addrinfo hints, *rai;
  struct ifreq ifr;
  const char *interface = NULL;

  while ((ret = getopt(argc, argv, "hi:lI")) > 0) {
    switch (ret) {
    case 'h':
      printf("usage: %s [-h] [-i interface] [-l length] ADDRESS\n", argv[0]);
      exit(1);
      break;
    case 'i':
      interface = optarg;
      break;
    case 'l':
      length = atoi(optarg);
      break;
    default:
      fprintf(stderr, "unknown options\n");
      exit(-1);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "you have to specify an address\n");
    exit(-1);
  }

  if (length >= BUFSIZE) {
    fprintf(stderr, "length too long\n");
    exit(-1);
  }
  length -= sizeof(struct timeval);
  if (length < 0)
    length = 0;

  id = getpid() & 0xffff;
  seq = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  if ((ret = getaddrinfo(argv[optind], NULL, &hints, &rai))) {
    fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret));
    exit(-1);
  }

  memset(&dst, 0, sizeof(dst));
  dst.sin_family = AF_INET;
  dst.sin_addr = ((struct sockaddr_in *)rai->ai_addr)->sin_addr;

  if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("socket failed");
    exit(-1);
  }

  if (interface) {
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) <
        0) {
      perror("setsockopt SO_BINDTODEVICE failed");
      exit(-1);
    }
  }

  signal(SIGALRM, sig_alrm);
  alarm(1);
  for (;;)
    recv_echorep();
}
