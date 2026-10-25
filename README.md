<p align="center">
  <img width="96" src="data/com.github.thenathattify.marker.svg" alt="Marker logo" />
</p>

<h1 align="center">Marker</h1>

<p align="center">
  <a href="https://github.com/TheNathattify/Marker/actions/workflows/ci.yml"><img src="https://github.com/TheNathattify/Marker/actions/workflows/ci.yml/badge.svg" alt="CI status" /></a>
  <a href="https://github.com/TheNathattify/Marker/blob/main/LICENSE.md"><img src="https://img.shields.io/github/license/TheNathattify/Marker" alt="License: GPL-3.0-or-later" /></a>
</p>

<p align="center"><i>A modern, fast, and feature-rich Markdown editor for the GNOME desktop, built with GTK 4, Libadwaita and WebKitGTK 6.0.</i></p>

Marker combines a distraction-free writing environment with a powerful live preview engine, scientific typesetting, diagramming tools, and rich document export capabilities. Open [`example.md`](example.md) to see most features in action.

---

## Key features

- **Real-time live preview** — fast, sandboxed WebKitGTK 6.0 preview that stays synchronized with the editor, including scroll position.
- **Document outline** — instant ATX/Setext heading navigation in the sidebar; click a heading to jump straight to its section in the preview.
- **Interactive task lists** — GFM task lists (`- [ ]` / `- [x]`) render as real checkboxes in the preview; click one to toggle it in the source, with undo support.
- **Mathematical typesetting** — complex formulas rendered in real time with KaTeX (bundled, works offline) or MathJax.
- **Diagrams and plotting** — Mermaid diagrams (flowcharts, sequence, state, …) and Charter plots, rendered live.
- **Syntax highlighting** — code blocks highlighted in the preview (highlight.js) and in the editor (GtkSourceView 5).
- **Find and replace** — interactive search/replace with match highlighting and navigation.
- **Multi-format export** — export to HTML, PDF, LaTeX, RTF, ODT and DOCX (via Pandoc and Scidown), or print to PDF.
- **Integrated sketcher** — draw hand-drawn sketches and embed them directly into your notes.
- **Draft recovery** — automatic background draft caching, so your work is never lost.
- **Four view modes** — editor only, preview only, dual-pane and dual-window.
- **Customizable styling** — 21 preview themes and 84 code-highlighting themes, plus support for your own CSS.
- **Modern GNOME interface** — clean Libadwaita design, responsive adaptive layout, automatic dark/light switching.
- **Fully translated** — available in 14 languages.

## What's new in this fork

