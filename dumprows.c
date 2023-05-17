/* dumprows.c - Database Utility Map-Producing Read-Only Web Service
 *
 * Copyright (c) 2021-3 Jeffrey Paul Bourdier
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

#ifdef _WIN32
/* This eliminates deprecation warnings for functions that are considered "unsafe" (and would
 * result in error C4996) on Win32, thus allowing us to write simpler, more portable code.
 */
#define _CRT_SECURE_NO_WARNINGS
#else
/* This exposes strcasestr, which is a nonstandard extension on Linux. */
#define _GNU_SOURCE
#endif

#include <sys/stat.h>   /* stat, (struct) stat */
#include <ctype.h>      /* isdigit, isupper */
#include <errno.h>      /* EINVAL, errno */
#include <limits.h>     /* INT_MIN */
#include <stdio.h>      /* EOF, fclose, ferror, fflush, fgetc, fgets, FILE, fopen,
                           fputs, pclose/_pclose, popen/_popen, printf, puts */
#include <stdlib.h>     /* EXIT_FAILURE, EXIT_SUCCESS, free, getenv, malloc */
#include <string.h>     /* memcpy, strcasestr, strcmp, strerror, strlen, strncmp, _strnicmp, strstr */
#ifndef _WIN32
#  include <strings.h>  /* strncasecmp */
#endif
#include "html.h"       /* HTML_PROMPT_1, HTML_PROMPT_2, HTML_PROMPT_3, HTML_RESULTS */
#include "jb.h"         /* jb_command_error, jb_command_parse, jb_trim */


/*************
 * Constants *
 *************/

static const char * STR_USAGE = " FILE";
static const char * STR_HELP =
  "DUMPROWS (Database Utility Map-Producing Read-Only Web Service).\n"
  "This application is meant to be run as a Common Gateway Interface (CGI) program.\n"
  "To test via command line, set the QUERY_STRING environment variable.\n"
  "For more information, see the home page.\n"
  "Options:\n"
  "  -h, --help  output this message and exit";
static const char * STR_QUERY = "query string is not valid";
static const char * STR_RESULTS = "Results";
static const char * STR_FILE = "File is improperly formatted";
static const char * STR_DATABASE = "Unknown database engine/utility";
static const char * STR_PROMPT = "Prompt";
static const char * STR_READING = "Error reading template file";
static const char * STR_TEMPLATES = "Not all templates loaded successfully.";
static const char * STR_ERROR = "Error";

/* Command lines */
static const char * STR_SQLPLUS = "sqlplus -M \"HTML ON HEAD '<title>Results - DUMPROWS</title>"
  HTML_RESULTS "' BODY 'onload=''init()''' TABLE ''\" -S -F ";
static const char * STR_PSQL = "psql -H ";
static const char * STR_SQLITE = "sqlite3 -header -html -batch ";
static const char * STR_SPATIALITE = "spatialite -header -html -silent -batch ";


/*********************
 * Macro Definitions *
 *********************/

#ifdef _WIN32
/* On Win32, strncasecmp is unavailable.  Corresponding function _strnicmp is used instead. */
#define strncasecmp _strnicmp
#endif

#define char_to_hex(c) (c - (isdigit(c) ? '0' : ((isupper(c) ? 'A' : 'a') - 0xA)))
#define output_begin(title) printf("<html lang='en-US'><head><meta charset='UTF-8' /><title>%s - DUMPROWS</title>", title)
#define output_bridge(attribution) printf("</head><body%s>", attribution)
#define output_end() puts("</body></html>")


/*********************************
 * Private Function Declarations *
 *********************************/

#ifdef _WIN32
/* On Linux, this function is defined in string.h.  There is no Win32 equivalent. */
char * strcasestr(const char * haystack, const char * needle);
#endif

const char * read_file(char * argv[], size_t length, FILE ** stream_ptr, char ** string_ptr);
const char * read_line(char * string, int count, FILE * stream);
void output_prompt(const char * path);
const char * read_templates(const char * path, char ** string_ptr);
size_t validate_query(const char * string);
int finalize(char * string, char * query, const char * error);


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Process database utility input (SQL) and output (HTML) as needed to respond to an HTTP request using CGI.
 */
