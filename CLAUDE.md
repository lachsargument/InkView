# InkView

An e-ink desktop environment for the Kindle Scribe (first target),
portable to desktop Linux/BSD. BSD-2-Clause. The roadmap is in
`PLAN.md`. Device and cross-build
notes live in `../CLAUDE.md`.

## Commands

```sh
sh tools/fetch-fonts.sh        # once; large fonts, pinned and checked
meson setup build              # once; needs meson, ninja, pkg-config, sdl3
ninja -C build                 # build
meson test -C build            # run tests/test_core.c
./build/inkview                # SDL3 window (INKVIEW_SCALE=1|2, default 2)
./build/inkview --snapshot out.bmp   # render one frame, no window
clang-format -i src/*.[ch] tests/*.c # format C (.clang-format)
uvx ruff format tools/*.py           # format Python (ruff.toml)
```

Inkscreen (`fonts/inkscreen14.bdf`, `fonts/inkscreen14b.bdf`,
`fonts/OFL.txt`) is built in its own repo,
`/Users/khangai/Developer/GitHub/caffeine-type` (generator, pixel
editor, TTF converter, release script); copy the BDFs here after a
rebuild:

```sh
cp ../../GitHub/caffeine-type/fonts/inkscreen14*.bdf ../../GitHub/caffeine-type/fonts/OFL.txt fonts/
```

Fonts in git: only the small BDFs. Everything large or downloadable
(Inksans CJK W3/W6, FreeFont, Noto Emoji, Courier 10 Pitch, Unifont)
comes from `tools/fetch-fonts.sh` with pinned URLs and SHA-256 and is
gitignored. To update one, change its URL and hash in the script and
delete its marker file. Sources and licenses: `fonts/README.md`.

Regenerate the Unicode-encoded CJK fonts (committed in
`fonts/unicode/`) after changing `tools/bdfuni.py`:

```sh
python3 tools/bdfuni.py fonts/unicode/jiskan16.bdf fonts/jiskan16.bdf+fonts/8x16rk.bdf
python3 tools/bdfuni.py fonts/unicode/k14.bdf fonts/k14.bdf
```

`INKVIEW_FONTDIR` overrides the font directory (default: `fonts/` in
the source tree).

## Layout

- `src/gfx.[ch]` - 8-bit gray surface, logical coords x integer scale,
  fills, 1-bit tile patterns, bitmaps
- `src/bdf.[ch]` - BDF bitmap font loader (fonts are untrusted input)
- `src/text.[ch]` - UTF-8 decoding, fallback font chains
- `src/menu.[ch]` - pop-up and pull-right menus, reusable by tools
- `src/wm.[ch]` - window manager: frames, stacking, select, drag,
  root and frame menus
- `src/event.h` - input events; pen maps to left/right/middle
- `src/backend.h` - platform interface; `backend_sdl.c` is desktop
- `fonts/` - bundled fonts (see licensing below)

## Design rules

- Specs are in 1x logical pixels; the Kindle runs at integer x2
  in landscape (2480x1860 -> 1240x930). The panel is portrait, so the
  Kindle backend rotates frames on present and rotates pen/touch input
  back; nothing above the backend knows about it. The surface is
  physical so vector text can
  render at full resolution later; bitmap UI is pixel-doubled.
- Keep the UI pure black/white where possible: 1-bit content can use
  fast e-ink waveforms (A2/DU); gray needs slow GC16.
- Every middle-button action must also be reachable with left/right.
- Never name UI styles after SunView, IRIX, or Windows (trademarks);
  use descriptive names.
- Only `backend_*.c` may be platform specific.
- Architecture (decided 2026-10-30): like Windows 3.1, InkView is one
  process that owns the screen; tools are modules inside it, not
  separate client processes. Keep a clear tool interface (create
  window, draw, receive events) so a protocol layer for external
  tools could be added later without changing the tools.
- UI/UX is tuned on macOS with the SDL backend first; no e-ink
  simulation in the SDL backend.

## Workstation-style frame (1x)

Title bar 18px black, title in Inkscreen 14 white, 8px left
inset. Left/right/bottom border 5px = 2 black + 1 white + 2 black; the
white line turns black when the window is selected.

## Patterns and popups (1x)

Measured from `/Users/khangai/Developer/Sun/SunOS/SunView/sv411exit.png`
(native 1152x900 1-bit). All tiles are anchored to the screen origin.

- Desktop: 4x4 tile `#...` `#...` `..#.` `..#.` (`PAT_DESKTOP`).
- Popup shadow (dialogs, menus): the popup rect offset +6,+6, drawn
  behind it with a 50% checkerboard, black where x+y is even
  (`PAT_SHADOW`).
- Dialog frame: solid 4px black border, no title bar.
- Menu (`src/menu.c`): 1px black border, Inkscreen 14 Bold
  (`inkscreen14b`), 17px rows, height = 17n + 2. Text 18px from the left
  border, baseline 12px below the row top. Highlight is a black bar
  with white text, inset 1px (15px tall). Pull-right arrow is a 9x7
  bitmap, 18px left of the right border. A submenu's top border lines
  up with its parent row and overlaps the parent's right edge by 22px.
- Menu interaction: press-drag-release picks (walking menus); a click
  without moving leaves the menu up so a pen can tap items. Motion
  without a held button is ignored (pens have no hover).
- Right-press on empty desktop opens the root menu; on a window's title
  bar or border opens the frame menu (Move, Resize, Front, Back,
  Redisplay, Quit). Move/Resize act on the next press, so neither
  needs the middle button.

## Font licensing

`k14`, `jiskan16` and `8x16rk` (JIS) are loaded only through their
`fonts/unicode/` conversions. Per-font licenses: `fonts/README.md`.

Inkscreen is the UI and terminal font, by Caffeine Type Foundry: a
GNU Unifont derivative (OFL 1.1, ship `fonts/OFL.txt` with it) redrawn
to a 14-row design after SunOS screen.r.14 (ink rows 10..-3, accents
on capitals up to 11) in an 8x16 cell (ascent 12, descent 4). Covers
ASCII and Latin-1. Its design notes live in caffeine-type's CLAUDE.md.

Plan 9 / plan9port bitmap fonts are not all MIT: `luc`, `lucm`,
`lucsans`, `pelm` are B&H (no derivatives, no separate
redistribution); only `misc`/`fixed` (X11 MIT / public domain) are
free, and those are available from Xorg directly.

Spacing rule (measured: screen.r.14 never touches, screen.b.14 almost
never): column x 0 is the gap column and stays empty in every glyph,
so neighbours never touch. Regular is 8 wide; bold is 9 wide like
screen.b.14, so the extra stroke pixel never eats the gap. A VT-style terminal keeps its 8-px cell
grid and draws bold at an 8-px advance: the 9th column lands in the
next cell's empty gap column, so bold glyphs may touch but never
overlap (as xterm does).

Fallback fonts are aligned to the primary's line-box top, not its
baseline (`text.c`), since Inkscreen is 12/4 and the CJK fonts are 14/2.
Terminal chain until Phase 1 lands: inkscreen14 -> fangsongti16
(GB2312) -> jiskan16 (+8x16rk half-width kana); CJK is 16x16.

Vector text (not implemented yet, FreeType): FreeSans first, then Inksans CJK JP W3, then Unifont, emoji from Noto Emoji. Inksans CJK
(caffeine-type, Noto Sans CJK at fixed weights W1-W8 like Hiragino
Sans; W3 for text, W6 for bold). FreeSans is GPL with the font exception: ship it as
its own file next to Inksans, never merged into one font.
