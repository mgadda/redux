/*%option noyywrap warn yylineno*/
/*%option noyywrap warn nodefault yylineno*/
/*%option bison-bridge bison-locations*/
%option noyywrap nodefault yylineno

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

[\s\t\n]   
[0-9]+    { SAVE_TOKEN; return TINTEGER; }
.         printf("mystery character: %s\n", yytext);


%%