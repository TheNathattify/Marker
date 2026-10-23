/*
 * marker-markdown.c
 *
 * Copyright (C) 2017-2020 - 2018 Fabio Colacio
 *
 * Marker is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * Marker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with Marker; see the file LICENSE.md. If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "scidown/src/html.h"
#include "scidown/src/latex.h"
#include "scidown/src/document.h"
#include "scidown/src/buffer.h"

#include "marker-markdown.h"
#include "marker-prefs.h"
#include "marker-utils.h"




char* html_header(MarkerMathJSMode    mathjs_mode,
                  MarkerHighlightMode highlight_mode,
                  MarkerMermaidMode   mermaid_mode)
{
  char* mathjs_script;
  char* mathjs_auto;
  char* mathjs_css;

  char* highlight_css;
  char* highlight_script;

  char* mermaid_script;

  char * local_highlight_css = marker_prefs_get_highlight_theme();
  MarkerMathBackEnd backend = marker_prefs_get_math_backend();
  const gchar *scripts_dir = marker_utils_get_scripts_dir ();


  switch (mathjs_mode) {
    case MATHJS_OFF:
      mathjs_script = g_strdup(" ");
      mathjs_css = g_strdup(" ");
      mathjs_auto = g_strdup(" ");
      break;
  case MATHJS_NET:
      if (backend == KATEX)
      {
        mathjs_css = g_strdup("<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/KaTeX/0.9.0-alpha2/katex.min.css\" crossorigin=\"anonymous\">");
        mathjs_script = g_strdup("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/KaTeX/0.9.0-alpha2/katex.min.js\" crossorigin=\"anonymous\"></script>");
        mathjs_auto = g_strdup("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/KaTeX/0.9.0-alpha2/contrib/auto-render.min.js\" crossorigin=\"anonymous\"></script>");
      } else
      {
        mathjs_css = g_strdup(" ");
        mathjs_script = g_strdup("<script type=\"text/javascript\" src=\"https://cdn.jsdelivr.net/npm/mathjax@2.7.1/MathJax.js?config=TeX-AMS_HTML\"></script>");
        mathjs_auto = g_strdup(" ");
      }
      break;
  case MATHJS_LOCAL:
      if (backend == KATEX)
      {
        mathjs_css = g_strdup_printf("<link rel=\"stylesheet\" href=\"file://%skatex/katex.min.css\">", scripts_dir);
        mathjs_script = g_strdup_printf("<script src=\"file://%skatex/katex.min.js\"></script>", scripts_dir);
        mathjs_auto = g_strdup_printf("<script src=\"file://%skatex/contrib/auto-render.min.js\"></script>", scripts_dir);
      } else
      {
        mathjs_css = g_strdup(" ");
        mathjs_script = g_strdup_printf("<script src=\"file:///usr/share/javascript/mathjax/MathJax.js?config=TeX-AMS_HTML\"></script>");
        mathjs_auto = g_strdup(" ");
      }
      break;
  }

 switch (highlight_mode){
    case HIGHLIGHT_OFF:
      highlight_css = g_strdup(" ");
      highlight_script = g_strdup(" ");
      break;
    case HIGHLIGHT_NET:
      highlight_css = g_strdup_printf("<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/9.12.0/styles/%s.min.css\">",
                                      local_highlight_css);
      highlight_script = g_strdup("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/9.12.0/highlight.min.js\"></script>");
      break;
    case HIGHLIGHT_LOCAL:
      highlight_css = g_strdup_printf("<link rel=\"stylesheet\" href=\"file://%shighlight/styles/%s.css\">", scripts_dir, local_highlight_css);
      highlight_script = g_strdup_printf("<script src=\"file://%shighlight/highlight.pack.js\"></script>", scripts_dir);
      break;
  }

  switch (mermaid_mode)
  {
    case MERMAID_OFF:
      mermaid_script = g_strdup(" ");
      break;
    case MERMAID_NET:
      mermaid_script = g_strdup("<script src=\"https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js\"></script>");
      break;
    case MERMAID_LOCAL:
      mermaid_script = g_strdup_printf("<script src=\"file://%smermaid/mermaid.min.js\"></script>", scripts_dir);
      break;
  }

  const char *csp_meta =
    "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'self' 'unsafe-inline' file: https://cdnjs.cloudflare.com https://cdn.jsdelivr.net; script-src 'self' 'unsafe-inline' 'unsafe-eval' file: https://cdnjs.cloudflare.com https://cdn.jsdelivr.net; style-src 'self' 'unsafe-inline' file: https://cdnjs.cloudflare.com https://cdn.jsdelivr.net; img-src * file: data: https: http:; font-src file: data: https://cdnjs.cloudflare.com https://cdn.jsdelivr.net; media-src file: data: https: http:; connect-src 'none'; frame-src 'none'; object-src 'none'; form-action 'none';\">";

  char * buffer = g_strdup_printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
                                  csp_meta,
                                  mathjs_css,
                                  highlight_css,
                                  mathjs_script,
                                  mathjs_auto,
                                  highlight_script,
                                  mermaid_script);

  g_free(mathjs_script);
  g_free(mathjs_auto);
  g_free(mathjs_css);

  g_free(highlight_css);
  g_free(highlight_script);
  g_free(local_highlight_css);

  g_free(mermaid_script);

  return buffer;
}


char*
html_footer(MarkerMathJSMode     mathjs_mode,
            MarkerHighlightMode highlight_mode,
            MarkerMermaidMode   mermaid_mode)
{
  char * mathjs_render;
  char * highlight_render;
  char * mermaid_render;
  MarkerMathBackEnd backend = marker_prefs_get_math_backend();

  switch(mathjs_mode){
    case MATHJS_OFF:
      mathjs_render = g_strdup(" ");
      break;
    default:
      if (backend == KATEX)
        mathjs_render = g_strdup("<script>if (typeof renderMathInElement === 'function') { renderMathInElement(document.body); }</script>");
      else
        mathjs_render = g_strdup(" ");
      break;
  }

  switch(highlight_mode)
  {
    case HIGHLIGHT_OFF:
      highlight_render = g_strdup(" ");
      break;
    default:
      highlight_render = g_strdup("<script>if (typeof hljs !== 'undefined') { hljs.initHighlightingOnLoad(); }</script>");
      break;
  }

  switch(mermaid_mode)
  {
    case MERMAID_OFF:
      mermaid_render = g_strdup(" ");
      break;
    default:
      mermaid_render = g_strdup("<script>\n"
                                "function initMermaid() {\n"
                                "  if (typeof mermaid !== 'undefined') {\n"
                                "    try {\n"
                                "      mermaid.initialize({startOnLoad:false, securityLevel:'loose', theme:'default'});\n"
                                "      if (typeof mermaid.run === 'function') {\n"
                                "        mermaid.run({querySelector: '.mermaid'});\n"
                                "      } else if (typeof mermaid.init === 'function') {\n"
                                "        mermaid.init(undefined, document.querySelectorAll('.mermaid'));\n"
                                "      }\n"
                                "    } catch (e) { console.error('Mermaid render error:', e); }\n"
                                "  }\n"
                                "}\n"
                                "if (document.readyState === 'loading') {\n"
                                "  document.addEventListener('DOMContentLoaded', initMermaid);\n"
                                "} else {\n"
                                "  initMermaid();\n"
                                "}\n"
                                "</script>");
      break;
  }

  char* buffer = g_strdup_printf("%s\n%s\n%s\n", mathjs_render, highlight_render, mermaid_render);
  g_free(highlight_render);
  g_free(mathjs_render);
  g_free(mermaid_render);
  return buffer;
}

scidown_render_flags
get_render_mode(MarkerMermaidMode mermaid_mode)
{
  scidown_render_flags mode = 0;

  if (mermaid_mode != MERMAID_OFF)
  {
    mode |= SCIDOWN_RENDER_MERMAID;
  }

  if (marker_prefs_get_use_charter())
  {
    mode |= SCIDOWN_RENDER_CHARTER;
  }

  return mode;
}

localization get_local()
{
  localization local;
  local.figure = "Figure";
  local.listing = "Listing";
  local.table = "Table";
  return local;
}

char*
marker_markdown_css(const char* css_link)
{
  if (!css_link || css_link[0] == '\0')
  {
    return g_strdup("");
  }

  gchar *contents = NULL;
  gsize length = 0;

  if (g_path_is_absolute (css_link) && g_file_test (css_link, G_FILE_TEST_IS_REGULAR))
  {
    if (g_file_get_contents (css_link, &contents, &length, NULL))
      return contents;
  }

  g_autofree gchar *user_path = g_build_filename (marker_utils_get_user_styles_dir (), css_link, NULL);
  if (g_file_test (user_path, G_FILE_TEST_IS_REGULAR))
  {
    if (g_file_get_contents (user_path, &contents, &length, NULL))
      return contents;
  }

  g_autofree gchar *sys_path = g_build_filename (marker_utils_get_styles_dir (), css_link, NULL);
  if (g_file_test (sys_path, G_FILE_TEST_IS_REGULAR))
  {
    if (g_file_get_contents (sys_path, &contents, &length, NULL))
      return contents;
  }

  return g_strdup("");
}

char*
marker_markdown_scidown_css(void)
{
  g_autofree gchar *path = g_strdup_printf("%s%s", marker_utils_get_styles_dir (), "scidown.css");
  gchar *contents = NULL;
  gsize length = 0;
  if (g_file_get_contents(path, &contents, &length, NULL))
  {
    return contents;
  }
  return g_strdup("");
}


/*
 * GFM task lists --------------------------------------------------------------
 *
 * scidown has no task-list support (Mandarancio/scidown#7,
 * fabiocolacio/Marker#218): the inline parser swallows the "[ ]" / "[x]"
 * markers before they ever reach the HTML output, so a footer script can
 * never recover them. Instead, rewrite the markers to private-use Unicode
 * placeholders (U+E000 / U+E001) *before* rendering, then swap those
 * placeholders for real <input type="checkbox"> elements *after* the HTML
 * has been produced.
 *
 * "[ ]" and "[x]" are 3 bytes long, and so are the U+E000/U+E001 UTF-8
 * sequences: byte offsets (and therefore the cursor position passed to
 * scidown for preview scroll synchronization) are preserved exactly.
 *
 * The interactive preview maps the N-th rendered checkbox
 * (data-task-index) back to the N-th source marker, so
 * preprocess_task_lists() and marker_markdown_get_task_item_offset() must
 * agree on what a task item is and in which order they appear: both drive
 * the same TaskItemScanner below.
 */

