all: redux
redux: redux.y redux.l main.cpp ast.h
	bison -v -g -o parser.cpp redux.y && \
	flex -o tokens.cpp redux.l && \
	gcc -x c++ -lstdc++ -g -o $@ parser.cpp tokens.cpp main.cpp

graph: redux
	dot -Tsvg parser.dot -o parser.svg

  
clean:
	rm -rf redux parser.cpp tokens.cpp parser.hpp parser.dot parser.svg redux.dSYM 
	