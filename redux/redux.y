
%{
  #include <stdlib.h>
  #include "ast.h"
  redux::Block *programBlock;
  
  #define YYDEBUG 1  
  extern int yylex();
  extern int yylineno;
  
  int exit_status;
  
  void yyerror(const char *s, ...) { 
    va_list ap;
    va_start(ap, s);
    
    fprintf(stderr, "\n\n%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n\n");
    
    exit_status = 1;
    //printf(">>>> ERROR: line %d, %s\n", yylineno, s); 
  }
%}

%defines

%union {
  redux::Node *node;
  
  redux::Block *block;

  redux::Function *function_decl;
  redux::FunctionCall *function_call;
  
  redux::Variable *variable_decl;  
  redux::VariableList *varlist;
  
  redux::Expression *expression;
  redux::ExpressionList *exprlist;
  
  redux::Integer *integer;
  redux::Float *float_num;

  redux::Keyword *keyword;
  
  redux::NodeList *list;
  redux::Hash *hash;

  redux::Identifier *identifier;
  
  std::string *string;
  int token;
}

%token <string> T_IDENTIFIER T_INTEGER T_HEX_INTEGER T_BIN_INTEGER T_OCT_INTEGER T_STRING T_FLOAT

%token <token> T_LPAREN T_RPAREN T_LCURLY T_RCURLY T_LBRACKET T_RBRACKET T_COMMA 
%token <token> T_SEMICOLON T_EQUAL T_DOT T_COLON
%token <token> T_PLUS T_MINUS
%token <token> T_RETURN
%token <token> T_CEQUAL T_CNOT_EQUAL

%left T_PLUS T_MINUS
%right T_CEQUAL T_CNOT_EQUAL

%type <token> binary_operator

%type <block> program statements block
%type <node> statement
%type <expression> expression numeric
%type <identifier> ident

%type <variable_decl> var_decl 
%type <function_decl> func_decl 
%type <varlist> func_decl_args
%type <exprlist> call_args
%type <keyword> return_statement

%type <list> elementList
%type <hash> keyValueList

%locations 

%start program

%%

empty: /* nothing */
;

program: statements         { programBlock = $1; }
  ;

statements: statement       { $$ = new redux::Block(); $$->nodes.push_back($<expression>1); }
  | statements statement    { $1->nodes.push_back($<expression>2); }
  ;

eos: empty
  | T_SEMICOLON

statement: var_decl eos     { $$ = $1; }
  | func_decl eos           { $$ = $1; }
  | expression eos          { $$ = $1; }
  | return_statement eos    { $$ = $1; }
  ;

  
expression: ident T_EQUAL expression                { $$ = new redux::Assignment($1->name, $3); } 
  | ident T_LPAREN call_args T_RPAREN               { $$ = new redux::FunctionCall($1->name, *$3); } /* method call */
  | ident T_DOT ident T_LPAREN call_args T_RPAREN 
  | T_LCURLY keyValueList T_RCURLY   /* hash declaration */
  | expression binary_operator expression           { $$ = new redux::BinaryOperator($2, $1, $3); }   /* method call via binary operator */ 
  | T_LPAREN expression T_RPAREN                    { $$ = $2; } /* grouping () */
  | T_LBRACKET elementList T_RBRACKET   /*{ $$ = new RDXExpression(); $$->elements = *$2; }*/
  | numeric                                         { $$ = $1; } /* 10, -3, 1.4345 */
  | ident                                           { $$ = $1; } /* Foo, Bar */
  ;

var_decl: ident ident                   { $$ = new redux::Variable($1->name, $2->name); }
  | ident ident T_EQUAL expression      { $$ = new redux::Variable($1->name, $2->name); $$->value = $4; }
  ;

func_decl: ident ident T_LPAREN func_decl_args T_RPAREN block 
    {
      redux::Prototype *proto = new redux::Prototype($1->name, $2->name, *$4);
      $$ = new redux::Function(proto, $6); 
    }    
  ;

block: T_LCURLY statements T_RCURLY   { $$ = $2; }
  | T_LCURLY T_RCURLY                 { $$ = new redux::Block(); }
  ;

func_decl_args: empty                 { $$ = new redux::VariableList(); }
  | var_decl                          { $$ = new redux::VariableList(); $$->push_back($<variable_decl>1); }
  | func_decl_args T_COMMA var_decl   { $1->push_back($<variable_decl>3); }
  ;

call_args: empty                      { $$ = new redux::ExpressionList(); }
  | expression                        { $$ = new redux::ExpressionList(); $$->push_back($1); }
  | call_args T_COMMA expression      { $1->push_back($3); }
  ;

ident: T_IDENTIFIER                   { $$ = new redux::Identifier(*$1); }
  ;
 
numeric: T_INTEGER                    { $$ = new redux::Integer(atol($1->c_str())); delete $1; } /*{ $$ = new RDXInteger(atol($1->c_str())); delete $1; }*/
  | T_FLOAT                           { $$ = new redux::Float(atof($1->c_str())); delete $1; }
  ;

keyValueList: keyValue /* | keyValueList T_ENDL */  
  | keyValueList T_COMMA keyValue
  ;

keyValue: ident T_COLON expression
  ;

binary_operator: T_PLUS | T_MINUS | T_CEQUAL | T_CNOT_EQUAL 
  ;

elementList: expression                 /*{ $$ = new RDXList(); $$->push_back($1); }*/
  | elementList T_COMMA expression      /*{ $1->push_back($2); }*/
  ;


return_statement: T_RETURN expression;



%%