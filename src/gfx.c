#include "gfx.h"

#include <stdlib.h>
#include <string.h>

static struct rect intersect(struct rect a, struct rect b)
{
    i32 x0 = a.x > b.x ? a.x : b.x;
    i32 y0 = a.y > b.y ? a.y : b.y;
    i32 x1 = a.x + a.w < b.x + b.w ? a.x + a.w : b.x + b.w;
    i32 y1 = a.y + a.h < b.y + b.h ? a.y + a.h : b.y + b.h;

    if (x1 <= x0 || y1 <= y0)
        return (struct rect){0, 0, 0, 0};
    return (struct rect){x0, y0, x1 - x0, y1 - y0};
}

/* logical rect -> clipped physical rect */
static struct rect to_phys(const struct surface *s, struct rect r)
{
    struct rect p = {r.x * s->scale, r.y * s->scale, r.w * s->scale,
                     r.h * s->scale};
    return intersect(p, s->clip);
}

int surface_init(struct surface *s, i32 lw, i32 lh, i32 scale)
{
    s->scale = scale;
    s->w = lw * scale;
    s->h = lh * scale;
    s->px = malloc((usize)s->w * (usize)s->h);
    if (!s->px)
        return -1;
    memset(s->px, WHITE, (usize)s->w * (usize)s->h);
    gfx_clip(s, NULL);
    return 0;
}

void surface_free(struct surface *s)
{
    free(s->px);
    s->px = NULL;
}

void gfx_clip(struct surface *s, const struct rect *r)
{
    struct rect full = {0, 0, s->w, s->h};

    s->clip = full;
    if (r)
        s->clip = to_phys(s, *r);
}

void gfx_fill(struct surface *s, struct rect r, u8 color)
{
    struct rect p = to_phys(s, r);

    for (i32 y = p.y; y < p.y + p.h; y++)
        memset(s->px + (usize)y * (usize)s->w + (usize)p.x, color, (usize)p.w);
}

const struct pattern PAT_DESKTOP = {4, 4, {0x80, 0x80, 0x20, 0x20}};
const struct pattern PAT_SHADOW = {2, 2, {0x80, 0x40}};

void gfx_pattern(struct surface *s, struct rect r, const struct pattern *pat)
{
    struct rect p = to_phys(s, r);

    for (i32 y = p.y; y < p.y + p.h; y++) {
        u8 *row = s->px + (usize)y * (usize)s->w;
        u8 bits = pat->rows[(y / s->scale) % pat->h];

        for (i32 x = p.x; x < p.x + p.w; x++)
            row[x] = bits & (0x80 >> ((x / s->scale) % pat->w)) ? BLACK : WHITE;
    }
}

void gfx_bitmap(struct surface *s, i32 x, i32 y, const u8 *bits, i32 w, i32 h,
                usize stride, u8 color)
{
    for (i32 r = 0; r < h; r++) {
        const u8 *row = bits + (usize)r * stride;

        for (i32 c = 0; c < w; c++)
            if (row[c / 8] & (0x80 >> (c % 8)))
                gfx_fill(s, (struct rect){x + c, y + r, 1, 1}, color);
    }
}
