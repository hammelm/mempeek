/*  Copyright (c) 2016, Martin Hammel
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

#include "arrays.h"

#include "mempeek_exceptions.h"

#include <algorithm>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager implementation
//////////////////////////////////////////////////////////////////////////////

ArrayManager::ArrayManager()
{}

ArrayManager::~ArrayManager()
{
    for( auto value: m_Arrays ) delete value.second;

    while( !m_Stack.empty() ) pop();
    release_storage();
}

ArrayManager::array* ArrayManager::alloc_global( std::string name )
{
    auto iter = m_Arrays.find( name );

    if( iter == m_Arrays.end() ) {
        ArrayManager::array* array = new ArrayManager::globalarray();
        iter = m_Arrays.insert( make_pair( name, array ) ).first;
    }

    return iter->second;
}

ArrayManager::array* ArrayManager::alloc_delegate( std::string name, ArrayManager::array* array )
{
    if( m_Arrays.find( name ) != m_Arrays.end() ) return nullptr;

    ArrayManager::array* ref = new ArrayManager::delegatearray( array );
    m_Arrays[ name ] = ref;
    return ref;
}

ArrayManager::refarray* ArrayManager::alloc_ref( std::string name )
{
    if( m_Arrays.find( name ) != m_Arrays.end() ) return nullptr;

    ArrayManager::refarray* ref = new ArrayManager::refarray();
    m_Arrays[ name ] = ref;
    return ref;
}

ArrayManager::array* ArrayManager::alloc_local( std::string name )
{
    auto iter = m_Arrays.find( name );

    if( iter == m_Arrays.end() ) {
        ArrayManager::array* array = new ArrayManager::localarray( m_Storage, m_StorageSize++ );
        iter = m_Arrays.insert( make_pair( name, array ) ).first;
    }

    return iter->second;
}

void ArrayManager::get_autocompletion( std::set< std::string >& completions, std::string prefix )
{
    for( auto iter = m_Arrays.lower_bound( prefix ); iter != m_Arrays.end(); iter++ ) {
        if( iter->first.substr( 0, prefix.length() ) != prefix ) break;
        completions.insert( iter->first );
    }
}


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::array implementation
//////////////////////////////////////////////////////////////////////////////

ArrayManager::array::~array()
{
}

bool ArrayManager::array::is_local() const
{
    return false;
}

ArrayManager::array::data_t* ArrayManager::array::get_data_from_sibling( array* array )
{
    return array->get_data();
}

uint64_t* ArrayManager::array::realloc( uint64_t* old_array, uint64_t old_size, uint64_t new_size )
{
    uint64_t* new_array = new(std::nothrow) uint64_t[new_size];
    if( !new_array ) throw ASTExceptionOutOfMemory( new_size );

    if( new_size < old_size ) copy( old_array, old_array + new_size, new_array );
    else {
        copy( old_array, old_array + old_size, new_array );
        fill( new_array + old_size, new_array + new_size, 0 );
    }

    return new_array;
}


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::globalarray implementation
//////////////////////////////////////////////////////////////////////////////

ArrayManager::globalarray::~globalarray()
{
    if( m_Data.array ) delete[] m_Data.array;
}

uint64_t ArrayManager::globalarray::get( uint64_t index ) const
{
    if( index >= m_Data.size ) throw ASTExceptionOutOfBounds( index, m_Data.size );
    return m_Data.array[ index ];
}

void ArrayManager::globalarray::set( uint64_t index, uint64_t value )
{
    if( index >= m_Data.size ) throw ASTExceptionOutOfBounds( index, m_Data.size );
    m_Data.array[ index ] = value;
}

uint64_t ArrayManager::globalarray::get_size() const
{
    return m_Data.size;
}

void ArrayManager::globalarray::resize( uint64_t size )
{
    if( size == 0 ) {
        if( m_Data.array ) delete[] m_Data.array;
        m_Data.array = nullptr;
        m_Data.size = 0;
    }
    else {
        uint64_t* array = realloc( m_Data.array, m_Data.size, size );
        if( m_Data.array ) delete[] m_Data.array;
        m_Data.array = array;
        m_Data.size = size;
    }
}

ArrayManager::array::data_t* ArrayManager::globalarray::get_data()
{
   return &m_Data;
}


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::localarray implementation
//////////////////////////////////////////////////////////////////////////////

ArrayManager::localarray::localarray( ArrayManager::arraydata_t*& storage, size_t offset )
 : m_Storage( storage ),
   m_Offset( offset )
{}

bool ArrayManager::localarray::is_local() const
{
    return true;
}

uint64_t ArrayManager::localarray::get( uint64_t index ) const
{
    ArrayManager::arraydata_t& arraydata = m_Storage[ m_Offset ];

    if( index >= arraydata.size ) throw ASTExceptionOutOfBounds( index, arraydata.size );
    return arraydata.array[ index ];
}

void ArrayManager::localarray::set( uint64_t index, uint64_t value )
{
    ArrayManager::arraydata_t& arraydata = m_Storage[ m_Offset ];

    if( index >= arraydata.size ) throw ASTExceptionOutOfBounds( index, arraydata.size );
    arraydata.array[ index ] = value;
}

uint64_t ArrayManager::localarray::get_size() const
{
    ArrayManager::arraydata_t& arraydata = m_Storage[ m_Offset ];

    return arraydata.size;
}

void ArrayManager::localarray::resize( uint64_t size )
{
    ArrayManager::arraydata_t& arraydata = m_Storage[ m_Offset ];

    if( size == 0 ) {
        if( arraydata.array ) delete[] arraydata.array;
        arraydata.array = nullptr;
        arraydata.size = 0;
    }
    else {
        uint64_t* array = realloc( arraydata.array, arraydata.size, size );
        if( arraydata.array ) delete[] arraydata.array;
        arraydata.array = array;
        arraydata.size = size;
    }
}

ArrayManager::array::data_t* ArrayManager::localarray::get_data()
{
    return m_Storage + m_Offset;
}


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::delegatearray implementation
//////////////////////////////////////////////////////////////////////////////

ArrayManager::delegatearray::delegatearray( ArrayManager::array* array)
 : m_Array( array )
{}

uint64_t ArrayManager::delegatearray::get( uint64_t index ) const
{
    return m_Array->get( index );
}

void ArrayManager::delegatearray::set( uint64_t index, uint64_t value )
{
    m_Array->set( index, value );
}

uint64_t ArrayManager::delegatearray::get_size() const
{
    return m_Array->get_size();
}

void ArrayManager::delegatearray::resize( uint64_t size )
{
    m_Array->resize( size );
}

ArrayManager::array::data_t* ArrayManager::delegatearray::get_data()
{
    return get_data_from_sibling( m_Array );
}


//////////////////////////////////////////////////////////////////////////////
// class ArrayManager::refarray implementation
//////////////////////////////////////////////////////////////////////////////

ArrayManager::refarray::refarray()
 : m_Data( nullptr )
{}

uint64_t ArrayManager::refarray::get( uint64_t index ) const
{
    if( index >= m_Data->size ) throw ASTExceptionOutOfBounds( index, m_Data->size );
    return m_Data->array[ index ];
}

void ArrayManager::refarray::set( uint64_t index, uint64_t value )
{
    if( index >= m_Data->size ) throw ASTExceptionOutOfBounds( index, m_Data->size );
    m_Data->array[ index ] = value;
}

uint64_t ArrayManager::refarray::get_size() const
{
    return m_Data->size;
}

void ArrayManager::refarray::resize( uint64_t size )
{
    if( size == 0 ) {
        if( m_Data->array ) delete[] m_Data->array;
        m_Data->array = nullptr;
        m_Data->size = 0;
    }
    else {
        uint64_t* array = realloc( m_Data->array, m_Data->size, size );
        if( m_Data->array ) delete[] m_Data->array;
        m_Data->array = array;
        m_Data->size = size;
    }
}

ArrayManager::array::data_t* ArrayManager::refarray::get_data()
{
    return m_Data;
}

void ArrayManager::refarray::set_ref( ArrayManager::array* array )
{
    m_Data = get_data_from_sibling( array );
}