static const char TASKLIST_PLACEHOLDER_UNCHECKED[]  = "\xEE\x80\x80"; /* U+E000 */
static const char TASKLIST_PLACEHOLDER_CHECKED[]    = "\xEE\x80\x81"; /* U+E001 */
static const char TASKLIST_PLACEHOLDER_NEUTRALIZED[] = "\xEE\x80\x82"; /* U+E002 */

static const char TASKLIST_INPUT_UNCHECKED_FMT[] =
  "<input type=\"checkbox\" class=\"task-list-item-checkbox\" data-task-index=\"%u\">";
static const char TASKLIST_INPUT_CHECKED_FMT[] =
  "<input type=\"checkbox\" class=\"task-list-item-checkbox\" data-task-index=\"%u\" checked>";

static gboolean
is_fence_marker (const char *start, const char *line_end, char *fence_char)
{
  const char *p = start;
  const char c = *p;
  int count = 0;

  while (p < line_end && *p == c) { count++; p++; }

  if (count >= 3 && (c == '`' || c == '~'))
  {
    if (fence_char)
      *fence_char = c;
    return TRUE;
  }
  return FALSE;
}

/*
 * Iterates task-list items in document order. Fenced code blocks are
 * skipped entirely.
 */
typedef struct
{
  const char *next_line; /* where to resume scanning */
  const char *end;      /* one past the last byte */
  gboolean    in_fence;
  char        fence_char;
} TaskItemScanner;

