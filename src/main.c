#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"
#include "wm.h"

/* Kindle Scribe 2480x1860 landscape at integer x2; the panel is portrait,
 * so the Kindle backend rotates on present and un-rotates pen input */
#define LOGICAL_W 1240
#define LOGICAL_H 930
/* terminal cell: Inkscreen 8x16, CJK 16x16 */
#define LINE_H 16

struct lines {
    const struct fontset *fs;
    const char *const *text;
    usize n;
};

static struct font inkscreen, inkscreenb, fangsong16, jiskan16;
static struct fontset ui_fs, menu_fs, term_fs;

enum { ACT_REDISPLAY = 1, ACT_EXIT };

/* placeholders until the tools exist: action 0 just closes the menu */
static const struct menu_item toolchest_items[] = {
    {"terminal", NULL, 0}, {"textedit", NULL, 0},  {"writepad", NULL, 0},
    {"fileview", NULL, 0}, {"bookview", NULL, 0},  {"webview", NULL, 0},
    {"imgview", NULL, 0},  {"audioplay", NULL, 0},
};
static const struct menu_item games_items[] = {
    {"sudoku", NULL, 0},
    {"chess", NULL, 0},
};
static const struct menu_item utilities_items[] = {
    {"clock", NULL, 0},
    {"calc", NULL, 0},
    {"osview", NULL, 0},
    {"localsend", NULL, 0},
};
static const struct menu_item services_items[] = {
    {"Redisplay All", NULL, ACT_REDISPLAY},
    {"Lock Screen", NULL, 0},
    {"Save Layout", NULL, 0},
};
static const struct menu toolchest_menu = {toolchest_items, 8};
static const struct menu games_menu = {games_items, 2};
static const struct menu utilities_menu = {utilities_items, 4};
static const struct menu services_menu = {services_items, 3};
static const struct menu_item root_items[] = {
    {"Toolchest", &toolchest_menu, 0}, {"Games", &games_menu, 0},
    {"Utilities", &utilities_menu, 0}, {"Services", &services_menu, 0},
    {"Exit InkView", NULL, ACT_EXIT},
};
static const struct menu root_menu = {root_items, 5};

static const char *const console_text[] = {
    "InkView 0.0.1",
    "display: 1240x930 logical, x2, landscape",
    "fonts: inkscreen14 fangsongti16 jiskan16",
};

static const char *const term_text[] = {
    "inkview% ls",
    "desktop   developer document  download",
    "inkview% echo 中文 繁體語言 日本語 かな ｶﾅ",
    "中文 繁體語言 日本語 かな ｶﾅ",
    "inkview% _",
};

static void draw_lines(struct window *w, struct surface *s, struct rect c)
{
    const struct lines *l = w->data;

    for (usize i = 0; i < l->n; i++)
        text_draw(s, l->fs, c.x + 4,
                  c.y + 2 + (i32)i * LINE_H + l->fs->f[0]->ascent, l->text[i],
                  BLACK);
}

static int load_font(struct font *f, const char *name)
{
    const char *dir = getenv("INKVIEW_FONTDIR");
    char path[1024];

    snprintf(path, sizeof(path), "%s/%s", dir ? dir : INKVIEW_FONTDIR, name);
    if (bdf_load(f, path)) {
        fprintf(stderr, "inkview: cannot load font %s\n", path);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *scale_env = getenv("INKVIEW_SCALE");
    const char *snapshot = NULL;
    struct surface s;
    struct desktop d = {.w = LOGICAL_W, .h = LOGICAL_H};
    struct lines console_l = {&term_fs, console_text, 3};
    struct lines term_l = {&term_fs, term_text, 5};
    struct window console = {
        {0, 0, LOGICAL_W, 80}, "<< CONSOLE >>", draw_lines, &console_l};
    struct window term = {
        {60, 160, 640, 420}, "terminal - /bin/sh", draw_lines, &term_l};
    struct ev e;

    if (argc == 3 && strcmp(argv[1], "--snapshot") == 0)
        snapshot = argv[2];

    if (load_font(&inkscreen, "inkscreen14.bdf") ||
        load_font(&inkscreenb, "inkscreen14b.bdf") ||
        load_font(&fangsong16, "fangsongti16.bdf") ||
        load_font(&jiskan16, "unicode/jiskan16.bdf"))
        return 1;
    ui_fs = (struct fontset){{&inkscreen}, 1};
    /* Chinese, then Japanese kana/kanji; Inksans CJK replaces both */
    term_fs = (struct fontset){{&inkscreen, &fangsong16, &jiskan16}, 3};
    menu_fs = (struct fontset){{&inkscreenb, &fangsong16, &jiskan16}, 3};

    if (surface_init(&s, LOGICAL_W, LOGICAL_H, scale_env ? atoi(scale_env) : 2))
        return 1;
    d.title_font = &ui_fs;
    d.root_menu = &root_menu;
    d.menu.fs = &menu_fs;
    d.menu.sw = LOGICAL_W;
    d.menu.sh = LOGICAL_H;
    wm_add(&d, &console);
    wm_add(&d, &term);
    d.selected = &term;
    wm_draw(&d, &s);

    if (snapshot)
        return backend_snapshot(&s, snapshot) ? 1 : 0;

    if (backend_open("InkView", &s))
        return 1;
    backend_present(&s);
    while (backend_wait(&e, &s)) {
        bool redraw = e.type == EV_EXPOSE || wm_pointer(&d, &e);

        if (d.root_action == ACT_EXIT)
            break;
        /* ponytail: Redisplay is a plain repaint until the e-ink backend
         * can issue a full GC16 flash to clear ghosting */
        if (d.root_action == ACT_REDISPLAY)
            redraw = true;
        d.root_action = 0;
        if (redraw) {
            wm_draw(&d, &s);
            backend_present(&s);
        }
    }
    backend_close();
    surface_free(&s);
    return 0;
}
