
all: redux
redux: redux.l redux.y redux_glue.h redux_glue.c main.c
	bison -d redux.y && \
	flex redux.l && \
	gcc -o $@ *.c -lfl

clean:
	rm -f lex.yy.c redux.tab.c redux.tab.h redux
	