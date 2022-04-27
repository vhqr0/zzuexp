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
#define UNPATHLEN 108

char buf[BUFSIZE], ntopbuf[ADDRSTRLEN];

void writen(int fd, char *b, int n) {
  int nread;

  while (n > 0) {
    if ((nread = write(fd, b, n)) < 0) {
      if (errno == EINTR)
        continue;
      perror("write failed");
      exit(-1);
    }
    if (!nread) {
      fprintf(stderr, "write unexpected EOF\n");
      exit(-1);
    }
    b += nread;
    n -= nread;
  }
}

void c_echo(int sockfd, int type) {
  int nread;
  fd_set rfds;

  for (;;) {
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(sockfd, &rfds);
    if (select(sockfd + 1, &rfds, NULL, NULL, NULL) < 0) {
      if (errno == EINTR)
        continue;
      perror("select failed");
      exit(-1);
    }
    if (FD_ISSET(sockfd, &rfds)) {
      if ((nread = read(sockfd, buf, sizeof(buf))) < 0) {
        if (errno == EINTR)
          continue;
        perror("read failed");
        exit(-1);
      }
      if (nread == 0 && type == SOCK_STREAM) {
        fprintf(stderr, "server terminated prematurely\n");
        exit(-1);
      }
      if (nread)
        writen(STDOUT_FILENO, buf, nread);
    }
    if (FD_ISSET(STDIN_FILENO, &rfds)) {
      if ((nread = read(STDIN_FILENO, buf, sizeof(buf))) < 0) {
        if (errno == EINTR)
          continue;
        perror("read failed");
        exit(-1);
      }
      if (nread)
        writen(sockfd, buf, nread);
      else
        return;
    }
  }
}

void s_echo(int sockfd, int type, struct sockaddr *cliaddr, socklen_t socklen,
            int n) {
  int nread;

  switch (type) {
  case SOCK_STREAM:
    while ((nread = read(sockfd, buf, sizeof(buf))) > 0)
      writen(sockfd, buf, nread);
    if (nread < 0) {
      perror("read failed");
      exit(-1);
    }
    break;
  case SOCK_DGRAM:
    if (sendto(sockfd, buf, n, 0, cliaddr, socklen) < 0) {
      perror("send failed");
      exit(-1);
    }
    break;
  }
}

void c_daytime(int sockfd, int type) {
  int nread;

  switch (type) {
  case SOCK_STREAM:
    while ((nread = read(sockfd, buf, sizeof(buf))) > 0)
      writen(STDOUT_FILENO, buf, nread);
    break;
  case SOCK_DGRAM:
    if (write(sockfd, "", 1) < 0) {
      perror("write failed");
      exit(-1);
    }
    if ((nread = read(sockfd, buf, sizeof(buf))) > 0)
      writen(STDOUT_FILENO, buf, nread);
    break;
  }
  if (nread < 0) {
    perror("read failed");
    exit(-1);
  }
}

void s_daytime(int sockfd, int type, struct sockaddr *cliaddr,
               socklen_t socklen, int n) {
  int nread;
  time_t ticks;

  ticks = time(NULL);
  if ((nread = snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks))) < 0) {
    perror("snprintf failed");
    exit(-1);
  }
  if (nread >= sizeof(buf)) {
    fprintf(stderr, "snprintf buffer too small: %d/%d\n", nread,
            (int)(sizeof(buf)));
    exit(-1);
  }
  switch (type) {
  case SOCK_STREAM:
    writen(sockfd, buf, nread);
    break;
  case SOCK_DGRAM:
    if (sendto(sockfd, buf, nread, 0, cliaddr, socklen) < 0) {
      perror("sendto failed");
      exit(-1);
    }
    break;
  }
}

void sig_chld(int signo) {
  int pid, stat;

  if ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    printf("child %d terminated\n", pid);
}

