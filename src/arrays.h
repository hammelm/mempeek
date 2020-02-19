/*  Copyright (c) 2016-2020, Martin Hammel
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

#ifndef __arrays_h__
#define __arrays_h__

#include <string>
#include <stack>
#include <map>
#include <set>
#include <algorithm>

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager
//////////////////////////////////////////////////////////////////////////////

class ArrayManager {
public:
    class array;
    class refarray;

    ArrayManager();
    ~ArrayManager();

    ArrayManager::array* alloc_global( std::string name );
    ArrayManager::array* alloc_delegate( std::string name, ArrayManager::array* array );
    ArrayManager::refarray* alloc_ref( std::string name );
    ArrayManager::array* alloc_local( std::string name );

    void get_autocompletion( std::set< std::string >& completions, std::string prefix );

    ArrayManager::array* get( std::string name );

    void push();
    void pop();

private:
    class globalarray;
    class localarray;
    class delegatearray;

    friend class ArrayManager::array;

    void release_storage();

    std::map< std::string, ArrayManager::array* > m_Arrays;

    typedef struct {
        uint64_t size = 0;
        uint64_t* array = nullptr;
    } arraydata_t;

    arraydata_t* m_Storage = nullptr;
    size_t m_StorageSize = 0;
    std::stack< arraydata_t* > m_Stack;
};


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::array
//////////////////////////////////////////////////////////////////////////////

class  ArrayManager::array {
public:
    virtual ~array();

    virtual bool is_local() const;

    virtual uint64_t get( uint64_t index ) const = 0;
    virtual void set( uint64_t index, uint64_t value ) = 0;

    virtual uint64_t get_size() const = 0;

    virtual void resize( uint64_t size ) = 0;

protected:
    typedef ArrayManager::arraydata_t data_t;

    virtual data_t* get_data() = 0;

    static data_t* get_data_from_sibling( array* array );

    static uint64_t* realloc( uint64_t* old_array, uint64_t old_size, uint64_t new_size );
};


/////////////////////////////////////////////////////////////////////////////
// class ArrayManager::globalarray
//////////////////////////////////////////////////////////////////////////////

class ArrayManager::globalarray final : public ArrayManager::array {
public:
    ~globalarray();

    uint64_t get( uint64_t index ) const override;
    void set( uint64_t index, uint64_t value ) override;

    virtual uint64_t get_size() const override;

    virtual void resize( uint64_t size ) override;

protected:
    virtual data_t* get_data() override;

private:
    data_t m_Data;
};


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::localarray
//////////////////////////////////////////////////////////////////////////////

class ArrayManager::localarray final : public ArrayManager::array {
public:
    localarray( ArrayManager::arraydata_t*& storage, size_t offset );

    bool is_local() const override;

    uint64_t get( uint64_t index ) const override;
    void set( uint64_t index, uint64_t value ) override;

    virtual uint64_t get_size() const override;

    virtual void resize( uint64_t size ) override;

protected:
    virtual data_t* get_data() override;

private:
    data_t*& m_Storage;
    size_t m_Offset;
};


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::delegatearray
//////////////////////////////////////////////////////////////////////////////

class ArrayManager::delegatearray final : public ArrayManager::array {
public:
    delegatearray( ArrayManager::array* array );

    uint64_t get( uint64_t index ) const override;
    void set( uint64_t index, uint64_t value ) override;

    virtual uint64_t get_size() const override;

    virtual void resize( uint64_t size ) override;

protected:
    virtual data_t* get_data() override;

private:
    ArrayManager::array* m_Array;
};


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::refarray
//////////////////////////////////////////////////////////////////////////////

class ArrayManager::refarray final : public ArrayManager::array {
public:
    refarray();

    uint64_t get( uint64_t index ) const override;
    void set( uint64_t index, uint64_t value ) override;

    virtual uint64_t get_size() const override;

    virtual void resize( uint64_t size ) override;

    void set_ref( ArrayManager::array* array );
    void push_ref( ArrayManager::array* array );
    void pop_ref();

protected:
    virtual data_t* get_data() override;

private:
    data_t* m_Data;

    std::stack< data_t* > m_Stack;
};


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager inline functions
//////////////////////////////////////////////////////////////////////////////

inline ArrayManager::array* ArrayManager::get( std::string name )
{
    auto iter = m_Arrays.find( name );
    if( iter == m_Arrays.end() ) return nullptr;
    else return iter->second;
}

inline void ArrayManager::push()
{
    if( m_StorageSize > 0 ) {
        m_Stack.push( m_Storage );
        m_Storage = new arraydata_t[ m_StorageSize ];
    }
}

inline void ArrayManager::pop()
{
    if( !m_Stack.empty() ) {
        release_storage();
        m_Storage = m_Stack.top();
        m_Stack.pop();
    }
}

inline void ArrayManager::release_storage()
{
    if( m_Storage ) {
        std::for_each( m_Storage, m_Storage + m_StorageSize, [] ( arraydata_t& arraydata ) {
            if( arraydata.array ) delete[] arraydata.array;
        });
        delete[] m_Storage;
        m_Storage = nullptr;
    }
}


#endif // __arrays_h__
