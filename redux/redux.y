%{
  #include <stdio.h>
  #include "redux.h"  
%}

%token NUMBER

%%
program: NUMBER
%%