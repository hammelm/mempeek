#include "mempeek_ast.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "parser.hpp"

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ASTNode implementation
//////////////////////////////////////////////////////////////////////////////

Environment ASTNode::s_Environment;

ASTNode::~ASTNode()
{
	for( ASTNode* child: m_Children ) {
		delete child;
	}
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
	case 4: return T_32BIT;
	case 8: return T_64BIT;
	default: return 0;
	}
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBlock implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeBlock::ASTNodeBlock()
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

	for( ASTNode* node: get_children() ) {
		node->execute();
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeWhile implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeWhile::ASTNodeWhile( ASTNode* condition, ASTNode* block )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeWhile condition=[" << condition << "] block=[" << block << "]" << endl;
#endif

	push_back( condition );
	push_back( block );
}

uint64_t ASTNodeWhile::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeWhile" << endl;
#endif

	ASTNode* condition = get_children()[0];
	ASTNode* block = get_children()[1];

	while( condition->execute() ) {
		block->execute();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// class ASTNodePoke implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePoke::ASTNodePoke( ASTNode* address, ASTNode* value, int size_restriction )
 : m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePoke address=[" << address << "] value=[" << value << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cout << "8" << endl; break;
	case T_16BIT: cout << "16" << endl; break;
	case T_32BIT: cout << "32" << endl; break;
	case T_64BIT: cout << "64" << endl; break;
	default: cout << "ERR" << endl; break;
	}
#endif

	push_back( address );
	push_back( value );
}

ASTNodePoke::ASTNodePoke( ASTNode* address, ASTNode* value, ASTNode* mask, int size_restriction )
 : m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePoke address=[" << address << "] value=[" << value
		 << "] mask=[" << mask << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cout << "8" << endl; break;
	case T_16BIT: cout << "16" << endl; break;
	case T_32BIT: cout << "32" << endl; break;
	case T_64BIT: cout << "64" << endl; break;
	default: cout << "ERR" << endl; break;
	}
#endif

	push_back( address );
	push_back( value );
	push_back( mask );
}

uint64_t ASTNodePoke::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodePoke" << endl;
#endif

	// FIXME: to be implemented

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePrint implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePrint::ASTNodePrint()
 : m_Text( "\n" )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint newline" << endl;
#endif
}

ASTNodePrint::ASTNodePrint( std::string text )
 : m_Text( text )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint text=\"" << text << "\"" << endl;
#endif
}

ASTNodePrint::ASTNodePrint( ASTNode* expression, int modifier )
 : m_Modifier( modifier )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodePrint expression=[" << expression << "] modifier=0x"
		 << hex << setw(2) << setfill('0') << modifier << dec << endl;
#endif

	push_back( expression );
}

uint64_t ASTNodePrint::execute()
{
#ifdef ASTDEBUG

	cerr << "AST[" << this << "]: executing ASTNodePrint" << endl;

	for( ASTNode* node: get_children() ) {
		uint64_t result = node->execute();
		cout << "printing expression: ";
		print_value( cout, result );
		cout << endl;
	}

	if( m_Text == "\n" ) cout << "printing newline" << endl;
	else if( m_Text != "" ) cout << "printing text: \"" << m_Text << "\"" << endl;

#else

	for( ASTNode* node: get_children() ) {
		print_value( cout, node->execute() );
	}

	cout << m_Text;

	if( m_Text == "\n" ) cout << flush;

#endif

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
		out << "0x" << setw(size) << setfill('0') << value;
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
// class ASTNodeDef implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "]" << endl;
#endif

	m_Def = get_environment().set_def( name );

	push_back( address );
}

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address, std::string from )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "] from=" << from << endl;
#endif

	m_Def = get_environment().set_def( name );

	push_back( address );
}

uint64_t ASTNodeDef::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeDef" << endl;
#endif

	uint64_t address = get_children()[0]->execute();
	if( m_Def ) *m_Def = address;

	// FIXME: "from" not implemented

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRestriction implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeRestriction::ASTNodeRestriction( ASTNode* node, int size_restriction )
 : m_SizeRestriction( size_restriction )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeRestriction node=[" << node << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cout << "8" << endl; break;
	case T_16BIT: cout << "16" << endl; break;
	case T_32BIT: cout << "32" << endl; break;
	case T_64BIT: cout << "64" << endl; break;
	default: cout << "ERR" << endl; break;
	}
#endif

	push_back( node );
}

uint64_t ASTNodeRestriction::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeRestriction" << endl;
#endif

	ASTNode* node = get_children()[0];

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

ASTNodeVar::ASTNodeVar( std::string name )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << endl;
#endif

	m_Var = get_environment().get( name );
}

ASTNodeVar::ASTNodeVar( std::string name, ASTNode* index )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << " index=[" << index << "]" << endl;
#endif

	m_Var = get_environment().get( name );

	push_back( index );
}

uint64_t ASTNodeVar::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeVar" << endl;
#endif

	uint64_t offset = 0;
	if( get_children().size() > 0 ) offset = get_children()[0]->execute();

	if( m_Var ) return *m_Var + offset;
	else return 0;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeConstant implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeConstant::ASTNodeConstant( std::string str )
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
