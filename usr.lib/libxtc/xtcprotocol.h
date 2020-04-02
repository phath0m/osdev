#ifndef _XTCPROTOCOL_H
#define _XTCPROTOCOL_H

#include <stdint.h>

#define XTC_SOCK_PATH       "/var/run/xtc.sock"

#define XTC_NEXTEVENT       0x01
#define XTC_NEWWIN          0x02
#define XTC_SETWINTITLE     0x03
#define XTC_PUTSTRING       0x04
#define XTC_CLEAR           0x05
#define XTC_PAINT           0x06
#define XTC_OPEN_CANVAS     0x07
#define XTC_RESIZE          0x08

#define XTC_ERR_NOMOREWINDOWS   0x01
#define XTC_ERR_NOSUCHWIN       0x02

struct xtc_msg_hdr {
    uint8_t     opcode;
    uint32_t    parameters[4];
    uint32_t    error;
    uint32_t    payload_size;
};


#endif
