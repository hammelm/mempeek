/*  Copyright (c) 2015-2018, Martin Hammel
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
#include <utility>
#include <stdio.h>

#include "mempeek_parser.h"
#include "mempeek_ast.h"

using namespace std;

static size_t arrayarg_counter = 0;
static size_t arrayarg_depth = 0;

static ASTNodePrint::ptr printnode = nullptr;
 
int yylex( yyvalue_t*, YYLTYPE*, yyscan_t );

void yyerror( YYLTYPE* yylloc, yyscan_t, yyenv_t, yynodeptr_t&, const char* ) { throw ASTExceptionSyntaxError( *yylloc ); }

void yyarg_start()
{
    arrayarg_depth++;
}

void yyarg_end()
{
    if( --arrayarg_depth == 0 ) arrayarg_counter = 0; 
}

arglist_t::value_type yyfuncarg( const yylloc_t& location, Environment* env, std::string func, const arglist_t& args )
{
	try {
		ASTNode::ptr node = env->get_function( location, func, args );
		return make_pair( node, string( "" ) );
	}
	catch( ASTExceptionNamingConflict& ) {
		string result = "@arg" + to_string( arrayarg_counter++ );
		env->alloc_array( result );
		ASTNode::ptr node = env->get_arrayfunc( location, func, result, args );
		return make_pair( node, result );
	}
}

arglist_t::value_type yystringarg( const yylloc_t& location, Environment* env, std::string value )
{
    string result = "@arg" + to_string( arrayarg_counter++ );
    env->alloc_array( result );
    ASTNode::ptr node = make_shared<ASTNodeString>( location, env, result, value );
    return make_pair( node, result );
}

%}

%define api.value.type { yyvalue_t }
%define api.pure full
%parse-param { yyscan_t scanner } { yyenv_t env } { yynodeptr_t& yyroot }
%lex-param { yyscan_t scanner }

%token T_DEF T_FROM
%token T_DIM
%token T_MAP T_AT
%token T_DEFPROC T_ENDPROC
%token T_DEFFUNC T_ENDFUNC
%token T_ELLIPSIS T_ARGS
%token T_DROP
%token T_EXIT T_GLOBAL T_STATIC
%token T_IMPORT T_RUN
%token T_PEEK
%token T_POKE T_MASK
%token T_IF T_THEN T_ELSE T_ENDIF
%token T_WHILE T_DO T_ENDWHILE
%token T_FOR T_TO T_STEP T_ENDFOR
%token T_PRINT T_DEC T_HEX T_BIN T_NEG T_FLOAT T_ARRAY T_STRING T_NOENDL
%token T_SLEEP T_UNTIL T_NOW
%token T_BREAK T_QUIT
%token T_PRAGMA T_WORDSIZE T_LOADPATH

%token T_BIT_NOT T_LOG_NOT T_BIT_AND T_LOG_AND T_BIT_XOR T_LOG_XOR T_BIT_OR T_LOG_OR
%token T_LSHIFT T_RSHIFT T_PLUS T_MINUS T_MUL T_DIV T_MOD T_SDIV T_SMOD
%token T_LT T_GT T_LE T_GE T_EQ T_NE T_SLT T_SGT T_SLE T_SGE
%token T_ASSIGN

%token T_8BIT T_16BIT T_32BIT T_64BIT

%token T_IDENTIFIER T_CONSTANT T_FCONST T_SCONST

%token T_END_OF_STATEMENT 0

%start start

%%


/*****************************************************************************
 * blocks and statements
 ****************************************************************************/

start : toplevel_block                                  { yyroot = $1.node; }
      ;

toplevel_block : toplevel_statement                     { $$.node = make_shared<ASTNodeBlock>( @$ ); $$.node->add_child( $1.node ); }
               | toplevel_block toplevel_statement      { $$.node = $1.node; $$.node->add_child( $2.node ); }
               ;

