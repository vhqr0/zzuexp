#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>

#define BUFSIZE 4096
#define ADDRSTRLEN                                                             \
  INET_ADDRSTRLEN > INET6_ADDRSTRLEN ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN

extern struct sockaddr_un esun;
#define UNPATHLEN sizeof(esun.sun_path)

char buf[BUFSIZE], ntopbuf[ADDRSTRLEN];

void Perror(const char *msg) {
  perror(msg);
  exit(-1);
}

void Pmsg(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(-1);
}

short Ntop(void *addr) {
  int domain;
  void *src;
  short port;
  domain = ((struct sockaddr *)addr)->sa_family;
  switch (domain) {
  case AF_INET:
    src = &((struct sockaddr_in *)addr)->sin_addr;
    port = ((struct sockaddr_in *)addr)->sin_port;
    break;
  case AF_INET6:
    src = &((struct sockaddr_in6 *)addr)->sin6_addr;
    port = ((struct sockaddr_in6 *)addr)->sin6_port;
    break;
  }
  if (!inet_ntop(domain, src, ntopbuf, sizeof(ntopbuf)))
    Perror("inet_ntop faied");
  return port;
}

int Pton(const char *address, void *addr) {
  int domain, ret;
  void *dst = NULL;
  domain = ((struct sockaddr *)addr)->sa_family;
  switch (domain) {
  case AF_INET:
    dst = &((struct sockaddr_in *)addr)->sin_addr;
    break;
  case AF_INET6:
    dst = &((struct sockaddr_in6 *)addr)->sin6_addr;
    break;
  }
  if ((ret = inet_pton(domain, address, dst)) < 0)
    Perror("inet_pton failed");
  return ret;
}

void Addrinfo(const char *address, void *addr) {
  int domain, ret;
  void *src = NULL, *dst = NULL;
  struct addrinfo hints, *rai;
  domain = ((struct sockaddr *)addr)->sa_family;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = domain;
  if ((ret = getaddrinfo(address, NULL, &hints, &rai))) {
    fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret));
    exit(-1);
  }
  switch (domain) {
  case AF_INET:
    dst = &((struct sockaddr_in *)addr)->sin_addr;
    src = &((struct sockaddr_in *)rai->ai_addr)->sin_addr;
    ret = sizeof(struct in_addr);
    break;
  case AF_INET6:
    dst = &((struct sockaddr_in6 *)addr)->sin6_addr;
    src = &((struct sockaddr_in6 *)rai->ai_addr)->sin6_addr;
    ret = sizeof(struct in6_addr);
    break;
  }
  memcpy(dst, src, ret);
  freeaddrinfo(rai);
}

int Bcastcheck(int fd, void *addr) {
  int on = 1, ret = 0;
  if (((struct sockaddr *)addr)->sa_family == AF_INET &&
      ((struct sockaddr_in *)addr)->sin_addr.s_addr ==
          htonl(INADDR_BROADCAST)) {
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
      Perror("setsockopt SO_BROADCAST failed");
    ret = 1;
  }
  return ret;
}

