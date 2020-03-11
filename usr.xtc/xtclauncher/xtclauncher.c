#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxtc.h>
#include <unistd.h>
#include <collections/list.h>
#include <libmemgfx/canvas.h>

#define MENU_TEXT_PADDING   2

#define TEXT_CELL_HEIGHT    (MENU_TEXT_PADDING+12)


struct menu_context {
    struct list     entries;
};

struct menu_entry {
    char *    text;
    char *    program;
};


static int
get_longest_entry(struct menu_context *ctx)
{
    list_iter_t iter;

    list_get_iter(&ctx->entries, &iter);

    int longest_so_far = 0;
    struct menu_entry *entry;

    while (iter_move_next(&iter, (void**)&entry)) {
        if (strlen(entry->text) > longest_so_far) {
            longest_so_far = strlen(entry->text);
        }
    }
    
    return longest_so_far;
}

static void
draw_menu_items(canvas_t *canvas, struct menu_context *ctx)
{
    list_iter_t iter;

    list_get_iter(&ctx->entries, &iter);

    int i = 0;
    struct menu_entry *entry;

    while (iter_move_next(&iter, (void**)&entry)) {
      
        canvas_rect(canvas, 1, MENU_TEXT_PADDING+i*TEXT_CELL_HEIGHT, canvas->width - 3, TEXT_CELL_HEIGHT, 0);
        canvas_puts(canvas, MENU_TEXT_PADDING, MENU_TEXT_PADDING+MENU_TEXT_PADDING / 2 + i*TEXT_CELL_HEIGHT, entry->text, 0x0);
        
        i++;
    }
}

static void
exec_app(char *app_path)
{
    pid_t child = fork();

    if (!child) {
        char *argv[] = {
            app_path,
            NULL
        };

        execv(app_path, argv);
        exit(0);
    }
}

static void
handle_click(struct menu_context *ctx, xtc_event_t *event)
{
    int y = event->parameters[1];

    int index = y / TEXT_CELL_HEIGHT;
    printf("index is %d\n", y);

    if (index > LIST_SIZE(&ctx->entries)) {
        return;
    }

    /* inefficient? yes. My linked list implementation doesn't support random access*/
    list_iter_t iter;
    list_get_iter(&ctx->entries, &iter);

    int i = 0;
    struct menu_entry *entry;
    
    while (iter_move_next(&iter, (void**)&entry)) {
        if (i == index) {
            exec_app(entry->program);
            printf("exec %s\n", entry->program);
            break;
        }
        i++;
    }
   
}

int
main(int argc, const char *argv[])
{
    if (xtc_connect() != 0) {
        fprintf(stderr, "could not connect to server!\n");
        return -1;
    }
  
    struct menu_context ctx;

    memset(&ctx, 0, sizeof(ctx));

    struct menu_entry terminal_entry;
    terminal_entry.text = "Terminal";
    terminal_entry.program = "/usr/xtc/bin/xtcterm";

    list_append(&ctx.entries, &terminal_entry);

    int width = get_longest_entry(&ctx)*12 + MENU_TEXT_PADDING*2;
    int height = LIST_SIZE(&ctx.entries)*12 + MENU_TEXT_PADDING*4;
    xtc_win_t win = xtc_window_new(width, height, 0);

    canvas_t *canvas = xtc_open_canvas(win, 0);

    canvas_clear(canvas, 0xA0A0A0);

    draw_menu_items(canvas, &ctx);

    xtc_set_window_title(win, "Launcher");

    xtc_event_t event;

    while (xtc_next_event(win, &event) == 0) {
        switch (event.type) {
            case XTC_EVT_CLICK:
                handle_click(&ctx, &event);
                break;
            default:
                break;
        }
    }
    
    return 0;
}
