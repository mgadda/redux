%{
  #include "node.h"
  //DXBlock *programBlock;
  #define YYDEBUG 1  
  extern int yylex();
  void yyerror(const char *s) { printf("ERROR: %s\n", s); }
%}

%defines

%union {
  RDXExpression *expr;
  std::string *string;
  int token;
}

%token <string> TINTEGER;

%type <expr> numeric;

%start numeric

%%
numeric: TINTEGER { $$ = new RDXInteger(atol($1->c_str())); delete $1; }

%%