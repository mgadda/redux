//
//  named_values.cpp
//  redux
//
//  Created by Matthew Gadda on 1/22/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "named_values.h"

namespace redux {
    
  llvm::AllocaInst *NamedValues::get(const std::string &name) {
    std::deque<AllocaMap>::iterator dit;
    
    for (dit = named_values.begin(); dit != named_values.end(); ++dit) {
      if (dit->count(name) > 0) {
        return (*dit)[name];
      }
    }
    return NULL;
  }

  void NamedValues::set(const std::string &name, llvm::AllocaInst* alloca_inst) {
    named_values.back()[name] = alloca_inst;
  }

  void NamedValues::save() {
    named_values.push_front(AllocaMap());
  }

  void NamedValues::restore() {
    if (named_values.size() > 1)
      named_values.pop_front();
  }

  AllocaMap &NamedValues::current() {
    return named_values.front();
  }

  NamedValues::NamedValues() {
    save();
  }
}