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

#include <math.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// floating point builtin functions
//////////////////////////////////////////////////////////////////////////////

static ASTNode::ptr int2float( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = args[0];
        return *(uint64_t*)&d1;
    });
}

static ASTNode::ptr float2int( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        return (uint64_t)(int64_t)d1;
    });
}

static ASTNode::ptr fadd( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<2> >( location, [] ( uint64_t args[2] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 + d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fsub( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<2> >( location, [] ( uint64_t args[2] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 - d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fmul( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<2> >( location, [] ( uint64_t args[2] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 * d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fdiv( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<2> >( location, [] ( uint64_t args[2] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 / d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fsqrt( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = sqrt( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr flog( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = log( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fpow( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<2> >( location, [] ( uint64_t args[2] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = pow( d1, d2 );
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fabs_( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = fabs( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr ffloor( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = floor( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fceil( const yylloc_t& location )
{
    return make_shared< ASTNodeBuiltin<1> >( location, [] ( uint64_t args[1] ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = ceil( d1 );
        return *(uint64_t*)&d2;
    });
}


//////////////////////////////////////////////////////////////////////////////
// class BuiltinManager implementation
//////////////////////////////////////////////////////////////////////////////

BuiltinManager::BuiltinManager()
{
    m_Builtins[ "int2float" ] = make_pair< size_t, nodecreator_t >( 1, int2float );
    m_Builtins[ "float2int" ] = make_pair< size_t, nodecreator_t >( 1, float2int );
    m_Builtins[ "fadd" ] = make_pair< size_t, nodecreator_t >( 2, fadd );
    m_Builtins[ "fsub" ] = make_pair< size_t, nodecreator_t >( 2, fsub );
    m_Builtins[ "fmul" ] = make_pair< size_t, nodecreator_t >( 2, fmul );
    m_Builtins[ "fdiv" ] = make_pair< size_t, nodecreator_t >( 2, fdiv );
    m_Builtins[ "fsqrt" ] = make_pair< size_t, nodecreator_t >( 1, fsqrt );
    m_Builtins[ "flog" ] = make_pair< size_t, nodecreator_t >( 1, flog );
    m_Builtins[ "fpow" ] = make_pair< size_t, nodecreator_t >( 2, fpow );
    m_Builtins[ "fabs" ] = make_pair< size_t, nodecreator_t >( 1, fabs_ );
    m_Builtins[ "ffloor" ] = make_pair< size_t, nodecreator_t >( 1, ffloor );
    m_Builtins[ "fceil" ] = make_pair< size_t, nodecreator_t >( 1, fceil );
}

void BuiltinManager::get_autocompletion( std::set< std::string >& completions, std::string prefix )
{
    for( auto iter = m_Builtins.lower_bound( prefix ); iter != m_Builtins.end(); iter++ ) {
        if( iter->first.substr( 0, prefix.length() ) != prefix ) break;
        completions.insert( iter->first );
    }
}

std::shared_ptr<ASTNode> BuiltinManager::get_subroutine( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params )
{
    auto iter = m_Builtins.find( name );
    if( iter == m_Builtins.end() ) return nullptr;

    if( iter->second.first != params.size() ) throw ASTExceptionSyntaxError( location );

    ASTNode::ptr node = iter->second.second( location );

    bool is_const = true;
    for( auto param: params ) {
        node->add_child( param );
        is_const &= param->is_constant();
    }

    if( is_const ) return make_shared< ASTNodeConstant >( location, node->execute() );
    else return node;
}