block : statement                                       { $$.node = make_shared<ASTNodeBlock>( @$ ); $$.node->add_child( $1.node ); }
      | block statement                                 { $$.node = $1.node; $$.node->add_child( $2.node ); }
      ;

subroutine_block :                                          { $$.node = make_shared<ASTNodeBlock>( @$ ); env->set_subroutine_body( $$.node ); }
                   subroutine_statement                     { $$.node = $1.node; $$.node->add_child( $2.node ); }
                 | subroutine_block subroutine_statement    { $$.node = $1.node; $$.node->add_child( $2.node ); }
                 ;

toplevel_statement : statement                          { $$.node = $1.node; }
                   | pragma_stmt T_END_OF_STATEMENT     { $$.node = nullptr; }
                   | map_stmt T_END_OF_STATEMENT        { $$.node = $1.node; }
                   | def_stmt T_END_OF_STATEMENT        { $$.node = $1.node; }
                   | import_stmt T_END_OF_STATEMENT     { $$.node = $1.node; }
                   | proc_def
                   | func_def
                   | drop_stmt T_END_OF_STATEMENT
                   ;

statement : %empty                                          { $$.node = nullptr; }
          | dim_stmt T_END_OF_STATEMENT                     { $$.node = $1.node; }
          | assign_stmt T_END_OF_STATEMENT                  { $$.node = $1.node; }
          | poke_stmt T_END_OF_STATEMENT                    { $$.node = $1.node; }
          | print_stmt T_END_OF_STATEMENT                   { $$.node = $1.node; }
          | sleep_stmt T_END_OF_STATEMENT                   { $$.node = $1.node; }
          | T_EXIT T_END_OF_STATEMENT                       { $$.node = make_shared<ASTNodeBreak>( @1, T_EXIT ); }
          | T_BREAK T_END_OF_STATEMENT                      { $$.node = make_shared<ASTNodeBreak>( @1, T_BREAK ); }
          | T_QUIT T_END_OF_STATEMENT                       { $$.node = make_shared<ASTNodeBreak>( @1, T_QUIT ); }
          | if_block                                        { $$.node = $1.node; }
          | while_block                                     { $$.node = $1.node; }
          | for_block                                       { $$.node = $1.node; }
          | plain_identifier proc_args T_END_OF_STATEMENT   { $$.node = env->get_procedure( @1, $1.value, $2.arglist ); if( !$$.node ) throw ASTExceptionSyntaxError( @1 ); }
          ;

subroutine_statement : statement                        { $$.node = $1.node; }
                     | global_stmt                      { $$.node = $1.node; }
                     | static_stmt                      { $$.node = $1.node; }
                     ;


/*****************************************************************************
 * procedure and function definitions
 ****************************************************************************/

proc_def : T_DEFPROC plain_identifier                   { env->enter_subroutine_context( @1, $2.value, Environment::PROCEDURE ); }
               proc_arg_decl T_END_OF_STATEMENT
               subroutine_block
           T_ENDPROC                                    { env->commit_subroutine_context(); }
           T_END_OF_STATEMENT                           { $$.node = nullptr; }
         ;

func_def : T_DEFFUNC arrayfunc_def plain_identifier     { env->enter_subroutine_context( @1, $3.value, $2.token ? Environment::ARRAYFUNC : Environment::FUNCTION ); }
               '(' func_arg_decl ')' T_END_OF_STATEMENT
               subroutine_block
           T_ENDFUNC                                    { env->commit_subroutine_context(); }
           T_END_OF_STATEMENT                           { $$.node = nullptr; }
         ;

arrayfunc_def : %empty                                  { $$.token = 0; }
              | '[' ']'                                 { $$.token = 1; }
              ;

proc_arg_decl : proc_arg_list
              | proc_arg_list T_ELLIPSIS                { env->set_subroutine_varargs(); }
              | T_ELLIPSIS                              { env->set_subroutine_varargs(); }
              ;

proc_arg_list : %empty
              | proc_arg_list plain_identifier          { env->set_subroutine_param( $2.value ); }
              | proc_arg_list plain_identifier '[' ']'  { env->set_subroutine_param( $2.value, true ); }
              ;