int Mcastcheck(int fd, void *addr, const char *interface, int checkonly) {
  int ret;
  struct ifreq ifr;
  struct ip_mreq imr4;
  struct ipv6_mreq imr6;
  switch (((struct sockaddr *)addr)->sa_family) {
  case AF_INET:
    if (!IN_MULTICAST(((struct sockaddr_in *)addr)->sin_addr.s_addr))
      return 0;
    if (checkonly)
      return 1;
    memset(&imr4, 0, sizeof(imr4));
    memcpy(&imr4.imr_multiaddr, &((struct sockaddr_in *)addr)->sin_addr,
           sizeof(struct in_addr));
    if (interface) {
      memset(&ifr, 0, sizeof(ifr));
      strncpy(ifr.ifr_name, interface, IFNAMSIZ);
      if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
        Perror("ioctl SIOCGIFADDR failed");
      memcpy(&imr4.imr_interface,
             &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr,
             sizeof(struct in_addr));
    }
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr4, sizeof(imr4)) < 0)
      Perror("setsockopt IP_ADD_MEMBERSHIP failed");
    memset(&((struct sockaddr_in *)addr)->sin_addr, 0, sizeof(struct in_addr));
    break;
  case AF_INET6:
    if (!IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)addr)->sin6_addr))
      return 0;
    if (checkonly)
      return 1;
    memset(&imr6, 0, sizeof(imr6));
    memcpy(&imr6.ipv6mr_multiaddr, &((struct sockaddr_in6 *)addr)->sin6_addr,
           sizeof(struct in6_addr));
    if (interface) {
      memset(&ifr, 0, sizeof(ifr));
      strncpy(ifr.ifr_name, interface, IFNAMSIZ);
      if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
        Perror("ioctl SIOCGIFINDEX failed");
      imr6.ipv6mr_interface = ifr.ifr_ifindex;
    }
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &imr6, sizeof(imr6)) < 0)
      Perror("setsockopt IPV6_JOIN_GROUP failed");
    memset(&((struct sockaddr_in6 *)addr)->sin6_addr, 0,
           sizeof(struct sockaddr_in6));
    break;
  default:
    return 0;
  }
  return 1;
}

int Read(int fd, char *b, int n) {
  int ret;
doread:
  if ((ret = read(fd, b, n)) < 0) {
    if (errno == EINTR)
      goto doread;
    Perror("read failed");
  }
  return ret;
}

int Write(int fd, char *b, int n) {
  int ret;
dowrite:
  if ((ret = write(fd, b, n)) < 0) {
    if (errno == EINTR)
      goto dowrite;
    Perror("write failed");
  }
  return ret;
}

int Recvfrom(int fd, void *b, int n, void *addr, socklen_t *socklen) {
  int ret;
dorecv:
  if ((ret = recvfrom(fd, b, n, 0, (struct sockaddr *)addr, socklen)) < 0) {
    if (errno == EINTR)
      goto dorecv;
    Perror("recvfrom failed");
  }
  return ret;
}

int Sendto(int fd, void *b, int n, void *addr, socklen_t socklen) {
  int ret;
dosend:
  if ((ret = sendto(fd, b, n, 0, (struct sockaddr *)addr, socklen)) < 0) {
    if (errno == EINTR)
      goto dosend;
    Perror("sendto failed");
  }
  return ret;
}

void Writen(int fd, char *b, int n) {
  int nread;
  while (n > 0) {
    nread = Write(fd, b, n);
    if (!nread)
      Pmsg("write unexpected EOF");
    b += nread;
    n -= nread;
  }
}

void c_echo(int sockfd, int type, int udp_sendto, struct sockaddr *servaddr,
            socklen_t socklen) {
  int nread;
  fd_set rfds;

  for (;;) {
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(sockfd, &rfds);
    if (select(sockfd + 1, &rfds, NULL, NULL, NULL) < 0) {
      if (errno == EINTR)
        continue;
      Perror("select failed");
    }
    if (FD_ISSET(sockfd, &rfds)) {
      nread = Read(sockfd, buf, sizeof(buf));
      if (!nread && type == SOCK_STREAM)
        Pmsg("server terminated prematurely");
      if (nread)
        Writen(STDOUT_FILENO, buf, nread);
    }
    if (FD_ISSET(STDIN_FILENO, &rfds)) {
      nread = Read(STDIN_FILENO, buf, sizeof(buf));
      if (!nread)
        return;
      switch (type) {
      case SOCK_STREAM:
        Writen(sockfd, buf, nread);
        break;
      case SOCK_DGRAM:
        if (udp_sendto)
          Sendto(sockfd, buf, nread, servaddr, socklen);
        else
          Write(sockfd, buf, nread);
        break;
      }
    }
  }
}

