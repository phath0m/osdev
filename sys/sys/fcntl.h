#ifndef _SYS_FCNTL_H
#define _SYS_FCTNL_H

#define F_DUPFD     0x00
#define F_GETFD     0x01
#define F_SETFD     0x02

#define FD_CLOEXEC  0x01

#define O_RDONLY    0x00
#define O_WRONLY    0x01
#define O_RDWR      0x02
#define O_APPEND    0x08

#define O_CREAT     0x0200

#define O_CLOEXEC   0x80000

#endif
