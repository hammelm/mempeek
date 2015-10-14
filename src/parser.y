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

#include <iostream>
#include <stdio.h>

#include "mempeek_parser.h"
#include "mempeek_exceptions.h"
#include "mempeek_ast.h"

using namespace std;

int yylex( yyvalue_t*, YYLTYPE*, yyscan_t );

void yyerror( YYLTYPE* yylloc, yyscan_t, yynodeptr_t&, const char* ) { throw ASTExceptionSyntaxError( yylloc ); }

%}

%define api.value.type { yyvalue_t }
%define api.pure full
%parse-param { yyscan_t scanner } { yynodeptr_t& yyroot }
%lex-param { yyscan_t scanner }

%token T_DEF T_FROM
%token T_MAP
%token T_IMPORT
%token T_PEEK
%token T_POKE T_MASK
%token T_IF T_THEN T_ELSE T_ENDIF
%token T_WHILE T_DO T_ENDWHILE
%token T_FOR T_TO T_STEP T_ENDFOR
%token T_PRINT T_DEC T_HEX T_BIN T_NEG T_NOENDL
%token T_SLEEP
%token T_BREAK T_QUIT

%token T_BIT_NOT T_LOG_NOT T_BIT_AND T_LOG_AND T_BIT_XOR T_LOG_XOR T_BIT_OR T_LOG_OR 
%token T_LSHIFT T_RSHIFT T_PLUS T_MINUS T_MUL T_DIV T_MOD
%token T_LT T_GT T_LE T_GE T_EQ T_NE
%token T_ASSIGN

%token T_8BIT T_16BIT T_32BIT T_64BIT

%token T_IDENTIFIER T_CONSTANT T_STRING

%token T_END_OF_STATEMENT

%start start

%%

start : toplevel_block                                  { yyroot = $1.node; (void)@$; /* enable location tracking */ }
      ;

toplevel_block : toplevel_statement                     { $$.node = make_shared<ASTNodeBlock>( &yylloc ); $$.node->add_child( $1.node ); }
               | toplevel_block toplevel_statement      { $$.node = $1.node; $$.node->add_child( $2.node ); }
               ;

block : statement                                       { $$.node = make_shared<ASTNodeBlock>( &yylloc ); $$.node->add_child( $1.node ); }
      | block statement                                 { $$.node = $1.node; $$.node->add_child( $2.node ); }
      ;

toplevel_statement : statement                          { $$.node = $1.node; }
                   | map_stmt T_END_OF_STATEMENT        { $$.node = $1.node; }
                   | def_stmt T_END_OF_STATEMENT        { $$.node = $1.node; }
                   | import_stmt T_END_OF_STATEMENT     { $$.node = $1.node; }
                   ;

statement : assign_stmt T_END_OF_STATEMENT              { $$.node = $1.node; } 
          | poke_stmt T_END_OF_STATEMENT                { $$.node = $1.node; }
          | print_stmt T_END_OF_STATEMENT               { $$.node = $1.node; }
          | sleep_stmt T_END_OF_STATEMENT               { $$.node = $1.node; }
          | T_BREAK T_END_OF_STATEMENT                  { $$.node = make_shared<ASTNodeBreak>( &yylloc, T_BREAK ); }
          | T_QUIT T_END_OF_STATEMENT                   { $$.node = make_shared<ASTNodeBreak>( &yylloc, T_QUIT ); }
          | if_block                                    { $$.node = $1.node; }
          | while_block                                 { $$.node = $1.node; }
          | for_block                                   { $$.node = $1.node; }
          ;

if_block : if_def statement                                             { $$.node = make_shared<ASTNodeIf>( &yylloc, $1.node, $2.node ); }
         | if_def statement else_def                                    { $$.node = make_shared<ASTNodeIf>( &yylloc, $1.node, $2.node, $3.node ); }
         | if_def T_END_OF_STATEMENT block T_ENDIF T_END_OF_STATEMENT   { $$.node = make_shared<ASTNodeIf>( &yylloc, $1.node, $3.node ); }
         | if_def T_END_OF_STATEMENT block else_def                     { $$.node = make_shared<ASTNodeIf>( &yylloc, $1.node, $3.node, $4.node ); }
         ;

if_def : T_IF expression T_THEN                         { $$.node = $2.node; }
       ;

else_def : T_ELSE statement                                             { $$.node = $2.node; }
         | T_ELSE T_END_OF_STATEMENT block T_ENDIF T_END_OF_STATEMENT   { $$.node = $3.node; }
         ; 

