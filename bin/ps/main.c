#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/kernlink.h>
#include <sys/socket.h>
#include "./list.h"

struct ps_entry {
    int     pid;
    int     ppid;
    int     gid;
    int     uid;
    char    cmd[256];
    char    tty[32];
    time_t  stime;
    char *  argv;
    char *  envp;
};

struct ps_options {
    bool    display_all;
    bool    display_extended;
    bool    display_all_terminals;
};

static void
get_time_string(char *buf, size_t buf_size, time_t stime)
{
    time_t now = time(NULL);

    struct tm *tp = gmtime(&stime);

    if (now - stime < 86400) {
        strftime(buf, buf_size, "%R", tp);
    } else {
        strftime(buf, buf_size, "%b%d", tp);
    }
}

static void
ps_print_basic(struct ps_entry *entry)
{
    printf("%5d ", entry->pid);
    printf("%-10s ", entry->tty);
    printf("%s\n", entry->cmd);
}

static void
ps_print_extended(struct ps_entry *entry)
{
    char time_buf[10];

    get_time_string(time_buf, 10, entry->stime);

    struct passwd *pwd = getpwuid(entry->uid);

    if (pwd) {
        printf("%-10s", pwd->pw_name);
    } else {
        printf("%-10s", "?");
    }
    printf("%5d ", entry->pid);
    printf("%5d ", entry->ppid);
    printf("%-6s", time_buf);
    printf("%-10s ", entry->tty);
    printf("%s\n", entry->cmd);
}

static void
ps_print_procs(struct ps_options *options, struct list *listp)
{
    if (options->display_extended) {
        printf("UID         PID  PPID STIME TTY        CMD\n");
    } else {
        printf("  PID TTY        CMD\n");
    }
    
    char *ttydev = ttyname(0);

    const char *actual_tty = strrchr(ttydev, '/') + 1;

    list_iter_t iter;

    list_get_iter(listp, &iter);

    struct ps_entry *entry;

    while (iter_move_next(&iter, (void**)&entry)) {
        if (!options->display_all && strcmp(entry->tty, actual_tty)) {
            continue;
        }

        if (options->display_extended) {
            ps_print_extended(entry);
        } else {
            ps_print_basic(entry);
        }
    }
}

static int
parse_arguments(struct ps_options *options, int argc, char *argv[])
{
    int c;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "aefA")) != -1) {
            switch (c) {
                case 'a':
                    options->display_all_terminals = true;
                    break;
                case 'e':
                case 'A':
                    options->display_all = true;
                    break;
                case 'f':
                    options->display_extended = true;
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
read_procs(struct list *listp)
{
    int sfd = socket(AF_KLINK, 0, SOCK_SEQPACKET);

    if (sfd < 0) {
        return -1;
    }

    if (connect(sfd, 0, 0)) {
        return -1;
    }

    struct klink_dgram *proclist = (struct klink_dgram*)malloc(65535);

    klink_query(sfd, KWHAT_PROCLIST, 0, proclist, 65535);

    int nprocs = proclist->size / sizeof(struct klink_proc_info);

    struct klink_proc_info *procs = (struct klink_proc_info*)(proclist + 1);

    for (int i = 0; i < nprocs; i++) {
        struct klink_proc_info *proc = &procs[i];

        struct ps_entry *entry = (struct ps_entry*)malloc(sizeof(struct ps_entry));
        entry->pid = proc->pid;
        entry->ppid = proc->ppid;
        entry->gid = proc->gid;
        entry->uid = proc->uid;

        struct klink_dgram pstat[sizeof(struct klink_dgram)+sizeof(struct klink_proc_stat)];
        klink_query(sfd, KWHAT_PROCSTAT, entry->pid, pstat, sizeof(pstat));

        struct klink_proc_stat *stat = (struct klink_proc_stat*)(pstat + 1);
        entry->stime = stat->stime;
        strncpy(entry->cmd, stat->cmd, 255);
        strncpy(entry->tty, stat->tty, 32);

        list_append(listp, entry);
    }

    free(proclist);

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

    struct list proclist;

    memset(&proclist, 0, sizeof(proclist));

    read_procs(&proclist);

    ps_print_procs(&options, &proclist);

    list_destroy(&proclist, true);

    return 0;
}
