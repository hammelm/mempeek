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

	void map_memory( void* phys_addr, size_t size );

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
