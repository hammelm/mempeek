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

#ifndef __environment_h__
#define __environment_h__

#include "mempeek_parser.h"
#include "mmap.h"

#include <string>
#include <utility>
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <memory>

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// class Environment
//////////////////////////////////////////////////////////////////////////////

class SubroutineManager;
class LocalEnvironment;
class ASTNode;

class Environment {
public:
	class var;

	Environment();
	~Environment();

	var* alloc_def( std::string name );
	var* alloc_var( std::string name );
    var* alloc_global( std::string name );

	const var* get( std::string name );

	std::set< std::string > get_struct_members( std::string name );

    bool map_memory( void* phys_addr, size_t size, std::string device );

	MMap* get_mapping( void* phys_addr, size_t size );

	void enter_subroutine_context( const yylloc_t& location, std::string name, bool is_function );
    void set_subroutine_param( std::string name );
    void set_subroutine_body( std::shared_ptr<ASTNode> body );
	void commit_subroutine_context();

	std::shared_ptr<ASTNode> get_procedure( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );
    std::shared_ptr<ASTNode> get_function( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );

private:
	class defvar;
    class structvar;
	class globalvar;

	std::map< std::string, var* > m_Vars;
	std::map< void*, MMap* > m_Mappings;

	SubroutineManager* m_ProcedureManager;
	SubroutineManager* m_FunctionManager;

	LocalEnvironment* m_LocalEnv = nullptr;
	SubroutineManager* m_SubroutineContext = nullptr;
};


//////////////////////////////////////////////////////////////////////////////
// class SubroutineManager
//////////////////////////////////////////////////////////////////////////////

class SubroutineManager {
public:
    SubroutineManager( Environment* env );
    ~SubroutineManager();

    LocalEnvironment* begin_subroutine( const yylloc_t& location, std::string name, bool is_function );
    void set_param( std::string name );
    void set_body( std::shared_ptr<ASTNode> body );
    void commit_subroutine();

    std::shared_ptr<ASTNode> get_subroutine( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );

private:
    typedef struct {
        LocalEnvironment* env;
        std::vector< Environment::var* > params;
        std::shared_ptr<ASTNode> body;
        Environment::var* retval = nullptr;
        yylloc_t location;
    } subroutine_t;

    Environment* m_Environment;

    std::map< std::string, subroutine_t* > m_Subroutines;

    std::string m_PendingName;
    subroutine_t* m_PendingSubroutine = nullptr;
};

//////////////////////////////////////////////////////////////////////////////
// class LocalEnvironment
//////////////////////////////////////////////////////////////////////////////

class LocalEnvironment {
public:
    LocalEnvironment();
    ~LocalEnvironment();

    Environment::var* alloc_var( std::string name );

    const Environment::var* get( std::string name );

    void push();
    void pop();

private:
    class localvar;

    std::map< std::string, Environment::var* > m_Vars;

    uint64_t* m_Storage = nullptr;
    std::stack< uint64_t* > m_Stack;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment::var
//////////////////////////////////////////////////////////////////////////////

class  Environment::var {
public:
	virtual ~var();

	virtual bool is_def() const;
    virtual bool is_local() const;

	virtual uint64_t get() const = 0;
	virtual void set( uint64_t value ) = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment::defvar
//////////////////////////////////////////////////////////////////////////////

class Environment::defvar : public Environment::var {
public:
    bool is_def() const override;

    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t m_Value = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment::structvar
//////////////////////////////////////////////////////////////////////////////

class Environment::structvar : public Environment::var {
public:
    structvar( const Environment::var* base );

    bool is_def() const override;

    uint64_t get() const override;
    void set( uint64_t offset ) override;

private:
    uint64_t m_Offset = 0;
    const Environment::var* m_Base;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment::globalvar
//////////////////////////////////////////////////////////////////////////////

class Environment::globalvar : public Environment::var {
public:
    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t m_Value = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class LocalEnvironment::localvar
//////////////////////////////////////////////////////////////////////////////

class LocalEnvironment::localvar : public Environment::var {
public:
    localvar( uint64_t*& storage, size_t offset );

    bool is_local() const override;

    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t*& m_Storage;
    size_t m_Offset;
};


//////////////////////////////////////////////////////////////////////////////
// class LocalEnvironment inline functions
//////////////////////////////////////////////////////////////////////////////

inline void LocalEnvironment::push()
{
    m_Stack.push( m_Storage );
    m_Storage = new uint64_t[ m_Vars.size() ];
}

inline void LocalEnvironment::pop()
{
    if( !m_Stack.empty() ) {
        delete[] m_Storage;
        m_Storage = m_Stack.top();
        m_Stack.pop();
    }
}


#endif // __environment_h__
