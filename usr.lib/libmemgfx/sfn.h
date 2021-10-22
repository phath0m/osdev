#ifndef _LIBMEMGFX_SSFN_H
#define _LIBMEMGFX_SSFN_H

#include <stdint.h>
#include <sys/types.h>


typedef void * sfn_font_t;
typedef struct canvas canvas_t;
typedef uint32_t color_t;

int             sfn_bbox(sfn_font_t *, const char *, int *, int *, int *, int *);
sfn_font_t  *   sfn_load(const char *, int);
void            sfn_render(canvas_t *, int, int, sfn_font_t *, const char *, color_t);

#endif