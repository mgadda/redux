//
//  main.cpp
//  Redux
//
//  Created by Matt Gadda on 12/14/11.
//  Copyright (c) 2011 Catalpa Lab. All rights reserved.
//

#include <iostream>
#include "node.h"

//extern RDXBlock* programBlock;
extern int yyparse();
extern int yylex();
extern int yydebug;
extern FILE *yyin;

int main(int argc, const char * argv[])
{
  //yydebug=1;
  
  if(argc > 1) {
    if(!(yyin = fopen(argv[1], "r"))) {
      perror(argv[1]);
      return 1;
    }
  }
  yyparse();
  
  //std::cout << programBlock << std::endl;
  return 0;
}
