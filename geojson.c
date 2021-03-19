/* geojson.c - GeoJSON functions for DUMPROWS
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

#include <ctype.h>    /* isspace */
#include <errno.h>    /* errno */
#include <stdlib.h>   /* free, malloc, strtod */
#include <string.h>   /* strlen, strncmp */
#include "geojson.h"  /* (struct) geojson_info, GEOJSON_LINESTRING, GEOJSON_POINT, GEOJSON_POLYGON */
#include "text.h"     /* text_append, text_compare, text_search */


/**************************
 * Enum Type Declarations *
 **************************/

enum parsing_state
{
  PARSING_ERROR,
  PARSING_BEGIN,
  PARSING_NAME1,
  PARSING_COLON1,
  PARSING_VALUE1,
  PARSING_COMMA,
  PARSING_NAME2,
  PARSING_COLON2,
  PARSING_VALUE2,
  PARSING_END,
  PARSING_DONE
};


/*********************************
 * Private Function Declarations *
 *********************************/

size_t parse_char(const char * text, struct geojson_info * info_ptr, size_t size, char c);
size_t parse_name(const char * text, struct geojson_info * info_ptr, size_t size, const char * name);
size_t parse_type(const char * text, struct geojson_info * info_ptr, size_t size);
size_t parse_quote(const char * text);
size_t parse_bounds(const char * text, struct geojson_info * info_ptr, size_t size);


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parse HTML <td> element content for GeoJSON geometry.
 *   text:  content of <td> element to parse
 *   info_ptr:  receives GeoJSON geometry information
 * Return Value:  Zero on success (content appears to be valid GeoJSON geometry); otherwise, nonzero.
 */
