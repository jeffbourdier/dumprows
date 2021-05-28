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

#include <sys/stat.h>  /* stat, (struct) stat */
#include <ctype.h>     /* isdigit, isupper */
#include <limits.h>    /* INT_MIN */
#ifndef _WIN32
#  include <libgen.h>  /* basename */
#  include <stdint.h>  /* uintmax_t */
#endif
#include <stdio.h>     /* fclose, FILE, fprintf, fputs, remove */
#include <stdlib.h>    /* EXIT_FAILURE, EXIT_SUCCESS, free, getenv/_dupenv_s, malloc, realloc, system */
#include <string.h>    /* memcpy, strcat_s, strchr, strlen, strncmp */
#include <time.h>      /* localtime/localtime_s, strftime, time, (struct) tm */
#include "geojson.h"   /* (struct) geojson_info, geojson_parse */
#include "html.h"      /* html_element, html_format, html_output */
#include "jb.h"        /* (struct) jb_command_option, jb_command_parse, jb_exe_strip,
                          jb_file_open, jb_file_write, JB_PATH_MAX_LENGTH, jb_trim */
#include "text.h"      /* text_compare, text_find, text_format, text_search */


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
static const char * STR_INVALID_QUERY = "query is not a valid SQL SELECT statement";
static const char * STR_TMP_PATH = "temporary path could not be determined";
static const char * STR_FILE_WRITTEN = "temporary file could not be written";
static const char * STR_FILE_READ = "temporary file could not be read";
static const char * STR_DB_UTILITY = "database utility could not be executed";


/*********************
 * Macro Definitions *
 *********************/

#define char_to_hex(c) (c - (isdigit(c) ? '0' : ((isupper(c) ? 'A' : 'a') - 0xA)))

/* On Win32, getenv is considered "unsafe" and results in error C4996.  Corresponding function _dupenv_s is used instead. */
#ifdef _WIN32
#  define get_environment_variable(value_ptr, name) _dupenv_s(value_ptr, NULL, name)
#else
#  define get_environment_variable(value_ptr, name) ((*value_ptr = getenv(name)) ? 0 : -1)
#endif


/*********************************
 * Private Function Declarations *
 *********************************/

void format_prompt(char ** addl_head_ptr, const char ** body_attr_ptr, const char ** body_content_ptr);
int validate_query(const char * s);
void format_results(char * output, char ** addl_head_ptr, const char ** body_attr_ptr, char ** body_content_ptr);
int finalize(char * name, time_t t, char * remote, char * query, const char * error);
void log_message(char * name, time_t t, char * remote, char * query, const char * error);


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
  char * v, * r, * p, * q, * p0, * q0, s[JB_PATH_MAX_LENGTH], s0[JB_PATH_MAX_LENGTH], s1[JB_PATH_MAX_LENGTH];
  time_t t = time(NULL);
  const char * p1, * q1;
  struct stat st;

  /* Verify usage. */
  n = sizeof(options) / sizeof(struct jb_command_option);
  n = jb_command_parse(argc, argv, STR_USAGE, STR_HELP, options, n, 1);
  if (n < 0) return (n == INT_MIN) ? EXIT_SUCCESS : EXIT_FAILURE;
  v = options[0].is_present ? argv[0] : NULL;

  /* Retrieve the following CGI environment variables:
   *   - REMOTE_ADDR, which is used in logging and temporary file naming
   *   - QUERY_STRING, which should contain an SQL SELECT statement
   */
  if (get_environment_variable(&r, "REMOTE_ADDR") || !r) return finalize(v, t, NULL, NULL, STR_REMOTE_ADDR);
  if (get_environment_variable(&p, "QUERY_STRING") || !p) return finalize(v, t, r, NULL, STR_QUERY_STRING);

  /* If the query string is empty, output a web page to prompt for a query. */
  if (!strlen(p))
  {
#ifdef _WIN32
    free(p);
#endif
    format_prompt(&q0, &p1, &q1);
    html_output("prompt", q0, p1, q1);
    free(q0);
    return finalize(v, t, r, NULL, NULL);
  }

  /* The query string must begin with "query=". */
  if (strncmp(p, "query=", 6))
  {
#ifdef _WIN32
    free(p);
#endif
    return finalize(v, t, r, NULL, STR_INVALID_QUERY);
  }

  /* Replace plus signs in the query string with spaces. */
  for (p0 = p + 6; *p0; ++p0) if (*p0 == '+') *p0 = ' ';

  /* URL-decode the query. */
  for (q0 = q = (char *)malloc(strlen(p0 = p + 6) + 2); p1 = strchr(p0, '%'); p0 += n + 3, q0 += n + 1)
  {
    if (!p1[1]) break; i = 0x10 * char_to_hex(p1[1]);
    if (!p1[2]) break; i += char_to_hex(p1[2]);
    memcpy(q0, p0, n = p1 - p0);
    q0[n] = i;
  }
  memcpy(q0, p0, strlen(p0) + 1);
