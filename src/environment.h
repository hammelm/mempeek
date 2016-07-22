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
#include "builtins.h"
#include "mmap.h"
#include "md5.h"

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
class VarStorage;
class ASTNode;

class Environment {
public:
	class var;

	Environment();
	~Environment();

	std::shared_ptr<ASTNode> parse( const char* str, bool is_file, bool run_once );
    std::shared_ptr<ASTNode> parse( const yylloc_t& location, const char* str, bool is_file, bool run_once );

    bool add_include_path( std::string path );

	var* alloc_def( std::string name );
	var* alloc_var( std::string name );
    var* alloc_global( std::string name );
    var* alloc_static( std::string name );

	const var* get( std::string name );

	std::set< std::string > get_autocompletion( std::string prefix );
	std::set< std::string > get_struct_members( std::string name );

    bool map_memory( void* phys_addr, size_t size, std::string device );

	MMap* get_mapping( void* phys_addr, size_t size );

	void enter_subroutine_context( const yylloc_t& location, std::string name, bool is_function );
    void set_subroutine_param( std::string name );
    void set_subroutine_body( std::shared_ptr<ASTNode> body );
	void commit_subroutine_context();

	std::shared_ptr<ASTNode> get_procedure( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );
    std::shared_ptr<ASTNode> get_function( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );

    bool drop_procedure( std::string name );
    bool drop_function( std::string name );

    static int get_default_size();
    static bool set_default_size( int size );
    static void push_default_size();
    static void pop_default_size();

    static int get_default_modifier();
    static bool set_default_modifier( int modifier );
    static void push_default_modifier();
    static void pop_default_modifier();

    static void set_terminate();
    static void clear_terminate();
    static bool is_terminated();

    static uint64_t parse_int( std::string str );
    static uint64_t parse_float( std::string str );

private:
    VarStorage* m_GlobalVars;

	std::map< void*, MMap* > m_Mappings;

	BuiltinManager* m_BuiltinManager;

	SubroutineManager* m_ProcedureManager;
	SubroutineManager* m_FunctionManager;

	SubroutineManager* m_SubroutineContext = nullptr;
    VarStorage* m_LocalVars = nullptr;

	std::vector< std::string > m_IncludePaths;
	std::set< MD5 > m_ImportedFiles;

	static int s_DefaultSize;
	static std::stack<int> s_DefaultSizeStack;

    static int s_DefaultModifier;
    static std::stack<int> s_DefaultModifierStack;

    static volatile sig_atomic_t s_IsTerminated;
};


//////////////////////////////////////////////////////////////////////////////
// class SubroutineManager
//////////////////////////////////////////////////////////////////////////////

class SubroutineManager {
public:
    SubroutineManager( Environment* env );
    ~SubroutineManager();

    VarStorage* begin_subroutine( const yylloc_t& location, std::string name, bool is_function );
    void set_param( std::string name );
    void set_body( std::shared_ptr<ASTNode> body );
    void commit_subroutine();
    void abort_subroutine();

    bool drop_subroutine( std::string name );

    void get_autocompletion( std::set< std::string >& completions, std::string prefix );

