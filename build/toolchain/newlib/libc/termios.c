#include <errno.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

speed_t
cfgetispeed(const struct termios *p)
{
    return 0;
}

speed_t
cfgetospeed(const struct termios * p)
{
    return 0;
}

int
cfsetispeed(struct termios *p, speed_t s)
{
    return 0;
}

int
cfsetospeed(struct termios *p, speed_t s)
{
    return 0;
}

int
tcdrain(int fd)
{
    return 0;
}

int
tcflow(int fd, int action)
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
tcsendbreak(int fd, int duration)
{
    return 0;
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