func_arg_decl : func_arg_list
              | func_arg_list ',' T_ELLIPSIS            { env->set_subroutine_varargs(); }
              | T_ELLIPSIS                              { env->set_subroutine_varargs(); }
              ;

func_arg_list : %empty
              | plain_identifier                            { env->set_subroutine_param( $1.value ); }
              | plain_identifier '[' ']'                    { env->set_subroutine_param( $1.value, true ); }
              | func_arg_list ',' plain_identifier          { env->set_subroutine_param( $3.value ); }
              | func_arg_list ',' plain_identifier '[' ']'  { env->set_subroutine_param( $3.value, true ); }
              ;

proc_args :                                             { yyarg_start(); }
            proc_args_items                             { $$.arglist = std::move( $2.arglist ); yyarg_end(); }
          ;

proc_args_items : %empty                                { $$.arglist.clear(); }
                | proc_args_items subroutine_arg        { $$.arglist = std::move( $1.arglist ); $$.arglist.push_back( $2.arglist[0] ); }
                ;

func_args : %empty                                      { $$.arglist.clear(); }
          |                                             { yyarg_start(); }
            func_args_items                             { $$.arglist = std::move( $2.arglist ); yyarg_end(); }
          ;

func_args_items : subroutine_arg                        { $$.arglist = std::move( $1.arglist ); }
                | func_args_items ',' subroutine_arg    { $$.arglist = std::move( $1.arglist ); $$.arglist.push_back( $3.arglist[0] ); }
                ;

subroutine_arg : T_SCONST                               { $$.arglist.clear(); $$.arglist.push_back( yystringarg( @$, env, $1.value.substr( 1, $1.value.length() - 2 ) ) ); }
               | plain_identifier '(' func_args ')'     { $$.arglist.clear(); $$.arglist.push_back( yyfuncarg( @1, env, $1.value, $3.arglist ) ); }
               | plain_identifier '[' ']'               { $$.arglist.clear(); $$.arglist.push_back( make_pair( ASTNode::ptr(nullptr), $1.value ) ); }
               | expression                             { $$.arglist.clear(); $$.arglist.push_back( make_pair( $1.node, string("") ) ); }
               ;

global_stmt : T_GLOBAL plain_identifier T_END_OF_STATEMENT          { if( !env->alloc_global_var( $2.value ) ) throw ASTExceptionNamingConflict( @1, $2.value ); }
            | T_GLOBAL plain_identifier '[' ']' T_END_OF_STATEMENT  { if( !env->alloc_global_array( $2.value ) ) throw ASTExceptionNamingConflict( @1, $2.value ); }
            ;

static_stmt : T_STATIC plain_identifier
              T_ASSIGN expression T_END_OF_STATEMENT            { $$.node = make_shared<ASTNodeStatic>( @1, env, $2.value, $4.node, true ); }
            | T_STATIC plain_identifier '[' expression ']'
              T_END_OF_STATEMENT                                { $$.node = make_shared<ASTNodeStatic>( @1, env, $2.value, $4.node, false ); }
            | T_STATIC plain_identifier '[' ']'
              T_ASSIGN '[' comma_list ']' T_END_OF_STATEMENT    { $$.node = make_shared<ASTNodeStatic>( @1, env, $2.value ); for( auto arg: $7.arglist ) $$.node->add_child( arg.first ); }
            | T_STATIC plain_identifier '[' ']' T_ASSIGN
              plain_identifier '[' ']'	T_END_OF_STATEMENT      { $$.node = make_shared<ASTNodeStatic>( @1, env, $2.value, $6.value ); }
            ;

drop_stmt : T_DROP plain_identifier                     { if( !env->drop_procedure( $2.value ) ) throw ASTExceptionNamingConflict( @1, $2.value ); }
          | T_DROP plain_identifier '(' ')'             { if( !env->drop_function( $2.value ) ) throw ASTExceptionNamingConflict( @1, $2.value ); }
          ;


