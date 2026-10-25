project(
  'marker',
  'c',
  version: '2026.10.6',
  meson_version: '>= 0.62.0',
  default_options: [
    'warning_level=2',
    'c_std=gnu11',
  ],
  license: 'GPL-3.0-or-later'
)

prefix = get_option('prefix')
datadir = prefix / get_option('datadir')
libdir = prefix / get_option('libdir')
localedir = prefix / get_option('localedir')
appdir = datadir / 'com.github.thenathattify.marker'
icons_dir = appdir / 'icons'
styles_dir = appdir / 'styles'
common_dir = appdir / 'common'
scripts_dir = appdir / 'scripts'
highlight_styles_dir = scripts_dir / 'highlight' / 'styles'
appdata_dir = datadir / 'metainfo'

executable_name = 'marker'

gnome = import('gnome')
i18n = import('i18n')
subdir('help')
subdir('po')

res = gnome.compile_resources(
  'marker_resources',
  'src/resources/marker.gresource.xml',
  source_dir : 'src/resources'
)

marker_sources = [
  'src/marker.c',
  'src/marker-editor.c',
  'src/marker-document.c',
  'src/marker-search-controller.c',
  'src/marker-window.c',
  'src/marker-exporter.c',
  'src/marker-markdown.c',
  'src/marker-prefs.c',
  'src/marker-source-view.c',
  'src/marker-preview.c',
  'src/marker-string.c',
  'src/marker-widget.c',
  'src/marker-sketcher-window.c',
  'src/marker-utils.c',
  'src/marker-outline.c',
  res
]

hoedown_sources = [
  'src/scidown/src/autolink.c',
  'src/scidown/src/buffer.c',
  'src/scidown/src/document.c',
  'src/scidown/src/escape.c',
  'src/scidown/src/html.c',
  'src/scidown/src/utils.c',
  'src/scidown/src/latex.c',
  'src/scidown/src/html_blocks.c',
  'src/scidown/src/html_smartypants.c',
  'src/scidown/src/stack.c',
  'src/scidown/src/version.c',
  'src/scidown/src/constants.c',
  'src/scidown/src/charter/src/charter_string.c',
  'src/scidown/src/charter/src/csv_parser/csvparser.c',
  'src/scidown/src/charter/src/tinyexpr/tinyexpr.c',
  'src/scidown/src/charter/src/clist.c',
  'src/scidown/src/charter/src/parser.c',
  'src/scidown/src/charter/src/charter.c',
  'src/scidown/src/charter/src/svg.c',
  'src/scidown/src/charter/src/latex.c',
  'src/scidown/src/charter/src/svg_utils.c'
]

deps = [
  dependency('gtk4', version: '>= 4.16.0'),
  dependency('libadwaita-1', version: '>= 1.8.0'),
  dependency('glib-2.0', version: '>= 2.74.0'),
  dependency('gio-2.0', version: '>= 2.74.0'),
  dependency('gtksourceview-5', version: '>= 5.6.0'),
  dependency('webkitgtk-6.0', version: '>= 2.40.0')
]

add_project_arguments(
  '-DSTYLES_DIR="@0@/"'.format(styles_dir),
  '-DSCRIPTS_DIR="@0@/"'.format(scripts_dir),
  '-DICONS_DIR="@0@/"'.format(icons_dir),
  '-DHIGHLIGHT_STYLES_DIR="@0@/"'.format(highlight_styles_dir),
  '-DMARKER_VERSION="@0@"'.format(meson.project_version()),
  '-DLOCALE_DIR="@0@"'.format(localedir),
  '-DCOMMON_DIR="@0@/"'.format(common_dir),
  language: 'c'
)

executable(
  executable_name,
  sources: [marker_sources, hoedown_sources],
  dependencies: deps,
  link_args: '-lm',
  install: true
)

install_data(
  'data/com.github.thenathattify.marker.svg',
  install_dir: datadir / 'icons' / 'hicolor' / 'scalable' / 'apps'
)

install_data(
  'data/com.github.thenathattify.marker-symbolic.svg',
  install_dir: datadir / 'icons' / 'hicolor' / 'symbolic' / 'apps'
)

install_data(
  'data/com.github.thenathattify.marker.desktop',
  install_dir: datadir / 'applications'
)

install_data(
  'data/com.github.thenathattify.marker.gschema.xml',
  install_dir: datadir / 'glib-2.0' / 'schemas'
)

install_data(
  'data/com.github.thenathattify.marker.appdata.xml',
  install_dir: appdata_dir
)

install_subdir(
  'data/styles',
  install_dir: appdir
)

install_subdir(
  'data/common',
  install_dir: appdir
)

install_subdir(
  'data/scripts',
  install_dir: appdir
)

install_subdir(
  'data/icons',
  install_dir: appdir
)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true
)
