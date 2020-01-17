#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/kernlink.h>
#include <sys/socket.h>

static int
klink_query(int sfd, int what, int item, struct klink_dgram *resp, size_t len)
{
    struct klink_dgram req;

    req.opcode = KLINK_QUERY;
    req.what = what;
    req.item = item;
    req.size = 0;

    if (write(sfd, &req, sizeof(struct klink_dgram)) < 0) {
        return -1;
    }

    int ret = read(sfd, resp, len);

    if (ret < sizeof(struct klink_dgram)) {
        return -1;
    }

    if (resp->what == KWHAT_ERROR) {
        return -1;
    }

    return 0;
}

static int
print_heap_info()
{
    int sfd = socket(AF_KLINK, 0, SOCK_SEQPACKET);

    if (sfd < 0) {
        return -1;
    }

    if (connect(sfd, 0, 0)) {
        return -1;
    }

    struct klink_dgram *resp = (struct klink_dgram*)malloc(65535);

    klink_query(sfd, KWHAT_HEAPSTAT, 0, resp, 65535);

    struct klink_heap_stat *heapinfo = (struct klink_heap_stat*)(resp + 1);

    printf("heap_start       : %p\n", (void*)heapinfo->heap_start);
    printf("heap_break       : %p\n", (void*)heapinfo->heap_break);
    printf("heap_end         : %p\n", (void*)heapinfo->heap_end);
    printf("allocated_blocks : %d\n", heapinfo->blocks_allocated);
    printf("free_blocks      : %d\n", heapinfo->blocks_free);
    printf("vfs_nodes        : %d\n", heapinfo->node_count);
    printf("files            : %d\n", heapinfo->file_count);
    printf("procs            : %d\n", heapinfo->proc_count);
    printf("list_elems       : %d\n", heapinfo->list_elem_count);

    free(resp);

    close(sfd);
    
    return 0;
}

int
main(int argc, char *argv[])
{
    print_heap_info();

    return 0;
}
