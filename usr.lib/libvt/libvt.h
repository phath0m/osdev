#ifndef _LIBVT_H
#define _LIBVT_H

typedef enum {
    VT_ATTR_DEF_BACKGROUND,
    VT_ATTR_DEF_FOREGROUND,
    VT_ATTR_BACKGROUND,
    VT_ATTR_FOREGROUND,
} vt_attr_t;

typedef enum {
    VT_KEY_UP_ARROW,
    VT_KEY_DOWN_ARROW,
    VT_KEY_LEFT_ARROW,
    VT_KEY_RIGHT_ARROW,
    VT_KEY_PAGE_UP,
    VT_KEY_PAGE_DOWN
} vt_key_t;

typedef struct vtemu vtemu_t;

typedef int     (*vt_set_attributes_t)(vtemu_t *emu, vt_attr_t attr, int val);
typedef int     (*vt_set_cursor_t)(vtemu_t *emu, int x, int y);
typedef int     (*vt_get_cursor_t)(vtemu_t *emu, int *x, int *y);
typedef int     (*vt_set_palette_t)(vtemu_t *emu, int i, int col);
typedef int     (*vt_erase_area_t)(vtemu_t *emu, int start_x, int start_y, int end_x, int end_y);
typedef int     (*vt_eval_custom_seq)(vtemu_t *, const char *);
typedef int     (*vt_get_custom_seq_len)(vtemu_t *, const char);

typedef void    (*vt_put_text_t)(vtemu_t *emu, char *text, size_t nbyte);

struct vtops {
    vt_set_attributes_t set_attributes;
    vt_set_cursor_t     set_cursor;
    vt_get_cursor_t     get_cursor;
    vt_set_palette_t    set_palette;
    vt_erase_area_t     erase_area;
    vt_put_text_t       put_text;
    vt_eval_custom_seq  eval_custom_seq;
    vt_get_custom_seq_len   get_custom_seq_len;
};

typedef struct vtemu {
    struct vtops    ops;
    void *          state;      /* user defined state */
    void *          _private;   /* private data */ 
    
} vtemu_t;

#define VTOPS_SET_ATTR(emu,attr,val) ((emu)->ops.set_attributes(emu, attr, val))
#define VTOPS_PUT_TEXT(emu,text,nbyte) ((emu)->ops.put_text(emu, text, nbyte))
#define VTOPS_ERASE_AREA(emu,x,y,ex,ey) ((emu)->ops.erase_area(emu, x, y, ex, ey))
#define VTOPS_GET_CURSOR(emu,x,y) ((emu)->ops.get_cursor(emu, x, y))
#define VTOPS_SET_CURSOR(emu,x,y) ((emu)->ops.set_cursor(emu, x, y))
#define VTOP_SET_PALETTE(emu, i, col) ((emu)->ops.set_palette(emu, i, col))
#define VTOPS_EVAL_CUSTOM_SEQ(emu, csi) ((emu)->ops.eval_custom_seq((emu), (csi)))
#define VTOPS_GET_CUSTOM_SEQ_LEN(emu, ch) ((emu)->ops.get_custom_seq_len((emu), (ch)))

vtemu_t *   vtemu_new(struct vtops *ops, void *state);
void        vtemu_resize(vtemu_t *emu, int width, int height);
void        vtemu_run(vtemu_t *emu);
void        vtemu_sendchar(vtemu_t *emu, char ch);
void        vtemu_sendkey(vtemu_t *emu, vt_key_t key);
int         vtemu_spawn(vtemu_t *emu, char *bin);

#endif
