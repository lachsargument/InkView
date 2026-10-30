#ifndef INKVIEW_EVENT_H
#define INKVIEW_EVENT_H

#include "types.h"

enum ev_type { EV_NONE, EV_QUIT, EV_EXPOSE, EV_DOWN, EV_UP, EV_MOTION };

/*
 * Pen mapping on the Kindle: tip -> left, tip + side button -> right,
 * eraser -> middle. Anything bound to middle must also be reachable
 * with left/right; middle is only ever a shortcut.
 */
enum { PTR_LEFT = 1, PTR_MIDDLE = 2, PTR_RIGHT = 4 };

struct ev {
    enum ev_type type;
    i32 x, y; /* logical pixels */
    u32 button;
};

#endif
