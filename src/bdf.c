#include "bdf.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NO_CP UINT32_MAX
/* fonts come from users via fontbook; cap glyph size before allocating */
#define MAX_GLYPH_DIM 256

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int parse_row(const char *line, u8 *out, usize n)
{
    for (usize i = 0; i < n; i++) {
        int hi = hexval(line[2 * i]);
        int lo = hi < 0 ? -1 : hexval(line[2 * i + 1]);

        if (lo < 0)
            return -1;
        out[i] = (u8)(hi << 4 | lo);
    }
    return 0;
}

static int glyph_cmp(const void *a, const void *b)
{
    u32 x = ((const struct glyph *)a)->cp;
    u32 y = ((const struct glyph *)b)->cp;

    return (x > y) - (x < y);
}

static int grow(void **p, usize *cap, usize need, usize elem)
{
    usize n = *cap ? *cap : 64;
    void *q;

    if (need <= *cap)
        return 0;
    while (n < need)
        n *= 2;
    q = realloc(*p, n * elem);
    if (!q)
        return -1;
    *p = q;
    *cap = n;
    return 0;
}

int bdf_load(struct font *f, const char *path)
{
    FILE *fp = fopen(path, "r");
    char *line = NULL;
    size_t linecap = 0;
    struct glyph g = {0};
    bool in_char = false, have_asc = false, have_desc = false;
    i32 bb_h = 0, bb_y = 0;
    usize gcap = 0, bcap = 0, blen = 0;
    int ret = -1;

    memset(f, 0, sizeof(*f));
    if (!fp)
        return -1;
    errno = EINVAL;

    while (getline(&line, &linecap, fp) > 0) {
        int a, b, c, d;

        if (sscanf(line, "FONTBOUNDINGBOX %d %d %d %d", &a, &b, &c, &d) == 4) {
            bb_h = b;
            bb_y = d;
        } else if (sscanf(line, "FONT_ASCENT %d", &a) == 1) {
            f->ascent = a;
            have_asc = true;
        } else if (sscanf(line, "FONT_DESCENT %d", &a) == 1) {
            f->descent = a;
            have_desc = true;
        } else if (strncmp(line, "STARTCHAR", 9) == 0) {
            memset(&g, 0, sizeof(g));
            g.cp = NO_CP;
            in_char = true;
        } else if (!in_char) {
            continue;
        } else if (sscanf(line, "ENCODING %d", &a) == 1) {
            /* "ENCODING -1 n" glyphs are outside the charset; skip them */
            g.cp = a >= 0 ? (u32)a : NO_CP;
        } else if (sscanf(line, "DWIDTH %d", &a) == 1) {
            g.dwidth = (i16)a;
        } else if (sscanf(line, "BBX %d %d %d %d", &a, &b, &c, &d) == 4) {
            if (a < 0 || b < 0 || a > MAX_GLYPH_DIM || b > MAX_GLYPH_DIM ||
                c < -MAX_GLYPH_DIM || c > MAX_GLYPH_DIM || d < -MAX_GLYPH_DIM ||
                d > MAX_GLYPH_DIM)
                goto out;
            g.w = (i16)a;
            g.h = (i16)b;
            g.x = (i16)c;
            g.y = (i16)d;
        } else if (strncmp(line, "BITMAP", 6) == 0) {
            usize stride = glyph_stride(&g);
            usize need = stride * (usize)g.h;

            if (grow((void **)&f->bits, &bcap, blen + need, 1))
                goto out;
            g.off = blen;
            for (i32 r = 0; r < g.h; r++)
                if (getline(&line, &linecap, fp) <= 0 ||
                    parse_row(line, f->bits + blen + (usize)r * stride, stride))
                    goto out;
            blen += need;
        } else if (strncmp(line, "ENDCHAR", 7) == 0) {
            in_char = false;
            if (g.cp == NO_CP)
                continue;
            if (grow((void **)&f->glyphs, &gcap, f->nglyphs + 1, sizeof(g)))
                goto out;
            f->glyphs[f->nglyphs++] = g;
        }
    }

    if (!have_asc)
        f->ascent = bb_h + bb_y;
    if (!have_desc)
        f->descent = -bb_y;
    if (f->nglyphs)
        qsort(f->glyphs, f->nglyphs, sizeof(*f->glyphs), glyph_cmp);
    ret = f->nglyphs ? 0 : -1;

out:
    free(line);
    fclose(fp);
    if (ret)
        bdf_free(f);
    return ret;
}

void bdf_free(struct font *f)
{
    free(f->glyphs);
    free(f->bits);
    memset(f, 0, sizeof(*f));
}

const struct glyph *font_glyph(const struct font *f, u32 cp)
{
    usize lo = 0, hi = f->nglyphs;

    while (lo < hi) {
        usize mid = lo + (hi - lo) / 2;

        if (f->glyphs[mid].cp == cp)
            return &f->glyphs[mid];
        if (f->glyphs[mid].cp < cp)
            lo = mid + 1;
        else
            hi = mid;
    }
    return NULL;
}
