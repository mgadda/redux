
all: redux
redux: redux.l redux.y redux.h main.c redux.c
	bison -d redux.y && \
	flex -oredux.lex.c redux.l && \
	gcc -o $@ *.c -lfl

clean:
	rm -f redux.lex.c redux.tab.c redux.tab.h redux
	