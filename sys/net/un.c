/*
 * un.c - UNIX sockets (sort of)
 *
 * This file implements IPC sockets. Currently, this only supports connection 
 * based socks (SOCK_STREAM) and does not support advanced things like sharing
 * file descriptors
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
#include <sys/pipe.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/vnode.h>

#define AF_UNIX     0x01

static int un_accept(struct socket *socket, struct socket **result, void *address, size_t *address_len);
static int un_bind(struct socket *socket, void *address, size_t address_len);
static int un_close(struct socket *sock);
static int un_connect(struct socket *socket, void *address, size_t address_len);
static int un_destroy(struct socket *sock);
static int un_duplicate(struct socket *sock);
static int un_init(struct socket *socket, int type, int protocol);
static size_t un_recv(struct socket *sock, void *buf, size_t size);
static size_t un_send(struct socket *sock, const void *buf, size_t size);

struct socket_ops un_ops = {
    .accept     = un_accept,
    .bind       = un_bind,
    .close      = un_close,
    .connect    = un_connect,
    .destroy    = un_destroy,
    .duplicate  = un_duplicate,
    .init       = un_init,
    .recv       = un_recv,
    .send       = un_send
};

struct protocol un_protocol = {
    .address_family = AF_UNIX,
    .ops            = &un_ops
};

struct un_conn {
    struct proc *       client;
    struct file *       tx_pipe[2];
    struct file *       rx_pipe[2];
    struct un_conn *    peer;
    bool                accepted;
    bool                closed;
    int                 refs;
    struct vnode *      host;
};

static int
un_accept(struct socket *socket, struct socket **result, void *address, size_t *address_len)
{
    if (!socket->state) {
        return -(EINVAL);
    }

    struct un_conn *server_state = socket->state;

    if (!server_state->host) {
        /* if this isn't set, this socket is not bound... */
        return -(EINVAL );
    }

    struct vnode *host = server_state->host;

    while (LIST_SIZE(&host->un.un_connections) == 0) {
        thread_yield();
    }

    struct un_conn *client_conn;

    list_remove_back(&host->un.un_connections, (void**)&client_conn);

    struct socket *client = calloc(1, sizeof(struct socket*));

    struct un_conn *server_conn = calloc(1, sizeof(struct un_conn));

    server_conn->refs = 1;
    server_conn->rx_pipe[0] = client_conn->tx_pipe[0];
    server_conn->tx_pipe[1] = client_conn->rx_pipe[1];
    server_conn->peer = client_conn;
    client_conn->peer = server_conn;
    client_conn->accepted = true;
    client->state = server_conn;
    client->protocol = &un_protocol;
    *result = client;

    return 0;
}

static int
un_bind(struct socket *socket, void *address, size_t address_len)
{
    struct sockaddr_un *addr_un = (struct sockaddr_un*)address;
    struct un_conn *state = calloc(1, sizeof(struct un_conn));

    socket->state = state;

    int res = vfs_mknod(current_proc, addr_un->sun_path, 0777 | S_IFSOCK, 0);
    
    if (res != 0) {
        return res;
    }

    if (vn_open(current_proc->root, current_proc->cwd, &state->host, addr_un->sun_path)) {
        panic("this shouldn't have happened. get help please");
    }

    VN_INC_REF(state->host);

    return 0;   
}

static int
un_destroy(struct socket *sock)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    KASSERT("tx/rx pipes should be called before the vnode is destroyed",
            conn->rx_pipe[0] == NULL && conn->tx_pipe[1] == NULL);

    if (conn->host) {
        VN_DEC_REF(conn->host);
    }

    free(conn);

    return 0;
}

static int
un_duplicate(struct socket *sock)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    conn->refs++;

    return 0;
}

static int
un_close(struct socket *sock)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    if (!conn) {
        return 0;
    }

    conn->refs--;

    if (conn->refs == 0) {
        KASSERT("un tx/rx pipes should not have been closed already",
                conn->rx_pipe[0] != NULL && conn->tx_pipe[1] != NULL);
        fop_close(conn->rx_pipe[0]);
        fop_close(conn->tx_pipe[1]);
        
        conn->rx_pipe[0] = NULL;
        conn->tx_pipe[1] = NULL;
        conn->closed = 1;
    }

    return 0;
}

static int
un_connect(struct socket *socket, void *address, size_t address_len)
{
    struct sockaddr_un *addr_un = (struct sockaddr_un*)address;
    struct vnode *host;

    if (vn_open(current_proc->root, current_proc->cwd, &host, addr_un->sun_path) != 0) {
        return -1;
    }

    struct un_conn *conn = calloc(1, sizeof(struct un_conn));

    conn->host = host;
    conn->refs = 1;

    socket->state = conn;

    create_pipe(conn->tx_pipe);
    create_pipe(conn->rx_pipe);

    list_append(&host->un.un_connections, conn);

    while (!conn->accepted) {
        thread_yield();
    }

    return 0;
}

static int
un_init(struct socket *socket, int type, int protocol)
{
    return 0;
}

static size_t
un_recv(struct socket *sock, void *buf, size_t size)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    if (conn->peer->closed) {
        return -(ECONNRESET);
    }

    int ret = fop_read(conn->rx_pipe[0], buf, size);
    
    if (ret == -(EPIPE)) {
        return -(ECONNRESET);
    }

    return ret;
}

static size_t
un_send(struct socket *sock, const void *buf, size_t size)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    if (conn->peer->closed) {
        return -(ECONNRESET);
    }

    int ret = fop_write(conn->tx_pipe[1], buf, size);
    
    if (ret == -(ESPIPE)) {
        return -(ECONNRESET);
    }

    return ret;
}
