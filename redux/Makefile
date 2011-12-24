all: redux
redux: redux.y redux.l main.cpp node.h
	bison -o parser.cpp redux.y && \
	flex -o tokens.cpp redux.l && \
	gcc -x c++ -lstdc++ -g -o $@ parser.cpp tokens.cpp main.cpp

clean:
	rm -rf redux parser.cpp tokens.cpp parser.hpp redux.dSYM
	