#ifndef INKVIEW_MENU_H
#define INKVIEW_MENU_H

#include "event.h"
#include "text.h"

#define MENU_DEPTH 4

struct menu;

struct menu_item {
    const char *label;
    const struct menu *sub; /* pull-right, or NULL */
    i32 action;             /* > 0; 0 just closes the menu */
};

struct menu {
    const struct menu_item *items;
    usize n;
};

struct menu_level {
    const struct menu *m;
    struct rect r; /* outer box, logical */
    i32 hl;        /* highlighted item, -1 for none */
};

/*
 * Press, drag, release picks an item (walking menus). A press and
 * release without moving leaves the menu up, so a pen can tap through
 * it item by item instead of holding the side button while dragging.
 */
struct menu_state {
    struct menu_level lvl[MENU_DEPTH];
    usize depth; /* 0 when closed */
    const struct fontset *fs;
    i32 sw, sh; /* screen size, for clamping */
    bool tracking, moved;
    i32 x0, y0;
};

/* opens at the pointer, as a button press that is still held */
void menu_open(struct menu_state *ms, const struct menu *m, i32 x, i32 y);
void menu_close(struct menu_state *ms);
void menu_draw(const struct menu_state *ms, struct surface *s);
/* consumes every pointer event while open; returns the chosen action */
i32 menu_pointer(struct menu_state *ms, const struct ev *e, bool *redraw);

#endif
