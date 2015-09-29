#include "mempeek_ast.h"

#include <iostream>
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


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef implementation
//////////////////////////////////////////////////////////////////////////////

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address )
{
	push_back( address );
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "]" << endl;
}

ASTNodeDef::ASTNodeDef( std::string name, ASTNode* address, std::string from )
{
	push_back( address );
	cerr << "AST[" << this << "]: creating ASTNodeDef name=" << name << " address=[" << address << "] from=" << from << endl;
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
	cerr << "AST[" << this << "]: creating ASTNodeRestriction node=[" << node << "] restriction=";
	switch( m_SizeRestriction ) {
	case T_8BIT: cout << "8" << endl; break;
	case T_16BIT: cout << "16" << endl; break;
	case T_32BIT: cout << "32" << endl; break;
	case T_64BIT: cout << "64" << endl; break;
	default: cout << "ERR" << endl; break;
	}
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
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << endl;
}

ASTNodeVar::ASTNodeVar( std::string name, ASTNode* index )
{
	push_back( index );
	cerr << "AST[" << this << "]: creating ASTNodeVar name=" << name << " index=[" << index << "]" << endl;
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
	cerr << "AST[" << this << "]: creating ASTNodeConstant value=" << m_IntResult << endl;
}

void ASTNodeConstant::execute()
{
	// nothing to do
};
