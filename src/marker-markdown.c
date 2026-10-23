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
 */

static const char TASKLIST_PLACEHOLDER_UNCHECKED[] = "\xEE\x80\x80"; /* U+E000 */
static const char TASKLIST_PLACEHOLDER_CHECKED[]   = "\xEE\x80\x81"; /* U+E001 */

static const char TASKLIST_HTML_UNCHECKED[] =
  "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled>";
static const char TASKLIST_HTML_CHECKED[] =
  "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled checked>";

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
 * Rewrites "- [ ]" / "- [x]" (and ordered variants) list-item markers
 * into U+E000/U+E001 placeholders. Returns NULL when the document
 * contains no task-list item, so the original buffer can be rendered
 * as-is. Content of fenced code blocks is left untouched.
 */
static gchar*
preprocess_task_lists (const char *markdown, size_t size)
{
  GString *out = NULL;
  const char *copied = markdown;
  const char *line = markdown;
  const char *end = markdown + size;
  gboolean in_fence = FALSE;
  char fence_char = 0;

  while (line < end)
  {
    const char *eol = (const char *) memchr (line, '\n', (size_t) (end - line));
    const char *next = eol ? (eol + 1) : end;
    const char *line_end = eol ? eol : end;

    const char *p = line;
    while (p < line_end && *p == ' ')
      p++;

    /* Track fenced code blocks (``` / ~~~): never touch their content. */
    if (p < line_end && (p - line) <= 3 && (*p == '`' || *p == '~'))
    {
      char c = 0;
      if (is_fence_marker (p, line_end, &c))
      {
        if (!in_fence)
        {
          in_fence = TRUE;
          fence_char = c;
        }
        else if (c == fence_char)
        {
          in_fence = FALSE;
        }
      }
    }

    if (!in_fence && p < line_end)
    {
      const char *m = p;

      /* unordered marker: -, * or + */
      if (*m == '-' || *m == '*' || *m == '+')
        m++;
      /* ordered marker: digits followed by . or ) */
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

      if (m > p && m < line_end)
      {
        const char *s = m;
        while (s < line_end && (*s == ' ' || *s == '\t'))
          s++;

        if (s > m &&
            (s + 3) <= line_end &&
            s[0] == '[' &&
            (s[1] == ' ' || s[1] == 'x' || s[1] == 'X') &&
            s[2] == ']' &&
            (s + 3 == line_end || s[3] == ' ' || s[3] == '\t' || s[3] == '\r'))
        {
          if (!out)
            out = g_string_sized_new (size + 32);

          g_string_append_len (out, copied, (gssize) (s - copied));
          g_string_append (out, (s[1] == ' ')
                            ? TASKLIST_PLACEHOLDER_UNCHECKED
                            : TASKLIST_PLACEHOLDER_CHECKED);
          copied = s + 3;
        }
      }
    }

    line = next;
  }

  if (!out)
    return NULL;

  g_string_append_len (out, copied, (gssize) (end - copied));
  return g_string_free (out, FALSE);
}

static void
tasklist_replace (GString *str, const char *placeholder, const char *replacement)
{
  const gsize placeholder_len = strlen (placeholder);
  const char *found;

  while ((found = strstr (str->str, placeholder)) != NULL)
  {
    const gsize pos = (gsize) (found - str->str);
    g_string_erase (str, pos, placeholder_len);
    g_string_insert (str, pos, replacement);
  }
}

/*
 * Takes ownership of html; swaps the placeholders for checkbox inputs.
 * Returns html untouched when it contains no placeholder.
 */
static gchar*
tasklists_postprocess (gchar *html)
{
  GString *str;

  if (!html)
    return html;

  if (!strstr (html, TASKLIST_PLACEHOLDER_UNCHECKED) &&
      !strstr (html, TASKLIST_PLACEHOLDER_CHECKED))
    return html;

  str = g_string_new (html);
  g_free (html);

  tasklist_replace (str, TASKLIST_PLACEHOLDER_UNCHECKED, TASKLIST_HTML_UNCHECKED);
  tasklist_replace (str, TASKLIST_PLACEHOLDER_CHECKED, TASKLIST_HTML_CHECKED);

  return g_string_free (str, FALSE);
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
