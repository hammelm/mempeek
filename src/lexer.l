%{
#include "parser.hpp"

#define TOKEN( t ) yylval.value = yytext; yylval.token = t; return t;
%}

%option noyywrap

%%

[ \t]                   ;
\\[ \t]*\n              ;

"\n"                    TOKEN( T_END_OF_STATEMENT )
";"                     TOKEN( T_END_OF_STATEMENT )

"def"                   TOKEN( T_DEF )
"from"                  TOKEN( T_FROM )
"peek"                  TOKEN( T_PEEK )
"poke"                  TOKEN( T_POKE )
"mask"                  TOKEN( T_MASK )
"while"                 TOKEN( T_WHILE )
"do"                    TOKEN( T_DO )
"endwhile"              TOKEN( T_ENDWHILE )
"exit"                  TOKEN( T_EXIT )
"halt"                  TOKEN( T_HALT )

":8"                    TOKEN( T_8BIT )
":16"                   TOKEN( T_16BIT )
":32"                   TOKEN( T_32BIT )
":64"                   TOKEN( T_64BIT )

"."                     TOKEN( '.' )
"["                     TOKEN( '[' )
"]"                     TOKEN( ']' )

[a-zA-Z][a-zA-Z0-9_]*   TOKEN( T_IDENTIFIER )
"0x"[0-9a-fA-F]+        TOKEN( T_CONSTANT )
"0b"[01]+               TOKEN( T_CONSTANT )
[0-9]+                  TOKEN( T_CONSTANT )
\".*\"                  TOKEN( T_STRING )

.                       printf( "unknown token\n" ); yyterminate();

%%
