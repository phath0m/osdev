/*
 * socket.c - Socket implementation
 *
 * This file is responsible for providing the core functionality required to
 * interact with sockets via a struct file instance. No actual networking logic
 * is implemented here, rather, this acts as the "interface" to interact with
 * protocols that define their own type of sockets
 */
#include <ds/list.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/vnode.h>

static struct list protocol_list;

/*
 * Wrapper functions to connect socket API to underlying file instance
 */
static int
sock_file_close(struct file *fp)
{
    struct socket *sock = (struct socket*)fp->state;

    return sock_close(sock);
}

static int
sock_file_destroy(struct file *fp)
{
    struct socket *sock = (struct socket*)fp->state;

    sock_destroy(sock);

    return 0;
}

static int
sock_file_duplicate(struct file *fp)
{
    struct socket *sock = (struct socket*)fp->state;

    return sock_duplicate(sock); 
}

static int
sock_file_read(struct file *fp, void *buf, size_t nbyte)
{
    struct socket *sock = (struct socket*)fp->state;

    return sock_recv(sock, buf, nbyte);
}

static int
sock_file_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct socket *sock = (struct socket*)fp->state;

    return sock_send(sock, buf, nbyte);
}

struct fops sock_file_ops = {
    .close      = sock_file_close,
    .destroy    = sock_file_destroy,
    .duplicate  = sock_file_duplicate,
    .read       = sock_file_read,
    .write      = sock_file_write,
};

static struct protocol *
get_protocol_from_domain(int domain)
{
    list_iter_t iter;

    list_get_iter(&protocol_list, &iter);

    struct protocol *prot;
    struct protocol *match = NULL;

    while (iter_move_next(&iter, (void**)&prot)) {
        if (prot->address_family == domain) {
            match = prot;
            break;
        }
    }

    iter_close(&iter);

    return match;
}

void
register_protocol(struct protocol *protocol)
{
    list_append(&protocol_list, protocol);
}

int
sock_accept(struct socket *sock, struct socket **result, void *address, size_t *address_len)
{
    if (!sock) {
        return -(EINVAL);
    }

    struct protocol *prot = sock->protocol;

    if (!prot->ops || !prot->ops->accept) {
        return -(ENOTSUP);
    }

    return prot->ops->accept(sock, result, address, address_len);
}

int
sock_bind(struct socket *sock, void *address, size_t address_len)
{
    if (!sock) {
        return -(EINVAL);
    }

    struct protocol *prot = sock->protocol;

    if (!prot->ops || !prot->ops->bind) {
        return -(ENOTSUP);
    }

    return prot->ops->bind(sock, address, address_len);
}

int
sock_close(struct socket *sock)
{
    struct protocol *prot = sock->protocol;

    int ret;

    if (!prot->ops || !prot->ops->close) {
        ret = 0;
    } else {
        ret = prot->ops->close(sock);
    }

    return ret;
}

int
sock_connect(struct socket *sock, void *address, size_t address_size)
{
    if (!sock) {
        return -(EINVAL);
    }

    struct protocol *prot = sock->protocol;

    if (!prot->ops || !prot->ops->connect) {
        return -(ENOTSUP);
    }

    return prot->ops->connect(sock, address, address_size);
}

int
sock_destroy(struct socket *sock)
{
    struct protocol *prot = sock->protocol;

    int ret;

    if (!prot->ops || !prot->ops->close) {
        ret = 0;
    } else {
        ret = prot->ops->destroy(sock);
    }

    free(sock);

    return ret;
}

int
sock_duplicate(struct socket *sock)
{   
    struct protocol *prot = sock->protocol;
    
    int ret;
    
    if (!prot->ops || !prot->ops->duplicate) {
        ret = 0;
    } else {
        ret = prot->ops->duplicate(sock);
    }
    
    return ret;
}

int
sock_new(struct socket **result, int domain, int type, int protocol)
{
    struct protocol *prot = get_protocol_from_domain(domain);

    if (!prot) {
        return -(EINVAL);
    }

    struct socket *sock = (struct socket*)calloc(1, sizeof(struct socket));

    sock->protocol = prot;
    sock->type = type;
    *result = sock;

    if (prot->ops && prot->ops->init) {
        return prot->ops->init(sock, type, protocol);
    }

    return 0;
}

size_t
sock_recv(struct socket *sock, void *buf, size_t nbyte)
{
    if (!sock) {
        return -(EINVAL);
    }

    struct protocol *prot = sock->protocol;

    if (!prot->ops || !prot->ops->recv) {
        return -(ENOTSUP);
    }

    return prot->ops->recv(sock, buf, nbyte);
}

size_t
sock_send(struct socket *sock, const void *buf, size_t nbyte)
{
    if (!sock) {
        return -(EINVAL);
    }

    struct protocol *prot = sock->protocol;

    if (!prot->ops || !prot->ops->send) {
        return -(ENOTSUP);
    }

    return prot->ops->send(sock, buf, nbyte);
}

struct file *
sock_to_file(struct socket *sock)
{
    struct file *ret = file_new(&sock_file_ops, NULL);

    ret->state = sock;
    ret->flags = O_RDWR;
    sock->refs++;

    return ret;
}

struct socket *
file_to_sock(struct file *file)
{
    struct socket *sock = (struct socket*)file->state;

    return sock;
}
