#include "text.h"

#define REPLACEMENT 0xfffd

u32 utf8_next(const char **s)
{
    const u8 *p = (const u8 *)*s;
    u32 cp, min;
    int n;

    if (p[0] < 0x80) {
        *s += 1;
        return p[0];
    }
    if ((p[0] & 0xe0) == 0xc0) {
        n = 1;
        cp = p[0] & 0x1f;
        min = 0x80;
    } else if ((p[0] & 0xf0) == 0xe0) {
        n = 2;
        cp = p[0] & 0x0f;
        min = 0x800;
    } else if ((p[0] & 0xf8) == 0xf0) {
        n = 3;
        cp = p[0] & 0x07;
        min = 0x10000;
    } else {
        *s += 1;
        return REPLACEMENT;
    }
    /* a NUL fails the continuation test, so we never run past the end */
    for (int i = 1; i <= n; i++) {
        if ((p[i] & 0xc0) != 0x80) {
            *s += i;
            return REPLACEMENT;
        }
        cp = cp << 6 | (p[i] & 0x3f);
    }
    *s += n + 1;
    if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        return REPLACEMENT;
    return cp;
}

static const struct glyph *lookup(const struct fontset *fs, u32 cp,
                                  const struct font **owner)
{
    for (usize i = 0; i < fs->n; i++) {
        const struct glyph *g = font_glyph(fs->f[i], cp);

        if (g) {
            *owner = fs->f[i];
            return g;
        }
    }
    *owner = fs->f[0];
    return font_glyph(fs->f[0], '?');
}

/* s == NULL only measures */
static i32 run(struct surface *s, const struct fontset *fs, i32 x, i32 baseline,
               const char *str, u8 color)
{
    i32 pen = x;

    while (*str) {
        const struct font *f;
        const struct glyph *g = lookup(fs, utf8_next(&str), &f);

        if (!g)
            continue;
        /* fallback fonts share the primary's line box top, not its
         * baseline: Inkscreen sits 12/4 in a 16px cell, CJK fonts 14/2 */
        if (s)
            gfx_bitmap(s, pen + g->x,
                       baseline - fs->f[0]->ascent + f->ascent - g->y - g->h,
                       f->bits + g->off, g->w, g->h, glyph_stride(g), color);
        pen += g->dwidth;
    }
    return pen - x;
}

i32 text_draw(struct surface *s, const struct fontset *fs, i32 x, i32 baseline,
              const char *str, u8 color)
{
    return run(s, fs, x, baseline, str, color);
}

i32 text_width(const struct fontset *fs, const char *str)
{
    return run(NULL, fs, 0, 0, str, BLACK);
}
