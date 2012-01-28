all: redux
redux: redux.y redux.l main.cpp codegen.h codegen.cpp ast.h named_values.h named_values.cpp
	bison -v -g -o parser.cpp redux.y && \
	flex -o tokens.cpp redux.l && \
	clang -x c++ -lstdc++ -Wall `llvm-config --libs core jit native --cxxflags --ldflags` -g -o $@ codegen.cpp parser.cpp tokens.cpp main.cpp named_values.cpp

graph: redux
	dot -Tsvg parser.dot -o parser.svg

  
clean:
	rm -rf redux parser.cpp tokens.cpp parser.hpp parser.dot parser.svg parser.output redux.dSYM 
	