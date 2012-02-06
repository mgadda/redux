//
//  codegen.cpp
//  redux
//
//  Created by Matthew Gadda on 1/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <map>

#include "codegen.h"
#include "ast.h"
#include "class.h"
#include "parser.hpp"

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Constants.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Instructions.h>

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
    
    arg_types.push_back(llvmTypeForString(type, module));
  }

  // Specify return type
  Type *return_type = llvmTypeForString(prototype.return_type, module);
  
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
  // if "this" is in named_values, find its type and check if there's
  // a function with type#fun signature, otherwise, just call function
  //named_values.get("this");
  
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
      case T_BITOR:
        return builder().CreateOr(left_value, right_value);
        break;
      case T_BITAND:
        return builder().CreateAnd(left_value, right_value);
        break;
      case T_CLESS_THAN:
        return builder().CreateICmpULT(left_value, right_value);
        break;
      case T_CGREATER_THAN:
        return builder().CreateICmpUGT(left_value, right_value);
        break;
      case T_CLTE:
        return builder().CreateICmpULE(left_value, right_value);
        break;        
      case T_CGTE:
        return builder().CreateICmpUGE(left_value, right_value);
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
      case T_CGREATER_THAN:
        return builder().CreateFCmpUGT(left_value, right_value);
        break;
      case T_CLTE:
        return builder().CreateFCmpULE(left_value, right_value);
        break;        
      case T_CGTE:
        return builder().CreateFCmpUGE(left_value, right_value);
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
  if (!current_function) {
    // TODO: Or, make it a global variable?
    return 0;
  }
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
    Type *init_val_ty = llvmTypeForString(variable.type, module);    
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

llvm::Value *CodeGenContext::generate(redux::Class &klass) {
  // IMPORTANT: must process fields first (to create type) before functions can
  // be generated, since they'll reference the instance variables
  
  // declare struct type with klass name
  // it should have fields that correspond to klass's variables
  
  //std::string class_type_name = std::string("class.") + klass.name;
  std::string class_type_name = klass.name;
  
  StructType *class_struct_type = module.getTypeByName(class_type_name);
  
  // TODO: determine way to look up variables in function bodies, need to keep
  // track of order in which they're stored in the struct type
  std::vector<Type*> fields;
  std::map<std::string, redux::Variable*>::iterator vit;
  int idx;
  for (idx=0, vit = klass.variables.begin(); vit != klass.variables.end(); ++vit, idx++) {
    std::string name = (*vit).first;
    redux::Variable &var = *(*vit).second;
    Type *ty = llvmTypeForString(var.type, module);
    fields.push_back(ty);
    class_member_indices[class_type_name][var.name] = idx;
  }
  
  if (!class_struct_type) {
    class_struct_type = StructType::create(module.getContext(), fields, class_type_name);    
  }
  else {
    class_struct_type->setBody(fields);
  }

  generate_new_method_for_type(*class_struct_type);
  //module.NumeredTypesMapTy;
  
  //CallInst::CreateMalloc(llvm::BasicBlock *InsertAtEnd, llvm::Type *IntPtrTy, llvm::Type *AllocTy, llvm::Value *AllocSize);
  
  //llvm::TargetData::getTypeAllocSize(class_struct_type);
 
  // klass.name, what we prefix methods with ClassName_method_name
  // iterate over each function and generate codegen for each, but use
  // the modified method name
  // arguments: pointer to ClassName, rest of args if any

  // method definitions should have instance and static variables at their disposal
  // that is, in named_values when they're being generated.
  
  // Modify each method's prototype to accept a pointer to an object of class_struct_type
  /*
   class Foo {
     int compute(int x) {}
   }
   becomes:
   class.Foo#compute
   compute(int x) -> Foo.compute(Foo this, int x)
   */
  
  // Generate instance methods
  std::map<std::string, redux::Function*>::iterator it;
  
  for (it = klass.methods.begin(); it != klass.methods.end(); ++it) {
    redux::Function *method = (*it).second;
    method->prototype->name = class_type_name + "#" + method->prototype->name;
    method->prototype->args.insert(method->prototype->args.begin(), new redux::Variable(class_type_name, "this"));
    method->codeGen(*this);
  }

  // Generate class methods
  for (it = klass.class_methods.begin(); it != klass.class_methods.end(); ++it) {
    redux::Function *method = (*it).second;
    method->prototype->name = class_type_name + "#" + method->prototype->name;
    method->codeGen(*this);
  }

  return 0;
}