int main(int argc, char **argv) {
  int sockfd, clifd, domain = AF_UNIX, type = SOCK_STREAM, backlog = 5,
                     mcast = 0, nread, ret;
  short port = 0, cliport;
  const char *address = "/tmp/serv.sock", *interface = NULL;
  struct addrinfo hints, *rai;
  union {
    struct sockaddr_in a4;
    struct sockaddr_in6 a6;
    struct sockaddr_un un;
  } servaddr, cliaddr;
  socklen_t socklen;
  struct ifreq ifr;
  struct ip_mreq imr4;
  struct ipv6_mreq imr6;
  enum { c_mode, s_mode } mode = c_mode;
  enum { e_serv, d_serv } serv = d_serv;
  void (*cf)(int, int);
  void (*sf)(int, int, struct sockaddr *, socklen_t, int);

  while ((ret = getopt(argc, argv, "hTU4::6::u:p:m::b:csed")) > 0) {
    switch (ret) {
    case 'h':
      printf("usage: %s [-h] [-TU] [-46[ADDRESS] -u [PATH]] [-p PORT] "
             "[-m[INTERFACE]] [-b BACKLOG] [-cs] [-ed]\n",
             argv[0]);
      exit(1);
      break;
    case 'T':
      type = SOCK_STREAM;
      break;
    case 'U':
      type = SOCK_DGRAM;
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
    case 'm':
      mcast = 1;
      interface = optarg;
      break;
    case 'p':
      port = atoi(optarg);
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
      fprintf(stderr, "unknown option: %c\n", ret);
      exit(-1);
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
    if (strlen(address) >= UNPATHLEN) {
      fprintf(stderr, "path too long: %s\n", address);
      exit(-1);
    }
    strncpy(servaddr.un.sun_path, address, UNPATHLEN);
    socklen = sizeof(struct sockaddr_un);
    break;
  }
  if (address && (domain == AF_INET || domain == AF_INET6)) {
    switch (domain) {
    case AF_INET:
      ret = inet_pton(AF_INET, address, &servaddr.a4.sin_addr);
      break;
    case AF_INET6:
      ret = inet_pton(AF_INET6, address, &servaddr.a6.sin6_addr);
      break;
    }
    if (ret < 0) {
      perror("inet_pton failed");
      exit(-1);
    }
    if (!ret) {
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = domain;
      hints.ai_socktype = type;
      if ((ret = getaddrinfo(address, NULL, &hints, &rai))) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret));
        exit(-1);
      }
      switch (domain) {
      case AF_INET:
        servaddr.a4.sin_addr = ((struct sockaddr_in *)rai->ai_addr)->sin_addr;
        break;
      case AF_INET6:
        servaddr.a6.sin6_addr =
            ((struct sockaddr_in6 *)rai->ai_addr)->sin6_addr;
        break;
      }
      freeaddrinfo(rai);
    }
  }

  if ((sockfd = socket(domain, type, 0)) < 0) {
    perror("socket failed");
    exit(-1);
  }

  switch (mode) {

  case c_mode:

    if (domain == AF_UNIX && type == SOCK_DGRAM) {
      memset(&cliaddr, 0, sizeof(cliaddr));
      cliaddr.un.sun_family = AF_UNIX;
      if ((nread = snprintf(cliaddr.un.sun_path, UNPATHLEN, "/tmp/serv%d.sock",
                            getpid())) < 0) {
        perror("snprintf failed");
        exit(-1);
      }
      if (nread >= UNPATHLEN) {
        fprintf(stderr, "snprintf buffer too small: %d/%d\n", nread, UNPATHLEN);
        exit(-1);
      }
      if (bind(sockfd, (struct sockaddr *)&cliaddr, socklen) < 0) {
        perror("bind failed");
        exit(-1);
      }
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, socklen) < 0) {
      perror("connect failed");
      exit(-1);
    }

    cf(sockfd, type);

    close(sockfd);

    break;

  case s_mode:

    if (domain == AF_UNIX && unlink(servaddr.un.sun_path) < 0 &&
        errno != ENOENT) {
      perror("unlink failed");
      exit(-1);
    }

    if (mcast) {
      switch (domain) {
      case AF_INET:
        memset(&imr4, 0, sizeof(imr4));
        memcpy(&imr4.imr_multiaddr, &servaddr.a4.sin_addr,
               sizeof(struct in_addr));
        if (interface) {
          memset(&ifr, 0, sizeof(ifr));
          strncpy(ifr.ifr_name, interface, IFNAMSIZ);
          if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
            perror("ioctl SIOCGIFADDR failed");
            exit(-1);
          }
          memcpy(&imr4.imr_interface,
                 &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr,
                 sizeof(struct in_addr));
        }
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr4,
                       sizeof(imr4)) < 0) {
          perror("setsockopt IP_ADD_MEMBERSHIP failed");
          exit(-1);
        }
        memset(&servaddr.a4.sin_addr, 0, sizeof(struct in_addr));
        break;
      case AF_INET6:
        memset(&imr6, 0, sizeof(imr6));
        memcpy(&imr6.ipv6mr_multiaddr, &servaddr.a6.sin6_addr,
               sizeof(struct in6_addr));
        if (interface) {
          memset(&ifr, 0, sizeof(ifr));
          strncpy(ifr.ifr_name, interface, IFNAMSIZ);
          if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
            perror("ioctl SIOCGIFINDEX failed");
            exit(-1);
          }
          imr6.ipv6mr_interface = ifr.ifr_ifindex;
        }
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &imr6,
                       sizeof(imr6)) < 0) {
          perror("setsockopt IPV6_JOIN_GROUP failed");
          exit(-1);
        }
        memset(&servaddr.a6.sin6_addr, 0, sizeof(struct sockaddr_in6));
        break;
      }
    }

    if (bind(sockfd, (struct sockaddr *)&servaddr, socklen) < 0) {
      perror("bind failed");
      exit(-1);
    }

    if (type == SOCK_STREAM) {
      if (listen(sockfd, backlog) < 0) {
        perror("listen failed");
        exit(-1);
      }
      signal(SIGCHLD, sig_chld);
    }

    for (;;) {

      socklen = sizeof(cliaddr);
      switch (type) {
      case SOCK_STREAM:
        if ((clifd = accept(sockfd, (struct sockaddr *)&cliaddr, &socklen)) <
            0) {
          perror("accept failed");
          exit(-1);
        }
        break;
      case SOCK_DGRAM:
        if ((nread = recvfrom(sockfd, buf, sizeof(buf), 0,
                              (struct sockaddr *)&cliaddr, &socklen)) < 0) {
          perror("recvfrom failed");
          exit(-1);
        }
        break;
      }

      switch (((struct sockaddr *)&cliaddr)->sa_family) {
      case AF_INET:
        cliport = ntohs(cliaddr.a4.sin_port);
        if (!inet_ntop(AF_INET, &cliaddr.a4.sin_addr, ntopbuf,
                       sizeof(ntopbuf))) {
          perror("inet_ntop failed");
          exit(-1);
        }
        printf("client address %s, port %d\n", ntopbuf,
               (unsigned short)cliport);
        break;
      case AF_INET6:
        cliport = ntohs(cliaddr.a6.sin6_port);
        if (!inet_ntop(AF_INET6, &cliaddr.a6.sin6_addr, ntopbuf,
                       sizeof(ntopbuf))) {
          perror("inet_ntop failed");
          exit(-1);
        }
        printf("client address %s, port %d\n", ntopbuf,
               (unsigned short)cliport);
        break;
      case AF_UNIX:
        printf("client address %s\n", cliaddr.un.sun_path);
        break;
      default:
        fprintf(stderr, "unknown address family: %d\n",
                ((struct sockaddr *)&cliaddr)->sa_family);
        exit(-1);
      }

      switch (type) {
      case SOCK_STREAM:
        if ((ret = fork()) < 0) {
          perror("fork failed");
          exit(-1);
        }
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
