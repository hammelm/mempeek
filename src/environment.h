#ifndef __environment_h__
#define __environment_h__

#include <string>
#include <utility>
#include <map>

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// class Environment
//////////////////////////////////////////////////////////////////////////////

class Environment {
public:
	Environment();
	~Environment();

	uint64_t* set_def( std::string name );
	const uint64_t* get_def( std::string name );

	uint64_t* set_var( std::string name );
	const uint64_t* get_var( std::string name );

	const uint64_t* get( std::string name );

private:
	std::map< std::string, std::pair< uint64_t*, bool > > m_Vars;
};

#endif // __environment_h__
