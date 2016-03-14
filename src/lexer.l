/*  Copyright (c) 2015, Martin Hammel
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.
    
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
"defproc"               TOKEN( T_DEFPROC )
"endproc"               TOKEN( T_ENDPROC )
"deffunc"               TOKEN( T_DEFFUNC )
"endfunc"               TOKEN( T_ENDFUNC )
"exit"                  TOKEN( T_EXIT )
"global"                TOKEN( T_GLOBAL )
"static"                TOKEN( T_STATIC )
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
"float"                 TOKEN( T_FLOAT )
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
","                     TOKEN( ',' )
"["                     TOKEN( '[' )
"]"                     TOKEN( ']' )
"("                     TOKEN( '(' )
")"                     TOKEN( ')' )

[a-zA-Z][a-zA-Z0-9_]*   TOKEN( T_IDENTIFIER )
"0x"[0-9a-fA-F]+        TOKEN( T_CONSTANT )
"0b"[01]+               TOKEN( T_CONSTANT )
[0-9]+                  TOKEN( T_CONSTANT )
\"[^"]*\"               TOKEN( T_STRING )

[+-]?[0-9]*\.[0-9]+                     TOKEN( T_FCONST )
[+-]?[0-9]*\.?[0-9]+([eE][+-]?[0-9]+)?  TOKEN( T_FCONST )

.                       printf( "unknown token\n" ); yyterminate();

%%
