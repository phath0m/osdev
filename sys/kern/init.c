/*
 * init.c - Machine independent kernel initialization
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
#include <ds/dict.h>
#include <sys/cdev.h>
#include <sys/devno.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/vnode.h>
#include <sys/world.h>

/*
 * parse the kernel command-line. Very simple and primative function, nothing
 * fancy supported here.... 
 */
void
parse_cmdline(struct dict *dictp, const char *cmdline)
{
    bool option_end;
    int i;
    int option_count;
    size_t cmdline_len;

    char *opt_name;
    char *opt_val;

    static char cmdline_bak[512];
    static char *options[512];

    option_count = 0;
    option_end = true;
    cmdline_len = strlen(cmdline);

    memset(dictp, 0, sizeof(struct dict));

    if (cmdline_len >= 512) {
        /* it shouldn't be longer than this*/
        printf("kernel command line too big!\n\r");
        return;
    }

    strncpy(cmdline_bak, cmdline, 512);

    for (i = 0; i < cmdline_len; i++) {
        if (cmdline_bak[i] == ' ' || cmdline_bak[i] == ',') {
            cmdline_bak[i] = '\0';
            option_end = true;
        } else if (option_end) {
            options[option_count++] = &cmdline_bak[i];
            option_end = false;
        }
    }

    for (i = 0; i < option_count; i++) {
        opt_name = options[i];
        opt_val = strrchr(options[i], '=');
        
        if (!opt_val) continue;

        *opt_val = '\0';
        opt_val++;
        dict_set(dictp, opt_name, opt_val);
    }
}

bool
parse_uuid(const char *uuid, int buf_len, uint8_t *buf)
{
    int i;
    int j;
    size_t uuid_len;

    char octet_str[3];

    i = 0;
    j = 0;
    uuid_len = strlen(uuid);

    if (uuid_len < buf_len*2) return false;

    while (i < uuid_len && j < buf_len) {
        if (uuid[i] == '-') {
            i++;
            continue;
        }

        octet_str[0] = uuid[i];
        octet_str[1] = uuid[i+1];
        octet_str[2] = 0;

        /* should I check for error here? Yes. Will I? No. */
        buf[j++] = atoi(octet_str, 16);
        i += 2;
    }


    if (j == buf_len) return true;

    return false;
}

/* open up a tmpfs instance and extract the ramdisk to it */
static struct vnode *
initrd_open()
{
    extern void *start_initramfs;
    extern void tar_extract_archive(struct vnode *, struct vnode *, const void *);

    struct vnode *vn;

    if (fs_open(NULL, &vn, "tmpfs", 0) != 0) {
        panic("could not mount tmpfs!");
    }

    vn->mode = 0755;

    current_proc->root = vn;
    current_proc->cwd = vn;

    tar_extract_archive(vn, NULL, start_initramfs);

    return vn;
}

struct vnode *
rootfs_open(const uint8_t *uuid)
{
    extern struct list device_list;

    list_iter_t iter;
    struct cdev *dev;
    //struct vnode *root;
    bool found = false;

    list_get_iter(&device_list, &iter);

    while (iter_move_next(&iter, (void**)&dev)) {
        if (dev->majorno == DEV_MAJOR_RAW_DISK && fs_probe(dev, "ext2", 16, uuid)) {
            found = true;
            break;
        }
    }

    iter_close(&iter);

    if (!found) {
        return NULL;
    }

    /*
    if (fs_open(dev, &root, "ext2", 0) != 0) {
        panic("could not mount rootfs!");
    }
    */
    panic("Fix me");

    //current_proc->root = root;
    //current_proc->cwd = root;

    return NULL;//root;
}

/* stage three of kernel initialization; exec /sbin/doit */
static int
exec_init(const char *args)
{
    const char *init_argv[] = {
        "/sbin/doit",
        args,
        NULL
    };

    const char *init_envp[] = {
        "CONSOLE=/dev/lfb",
        NULL
    };

    printf("kernel: invoke /sbin/doit\n\r");
    current_proc->umask = 0744;
    proc_execve(init_argv[0], init_argv, init_envp);

    return 0;
}

/* stage two of the kernel initialization process; mount ramdisk */
static int
init_thread(void *argp)
{
    extern struct thread *sched_curr_thread;

    char *runlevel;
    char *rootfs_uuid_str;

    uint8_t rootfs_uuid[16];

    struct dict opts;

    struct proc *init;
    struct session *init_session;
    struct pgrp *init_group;
    struct vnode *root;
    struct world *init_world;

    /* setup the first process */    
    init = proc_new();
    init_session = session_new(init);
    init_group = pgrp_new(init, init_session);
    init_world = world_new(init, WF_NONE);

    init->thread = sched_curr_thread;
    init->group = init_group;
    init->world = init_world;

    sched_curr_thread->proc = init;
    current_proc = init;

    parse_cmdline(&opts, argp);

    /* this is ugly and not how I want to do this (Initializing these filesystems
     * here). I'd prefer a more modular approach. We'll fix this some day
     */
    devfs_init();
    tmpfs_init();
    ext2_init();

    /* now mount the ramdisk */
    root = NULL;

    if (dict_get(&opts, "rootfs", (void**)&rootfs_uuid_str)) {
        printf("rootfs: %s\n\r", rootfs_uuid_str);
        if (!parse_uuid(rootfs_uuid_str, 16, rootfs_uuid)) {
            panic("invalid rootfs UUID specified");
        }

        root = rootfs_open(rootfs_uuid);
    }

    if (!root) root = initrd_open();

    root->mode = 0755;

    if (fs_mount(root, NULL, "devfs", "/dev", 0) != 0) {
        panic("could not mount devfs!");
    }

    
    if (fs_mount(root, NULL, "tmpfs", "/var/run", 0) != 0) {
        panic("could not mount tmpfs!");
    }

    if (!dict_get(&opts, "runlevel", (void**)&runlevel)) {
        runlevel = "1";
    }

    exec_init(runlevel);

    return 0;
}

int
kmain(const char *args)
{
    extern void kmsg_device_init();
    extern void pseudo_devices_init();
 
    /* initialize various pools for the various subsystems  */
    pool_init(&proc_pool, sizeof(struct proc), 0);
    pool_init(&thread_pool, sizeof(struct thread), 0);
    pool_init(&vn_pool, sizeof(struct vnode), 0);
    pool_init(&file_pool, sizeof(struct file), 0);

    /* initialize the socket subsystem */
    sock_init();

    /* initialize all system calls*/
    syscalls_init();

    kmsg_device_init();
    pseudo_devices_init();

    thread_run((kthread_entry_t)init_thread, NULL, (void*)args);

    for (;;) {
        thread_yield();
    }
}
