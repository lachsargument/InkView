#include "wm.h"

#include <string.h>

/* workstation-style frame, logical pixels */
#define TITLE_H 18
#define TITLE_INSET 8
#define BORDER 5 /* 2 black + 1 white + 2 black */
/* how much of a dragged window must stay on screen */
#define KEEP_VISIBLE 32
#define MIN_W 120
#define MIN_H 60

enum {
    WACT_MOVE = 1,
    WACT_RESIZE,
    WACT_FRONT,
    WACT_BACK,
    WACT_REDISPLAY,
    WACT_QUIT,
};

/* Move and Resize from the menu make every frame operation reachable
 * without the middle button */
static const struct menu_item frame_items[] = {
    {"Move", NULL, WACT_MOVE},           {"Resize", NULL, WACT_RESIZE},
    {"Front", NULL, WACT_FRONT},         {"Back", NULL, WACT_BACK},
    {"Redisplay", NULL, WACT_REDISPLAY}, {"Quit", NULL, WACT_QUIT},
};
static const struct menu frame_menu = {frame_items, 6};

int wm_add(struct desktop *d, struct window *w)
{
    if (d->n == WM_MAX_WINDOWS)
        return -1;
    d->win[d->n++] = w;
    return 0;
}

struct rect wm_content(const struct window *w)
{
    return (struct rect){w->r.x + BORDER, w->r.y + TITLE_H, w->r.w - 2 * BORDER,
                         w->r.h - TITLE_H - BORDER};
}

static void draw_frame(struct desktop *d, struct surface *s, struct window *w)
{
    struct rect r = w->r;
    struct rect bar = {r.x, r.y, r.w, TITLE_H};
    i32 side_h = r.h - TITLE_H;
    const struct font *f = d->title_font->f[0];

    gfx_fill(s, bar, BLACK);
    gfx_fill(s, (struct rect){r.x, r.y + TITLE_H, BORDER, side_h}, BLACK);
    gfx_fill(s,
             (struct rect){r.x + r.w - BORDER, r.y + TITLE_H, BORDER, side_h},
             BLACK);
    gfx_fill(s, (struct rect){r.x, r.y + r.h - BORDER, r.w, BORDER}, BLACK);

    /* selection fills the white middle line, leaving a solid black border */
    if (w != d->selected) {
        gfx_fill(s, (struct rect){r.x + 2, r.y + TITLE_H, 1, side_h - 2},
                 WHITE);
        gfx_fill(s, (struct rect){r.x + r.w - 3, r.y + TITLE_H, 1, side_h - 2},
                 WHITE);
        gfx_fill(s, (struct rect){r.x + 2, r.y + r.h - 3, r.w - 4, 1}, WHITE);
    }

    gfx_clip(s, &bar);
    text_draw(s, d->title_font, r.x + TITLE_INSET,
              r.y + (TITLE_H - f->ascent - f->descent) / 2 + f->ascent,
              w->title, WHITE);
    gfx_clip(s, NULL);
}

void wm_draw(struct desktop *d, struct surface *s)
{
    gfx_pattern(s, (struct rect){0, 0, d->w, d->h}, &PAT_DESKTOP);

    for (usize i = 0; i < d->n; i++) {
        struct window *w = d->win[i];
        struct rect c = wm_content(w);

        draw_frame(d, s, w);
        gfx_fill(s, c, WHITE);
        if (w->draw) {
            gfx_clip(s, &c);
            w->draw(w, s, c);
            gfx_clip(s, NULL);
        }
    }
    menu_draw(&d->menu, s);
}

static struct window *hit(struct desktop *d, i32 x, i32 y)
{
    for (usize i = d->n; i-- > 0;)
        if (rect_hit(d->win[i]->r, x, y))
            return d->win[i];
    return NULL;
}

static void raise_window(struct desktop *d, struct window *w)
{
    for (usize i = 0; i < d->n; i++) {
        if (d->win[i] != w)
            continue;
        memmove(&d->win[i], &d->win[i + 1], (d->n - i - 1) * sizeof(d->win[0]));
        d->win[d->n - 1] = w;
        return;
    }
}

