#include "mempeek_exceptions.h"

#include <string>
#include <typeinfo>

#include <string.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeException implementation
//////////////////////////////////////////////////////////////////////////////

ASTException::~ASTException()
{
	if( m_Location ) delete[] m_Location;
	if( m_What ) delete[] m_What;
}

const char* ASTException::what() const noexcept
{
	if( m_What ) return m_What;
	else return typeid( *this ).name();
}

const char* ASTException::get_location() const
{
	if( m_Location ) return m_Location;
	else return "";
}

void ASTException::loc( const ASTNode::location_t& location )
{
	string str = "";

    if( location.file != "" ) {
    	str += location.file;
    	str += ':';
    	str += to_string( location.first_line );
    	if( location.first_line != location.last_line ) {
    		str += '-';
    		str += to_string( location.last_line );
    	}
    	str += ": ";
    }

	if( m_Location ) delete[] m_Location;
	m_Location = strcpy( new char[ str.length() + 1 ], str.c_str() );
}

void ASTException::msg( const char* msg )
{
	if( m_What ) delete[] m_What;
    m_What = strcpy( new char[ strlen(msg) + 1 ], msg );
}

void ASTException::create_msg( const char* msg, std::vector< std::string >& args )
{
    string what = "";
    int param = 0;
    enum { TEXT, PARAM_START, PARAM_PENDING, PARAM_COMPLETED } state = TEXT;

    do {
    	char c = *msg;
    	switch( state ) {
    	case TEXT:
    		if( c == '$' ) state = PARAM_START;
    		else if( c ) what += c;
    		break;

    	case PARAM_START:
    		if( c == '$' ) {
    			what += '$';
    			state = TEXT;
    		}
    		else if( isdigit(c) ) {
    			param = c - '0';
    			state = PARAM_PENDING;
    		}
    		else {
    			what += '$';
    			if( c ) what += c;
    			state = TEXT;
    		}
			break;

    	case PARAM_PENDING:
    		if( isdigit(c) ) {
    			param *= 10;
    			param += c - '0';
    		}
    		else {
    			if( args.size() > param ) what += args[param];
    			else {
    				what += '$';
    				what += to_string( param );
    			}
    			if( c ) what += c;
    			state = TEXT;
    		}
    	}
    } while( *msg++ );

	if( m_What ) delete[] m_What;
    m_What = strcpy( new char[ what.length() + 1 ], what.c_str() );
}