/*****************************************************************************
 * control flow structures
 ****************************************************************************/

if_block : if_def statement                                             { $$.node = make_shared<ASTNodeIf>( @$, $1.node, $2.node ); }
         | if_def statement else_def                                    { $$.node = make_shared<ASTNodeIf>( @$, $1.node, $2.node, $3.node ); }
         | if_def T_END_OF_STATEMENT block T_ENDIF T_END_OF_STATEMENT   { $$.node = make_shared<ASTNodeIf>( @$, $1.node, $3.node ); }
         | if_def T_END_OF_STATEMENT block else_def                     { $$.node = make_shared<ASTNodeIf>( @$, $1.node, $3.node, $4.node ); }
         ;

if_def : T_IF expression T_THEN                                         { $$.node = $2.node; }
       ;

else_def : T_ELSE statement                                             { $$.node = $2.node; }
         | T_ELSE T_END_OF_STATEMENT block T_ENDIF T_END_OF_STATEMENT   { $$.node = $3.node; }
         ;

while_block : T_WHILE expression T_DO statement         { $$.node = make_shared<ASTNodeWhile>( @$, $2.node, $4.node ); }
            | T_WHILE expression T_DO T_END_OF_STATEMENT
                  block
              T_ENDWHILE T_END_OF_STATEMENT             { $$.node = make_shared<ASTNodeWhile>( @$, $2.node, $5.node ); }
            ;

for_block : for_def statement                           { $$.node = $1.node; $$.node->add_child( $2.node ); }
          | for_def T_END_OF_STATEMENT
                block
            T_ENDFOR T_END_OF_STATEMENT                 { $$.node = $1.node; $$.node->add_child( $3.node ); }
          ;

for_def : T_FOR plain_identifier T_FROM expression T_TO expression T_DO                     { $$.node = make_shared<ASTNodeFor>( @$, make_shared<ASTNodeAssign>( @2, env, $2.value, $4.node ), $6.node ); }
        | T_FOR plain_identifier T_FROM expression T_TO expression T_STEP expression T_DO   { $$.node = make_shared<ASTNodeFor>( @$, make_shared<ASTNodeAssign>( @2, env, $2.value, $4.node ), $6.node, $8.node ); }
        ;


/*****************************************************************************
 * variables and arrays
 ****************************************************************************/

assign_stmt : plain_identifier T_ASSIGN expression          { $$.node = make_shared<ASTNodeAssign>( @$, env, $1.value, $3.node ); }
            | plain_identifier '[' expression ']'
              T_ASSIGN expression                           { $$.node = make_shared<ASTNodeAssign>( @$, env, $1.value, $3.node, $6.node ); }
            | plain_identifier '[' ']' T_ASSIGN T_SCONST    { $$.node = make_shared<ASTNodeString>( @$, env, $1.value, $5.value.substr( 1, $5.value.length() - 2 ) ); }
            | plain_identifier '[' ']'
              T_ASSIGN plain_identifier '[' ']'             { $$.node = make_shared<ASTNodeAssign>( @$, env, $1.value, $5.value ); }
            | plain_identifier '[' ']'
              T_ASSIGN '[' comma_list ']'                   { $$.node = make_shared<ASTNodeAssign>( @$, env, $1.value ); for( auto arg: $6.arglist ) $$.node->add_child( arg.first ); }
            | plain_identifier '[' ']'
              T_ASSIGN T_ARGS '{' expression '}' '[' ']'    { $$.node = make_shared<ASTNodeAssignArg>( @$, env, $1.value, $7.node ); }
            | plain_identifier '[' ']'
              T_ASSIGN plain_identifier '(' func_args ')'   { $$.node = env->get_arrayfunc( @1, $5.value, $1.value, $7.arglist ); if( !$$.node ) throw ASTExceptionSyntaxError( @1 ); }
            ;

