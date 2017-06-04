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
#include "mempeek_exceptions.h"

#include <math.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBuiltin
//////////////////////////////////////////////////////////////////////////////

template< size_t NUM_ARGS , uint32_t SIGNATURE = 0 >
class ASTNodeBuiltin : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeBuiltin> ptr;
    typedef union {
    	uint64_t value;
    	Environment::array* array;
    } args_t[NUM_ARGS];

    ASTNodeBuiltin( const yylloc_t& yylloc, Environment* env, const arglist_t& args, std::function< uint64_t( const args_t& ) > builtin, bool is_const = !SIGNATURE );

    uint64_t execute() override;

private:
    Environment* m_Env;
    std::vector< Environment::array* > m_ArrayParameters;

    std::function< uint64_t( const args_t& ) > m_Builtin;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBuiltin template functions
//////////////////////////////////////////////////////////////////////////////

template< size_t NUM_ARGS, uint32_t SIGNATURE >
inline ASTNodeBuiltin< NUM_ARGS, SIGNATURE >::ASTNodeBuiltin( const yylloc_t& yylloc, Environment* env, const arglist_t& args,
		                                                      std::function< uint64_t( const args_t& ) > builtin, bool is_const )
 : ASTNode( yylloc ),
   m_Env( env ),
   m_Builtin( builtin )
{
#ifdef ASTDEBUG
    std::cerr << "AST[" << this << "]: creating ASTNodeBuiltin<" << NUM_ARGS << "," << hex << SIGNATURE << dec << ">" << std::endl;
#endif

    if( args.size() != NUM_ARGS ) throw ASTExceptionSyntaxError( yylloc );

    for( size_t i = 0; i < NUM_ARGS; i++ ) {
        if( args[i].first ) {
        	if( (SIGNATURE & (1 << i)) != 0 ) throw ASTExceptionSyntaxError( yylloc );
			add_child( args[i].first );
			is_const &= args[i].first->is_constant();
        }
        else {
        	if( (SIGNATURE & (1 << i)) == 0 ) throw ASTExceptionSyntaxError( yylloc );
        	Environment::array* array = m_Env->get_array( args[i].second );
        	if( !array ) throw ASTExceptionUndefinedVar( yylloc, args[i].second );
        	m_ArrayParameters.push_back( array );
        }
    }

    if( is_const ) set_constant();
}

template< size_t NUM_ARGS, uint32_t SIGNATURE >
inline uint64_t ASTNodeBuiltin< NUM_ARGS, SIGNATURE >::execute()
{
#ifdef ASTDEBUG
    std::cerr << "AST[" << this << "]: executing ASTNodeBuiltin<" << NUM_ARGS << "," << hex << SIGNATURE << dec << ">" << std::endl;
#endif

    assert( get_children().size() + m_ArrayParameters.size() == NUM_ARGS );

    args_t args;
    for( size_t i = 0, j = 0, sum = 0; sum < NUM_ARGS; sum++  ) {
    	if( (SIGNATURE & (1 << sum)) ) args[sum].array = m_ArrayParameters[ i++ ];
    	else args[sum].value = get_children()[ j++ ]->execute();
    }
    return m_Builtin( args );
}


//////////////////////////////////////////////////////////////////////////////
// floating point builtin functions
//////////////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////////////////////////////////////////
// class BuiltinManager implementation
//////////////////////////////////////////////////////////////////////////////

BuiltinManager::BuiltinManager( Environment* env )
 : m_Env( env )
{
    m_Builtins[ "int2float" ] = int2float;
    m_Builtins[ "float2int" ] = float2int;
    m_Builtins[ "fadd" ] = fadd;
    m_Builtins[ "fsub" ] = fsub;
    m_Builtins[ "fmul" ] = fmul;
    m_Builtins[ "fdiv" ] = fdiv;
    m_Builtins[ "fsqrt" ] = fsqrt;
    m_Builtins[ "fpow" ] = fpow;
    m_Builtins[ "flog" ] = flog;
    m_Builtins[ "fexp" ] = fexp;
    m_Builtins[ "fsin" ] = fsin;
    m_Builtins[ "fcos" ] = fcos;
    m_Builtins[ "ftan" ] = ftan;
    m_Builtins[ "fasin" ] = fasin;
    m_Builtins[ "facos" ] = facos;
    m_Builtins[ "fatan" ] = fatan;
    m_Builtins[ "fabs" ] = fabs_;
    m_Builtins[ "ffloor" ] = ffloor;
    m_Builtins[ "fceil" ] = fceil;
    m_Builtins[ "fround" ] = fround;
}

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
