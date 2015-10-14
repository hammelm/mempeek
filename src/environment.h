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

#ifndef __environment_h__
#define __environment_h__

#include "mmap.h"

#include <string>
#include <utility>
#include <map>
#include <set>

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// class Environment
//////////////////////////////////////////////////////////////////////////////

class Environment {
public:
	class var;

	Environment();
	~Environment();

	var* alloc_def( std::string name );
	var* alloc_var( std::string name );

	const var* get( std::string name );

	std::set< std::string > get_struct_members( std::string name );

    bool map_memory( void* phys_addr, size_t size, std::string device );

	MMap* get_mapping( void* phys_addr, size_t size );

private:
	std::map< std::string, var* > m_Vars;
	std::map< void*, MMap* > m_Mappings;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment::var
//////////////////////////////////////////////////////////////////////////////

class  Environment::var {
public:
	var( uint64_t value, bool is_def );
	var( uint64_t value, const var* base_value );

	bool is_def() const;

	uint64_t get() const;
	void set( uint64_t value );

private:
	bool m_IsDef;
	uint64_t m_Value;
	const uint64_t* m_BaseValue;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment::var inline functions
//////////////////////////////////////////////////////////////////////////////

inline Environment::var::var( uint64_t value, bool is_def )
 : m_IsDef( is_def ),
   m_Value( value ),
   m_BaseValue( nullptr )
{}

inline Environment::var::var( uint64_t value, const var* base_value )
 : m_IsDef( true ),
   m_Value( value ),
   m_BaseValue( &base_value->m_Value )
{}

inline bool Environment::var::is_def() const
{
	return m_IsDef;
}

inline uint64_t Environment::var::get() const
{
	if( m_BaseValue ) return m_Value + *m_BaseValue;
	else return m_Value;
}

inline void Environment::var::set( uint64_t value )
{
	m_Value = value;
}


#endif // __environment_h__
