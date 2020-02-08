#include <ds/list.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/vfs.h>
// remove me
#include <sys/systm.h>

#define AF_UNIX     0x01

static int un_accept(struct socket *socket, struct socket **result, void *address, size_t *address_len);
static int un_bind(struct socket *socket, void *address, size_t address_len);
static int un_close(struct socket *sock);
static int un_connect(struct socket *socket, void *address, size_t address_len);
static int un_init(struct socket *socket, int type, int protocol);
static size_t un_recv(struct socket *sock, void *buf, size_t size);
static size_t un_send(struct socket *sock, const void *buf, size_t size);

struct socket_ops un_ops = {
    .accept     = un_accept,
    .bind       = un_bind,
    .close      = un_close,
    .connect    = un_connect,
    .init       = un_init,
    .recv       = un_recv,
    .send       = un_send
};

struct protocol un_domain = {
    .address_family = AF_UNIX,
    .ops            = &un_ops
};

struct un_conn {
    struct proc *       client;
    struct file *       tx_pipe[2];
    struct file *       rx_pipe[2];
    struct vfs_node *   host;
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

    struct vfs_node *host = server_state->host;

    while (LIST_SIZE(&host->un.un_connections) == 0) {
        thread_yield();
    }

    struct un_conn *client_conn;

    list_remove_back(&host->un.un_connections, (void**)&client_conn);

    struct socket *client = calloc(1, sizeof(struct socket*));

    struct un_conn *server_conn = calloc(1, sizeof(struct un_conn));

    server_conn->rx_pipe[0] = client_conn->tx_pipe[0];
    server_conn->tx_pipe[1] = client_conn->rx_pipe[1];

    client->state = server_conn;
    client->protocol = &un_domain;

    *result = client;

    return 0;
}

static int
un_bind(struct socket *socket, void *address, size_t address_len)
{
    struct sockaddr_un *addr_un = (struct sockaddr_un*)address;
    struct un_conn *state = calloc(1, sizeof(struct un_conn));

    socket->state = state;

    int res = fops_mknod(current_proc, addr_un->sun_path, 0777 | S_IFSOCK, 0);
    
    if (res != 0) {
        return res;
    }

    if (vfs_get_node(current_proc->root, current_proc->cwd, &state->host, addr_un->sun_path)) {
        panic("this shouldn't have happened. get help please");
    }

    INC_NODE_REF(state->host);

    return 0;   
}

static int
un_close(struct socket *sock)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    if (conn->tx_pipe[1]) {
        fops_close(conn->tx_pipe[1]);
        fops_close(conn->rx_pipe[0]);
    }

    if (conn->host) {
        DEC_NODE_REF(conn->host);
    }

    free(conn);

    return 0;
}

static int
un_connect(struct socket *socket, void *address, size_t address_len)
{
    struct sockaddr_un *addr_un = (struct sockaddr_un*)address;
    struct vfs_node *host;

    if (vfs_get_node(current_proc->root, current_proc->cwd, &host, addr_un->sun_path) != 0) {
        return -1;
    }

    struct un_conn *conn = calloc(1, sizeof(struct un_conn));

    conn->host = host;

    socket->state = conn;

    create_pipe(conn->tx_pipe);
    create_pipe(conn->rx_pipe);

    list_append(&host->un.un_connections, conn);

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

    return fops_read(conn->rx_pipe[0], buf, size);
}

static size_t
un_send(struct socket *sock, const void *buf, size_t size)
{
    struct un_conn *conn = (struct un_conn*)sock->state;

    return fops_write(conn->tx_pipe[1], buf, size);
}

__attribute__((constructor))
void
_init_un()
{
    register_protocol(&un_domain);
}
