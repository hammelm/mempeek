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

#include "mempeek_ast.h"

#include "mempeek_exceptions.h"
#include "mempeek_parser.h"
#include "parser.h"
#include "lexer.h"

#include <iostream>
#include <iomanip>

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ASTNode implementation
//////////////////////////////////////////////////////////////////////////////

Environment ASTNode::s_Environment;
volatile bool ASTNode::s_IsTerminated = false;

std::vector< std::string > ASTNode::s_IncludePaths;


ASTNode::ASTNode( const yylloc_t* yylloc )
{
	if( yylloc ) {
		m_Location.file = yylloc->file ? yylloc->file : "";
		m_Location.first_line = yylloc->first_line;
		m_Location.last_line = yylloc->last_line;
	}
	else {
		m_Location.file = "";
		m_Location.first_line = 0;
		m_Location.last_line = 0;
	}
}

ASTNode::~ASTNode()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: destructing" << endl;
#endif
}

uint64_t ASTNode::parse_int( string str )
{
	uint64_t value = 0;

	if( str.length() > 2 )
	{
		if( str[0] == '0' && str[1] == 'b' ) {
			for( size_t i = 2; i < str.length(); i++ ) {
				value <<= 1;
				if( str[i] == '1' ) value |= 1;
				else if( str[i] != '0' ) return 0;
			}
			return value;
		}

		if( str[0] == '0' && str[1] == 'x' ) {
			istringstream stream( str );
			stream >> hex >> value;
			if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return value;
			else return 0;
		}
	}

	istringstream stream( str );
	stream >> dec >> value;
	if( !stream.fail() && !stream.bad() && (stream.eof() || (stream >> ws).eof()) ) return value;
	else return 0;
}

int ASTNode::get_default_size()
{
	switch( sizeof(long) ) {
	case 2: return T_16BIT;
	case 8: return T_64BIT;
	default: return T_32BIT;
	}
}

ASTNode::ptr ASTNode::parse( const char* str, bool is_file )
{
	location_t location = { "", 0, 0 };
	return parse( location, str, is_file );
}

ASTNode::ptr ASTNode::parse( const location_t& location, const char* str, bool is_file )
{
    ASTNode::ptr yyroot = nullptr;
    FILE* file = nullptr;

    if( is_file ) {
        file = fopen( str, "r" );
        if( !file ) {
            for( string path: s_IncludePaths ) {
                if( path.length() > 0 && path.back() != '/' ) path += '/';
                path += str;
                file = fopen( path.c_str(), "r" );
                if( file ) break;
            }
        }

        if( !file ) throw ASTExceptionFileNotFound( location, str );
    }

    yyscan_t scanner;
    yylex_init( &scanner );
    yyset_extra( is_file ? str : nullptr, scanner );

    YY_BUFFER_STATE lex_buffer;
    if( file ) lex_buffer = yy_create_buffer( file, YY_BUF_SIZE, scanner );
    else lex_buffer = yy_scan_string( str, scanner );

    yy_switch_to_buffer( lex_buffer, scanner );

    try {
        yyparse( scanner, yyroot );
    }
    catch( ... ) {
        yy_delete_buffer( lex_buffer, scanner );
        yylex_destroy( scanner );

        if( is_file  ) fclose( file );

        throw;
    }

    yy_delete_buffer( lex_buffer, scanner );
    yylex_destroy( scanner );

    if( file  ) fclose( file );

    return yyroot;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBreak
//////////////////////////////////////////////////////////////////////////////

ASTNodeBreak::ASTNodeBreak( const yylloc_t* yylloc, int token )
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
    case T_BREAK: throw ASTExceptionBreak();
    case T_QUIT: throw ASTExceptionQuit();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBlock implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeBlock::ASTNodeBlock( const yylloc_t* yylloc )
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
        if( is_terminated() ) throw ASTExceptionTerminate();
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeIf implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeIf::ASTNodeIf( const yylloc_t* yylloc, ASTNode::ptr condition, ASTNode::ptr then_block )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeIf condition=[" << condition
		 << "] then_block=[" << then_block << "]" << endl;
#endif

	add_child( condition );
	add_child( then_block );
}

ASTNodeIf::ASTNodeIf( const yylloc_t* yylloc, ASTNode::ptr condition, ASTNode::ptr then_block, ASTNode::ptr else_block )
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

ASTNodeWhile::ASTNodeWhile( const yylloc_t* yylloc, ASTNode::ptr condition, ASTNode::ptr block )
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
	    catch( ASTNodeBreak& ) {
	        break;
	    }
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeFor
//////////////////////////////////////////////////////////////////////////////

ASTNodeFor::ASTNodeFor( const yylloc_t* yylloc, ASTNodeAssign::ptr var, ASTNode::ptr to )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeFor var=[" << var << "] to=[" << to << "]" << endl;
#endif

    m_Var = var->get_var();

    add_child( var );
    add_child( to );
}

