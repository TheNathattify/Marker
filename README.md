# <img width="32" src="data/com.github.adrighem.marker.svg" alt="Marker Logo"/> Marker

A modern, fast, and feature-rich Markdown editor designed for GNOME, built with GTK 4 and Libadwaita.

Marker combines a distraction-free writing environment with a powerful live preview engine, scientific typesetting, diagramming tools, and rich document export capabilities.

---

## Key Features

- **Modern GNOME Interface**: Clean Libadwaita styling, responsive adaptive layouts, and automatic dark/light theme switching.
- **Real-Time Live Preview**: Fast, sandboxed WebKitGTK 6.0 preview synchronized with your document.
- **Document Outline**: Instant ATX and Setext heading navigation in the sidebar for effortless document traversal.
- **Find and Replace**: Interactive search and replace controller with match highlighting and navigation.
- **Draft Recovery & Safety**: Automatic background draft caching so your work is never lost.
- **Trailing Whitespace Trimming**: Configurable option to clean trailing whitespace on save.
- **Mathematical Typesetting**: Render complex formulas in real time with KaTeX or MathJax backends.
- **Diagrams and Plotting**: Native support for Mermaid diagrams (flowcharts, sequence charts, state diagrams) and Charter plots.
- **Syntax Highlighting**: Beautiful code block syntax highlighting powered by highlight.js and GtkSourceView 5.
- **Integrated Sketcher**: Draw and embed hand-drawn sketches directly into your notes.
- **Multi-Format Export**: Export your documents to HTML, PDF, LaTeX, RTF, ODT, and DOCX (powered by Pandoc and Scidown).
- **Customizable Styling**: User-selectable editor themes and custom preview CSS stylesheets.

---

## Screenshots

![Main Window](help/C/figures/scrot1.png)

![Plotting](help/C/figures/scrot2.png)

![Sketch Editor](help/C/figures/sketcher.png)

---

## Installation and Building

### Building with Meson & Ninja

#### Dependencies
- `meson` (>= 0.62.0)
- `ninja`
- `gtk4` (>= 4.10.0)
- `libadwaita-1` (>= 1.4.0)
- `gtksourceview-5` (>= 5.6.0)
- `webkitgtk-6.0` (>= 2.40.0)
- `glib-2.0` / `gio-2.0` (>= 2.74.0)
- `pandoc` (optional, for document export)

#### Build Commands
```bash
git clone https://github.com/adrighem/Marker.git
cd Marker
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
```

### Building Flatpak

You can build and test Marker in a clean Flatpak sandbox using Flatpak Builder:

```bash
flatpak-builder --user --install --force-clean build-flatpak com.github.adrighem.marker.json
```

Or run directly:
```bash
flatpak-builder --run build-flatpak com.github.adrighem.marker.json marker
```

---

## Attribution & Acknowledgments

Marker is a modernized continuation forked from the original [Marker by Fabio Colacio](https://github.com/fabiocolacio/Marker).

Special thanks to Fabio Colacio, Martino Ferrari, and all original contributors for creating the foundation of this project.

---

## License

Marker is licensed under the [GNU General Public License v3.0 or later](LICENSE.md).
