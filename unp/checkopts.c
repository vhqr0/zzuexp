#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

struct sockopt {
  const char *str;
  int level;
  int name;
  enum { nullval, flagval, intval, timeval, lingerval } type;
} sockopts[] = {{"SO_BROADCAST", SOL_SOCKET, SO_BROADCAST, flagval},
                {"SO_DEBUG", SOL_SOCKET, SO_DEBUG, flagval},
                {"SO_DONTROUTE", SOL_SOCKET, SO_DONTROUTE, flagval},
                {"SO_ERROR", SOL_SOCKET, SO_ERROR, intval},
                {"SO_KEEPALIVE", SOL_SOCKET, SO_KEEPALIVE, flagval},
                {"SO_LINGER", SOL_SOCKET, SO_LINGER, lingerval},
                {"SO_OOBINLINE", SOL_SOCKET, SO_OOBINLINE, flagval},
                {"SO_RCVBUF", SOL_SOCKET, SO_RCVBUF, intval},
                {"SO_SNDBUF", SOL_SOCKET, SO_SNDBUF, intval},
                {"SO_RCVLOWAT", SOL_SOCKET, SO_RCVLOWAT, intval},
                {"SO_SNDLOWAT", SOL_SOCKET, SO_SNDLOWAT, intval},
                {"SO_RCVTIMEO", SOL_SOCKET, SO_RCVTIMEO, timeval},
                {"SO_SNDTIMEO", SOL_SOCKET, SO_SNDTIMEO, timeval},
                {"SO_REUSEADDR", SOL_SOCKET, SO_REUSEADDR, flagval},

#ifdef SO_REUSEPORT
                {"SO_REUSEPORT", SOL_SOCKET, SO_REUSEPORT, flagval},
#else
                {"SO_REUSEPORT", 0, 0, nullval},
#endif

                {"SO_TYPE", SOL_SOCKET, SO_TYPE, intval},

#ifdef SO_USELOOPBACK
                {"SO_USELOOPBACK", SOL_SOCKET, SO_USELOOPBACK, flagval},
#else
                {"SO_USELOOPBACK", 0, 0, nullval},
#endif

                {"IP_TOS", IPPROTO_IP, IP_TOS, intval},
                {"IP_TTL", IPPROTO_IP, IP_TTL, intval},
                {"IPV6_DONTFRAG", IPPROTO_IPV6, IPV6_DONTFRAG, flagval},
                {"IPV6_UNICAST_HOPS", IPPROTO_IPV6, IPV6_UNICAST_HOPS, intval},
                {"IPV6_V6ONLY", IPPROTO_IPV6, IPV6_V6ONLY, flagval},
                {"TCP_MAXMSG", IPPROTO_TCP, TCP_MAXSEG, intval},
                {"TCP_NODELAY", IPPROTO_TCP, TCP_NODELAY, flagval},
                {NULL, 0, 0, nullval}};

int main() {
  struct sockopt *opt;
  union {
    int intval;
    struct timeval timeval;
    struct linger lingerval;
  } val;
  socklen_t socklen;
  int sock4fd, sock6fd, ret;
  char buf[20];

  buf[19] = 0;
  sock4fd = socket(AF_INET, SOCK_STREAM, 0);
  sock6fd = socket(AF_INET6, SOCK_STREAM, 0);

  for (opt = sockopts; opt->str; opt++) {
    if (strlen(opt->str) >= 20) {
      fprintf(stderr, "optstr too long\n");
      exit(-1);
    }
    memset(buf, ' ', 19);
    memcpy(buf, opt->str, strlen(opt->str));
    printf("%s\t", buf);

    if (opt->type != nullval) {
      socklen = sizeof(val);
      if (getsockopt(opt->level == IPPROTO_IPV6 ? sock6fd : sock4fd, opt->level,
                     opt->name, &val, &socklen) < 0) {
        perror("getsockopt failed");
        exit(-1);
      }
      switch (opt->type) {
      case nullval:
      case flagval:
      case intval:
        ret = sizeof(int);
        break;
      case timeval:
        ret = sizeof(struct timeval);
        break;
      case lingerval:
        ret = sizeof(struct linger);
        break;
      }
      if (socklen != ret) {
        fprintf(stderr, "getsockopt unexpected length\n");
        exit(-1);
      }
    }

    switch (opt->type) {
    case nullval:
      puts("UNSUPPORTED");
      break;
    case flagval:
      puts(val.intval ? "on" : "off");
      break;
    case intval:
      printf("%d\n", val.intval);
      break;
    case timeval:
      printf("%ld s %ld us\n", val.timeval.tv_sec, val.timeval.tv_usec);
      break;
    case lingerval:
      if (val.lingerval.l_onoff)
        printf("%d\n", val.lingerval.l_linger);
      else
        puts("off");
      break;
    }
  }
}
