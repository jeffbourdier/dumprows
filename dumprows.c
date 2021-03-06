/* dumprows.c - Database Utility Map-Producing Read-Only Web Service
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

/* stat, (struct) stat */
#include <sys/stat.h>

/* EXIT_FAILURE, EXIT_SUCCESS, free, getenv/_dupenv_s, malloc, system */
#include <stdlib.h>

#ifndef _WIN32
/* uintmax_t */
#  include <stdint.h>

/* memcpy, strchr, strlen */
#  include <string.h>
#endif

/* localtime/localtime_s, strftime, time, (struct) tm */
#include <time.h>

/* libgen.h:
 *   basename
 *
 * limits.h:
 *   INT_MIN
 *
 * stdio.h:
 *   fclose, FILE, fprintf, fputc, remove, snprintf/sprintf_s
 *
 * (struct) jb_command_option, jb_command_parse, jb_exe_strip, jb_file_open, jb_file_write, JB_MAX_PATH_LENGTH, jb_trim
 */
#include "jb.h"

/* (Win32 only) string.h:
 *   memcpy, strcat_s, strchr, strlen
 *
 * text_compare, text_output, text_search
 */
#include "text.h"


/*************
 * Constants *
 *************/

static const char * STR_USAGE = "COMMAND";
static const char * STR_HELP =
  "DUMPROWS (Database Utility Map-Producing Read-Only Web Service).\n"
  "Options:\n"
  "  -h, --help  output this message and exit\n"
  "  -l, --log   write message to log file";
static const char * STR_REMOTE_ADDR = "remote address could not be retrieved";
static const char * STR_QUERY_STRING = "query string could not be retrieved";
static const char * STR_INVALID_QUERY = "query string is not a valid SQL SELECT statement";
static const char * STR_TMP_PATH = "temporary path could not be determined";
static const char * STR_FILE_WRITTEN = "temporary file could not be written";
static const char * STR_FILE_READ = "temporary file could not be read";
static const char * STR_DB_UTILITY = "database utility could not be executed";


/*********************
 * Macro Definitions *
 *********************/

/* On Win32, getenv is considered "unsafe" and results in error C4996, and snprintf is unavailable.
 * Corresponding functions _dupenv_s and sprintf_s (respectively) are used instead.
 */
#ifdef _WIN32
#  define format_string sprintf_s
#  define get_environment_variable(value_ptr, name) _dupenv_s(value_ptr, NULL, name)
#else
#  define format_string snprintf
#  define get_environment_variable(value_ptr, name) ((*value_ptr = getenv(name)) ? 0 : -1)
#endif


/*********************************
 * Private Function Declarations *
 *********************************/

int validate_query(const char * s);
void format_results(char * html, char ** addl_head, char ** body_attr, char ** body_content);
char * format_element(const char * name, const char * content);
int finalize(char * name, time_t t, char * remote_addr, char * query_string, const char * error);
void log_message(char * name, time_t t, char * remote_addr, char * query_string, const char * error);


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Process database utility input (SQL) and output (HTML) as needed to respond to an HTTP request using CGI.
 */
