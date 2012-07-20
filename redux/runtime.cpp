//
//  runtime.cpp
//  redux
//
//  Created by Matthew Gadda on 2/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <gc.h>
#include "runtime.h"
#include <assert.h>

void init_redux() {
  GC_INIT();
  int i;
  for (i = 0; i < 10000000; ++i)
  {
    int **p = (int **) GC_MALLOC(sizeof(int *));
    int *q = (int *) GC_MALLOC_ATOMIC(sizeof(int));
    assert(*p == 0);
    *p = (int *) GC_REALLOC(q, 2 * sizeof(int));
    if (i % 100000 == 0)
      printf("Heap size = %d\n", GC_get_heap_size());
    }
}

int runtime_fun(int value) {
  std::cout << "Value is: " << value << std::endl;
  void *mem = GC_MALLOC(10);
  return value + 10;
}