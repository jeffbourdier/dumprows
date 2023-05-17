/* html.h - HTML string constants for DUMPROWS
 *
 * Copyright (c) 2021-3 Jeffrey Paul Bourdier
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


/*************
 * Constants *
 *************/

static const char * HTML_PROMPT_1 =
  "<style>textarea { width: 99%; min-height: 150px; resize: vertical; }</style>"
  "<script>"
    "var select, paragraph, div, button, controls, paragraphs = [], reader = new FileReader(), templates = "
      "[ { title: '' }, "
        "{ title: 'Row Count', "
          "format: 'SELECT COUNT(*) FROM {Table}', "
          "params: ['Table'] "
        "}, "
        "{ title: 'Unique Values', "
          "format: 'SELECT {Column}, COUNT(*) FROM {Table} GROUP BY {Column} ORDER BY 2 DESC', "
          "params: ['Table', 'Column'] "
        "} "
      "]; "
    "function init() "
    "{ select = document.getElementsByTagName('select')[0]; "
      "paragraph = document.getElementById('message'); "
      "div = document.getElementsByTagName('div')[0]; "
      "button = document.getElementsByTagName('button')[0]; "
      "controls = document.forms[0].elements; "
      "for (let n = templates.length, i = 0; i < n; ++i) addTemplate(templates[i]); ";

static const char * HTML_PROMPT_2 =
    "} "
    "function loadTemplates(input) "
    "{ reader.onloadend = function () "
      "{ if (processTemplates(reader.result)) showMessage('Templates loaded successfully.', 'green'); "
        "else showMessage('Template file is invalid.', 'red'); "
      "}; "
      "reader.readAsText(input.files[0]); "
    "} "
    "function processTemplates(text) "
    "{ if (!text) return true; "
      "var a, n, i, p, m, j; "
      "try { a = JSON.parse(text); } "
      "catch (ex) { console.error(ex); return false; } "
      "if (!Array.isArray(a)) return false; "
      "for (n = a.length, i = 0; i < n; ++i) "
      "{ p = a[i]; "
        "if (!p.hasOwnProperty('title') || typeof p.title != 'string') return false; "
        "if (!p.hasOwnProperty('format') || typeof p.format != 'string') return false; "
        "if (!p.hasOwnProperty('params') || !Array.isArray(p.params)) return false; "
        "for (m = p.params.length, j = 0; j < m; ++j) if (typeof p.params[j] != 'string') return false; "
        "templates.push(p); "
        "addTemplate(p); "
      "} "
      "return true; "
    "} "
    "function addTemplate(template) { var p = document.createElement('option'); p.text = template.title; select.add(p); } "
    "function selectTemplate() "
    "{ button.disabled = true; "
      "div.innerHTML = ''; "
      "if (!select.selectedIndex) return; "
      "var p, q, r, m, i, a = templates[select.selectedIndex].params, n = a.length; "
      "if (!n) { button.disabled = false; return; } "
      "for (m = paragraphs.length, i = 0; i < n; ++i) "
      "{ if (m > i) "
        "{ p = paragraphs[i]; "
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
          "{ for (var n = templates[select.selectedIndex].params.length, i = 0; i < n; ++i) "
              "if (!paragraphs[i].firstChild.lastChild.value.length) { button.disabled = true; return; } "
            "button.disabled = false; "
          "}; "
          "q.appendChild(r); "
          "p.appendChild(q); "
          "paragraphs.push(p); "
        "}; "
        "div.appendChild(p); "
      "} "
    "} "
    "function generateQuery() "
    "{ var n, i, p = templates[select.selectedIndex], q = p.format, a = p.params; "
      "for (n = a.length, i = 0; i < n; ++i) "
        "q = q.replace(new RegExp('\\{' + a[i] + '\\}', 'g'), paragraphs[i].firstChild.lastChild.value); "
      "controls[0].value = q; "
      "controls[1].disabled = false; "
    "} "
    "function showMessage(text, color) { paragraph.textContent = text; paragraph.style.color = color; }"
  "</script>";

