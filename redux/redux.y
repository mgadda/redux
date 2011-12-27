%{
  #include "node.h"
  RDXBlock *programBlock;
  
  #define YYDEBUG 1  
  extern int yylex();
  void yyerror(const char *s) { printf("ERROR: %s\n", s); }
%}

%defines

%union {
  RDXBlock *block;
  RDXExpression *expression;
  RDXList *list;
  
  RDXObject *object;
  RDXSymbol *symbol;
  RDXInteger *integer;

  std::string *string;
  int token;
}

%token <string> SYMBOL INTEGER HEX_INTEGER BIN_INTEGER OCT_INTEGER STRING FLOAT
%token <token> LPAREN RPAREN QUOTE BACKQUOTE SPLICE COMMA AT_SPLICE

%type <block> program expressions
%type <expression> expression
%type <list> elementList
%type <object> element; 

%start program

%%

program: expressions          { programBlock = $1; }
;

expressions: expression       { $$ = new RDXBlock(); $$->expressions.push_back($<expression>1); }
  | expressions expression    { $1->expressions.push_back($<expression>2); }
;

expression: LPAREN RPAREN     { $$ = new RDXExpression(); }
  | LPAREN elementList RPAREN { $$ = new RDXExpression(); $$->elements = *$2; }
;

elementList: element          { $$ = new RDXList(); $$->push_back($1); }
  | elementList element       { $1->push_back($2); }
;

element: 
  | expression
  | INTEGER                   { $$ = new RDXInteger(atol($1->c_str())); delete $1; }
  | SYMBOL                    { $$ = new RDXSymbol(*$1); delete $1; }
;


%%