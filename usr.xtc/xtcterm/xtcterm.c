#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <thread.h>
#include <libxtc.h>
#include <libvt.h>
#include <unistd.h>
#include <libmemgfx/canvas.h>
#include <sys/ioctl.h>
#include "keymap.h"

#define KEY_UP_ARROW        72
#define KEY_DOWN_ARROW      80
#define KEY_LEFT_ARROW      75
#define KEY_RIGHT_ARROW     77
#define KEY_PAGE_UP         73
#define KEY_PAGE_DOWN       81

static int color_palette[] = {
    0x0000000, 0x1e92f2f, 0x20ed839, 0x3dddd13,
    0x43b48e3, 0x5f996e2, 0x623edda, 0x7ababab,
    0x8343434, 0x9e92f2f, 0xA0ed839, 0xBdddd13,
    0xC3b48e3, 0xDf996e2, 0xE23edda, 0xFf9f9f9,
    /* background */
    0xF9F9F9,
    /* foreground */
    0x102015
};

struct termstate {
    int             width; /* how many columns */
    int             height; /* how many rows */
    int             foreground;
    int             background;
    int             position;
    canvas_t *      canvas;
};

#define GET_TERM_X(t) ((t)->position % (t)->width)
#define GET_TERM_Y(t) ((t)->position / (t)->width)

/* actually draw a character on the screen */
static void
put_char(struct termstate *state, char ch)
{
    if (state->position >= (state->width * state->height)) {
        canvas_scroll(state->canvas, 12, 0);
        canvas_fill(state->canvas, 0, state->canvas->height - 12, state->canvas->width, 12,
                state->background);
        state->position = state->width * (state->height - 1);
    }

    int x = GET_TERM_X(state) * 8;
    int y = GET_TERM_Y(state) * 12;

    switch (ch) {
        case '\n':
            state->position += state->width;
            break;
        case '\r':
            state->position -= state->position % state->width;
            break;
        case '\b':
            state->position--;
            canvas_fill(state->canvas, GET_TERM_X(state)*8, GET_TERM_Y(state)*12,
                    8, 12, state->background);
            break;
        default:
            canvas_fill(state->canvas, x, y, 8, 12, state->background);
            canvas_putc(state->canvas, x, y, ch, state->foreground);
            state->position++;
            break;
    }
}

int
vtop_erase_area(vtemu_t *emu, int start_x, int start_y, int end_x, int end_y)
{
    struct termstate *state = emu->state;
    int new_pos = start_y * state->width + start_x;
    int end_pos = end_y * state->width + end_x;
    int old_pos = state->position; 
    state->position = new_pos;

    printf("Clear (%d,%d) -> (%d,%d)\n", start_x, start_y, end_x, end_y);
    while (state->position < end_pos) {
        //put_char(state, ' ');

        canvas_fill(state->canvas, GET_TERM_X(state)*8, GET_TERM_Y(state)*12,
                    8, 12, state->background);

        state->position++;
    }

    state->position = old_pos;
    
    return 0;
}

static int
vtop_get_cursor(vtemu_t *emu, int *x, int *y)
{
    struct termstate *state = emu->state;

    *x = GET_TERM_X(state);
    *y = GET_TERM_Y(state);

    return 0;
}

static int
vtop_set_cursor(vtemu_t *emu, int x, int y)
{
    struct termstate *state = emu->state;

    state->position = y * state->width + x;

    return 0;
}

static int
vtop_set_attribute(vtemu_t *emu, vt_attr_t attr, int val)
{
    struct termstate *state = emu->state;

    switch (attr) {
        case VT_ATTR_DEF_BACKGROUND:
           state->background = color_palette[0x10];
           break;
        case VT_ATTR_DEF_FOREGROUND:
           state->foreground = color_palette[0x11];
           break; 
        case VT_ATTR_BACKGROUND:
            state->background = color_palette[val];
            break;
        case VT_ATTR_FOREGROUND:
            state->foreground = color_palette[val];
            break;
    }

    return 0;
}

static int
vtop_set_palette(vtemu_t *emu, int i, int col)
{
    color_palette[i] = col;

    return 0;
}