static void lower_window(struct desktop *d, struct window *w)
{
    for (usize i = 0; i < d->n; i++) {
        if (d->win[i] != w)
            continue;
        memmove(&d->win[1], &d->win[0], i * sizeof(d->win[0]));
        d->win[0] = w;
        return;
    }
}

static void remove_window(struct desktop *d, struct window *w)
{
    for (usize i = 0; i < d->n; i++) {
        if (d->win[i] != w)
            continue;
        memmove(&d->win[i], &d->win[i + 1], (d->n - i - 1) * sizeof(d->win[0]));
        d->n--;
        break;
    }
    if (d->selected == w)
        d->selected = NULL;
}

static i32 clamp(i32 v, i32 lo, i32 hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void frame_action(struct desktop *d, struct window *w, i32 act)
{
    switch (act) {
    case WACT_MOVE:
    case WACT_RESIZE:
        /* the next press, anywhere, moves or resizes this window */
        d->pending = act;
        d->pending_win = w;
        break;
    case WACT_FRONT:
        raise_window(d, w);
        break;
    case WACT_BACK:
        lower_window(d, w);
        break;
    case WACT_QUIT:
        remove_window(d, w);
        break;
    default:
        break;
    }
}

static void begin_pending(struct desktop *d, const struct ev *e)
{
    struct window *w = d->pending_win;

    if (d->pending == WACT_MOVE) {
        d->drag = w;
        d->drag_dx = e->x - w->r.x;
        d->drag_dy = e->y - w->r.y;
    } else {
        d->resize = w;
    }
    d->pending = 0;
}

static bool in_frame(const struct window *w, i32 x, i32 y)
{
    return rect_hit(w->r, x, y) && !rect_hit(wm_content(w), x, y);
}

static bool press(struct desktop *d, const struct ev *e)
{
    struct window *w;

    if (d->pending) {
        begin_pending(d, e);
        return true;
    }
    w = hit(d, e->x, e->y);
    d->selected = w;
    if (w)
        raise_window(d, w);

    if (e->button == PTR_RIGHT) {
        /* content right-clicks will belong to the tool's own menu */
        if (w && !in_frame(w, e->x, e->y))
            return true;
        d->menu_win = w;
        menu_open(&d->menu, w ? &frame_menu : d->root_menu, e->x, e->y);
        return true;
    }
    /* left drags by the title bar; middle drags from anywhere */
    if (w && (e->button == PTR_MIDDLE || e->y < w->r.y + TITLE_H)) {
        d->drag = w;
        d->drag_dx = e->x - w->r.x;
        d->drag_dy = e->y - w->r.y;
    }
    return true;
}

bool wm_pointer(struct desktop *d, const struct ev *e)
{
    struct window *w;

    if (d->menu.depth) {
        bool redraw = false;
        i32 act = menu_pointer(&d->menu, e, &redraw);

        if (act && d->menu_win)
            frame_action(d, d->menu_win, act);
        else if (act)
            d->root_action = act;
        return redraw || act;
    }

    switch (e->type) {
    case EV_DOWN:
        return press(d, e);
    case EV_MOTION:
        if (d->resize) {
            w = d->resize;
            w->r.w = clamp(e->x - w->r.x + 1, MIN_W, d->w - w->r.x);
            w->r.h = clamp(e->y - w->r.y + 1, MIN_H, d->h - w->r.y);
            return true;
        }
        if (!d->drag)
            return false;
        w = d->drag;
        /* keep the title bar reachable so the window can be dragged back */
        w->r.x = clamp(e->x - d->drag_dx, KEEP_VISIBLE - w->r.w,
                       d->w - KEEP_VISIBLE);
        w->r.y = clamp(e->y - d->drag_dy, 0, d->h - TITLE_H);
        return true;
    case EV_UP:
        d->drag = NULL;
        d->resize = NULL;
        return false;
    default:
        return false;
    }
}
