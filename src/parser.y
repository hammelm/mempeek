%{

#include <iostream>
#include <stdio.h>

#include "mempeek_ast.h"

using namespace std;

extern int yylex();
void yyerror( const char* s) { cerr << "ERROR: " << s << endl; }

%}

%code requires {

#include <string>

class ASTNode;

typedef struct {
    std::string value = "";
    int token = 0;
    ASTNode* node = nullptr;
} yyvalue_t;

}

%define api.value.type { yyvalue_t }

%token T_DEF T_FROM
%token T_PEEK
%token T_POKE T_MASK
%token T_EXIT
%token T_HALT

%token T_8BIT T_16BIT T_32BIT T_64BIT

%token T_IDENTIFIER T_CONSTANT T_STRING

%token T_END_OF_STATEMENT

%start block

%debug

%%

block : statement
      | block statement

statement : def_stmt T_END_OF_STATEMENT
          | poke_stmt T_END_OF_STATEMENT
          ;

def_stmt : T_DEF plain_identifier expression                            { $$.node = new ASTNodeDef( $2.value, $3.node ); }
         | T_DEF struct_identifier expression                           { $$.node = new ASTNodeDef( $2.value, $3.node ); }
         | T_DEF plain_identifier expression T_FROM plain_identifier    { $$.node = new ASTNodeDef( $2.value, $3.node, $5.value ); }
         ;

poke_stmt : poke_token expression expression
          | poke_token expression expression T_MASK expression
          ;

poke_token : T_POKE
           | T_POKE size_suffix
           ;

size_suffix : T_8BIT
            | T_16BIT
            | T_32BIT
            | T_64BIT
            ;

expression : T_CONSTANT                                 { $$.node = new ASTNodeConstant( $1.value ); }
           | identifier                                 { $$.node = $1.node; }
           ;

identifier : index_identifier                           { $$.node = $1.node; }
           | index_identifier size_suffix               { $$.node = new ASTNodeRestriction( $1.node, $2.token ); }
           ;

index_identifier : base_identifier                      { $$.node = new ASTNodeVar( $1.value ); }
                 | base_identifier '[' expression ']'   { $$.node = new ASTNodeVar( $1.value, $3.node ); }
                 ;

base_identifier : struct_identifier                     { $$.value = $1.value; }
                | plain_identifier                      { $$.value = $1.value; }
                ;

struct_identifier : T_IDENTIFIER '.' T_IDENTIFIER       { $$.value = $1.value + '.' + $3.value; }
                  ;

plain_identifier : T_IDENTIFIER
                 ;
%%
