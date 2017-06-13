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

ASTNode::ptr strlen_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1,0x01> >( location, env, args, [] ( const ASTNodeBuiltin<1,0x01>::args_t& args ) -> uint64_t {
        return ASTNodeString::get_length( args[0].array );
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
