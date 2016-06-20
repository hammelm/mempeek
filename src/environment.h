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
#include "subroutines.h"
#include "variables.h"
#include "arrays.h"
#include "mmap.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>


//////////////////////////////////////////////////////////////////////////////
// class Environment
//////////////////////////////////////////////////////////////////////////////

class ASTNode;

class Environment {
public:

    typedef VarManager::var var;
    typedef ArrayManager::array array;
    typedef ArrayManager::refarray refarray;

	Environment();
	~Environment();

	std::shared_ptr<ASTNode> parse( const char* str, bool is_file );
    std::shared_ptr<ASTNode> parse( const yylloc_t& location, const char* str, bool is_file );

    void add_include_path( std::string path );

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

    bool map_memory( void* phys_addr, size_t size, std::string device );

	MMap* get_mapping( void* phys_addr, size_t size );

	void enter_subroutine_context( const yylloc_t& location, std::string name, bool is_function );
    void set_subroutine_param( std::string name, bool is_array = false );
    void set_subroutine_body( std::shared_ptr<ASTNode> body );
	void commit_subroutine_context();

	std::shared_ptr<ASTNode> get_procedure( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );
    std::shared_ptr<ASTNode> get_function( const yylloc_t& location, std::string name, std::vector< std::shared_ptr<ASTNode> >& params );

    bool drop_procedure( std::string name );
    bool drop_function( std::string name );

    static int get_default_size();

    static void set_terminate();
    static void clear_terminate();
    static bool is_terminated();

private:
    VarManager* m_GlobalVars;
    ArrayManager* m_GlobalArrays;

	std::map< void*, MMap* > m_Mappings;

	BuiltinManager* m_BuiltinManager;

	SubroutineManager* m_ProcedureManager;
	SubroutineManager* m_FunctionManager;

	SubroutineManager* m_SubroutineContext = nullptr;
    VarManager* m_LocalVars = nullptr;
    ArrayManager* m_LocalArrays = nullptr;

	std::vector< std::string > m_IncludePaths;

    static volatile sig_atomic_t s_IsTerminated;
};


//////////////////////////////////////////////////////////////////////////////
// class Environment inline functions
//////////////////////////////////////////////////////////////////////////////

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

inline void Environment::add_include_path( std::string path )
{
    m_IncludePaths.push_back( path );
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


#endif // __environment_h__