while_block : T_WHILE expression T_DO statement         { $$.node = make_shared<ASTNodeWhile>( &yylloc, $2.node, $4.node ); }
            | T_WHILE expression T_DO T_END_OF_STATEMENT
                  block
              T_ENDWHILE T_END_OF_STATEMENT             { $$.node = make_shared<ASTNodeWhile>( &yylloc, $2.node, $5.node ); }
            ;

for_block : for_def statement                           { $$.node = $1.node; $$.node->add_child( $2.node ); }
          | for_def T_END_OF_STATEMENT
                block
            T_ENDFOR T_END_OF_STATEMENT                 { $$.node = $1.node; $$.node->add_child( $3.node ); }
          ;

for_def : T_FOR plain_identifier T_FROM expression T_TO expression T_DO                     { $$.node = make_shared<ASTNodeFor>( &yylloc, make_shared<ASTNodeAssign>( &yylloc, $2.value, $4.node ), $6.node ); }
        | T_FOR plain_identifier T_FROM expression T_TO expression T_STEP expression T_DO   { $$.node = make_shared<ASTNodeFor>( &yylloc, make_shared<ASTNodeAssign>( &yylloc, $2.value, $4.node ), $6.node, $8.node ); }
        ;

assign_stmt : plain_identifier T_ASSIGN expression      { $$.node = make_shared<ASTNodeAssign>( &yylloc, $1.value, $3.node ); }

def_stmt : T_DEF plain_identifier expression                            { $$.node = make_shared<ASTNodeDef>( &yylloc, $2.value, $3.node ); }
         | T_DEF struct_identifier expression                           { $$.node = make_shared<ASTNodeDef>( &yylloc, $2.value, $3.node ); }
         | T_DEF plain_identifier expression T_FROM plain_identifier    { $$.node = make_shared<ASTNodeDef>( &yylloc, $2.value, $3.node, $5.value ); }
         ;

map_stmt : T_MAP T_CONSTANT T_CONSTANT                  { $$.node = make_shared<ASTNodeMap>( &yylloc, $2.value, $3.value ); }
         | T_MAP T_CONSTANT T_CONSTANT T_STRING         { $$.node = make_shared<ASTNodeMap>( &yylloc, $2.value, $3.value, $4.value.substr( 1, $4.value.length() - 2 ) ); }
         ;

import_stmt : T_IMPORT T_STRING                         { $$.node = make_shared<ASTNodeImport>( &yylloc, $2.value.substr( 1, $2.value.length() - 2 ) ); }
            ;

poke_stmt : poke_token expression expression                        { $$.node = make_shared<ASTNodePoke>( &yylloc, $2.node, $3.node, $1.token ); }
          | poke_token expression expression T_MASK expression      { $$.node = make_shared<ASTNodePoke>( &yylloc, $2.node, $3.node, $5.node, $1.token ); }
          ;

poke_token : T_POKE                                     { $$.token = ASTNode::get_default_size(); }
           | T_POKE size_suffix                         { $$.token = $2.token; }
           ;

size_suffix : T_8BIT                                    { $$.token = $1.token; }
            | T_16BIT                                   { $$.token = $1.token; }
            | T_32BIT                                   { $$.token = $1.token; }
            | T_64BIT                                   { $$.token = $1.token; }
            ;

print_stmt : T_PRINT print_args                         { $$.node = $2.node; $$.node->add_child( make_shared<ASTNodePrint>( &yylloc ) ); }
           | T_PRINT print_args T_NOENDL                { $$.node = $2.node; }
           ;

print_args : %empty                                     { $$.node = make_shared<ASTNodeBlock>( &yylloc ); $$.token = ASTNodePrint::MOD_HEX | ASTNodePrint::get_default_size(); }
           | print_args print_format                    { $$.node = $1.node; $$.token = $2.token | ASTNodePrint::get_default_size(); }
           | print_args print_format print_size         { $$.node = $1.node; $$.token = $2.token | $3.token; }
           | print_args expression                      { $$.node = $1.node; $$.token = $1.token; $$.node->add_child( make_shared<ASTNodePrint>( &yylloc, $2.node, $$.token ) ); }
           | print_args T_STRING                        { $$.node = $1.node; $$.token = $1.token; $$.node->add_child( make_shared<ASTNodePrint>( &yylloc, $2.value.substr( 1, $2.value.length() - 2 ) ) ); }
           ;

