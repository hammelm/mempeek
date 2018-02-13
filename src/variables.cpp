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

#include "variables.h"

#include "environment.h"
#include "parser.h"

#include <assert.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class VarManager implementation
//////////////////////////////////////////////////////////////////////////////

VarManager::VarManager()
{}

VarManager::~VarManager()
{
    for( auto value: m_Vars ) delete value.second;

    if( m_Storage ) delete[] m_Storage;
    while( !m_Stack.empty() ) {
        delete[] m_Stack.top();
        m_Stack.pop();
    }
}

VarManager::var* VarManager::alloc_def( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        size_t dot = name.find( '.' );

        if( dot == string::npos ) {
            VarManager::var* var = new VarManager::defvar();
            iter = m_Vars.insert( make_pair( name, var ) ).first;
        }
        else {
            auto base = m_Vars.find( name.substr( 0, dot ) );
            if( base == m_Vars.end() ) return nullptr;

            VarManager::var* var = new VarManager::structvar( base->second );
            iter = m_Vars.insert( make_pair( name, var ) ).first;
        }
    }
    else if( !iter->second->is_def() ) return nullptr;

    return iter->second;
}

VarManager::var* VarManager::alloc_global( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        VarManager::var* var = new VarManager::globalvar();
        iter = m_Vars.insert( make_pair( name, var ) ).first;
    }
    else if( iter->second->is_def() ) return nullptr;

    return iter->second;
}

VarManager::var* VarManager::alloc_delegate( std::string name, VarManager::var* var )
{
    if( m_Vars.find( name ) != m_Vars.end() ) return nullptr;

    VarManager::var* ref = new VarManager::delegatevar( var );
    m_Vars[ name ] = ref;
    return ref;
}

VarManager::var* VarManager::alloc_local( std::string name )
{
    auto iter = m_Vars.find( name );

    if( iter == m_Vars.end() ) {
        VarManager::var* var = new VarManager::localvar( m_Storage, m_StorageSize++ );
        iter = m_Vars.insert( make_pair( name, var ) ).first;
    }
    else if( iter->second->is_def() ) return nullptr;

    return iter->second;
}

void VarManager::get_autocompletion( std::set< std::string >& completions, std::string prefix )
{
    for( auto iter = m_Vars.lower_bound( prefix ); iter != m_Vars.end(); iter++ ) {
        if( iter->first.substr( 0, prefix.length() ) != prefix ) break;
        completions.insert( iter->first );
    }
}

std::set< std::string > VarManager::get_struct_members( std::string name )
{
    set< string > members;

    for( auto iter = m_Vars.lower_bound( name + '.' ); iter != m_Vars.end(); iter++  ) {
        if( iter->first.substr( 0, name.length() + 1 ) != name + '.' ) break;
        members.insert( iter->first.substr( name.length() + 1, string::npos ) );
    }

    return members;
}


//////////////////////////////////////////////////////////////////////////////
// class VarManager::var implementation
//////////////////////////////////////////////////////////////////////////////

VarManager::var::~var()
{}

bool VarManager::var::is_def() const
{
    return false;
}

bool VarManager::var::is_local() const
{
    return false;
}

void VarManager::var::set_range( uint64_t range )
{
    // nothing to do
}

uint64_t VarManager::var::get_range() const
{
    return 0;
}

void VarManager::var::set_size( size_t range )
{
    // nothing to do
}

size_t VarManager::var::get_size() const
{
    switch( Environment::get_default_size() ) {
    case T_8BIT: return 1;
    case T_16BIT: return 2;
    case T_32BIT: return 4;
    case T_64BIT: return 8;
    default: assert(false);
    }
}


//////////////////////////////////////////////////////////////////////////////
// class VarManager::defvar implementation
//////////////////////////////////////////////////////////////////////////////

bool VarManager::defvar::is_def() const
{
    return true;
}

uint64_t VarManager::defvar::get() const
{
    return m_Value;
}

void VarManager::defvar::set( uint64_t value )
{
    m_Value = value;
}


//////////////////////////////////////////////////////////////////////////////
// class VarManager::structvar implementation
//////////////////////////////////////////////////////////////////////////////

VarManager::structvar::structvar( const VarManager::var* base )
 : m_Base( base )
{}

bool VarManager::structvar::is_def() const
{
    return true;
}

uint64_t VarManager::structvar::get() const
{
    return m_Base->get() + m_Offset;
}

void VarManager::structvar::set( uint64_t offset )
{
    m_Offset = offset;
}

void VarManager::structvar::set_range( uint64_t range )
{
    m_Range = range;
}

uint64_t VarManager::structvar::get_range() const
{
    return m_Range;
}

void VarManager::structvar::set_size( size_t size )
{
    m_Size = size;
}

size_t VarManager::structvar::get_size() const
{
    return m_Size;
}


//////////////////////////////////////////////////////////////////////////////
// class VarManager::globalvar implementation
//////////////////////////////////////////////////////////////////////////////

uint64_t VarManager::globalvar::get() const
{
    return m_Value;
}

void VarManager::globalvar::set( uint64_t value )
{
    m_Value = value;
}


//////////////////////////////////////////////////////////////////////////////
// class VarManager::localvar implementation
//////////////////////////////////////////////////////////////////////////////

VarManager::localvar::localvar( uint64_t*& storage, size_t offset )
 : m_Storage( storage ),
   m_Offset( offset )
{}

bool VarManager::localvar::is_local() const
{
    return true;
}

uint64_t VarManager::localvar::get() const
{
    return m_Storage[ m_Offset ];
}

void VarManager::localvar::set( uint64_t value )
{
    m_Storage[ m_Offset ] = value;
}


//////////////////////////////////////////////////////////////////////////////
// class VarManager::delegatevar implementation
//////////////////////////////////////////////////////////////////////////////

VarManager::delegatevar::delegatevar( VarManager::var* var)
 : m_Var( var )
{}

uint64_t VarManager::delegatevar::get() const
{
    return m_Var->get();
}

void VarManager::delegatevar::set( uint64_t value )
{
    m_Var->set( value );
}
