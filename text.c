/* text.c - text (string) functions for DUMPROWS
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

#include <string.h>  /* strchr, strlen, strrchr */
#include "text.h"    /* text_compare */


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Find the first case-insensitive occurrence of a substring within a string.
 *   haystack:  string to search
 *   needle:  substring to find
 * Return Value:  A pointer to the first occurrence of the substring, or NULL if the substring was not found.
 */
char * text_find(char * haystack, const char * needle)
{
  size_t n = strlen(needle);
  char * p, * q = haystack + strlen(haystack) - n;

  for (p = haystack; p <= q; ++p) if (!text_compare(p, needle, n)) return p;
  return NULL;
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
