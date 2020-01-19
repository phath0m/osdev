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

/* ioctl commands for textscreen device */
#define TEXTSCREEN_CLEAR    0x0000
#define TEXTSCREEN_SETBG    0x0001
#define TEXTSCREEN_SETFG    0x0002

struct termstate {
    int     textscreen; /* file descriptor to output device */
    bool    csi_initiated; /* Should be processing a CSI sequence */
    bool    escape_initiated; /* Should we be processing an escape character ? */
    char    csi_buf[256]; /* buffer for CSI sequences */
    int     csi_len; /* length of CSI buf */
    char    buf[TERM_BUF_SIZE]; /* buffer of data to print; used to avoid excessive write() calls */
    int     buf_len; /* length of buf*/
};

/*
 * SGR - Select Graphic Rendition; sets display attributes
 */
static void
eval_sgr_parameter(struct termstate *state, int param)
{
    switch (param) {
        case 0:
            ioctl(state->textscreen, TEXTSCREEN_SETFG, (void*)7);
            ioctl(state->textscreen, TEXTSCREEN_SETBG, (void*)0);
            break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
            ioctl(state->textscreen, TEXTSCREEN_SETFG, (void*)(param - 30));
            break;    
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
            ioctl(state->textscreen, TEXTSCREEN_SETBG, (void*)(param - 40));
            break;
        default:
            break;
    }
}

static void
eval_csi_parameter(struct termstate *state, int param, char final_byte)
{
    switch (final_byte) {
        case 'm':
            eval_sgr_parameter(state, param);
            break;
    }
}

static void
eval_csi_command(struct termstate *state, char final_byte)
{
    char *last_parameter = state->csi_buf;

    for (int i = 0; i < state->csi_len; i++) {
        if (state->csi_buf[i] == ';') {
            state->csi_buf[i] = 0;
            eval_csi_parameter(state, atoi(last_parameter), final_byte);
            last_parameter = &state->csi_buf[i+1];
        }
    }   
   
    eval_csi_parameter(state, atoi(last_parameter), final_byte);

    memset(state->csi_buf, 0, sizeof(state->csi_buf));
    state->csi_len = 0;
}

static void
eval_escape_char(struct termstate *state, char ch)
{
    switch (ch) {
        case '[':
            state->csi_initiated = true;
            break;
        default:
            break;
    }
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

    if (ch != '\b') {
        state->buf[state->buf_len++] = ch;
    }
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
    char input_buffer[512];
    int buf_pos = 0;
    char ch = 0;

    for (;;) {
        read(kbd, &scancode, 1);

        bool key_up = (scancode & 128) != 0;
        uint8_t key = scancode & 127;

        if (key == 42 || key == 54) {
            shift_pressed = !key_up;
            continue;
        }

        if (!key_up) {
            continue;
        }

        if (shift_pressed) {
            ch = kbdus_shift[key];
        } else {
            ch = kbdus[key];
        }

        if (!ch) {
            continue;
        }
        
        if (ch == '\b' && buf_pos > 0) {
            input_buffer[buf_pos - 1] = 0;
            buf_pos--;
        } else if (ch == '\n' || buf_pos >= sizeof(input_buffer)) {
            input_buffer[buf_pos++] = '\n';
            write(ptm, input_buffer, buf_pos);
            buf_pos = 0;
        } else {
            input_buffer[buf_pos++] = ch;     
        }
        write(vga, &ch, 1);
        
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

static void
systerm_main()
{
    int ptm = mkpty();
    int vga = open("/dev/vga", O_WRONLY);
    int kbd = open("/dev/kbd", O_RDONLY);

    if (ptm == -1 || vga == -1 || kbd == -1) {
        close(ptm);
        close(vga);
        close(kbd);
        return;
    }

    ioctl(vga, TEXTSCREEN_SETFG, (void*)7);

    pid_t child = fork();

    if (!child) {
        invoke_newtty(ptm);
    }

    struct termstate state;
    
    memset(&state, 0, sizeof(state));

    state.textscreen = vga;
   
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
