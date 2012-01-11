//
//  main.cpp
//  Redux
//
//  Created by Matt Gadda on 12/14/11.
//  Copyright (c) 2011 Catalpa Lab. All rights reserved.
//

#include <iostream>
#include "ast.h"
#include <pthread.h>
#include <getopt.h>
#include "parser.hpp"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

extern redux::Block *fileBlock;
extern redux::Node *topLevelNode;
extern int yyparse();
extern int yylex();
extern int yydebug;
extern FILE *yyin;
extern void yyrestart(FILE *input_file);
extern int yylineno;
extern int exit_status;

extern int start_token;

void *do_work(void *thread_id);

int main(int argc, char * const argv[])
{ 
  yydebug=0;
  exit_status=0;
  bool interactive=false;
  
  int c;
  int index;
  
  while ((c = getopt(argc, argv, "vi")) != -1) {
    switch (c) {
      case 'v':
        yydebug=1;
        break;
      case 'i':
        interactive=true;
      default:
        break;
    }
  }
  
  // case, no non-opt arguments, just parse into from stdin
  if (optind == argc && !interactive) {
    yyparse();
    return 0;
  }
  
//  void *status;
//  pthread_t worker[10];
//  int ret;
  
//  ret = pthread_create(&worker[0], NULL, do_work, NULL);
//  if(ret) {
//    printf("ERROR: pthread_create() returned %d\n", ret);
//  }
//  pthread_create(&worker[1], NULL, do_work, NULL);
//  if(ret) {
//    printf("ERROR: pthread_create() returned %d\n", ret);
//  }
  
  for (index = optind; index < argc; index++) {
    llvm::Module *module = new llvm::Module(argv[index], llvm::getGlobalContext()); 
    CodeGenContext *context = new CodeGenContext(*module);
    
    FILE *f = fopen(argv[index], "r");
    
    if(!f) {
      perror(argv[index]);
      return 1;
    }
    
    start_token = START_FILE;
    yyrestart(f);
//    int t;
//    while ((t = yylex())) {
//      printf("%d\n", t);
//    }
    yylineno = 1;
    yyparse();
    
    if(fileBlock) {
      std::cout << "\n\nCompiling module " << argv[index] << std::endl;
      context->generate(*fileBlock);
      module->dump();
      delete fileBlock;
    }
    else {
      std::cout << "\n\nFailed to compile Module " << argv[index] << std::endl;
    }
    
    delete module;
    delete context;
    fclose(f);
  }
  
  if (interactive) {
    llvm::Module *module = new llvm::Module("interactive", llvm::getGlobalContext()); 
    CodeGenContext interactive_context(*module);
    
    //isatty(1);
    
    while(1) {
      std::cout << "> ";
      
      start_token = START_CONSOLE;
      yyrestart(stdin);
      yyparse();
            
      if (topLevelNode) 
        topLevelNode->codeGen(interactive_context)->dump();
      
      //interactive_context.generate(*topLevelNode)->dump();
      
      //std::cout << "=> " << interactive_context.generate(*fileBlock) << std::endl;
    }
    
    // TODO: provide way to exit interactive mode and clean up memory (delete module)
  }
  
//  pthread_join(worker[0], &status);
//  pthread_join(worker[1], &status);
//  
//  pthread_exit(NULL);
  //std::cout << fileBlock << std::endl;
  if (exit_status) exit(exit_status);
  
  return 0;
}

//void *do_work(void *thread_id) {
//  
//  long wait_time = random()%10;
//  printf("Doing some work on a thread %lu for %ld seconds.\n", (unsigned long)pthread_self(), wait_time);
//  sleep((unsigned int)wait_time);
//  printf("done.\n");
//  return NULL;
//}
//