int main(int argc, char * argv[])
{
  static const char * format = "%s < %s > %s 2>&1";

  static struct jb_command_option options[] =
  {
    { { "log", "l" }, 0 }
  };

  int n, i;
  char * v, * r, * p, * q, * p0, * q0, * p1, s[JB_MAX_PATH_LENGTH], s0[JB_MAX_PATH_LENGTH], s1[JB_MAX_PATH_LENGTH];
  time_t t = time(NULL);
  struct stat st;

  /* Verify usage. */
  n = sizeof(options) / sizeof(struct jb_command_option);
  n = jb_command_parse(argc, argv, STR_USAGE, STR_HELP, options, n, 1);
  if (n < 0) return (n == INT_MIN) ? EXIT_SUCCESS : EXIT_FAILURE;
  v = options[0].is_present ? argv[0] : NULL;

  /* Retrieve the following CGI environment variables:
   *   - REMOTE_ADDR, which is used in logging and temporary file naming
   *   - QUERY_STRING, which should be an SQL SELECT statement
   */
  if (get_environment_variable(&r, "REMOTE_ADDR") || !r) return finalize(v, t, NULL, NULL, STR_REMOTE_ADDR);
  if (get_environment_variable(&p, "QUERY_STRING") || !p) return finalize(v, t, r, NULL, STR_QUERY_STRING);

  /* URL-decode the query string. */
  q = (char *)malloc(n = strlen(p) + 2);
  for (p0 = p, q0 = q; p1 = strchr(p0, '%'); p0 += n + 3, q0 += n + 1)
  {
    if (!p1[1]) break; i = 0x10 * (p1[1] - 0x30);
    if (!p1[2]) break; i += p1[2] - 0x30;
    memcpy(q0, p0, n = p1 - p0);
    q0[n] = i;
  }
  memcpy(q0, p0, n = strlen(p0) + 1);
#ifdef _WIN32
  free(p);
#endif

  /* Verify that the query string is a valid SQL SELECT statement. */
  if (!(n = validate_query(q0 = jb_trim(q)))) return finalize(v, t, r, q, STR_INVALID_QUERY);
  if (q0[n - 1] != ';') { q0[n] = ';'; q0[++n] = '\0'; }

  /* Write the query string to a temporary file.  This will serve as
   * input to the database utility command line, to be executed shortly.
   */
#ifdef _WIN32
  /* For some very mysterious reason, if r is passed to sprintf_s (format_string),
   * Win32 treats it as NULL.  The call to strcat_s is to circumvent this.
   */
  if (get_environment_variable(&p, "TMP") || !p) return finalize(v, t, r, q, STR_TMP_PATH);
  format_string(s, JB_MAX_PATH_LENGTH, "%s\\%d_", p, t);
  free(p);
  strcat_s(s, JB_MAX_PATH_LENGTH, r);
#else
  format_string(s, JB_MAX_PATH_LENGTH, "/tmp/%ju_%s", (uintmax_t)t, r);
#endif
  format_string(s0, JB_MAX_PATH_LENGTH, "%s.%s", s, "sql");
  if (jb_file_write(s0, q0, n)) return finalize(v, t, r, q, STR_FILE_WRITTEN);

  /* Execute the command line (which should invoke a database utility).  Note that input (SQL) is redirected
   * from the temporary file just written, and output (HTML) is redirected to another temporary file.
   */
  format_string(s1, JB_MAX_PATH_LENGTH, "%s.%s", s, "html");
  p = (char *)malloc(n = strlen(format) + strlen(p1 = argv[argc - 1]) + strlen(s0) + strlen(s1));
  format_string(p, n, format, p1, s0, s1);
  n = system(p);
  free(p);
  remove(s0);
  if (n < 0) { remove(s1); return finalize(v, t, r, q, STR_DB_UTILITY); }

  /* Read (from the resulting temporary file) the HTML output produced by the database utility command line. */
  if (!(n = stat(s1, &st))) p = (char *)jb_file_read(s1, st.st_size);
  remove(s1);
  if (n || !p) return finalize(v, t, r, q, STR_FILE_READ);
  p[st.st_size] = '\0';

  /* Output the results, and we're done. */
  format_results(p, &q0, &p0, &p1);
  free(p);
  text_output("results", q0, p0, p1);
  free(q0);
  free(p0);
  free(p1);
  return finalize(v, t, r, q, NULL);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Determine whether or not a string appears to be a (singular) valid SQL SELECT statement.
 *   s:  string to validate
 * Return Value:  On success, the length of the string; otherwise, zero.
 */
int validate_query(const char * s)
{
  int i, n = strlen(s);

  /* There is a minimum string length (SELECT *). */
  if (n < 8) return 0;

  /* Check for unquoted semicolons (one at the end is OK). */
  i = text_search(s, ";");
  if (i && i < n) return 0;

  /* If the string begins with SELECT, make sure it does not have an unquoted INTO. */
  if (!text_compare(s, "SELECT", 6)) return text_search(s, "INTO") ? 0 : n;

  /* The only other substring with which the string can begin is WITH, but it must
   * have an unquoted SELECT, and must not have an unquoted INSERT, UPDATE, or DELETE.
   */
  if (text_compare(s, "WITH", 4)) return 0;
  if (text_search(s, "INSERT") || text_search(s, "UPDATE") || text_search(s, "DELETE")) return 0;
  return text_search(s, "SELECT") ? n : 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Build formatted strings representing the content/attribution for an HTML document to contain the results of the query.
 *   output:  output (presumably HTML) from database utility
 *   addl_head_ptr:  receives additional <head> element content (style, etc.)
 *   body_attr_ptr:  receives <body> element attribution (e.g., onload)
 *   body_content_ptr:  receives <body> element content
 */
void format_results(char * output, char ** addl_head, char ** body_attr, char ** body_content)
{
  static const char * table_style =
    "table { margin: auto; border-collapse: collapse; } "
    "table, th, td { border: 1px solid; padding: 4px; } "
    "th { background-color: #DFDFDF; }";

  int i;
  char * p, * q;

  /* Every database utility's output is different (e.g., some include <HTML>/<BODY>/<TABLE> tags, others don't).
   * The common denominator is the <TR> tags, which should always be present if the query was successful and rows were
   * returned.  If the output is empty, it probably means no rows were returned (e.g., SQLite/SpatiaLite does this).
   */
  if (!strlen(output)) { *addl_head = *body_attr = NULL; *body_content = format_element("h1", "No results."); return; }

  /* If the output is not HTML, it probably means there was an error
   * (syntax or otherwise) with the query (table/view not found, etc.).
   */
  if (output[0] != '<') { *addl_head = *body_attr = NULL; *body_content = format_element("pre", output); return; }

  /* If no <tr> tag is found, it could mean a query error or no rows were returned (as
   * with SQL*Plus).  In this case, just use the content of the <body> element as-is.
   */
  if (!(i = text_search(output, "<tr>")))
  {
    *addl_head = *body_attr = NULL;
    i = text_search(output, "<body>") + 5;
    output[text_search(output, "</body>") - 1] = '\0';
    *body_content = (char *)malloc(strlen(output) - i + 1);
    p = jb_trim(output + i);
    memcpy(*body_content, p, strlen(p) + 1);
    return;
  }

  /* If the resulting table has a WKT geometry column, show the features on a map.
   * (For now, just assume it doesn't, extract the <tr> elements and embed them in our own <table> element.)
   */
  for (q = p = output + i - 1; i = text_search(q, "</tr>"); q += i + 4);
  *q = '\0';
  *addl_head = format_element("style", table_style);
  *body_attr = NULL;
  *body_content = format_element("table", p);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Build a formatted string representing an HTML element.
 *   name:  element name
 *   content:  element content
 * Return Value:  Formatted string.  Memory for this buffer is obtained with malloc, and should be freed with free.
 */
char * format_element(const char * name, const char * content)
{
  static const char * format = "<%s>%s</%s>";

  char * p;
  size_t n;

  p = (char *)malloc(n = strlen(format) + 2 * strlen(name) + strlen(content));
  format_string(p, n, format, name, content, name);
  return p;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output (as HTML) an error message, and/or write a message to the log file.
 *   name:  pathname of executable file if logging; otherwise, NULL
 *   t:  value of time (in seconds since Epoch)
 *   remote_addr:  CGI environment variable (IP address of remote host)
 *   query_string:  CGI environment variable (query string from URL)
 *   error:  error message (if any)
 * Return Value:  Exit status (EXIT_SUCCESS or EXIT_FAILURE).
 */
int finalize(char * name, time_t t, char * remote_addr, char * query_string, const char * error)
{
  static const char * format = "<h1>Error: %s</h1>";

  char * p;
  size_t n;

  /* If there is an error message, output it as HTML. */
  if (error)
  {
    p = (char *)malloc(n = strlen(format) + strlen(error));
    format_string(p, n, format, error);
    text_output("error", NULL, NULL, p);
    free(p);
  }

  /* If specified, write a message to the log file. */
  if (name) log_message(name, t, remote_addr, query_string, error);

  /* Free memory as needed. */
#ifdef _WIN32
  free(remote_addr);
#endif
  free(query_string);

  /* Return the appropriate exit status based on whether or not there is an error message. */
  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Write a message to the log file.
 *   name:  pathname of executable file
 *   t:  value of time (in seconds since Epoch)
 *   remote_addr:  CGI environment variable (IP address of remote host)
 *   query_string:  CGI environment variable (query string from URL)
 *   error:  error message (if any)
 */
void log_message(char * name, time_t t, char * remote_addr, char * query_string, const char * error)
{
#ifdef _WIN32
  static const char * r = "C:\\ProgramData\\";
#else
  static const char * r = "/var/log/";
#endif

  char * p, s[JB_MAX_PATH_LENGTH], ts[20];
  struct tm * tm_ptr;
  size_t n = sizeof(ts);
  FILE * f;

  /* Parse the filename from the executable file path, and then format the log file pathname.
   * (It is assumed that on Win32, the filename ends in ".exe", whereas on Linux, the filename has no extension.)
   */
  p = basename(name);
#ifdef _WIN32
  jb_exe_strip(p);
#endif
  format_string(s, JB_MAX_PATH_LENGTH, "%s%s.log", r, p);

  /* Format the timestamp.
   * (Despite the documentation, Win32 does not support the %F or %T formatting
   * codes for strftime, so build the ISO 8601 date-time format string manually.)
   */
#ifdef _WIN32
  tm_ptr = (struct tm *)malloc(sizeof(struct tm));
  localtime_s(tm_ptr, &t);
#else
  tm_ptr = localtime(&t);
#endif
  strftime(ts, n, "%Y-%m-%d %H:%M:%S", tm_ptr);
#ifdef _WIN32
  free(tm_ptr);
#endif

  /* Append the message to the log file.
   * (Note that on Win32, by default, a file is opened in text mode, so "\n" is translated to "\r\n" on output.)
   */
  jb_file_open(&f, s, "a");
  if (!f) return;
  fprintf(f, "\n%s", ts);
  if (remote_addr) fprintf(f, "\t%s", remote_addr);
  if (error) fprintf(f, "\nError: %s", error);
  if (query_string) fprintf(f, "\n%s", query_string);
  fputc('\n', f);
  fclose(f);
}
