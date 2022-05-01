#include <signal.h>
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
#include <netinet/icmp6.h>
#include <netinet/in.h>

#define BUFSIZE 1500

char *buf[BUFSIZE], ntopbuf[INET6_ADDRSTRLEN];
struct icmp6_hdr *icmp6hdr = (struct icmp6_hdr *)buf;
struct nd_neighbor_solicit *ns = (struct nd_neighbor_solicit *)buf;
struct nd_neighbor_advert *na = (struct nd_neighbor_advert *)buf;

int sockfd, id, seq, length = 64, arping = 0;

struct sockaddr_in6 dst;
struct in6_addr tgt;

void send_echoreq() {
  memset(buf, 0, sizeof(buf));
  icmp6hdr->icmp6_type = ICMP6_ECHO_REQUEST;
  icmp6hdr->icmp6_id = id;
  icmp6hdr->icmp6_seq = seq++;
  if (gettimeofday((struct timeval *)(icmp6hdr + 1), NULL) < 0) {
    perror("gettimeofday failed");
    exit(-1);
  }
  if (length)
    memset(buf + sizeof(struct icmp6_hdr) + sizeof(struct timeval), 0, length);
  if (sendto(sockfd, buf,
             sizeof(struct icmp6_hdr) + sizeof(struct timeval) + length, 0,
             (struct sockaddr *)&dst, sizeof(dst)) < 0) {
    perror("sendto failed");
    exit(-1);
  }
}

void recv_echorep() {
  int nread;
  double rtt;
  struct timeval tv;
  struct sockaddr_in6 src;
  socklen_t socklen;

  socklen = sizeof(src);
  if ((nread = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&src,
                        &socklen)) < 0) {
    perror("recvfrom failed");
    exit(-1);
  }
  if (nread != sizeof(struct icmp6_hdr) + sizeof(struct timeval) + length ||
      icmp6hdr->icmp6_id != id)
    return;
  if (!inet_ntop(AF_INET6, &src.sin6_addr, ntopbuf, sizeof(ntopbuf))) {
    perror("inet_ntop failed");
    exit(-1);
  }
  if (gettimeofday(&tv, NULL) < 0) {
    perror("gettimeofday failed");
    exit(-1);
  }
  rtt = (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0 -
        (((struct timeval *)(icmp6hdr + 1))->tv_sec) * 1000.0 -
        (((struct timeval *)(icmp6hdr + 1))->tv_usec) / 1000.0;
  printf("recvfrom %s, %d bytes, seq: %d, rtt: %lfms\n", ntopbuf,
         (int)sizeof(struct timeval) + length, icmp6hdr->icmp6_seq, rtt);
}

void send_ns() {
  memset(buf, 0, sizeof(buf));
  ns->nd_ns_type = ND_NEIGHBOR_SOLICIT;
  ns->nd_ns_target = tgt;
  if (sendto(sockfd, buf, sizeof(struct nd_neighbor_solicit), 0,
             (struct sockaddr *)&dst, sizeof(dst)) < 0) {
    perror("sendto failed");
    exit(-1);
  }
}

void recv_na() {
  int nread;
  struct sockaddr_in6 src;
  socklen_t socklen;

  socklen = sizeof(src);
  if ((nread = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&src,
                        &socklen)) < 0) {
    perror("recvfrom failed");
    exit(-1);
  }
  if (nread < sizeof(struct nd_neighbor_advert) ||
      !IN6_ARE_ADDR_EQUAL(&tgt, &na->nd_na_target))
    return;
  if (!inet_ntop(AF_INET6, &src.sin6_addr, ntopbuf, sizeof(ntopbuf))) {
    perror("inet_ntop failed");
    exit(-1);
  }
  printf("recvfrom %s\n", ntopbuf);
}

void sig_alrm(int signo) {
  if (arping)
    send_ns();
  else
    send_echoreq();
  alarm(1);
}

int main(int argc, char **argv) {
  int ret;
  struct addrinfo hints, *rai;
  struct icmp6_filter filter;
  struct ifreq ifr;
  const char *interface = NULL;

  while ((ret = getopt(argc, argv, "hi:l:a")) > 0) {
    switch (ret) {
    case 'h':
      printf("usage: %s [-h] [-l LENGTH] [-a] ADDRESS\n", argv[0]);
      exit(1);
      break;
    case 'i':
      interface = optarg;
      break;
    case 'l':
      length = atoi(optarg);
      break;
    case 'a':
      arping = 1;
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
  hints.ai_family = AF_INET6;
  if ((ret = getaddrinfo(argv[optind], NULL, &hints, &rai))) {
    fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret));
    exit(-1);
  }

  memset(&dst, 0, sizeof(dst));
  dst.sin6_family = AF_INET6;
  dst.sin6_addr = ((struct sockaddr_in6 *)rai->ai_addr)->sin6_addr;

  if (arping) {
    static const char *soladdr =
        "\xff\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xff";
    tgt = dst.sin6_addr;
    memcpy(&dst.sin6_addr, soladdr, 13);
  }

  if ((sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0) {
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

  if (arping) {
    /* TODO: set socket hoplimit to 255 */
  }

  ICMP6_FILTER_SETBLOCKALL(&filter);
  if (arping)
    ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filter);
  else
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &filter);
  if (setsockopt(sockfd, IPPROTO_ICMPV6, ICMP6_FILTER, &filter,
                 sizeof(filter)) < 0) {
    perror("setsockopt failed");
    exit(-1);
  }

  signal(SIGALRM, sig_alrm);
  alarm(1);
  for (;;)
    if (arping)
      recv_na();
    else
      recv_echorep();
}
