#ifndef __mempeek_ast_h__
#define __mempeek_ast_h__

#include <string>
#include <deque>

#include <stdint.h>

#ifdef ASTDEBUG
#include <iostream>
#endif


//////////////////////////////////////////////////////////////////////////////
// class ASTNode
//////////////////////////////////////////////////////////////////////////////

class ASTNode {
public:
	ASTNode() {}
	virtual ~ASTNode();

	void push_front( ASTNode* node );
	void push_back( ASTNode* node );

	virtual void execute() = 0;

	uint64_t get_int_result();
	std::string get_string_result();

	static int get_default_size();

protected:
	typedef std::deque< ASTNode* > nodelist_t;

	const nodelist_t& get_children();

	static uint64_t parse_int( std::string str );

	uint64_t m_IntResult = 0;
	std::string m_StringResult = "";

private:
	nodelist_t m_Children;

	ASTNode( const ASTNode& ) = delete;
	ASTNode& operator=( const ASTNode& ) = delete;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBlock
//////////////////////////////////////////////////////////////////////////////

class ASTNodeBlock : public ASTNode {
public:
	ASTNodeBlock();

	void execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePoke
//////////////////////////////////////////////////////////////////////////////

class ASTNodePoke : public ASTNode {
public:
	ASTNodePoke( ASTNode* address, ASTNode* value, int size_restriction );
	ASTNodePoke( ASTNode* address, ASTNode* value, ASTNode* mask, int size_restriction );

	void execute() override;

private:
	int m_SizeRestriction;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef
//////////////////////////////////////////////////////////////////////////////

class ASTNodeDef : public ASTNode {
public:
	ASTNodeDef( std::string name, ASTNode* address );
	ASTNodeDef( std::string name, ASTNode* address, std::string from );

	void execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRestriction
//////////////////////////////////////////////////////////////////////////////

class ASTNodeRestriction : public ASTNode {
public:
	ASTNodeRestriction( ASTNode* node, int size_restriction );

	void execute() override;

private:
	int m_SizeRestriction;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeVar
//////////////////////////////////////////////////////////////////////////////

class ASTNodeVar : public ASTNode {
public:
	ASTNodeVar( std::string name );
	ASTNodeVar( std::string name, ASTNode* index );

	void execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeConstant
//////////////////////////////////////////////////////////////////////////////

class ASTNodeConstant : public ASTNode {
public:
	ASTNodeConstant( std::string str );

	void execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNode inline functions
//////////////////////////////////////////////////////////////////////////////

inline void ASTNode::push_front( ASTNode* node )
{
#ifdef ASTDEBUG
	std::cerr << "AST[" << this << "]: push front node=[" << node << "]" << std::endl;
#endif

	m_Children.push_front( node );
}

inline void ASTNode::push_back( ASTNode* node )
{
#ifdef ASTDEBUG
	std::cerr << "AST[" << this << "]: push back node=[" << node << "]" << std::endl;
#endif

	m_Children.push_back( node );
}

inline uint64_t ASTNode::get_int_result()
{
	return m_IntResult;
}

inline std::string ASTNode::get_string_result()
{
	return m_StringResult;
}

inline const ASTNode::nodelist_t& ASTNode::get_children()
{
	return m_Children;
}


#endif // __mempeek_ast_h__