int main(int argc, char * argv[])
{
  int n, i;
  char * s, * r, * q, * q1, * s1;
  FILE * f;
  const char * p;

  /* Verify usage. */
  n = jb_command_parse(argc, argv, STR_USAGE, STR_HELP, NULL, 0, 1);
  if (n < 0) return (n == INT_MIN) ? EXIT_SUCCESS : EXIT_FAILURE;

  /* Retrieve the CGI environment variable QUERY_STRING, which (if nonempty) should contain an SQL SELECT statement. */
  if (!(s = getenv("QUERY_STRING"))) { jb_command_error(argv[0], STR_USAGE); return EXIT_FAILURE; }

  /* Retrieve the CGI environment variable SERVER_SOFTWARE, which is used to determine
   * whether or not to output the entity-header (required by all web servers except IIS).
   */
  if (!(r = getenv("SERVER_SOFTWARE")) || !strlen(r) || strncmp(r, "Microsoft-IIS", 13))
    printf("Content-Type: text/html\r\n\r\n");

  puts("<!DOCTYPE html>");

  /* Read and parse the script file. */
  p = read_file(argv, n = strlen(s), &f, &r);
  if (f) fclose(f);
  if (p) return finalize(r, NULL, p);

  /* If the query string is empty, output a web page to prompt for a query. */
  if (!n) { output_prompt(r); return finalize(r, NULL, NULL); }

  /* The query string must begin with "q=". */
  if (strncmp(s, "q=", 2)) return finalize(r, NULL, STR_QUERY);

  /* Replace plus signs in the query string with spaces. */
  for (q = s; *q; ++q) if (*q == '+') *q = ' ';

  /* Allocate memory for the query (i.e., the SQL SELECT statement to be parsed from the query string).
   * (Conveniently, the "q=" provides two extra bytes needed for an optional semicolon and a terminating null byte.)
   */
  if (!(q = malloc(strlen(s)))) return finalize(r, NULL, strerror(errno));

  /* URL-decode the query. */
  for (q1 = q, s1 = s + 2; p = strchr(s1, '%'); q1 += n + 1, s1 += n + 3)
  {
    if (!p[1]) break; i = 0x10 * char_to_hex(p[1]);
    if (!p[2]) break; i += char_to_hex(p[2]);
    memcpy(q1, s1, n = p - s1); q1[n] = i;
  }
  memcpy(q1, s1, strlen(s1) + 1);

  /* Verify that the query is a valid SQL SELECT statement. */
  if (!(n = validate_query(q1 = jb_trim(q)))) return finalize(r, q, STR_QUERY);
  if (q1[n - 1] != ';') { q1[n] = ';'; q1[++n] = '\0'; }

  /* Execute the command line (which should invoke a database utility), creating a pipe between this
   * process and the child process.  The writable end of the pipe is associated with the resulting
   * stream, and the readable end is associated with the spawned command's standard input.
   */
#ifdef _WIN32
  f = _popen(r, "w");
#else
  f = popen(r, "w");
#endif
  if (!f) return finalize(r, q, strerror(errno));

  /* SQL*Plus is the only database utility that outputs the entire <html> element,
   * so if the database utility is not SQL*Plus, output the beginning of the HTML.
   */
  if (n = strncmp(r, STR_SQLPLUS, strlen(STR_SQLPLUS)))
  {
    /* The single-quoted strings in this macro are escaped (necessarily) for SQL*Plus, so unescape them here. */
    if (!(s = malloc(strlen(p = HTML_RESULTS)))) return finalize(r, q, strerror(errno));
    for (s1 = s; *p; ++s1) { *s1 = *p; if (*++p == '\'' && *s1 == '\'') ++p; } *s1 = '\0';

    /* Output the beginning of the HTML, and free memory allocated memory for the unescaped string. */
    output_begin(STR_RESULTS); fputs(s, stdout); output_bridge(" onload='init()'"); free(s);

    /* psql outputs the <table> tags, whereas the others (SQLite and SpatiaLite)
     * don't, so if the database utility is not psql, output the <table> start-tag.
     */
    if (i = strncmp(r, STR_PSQL, strlen(STR_PSQL))) puts("<table>");
  }

  /* Standard output is usually buffered, so ensure that the HTML
   * of this process is output before that of the child process.
   */
  fflush(stdout);

  /* Write the query to the spawned command's standard input via the stream (the writable end
   * of the pipe).  Note that the spawned command does not output until the stream is closed.
   */
  fputs(q1, f);
#ifdef _WIN32
  _pclose(f);
#else
  pclose(f);
#endif

  /* Output the <table> end-tag and the ending of the HTML as needed (see above), and we're done. */
  if (n)
  {
    if (i) puts("</table>");
    output_end();
  }
  return finalize(r, q, NULL);
}

