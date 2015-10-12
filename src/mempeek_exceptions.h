#ifndef __mempeek_exceptions_h__
#define __mempeek_exceptions_h__

#include "mempeek_ast.h"

#include <sstream>
#include <string>
#include <vector>


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeException
//////////////////////////////////////////////////////////////////////////////

class ASTException : public std::exception {
public:
	~ASTException();

    const char* what() const noexcept override;

    const char* get_location() const;

protected:
    void loc( const ASTNode::location_t& location );
    void msg( const char* msg );

    template< typename... ARGS >
    void msg( const char* msg, ARGS... varargs );

private:
    void create_msg( const char* msg, std::vector< std::string >& args );

    template < typename... ARGS >
    struct argreader {
    	static void push_args( std::vector< std::string >& args, ARGS... varargs );
    };

    const char* m_Location = nullptr;
    const char* m_What = nullptr;
};


//////////////////////////////////////////////////////////////////////////////
// ASTNode exception types
//////////////////////////////////////////////////////////////////////////////

class ASTControlflowException : public ASTException {};
class ASTCompileException : public ASTException {};
class ASTRuntimeException : public ASTException {};


//////////////////////////////////////////////////////////////////////////////
// ASTNode control flow exceptions
//////////////////////////////////////////////////////////////////////////////

class ASTExceptionBreak : public ASTControlflowException {};
class ASTExceptionQuit : public ASTControlflowException {};
class ASTExceptionTerminate : public ASTControlflowException {};


//////////////////////////////////////////////////////////////////////////////
// ASTNode compile exceptions
//////////////////////////////////////////////////////////////////////////////

class ASTExceptionSyntaxError : public ASTCompileException {
public:
	ASTExceptionSyntaxError( const yylloc_t* yylloc )
	{
		if( yylloc ) {
			ASTNode::location_t location;
			location.file = yylloc->file ? yylloc->file : "";
			location.first_line = yylloc->first_line;
			location.last_line = yylloc->last_line;
			loc( location );
		}
		msg( "syntax error" );
	}
};

class ASTExceptionNamingConflict : public ASTCompileException {
public:
	ASTExceptionNamingConflict( const ASTNode::location_t& location, std::string name )
	{
		loc( location );
		msg( "conflicting name \"$0\"", name );
	}
};

class ASTExceptionUndefinedVar : public ASTCompileException {
public:
	ASTExceptionUndefinedVar( const ASTNode::location_t& location, std::string name )
	{
		loc( location );
		msg( "using undefined var \"$0\"", name );
	}
};

class ASTExceptionMappingFailure : public ASTCompileException {
public:
	ASTExceptionMappingFailure( const ASTNode::location_t& location, std::string address,
								std::string size, std::string device )
	{
		loc( location );
		msg( "failed to map address range $0 size $1 of device \"$2\"", address, size, device );
	}
};

class ASTExceptionFileNotFound : public ASTCompileException {
public:
	ASTExceptionFileNotFound( const ASTNode::location_t& location, const char* file )
	{
		loc( location );
		msg( "file \"$0\" not found", file );
	}
};


//////////////////////////////////////////////////////////////////////////////
// ASTNode runtime exceptions
//////////////////////////////////////////////////////////////////////////////

class ASTExceptionDivisionByZero : public ASTRuntimeException {
public:
	ASTExceptionDivisionByZero( const ASTNode::location_t& location )
	{
		loc( location );
		msg( "division by zero" );
	}
};

class ASTExceptionNoMapping : public ASTRuntimeException {
public:
	ASTExceptionNoMapping( const ASTNode::location_t& location, void* address, size_t size )
	{
		loc( location );
		msg( "no mapping found for $0 bit access to address $1", size * 8, address );
	}
};

class ASTExceptionBusError : public ASTRuntimeException {
public:
	ASTExceptionBusError( const ASTNode::location_t& location, void* address, size_t size )
	{
		loc( location );
		msg( "failed $0 bit access to address $1", size * 8, address );
	}

};


//////////////////////////////////////////////////////////////////////////////
// class ASTNodeException template functions
//////////////////////////////////////////////////////////////////////////////

template< typename... ARGS >
inline void ASTException::msg( const char* msg, ARGS... varargs )
{
	std::vector< std::string > args;
	argreader< ARGS... >::push_args( args, varargs... );
	create_msg( msg, args );
}

template< typename T >
struct ASTException::argreader< T > {
	static void push_args( std::vector< std::string >& args, const T& t )
	{
		std::ostringstream ostr;
		ostr << t;
		args.push_back( ostr.str() );
	}
};

template< typename T, typename... ARGS >
struct ASTException::argreader< T, ARGS... > {
	static void push_args( std::vector< std::string >& args, const T& t, ARGS... varargs )
	{
		std::ostringstream ostr;
		ostr << t;
		args.push_back( ostr.str() );

		ASTException::argreader< ARGS... >::push_args( args, varargs... );
	}
};


#endif // __mempeek_exceptions_h__
