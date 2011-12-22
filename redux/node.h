#include <iostream>
#include <vector>

class RDXExpression;
typedef std::vector<RDXExpression*> ExpressionList;

class RDXNode {
public:
  virtual ~RDXNode() {}
};

class RDXExpression : RDXNode {
public:  
};

class RDXInteger : public RDXExpression {
public:
  long long value;
  RDXInteger(long long value) : value(value) {}
};

class RDXBlock : public RDXExpression {
public:  
  ExpressionList expression;
  RDXBlock() {}  
};