#include <unistd.h>
#include <sys/syscalls.h>
#include <sys/types.h>

void
thread_pause()
{
    asm volatile("int $0x80" : : "a"(SYS_THREAD_SLEEP));
}

int
thread_signal(pid_t tid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_THREAD_WAKE), "b"(tid));

    if (ret < 0) {
        return -1;
    }

    return ret;
}
