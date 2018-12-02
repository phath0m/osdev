#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ds/list.h>
#include <sys/net.h>
#include <sys/proc.h>
#include <sys/types.h>

#define AF_KLINK    40

#define KLINK_QUERY     0x01
#define KWHAT_PROCLIST  0x02
#define KWHAT_PROC_ARGV 0x03
#define KWHAT_ENVIRON   0x04

#define MIN(a,b) (((a)<(b))?(a):(b))

struct klink_session {
    struct list resp_queue;
};

struct klink_dgram {
    uint16_t    opcode;
    uint16_t    what;
    uint32_t    item;
    uint16_t    seq;
    uint16_t    size;
} __attribute__((packed));

struct klink_proc_info {
    uint16_t    pid;
    uint16_t    uid;
    uint16_t    gid;
    char        name[256];
};

static int klink_close(struct socket *sock);
static int klink_connect(struct socket *socket, void *address, size_t address_len);
static int klink_init(struct socket *socket, int type, int protocol);
static size_t klink_recv(struct socket *socket, void *buf, size_t size);
static size_t klink_send(struct socket *socket, const void *buf, size_t);

struct socket_ops klink_ops = {
    .accept     = NULL,
    .close      = klink_close,
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
klink_send_proclist(struct klink_session *session)
{
    /*
     * defined in sys/kern/proc.c
     */
    extern struct list process_list;

    list_iter_t iter;

    list_get_iter(&process_list, &iter);

    size_t resp_sz = sizeof(struct klink_dgram) + LIST_SIZE(&process_list)*sizeof(struct klink_proc_info);

    struct klink_dgram *resp = (struct klink_dgram*)calloc(0, resp_sz);
    struct klink_proc_info *procs = (struct klink_proc_info*)(resp + 1);
    struct proc *proc;
    int i = 0;

    while (iter_move_next(&iter, (void**)&proc)) {
        struct klink_proc_info *info = (struct klink_proc_info*)&procs[i++];

        info->pid = proc->pid;
        info->uid = proc->creds.uid;
        info->gid = proc->creds.gid;

        strcpy(info->name, proc->name);
    }

    resp->size = LIST_SIZE(&process_list) * sizeof(struct klink_proc_info);

    list_append(&session->resp_queue, resp);

    iter_close(&iter);

    return 0;
}

static int
klink_query(struct klink_session *session, struct klink_dgram *req)
{
    switch (req->what) {
        case KWHAT_PROCLIST:
            klink_send_proclist(session);
            break;
        case KWHAT_PROC_ARGV:
            break;
    }
    return 0;
}

static int
klink_close(struct socket *sock)
{
    struct klink_session *session = (struct klink_session*)sock->state;

    list_destroy(&session->resp_queue, true);

    free(session);

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
    struct klink_session *session = (struct klink_session*)calloc(0, sizeof(struct klink_session));

    socket->state = session;

    return 0;
}

static size_t
klink_recv(struct socket *sock, void *buf, size_t size)
{
    struct klink_session *session = (struct klink_session*)sock->state;

    if (LIST_SIZE(&session->resp_queue)) {
        struct klink_dgram *resp;

        if (!list_remove_front(&session->resp_queue, (void**)&resp)) {
            return -1;
        }

        size_t minsize = MIN(sizeof(struct klink_dgram) + resp->size, size);

        memcpy(buf, resp, minsize);

        free(resp);

        return minsize;
    }
    return 0;
}

static size_t
klink_send(struct socket *sock, const void *buf, size_t size)
{
    struct klink_session *session = (struct klink_session*)sock->state;
    struct klink_dgram *dgram = (struct klink_dgram*)buf;
    
    switch (dgram->opcode) {
        case KLINK_QUERY:
            klink_query(session, dgram);
            break;
    }

    return size;
}

__attribute__((constructor))
void
init_klink()
{
    register_protocol(&klink_domain);
}
