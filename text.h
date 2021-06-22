/* text.h - text (string) functions for DUMPROWS
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

#include <stdio.h>      /* snprintf/sprintf_s */
#include <string.h>     /* strncat/strncat_s, _strnicmp */
#ifndef _WIN32
#  include <strings.h>  /* strncasecmp */
#endif


/*********************
 * Macro Definitions *
 *********************/

/* On Win32, strncat is considered "unsafe" (resulting in error C4996), and strncasecmp & snprintf are unavailable.
 * Corresponding functions strncat_s, _strnicmp, & sprintf_s (respectively) are used instead.
 */
#ifdef _WIN32
#  define text_append(dest, size, src, n) strncat_s(dest, size, src, n)
#  define text_compare _strnicmp
#  define text_format sprintf_s
#else
#  define text_append(dest, size, src, n) strncat(dest, src, n)
#  define text_compare strncasecmp
#  define text_format snprintf
#endif


/*************************
 * Function Declarations *
 *************************/

char * text_find(char * haystack, const char * needle);
char * text_read(const char * path);
int text_search(const char * haystack, const char * needle);


#endif  /* (prevent multiple inclusion) */