This is a personal hard fork of [adrighem/Marker](https://github.com/adrighem/Marker) maintained by **Nathan Obry** ([@thenathattify](https://github.com/thenathattify)), rebranded under the app-ID `com.github.thenathattify.marker`. Versions follow a date-based scheme (`2026.10.x`).

Since the GTK 4 port, the fork adds:

**Interface and workflow**

- **GNOME HIG compliance pass** — `AdwToolbarView`, view switcher, split buttons, shortcuts dialog, and modern dialogs throughout.
- **Reworked sidebar** — hidden by default, appears automatically when two or more documents are open, with a toggle button in the header bar, per-document rename (pencil) and close buttons, and clean truncation of long file names.
- **Split save button** — the floppy icon saves; its dropdown offers Save, Save As and Export.
- **Single fullscreen button** — one toggle in the header bar instead of duplicated menu entries.
- **Editor font size preference** — independent from the preview zoom.
- **Document renaming** from the sidebar (pencil button or <kbd>F2</kbd>).

**Zoom, reworked**

- **Per-pane Ctrl+scroll zoom** — the editor and the preview zoom independently.
- **Editable zoom field** in the header bar, applied instantly (no more multi-second lag).
- **Fit-to-width button** — a short click fits the preview to the pane once; a long click toggles automatic re-fitting when the window is resized.
- **Auto-fit on load** — with auto-fit enabled, the preview is fitted as the document loads, without a second render.
- **Manual zoom disables auto-fit**, and the scroll position is preserved while zooming.

**Preview rendering**

- **Bundled, offline-capable libraries** — KaTeX 0.19.1, highlight.js 11.12.0 and Mermaid 12.2.0 ship with the app.
- **Fixed heading levels** — `#` is a real `<h1>` again, and stray `-`/`=` lines no longer produce phantom headings.
- **Fixed fenced code blocks** — a code fence directly after a text line renders as code (GitHub-style), instead of being swallowed into the paragraph.
- **Robust Mermaid** — diagrams containing `<`, `&` or `>` are no longer mangled, and one invalid diagram no longer breaks the following ones.
- **Outline navigation** — clicking a heading in the sidebar scrolls the preview to the exact section.

**Themes**

- **Fedora Dark / Fedora Light** preview themes built on official GNOME/libadwaita color tokens.
- **Adwaita / Adwaita-dark** code-highlighting themes matching the editor's own color schemes.

**Stability**

- All six GTK 4 port regressions fixed (view-mode crashes, inert menus, greyed-out actions, blank preview, fullscreen state, cursor position).
- Memory leaks, invalid frees and use-after-free bugs fixed; sidebar/menu state desyncs fixed; WebKitGTK freeze on Wayland + NVIDIA worked around.

**Integration and packaging**

- **D-Bus activation** — a proper service file is shipped, giving single-instance behavior and faster startup.
- **Automatic settings migration** from the previous `com.github.adrighem.marker` schema when upgrading (see [`marker-rebrand.sh`](marker-rebrand.sh)).
- **Green CI on every push** — AppStream/desktop/schema validation plus a full Flatpak build.
- **Flatpak manifest** targeting GNOME Platform/SDK 50.

## Keyboard shortcuts

| Shortcut | Action |
|---|---|
| <kbd>Ctrl</kbd>+<kbd>N</kbd> | New document |
| <kbd>Ctrl</kbd>+<kbd>O</kbd> | Open a document |
| <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>O</kbd> | Open in a new window |
| <kbd>Ctrl</kbd>+<kbd>S</kbd> | Save |
| <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>S</kbd> | Save as |
| <kbd>Ctrl</kbd>+<kbd>E</kbd> | Export |
| <kbd>F2</kbd> | Rename document |
| <kbd>F5</kbd> / <kbd>Ctrl</kbd>+<kbd>R</kbd> | Reload from disk |
| <kbd>Ctrl</kbd>+<kbd>W</kbd> | Close document |
| <kbd>Ctrl</kbd>+<kbd>B</kbd> / <kbd>I</kbd> / <kbd>M</kbd> | Bold / italic / monospace selection |
| <kbd>Ctrl</kbd>+<kbd>K</kbd> | Insert link |
| <kbd>Ctrl</kbd>+<kbd>D</kbd> | Draw a sketch |
| <kbd>Ctrl</kbd>+<kbd>F</kbd> / <kbd>H</kbd> | Find / find and replace |
| <kbd>Ctrl</kbd>+<kbd>+</kbd> / <kbd>-</kbd> | Zoom in / out (also <kbd>Ctrl</kbd>+scroll) |
| <kbd>Ctrl</kbd>+<kbd>0</kbd> | Reset zoom |
| <kbd>Ctrl</kbd>+<kbd>1</kbd>–<kbd>4</kbd> | Editor / preview / dual-pane / dual-window mode |
| <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>N</kbd> | New window |
| <kbd>F12</kbd> | Toggle sidebar |
| <kbd>F11</kbd> | Toggle fullscreen |
| <kbd>Ctrl</kbd>+<kbd>P</kbd> | Print |
| <kbd>Ctrl</kbd>+<kbd>Q</kbd> | Quit |

## Themes and customization

- **Preview themes** — 21 bundled CSS themes (GitHub, Nord, Solarized, Fedora Dark/Light, …), selectable in Preferences.
- **Code highlighting themes** — 84 bundled themes, including Adwaita and Adwaita-dark which match the editor.
- **Your own themes** — drop `.css` files into `~/.local/share/marker/styles/` and they appear in Preferences.
- **Editor** — font size, color scheme and zoom are configurable independently of the preview.

## Installation

### Dependencies

- `meson` (>= 0.62.0) and `ninja`
- `gtk4` (>= 4.16.0)
- `libadwaita-1` (>= 1.8.0)
- `gtksourceview-5` (>= 5.6.0)
- `webkitgtk-6.0` (>= 2.40.0)
- `glib-2.0` / `gio-2.0` (>= 2.74.0)
- `gettext`
- `pandoc` (optional, for document export)

### Build from source

```bash
git clone --recursive https://github.com/TheNathattify/Marker.git
cd Marker
meson setup build
ninja -C build
sudo ninja -C build install
```

The `--recursive` flag is required: the Markdown renderer (scidown) is a git submodule.

### Flatpak

```bash
flatpak install flathub org.gnome.Platform//50 org.gnome.Sdk//50
flatpak-builder --user --install --force-clean build-flatpak com.github.thenathattify.marker.json
```

To run the sandboxed build without installing it:

```bash
flatpak-builder --run build-flatpak com.github.thenathattify.marker.json marker
```

---

## Credits and attribution

Marker is a continuation of the original [Marker](https://github.com/fabiocolacio/Marker) by **Fabio Colacio**, modernized to GTK 4 / Libadwaita / WebKitGTK 6.0 by **Vincent van Adrighem** ([@adrighem](https://github.com/adrighem)), and maintained as this hard fork by **Nathan Obry** ([@thenathattify](https://github.com/thenathattify)).

Built on [scidown](https://github.com/Mandarancio/scidown) and [Charter](https://github.com/Mandarancio/charter) by Mandarancio, and on the excellent GTK, Libadwaita, GtkSourceView, WebKitGTK, KaTeX, highlight.js and Mermaid projects.

## License

Marker is licensed under the [GNU General Public License v3.0 or later](LICENSE.md).
