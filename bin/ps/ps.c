#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysctl.h>

#define PS_DISPLAY_ALL      0x01
#define PS_DISPLAY_ALL_TTYS 0x02
#define PS_FORMAT_EXTENDED  0x04
#define PS_FORMAT_JOB_CTL   0x08
#define PS_FORMAT(options) ((options) & 0xC)

static void
get_time_string(char *buf, size_t buf_size, time_t stime)
{
    time_t now;
    struct tm *tp;

    now = time(NULL);
    tp = gmtime(&stime);

    if (now - stime < 86400) {
        strftime(buf, buf_size, "%R", tp);
    } else {
        strftime(buf, buf_size, "%b%d", tp);
    }
}

static void
ps_print_basic(struct kinfo_proc *entry)
{
    printf("%5d %-10s %s\n", entry->pid, entry->tty, entry->cmd);
}

static void
ps_print_extended(struct kinfo_proc *entry)
{
    char time_buf[10];

    struct passwd *pwd;

    get_time_string(time_buf, 10, entry->stime);

    pwd = getpwuid(entry->uid);

    if (pwd) {
        printf("%-10s", pwd->pw_name);
    } else {
        printf("%-10s", "?");
    }
    
    printf("%5d %5d %-6s%-10s %s\n", entry->pid, entry->ppid, time_buf, entry->tty, entry->cmd);
}

static void
ps_print_job_ctl(struct kinfo_proc *entry)
{
    printf("%5d %5d %5d %-10s %s\n", entry->pid, entry->pgid, entry->sid, entry->tty, entry->cmd);
}

static void
ps_print_procs(int options, struct kinfo_proc *procs, int nprocs)
{
    bool show_all;
    int i;
    pid_t sid;

    switch (PS_FORMAT(options)) {
        case PS_FORMAT_EXTENDED:
            puts("UID         PID  PPID STIME TTY        CMD");
            break;
        case PS_FORMAT_JOB_CTL:
            puts("  PID  PGID   SID TTY        CMD");
            break;
        default:
            puts("  PID TTY        CMD");
            break;
    }

    sid = getsid(0);
    show_all = (options & PS_DISPLAY_ALL) != 0;

    for (i = 0; i < nprocs; i++) {
        struct kinfo_proc *entry;
        
        entry = &procs[i];

        if (!show_all && entry->sid != sid) {
            continue;
        }

        switch (PS_FORMAT(options)) {
            case PS_FORMAT_EXTENDED:
                ps_print_extended(entry);
                break;
            case PS_FORMAT_JOB_CTL:
                ps_print_job_ctl(entry);
                break;
            default:
                ps_print_basic(entry);
                break;
        }
    }
}

int
main(int argc, char *argv[])
{
    int c;
    int options;
    int nprocs;
    size_t bufsize;

    struct kinfo_proc *procs;

    int oid[3];

    options = 0;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "aefjA")) != -1) {
            switch (c) {
                case 'a':
                    options |= PS_DISPLAY_ALL_TTYS;
                    break;
                case 'e':
                case 'A':
                    options |= PS_DISPLAY_ALL;
                    break;
                case 'f':
                    options |= PS_FORMAT_EXTENDED;
                    break;
                case 'j':
                    options |= PS_FORMAT_JOB_CTL;
                    break;
                case '?':
                    return -1;
            }
        }
    }

    oid[0] = CTL_KERN;
    oid[1] = KERN_PROC;
    oid[2] = KERN_PROC_ALL;

    bufsize = 0;

    if (sysctl(oid, 3, NULL, &bufsize, NULL, 0)) {
        perror("sysctl");
        return -1;
    }

    procs = calloc(1, bufsize);

    if (sysctl(oid, 3, procs, &bufsize, NULL, 0)) {
        perror("sysctl");
        return -1;
    }

    nprocs = bufsize / sizeof(struct kinfo_proc);

    ps_print_procs(options, procs, nprocs);

    free(procs);

    return 0;
}
