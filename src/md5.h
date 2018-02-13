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

#ifndef __md5_h__
#define __md5_h__

#include <stdint.h>

#include <ostream>

#include <string.h>


//////////////////////////////////////////////////////////////////////////////
// class MD5
//////////////////////////////////////////////////////////////////////////////

class MD5 {
public:
    MD5();
    ~MD5();

    MD5( const MD5& md5 );
    MD5& operator=( const MD5& md5 );

    void reset();

    bool load( const char* file );

    void check( uint8_t value );
    void check( const uint8_t* buffer, size_t size );

    void get_checksum( uint8_t* checksum ) const;
    const uint8_t* get_checksum() const;

    bool operator==( const MD5& md5 ) const;
    bool operator!=( const MD5& md5 ) const;
    bool operator<( const MD5& md5 ) const;
    bool operator>( const MD5& md5 ) const;
    bool operator<=( const MD5& md5 ) const;
    bool operator>=( const MD5& md5 ) const;

    friend std::ostream& operator<<( std::ostream& out, const MD5& md5 );

private:
    void calc_hash();

    uint8_t* m_Buffer;
    size_t m_BufferSize;
    uint64_t m_MessageSize;

    uint32_t* m_Hash;

    mutable bool m_HasChecksum;
    uint8_t* m_Checksum;
};


//////////////////////////////////////////////////////////////////////////////
// class MD5 inline functions
//////////////////////////////////////////////////////////////////////////////

inline MD5::MD5()
{
    m_Buffer = new uint8_t[64];
    m_Hash = new uint32_t[4];
    m_Checksum = new uint8_t[16];

    reset();
}


inline MD5::MD5( const MD5& md5 )
{
    m_Buffer = new uint8_t[64];
    m_Hash = new uint32_t[4];
    m_Checksum = new uint8_t[16];

    *this = md5;
}


inline MD5::~MD5()
{
    delete[] m_Buffer;
    delete[] m_Hash;
    delete[] m_Checksum;
}


inline void MD5::reset()
{
    m_HasChecksum = false;

    m_BufferSize = 0;
    m_MessageSize = 0;

    m_Hash[0] = 0x67452301;
    m_Hash[1] = 0xefcdab89;
    m_Hash[2] = 0x98badcfe;
    m_Hash[3] = 0x10325476;
}


inline void MD5::check( uint8_t value )
{
    m_HasChecksum = false;

    m_Buffer[ m_BufferSize++ ] = value;
    if( m_BufferSize >= 64 ) calc_hash();
    m_MessageSize++;
}


inline void MD5::check( const uint8_t* buffer, size_t size )
{
    m_HasChecksum = false;

    for( size_t i = 0; i < size; i++ ) {
        m_Buffer[ m_BufferSize++ ] = buffer[i];
        if( m_BufferSize >= 64 ) calc_hash();
    }

    m_MessageSize += size;
}


inline void MD5::get_checksum( uint8_t* checksum ) const
{
    memcpy( checksum, get_checksum(), 16 );
}


inline bool MD5::operator==( const MD5& md5 ) const
{
    return memcmp( get_checksum(), md5.get_checksum(), 16 ) == 0;
}

inline bool MD5::operator!=( const MD5& md5 ) const
{
    return memcmp( get_checksum(), md5.get_checksum(), 16 ) != 0;
}

inline bool MD5::operator<( const MD5& md5 ) const
{
    return memcmp( get_checksum(), md5.get_checksum(), 16 ) < 0;
}

inline bool MD5::operator>( const MD5& md5 ) const
{
    return memcmp( get_checksum(), md5.get_checksum(), 16 ) > 0;
}

inline bool MD5::operator<=( const MD5& md5 ) const
{
    return memcmp( get_checksum(), md5.get_checksum(), 16 ) <= 0;
}

inline bool MD5::operator>=( const MD5& md5 ) const
{
    return memcmp( get_checksum(), md5.get_checksum(), 16 ) >= 0;
}


#endif // __md5_h__