#ifdef _WIN32
  free(p);
#endif

  /* Verify that the query is a valid SQL SELECT statement. */
  if (!(n = validate_query(q0 = jb_trim(q)))) return finalize(v, t, r, q, STR_INVALID_QUERY);
  if (q0[n - 1] != ';') { q0[n] = ';'; q0[++n] = '\0'; }

  /* Write the query to a temporary file.  This will serve as input
   * to the database utility command line, to be executed shortly.
   */
#ifdef _WIN32
  /* For some very mysterious reason, if r is passed to text_format,
   * Win32 treats it as NULL.  The call to strcat_s is to circumvent this.
   */
  if (get_environment_variable(&p, "TMP") || !p) return finalize(v, t, r, q, STR_TMP_PATH);
  text_format(s, JB_PATH_MAX_LENGTH, "%s\\%d_", p, t);
  free(p);
  strcat_s(s, JB_PATH_MAX_LENGTH, r);
#else
  text_format(s, JB_PATH_MAX_LENGTH, "/tmp/%ju_%s", (uintmax_t)t, r);
#endif
  text_format(s0, JB_PATH_MAX_LENGTH, "%s.%s", s, "sql");
  if (jb_file_write(s0, q0, n)) return finalize(v, t, r, q, STR_FILE_WRITTEN);

  /* Execute the command line (which should invoke a database utility).  Note that input (SQL) is redirected
   * from the temporary file just written, and output (HTML) is redirected to another temporary file.
   */
  text_format(s1, JB_PATH_MAX_LENGTH, "%s.%s", s, "html");
  p = (char *)malloc(n = strlen(format) + strlen(p1 = argv[argc - 1]) + strlen(s0) + strlen(s1));
  text_format(p, n, format, p1, s0, s1);
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
  format_results(p, &q0, &p1, &p0);
  html_output("results", q0, p1, p0);
  free(p); free(q0); free(p0);
  return finalize(v, t, r, q, NULL);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Build formatted strings representing the content/attribution for an HTML document to prompt for a query.
 *   addl_head_ptr:  receives additional <head> element content (style, etc.)
 *   body_attr_ptr:  receives <body> element attribution (e.g., onload)
 *   body_content_ptr:  receives <body> element content
 */
