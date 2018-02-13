/*  Copyright (c) 2017-2018, Martin Hammel
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

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <regex>

using namespace std;

namespace {

std::vector< std::string > TOKENS;


//////////////////////////////////////////////////////////////////////////////
// builtin function node creators
//////////////////////////////////////////////////////////////////////////////

ASTNode::ptr strcat_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<3,0x07> >( location, env, args, [] ( const ASTNodeBuiltin<3,0x07>::args_t& args ) -> uint64_t {
        string str1 = ASTNodeString::get_string( args[1].array );
        string str2 = ASTNodeString::get_string( args[2].array );
        ASTNodeString::set_string( args[0].array, str1 + str2 );

    	return 0;
    });
}

ASTNode::ptr substr( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<4,0x03> >( location, env, args, [] ( const ASTNodeBuiltin<4,0x03>::args_t& args ) -> uint64_t {
    	string str = ASTNodeString::get_string( args[1].array );
    	uint64_t pos = args[2].value;
    	uint64_t len = args[3].value;

    	if( pos < str.length() ) {
    		if( len < str.length() ) ASTNodeString::set_string( args[0].array, str.substr( pos, len ) );
    		else ASTNodeString::set_string( args[0].array, str.substr( pos ) );
    	}
    	else ASTNodeString::set_string( args[0].array, "" );

    	return 0;
    });
}

ASTNode::ptr getline_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
    	string line;
    	std::getline( cin, line );
    	ASTNodeString::set_string( args[0].array, line );

    	return 0;
    });
}

ASTNode::ptr strlen_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
        return ASTNodeString::get_length( args[0].array );
    });
}

ASTNode::ptr strcmp_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2,0x03> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x03>::args_t& args ) -> uint64_t {
		string str1 = ASTNodeString::get_string( args[0].array );
		string str2 = ASTNodeString::get_string( args[1].array );

		int ret = strcmp( str1.c_str(), str2.c_str() );

		if( ret < 0 ) return 0;
		else if( ret > 0 ) return 2;
		else return 1;
    });
}

ASTNode::ptr str2int( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
    	return Environment::parse_int( ASTNodeString::get_string( args[0].array ) );
    });
}

ASTNode::ptr str2float( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
    	return Environment::parse_float( ASTNodeString::get_string( args[0].array ) );
    });
}

void tokenize_string( string text, string separator )
{
	regex pattern( separator );
	TOKENS.clear();

	for_each( sregex_token_iterator( text.begin(), text.end(), pattern, -1 ),
			  sregex_token_iterator(),
			  [] ( string token )
	{
		TOKENS.push_back( token );
	});
}

ASTNode::ptr tokenize( const yylloc_t& location, Environment* env, const arglist_t& args )
{
	if( args.size() == 1 ) {
		return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
			string text = ASTNodeString::get_string( args[0].array );

			tokenize_string( text, "\\s+" );

			return TOKENS.size();
		});
	}
	else {
	    return make_shared< ASTNodeBuiltin<2,0x03> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x03>::args_t& args ) -> uint64_t {
	    	string text = ASTNodeString::get_string( args[0].array );
	    	string separator = ASTNodeString::get_string( args[1].array );

	    	tokenize_string( text, separator );

	    	return TOKENS.size();
	    });
	}
}

ASTNode::ptr gettoken( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x01>::args_t& args ) -> uint64_t {
    	uint64_t index = args[1].value;

    	if( index < TOKENS.size() ) ASTNodeString::set_string( args[0].array, TOKENS[index] );
    	else ASTNodeString::set_string( args[0].array, "" );

    	return 0;
    });
}

ASTNode::ptr int2str( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x01>::args_t& args ) -> uint64_t {
    	ostringstream ostr;
    	ostr << args[1].value;

    	ASTNodeString::set_string( args[0].array, ostr.str() );

    	return 0;
    });
}

ASTNode::ptr signed2str( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x01>::args_t& args ) -> uint64_t {
    	int64_t value = args[1].value;

    	ostringstream ostr;
    	ostr << value;

    	ASTNodeString::set_string( args[0].array, ostr.str() );

    	return 0;
    });
}

ASTNode::ptr hex2str( const yylloc_t& location, Environment* env, const arglist_t& args )
{

	if( args.size() == 2 ) {
		return make_shared< ASTNodeBuiltin<2,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x01>::args_t& args ) -> uint64_t {
			ostringstream ostr;
			ostr << "0x" << hex << args[1].value;

			ASTNodeString::set_string( args[0].array, ostr.str() );

			return 0;
		});
	}
	else {
		return make_shared< ASTNodeBuiltin<3,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<3,0x01>::args_t& args ) -> uint64_t {
			ostringstream ostr;
			ostr << "0x" << hex << setw( args[2].value ) << setfill('0') << args[1].value;

			ASTNodeString::set_string( args[0].array, ostr.str() );

			return 0;
		});
	}
}


ASTNode::ptr bin2str( const yylloc_t& location, Environment* env, const arglist_t& args )
{
	if( args.size() == 2 ) {
		return make_shared< ASTNodeBuiltin<2,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x01>::args_t& args ) -> uint64_t {
			uint64_t value = args[1].value;

			deque<char> bin;
			do {
				if( (value & 1) == 0 ) bin.push_front( '0' );
				else bin.push_front( '1' );
				value >>= 1;
			} while( value );

			bin.push_front( 'b' );
			bin.push_front( '0' );

			ASTNodeString::set_string( args[0].array, string( bin.begin(), bin.end() ) );

			return 0;
		});
	}
	else {
		return make_shared< ASTNodeBuiltin<3,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<3,0x01>::args_t& args ) -> uint64_t {
			uint64_t value = args[1].value;
			ssize_t size = args[2].value;

			deque<char> bin;
			do {
				if( (value & 1) == 0 ) bin.push_front( '0' );
				else bin.push_front( '1' );
				value >>= 1;
			} while( --size > 0 || value );

			bin.push_front( 'b' );
			bin.push_front( '0' );

			ASTNodeString::set_string( args[0].array, string( bin.begin(), bin.end() ) );

			return 0;
		});
	}
}

ASTNode::ptr float2str( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<2,0x01>::args_t& args ) -> uint64_t {
    	double value = *(double*)&(args[1].value);

    	ostringstream ostr;
    	ostr << value;

    	ASTNodeString::set_string( args[0].array, ostr.str() );

    	return 0;
    });
}

} // anonymous namespace


//////////////////////////////////////////////////////////////////////////////
// Environment register functions
//////////////////////////////////////////////////////////////////////////////

void Environment::register_string_functions( BuiltinManager* manager )
{
    manager->register_function( "strlen", strlen_ );
    manager->register_function( "strcmp", strcmp_ );
    manager->register_function( "str2int", str2int );
    manager->register_function( "str2float", str2float );
    manager->register_function( "tokenize", tokenize );
}

void Environment::register_string_arrayfuncs( BuiltinManager* manager )
{
    manager->register_function( "strcat", strcat_ );
    manager->register_function( "substr", substr );
    manager->register_function( "getline", getline_ );
    manager->register_function( "gettoken", gettoken );
    manager->register_function( "int2str", int2str );
    manager->register_function( "signed2str", signed2str );
    manager->register_function( "hex2str", hex2str );
    manager->register_function( "bin2str", bin2str );
    manager->register_function( "float2str", float2str );
}
