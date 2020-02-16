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

#include "md5.h"

#include <fstream>
#include <iomanip>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class MD5 implementation
//////////////////////////////////////////////////////////////////////////////

#define F1( x, y, z ) (z ^ (x & (y ^ z)))
#define F2( x, y, z ) F1(z, x, y)
#define F3( x, y, z ) (x ^ y ^ z)
#define F4( x, y, z ) (y ^ (x | ~z))

#define MD5STEP( f, w, x, y, z, data, s ) ( w += f(x, y, z) + data,  w = w << s | w >> (32 - s), w += x )


bool MD5::load( const char* file )
{
    reset();

    ifstream in( file, ios::binary );

    for(;;) {
        uint8_t c = (uint8_t)in.get();
        if( in.eof() ) return true;
        if( in.fail() ) {
            reset();
            return false;
        }
        check( c );
    }
}


MD5& MD5::operator=( const MD5& md5 )
{
    if( this == &md5 ) return *this;

    m_BufferSize = md5.m_BufferSize;
    m_MessageSize = md5.m_MessageSize;
    if( m_BufferSize ) memcpy( m_Buffer, md5.m_Buffer, m_BufferSize );

    m_HasChecksum = md5.m_HasChecksum;
    memcpy( m_Hash, md5.m_Hash, 16 );
    if( m_HasChecksum ) memcpy( m_Checksum, md5.m_Checksum, 16 );

    return *this;
}


const uint8_t* MD5::get_checksum() const
{
    if( m_HasChecksum ) return m_Checksum;

    MD5 md5 = *this;
    md5.check( 0x80 );
    while( md5.m_BufferSize != 56 ) md5.check( 0x00 );

    uint64_t len = m_MessageSize << 3;

#ifdef BIG_ENDIAN

    uint8_t* l = (uint8_t*)&len + 7;
    for( int k = 0; k < 8; k++ ) md5.check( *l-- );

    uint8_t* checksum = m_Checksum;
    for( int i = 0; i < 4; i++ ) {
        uint8_t* hash = (uint8_t*)(md5.m_Hash + i) + 3;
        for( int j = 0; j < 4; j++ ) *checksum++ = *hash--;
    }

#else

    md5.check( (uint8_t*)&len, 8 );
    memcpy( m_Checksum, md5.m_Hash, 16 );

#endif

    m_HasChecksum = true;
    return m_Checksum;
}