def_stmt : T_DEF plain_identifier expression                                    { $$.node = make_shared<ASTNodeDef>( @$, env, $2.value, $3.node ); }
         | T_DEF struct_identifier expression                                   { $$.node = make_shared<ASTNodeDef>( @$, env, $2.value, $3.node ); }
         | T_DEF struct_identifier '{' expression '}' expression                { $$.node = make_shared<ASTNodeDef>( @$, env, $2.value, $4.node, $6.node, Environment::get_default_size() ); }
         | T_DEF struct_identifier size_suffix '{' expression '}' expression    { $$.node = make_shared<ASTNodeDef>( @$, env, $2.value, $5.node, $7.node, $3.token ); }
         | T_DEF plain_identifier expression T_FROM plain_identifier            { $$.node = make_shared<ASTNodeDef>( @$, env, $2.value, $3.node, $5.value ); }
         ;

dim_stmt : T_DIM plain_identifier '[' expression ']'    { $$.node = make_shared<ASTNodeDim>( @$, env, $2.value, $4.node ); }
         ;

comma_list : %empty                                     { $$.arglist.clear(); }
           | expression                                 { $$.arglist.clear(); $$.arglist.push_back( make_pair( $1.node, string("") ) ); }
           | comma_list ',' expression                  { $$.arglist = std::move( $1.arglist ); $$.arglist.push_back( make_pair( $3.node, string("") ) ); }
           ;


/*****************************************************************************
 * special functions
 ****************************************************************************/

map_stmt : T_MAP expression expression                  { $$.node = make_shared<ASTNodeMap>( @$, env, $2.node, $3.node ); }
         | T_MAP expression expression T_AT expression  { $$.node = make_shared<ASTNodeMap>( @$, env, $2.node, $5.node, $3.node ); }
         | T_MAP expression expression T_SCONST         { $$.node = make_shared<ASTNodeMap>( @$, env, $2.node, $3.node, $4.value.substr( 1, $4.value.length() - 2 ) ); }
         | T_MAP expression expression T_SCONST
           T_AT expression                              { $$.node = make_shared<ASTNodeMap>( @$, env, $2.node, $6.node, $3.node, $4.value.substr( 1, $4.value.length() - 2 ) ); }
         ;

pragma_stmt : T_PRAGMA T_PRINT print_array              { env->set_default_modifier( $3.token ); }
            | T_PRAGMA T_PRINT print_float              { env->set_default_modifier( $3.token | ASTNodePrint::MOD_64BIT ); }
            | T_PRAGMA T_PRINT print_format             { env->set_default_modifier( $3.token | ASTNodePrint::MOD_WORDSIZE ); }
            | T_PRAGMA T_PRINT print_format print_size  { env->set_default_modifier( $3.token | $4.token ); }
            | T_PRAGMA T_WORDSIZE T_CONSTANT            { if( !env->set_default_size( env->parse_int( $3.value ) ) ) throw ASTExceptionSyntaxError( @3 ); }
            | T_PRAGMA T_LOADPATH T_SCONST              { string path = $3.value.substr( 1, $3.value.length() - 2 ); if( !env->add_include_path( path ) ) throw ASTExceptionFileNotFound( @3, path.c_str() ); }
            ;

import_stmt : T_IMPORT T_SCONST                         { $$.node = make_shared<ASTNodeImport>( @$, env, $2.value.substr( 1, $2.value.length() - 2 ), true ); }
            | T_RUN T_SCONST                            { $$.node = make_shared<ASTNodeImport>( @$, env, $2.value.substr( 1, $2.value.length() - 2 ), false ); }
            ;

poke_stmt : poke_token expression expression                        { $$.node = make_shared<ASTNodePoke>( @$, env, $2.node, $3.node, $1.token ); }
          | poke_token expression expression T_MASK expression      { $$.node = make_shared<ASTNodePoke>( @$, env, $2.node, $3.node, $5.node, $1.token ); }
          ;