void s_echo(int sockfd, int type, struct sockaddr *cliaddr, socklen_t socklen,
            int n) {
  int nread;

  switch (type) {
  case SOCK_STREAM:
    for (;;) {
      nread = Read(sockfd, buf, sizeof(buf));
      if (!nread)
        break;
      Writen(sockfd, buf, nread);
    }
    break;
  case SOCK_DGRAM:
    Sendto(sockfd, buf, n, cliaddr, socklen);
    break;
  }
}

void c_daytime(int sockfd, int type, int udp_sendto, struct sockaddr *servaddr,
               socklen_t socklen) {
  int nread;

  switch (type) {
  case SOCK_STREAM:
    for (;;) {
      nread = Read(sockfd, buf, sizeof(buf));
      if (!nread)
        break;
      Writen(STDOUT_FILENO, buf, nread);
    }
    break;
  case SOCK_DGRAM:
    if (udp_sendto)
      Sendto(sockfd, "", 1, servaddr, socklen);
    else
      Write(sockfd, "", 1);
    for (;;) {
      nread = Read(sockfd, buf, sizeof(buf));
      if (nread)
        Writen(STDOUT_FILENO, buf, nread);
    }
    break;
  }
}

void s_daytime(int sockfd, int type, struct sockaddr *cliaddr,
               socklen_t socklen, int n) {
  int nread;
  time_t ticks;

  ticks = time(NULL);
  if ((nread = snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks))) < 0)
    Perror("snprintf failed");
  switch (type) {
  case SOCK_STREAM:
    Writen(sockfd, buf, nread);
    break;
  case SOCK_DGRAM:
    Sendto(sockfd, buf, nread, cliaddr, socklen);
    break;
  }
}

void sig_chld(int signo) {
  int pid, stat;

  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    printf("child %d terminated\n", pid);
}

