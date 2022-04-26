//
// Created by User on 12.11.2021.
//

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../../include/list/dump.h"

typedef ListItem  ListItem;
typedef list_index_t index_t;

static FILE   *stream  = NULL;
static size_t  n_dumps = 0;

static inline bool is_in_range(const index_t capacity, const index_t index)
{
   return 0 <= index && index <= capacity - 1;
}

static void dump_open()
{
   if (stream == NULL)
   {
      stream = fopen(LIST_DUMP_PATH, "w");
      assert(stream);
   
      fprintf(stream,
              "<!doctype html>                                                             \n"
              "<html>                                                                      \n"
              "\t<head lang='en'>                                                          \n"
              "\t\t<meta charset='utf-8'>                                                  \n"
              "\t\t<meta name='viewport' content='width=device-width'>                     \n"
              "\t\t<title>List dump</title>                                                \n"
              "\t\t<style>                                                                 \n"
              "\t\tbody {                                                                  \n"
              "\t\t\tbackground: #222222;                                                  \n"
              "\t\t\tcolor:      #DDDDDD;                                                  \n"
              "\t\t\tfont-family: Arial, sans-serif;                                       \n"
              "\t\t}                                                                       \n"
              "\t\t.list {                                                                 \n"
              "\t\t\twidth: max-content;                                                   \n"
              "\t\t\tborder: 3px solid #AAAAAA;                                            \n"
              "\t\t\tfont-size: 20px;                                                      \n"
              "\t\t\tpadding: 10px 20px;                                                   \n"
              "\t\t}                                                                       \n"
              "\t\t.img {                                                                  \n"
              "\t\t\toverflow-x: auto;                                                     \n"
              "\t\t}                                                                       \n"
              "\t\t</style>                                                                \n"
              "\t</head>                                                                   \n"
              "\t<body>                                                                    \n");
   }
}

static void dump_close()
{
   fprintf(stream,
           "\t</body>\n"
           "</html>\n");
   
   assert(fclose(stream) == 0);
}

void list_dump_init()
{
   dump_open();
   atexit(dump_close);
}

void list_dump(const List *const p_list)
{
   assert(p_list);
   
   const ListNode *const nodes             = p_list->nodes;
   FILE           *const dotfile           = fopen(LIST_DOTFILE_PATH, "w");
   
   assert(dotfile);
   
   fprintf(dotfile,
               "digraph G                                                                                      \n"
               "{                                                                                              \n"
               "\tbgcolor  = \"#222222\"                                                                       \n"
               "\tcompound = true                                                                              \n"
               "\tnewrank  = true                                                                              \n"
               "\tgraph[dpi = 250, splines = polyline, nodesep = 0.3, ranksep = 1, rankdir = LR]               \n"
               "\tnode[shape = box, style = \"filled\", penwidth = 2, color = \"#333333\", margin = 0.04,      \n"
               "\tfontcolor = \"white\", fontsize = 8, fillcolor = \"#666666\", fontname = \"Consolas\"]       \n"
               "\tedge[arrowsize = 0.7, style = solid, penwidth = 1.1]                                         \n"
               "\t0");
   
   for (list_index_t i = 1; i < p_list->capacity; i++)
      fprintf(dotfile, " -> %lld", i);
   
   fprintf(dotfile, "[style = invis, weight = 0, minlen = \"1\"]\n");
   
   for (list_index_t i = 0; i < p_list->capacity; i++)
   {
      fprintf(dotfile,
              "\tsubgraph cluster%lld                                                                              \n"
              "\t{                                                                                                 \n"
              "\t\tmargin    = 0                                                                                   \n"
              "\t\tfontcolor = \"white\"                                                                           \n"
              "\t\tfontname  = \"Consolas\"                                                                        \n"
              "\t\tbgcolor   = \"#333333\"                                                                         \n"
              "\t\tcolor     = \"#333333\"                                                                         \n"
              "\t\tpenwidth  = 2                                                                                   \n"
              "\t\tfontsize  = 8                                                                                   \n"
              "\t\tlabel     = %lld                                                                                \n"
              "\t\t%lld[shape = record, label = \"next: %lld | <p>prev: %lld\"]\n"
              "\t}                                                                                                 \n",
              i, i, i, nodes[i].next, nodes[i].prev);
   
      if (is_in_range(p_list->capacity, nodes[i].next))
      {
         if (nodes[i].prev >= 0)
            fprintf(dotfile,
                    "\t%lld:n -> %lld:n[color = \"#87CEFA 99\"] \n",
                    i, nodes[i].next);
         else if (nodes[i].prev == -1)
            fprintf(dotfile,
                    "\t%lld:n -> %lld:n[color = \"#DDDDDD 99\", fontcolor = \"white\"]",
                    i, nodes[i].next);
      }
      else
         fprintf(dotfile,
                 "\tTRASH[color = \"#666666\", fillcolor = \"#333333\"]     \n"
                 "\t%lld:n -> TRASH[color = \"#FFFF00 99\", style = dashed] \n",
                 i);
   
   
      if (is_in_range(p_list->capacity, nodes[i].prev))
         fprintf(dotfile,
                 "\t%lld:p -> %lld:p[color = \"#DC143C 99\"] \n",
                 i, nodes[i].prev);
      else if (nodes[i].prev != -1)
         fprintf(dotfile,
                 "\tTRASH[color = \"#666666\", fillcolor = \"#333333\"]    \n"
                 "\t%lld:p -> TRASH[color = \"#FFFF00 99\", style = dashed]\n",
                 i);
   }
   
   fprintf(dotfile, "}\n");
   assert(fclose(dotfile) == 0);
   
   char png_path[64]    = "dump_";
   char system_cmd[128] = "dot ";
   
   sprintf(png_path,  "%zu.png", n_dumps);
   strcat(system_cmd, LIST_DOTFILE_PATH);
   strcat(system_cmd, " -Tpng -o ");
   strcat(system_cmd, png_path);
   
   system(system_cmd);
   
   fprintf(stream,
           "\t\t<hr width = '100%%' >                                     \n"
           "\t\t<div class = 'list'>                                     \n"
           "\t\t\t<pre>List address: %p</pre>                            \n"
           #ifdef    LIST_LOGIC_INDEX
           "\t\t\t<pre>Shift:        %lld</pre>                          \n"
           #endif // LIST_LOGIC_INDEX
           "\t\t\t<pre>Capacity:     %lld</pre>                          \n"
           "\t\t\t<pre>Iterator:     %lld</pre>                          \n"
           "\t\t\t<pre>Size:         %lld</pre>                          \n"
           "\t\t\t<pre>Free:         %lld</pre>                          \n"
           "\t\t\t<pre>NODS:         %p</pre>                            \n"
           "\t\t</div>                                                   \n"
           "\t\t<div class = 'img'>                                      \n"
           "\t\t\t<img src = \"%s\" height = 500px>                      \n"
           "\t\t</div>                                                   \n",
           
           p_list,
           #ifdef    LIST_LOGIC_INDEX
           p_list->shift,
           #endif // LIST_LOGIC_INDEX
           p_list->capacity,
           p_list->iterator,
           p_list->size,
           p_list->free,
           p_list->nodes,
           png_path);
   
   n_dumps += 1;
}