poke_token : T_POKE                                     { $$.token = Environment::get_default_size(); }
           | T_POKE size_suffix                         { $$.token = $2.token; }
           ;

size_suffix : T_8BIT                                    { $$.token = $1.token; }
            | T_16BIT                                   { $$.token = $1.token; }
            | T_32BIT                                   { $$.token = $1.token; }
            | T_64BIT                                   { $$.token = $1.token; }
            ;

print_stmt : print_stmt_endl                            { $$.node = printnode; }
           | print_stmt_endl T_NOENDL                   { $$.node = printnode; printnode->set_endl( false ); }
           ;

print_stmt_endl : T_PRINT                               { printnode = make_shared<ASTNodePrint>( @$ ); yyarg_start(); }
                  print_args                            { yyarg_end(); }
                ;

print_args : %empty                                         { $$.token = env->get_default_modifier(); }
           | print_args print_array                         { $$.token = ($1.token & ~ASTNodePrint::MOD_ARRAYMASK) | $2.token; }
           | print_args print_float                         { $$.token = ($1.token & ~ASTNodePrint::MOD_TYPESIZEMASK) | $2.token | ASTNodePrint::MOD_64BIT; }
           | print_args print_format                        { $$.token = ($1.token & ~ASTNodePrint::MOD_TYPESIZEMASK) | $2.token | ASTNodePrint::MOD_WORDSIZE; }
           | print_args print_format print_size             { $$.token = ($1.token & ~ASTNodePrint::MOD_TYPESIZEMASK) | $2.token | $3.token; }
           | print_args T_SCONST                            { $$.token = $1.token; printnode->add_arg( $2.value.substr( 1, $2.value.length() - 2 ) ); }
           | print_args plain_identifier '(' func_args ')'  { $$.token = $1.token; printnode->add_arg( yyfuncarg( @2, env, $2.value, $4.arglist ).first, $$.token ); }
           | print_args plain_identifier '[' ']'            { $$.token = $1.token; printnode->add_arg( make_shared<ASTNodeArray>( yylloc, env, $2.value ), $$.token ); }
           | print_args expression                          { $$.token = $1.token; printnode->add_arg( $2.node, $$.token & ~ASTNodePrint::MOD_ARRAYMASK ); }
           ;

print_array : T_ARRAY                                   { $$.token = ASTNodePrint::MOD_ARRAY; }
            | T_STRING                                  { $$.token = ASTNodePrint::MOD_STRING; }

print_float : T_FLOAT                                   { $$.token = ASTNodePrint::MOD_FLOAT; }
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

sleep_stmt : T_SLEEP expression                         { $$.node = make_shared<ASTNodeSleep>( @$, $2.node, false ); }
           | T_SLEEP T_UNTIL expression                 { $$.node = make_shared<ASTNodeSleep>( @$, $3.node, true ); }
           ;


/*****************************************************************************
 * expressions
 ****************************************************************************/