int geojson_parse(const char * text, struct geojson_info * info_ptr)
{
  static const char * name1 = "type";
  static const char * name2 = "coordinates";

  const char * p, * q = text + text_search(text, "</td>") - 1;
  enum parsing_state state = PARSING_BEGIN;
  size_t n, m = q - text;

  info_ptr->text = (char *)malloc(m);
  *info_ptr->text = '\0';
  for (p = info_ptr->begin = text; p < q; p += n)
  {
    /* Skip any white-space. */
    if (isspace(*p)) { n = 1; continue; }

    /* Take the appropriate action based on the parsing state. */
    switch (state)
    {
    case PARSING_BEGIN:  state = (n = parse_char(p, info_ptr, m, '{'  )) ? PARSING_NAME1  : PARSING_ERROR; break;
    case PARSING_NAME1:  state = (n = parse_name(p, info_ptr, m, name1)) ? PARSING_COLON1 : PARSING_ERROR; break;
    case PARSING_COLON1: state = (n = parse_char(p, info_ptr, m, ':'  )) ? PARSING_VALUE1 : PARSING_ERROR; break;
    case PARSING_VALUE1: state = (n = parse_type(p, info_ptr, m       )) ? PARSING_COMMA  : PARSING_ERROR; break;
    case PARSING_COMMA:  state = (n = parse_char(p, info_ptr, m, ','  )) ? PARSING_NAME2  : PARSING_ERROR; break;
    case PARSING_NAME2:  state = (n = parse_name(p, info_ptr, m, name2)) ? PARSING_COLON2 : PARSING_ERROR; break;
    case PARSING_COLON2: state = (n = parse_char(p, info_ptr, m, ':'  )) ? PARSING_VALUE2 : PARSING_ERROR; break;
    case PARSING_VALUE2: state = (n = parse_bounds(p, info_ptr, m     )) ? PARSING_END    : PARSING_ERROR; break;
    case PARSING_END:    state = (n = parse_char(p, info_ptr, m, '}'  )) ? PARSING_DONE   : PARSING_ERROR; break;
    default: state = PARSING_ERROR;
    }
    if (state == PARSING_ERROR) break;
  }
  if (state == PARSING_ERROR) { free(info_ptr->text); return -1; }

  /* All good. */
  info_ptr->end = q;
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parse GeoJSON text for a character.
 *   text:  GeoJSON text to parse
 *   info_ptr:  pointer to geojson_info structure
 *   size:  number of bytes to allocated for info_ptr->text
 *   c:  character
 * Return Value:  On success, the number of characters parsed; otherwise, zero.
 */
size_t parse_char(const char * text, struct geojson_info * info_ptr, size_t size, char c)
{
  if (*text != c) return 0;
  text_append(info_ptr->text, size, text, 1);
  return 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parse GeoJSON text for a member name (a token that may or may not be quoted).
 *   text:  GeoJSON text to parse
 *   info_ptr:  pointer to geojson_info structure
 *   size:  number of bytes to allocated for info_ptr->text
 *   name:  member name
 * Return Value:  On success, the number of characters parsed; otherwise, zero.
 */
size_t parse_name(const char * text, struct geojson_info * info_ptr, size_t size, const char * name)
{
  const char * p = text;
  size_t n = parse_quote(p), m = strlen(name);

  if (strlen(p += n) < m || strncmp(p, name, m)) return 0;
  if (text_compare(p += m, text, n)) return 0;
  text_append(info_ptr->text, size, text + n, m);
  return p + n - text;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parse GeoJSON text for a supported geometry type (which must be quoted).
 *   text:  GeoJSON text to parse
 *   info_ptr:  pointer to geojson_info structure
 *   size:  number of bytes to allocated for info_ptr->text
 * Return Value:  On success, the number of characters parsed; otherwise, zero.
 */
size_t parse_type(const char * text, struct geojson_info * info_ptr, size_t size)
{
  const char * p = text;
  size_t n = parse_quote(p), m;

  if (!n || strlen(p += n) < 10 + n) return 0;
  if (!strncmp(p, "Point",      m =  5)) info_ptr->type = GEOJSON_POINT;      else
  if (!strncmp(p, "LineString", m = 10)) info_ptr->type = GEOJSON_LINESTRING; else
  if (!strncmp(p, "Polygon",    m =  7)) info_ptr->type = GEOJSON_POLYGON;    else return 0;
  if (text_compare(p += m, text, n)) return 0;
  text_append(info_ptr->text, size, "'", 1);
  text_append(info_ptr->text, size, text + n, m);
  text_append(info_ptr->text, size, "'", 1);
  return p + n - text;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parse GeoJSON text for a quote character reference.
 *   text:  GeoJSON text to parse
 * Return Value:  On success, the number of characters parsed; otherwise, zero.
 */
size_t parse_quote(const char * text)
{
  if (*text == '\'' || *text == '\"') return 1;
  return (strlen(text) < 6 || (text_compare(text, "&apos;", 6) && text_compare(text, "&quot;", 6))) ? 0 : 6;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Parse GeoJSON text for bounding coordinates.
 *   text:  GeoJSON text to parse
 *   info_ptr:  pointer to geojson_info structure
 *   size:  number of bytes to allocated for info_ptr->text
 * Return Value:  On success, the number of characters parsed; otherwise, zero.
 */
size_t parse_bounds(const char * text, struct geojson_info * info_ptr, size_t size)
{
  const char * p;
  int i = 0, n = 0;
  double d;
  char * q;

  info_ptr->min_x =  180; info_ptr->min_y =  90;
  info_ptr->max_x = -180; info_ptr->max_y = -90;

  for (p = text; *p; ++p)
  {
    /* Skip white-space and commas. */
    if (isspace(*p) || *p == ',') continue;

    /* A left/opening bracket means increment the count, and the next coordinate should be an X-value. */
    if (*p == '[') { ++i; n = 1; continue; }

    /* A right/closing bracket means decrement the count.  If the count is back down to zero,
     * we're done.  Determine whether or not bounding coordinates were successfully parsed.
     */
    if (*p == ']')
    {
      if (--i) continue;
      if (info_ptr->min_x == 180 || info_ptr->min_y == 90 || info_ptr->max_x == -180 || info_ptr->max_y == -90) return 0;
      text_append(info_ptr->text, size, text, n = ++p - text);
      return n;
    }

    /* The only other token encountered here should be a floating-point number representing a coordinate. */
    errno = 0;
    d = strtod(p, &q);
    if (errno) return 0;
    p = q - 1;

    /* If the coordinate is supposed to be an X-value, see if it should be the new minimum/maximum. */
    if (n > 0)
    {
      if (d < info_ptr->min_x) info_ptr->min_x = d;
      if (d > info_ptr->max_x) info_ptr->max_x = d;

      /* The next coordinate should be a Y-value. */
      n = -1; continue;
    }

    /* If the coordinate is supposed to be a Y-value, see if it should be the new minimum/maximum. */
    if (n < 0)
    {
      if (d < info_ptr->min_y) info_ptr->min_y = d;
      if (d > info_ptr->max_y) info_ptr->max_y = d;

      /* No more coordinates should be parsed until the next X-coordinate,
       * which will be heralded by the next left/opening bracket.
       */
      n = 0; continue;
    }
  }

  /* If we get here, the string is not valid GeoJSON. */
  return 0;
}
