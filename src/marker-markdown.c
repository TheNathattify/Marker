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

  char * buffer = g_strdup_printf("%s\n%s\n%s\n%s\n%s\n%s\n",mathjs_css, highlight_css, mathjs_script, mathjs_auto, highlight_script, mermaid_script);

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
  g_autofree gchar *path = g_strdup_printf("%s%s", marker_utils_get_styles_dir (), css_link);
  gchar *contents = NULL;
  gsize length = 0;
  if (g_file_get_contents(path, &contents, &length, NULL))
  {
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
  hoedown_document_render(document, buffer, (uint8_t*) markdown, size, cursor_position);

  const char* buf_cstr = hoedown_buffer_cstr(buffer);
  html = g_strdup(buf_cstr);

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
  hoedown_document_render(document, buffer, (uint8_t*) markdown, size, cursor_position);

  const char* buf_cstr = hoedown_buffer_cstr(buffer);
  html = g_strdup(buf_cstr);

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
