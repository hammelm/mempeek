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

#ifndef __mempeek_ast_h__
#define __mempeek_ast_h__

#include "mempeek_parser.h"
#include "environment.h"

#include <ostream>
#include <string>
#include <vector>
#include <memory>

#include <stdint.h>

#ifdef ASTDEBUG
#include <iostream>
#endif


//////////////////////////////////////////////////////////////////////////////
// class ASTNode
//////////////////////////////////////////////////////////////////////////////

class ASTNode {
public:
    typedef std::shared_ptr<ASTNode> ptr;

	typedef struct {
		std::string file;
		int first_line;
		int last_line;
	} location_t;

	ASTNode( const yylloc_t* yylloc );
	virtual ~ASTNode();

	const ASTNode::location_t& get_location();

	void add_child( ASTNode::ptr node );

	virtual uint64_t execute() = 0;

	static ASTNode::ptr parse( const char* str, bool is_file );
	static ASTNode::ptr parse( const location_t& location, const char* str, bool is_file );

	static int get_default_size();

	static Environment& get_environment();

	static void add_include_path( std::string path );

	static void set_terminate();
    static void clear_terminate();
	static bool is_terminated();

protected:
	typedef std::vector< ASTNode::ptr > nodelist_t;

	const nodelist_t& get_children();

	static uint64_t parse_int( std::string str );

private:
	nodelist_t m_Children;

	location_t m_Location;

	static Environment s_Environment;
	static volatile bool s_IsTerminated;

	static std::vector< std::string > s_IncludePaths;

	ASTNode( const ASTNode& ) = delete;
	ASTNode& operator=( const ASTNode& ) = delete;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBreak
//////////////////////////////////////////////////////////////////////////////

class ASTNodeBreak : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeBreak> ptr;

    ASTNodeBreak( const yylloc_t* yylloc, int token );

    uint64_t execute() override;

private:
    int m_Token;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBlock
//////////////////////////////////////////////////////////////////////////////

class ASTNodeBlock : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeBlock> ptr;

	ASTNodeBlock( const yylloc_t* yylloc );

	uint64_t execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeAssign
//////////////////////////////////////////////////////////////////////////////

class ASTNodeAssign : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeAssign> ptr;

    ASTNodeAssign( const yylloc_t* yylloc, std::string name, ASTNode::ptr expression );

    uint64_t execute() override;

    Environment::var* get_var();

private:
    Environment::var* m_Var;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeIf
//////////////////////////////////////////////////////////////////////////////

class ASTNodeIf : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeIf> ptr;

	ASTNodeIf( const yylloc_t* yylloc, ASTNode::ptr condition, ASTNode::ptr then_block );
	ASTNodeIf( const yylloc_t* yylloc, ASTNode::ptr condition, ASTNode::ptr then_block, ASTNode::ptr else_block  );

	uint64_t execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeWhile
//////////////////////////////////////////////////////////////////////////////

class ASTNodeWhile : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeWhile> ptr;

	ASTNodeWhile( const yylloc_t* yylloc, ASTNode::ptr condition, ASTNode::ptr block );

	uint64_t execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeFor
//////////////////////////////////////////////////////////////////////////////

class ASTNodeFor : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeFor> ptr;

    ASTNodeFor( const yylloc_t* yylloc, ASTNodeAssign::ptr var, ASTNode::ptr to );
    ASTNodeFor( const yylloc_t* yylloc, ASTNodeAssign::ptr var, ASTNode::ptr to, ASTNode::ptr step );

    uint64_t execute() override;

private:
    Environment::var* m_Var;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePeek
//////////////////////////////////////////////////////////////////////////////

class ASTNodePeek : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodePeek> ptr;

	ASTNodePeek( const yylloc_t* yylloc, ASTNode::ptr address, int size_restriction );

	uint64_t execute() override;

private:
	template< typename T> uint64_t peek();

	int m_SizeRestriction;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePoke
//////////////////////////////////////////////////////////////////////////////

class ASTNodePoke : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodePoke> ptr;

	ASTNodePoke( const yylloc_t* yylloc, ASTNode::ptr address, ASTNode::ptr value, int size_restriction );
	ASTNodePoke( const yylloc_t* yylloc, ASTNode::ptr address, ASTNode::ptr value, ASTNode::ptr mask, int size_restriction );

	uint64_t execute() override;

private:
	template< typename T> void poke();

	int m_SizeRestriction;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodePrint
//////////////////////////////////////////////////////////////////////////////

class ASTNodePrint : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodePrint> ptr;

	enum {
		MOD_DEC = 0x01,
		MOD_HEX = 0x02,
		MOD_BIN = 0x03,
		MOD_NEG = 0x04,

		MOD_8BIT = 0x10,
		MOD_16BIT = 0x20,
		MOD_32BIT = 0x30,
		MOD_64BIT = 0x40,

