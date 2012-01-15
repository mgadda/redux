//
//  codegen.cpp
//  redux
//
//  Created by Matthew Gadda on 1/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "codegen.h"
#include "ast.h"
#include "parser.hpp"

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Constants.h>
#include <llvm/Analysis/Verifier.h>

using namespace llvm;

CodeGenContext::CodeGenContext(Module &module) 
  : module(module)//, builder(*new IRBuilder<>(getGlobalContext())) 
{  
}


llvm::IRBuilder<> &CodeGenContext::builder() { 
  return *builder_stack.top(); 
}

void CodeGenContext::pop_builder() { 
  delete builder_stack.top(); builder_stack.pop(); 
}

void CodeGenContext::push_builder() { 
  builder_stack.push(new llvm::IRBuilder<>(llvm::getGlobalContext()));
}


Value *CodeGenContext::generate(redux::Block &block) {

  /*
   A block is a not basic block.
   A block is a collection of nodes (statements or expressions, the last of which
   becomes a reasonable value for the block.
   since a node can be a expression (3 + 4) or a statement (function definition)
   what is the return value for things that are expressions? 
   
   */
  // TODO: iterate over all nodes calling codegen on each.
  // no need to do anything with the value, codegen() itself causes insertion into module
  Value *last=NULL;
  for (redux::NodeList::iterator it = block.nodes.begin(); it != block.nodes.end(); ++it) {
    
    last = (**it).codeGen(*this);
  }
  return last;
}

Value *CodeGenContext::generate(redux::Function &function) {
  namedValues.clear();
  push_builder();
  
  // function should expect to already have its prototype set
  //printf("Defining function %s\n", function.prototype->name.c_str());
  redux::Prototype &proto = *function.prototype;
  llvm::Function *fun = (llvm::Function*)proto.codeGen(*this);
  
  if (!fun) {
    return NULL;
  }
  
  BasicBlock *basic_block = BasicBlock::Create(getGlobalContext(), "entry", fun);
  builder().SetInsertPoint(basic_block);
  
  // Return value is value of last node in function block
  if (Value *return_val = function.body->codeGen(*this)) {
    builder().CreateRet(return_val);
    
    // Validate the generated code, checking for consistency.
    verifyFunction(*fun);
    
    return fun;
  }
  else {
    // empty function, returns 0, 0.0 or void
    //builder().CreateRet(ConstantInt::get(getGlobalContext(), APInt(1, 0, true)));
    builder().CreateRet(UndefValue::get(fun->getReturnType()));
    verifyFunction(*fun);
    pop_builder();
    
    return fun;
  }
  
  pop_builder();
  
  fun->eraseFromParent();
  return NULL;
}

Value *CodeGenContext::generate(redux::Prototype &prototype) {
  
  // Define vector containing the types of the arguments  
  std::vector<Type*> arg_types;

  redux::VariableList::iterator it;
  for(it = prototype.args.begin(); it != prototype.args.end(); it++) {
    std::string type = (**it).type;
    
    arg_types.push_back(llvmTypeForString(type));
  }

  // Specify return type
  Type *return_type = llvmTypeForString(prototype.return_type);
  
  // Create function (w/o definition)
  FunctionType *function_type = FunctionType::get(return_type, arg_types, false);
  llvm::Function *function = llvm::Function::Create(function_type, Function::ExternalLinkage, prototype.name, &module);
  

  // Check for name conflicts
  if (function->getName() != prototype.name) {
    // Delete new function prototype if there was already function with this name
    function->eraseFromParent();
    function = module.getFunction(prototype.name);
    
    // If previous function already had definition, error!
    if (!function->empty()) {
      fprintf(stderr, "redefinition of function %s", prototype.name.c_str());
      return NULL;
    }
    
    // If function's arg length is different, reject
    if (function->arg_size() != prototype.args.size()) {
      fprintf(stderr, 
              "redefinition of function %s with different number of arguments (%lu and %lu)\n", 
              prototype.name.c_str(), 
              function->arg_size(), 
              prototype.args.size());
      return NULL;
    }
  }
  
  // Give names to function arguments
  size_t idx = 0;
  for (llvm::Function::arg_iterator ait = function->arg_begin(); ait != function->arg_end(); ++ait, ++idx) {
    ait->setName(prototype.args[idx]->name);
    
    // Add arg names to symbol table
    namedValues[prototype.args[idx]->name] = ait;
  }
  
  return function;
}

