# InkView plan

InkView is a window system and desktop for e-ink screens. The first
target is the Kindle Scribe (gen 1). It also runs on macOS and Linux
through the SDL3 backend.

## Decisions

- Architecture: like Windows 3.1. InkView is one process that owns the
  screen. Tools are modules in that process. A clear tool interface
  keeps the option to add a protocol for external tools later.
- Work order: tune the UI and UX on macOS first. Port to the Kindle
  after that. The SDL backend does not simulate e-ink.
- Scale: specs are in 1x logical pixels. The Kindle runs at 2x in
  landscape (2480x1860 physical, 1240x930 logical).
- Input: the pen is the mouse. Tip is left, tip with the side button
  is right, eraser is middle. Every middle-button action also works
  with left or right.
- Keyboard and mouse for the Kindle come from an Android app over
  Wi-Fi. InkView does not use Bluetooth HID.
- License: BSD-2-Clause. Fonts keep their own licenses.

## Done

- Core: gray surface with integer scale, tile patterns, BDF loader,
  UTF-8 text with fallback font chains.
- Menus: pop-up and pull-right menus. Press, drag and release picks an
  item. A click without a drag keeps the menu open for the pen.
- Window manager: frames, stacking, selection, move, resize, root menu
  and frame menu.
- SDL3 backend and a snapshot mode that writes one frame to a BMP.
- Fonts: Inkscreen (bitmap, ASCII and Latin-1) and Inksans CJK
  (vector, W1 to W8) in the caffeine-type repo.
- Font set cleanup: retired fonts removed. `tools/fetch-fonts.sh` gets
  the large fonts at pinned versions.

## Phase 1: fonts and text

Goal: one vector text engine, the new font set, and Inksans CJK as the
CJK font in the UI and the terminal.

Font set:

| Font | Format | Use |
|---|---|---|
| Inkscreen, Inkscreen Bold | BDF | UI, menus, terminal |
| Inksans CJK | TTC | CJK text, default face JP, W3 for text, W6 for bold |
| FreeSans, FreeSerif, FreeMono | OTF | vector text, FreeSans is the default |
| Fangsong ti, Jiskan, k14 | TTF from BDF (bdf2ttf) | optional bitmap CJK |
| Courier 10 Pitch | OTF from Type 1 | optional typewriter font |
| Noto Emoji | TTF | emoji, monochrome |
| Unifont | OTF | last fallback for any character |

Tasks:

1. Add FreeType as the vector font engine (FTL license, credit it in
   the docs).
2. Load a face from a font collection by name, for example
   "Inksans CJK JP W3".
3. Mix bitmap and vector fonts in one fallback chain.
4. Set the UI chain: Inkscreen, then Inksans CJK JP W3, then Unifont.
   Menus use Inkscreen Bold, then Inksans CJK JP W6.
5. Draw CJK at 16 logical pixels (32 physical pixels on the Kindle).
   Keep the terminal cell grid: CJK takes two 8x16 cells.
6. Set the vector text chain: FreeSans, then Inksans CJK JP W3, then
   Unifont. Noto Emoji draws emoji.
7. Compare gray and 1-bit rendering for CJK at 32 pixels. Gray needs
   the slow e-ink waveform.
8. Take the BDF CJK fonts out of the UI and terminal chains. They
   stay as optional fonts.
9. Convert Fangsong ti, Jiskan and k14 with bdf2ttf. Convert Courier
   10 Pitch from Type 1 to OTF.
10. Ship each license next to its font. FreeSans, FreeSerif and
    FreeMono stay separate files (GPL with the font exception).

## Phase 2: toolkit and window system

Goal: everything a tool needs, tuned on macOS.

1. Tool interface: open, draw, handle an event, close. One tool can
   own more than one window.
2. Event loop with timers. The clock needs a timer.
3. Damage tracking: redraw only the changed rectangles. The Kindle
   needs this to refresh small areas.
4. Keyboard: key events in the SDL backend, keyboard focus, text input.
5. Panes: a window holds one or more panes. A pane has a 1-pixel ring
   that is white, and black when the pane is selected.
6. Close and iconify. Icons sit on the desktop.
7. Controls: button, checkbox, choice, slider, text field, scroll bar,
   list.
8. Dialogs: alert, confirm, and a file chooser. A dialog has a solid
   4-pixel border and the popup shadow.
9. Clipboard inside InkView.
10. Taskbar style: a bar at the top or bottom, a start menu, three
    title-bar buttons, iconify to the bar. A setting switches between
    the workstation style and the taskbar style.
11. Tiling as an option next to stacking.
12. Config files in `.config` under the user data folder.
13. Services menu: Redisplay All, Lock Screen, Save Layout.

User data folder on the Kindle: `/mnt/us/inkview/` with `desktop`,
`document`, `developer`, `download` and `.config`.

## Phase 3: first tools

1. terminal: a pseudo-terminal and a VT parser. Bold uses an 8-pixel
   advance in the cell grid.
2. textedit: plain text.
3. clock.
4. calc.
5. sudoku and life (Games menu). Make them together.
6. fileview: a file manager.
7. osview: CPU, memory, storage, battery, Wi-Fi.

## Phase 4: Kindle backend

1. Display: write the framebuffer and send refresh requests to the
   `hwtcon` driver. Write this code ourselves. FBInk is GPL-3, so do
   not link it.
2. Rotation: rotate frames on present and rotate input back.
3. Refresh modes: fast modes for 1-bit areas, the gray mode where gray
   is necessary, a full refresh now and then to clear ghosting.
4. Pen input: tip, side button, eraser, pressure.
5. Touch input. Find out how many touch points the panel reports.
6. Screenshot gesture: two fingers on opposite corners, as in the
   Kindle UI. This needs multi-touch.
7. Start and stop: pause the Kindle framework while InkView runs, and
   start it again on exit. Add a launcher entry.
8. Power: sleep, wake, and the screen saver.
9. Check which libraries the firmware already has (for example the
   font engine).

## Phase 5: Android input bridge

1. A small protocol over Wi-Fi for key, pointer and scroll events.
2. Pairing and a check that only the paired phone can send input.
3. The Android app: it reads a USB or Bluetooth keyboard and mouse and
   sends their events to InkView.

## Phase 6: more tools

- IME with RIME.
- writepad: rich text.
- bookview: EPUB, and MOBI if possible.
- webview: simple HTML and CSS.
- imgview.
- localsend: file transfer with the LocalSend protocol.
- audioplay: Bluetooth audio through the Kindle stack.
- chess (later than sudoku and life).
- Inter as an extra sans for documents.
