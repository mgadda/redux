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
extern void yyrestart(FILE *input_file);

int main(int argc, const char * argv[])
{
  //yydebug=1;
  
  if(argc < 2) {
    yyparse();
    return 0;
  }
    
  for(int i=1; i<argc; i++) {
    FILE *f = fopen(argv[1], "r");
    
    if(!f) {
      perror(argv[i]);
      return 1;
    }
    
    yyrestart(f);
    yyparse();
    fclose(f);
  }
  
  //std::cout << programBlock << std::endl;
  return 0;
}
