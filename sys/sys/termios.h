/*
 * termios.h
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
#ifndef _ELYSIUM_SYS_TERMIOS_H
#define _ELYSIUM_SYS_TERMIOS_H
#ifdef __cplusplus
extern "C" {
#endif

/* input modes */
#define BRKINT          0x00000002
#define IGNCR           0x00000080
#define IGNPAR          0x00000004
#define INLCR           0x00000040
#define INPCK           0x00000010
#define ISTRIP          0x00000020
#define IUCLC           0x00001000
#define IXANY           0x00000800
#define IXOFF           0x00000400
#define ICRNL           0x00000100
#define IXON            0x00000200
#define PARMRK          0x00000008

/* output modes */
#define OPOST           0x00000001
#define OLCUC           0x00000020
#define ONLCR           0x00000002
#define ONOCR           0x00000040  
#define ONLRET          0x00000080

/* baud rates */
#define B0              0
#define B50             50
#define B75             75
#define B110            110
#define B134            134
#define B150            150
#define B200            200
#define B300            300
#define B600            600
#define B1200           1200
#define B1800           1800
#define B2400           2400
#define B4800           4800
#define B9600           9600
#define B19200          19200
#define B38400          38400

#define CSIZE           0x00000300
#define CS5             0x00000000
#define CS6             0x00000100
#define CS7             0x00000200
#define CS8             0x00000300

#define CSTOPB          0x00000400
#define CREAD           0x00000800
#define PARENB          0x00001000
#define PARODD          0x00002000
#define HPCL            0x00004000
#define CLOCAL          0x00004000

/* local modes */
#define ECHO            0x00000008
#define ECHOE           0x00000002
#define ECHOK           0x00000004
#define ECHONL          0x00000010
#define ICANON          0x00000100
#define ISIG            0x00000080
#define IEXTEN          0x00000400
#define NOFLSH          0x80000000
#define TOSTOP          0x00400000
#define XCASE           0x01000000

#define VMIN            16
#define VTIME           17

#define NCCS            20

typedef unsigned int    tcflag_t;
typedef unsigned char   cc_t;

struct termios {
    tcflag_t    c_iflag;
    tcflag_t    c_oflag;
    tcflag_t    c_cflag;
    tcflag_t    c_lflag;
    cc_t        c_cc[NCCS];
};


#define TCSANOW     0x00
#define TCSADRAIN   0x01
#define TCSAFLUSH   0x02

/* these are the ioctl's to set the attributes */
#define TCGETS      0x00
#define TCSETS      0x01
#define TCSETSW     0x02
#define TCSETSF     0x03

#define TIOCGWINSZ  0x04
#define TIOCSWINSZ  0x05
#define TIOCSCTTY   0x06
#define TIOCSPGRP   0x07
#define TIOCGPGRP   0x08

#ifndef __KERNEL__
speed_t     cfgetispeed(const struct termios *);
speed_t     cfgetospeed(const struct termios *);
int         cfsetispeed(struct termios *, speed_t);
int         cfsetospeed(struct termios *, speed_t);
int         tcdrain(int);
int         tcflow(int, int);
int         tcflush(int, int);
int         tcgetattr(int, struct termios *);
pid_t       tcgetsid(int);
int         tcsendbreak(int, int);
int         tcsetattr(int, int, struct termios *);
#endif /* __KERNEL__*/

#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_TERMIOS_H */
