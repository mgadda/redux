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

int main(int argc, const char * argv[])
{
  //yydebug=1;
  yyparse();
  // while(1)
  //   yylex();
  
  //std::cout << yylex();
  //std::cout << programBlock << std::endl;
  return 0;
}
