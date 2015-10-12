%{
#include "mempeek_parser.h"
#include "parser.h"

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; yylloc->file = yyget_extra( yyscanner );

#define TOKEN( t ) yylval_param->value = yytext; yylval_param->token = t; return t;
%}

%option noyywrap
%option yylineno
%option reentrant bison-bridge bison-locations
%option extra-type="const char*"

%%

^[ \t]*("#".*)?\n       ; /* ignore empty lines */
"#".*$                  ; /* ignore comments */
[ \t]                   ; /* skip whitespace */
\\[ \t]*\n              ; /* wrap lines with backslash */

"\n"                    TOKEN( T_END_OF_STATEMENT )
";"                     TOKEN( T_END_OF_STATEMENT )

"def"                   TOKEN( T_DEF )
"map"                   TOKEN( T_MAP )
"from"                  TOKEN( T_FROM )
"import"                TOKEN( T_IMPORT )
"peek"                  TOKEN( T_PEEK )
"poke"                  TOKEN( T_POKE )
"mask"                  TOKEN( T_MASK )
"if"                    TOKEN( T_IF )
"then"                  TOKEN( T_THEN )
"else"                  TOKEN( T_ELSE )
"endif"                 TOKEN( T_ENDIF )
"while"                 TOKEN( T_WHILE )
"do"                    TOKEN( T_DO )
"endwhile"              TOKEN( T_ENDWHILE )
"for"                   TOKEN( T_FOR )
"to"                    TOKEN( T_TO )
"step"                  TOKEN( T_STEP )
"endfor"                TOKEN( T_ENDFOR )
"print"                 TOKEN( T_PRINT )
"dec"                   TOKEN( T_DEC )
"hex"                   TOKEN( T_HEX )
"bin"                   TOKEN( T_BIN )
"neg"                   TOKEN( T_NEG )
"noendl"                TOKEN( T_NOENDL )
"sleep"                 TOKEN( T_SLEEP )
"break"                 TOKEN( T_BREAK )
"quit"                  TOKEN( T_QUIT )

":8"                    TOKEN( T_8BIT )
":16"                   TOKEN( T_16BIT )
":32"                   TOKEN( T_32BIT )
":64"                   TOKEN( T_64BIT )

":="                    TOKEN( T_ASSIGN );

"+"                     TOKEN( T_PLUS )
"-"                     TOKEN( T_MINUS )
"*"                     TOKEN( T_MUL )
"/"                     TOKEN( T_DIV )
"%"                     TOKEN( T_MOD )
"<<"                    TOKEN( T_LSHIFT )
">>"                    TOKEN( T_RSHIFT )

"&"                     TOKEN( T_BIT_AND )
"|"                     TOKEN( T_BIT_OR )
"^"                     TOKEN( T_BIT_XOR )
"~"                     TOKEN( T_BIT_NOT )

"&&"                    TOKEN( T_LOG_AND )
"||"                    TOKEN( T_LOG_OR )
"^^"                    TOKEN( T_LOG_XOR )
"!"                     TOKEN( T_LOG_NOT )

"<"                     TOKEN( T_LT )
">"                     TOKEN( T_GT )
"<="                    TOKEN( T_LE )
">="                    TOKEN( T_GE )
"=="                    TOKEN( T_EQ )
"!="                    TOKEN( T_NE )

"."                     TOKEN( '.' )
"["                     TOKEN( '[' )
"]"                     TOKEN( ']' )
"("                     TOKEN( '(' )
")"                     TOKEN( ')' )

[a-zA-Z][a-zA-Z0-9_]*   TOKEN( T_IDENTIFIER )
"0x"[0-9a-fA-F]+        TOKEN( T_CONSTANT )
"0b"[01]+               TOKEN( T_CONSTANT )
[0-9]+                  TOKEN( T_CONSTANT )
\"[^"]*\"               TOKEN( T_STRING )

.                       printf( "unknown token\n" ); yyterminate();

%%
