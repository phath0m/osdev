#include <authlib.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libinput/kbd.h>
#include <libmemgfx/canvas.h>
#include <libmemgfx/display.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include "keymap.h"

static display_t *login_display;
static canvas_t *login_canvas;

static int
do_login(struct passwd *pwd)
{
    static char *sh_argv[3] = {
        "/bin/sh",
        "/etc/xtc/xtcwm.rc",
        NULL
    };

    pid_t child = fork();

    if (child == 0) {
        setuid(pwd->pw_uid);
        setgid(pwd->pw_gid);
        setenv("HOME", pwd->pw_dir, true);
        execv("/bin/sh", sh_argv);
        perror("execv");
        exit(-1);
    } else {
        wait(NULL);
    }

    return 0;
}

static int
attempt_login(const char *login, const char *passwd)
{
    struct passwd *pwd = getpwnam(login);

    if (!pwd) {
        return -1;
    }
    
    int login_flags = 0;
    struct login login_buf;

    if (authlib_login(&login_buf, login, passwd, login_flags) != 0) {
        return -1;
    } 

    do_login(pwd);
    authlib_logout(&login_buf);
    
    return 0;
}

static void
textbox(char *text, int x, int y, int width)
{
    canvas_fill(login_canvas, x, y, width, 16, 0xA0A0A0);
    canvas_rect(login_canvas, x, y, width, 16, 0);
    canvas_puts(login_canvas, x + 2, y + 2, text, 0); 
}

int
main(int argc, char *argv[])
{
    login_display = display_open();

    int width = display_width(login_display);
    int height = display_height(login_display);

    login_canvas = canvas_new(width, height, CANVAS_PARTIAL_RENDER);
    canvas_clear(login_canvas, 0x505170);
   
    int prompt_width = width / 2;
    int prompt_height = height / 4;
    int prompt_x = prompt_width / 2;
    int prompt_y = height / 3;
    int username_y = prompt_y + prompt_height / 2 - 12;
    int password_y = prompt_y + prompt_height / 2 + 12;
    int username_width = 110;
    int password_width = 110;
    int username_text_x = prompt_x + 2 + username_width;
    int password_text_x = prompt_x + 2 + password_width;

    canvas_fill(login_canvas, prompt_x, prompt_y, prompt_width, prompt_height, 0xFFFFFF);
    canvas_fill(login_canvas, prompt_x, prompt_y, prompt_width, 20, 0x0);
    canvas_puts(login_canvas, prompt_x + 2, prompt_y + 4, "XTC Login", 0xFFFFFF);
    canvas_rect(login_canvas, prompt_x, prompt_y, prompt_width, prompt_height, 0x0);
    canvas_puts(login_canvas, prompt_x + 10, username_y, "   Login: ", 0);
    canvas_puts(login_canvas, prompt_x + 10, password_y, "Password: ", 0);

    textbox("", username_text_x, username_y, 200);
    textbox("", password_text_x, password_y, 200);

    display_render(login_display, login_canvas, 0, 0, width, height);

    int username_index = 0;
    char username[32];
   
    bzero(username, 32);

    int password_index =0 ;
    char password[32];
    bzero(password, 32);

    int active_textbox = 0;

    kbd_t *kb = kbd_open();
    kbd_event_t event;

    for (;;) {
        kbd_next_event(kb, &event);
        
        uint8_t scancode = event.scancode;
		bool shift_pressed = false;
		bool control_pressed = false;
		char ch;

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

        if (key_up || control_pressed) {
            continue;
        }

        if (ch == '\t') {
            active_textbox = !active_textbox;
            continue;
        }
        
        if (ch == '\r') {
            if (!active_textbox) active_textbox = !active_textbox;
            else attempt_login(username, password);
            continue;
        }

        char *textboxp;
        int *textbox_indexp;

        if (active_textbox == 0) {
            textboxp = username;
            textbox_indexp = &username_index;
        } else {
            textboxp = password;
            textbox_indexp = &password_index;
        }

        if (ch == '\b') {
            textboxp[*textbox_indexp] = '\x00';
            if (*textbox_indexp) *textbox_indexp -= 1;
        } else {
            textboxp[*textbox_indexp] = ch;
            *textbox_indexp += 1;
        }

        if (active_textbox == 0) {
            textbox(username, username_text_x, username_y, 200);
        } else {
            textbox(password, password_text_x, password_y, 200);
        }

        display_render(login_display, login_canvas, 0, 0, width, height);
    }

    kbd_close(kb);

    return 0;
}

