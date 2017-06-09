/*  Copyright (c) 2017, Martin Hammel
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

#include "builtins.h"
#include "mempeek_ast.h"

using namespace std;

namespace {


//////////////////////////////////////////////////////////////////////////////
// class MPString
//////////////////////////////////////////////////////////////////////////////

class MPString {
public:
	MPString( Environment::array* array );

	size_t get_length();

	string get();
	void set( string str );

private:
	typedef union {
		uint64_t integer;
		uint8_t string[8];
	} element_t;

	Environment::array* m_Array;
	size_t m_ArraySize;
};


//////////////////////////////////////////////////////////////////////////////
// class MPString implementation
//////////////////////////////////////////////////////////////////////////////

MPString::MPString( Environment::array* array )
{
	m_Array = array;
	m_ArraySize = array->get_size();
}

size_t MPString::get_length()
{
	size_t len = 0;

	for( size_t i = 0; i < m_ArraySize; i++ ) {
		element_t value;
		value.integer = m_Array->get(i);

		for( size_t j = 0; j < 8; j++ ) {
			if( value.string[j] ) len++;
			else return len;
		}
	}

	return len;
}

string MPString::get()
{
	string str;

	for( size_t i = 0; i < m_ArraySize; i++ ) {
		element_t value;
		value.integer = m_Array->get(i);
		for( size_t j = 0; j < 8; j++ ) {
			uint8_t c = value.string[j];
			if( c ) str += c;
			else return str;
		}
	}

	return str;
}

void MPString::set( string str )
{
	m_ArraySize = (str.length() + 7) / 8;
	m_Array->resize( m_ArraySize );

	for( size_t i = 0; i < m_ArraySize; i++ ) {
		element_t value;
		for( size_t j = 0; j < 8; j++ ) {
			size_t pos = 8 * i + j;
			value.string[j] = (pos < str.length()) ? str[pos] : 0;
		}
		m_Array->set( i, value.integer );
	}
}


//////////////////////////////////////////////////////////////////////////////
// builtin function node creators
//////////////////////////////////////////////////////////////////////////////

ASTNode::ptr strcat_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<3,0x07> >( location, env, args, [] ( const ASTNodeBuiltin<3,0x07>::args_t& args ) -> uint64_t {
    	MPString dst( args[0].array );
    	MPString src0( args[1].array );
    	MPString src1( args[2].array );

    	dst.set( src0.get() + src1.get() );

    	return 0;
    });
}

ASTNode::ptr substr( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<4,0x03> >( location, env, args, [] ( const ASTNodeBuiltin<4,0x03>::args_t& args ) -> uint64_t {
    	MPString dst( args[0].array );
    	string src = MPString( args[1].array ).get();
    	uint64_t pos = args[2].value;
    	uint64_t len = args[3].value;

    	if( pos < src.length() ) {
    		if( len < src.length() ) dst.set( src.substr( pos, len ) );
    		else dst.set( src.substr( pos ) );
    	}
    	else dst.set("");

    	return 0;
    });
}

ASTNode::ptr strlen_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
    	MPString str( args[0].array );
    	return str.get_length();
    });
}


} // anonymous namespace


//////////////////////////////////////////////////////////////////////////////
// Environment register functions
//////////////////////////////////////////////////////////////////////////////

void Environment::register_string_functions( BuiltinManager* manager )
{
    manager->register_function( "strlen", strlen_ );
}

void Environment::register_string_arrayfuncs( BuiltinManager* manager )
{
    manager->register_function( "strcat", strcat_ );
    manager->register_function( "substr", substr );
}
