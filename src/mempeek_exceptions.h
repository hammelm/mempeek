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

#ifndef __mempeek_exceptions_h__
#define __mempeek_exceptions_h__

#include "mempeek_ast.h"

#include <sstream>
#include <iomanip>
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
    void loc( const yylloc_t& location );
    void msg( const char* msg );
    void clone( const ASTException& ex );

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

class ASTExceptionExit : public ASTControlflowException {};
class ASTExceptionBreak : public ASTControlflowException {};
class ASTExceptionQuit : public ASTControlflowException {};
class ASTExceptionTerminate : public ASTControlflowException {};


//////////////////////////////////////////////////////////////////////////////
// ASTNode compile exceptions
//////////////////////////////////////////////////////////////////////////////

class ASTExceptionSyntaxError : public ASTCompileException {
public:
	ASTExceptionSyntaxError( const yylloc_t& location )
	{
		loc( location );
		msg( "syntax error" );
	}
};

class ASTExceptionNamingConflict : public ASTCompileException {
public:
	ASTExceptionNamingConflict( const yylloc_t& location, std::string name )
	{
		loc( location );
		msg( "conflicting name \"$0\"", name );
	}
};

class ASTExceptionUndefinedVar : public ASTCompileException {
public:
	ASTExceptionUndefinedVar( const yylloc_t& location, std::string name )
	{
		loc( location );
		msg( "using undefined var \"$0\"", name );
	}
};

class ASTExceptionNoReturnValue : public ASTCompileException {
public:
    ASTExceptionNoReturnValue( const yylloc_t& location )
    {
        loc( location );
        msg( "no return value" );
    }
};

class ASTExceptionMappingFailure : public ASTCompileException {
public:
	ASTExceptionMappingFailure( const yylloc_t& location, uint64_t address,
								uint64_t size, std::string device )
	{
		loc( location );
		std::ostringstream a, s;
		a << std::hex << std::setw( sizeof(void*) * 2 ) << std::setfill( '0' ) << address;
	    s << std::hex << std::setw( sizeof(void*) * 2 ) << std::setfill( '0' ) << size;
		msg( "failed to map address range $0 size $1 of device \"$2\"", a.str(), s.str(), device );
	}
};

class ASTExceptionFileNotFound : public ASTCompileException {
public:
	ASTExceptionFileNotFound( const yylloc_t& location, const char* file )
	{
		loc( location );
		msg( "file \"$0\" not found", file );
	}
};

class ASTExceptionNonconstExpression : public ASTCompileException {
public:
    ASTExceptionNonconstExpression( const yylloc_t& location )
    {
        loc( location );
        msg( "illegal usage of non-const expression" );
    }
};

class ASTExceptionConstDivisionByZero : public ASTCompileException {
public:
    ASTExceptionConstDivisionByZero( const ASTException& ex )
    {
        clone( ex );
    }
};


//////////////////////////////////////////////////////////////////////////////
// ASTNode runtime exceptions
//////////////////////////////////////////////////////////////////////////////

class ASTExceptionDivisionByZero : public ASTRuntimeException {
public:
	ASTExceptionDivisionByZero( const yylloc_t& location )
	{
		loc( location );
		msg( "division by zero" );
	}
};

class ASTExceptionNoMapping : public ASTRuntimeException {
public:
	ASTExceptionNoMapping( const yylloc_t& location, void* address, size_t size )
	{
		loc( location );
		msg( "no mapping found for $0 bit access to address $1", size * 8, address );
	}
};

class ASTExceptionBusError : public ASTRuntimeException {
public:
	ASTExceptionBusError( const yylloc_t& location, void* address, size_t size )
	{
		loc( location );
		msg( "failed $0 bit access to address $1", size * 8, address );
	}

};

class ASTExceptionOutOfBounds : public ASTRuntimeException {
public:
    ASTExceptionOutOfBounds( const yylloc_t& location, uint64_t index, uint64_t size )
     : ASTExceptionOutOfBounds( index, size )
    {
        loc( location );
    }

    ASTExceptionOutOfBounds( uint64_t index, uint64_t size )
    {
        msg( "index $0 does not match size $1", index, size );
    }

    ASTExceptionOutOfBounds( const yylloc_t& location, const ASTExceptionOutOfBounds& ex )
    {
        clone( ex );
        loc( location );
    }
};

class ASTExceptionOutOfMemory : public ASTRuntimeException {
public:
    ASTExceptionOutOfMemory( uint64_t size )
    {
        msg( "failed to allocate array of size $0", size );
    }

    ASTExceptionOutOfMemory( const yylloc_t& location, const ASTExceptionOutOfMemory& ex )
    {
        clone( ex );
        loc( location );
    }
};

class ASTExceptionDroppedSubroutine : public ASTRuntimeException {
public:
    ASTExceptionDroppedSubroutine( const yylloc_t& location )
    {
        loc( location );
        msg( "calling dropped subroutine" );
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
