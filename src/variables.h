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

#ifndef __variables_h__
#define __variables_h__

#include <string>
#include <stack>
#include <map>
#include <set>

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// class VarManager
//////////////////////////////////////////////////////////////////////////////

class VarManager {
public:
    class var;

    VarManager();
    ~VarManager();

    VarManager::var* alloc_def( std::string name );
    VarManager::var* alloc_global( std::string name );
    VarManager::var* alloc_delegate( std::string name, VarManager::var* var );
    VarManager::var* alloc_local( std::string name );

    void get_autocompletion( std::set< std::string >& completions, std::string prefix );

    std::set< std::string > get_struct_members( std::string name );

    const VarManager::var* get( std::string name );

    void push();
    void pop();

private:
    class defvar;
    class structvar;
    class globalvar;
    class localvar;
    class delegatevar;

    std::map< std::string, VarManager::var* > m_Vars;

    uint64_t* m_Storage = nullptr;
    size_t m_StorageSize = 0;
    std::stack< uint64_t* > m_Stack;
};


//////////////////////////////////////////////////////////////////////////////
// class VarManager::var
//////////////////////////////////////////////////////////////////////////////

class  VarManager::var {
public:
    virtual ~var();

    virtual bool is_def() const;
    virtual bool is_local() const;

    virtual void set_range( uint64_t range );
    virtual uint64_t get_range() const;

    virtual size_t get_size() const;
    virtual void set_size( size_t );

    virtual uint64_t get() const = 0;
    virtual void set( uint64_t value ) = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class VarManager::defvar
//////////////////////////////////////////////////////////////////////////////

class VarManager::defvar : public VarManager::var {
public:
    bool is_def() const override;

    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t m_Value = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class VarManager::structvar
//////////////////////////////////////////////////////////////////////////////

class VarManager::structvar : public VarManager::var {
public:
    structvar( const VarManager::var* base );

    bool is_def() const override;

    void set_range( uint64_t range ) override;
    uint64_t get_range() const override;

    size_t get_size() const override;
    void set_size( size_t ) override;

    uint64_t get() const override;
    void set( uint64_t offset ) override;

private:
    uint64_t m_Offset = 0;
    uint64_t m_Range = 0;
    size_t m_Size = 0;
    const VarManager::var* m_Base;
};


//////////////////////////////////////////////////////////////////////////////
// class VarManager::globalvar
//////////////////////////////////////////////////////////////////////////////

class VarManager::globalvar : public VarManager::var {
public:
    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    uint64_t m_Value = 0;
};


//////////////////////////////////////////////////////////////////////////////
// class VarManager::localvar
//////////////////////////////////////////////////////////////////////////////

class VarManager::localvar : public VarManager::var {
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
// class VarManager::delegatevar
//////////////////////////////////////////////////////////////////////////////

class VarManager::delegatevar : public VarManager::var {
public:
    delegatevar( VarManager::var* var );

    uint64_t get() const override;
    void set( uint64_t value ) override;

private:
    VarManager::var* m_Var;
};


//////////////////////////////////////////////////////////////////////////////
// class VarManager inline functions
//////////////////////////////////////////////////////////////////////////////

inline const VarManager::var* VarManager::get( std::string name )
{
    auto iter = m_Vars.find( name );
    if( iter == m_Vars.end() ) return nullptr;
    else return iter->second;
}

inline void VarManager::push()
{
    if( m_StorageSize > 0 ) {
        m_Stack.push( m_Storage );
        m_Storage = new uint64_t[ m_StorageSize ];
    }
}

inline void VarManager::pop()
{
    if( !m_Stack.empty() ) {
        delete[] m_Storage;
        m_Storage = m_Stack.top();
        m_Stack.pop();
    }
}


#endif // __variables_h__
