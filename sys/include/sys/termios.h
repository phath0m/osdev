#ifndef _SYS_TERMIOS_H
#define _SYS_TERMIOS_H

#define BRKINT          0x00000002
#define INPCK           0x00000010
#define ISTRIP          0x00000020
#define ICRNL           0x00000100
#define IXON            0x00000200

#define OPOST           0x00000001

#define CS8             0x00000300

#define ECHO            0x00000008
#define ICANON          0x00000100
#define ISIG            0x00000080
#define IEXTEN          0x00000400


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

#endif
