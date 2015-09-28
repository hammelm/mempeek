%{
#include "parser.hpp"

#define TOKEN( t ) printf( "\"%s\": token " #t "\n", yytext ); return t;
%}

%option noyywrap

%%

[ \t\n]                 ;

"def"                   TOKEN( T_DEF )
"from"                  TOKEN( T_FROM )
"peek"                  TOKEN( T_PEEK )
"poke"                  TOKEN( T_POKE )
"mask"                  TOKEN( T_MASK )
"exit"                  TOKEN( T_EXIT )
"halt"                  TOKEN( T_HALT )

":8"                    TOKEN( T_8BIT )
":16"                   TOKEN( T_16BIT )
":32"                   TOKEN( T_32BIT )
":64"                   TOKEN( T_64BIT )

"."                     TOKEN( T_DOT )

[a-zA-Z][a-zA-Z0-9_]*   TOKEN( T_IDENTIFIER )
"0x"[0-9a-fA-F]+        TOKEN( T_CONSTANT )
"0b"[01]+               TOKEN( T_CONSTANT )
[0-9]+                  TOKEN( T_CONSTANT )
\".*\"                  TOKEN( T_STRING )

%%