ASTNodeFor::ASTNodeFor( const yylloc_t* yylloc, ASTNodeAssign::ptr var, ASTNode::ptr to, ASTNode::ptr step )
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
        catch( ASTNodeBreak& ) {
            break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePeek implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePeek::ASTNodePeek( const yylloc_t* yylloc, ASTNode::ptr address, int size_restriction )
 : ASTNode( yylloc ),
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
	MMap* mmap = get_environment().get_mapping( address, sizeof(T) );

    if( !mmap ) throw ASTExceptionNoMapping( get_location(), address, sizeof(T) );

    uint64_t ret = mmap->peek<T>( address );
    if( mmap->has_failed() ) throw ASTExceptionBusError( get_location(), address, sizeof(T) );

    return ret;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePoke implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePoke::ASTNodePoke( const yylloc_t* yylloc, ASTNode::ptr address, ASTNode::ptr value, int size_restriction )
 : ASTNode( yylloc ),
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

ASTNodePoke::ASTNodePoke( const yylloc_t* yylloc, ASTNode::ptr address, ASTNode::ptr value, ASTNode::ptr mask, int size_restriction )
 : ASTNode( yylloc ),
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

	MMap* mmap = get_environment().get_mapping( address, sizeof(T) );

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

ASTNodePrint::ASTNodePrint( const yylloc_t* yylloc )
 : ASTNode( yylloc ),
   m_Text( "\n" )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint newline" << endl;
#endif
}

ASTNodePrint::ASTNodePrint( const yylloc_t* yylloc, std::string text )
 : ASTNode( yylloc ),
   m_Text( text )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint text=\"" << text << "\"" << endl;
#endif
}

ASTNodePrint::ASTNodePrint( const yylloc_t* yylloc, ASTNode::ptr expression, int modifier )
 : ASTNode( yylloc ),
   m_Modifier( modifier )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint expression=[" << expression << "] modifier=0x"
		 << hex << setw(2) << setfill('0') << modifier << dec << endl;
#endif

	add_child( expression );
}

uint64_t ASTNodePrint::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodePrint" << endl;
#endif

	for( ASTNode::ptr node: get_children() ) print_value( cout, node->execute() );
	cout << m_Text << flush;

	return 0;
}

int ASTNodePrint::get_default_size()
{
	switch( ASTNode::get_default_size() ) {
	case T_16BIT: return MOD_16BIT;
	case T_32BIT: return MOD_32BIT;
	case T_64BIT: return MOD_64BIT;
	default: return 0;
	}
}