void MD5::calc_hash()
{
    uint32_t* in = (uint32_t*)m_Buffer;

#ifdef BIG_ENDIAN
    for( int i = 0; i < 64; ) {
        uint8_t* b0 = (uint8_t*)in + i++;
        uint8_t* b1 = (uint8_t*)in + i++;
        uint8_t* b2 = (uint8_t*)in + i++;
        uint8_t* b3 = (uint8_t*)in + i++;

        uint8_t d0 = *b0; *b0 = *b3; *b3 = d0;
        uint8_t d1 = *b1; *b1 = *b2; *b2 = d1;
    }
#endif

    uint32_t a = m_Hash[0];
    uint32_t b = m_Hash[1];
    uint32_t c = m_Hash[2];
    uint32_t d = m_Hash[3];

    MD5STEP( F1, a, b, c, d, in[0] + 0xd76aa478, 7 );
    MD5STEP( F1, d, a, b, c, in[1] + 0xe8c7b756, 12 );
    MD5STEP( F1, c, d, a, b, in[2] + 0x242070db, 17 );
    MD5STEP( F1, b, c, d, a, in[3] + 0xc1bdceee, 22 );
    MD5STEP( F1, a, b, c, d, in[4] + 0xf57c0faf, 7 );
    MD5STEP( F1, d, a, b, c, in[5] + 0x4787c62a, 12 );
    MD5STEP( F1, c, d, a, b, in[6] + 0xa8304613, 17 );
    MD5STEP( F1, b, c, d, a, in[7] + 0xfd469501, 22 );
    MD5STEP( F1, a, b, c, d, in[8] + 0x698098d8, 7 );
    MD5STEP( F1, d, a, b, c, in[9] + 0x8b44f7af, 12 );
    MD5STEP( F1, c, d, a, b, in[10] + 0xffff5bb1, 17 );
    MD5STEP( F1, b, c, d, a, in[11] + 0x895cd7be, 22 );
    MD5STEP( F1, a, b, c, d, in[12] + 0x6b901122, 7 );
    MD5STEP( F1, d, a, b, c, in[13] + 0xfd987193, 12 );
    MD5STEP( F1, c, d, a, b, in[14] + 0xa679438e, 17 );
    MD5STEP( F1, b, c, d, a, in[15] + 0x49b40821, 22 );

    MD5STEP( F2, a, b, c, d, in[1] + 0xf61e2562, 5 );
    MD5STEP( F2, d, a, b, c, in[6] + 0xc040b340, 9 );
    MD5STEP( F2, c, d, a, b, in[11] + 0x265e5a51, 14 );
    MD5STEP( F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20 );
    MD5STEP( F2, a, b, c, d, in[5] + 0xd62f105d, 5 );
    MD5STEP( F2, d, a, b, c, in[10] + 0x02441453, 9 );
    MD5STEP( F2, c, d, a, b, in[15] + 0xd8a1e681, 14 );
    MD5STEP( F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20 );
    MD5STEP( F2, a, b, c, d, in[9] + 0x21e1cde6, 5 );
    MD5STEP( F2, d, a, b, c, in[14] + 0xc33707d6, 9 );
    MD5STEP( F2, c, d, a, b, in[3] + 0xf4d50d87, 14 );
    MD5STEP( F2, b, c, d, a, in[8] + 0x455a14ed, 20 );
    MD5STEP( F2, a, b, c, d, in[13] + 0xa9e3e905, 5 );
    MD5STEP( F2, d, a, b, c, in[2] + 0xfcefa3f8, 9 );
    MD5STEP( F2, c, d, a, b, in[7] + 0x676f02d9, 14 );
    MD5STEP( F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20 );

    MD5STEP( F3, a, b, c, d, in[5] + 0xfffa3942, 4 );
    MD5STEP( F3, d, a, b, c, in[8] + 0x8771f681, 11 );
    MD5STEP( F3, c, d, a, b, in[11] + 0x6d9d6122, 16 );
    MD5STEP( F3, b, c, d, a, in[14] + 0xfde5380c, 23 );
    MD5STEP( F3, a, b, c, d, in[1] + 0xa4beea44, 4 );
    MD5STEP( F3, d, a, b, c, in[4] + 0x4bdecfa9, 11 );
    MD5STEP( F3, c, d, a, b, in[7] + 0xf6bb4b60, 16 );
    MD5STEP( F3, b, c, d, a, in[10] + 0xbebfbc70, 23 );
    MD5STEP( F3, a, b, c, d, in[13] + 0x289b7ec6, 4 );
    MD5STEP( F3, d, a, b, c, in[0] + 0xeaa127fa, 11 );
    MD5STEP( F3, c, d, a, b, in[3] + 0xd4ef3085, 16 );
    MD5STEP( F3, b, c, d, a, in[6] + 0x04881d05, 23 );
    MD5STEP( F3, a, b, c, d, in[9] + 0xd9d4d039, 4 );
    MD5STEP( F3, d, a, b, c, in[12] + 0xe6db99e5, 11 );
    MD5STEP( F3, c, d, a, b, in[15] + 0x1fa27cf8, 16 );
    MD5STEP( F3, b, c, d, a, in[2] + 0xc4ac5665, 23 );

    MD5STEP( F4, a, b, c, d, in[0] + 0xf4292244, 6 );
    MD5STEP( F4, d, a, b, c, in[7] + 0x432aff97, 10 );
    MD5STEP( F4, c, d, a, b, in[14] + 0xab9423a7, 15 );
    MD5STEP( F4, b, c, d, a, in[5] + 0xfc93a039, 21 );
    MD5STEP( F4, a, b, c, d, in[12] + 0x655b59c3, 6 );
    MD5STEP( F4, d, a, b, c, in[3] + 0x8f0ccc92, 10 );
    MD5STEP( F4, c, d, a, b, in[10] + 0xffeff47d, 15 );
    MD5STEP( F4, b, c, d, a, in[1] + 0x85845dd1, 21 );
    MD5STEP( F4, a, b, c, d, in[8] + 0x6fa87e4f, 6 );
    MD5STEP( F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10 );
    MD5STEP( F4, c, d, a, b, in[6] + 0xa3014314, 15 );
    MD5STEP( F4, b, c, d, a, in[13] + 0x4e0811a1, 21 );
    MD5STEP( F4, a, b, c, d, in[4] + 0xf7537e82, 6 );
    MD5STEP( F4, d, a, b, c, in[11] + 0xbd3af235, 10 );
    MD5STEP( F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15 );
    MD5STEP( F4, b, c, d, a, in[9] + 0xeb86d391, 21 );

    m_Hash[0] += a;
    m_Hash[1] += b;
    m_Hash[2] += c;
    m_Hash[3] += d;

    m_BufferSize = 0;
}


ostream& operator<<( ostream& out, const MD5& md5 )
{
    const ios_base::fmtflags formatflags = out.flags( ios::hex );

    const uint8_t* checksum = md5.get_checksum();
    for( int i = 0; i < 16; i++ ) out << setw( 2 ) << setfill( '0' ) << (unsigned int)checksum[i];

    out.flags( formatflags );
    return out;
}
