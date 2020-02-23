#ifndef SYS_NET_H
#define SYS_NET_H

#include <sys/types.h>

#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

struct socket;
struct socket_ops;

typedef int (*sock_accept_t)(struct socket *socket, struct socket **result, void *address, size_t *address_len);
typedef int (*sock_bind_t)(struct socket *socket, void *address, size_t address_len);
typedef int (*sock_close_t)(struct socket *socket);
typedef int (*sock_connect_t)(struct socket *socket, void *address, size_t address_len);
typedef int (*sock_destroy_t)(struct socket *socket);
typedef int (*sock_duplicate_t)(struct socket *sock);
typedef int (*sock_init_t)(struct socket *socket, int type, int protocol);
typedef size_t (*sock_recv_t)(struct socket *socket, void *buf, size_t size);
typedef size_t (*sock_send_t)(struct socket *socket, const void *buf, size_t size);

struct protocol {
    uint32_t            address_family;
    struct socket_ops * ops;
};

struct sockaddr {
    uint16_t            sa_family;
    uint8_t             sa_data[14];
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

void register_protocol(struct protocol *protocol);
int sock_accept(struct socket *sock, struct socket **result, void *address, size_t *address_len);
int sock_bind(struct socket *sock, void *address, size_t address_len);
int sock_close(struct socket *sock);
int sock_connect(struct socket *sock, void *address, size_t address_size);
int sock_destroy(struct socket *sock);
int sock_duplicate(struct socket *sock);
int sock_new(struct socket **result, int domain, int type, int protocol);
size_t sock_recv(struct socket *sock, void *buf, size_t nbyte);
size_t sock_send(struct socket *sock, const void *buf, size_t nbyte);
struct file *sock_to_file(struct socket *sock);
struct socket *file_to_sock(struct file *file);

#endif
