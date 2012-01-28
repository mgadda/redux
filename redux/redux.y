
%{
  #include <stdlib.h>
  #include <sstream>
  
  #include "ast.h"
  redux::Block *fileBlock;
  redux::Node *topLevelNode;
  
  #define YYDEBUG 1  
  extern int yylex();
  extern int yylineno;
  
  int exit_status;

  void yyerror(const char *s, ...);
  
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
  redux::Prototype *function_proto;
  
  redux::Variable *variable_decl;  
  redux::VariableList *varlist;
  
  redux::Expression *expression;
  redux::ExpressionList *exprlist;
  
  redux::Integer *integer;
  redux::Float *float_num;

  redux::ReturnKeyword *return_keyword;
  
  redux::IfElse *ifelse;
  
  redux::NodeList *list;
  redux::Hash *hash;

  redux::Identifier *identifier;
  
  std::string *string;
  int token;
}

%token <string> T_IDENTIFIER T_INTEGER T_HEX_INTEGER T_BIN_INTEGER T_OCT_INTEGER T_STRING T_FLOAT T_TRUE T_FALSE

%token <token> T_LPAREN T_RPAREN T_LCURLY T_RCURLY T_LBRACKET T_RBRACKET T_COMMA 
%token <token> T_SEMICOLON T_EQUAL T_DOT T_COLON T_NEWLINE
%token <token> T_PLUS T_MINUS T_MULTIPLY T_DIVIDE
%token <token> T_RETURN T_EXTERN T_IF T_ELSIF T_ELSE
%token <token> T_BITOR T_BITAND
%token <token> T_CEQUAL T_CNOT_EQUAL T_CLESS_THAN T_CGREATER_THAN T_CLTE T_CGTE


%left T_PLUS T_MINUS
%right T_CEQUAL T_CNOT_EQUAL

%type <token> binary_operator

%type <block> file statements block elsif
%type <node> statement console
%type <expression> expression numeric bool func_call
%type <identifier> ident

%type <variable_decl> var_decl 
%type <function_decl> func_decl 
%type <function_proto> func_prototype 
%type <varlist> func_decl_args
%type <exprlist> call_args
%type <return_keyword> return_statement

%type <ifelse> if_stmt

%type <list> elementList
%type <hash> keyValueList

%token START_FILE
%token START_CONSOLE

%locations 
%error-verbose
  //%expect 14

%start start

%%

start: START_FILE file
  | START_CONSOLE console;
  
file: statements         { fileBlock = $1; }
  ;

console: interactive_statement { topLevelNode = $<node>1; YYACCEPT; }
;

interactive_statement: var_decl     
  | func_decl            
  | func_prototype
  | expression           
  ;
  
statements: statement       { $$ = new redux::Block(); $$->nodes.push_back($<expression>1); }
  | statements statement    { $1->nodes.push_back($<expression>2); }
  ;

statement: var_decl      { $$ = $1; }
  | func_decl            { $$ = $1; }
  | func_prototype       { $$ = $1; }
  | expression           { $$ = $1; }
  | return_statement     { $$ = $1; }
  ;

  
expression: ident T_EQUAL expression                { $$ = new redux::Assignment($1->name, $3); } 
  | func_call                                       { $$ = $1; }
  | T_LCURLY keyValueList T_RCURLY   /* hash declaration */
  | expression binary_operator expression           { $$ = new redux::BinaryOperator($2, $1, $3); }   /* method call via binary operator */ 
  | T_LPAREN expression T_RPAREN                    { $$ = $2; } /* grouping () */
  | T_LBRACKET elementList T_RBRACKET   /*{ $$ = new RDXExpression(); $$->elements = *$2; }*/
  | numeric                                         { $$ = $1; } /* 10, -3, 1.4345 */
  | bool                                            { $$ = $1; }
  | ident                                           { $$ = $1; } /* Foo, bar */
  | if_stmt                                         { $$ = $1; }
  ;

func_call: ident T_LPAREN call_args T_RPAREN        { $$ = new redux::FunctionCall($1->name, *$3); } /* method call */
  | ident T_DOT ident T_LPAREN call_args T_RPAREN   
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

func_prototype: T_EXTERN ident ident T_LPAREN func_decl_args T_RPAREN    { $$ = new redux::Prototype($2->name, $3->name, *$5); }
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
  | T_HEX_INTEGER                     { 
      long i=0;
      std::istringstream ss;
      ss.str(*$1);
      ss >> std::hex >> i;
      $$ = new redux::Integer(i); delete $1; 
    }
  | T_BIN_INTEGER                     {
      char *end;
      $$ = new redux::Integer(strtol($1->c_str()+2, &end, 2)); 
      delete $1; 
    }
  | T_OCT_INTEGER                     { 
      long i=0;
      std::istringstream ss;
      ss.str(*$1);
      ss >> std::oct >> i;
      $$ = new redux::Integer(i); 
      delete $1; 
    }
  ;

bool: T_TRUE                          { $$ = new redux::Boolean(true); }
  | T_FALSE                           { $$ = new redux::Boolean(false); }
  ;

if_stmt: T_IF expression block                { $$ = new redux::IfElse(*$2, *$3); }  
  | T_IF expression block T_ELSE block        { $$ = new redux::IfElse(*$2, *$3, $5); }
  | T_IF expression block elsif               { $$ = new redux::IfElse(*$2, *$3, $4); }
  | T_IF expression block elsif T_ELSE block  { ((redux::IfElse*)$4->nodes.back())->else_block = $6; $$ = new redux::IfElse(*$2, *$3, $4); }
  ;

elsif: T_ELSIF expression block      { $$ = new redux::Block(); $$->nodes.push_back(new redux::IfElse(*$2, *$3)); }
  | elsif T_ELSIF expression block   { 
      ((redux::IfElse*)$$->nodes.back())->else_block = new redux::Block();
      ((redux::IfElse*)$$->nodes.back())->else_block->nodes.push_back(new redux::IfElse(*$3, *$4));      
    }
  ;
  
keyValueList: keyValue /* | keyValueList T_ENDL */  
  | keyValueList T_COMMA keyValue
  ;

keyValue: ident T_COLON expression
  ;

binary_operator: T_PLUS | T_MINUS | T_MULTIPLY 
  | T_DIVIDE | T_CEQUAL | T_CNOT_EQUAL | T_CLESS_THAN | T_CGREATER_THAN
  | T_CLTE | T_CGTE
  | T_BITOR | T_BITAND
  ;

elementList: expression                 /*{ $$ = new RDXList(); $$->push_back($1); }*/
  | elementList T_COMMA expression      /*{ $1->push_back($2); }*/
  ;


return_statement: T_RETURN expression { $$ = new redux::ReturnKeyword(*$2); }
  | T_RETURN                         { $$ = new redux::ReturnKeyword(); }
  ;

empty: /* nothing */
  ;

    


%%