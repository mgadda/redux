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
  function_pass_manager = NULL;
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
  //namedValues.clear();
  named_values.save();
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
  
  // Allocate stack space for function argument variables and load them into llvm variables
  size_t idx = 0;
  for (llvm::Function::arg_iterator ait = fun->arg_begin(); ait != fun->arg_end(); ++ait, ++idx) {
    // Add arg names to symbol table
    llvm::AllocaInst *alloca = declareStackVar(fun, *proto.args[idx]);
    builder().CreateStore(ait, alloca);
    
    named_values.set(proto.args[idx]->name, alloca);
  }
  
  // Return value is value of last node in function block
  if (Value *return_val = function.body->codeGen(*this)) {

    if (!isa<ReturnInst>(return_val)) {

      if(!isa<StoreInst>(return_val)) {
        // Dynamically look up op code so we can cast from block's last value
        // to return type of function at runtime.
        Instruction::CastOps op_code = CastInst::getCastOpcode(return_val, true, fun->getReturnType(), true);
        builder().CreateRet(builder().CreateCast(op_code, return_val, fun->getReturnType()));
      }
      else {
        builder().CreateRet(return_val);
      }
      
    }
    
    // Validate the generated code, checking for consistency.
    verifyFunction(*fun);

    if (function_pass_manager)
      function_pass_manager->run(*fun);

    pop_builder();
    named_values.restore();
    return fun;
  }
  else {
    // empty function, returns 0, 0.0 or void
    //builder().CreateRet(ConstantInt::get(getGlobalContext(), APInt(1, 0, true)));
    if (fun->getReturnType() == Type::getVoidTy(getGlobalContext())) {
      builder().CreateRetVoid();
    }
    else {
      builder().CreateRet(UndefValue::get(fun->getReturnType()));
    }    
    verifyFunction(*fun);

    if (function_pass_manager)
      function_pass_manager->run(*fun);

    pop_builder();
    named_values.restore();    
    return fun;
  }

  pop_builder();
  named_values.restore();
  
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
  Function::ArgumentListType::const_iterator cit = callee->getArgumentList().begin();
  
  for (size_t i=0, e=function_call.args.size(); i!=e; ++i, ++cit) {
    Type *expected_arg_type = cit->getType();
    
    Value *arg_value = function_call.args[i]->codeGen(*this);
    Instruction::CastOps op_code = CastInst::getCastOpcode(arg_value, true, expected_arg_type, true);
    
    callee_args.push_back(builder().CreateCast(op_code, arg_value, expected_arg_type));
  }
  
  if (callee->getReturnType() == Type::getVoidTy(getGlobalContext()))
    return builder().CreateCall(callee, callee_args);
  else
    return builder().CreateCall(callee, callee_args, "calltmp");
}

