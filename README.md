<p align="center">
  <img width="96" src="data/com.github.thenathattify.marker.svg" alt="Marker logo" />
</p>

<h1 align="center">Marker</h1>

<p align="center">
  <a href="https://github.com/TheNathattify/Marker/actions/workflows/ci.yml"><img src="https://github.com/TheNathattify/Marker/actions/workflows/ci.yml/badge.svg" alt="CI status" /></a>
  <a href="https://github.com/TheNathattify/Marker/blob/main/LICENSE.md"><img src="https://img.shields.io/github/license/TheNathattify/Marker" alt="License: GPL-3.0-or-later" /></a>
  <a href="https://github.com/TheNathattify/Marker/releases"><img src="https://img.shields.io/github/v/release/TheNathattify/Marker" alt="Latest release" /></a>
</p>

<p align="center"><i>A modern, fast, and feature-rich Markdown editor for the GNOME desktop, built with GTK 4, Libadwaita and WebKitGTK 6.0.</i></p>

Marker combines a distraction-free writing environment with a powerful live preview engine, scientific typesetting, diagramming tools, and rich document export capabilities. Open [`example.md`](example.md) to see most features in action.

---

## Why Marker?

| | **Marker** | Typora | Obsidian | VS Code | Ghostwriter |
|---|---|---|---|---|---|
| **Price** | Free (GPL-3.0) | Paid | Free (paid for sync/publish) | Free | Free |
| **Native GNOME** | ✅ GTK4 + Libadwaita | ❌ Qt | ❌ Electron | ❌ Electron | ✅ Qt |
| **Lightweight** | ✅ C, native | ⚠️ Qt | ❌ Electron (~200 MB) | ❌ Electron | ✅ Qt |
| **Live preview** | ✅ Real-time, synced | ✅ WYSIWYG | ✅ | ⚠️ Basic | ✅ |
| **Math (KaTeX)** | ✅ Built-in, offline | ✅ | ⚠️ Plugin | ⚠️ Extension | ❌ |
| **Diagrams (Mermaid)** | ✅ Built-in | ❌ | ⚠️ Plugin | ⚠️ Extension | ❌ |
| **Syntax highlighting** | ✅ 84 themes | ✅ | ✅ | ✅ | ⚠️ Limited |
| **Export** | ✅ HTML, PDF, ODT, DOCX, LaTeX, RTF | ✅ HTML, PDF, DOCX… | ⚠️ Paid | ⚠️ Extension | ⚠️ ODT, HTML |
| **Task lists** | ✅ Interactive checkboxes | ✅ | ✅ | ⚠️ Basic | ✅ |
| **Multi-document sidebar** | ✅ | ❌ | ✅ | ⚠️ Tabs | ❌ |
| **Cloud / account required** | ❌ No | ❌ No | ⚠️ Optional | ❌ No | ❌ No |

**Marker is the only fully-featured Markdown editor that is native to GNOME, free, open source, and requires no account or cloud service.**

---

## Key features

- **Real-time live preview** — fast, sandboxed WebKitGTK 6.0 preview that stays synchronized with the editor, including scroll position.
- **Document outline** — instant ATX/Setext heading navigation in the sidebar; click a heading to jump straight to its section in the preview.
- **Interactive task lists** — GFM task lists (`- [ ]` / `- [x]`) render as real checkboxes in the preview; click one to toggle it in the source, with undo support.
- **Mathematical typesetting** — complex formulas rendered in real time with KaTeX (bundled, works offline).
- **Diagrams and plotting** — Mermaid diagrams (flowcharts, sequence, state, …) and Charter plots, rendered live.
- **Syntax highlighting** — code blocks highlighted in the preview (highlight.js, 84 themes) and in the editor (GtkSourceView 5).
- **Find and replace** — interactive search/replace with match highlighting and navigation.
- **Multi-format export** — export to HTML, PDF, LaTeX, RTF, ODT and DOCX (via Pandoc and Scidown), or print to PDF.
- **Integrated sketcher** — draw hand-drawn sketches and embed them directly into your notes.
- **Draft recovery** — automatic background draft caching, so your work is never lost.
- **Four view modes** — editor only, preview only, dual-pane and dual-window.
- **Customizable styling** — 21 preview themes and 84 code-highlighting themes, plus support for your own CSS.
- **Modern GNOME interface** — clean Libadwaita design, responsive adaptive layout, automatic dark/light switching.
- **Fully translated** — available in 14 languages.

