/*  Copyright (c) 2016, Martin Hammel
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

using namespace std;


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
// class VarManager::refvar implementation
//////////////////////////////////////////////////////////////////////////////

VarManager::refvar::refvar( VarManager::var* var)
 : m_Var( var )
{}

uint64_t VarManager::refvar::get() const
{
    return m_Var->get();
}

void VarManager::refvar::set( uint64_t value )
{
    m_Var->set( value );
}
