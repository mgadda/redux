GC_PREFIX=/Users/mgadda/Projects/gc/gc-7.2alpha6
GC_INCLUDE=${GC_PREFIX}/include
GC_LIB=${GC_PREFIX}/.libs
GC_STATIC_LIB=${GC_PREFIX}/.libs/static

all: redux
redux: redux.y redux.l main.cpp codegen.h codegen.cpp ast.h named_values.h named_values.cpp class.h class.cpp gc.ll
	bison -v -g -o parser.cpp redux.y && \
	flex -o tokens.cpp redux.l && \
	clang -x c++ -lstdc++ -Wall `llvm-config --libs core jit native bitreader bitwriter linker --cxxflags --ldflags` -g -o $@ codegen.cpp parser.cpp tokens.cpp main.cpp named_values.cpp class.cpp

runtime: runtime.h runtime.cpp
	clang++ -Wall -I${GC_INCLUDE} -L${GC_STATIC_LIB} -lgc -g -shared -o libredux.a runtime.cpp

test: redux runtime test_code/class_test.rdx
	./redux test_code/class_test.rdx | llc > test_code/class_test.s && \
	clang -Wall test_code/class_test.s test_code/driver.c -L./ -lredux -o class_test 
  #clang -Wall test_code/class_test.s test_code/driver.c -o class_test

test2: redux runtime test_code/compileme.rdx rrt.c 
	./redux -d test_code/compileme.rdx > test_code/compileme.ll && \
	llc test_code/compileme.ll && \
	gcc -nostartfiles -c rrt.c && \
  llvm-mc -filetype=obj test_code/compileme.s -o compileme.o && \
	ld -macosx_version_min 10.7 compileme.o rrt.o -o compileme
  	
graph: redux
	dot -Tsvg parser.dot -o parser.svg

  
clean:
	rm -rf redux parser.cpp tokens.cpp parser.hpp parser.dot parser.svg parser.output redux.dSYM \
  class_test.bc class_test.ll test_code/class_test.s build/ compileme.o test_code/compileme.bc \
	rrt.o
	