static const char * HTML_PROMPT_3 =
  "<p>Query Templates: <select oninput='selectTemplate()'></select></p>"
  "<p>Load query templates from file: <input type='file' oninput='loadTemplates(this)' /></p>"
  "<p id='message'></p>"
  "<fieldset>"
    "<legend>Template Parameters</legend>"
    "<div></div>"
    "<button onclick='generateQuery()' disabled>Generate Query</button>"
  "</fieldset>"
  "<form>"
    "<p>Query:<br /><textarea name='q' oninput='controls[1].disabled=!controls[0].value.length'></textarea></p>"
    "<p><input type='submit' disabled /></p>"
  "</form>";


/*********************
 * Macro Definitions *
 *********************/

#define HTML_RESULTS \
  "<style>" \
    "table { margin: auto; border-collapse: collapse; } " \
    "table caption { white-space: nowrap; padding-top: 4px; } " \
    "table, th, td { border: 1px solid; padding: 4px; } " \
    "th { background-color: #DFDFDF; }" \
  "</style>" \
  "<script>" \
    "var leafletLoaded, mappingLoaded; " \
    "function init() { var t = document.querySelector(''table''); setCaption(t); tryMapping(t); } " \
    "function setCaption(t) " \
    "{ var p = t.createCaption(), q, r; " \
      "q = document.createElement(''span''); " \
      "q.style.cssFloat = ''right''; " \
      "q.style.fontSize = ''smaller''; " \
      "q.style.marginLeft = ''4px''; " \
      "q.style.marginRight = ''4px''; " \
      "q.textContent = ''Generated by ''; " \
      "r = document.createElement(''a''); " \
      "r.style.fontVariant = ''small-caps''; " \
      "r.target = ''_blank''; " \
      "r.href = ''https://jeffbourdier.github.io/dumprows''; " \
      "r.textContent = ''DumpRows''; " \
      "q.appendChild(r); " \
      "p.appendChild(q); " \
      "q = document.createElement(''span''); " \
      "q.style.cssFloat = ''left''; " \
      "q.style.marginRight = ''8px''; " \
      "r = document.body.lastElementChild; " \
      "if (r.tagName == ''P'') " \
      "{ q.textContent = r.textContent.trim(); r.remove(); " \
        "r = document.body.firstElementChild; " \
        "if (r.tagName == ''P'') r.remove(); " \
      "} " \
      "else q.textContent = (t.rows.length - 1) + '' rows''; " \
      "p.appendChild(q); " \
    "} " \
    "function tryMapping(t) " \
    "{ for (var p, k, g = -1, c = t.rows[1].cells, n = c.length, i = 0; i < n; ++i) " \
      "{ try { p = JSON.parse(c[i].textContent); } catch { continue; } " \
        "if ((k = Object.keys(p)).length != 2 || k[0] != ''type'' || k[1] != ''coordinates'' " \
          "|| (p.type != ''Point'' && p.type != ''LineString'' && p.type != ''Polygon'' && p.type != ''MultiPolygon'') " \
          "|| !Array.isArray(p.coordinates)) continue; " \
        "if (g > 0) return; " \
        "g = i; " \
      "} " \
      "if (g < 0) return; " \
      "p = document.createElement(''link''); " \
      "p.rel = ''stylesheet''; " \
      "p.type = ''text/css''; " \
      "p.href = ''https://unpkg.com/leaflet@1.7.1/dist/leaflet.css''; " \
      "document.head.insertBefore(p, document.head.lastChild); " \
      "p = document.createElement(''script''); " \
      "p.src = ''https://unpkg.com/leaflet@1.7.1/dist/leaflet.js''; " \
      "p.onload = function () { leafletLoaded = true; if (mappingLoaded) initMapping(t, g); }; " \
      "document.head.insertBefore(p, document.head.lastChild); " \
      "p = document.createElement(''script''); " \
      "p.src = ''https://jeffbourdier.github.io/dumprows/mapping.js''; " \
      "document.head.insertBefore(p, document.head.lastChild); " \
      "p.onload = function () { mappingLoaded = true; if (leafletLoaded) initMapping(t, g); }; " \
    "}" \
  "</script>"


#endif  /* (prevent multiple inclusion) */
