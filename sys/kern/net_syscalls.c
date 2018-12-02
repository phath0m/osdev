#include <sys/errno.h>
#include <sys/net.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/types.h>

// remove
#include <stdio.h>

int
sys_connect(int fd, void *address, size_t address_len)
{
    struct file *fp = proc_getfile(fd);
    
    if (fp) {
        struct socket *sock = file_to_sock(fp);

        return sock_connect(sock, address, address_len);
    }
    
    return -(EBADF);
}

int
sys_socket(int domain, int type, int protocol)
{
    struct socket *sock;

    int ret = sock_new(&sock, domain, type, protocol);

    if (ret != 0) {
        return ret;
    }

    struct file *file = sock_to_file(sock);

    return proc_newfildes(file);
}

static int
sys_connect_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, address_len, 2, argv);

    return sys_connect(fd, address, address_len);
}

static int
sys_socket_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, domain, 0, argv);
    DEFINE_SYSCALL_PARAM(int, type, 1, argv);
    DEFINE_SYSCALL_PARAM(int, protocol, 2, argv);

    return sys_socket(domain, type, protocol);
}

__attribute__((constructor)) static void
_init_syscalls()
{
    register_syscall(SYS_CONNECT, 3, sys_connect_handler);
    register_syscall(SYS_SOCKET, 3, sys_socket_handler);
}
