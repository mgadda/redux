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
#include "parser.hpp"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/support/IRBuilder.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Assembly/PrintModulePass.h>

extern redux::Block *fileBlock;
extern redux::Node *topLevelNode;
extern int yyparse();
extern int yylex();
extern int yydebug;
extern FILE *yyin;
extern void yyrestart(FILE *input_file);
extern int yylineno;
extern int exit_status;

extern int start_token;

void *do_work(void *thread_id);

int main(int argc, char * const argv[])
{ 
  yydebug=0;
  exit_status=0;
  bool interactive=false;
  bool compiled=false;
  bool optimized=false;
  
  int c;
  int index;
  
  while ((c = getopt(argc, argv, "vicz")) != -1) {
    switch (c) {
      case 'v':
        yydebug=1;
        break;
      case 'i':
        interactive=true;
        break;
      case 'c':
        compiled=true;
        break;
      case 'z':
        optimized=true;
      default:
        break;
    }
  }
  
  if (!compiled) {
    llvm::InitializeNativeTarget();
  }  
  
  // case, no non-opt arguments, just parse, compile or run from stdin 
  if (optind == argc && !interactive) {
    llvm::Module *module = new llvm::Module("stdin", llvm::getGlobalContext()); 
    CodeGenContext *context = new CodeGenContext(*module);
    llvm::PassManager module_pass_manager;

    module_pass_manager.add(llvm::createPrintModulePass(&llvm::fouts()));
    
    yylineno = 1;
    start_token = START_FILE;
    
    yyparse();
    
    if(fileBlock) {
      llvm::fdbgs() << "\n\nCompiling module from stdin\n";
      context->generate(*fileBlock);
      llvm::verifyModule(*module);
      
      module_pass_manager.run(*module);
      
      delete fileBlock;
    }
    else {
      llvm::ferrs() << "\n\nFailed to compile Module stdin\n";
    }
    
    delete module;
    delete context;

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
  
  // TODO: create main function into which all non-function and class definitions
  // can be inserted, this will make it possible to just "run" a file without specifying main()
  for (index = optind; index < argc; index++) {
    llvm::Module *module = new llvm::Module(argv[index], llvm::getGlobalContext()); 
    CodeGenContext *context = new CodeGenContext(*module);
    llvm::PassManager module_pass_manager;
    
    module_pass_manager.add(llvm::createPrintModulePass(&llvm::fouts()));

    FILE *f = fopen(argv[index], "r");
    
    if(!f) {
      perror(argv[index]);
      return 1;
    }
    
    start_token = START_FILE;
    yyrestart(f);
//    int t;
//    while ((t = yylex())) {
//      printf("%d\n", t);
//    }
    yylineno = 1;
    yyparse();
    
    if(fileBlock) {
      //llvm::fdbgs() << "\n\nCompiling module " << argv[index] << "\n";
      context->generate(*fileBlock);
      llvm::verifyModule(*module);
      module_pass_manager.run(*module);
      delete fileBlock;
    }
    else {
      llvm::ferrs() << "\n\nFailed to compile Module " << argv[index] << "\n";
    }
    
    delete module;
    delete context;
    fclose(f);
  }
  
  if (interactive) {
    llvm::Module *module = new llvm::Module("interactive", llvm::getGlobalContext()); 
    CodeGenContext interactive_context(*module);
    llvm::FunctionPassManager func_pass_man(module);
          
    llvm::ExecutionEngine *jit_engine = llvm::EngineBuilder(module).create();

    // Set up the optimizer pipeline.  Start with registering info about how the
    // target lays out data structures.
    func_pass_man.add(new llvm::TargetData(*jit_engine->getTargetData()));
    // Provide basic AliasAnalysis support for GVN.
    func_pass_man.add(llvm::createBasicAliasAnalysisPass());
    // 
    func_pass_man.add(llvm::createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    func_pass_man.add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    func_pass_man.add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    func_pass_man.add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    func_pass_man.add(llvm::createCFGSimplificationPass());
    // Eliminate tail calls (Bam!)
    func_pass_man.add(llvm::createTailCallEliminationPass());
    
    func_pass_man.doInitialization();

    if (optimized)
      interactive_context.function_pass_manager = &func_pass_man;
    //isatty(1);

    while(1) {
      std::cout << "> ";
      
      start_token = START_CONSOLE;
      yyrestart(stdin);
      yyparse();
            
      if (topLevelNode) {
        
        //llvm::IRBuilder<> builder(llvm::getGlobalContext());
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvm::getGlobalContext());//, "entry", tlf);
        interactive_context.push_builder();        
        llvm::IRBuilder<> &builder = interactive_context.builder();
        
        builder.SetInsertPoint(entry);

        llvm::Value *value = topLevelNode->codeGen(interactive_context);

        if (!value) {
          builder.CreateRet(llvm::UndefValue::get(llvm::Type::getVoidTy(llvm::getGlobalContext())));
          continue;
        }
         
        value->dump();
        
        
        // Create anonymous function to execute value inside, return type is same as that of value
        llvm::FunctionType *function_type = llvm::FunctionType::get(value->getType(), false);        
        llvm::Function *tlf = llvm::Function::Create(function_type, llvm::Function::InternalLinkage, "", module);        
        
        // Now that we have a function with the proper return type, insert the 
        // function body (as a basic block).
        // Is there not an easier way to insert a basic block into a function post-creation?!
        tlf->getBasicBlockList().insert(tlf->getBasicBlockList().begin(), entry);
        
        if (llvm::CallInst *call_inst = llvm::dyn_cast<llvm::CallInst>(value)) {
          if (call_inst->getCalledFunction()->getReturnType() == llvm::Type::getVoidTy(llvm::getGlobalContext())) {
            builder.CreateRetVoid();            
          }
          else
            builder.CreateRet(value);
        }
        else
          builder.CreateRet(value);

        
        //tlf->dump();
        
        llvm::verifyFunction(*tlf);
        
        //std::vector<GenericValue> emptyArgs;
        llvm::GenericValue ret_value = jit_engine->runFunction(tlf, std::vector<llvm::GenericValue>());          
        //void (*tlf_pointer)() = (void (*)())jit_engine->getPointerToFunction(tlf);
        //void *ret = tlf_pointer();
        
        llvm::Type *returnType = tlf->getReturnType();
        if (returnType == llvm::Type::getInt1Ty(llvm::getGlobalContext())) {
          if (ret_value.IntVal == 0) llvm::fouts() << "\n=> false\n";          
          else if (ret_value.IntVal == 1) llvm::fouts() << "\n=> true\n";           
          llvm::fouts().flush();
        }
        else if (returnType->isIntegerTy()) {
          llvm::fouts() << "\n=> " << ret_value.IntVal << "\n";
          llvm::fouts().flush();
        }
        else if (returnType->isDoubleTy()) {
          char strDouble[20]; 
          sprintf(strDouble, "%f", ret_value.DoubleVal);
          
          llvm::fouts() << "\n=> " << strDouble << "\n";          
          llvm::fouts().flush();
        }
        else if (returnType == llvm::Type::getInt64PtrTy(llvm::getGlobalContext())) {
          llvm::fouts() << "\n=> " << ret_value.PointerVal << "\n";
          llvm::fouts().flush();
        }    
        

        // Cleanup before next expression
        tlf->eraseFromParent();
        interactive_context.pop_builder();
        topLevelNode = NULL;
      }      
  
      //interactive_context.generate(*topLevelNode)->dump();
      
      //std::cout << "=> " << interactive_context.generate(*fileBlock) << std::endl;
    }
    
    // TODO: provide way to exit interactive mode and clean up memory (delete module)
  }
  
//  pthread_join(worker[0], &status);
//  pthread_join(worker[1], &status);
//  
//  pthread_exit(NULL);
  //std::cout << fileBlock << std::endl;
  if (exit_status) exit(exit_status);
  
  return 0;
}

//void *do_work(void *thread_id) {
//  
//  long wait_time = random()%10;
//  printf("Doing some work on a thread %lu for %ld seconds.\n", (unsigned long)pthread_self(), wait_time);
//  sleep((unsigned int)wait_time);
//  printf("done.\n");
//  return NULL;
//}
//
