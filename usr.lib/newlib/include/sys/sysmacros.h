#ifndef _SYS_SYSMACROS_H
#define _SYS_SYSMACROS_H


#define minor(n) (n & 0xFF)
#define major(n) ((n >> 8) & 0xFF)
#define makedev(maj, min) (min | (maj << 8))

#endif
