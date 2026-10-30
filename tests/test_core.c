#include <stdio.h>
#include <stdlib.h>

#include "../src/wm.h"

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__,   \
                    #cond);                                                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static u32 decode(const char *s, usize *len)
{
    const char *p = s;
    u32 cp = utf8_next(&p);

    *len = (usize)(p - s);
    return cp;
}

static void test_utf8(void)
{
    usize n;

    CHECK(decode("A", &n) == 'A' && n == 1);
    CHECK(decode("\xe4\xb8\xad", &n) == 0x4e2d && n == 3);      /* 中 */
    CHECK(decode("\xf0\x9f\x98\x80", &n) == 0x1f600 && n == 4); /* emoji */
    CHECK(decode("\xc0\x80", &n) == 0xfffd);                    /* overlong */
    CHECK(decode("\xed\xa0\x80", &n) == 0xfffd);                /* surrogate */
    CHECK(decode("\xff", &n) == 0xfffd && n == 1);
    /* truncated sequence stops at the NUL instead of reading past it */
    CHECK(decode("\xe4\xb8", &n) == 0xfffd && n == 2);
}

static void test_font(const char *dir)
{
    char path[1024];
    struct font f;
    struct fontset fs = {{&f}, 1};
    const struct glyph *m;

    snprintf(path, sizeof(path), "%s/inkscreen14.bdf", dir);
    CHECK(bdf_load(&f, path) == 0);
    m = font_glyph(&f, 'M');
    CHECK(m && m->dwidth == 8);
    CHECK(font_glyph(&f, 0x4e2d) == NULL);
    CHECK(text_width(&fs, "AB") == 16);
    /* missing glyphs fall back to '?' rather than vanishing */
    CHECK(text_width(&fs, "\xe4\xb8\xad") == 8);
    bdf_free(&f);

    CHECK(bdf_load(&f, "/nonexistent.bdf") == -1);
}

/* fonts re-encoded from JIS by tools/bdfuni.py */
static void test_converted(const char *dir)
{
    char path[1024];
    struct font f;

    snprintf(path, sizeof(path), "%s/unicode/jiskan16.bdf", dir);
    CHECK(bdf_load(&f, path) == 0);
    CHECK(font_glyph(&f, 0x8a9e));          /* 語, from JIS X 0208 */
    CHECK(font_glyph(&f, 0x304b));          /* か */
    CHECK(font_glyph(&f, 0xff76));          /* ｶ, merged from 8x16rk */
    CHECK(font_glyph(&f, 0x4e2d)->w == 16); /* 中 keeps its bitmap */
    bdf_free(&f);
}

static bool black(const struct surface *s, i32 x, i32 y)
{
    return s->px[(usize)y * (usize)s->w + (usize)x] == BLACK;
}

/* pixel positions measured from the SunView capture sv411exit.png */
static void test_patterns(void)
{
    struct surface s;

    CHECK(surface_init(&s, 8, 8, 1) == 0);
    gfx_pattern(&s, (struct rect){0, 0, 8, 8}, &PAT_DESKTOP);
    CHECK(black(&s, 0, 0) && black(&s, 0, 1) && black(&s, 4, 0));
    CHECK(black(&s, 2, 2) && black(&s, 2, 3) && black(&s, 6, 7));
    CHECK(!black(&s, 1, 0) && !black(&s, 2, 0) && !black(&s, 0, 2));
    /* anchored to the screen, not to the filled rect */
    gfx_pattern(&s, (struct rect){1, 1, 7, 7}, &PAT_SHADOW);
    CHECK(black(&s, 2, 2) && !black(&s, 2, 3) && black(&s, 3, 3));
    surface_free(&s);

    CHECK(surface_init(&s, 4, 4, 2) == 0);
    gfx_pattern(&s, (struct rect){0, 0, 4, 4}, &PAT_DESKTOP);
    /* one logical pixel is a 2x2 block */
    CHECK(black(&s, 0, 0) && black(&s, 1, 1) && !black(&s, 2, 0));
    CHECK(black(&s, 4, 4) && black(&s, 5, 7) && !black(&s, 0, 4));
    surface_free(&s);
}