expression : expression T_LOG_OR and_expr               { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
           | expression T_LOG_XOR and_expr              { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
           | and_expr                                   { $$.node = $1.node; }
           ;

and_expr : and_expr T_LOG_AND comp_expr                 { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | comp_expr                                    { $$.node = $1.node; }
         ;

comp_expr : add_expr T_LT add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_GT add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_LE add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_GE add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_EQ add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_NE add_expr                      { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_SLT add_expr                     { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_SGT add_expr                     { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_SLE add_expr                     { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr T_SGE add_expr                     { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
          | add_expr                                    { $$.node = $1.node; }
          ;

add_expr : add_expr T_PLUS mul_expr                     { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | add_expr T_MINUS mul_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | add_expr T_BIT_OR mul_expr                   { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | add_expr T_BIT_XOR mul_expr                  { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | mul_expr                                     { $$.node = $1.node; }
         ;

mul_expr : mul_expr T_MUL shift_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | mul_expr T_DIV shift_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | mul_expr T_MOD shift_expr                    { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | mul_expr T_SDIV shift_expr                   { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | mul_expr T_SMOD shift_expr                   { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | mul_expr T_BIT_AND shift_expr                { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
         | shift_expr                                   { $$.node = $1.node; }
         ;

shift_expr : shift_expr T_LSHIFT unary_expr             { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
           | shift_expr T_RSHIFT unary_expr             { $$.node = make_shared<ASTNodeBinaryOperator>( @$, $1.node, $3.node, $2.token ); }
           | unary_expr                                 { $$.node = $1.node; }
           ;

unary_expr : T_MINUS atomic_expr                        { $$.node = make_shared<ASTNodeUnaryOperator>( @$, $2.node, $1.token ); }
           | T_BIT_NOT atomic_expr                      { $$.node = make_shared<ASTNodeUnaryOperator>( @$, $2.node, $1.token ); }
           | T_LOG_NOT atomic_expr                      { $$.node = make_shared<ASTNodeUnaryOperator>( @$, $2.node, $1.token ); }
           | atomic_expr                                { $$.node = $1.node; }
           ;

atomic_expr : T_CONSTANT                                { $$.node = make_shared<ASTNodeConstant>( @$, $1.value ); }
            | T_FCONST                                  { $$.node = make_shared<ASTNodeConstant>( @$, $1.value, true ); }
            | T_NOW                                     { $$.node = make_shared<ASTNodeSleep>( @$ ); }
            | var_identifier                            { $$.node = $1.node; }
            | '(' expression ')'                        { $$.node = $2.node; }
            | args_expr                                 { $$.node = $1.node; }
            | peek_token '(' expression ')'             { $$.node = make_shared<ASTNodePeek>( @$, env, $3.node, $1.token ); }
            | plain_identifier '(' func_args ')'        { $$.node = env->get_function( @1, $1.value, $3.arglist ); if( !$$.node ) throw ASTExceptionSyntaxError( @1 ); }
            ;

args_expr : T_ARGS '{' '?' '}'                              { $$.node = make_shared<ASTNodeArg>( @$, env ); }
          | T_ARGS '{' expression '}' '[' ']' '?'           { $$.node = make_shared<ASTNodeArg>( @$, env, $3.node, ASTNodeArg::GET_TYPE ); }
          | T_ARGS '{' expression '}'                       { $$.node = make_shared<ASTNodeArg>( @$, env, $3.node, ASTNodeArg::GET_VAR ); }
          | T_ARGS '{' expression '}' '[' '?' ']'           { $$.node = make_shared<ASTNodeArg>( @$, env, $3.node, ASTNodeArg::GET_ARRAYSIZE ); }
          | T_ARGS '{' expression '}' '[' expression ']'    { $$.node = make_shared<ASTNodeArg>( @$, env, $3.node, $6.node ); }
          ;

peek_token : T_PEEK                                     { $$.token = Environment::get_default_size(); }
           | T_PEEK size_suffix                         { $$.token = $2.token; }
           ;

var_identifier : plain_identifier                       { $$.node = make_shared<ASTNodeVar>( @$, env, $1.value ); }
               | struct_identifier                      { $$.node = make_shared<ASTNodeVar>( @$, env, $1.value ); }
               | struct_identifier '{' '?' '}'          { $$.node = make_shared<ASTNodeRange>( @$, env, $1.value ); }
               | struct_identifier '{' expression '}'   { $$.node = make_shared<ASTNodeRange>( @$, env, $1.value, $3.node ); }
               | array_identifier                       { $$.node = $1.node; }
               ;

array_identifier : plain_identifier '[' '?' ']'         { $$.node = make_shared<ASTNodeArray>( @$, env, $1.value ); }
                 | plain_identifier '[' expression ']'  { $$.node = make_shared<ASTNodeArray>( @$, env, $1.value, $3.node ); }
                 ;

struct_identifier : T_IDENTIFIER '.' T_IDENTIFIER       { $$.value = $1.value + '.' + $3.value; }
                  ;

plain_identifier : T_IDENTIFIER
                 ;
%%
