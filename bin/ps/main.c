#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/kernlink.h>
#include <sys/socket.h>

struct ps_entry {
    int     pid;
    int     ppid;
    int     gid;
    int     uid;
    char    cmd[256];
    char    tty[32];
    char *  argv;
    char *  envp;
};

struct ps_options {
    bool    display_all;
    bool    display_all_terminals;
};

static void
ps_print_procs(struct ps_entry **entries, int count)
{
    printf("  PID TTY        CMD\n");
    for (int i = 0; i < count; i++) {
        struct ps_entry *entry = entries[i];
        printf("%5d ", entry->pid);
        printf("%-10s ", entry->tty);
        printf("%s\n", entry->cmd);
    }
}

static int
parse_arguments(struct ps_options *options, int argc, char *argv[])
{
    int c;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "aA")) != -1) {
            switch (c) {
                case 'a':
                    options->display_all_terminals = true;
                    break;
                case 'A':
                    options->display_all = true;
                    break;
                case '?':
                    return -1;
            }
        }
    }
    
    return 0;
}

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
read_procs(struct ps_options *options, struct ps_entry **entries, int max)
{
    int sfd = socket(AF_KLINK, 0, SOCK_SEQPACKET);

    if (sfd < 0) {
        return -1;
    }

    if (connect(sfd, 0, 0)) {
        return -1;
    }

    struct klink_dgram *resp = (struct klink_dgram*)malloc(65535);

    klink_query(sfd, KWHAT_PROCLIST, 0, resp, 65535);

    int nprocs = resp->size / sizeof(struct klink_proc_info);

    struct klink_proc_info *procs = (struct klink_proc_info*)(resp + 1);

    for (int i = 0; i < nprocs; i++) {
        struct klink_proc_info *proc = &procs[i];

        struct ps_entry *entry = (struct ps_entry*)malloc(sizeof(struct ps_entry));
        entry->pid = proc->pid;
        entry->gid = proc->gid;
        entry->uid = proc->uid;

        entries[i] = entry;
    }

    for (int i = 0; i < nprocs; i++) {
        struct ps_entry *entry = entries[i];

        klink_query(sfd, KWHAT_PROCSTAT, entry->pid, resp, 65535);

        struct klink_proc_stat *stat = (struct klink_proc_stat*)(resp + 1);

        strncpy(entry->cmd, stat->cmd, 255);
        strncpy(entry->tty, stat->tty, 32);
    }

    free(resp);

    close(sfd);

    return nprocs;
}

int
main(int argc, char *argv[])
{
    struct ps_options options;

    memset(&options, 0, sizeof(struct ps_options));

    if (parse_arguments(&options, argc, argv)) {
        return -1;
    }

    struct ps_entry *entries[512];

    int count = read_procs(&options, entries, 512);

    ps_print_procs(entries, count);

    return 0;
}
