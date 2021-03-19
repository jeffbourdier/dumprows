/* html.h - HTML functions for DUMPROWS
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
#ifndef _HTML_H_
#define _HTML_H_


/*****************
 * Include Files *
 *****************/

#include "geojson.h"  /* (struct) geojson_info */


/*************************
 * Function Declarations *
 *************************/

char * html_element(const char * name, const char * content);
void html_format(char * rows, struct geojson_info * infos, int info_count,
                 char ** addl_head_ptr, const char ** body_attr_ptr, char ** body_content_ptr);
void html_output(const char * title, const char * addl_head, const char * body_attr, const char * body_content);


#endif  /* (prevent multiple inclusion) */
