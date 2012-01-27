//
//  named_values.h
//  redux
//
//  Created by Matthew Gadda on 1/22/12.
//  Copyright (c) 2012 Matthew Gadda. All rights reserved.
//

#include <map>
#include <string>
#include <deque>
#include <llvm/Instructions.h>

#ifndef redux_named_values_h
#define redux_named_values_h

namespace redux {
  typedef std::map<std::string, llvm::AllocaInst*> AllocaMap;

  class NamedValues {
    
    std::deque<AllocaMap> named_values;  
    
  public:
    NamedValues();
    
    llvm::AllocaInst *get(const std::string &name);
    void set(const std::string &name, llvm::AllocaInst* alloca_inst);
    
    void save();
    void restore();
    AllocaMap &current();
    //AllocaInst *operator[](const std::string &name);
  };  
}

#endif
