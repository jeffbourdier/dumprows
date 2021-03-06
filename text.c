/* text.c - text functions for DUMPROWS
 *
 * Copyright (c) 2021 Jeffrey Paul Bourdier
 *
 * Licensed under the MIT License.  This file may be used only in compliance with this License.
 * Software distributed under this License is provided "AS IS", WITHOUT WARRANTY OF ANY KIND.
 * For more information, see the accompanying License file or the following URL:
 *
 *   https://opensource.org/licenses/MIT
 */


/*****************
 * Include Files *
 *****************/

/* printf */
#include <stdio.h>

#ifndef _WIN32
/* strchr, strlen, strrchr */
#  include <string.h>
#endif

/* (Win32 only) string.h:
 *   strchr, strlen, strrchr
 *
 * text_compare
 */
#include "text.h"


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output an HTML document with the given content/attribution.
 *   title:  <title> element content
 *   addl_head:  additional <head> element content (style, etc.)
 *   body_attr:  <body> element attribution (e.g., onload)
 *   body_content:  <body> element content
 */
void text_output(const char * title, const char * addl_head, const char * body_attr, const char * body_content)
{
  static const char * format =
    "Content-Type: text/html\r\n\r\n"
    "<!DOCTYPE html>"
    "<html lang=\"en-US\">"
      "<head>"
        "<meta charset=\"UTF-8\" />"
        "<title>%s</title>"
        "%s"
      "</head>"
      "<body%s%s>%s</body>"
    "</html>";

  printf(format, title, (addl_head ? addl_head : ""), (body_attr ? " " : ""), (body_attr ? body_attr : ""), body_content);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Find the first unquoted, case-insensitive occurrence of a substring within a string.
 *   haystack:  string to search
 *   needle:  substring to find
 * Return Value:
 *   If greater than zero, the 1-based index of the first unquoted occurrence of the substring.
 *   Otherwise, if less than zero, a closing quote was not found.
 *   Otherwise, zero.
 */
int text_search(const char * haystack, const char * needle)
{
  size_t n = strlen(needle);
  const char * p, * q = haystack + strlen(haystack) - n;

  for (p = haystack; p <= q; ++p)
  {
    switch (*p)
    {
    case '\'': if (p = strchr(++p, '\'')) continue; return -1;
    case '\"': if (p = strchr(++p, '\"')) continue; return -1;
    default: if (!text_compare(p, needle, n)) return p - haystack + 1;
    }
  }
  return 0;
}
