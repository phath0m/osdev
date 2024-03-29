/*
 * pty_syscalls.c - Terminal related system calls
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
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vnode.h>

static int
sys_isatty(struct thread *th, syscall_args_t argv)
{
    struct cdev *tty;
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);

    TRACE_SYSCALL("isatty", "%d", fildes);

    file = procdesc_getfile(fildes);

    if (!file) {
        return -(EBADF);
    }

    if (FOP_GETDEV(file, &tty) != 0) {
        return -(ENOTTY);
    }

    if (!CDEVOPS_ISATTY(tty)) {
        return -(ENOTTY);
    }

    return 0;
}

static int
sys_ttyname(struct thread *th, syscall_args_t argv)
{
    struct cdev *tty;
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char *, buf, 1, argv);

    TRACE_SYSCALL("ttyname", "%d, 0x%p", fildes, buf);

    file = procdesc_getfile(fildes);

    if (!file) {
        return -(EBADF);
    }

    if (FOP_GETDEV(file, &tty) != 0) {
        return -(ENOTTY);
    }

    if (!CDEVOPS_ISATTY(tty)) {
        return -(ENOTTY);
    }

    // this is nasty... we're ignoring the length of buf and
    // also assuming its mounted to /dev. this is just a temporary
    // hack...
    sprintf(buf ,"/dev/%s", tty->name);

    return 0; 
}

static int
sys_mkpty(struct thread *th, syscall_args_t argv)
{
    extern struct file *mkpty();

    int fd;
    struct file *fp;

    TRACE_SYSCALL("mkpty", "void");

    fp = mkpty();

    if (!fp) {
        return -1;
    }
    
    fd = procdesc_getfd();

    if (fd < 0) {
        file_close(fp);
        return -1;
    } else {
        current_proc->files[fd] = fp;
    }

    return fd;
}

void
pty_syscalls_init()
{
    register_syscall(SYS_ISATTY, 1, sys_isatty);
    register_syscall(SYS_MKPTY, 0, sys_mkpty);
    register_syscall(SYS_TTYNAME, 3, sys_ttyname);
}
