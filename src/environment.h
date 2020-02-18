/*  Copyright (c) 2015-2020, Martin Hammel
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
#include "subroutines.h"
#include "variables.h"
#include "arrays.h"
#include "mmap.h"
#include "md5.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <memory>
#include <ostream>


//////////////////////////////////////////////////////////////////////////////
// class Environment
//////////////////////////////////////////////////////////////////////////////

class ASTNode;

class Environment {
public:

    typedef VarManager::var var;
    typedef ArrayManager::array array;
    typedef ArrayManager::refarray refarray;

    typedef enum { PROCEDURE, FUNCTION, ARRAYFUNC } subroutine_type_t;

	Environment();
	~Environment();

	std::shared_ptr<ASTNode> parse( const char* str, bool is_file, bool run_once );
    std::shared_ptr<ASTNode> parse( const yylloc_t& location, const char* str, bool is_file, bool run_once );

    void set_stdout( std::ostream& out );

    std::ostream& get_stdout();

    bool add_include_path( std::string path );

	var* alloc_var( std::string name );
	var* alloc_def_var( std::string name );
    var* alloc_global_var( std::string name );
    var* alloc_static_var( std::string name );

	array* alloc_array( std::string name );
    array* alloc_global_array( std::string name );
    array* alloc_static_array( std::string name );
    refarray* alloc_ref_array( std::string name );

	const var* get_var( std::string name );
	array* get_array( std::string name );

	std::set< std::string > get_autocompletion( std::string prefix );
	std::set< std::string > get_struct_members( std::string name );

    bool map_memory( void* phys_addr, void* map_addr, size_t size, std::string device );

	MMap* get_mapping( void* phys_addr, size_t size );

	void enter_subroutine_context( const yylloc_t& location, std::string name, subroutine_type_t type );
    void set_subroutine_param( std::string name, bool is_array = false );
    void set_subroutine_body( std::shared_ptr<ASTNode> body );
    void set_subroutine_varargs();
	void commit_subroutine_context();

	std::shared_ptr<ASTNode> get_procedure( const yylloc_t& location, std::string name, const arglist_t& args );
    std::shared_ptr<ASTNode> get_function( const yylloc_t& location, std::string name, const arglist_t& args );
    std::shared_ptr<ASTNode> get_arrayfunc( const yylloc_t& location, std::string name, std::string ret, const arglist_t& args );

    bool drop_procedure( std::string name );
    bool drop_function( std::string name );

    int get_default_size();
    bool set_default_size( int size );
    void push_default_size();
    void pop_default_size();

    int get_default_modifier();
    bool set_default_modifier( int modifier );
    void push_default_modifier();
    void pop_default_modifier();

    size_t get_num_varargs();
    uint64_t get_vararg_value( size_t index );
    array* get_vararg_array( size_t index );
    void append_vararg( uint64_t value );
    void append_vararg( array* array );
    void flush_varargs();
    void push_varargs();
    void pop_varargs();

    void set_terminate();
    void clear_terminate();
    bool is_terminated();

    static uint64_t parse_int( std::string str );
    static uint64_t parse_int( std::string str, bool& is_ok );
    static uint64_t parse_float( std::string str );
    static uint64_t parse_float( std::string str, bool& is_ok );

private:
    VarManager* m_GlobalVars;
    ArrayManager* m_GlobalArrays;

	std::map< void*, MMap* > m_Mappings;

	BuiltinManager* m_BuiltinManager;

	SubroutineManager* m_ProcedureManager;
	SubroutineManager* m_FunctionManager;
	SubroutineManager* m_ArrayfuncManager;

	SubroutineManager* m_SubroutineContext = nullptr;
    VarManager* m_LocalVars = nullptr;
    ArrayManager* m_LocalArrays = nullptr;

	std::vector< std::string > m_IncludePaths;
	std::set< MD5 > m_ImportedFiles;

	int m_DefaultSize;
	std::stack<int> m_DefaultSizeStack;

    int m_DefaultModifier;
    std::stack<int> m_DefaultModifierStack;

    std::stack< std::vector< std::pair< uint64_t, array* > > > m_ArgStack;

    volatile sig_atomic_t m_IsTerminated;

    std::ostream* m_Stdout;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment inline functions
//////////////////////////////////////////////////////////////////////////////

inline void Environment::set_stdout( std::ostream& out )
{
    m_Stdout = &out;
}

inline std::ostream& Environment::get_stdout()
{
    return *m_Stdout;
}

inline Environment::var* Environment::alloc_def_var( std::string name )
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

inline Environment::var* Environment::alloc_global_var( std::string name )
{
    Environment::var* var = m_GlobalVars->alloc_global( name );

    if( var && m_LocalVars ) return m_LocalVars->alloc_delegate( name, var );
    else return var;
}

inline Environment::var* Environment::alloc_static_var( std::string name )
{
    if( m_LocalVars ) return m_LocalVars->alloc_global( name );
    else return m_GlobalVars->alloc_global( name );
}

inline Environment::array* Environment::alloc_array( std::string name )
{
    if( m_LocalArrays ) return m_LocalArrays->alloc_local( name );
    else return m_GlobalArrays->alloc_global( name );
}

inline Environment::array* Environment::alloc_global_array( std::string name )
{
    Environment::array* array = m_GlobalArrays->alloc_global( name );

    if( array && m_LocalArrays ) return m_LocalArrays->alloc_delegate( name, array );
    else return array;
}

inline Environment::array* Environment::alloc_static_array( std::string name )
{
    if( m_LocalArrays ) return m_LocalArrays->alloc_global( name );
    else return m_GlobalArrays->alloc_global( name );
}

inline Environment::refarray* Environment::alloc_ref_array( std::string name )
{
    if( m_LocalArrays ) return m_LocalArrays->alloc_ref( name );
    else return m_GlobalArrays->alloc_ref( name );
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
    if( m_FunctionManager->drop_subroutine( name ) ) return true;
    else return m_ArrayfuncManager->drop_subroutine( name );
}

inline void Environment::set_terminate()
{
    m_IsTerminated = 1;
}

inline void Environment::clear_terminate()
{
    m_IsTerminated = 0;
}

inline bool Environment::is_terminated()
{
    return m_IsTerminated == 1;
}

inline uint64_t Environment::parse_int( std::string str )
{
    bool dummy = false;
    return parse_int( str, dummy );
}

inline uint64_t Environment::parse_float( std::string str )
{
    bool dummy = false;
    return parse_float( str, dummy );
}

inline int Environment::get_default_size()
{
    return m_DefaultSize;
}

inline void Environment::push_default_size()
{
    m_DefaultSizeStack.push( m_DefaultSize );
}

inline void Environment::pop_default_size()
{
    m_DefaultSize = m_DefaultSizeStack.top();
    m_DefaultSizeStack.pop();
}
inline int Environment::get_default_modifier()
{
    return m_DefaultModifier;
}

inline void Environment::push_default_modifier()
{
    m_DefaultModifierStack.push( m_DefaultModifier );
}

inline void Environment::pop_default_modifier()
{
    m_DefaultModifier = m_DefaultModifierStack.top();
    m_DefaultModifierStack.pop();
}

inline size_t Environment::get_num_varargs()
{
    return m_ArgStack.top().size();
}

inline uint64_t Environment::get_vararg_value( size_t index )
{
    return m_ArgStack.top()[index].first;
}

inline Environment::array* Environment::get_vararg_array( size_t index )
{
    return m_ArgStack.top()[index].second;
}

inline void Environment::append_vararg( uint64_t value )
{
    m_ArgStack.top().push_back( std::make_pair( value, (array*)nullptr ) );
}

inline void Environment::append_vararg( array* array )
{
    m_ArgStack.top().push_back( std::make_pair( (uint64_t)0, array ) );
}

inline void Environment::flush_varargs()
{
    m_ArgStack.top().clear();
}

inline void Environment::push_varargs()
{
    m_ArgStack.emplace();
}

inline void Environment::pop_varargs()
{
    m_ArgStack.pop();
}


#endif // __environment_h__
