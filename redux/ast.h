
#ifndef __ast_h__
#define __ast_h__

#include <iostream>
#include <vector>
#include <map>

#include <llvm/Value.h>
#include "codegen.h"

namespace redux {

  class Node;
  class Expression;
  class Variable;
  
  typedef std::vector<Node*> NodeList;
  typedef std::vector<Expression*> ExpressionList;  
  typedef std::vector<Variable*> VariableList;  
  typedef std::map<std::string, Node*> Hash;

  class Node {    
  public:
    int lineno;
    
    virtual ~Node() {}
    virtual const std::string node_type()=0;
    
    virtual llvm::Value* codeGen(CodeGenContext& context)=0;
  };

  /* one or more expressions */
  class Block : public Node {
  public:
    NodeList nodes;
    virtual const std::string node_type() { return "Block"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class Expression : public Node {    
  };
  
  class Prototype : public Node {
  public:
    std::string return_type;
    std::string name;
    VariableList args;
    
    Prototype(const std::string &return_type,
              const std::string &name, 
              VariableList args)
    : return_type(return_type), name(name), args(args) {}
    
    virtual const std::string node_type() { return "Prototype"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class Function : public Node {
  public:
    Prototype *prototype;
    Block *body;
    
    Function(Prototype *prototype, Block *body) : prototype(prototype), body(body) {}

    virtual const std::string node_type() { return "Function"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class FunctionCall : public Expression {
  public:
    std::string callee;
    ExpressionList args;
    
    FunctionCall(const std::string &callee, ExpressionList &args)
      : callee(callee), args(args) {} 

    virtual const std::string node_type() { return "FunctionCall"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class Identifier : public Expression {
  public:
    std::string name;
    Identifier(const std::string &name) : name(name) {}
    
    virtual const std::string node_type() { return "Identifier"; }    
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };

  class Variable : public Node {
  public:
    std::string name;
    std::string type;
    Expression *value;
    
    Variable(std::string type, std::string name) : name(name), type(type) {}

    virtual const std::string node_type() { return "Variable"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class Integer : public Expression {
  public:
    int64_t value;
    Integer(long long value) : value(value) {}

    virtual const std::string node_type() { return "Integer"; }    
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class Float : public Expression {
  public:
    double value;
    Float(double value) : value(value) {}
    virtual const std::string node_type() { return "Float"; }        
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  };
  
  class Boolean : public Expression {
  public:
    bool truthiness;
    Boolean(bool value) : truthiness(value) {}
    virtual const std::string node_type() { return "Bool"; }        
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }    
  };
  
  class BinaryOperator : public Expression {
  public:
    int operation; // T_PLUS, T_MINUS, T_CEQUAL, etc
    Expression *lhs;
    Expression *rhs;
    
    BinaryOperator(int operation, Expression *lhs, Expression *rhs)
      : operation(operation), lhs(lhs), rhs(rhs) {}

    virtual const std::string node_type() { return "BinaryOperator"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
    
  };
  
  class Assignment : public Expression {
  public:
    std::string var_name;
    Expression *value;

    Assignment(const std::string &var_name, Expression *value)
      : var_name(var_name), value(value) {}

    virtual const std::string node_type() { return "Assignment"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }

  };
  
  class ReturnKeyword : public Node {
  public:
    Expression *returnExpression;
    
    ReturnKeyword(Expression &returnExpression) : returnExpression(&returnExpression) {}
    ReturnKeyword() : returnExpression(NULL) {}
    
    virtual const std::string node_type() { return "Keyword"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }

  };
  
  class IfElse : public Expression {
  public:
    Expression &condition;
    Block &then_block;
    Block *else_block;
    
    IfElse(Expression &condition_expression, Block &then_block) 
      : condition(condition_expression), then_block(then_block) {}    
    IfElse(Expression &condition_expression, Block &then_block, Block *else_block) 
      : condition(condition_expression), then_block(then_block), else_block(else_block) {}    

    virtual const std::string node_type() { return "IfElse"; }
    virtual llvm::Value* codeGen(CodeGenContext& context) { return context.generate(*this); }
  
  };
}

#endif