static void
vtop_put_text(vtemu_t *emu, char *text, size_t nbyte)
{
    struct termstate *state = emu->state;

    for (int i = 0; i < nbyte; i++) {
        put_char(state, text[i]);
    }
}

static void *
term_output_thread(void *argp)
{
    vtemu_t *emu = (vtemu_t*)argp;

    vtemu_run(emu);

    return NULL;
}

static void
process_character(vtemu_t *emu, uint8_t scancode)
{
    static bool shift_pressed = false;
    static bool control_pressed = false;
    char ch;

    bool key_up = (scancode & 128) != 0;
    uint8_t key = scancode & 127;

    if (key == 42 || key == 54) {
        shift_pressed = !key_up;
        return;
    }

    if (key == 29) {
        control_pressed = !key_up;
        return;
    }

    if (shift_pressed) {
        ch = kbdus_shift[key];
    } else {
        ch = kbdus[key];
    }

    if (key_up) {
        return;
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
            vtemu_sendkey(emu, VT_KEY_UP_ARROW);
            break;
        case KEY_DOWN_ARROW:
            vtemu_sendkey(emu, VT_KEY_DOWN_ARROW);
            break;
        case KEY_LEFT_ARROW:
            vtemu_sendkey(emu, VT_KEY_LEFT_ARROW);
            break;
        case KEY_RIGHT_ARROW:
            vtemu_sendkey(emu, VT_KEY_RIGHT_ARROW);
            break;
        case KEY_PAGE_UP:
            vtemu_sendkey(emu, VT_KEY_PAGE_UP);
            break;
        case KEY_PAGE_DOWN:
            vtemu_sendkey(emu, VT_KEY_PAGE_DOWN);
            break;    
        default:
            if (ch) {
                vtemu_sendchar(emu, ch);
            }
            break;
    }
}

struct vtops vtops = {
    .erase_area     = vtop_erase_area,
    .get_cursor     = vtop_get_cursor,
    .put_text       = vtop_put_text,
    .set_attributes = vtop_set_attribute,
    .set_cursor     = vtop_set_cursor,
    .set_palette    = vtop_set_palette
};

static void
handle_sigchld(int signo)
{
    printf("Bye!\n");
    kill(9, getpid());
}

int
main(int argc, const char *argv[])
{ 
    if (fork() != 0) {
        exit(0);
    }

    if (xtc_connect() != 0) {
        fprintf(stderr, "could not connect to server!\n");
        return -1;
    }
    
    struct sigaction sigact;
    sigact.sa_handler = handle_sigchld;
    sigact.sa_flags = 0;
    sigaction(17, &sigact, (struct sigaction *)NULL);

    struct termstate state;
    memset(&state, 0, sizeof(state));
    state.width = 80;
    state.height = 25;
    state.foreground = color_palette[0x11];
    state.background = color_palette[0x10];

    vtemu_t *emu = vtemu_new(&vtops, &state);
    vtemu_resize(emu, state.width, state.height-1);    
    vtemu_spawn(emu, "/bin/sh");

    xtc_win_t win = xtc_window_new(80*8, 25*12, 0);

    canvas_t *canvas = xtc_open_canvas(win, 0);
    state.canvas = canvas;

    canvas_clear(canvas, state.background);

    xtc_set_window_title(win, "Terminal");

    thread_t thread;
    thread_create(&thread, term_output_thread, emu);

    xtc_event_t event;

    while (xtc_next_event(win, &event) == 0) {
        switch (event.type) {
            case XTC_EVT_KEYPRESS: {
                uint8_t scancode = event.parameters[0];
                process_character(emu, scancode);
                break;
            }
            case XTC_EVT_RESIZE: {
                int width = (event.parameters[0] / 8);
                int height = (event.parameters[1] / 12);
                state.width = width;
                state.height = height;
                canvas_resize(canvas, width*8, height*12);
                xtc_resize(win, width*8, height*12);
                vtemu_resize(emu, state.width, state.height-1);
                break;
            }
            default:
                break;
        }
    }
    
    return 0;
}