llvm::Function *CodeGenContext::generate_new_method_for_type(StructType &type) {
  // create method called "class_name#foo"
  // someday it should invoke initialize() instance method on newly minted instance
  push_builder();
  
  redux::Prototype proto(type.getName(), type.getName().str() + "#new", redux::VariableList());
  llvm::Function *new_function = (llvm::Function*)proto.codeGen(*this);
  
  if (!new_function) {
    return NULL;
  }
  
  BasicBlock *basic_block = BasicBlock::Create(getGlobalContext(), "entry", new_function);
  builder().SetInsertPoint(basic_block);
  
  // Obtain size of type (this is portable hack brought to you by: http://nondot.org/sabre/LLVMNotes/SizeOf-OffsetOf-VariableSizedStructs.txt)  
  Value* gepIdx[1] = {ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 1)};
  Value *size;// = builder().CreateGEP(Constant::getNullValue(type.getPointerTo()), gepIdx); 
  size = GetElementPtrInst::Create(Constant::getNullValue(type.getPointerTo()), gepIdx, "sizeof", basic_block);
  
  //Value *sizeI = builder().CreateIntCast(size, Type::getInt32Ty(getGlobalContext()), false);
  //CastInst::CreateIntegerCast(size, <#llvm::Type *Ty#>, <#bool isSigned#>)
  
  Value *sizeI = builder().CreatePointerCast(size, Type::getInt32Ty(getGlobalContext()));
  
//  sizeI->dump();
//  sizeI->getType()->dump();
  
  Instruction *malloc_inst = CallInst::CreateMalloc(basic_block, Type::getInt32Ty(getGlobalContext()), type.getPointerTo(), sizeI);
  builder().Insert(malloc_inst);
  //ReturnInst::Create(getGlobalContext(), malloc_inst, basic_block);
  //llvm::AllocInst *alloca = builder().CreateAlloca(type.getPointerTo());  
  
  //gepIdx[0] = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
  Value *bit_cast = builder().CreateBitCast(builder().CreateGEP(malloc_inst, gepIdx), type.getPointerTo());
  
  builder().CreateRet(bit_cast);

  verifyFunction(*new_function);
  
  pop_builder();

  
  return new_function;
}

llvm::Value *CodeGenContext::generate(redux::Constructor &constructor) {
  return NULL;
}


llvm::Value *CodeGenContext::generate(redux::MemberAccess &member_access) {
  
  /*
   if expression is present, determine type of expression, must be struct type
   1. look up offset of member identifier's name into structs of this type (use global mapping)
   2. use getelementptr to access the value stored at the appropriate offset
   3. return value
   */
  if (member_access.expression) {
      
    llvm::Value *obj_expr_value = member_access.expression->codeGen(*this);
    Type *obj_expr_type = obj_expr_value->getType();
    if (obj_expr_type->isStructTy()) {
      StructType *struct_ty = dyn_cast<StructType>(obj_expr_type);
      
      int offset_idx = class_member_indices[struct_ty->getName()][member_access.member_identifier.name];      
      Value *offset_idx_value = ConstantInt::get(getGlobalContext(), APInt(8, offset_idx, true));
          
      Value *offset_values[2] = {ConstantInt::get(getGlobalContext(), APInt(1, 0, true)), offset_idx_value};
      return builder().CreateGEP(obj_expr_value, offset_values);
    }
  }
  /*
   if expression is not present, it means the member access occurred in the form of @var_name.
   1. determine struct type based on current class being constructed (where is this stored?)
      it could be stored in named_values as the this alloca pointer
   2. look up offset of member identifier's name into structs of this type (use global mapping)
   3. use getelementptr to access the value stored at the appropriate offset
   4. return value
   */
  if (!member_access.expression) {
    AllocaInst *this_alloc_inst = named_values.get("this"); 
    LoadInst *this_load_inst = builder().CreateLoad(this_alloc_inst, "this");

    StructType *struct_ty = dyn_cast<StructType>(this_alloc_inst->getAllocatedType());
    
    int offset_idx = class_member_indices[struct_ty->getName().str()][member_access.member_identifier.name];      
    Value *offset_idx_value = ConstantInt::get(getGlobalContext(), APInt(8, offset_idx, true));
    
    Value *offset_values[2] = {ConstantInt::get(getGlobalContext(), APInt(1, 0, true)), offset_idx_value};
    return builder().CreateGEP(this_load_inst, offset_values);
  
  } 
  return NULL;
}