static void
task_item_scanner_init (TaskItemScanner *scan, const char *text, size_t size)
{
  scan->next_line = text;
  scan->end = text + size;
  scan->in_fence = FALSE;
  scan->fence_char = 0;
}

/*
 * Advances to the next task-list item ("[ ]" / "[x]" following a list
 * marker). Returns FALSE at the end of the document; otherwise
 * *marker_start receives the position of the '[' byte and *checked whether
 * the item is checked.
 */
static gboolean
task_item_scanner_next (TaskItemScanner  *scan,
                        const char      **marker_start,
                        gboolean          *checked)
{
  while (scan->next_line < scan->end)
  {
    const char *line = scan->next_line;
    const char *eol =
      (const char *) memchr (line, '\n', (size_t) (scan->end - line));
    const char *line_end = eol ? eol : scan->end;

    scan->next_line = eol ? (eol + 1) : scan->end;

    const char *p = line;
    while (p < line_end && *p == ' ')
      p++;

    /* Track fenced code blocks (``` / ~~~): never touch their content. */
    if (p < line_end && (p - line) <= 3 && (*p == '`' || *p == '~'))
    {
      char c = 0;
      if (is_fence_marker (p, line_end, &c))
      {
        if (!scan->in_fence)
        {
          scan->in_fence = TRUE;
          scan->fence_char = c;
        }
        else if (c == scan->fence_char)
        {
          scan->in_fence = FALSE;
        }
      }
    }

    if (scan->in_fence || p >= line_end)
      continue;

    /* list-item marker: '-', '*' or '+', or digits followed by '.' / ')' */
    const char *m = p;
    if (*m == '-' || *m == '*' || *m == '+')
      m++;
    else
    {
      const char *d = m;
      while (m < line_end && *m >= '0' && *m <= '9')
        m++;
      if (m == d || m >= line_end || (*m != '.' && *m != ')'))
        m = p;
      else
        m++;
    }

    if (m == p || m >= line_end)
      continue;

    const char *s = m;
    while (s < line_end && (*s == ' ' || *s == '\t'))
      s++;

    /* "[ ]" / "[x]" followed by a space, a tab or the end of the line */
    if (s > m &&
        (s + 3) <= line_end &&
        s[0] == '[' &&
        (s[1] == ' ' || s[1] == 'x' || s[1] == 'X') &&
        s[2] == ']' &&
        (s + 3 == line_end || s[3] == ' ' || s[3] == '\t' || s[3] == '\r'))
    {
      *marker_start = s;
      *checked = (s[1] != ' ');
      return TRUE;
    }
  }

  return FALSE;
}

