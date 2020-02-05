#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include <stdint.h>

typedef unsigned int socklen_t;

#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_RDM        4
#define SOCK_SEQPACKET  5

#define PF_UNSPEC   0
#define PF_LOCAL    1
#define PF_UNIX     PF_LOCAL
#define PF_FILE     PF_LOCAL
#define PF_INET     2
#define PF_INET6    10
#define PF_PACKET   17
#define PF_KLINK    40

#define AF_UNSPEC   PF_UNSPEC
#define AF_LOCAL    PF_LOCAL
#define AF_UNIX     PF_LOCAL
#define AF_FILE     PF_LOCAL
#define AF_INET     PF_INET
#define AF_INET6    PF_INET6
#define AF_PACKET   PF_PACKET
#define AF_KLINK    PF_KLINK

struct sockaddr {
    uint16_t    sa_family;
    uint8_t     sa_data[14];
};

int accept(int, struct sockaddr *, socklen_t *);
int bind(int, const struct sockaddr *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int socket(int, int, int);

#endif
