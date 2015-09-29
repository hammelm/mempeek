#include "mempeek_ast.h"

#include <sstream>

#include "parser.hpp"

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ASTNode implementation
//////////////////////////////////////////////////////////////////////////////

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
	switch( sizeof(int) ) {
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

void ASTNodeBlock::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeBlock" << endl;
#endif

	for( ASTNode* node: get_children() ) {
		node->execute();
	}
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

void ASTNodeWhile::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeWhile" << endl;
#endif

	ASTNode* condition = get_children()[0];
	ASTNode* block = get_children()[1];

	for(;;) {
		condition->execute();
		if( condition->get_int_result() == 0 ) break;
		block->execute();
	}
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

void ASTNodePoke::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodePoke" << endl;
#endif

	// FIXME: to be implemented
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "]" << endl;
#endif

	push_back( address );
}

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address, std::string from )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "] from=" << from << endl;
#endif

	push_back( address );
}

void ASTNodeDef::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeDef" << endl;
#endif

	// FIXME: to be implemented
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

void ASTNodeRestriction::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeRestriction" << endl;
#endif

	ASTNode* node = get_children()[0];

	node->execute();
	m_IntResult = node->get_int_result();

	switch( m_SizeRestriction ) {
	case T_8BIT: m_IntResult &= 0xff; break;
	case T_16BIT: m_IntResult &= 0xffff; break;
	case T_32BIT: m_IntResult &= 0xffffffff; break;
	}
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeVar implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeVar::ASTNodeVar( std::string name )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << endl;
#endif
}

ASTNodeVar::ASTNodeVar( std::string name, ASTNode* index )
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << " index=[" << index << "]" << endl;
#endif

	push_back( index );
}

void ASTNodeVar::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeVar" << endl;
#endif

	// FIXME: to be implemented
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeConstant implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeConstant::ASTNodeConstant( std::string str )
{
	m_IntResult = parse_int( str );

#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeConstant value=" << m_IntResult << endl;
#endif
}

void ASTNodeConstant::execute()
{
#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: executing ASTNodeConstant" << endl;
#endif

	// nothing to do
};