/* Does the buffer contain a raw U+E000 / U+E001 sequence? */
static gboolean
contains_pua_placeholder (const char *buf, size_t size)
{
  for (size_t i = 0; (i + 3) <= size; i++)
    if ((unsigned char) buf[i]     == 0xEE &&
        (unsigned char) buf[i + 1] == 0x80 &&
        ((unsigned char) buf[i + 2] == 0x80 ||
         (unsigned char) buf[i + 2] == 0x81))
      return TRUE;
  return FALSE;
}

/* Rewrites every raw U+E000 / U+E001 to U+E002 (in place, 3 -> 3 bytes). */
static void
neutralize_pua_placeholders (char *buf, size_t size)
{
  for (size_t i = 0; (i + 3) <= size; i++)
    if ((unsigned char) buf[i]     == 0xEE &&
        (unsigned char) buf[i + 1] == 0x80 &&
        ((unsigned char) buf[i + 2] == 0x80 ||
         (unsigned char) buf[i + 2] == 0x81))
      buf[i + 2] = (char) 0x82;
}

/*
 * Rewrites "- [ ]" / "- [x]" (and ordered variants) list-item markers
 * into U+E000/U+E001 placeholders. Returns NULL when the document
 * contains no task-list item, so the original buffer can be rendered
 * as-is. Content of fenced code blocks is left untouched.
 */
static gchar*
preprocess_task_lists (const char *markdown, size_t size)
{
  g_autofree gchar *scratch = NULL;
  const char *scan_text = markdown;
  const char *copied = markdown;
  GString *out = NULL;
  TaskItemScanner scan;
  const char *marker = NULL;
  gboolean checked = FALSE;

  /* A stray U+E000/U+E001 in the document would be mistaken for a
   * placeholder by tasklists_postprocess() and shift the checkbox
   * numbering. Rewrite such chars to U+E002 first: same UTF-8 length,
   * so byte offsets (and the scidown cursor position) stay valid. */
  if (contains_pua_placeholder (markdown, size))
  {
    scratch = g_memdup2 (markdown, size);
    neutralize_pua_placeholders (scratch, size);
    scan_text = scratch;
    copied = scratch;
  }

  task_item_scanner_init (&scan, scan_text, size);
  while (task_item_scanner_next (&scan, &marker, &checked))
  {
    if (!out)
      out = g_string_sized_new (size + 32);

    g_string_append_len (out, copied, (gssize) (marker - copied));
    g_string_append (out, checked
                       ? TASKLIST_PLACEHOLDER_CHECKED
                       : TASKLIST_PLACEHOLDER_UNCHECKED);
    copied = marker + 3;
  }

  if (!out)
    return NULL;

  g_string_append_len (out, copied, (gssize) ((scan_text + size) - copied));
  return g_string_free (out, FALSE);
}