static i32 send(struct menu_state *ms, enum ev_type t, i32 x, i32 y)
{
    struct ev e = {t, x, y, PTR_RIGHT};
    bool redraw = false;

    return menu_pointer(ms, &e, &redraw);
}

/* item i of level k, at its text */
static i32 ix(const struct menu_state *ms, usize k)
{
    return ms->lvl[k].r.x + 20;
}

static i32 iy(const struct menu_state *ms, usize k, i32 i)
{
    return ms->lvl[k].r.y + 1 + 17 * i + 8;
}

static void test_menu(const char *dir)
{
    char path[1024];
    struct font f;
    struct fontset fs = {{&f}, 1};
    static const struct menu_item sub_items[] = {{"Leaf", NULL, 7}};
    static const struct menu sub = {sub_items, 1};
    static const struct menu_item items[] = {
        {"Open", NULL, 1}, {"More", &sub, 0}, {"Exit", NULL, 2}};
    static const struct menu m = {items, 3};
    struct menu_state ms = {.fs = &fs, .sw = 1240, .sh = 930};

    snprintf(path, sizeof(path), "%s/inkscreen14b.bdf", dir);
    CHECK(bdf_load(&f, path) == 0);

    /* geometry measured from SunView: 17px rows plus a 1px border */
    menu_open(&ms, &m, 100, 100);
    CHECK(ms.lvl[0].r.h == 17 * 3 + 2);
    CHECK(ms.lvl[0].r.x == 104);

    /* press, drag, release on a leaf picks it */
    send(&ms, EV_MOTION, ix(&ms, 0), iy(&ms, 0, 2));
    CHECK(ms.lvl[0].hl == 2);
    CHECK(send(&ms, EV_UP, ix(&ms, 0), iy(&ms, 0, 2)) == 2);
    CHECK(ms.depth == 0);

    /* a click without moving leaves the menu up for tapping */
    menu_open(&ms, &m, 100, 100);
    CHECK(send(&ms, EV_UP, 100, 100) == 0 && ms.depth == 1);
    /* tapping a pull-right opens the submenu on the same row */
    send(&ms, EV_DOWN, ix(&ms, 0), iy(&ms, 0, 1));
    CHECK(ms.depth == 2);
    CHECK(ms.lvl[1].r.y == ms.lvl[0].r.y + 17);
    CHECK(send(&ms, EV_UP, ix(&ms, 0), iy(&ms, 0, 1)) == 0 && ms.depth == 2);
    send(&ms, EV_DOWN, ix(&ms, 1), iy(&ms, 1, 0));
    CHECK(send(&ms, EV_UP, ix(&ms, 1), iy(&ms, 1, 0)) == 7);
    CHECK(ms.depth == 0);

    /* tapping outside closes without choosing */
    menu_open(&ms, &m, 100, 100);
    send(&ms, EV_UP, 100, 100);
    CHECK(send(&ms, EV_DOWN, 1000, 800) == 0 && ms.depth == 0);

    /* menus are pushed back on screen, shadow included */
    menu_open(&ms, &m, 1230, 920);
    CHECK(ms.lvl[0].r.x + ms.lvl[0].r.w + 6 <= 1240);
    CHECK(ms.lvl[0].r.y + ms.lvl[0].r.h + 6 <= 930);
    bdf_free(&f);
}

