
#ifndef __AST_H__
#define __AST_H__

#include <iostream>
#include <vector>
#include <map>

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
    virtual ~Node() {}
    virtual const std::string node_type()=0;
//    virtual void codeGen();
  };

  /* one or more expressions */
  class Block : public Node {
  public:
    NodeList nodes;
    virtual const std::string node_type() { return "Block"; }
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
    : name(name), args(args), return_type(return_type) {}
    
    virtual const std::string node_type() { return "Prototype"; }
  };
  
  class Function : public Node {
  public:
    Prototype *proto;
    Block *body;
    
    Function(Prototype *proto, Block *body) : proto(proto), body(body) {}

    virtual const std::string node_type() { return "Function"; }

  };
  
  class FunctionCall : public Expression {
  public:
    std::string callee;
    ExpressionList args;
    
    FunctionCall(const std::string &callee, ExpressionList &args)
      : callee(callee), args(args) {} 

    virtual const std::string node_type() { return "FunctionCall"; }

  };
  
  class Identifier : public Expression {
  public:
    std::string name;
    Identifier(const std::string &name) : name(name) {}
    
    virtual const std::string node_type() { return "Identifier"; }    
  };

  class Variable : public Node {
  public:
    std::string name;
    std::string type;
    Expression *value;
    
    Variable(std::string type, std::string name) : name(name), type(type) {}

    virtual const std::string node_type() { return "Variable"; }
  };
  
  class Integer : public Expression {
  public:
    long long value;
    Integer(long long value) : value(value) {}

    virtual const std::string node_type() { return "Integer"; }    
  };
  
  class Float : public Expression {
  public:
    double value;
    Float(double value) : value(value) {}
    virtual const std::string node_type() { return "Float"; }        
  };
  
  class BinaryOperator : public Expression {
  public:
    char operation; // +, -, !=, ==
    Expression *lhs;
    Expression *rhs;
    
    BinaryOperator(char operation, Expression *lhs, Expression *rhs)
      : operation(operation), lhs(lhs), rhs(rhs) {}

    virtual const std::string node_type() { return "BinaryOperator"; }
    
  };
  
  class Assignment : public Expression {
  public:
    std::string var_name;
    Expression *value;

    Assignment(const std::string &var_name, Expression *value)
      : var_name(var_name), value(value) {}

    virtual const std::string node_type() { return "Assignment"; }

  };
  
  class Keyword : public Node {
  public:
    Keyword() {}
    virtual const std::string node_type() { return "Keyword"; }

  };
}

#endif
