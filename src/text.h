#ifndef INKVIEW_TEXT_H
#define INKVIEW_TEXT_H

#include "bdf.h"
#include "gfx.h"

#define FONTSET_MAX 4

/* fallback chain: f[0] is the primary font and supplies the metrics */
struct fontset {
    const struct font *f[FONTSET_MAX];
    usize n;
};

/* decodes one code point and advances *s; malformed input -> U+FFFD */
u32 utf8_next(const char **s);

/* draws at the baseline, returns the advance in logical pixels */
i32 text_draw(struct surface *s, const struct fontset *fs, i32 x, i32 baseline,
              const char *str, u8 color);
i32 text_width(const struct fontset *fs, const char *str);

#endif
