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
		Environment::var* var = new Environment::var( 0, true );
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
