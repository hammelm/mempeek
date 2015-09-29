%{

#include <iostream>
#include <stdio.h>

#include "mempeek_ast.h"

using namespace std;

extern int yylex();
void yyerror( const char* s) { cerr << "ERROR: " << s << endl; }

ASTNode* yyroot = nullptr;

%}

%code requires {

#include <string>

class ASTNode;

typedef struct {
    std::string value = "";
    int token = 0;
    ASTNode* node = nullptr;
} yyvalue_t;

extern ASTNode* yyroot; 

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

%start start

%%

start : block                                           { yyroot = $1.node; }
      ;

block : statement                                       { $$.node = new ASTNodeBlock; $$.node->push_back( $1.node ); }
      | block statement                                 { $$.node = $1.node; $$.node->push_back( $2.node ); }
      ;

statement : def_stmt T_END_OF_STATEMENT                 { $$.node = $1.node; }
          | poke_stmt T_END_OF_STATEMENT                { $$.node = $1.node; }
          ;

def_stmt : T_DEF plain_identifier expression                            { $$.node = new ASTNodeDef( $2.value, $3.node ); }
         | T_DEF struct_identifier expression                           { $$.node = new ASTNodeDef( $2.value, $3.node ); }
         | T_DEF plain_identifier expression T_FROM plain_identifier    { $$.node = new ASTNodeDef( $2.value, $3.node, $5.value ); }
         ;

poke_stmt : poke_token expression expression                        { $$.node = new ASTNodePoke( $2.node, $3.node, $1.token ); }
          | poke_token expression expression T_MASK expression      { $$.node = new ASTNodePoke( $2.node, $3.node, $5.node, $1.token ); }
          ;

poke_token : T_POKE                                     { $$.token = ASTNode::get_default_size(); }
           | T_POKE size_suffix                         { $$.token = $2.token; }
           ;

size_suffix : T_8BIT                                    { $$.token = $1.token; }
            | T_16BIT                                   { $$.token = $1.token; }
            | T_32BIT                                   { $$.token = $1.token; }
            | T_64BIT                                   { $$.token = $1.token; }
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

plain_identifier : T_IDENTIFIER                         { $$.value = $1.value; }
                 ;
%%
