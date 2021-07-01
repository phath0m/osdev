#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "libvt.h"

extern int mkpty();


#define TERM_BUF_SIZE   1024

struct termstate {
    int     ptm; /* file descriptor to master pseudo-terminal */
    bool    csi_initiated; /* Should be processing a CSI sequence */
    bool    custom_seq_initiated; /* Should be processing a custom CSI sequence */
    bool    escape_initiated; /* Should we be processing an escape character ? */
    char    csi_buf[256]; /* buffer for CSI sequences */
    int     csi_len; /* length of CSI buf */
    int     custom_seq_len; /* how many characters should we read for this custom sequence */
    char    buf[TERM_BUF_SIZE]; /* buffer of data to print; used to avoid excessive write() calls */
    int     buf_len; /* length of buf*/
    int     width;
    int     height;
    vtemu_t *  emu;
};

/*
 * SGR - Select Graphic Rendition; sets display attributes
 */
static void
set_sgr_parameter(struct termstate *state, int param)
{
    switch (param) {
        case 0:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_DEF_FOREGROUND, 0);
            VTOPS_SET_ATTR(state->emu, VT_ATTR_DEF_BACKGROUND, 0);
            break;
        case 7:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_FOREGROUND, 0);
            VTOPS_SET_ATTR(state->emu, VT_ATTR_BACKGROUND, 7);
            break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_FOREGROUND, param - 30);
            break;    

        case 39:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_DEF_FOREGROUND, 0);
            break;
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_BACKGROUND, param - 40);
            break;
        case 49:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_DEF_BACKGROUND, 0);
            break;
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
            VTOPS_SET_ATTR(state->emu, VT_ATTR_FOREGROUND, param - 82);
            break;    
        default:
            printf("got uknown SGR parameter %d\n", param);
            break;
    }
}

static void
select_graphic_rendition(struct termstate *state, int args[], int argc)
{
    for (int i = 0; i < argc; i++) {
        set_sgr_parameter(state, args[i]);
    }
}

static void
cursor_position(struct termstate *state, int args[], int argc)
{
    if (argc == 2) {
        VTOPS_SET_CURSOR(state->emu, args[1]-1, args[0]-1);
    } else {
        VTOPS_SET_CURSOR(state->emu, 0, 0);
    }
}

static void
device_status_report(struct termstate *state, int args[], int argc)
{
    if (argc == 1 && args[0] == 6) {
        int cursor_x, cursor_y;

        VTOPS_GET_CURSOR(state->emu, &cursor_x, &cursor_y);

        char buf[16];
        sprintf(buf, "\x1B[%d;%dR", cursor_y+1, cursor_x+1);

        write(state->ptm, buf, strlen(buf));
    }
}

static void
erase_in_line(struct termstate *state, int args[], int argc)
{
    int cursor_x, cursor_y;

    VTOPS_GET_CURSOR(state->emu, &cursor_x, &cursor_y);

    int n = 0;

    if (argc == 1) {
        n = args[0];
    }
    
    switch (n) {
        case 0:
            VTOPS_ERASE_AREA(state->emu, cursor_x, cursor_y, state->width, cursor_y);
            break;
        case 1:
            VTOPS_ERASE_AREA(state->emu, 0, cursor_y, cursor_x, cursor_y);
            break;
        case 2:
            VTOPS_ERASE_AREA(state->emu, 0, cursor_y, state->width, cursor_y);
            break;
    }
}

static void
erase_in_display(struct termstate *state, int args[], int argc)
{
    int cursor_x, cursor_y;

    VTOPS_GET_CURSOR(state->emu, &cursor_x, &cursor_y);

    int param = 0;

    if (argc == 1) {
        param = args[0];
    }

    switch (param) {
        case 0:
            VTOPS_ERASE_AREA(state->emu, cursor_x, cursor_y, state->width, state->height);
            break;
        case 1:
            VTOPS_ERASE_AREA(state->emu, 0, 0, cursor_x, cursor_y);
            break;
        case 2:
            VTOPS_ERASE_AREA(state->emu, 0, 0, state->width, state->height);
            break;
    }
}

static void
eval_csi_parameter(struct termstate *state, char command, int args[], int argc)
{
    switch (command) {
        case 'm':
            select_graphic_rendition(state, args, argc);
            break;
        case 'n':
            device_status_report(state, args, argc);
            break;
        case 'J':
            erase_in_display(state, args, argc);
            break;
        case 'K':
            erase_in_line(state, args, argc);
            break;
        case 'h':
            //ioctl(state->textscreen, TXIOCURSOFF, NULL);
            break;
        case 'l':
            //ioctl(state->textscreen, TXIOCURSON, NULL);
            break;
        case 'H':
            cursor_position(state, args, argc);
            break;
        default:
            printf("unknown CSI command %c\n", command);
            break;
    }
}

static void
eval_csi_command(struct termstate *state, char final_byte)
{
    char *last_parameter = state->csi_buf;
    int args[64];
    int argc = 0;

    for (int i = 0; i < state->csi_len; i++) {
        if (state->csi_buf[i] == ';') {
            state->csi_buf[i] = 0;
            args[argc++] = atoi(last_parameter);
            last_parameter = &state->csi_buf[i+1];
        }
    }   
 
    args[argc++] = atoi(last_parameter);

    eval_csi_parameter(state, final_byte, args, argc);

    memset(state->csi_buf, 0, sizeof(state->csi_buf));
    state->csi_len = 0;
}

