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

void ASTException::loc( const yylloc_t& location )
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

void ASTException::clone( const ASTException& ex )
{
    if( m_Location ) delete[] m_Location;
    if( ex.m_Location ) m_Location = strcpy( new char[ strlen( ex.m_Location ) + 1 ], ex.m_Location );
    else m_Location = nullptr;

    if( m_What ) delete[] m_What;
    if( ex.m_What ) m_What = strcpy( new char[ strlen( ex.m_What ) + 1 ], ex.m_What );
    else m_What = nullptr;
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
