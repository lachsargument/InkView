#ifndef INKVIEW_GFX_H
#define INKVIEW_GFX_H

#include "types.h"

enum { BLACK = 0, WHITE = 255 };

struct rect {
    i32 x, y, w, h;
};

/*
 * 8-bit grayscale canvas. Callers draw in logical pixels; each logical
 * pixel covers scale x scale physical pixels. The canvas is physical
 * (not upscaled at present time) so vector text can later be rasterized
 * at full device resolution while bitmap UI stays pixel-doubled.
 */
struct surface {
    u8 *px;
    i32 w, h; /* physical */
    i32 scale;
    struct rect clip; /* physical */
};

int surface_init(struct surface *s, i32 lw, i32 lh, i32 scale);
void surface_free(struct surface *s);

/* r == NULL resets to the whole surface */
void gfx_clip(struct surface *s, const struct rect *r);

/* 1-bit tile up to 8x8, bit 7 of rows[y] is the leftmost pixel */
struct pattern {
    u8 w, h;
    u8 rows[8];
};

/* measured from a native 1152x900 SunView framebuffer capture */
extern const struct pattern PAT_DESKTOP; /* 4x4 short dashes, 25% */
extern const struct pattern PAT_SHADOW;  /* 50% checkerboard */

void gfx_fill(struct surface *s, struct rect r, u8 color);
/* tiles are anchored to the screen origin so moving windows never
 * shifts the pattern underneath them */
void gfx_pattern(struct surface *s, struct rect r, const struct pattern *p);
/* 1-bit MSB-first bitmap; set bits are painted, clear bits untouched */
void gfx_bitmap(struct surface *s, i32 x, i32 y, const u8 *bits, i32 w, i32 h,
                usize stride, u8 color);

static inline bool rect_hit(struct rect r, i32 x, i32 y)
{
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

#endif