Value *CodeGenContext::generate(redux::BinaryOperator &bin_operator) {
  Value *left_value = bin_operator.lhs->codeGen(*this);
  Value *right_value = bin_operator.rhs->codeGen(*this);
  
  // this code assumes these values are constants, which isn't true
  Type *lv_ty = left_value->getType();
  Type *rv_ty = right_value->getType();
  
  if (CallInst *call_inst = dyn_cast<CallInst>(left_value)) {
    lv_ty = call_inst->getCalledFunction()->getReturnType();
  }

  if (CallInst *call_inst = dyn_cast<CallInst>(right_value)) {
    rv_ty = call_inst->getCalledFunction()->getReturnType();
  }
  

  // values must be of same type 
  if ((lv_ty != rv_ty && rv_ty == Type::getDoubleTy(getGlobalContext()))) {
    Instruction::CastOps op_code = CastInst::getCastOpcode(left_value, true, rv_ty, true);
    left_value = builder().CreateCast(op_code, left_value, rv_ty);
  }
  else if ((lv_ty != rv_ty && lv_ty == Type::getDoubleTy(getGlobalContext()))) {
    Instruction::CastOps op_code = CastInst::getCastOpcode(right_value, true, lv_ty, true);
    right_value = builder().CreateCast(op_code, right_value, lv_ty);
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
  AllocaInst *alloca_inst = named_values.get(assignment.var_name);
  
  Value *value = assignment.value->codeGen(*this);
  
  if (!value || !alloca_inst) {
    return 0;
  }
  
  value = cast_as(builder(), value, alloca_inst->getAllocatedType());
  builder().CreateStore(value, alloca_inst);
  return value;
}

// Variable declaration (and possibly assignment)
Value *CodeGenContext::generate(redux::Variable &variable) {
  
  // TODO: check if we're inside a function, if not, we're making global variables here
  // IMPORTANT: the variable might be declared with global, function or block level scope
  llvm::Function *current_function = builder().GetInsertBlock()->getParent();
  AllocaInst *alloca_inst = declareStackVar(current_function, variable);
  
  named_values.set(variable.name, alloca_inst);
  
  Value *value = NULL;
  if (variable.value) {
    value = variable.value->codeGen(*this);    
    if (!value) {
      return 0;
    }
  }
  else { // if no value given, set it to 0 or 0.0
    Type *init_val_ty = llvmTypeForString(variable.type);    
    value = UndefValue::get(init_val_ty);
  }
  
  value = cast_as(builder(), value, alloca_inst->getAllocatedType());
  builder().CreateStore(value, alloca_inst);    
  return value;
  
}

Value *CodeGenContext::generate(redux::Identifier &identifier) {
  AllocaInst *alloca_inst = named_values.get(identifier.name);  
  
  if (!alloca_inst) {
    std::cerr << "unknown variable " << identifier.name << std::endl;
    return 0;
  }
  return builder().CreateLoad(alloca_inst, identifier.name);
}

Value *CodeGenContext::generate(redux::ReturnKeyword &return_keyword) {
  // TODO: this doesn't work if invoked inside if else graph
  //  llvm::Function *current_function = builder().GetInsertBlock()->getParent();
  //  
  //  BasicBlock *return_now_bb = BasicBlock::Create(getGlobalContext(), "return_now", current_function);
  //  builder().CreateBr(return_now_bb);
  //  builder().SetInsertPoint(return_now_bb);
  if (return_keyword.returnExpression) {
    Value *value = return_keyword.returnExpression->codeGen(*this);
    return builder().CreateRet(value);  
  }
  else
    return builder().CreateRetVoid();
  
  
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
  Value *cond = if_else.condition.codeGen(*this);
  if (!cond) {
    return NULL;
  }

  // Cast whatever into 0 or 1
//  Instruction::CastOps op_code = CastInst::getCastOpcode(cond, true, Type::getInt1Ty(getGlobalContext()), false);
//  cond = builder().CreateCast(op_code, cond, Type::getInt1Ty(getGlobalContext()));
  
  // Compare against 1, doesn't the cast cover this?  
  cond = builder().CreateICmpEQ(cond, ConstantInt::get(getGlobalContext(), APInt(1, 1, false)));
  
  Function *current_function = builder().GetInsertBlock()->getParent();
  

  BasicBlock *then_bb = BasicBlock::Create(getGlobalContext(), "then", current_function);
  BasicBlock *else_bb = BasicBlock::Create(getGlobalContext(), "else");
  BasicBlock *merge_bb = BasicBlock::Create(getGlobalContext(), "ifcont");
  
  builder().CreateCondBr(cond, then_bb, else_bb);
  
  builder().SetInsertPoint(then_bb);
  Value *then_value = if_else.then_block.codeGen(*this);
  builder().CreateBr(merge_bb);
  
  then_bb = builder().GetInsertBlock();
  current_function->getBasicBlockList().push_back(else_bb);
  builder().SetInsertPoint(else_bb);
  
  Value *else_value;
  if (if_else.else_block) {
    else_value = if_else.else_block->codeGen(*this);
  }
  else {
    else_value = UndefValue::get(then_value->getType());
  }
  
  if (!else_value)
    return 0;
  
  builder().CreateBr(merge_bb);
  else_bb = builder().GetInsertBlock();
  
  current_function->getBasicBlockList().push_back(merge_bb);
  builder().SetInsertPoint(merge_bb);  
  PHINode *phi_node = builder().CreatePHI(then_value->getType(), NULL);
  
  //ArrayRef<Value*> cast_values = binary_cast(builder(), then_value, else_value);
//  phi_node->addIncoming(cast_values[0], then_bb);
//  phi_node->addIncoming(cast_values[1], else_bb);
  phi_node->addIncoming(then_value, then_bb);
  phi_node->addIncoming(else_value, else_bb);
  
  return phi_node;
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

llvm::Value *CodeGenContext::cast_as(llvm::IRBuilder<> &builder, llvm::Value *source_value, llvm::Type* dest_type) {
  if (source_value->getType() == dest_type) {
    return source_value;
  }
  
  llvm::Instruction::CastOps op_code = llvm::CastInst::getCastOpcode(source_value, true, dest_type, true);
  return builder.CreateCast(op_code, source_value, dest_type);
}

ArrayRef<Value*> CodeGenContext::binary_cast(IRBuilder<> &builder, Value *left_value, Value *right_value) {
  // this code assumes these values are constants, which isn't true
  Type *lv_ty = left_value->getType();
  Type *rv_ty = right_value->getType();
  
  lv_ty->dump();
  rv_ty->dump();
  
  if (CallInst *call_inst = dyn_cast<CallInst>(left_value)) {
    lv_ty = call_inst->getCalledFunction()->getReturnType();
  }
  
  if (CallInst *call_inst = dyn_cast<CallInst>(right_value)) {
    rv_ty = call_inst->getCalledFunction()->getReturnType();
  }  
  
  if ((lv_ty != rv_ty && rv_ty == Type::getDoubleTy(getGlobalContext()))) {
    left_value = CodeGenContext::cast_as(builder, left_value, rv_ty);
  }
  else if ((lv_ty != rv_ty && lv_ty == Type::getDoubleTy(getGlobalContext()))) {
    right_value = CodeGenContext::cast_as(builder, right_value, lv_ty);
  }

  Value* values[2] = {left_value, right_value};
  ArrayRef<Value*> cast_values(values);
  return cast_values;

}

llvm::AllocaInst *CodeGenContext::declareStackVar(llvm::Function *func, redux::Variable &variable) {
  llvm::IRBuilder<> tmp_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
  return tmp_builder.CreateAlloca(llvmTypeForString(variable.type), 0, variable.name);
}
