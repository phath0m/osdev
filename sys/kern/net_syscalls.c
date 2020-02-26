#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>

static int
sys_accept(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t*, address_len, 2, argv);

    struct file *fp = procdesc_getfile(fd);

    if (!fp) {
        return -(EBADF);
    }

    struct socket *sock = file_to_sock(fp);
    struct socket *client;

    int res = sock_accept(sock, &client, address, address_len);

    if (res == 0) {
        struct file *file = sock_to_file(client);

        return procdesc_newfd(file);
    }

    return res;
}

static int
sys_bind(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, address_len, 2, argv);

    struct file *fp = fp = procdesc_getfile(fd);

    if (!fp) {
        return -(EBADF); 
    }   

    struct socket *sock = file_to_sock(fp);

    return sock_bind(sock, address, address_len);
}

static int
sys_connect(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, address_len, 2, argv);

    struct file *fp = procdesc_getfile(fd);

    if (fp) {
        struct socket *sock = file_to_sock(fp);

        return sock_connect(sock, address, address_len);
    }

    return -(EBADF);
}

static int
sys_socket(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, domain, 0, argv);
    DEFINE_SYSCALL_PARAM(int, type, 1, argv);
    DEFINE_SYSCALL_PARAM(int, protocol, 2, argv);

    struct socket *sock;

    int ret = sock_new(&sock, domain, type, protocol);

    if (ret != 0) {
        return ret;
    }

    struct file *file = sock_to_file(sock);

    return procdesc_newfd(file);
}

__attribute__((constructor))
void
_init_net_syscalls()
{
    register_syscall(SYS_ACCEPT, 3, sys_accept);
    register_syscall(SYS_BIND, 3, sys_bind);
    register_syscall(SYS_CONNECT, 3, sys_connect);
    register_syscall(SYS_SOCKET, 3, sys_socket);
}
