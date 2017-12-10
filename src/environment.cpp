/*  Copyright (c) 2015-2017, Martin Hammel
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

int Environment::s_DefaultModifier = ASTNodePrint::MOD_HEX | ASTNodePrint::MOD_WORDSIZE | ASTNodePrint::MOD_ARRAY;
std::stack<int> Environment::s_DefaultModifierStack;

std::stack< std::vector< std::pair< uint64_t, Environment::array* > > > Environment::s_ArgStack;

volatile sig_atomic_t Environment::s_IsTerminated = 0;


Environment::Environment()
{
    m_GlobalVars = new VarManager;
    m_GlobalArrays = new ArrayManager;

    m_BuiltinFunctions = new BuiltinManager( this );
    m_BuiltinArrayfuncs = new BuiltinManager( this );

    register_float_functions( m_BuiltinFunctions );
    register_string_functions( m_BuiltinFunctions );
    register_string_arrayfuncs( m_BuiltinArrayfuncs );

    m_ProcedureManager = new SubroutineManager( this );
    m_FunctionManager = new SubroutineManager( this );
    m_ArrayfuncManager = new SubroutineManager( this );
}

Environment::~Environment()
{
	for( auto value: m_Mappings ) delete value.second;

	delete m_ProcedureManager;
	delete m_FunctionManager;
	delete m_ArrayfuncManager;

	delete m_BuiltinFunctions;
	delete m_BuiltinArrayfuncs;

    delete m_GlobalArrays;
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
            m_LocalArrays = nullptr;
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

    m_BuiltinFunctions->get_autocompletion( completions, prefix );
    m_BuiltinArrayfuncs->get_autocompletion( completions, prefix );
    m_FunctionManager->get_autocompletion( completions, prefix );
    m_ArrayfuncManager->get_autocompletion( completions, prefix );
    // m_ProcedureManager is skipped intentionally; procedures are more like keywords than variables

    m_GlobalVars->get_autocompletion( completions, prefix );
    if( m_LocalVars ) m_LocalVars->get_autocompletion( completions, prefix );

    m_GlobalArrays->get_autocompletion( completions, prefix );
    if( m_LocalArrays ) m_LocalArrays->get_autocompletion( completions, prefix );

    return completions;
}

bool Environment::map_memory( void* phys_addr, void* map_addr, size_t size, std::string device )
{
	if( get_mapping( map_addr, size ) ) return true;

	MMap* mmap = MMap::create( phys_addr, size, device.c_str() );
	if( !mmap ) return false;

	mmap->set_base_address( map_addr );

	m_Mappings[ map_addr ] = mmap;
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

void Environment::enter_subroutine_context( const yylloc_t& location, std::string name, subroutine_type_t type )
{
    assert( m_SubroutineContext == nullptr && m_LocalVars == nullptr && m_LocalArrays == nullptr );

    if( m_BuiltinFunctions->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
    if( m_BuiltinArrayfuncs->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );

    switch( type ) {
    case PROCEDURE:
        if( m_FunctionManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
        if( m_ArrayfuncManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );

        m_SubroutineContext = m_ProcedureManager;
        break;

    case FUNCTION:
    	if( m_ProcedureManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
        if( m_ArrayfuncManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );

    	m_SubroutineContext = m_FunctionManager;
    	break;

    case ARRAYFUNC:
    	if( m_ProcedureManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );
        if( m_FunctionManager->has_subroutine( name ) ) throw ASTExceptionNamingConflict( location, name );

    	m_SubroutineContext = m_ArrayfuncManager;
    	break;
    }

    m_SubroutineContext->begin_subroutine( location, name, (type == FUNCTION) ? true : false );
    m_LocalVars = m_SubroutineContext->get_var_manager();
    m_LocalArrays = m_SubroutineContext->get_array_manager();

    if( type == ARRAYFUNC ) set_subroutine_param( "return", true );
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

void Environment::set_subroutine_varargs()
{
    assert( m_SubroutineContext );

    m_SubroutineContext->set_varargs();
}

void Environment::commit_subroutine_context( )
{
    assert( m_SubroutineContext );

    m_SubroutineContext->commit_subroutine();

    m_SubroutineContext = nullptr;
    m_LocalVars = nullptr;
    m_LocalArrays = nullptr;
}

std::shared_ptr<ASTNode> Environment::get_procedure( const yylloc_t& location, std::string name, const arglist_t& args )
{
    std::shared_ptr<ASTNode> node = m_ProcedureManager->get_subroutine( location, name, args );
    if( !node ) throw ASTExceptionNamingConflict( location, name );
    return node;
}

std::shared_ptr<ASTNode> Environment::get_function( const yylloc_t& location, std::string name, const arglist_t& args )
{
    std::shared_ptr<ASTNode> node = m_BuiltinFunctions->get_subroutine( location, name, args );
    if( node ) return node;

    node = m_FunctionManager->get_subroutine( location, name, args );
    if( !node ) throw ASTExceptionNamingConflict( location, name );
    return node;
}

std::shared_ptr<ASTNode> Environment::get_arrayfunc( const yylloc_t& location, std::string name, std::string ret, const arglist_t& args )
{
	arglist_t retargs;
	retargs.push_back( make_pair( ASTNode::ptr(nullptr), ret ) );

	bool ret_is_input = false;
	for( auto arg: args ) {
		if( arg.second == ret ) ret_is_input = true;
		retargs.push_back( arg );
	}
	if( ret_is_input ) retargs[0].second = "@return";

	std::shared_ptr<ASTNode> zero = make_shared<ASTNodeConstant>( location, 0 );
	std::shared_ptr<ASTNode> init = make_shared<ASTNodeDim>( location, this, retargs[0].second, zero );

    std::shared_ptr<ASTNode> subroutine = m_BuiltinArrayfuncs->get_subroutine( location, name, retargs );
    if( !subroutine ) subroutine = m_ArrayfuncManager->get_subroutine( location, name, retargs );
    if( !subroutine ) throw ASTExceptionNamingConflict( location, name );

	std::shared_ptr<ASTNode> block = make_shared<ASTNodeArrayBlock>( location, this, ret );
    block->add_child( init );
    block->add_child( subroutine );
    if( ret_is_input ) block->add_child( make_shared<ASTNodeAssign>( location, this, ret, retargs[0].second ) );

    return block;
}

uint64_t Environment::parse_int( string str, bool& is_ok )
{
    uint64_t value = 0;
    is_ok = true;

    if( str.length() > 2 )
    {
        if( str[0] == '0' && str[1] == 'b' ) {
            for( size_t i = 2; i < str.length(); i++ ) {
                value <<= 1;
                if( str[i] == '1' ) value |= 1;
                else if( str[i] != '0' ) {
                    is_ok = false;
                    return 0;
                }
            }
            return value;
        }

        if( str[0] == '0' && str[1] == 'x' ) {
            istringstream stream( str );
            stream >> hex >> value;
            if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return value;
            else  {
                is_ok = false;
                return 0;
            }
        }
    }

    istringstream stream( str );
    stream >> dec >> value;
    if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return value;
    else  {
        is_ok = false;
        return 0;
    }
}

uint64_t Environment::parse_float( string str, bool& is_ok )
{
    double d;
    is_ok = true;
    istringstream stream( str );
    stream >> d;
    if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return *(uint64_t*)&d;
    else  {
        is_ok = false;
        return 0;
    }
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
    int arraymod = modifier & ASTNodePrint::MOD_ARRAYMASK;
    int typemod = modifier & ASTNodePrint::MOD_TYPESIZEMASK;

    bool ret = true;

    if( arraymod ) {
    	switch( arraymod ) {
    	case ASTNodePrint::MOD_ARRAY:
    	case ASTNodePrint::MOD_STRING:
    		s_DefaultModifier &= ~ASTNodePrint::MOD_ARRAYMASK;
			s_DefaultModifier |= arraymod;
    		break;

    	default:
    		ret = false;
    		break;
    	}
    }

    if( typemod ) {
        switch( typemod & ASTNodePrint::MOD_TYPEMASK ) {
        case ASTNodePrint::MOD_FLOAT:
            if( (typemod & ASTNodePrint::MOD_SIZEMASK) == ASTNodePrint::MOD_64BIT ) {
				s_DefaultModifier &= ~ASTNodePrint::MOD_TYPESIZEMASK;
				s_DefaultModifier |= typemod;
            }
            else ret = false;
            break;

        case ASTNodePrint::MOD_BIN:
        case ASTNodePrint::MOD_DEC:
        case ASTNodePrint::MOD_HEX:
        case ASTNodePrint::MOD_NEG:
            switch( typemod & ASTNodePrint::MOD_SIZEMASK ) {
            case ASTNodePrint::MOD_8BIT:
            case ASTNodePrint::MOD_16BIT:
            case ASTNodePrint::MOD_32BIT:
            case ASTNodePrint::MOD_64BIT:
            case ASTNodePrint::MOD_WORDSIZE:
                s_DefaultModifier &= ~ASTNodePrint::MOD_TYPESIZEMASK;
                s_DefaultModifier |= typemod;
                break;

            default:
            	ret = false;
            	break;
            }

        default:
            ret = false;
            break;
        }
    }

    return ret;
}
