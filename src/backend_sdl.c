#include "backend.h"

#include <SDL3/SDL.h>
#include <stdio.h>

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;

static int fail(void)
{
    fprintf(stderr, "inkview: %s\n", SDL_GetError());
    return -1;
}

static void to_xrgb(const struct surface *s, void *dst, int pitch)
{
    for (i32 y = 0; y < s->h; y++) {
        u32 *row = (u32 *)((u8 *)dst + (usize)y * (usize)pitch);
        const u8 *src = s->px + (usize)y * (usize)s->w;

        for (i32 x = 0; x < s->w; x++)
            row[x] = 0x010101u * src[x];
    }
}

int backend_open(const char *title, const struct surface *s)
{
    i32 lw = s->w / s->scale, lh = s->h / s->scale;
    float fit = 1.0f;
    SDL_Rect usable;

    if (!SDL_Init(SDL_INIT_VIDEO))
        return fail();
    /* the portrait Kindle screen is taller than most laptop displays */
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable))
        fit = SDL_min(1.0f, SDL_min(0.95f * (float)usable.w / (float)lw,
                                    0.95f * (float)usable.h / (float)lh));
    if (!SDL_CreateWindowAndRenderer(
            title, (int)((float)lw * fit), (int)((float)lh * fit),
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &win, &ren))
        return fail();
    if (!SDL_SetRenderLogicalPresentation(ren, s->w, s->h,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX))
        return fail();
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888,
                            SDL_TEXTUREACCESS_STREAMING, s->w, s->h);
    if (!tex)
        return fail();
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
    return 0;
}

/* ponytail: full-frame upload; damage rects come with the e-ink backend */
void backend_present(const struct surface *s)
{
    void *px;
    int pitch;

    if (!SDL_LockTexture(tex, NULL, &px, &pitch))
        return;
    to_xrgb(s, px, pitch);
    SDL_UnlockTexture(tex);
    SDL_SetRenderDrawColor(ren, 64, 64, 64, 255);
    SDL_RenderClear(ren);
    SDL_RenderTexture(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

static u32 map_button(Uint8 b)
{
    switch (b) {
    case SDL_BUTTON_LEFT:
        return PTR_LEFT;
    case SDL_BUTTON_MIDDLE:
        return PTR_MIDDLE;
    case SDL_BUTTON_RIGHT:
        return PTR_RIGHT;
    default:
        return 0;
    }
}

bool backend_wait(struct ev *e, const struct surface *s)
{
    SDL_Event sev;

    for (;;) {
        if (!SDL_WaitEvent(&sev))
            return false;
        /* window coordinates -> physical surface pixels */
        SDL_ConvertEventToRenderCoordinates(ren, &sev);

        switch (sev.type) {
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            e->type = EV_EXPOSE;
            return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            e->button = map_button(sev.button.button);
            if (!e->button)
                continue;
            e->type = sev.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EV_DOWN : EV_UP;
            e->x = (i32)(sev.button.x / (float)s->scale);
            e->y = (i32)(sev.button.y / (float)s->scale);
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            e->type = EV_MOTION;
            e->button = 0;
            e->x = (i32)(sev.motion.x / (float)s->scale);
            e->y = (i32)(sev.motion.y / (float)s->scale);
            return true;
        default:
            continue;
        }
    }
}

int backend_snapshot(const struct surface *s, const char *path)
{
    SDL_Surface *out = SDL_CreateSurface(s->w, s->h, SDL_PIXELFORMAT_XRGB8888);
    bool ok;

    if (!out)
        return fail();
    to_xrgb(s, out->pixels, out->pitch);
    ok = SDL_SaveBMP(out, path);
    SDL_DestroySurface(out);
    return ok ? 0 : fail();
}

void backend_close(void)
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
