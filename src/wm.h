#ifndef INKVIEW_WM_H
#define INKVIEW_WM_H

#include "event.h"
#include "menu.h"
#include "text.h"

#define WM_MAX_WINDOWS 32

struct window {
    struct rect r; /* outer frame, logical */
    const char *title;
    void (*draw)(struct window *w, struct surface *s, struct rect content);
    void *data;
};

struct desktop {
    i32 w, h;                           /* logical */
    struct window *win[WM_MAX_WINDOWS]; /* bottom to top */
    usize n;
    struct window *selected;
    const struct fontset *title_font;

    struct window *drag, *resize;
    i32 drag_dx, drag_dy;
    i32 pending; /* Move/Resize chosen from the frame menu */
    struct window *pending_win;

    struct menu_state menu; /* caller sets menu.fs, .sw, .sh */
    const struct menu *root_menu;
    struct window *menu_win; /* frame menu target; NULL for root */
    i32 root_action;         /* last root menu choice, caller clears */
};

int wm_add(struct desktop *d, struct window *w);
struct rect wm_content(const struct window *w);
void wm_draw(struct desktop *d, struct surface *s);
/* returns true when the screen needs a redraw */
bool wm_pointer(struct desktop *d, const struct ev *e);

#endif