		MOD_SIZEMASK = 0xf0,
		MOD_TYPEMASK = 0x0f
	};

	ASTNodePrint( const yylloc_t* yylloc );
	ASTNodePrint( const yylloc_t* yylloc, std::string text );
	ASTNodePrint( const yylloc_t* yylloc, ASTNode::ptr expression, int modifier );

	uint64_t execute() override;

	static int get_default_size();

private:
	void print_value( std::ostream& out, uint64_t value );

	int m_Modifier = MOD_DEC | MOD_32BIT;
	std::string m_Text = "";
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeSleep
//////////////////////////////////////////////////////////////////////////////

class ASTNodeSleep : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeSleep> ptr;

    ASTNodeSleep( const yylloc_t* yylloc, ASTNode::ptr expression );

    uint64_t execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeDef
//////////////////////////////////////////////////////////////////////////////

class ASTNodeDef : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeDef> ptr;

	ASTNodeDef( const yylloc_t* yylloc, std::string name, ASTNode::ptr address );
	ASTNodeDef( const yylloc_t* yylloc, std::string name, ASTNode::ptr address, std::string from );

	uint64_t execute() override;

private:
	Environment::var* m_Def;

	const Environment::var* m_FromBase;
	std::vector< std::pair< Environment::var*, const Environment::var* > > m_FromMembers;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeMap
//////////////////////////////////////////////////////////////////////////////

class ASTNodeMap : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeMap> ptr;

	ASTNodeMap( const yylloc_t* yylloc, std::string address, std::string size );
    ASTNodeMap( const yylloc_t* yylloc, std::string address, std::string size, std::string device );

	uint64_t execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeImport
//////////////////////////////////////////////////////////////////////////////

class ASTNodeImport : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeImport> ptr;

	ASTNodeImport( const yylloc_t* yylloc, std::string file );

	uint64_t execute() override;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeUnaryOperator
//////////////////////////////////////////////////////////////////////////////

class ASTNodeUnaryOperator : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeUnaryOperator> ptr;

	ASTNodeUnaryOperator( const yylloc_t* yylloc, ASTNode::ptr expression, int op );

	uint64_t execute() override;

private:
	int m_Operator;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeBinaryOperator
//////////////////////////////////////////////////////////////////////////////

class ASTNodeBinaryOperator : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeBinaryOperator> ptr;

	ASTNodeBinaryOperator( const yylloc_t* yylloc, ASTNode::ptr expression1, ASTNode::ptr expression2, int op );

	uint64_t execute() override;

private:
	int m_Operator;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeRestriction
//////////////////////////////////////////////////////////////////////////////

class ASTNodeRestriction : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeRestriction> ptr;

	ASTNodeRestriction( const yylloc_t* yylloc, ASTNode::ptr node, int size_restriction );

	uint64_t execute() override;

private:
	int m_SizeRestriction;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeVar
//////////////////////////////////////////////////////////////////////////////

class ASTNodeVar : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeVar> ptr;

	ASTNodeVar( const yylloc_t* yylloc, std::string name );
	ASTNodeVar( const yylloc_t* yylloc, std::string name, ASTNode::ptr index );

	uint64_t execute() override;

private:
	const Environment::var* m_Var;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeConstant
//////////////////////////////////////////////////////////////////////////////

class ASTNodeConstant : public ASTNode {
public:
    typedef std::shared_ptr<ASTNodeConstant> ptr;

	ASTNodeConstant( const yylloc_t* yylloc, std::string str );

	uint64_t execute() override;

private:
	uint64_t m_Value;
};


//////////////////////////////////////////////////////////////////////////////
// class ASTNode inline functions
//////////////////////////////////////////////////////////////////////////////

inline const ASTNode::location_t& ASTNode::get_location()
{
	return m_Location;
}

inline void ASTNode::add_child( ASTNode::ptr node )
{
#ifdef ASTDEBUG
	std::cerr << "AST[" << this << "]: add child node=[" << node << "]" << std::endl;
#endif

	m_Children.push_back( node );
}

inline const ASTNode::nodelist_t& ASTNode::get_children()
{
	return m_Children;
}

inline Environment& ASTNode::get_environment()
{
	return s_Environment;
}

inline void ASTNode::add_include_path( std::string path )
{
    s_IncludePaths.push_back( path );
}

inline void ASTNode::set_terminate()
{
    s_IsTerminated = true;
}

inline void ASTNode::clear_terminate()
{
    s_IsTerminated = false;
}

inline bool ASTNode::is_terminated()
{
    return s_IsTerminated;
}


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeAssign inline functions
//////////////////////////////////////////////////////////////////////////////

inline Environment::var* ASTNodeAssign::get_var()
{
    return m_Var;
}


#endif // __mempeek_ast_h__
