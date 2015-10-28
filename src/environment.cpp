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

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class Environment implementation
//////////////////////////////////////////////////////////////////////////////

Environment::Environment()
{}

Environment::~Environment()
{
	for( auto value: m_Vars ) delete value.second;
	for( auto value: m_Mappings ) delete value.second;
}

Environment::var* Environment::alloc_def( std::string name )
{
	auto iter = m_Vars.find( name );

	if( iter == m_Vars.end() ) {
	    size_t dot = name.find( '.' );

	    if( dot == string::npos ) {
			Environment::var* var = new Environment::defvar();
			iter = m_Vars.insert( make_pair( name, var ) ).first;
	    }
	    else {
	        auto base = m_Vars.find( name.substr( 0, dot ) );
	        if( base == m_Vars.end() ) return nullptr;

			Environment::var* var = new Environment::structvar( base->second );
			iter = m_Vars.insert( make_pair( name, var ) ).first;
		}
	}
	else if( !iter->second->is_def() ) return nullptr;

	return iter->second;
}

Environment::var* Environment::alloc_var( std::string name )
{
    if( m_LocalEnv ) return m_LocalEnv->alloc_var( name );
    else return alloc_global( name );
}

Environment::var* Environment::alloc_global( std::string name )
{
	auto iter = m_Vars.find( name );

	if( iter == m_Vars.end() ) {
		Environment::var* var = new Environment::globalvar();
		iter = m_Vars.insert( make_pair( name, var ) ).first;
	}
	else if( iter->second->is_def() ) return nullptr;

	return iter->second;
}

const Environment::var* Environment::get( std::string name )
{
    if( m_LocalEnv ) {
        const Environment::var* var = m_LocalEnv->get( name );
        if( var ) return var;
    }

	auto iter = m_Vars.find( name );
	if( iter == m_Vars.end() ) return nullptr;
	else return iter->second;
}

std::set< std::string > Environment::get_struct_members( std::string name )
{
	set< string > members;

	for( auto iter = m_Vars.lower_bound( name + '.' ); iter != m_Vars.end(); iter++  ) {
		if( iter->first.substr( 0, name.length() + 1 ) != name + '.' ) break;
		members.insert( iter->first.substr( name.length() + 1, string::npos ) );
	}

	return members;
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
	if( iter == m_Mappings.end() ) mmap = m_Mappings.rbegin()->second;
	else {
		if( --iter == m_Mappings.end() ) return nullptr;
		mmap = iter->second;
	}

	if( (uint8_t*)mmap->get_base_address() + mmap->get_size() < (uint8_t*)phys_addr + size ) return nullptr;
	else return mmap;
}

void Environment::enter_subroutine_context( std::shared_ptr<ASTNodeSubroutine> subroutine )
{
    m_Subroutine = subroutine;
    m_LocalEnv = subroutine->get_local_environment();
}

void Environment::commit_subroutine_context( std::string name, bool is_function )
{
    if( is_function) {
        if( m_Functions.find( name ) != m_Functions.end() ) throw ASTExceptionNamingConflict( m_Subroutine->get_location(), name );
        m_Functions[ name ] = m_Subroutine;
    }
    else {
        if( m_Procedures.find( name ) != m_Procedures.end() ) throw ASTExceptionNamingConflict( m_Subroutine->get_location(), name );
        m_Procedures[ name ] = m_Subroutine;
    }

    m_Subroutine = nullptr;
    m_LocalEnv = nullptr;
}

void Environment::set_subroutine_param( std::string name )
{
    m_Subroutine->add_parameter( name );
}

std::shared_ptr<ASTNodeSubroutine> Environment::get_procedure( std::string name,
                                                               std::vector< std::shared_ptr<ASTNode> >& params )
{
    auto proc = m_Procedures.find( name );

    if( proc == m_Procedures.end() ) return nullptr;
    if( proc->second->get_num_parameters() != params.size() ) return nullptr;

    for( auto param: params ) {
        proc->second->add_child( param );
    }

    return proc->second;
}

std::shared_ptr<ASTNodeSubroutine> Environment::get_function( std::string name,
                                                              std::vector< std::shared_ptr<ASTNode> >& params )
{
    auto func = m_Functions.find( name );

    if( func == m_Functions.end() ) return nullptr;
    if( func->second->get_num_parameters() != params.size() ) return nullptr;

    for( auto param: params ) {
        func->second->add_child( param );
    }

    return func->second;
}


//////////////////////////////////////////////////////////////////////////////
// class LocalEnvironment implementation
//////////////////////////////////////////////////////////////////////////////

LocalEnvironment::LocalEnvironment()
{}

LocalEnvironment::~LocalEnvironment()
{
    if( m_Storage ) delete[] m_Storage;
    while( !m_Stack.empty() ) {
        delete[] m_Stack.top();
        m_Stack.pop();
    }
    for( auto value: m_Vars ) delete value.second;
}

Environment::var* LocalEnvironment::alloc_var( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        Environment::var* var = new LocalEnvironment::localvar( m_Storage, m_Vars.size() );
        iter = m_Vars.insert( make_pair( name, var ) ).first;
    }
    else if( iter->second->is_def() ) return nullptr;

    return iter->second;
}

const Environment::var* LocalEnvironment::get( std::string name )
{
    auto iter = m_Vars.find( name );
    if( iter == m_Vars.end() ) return nullptr;
    else return iter->second;
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
// class Environment::defvar implementation
//////////////////////////////////////////////////////////////////////////////

bool Environment::defvar::is_def() const
{
    return true;
}

uint64_t Environment::defvar::get() const
{
    return m_Value;
}

void Environment::defvar::set( uint64_t value )
{
    m_Value = value;
}


//////////////////////////////////////////////////////////////////////////////
// class Environment::structvar implementation
//////////////////////////////////////////////////////////////////////////////

Environment::structvar::structvar( const Environment::var* base )
 : m_Base( base )
{}

bool Environment::structvar::is_def() const
{
    return true;
}

uint64_t Environment::structvar::get() const
{
    return m_Base->get() + m_Offset;
}

void Environment::structvar::set( uint64_t offset )
{
    m_Offset = offset;
}


//////////////////////////////////////////////////////////////////////////////
// class Environment::globalvar implementation
//////////////////////////////////////////////////////////////////////////////

uint64_t Environment::globalvar::get() const
{
    return m_Value;
}

void Environment::globalvar::set( uint64_t value )
{
    m_Value = value;
}


//////////////////////////////////////////////////////////////////////////////
// class LocalEnvironment::localvar implementation
//////////////////////////////////////////////////////////////////////////////

LocalEnvironment::localvar::localvar( uint64_t*& storage, size_t offset )
 : m_Storage( storage ),
   m_Offset( offset )
{}

bool LocalEnvironment::localvar::is_local() const
{
    return true;
}

uint64_t LocalEnvironment::localvar::get() const
{
    return m_Storage[ m_Offset ];
}

void LocalEnvironment::localvar::set( uint64_t value )
{
    m_Storage[ m_Offset ] = value;
}
