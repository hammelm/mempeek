/*  Copyright (c) 2016-2018, Martin Hammel
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
#include "mempeek_exceptions.h"

#include <math.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class BuiltinManager implementation
//////////////////////////////////////////////////////////////////////////////

BuiltinManager::BuiltinManager( Environment* env )
 : m_Env( env )
{}

void BuiltinManager::get_autocompletion( std::set< std::string >& completions, std::string prefix )
{
    for( auto iter = m_Builtins.lower_bound( prefix ); iter != m_Builtins.end(); iter++ ) {
        if( iter->first.substr( 0, prefix.length() ) != prefix ) break;
        completions.insert( iter->first );
    }
}

std::shared_ptr<ASTNode> BuiltinManager::get_subroutine( const yylloc_t& location, std::string name, const arglist_t& args )
{
    auto iter = m_Builtins.find( name );
    if( iter == m_Builtins.end() ) return nullptr;

    std::shared_ptr<ASTNode> node = iter->second( location, m_Env, args );

    return node->is_constant() ? node->clone_to_const() : node;
}

void BuiltinManager::register_function( std::string name, nodecreator_t creator )
{
    auto ret = m_Builtins.insert( make_pair( name, creator ) );
    assert( ret.second );
}
