#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "keymap.h"

extern int mkpty();


#define TERM_BUF_SIZE   1024

#define KEY_UP_ARROW        72
#define KEY_DOWN_ARROW      80
#define KEY_LEFT_ARROW      75
#define KEY_RIGHT_ARROW     77
#define KEY_PAGE_UP         73
#define KEY_PAGE_DOWN       81

struct termstate {
    int     textscreen; /* file descriptor to output device */
    int     ptm; /* file descriptor to master pseudo-terminal */
    bool    csi_initiated; /* Should be processing a CSI sequence */
    bool    escape_initiated; /* Should we be processing an escape character ? */
    char    csi_buf[256]; /* buffer for CSI sequences */
    int     csi_len; /* length of CSI buf */
    char    buf[TERM_BUF_SIZE]; /* buffer of data to print; used to avoid excessive write() calls */
    int     buf_len; /* length of buf*/
    int     foreground;
    int     background;
};

/*
 * SGR - Select Graphic Rendition; sets display attributes
 */
static void
set_sgr_parameter(struct termstate *state, int param)
{
    switch (param) {
        case 0:
            ioctl(state->textscreen, TXIOSETFG, (void*)7);
            ioctl(state->textscreen, TXIOSETBG, (void*)0);
            state->background = 0;
            state->background = 7;
            break;
        case 7:
            ioctl(state->textscreen, TXIOSETFG, (void*)0);
            ioctl(state->textscreen, TXIOSETBG, (void*)7); 
            //int tmp = state->background;
            //state->background = state->foreground;
            //state->foreground = tmp;
            break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
            state->foreground = param - 30;
            ioctl(state->textscreen, TXIOSETFG, (void*)(param - 30));
            break;    
        case 39:
            ioctl(state->textscreen, TXIOSETFG, (void*)7);
            break;
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
            state->background = param - 40;
            ioctl(state->textscreen, TXIOSETBG, (void*)(param - 40));
            break;
        case 49:
            ioctl(state->textscreen, TXIOSETBG, (void*)0);
            break;
        default:
            printf("got uknown SGR parameter %d\n", param);
            break;
    }
}

static void
eval_sgr_command(struct termstate *state, int args[], int argc)
{
    for (int i = 0; i < argc; i++) {
        set_sgr_parameter(state, args[i]);
    }
}

static void
eval_set_cursor_pos(struct termstate *state, int args[], int argc)
{

    struct curpos new_pos;
    
    if (argc == 2) {
        new_pos.c_col = args[1];
        new_pos.c_row = args[0];
    } else {
        new_pos.c_row = 0;
        new_pos.c_col = 0;
    }

    ioctl(state->textscreen, TXIOSETCUR, &new_pos);
}

static void
eval_status_command(struct termstate *state, int args[], int argc)
{
    if (argc == 1 && args[0] == 6) {
        struct curpos curpos;
        ioctl(state->textscreen, TXIOGETCUR, &curpos);

        char buf[16];
        sprintf(buf, "\x1B[%d;%dR", curpos.c_row, curpos.c_col);

        write(state->ptm, buf, strlen(buf));
    }
}

static void
eval_K_command(struct termstate *state, int args[], int argc)
{
    if (argc == 1) {
        ioctl(state->textscreen, TXIOERSLIN, (void*)args[0]);
    } else {
        ioctl(state->textscreen, TXIOERSLIN, (void*)0);
    }
}

static void
eval_csi_parameter(struct termstate *state, char command, int args[], int argc)
{
    switch (command) {
        case 'm':
            eval_sgr_command(state, args, argc);
            break;
        case 'n':
            eval_status_command(state, args, argc);
            break;
        case 'K':
            eval_K_command(state, args, argc);
            break;
        case 'h':
            ioctl(state->textscreen, TXIOCURSOFF, NULL);
            break;
        case 'l':
            ioctl(state->textscreen, TXIOCURSON, NULL);
            break;
        case 'H':
            eval_set_cursor_pos(state, args, argc);
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
flush_buffer(struct termstate *state)
{
    if (state->buf_len == 0) {
        return;
    }
    write(state->textscreen, state->buf, state->buf_len);
    state->buf_len = 0;
}

static void
eval_escape_char(struct termstate *state, char ch)
{
    switch (ch) {
        case '[':
            state->csi_initiated = true;
            break;
        case 'c':
            flush_buffer(state);
            ioctl(state->textscreen, TXIOCLRSCR, NULL);
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

static void
input_loop(int ptm, int kbd, int vga)
{
    pid_t pid = fork();
    
    if (pid) {
        return;
    }

    uint8_t scancode;
    bool shift_pressed;
    bool control_pressed;
    char ch = 0;

    for (;;) {
        read(kbd, &scancode, 1);

        bool key_up = (scancode & 128) != 0;
        uint8_t key = scancode & 127;

        if (key == 42 || key == 54) {
            shift_pressed = !key_up;
            continue;
        }

        if (key == 29) {
            control_pressed = !key_up;
            continue;
        }

        if (shift_pressed) {
            ch = kbdus_shift[key];
        } else {
            ch = kbdus[key];
        }

        if (key_up) {
            continue;
        }

        if (control_pressed) {
            switch (ch) {
                case 'q':
                    ch = 17;
                    break;
                case 's':
                    ch = 19;
                    break;
            }
        }

        switch (key) {
            case KEY_UP_ARROW:
                write(ptm, "\x1B[A", 3);
                break;
            case KEY_DOWN_ARROW:
                write(ptm, "\x1B[B", 3);
                break;
            case KEY_LEFT_ARROW:
                write(ptm, "\x1B[D", 3);
                break;
            case KEY_RIGHT_ARROW:
                write(ptm, "\x1B[C", 3);
                break;
            case KEY_PAGE_UP:
                write(ptm, "\x1B[5~", 3);
                break;
            case KEY_PAGE_DOWN:
                write(ptm, "\x1B[6~", 3);
                break;    
            default:
                if (ch) {
                    write(ptm, &ch, 1);
                }
                break;
        }
    }
}

static void
invoke_newtty(int ptm)
{
    char *pts = ttyname(ptm);

    char *argv[] = {
        "/sbin/newtty",
        pts,
        NULL
    };
    
    execv(argv[0], argv);
}

struct lfb_req {
    uint8_t     opcode;
    uint32_t    length;
    uint32_t    offset;
    uint32_t    color;
    void *      data;
};

static void
systerm_main()
{
    int ptm = mkpty();
    int vga = open("/dev/lfb", O_WRONLY);
    int kbd = open("/dev/kbd", O_RDONLY);

    if (ptm == -1 || vga == -1 || kbd == -1) {
        close(ptm);
        close(vga);
        close(kbd);
        return;
    }

    ioctl(vga, TXIOSETFG, (void*)7);

    pid_t child = fork();

    if (!child) {
        invoke_newtty(ptm);
    }

    struct termstate state;
    
    memset(&state, 0, sizeof(state));

    state.ptm = ptm;
    state.textscreen = vga;
   
    ioctl(vga, TXIOCURSON, NULL);

    input_loop(ptm, kbd, vga);

    for ( ;; ) {
        char buf[1024];

        ssize_t nread = read(ptm, buf, 1024);
    
        if (nread > 0) {
            process_term_data(&state, buf, nread);
        }
    }
}

int
main()
{
    systerm_main();
    
    return 0;
}
