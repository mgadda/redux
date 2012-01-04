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

extern redux::Block *programBlock;
extern int yyparse();
extern int yylex();
extern int yydebug;
extern FILE *yyin;
extern void yyrestart(FILE *input_file);
extern int yylineno;
extern int exit_status;

void *do_work(void *thread_id);

int main(int argc, char * const argv[])
{
  yydebug=0;
  exit_status=0;
  
  int c;
  int index;
  
  while ((c = getopt(argc, argv, "v")) != -1) {
    switch (c) {
      case 'v':
        yydebug=1;
        break;
        
      default:
        break;
    }
  }
  
  // case, no non-opt arguments, just parse into from stdin
  if (optind == argc) {
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
    FILE *f = fopen(argv[index], "r");
    
    if(!f) {
      perror(argv[index]);
      return 1;
    }
    
    yyrestart(f);
//    int t;
//    while ((t = yylex())) {
//      printf("%d\n", t);
//    }
    yylineno = 1;
    yyparse();
    
    if(programBlock) {
      std::cout << programBlock << std::endl;
      delete programBlock;
    }
      
      
    
    fclose(f);
  }
  
//  pthread_join(worker[0], &status);
//  pthread_join(worker[1], &status);
//  
//  pthread_exit(NULL);
  //std::cout << programBlock << std::endl;
  if (exit_status) exit(exit_status);
  
  return 0;
}

void *do_work(void *thread_id) {
  
  long wait_time = random()%10;
  printf("Doing some work on a thread %d for %d seconds.\n",    pthread_self(), wait_time);
  sleep((unsigned int)wait_time);
  printf("done.\n");
  return NULL;
}

