/*
 * klink.c - Kernel link protocol
 *
 * This is a weird thing I came up with in lieu of having procfs. Basically,
 * this is a simple request/response driven protocol that can be used to query
 * information from kernel space. Sort of inspired by Linux's netlink sockets, 
 * but radically different in terms of use/purpose
 */
#include <ds/list.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/string.h>
#include <sys/types.h>

#define AF_KLINK    40

#define KERROR_NOENT    0x01

#define KLINK_QUERY     0x01
#define KWHAT_PROCLIST  0x02
#define KWHAT_PROCSTAT  0x03
#define KWHAT_ENVIRON   0x04
#define KWHAT_ERROR     0x05
#define KWHAT_HEAPSTAT  0x06

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
    uint16_t    ppid;
    uint16_t    uid;
    uint16_t    gid;
} __attribute__((packed));

struct klink_proc_stat {
    char        cmd[256];
    char        tty[32];
    uint32_t    stime;
} __attribute__((packed));

struct klink_heap_stat {
    uint32_t    heap_break;
    uint32_t    heap_start;
    uint32_t    heap_end;
    uint32_t    blocks_allocated;
    uint32_t    blocks_free;
    uint32_t    node_count;
    uint32_t    file_count;
    uint32_t    proc_count;
    uint32_t    list_elem_count;
    uint32_t    page_block_count;
    uint32_t    page_table_count;
    uint32_t    vm_block_count;
    uint32_t    vm_space_count;
} __attribute__((packed));

static int klink_connect(struct socket *socket, void *address, size_t address_len);
static int klink_destroy(struct socket *sock);
static int klink_init(struct socket *socket, int type, int protocol);
static size_t klink_recv(struct socket *socket, void *buf, size_t size);
static size_t klink_send(struct socket *socket, const void *buf, size_t);

struct socket_ops klink_ops = {
    .accept     = NULL,
    .close      = NULL,
    .connect    = klink_connect,
    .destroy    = klink_destroy,
    .init       = klink_init,
    .recv       = klink_recv,
    .send       = klink_send
};

struct protocol klink_domain = {
    .address_family = AF_KLINK,
    .ops            = &klink_ops
};

static int
klink_send_heapstat(struct klink_session *session)
{
    size_t resp_sz = sizeof(struct klink_dgram) + sizeof(struct klink_heap_stat);

    struct klink_dgram *resp = (struct klink_dgram*)calloc(1, resp_sz);
    struct klink_heap_stat *heapstat = (struct klink_heap_stat*)(resp + 1);

    resp->what = KWHAT_HEAPSTAT;
    resp->size = sizeof(struct klink_heap_stat);

    /* defined in sys/libk/malloc.c */
    extern intptr_t kernel_break;
    extern intptr_t kernel_heap_end;
    extern intptr_t kernel_heap_start;
    extern int kernel_heap_allocated_blocks;
    extern int kernel_heap_free_blocks;

    /* defined in sys/kern/vfs.c */
    extern int vfs_node_count;
    extern int vfs_file_count;

    /* defined in sys/kern/proc.c */
    extern int proc_count;

    /* defined in sys/libk/list.c */
    extern int list_elem_count;

    heapstat->heap_start = kernel_heap_start;
    heapstat->heap_break = kernel_break;
    heapstat->heap_end = kernel_heap_end;
    heapstat->blocks_allocated = kernel_heap_allocated_blocks;
    heapstat->blocks_free = kernel_heap_free_blocks;
    heapstat->node_count = vfs_node_count;
    heapstat->file_count = vfs_file_count;
    heapstat->proc_count = proc_count;
    heapstat->list_elem_count = list_elem_count;

    heapstat->page_block_count = vm_stat.frame_count;
    heapstat->page_table_count = vm_stat.page_table_count;
    heapstat->vm_block_count = vm_stat.page_count;
    heapstat->vm_space_count = vm_stat.vmspace_count;
    list_append(&session->resp_queue, resp);

    return 0;
}

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

        if (proc->parent) {
            info->ppid = proc->parent->pid;
        }
    }
    resp->size = LIST_SIZE(&process_list) * sizeof(struct klink_proc_info);

    list_append(&session->resp_queue, resp);

    iter_close(&iter);

    return 0;
}

static int
klink_send_procstat(struct klink_session *session, int target)
{
    /*
     * defined in sys/kern/proc.c
     */
    extern struct list process_list;
    
    list_iter_t iter;
    
    list_get_iter(&process_list, &iter);

    struct klink_dgram *resp = NULL;
    struct proc *proc;
    
    while (iter_move_next(&iter, (void**)&proc)) {
        if (proc && proc->pid == target) {
            size_t resp_sz = sizeof(struct klink_dgram) + sizeof(struct klink_proc_stat);

            resp = (struct klink_dgram*)(calloc(0, resp_sz));

            struct klink_proc_stat *stat = (struct klink_proc_stat*)(resp + 1);
            
            stat->stime = proc->start_time;
            char *tty = proc_getctty(proc);
            strncpy(stat->cmd, proc->name, 255);
            
            if (tty) {
                strncpy(stat->tty, tty, 31);
            } else {
                stat->tty[0] = '\x00';
            }

            break;
        }
    }

    iter_close(&iter);

    if (resp) {
        resp->what = KWHAT_PROCSTAT;
        resp->size = sizeof(struct klink_proc_stat);
    } else {
        resp->what = KWHAT_ERROR;
        resp->item = KERROR_NOENT;
        resp->size = 0;
    }

    list_append(&session->resp_queue, resp);

    return 0;
}

static int
klink_query(struct klink_session *session, struct klink_dgram *req)
{
    switch (req->what) {
        case KWHAT_PROCLIST:
            klink_send_proclist(session);
            break;
        case KWHAT_PROCSTAT:
            klink_send_procstat(session, req->item);
            break;
        case KWHAT_HEAPSTAT:
            klink_send_heapstat(session);
            break;
    }
    return 0;
}

static int
klink_destroy(struct socket *sock)
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
_init_klink()
{
    register_protocol(&klink_domain);
}
