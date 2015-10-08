#include "environment.h"

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
			Environment::var* var = new Environment::var( 0, true );
			iter = m_Vars.insert( make_pair( name, var ) ).first;
	    }
	    else {
	        auto base = m_Vars.find( name.substr( 0, dot ) );
	        if( base == m_Vars.end() ) return nullptr;

			Environment::var* var = new Environment::var( 0, base->second );
			iter = m_Vars.insert( make_pair( name, var ) ).first;
		}
	}
	else if( !iter->second->is_def() ) return nullptr;

	return iter->second;
}

Environment::var* Environment::alloc_var( std::string name )
{
	auto iter = m_Vars.find( name );

	if( iter == m_Vars.end() ) {
		Environment::var* var = new Environment::var( 0, false );
		iter = m_Vars.insert( make_pair( name, var ) ).first;
	}
	else if( iter->second->is_def() ) return nullptr;

	return iter->second;
}

const Environment::var* Environment::get( std::string name )
{
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

bool Environment::map_memory( void* phys_addr, size_t size )
{
	if( get_mapping( phys_addr, size ) ) return true;

	MMap* mmap = MMap::create( phys_addr, size );
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
