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
#include <llvm/PassManager.h>
#include <llvm/DerivedTypes.h>

#include "named_values.h"

#ifndef redux_codegen_h
#define redux_codegen_h

namespace redux {
  class Node;
  class Block;
  class Function;
  class Prototype;
  class FunctionCall;
  class Constructor;
  class BinaryOperator;
  class Assignment;
  class Variable;
  class Identifier;
  class Float;
  class Integer;
  class Boolean;
  class ReturnKeyword;
  class IfElse;
  class Class;
  class MemberAccess;
  class MethodCall;
  class MemberAssignment;
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
  redux::NamedValues named_values;

  std::stack<llvm::IRBuilder<> *> builder_stack;
  
  std::map<std::string, std::map<std::string, int> > class_member_indices;
  
public:
  llvm::FunctionPassManager *function_pass_manager;
  
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
  llvm::Value *generate(redux::Constructor &constructor);  
  llvm::Value *generate(redux::BinaryOperator &bin_operator);
  llvm::Value *generate(redux::Assignment &assignment);
  llvm::Value *generate(redux::Variable &variable);  
  llvm::Value *generate(redux::Identifier &identifier);  
  llvm::Value *generate(redux::ReturnKeyword &return_keyword);    
  llvm::Value *generate(redux::Integer &integer);
  llvm::Value *generate(redux::Float &float_val);
  llvm::Value *generate(redux::Boolean &bool_val);
  llvm::Value *generate(redux::IfElse &if_else);
  llvm::Value *generate(redux::Class &klass);
  llvm::Value *generate(redux::MethodCall &method_call);
  llvm::Value *generate(redux::MemberAccess &member_access);
  llvm::Value *generate(redux::MemberAssignment &member_assignment);
  
  static llvm::Type *llvmTypeForString(std::string &type, llvm::Module &module);
  
  static llvm::Value *cast_as(llvm::IRBuilder<> &builder, llvm::Value *source_value, llvm::Type* dest_type);
  static llvm::ArrayRef<llvm::Value*> binary_cast(llvm::IRBuilder<> &builder, llvm::Value *left_value, llvm::Value *right_value);
  
  static llvm::AllocaInst *declareStackVar(llvm::Function *func, redux::Variable &variable);
  
private:
  llvm::Function *generate_new_method_for_type(llvm::StructType &type);
};


#endif
