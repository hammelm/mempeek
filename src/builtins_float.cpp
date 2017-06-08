/*  Copyright (c) 2016-2017, Martin Hammel
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

#include <math.h>

using namespace std;


static ASTNode::ptr int2float( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = args[0].value;
        return *(int64_t*)&d1;
    });
}

static ASTNode::ptr float2int( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        return (uint64_t)(int64_t)d1;
    });
}

static ASTNode::ptr fadd( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2> >( location, env, args, [] ( const ASTNodeBuiltin<2>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 + d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fsub( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2> >( location, env, args, [] ( const ASTNodeBuiltin<2>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 - d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fmul( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2> >( location, env, args, [] ( const ASTNodeBuiltin<2>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 * d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fdiv( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2> >( location, env, args, [] ( const ASTNodeBuiltin<2>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = d1 / d2;
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fsqrt( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = sqrt( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fpow( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<2> >( location, env, args, [] ( const ASTNodeBuiltin<2>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = *(double*)(args + 1);
        double d3 = pow( d1, d2 );
        return *(uint64_t*)&d3;
    });
}

static ASTNode::ptr fexp( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = exp( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr flog( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = log( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fsin( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = sin( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fcos( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = cos( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr ftan( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = tan( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fasin( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = asin( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr facos( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = acos( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fatan( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = atan( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fabs_( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = fabs( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr ffloor( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = floor( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fceil( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = ceil( d1 );
        return *(uint64_t*)&d2;
    });
}

static ASTNode::ptr fround( const yylloc_t& location, Environment* env, const arglist_t& args )
{
    return make_shared< ASTNodeBuiltin<1> >( location, env, args, [] ( const ASTNodeBuiltin<1>::args_t& args ) -> uint64_t {
        double d1 = *(double*)(args + 0);
        double d2 = round( d1 );
        return *(uint64_t*)&d2;
    });
}

void Environment::register_float_functions( BuiltinManager* manager )
{
    manager->register_function( "int2float", int2float );
    manager->register_function( "float2int", float2int );
    manager->register_function( "fadd", fadd );
    manager->register_function( "fsub", fsub );
    manager->register_function( "fmul", fmul );
    manager->register_function( "fdiv", fdiv );
    manager->register_function( "fsqrt", fsqrt );
    manager->register_function( "fpow", fpow );
    manager->register_function( "flog", flog );
    manager->register_function( "fexp", fexp );
    manager->register_function( "fsin", fsin );
    manager->register_function( "fcos", fcos );
    manager->register_function( "ftan", ftan );
    manager->register_function( "fasin", fasin );
    manager->register_function( "facos", facos );
    manager->register_function( "fatan", fatan );
    manager->register_function( "fabs", fabs_ );
    manager->register_function( "ffloor", ffloor );
    manager->register_function( "fceil", fceil );
    manager->register_function( "fround", fround );
}
