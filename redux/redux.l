/*%option noyywrap warn yylineno*/
/*%option noyywrap warn nodefault yylineno*/
/*%option bison-bridge bison-locations*/
%option noyywrap nodefault yylineno warn
/*%option outfile="tokens.cpp"*/

%{
  #include <string>
  #include "node.h"
  #include "parser.hpp"
  #define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
  #define TOKEN(t) (yylval.token = t)
  //extern "C" int yywrap() { }
%}

%x COMMENT

%%
 /* C style comments-ish */
"/#"                  { BEGIN(COMMENT); }
<COMMENT>([^#]|\n)+|. 
<COMMENT>"#/"         { BEGIN(INITIAL); }
<COMMENT><<EOF>>      { printf("Line %d: Unterminated comment\n", yylineno); return 0; } 

 /* Ruby style comments */
"#".*\n               

[ \t\n]+ {}
\(        { return TOKEN(LPAREN); }
\)        { return TOKEN(RPAREN); }
'\''      { return TOKEN(QUOTE); }
'`'       { return TOKEN(BACKQUOTE); }
",@"      { return TOKEN(AT_SPLICE); }
","       { return TOKEN(COMMA); }


[-+]?[0-9]+\.[0-9]*     { SAVE_TOKEN; return FLOAT;}           
[-+]?[0-9]+             { SAVE_TOKEN; return INTEGER;}
"0x"[-+]?[0-9a-f]+      { SAVE_TOKEN; return HEX_INTEGER;}     
"0b"[-+]?[0-9]+         { SAVE_TOKEN; return BIN_INTEGER;}     
"0o"[-+]?[0-7]+         { SAVE_TOKEN; return OCT_INTEGER;}     
\"[^\"]*\"              { SAVE_TOKEN; return STRING;}
[a-zA-Z][a-zA-Z0-9_\-]* { SAVE_TOKEN; return SYMBOL;}


.                       { printf("mystery character: %x\n", yytext[0]); }


%%