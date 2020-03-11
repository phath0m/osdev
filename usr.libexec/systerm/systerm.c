#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <thread.h>
#include <libvt.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "keymap.h"

#define KEY_UP_ARROW        72
#define KEY_DOWN_ARROW      80
#define KEY_LEFT_ARROW      75
#define KEY_RIGHT_ARROW     77
#define KEY_PAGE_UP         73
#define KEY_PAGE_DOWN       81

struct termstate {
    int             width; /* how many columns */
    int             height; /* how many rows */
    int             foreground;
    int             background;
    int             position;
    int             textscreen;
};

static int vtop_get_cursor(vtemu_t *emu, int *x, int *y);
static int vtop_set_cursor(vtemu_t *emu, int x, int y);

int
vtop_erase_area(vtemu_t *emu, int start_x, int start_y, int end_x, int end_y)
{
    struct termstate *state = emu->state;
    int cur_x, cur_y;
    
    vtop_get_cursor(emu, &cur_x, &cur_y);
    vtop_set_cursor(emu, start_x, start_y);

    int new_pos = start_y * state->width + start_x;
    int end_pos = end_y * state->width + end_x;
    int pos = new_pos;

    while (pos < end_pos) {
        char space = ' ';
        write(state->textscreen, &space, 1);
        pos++;
    }

    vtop_set_cursor(emu, cur_x, cur_y);
    
    return 0;
}

static int
vtop_get_cursor(vtemu_t *emu, int *x, int *y)
{
    struct termstate *state = emu->state;

    struct curpos curpos;
    ioctl(state->textscreen, TXIOGETCUR, &curpos);
    
    *x = curpos.c_col;
    *y = curpos.c_row;

    return 0;
}

static int
vtop_set_cursor(vtemu_t *emu, int x, int y)
{
    struct termstate *state = emu->state;

    struct curpos new_pos;

    new_pos.c_row = y;
    new_pos.c_col = x;
    
    ioctl(state->textscreen, TXIOSETCUR, &new_pos);

    return 0;
}

static int
vtop_set_attribute(vtemu_t *emu, vt_attr_t attr, int val)
{
    struct termstate *state = emu->state;

    switch (attr) {
        case VT_ATTR_BACKGROUND:
            ioctl(state->textscreen, TXIOSETBG, (void*)val);
            break;
        case VT_ATTR_FOREGROUND:
            ioctl(state->textscreen, TXIOSETFG, (void*)val);
            break;
    }

    return 0;
}

static void
vtop_put_text(vtemu_t *emu, char *text, size_t nbyte)
{
    struct termstate *state = emu->state;
    write(state->textscreen, text, nbyte);
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
    .set_cursor     = vtop_set_cursor
};

static void
handle_sigchld(int signo)
{
    printf("Bye!\n");
    kill(9, getpid());
}

int
main(int argc, char *argv[])
{ 
    int console = -1;
    
    if (access("/dev/lfb", R_OK) == 0) {
        console = open("/dev/lfb", O_WRONLY);
    } else {
        console = open("/dev/vga", O_WRONLY);
    }

    int kbd = open("/dev/kbd", O_RDONLY);

    if (console == -1 || kbd == -1) {
        close(console);
        close(kbd);
        return -1;
    }

    ioctl(console, TXIOCURSON, NULL);
    ioctl(console, TXIOSETFG, (void*)7);
    
    struct sigaction sigact;
    sigact.sa_handler = handle_sigchld;
    sigact.sa_flags = 0;
    sigaction(17, &sigact, (struct sigaction *)NULL);

    struct termstate state;
    memset(&state, 0, sizeof(state));
    state.width = 80;
    state.height = 25;
    state.textscreen = console;

    char *shell = "/usr/bin/login";

    if (argc == 2) {
        shell = argv[1];
    }

    vtemu_t *emu = vtemu_new(&vtops, &state);
    vtemu_resize(emu, state.width, state.height-1);
    vtemu_spawn(emu, shell);

    thread_t thread;
    thread_create(&thread, term_output_thread, emu);

    uint8_t scancode;

    for (;;) {
        read(kbd, &scancode, 1);
        process_character(emu, scancode);
    }

    return 0;
}
