%{
#include <iostream>
#include <stdio.h>

using namespace std;

extern int yylex();
void yyerror( const char* s) { cerr << "ERROR: " << s << endl; }

int cnt = 0;
%}

%glr-parser

%token T_DEF T_FROM
%token T_PEEK
%token T_POKE T_MASK
%token T_EXIT
%token T_HALT

%token T_8BIT T_16BIT T_32BIT T_64BIT
 
%token T_IDENTIFIER T_CONSTANT T_STRING

%token T_END_OF_STATEMENT

%start block

%%

block : statement														{ $$ = cnt++; printf( "%d: block1 < %d >\n", $$, $1 ); }
      | block statement													{ $$ = cnt++; printf( "%d: block2 < %d %d >\n", $$, $1, $2 ); }

statement : def_stmt T_END_OF_STATEMENT									{ $$ = cnt++; printf( "%d: stmt1 < %d >\n", $$, $1 ); }
          | poke_stmt T_END_OF_STATEMENT								{ $$ = cnt++; printf( "%d: stmt2 < %d >\n", $$, $1 ); }
          ;

def_stmt : T_DEF plain_identifier expression							{ $$ = cnt++; printf( "%d: def1 < %d %d >\n", $$, $2, $3 ); }
		 | T_DEF struct_identifier expression							{ $$ = cnt++; printf( "%d: def2 < %d %d >\n", $$, $2, $3 ); }
         | T_DEF plain_identifier expression T_FROM plain_identifier	{ $$ = cnt++; printf( "%d: def3 < %d %d %d >\n", $$, $2, $3, $5 ); }
         ;

poke_stmt : poke_token expression expression							{ $$ = cnt++; printf( "%d: poke1 < %d %d %d >\n", $$, $1, $2, $3 ); }
          | poke_token expression expression T_MASK expression			{ $$ = cnt++; printf( "%d: poke2 < %d %d %d %d >\n", $$, $1, $2, $3, $5 ); }
          ;

poke_token : T_POKE														{ $$ = cnt++; printf( "%d: pokecmd1 <>\n", $$ ); }
           | T_POKE size_suffix											{ $$ = cnt++; printf( "%d: pokecmd2 < %d >\n", $$, $1 ); }
           ;

size_suffix : T_8BIT													{ $$ = cnt++; printf( "%d: 8bit <>\n", $$ ); }
		    | T_16BIT													{ $$ = cnt++; printf( "%d: 16bit <>\n", $$ ); }
		    | T_32BIT													{ $$ = cnt++; printf( "%d: 32bit <>\n", $$ ); }
		    | T_64BIT													{ $$ = cnt++; printf( "%d: 64bit <>\n", $$ ); }
            ;

expression : T_CONSTANT													{ $$ = cnt++; printf( "%d: expr1 <>\n", $$ ); }
           | identifier													{ $$ = cnt++; printf( "%d: expr2 < %d >\n", $$, $1 ); }
           ;

identifier : index_identifier											{ $$ = cnt++; printf( "%d: id1 < %d >\n", $$, $1 ); }
           | index_identifier size_suffix								{ $$ = cnt++; printf( "%d: id2 < %d %d >\n", $$, $1, $2 ); }
           ;

index_identifier : base_identifier										{ $$ = cnt++; printf( "%d: index1 < %d >\n", $$, $1 ); }
                 | base_identifier '[' expression ']'					{ $$ = cnt++; printf( "%d: index2 < %d %d >\n", $$, $1, $3 ); }
                 ; 

base_identifier : struct_identifier										{ $$ = cnt++; printf( "%d: base1 < %d >\n", $$, $1 ); }
				| plain_identifier										{ $$ = cnt++; printf( "%d: base2 < %d >\n", $$, $1 ); }
				;

struct_identifier : T_IDENTIFIER '.' T_IDENTIFIER						{ $$ = cnt++; printf( "%d: struct <>\n", $$ ); }
                  ;

plain_identifier : T_IDENTIFIER											{ $$ = cnt++; printf( "%d: struct <>\n", $$ ); }
                 ;
%%