int main(int argc, char **argv) {
  int sockfd, clifd, domain = AF_UNIX, type = SOCK_STREAM, udp_sendto = 0,
                     backlog = 5, nread, ret;
  short port = 0;
  const char *address = NULL, *interface = NULL;
  union {
    struct sockaddr_in a4;
    struct sockaddr_in6 a6;
    struct sockaddr_un un;
  } servaddr, cliaddr;
  socklen_t socklen;
  enum { c_mode, s_mode } mode = c_mode;
  enum { e_serv, d_serv } serv = d_serv;
  void (*cf)(int, int, int, struct sockaddr *, socklen_t);
  void (*sf)(int, int, struct sockaddr *, socklen_t, int);

  while ((ret = getopt(argc, argv, "hTU::4::6::u::p:i:b:csed")) > 0) {
    switch (ret) {
    case 'h':
      printf("usage: %s [-h] [-TU[S]] [-46u[ADDRESS]] [-p PORT] "
             "[-i [INTERFACE]] [-b BACKLOG] [-cs] [-ed]\n",
             argv[0]);
      exit(1);
      break;
    case 'T':
      type = SOCK_STREAM;
      break;
    case 'U':
      type = SOCK_DGRAM;
      udp_sendto = !!optarg;
      break;
    case '4':
      domain = AF_INET;
      address = optarg;
      break;
    case '6':
      domain = AF_INET6;
      address = optarg;
      break;
    case 'u':
      domain = AF_UNIX;
      address = optarg;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'i':
      interface = optarg;
      break;
    case 'b':
      backlog = atoi(optarg);
      break;
    case 'c':
      mode = c_mode;
      break;
    case 's':
      mode = s_mode;
      break;
    case 'e':
      serv = e_serv;
      break;
    case 'd':
      serv = d_serv;
      break;
    default:
      Pmsg("unknown option");
    }
  }

  switch (serv) {
  case e_serv:
    cf = c_echo;
    sf = s_echo;
    if (port <= 0)
      port = 7;
    break;
  case d_serv:
    cf = c_daytime;
    sf = s_daytime;
    if (port <= 0)
      port = 13;
    break;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  switch (domain) {
  case AF_INET:
    servaddr.a4.sin_family = AF_INET;
    servaddr.a4.sin_port = htons(port);
    socklen = sizeof(struct sockaddr_in);
    break;
  case AF_INET6:
    servaddr.a6.sin6_family = AF_INET6;
    servaddr.a6.sin6_port = htons(port);
    socklen = sizeof(struct sockaddr_in6);
    break;
  case AF_UNIX:
    servaddr.un.sun_family = AF_UNIX;
    if (!address)
      address = "/tmp/serv.sock";
    strncpy(servaddr.un.sun_path, address, UNPATHLEN);
    socklen = sizeof(struct sockaddr_un);
    break;
  }
  if (address && (domain == AF_INET || domain == AF_INET6) &&
      !Pton(address, &servaddr))
    Addrinfo(address, &servaddr);

  if ((sockfd = socket(domain, type, 0)) < 0)
    Perror("socket failed");

  switch (mode) {

  case c_mode:

    if (domain == AF_UNIX && type == SOCK_DGRAM) {
      memset(&cliaddr, 0, sizeof(cliaddr));
      cliaddr.un.sun_family = AF_UNIX;
      if (snprintf(cliaddr.un.sun_path, UNPATHLEN, "/tmp/serv%d.sock",
                   getpid()) < 0)
        Perror("snprintf failed");
      if (bind(sockfd, (struct sockaddr *)&cliaddr, socklen) < 0)
        Perror("bind failed");
    }

    if (Bcastcheck(sockfd, &servaddr) || Mcastcheck(sockfd, &servaddr, NULL, 1))
      udp_sendto = 1;

    if (type == SOCK_DGRAM && udp_sendto)
      goto skipconnect;

    if (connect(sockfd, (struct sockaddr *)&servaddr, socklen) < 0)
      Perror("connect failed");

  skipconnect:

    cf(sockfd, type, udp_sendto, (struct sockaddr *)&servaddr, socklen);

    close(sockfd);

    break;

  case s_mode:

    if (domain == AF_UNIX && unlink(servaddr.un.sun_path) < 0 &&
        errno != ENOENT)
      Perror("unlink failed");

    Mcastcheck(sockfd, &servaddr, interface, 0);

    if (bind(sockfd, (struct sockaddr *)&servaddr, socklen) < 0)
      Perror("bind failed");

    if (type == SOCK_STREAM) {
      if (listen(sockfd, backlog) < 0)
        Perror("listen failed");
      signal(SIGCHLD, sig_chld);
    }

    for (;;) {

      socklen = sizeof(cliaddr);
      switch (type) {
      case SOCK_STREAM:
        if ((clifd = accept(sockfd, (struct sockaddr *)&cliaddr, &socklen)) < 0)
          Perror("accept failed");
        break;
      case SOCK_DGRAM:
        nread = Recvfrom(sockfd, buf, sizeof(buf), &cliaddr, &socklen);
        break;
      }

      switch (domain) {
      case AF_INET:
      case AF_INET6:
        port = Ntop(&cliaddr);
        printf("client address %s, port %d\n", ntopbuf, (unsigned short)port);
        break;
      case AF_UNIX:
        printf("client address %s\n", cliaddr.un.sun_path);
        break;
      }

      switch (type) {
      case SOCK_STREAM:
        if ((ret = fork()) < 0)
          Perror("fork failed");
        if (ret) {
          printf("child %d forked\n", ret);
          close(clifd);
          continue;
        }
        sf(clifd, SOCK_STREAM, NULL, 0, 0);
        close(clifd);
        exit(0);
        break;
      case SOCK_DGRAM:
        sf(sockfd, SOCK_DGRAM, (struct sockaddr *)&cliaddr, socklen, nread);
        break;
      }
    }

    break;
  }

  close(sockfd);

  return 0;
}
