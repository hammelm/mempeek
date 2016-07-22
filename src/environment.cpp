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

#include "md5.h"
#include "mempeek_ast.h"
#include "mempeek_exceptions.h"
#include "parser.h"
#include "lexer.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class Environment implementation
//////////////////////////////////////////////////////////////////////////////

int Environment::s_DefaultSize = (sizeof(void*) == 8) ? T_64BIT : ((sizeof(void*) == 2) ? T_16BIT : T_32BIT);
std::stack<int> Environment::s_DefaultSizeStack;

int Environment::s_DefaultModifier = ASTNodePrint::MOD_HEX | ASTNodePrint::MOD_WORDSIZE;
std::stack<int> Environment::s_DefaultModifierStack;

volatile sig_atomic_t Environment::s_IsTerminated = 0;


Environment::Environment()
{
    m_GlobalVars = new VarStorage;

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

	delete m_GlobalVars;
}

std::shared_ptr<ASTNode> Environment::parse( const char* str, bool is_file, bool run_once )
{
    yylloc_t location = { "", 0, 0 };
    return parse( location, str, is_file, run_once );
}

std::shared_ptr<ASTNode> Environment::parse( const yylloc_t& location, const char* str, bool is_file, bool run_once )
{
    ASTNode::ptr yyroot = nullptr;
    FILE* file = nullptr;
    char* curdir = nullptr;
    string filename = str;
    MD5 md5;

    if( is_file ) {
        file = fopen( filename.c_str(), "r" );
        if( !file ) {
            for( string path: m_IncludePaths ) {
                if( path.length() > 0 && path.back() != '/' ) path += '/';
                filename = path + str;
                file = fopen( filename.c_str(), "r" );
                if( file ) break;
            }
        }

        if( !file ) throw ASTExceptionFileNotFound( location, str );

        if( run_once ) {
            md5.load( filename.c_str() );

            if( m_ImportedFiles.find( md5 ) == m_ImportedFiles.end() ) m_ImportedFiles.insert( md5 );
            else return nullptr;
        }

        char* absname = realpath( filename.c_str(), nullptr );
        if( absname ) {
            string newdir( absname );
            size_t last = newdir.find_last_of( '/' );
            if( last != string::npos ) {
                curdir = getcwd( nullptr, 0 );
                newdir = newdir.substr( 0, last + 1 );
                chdir( newdir.c_str() );
            }
            free( absname );
        }
    }

    yyscan_t scanner;
    yylex_init( &scanner );
    yyset_extra( is_file ? filename.c_str() : "", scanner );

    YY_BUFFER_STATE lex_buffer;
    if( file ) lex_buffer = yy_create_buffer( file, YY_BUF_SIZE, scanner );
    else lex_buffer = yy_scan_string( str, scanner );

    yy_switch_to_buffer( lex_buffer, scanner );

    if( is_file ) {
        push_default_size();
        push_default_modifier();
    }

    auto cleanup = [ lex_buffer, scanner, is_file, file, curdir ] () {
        yy_delete_buffer( lex_buffer, scanner );
        yylex_destroy( scanner );

        if( is_file ) {
            pop_default_size();
            pop_default_modifier();

            fclose( file );
        }

        if( curdir ) {
            chdir( curdir );
            free( curdir );
        }
    };

    try {
        yyparse( scanner, this, yyroot );
    }
    catch( ... ) {
        cleanup();

        if( is_file && run_once ) m_ImportedFiles.erase( md5 );

        if( m_SubroutineContext ) {
            m_SubroutineContext->abort_subroutine();
            m_SubroutineContext = nullptr;
            m_LocalVars = nullptr;
        }

        throw;
    }

    cleanup();

    return yyroot;
}

bool Environment::add_include_path( std::string path )
{
    bool ret = false;

    char* abspath = realpath( path.c_str(), nullptr );
    if( abspath ) {
        struct stat buf;
        if( stat( abspath, &buf ) == 0 && S_ISDIR( buf.st_mode ) ) {
            m_IncludePaths.push_back( abspath );
            ret = true;
        }
        free( abspath );
    }

    return ret;
}