    bool has_subroutine( std::string name );
    std::shared_ptr<ASTNode> get_subroutine( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );

private:
    typedef struct {
        VarStorage* vars;
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
// class VarStorage
//////////////////////////////////////////////////////////////////////////////

class VarStorage {
public:
    VarStorage();
    ~VarStorage();

    Environment::var* alloc_def( std::string name );
    Environment::var* alloc_global( std::string name );
    Environment::var* alloc_ref( std::string name, Environment::var* var );
    Environment::var* alloc_local( std::string name );

    void get_autocompletion( std::set< std::string >& completions, std::string prefix );

    std::set< std::string > get_struct_members( std::string name );

    const Environment::var* get( std::string name );

    void push();
    void pop();

private:
    class defvar;
    class structvar;
    class globalvar;
    class localvar;
    class refvar;

    std::map< std::string, Environment::var* > m_Vars;

    uint64_t* m_Storage = nullptr;
    size_t m_StorageSize = 0;
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
// class VarStorage::defvar
//////////////////////////////////////////////////////////////////////////////

class VarStorage::defvar : public Environment::var {
public:
    bool is_def() const override;

    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t m_Value = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::structvar
//////////////////////////////////////////////////////////////////////////////

class VarStorage::structvar : public Environment::var {
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
// class VarStorage::globalvar
//////////////////////////////////////////////////////////////////////////////

class VarStorage::globalvar : public Environment::var {
public:
    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t m_Value = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class VarStorage::localvar
//////////////////////////////////////////////////////////////////////////////

class VarStorage::localvar : public Environment::var {
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
// class VarStorage::refvar
//////////////////////////////////////////////////////////////////////////////

class VarStorage::refvar : public Environment::var {
public:
    refvar( Environment::var* var );

    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    Environment::var* m_Var;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment inline functions
//////////////////////////////////////////////////////////////////////////////

inline Environment::var* Environment::alloc_def( std::string name )
{
    return m_GlobalVars->alloc_def( name );
}

inline Environment::var* Environment::alloc_var( std::string name )
{
    if( m_LocalVars ) {
        const Environment::var* var = m_GlobalVars->get( name );
        if( var && var->is_def() ) return nullptr;
        return m_LocalVars->alloc_local( name );
    }
    else return m_GlobalVars->alloc_global( name );
}

inline Environment::var* Environment::alloc_global( std::string name )
{
    Environment::var* var = m_GlobalVars->alloc_global( name );

    if( var && m_LocalVars ) return m_LocalVars->alloc_ref( name, var );
    else return var;
}

inline Environment::var* Environment::alloc_static( std::string name )
{
    if( m_LocalVars ) return m_LocalVars->alloc_global( name );
    else return m_GlobalVars->alloc_global( name );
}

inline std::set< std::string > Environment::get_struct_members( std::string name )
{
    return m_GlobalVars->get_struct_members( name );
}

inline bool Environment::drop_procedure( std::string name )
{
    return m_ProcedureManager->drop_subroutine( name );
}

inline bool Environment::drop_function( std::string name )
{
    return m_FunctionManager->drop_subroutine( name );
}

inline void Environment::set_terminate()
{
    s_IsTerminated = 1;
}

inline void Environment::clear_terminate()
{
    s_IsTerminated = 0;
}

inline bool Environment::is_terminated()
{
    return s_IsTerminated == 1;
}

inline int Environment::get_default_size()
{
    return s_DefaultSize;
}

inline void Environment::push_default_size()
{
    s_DefaultSizeStack.push( s_DefaultSize );
}

inline void Environment::pop_default_size()
{
    s_DefaultSize = s_DefaultSizeStack.top();
    s_DefaultSizeStack.pop();
}
inline int Environment::get_default_modifier()
{
    return s_DefaultModifier;
}

inline void Environment::push_default_modifier()
{
    s_DefaultModifierStack.push( s_DefaultModifier );
}

inline void Environment::pop_default_modifier()
{
    s_DefaultModifier = s_DefaultModifierStack.top();
    s_DefaultModifierStack.pop();
}


//////////////////////////////////////////////////////////////////////////////
// class SubroutineManager inline functions
//////////////////////////////////////////////////////////////////////////////

inline bool SubroutineManager::has_subroutine( std::string name )
{
    auto iter = m_Subroutines.find( name );
    return iter != m_Subroutines.end();
}


//////////////////////////////////////////////////////////////////////////////
// class VarStorage inline functions
//////////////////////////////////////////////////////////////////////////////

inline const Environment::var* VarStorage::get( std::string name )
{
    auto iter = m_Vars.find( name );
    if( iter == m_Vars.end() ) return nullptr;
    else return iter->second;
}

inline void VarStorage::push()
{
    if( m_StorageSize > 0 )
        m_Stack.push( m_Storage );
        m_Storage = new uint64_t[ m_StorageSize ];
}

inline void VarStorage::pop()
{
    if( !m_Stack.empty() ) {
        delete[] m_Storage;
        m_Storage = m_Stack.top();
        m_Stack.pop();
    }
}


#endif // __environment_h__
