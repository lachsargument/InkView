#ifndef INKVIEW_BDF_H
#define INKVIEW_BDF_H

#include "types.h"

struct glyph {
    u32 cp;
    i16 dwidth;
    i16 w, h, x, y; /* BBX: y is the bottom edge, up from the baseline */
    usize off;      /* into font.bits */
};

struct font {
    struct glyph *glyphs; /* sorted by cp */
    usize nglyphs;
    u8 *bits; /* 1-bit rows, (w + 7) / 8 bytes each */
    i32 ascent, descent;
};

/* returns 0, or -1 with errno set; f is left empty on failure */
int bdf_load(struct font *f, const char *path);
void bdf_free(struct font *f);
const struct glyph *font_glyph(const struct font *f, u32 cp);

static inline usize glyph_stride(const struct glyph *g)
{
    return ((usize)g->w + 7) / 8;
}

#endif
