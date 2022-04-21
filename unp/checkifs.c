#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

void ioctln(int sockfd, unsigned long req, void *res) {
  if (ioctl(sockfd, req, res) < 0) {
    perror("ioctl failed");
    exit(-1);
  }
}

void ioctlifreq(int sockfd, unsigned long req, struct ifreq *ifr,
                const char *name) {
  memset(ifr, 0, sizeof(struct ifreq));
  strcpy(ifr->ifr_name, name);
  ioctln(sockfd, req, ifr);
}

void print_in_addr(struct sockaddr_in *addr) {
  char ntopbuf[INET_ADDRSTRLEN];

  if (addr->sin_family != AF_INET) {
    fprintf(stderr, "invalid address family: %d\n", addr->sin_family);
    exit(-1);
  }
  if (!inet_ntop(AF_INET, &addr->sin_addr, ntopbuf, sizeof(ntopbuf))) {
    perror("inet_ntop failed");
    exit(-1);
  }
  printf("%s", ntopbuf);
}

int main() {
  int sockfd, nread, flags, i, j;
  struct ifconf ifc;
  struct ifreq *ifcc, ifr;
  char ntopbuf[INET_ADDRSTRLEN];

  ifc.ifc_len = 1024 * sizeof(struct ifreq);
  ifc.ifc_buf = malloc(ifc.ifc_len);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(-1);
  }

  ioctln(sockfd, SIOCGIFCONF, &ifc);

  nread = ifc.ifc_len / sizeof(struct ifreq);

  for (i = 0; i < nread; i++) {
    ifcc = ifc.ifc_req + i;

    /* flags */
    ioctlifreq(sockfd, SIOCGIFFLAGS, &ifr, ifcc->ifr_name);
    flags = ifr.ifr_flags;
    printf("%s: < ", ifcc->ifr_name);
    if (flags & IFF_UP)
      printf("UP ");
    if (flags & IFF_BROADCAST)
      printf("BCAST ");
    if (flags & IFF_MULTICAST)
      printf("MCAST ");
    if (flags & IFF_LOOPBACK)
      printf("LOOP ");
    if (flags & IFF_POINTOPOINT)
      printf("P2P ");
    puts(">");
    if (!(flags & IFF_UP))
      continue;

    /* mtu */
    ioctlifreq(sockfd, SIOCGIFMTU, &ifr, ifcc->ifr_name);
    printf("MTU\t%d\n", ifr.ifr_mtu);

    /* mac address */
    ioctlifreq(sockfd, SIOCGIFHWADDR, &ifr, ifcc->ifr_name);
    printf("MAC\t");
    for (j = 0; j < 6; j++) {
      if (j)
        printf(":");
      printf("%02x", ifr.ifr_hwaddr.sa_data[j]);
    }
    puts("");

    /* ip address */
    if (ifcc->ifr_addr.sa_family != AF_INET) {
      fprintf(stderr, "unknown address family\t%d\n", ifcc->ifr_addr.sa_family);
      continue;
    }
    printf("IP\t");
    print_in_addr((struct sockaddr_in *)&ifcc->ifr_addr);
    puts("");

    /* ip mask */
    ioctlifreq(sockfd, SIOCGIFNETMASK, &ifr, ifcc->ifr_name);
    printf("MASK\t");
    print_in_addr((struct sockaddr_in *)&ifr.ifr_addr);
    puts("");

    /* ip bcast address */
    if (flags & IFF_BROADCAST) {
      ioctlifreq(sockfd, SIOCGIFBRDADDR, &ifr, ifcc->ifr_name);
      printf("BRD\t");
      print_in_addr((struct sockaddr_in *)&ifr.ifr_broadaddr);
    }

    /* ip p2p address */
    if (flags & IFF_POINTOPOINT) {
      ioctlifreq(sockfd, SIOCGIFDSTADDR, &ifr, ifcc->ifr_name);
      printf("P2P\t");
      print_in_addr((struct sockaddr_in *)&ifr.ifr_dstaddr);
    }

    puts("");
  }

  free(ifc.ifc_buf);
}