void format_prompt(char ** addl_head_ptr, const char ** body_attr_ptr, const char ** body_content_ptr)
{
  static const char * head_format =
    "<style>textarea { width: 99%%; min-height: 150px; resize: vertical; }</style>"
    "<script type=\"text/javascript\"><!--\r\n"
      "var selectElement, messageP, divElement, buttonElement, controls, "
        "pElements = [], fileReader = new FileReader(), templates = "
        "[ { title: '' }, "
          "{ title: 'Row Count', "
            "format: 'SELECT COUNT(*) FROM {Table}', "
            "parameters: ['Table'] }, "
          "{ title: 'Unique Values', "
            "format: 'SELECT {Column}, COUNT(*) FROM {Table} GROUP BY {Column} ORDER BY 2 DESC', "
            "parameters: ['Table', 'Column'] }, %s ]; "
      "function init() "
      "{ selectElement = document.getElementsByTagName('select')[0]; "
        "messageP = document.getElementById('messageP'); "
        "divElement = document.getElementsByTagName('div')[0]; "
        "buttonElement = document.getElementsByTagName('button')[0]; "
        "controls = document.forms[0].elements; "
        "for (var n = templates.length, i = 0; i < n; ++i) addTemplate(templates[i]); "
      "} "
      "function loadTemplates(fileInput) "
      "{ fileReader.onloadend = function () "
        "{ if (processTemplates(fileReader.result)) "
          "{ messageP.textContent = 'Templates loaded successfully.'; "
            "messageP.style.color = 'green'; "
          "} "
          "else "
          "{ messageP.textContent = 'Template file is invalid.'; "
            "messageP.style.color = 'red'; "
          "} "
        "}; "
        "fileReader.readAsText(fileInput.files[0]); "
      "} "
      "function processTemplates(text) "
      "{ var a, n, i, p, m, j; "
        "try { a = JSON.parse(text); } "
        "catch (ex) { console.error(ex); return false; } "
        "if (!Array.isArray(a)) return false; "
        "for (n = a.length, i = 0; i < n; ++i) "
        "{ p = a[i]; "
          "if (!p.hasOwnProperty('title') || typeof p.title != 'string') return false; "
          "if (!p.hasOwnProperty('format') || typeof p.format != 'string') return false; "
          "if (!p.hasOwnProperty('parameters') || !Array.isArray(p.parameters)) return false; "
          "for (m = p.parameters.length, j = 0; j < m; ++j) if (typeof p.parameters[j] != 'string') return false; "
          "templates.push(p); "
          "addTemplate(p); "
        "} "
        "return true; "
      "} "
      "function addTemplate(template) "
      "{ var p = document.createElement('option'); "
        "p.text = template.title; "
        "selectElement.add(p); "
      "} "
      "function selectTemplate() "
      "{ buttonElement.disabled = true; "
        "divElement.innerHTML = ''; "
        "if (!selectElement.selectedIndex) return; "
        "var p, q, r, m, i, a = templates[selectElement.selectedIndex].parameters, n = a.length; "
        "if (!n) { buttonElement.disabled = false; return; } "
        "for (m = pElements.length, i = 0; i < n; ++i) "
        "{ if (m > i) "
          "{ p = pElements[i]; "
            "q = p.firstChild; "
            "q.lastChild.value = ''; "
            "q.firstChild.textContent = a[i] + ': '; "
          "} "
          "else "
          "{ p = document.createElement('p'); "
            "q = document.createElement('label'); "
            "q.textContent = a[i] + ': '; "
            "r = document.createElement('input'); "
            "r.type = 'text'; "
            "r.oninput = function (e) "
            "{ for (var n = templates[selectElement.selectedIndex].parameters.length, i = 0; i < n; ++i) "
                "if (!pElements[i].firstChild.lastChild.value.length) { buttonElement.disabled = true; return; } "
              "buttonElement.disabled = false; "
            "}; "
            "q.appendChild(r); "
            "p.appendChild(q); "
            "pElements.push(p); "
          "}; "
          "divElement.appendChild(p); "
        "} "
      "} "
      "function generateQuery() "
      "{ var n, i, p = templates[selectElement.selectedIndex], q = p.format, a = p.parameters; "
        "for (n = a.length, i = 0; i < n; ++i) "
          "q = q.replace(new RegExp('\\{' + a[i] + '\\}', 'g'), pElements[i].firstChild.lastChild.value); "
        "controls[0].value = q; "
        "controls[1].disabled = false; "
      "}\r\n"
      "//-->\r\n"
    "</script>";
  static const char * body_attr = "onload=\"init()\"";
  static const char * body_content =
    "<p>Query Templates: <select oninput=\"selectTemplate()\"></select></p>"
    "<p>Load query templates from file: <input type=\"file\" oninput=\"loadTemplates(this)\" /></p>"
    "<p id=\"messageP\"></p>"
    "<fieldset>"
      "<legend>Template Parameters</legend>"
      "<div></div>"
      "<button onclick=\"generateQuery()\" disabled>Generate Query</button>"
    "</fieldset>"
    "<form>"
      "<p>Query:<br /><textarea name=\"query\" oninput=\"controls[1].disabled=!controls[0].value.length\"></textarea></p>"
      "<p><input type=\"submit\" disabled /></p>"
    "</form>";

  size_t n;

  /* Build the additional <head> element content (including any query templates found on the server). */
  *addl_head_ptr = (char *)malloc(n = strlen(head_format));
  text_format(*addl_head_ptr, n, head_format, "");

  /* The <body> element attribution and content are straightforward. */
  *body_attr_ptr = body_attr;
  *body_content_ptr = body_content;
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
void format_results(char * output, char ** addl_head_ptr, const char ** body_attr_ptr, char ** body_content_ptr)
{
  static const size_t k = sizeof(struct geojson_info);

  int n = -1, m, g = -1;
  char * p, * q, * r;
  struct geojson_info info, * infos = NULL;

  /* Every database utility's output is different (e.g., some include <HTML>/<BODY>/<TABLE> tags, others don't).
   * The common denominator is the <TR> tags, which should always be present if the query was successful and rows were
   * returned.  If the output is empty, it probably means no rows were returned (e.g., SQLite/SpatiaLite does this).
   */
  if (!strlen(output))
  {
    *body_attr_ptr = *addl_head_ptr = NULL;
    *body_content_ptr = html_element("h1", "No results.");
    return;
  }

  /* If the output is not HTML, it probably means there was an error
   * (syntax or otherwise) with the query (table/view not found, etc.).
   */
  if (output[0] != '<')
  {
    *body_attr_ptr = *addl_head_ptr = NULL;
    *body_content_ptr = html_element("pre", output);
    return;
  }

  /* If no <tr> tag is found, it could mean a query error or no rows were returned (as
   * with SQL*Plus).  In this case, just use the content of the <body> element as-is.
   */
  if (!(p = text_find(output, "<tr>")))
  {
    *body_attr_ptr = *addl_head_ptr = NULL;
    p = text_find(output, "<body>") + 6;
    *(q = text_find(p, "</body>")) = '\0';
    *body_content_ptr = (char *)malloc(n = (q - p) + 1);
    p = jb_trim(p);
    memcpy(*body_content_ptr, p, strlen(p) + 1);
    return;
  }

  /* Iterate through each row, looking for a GeoJSON geometry column. */
  for (r = q = p; r = text_find(q, "</tr>"); q = r + 5)
  {
    /* Increment the row count.  If this is the header row, or we already
     * know that there's no geometry column, go on to the next row.
     */
    ++n; if (!n || !g) continue;

    /* Iterate through each cell, looking for a consistent geometry column. */
    for (m = 1; (q = text_find(q, "<td")) && q < r; ++m)
    {
      /* Advance the content pointer to the character after the <td> tag.  (There may be attributes in this tag.) */
      while (*q++ != '>');

      /* Determine whether or not this cell contains valid GeoJSON geometry. */
      if (g > 0)
      {
        if (m < g) continue;
        if (m > g || geojson_parse(q, &info)) { g = 0; break; }
      }
      else
      {
        if (geojson_parse(q, &info)) continue;
        g = m;
      }

      /* A GeoJSON geometry column was found, so add its information to the list. */
      infos = (struct geojson_info *)realloc(infos, n * k);
      infos[n - 1] = info;
    }

    /* If no geometry column was found past the header row, don't bother continuing to look. */
    if (n && g < 0) g = 0;
  }
  *q = '\0';
  html_format(p, infos, g ? n : 0, addl_head_ptr, body_attr_ptr, body_content_ptr);
  free(infos);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output (as HTML) an error message, and/or write a message to the log file.
 *   name:  pathname of executable file if logging; otherwise, NULL
 *   t:  value of time (in seconds since Epoch)
 *   remote:  IP address of remote host
 *   query:  URL-decoded query (SQL SELECT statement)
 *   error:  error message (if any)
 * Return Value:  Exit status (EXIT_SUCCESS or EXIT_FAILURE).
 */
int finalize(char * name, time_t t, char * remote, char * query, const char * error)
{
  static const char * format = "<h1>Error: %s</h1>";

  char * p;
  size_t n;

  /* If there is an error message, output it as HTML. */
  if (error)
  {
    p = (char *)malloc(n = strlen(format) + strlen(error));
    text_format(p, n, format, error);
    html_output("error", NULL, NULL, p);
    free(p);
  }

  /* If specified, write a message to the log file. */
  if (name) log_message(name, t, remote, query, error);

  /* Free memory as needed. */
#ifdef _WIN32
  free(remote);
#endif
  free(query);

  /* Return the appropriate exit status based on whether or not there is an error message. */
  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Write a message to the log file.
 *   name:  pathname of executable file
 *   t:  value of time (in seconds since Epoch)
 *   remote:  IP address of remote host
 *   query:  URL-decoded query (SQL SELECT statement)
 *   error:  error message (if any)
 */
void log_message(char * name, time_t t, char * remote, char * query, const char * error)
{
#ifdef _WIN32
  static const char * r = "C:\\ProgramData\\";
#else
  static const char * r = "/var/log/";
#endif
  static const char * h = "------------------------------------------------------------------------------\n";

  char * p, s[JB_PATH_MAX_LENGTH], ts[20];
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
  text_format(s, JB_PATH_MAX_LENGTH, "%s%s.log", r, p);

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

  /* Retrieve the CGI environment variable SCRIPT_NAME, which identifies the "source." */
  if (get_environment_variable(&p, "SCRIPT_NAME")) p = NULL;

  /* Append the message to the log file.
   * (Note that on Win32, by default, a file is opened in text mode, so "\n" is translated to "\r\n" on output.)
   */
  jb_file_open(&f, s, "a");
  if (!f) return;
  fputs(h, f);
  fprintf(f, "Timestamp:  %s\n", ts);
  if (p) fprintf(f, "Script Name:  %s\n", p);
#ifdef _WIN32
  free(p);
#endif
  if (remote) fprintf(f, "Remote IP Address:  %s\n", remote);
  if (error) fprintf(f, "Error:  %s\n", error);
  if (query) fprintf(f, "Query:\n%s\n", query);
  fputs(h, f);
  fclose(f);
}
