/*
 * socket.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

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
    uint16_t            sa_family;
    uint8_t             sa_data[14];
};

#ifdef __KERNEL__
struct socket;
struct socket_ops;

typedef int (*sock_accept_t)(struct socket *, struct socket **, void *, size_t *);
typedef int (*sock_bind_t)(struct socket *, void *, size_t);
typedef int (*sock_close_t)(struct socket *);
typedef int (*sock_connect_t)(struct socket *, void *, size_t);
typedef int (*sock_destroy_t)(struct socket *);
typedef int (*sock_duplicate_t)(struct socket *);
typedef int (*sock_init_t)(struct socket *, int, int);
typedef size_t (*sock_recv_t)(struct socket *, void *, size_t);
typedef size_t (*sock_send_t)(struct socket *, const void *, size_t);

struct protocol {
    uint32_t            address_family;
    struct socket_ops * ops;
};

struct socket {
    struct protocol *   protocol;
    int                 type;
    int                 refs;
    void *              state;
};

struct socket_ops {
    sock_accept_t       accept;
    sock_close_t        close;
    sock_bind_t         bind;
    sock_connect_t      connect;
    sock_destroy_t      destroy;
    sock_duplicate_t    duplicate;
    sock_init_t         init;
    sock_recv_t         recv;
    sock_send_t         send;
};

void            register_protocol(struct protocol *);
int             sock_accept(struct socket *, struct socket **, void *, size_t *);
int             sock_bind(struct socket *, void *, size_t);
int             sock_close(struct socket *);
int             sock_connect(struct socket *, void *, size_t);
int             sock_destroy(struct socket *);
int             sock_duplicate(struct socket *);
int             sock_new(struct socket **, int, int, int);
size_t          sock_recv(struct socket *, void *, size_t);
size_t          sock_send(struct socket *, const void *, size_t);
struct file *   sock_to_file(struct socket *);
struct socket * file_to_sock(struct file *);

void            sock_init();
#else /* __KERNEL__ */
int             accept(int, struct sockaddr *, socklen_t *);
int             bind(int, const struct sockaddr *, socklen_t);
int             connect(int, const struct sockaddr *, socklen_t);
int             socket(int, int, int);
#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_SOCKET_H */
