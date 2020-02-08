#ifndef _SYS_KERNLINK_H
#define _SYS_KERNLINK_H

#include <stdint.h>

#define KERROR_NOENT    0x01

#define KLINK_QUERY     0x01
#define KWHAT_PROCLIST  0x02
#define KWHAT_PROCSTAT  0x03
#define KWHAT_ENVIRON   0x04
#define KWHAT_ERROR     0x05
#define KWHAT_HEAPSTAT  0x06

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

#endif