/*
 * Takes ownership of html; swaps the placeholders (in document order)
 * for numbered checkbox inputs. data-task-index matches the 0-based
 * task-item order used by marker_markdown_get_task_item_offset(), so
 * the interactive preview can map a click back to the source buffer.
 * Returns html untouched when it contains no placeholder.
 */
static gchar*
tasklists_postprocess (gchar *html)
{
  GString *str;
  guint task_index = 0;
  gsize search_from = 0;

  if (!html)
    return html;

  if (!strstr (html, TASKLIST_PLACEHOLDER_UNCHECKED) &&
      !strstr (html, TASKLIST_PLACEHOLDER_CHECKED))
    return html;

  str = g_string_new (html);
  g_free (html);

  for (;;)
  {
    /* Resume after the input inserted during the previous iteration. */
    const char *base = str->str + search_from;
    const char *unchecked = strstr (base, TASKLIST_PLACEHOLDER_UNCHECKED);
    const char *checked = strstr (base, TASKLIST_PLACEHOLDER_CHECKED);
    const char *found;
    gboolean is_checked;
    gchar *input;
    gsize pos;

    if (unchecked != NULL && (checked == NULL || unchecked < checked))
    {
      found = unchecked;
      is_checked = FALSE;
    }
    else if (checked != NULL)
    {
      found = checked;
      is_checked = TRUE;
    }
    else
    {
      break;
    }

    pos = (gsize) (found - str->str);
    input = g_strdup_printf (is_checked
                               ? TASKLIST_INPUT_CHECKED_FMT
                               : TASKLIST_INPUT_UNCHECKED_FMT,
                             task_index++);
    g_string_erase (str, pos, strlen (TASKLIST_PLACEHOLDER_UNCHECKED));
    g_string_insert (str, pos, input);
    search_from = pos + strlen (input);
    g_free (input);
  }

  return g_string_free (str, FALSE);
}

/*
 * Script injected in the live preview only (cursor_position >= 0):
 * checkbox clicks are reported to the application through a custom
 * marker://task-toggle/<index> URI intercepted by MarkerPreview.
 * Rendered exports (cursor_position < 0) do not include it.
 */
static const char TASKLIST_TOGGLE_SCRIPT[] =
  "<script>\n"
  "function initTaskListToggle() {\n"
  "  try {\n"
  "    if (sessionStorage.getItem('marker-tl-restore') === '1') {\n"
  "      var y = parseInt(sessionStorage.getItem('marker-tl-scroll') || '0', 10);\n"
  "      sessionStorage.removeItem('marker-tl-restore');\n"
  "      sessionStorage.removeItem('marker-tl-scroll');\n"
  "      var anchor = document.getElementById('cursor_pos');\n"
  "      if (anchor) { anchor.removeAttribute('id'); }\n"
  "      if (y > 0) { window.scrollTo(0, y); }\n"
  "    }\n"
  "  } catch (err) { }\n"
  "  document.addEventListener('click', function (e) {\n"
  "    var el = e.target;\n"
  "    if (el && el.classList && el.classList.contains('task-list-item-checkbox')) {\n"
  "      e.preventDefault();\n"
  "      e.stopPropagation();\n"
  "      try {\n"
  "        sessionStorage.setItem('marker-tl-restore', '1');\n"
  "        sessionStorage.setItem('marker-tl-scroll', String(window.scrollY));\n"
  "      } catch (err2) { }\n"
  "      window.location.href = 'marker://task-toggle/' + el.getAttribute('data-task-index');\n"
  "    }\n"
  "  }, true);\n"
  "}\n"
  "if (document.readyState === 'loading') {\n"
  "  document.addEventListener('DOMContentLoaded', initTaskListToggle);\n"
  "} else {\n"
  "  initTaskListToggle();\n"
  "}\n"
  "</script>\n";

/*
 * Returns the byte offset of the "[" marker of the index-th (0-based)
 * task-list item, in the same order used by preprocess_task_lists().
 * Returns FALSE when the document has no such item.
 */