void ASTNodePrint::print_value( std::ostream& out, uint64_t value )
{
	int size = 0;
	int64_t nvalue;

	switch( m_Modifier & MOD_SIZEMASK ) {
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

	switch( m_Modifier & MOD_TYPEMASK ) {
	case MOD_HEX: {
		const ios_base::fmtflags oldflags = out.flags( ios::hex | ios::right | ios::fixed );
		out << "0x" << setw( 2 * size ) << setfill('0') << value;
		out.flags( oldflags );
		break;
	}

	case MOD_DEC:
	case MOD_NEG: {
		const ios_base::fmtflags oldflags = out.flags( ios::dec | ios::right | ios::fixed );
		if( (m_Modifier & MOD_TYPEMASK) == MOD_DEC ) out << value;
		else out << nvalue;
		out.flags( oldflags );
		break;
	}

	case MOD_BIN: {
		for( int i = size * 8 - 1; i >= 0; i-- ) {
			out << ((value & (1 << i)) ? '1' : '0');
			if( i > 0 && i % 4 == 0 ) out << ' ';
		}
		break;
	}

	}
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeSleep implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeSleep::ASTNodeSleep( const yylloc_t* yylloc, ASTNode::ptr expression )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeSleep expression=[" << expression << "]" << endl;
#endif

    add_child( expression );
}

uint64_t ASTNodeSleep::execute()
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: executing ASTNodeSleep" << endl;
#endif

    uint64_t time = get_children()[0]->execute() * 1000;

    struct timespec ts;
    ts.tv_sec = time / 1000000000;
    ts.tv_nsec = time % 1000000000;

    for(;;) {
        struct timespec remaining;
        int ret = nanosleep( &ts, &remaining );
        if( ret == 0 ) break;
        if( errno == EINTR ) {
            if( is_terminated() ) break;
            ts = remaining;
            continue;
        }
        else {
            cerr << "nanosleep failed with errno " << errno << endl;
            break;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// class ASTNodeAssign implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeAssign::ASTNodeAssign( const yylloc_t* yylloc, std::string name, ASTNode::ptr expression )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeAssign name=" << name << " expression=[" << expression << "]" << endl;
#endif

	m_Var = get_environment().alloc_var( name );

	add_child( expression );

	if( !m_Var ) throw ASTExceptionNamingConflict( get_location(), name );
}

uint64_t ASTNodeAssign::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeAssign" << endl;
#endif

	uint64_t value = get_children()[0]->execute();
	if( m_Var ) m_Var->set( value );

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDef::ASTNodeDef( const yylloc_t* yylloc, std::string name, ASTNode::ptr expression )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " expression=[" << expression << "]" << endl;
#endif

	m_Def = get_environment().alloc_def( name );
	m_FromBase = nullptr;

	add_child( expression );

	if( !m_Def ) throw ASTExceptionNamingConflict( get_location(), name );
}

ASTNodeDef::ASTNodeDef( const yylloc_t* yylloc, std::string name, ASTNode::ptr expression, std::string from )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " expression=[" << expression << "] from=" << from << endl;
#endif

	m_Def = get_environment().alloc_def( name );

	m_FromBase = get_environment().get( from );
	if( m_FromBase && !m_FromBase->is_def() ) m_FromBase = nullptr;

	add_child( expression );

	if( !m_Def ) throw ASTExceptionNamingConflict( get_location(), name );
    if( !m_FromBase ) throw ASTExceptionUndefinedVar( get_location(), from );

    for( string member: get_environment().get_struct_members( from ) ) {
        Environment::var* dst = get_environment().alloc_def( name + '.' + member );
        const Environment::var* src = get_environment().get( from + '.' + member );
        m_FromMembers.push_back( make_pair( dst, src ) );
    }
}

