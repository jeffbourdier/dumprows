/* html.c - HTML functions for DUMPROWS
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

#include <stdio.h>    /* printf */
#include <stdlib.h>   /* free, malloc */
#include <string.h>   /* strlen */
#include "geojson.h"  /* (struct) geojson_info, GEOJSON_LINESTRING, GEOJSON_POINT, GEOJSON_POLYGON */
#include "html.h"     /* html_element */
#include "text.h"     /* text_append, text_format */


/*************
 * Functions *
 *************/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Build a formatted string representing an HTML element.
 *   name:  element name
 *   content:  element content
 * Return Value:  Formatted string.  Memory for this buffer is obtained with malloc, and should be freed with free.
 */
char * html_element(const char * name, const char * content)
{
  static const char * format = "<%s>%s</%s>";

  char * p;
  size_t n;

  p = (char *)malloc(n = strlen(format) + 2 * strlen(name) + strlen(content));
  text_format(p, n, format, name, content, name);
  return p;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Build formatted strings representing the content/attribution for an HTML document to contain
 * the <tr> elements resulting from the query, optionally including a map with GeoJSON features.
 *   rows:  <tr> elements (output from database utility)
 *   infos:  array of geojson_info structures
 *   info_count:  number of items in infos
 *   addl_head_ptr:  receives additional <head> element content (style, etc.)
 *   body_attr_ptr:  receives <body> element attribution (e.g., onload)
 *   body_content_ptr:  receives <body> element content
 */
void html_format(char * rows, struct geojson_info * infos, int info_count,
                 char ** addl_head_ptr, const char ** body_attr_ptr, char ** body_content_ptr)
{
  static const char * table_style =
    "table { margin: auto; border-collapse: collapse; } "
    "table caption { font-size: smaller; text-align: right; padding-bottom: 4px; } "
    "table, th, td { border: 1px solid; padding: 4px; } "
    "th { background-color: #DFDFDF; }";
  static const char * feature_format = "{ type: 'Feature', properties: { index: %d }, geometry: %s }";
  static const char * head_format =
    "<style> "
      "#tableDiv { overflow: auto; margin-bottom: 8px; margin-right: 8px; } "
      "%s td.selected { background-color: aqua; } "
      "#mapDiv { border: 2px solid gray; } "
    "</style>"
    "<link rel=\"stylesheet\" type=\"text/css\" href=\"https://unpkg.com/leaflet@1.7.1/dist/leaflet.css\" />"
    "<script src=\"https://unpkg.com/leaflet@1.7.1/dist/leaflet.js\"></script>"
    "<script type=\"text/javascript\"><!--\r\n"
      "var horizontal, usableWidth, usableHeight, halfWidth, halfHeight, tableDiv, mapDiv, map, control, rows, "
        "selectedIndex = 0, selectedGeometry, backPoint, forePoint,  backLine, foreLine, backPolygon, forePolygon, "
        "extents = [[%f, %f], [%f, %f]], geoObject = { type: 'FeatureCollection', features: [ %s ] }; "
      "function init() "
      "{ tableDiv = document.getElementById('tableDiv'); "
        "mapDiv = document.getElementById('mapDiv'); "
        "window.onresize = function () "
          "{ if (window.innerHeight > window.innerWidth) splitHorizontally(); else splitVertically(); }; "
        "window.onresize(); "
        "var n, i, p, q; "
        "map = L.map('mapDiv').fitBounds(extents); "
        "control = L.control(); "
        "control.onAdd = function () "
          "{ var n, i, q, p = L.DomUtil.create('div', 'leaflet-bar'), a = "
            "[ { svg: "
                  "'<rect stroke=\"gray\" fill=\"gray\" x=\"1\" y=\"1\" width=\"2\" height=\"7\" />"
                  "<rect stroke=\"gray\" fill=\"gray\" x=\"5\" y=\"1\" width=\"2\" height=\"7\" />"
                  "<rect stroke=\"gray\" fill=\"gray\" x=\"10\" y=\"10\" width=\"7\" height=\"2\" />"
                  "<rect stroke=\"gray\" fill=\"gray\" x=\"10\" y=\"14\" width=\"7\" height=\"2\" />"
                  "<path stroke=\"black\" fill=\"none\" d=\"M 1,10 l 0,5 7,0 m 0,0 -2,-2 0,4 z M 17,8 l 0,-5 -7,0 m 0,0 2,-2 0,4 z\" />', "
                "title: 'Switch View', "
                "call: 'switchView()' }, "
              "{ svg: "
                  "'<rect stroke=\"gray\" fill=\"#FFD\" x=\"1\" y=\"1\" width=\"16\" height=\"16\" />"
                  "<line stroke-width=\"2\" stroke=\"black\" x1=\"18\" y1=\"18\" x2=\"9\" y2=\"9\" />"
                  "<circle stroke=\"black\" fill=\"silver\" cx=\"9\" cy=\"9\" r=\"4\" />"
                  "<polygon stroke=\"black\" fill=\"black\" points=\"16,2 12,2 16,6\" />"
                  "<polygon stroke=\"black\" fill=\"black\" points=\"2,2 2,6 6,2\" />"
                  "<polygon stroke=\"black\" fill=\"black\" points=\"2,16 6,16 2,12\" />', "
                "title: 'Zoom to Full Extent', "
                "call: 'zoomToFullExtent()' }, "
              "{ svg: "
                  "'<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"8,1 17,1 17,4 11,7\" />"
                  "<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"17,4 17,13 14,13 11,7\" />"
                  "<polygon stroke=\"teal\" fill=\"aqua\" points=\"5,1 8,1 14,13 5,13\" />"
                  "<line stroke-width=\"2\" stroke=\"black\" x1=\"12\" y1=\"18\" x2=\"5\" y2=\"11\" />"
                  "<circle stroke=\"black\" fill=\"silver\" cx=\"5\" cy=\"11\" r=\"4\" />', "
                "title: 'Zoom to Selection', "
                "call: 'zoomToSelection()' }, "
              "{ svg: "
                  "'<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"8,1 17,1 17,4 11,7\" />"
                  "<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"17,4 17,13 14,13 11,7\" />"
                  "<polygon stroke=\"teal\" fill=\"aqua\" points=\"5,1 8,1 14,13 5,13\" />"
                  "<path stroke=\"black\" fill=\"white\" d=\"m 5,19 -4,-4 0,-1 1,-1 1,0 2,2 1,-1 -4,-4 0,-1 1,-1 1,0 0,1 0,-2 1,0 1,1 0,1 0,-1 1,-1 1,1 0,1 1,-1 1,1 2,10 z\" />"
                  "<path stroke=\"white\" fill=\"black\" d=\"m 3,9 4,4 1,0 -3,-5 3,5 1,0 -2,-5 2,5 1,0 -1,-4\" />', "
                "title: 'Pan to Selection', "
                "call: 'panToSelection()' }, "
              "{ svg: "
                  "'<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"5,1 17,1 17,5 9,9\" />"
                  "<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"17,5 17,17 13,17 9,9\" />"
                  "<polygon stroke=\"maroon\" fill=\"#FFCCCC\" points=\"1,1 5,1 13,17 1,17\" />', "
                "title: 'Clear Selection', "
                "call: 'clearSelection()' } ]; "
            "for (n = a.length, i = 0; i < n; ++i)"
            "{ q = L.DomUtil.create('a', null, p); "
              "q.innerHTML = '<svg width=\"18\" height=\"18\">' + a[i].svg + '</svg>'; "
              "q.title = a[i].title; "
              "q.href = 'javascript:' + a[i].call; "
            "}; "
            "return p; "
          "}; "
        "control.addTo(map); "
        "const url = 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', "
          "osm = 'Base data &copy; <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors'; "
        "L.tileLayer(url, { attribution: osm }).addTo(map); "
        "p = L.geoJSON(null, { pointToLayer: stylePoint, onEachFeature: addListener }).addTo(map); "
        "q = L.geoJSON(null, { style: { color: 'maroon', fillColor: 'red' }, onEachFeature: addListener }).addTo(map); "
        "for (n = geoObject.features.length, i = 0; i < n; ++i) "
          "if (geoObject.features[i].geometry.type == 'Point') p.addData(geoObject.features[i]); "
          "else q.addData(geoObject.features[i]); "
        "rows = document.getElementsByTagName('tr'); "
        "backPoint = L.circleMarker(null, { radius: 7, color: 'aqua', fillOpacity: 1 }); "
        "forePoint = L.circleMarker(null, { radius: 5, color: 'teal', fill: false }); "
        "backLine = L.polyline([null], { color: 'aqua', weight: 7 }); "
        "foreLine = L.polyline([null], { color: 'teal' }); "
        "backPolygon = L.polygon([null], { color: 'aqua', fillOpacity: 0.8, weight: 7 }); "
        "forePolygon = L.polygon([null], { color: 'teal', fill: false }); "
      "} "
      "function stylePoint(feature, latLng) "
      "{ return L.circleMarker(latLng, { radius: 5, color: 'maroon', fillColor: 'red', fillOpacity: 1 }); } "
      "function addListener(feature, layer) { layer.on('click', function () { selectFeature(feature.properties.index); }); } "
      "function selectFeature(index) "
      "{ clearSelection(); "
        "selectedGeometry = geoObject.features[(selectedIndex = index) - 1].geometry; "
        "selectRow(true); "
        "switch (selectedGeometry.type) "
        "{ case 'Point': "
            "var latLng = L.GeoJSON.coordsToLatLng(selectedGeometry.coordinates); "
            "backPoint.setLatLng(latLng).addTo(map); "
            "forePoint.setLatLng(latLng).addTo(map); "
            "break; "
          "case 'LineString': "
            "var latLngs = L.GeoJSON.coordsToLatLngs(selectedGeometry.coordinates); "
            "backLine.setLatLngs(latLngs).addTo(map); "
            "foreLine.setLatLngs(latLngs).addTo(map); "
            "break; "
          "case 'Polygon': case 'MultiPolygon': "
            "var latLngs = L.GeoJSON.coordsToLatLngs(selectedGeometry.coordinates, "
              "(selectedGeometry.type == 'MultiPolygon') ? 2 : 1); "
            "backPolygon.setLatLngs(latLngs).addTo(map); "
            "forePolygon.setLatLngs(latLngs).addTo(map); "
            "break; "
        "} "
      "} "
      "function switchView() { if (horizontal) splitVertically(); else splitHorizontally(); } "
      "function splitHorizontally() "
      "{ orientView(true); "
        "tableDiv.style.float = ''; "
        "tableDiv.style.maxWidth = usableWidth + 'px'; "
        "tableDiv.style.maxHeight = halfHeight + 'px'; "
        "mapDiv.style.height = Math.max(halfHeight, usableHeight - tableDiv.clientHeight) + 'px'; "
      "} "
      "function splitVertically() "
      "{ orientView(false); "
        "tableDiv.style.float = 'left'; "
        "tableDiv.style.maxWidth = halfWidth + 'px'; "
        "tableDiv.style.maxHeight = usableHeight + 'px'; "
        "mapDiv.style.height = usableHeight + 'px'; "
      "} "
      "function orientView(horizontally) "
      "{ const space = 20; "
        "halfWidth = (usableWidth = window.innerWidth - space) / 2; "
        "halfHeight = (usableHeight = window.innerHeight - space - ((horizontal = horizontally) ? 8 : 0)) / 2; "
      "} "
      "function zoomToFullExtent() { map.flyToBounds(extents); } "
      "function zoomToSelection() "
      "{ if (selectedIndex < 1) return; "
        "var bounds; "
        "switch (selectedGeometry.type) "
        "{ case 'Point': var latLng = forePoint.getLatLng(); bounds = L.latLngBounds(latLng, latLng); break; "
          "case 'LineString': bounds = foreLine.getBounds(); break; "
          "case 'Polygon': case 'MultiPolygon': bounds = forePolygon.getBounds(); break; "
          "default: return; "
        "} "
        "map.flyToBounds(bounds); "
      "} "
      "function panToSelection() "
      "{ if (selectedIndex < 1) return; "
        "var latLng; "
        "switch (selectedGeometry.type) "
        "{ case 'Point': latLng = forePoint.getLatLng(); break; "
          "case 'LineString': latLng = foreLine.getCenter(); break; "
          "case 'Polygon': case 'MultiPolygon': latLng = forePolygon.getCenter(); break; "
          "default: return; "
        "} "
        "map.panTo(latLng); "
      "} "
      "function clearSelection() "
      "{ if (selectedIndex < 1) return; "
        "forePolygon.remove(); foreLine.remove(); forePoint.remove(); "
        "backPolygon.remove(); backLine.remove(); backPoint.remove(); "
        "selectRow(false); selectedIndex = 0; "
      "} "
      "function selectRow(selecting) "
      "{ for (var s = selecting ? 'selected' : '', r = rows[selectedIndex], n = r.childNodes.length, i = 0; i < n; ++i) "
          "r.childNodes[i].className = s; "
      "}\r\n"
      "//-->\r\n"
    "</script>";
  static const char * body_attr = "onload=\"init()\"";
  static const char * point_text     = "&bull;&nbsp;Point";
  static const char * linestring_text = "&acd;&nbsp;LineString";
  static const char * polygon_text   = "&rect;&nbsp;Polygon";
  static const char * anchor_format = "<a href=\"javascript:selectFeature(%d)\">%s</a>";
  static const char * body_format = "<div id=\"tableDiv\"><table><caption>Generated by <a style=\"font-variant: small-caps\" target=\"_blank\" href=\"https://jeffbourdier.github.io/dumprows\">DumpRows</a></caption>%s</table></div><div id=\"mapDiv\"></div>";

  char * p, * q;
  size_t n, m, k;
  const char * r;
  int i, j;
  double min_lat = 90, min_lng = 180, max_lat = -90, max_lng = -180;

  /* If there's no geometry, we can make this quick & easy. */
  if (!info_count)
  {
    *addl_head_ptr = html_element("style", table_style);
    *body_attr_ptr = NULL;
    *body_content_ptr = html_element("table", rows);
    return;
  }

  /* The <tr> elements include a GeoJSON geometry column, so show the features on a map. */
  p = (char *)malloc(n = strlen(rows));
  q = (char *)malloc(m = n + info_count * (strlen(anchor_format) + 32));

  /* Iterate through each pointer, processing as necessary to build the formatted strings. */
  for (r = rows, *q = '\0', i = k = 0; i < info_count; ++i)
  {
    /* Determine the full bounds/extents (for the additional <head> element content). */
    if (infos[i].min_y < min_lat) min_lat = infos[i].min_y;
    if (infos[i].min_x < min_lng) min_lng = infos[i].min_x;
    if (infos[i].max_y > max_lat) max_lat = infos[i].max_y;
    if (infos[i].max_x > max_lng) max_lng = infos[i].max_x;

    /* Populate GeoJSON features (for the additional <head> element content). */
    if (i) { p[k = strlen(p)] = ','; ++k; r = infos[i - 1].end; }
    text_format(p + k, n - k, feature_format, j = i + 1, infos[i].text);
    free(infos[i].text);

    /* The original GeoJSON text in the geometry column will be replaced
     * with a feature selection link (for the <body> element content).
     */
    text_append(q, m, r, infos[i].begin - r);
    k = strlen(q);
    switch (infos[i].type)
    {
    case GEOJSON_POINT:      r = point_text;      break;
    case GEOJSON_LINESTRING: r = linestring_text; break;
    case GEOJSON_POLYGON:    r = polygon_text;    break;
    }
    text_format(q + k, m - k, anchor_format, j, r);
  }
  /* Get the tail end of the <body> element content. */
  r = infos[i - 1].end;
  text_append(q, m, r, strlen(r));

  /* Build the additional <head> element content (including full bounds/extents and GeoJSON features). */
  *addl_head_ptr = (char *)malloc(n += strlen(head_format) + strlen(table_style) + 64);
  text_format(*addl_head_ptr, n, head_format, table_style, min_lat, min_lng, max_lat, max_lng, p);

  /* The <body> element attribution is straightforward. */
  *body_attr_ptr = body_attr;

  /* Build the <body> element content (replacing GeoJSON geometry with feature selection link). */
  *body_content_ptr = (char *)malloc(m += strlen(body_format));
  text_format(*body_content_ptr, m, body_format, q);
  free(p); free(q);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Output an HTML document with the given content/attribution.
 *   title:  <title> element content
 *   addl_head:  additional <head> element content (style, etc.)
 *   body_attr:  <body> element attribution (e.g., onload)
 *   body_content:  <body> element content
 */
void html_output(const char * title, const char * addl_head, const char * body_attr, const char * body_content)
{
  static const char * format =
    "Content-Type: text/html\r\n\r\n"
    "<!DOCTYPE html>"
    "<html lang=\"en-US\">"
      "<head>"
        "<meta charset=\"UTF-8\" />"
        "<title>%s</title>"
        "%s"
      "</head>"
      "<body%s%s>%s</body>"
    "</html>";

  printf(format, title, (addl_head ? addl_head : ""), (body_attr ? " " : ""), (body_attr ? body_attr : ""), body_content);
}