gboolean
marker_markdown_get_task_item_offset (const char *markdown,
                                       size_t      size,
                                       guint       index,
                                       gsize      *offset)
{
  TaskItemScanner scan;
  const char *marker = NULL;
  gboolean checked = FALSE;
  guint current = 0;

  task_item_scanner_init (&scan, markdown, size);
  while (task_item_scanner_next (&scan, &marker, &checked))
  {
    if (current == index)
    {
      if (offset)
        *offset = (gsize) (marker - markdown);
      return TRUE;
    }
    current++;
  }

  return FALSE;
}

char*
marker_markdown_to_html(const char*         markdown,
                        size_t              size,
                        char *              base_folder,
                        MarkerMathJSMode     katex_mode,
                        MarkerHighlightMode highlight_mode,
                        MarkerMermaidMode   mermaid_mode,
                        const char*         stylesheet_location,
                        int                 cursor_position)
{
  char* html = NULL;

  hoedown_renderer* renderer;
  hoedown_document* document;
  hoedown_buffer* buffer;
  scidown_render_flags html_mode = get_render_mode(mermaid_mode);

  renderer = hoedown_html_renderer_new(html_mode, 0, get_local());

  g_autofree char *base_header = html_header(katex_mode, highlight_mode, mermaid_mode);
  g_autofree char *custom_css = marker_markdown_css(stylesheet_location);
  g_autofree char *scidown_css = marker_markdown_scidown_css();

  g_autofree char *header = g_strdup_printf("%s<style>\n%s\n%s\n</style>\n",
                                            base_header,
                                            custom_css ? custom_css : "",
                                            scidown_css ? scidown_css : "");

  g_autofree char *footer = html_footer(katex_mode, highlight_mode, mermaid_mode);
  if (cursor_position >= 0)
  {
    char *interactive_footer = g_strconcat (footer, TASKLIST_TOGGLE_SCRIPT, NULL);
    g_free (footer);
    footer = interactive_footer;
  }

  ext_definition def = {header, footer};

  document = hoedown_document_new(renderer,
                                  HOEDOWN_EXT_BLOCK         |
                                  HOEDOWN_EXT_SPAN          |
                                  HOEDOWN_EXT_FLAGS,
                                  &def,
                                  base_folder,
                                  16);

  buffer = hoedown_buffer_new(500);
  g_autofree gchar *tasklist_md = preprocess_task_lists(markdown, size);
  hoedown_document_render(document, buffer,
                          (uint8_t*) (tasklist_md ? tasklist_md : markdown),
                          tasklist_md ? strlen(tasklist_md) : size,
                          cursor_position);

  const char* buf_cstr = hoedown_buffer_cstr(buffer);
  html = tasklists_postprocess(g_strdup(buf_cstr));

  hoedown_html_renderer_free(renderer);
  hoedown_document_free(document);
  hoedown_buffer_free(buffer);

  return html;
}

char*
marker_markdown_to_html_with_css_inline(const char*         markdown,
                                        size_t              size,
                                        char *              base_folder,
                                        MarkerMathJSMode     katex_mode,
                                        MarkerHighlightMode highlight_mode,
                                        MarkerMermaidMode   mermaid_mode,
                                        const char*         stylesheet_location,
                                        int                 cursor_position)
{
  char* html = NULL;

  g_autofree char *inline_css = marker_markdown_css(stylesheet_location);
  g_autofree char *common_css = marker_markdown_scidown_css();
  g_autofree char *base_header = html_header(katex_mode, highlight_mode, mermaid_mode);

  hoedown_renderer* renderer;
  hoedown_document* document;
  hoedown_buffer* buffer;
  scidown_render_flags html_mode = get_render_mode(mermaid_mode);

  renderer = hoedown_html_renderer_new(html_mode, 0, get_local());

  g_autofree char *header = g_strdup_printf("%s<style>\n%s\n%s\n</style>\n",
                                            base_header,
                                            inline_css ? inline_css : "",
                                            common_css ? common_css : "");

  g_autofree char *footer = html_footer(katex_mode, highlight_mode, mermaid_mode);
  if (cursor_position >= 0)
  {
    char *interactive_footer = g_strconcat (footer, TASKLIST_TOGGLE_SCRIPT, NULL);
    g_free (footer);
    footer = interactive_footer;
  }

  ext_definition def = {header, footer};
  document = hoedown_document_new(renderer,
                                  HOEDOWN_EXT_BLOCK         |
                                  HOEDOWN_EXT_SPAN          |
                                  HOEDOWN_EXT_FLAGS,
                                  &def,
                                  base_folder,
                                  16);

  buffer = hoedown_buffer_new(500);
  g_autofree gchar *tasklist_md = preprocess_task_lists(markdown, size);
  hoedown_document_render(document, buffer,
                          (uint8_t*) (tasklist_md ? tasklist_md : markdown),
                          tasklist_md ? strlen(tasklist_md) : size,
                          cursor_position);

  const char* buf_cstr = hoedown_buffer_cstr(buffer);
  html = tasklists_postprocess(g_strdup(buf_cstr));

  hoedown_html_renderer_free(renderer);
  hoedown_document_free(document);
  hoedown_buffer_free(buffer);

  return html;
}

