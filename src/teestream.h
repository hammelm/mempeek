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

#ifndef __teestream_h__
#define __teestream_h__

#include <streambuf>
#include <ostream>
#include <vector>


//////////////////////////////////////////////////////////////////////////////
// class basic_teebuf
//////////////////////////////////////////////////////////////////////////////

template< typename CharT, typename Traits = std::char_traits< CharT > >
class basic_teebuf : public std::basic_streambuf< CharT, Traits >
{
public:
    void attach( std::basic_streambuf< CharT, Traits >* buf );
    void detach( std::basic_streambuf< CharT, Traits >* buf );

protected:
    using typename std::basic_streambuf< CharT, Traits >::int_type;

    int_type overflow( int_type ch ) override;
    int sync() override;

private:
    std::vector< std::basic_streambuf< CharT, Traits >* > m_AttachedBuffers;
};


//////////////////////////////////////////////////////////////////////////////
// class basic_teestream
//////////////////////////////////////////////////////////////////////////////

template< typename CharT, typename Traits = std::char_traits< CharT > >
class basic_teestream : public std::basic_ostream< CharT, Traits >
{
public:
    basic_teestream();
    basic_teestream( std::basic_ostream< CharT, Traits >& stream );

    void attach( std::basic_ostream< CharT, Traits >& stream );
    void detach( std::basic_ostream< CharT, Traits >& stream );

private:
    basic_teebuf< CharT, Traits > m_Teebuf;
};


//////////////////////////////////////////////////////////////////////////////
// class basic_teebuf template functions
//////////////////////////////////////////////////////////////////////////////

template< typename CharT, typename Traits >
inline void basic_teebuf< CharT, Traits >::attach( std::basic_streambuf< CharT, Traits >* buf )
{
    m_AttachedBuffers.push_back( buf );
}

template< typename CharT, typename Traits >
inline void basic_teebuf< CharT, Traits >::detach( std::basic_streambuf< CharT, Traits >* buf )
{
    size_t num_buffers = m_AttachedBuffers.size();

    for( size_t i = 0; i < num_buffers; ) {
        if( m_AttachedBuffers[i] != buf ) i++;
        else {
            m_AttachedBuffers[i] = m_AttachedBuffers[ --num_buffers ];
            m_AttachedBuffers.pop_back();
        }
    }
}

template< typename CharT, typename Traits >
inline typename basic_teebuf< CharT, Traits >::int_type basic_teebuf< CharT, Traits >::overflow( int_type ch )
{
    int_type ret = Traits::eof() - 1;

    for( auto buf: m_AttachedBuffers ) {
        if( buf->sputc( ch ) == Traits::eof() ) ret = Traits::eof();
    }

    return ret;
}

template< typename CharT, typename Traits >
inline int basic_teebuf< CharT, Traits >::sync()
{
    int ret = 0;

    for( auto buf: m_AttachedBuffers ) {
        if( buf->pubsync() != 0 ) ret = -1;
    }

    return ret;
}


//////////////////////////////////////////////////////////////////////////////
// class basic_teestream template functions
//////////////////////////////////////////////////////////////////////////////

template< typename CharT, typename Traits >
inline basic_teestream< CharT, Traits >::basic_teestream()
 : std::basic_ostream< CharT, Traits >( &m_Teebuf ) // TODO: is this ok? m_Teebuf is not constructed
{}                                                  //       when base class constructor is called

template< typename CharT, typename Traits >
inline basic_teestream< CharT, Traits >::basic_teestream( std::basic_ostream< CharT, Traits >& stream )
 : basic_teestream()
{
    attach( stream );
}

template< typename CharT, typename Traits >
inline void basic_teestream< CharT, Traits >::attach( std::basic_ostream< CharT, Traits >& stream )
{
    m_Teebuf.attach( stream.rdbuf() );
}

template< typename CharT, typename Traits >
inline void basic_teestream< CharT, Traits >::detach( std::basic_ostream< CharT, Traits >& stream )
{
    m_Teebuf.detach( stream.rdbuf() );
}


//////////////////////////////////////////////////////////////////////////////
// typedefs for specialized teestreams
//////////////////////////////////////////////////////////////////////////////

typedef basic_teestream<char> teestream;
typedef basic_teestream<wchar_t> wteestream;


#endif // __teestream_h__