/* frame menu reached by right-pressing a title bar */
static void test_frame_menu(const char *dir)
{
    char path[1024];
    struct font f;
    struct fontset fs = {{&f}, 1};
    struct window a = {{10, 10, 300, 200}, "a", NULL, NULL};
    struct window b = {{50, 50, 300, 200}, "b", NULL, NULL};
    struct desktop d = {.w = 1240, .h = 930, .title_font = &fs};
    struct ev e = {EV_DOWN, 60, 55, PTR_RIGHT};
    i32 y;

    snprintf(path, sizeof(path), "%s/inkscreen14b.bdf", dir);
    CHECK(bdf_load(&f, path) == 0);
    d.menu = (struct menu_state){.fs = &fs, .sw = 1240, .sh = 930};
    wm_add(&d, &a);
    wm_add(&d, &b);

    wm_pointer(&d, &e);
    CHECK(d.menu.depth == 1 && d.menu_win == &b);
    /* drag down to "Back" (item 3) and release */
    y = d.menu.lvl[0].r.y + 1 + 17 * 3 + 8;
    e = (struct ev){EV_MOTION, d.menu.lvl[0].r.x + 20, y, 0};
    wm_pointer(&d, &e);
    e.type = EV_UP;
    e.button = PTR_RIGHT;
    wm_pointer(&d, &e);
    CHECK(d.menu.depth == 0);
    CHECK(d.win[0] == &b && d.win[1] == &a);
    bdf_free(&f);
}

/* ASCII + Latin-1, every glyph has the advance; ink stays within rows
 * 10 .. -3 except accents on capitals, which may reach row 11 */
static void check_inkscreen_font(const struct font *f, i16 advance)
{
    CHECK(f->nglyphs == 191 && f->ascent == 12 && f->descent == 4);
    for (usize i = 0; i < f->nglyphs; i++) {
        const struct glyph *g = &f->glyphs[i];

        CHECK(g->dwidth == advance && g->w == advance);
        for (i32 r = 0; r < g->h; r++) {
            const u8 *row = f->bits + g->off + (usize)r * glyph_stride(g);
            i32 y = g->y + g->h - 1 - r;

            for (usize b = 0; b < glyph_stride(g); b++)
                if (row[b])
                    CHECK(y <= (g->cp < 0xC0 ? 10 : 11) && y >= -3);
        }
    }
}

/* Inkscreen, built in the caffeine-type repo: 14-row design in a 16-row cell
 */
static void test_inkscreen(const char *dir)
{
    char path[1024];
    struct font f, cjk;
    struct fontset fs = {{&f, &cjk}, 2};
    struct surface s;
    i32 top = -1;

    /* bold is one pixel wider, like SunOS screen.b.14 */
    snprintf(path, sizeof(path), "%s/inkscreen14b.bdf", dir);
    CHECK(bdf_load(&f, path) == 0);
    check_inkscreen_font(&f, 9);
    bdf_free(&f);

    snprintf(path, sizeof(path), "%s/inkscreen14.bdf", dir);
    CHECK(bdf_load(&f, path) == 0);
    check_inkscreen_font(&f, 8);

    /* a 16px CJK fallback lines up with the top of Inkscreen's line box */
    snprintf(path, sizeof(path), "%s/fangsongti16.bdf", dir);
    CHECK(bdf_load(&cjk, path) == 0);
    CHECK(surface_init(&s, 16, 16, 1) == 0);
    text_draw(&s, &fs, 0, f.ascent, "\xe4\xb8\x80", BLACK); /* 一 */
    for (i32 y = 0; y < 16 && top < 0; y++)
        for (i32 xx = 0; xx < 16; xx++)
            if (black(&s, xx, y))
                top = y;
    /* same row as when fangsong draws alone on its own 14px ascent */
    CHECK(top >= 0);
    gfx_fill(&s, (struct rect){0, 0, 16, 16}, WHITE);
    fs = (struct fontset){{&cjk}, 1};
    text_draw(&s, &fs, 0, cjk.ascent, "\xe4\xb8\x80", BLACK);
    for (i32 xx = 0; xx < 16; xx++)
        if (black(&s, xx, top))
            top = -2;
    CHECK(top == -2);
    surface_free(&s);
    bdf_free(&cjk);
    bdf_free(&f);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FONTDIR\n", argv[0]);
        return 2;
    }
    test_utf8();
    test_patterns();
    test_font(argv[1]);
    test_converted(argv[1]);
    test_inkscreen(argv[1]);
    test_menu(argv[1]);
    test_frame_menu(argv[1]);
    puts("ok");
    return 0;
}
