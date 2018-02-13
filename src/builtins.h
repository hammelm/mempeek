/*  Copyright (c) 2016-2018, Martin Hammel
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

#ifndef __builtins_h__
#define __builtins_h__

#include "mempeek_parser.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <utility>
#include <memory>

class ASTNode;


//////////////////////////////////////////////////////////////////////////////
// class BuiltinManager
//////////////////////////////////////////////////////////////////////////////

class BuiltinManager {
public:
    BuiltinManager( Environment* env );

    void get_autocompletion( std::set< std::string >& completions, std::string prefix );

    bool has_subroutine( std::string name );
    std::shared_ptr<ASTNode> get_subroutine( const yylloc_t& location, std::string name, const arglist_t& args );

    typedef std::function< std::shared_ptr<ASTNode>( const yylloc_t& location, Environment* env, const arglist_t& args ) > nodecreator_t;

    void register_function( std::string name, nodecreator_t creator );

private:
    typedef std::map< std::string, nodecreator_t > builtinmap_t;

    Environment* m_Env;

    // TODO: make m_Builtins static
    builtinmap_t m_Builtins;
};


//////////////////////////////////////////////////////////////////////////////
// class BuiltinManager inline functions
//////////////////////////////////////////////////////////////////////////////

inline bool BuiltinManager::has_subroutine( std::string name )
{
    auto iter = m_Builtins.find( name );
    return iter != m_Builtins.end();
}


#endif // __builtins_h__
