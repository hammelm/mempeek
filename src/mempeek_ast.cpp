#include "mempeek_ast.h"

#include <sstream>

#include "parser.hpp"

#ifdef ASTDEBUG
#include <iostream>
#endif

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
// class ASTNodePoke implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodePoke::ASTNodePoke( ASTNode* address, ASTNode* value, int size_restriction )
 : m_SizeRestriction( size_restriction )
{
	push_back( address );
	push_back( value );

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
}

ASTNodePoke::ASTNodePoke( ASTNode* address, ASTNode* value, ASTNode* mask, int size_restriction )
 : m_SizeRestriction( size_restriction )
{
	push_back( address );
	push_back( value );
	push_back( mask );

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
}

void ASTNodePoke::execute()
{
	// FIXME: to be implemented
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address )
{
	push_back( address );

#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "]" << endl;
#endif
}

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address, std::string from )
{
	push_back( address );

#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "] from=" << from << endl;
#endif
}

void ASTNodeDef::execute()
{
	// FIXME: to be implemented
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRestriction implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeRestriction::ASTNodeRestriction( ASTNode* node, int size_restriction )
 : m_SizeRestriction( size_restriction )
{
	push_back( node );

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
}

void ASTNodeRestriction::execute()
{
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
	push_back( index );

#ifdef ASTDEBUG
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << " index=[" << index << "]" << endl;
#endif
}

void ASTNodeVar::execute()
{
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
	// nothing to do
};
