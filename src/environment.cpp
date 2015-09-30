#include "environment.h"

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class Environment implementation
//////////////////////////////////////////////////////////////////////////////

Environment::Environment()
{}

Environment::~Environment()
{
	for( auto value: m_Vars ) {
		delete value.second.first;
	}
}

uint64_t* Environment::set_def( std::string name )
{
	uint64_t* var = nullptr;

	auto iter = m_Vars.find( name );
	if( iter == m_Vars.end() ) {
		var = new uint64_t;
		m_Vars[ name ] = make_pair( var, false );
	}
	else if( iter->second.second == false ) var = iter->second.first;

	return var;
}

const uint64_t* Environment::get_def( std::string name )
{
	auto iter = m_Vars.find( name );
	if( iter != m_Vars.end() && iter->second.second == false ) return iter->second.first;
	else return nullptr;
}

uint64_t* Environment::set_var( std::string name )
{
	uint64_t* var = nullptr;

	auto iter = m_Vars.find( name );
	if( iter == m_Vars.end() ) {
		var = new uint64_t;
		m_Vars[ name ] = make_pair( var, true );
	}
	else if( iter->second.second == true ) var = iter->second.first;

	return var;
}

const uint64_t* Environment::get_var( std::string name )
{
	auto iter = m_Vars.find( name );
	if( iter != m_Vars.end() && iter->second.second == true ) return iter->second.first;
	else return nullptr;
}

const uint64_t* Environment::get( std::string name )
{
	auto iter = m_Vars.find( name );
	if( iter == m_Vars.end() ) return nullptr;
	else return iter->second.first;
}