static void
eval_custom_sequence(struct termstate *state)
{
    switch (state->csi_buf[0]) {
        case 'P': {
            char c = state->csi_buf[1];
            int pal_index = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
            int rgb = strtol(&state->csi_buf[2], NULL, 16);
            
            VTOP_SET_PALETTE(state->emu, pal_index, rgb);
            break;
        }
        case 'B': {
            int rgb = strtol(&state->csi_buf[1], NULL, 16);
            VTOP_SET_PALETTE(state->emu, 0x10, rgb);
            break;
        }
        case 'F': {
            int rgb = strtol(&state->csi_buf[1], NULL, 16);
            VTOP_SET_PALETTE(state->emu, 0x11, rgb);
            break;
        }
    }
    memset(state->csi_buf, 0, sizeof(state->csi_buf));
    state->csi_len = 0;

}

static void
flush_buffer(struct termstate *state)
{
    if (state->buf_len == 0) {
        return;
    }

    VTOPS_PUT_TEXT(state->emu, state->buf, state->buf_len);

    state->buf_len = 0;
}

static void
eval_escape_char(struct termstate *state, char ch)
{
    switch (ch) {
        case '[':
            state->csi_initiated = true;
            break;
        case ']':
            state->custom_seq_initiated = true;
            break;
        case 'c':
            flush_buffer(state);
            VTOPS_ERASE_AREA(state->emu, 0, 0, state->width, state->height);
            VTOPS_SET_CURSOR(state->emu, 0, 0);
            break;
        default:
            break;
    }
}

static inline void
process_term_char(struct termstate *state, char ch)
{
    if (state->csi_initiated) {
        bool is_parameter = (ch >= 0x30 && ch < 0x40);
        bool is_final_byte = (ch >= 0x40 && ch < 0x7F);

        if (is_parameter) {
            state->csi_buf[state->csi_len++] = ch;
        } else if (is_final_byte) {
            state->csi_initiated = false;
            eval_csi_command(state, ch);
        }

        return;
    }
    
    if (state->custom_seq_initiated) {
        if (state->csi_len == 0) {
            switch (ch) {
                case 'P':
                    state->custom_seq_len = 8;
                    break;
                case 'F':
                case 'B':
                    state->custom_seq_len = 7;
                    break;
                default:
                    state->custom_seq_len = 0;
                    break;
            }
        }
        
        state->csi_buf[state->csi_len++] = ch;

        if (state->csi_len >= state->custom_seq_len) {
            eval_custom_sequence(state);
            state->custom_seq_initiated = false;
        }
        
        return;
    }

    if (state->escape_initiated) {
        state->escape_initiated = false;
        eval_escape_char(state, ch);
        return;
    }

    if (ch == '\x1B') {
        state->escape_initiated = true;
        flush_buffer(state);
        return;
    }

    if (state->buf_len >= TERM_BUF_SIZE) {
        flush_buffer(state);
    }

    state->buf[state->buf_len++] = ch;
}

static void
process_term_data(struct termstate *state, char *buf, int size)
{
    for (int i = 0; i < size; i++) {
        process_term_char(state, buf[i]);
    }
    
    flush_buffer(state);
}


vtemu_t *
vtemu_new(struct vtops *ops, void *state)
{
    vtemu_t *emu = calloc(1, sizeof(vtemu_t));

    memcpy(&emu->ops, ops, sizeof(struct vtops));

    struct termstate *vtstate = calloc(1, sizeof(struct termstate));

    emu->state = state;
    emu->_private = vtstate;
    
    int ptm = mkpty();

    vtstate->emu = emu;
    vtstate->ptm = ptm;

    return emu;
}

void
vtemu_resize(vtemu_t *emu, int width, int height)
{
    struct termstate *state = emu->_private;

    state->width = width;
    state->height = height;
}

void
vtemu_run(vtemu_t *emu)
{
    struct termstate *state = emu->_private;

    char buf[1024];

    for (;;) {
        ssize_t nread = read(state->ptm, buf, 1024);
        
        if (nread > 0) {
            process_term_data(state, buf, nread);
        }
    }
}

void
vtemu_sendchar(vtemu_t *emu, char ch)
{
    struct termstate *state = emu->_private;

    write(state->ptm, &ch, 1);
}

void
vtemu_sendkey(vtemu_t *emu, vt_key_t key)
{
    struct termstate *state = emu->_private;

    switch (key) {
        case VT_KEY_UP_ARROW:
            write(state->ptm, "\x1B[A", 3);
            break;
        case VT_KEY_DOWN_ARROW:
            write(state->ptm, "\x1B[B", 3);
            break;
        case VT_KEY_LEFT_ARROW:
            write(state->ptm, "\x1B[D", 3);
            break;
        case VT_KEY_RIGHT_ARROW:
            write(state->ptm, "\x1B[C", 3);
            break;
        case VT_KEY_PAGE_UP:
            write(state->ptm, "\x1B[5~", 4);
            break;
        case VT_KEY_PAGE_DOWN:
            write(state->ptm, "\x1B[6~", 4);
            break;    

    }
}

int
vtemu_spawn(vtemu_t *emu, char *bin)
{
    pid_t child = fork();

    if (child) {
        return 0;
    }

    struct termstate *state = emu->_private;

    char *pts = ttyname(state->ptm);

    char *argv[] = {
        bin,
        NULL
    };

    int fd = open(pts, O_RDWR);

    struct winsize winsize;
    winsize.ws_row = state->height;
    winsize.ws_col = state->width;
    ioctl(fd, TIOCSWINSZ, &winsize);

    for (int i = 0; i < 3; i++) {
        dup2(fd, i);
    }

    execv(argv[0], argv);
    
    return 0;
}
