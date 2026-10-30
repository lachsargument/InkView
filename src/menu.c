#include "menu.h"

/* measured from SunView 4.1 captures, logical pixels */
#define ITEM_H 17
#define TEXT_X 18     /* left border to pen position */
#define BASELINE 12   /* item top to baseline */
#define RIGHT_PAD 13  /* text end to right border */
#define ARROW_X 19    /* arrow left edge, from the outer right edge */
#define ARROW_ROOM 14 /* extra width for items with an arrow */
#define SHADOW 6
#define SUB_OVERLAP 22 /* submenu covers this much of the parent */
#define DRAG_SLOP 2

/* the pull-right arrow, 9x7 */
static const u8 arrow[] = {0x04, 0x00, 0x06, 0x00, 0xfd, 0x00, 0x01,
                           0x80, 0xfd, 0x00, 0x06, 0x00, 0x04, 0x00};

static i32 clamp(i32 v, i32 lo, i32 hi)
{
    return v > hi ? hi : v < lo ? lo : v;
}

static struct rect layout(const struct menu_state *ms, const struct menu *m,
                          i32 x, i32 y)
{
    i32 tw = 0, w, h;

    for (usize i = 0; i < m->n; i++) {
        i32 iw = text_width(ms->fs, m->items[i].label) +
                 (m->items[i].sub ? ARROW_ROOM : 0);
        if (iw > tw)
            tw = iw;
    }
    w = TEXT_X + tw + RIGHT_PAD;
    h = ITEM_H * (i32)m->n + 2;
    return (struct rect){clamp(x, 0, ms->sw - w - SHADOW),
                         clamp(y, 0, ms->sh - h - SHADOW), w, h};
}

static i32 item_top(const struct menu_level *l, i32 i)
{
    return l->r.y + 1 + ITEM_H * i;
}

static i32 hit_item(const struct menu_level *l, i32 x, i32 y)
{
    i32 i;

    if (x < l->r.x + 1 || x >= l->r.x + l->r.w - 1 || y < l->r.y + 1)
        return -1;
    i = (y - l->r.y - 1) / ITEM_H;
    return i < (i32)l->m->n ? i : -1;
}

void menu_open(struct menu_state *ms, const struct menu *m, i32 x, i32 y)
{
    /* pointer sits just left of the first item, centered on it */
    ms->lvl[0] = (struct menu_level){m, layout(ms, m, x + 4, y - 9), -1};
    ms->depth = 1;
    ms->tracking = true;
    ms->moved = false;
    ms->x0 = x;
    ms->y0 = y;
}

void menu_close(struct menu_state *ms)
{
    ms->depth = 0;
    ms->tracking = false;
}

/* highlights the item under the pointer, opening or closing submenus */
static void track(struct menu_state *ms, i32 x, i32 y)
{
    for (usize k = ms->depth; k-- > 0;) {
        struct menu_level *l = &ms->lvl[k];
        i32 i = hit_item(l, x, y);
        const struct menu *sub;

        if (i < 0)
            continue;
        sub = l->m->items[i].sub;
        if (l->hl == i && ms->depth > k + 1 && ms->lvl[k + 1].m == sub)
            return;
        l->hl = i;
        ms->depth = k + 1;
        if (sub && ms->depth < MENU_DEPTH) {
            ms->lvl[k + 1] = (struct menu_level){
                sub,
                layout(ms, sub, l->r.x + l->r.w - 1 - SUB_OVERLAP,
                       item_top(l, i) - 1),
                -1};
            ms->depth++;
        }
        return;
    }
    ms->lvl[ms->depth - 1].hl = -1;
}

static const struct menu_item *item_at(const struct menu_state *ms, i32 x,
                                       i32 y)
{
    for (usize k = ms->depth; k-- > 0;) {
        i32 i = hit_item(&ms->lvl[k], x, y);

        if (i >= 0)
            return &ms->lvl[k].m->items[i];
    }
    return NULL;
}

i32 menu_pointer(struct menu_state *ms, const struct ev *e, bool *redraw)
{
    const struct menu_item *it;

    switch (e->type) {
    case EV_DOWN:
        *redraw = true;
        if (!item_at(ms, e->x, e->y)) {
            menu_close(ms);
            return 0;
        }
        ms->tracking = true;
        ms->moved = true;
        track(ms, e->x, e->y);
        return 0;
    case EV_MOTION:
        /* pens have no hover, so only a held button tracks */
        if (!ms->tracking)
            return 0;
        if (e->x - ms->x0 > DRAG_SLOP || ms->x0 - e->x > DRAG_SLOP ||
            e->y - ms->y0 > DRAG_SLOP || ms->y0 - e->y > DRAG_SLOP)
            ms->moved = true;
        track(ms, e->x, e->y);
        *redraw = true;
        return 0;
    case EV_UP:
        if (!ms->tracking)
            return 0;
        ms->tracking = false;
        it = item_at(ms, e->x, e->y);
        if (it && !it->sub) {
            menu_close(ms);
            *redraw = true;
            return it->action;
        }
        /* a plain click, or a release on a pull-right, keeps it up */
        if (!ms->moved || it)
            return 0;
        menu_close(ms);
        *redraw = true;
        return 0;
    default:
        return 0;
    }
}

void menu_draw(const struct menu_state *ms, struct surface *s)
{
    for (usize k = 0; k < ms->depth; k++) {
        const struct menu_level *l = &ms->lvl[k];
        struct rect r = l->r;

        gfx_pattern(s, (struct rect){r.x + SHADOW, r.y + SHADOW, r.w, r.h},
                    &PAT_SHADOW);
        gfx_fill(s, r, BLACK);
        gfx_fill(s, (struct rect){r.x + 1, r.y + 1, r.w - 2, r.h - 2}, WHITE);

        for (usize i = 0; i < l->m->n; i++) {
            const struct menu_item *it = &l->m->items[i];
            i32 top = item_top(l, (i32)i);
            u8 color = BLACK;

            if ((i32)i == l->hl) {
                gfx_fill(s,
                         (struct rect){r.x + 2, top + 1, r.w - 4, ITEM_H - 2},
                         BLACK);
                color = WHITE;
            }
            text_draw(s, ms->fs, r.x + TEXT_X, top + BASELINE, it->label,
                      color);
            if (it->sub)
                gfx_bitmap(s, r.x + r.w - ARROW_X, top + 4, arrow, 9, 7, 2,
                           color);
        }
    }
}