print_format : T_DEC                                    { $$.token = ASTNodePrint::MOD_DEC; }
             | T_HEX                                    { $$.token = ASTNodePrint::MOD_HEX; }
             | T_BIN                                    { $$.token = ASTNodePrint::MOD_BIN; }
             | T_NEG                                    { $$.token = ASTNodePrint::MOD_NEG; }
             ;

print_size : T_8BIT                                     { $$.token = ASTNodePrint::MOD_8BIT; }
           | T_16BIT                                    { $$.token = ASTNodePrint::MOD_16BIT; }
           | T_32BIT                                    { $$.token = ASTNodePrint::MOD_32BIT; }
           | T_64BIT                                    { $$.token = ASTNodePrint::MOD_64BIT; }
           ;

sleep_stmt : T_SLEEP expression                         { $$.node = make_shared<ASTNodeSleep>( &yylloc, $2.node ); }
           ;

expression : expression T_LOG_OR and_expr               { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
           | expression T_LOG_XOR and_expr              { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
           | and_expr                                   { $$.node = $1.node; }
           ;

and_expr : and_expr T_LOG_AND comp_expr                 { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
         | comp_expr                                    { $$.node = $1.node; }
         ;

comp_expr : add_expr T_LT add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
          | add_expr T_GT add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
          | add_expr T_LE add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
          | add_expr T_GE add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
          | add_expr T_EQ add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
          | add_expr T_NE add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
          | add_expr                                    { $$.node = $1.node; }
          ;

add_expr : add_expr T_PLUS mul_expr                     { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
         | add_expr T_MINUS mul_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
         | add_expr T_BIT_OR mul_expr                   { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
         | add_expr T_BIT_XOR mul_expr                  { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
         | mul_expr                                     { $$.node = $1.node; }
         ;

mul_expr : mul_expr T_MUL shift_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
         | mul_expr T_DIV shift_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); }
         | mul_expr T_MOD shift_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); }
         | mul_expr T_BIT_AND shift_expr                { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); }
         | shift_expr                                   { $$.node = $1.node; }
         ; 

shift_expr : shift_expr T_LSHIFT unary_expr             { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
           | shift_expr T_RSHIFT unary_expr             { $$.node = make_shared<ASTNodeBinaryOperator>( &yylloc, $1.node, $3.node, $2.token ); } 
           | unary_expr                                 { $$.node = $1.node; }
           ;

unary_expr : T_MINUS atomic_expr                        { $$.node = make_shared<ASTNodeUnaryOperator>( &yylloc, $2.node, $1.token ); }
           | T_BIT_NOT atomic_expr                      { $$.node = make_shared<ASTNodeUnaryOperator>( &yylloc, $2.node, $1.token ); }
           | T_LOG_NOT atomic_expr                      { $$.node = make_shared<ASTNodeUnaryOperator>( &yylloc, $2.node, $1.token ); }
           | atomic_expr                                { $$.node = $1.node; }
           ;

atomic_expr : T_CONSTANT                                { $$.node = make_shared<ASTNodeConstant>( &yylloc, $1.value ); }
            | identifier                                { $$.node = $1.node; }
            | '(' expression ')'                        { $$.node = $2.node; }
            | peek_token '(' expression ')'             { $$.node = make_shared<ASTNodePeek>( &yylloc, $3.node, $1.token ); }
            ;

peek_token : T_PEEK                                     { $$.token = ASTNode::get_default_size(); }
           | T_PEEK size_suffix                         { $$.token = $2.token; }
           ;

identifier : index_identifier                           { $$.node = $1.node; }
           | index_identifier size_suffix               { $$.node = make_shared<ASTNodeRestriction>( &yylloc, $1.node, $2.token ); }
           ;

index_identifier : base_identifier                      { $$.node = make_shared<ASTNodeVar>( &yylloc, $1.value ); }
                 | base_identifier '[' expression ']'   { $$.node = make_shared<ASTNodeVar>( &yylloc, $1.value, $3.node ); }
                 ;

base_identifier : struct_identifier                     { $$.value = $1.value; }
                | plain_identifier                      { $$.value = $1.value; }
                ;

struct_identifier : T_IDENTIFIER '.' T_IDENTIFIER       { $$.value = $1.value + '.' + $3.value; }
                  ;

plain_identifier : T_IDENTIFIER
                 ;
%%