uint64_t ASTNodeDef::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeDef" << endl;
#endif

	uint64_t value = get_children()[0]->execute();
	if( m_Def ) m_Def->set( value );

	if( m_FromBase ) {
		uint64_t base = m_FromBase->get();
		for( auto member: m_FromMembers ) {
			member.first->set( member.second->get() - base );
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeMap implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeMap::ASTNodeMap( const yylloc_t* yylloc, std::string address, std::string size )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeMap address=" << address << " size=" << size << endl;
#endif

	uint64_t phys_addr = parse_int( address );
	uint64_t mapping_size = parse_int( size );

	if( !get_environment().map_memory( (void*)phys_addr, (size_t)mapping_size, "/dev/mem" ) ) {
		throw ASTExceptionMappingFailure( get_location(), address, size, "/dev/mem" );
	}
}

ASTNodeMap::ASTNodeMap( const yylloc_t* yylloc, std::string address, std::string size, std::string device )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
    cerr << "AST[" << this << "]: creating ASTNodeMap address=" << address << " size=" << size << " device=" << device << endl;
#endif

    uint64_t phys_addr = parse_int( address );
    uint64_t mapping_size = parse_int( size );

    if( !get_environment().map_memory( (void*)phys_addr, (size_t)mapping_size, device ) ) {
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

ASTNodeImport::ASTNodeImport( const yylloc_t* yylloc, std::string file )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeImport file=" << file << endl;
#endif

	ASTNode::ptr yyroot = ASTNode::parse( get_location(), file.c_str(), true );
	add_child( yyroot );
}

uint64_t ASTNodeImport::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeImport" << endl;
#endif

	try {
		get_children()[0]->execute();
	}
	catch( ASTExceptionBreak& ) {
		// nothing to do
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeUnaryOperator implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeUnaryOperator::ASTNodeUnaryOperator( const yylloc_t* yylloc, ASTNode::ptr expression, int op )
 : ASTNode( yylloc ),
   m_Operator( op )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeUnaryOperator expression=[" << expression << "] operator=" << op << endl;
#endif

	add_child( expression );
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


/////////////////////////////////////////////////////////////////////////////
// class ASTNodeBinaryOperator implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeBinaryOperator::ASTNodeBinaryOperator( const yylloc_t* yylloc, ASTNode::ptr expression1, ASTNode::ptr expression2, int op )
 : ASTNode( yylloc ),
   m_Operator( op )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeBinaryOperator expression1=[" << expression1
	     << "] expression2=[" << expression2 << "] operator=" << op << endl;
#endif

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
	case T_LSHIFT: return r0 << r1;
	case T_RSHIFT: return r0 >> r1;
	case T_LT: return (r0 < r1) ? 0xffffffffffffffff : 0;
	case T_GT: return (r0 > r1) ? 0xffffffffffffffff : 0;
	case T_LE: return (r0 <= r1) ? 0xffffffffffffffff : 0;
	case T_GE: return (r0 >= r1) ? 0xffffffffffffffff : 0;
	case T_EQ: return (r0 == r1) ? 0xffffffffffffffff : 0;
	case T_NE: return (r0 != r1) ? 0xffffffffffffffff : 0;
	case T_BIT_AND: return r0 & r1;
	case T_BIT_XOR: return r0 ^ r1;
	case T_BIT_OR: return r0 | r1;
	case T_LOG_AND: return ((r0 != 0) && (r1 != 0)) ? 0xffffffffffffffff : 0;
	case T_LOG_XOR: return ((r0 != 0) && (r1 == 0) || (r0 == 0) && (r1 != 0)) ? 0xffffffffffffffff : 0;
	case T_LOG_OR: return ((r0 != 0) || (r1 != 0)) ? 0xffffffffffffffff : 0;

	default: return 0;
	}
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRestriction implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeRestriction::ASTNodeRestriction( const yylloc_t* yylloc, ASTNode::ptr node, int size_restriction )
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

ASTNodeVar::ASTNodeVar( const yylloc_t* yylloc, std::string name )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << endl;
#endif

	m_Var = get_environment().get( name );

    if( !m_Var ) throw ASTExceptionUndefinedVar( get_location(), name );
}

ASTNodeVar::ASTNodeVar( const yylloc_t* yylloc, std::string name, ASTNode::ptr index )
 : ASTNode( yylloc )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << " index=[" << index << "]" << endl;
#endif

	m_Var = get_environment().get( name );

	add_child( index );

    if( !m_Var ) throw ASTExceptionUndefinedVar( get_location(), name );
}

uint64_t ASTNodeVar::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeVar" << endl;
#endif

	uint64_t offset = 0;
	if( get_children().size() > 0 ) offset = get_children()[0]->execute();

	if( m_Var ) return m_Var->get() + offset;
	else return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeConstant implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeConstant::ASTNodeConstant( const yylloc_t* yylloc, std::string str )
 : ASTNode( yylloc )
{
	m_Value = parse_int( str );

#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeConstant value=" << m_Value << endl;
#endif
}

uint64_t ASTNodeConstant::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeConstant" << endl;
#endif

	return m_Value;
};