#ifdef _WIN32

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Find the first case-insensitive occurrence of a substring within a string.
 *   haystack:  string to search
 *   needle:  substring to find
 * Return Value:  A pointer to the first occurrence of the substring, or NULL if the substring is not found.
 * (On Linux, this function is defined in string.h.  There is no Win32 equivalent.)
 */
char * strcasestr(const char * haystack, const char * needle)
{
  size_t n = strlen(needle);
  const char * p, * q = haystack + strlen(haystack) - n;

  for (p = haystack; p <= q; ++p) if (!_strnicmp(p, needle, n)) return (char *)p;
  return NULL;
}

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Read and parse the script file.
 *   argv:  array of command line arguments (same as argv passed to main)
 *   length:  length of query string (determines what string_ptr receives)
 *   stream_ptr:  receives pointer to script file (which, if non-null, should be closed with fclose)
 *   string_ptr:  if length is zero, receives template path; otherwise, receives command line
 *     (memory for this buffer is obtained with malloc, and should be freed with free)
 * Return Value:  NULL on success; otherwise, an error message.
 */
const char * read_file(char * argv[], size_t length, FILE ** stream_ptr, char ** string_ptr)
{
  struct stat st;
  int n, m, k;
  char * s;
  const char * p, * q;

  /* Ensure that string_ptr can be safely passed to free. */
  *string_ptr = NULL;

  /* Open the script file for reading. */
  if (!(*stream_ptr = fopen(argv[1], "r"))) return strerror(errno);

  /* Allocate memory for a buffer to store the data read from the file. */
  if (stat(argv[1], &st) || !(s = malloc(n = st.st_size + 1))) return strerror(errno);

  /* The first line should be the interpreter directive (i.e., the "shebang" line). */
  if (p = read_line(s, n, *stream_ptr)) { free(s); return p; }
  if (strncmp(s, "#!", 2) || strcmp(s + strlen(s) - strlen(argv[0]), argv[0])) { free(s); return STR_FILE; }

  /* The next line should indicate the database engine/utility. */
  if (p = read_line(s, n, *stream_ptr)) { free(s); return p; }
  if (!strcmp(s, "Oracle")) q = STR_SQLPLUS;
  else if (!strcmp(s, "PostgreSQL")) q = STR_PSQL;
  else if (!strcmp(s, "SQLite")) q = STR_SQLITE;
  else if (!strcmp(s, "SpatiaLite")) q = STR_SPATIALITE;
  else { free(s); return STR_DATABASE; }

  /* The next line should comprise the connection information.  If the query string is nonempty (i.e., we're going to run
   * the query and output results), build the command line using the database utility and this connection information.
   */
  if (p = read_line(s, n, *stream_ptr)) { free(s); return p; }
  if (length)
  {
    *string_ptr = malloc((m = strlen(q)) + (k = strlen(s) + 1));
    memcpy(*string_ptr, q, m);
    memcpy(*string_ptr + m, s, k);
  }

  /* The next line can be empty.  If nonempty, it should contain the relative path to a query template (JSON) file.
   * This applies if the query string is empty (i.e., we're going to output a web page to prompt for a query).
   */
  if (p = read_line(s, n, *stream_ptr)) { free(s); return p; }
  if (length) free(s); else *string_ptr = s;

  /* There should be nothing else in the file. */
  return (fgetc(*stream_ptr) == EOF) ? (ferror(*stream_ptr) ? strerror(errno) : NULL) : STR_FILE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Read a line from the script file.
 *   string:  buffer to store data read from file
 *   count:  maximum number of characters to read
 *   stream:  pointer to script file
 * Return Value:  NULL on success; otherwise, an error message.
 */
const char * read_line(char * string, int count, FILE * stream)
{
  int n;

  if (!fgets(string, count, stream)) return errno ? strerror(errno) : STR_FILE;

  /* The last character should be a newline, and it should be trimmed. */
  if (!(n = strlen(string)) || string[--n] != '\n') return STR_FILE;
  string[n] = '\0'; return NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output an HTML document to prompt for a query.
 *   path:  pathname of query template (JSON) file
 */
void output_prompt(const char * path)
{
  char * s;
  const char * p;

  /* Begin HTML output. */
  output_begin(STR_PROMPT); fputs(HTML_PROMPT_1, stdout);

  /* If a query template file was specified, read its contents (which should be JSON). */
  if (strlen(path))
  {
    if (p = read_templates(path, &s)) printf("showMessage('%s: %s', 'red'); ", STR_READING, p);
    else printf("if (!processTemplates('%s')) showMessage('%s', 'orange'); ", s, STR_TEMPLATES);
    free(s);
  }

  /* Continue HTML output. */
  fputs(HTML_PROMPT_2, stdout); output_bridge(" onload='init()'"); fputs(HTML_PROMPT_3, stdout); output_end();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Read the contents of a query template (JSON) file into a character buffer.
 *   path:  file pathname
 *   string_ptr:  receives data read from file
 *     (memory for this buffer is obtained with malloc, and should be freed with free)
 * Return Value:  NULL on success; otherwise, an error message.
 */
const char * read_templates(const char * path, char ** string_ptr)
{
  struct stat st;
  FILE * f;
  char * p;
  int c;

  /* Ensure that string_ptr can be safely passed to free. */
  *string_ptr = NULL;

  /* Allocate memory for the buffer to store the data read from the file. */
  if (stat(path, &st) || !(*string_ptr = malloc(2 * st.st_size + 1))) return strerror(errno);

  /* Open the file for reading. */
  if (!(f = fopen(path, "rb"))) return strerror(errno);

  /* Read the contents of the file into the buffer, excluding line terminators and escaping single quotes. */
  for (p = *string_ptr; (c = fgetc(f)) != EOF;)
    switch (c)
    {
      case '\r': case '\n': continue;
      case '\'': *p = '\\'; ++p;
      default: *p = c; ++p;
    }
  if (ferror(f)) p = strerror(errno); else *p = '\0';
  fclose(f); return *p ? p : NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Determine whether or not a string appears to be a (singular) valid SQL SELECT statement.
 *   string:  string to validate
 * Return Value:  On success, the length of the string; otherwise, zero.
 */
size_t validate_query(const char * string)
{
  size_t n = strlen(string);
  const char * p;

  /* There is a minimum string length (SELECT *). */
  if (n < 8) return 0;

  /* Check for semicolons (one at the end is OK). */
  p = strstr(string, ";");
  if (p && p < string + n - 1) return 0;

  /* If the string begins with SELECT, make sure it does not contain INTO. */
  if (!strncasecmp(string, "SELECT", 6)) return strcasestr(string, "INTO") ? 0 : n;

  /* The only other substring with which the string can begin is WITH, but it
   * must also contain SELECT, and must not contain INSERT, UPDATE, or DELETE.
   */
  if (strncasecmp(string, "WITH", 4)) return 0;
  if (strcasestr(string, "INSERT") || strcasestr(string, "UPDATE") || strcasestr(string, "DELETE")) return 0;
  return strcasestr(string, "SELECT") ? n : 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Free memory as needed, and optionally output (as HTML) an error message.
 *   string:  template path or command line
 *   query:  URL-decoded query (SQL SELECT statement)
 *   error:  error message (if any)
 * Return Value:  Exit status (EXIT_SUCCESS or EXIT_FAILURE).
 */
int finalize(char * string, char * query, const char * error)
{
  /* Free memory as needed. */
  free(string); free(query);

  /* If there is an error message, output it as HTML. */
  if (error) { output_begin(STR_ERROR); output_bridge(""); printf("<h1>%s: %s</h1>", STR_ERROR, error); output_end(); }

  /* Return the appropriate exit status based on whether or not there is an error message. */
  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
