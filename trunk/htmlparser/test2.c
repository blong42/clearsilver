// Author: falmeida@google.com (Filipe Almeida)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG

#include "htmlparser.h"

#define MARKER "<?state?>"

int main(int argc, char **argv) 
{
    htmlparser_ctx *html = htmlparser_new();
    char *buffer = malloc(262144);
    int i = 0;
    int cnt;
    char *start;
    char *end;
    state st;
    char *tag;
    char *attr;

    char c = getchar();
    while(c != EOF) {
      buffer[i++] = c;
      c = getchar();
    }
    buffer[i] = 0;

    start = buffer;
    end = buffer;

    while(*start) {
      end = strstr(start, MARKER);
      if(end == NULL) {
        printf("%s", start);
        return;
      }

      start[end - start] = 0;
      printf("%s", start);
      st = htmlparser_parse(html, start, end - start);

      tag = htmlparser_tag(html);
      attr = htmlparser_attr(html);

      printf("<?state state=%s, ", htmlparser_state_str(html));

      if(tag)
        printf("tag=%s, ", tag);


      if(attr)
        printf("attr=%s, ", attr);

      printf("is_quoted=%d, is_js=%d, value_index=%d, ",
             htmlparser_is_attr_quoted(html),
             htmlparser_in_js(html),
             htmlparser_value_index(html)
             );

      if(htmlparser_in_js(html))
        printf("js_state=%s, js_is_quoted=%d, ",
               htmlparser_js_state_str(html), htmlparser_is_js_quoted(html));

      printf(" ?>");
      
      start = end + strlen(MARKER);

    }
    printf("\n");

//    htmlparser_parse(html, buffer, i + 1);
}
