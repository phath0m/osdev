/*
 * socket.c - Socket implementation
 *
 * This file is responsible for providing the core functionality required to
 * interact with sockets via a struct file instance. No actual networking logic
 * is implemented here, rather, this acts as the "interface" to interact with
 * protocols that define their own type of sockets
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
#include <ds/list.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/vnode.h>

/* the built-in protocols */
extern struct protocol un_protocol;

struct protocol *socket_builtin_protocols[] = {
    &un_protocol,
    NULL
};


static struct list protocol_list;

/*
 * Wrapper functions to connect socket API to underlying file instance
 */
static int
sock_file_close(struct file *fp)
{
    return SOCK_CLOSE(fp->state);
}

static int
sock_file_destroy(struct file *fp)
{
    int res;

    res = SOCK_DESTROY(fp->state);

    if (res == 0) free(fp->state);

    return res;
}

static int
sock_file_duplicate(struct file *fp)
{
    return SOCK_DUPLICATE(fp->state); 
}

static int
sock_file_getvn(struct file *fp, struct vnode **vn)
{
    return SOCK_GETVN(fp->state, vn);
}

static int
sock_file_read(struct file *fp, void *buf, size_t nbyte)
{
    return SOCK_RECV(fp->state, buf, nbyte);
}

static int
sock_file_write(struct file *fp, const void *buf, size_t nbyte)
{
    return SOCK_SEND(fp->state, buf, nbyte);
}

struct fops sock_file_ops = {
    .close      = sock_file_close,
    .destroy    = sock_file_destroy,
    .duplicate  = sock_file_duplicate,
    .getvn      = sock_file_getvn,
    .read       = sock_file_read,
    .write      = sock_file_write,
};

static struct protocol *
get_protocol_from_domain(int domain)
{
    list_iter_t iter;

    struct protocol *prot;
    struct protocol *match;

    list_get_iter(&protocol_list, &iter);

    match = NULL;

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
sock_new(struct socket **result, int domain, int type, int protocol)
{
    struct protocol *prot;
    struct socket *sock;

    prot = get_protocol_from_domain(domain);

    if (!prot) {
        return -(EINVAL);
    }

    sock = calloc(1, sizeof(struct socket));

    sock->protocol = prot;
    sock->type = type;

    *result = sock;

    if (prot->ops && prot->ops->init) {
        return prot->ops->init(sock, type, protocol);
    }

    return 0;
}

struct file *
sock_to_file(struct socket *sock)
{
    struct file *ret;
    
    ret = file_new(&sock_file_ops, NULL);

    ret->state = sock;
    ret->flags = O_RDWR;

    sock->refs++;

    return ret;
}

struct socket *
file_to_sock(struct file *file)
{
    return file->state;
}

void
sock_init()
{
    int i;
    struct protocol *prot;

    i = 0;

    while ((prot = socket_builtin_protocols[i++])) {
        register_protocol(prot);
    }
}