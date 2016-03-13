/*  Copyright (c) 2016, Martin Hammel
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

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// builtin functions
//////////////////////////////////////////////////////////////////////////////

static ASTNode::ptr test( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<3> >( location, [] ( uint64_t args[3] ) -> uint64_t {
        return args[0] + args[1] * args[2];
    });
}

//////////////////////////////////////////////////////////////////////////////
// class BuiltinManager implementation
//////////////////////////////////////////////////////////////////////////////

BuiltinManager::BuiltinManager()
{
    m_Builtins[ "test" ] = make_pair< size_t, nodecreator_t >( 3, test );
}

std::shared_ptr<ASTNode> BuiltinManager::get_subroutine( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params )
{
    auto iter = m_Builtins.find( name );
    if( iter == m_Builtins.end() ) return nullptr;

    if( iter->second.first != params.size() ) throw ASTExceptionSyntaxError( location );

    ASTNode::ptr node = iter->second.second( location );
    for( auto param: params ) node->add_child( param );

    return node;
}
