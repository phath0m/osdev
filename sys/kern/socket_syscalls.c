/*
 * socket_syscalls.c - Socket related system calls
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
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>

static int
sys_accept(struct thread *th, syscall_args_t argv)
{
    int res;
    struct file *file;
    struct file *fp;
    struct socket *sock;
    struct socket *client;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t*, address_len, 2, argv);

    fp = procdesc_getfile(fd);

    if (!fp) {
        return -(EBADF);
    }

    sock = file_to_sock(fp);

    res = SOCK_ACCEPT(sock, &client, address, address_len);

    if (res == 0) {
        file = sock_to_file(client);

        return procdesc_newfd(file);
    }

    return res;
}

static int
sys_bind(struct thread *th, syscall_args_t argv)
{
    struct file *fp;
    struct socket *sock;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, address_len, 2, argv);

    fp = procdesc_getfile(fd);

    if (!fp) {
        return -(EBADF); 
    }   

    sock = file_to_sock(fp);

    return SOCK_BIND(sock, address, address_len);
}

static int
sys_connect(struct thread *th, syscall_args_t argv)
{
    struct file *fp;
    struct socket *sock;

    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, address_len, 2, argv);

    fp = procdesc_getfile(fd);

    if (fp) {
        sock = file_to_sock(fp);

        return SOCK_CONNECT(sock, address, address_len);
    }

    return -(EBADF);
}

static int
sys_socket(struct thread *th, syscall_args_t argv)
{
    int ret;
    struct file *file;
    struct socket *sock;

    DEFINE_SYSCALL_PARAM(int, domain, 0, argv);
    DEFINE_SYSCALL_PARAM(int, type, 1, argv);
    DEFINE_SYSCALL_PARAM(int, protocol, 2, argv);

    ret = sock_new(&sock, domain, type, protocol);

    if (ret != 0) {
        return ret;
    }

    file = sock_to_file(sock);

    return procdesc_newfd(file);
}

void
socket_syscalls_init()
{
    register_syscall(SYS_ACCEPT, 3, sys_accept);
    register_syscall(SYS_BIND, 3, sys_bind);
    register_syscall(SYS_CONNECT, 3, sys_connect);
    register_syscall(SYS_SOCKET, 3, sys_socket);
}
