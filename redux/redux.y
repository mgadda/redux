%{
  #include <stdio.h>
  #include "redux_glue.h"  
%}

%token NUMBER

%%
program: NUMBER
%%