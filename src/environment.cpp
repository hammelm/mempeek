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

#include "environment.h"

#include "mempeek_ast.h"
#include "mempeek_exceptions.h"
#include "parser.h"
#include "lexer.h"

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class Environment implementation
//////////////////////////////////////////////////////////////////////////////

volatile sig_atomic_t Environment::s_IsTerminated = 0;


Environment::Environment()
{
    m_GlobalVars = new VarManager;
    m_GlobalArrays = new ArrayManager;

    m_BuiltinManager = new BuiltinManager;

    m_ProcedureManager = new SubroutineManager( this );
    m_FunctionManager = new SubroutineManager( this );
}

Environment::~Environment()
{
	for( auto value: m_Mappings ) delete value.second;

	delete m_ProcedureManager;
	delete m_FunctionManager;

	delete m_BuiltinManager;

    delete m_GlobalArrays;
	delete m_GlobalVars;
}

std::shared_ptr<ASTNode> Environment::parse( const char* str, bool is_file )
{
    yylloc_t location = { "", 0, 0 };
    return parse( location, str, is_file );
}

std::shared_ptr<ASTNode> Environment::parse( const yylloc_t& location, const char* str, bool is_file )
{
    ASTNode::ptr yyroot = nullptr;
    FILE* file = nullptr;

    if( is_file ) {
        file = fopen( str, "r" );
        if( !file ) {
            for( string path: m_IncludePaths ) {
                if( path.length() > 0 && path.back() != '/' ) path += '/';
                path += str;
                file = fopen( path.c_str(), "r" );
                if( file ) break;
            }
        }

        if( !file ) throw ASTExceptionFileNotFound( location, str );
    }

    yyscan_t scanner;
    yylex_init( &scanner );
    yyset_extra( is_file ? str : "", scanner );

    YY_BUFFER_STATE lex_buffer;
    if( file ) lex_buffer = yy_create_buffer( file, YY_BUF_SIZE, scanner );
    else lex_buffer = yy_scan_string( str, scanner );

    yy_switch_to_buffer( lex_buffer, scanner );

    try {
        yyparse( scanner, this, yyroot );
    }
    catch( ... ) {
        yy_delete_buffer( lex_buffer, scanner );
        yylex_destroy( scanner );

        if( is_file  ) fclose( file );

        if( m_SubroutineContext ) {
            m_SubroutineContext->abort_subroutine();
            m_SubroutineContext = nullptr;
            m_LocalVars = nullptr;
            m_LocalArrays = nullptr;
        }

        throw;
    }

    yy_delete_buffer( lex_buffer, scanner );
    yylex_destroy( scanner );

    if( file  ) fclose( file );

    return yyroot;
}

const Environment::var* Environment::get_var( std::string name )
{
    if( m_LocalVars ) {
        const Environment::var* var = m_LocalVars->get( name );
        if( var ) return var;

        var = m_GlobalVars->get( name );
        if( var && var->is_def() ) return var;
        else return nullptr;
    }
    else return m_GlobalVars->get( name );
}

Environment::array* Environment::get_array( std::string name )
{
    if( m_LocalArrays ) {
        Environment::array* array = m_LocalArrays->get( name );
        if( array ) return array;
    }

    return m_GlobalArrays->get( name );
}

std::set< std::string > Environment::get_autocompletion( std::string prefix )
{
    set< string > completions;

    m_BuiltinManager->get_autocompletion( completions, prefix );
    m_FunctionManager->get_autocompletion( completions, prefix );
    // m_ProcedureManager is skipped intentionally; procedures are more like keywords than variables

    m_GlobalVars->get_autocompletion( completions, prefix );
    if( m_LocalVars ) m_LocalVars->get_autocompletion( completions, prefix );

    m_GlobalArrays->get_autocompletion( completions, prefix );
    if( m_LocalArrays ) m_LocalArrays->get_autocompletion( completions, prefix );

    return completions;
}

bool Environment::map_memory( void* phys_addr, size_t size, std::string device )
{
	if( get_mapping( phys_addr, size ) ) return true;

	MMap* mmap = MMap::create( phys_addr, size, device.c_str() );
	if( !mmap ) return false;

	m_Mappings[ phys_addr ] = mmap;
	return true;
}

MMap* Environment::get_mapping( void* phys_addr, size_t size )
{
	if( m_Mappings.empty() ) return nullptr;

	auto iter = m_Mappings.upper_bound( phys_addr );

	MMap* mmap;
	if( iter == m_Mappings.begin() ) return nullptr;
	else if( iter == m_Mappings.end() ) mmap = m_Mappings.rbegin()->second;
	else mmap = (--iter)->second;

	if( (uint8_t*)mmap->get_base_address() + mmap->get_size() < (uint8_t*)phys_addr + size ) return nullptr;
	else return mmap;
}

void Environment::enter_subroutine_context( const yylloc_t& location, std::string name, bool is_function )
{
    assert( m_SubroutineContext == nullptr && m_LocalVars == nullptr && m_LocalArrays == nullptr );

    if( is_function ) {
        if( m_BuiltinManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
        if( m_FunctionManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
    }
    else if( m_ProcedureManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );

    m_SubroutineContext = is_function ? m_FunctionManager : m_ProcedureManager;

    m_SubroutineContext->begin_subroutine( location, name, is_function );
    m_LocalVars = m_SubroutineContext->get_var_manager();
    m_LocalArrays = m_SubroutineContext->get_array_manager();
}

void Environment::set_subroutine_param( std::string name, bool is_array )
{
    assert( m_SubroutineContext );

    m_SubroutineContext->set_param( name, is_array );
}

void Environment::set_subroutine_body( std::shared_ptr<ASTNode> body  )
{
    assert( m_SubroutineContext );

    m_SubroutineContext->set_body( body );
}

void Environment::commit_subroutine_context( )
{
    assert( m_SubroutineContext );

    m_SubroutineContext->commit_subroutine();

    m_SubroutineContext = nullptr;
    m_LocalVars = nullptr;
    m_LocalArrays = nullptr;
}

std::shared_ptr<ASTNode> Environment::get_procedure( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params )
{
    std::shared_ptr<ASTNode> node = m_ProcedureManager->get_subroutine( location, name, params );
    if( !node ) throw ASTExceptionNamingConflict( location, name );
    return node;
}

std::shared_ptr<ASTNode> Environment::get_function( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params )
{
    std::shared_ptr<ASTNode> node = m_BuiltinManager->get_subroutine( location, name, params );
    if( node ) return node;

    node = m_FunctionManager->get_subroutine( location, name, params );
    if( !node ) throw ASTExceptionNamingConflict( location, name );
    return node;
}

int Environment::get_default_size()
{
    switch( sizeof(void*) ) {
    case 2: return T_16BIT;
    case 8: return T_64BIT;
    default: return T_32BIT;
    }
}
