/*
 * vfs_syscalls.c
 *
 * This file is responsible for implementing generic filesystem system calls. 
 * Pretty much all system calls that actually perform I/O to files are here.
 *
 * It should be noted that things that manipulate file descriptors (IE: dup)
 * are in proc_syscalls.c, since they don't actually perform I/O, rather, they
 * just manipulate the process descriptor's file descriptor table
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
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/interrupt.h>
#include <sys/mount.h>
#include <sys/pipe.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>

static int
sys_access(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("access", "\"%s\", %d", path, mode);

    return vfs_access(current_proc, path, mode);
}

static int
sys_chmod(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("chmod", "\"%s\", %d", path, mode);

    return vfs_chmod(current_proc, path, mode);
}

static int
sys_chown(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(uid_t, owner, 1, argv);
    DEFINE_SYSCALL_PARAM(gid_t, group, 2, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("chown", "\"%s\", %d, %d", path, owner, group);

    return vfs_chown(current_proc, path, owner, group);
}

static int
sys_close(struct thread *th, syscall_args_t argv)
{
    struct file *fp;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);

    TRACE_SYSCALL("close", "%d", fd);

    fp = procdesc_getfile(fd);

    if (fp) {
        current_proc->files[fd] = NULL;

        fop_close(fp);

        return 0;
    }

    return -(EBADF);
}

static int
sys_creat(struct thread *th, syscall_args_t argv)
{
    int fd;
    int succ;
    struct file *file;

    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);
    
    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("creat", "\"%s\", %d", path, mode);

    mode &= current_proc->umask;

    succ = vfs_creat(current_proc, &file, path, mode);

    if (succ == 0) {
        fd = procdesc_getfd();

        if (fd < 0) {
            fop_close(file);
        } else {
            current_proc->files[fd] = file;
        }

        return fd;
    }

    return -(succ);
}

static int
sys_fchmod(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);

    TRACE_SYSCALL("fchmod", "%d, %d", fd, mode);

    file = procdesc_getfile(fd);

    if (file) {
        return fop_fchmod(file, mode);
    }

    return -(EBADF);
}

static int
sys_fchown(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(uid_t, owner, 1, argv);
    DEFINE_SYSCALL_PARAM(gid_t, group, 2, argv);

    TRACE_SYSCALL("fchown", "%d, %d, %d", fd, owner, group);

    file = procdesc_getfile(fd);

    if (file) {
        return fop_fchown(file, owner, group);
    }

    return -(EBADF);
}

static int
sys_ftruncate(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, length, 1, argv);

    TRACE_SYSCALL("ftruncate", "%d, 0x%p", fd, length);

    file = procdesc_getfile(fd);

    if (file) {
        return fop_ftruncate(file, length);
    }

    return -(EBADF);
}

static int
sys_ioctl(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(uint32_t, request, 1, argv);
    DEFINE_SYSCALL_PARAM(void*, arg, 2, argv);

    TRACE_SYSCALL("ioctl", "%d, %d, %p", fd, request, arg);

    file = procdesc_getfile(fd);

    if (file) {
        bus_interrupts_on();
        return fop_ioctl(file, (uint64_t)request, arg);
    }

    return -(EBADF);
}

static int
sys_fstat(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    if (vm_access(th->address_space, buf, sizeof(struct stat), VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("fstat", "%d, %p", fd, buf);

    file = procdesc_getfile(fd);

    if (file) {
        return fop_stat(file, buf);
    }

    return -(EBADF);
}

static int
sys_open(struct thread *th, syscall_args_t argv)
{
    int fd;
    int succ;

    struct file *file;

    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("open", "\"%s\", %d", path, mode);

    succ = vfs_open_r(current_proc, &file, path, mode);

    if (succ == 0) {
        fd = procdesc_getfd();

        if (fd < 0) {
            fop_close(file);
        } else {
            current_proc->files[fd] = file;
        }

        return fd;
    }

    return succ;
}

static int
sys_pipe(struct thread *th, syscall_args_t argv)
{
    struct file *files[2];

    DEFINE_SYSCALL_PARAM(int *, pipefd, 0, argv);

    if (vm_access(th->address_space, pipefd, sizeof(int[2]), VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("pipe", "%p", pipefd);

    create_pipe(files, NULL);

    pipefd[0] = procdesc_newfd(files[0]);
    pipefd[1] = procdesc_newfd(files[1]);

    return 0;
}

static int
sys_read(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char*, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, nbyte, 2, argv);

    if (vm_access(th->address_space, buf, nbyte, VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("read", "%d, %p, %d", fildes, buf, nbyte);

    bus_interrupts_on();

    file = procdesc_getfile(fildes);

    if (file) {
        return fop_read(file, buf, nbyte);
    }

    return -(EBADF);
}

static int
sys_readdir(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(struct dirent *, dirent, 1, argv);

    if (vm_access(th->address_space, dirent, sizeof(struct dirent), VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("readdir", "%d, %p", fildes, dirent);

    file = procdesc_getfile(fildes);

    if (file) {
        return fop_readdirent(file, dirent);
    }

    return -(EBADF);
}

static int
sys_rmdir(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("rmdir", "\"%s\"", path);

    return vfs_rmdir(current_proc, path);
}

static int
sys_lseek(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, offset, 1, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 2, argv);

    TRACE_SYSCALL("lseek", "%d, %d, %d", fd, offset, whence);

    file = procdesc_getfile(fd);

    if (file) {
        return fop_seek(file, offset, whence);
    }

    return -(EBADF);
}

static int
sys_lseek64(struct thread *th, syscall_args_t argv)
{
    uint64_t offset;
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(uint64_t, offset_low, 1, argv);
    DEFINE_SYSCALL_PARAM(uint64_t, offset_high, 2, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 3, argv);

    offset = (offset_low | (offset_high << 32));

    TRACE_SYSCALL("lseek64", "%d, %d, %d, %d", fd, offset_low, offset_high, whence);

    file = procdesc_getfile(fd);

    if (file) {
        return fop_seek(file, offset, whence);
    }

    return -(EBADF);
}

static int
sys_mkdir(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);
   
    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("mkdir", "\"%s\", %x", path, mode);

    mode &= current_proc->umask;

    return vfs_mkdir(current_proc, path, mode);
}

static int
sys_mknod(struct thread *th, syscall_args_t argv)
{
    mode_t extra;

    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);
    DEFINE_SYSCALL_PARAM(dev_t, dev, 2, argv);
    
    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("mknod", "\"%s\", %x, %d", path, mode, dev);

    extra = mode & ~(0777);
    mode &= current_proc->umask;
    mode |= extra;

    return vfs_mknod(current_proc, path, mode, dev);
}

static int
sys_mount(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, source, 0, argv);
    DEFINE_SYSCALL_PARAM(const char *, target, 1, argv);
    DEFINE_SYSCALL_PARAM(const char *, fstype, 2, argv);
    DEFINE_SYSCALL_PARAM(uint32_t, mountflags, 3, argv);
  
    if (source && vm_access(th->address_space, source, 1, VM_READ)) {
        return -(EFAULT);
    }

    if (target && vm_access(th->address_space, target, 1, VM_READ)) {
        return -(EFAULT);
    }

    if (fstype && vm_access(th->address_space, fstype, 1, VM_READ)) {
        return -(EFAULT);
    }

    return fs_mount(current_proc->root, source, fstype, target, mountflags);
}

static int
sys_stat(struct thread *th, syscall_args_t argv)
{
    int res;

    struct file *file;

    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    if (vm_access(th->address_space, buf, sizeof(struct stat), VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("stat", "\"%s\", %p", path, buf);

    res = vfs_open_r(current_proc, &file, path, O_RDONLY);

    if (res == 0) {
        res = fop_stat(file, buf);

        fop_close(file);
    }

    return res;
}

static int
sys_truncate(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, length, 1, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("truncate", "\"%s\", %d", path, length);

    return vfs_truncate(current_proc, path, length);
}

static int
sys_unlink(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("unlink", "\"%s\"", path);

    return vfs_unlink(current_proc, path);
}

static int
sys_utimes(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct timeval *, tv, 1, argv);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    if (vm_access(th->address_space, tv, sizeof(struct timeval), VM_READ)) {
        return -(EFAULT);
    }

    return vfs_utimes(current_proc, path, tv);
}

static int
sys_write(struct thread *th, syscall_args_t argv)
{
    struct file *file;

    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(void *, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, nbyte, 2, argv);

    TRACE_SYSCALL("write", "%d, %p, %d", fildes, buf, nbyte);

    if (vm_access(th->address_space, buf, nbyte, VM_READ)) {
        return -(EFAULT);
    }

    bus_interrupts_on();

    file = procdesc_getfile(fildes);

    if (file) {
        return fop_write(file, buf, nbyte);
    }

    return -(EBADF);
}

void
vfs_syscalls_init()
{
    register_syscall(SYS_CLOSE, 1, sys_close);
    register_syscall(SYS_CREAT, 2, sys_creat);
    register_syscall(SYS_CHMOD, 2, sys_chmod);
    register_syscall(SYS_FCHMOD, 2, sys_fchmod);
    register_syscall(SYS_FSTAT, 2, sys_fstat);
    register_syscall(SYS_IOCTL, 3, sys_ioctl);
    register_syscall(SYS_LSEEK, 3, sys_lseek);
    register_syscall(SYS_OPEN, 2, sys_open);
    register_syscall(SYS_PIPE, 1, sys_pipe);
    register_syscall(SYS_READ, 3, sys_read);
    register_syscall(SYS_READDIR, 2, sys_readdir);
    register_syscall(SYS_RMDIR, 1, sys_rmdir);
    register_syscall(SYS_MKDIR, 2, sys_mkdir);
    register_syscall(SYS_STAT, 2, sys_stat);
    register_syscall(SYS_UNLINK, 1, sys_unlink);
    register_syscall(SYS_WRITE, 3, sys_write);
    register_syscall(SYS_CHOWN, 3, sys_chown);
    register_syscall(SYS_FCHOWN, 3, sys_fchown);
    register_syscall(SYS_TRUNCATE, 2, sys_truncate);
    register_syscall(SYS_FTRUNCATE, 2, sys_ftruncate);
    register_syscall(SYS_ACCESS, 2, sys_access);
    register_syscall(SYS_MKNOD, 3, sys_mknod);
    register_syscall(SYS_UTIMES, 2, sys_utimes);
    register_syscall(SYS_MOUNT, 4, sys_mount);
    register_syscall(SYS_LSEEK64, 4, sys_lseek64);
}