Value *CodeGenContext::generate(redux::FunctionCall &function_call) {
  llvm::Function *callee = module.getFunction(function_call.callee);
  if (callee == NULL) {
    fprintf(stderr, "undefined function %s\n", function_call.callee.c_str());
    return 0;
  }
  
  if (callee->arg_size() != function_call.args.size()) {
    fprintf(stderr, "wrong number of arguments passed to %s\n", function_call.callee.c_str());
    return 0;
  }

  std::vector<Value*> callee_args;
  for (size_t i=0, e=function_call.args.size(); i!=e; ++i) {
    callee_args.push_back(function_call.args[i]->codeGen(*this));
  }
  
  return builder().CreateCall(callee, callee_args, "calltmp");
}

Value *CodeGenContext::generate(redux::BinaryOperator &bin_operator) {
  Value *left_value = bin_operator.lhs->codeGen(*this);
  Value *right_value = bin_operator.rhs->codeGen(*this);
  
  // values must be of same type!
  if (left_value->getType() != right_value->getType()) {
    std::cerr << "operand type mismatch" << std::endl;    
    return 0;
  }
  
  if (left_value->getType() == Type::getInt64Ty(getGlobalContext())) {
    switch (bin_operator.operation) {
      case T_PLUS:
        return builder().CreateAdd(left_value, right_value);
        break;
      case T_MINUS:
        return builder().CreateSub(left_value, right_value);
        break;
      case T_MULTIPLY:
        return builder().CreateMul(left_value, right_value);
        break;
      case T_DIVIDE:
        return builder().CreateSDiv(left_value, right_value);
        break;
      case T_CLESS_THAN:
        return builder().CreateICmpULT(left_value, right_value);
        break;
      case T_CEQUAL:
        return builder().CreateICmpEQ(left_value, right_value);
        break;
      case T_CNOT_EQUAL:
        return builder().CreateICmpNE(left_value, right_value);
        break;
      default:
        break;
    }
    
  }
  else if (left_value->getType() == Type::getDoubleTy(getGlobalContext())) {
    switch (bin_operator.operation) {
      case T_PLUS:
        return builder().CreateFAdd(left_value, right_value);
        break;
      case T_MINUS:
        return builder().CreateFSub(left_value, right_value);
        break;
      case T_MULTIPLY:
        return builder().CreateFMul(left_value, right_value);
        break;
      case T_DIVIDE:
        return builder().CreateFDiv(left_value, right_value);
        break;
      case T_CLESS_THAN:
        return builder().CreateFCmpULT(left_value, right_value);
        break;
      case T_CEQUAL:
        return builder().CreateFCmpUEQ(left_value, right_value);
        break;
      case T_CNOT_EQUAL:
        return builder().CreateFCmpUNE(left_value, right_value);
        break;
      default:
        break;
    }
  }
  else {
    std::cerr << "Binary operations involving "<< left_value->getType() <<"and"<< right_value->getType() <<" are not supported" << std::endl;
  }
  
  return 0;
}

Value *CodeGenContext::generate(redux::Assignment &assignment) {
  return NULL;
}

Value *CodeGenContext::generate(redux::Variable &variable) {
  return NULL;
}

Value *CodeGenContext::generate(redux::Identifier &identifier) {
  Value *value = namedValues[identifier.name];  
  
  if (!value) {
    std::cerr << "unknown variable " << identifier.name << std::endl;
    return 0;
  }
  return value;
}

Value *CodeGenContext::generate(redux::ReturnKeyword &return_keyword) {
  // TODO: ensure returnExpression evaluates to same type as function prototype
  return builder().CreateRet(return_keyword.returnExpression.codeGen(*this));
}

Value *CodeGenContext::generate(redux::Integer &integer) {
  //  LLVMContext &context = getGlobalContext();
  //  IntegerType *int64_type = Type::getInt64Ty(context);  
  //  return ConstantInt::get(int64_type, integer.value);
  return ConstantInt::get(getGlobalContext(), APInt(64, integer.value, true));
}

Value *CodeGenContext::generate(redux::Float &float_val) {
  return ConstantFP::get(getGlobalContext(), APFloat(float_val.value));
}

Value *CodeGenContext::generate(redux::Boolean &bool_val) {
  return ConstantInt::get(getGlobalContext(), APInt(1, bool_val.truthiness, false));
}

Value *CodeGenContext::generate(redux::IfElse &if_else) {
  return NULL;
}

CodeGenContext::~CodeGenContext() {
  if (mainFunction) delete mainFunction;
}


llvm::Type *CodeGenContext::llvmTypeForString(std::string &type) {
  if (type == "int") {
    return Type::getInt64Ty(getGlobalContext());
  }
  else if (type == "float" || type == "double") {
    return Type::getDoubleTy(getGlobalContext());
  }
  else if (type == "bool") {
    return Type::getInt1Ty(getGlobalContext());
  }
  else if (type == "void") {
    return Type::getVoidTy(getGlobalContext());
  }
  return NULL;
}
