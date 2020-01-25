#include <errno.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

int
tcdrain(int fd)
{
    return 0;
}

int
tcflush(int fd, int queue_selector)
{
    return 0;
}

int
tcgetattr(int fd, struct termios *buf)
{
    return ioctl(fd, TCGETS, buf);
}

int
tcsetattr(int fd, int mode, struct termios *buf)
{
    switch (mode) {
        case TCSANOW:
            return ioctl(fd, TCSETS, buf);
        case TCSADRAIN:
            return ioctl(fd, TCSETSW, buf);
        case TCSAFLUSH:
            return ioctl(fd, TCSETSF, buf);
        default:
            errno = EINVAL;
            return -1;
    }
}
