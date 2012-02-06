//
//  class.h
//  redux
//
//  Created by Matthew Gadda on 1/2/12.
//  Copyright (c) 2012 Catalpa Labs. All rights reserved.
//

#ifndef redux_class_h
#define redux_class_h

#include <map>
#include <vector>
#include <string>
#include <llvm/Value.h>

#include "ast.h"
#include "codegen.h"

namespace redux {
  class Class : public Node {
  public:
    std::string name;
    std::string parent_name;
    std::map<std::string, Function*> methods;
    std::map<std::string, Variable*> variables;

    std::map<std::string, Function*> class_methods;
    std::map<std::string, Variable*> class_variables;

    Class(const std::string name) : name(name) {};
    Class(const std::string name, const std::string parent_name) : name(name), parent_name(parent_name) {};
    
    void add_instance_method(Function &function);
    void add_instance_variable(Variable &variable);

    void add_class_method(Function &function);
    void add_class_variable(Variable &variable);

    virtual const std::string node_type() { return "Class"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }        
  };  
}

#endif
