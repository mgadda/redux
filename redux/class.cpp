//
//  class.cpp
//  redux
//
//  Created by Matthew Gadda on 1/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "class.h"

namespace redux {
  void Class::add_instance_method(Function &function) {
    methods[function.prototype->name] = &function;
  }
  
  void Class::add_instance_variable(Variable &variable) {
    variables[variable.name] = &variable;
  }
  
  void Class::add_class_method(Function &function) {
    class_methods[function.prototype->name] = &function;    
  }
  void Class::add_class_variable(Variable &variable) {
    class_variables[variable.name] = &variable;    
  }

}