const Environment::var* Environment::get( std::string name )
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

std::set< std::string > Environment::get_autocompletion( std::string prefix )
{
    set< string > completions;

    m_BuiltinManager->get_autocompletion( completions, prefix );
    m_FunctionManager->get_autocompletion( completions, prefix );
    // m_ProcedureManager is skipped intentionally; procedures are more like keywords than variables

    m_GlobalVars->get_autocompletion( completions, prefix );
    if( m_LocalVars ) m_LocalVars->get_autocompletion( completions, prefix );

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
    assert( m_SubroutineContext == nullptr && m_LocalVars == nullptr );

    if( is_function ) {
        if( m_BuiltinManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
        if( m_FunctionManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
    }
    else if( m_ProcedureManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );

    m_SubroutineContext = is_function ? m_FunctionManager : m_ProcedureManager;

    m_LocalVars = m_SubroutineContext->begin_subroutine( location, name, is_function );
}

void Environment::set_subroutine_param( std::string name )
{
    assert( m_SubroutineContext );

    m_SubroutineContext->set_param( name );
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

uint64_t Environment::parse_int( string str )
{
    uint64_t value = 0;

    if( str.length() > 2 )
    {
        if( str[0] == '0' && str[1] == 'b' ) {
            for( size_t i = 2; i < str.length(); i++ ) {
                value <<= 1;
                if( str[i] == '1' ) value |= 1;
                else if( str[i] != '0' ) return 0;
            }
            return value;
        }

        if( str[0] == '0' && str[1] == 'x' ) {
            istringstream stream( str );
            stream >> hex >> value;
            if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return value;
            else return 0;
        }
    }

    istringstream stream( str );
    stream >> dec >> value;
    if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return value;
    else return 0;
}

uint64_t Environment::parse_float( string str )
{
    double d;
    istringstream stream( str );
    stream >> d;
    if( stream.fail() || stream.bad() || !(stream.eof() || (stream >> ws).eof()) ) return 0;
    else return *(uint64_t*)&d;
}

bool Environment::set_default_size( int size )
{
    switch( size ) {
    case 8:
        s_DefaultSize = T_8BIT;
        return true;

    case 16: s_DefaultSize = T_16BIT;
        return true;

    case 32:
        s_DefaultSize = T_32BIT;
        return true;

    case 64:
        s_DefaultSize = T_64BIT;
        return true;

    default:
        return false;
    }
}

bool Environment::set_default_modifier( int modifier )
{
    switch( modifier & ASTNodePrint::MOD_TYPEMASK ) {
    case ASTNodePrint::MOD_FLOAT:
        if( (modifier & ASTNodePrint::MOD_SIZEMASK) != ASTNodePrint::MOD_64BIT ) return false;
        s_DefaultModifier = modifier;
        return true;

    case ASTNodePrint::MOD_BIN:
    case ASTNodePrint::MOD_DEC:
    case ASTNodePrint::MOD_HEX:
    case ASTNodePrint::MOD_NEG:
        switch( modifier & ASTNodePrint::MOD_SIZEMASK ) {
        case ASTNodePrint::MOD_8BIT:
        case ASTNodePrint::MOD_16BIT:
        case ASTNodePrint::MOD_32BIT:
        case ASTNodePrint::MOD_64BIT:
        case ASTNodePrint::MOD_WORDSIZE:
            s_DefaultModifier = modifier;
            return true;

        default:
            return false;
        }

    default:
        return false;
    }
}


//////////////////////////////////////////////////////////////////////////////
// class SubroutineManager implementation
//////////////////////////////////////////////////////////////////////////////

SubroutineManager::SubroutineManager( Environment* env )
 : m_Environment( env )
{}

SubroutineManager::~SubroutineManager()
{
    if( m_PendingSubroutine ) {
        delete m_PendingSubroutine->vars;
        delete m_PendingSubroutine;
    }

    for( auto value: m_Subroutines ) {
        delete value.second->vars;
        delete value.second;
    }
}

VarStorage* SubroutineManager::begin_subroutine( const yylloc_t& location, std::string name, bool is_function )
{
    assert( m_PendingSubroutine == nullptr );

    if( m_Subroutines.find( name ) != m_Subroutines.end() ) throw ASTExceptionNamingConflict( location, name );

    m_PendingName = name;
    m_PendingSubroutine = new subroutine_t;
    m_PendingSubroutine->vars = new VarStorage;
    m_PendingSubroutine->location = location;

    if( is_function ) {
        m_PendingSubroutine->retval =  m_PendingSubroutine->vars->alloc_local( "return" );
        assert( m_PendingSubroutine->retval );
    }

    return m_PendingSubroutine->vars;
}

void SubroutineManager::set_param( std::string name )
{
    assert( m_PendingSubroutine );

    Environment::var* var = m_Environment->alloc_var( name );
    if( !var ) throw ASTExceptionNamingConflict( m_PendingSubroutine->location, name );

    m_PendingSubroutine->params.push_back( var );
}

void SubroutineManager::set_body( std::shared_ptr<ASTNode> body )
{
    assert( m_PendingSubroutine );

    m_PendingSubroutine->body = body;
}

void SubroutineManager::commit_subroutine()
{
    assert( m_PendingSubroutine );

    m_Subroutines[ m_PendingName ] = m_PendingSubroutine;

    m_PendingSubroutine = nullptr;
}

void SubroutineManager::abort_subroutine()
{
    assert( m_PendingSubroutine );

    delete m_PendingSubroutine->vars;
    delete m_PendingSubroutine;

    m_PendingSubroutine = nullptr;
}

bool SubroutineManager::drop_subroutine( std::string name )
{
    auto iter = m_Subroutines.find( name );
    if( iter == m_Subroutines.end() ) return false;

    delete iter->second->vars;
    delete iter->second;

    m_Subroutines.erase( iter );

    return true;
}

void SubroutineManager::get_autocompletion( std::set< std::string >& completions, std::string prefix )
{
    for( auto iter = m_Subroutines.lower_bound( prefix ); iter != m_Subroutines.end(); iter++ ) {
        if( iter->first.substr( 0, prefix.length() ) != prefix ) break;
        completions.insert( iter->first );
    }
}

std::shared_ptr<ASTNode> SubroutineManager::get_subroutine( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params )
{
    subroutine_t* subroutine;

    if( m_PendingSubroutine && m_PendingName == name ) subroutine = m_PendingSubroutine;
    else {
        auto value = m_Subroutines.find( name );
        if( value == m_Subroutines.end() ) return nullptr;
        subroutine = value->second;
    }

    if( subroutine->params.size() != params.size() ) throw ASTExceptionSyntaxError( location );

    ASTNode::ptr node = make_shared<ASTNodeSubroutine>( location, subroutine->body, subroutine->vars, subroutine->params, subroutine->retval );

    for( auto param: params ) node->add_child( param );

    return node;
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage implementation
//////////////////////////////////////////////////////////////////////////////

VarStorage::VarStorage()
{}

VarStorage::~VarStorage()
{
    for( auto value: m_Vars ) delete value.second;

    if( m_Storage ) delete[] m_Storage;
    while( !m_Stack.empty() ) {
        delete[] m_Stack.top();
        m_Stack.pop();
    }
}

Environment::var* VarStorage::alloc_def( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        size_t dot = name.find( '.' );

        if( dot == string::npos ) {
            Environment::var* var = new VarStorage::defvar();
            iter = m_Vars.insert( make_pair( name, var ) ).first;
        }
        else {
            auto base = m_Vars.find( name.substr( 0, dot ) );
            if( base == m_Vars.end() ) return nullptr;

            Environment::var* var = new VarStorage::structvar( base->second );
            iter = m_Vars.insert( make_pair( name, var ) ).first;
        }
    }
    else if( !iter->second->is_def() ) return nullptr;

    return iter->second;
}

Environment::var* VarStorage::alloc_global( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        Environment::var* var = new VarStorage::globalvar();
        iter = m_Vars.insert( make_pair( name, var ) ).first;
    }
    else if( iter->second->is_def() ) return nullptr;

    return iter->second;
}

Environment::var* VarStorage::alloc_ref( std::string name, Environment::var* var )
{
    if( m_Vars.find( name ) != m_Vars.end() ) return nullptr;

    Environment::var* ref = new VarStorage::refvar( var );
    m_Vars[ name ] = ref;
    return ref;
}

Environment::var* VarStorage::alloc_local( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        Environment::var* var = new VarStorage::localvar( m_Storage, m_StorageSize++ );
        iter = m_Vars.insert( make_pair( name, var ) ).first;
    }
    else if( iter->second->is_def() ) return nullptr;

    return iter->second;
}

void VarStorage::get_autocompletion( std::set< std::string >& completions, std::string prefix )
{
    for( auto iter = m_Vars.lower_bound( prefix ); iter != m_Vars.end(); iter++ ) {
        if( iter->first.substr( 0, prefix.length() ) != prefix ) break;
        completions.insert( iter->first );
    }
}

std::set< std::string > VarStorage::get_struct_members( std::string name )
{
    set< string > members;

    for( auto iter = m_Vars.lower_bound( name + '.' ); iter != m_Vars.end(); iter++  ) {
        if( iter->first.substr( 0, name.length() + 1 ) != name + '.' ) break;
        members.insert( iter->first.substr( name.length() + 1, string::npos ) );
    }

    return members;
}


//////////////////////////////////////////////////////////////////////////////
// class Environment::var implementation
//////////////////////////////////////////////////////////////////////////////

Environment::var::~var()
{}

bool Environment::var::is_def() const
{
    return false;
}

bool Environment::var::is_local() const
{
    return false;
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::defvar implementation
//////////////////////////////////////////////////////////////////////////////

bool VarStorage::defvar::is_def() const
{
    return true;
}

uint64_t VarStorage::defvar::get() const
{
    return m_Value;
}

void VarStorage::defvar::set( uint64_t value )
{
    m_Value = value;
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::structvar implementation
//////////////////////////////////////////////////////////////////////////////

VarStorage::structvar::structvar( const Environment::var* base )
 : m_Base( base )
{}

bool VarStorage::structvar::is_def() const
{
    return true;
}

uint64_t VarStorage::structvar::get() const
{
    return m_Base->get() + m_Offset;
}

void VarStorage::structvar::set( uint64_t offset )
{
    m_Offset = offset;
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::globalvar implementation
//////////////////////////////////////////////////////////////////////////////

uint64_t VarStorage::globalvar::get() const
{
    return m_Value;
}

void VarStorage::globalvar::set( uint64_t value )
{
    m_Value = value;
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::localvar implementation
//////////////////////////////////////////////////////////////////////////////

VarStorage::localvar::localvar( uint64_t*& storage, size_t offset )
 : m_Storage( storage ),
   m_Offset( offset )
{}

bool VarStorage::localvar::is_local() const
{
    return true;
}

uint64_t VarStorage::localvar::get() const
{
    return m_Storage[ m_Offset ];
}

void VarStorage::localvar::set( uint64_t value )
{
    m_Storage[ m_Offset ] = value;
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::refvar implementation
//////////////////////////////////////////////////////////////////////////////

VarStorage::refvar::refvar( Environment::var* var)
 : m_Var( var )
{}

uint64_t VarStorage::refvar::get() const
{
    return m_Var->get();
}

void VarStorage::refvar::set( uint64_t value )
{
    m_Var->set( value );
}
