#ifndef INKVIEW_BACKEND_H
#define INKVIEW_BACKEND_H

#include "event.h"
#include "gfx.h"

/* one implementation per platform: SDL3 on desktop, framebuffer on Kindle */
int backend_open(const char *title, const struct surface *s);
void backend_present(const struct surface *s);
/* blocks for the next event; returns false on quit */
bool backend_wait(struct ev *e, const struct surface *s);
int backend_snapshot(const struct surface *s, const char *path);
void backend_close(void);

#endif
