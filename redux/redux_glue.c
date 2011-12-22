#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "redux_glue.h"

void yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "%d: error: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}