char*
marker_markdown_to_latex(const char*         markdown,
                         size_t              size,
                         char *              base_folder,
                         MarkerMathJSMode     katex_mode,
                         MarkerHighlightMode highlight_mode,
                         MarkerMermaidMode   mermaid_mode,
                         const char*         stylesheet_location)
{
  char* latex = NULL;

  hoedown_renderer* renderer;
  hoedown_document* document;
  hoedown_buffer* buffer;
  scidown_render_flags mode = get_render_mode(mermaid_mode);

  renderer = scidown_latex_renderer_new(mode, 0, get_local());

  ext_definition def = {NULL, NULL};


  document = hoedown_document_new(renderer,
                                  HOEDOWN_EXT_BLOCK         |
                                  HOEDOWN_EXT_SPAN          |
                                  HOEDOWN_EXT_FLAGS,
                                  &def,
                                  base_folder,
                                  16);

  buffer = hoedown_buffer_new(500);
  hoedown_document_render(document, buffer, (uint8_t*) markdown, size, -1);

  const char* buf_cstr = hoedown_buffer_cstr(buffer);
  latex = g_strdup(buf_cstr);
  hoedown_html_renderer_free(renderer);
  hoedown_document_free(document);
  hoedown_buffer_free(buffer);

  return latex;
}

void
marker_markdown_to_html_file(const char*         markdown,
                             size_t              size,
                             char               *base_folder,
                             MarkerMathJSMode     katex_mode,
                             MarkerHighlightMode highlight_mode,
                             MarkerMermaidMode   mermaid_mode,
                             const char*         stylesheet_location,
                             const char*         filepath)
{
  g_autofree char* html = marker_markdown_to_html(markdown,
                                                  size,
                                                  base_folder,
                                                  katex_mode,
                                                  highlight_mode,
                                                  mermaid_mode,
                                                  stylesheet_location,
                                                  -1);
  if (html && filepath)
  {
    g_file_set_contents(filepath, html, -1, NULL);
  }
}

void
marker_markdown_to_html_file_with_css_inline(const char*         markdown,
                                             size_t              size,
                                             char               *base_folder,
                                             MarkerMathJSMode     katex_mode,
                                             MarkerHighlightMode highlight_mode,
                                             MarkerMermaidMode   mermaid_mode,
                                             const char*         stylesheet_location,
                                             const char*         filepath)
{
  g_autofree char* html = marker_markdown_to_html_with_css_inline(markdown,
                                                                  size,
                                                                  base_folder,
                                                                  katex_mode,
                                                                  highlight_mode,
                                                                  mermaid_mode,
                                                                  stylesheet_location,
                                                                  -1);
  if (html && filepath)
  {
    g_file_set_contents(filepath, html, -1, NULL);
  }
}

void
marker_markdown_to_latex_file(const char*         markdown,
                              size_t              size,
                              char               *base_folder,
                              MarkerMathJSMode     katex_mode,
                              MarkerHighlightMode highlight_mode,
                              MarkerMermaidMode   mermaid_mode,
                              const char*         filepath)
{
  g_autofree char* latex = marker_markdown_to_latex(markdown,
                                                    size,
                                                    base_folder,
                                                    katex_mode,
                                                    highlight_mode,
                                                    mermaid_mode,
                                                    NULL);
  if (latex && filepath)
  {
    g_file_set_contents(filepath, latex, -1, NULL);
  }
}

metadata*
marker_markdown_metadata                     (const char         *markdown,
                                              size_t              size)
{
  return document_metadata((uint8_t*)markdown, size);
}
