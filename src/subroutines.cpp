/*  Copyright (c) 2016-2018, Martin Hammel
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

#include "subroutines.h"

#include "mempeek_ast.h"
#include "mempeek_exceptions.h"

#include <assert.h>

using namespace std;


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
        delete m_PendingSubroutine->arrays;
        delete m_PendingSubroutine;
    }

    for( auto value: m_Subroutines ) {
        delete value.second->vars;
        delete value.second->arrays;
        delete value.second;
    }
}

void SubroutineManager::begin_subroutine( const yylloc_t& location, std::string name, bool is_function )
{
    assert( m_PendingSubroutine == nullptr );

    if( m_Subroutines.find( name ) != m_Subroutines.end() ) throw ASTExceptionNamingConflict( location, name );

    m_PendingName = name;
    m_PendingSubroutine = new subroutine_t;
    m_PendingSubroutine->vars = new VarManager;
    m_PendingSubroutine->arrays = new ArrayManager;
    m_PendingSubroutine->location = location;
    m_PendingSubroutine->has_varargs = false;

    if( is_function ) {
        m_PendingSubroutine->retval =  m_PendingSubroutine->vars->alloc_local( "return" );
        assert( m_PendingSubroutine->retval );
    }
}

void SubroutineManager::set_param( std::string name, bool is_array )
{
    assert( m_PendingSubroutine );

    if( is_array ) {
        Environment::refarray* array = m_Environment->alloc_ref_array( name );
        if( !array ) throw ASTExceptionNamingConflict( m_PendingSubroutine->location, name );

        param_t param;
        param.is_array = true;
        param.param.array = array;
        m_PendingSubroutine->params.push_back( param );
    }
    else {
        Environment::var* var = m_Environment->alloc_var( name );
        if( !var ) throw ASTExceptionNamingConflict( m_PendingSubroutine->location, name );

        param_t param;
        param.is_array = false;
        param.param.var = var;
        m_PendingSubroutine->params.push_back( param );
    }
}

void SubroutineManager::set_body( std::shared_ptr<ASTNode> body )
{
    assert( m_PendingSubroutine );

    m_PendingSubroutine->body = body;
}

void SubroutineManager::set_varargs()
{
    assert( m_PendingSubroutine );

    m_PendingSubroutine->has_varargs = true;
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

std::shared_ptr<ASTNode> SubroutineManager::get_subroutine( const yylloc_t& location, std::string name, const arglist_t& args )
{
    subroutine_t* subroutine;

    if( m_PendingSubroutine && m_PendingName == name ) subroutine = m_PendingSubroutine;
    else {
        auto value = m_Subroutines.find( name );
        if( value == m_Subroutines.end() ) return nullptr;
        subroutine = value->second;
    }

    if( subroutine->has_varargs ) {
        if( subroutine->params.size() > args.size() ) throw ASTExceptionSyntaxError( location );
    }
    else {
        if( subroutine->params.size() != args.size() ) throw ASTExceptionSyntaxError( location );
    }

    const size_t num_params = subroutine->params.size();
    const size_t num_varargs = args.size() - num_params;

    ASTNodeSubroutine::ptr node = make_shared<ASTNodeSubroutine>( location, subroutine->body,
                                                                  m_Environment, subroutine->vars, subroutine->arrays,
                                                                  subroutine->params, num_varargs, subroutine->retval );

    for( size_t i = 0; i < num_params; i++ ) {
        if( args[i].second.empty() ) {
            if( subroutine->params[i].is_array ) throw ASTExceptionSyntaxError( location );
            node->add_child( args[i].first );
        }
        else {
            if( !subroutine->params[i].is_array ) throw ASTExceptionSyntaxError( location );
            if( args[i].first ) node->add_child( args[i].first );
            else node->add_child( make_shared<ASTNodeArray>( location, m_Environment, args[i].second ) );
        }
    }

    for( size_t j = 0; j < num_varargs; j++ ) {
        const size_t i = j + num_params;
        if( args[i].first ) node->add_child( args[i].first );
        else node->add_child( make_shared<ASTNodeArray>( location, m_Environment, args[i].second ) );
    }

    return node;
}