llvm::Value *CodeGenContext::generate(redux::MethodCall &method_call) {
  bool is_class_method = false;
  StructType *class_type = NULL;
  // How do we know if Foo is a variabe or a class? 
  // check class_member_indices top level keys for something with the same name
  // class names always take precendence over variable names
  // this could be enforced syntactically by requiring variables have lower case names
  // and types have TitleCase names
  if (method_call.expression->node_type() == "Identifier") {
    redux::Identifier *ident = (redux::Identifier*)(method_call.expression);
    class_type = module.getTypeByName(ident->name);
    if (class_type) {
      // then this identifier refers to a class, not an instance of a class
      
      // if method name is "new," we have to construct a new object on the heap
      // and invoke its initialize method.
      // ney, do not construct it here, do that in class definition.
      // merely call method here. This should collapse into the case below.
      
      // if method name is anything else, invoke it like below, but just suppress
      // the passing of a this pointer, because this is a static method
      is_class_method = true;
    }
  }
  

  // Attempt to convert an expression into something with a type, that
  // type must be a known class so that we can reconstruct what the actual
  // function name is that should be invoked with the first argument being the
  // expression value itself.
  
  Value *object_expr_value = NULL;
  Value *derefed_obj_value = NULL;
  Type *obj_expr_type = NULL;
  if (!is_class_method) {
    object_expr_value = method_call.expression->codeGen(*this);
    
    derefed_obj_value = builder().CreateLoad(object_expr_value);
    obj_expr_type = derefed_obj_value->getType();
    
  }
  else
    obj_expr_type = class_type;
  
  obj_expr_type->dump();
  
  if (obj_expr_type->isStructTy()) {
    StructType *struct_ty = dyn_cast<StructType>(obj_expr_type);
  //builder().CreateBitCast(obj_expr_type, object_expr_value->get)
    
    std::string method_name = struct_ty->getName().str() + "#" + method_call.method_identifier.name;
    
    llvm::Function *callee = module.getFunction(method_name);
    if (callee == NULL) {
      fprintf(stderr, "undefined method %s\n", method_name.c_str());
      return 0;
    }
    
    size_t expected_args = method_call.args.size();
    if (!is_class_method) {
      // Add 1 here for the new "this" argument
      expected_args = method_call.args.size() + 1;
    }
    
    if (callee->arg_size() != expected_args) {
      fprintf(stderr, "wrong number of arguments passed to %s\n", method_name.c_str());
      return 0;
    }
    
    std::vector<Value*> callee_args;
    
    if (!is_class_method) {
      callee_args.push_back(object_expr_value);
    }
    
    // Cast arguments to expected types
    Function::ArgumentListType::const_iterator cit = callee->getArgumentList().begin();
    for (size_t i=0, e=method_call.args.size(); i!=e; ++i, ++cit) {
      Type *expected_arg_type = cit->getType();
      
      Value *arg_value = method_call.args[i]->codeGen(*this);
      Instruction::CastOps op_code = CastInst::getCastOpcode(arg_value, true, expected_arg_type, true);
      
      callee_args.push_back(builder().CreateCast(op_code, arg_value, expected_arg_type));
    }
    
    // void functions can't have names, apparently (better solution: get rid of void entirely)
    if (callee->getReturnType() == Type::getVoidTy(getGlobalContext()))
      return builder().CreateCall(callee, callee_args);
    else
      return builder().CreateCall(callee, callee_args, "calltmp");
    
  }
  return NULL;
}

llvm::Value *CodeGenContext::generate(redux::MemberAssignment &member_assignment) {
  
}

CodeGenContext::~CodeGenContext() {
  if (mainFunction) delete mainFunction;
}


llvm::Type *CodeGenContext::llvmTypeForString(std::string &type, llvm::Module &module) {
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
  else {
    return PointerType::get(module.getTypeByName(type), 0);
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
  return tmp_builder.CreateAlloca(llvmTypeForString(variable.type, *(func->getParent())), 0, variable.name);
}
