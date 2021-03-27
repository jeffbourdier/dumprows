/* geojson.h - GeoJSON functions for DUMPROWS
 *
 * Copyright (c) 2021 Jeffrey Paul Bourdier
 *
 * Licensed under the MIT License.  This file may be used only in compliance with this License.
 * Software distributed under this License is provided "AS IS", WITHOUT WARRANTY OF ANY KIND.
 * For more information, see the accompanying License file or the following URL:
 *
 *   https://opensource.org/licenses/MIT
 */


/* Prevent multiple inclusion. */
#ifndef _GEOJSON_H_
#define _GEOJSON_H_


/**************************
 * Enum Type Declarations *
 **************************/

enum geojson_type
{
  GEOJSON_POINT,
  GEOJSON_LINESTRING,
  GEOJSON_POLYGON
};


/**************************
 * Structure Declarations *
 **************************/

struct geojson_info
{
  const char * begin;
  const char * end;
  char * text;
  enum geojson_type type;
  double min_x;
  double min_y;
  double max_x;
  double max_y;
};


/*************************
 * Function Declarations *
 *************************/

int geojson_parse(char * text, struct geojson_info * info_ptr);


#endif  /* (prevent multiple inclusion) */
