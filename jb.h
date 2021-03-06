/* jb.h - Jeff Bourdier (JB) library
 *
 * Copyright (c) 2017-21 Jeffrey Paul Bourdier
 *
 * Licensed under the MIT License.  This file may be used only in compliance with this License.
 * Software distributed under this License is provided "AS IS", WITHOUT WARRANTY OF ANY KIND.
 * For more information, see the accompanying License file or the following URL:
 *
 *   https://opensource.org/licenses/MIT
 */


/* Prevent multiple inclusion. */
#ifndef _JB_H_
#define _JB_H_


/*****************
 * Include Files *
 *****************/

/* INT_MIN (returned by jb_command_parse), PATH_MAX */
#include <limits.h>

/* fopen/fopen_s */
#include <stdio.h>

#ifdef _WIN32
/* _MAX_PATH */
#  include <stdlib.h>
#else
/* errno */
#  include <errno.h>

/* basename, dirname */
#  include <libgen.h>
#endif


/**************************
 * Structure Declarations *
 **************************/

struct jb_command_option
{
  const char * text[2];
  union { int is_present; const char * argument; };
};


/*********************
 * Macro Definitions *
 *********************/

/* On Win32, fopen is considered "unsafe" and results in error C4996.
 * The corresponding "safe" function, fopen_s, is used instead.
 */
#ifdef _WIN32
#  define JB_DIRECTORY_SEPARATOR '\\'
#  define JB_MAX_PATH_LENGTH _MAX_PATH
#  define jb_file_open(stream_ptr, pathname, mode) fopen_s(stream_ptr, pathname, mode)
#else
#  define JB_DIRECTORY_SEPARATOR '/'
#  define JB_MAX_PATH_LENGTH PATH_MAX
#  define jb_file_open(stream_ptr, pathname, mode) ((*stream_ptr = fopen(pathname, mode)) ? 0 : errno)
#endif


/*************************
 * Function Declarations *
 *************************/

#ifdef _WIN32
/* On Linux, these functions are defined in libgen.h.  There are no Win32 equivalents. */
char * basename(char * path);
char * dirname(char * path);

/* This function applies only to Win32. */
void jb_exe_strip(char * filename);
#endif

int jb_command_parse(int argc, char * argv[], const char * usage, const char * help,
                     struct jb_command_option * options, int option_count, int arg_count);
void * jb_file_read(const char * path, size_t size);
int jb_file_write(const char * path, const void * buffer, size_t size);
char * jb_trim(char * s);


#endif  /* (prevent multiple inclusion) */
