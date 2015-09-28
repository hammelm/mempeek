%{
#include <iostream>

using namespace std;

extern int yylex();
void yyerror( const char* s) { cerr << "ERROR: " << s << endl; }
%}

%token T_DEF T_FROM
%token T_PEEK
%token T_POKE T_MASK
%token T_EXIT
%token T_HALT

%token T_8BIT T_16BIT T_32BIT T_64BIT

%token T_DOT T_IDENTIFIER T_CONSTANT T_STRING

%start block

%%

block : statement
      | statement block 

statement : def_stmt
          | peek_stmt
          ;

def_stmt : T_DEF identifier expression
         | T_DEF T_IDENTIFIER expression T_FROM T_IDENTIFIER
         ;

peek_stmt : T_PEEK identifier
          | T_PEEK size_suffix identifier;

size_suffix : T_8BIT | T_16BIT | T_32BIT | T_64BIT
            ;

expression : T_CONSTANT
           | identifier
           ;

identifier : T_IDENTIFIER
           | T_IDENTIFIER T_DOT T_IDENTIFIER
           ;

%%