---

## Installation

### Flatpak (recommended, any distribution)

```bash
flatpak remote-add --if-not-exists --from marker \
  https://thenathattify.github.io/Marker/marker.flatpakrepo
flatpak install marker com.github.thenathattify.marker
```

### Fedora / RHEL / CentOS (COPR)

```bash
sudo dnf copr enable thenathattify/marker
sudo dnf install marker
```

Supported: Fedora 43, 44, 45 (x86_64).

### Ubuntu / Debian (PPA)

```bash
sudo add-apt-repository ppa:thenathattify/marker
sudo apt update && sudo apt install marker
```

Supported: Ubuntu 26.04 (resolute). Debian users can also build from source (see below).

### Arch Linux (AUR)

> ⚠️ AUR account creation is temporarily closed due to bot waves. Until then,
> build directly from the PKGBUILD in this repository:

```bash
git clone https://github.com/TheNathattify/Marker.git
cd Marker/packaging/aur
makepkg -si
```

Or with an AUR helper once the package is published:

```bash
yay -S marker        # or: paru -S marker
```

### AppImage

Not yet available. Use Flatpak or build from source.

---

## Build from source

### Dependencies by distribution

**Fedora / RHEL / CentOS:**

```bash
sudo dnf install meson ninja-build gcc \
  gtk4-devel libadwaita-devel gtksourceview5-devel webkitgtk6.0-devel \
  glib2-devel gettext itstool appstream desktop-file-utils pandoc
```

**Ubuntu / Debian:**

```bash
sudo apt install meson ninja-build gcc \
  libgtk-4-dev libadwaita-1-dev libgtksourceview-5-dev libwebkitgtk-6.0-dev \
  libglib2.0-dev gettext itstool appstream desktop-file-utils pandoc
```

**Arch Linux:**

```bash
sudo pacman -S meson ninja gcc \
  gtk4 libadwaita gtksourceview5 webkitgtk-6.0 \
  glib2 gettext itstool appstream desktop-file-utils pandoc
```

**openSUSE:**

```bash
sudo zypper install meson ninja gcc \
  gtk4-devel libadwaita-devel gtksourceview5-devel webkitgtk6.0-devel \
  glib2-devel gettext itstool appstream desktop-file-utils pandoc
```

### Minimum version requirements

| Dependency | Minimum version |
|---|---|
| meson | >= 0.62.0 |
| ninja | any |
| gcc / clang | any recent |
| gtk4 | >= 4.16.0 |
| libadwaita | >= 1.8.0 |
| gtksourceview5 | >= 5.6.0 |
| webkitgtk-6.0 | >= 2.40.0 |
| glib-2.0 | >= 2.74.0 |
| gettext | any |
| itstool | any |
| appstream | any |
| desktop-file-utils | any |
| pandoc | optional (for document export) |

### Compile and install (one command, any distribution)

After installing the dependencies for your distribution (see above):

```bash
git clone --recursive https://github.com/TheNathattify/Marker.git && \
cd Marker && \
meson setup build --prefix=/usr && \
ninja -C build && \
sudo ninja -C build install
```

The `--recursive` flag is required: the Markdown renderer (scidown) and its
dependencies (charter, tinyexpr) are git submodules.

### Run without installing

