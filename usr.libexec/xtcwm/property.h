#ifndef _PROPERTY_H
#define _PROPERTY_H

typedef enum {
    XTC_BACKGROUND_COL          = 0,
    XTC_WIN_BORDER_COL          = 1,
    XTC_WIN_TEXT_COL            = 2,
    XTC_WIN_TITLE_COL           = 3,
    XTC_WIN_ACTIVE_TITLE_COL    = 4,
    XTC_WIN_ACTIVE_TEXT_COL     = 5,
    XTC_WIN_COL                 = 6,
    XTC_CHISEL_DARK_COL         = 7,
    XTC_CHISEL_LIGHT_COL        = 8,
    XTC_CHISEL_ACTIVE_DARK_COL  = 9,
    XTC_CHISEL_ACTIVE_LIGHT_COL = 0xA
} xtc_property_t;

extern int xtc_properties[];

#define XTC_GET_PROPERTY(prop) (xtc_properties[(prop)])
#define XTC_SET_PROPERTY(prop, val) (xtc_properties[(prop)] = (val))

#endif
