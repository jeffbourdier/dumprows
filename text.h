/* text.h - text functions for DUMPROWS
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
#ifndef _TEXT_H_
#define _TEXT_H_


/*****************
 * Include Files *
 *****************/

#ifdef _WIN32
/* _strnicmp */
#  include <string.h>
#else
/* strncasecmp */
#  include <strings.h>
#endif


/*********************
 * Macro Definitions *
 *********************/

#ifdef _WIN32
#  define text_compare _strnicmp
#else
#  define text_compare strncasecmp
#endif


/*************************
 * Function Declarations *
 *************************/

void text_output(const char * title, const char * addl_head, const char * body_attr, const char * body_content);
int text_search(const char * haystack, const char * needle);


#endif  /* (prevent multiple inclusion) */
