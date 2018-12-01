#include <stdio.h>
#include <ds/list.h>
#include <sys/net.h>
#include <sys/types.h>

#define AF_KLINK    0x100

struct klink_session {
    struct list queue;
};

struct klink_req {
    uint16_t    opcode;
    uint16_t    what;
    uint16_t    seq;
};

struct klink_resp {
    uint16_t    seq;
    uint16_t    length;
    char        payload[];
};

static int klink_accept(struct socket *socket, struct socket **result, void *address, size_t address_len);
static int klink_connect(struct socket *socket, void *address, size_t address_len);
static int klink_init(struct socket *socket, int type, int protocol);
static size_t klink_recv(struct socket *socket, void *buf, size_t size);
static size_t klink_send(struct socket *socket, const void *buf, size_t);

struct socket_ops klink_ops = {
    .accept     = klink_accept,
    .connect    = klink_connect,
    .init       = klink_init,
    .recv       = klink_recv,
    .send       = klink_send
};

struct protocol klink_domain = {
    .address_family = AF_KLINK,
    .ops            = &klink_ops
};

static int 
klink_accept(struct socket *socket, struct socket **result, void *address, size_t address_len)
{
    return 0;
}

static int
klink_connect(struct socket *socket, void *address, size_t address_len)
{
    return 0;
}

static int
klink_init(struct socket *socket, int type, int protocol)
{
    return 0;
}

static size_t
klink_recv(struct socket *socket, void *buf, size_t size)
{
    return 0;
}

static size_t
klink_send(struct socket *socket, const void *buf, size_t size)
{
    return 0;
}

__attribute__((constructor))
void
init_klink()
{
    register_protocol(&klink_domain);
}
