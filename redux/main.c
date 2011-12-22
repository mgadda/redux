//
//  main.cpp
//  redux
//
//  Created by Matt Gadda on 12/14/11.
//  Copyright (c) 2011 Catalpa Lab. All rights reserved.
//

#include <stdio.h>

#include "redux_glue.h"
#include "redux.tab.h"

int main (int argc, const char * argv[])
{
  printf("> "); 
  //yyparse();
  
  while(1) {
    printf("%d", yylex());
  }
  return 0;
}
