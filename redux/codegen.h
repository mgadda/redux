//
//  codegen.h
//  redux
//
//  Created by Matthew Gadda on 1/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include <stack>
#include <map>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/support/IRBuilder.h>

#ifndef redux_codegen_h
#define redux_codegen_h

namespace redux {
  class Node;
  class Block;
  class Function;
  class Prototype;
  class FunctionCall;
  class BinaryOperator;
  class Assignment;
  class Variable;
  class Identifier;
  class Float;
  class Integer;
  class Boolean;
  class ReturnKeyword;
}


class CodeGenBlock {
public:
  llvm::BasicBlock *block;
  std::map<std::string, llvm::Value*> locals;
  
};

class CodeGenContext {
  std::stack<CodeGenBlock*> blocks;
  llvm::Function *mainFunction; // needed for full compilation, but not not JIT
  llvm::Module &module;
  std::map<std::string, llvm::Value*> namedValues;

  std::stack<llvm::IRBuilder<> *> builder_stack;
public:
  llvm::IRBuilder<> &builder();
  void pop_builder();
  void push_builder();
    
  CodeGenContext(llvm::Module &module);
  ~CodeGenContext();
  
  //llvm::Value *generate(redux::Node &node) { return 10; }
  llvm::Value *generate(redux::Block &block);
  llvm::Value *generate(redux::Function &function);
  llvm::Value *generate(redux::Prototype &prototype);
  llvm::Value *generate(redux::FunctionCall &function_call);
  llvm::Value *generate(redux::BinaryOperator &bin_operator);
  llvm::Value *generate(redux::Assignment &assignment);
  llvm::Value *generate(redux::Variable &variable);  
  llvm::Value *generate(redux::Identifier &identifier);  
  llvm::Value *generate(redux::ReturnKeyword &return_keyword);    
  llvm::Value *generate(redux::Integer &integer);
  llvm::Value *generate(redux::Float &float_val);
  llvm::Value *generate(redux::Boolean &bool_val);
  
  static llvm::Type *llvmTypeForString(std::string &type);
};


#endif
