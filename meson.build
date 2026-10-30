project('inkview', 'c',
  version: '0.0.1',
  license: 'BSD-2-Clause',
  default_options: ['c_std=c11', 'warning_level=3'])

add_project_arguments(
  '-D_POSIX_C_SOURCE=200809L',
  '-DINKVIEW_FONTDIR="' + (meson.project_source_root() / 'fonts') + '"',
  language: 'c')

# platform-independent core: rendering, fonts, window manager
core = static_library('inkcore',
  'src/gfx.c',
  'src/bdf.c',
  'src/text.c',
  'src/menu.c',
  'src/wm.c')

sdl3 = dependency('sdl3')

executable('inkview',
  'src/main.c',
  'src/backend_sdl.c',
  link_with: core,
  dependencies: sdl3)

test_core = executable('test_core', 'tests/test_core.c', link_with: core)
test('core', test_core,
  args: [meson.project_source_root() / 'fonts'])
