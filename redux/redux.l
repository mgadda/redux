/*%option noyywrap warn yylineno*/
/*%option noyywrap warn nodefault yylineno*/
/*%option bison-bridge bison-locations*/
%option noyywrap nodefault yylineno warn
/*%option outfile="tokens.cpp"*/

%{
  #include <string>
  #include "ast.h"
  #include "parser.hpp"
  #define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
  #define TOKEN(t) (yylval.token = t)
  //extern "C" int yywrap() { }
  
  int start_token;
%}

%x COMMENT

%%
%{
  if (start_token) {
    int t = start_token;
    start_token = 0;
    return t;
  }
%}

 /* C style comments-ish */
"/#"                  { BEGIN(COMMENT); }
<COMMENT>([^#]|\n)+|. 
<COMMENT>"#/"         { BEGIN(INITIAL); }
<COMMENT><<EOF>>      { printf("Line %d: Unterminated comment\n", yylineno); return 0; } 

  /* Ruby style comments */
"#".*                 

  /* Keywords */
"return"                { return TOKEN(T_RETURN); }
"extern"                { return T_EXTERN; }
"if"                    { return T_IF; }
"elsif"                 { return T_ELSIF; }
"else"                  { return T_ELSE; }

  /* Literals */
"true"                  { return T_TRUE; }
"false"                 { return T_FALSE; }
[-+]?[0-9]+\.[0-9]*     { SAVE_TOKEN; return T_FLOAT;}           
[-+]?[0-9]+             { SAVE_TOKEN; return T_INTEGER;}
"0x"[-+]?[0-9a-f]+      { SAVE_TOKEN; return T_HEX_INTEGER;}     
"0b"[-+]?[0-9]+         { SAVE_TOKEN; return T_BIN_INTEGER;}     
"0o"[-+]?[0-7]+         { SAVE_TOKEN; return T_OCT_INTEGER;}     
\"[^\"]*\"              { SAVE_TOKEN; return T_STRING;}
[a-zA-Z][a-zA-Z0-9_\-]*\?? { SAVE_TOKEN; return T_IDENTIFIER;}


[ \t]+ {}
\n        { return TOKEN(T_NEWLINE); }
\(        { return TOKEN(T_LPAREN); }
\)        { return TOKEN(T_RPAREN); }
\{        { return TOKEN(T_LCURLY); }
\}        { return TOKEN(T_RCURLY); }
\[        { return TOKEN(T_LBRACKET); }
\]        { return TOKEN(T_RBRACKET); }
","       { return TOKEN(T_COMMA); }
";"       { return TOKEN(T_SEMICOLON); }
":"       { return TOKEN(T_COLON); }
"="       { return TOKEN(T_EQUAL); }
"!="      { return TOKEN(T_CNOT_EQUAL); }
"=="      { return TOKEN(T_CEQUAL); }
\.        { return TOKEN(T_DOT); }
"+"       { return TOKEN(T_PLUS); }
"-"       { return TOKEN(T_MINUS); }
"*"       { return TOKEN(T_MULTIPLY); }
"/"       { return TOKEN(T_DIVIDE); }
"<"       { return TOKEN(T_CLESS_THAN); }



.                       { printf("mystery character: %x\n", yytext[0]); }


%%