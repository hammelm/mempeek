/*  Copyright (c) 2015-2020, Martin Hammel
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

#include "mempeek_ast.h"

#include "mempeek_exceptions.h"
#include "mempeek_parser.h"
#include "parser.h"
#include "lexer.h"

#include <iostream>
#include <iomanip>
#include <list>

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ASTNode implementation
//////////////////////////////////////////////////////////////////////////////

ASTNode::ASTNode( const yylloc_t& yylloc )
 : m_Location( yylloc )
{}

ASTNode::~ASTNode()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: destructing" << endl;
#endif
}

bool ASTNode::get_array_result( Environment::array*& array )
{
    return false;
}

ASTNode::ptr ASTNode::clone_to_const()
{
    return nullptr;
}

uint64_t ASTNode::compiletime_execute( ASTNode* node )
{
    if( !node->is_constant() ) throw ASTExceptionNonconstExpression( node->get_location() );

    try {
        return node->execute();
    }
    catch( const ASTExceptionDivisionByZero& ex ) {
        throw ASTExceptionConstDivisionByZero( ex );
    }
}

//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBreak
//////////////////////////////////////////////////////////////////////////////

ASTNodeBreak::ASTNodeBreak( const yylloc_t& yylloc, int token )
 : ASTNode( yylloc ),
   m_Token( token )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeBreak token=" << token << endl;
#endif
}

uint64_t ASTNodeBreak::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeBreak" << endl;
#endif

    switch( m_Token ) {
    case T_EXIT: throw ASTExceptionExit();
    case T_BREAK: throw ASTExceptionBreak();
    case T_QUIT: throw ASTExceptionQuit();
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBlock implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeBlock::ASTNodeBlock( const yylloc_t& yylloc )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeBlock" << endl;
#endif
}

uint64_t ASTNodeBlock::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeBlock" << endl;
#endif

	for( ASTNode::ptr node: get_children() ) {
	    node->execute();
        if( Environment::is_terminated() ) throw ASTExceptionTerminate();
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeArrayBlock implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeArrayBlock::ASTNodeArrayBlock( const yylloc_t& yylloc, Environment* env, std::string result )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeArrayBlock result=" << result << endl;
#endif

    m_Array = env->get_array( result );

    if( !m_Array ) throw ASTExceptionUndefinedVar( get_location(), result );
}

uint64_t ASTNodeArrayBlock::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeArrayBlock" << endl;
#endif

	for( ASTNode::ptr node: get_children() ) {
	    node->execute();
        if( Environment::is_terminated() ) throw ASTExceptionTerminate();
	}

	return 0;
}

bool ASTNodeArrayBlock::get_array_result( Environment::array*& array )
{
    array = m_Array;
    return true;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeSubroutine implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeSubroutine::ASTNodeSubroutine( const yylloc_t& yylloc, std::weak_ptr<ASTNode> body,
                                      Environment* env, VarManager* vars, ArrayManager* arrays,
                                      std::vector< SubroutineManager::param_t >& params,
                                      size_t num_varargs, Environment::var* retval )
 : ASTNode( yylloc ),
   m_Env( env ),
   m_LocalVars( vars ),
   m_LocalArrays( arrays ),
   m_Params( params ),
   m_NumVarargs( num_varargs ),
   m_Retval( retval ),
   m_Body( body )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeSubroutine num_varargs=" << num_varargs << endl;
#endif
}

uint64_t ASTNodeSubroutine::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeSubroutine" << endl;
#endif

    int phase = 0;

    auto cleanup = [ &phase, this ] () {
        if( phase >= 2 ) {
            m_LocalVars->pop();
            m_LocalArrays->pop();
        }
        if( phase >= 1 ) {
            for( size_t i = 0; i < m_Params.size(); i++ ) {
                if( m_Params[i].is_array ) m_Params[i].param.array->pop_ref();
            }
            m_Env->pop_varargs();
        }
    };

    list< ArrayManager::refarray > refarrays;
    try {
        ASTNode::ptr body = m_Body.lock();
        if( !body ) throw ASTExceptionDroppedSubroutine( get_location() );

        typedef union {
            uint64_t value;
            Environment::array* array;
        } param_t;

        const size_t num_params = m_Params.size();
        vector<param_t> params( num_params );

        for( size_t i = 0; i < num_params; i++ ) {
            params[i].value = get_children()[i]->execute();
            if( m_Params[i].is_array ) get_children()[i]->get_array_result( params[i].array );
        }

        m_Env->push_varargs();
        phase = 1;

        for( size_t i = 0; i < num_params; i++ ) {
            if( m_Params[i].is_array ) m_Params[i].param.array->push_ref( params[i].array );
        }

        for( size_t j = 0; j < m_NumVarargs; j++ ) {
            const size_t i = j + num_params;

            Environment::array* array;
            uint64_t value = get_children()[i]->execute();
            bool is_array = get_children()[i]->get_array_result( array );

            if( is_array ) {
                refarrays.emplace( refarrays.begin() );
                refarrays.front().set_ref( array );
                m_Env->append_vararg( &refarrays.front() );
            }
            else m_Env->append_vararg( value );
        }

        m_LocalVars->push();
        m_LocalArrays->push();
        phase = 2;

        for( size_t i = 0; i < num_params; i++ ) {
            if( !m_Params[i].is_array ) m_Params[i].param.var->set( params[i].value );
        }

        if( m_Retval ) m_Retval->set(0);

        body->execute();
    }
    catch( ASTExceptionExit& ) {
        // nothing to do
    }
    catch( ASTExceptionBreak& ) {
        // nothing to do
    }
    catch( ... ) {
        cleanup();
        throw;
    }

    uint64_t ret = 0;
    if( phase == 2 && m_Retval ) ret = m_Retval->get();

    cleanup();

    return ret;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeIf implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeIf::ASTNodeIf( const yylloc_t& yylloc, ASTNode::ptr condition, ASTNode::ptr then_block )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeIf condition=[" << condition
		 << "] then_block=[" << then_block << "]" << endl;
#endif

	add_child( condition );
	add_child( then_block );
}

ASTNodeIf::ASTNodeIf( const yylloc_t& yylloc, ASTNode::ptr condition, ASTNode::ptr then_block, ASTNode::ptr else_block )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeIf condition=[" << condition
		 << "] then_block=[" << then_block << "] else_block=[" << else_block << "]" << endl;
#endif

	add_child( condition );
	add_child( then_block );
	add_child( else_block );
}

uint64_t ASTNodeIf::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeIf" << endl;
#endif

	if( get_children()[0]->execute() ) {
		get_children()[1]->execute();
	}
	else {
		if( get_children().size() > 2 ) get_children()[2]->execute();
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeWhile implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeWhile::ASTNodeWhile( const yylloc_t& yylloc, ASTNode::ptr condition, ASTNode::ptr block )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeWhile condition=[" << condition << "] block=[" << block << "]" << endl;
#endif

	add_child( condition );
	add_child( block );
}

uint64_t ASTNodeWhile::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeWhile" << endl;
#endif

	ASTNode::ptr condition = get_children()[0];
	ASTNode::ptr block = get_children()[1];

	while( condition->execute() ) {
	    try {
	        block->execute();
	    }
	    catch( ASTExceptionBreak& ) {
	        break;
	    }
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeFor
//////////////////////////////////////////////////////////////////////////////

ASTNodeFor::ASTNodeFor( const yylloc_t& yylloc, ASTNodeAssign::ptr var, ASTNode::ptr to )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeFor var=[" << var << "] to=[" << to << "]" << endl;
#endif

    m_Var = var->get_var();

    add_child( var );
    add_child( to );
}

ASTNodeFor::ASTNodeFor( const yylloc_t& yylloc, ASTNodeAssign::ptr var, ASTNode::ptr to, ASTNode::ptr step )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeFor var=[" << var << "] to=[" << to << "] step=[" << step << "]" << endl;
#endif

    m_Var = var->get_var();

    add_child( var );
    add_child( to );
    add_child( step );
}

uint64_t ASTNodeFor::execute()
{
    int child = 0;

    get_children()[child++]->execute(); // execute assignment of loop variable

    const int64_t to = get_children()[child++]->execute();
    const int64_t step = (get_children().size() > 3) ? get_children()[child++]->execute() : 1;

    ASTNode::ptr block = get_children()[child];
    for( int64_t i = m_Var->get(); step > 0 && i <= to || step < 0 && i >= to; m_Var->set( i += step ) ) {
        try {
            block->execute();
        }
        catch( ASTExceptionBreak& ) {
            break;
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePeek implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePeek::ASTNodePeek( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, int size_restriction )
 : ASTNode( yylloc ),
   m_Env( env ),
   m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePeek address=[" << address << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cerr << "8" << endl; break;
	case T_16BIT: cerr << "16" << endl; break;
	case T_32BIT: cerr << "32" << endl; break;
	case T_64BIT: cerr << "64" << endl; break;
	default: cerr << "ERR" << endl; break;
	}
#endif

	add_child( address );
}

uint64_t ASTNodePeek::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodePeek" << endl;
#endif

	switch( m_SizeRestriction ) {
	case T_8BIT: return peek<uint8_t>();
	case T_16BIT: return peek<uint16_t>();
	case T_32BIT: return peek<uint32_t>();
	case T_64BIT: return peek<uint64_t>();
	};

	return 0;
}

template< typename T >
uint64_t ASTNodePeek::peek()
{
	void* address = (void*)get_children()[0]->execute();
	MMap* mmap = m_Env->get_mapping( address, sizeof(T) );

    if( !mmap ) throw ASTExceptionNoMapping( get_location(), address, sizeof(T) );

    uint64_t ret = mmap->peek<T>( address );
    if( mmap->has_failed() ) throw ASTExceptionBusError( get_location(), address, sizeof(T) );

    return ret;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePoke implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePoke::ASTNodePoke( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, ASTNode::ptr value, int size_restriction )
 : ASTNode( yylloc ),
   m_Env( env ),
   m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePoke address=[" << address << "] value=[" << value << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cerr << "8" << endl; break;
	case T_16BIT: cerr << "16" << endl; break;
	case T_32BIT: cerr << "32" << endl; break;
	case T_64BIT: cerr << "64" << endl; break;
	default: cerr << "ERR" << endl; break;
	}
#endif

	add_child( address );
	add_child( value );
}

ASTNodePoke::ASTNodePoke( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, ASTNode::ptr value, ASTNode::ptr mask, int size_restriction )
 : ASTNode( yylloc ),
   m_Env( env ),
   m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePoke address=[" << address << "] value=[" << value
		 << "] mask=[" << mask << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cerr << "8" << endl; break;
	case T_16BIT: cerr << "16" << endl; break;
	case T_32BIT: cerr << "32" << endl; break;
	case T_64BIT: cerr << "64" << endl; break;
	default: cerr << "ERR" << endl; break;
	}
#endif

	add_child( address );
	add_child( value );
	add_child( mask );
}

uint64_t ASTNodePoke::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodePoke" << endl;
#endif

	switch( m_SizeRestriction ) {
	case T_8BIT: poke<uint8_t>(); break;
	case T_16BIT: poke<uint16_t>(); break;
	case T_32BIT: poke<uint32_t>(); break;
	case T_64BIT: poke<uint64_t>(); break;
	};

	return 0;
}

template< typename T >
void ASTNodePoke::poke()
{
	void* address = (void*)get_children()[0]->execute();
	T value = get_children()[1]->execute();

	MMap* mmap = m_Env->get_mapping( address, sizeof(T) );

	if( !mmap ) throw ASTExceptionNoMapping( get_location(), address, sizeof(T) );

	if( get_children().size() == 2 ) mmap->poke<T>( address, value );
	else {
		T mask = get_children()[2]->execute();
		mmap->clear<T>( address, mask );
		mmap->set<T>( address, value & mask);
	}

    if( mmap->has_failed() ) throw ASTExceptionBusError( get_location(), address, sizeof(T) );
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePrint implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePrint::ASTNodePrint( const yylloc_t& yylloc )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint" << endl;
#endif
}

void ASTNodePrint::add_arg( ASTNode::ptr node, int modifier )
{
	if( (modifier & MOD_SIZEMASK) == MOD_WORDSIZE ) {
	    modifier &= ~MOD_SIZEMASK;
	    modifier |= size_to_mod( Environment::get_default_size() );
	}

	m_Args.push_back( { node, "", modifier } );
}

void ASTNodePrint::add_arg( std::string text )
{
	m_Args.push_back( { nullptr, text, 0 } );
}

void ASTNodePrint::set_endl( bool enable )
{
	m_PrintEndl = enable;
}

uint64_t ASTNodePrint::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodePrint" << endl;
#endif

	for( arg_t arg: m_Args ) {
		if( arg.node ) {
	        Environment::array* array;
	        uint64_t value = arg.node->execute();
	        if( (arg.mode & MOD_ARRAYMASK ) != 0 && arg.node->get_array_result( array ) ) print_array( cout, array, arg.mode );
	        else print_value( cout, value, arg.mode );
		}
		else {
			cout << arg.text;
		}
	}

	if( m_PrintEndl ) cout << endl;
	else cout << flush;

	return 0;
}

int ASTNodePrint::size_to_mod( int size )
{
	switch( size ) {
    case T_8BIT: return MOD_8BIT;
	case T_16BIT: return MOD_16BIT;
	case T_32BIT: return MOD_32BIT;
	case T_64BIT: return MOD_64BIT;
	default: return 0;
	}
}

void ASTNodePrint::print_value( std::ostream& out, uint64_t value, int modifier )
{
	int size = 0;
	int64_t nvalue;

	switch( modifier & MOD_SIZEMASK ) {
	case MOD_8BIT:
		value &= 0xff;
		nvalue = (int8_t)value;
		size = 1;
		break;

	case MOD_16BIT:
		value &= 0xffff;
		nvalue = (int16_t)value;
		size = 2;
		break;

	case MOD_32BIT:
		value &= 0xffffffff;
		nvalue = (int32_t)value;
		size = 4;
		break;

	case MOD_64BIT:
		nvalue = (int64_t)value;
		size = 8;
		break;
	}

	switch( modifier & MOD_TYPEMASK ) {
	case MOD_HEX: {
		const ios_base::fmtflags oldflags = out.flags( ios::hex | ios::right | ios::fixed );
		out << "0x" << setw( 2 * size ) << setfill('0') << value;
		out.flags( oldflags );
		break;
	}

	case MOD_DEC:
	case MOD_NEG: {
		const ios_base::fmtflags oldflags = out.flags( ios::dec | ios::right | ios::fixed );
		if( (modifier & MOD_TYPEMASK) == MOD_DEC ) out << value;
		else out << nvalue;
		out.flags( oldflags );
		break;
	}

	case MOD_BIN: {
		for( int i = size * 8 - 1; i >= 0; i-- ) {
			out << ((value & ((uint64_t)1 << i)) ? '1' : '0');
			if( i > 0 && i % 4 == 0 ) out << ' ';
		}
		break;
	}

	case MOD_FLOAT: {
	    assert( (modifier & MOD_SIZEMASK) == MOD_64BIT );
	    double d = *(double*)&value;
	    out << d;
	    break;
	}

	}
}

void ASTNodePrint::print_array( std::ostream& out, Environment::array* array, int modifier )
{
    const size_t size = array->get_size();

    switch( modifier & MOD_ARRAYMASK ) {
    case MOD_ARRAY: {
        if( size ) {
            out << '[';
            for( size_t i = 0; i < size; i++ ) {
                out << ' ';
                print_value( out, array->get(i), modifier );
            }
            out << " ]";
        }
        else out << "[]";
        break;
    }

    case MOD_STRING: {
        out << ASTNodeString::get_string( array );
        break;
    }

    }
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeSleep implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeSleep::ASTNodeSleep( const yylloc_t& yylloc )
 : ASTNode( yylloc ),
   m_Mode( RETRIEVE_TIME )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeSleep" << endl;
#endif
}

ASTNodeSleep::ASTNodeSleep( const yylloc_t& yylloc, ASTNode::ptr expression, bool is_absolute )
 : ASTNode( yylloc ),
   m_Mode( is_absolute ? SLEEP_ABSOLUTE : SLEEP_RELATIVE )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeSleep " << (is_absolute ? "abs" : "rel") << " expression=[" << expression << "]" << endl;
#endif

    add_child( expression );
}

uint64_t ASTNodeSleep::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeSleep" << endl;
#endif

    struct timespec ts;

    if( m_Mode == RETRIEVE_TIME || m_Mode == SLEEP_RELATIVE ) {
        clock_gettime( CLOCK_MONOTONIC, &ts );
        if( m_Mode == RETRIEVE_TIME ) return ts.tv_sec * 1000000ULL + (ts.tv_nsec + 500) / 1000;
    }

    uint64_t time = get_children()[0]->execute() * 1000;

    if( m_Mode == SLEEP_ABSOLUTE ) {
        ts.tv_sec = time / 1000000000;
        ts.tv_nsec = time % 1000000000;
    }
    else {
        ts.tv_sec += time / 1000000000;
        ts.tv_nsec += time % 1000000000;
        if( ts.tv_nsec >= 1000000000 ) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
    }

    for(;;) {
        int ret = clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr );
        if( ret == 0 ) break;
        if( ret == EINTR ) {
            if( Environment::is_terminated() ) break;
            continue;
        }
        else {
            cerr << "nanosleep failed with errno " << ret << endl;
            break;
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeAssign implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeAssign::ASTNodeAssign( const yylloc_t& yylloc, Environment* env, std::string name )
 : ASTNode( yylloc ),
   m_Type( ARRAYLIST )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeAssign name=" << name << endl;
#endif

    m_LValue.array = env->alloc_array( name );

    if( !m_LValue.array ) throw ASTExceptionNamingConflict( get_location(), name );
}

ASTNodeAssign::ASTNodeAssign( const yylloc_t& yylloc, Environment* env, std::string name, std::string copy )
 : ASTNode( yylloc ),
   m_Type( ARRAYCOPY )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeAssign name=" << name << " copy=" << copy << endl;
#endif

    m_LValue.array = env->alloc_array( name );
    m_Copy = env->alloc_array( copy );

    if( !m_LValue.array ) throw ASTExceptionNamingConflict( get_location(), name );
    if( !m_Copy ) throw ASTExceptionNamingConflict( get_location(), copy );
}

ASTNodeAssign::ASTNodeAssign( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr expression )
 : ASTNode( yylloc ),
   m_Type( VAR )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeAssign name=" << name << " expression=[" << expression << "]" << endl;
#endif

	m_LValue.var = env->alloc_var( name );

	add_child( expression );

	if( !m_LValue.var ) throw ASTExceptionNamingConflict( get_location(), name );
}

ASTNodeAssign::ASTNodeAssign( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr index, ASTNode::ptr expression )
 : ASTNode( yylloc ),
   m_Type( ARRAY )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeAssign name=" << name << " index=[" << index <<"] expression=[" << expression << "]" << endl;
#endif

	m_LValue.array = env->alloc_array( name );

    add_child( index );
	add_child( expression );

	if( !m_LValue.array ) throw ASTExceptionNamingConflict( get_location(), name );
}

uint64_t ASTNodeAssign::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeAssign" << endl;
#endif

	switch( m_Type ) {
	case VAR:
	    if( m_LValue.var ) {
            uint64_t value = get_children()[0]->execute();
            m_LValue.var->set( value );
	    }
	    break;

	case ARRAY:
	    if( m_LValue.array ) {
            uint64_t index = get_children()[0]->execute();
            uint64_t value = get_children()[1]->execute();
            m_LValue.array->set( index, value );
	    }
	    break;

	case ARRAYLIST:
        if( m_LValue.array ) {
            size_t size = get_children().size();
            m_LValue.array->resize( size );
            for( size_t i = 0; i < size; i++ ) {
                m_LValue.array->set( i, get_children()[i]->execute() );
            }
        }
        break;

	case ARRAYCOPY:
	    if( m_LValue.array && m_Copy ) {
            size_t size = m_Copy->get_size();
            m_LValue.array->resize( size );
            for( size_t i = 0; i < size; i++ ) {
                m_LValue.array->set( i, m_Copy->get( i ) );
            }
	    }
        break;
	}

	return 0;
}

Environment::var* ASTNodeAssign::get_var()
{
    return ( m_Type == VAR ) ? m_LValue.var : nullptr;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeAssignArg implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeAssignArg::ASTNodeAssignArg( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr expression )
 : ASTNode( yylloc ),
   m_Env( env )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeAssignArg name=" << name << " expression=[" << expression << "]" << endl;
#endif

    m_Array = env->alloc_array( name );

    add_child( expression );

    if( !m_Array ) throw ASTExceptionNamingConflict( get_location(), name );
}

uint64_t ASTNodeAssignArg::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeAssignArg" << endl;
#endif

    const uint64_t num_args = m_Env->get_num_varargs();
    const uint64_t arg = get_children()[0]->execute();
    if( arg >= num_args ) throw ASTExceptionOutOfBounds( get_location(), arg, num_args );

    Environment::array* array = m_Env->get_vararg_array( arg );
    if( !array ) throw ASTExceptionArgTypeMismatch( get_location(), arg, false );

    const size_t size = array->get_size();
    m_Array->resize( size );

    for( size_t i = 0; i < size; i++ ) {
        m_Array->set( i, array->get(i) );
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeString implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeString::ASTNodeString( const yylloc_t& yylloc, Environment* env, std::string name, std::string str )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeString name=" << name << " str=" << str << endl;
#endif

    m_Array = env->alloc_array( name );
    m_String = str;

    if( !m_Array ) throw ASTExceptionNamingConflict( get_location(), name );
}

uint64_t ASTNodeString::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeString" << endl;
#endif

    set_string( m_Array, m_String );

    return 0;
}

bool ASTNodeString::get_array_result( Environment::array*& array )
{
    array = m_Array;
    return true;
}

size_t ASTNodeString::get_length( const Environment::array* array )
{
    const size_t size = array->get_size();
	size_t len = 0;

	for( size_t i = 0; i < size; i++ ) {
		element_t value;
		value.integer = array->get(i);

		for( size_t j = 0; j < 8; j++ ) {
			if( value.string[j] ) len++;
			else return len;
		}
	}

	return len;
}

std::string ASTNodeString::get_string( const Environment::array* array )
{
    const size_t size = array->get_size();
	string str;

	for( size_t i = 0; i < size; i++ ) {
		element_t value;
		value.integer = array->get(i);
		for( size_t j = 0; j < 8; j++ ) {
			uint8_t c = value.string[j];
			if( c ) str += c;
			else return str;
		}
	}

	return str;
}

void ASTNodeString::set_string( Environment::array* array, std::string str )
{
	const size_t size = (str.length() + 7) / 8;
	array->resize( size );

	for( size_t i = 0; i < size; i++ ) {
		element_t value;
		for( size_t j = 0; j < 8; j++ ) {
			size_t pos = 8 * i + j;
			value.string[j] = (pos < str.length()) ? str[pos] : 0;
		}
		array->set( i, value.integer );
	}
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeStatic implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeStatic::ASTNodeStatic( const yylloc_t& yylloc, Environment* env, std::string name )
 : ASTNode( yylloc ),
   m_Status( ARRAY )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeStatic name=" << name << endl;
#endif

    m_Data.array = env->alloc_static_array( name );

    if( !m_Data.array ) throw ASTExceptionNamingConflict( get_location(), name );
}

ASTNodeStatic::ASTNodeStatic( const yylloc_t& yylloc, Environment* env, std::string name, std::string from )
 : ASTNode( yylloc ),
   m_Status( ARRAY )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeStatic name=" << name << " from=" << from << endl;
#endif

    m_Data.array = env->alloc_static_array( name );
    m_Copy = env->get_array( from );

    if( !m_Data.array ) throw ASTExceptionNamingConflict( get_location(), name );
    if( !m_Copy ) throw ASTExceptionNamingConflict( get_location(), from );
}

ASTNodeStatic::ASTNodeStatic( const yylloc_t& yylloc, Environment* env, std::string name,
                              ASTNode::ptr expression, bool is_var )
 : ASTNode( yylloc ),
   m_Status( is_var ? VAR : EMPTY_ARRAY )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeStatic name=" << name << " expression=[" << expression << "] is_var=" << is_var << endl;
#endif

    if( is_var ) m_Data.var = env->alloc_static_var( name );
    else m_Data.array = env->alloc_static_array( name );

    if( expression->is_constant() ) add_child( make_shared<ASTNodeConstant>( yylloc, compiletime_execute( expression ) ) );
    else add_child( expression );

    if( is_var && !m_Data.var || !is_var && !m_Data.array ) throw ASTExceptionNamingConflict( get_location(), name );
}

uint64_t ASTNodeStatic::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeStatic" << endl;
#endif

    if( m_Status == INITIALIZED ) return 0;

    switch( m_Status ) {
    case INITIALIZED:
        break;

    case VAR:
        if( m_Data.var ) {
            uint64_t value = get_children()[0]->execute();
            m_Data.var->set( value );
            m_Status = INITIALIZED;
        }
        break;

    case EMPTY_ARRAY:
        if( m_Data.array )  {
            uint64_t value = get_children()[0]->execute();
            m_Data.array->resize( value );
            m_Status = INITIALIZED;
        }
        break;

    case ARRAY:
        if( m_Data.array ) {
            if( m_Copy ) {
            	size_t size = m_Copy->get_size();
            	m_Data.array->resize( size );
            	for( size_t i = 0; i < size; i++ ) {
            		m_Data.array->set( i, m_Copy->get( i ) );
            	}
            }
            else {
                size_t size = get_children().size();
				m_Data.array->resize( size );
				for( size_t i = 0; i < size; i++ ) {
					m_Data.array->set( i, get_children()[i]->execute() );
				}
            }
            m_Status = INITIALIZED;
        }
        break;

    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDef::ASTNodeDef( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr expression )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " expression=[" << expression << "]" << endl;
#endif

    const uint64_t value = compiletime_execute( expression );

    Environment::var* def = env->alloc_def_var( name );
    if( !def ) throw ASTExceptionNamingConflict( get_location(), name );
    def->set( value );
}

ASTNodeDef::ASTNodeDef( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr range_expr, ASTNode::ptr value_expr, int size )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " range=[" << size << "] range_expr=[" << range_expr << "] size=" << endl;
    switch( size ) {
    case T_8BIT: cerr << "1" << endl; break;
    case T_16BIT: cerr << "2" << endl; break;
    case T_32BIT: cerr << "4" << endl; break;
    case T_64BIT: cerr << "8" << endl; break;
    default: cerr << "ERR" << endl; break;
    }
#endif

    const uint64_t range = compiletime_execute( range_expr );
    const uint64_t value = compiletime_execute( value_expr );

    Environment::var* def = env->alloc_def_var( name );
    if( !def ) throw ASTExceptionNamingConflict( get_location(), name );
    def->set( value );
    def->set_range( range );

    switch( size ) {
    case T_8BIT: def->set_size(1); break;
    case T_16BIT: def->set_size(2); break;
    case T_32BIT: def->set_size(4); break;
    case T_64BIT: def->set_size(8); break;
    }
}

ASTNodeDef::ASTNodeDef( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr expression, std::string from )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " expression=[" << expression << "] from=" << from << endl;
#endif

    const uint64_t value = compiletime_execute( expression );

    Environment::var* def = env->alloc_def_var( name );
    if( !def ) throw ASTExceptionNamingConflict( get_location(), name );
    def->set( value );

    const Environment::var* from_base = env->get_var( from );
    if( !from_base || !from_base->is_def() ) throw ASTExceptionNamingConflict( get_location(), from );

    const uint64_t from_value = from_base->get();

    for( string member: env->get_struct_members( from ) ) {
        Environment::var* dst = env->alloc_def_var( name + '.' + member );
        const Environment::var* src = env->get_var( from + '.' + member );

        dst->set( src->get() - from_value );
        dst->set_range( src->get_range() );
        dst->set_size( src->get_size() );
    }
}

uint64_t ASTNodeDef::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeDef" << endl;
#endif

	// nothing to do

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDim implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDim::ASTNodeDim( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr size )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeDim name=" << name << " size=[" << size << "]" << endl;
#endif

    m_Array = env->alloc_array( name );
    if( !m_Array ) throw ASTExceptionNamingConflict( get_location(), name );

    add_child( size );
}

uint64_t ASTNodeDim::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeDim" << endl;
#endif

    uint64_t size = get_children()[0]->execute();
    m_Array->resize( size );

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeMap implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeMap::ASTNodeMap( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, ASTNode::ptr size )
 : ASTNodeMap( yylloc, env, compiletime_execute( address ), compiletime_execute( size ), "/dev/mem" )
{}

ASTNodeMap::ASTNodeMap( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, ASTNode::ptr at, ASTNode::ptr size )
 : ASTNodeMap( yylloc, env, compiletime_execute( address ), compiletime_execute( at ), compiletime_execute( size ), "/dev/mem" )
{}

ASTNodeMap::ASTNodeMap( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, ASTNode::ptr size, std::string device )
 : ASTNodeMap( yylloc, env, compiletime_execute( address ), compiletime_execute( size ), device )
{}

ASTNodeMap::ASTNodeMap( const yylloc_t& yylloc, Environment* env, ASTNode::ptr address, ASTNode::ptr at, ASTNode::ptr size, std::string device )
: ASTNodeMap( yylloc, env, compiletime_execute( address ), compiletime_execute( at ), compiletime_execute( size ), device )
{}

ASTNodeMap::ASTNodeMap( const yylloc_t& yylloc, Environment* env, uint64_t address, uint64_t size, std::string device )
 : ASTNodeMap( yylloc, env, address, address, size, device )
{}

ASTNodeMap::ASTNodeMap( const yylloc_t& yylloc, Environment* env, uint64_t address, uint64_t at, uint64_t size, std::string device )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
        cerr << "AST[" << this << "]: creating ASTNodeMap address=[" << address << "] at=[" << at << "] size=[" << size << "] device=" << device << endl;
#endif

    if( !env->map_memory( (void*)address, (void*)at, (size_t)size, device ) ) {
        throw ASTExceptionMappingFailure( get_location(), address, size, device );
    }
}

uint64_t ASTNodeMap::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeMap" << endl;
#endif

	// nothing to do

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeImport implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeImport::ASTNodeImport( const yylloc_t& yylloc, Environment* env, std::string file, bool run_once )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeImport file=" << file << endl;
#endif

	ASTNode::ptr yyroot = env->parse( get_location(), file.c_str(), true, run_once );
	add_child( yyroot );
}

uint64_t ASTNodeImport::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeImport" << endl;
#endif

	try {
		if( get_children().size() > 0 ) get_children()[0]->execute();
	}
    catch( ASTExceptionExit& ) {
        // nothing to do
    }
	catch( ASTExceptionBreak& ) {
		// nothing to do
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeUnaryOperator implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeUnaryOperator::ASTNodeUnaryOperator( const yylloc_t& yylloc, ASTNode::ptr expression, int op )
 : ASTNode( yylloc ),
   m_Operator( op )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeUnaryOperator expression=[" << expression << "] operator=" << op << endl;
#endif

	add_child( expression );

	if( expression->is_constant() ) set_constant();
}

uint64_t ASTNodeUnaryOperator::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeUnaryOperator" << endl;
#endif

	uint64_t r = get_children()[0]->execute();

	switch( m_Operator ) {
	case T_MINUS: return -r;
	case T_BIT_NOT: return ~r;
	case T_LOG_NOT: return r ? 0 : 0xffffffffffffffff;

	default: return 0;
	}
}

ASTNode::ptr ASTNodeUnaryOperator::clone_to_const()
{
    if( !is_constant() ) return nullptr;

#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: running const optimization" << endl;
#endif

    return make_shared<ASTNodeConstant>( get_location(), compiletime_execute( this ) );
}


/////////////////////////////////////////////////////////////////////////////
// class ASTNodeBinaryOperator implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeBinaryOperator::ASTNodeBinaryOperator( const yylloc_t& yylloc, ASTNode::ptr expression1, ASTNode::ptr expression2, int op )
 : ASTNode( yylloc ),
   m_Operator( op )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeBinaryOperator expression1=[" << expression1
	     << "] expression2=[" << expression2 << "] operator=" << op << endl;
#endif

    if( expression1->is_constant() && expression2->is_constant() ) set_constant();

	add_child( expression1 );
	add_child( expression2 );
}

uint64_t ASTNodeBinaryOperator::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeBinaryOperator" << endl;
#endif

	uint64_t r0 = get_children()[0]->execute();
	uint64_t r1 = get_children()[1]->execute();

	switch( m_Operator ) {
	case T_PLUS: return r0 + r1;
	case T_MINUS: return r0 - r1;
	case T_MUL: return r0 * r1;
	case T_DIV: if( r1 != 0 ) return r0 / r1; else throw ASTExceptionDivisionByZero( get_location() );
	case T_MOD: if( r1 != 0 ) return r0 % r1; else throw ASTExceptionDivisionByZero( get_location() );
	case T_SDIV: if( r1 != 0 ) return ((int64_t)r0 / (int64_t)r1); else throw ASTExceptionDivisionByZero( get_location() );
	case T_SMOD: if( r1 != 0 ) return ((int64_t)r0 % (int64_t)r1); else throw ASTExceptionDivisionByZero( get_location() );
	case T_LSHIFT: return r0 << r1;
	case T_RSHIFT: return r0 >> r1;
	case T_LT: return (r0 < r1) ? 0xffffffffffffffff : 0;
	case T_GT: return (r0 > r1) ? 0xffffffffffffffff : 0;
	case T_LE: return (r0 <= r1) ? 0xffffffffffffffff : 0;
	case T_GE: return (r0 >= r1) ? 0xffffffffffffffff : 0;
	case T_EQ: return (r0 == r1) ? 0xffffffffffffffff : 0;
	case T_NE: return (r0 != r1) ? 0xffffffffffffffff : 0;
    case T_SLT: return ((int64_t)r0 < (int64_t)r1) ? 0xffffffffffffffff : 0;
    case T_SGT: return ((int64_t)r0 > (int64_t)r1) ? 0xffffffffffffffff : 0;
    case T_SLE: return ((int64_t)r0 <= (int64_t)r1) ? 0xffffffffffffffff : 0;
    case T_SGE: return ((int64_t)r0 >= (int64_t)r1) ? 0xffffffffffffffff : 0;
	case T_BIT_AND: return r0 & r1;
	case T_BIT_XOR: return r0 ^ r1;
	case T_BIT_OR: return r0 | r1;
	case T_LOG_AND: return ((r0 != 0) && (r1 != 0)) ? 0xffffffffffffffff : 0;
	case T_LOG_XOR: return ((r0 != 0) && (r1 == 0) || (r0 == 0) && (r1 != 0)) ? 0xffffffffffffffff : 0;
	case T_LOG_OR: return ((r0 != 0) || (r1 != 0)) ? 0xffffffffffffffff : 0;

	default: return 0;
	}
}

ASTNode::ptr ASTNodeBinaryOperator::clone_to_const()
{
    if( !is_constant() ) return nullptr;

#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: running const optimization" << endl;
#endif

    return make_shared<ASTNodeConstant>( get_location(), compiletime_execute( this ) );
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRestriction implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeRestriction::ASTNodeRestriction( const yylloc_t& yylloc, ASTNode::ptr node, int size_restriction )
 : ASTNode( yylloc ),
   m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeRestriction node=[" << node << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cerr << "8" << endl; break;
	case T_16BIT: cerr << "16" << endl; break;
	case T_32BIT: cerr << "32" << endl; break;
	case T_64BIT: cerr << "64" << endl; break;
	default: cerr << "ERR" << endl; break;
	}
#endif

	add_child( node );
}

uint64_t ASTNodeRestriction::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeRestriction" << endl;
#endif

	ASTNode::ptr node = get_children()[0];

	uint64_t result = node->execute();

	switch( m_SizeRestriction ) {
	case T_8BIT: result &= 0xff; break;
	case T_16BIT: result &= 0xffff; break;
	case T_32BIT: result &= 0xffffffff; break;
	}

	return result;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeVar implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeVar::ASTNodeVar( const yylloc_t& yylloc, Environment* env, std::string name )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << endl;
#endif

	m_Var = env->get_var( name );

    if( !m_Var ) throw ASTExceptionUndefinedVar( get_location(), name );

    if( m_Var->is_def() ) set_constant();
}

uint64_t ASTNodeVar::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeVar" << endl;
#endif

	if( m_Var ) return m_Var->get();
	else return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeArg implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeArg::ASTNodeArg( const yylloc_t& yylloc, Environment* env )
 : ASTNode( yylloc ),
   m_Env( env )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeArg" << endl;
#endif
}

ASTNodeArg::ASTNodeArg( const yylloc_t& yylloc, Environment* env, ASTNode::ptr arg_index, query_t query )
 : ASTNode( yylloc ),
   m_Env( env ),
   m_Query( query )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeArg arg_index=[" << arg_index << "]" << endl;
#endif
    add_child( arg_index );
}

ASTNodeArg::ASTNodeArg( const yylloc_t& yylloc, Environment* env, ASTNode::ptr arg_index, ASTNode::ptr array_index )
 : ASTNode( yylloc ),
   m_Env( env )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeArg arg_index=[" << arg_index << "] array_index=[" << array_index << "]" << endl;
#endif
    add_child( arg_index );
    add_child( array_index );
}

uint64_t ASTNodeArg::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeArg" << endl;
#endif

    const uint64_t num_args = m_Env->get_num_varargs();

    if( get_children().size() == 0 ) return num_args;

    const uint64_t arg = get_children()[0]->execute();
    if( arg >= num_args ) throw ASTExceptionOutOfBounds( get_location(), arg, num_args );

    Environment::array* array = m_Env->get_vararg_array( arg );

    if( get_children().size() == 1 ) {
        switch( m_Query ) {
        case GET_TYPE: {
            return array ? 0xffffffffffffffff : 0;
        } break;

        case GET_VAR: {
            if( array ) throw ASTExceptionArgTypeMismatch( get_location(), arg, true );
            else return m_Env->get_vararg_value( arg );
        } break;

        case GET_ARRAYSIZE: {
            if( array ) return array->get_size();
            else throw ASTExceptionArgTypeMismatch( get_location(), arg, false );
        } break;
        }
    }
    else {
        const uint64_t index = get_children()[1]->execute();
        if( array ) return array->get( index );
        else throw ASTExceptionArgTypeMismatch( get_location(), arg, false );
    }
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRange implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeRange::ASTNodeRange( const yylloc_t& yylloc, Environment* env, std::string name )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeRange name=" << name << endl;
#endif

    m_Var = env->get_var( name );

    if( !m_Var || !m_Var->is_def() ) throw ASTExceptionUndefinedVar( get_location(), name );

    set_constant();
}

ASTNodeRange::ASTNodeRange( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr index )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeRange name=" << name << " index=[" << index << "]" << endl;
#endif

    m_Var = env->get_var( name );

    if( !m_Var || !m_Var->is_def() ) throw ASTExceptionUndefinedVar( get_location(), name );

    add_child( index );
    if( index->is_constant() ) set_constant();
}

uint64_t ASTNodeRange::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeRange" << endl;
#endif

    if( !m_Var ) return 0;

    uint64_t range = m_Var->get_range();
    if( get_children().size() == 0 ) return range;

    uint64_t index = get_children()[0]->execute();
    if( index >= range ) throw ASTExceptionOutOfBounds( get_location(), index, range );

    uint64_t value = m_Var->get();
    return value + m_Var->get_size() * index;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeArray implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeArray::ASTNodeArray( const yylloc_t& yylloc, Environment* env, std::string name )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeArray name=" << name << endl;
#endif

    m_Array = env->get_array( name );

    if( !m_Array ) throw ASTExceptionUndefinedVar( get_location(), name );
}

ASTNodeArray::ASTNodeArray( const yylloc_t& yylloc, Environment* env, std::string name, ASTNode::ptr index )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeArray name=" << name << " index=[" << index << "]" << endl;
#endif

    m_Array = env->get_array( name );

    add_child( index );

    if( !m_Array ) throw ASTExceptionUndefinedVar( get_location(), name );
}

uint64_t ASTNodeArray::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeArray" << endl;
#endif

    if( get_children().size() == 0 ) return m_Array->get_size();

    uint64_t index = get_children()[0]->execute();
    return m_Array ? m_Array->get( index ) : 0;
}

bool ASTNodeArray::get_array_result( Environment::array*& array )
{
    array = m_Array;
    return true;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeConstant implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeConstant::ASTNodeConstant( const yylloc_t& yylloc, std::string str, bool is_float )
 : ASTNode( yylloc )
{
	m_Value = is_float ? Environment::parse_float( str ) : Environment::parse_int( str );

#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeConstant value=" << m_Value << endl;
#endif

	set_constant();
}

ASTNodeConstant::ASTNodeConstant( const yylloc_t& yylloc, uint64_t value )
 : ASTNode( yylloc )
{
    m_Value = value;

#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeConstant value=" << m_Value << endl;
#endif

    set_constant();
}

uint64_t ASTNodeConstant::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeConstant" << endl;
#endif

	return m_Value;
};