```bash
git clone --recursive https://github.com/TheNathattify/Marker.git && \
cd Marker && \
meson setup build && \
ninja -C build && \
./build/src/marker
```

### Uninstall

```bash
sudo ninja -C build uninstall
```

---

## What's new in this fork

This is a personal hard fork of [adrighem/Marker](https://github.com/adrighem/Marker) maintained by **Nathan Obry** ([@thenathattify](https://github.com/thenathattify)), rebranded under the app-ID `com.github.thenathattify.marker`. Versions follow a date-based scheme (`2026.10.x`).

**Interface and workflow**

- **Modern GNOME design** — `AdwToolbarView`, view switcher, split buttons, shortcuts dialog, and native dialogs throughout.
- **Smart sidebar** — hidden by default, appears automatically when two or more documents are open, with a toggle button in the header bar, per-document rename (pencil) and close buttons, and clean truncation of long file names.
- **Split save button** — the floppy icon saves; its dropdown offers Save, Save As and Export.
- **Single fullscreen button** — one toggle in the header bar.
- **Editor font size preference** — independent from the preview zoom.
- **Document renaming** from the sidebar (pencil button or <kbd>F2</kbd>).

**Zoom, reworked**

- **Per-pane Ctrl+scroll zoom** — the editor and the preview zoom independently.
- **Editable zoom field** in the header bar, applied instantly.
- **Fit-to-width button** — a short click fits the preview to the pane once; a long click toggles automatic re-fitting when the window is resized.
- **Auto-fit on load** — with auto-fit enabled, the preview is fitted as the document loads.
- **Manual zoom disables auto-fit**, and the scroll position is preserved while zooming.

**Preview rendering**

- **Bundled, offline-capable libraries** — KaTeX, highlight.js and Mermaid ship with the app (no internet required).
- **Correct heading levels** — `#` is a real `<h1>`.
- **GitHub-style code blocks** — a code fence directly after a text line renders as code.
- **Robust Mermaid diagrams** — diagrams containing `<`, `&` or `>` render correctly, and one invalid diagram no longer breaks the following ones.
- **Outline navigation** — clicking a heading in the sidebar scrolls the preview to the exact section.

**Themes**

- **Fedora Dark / Fedora Light** preview themes built on official GNOME/libadwaita color tokens.
- **Adwaita / Adwaita-dark** code-highlighting themes matching the editor's own color schemes.

**Integration and packaging**

- **Single-instance behavior** — D-Bus activation for faster startup.
- **Automatic settings migration** from the previous `com.github.adrighem.marker` schema when upgrading.
- **Official packages** — COPR (Fedora), PPA (Ubuntu), AUR (Arch), signed Flatpak repo.

---

## Themes and customization

- **Preview themes** — 21 bundled CSS themes (GitHub, Nord, Solarized, Fedora Dark/Light, …), selectable in Preferences.
- **Code highlighting themes** — 84 bundled themes, including Adwaita and Adwaita-dark which match the editor.
- **Your own themes** — drop `.css` files into `~/.local/share/marker/styles/` and they appear in Preferences.
- **Editor** — font size, color scheme and zoom are configurable independently of the preview.

---

## Credits and attribution

Marker is a continuation of the original [Marker](https://github.com/fabiocolacio/Marker) by **Fabio Colacio**, modernized to GTK 4 / Libadwaita / WebKitGTK 6.0 by **Vincent van Adrighem** ([@adrighem](https://github.com/adrighem)), and maintained as this hard fork by **Nathan Obry** ([@thenathattify](https://github.com/thenathattify)).

Built on [scidown](https://github.com/Mandarancio/scidown) and [Charter](https://github.com/Mandarancio/charter) by Mandarancio, and on the excellent GTK, Libadwaita, GtkSourceView, WebKitGTK, KaTeX, highlight.js and Mermaid projects.

---

## License

Marker is licensed under the [GNU General Public License v3.0 or later](LICENSE.md).
