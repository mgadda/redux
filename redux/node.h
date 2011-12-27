#include <iostream>
#include <vector>

class RDXObject;
typedef std::vector<RDXObject*> RDXList;

class RDXObject {
public:
  virtual ~RDXObject() {}
};

/* one or more expressions */
class RDXBlock : public RDXObject {
public:
  RDXList expressions;
};

/* (add 1 2) */
class RDXExpression : public RDXObject {
public:
  RDXList elements;
};


class RDXInteger : public RDXObject {
public:
  long long value;
  RDXInteger(long long value) : value(value) {}
};

class RDXSymbol : public RDXObject {  
public:
  std::string name;
  RDXSymbol(std::string name) : name(name) {}
};

