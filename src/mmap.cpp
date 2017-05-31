/*  Copyright (c) 2015-2017, Martin Hammel
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

#include "mmap.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>


//////////////////////////////////////////////////////////////////////////////
// class MMap implementation
//////////////////////////////////////////////////////////////////////////////

sig_atomic_t MMap::s_SignalEnable = 0;
sigjmp_buf MMap::s_SignalRecovery;


MMap* MMap::create( void* phys_addr, size_t size )
{
    return create( phys_addr, size, "/dev/mem" );
}


MMap* MMap::create( void* phys_addr, size_t size, const char* device )
{
    int fd = open( device, O_RDWR );
    if( fd < 0 ) return nullptr;

    MMap* mmap = new MMap;

    const int pagesize = getpagesize();

    mmap->m_PhysAddr = (uintptr_t)phys_addr;
    mmap->m_Size = size;
    mmap->m_PageOffset = mmap->m_PhysAddr % pagesize;

    const uintptr_t page_addr = mmap->m_PhysAddr - mmap->m_PageOffset;

    mmap->m_MappingSize = size + mmap->m_PageOffset;
    mmap->m_MappingSize -= mmap->m_MappingSize % pagesize;
    if( mmap->m_MappingSize < size + mmap->m_PageOffset ) mmap->m_MappingSize += pagesize;

    mmap->m_VirtAddr = ::mmap( 0, mmap->m_MappingSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_addr );

    close(fd);

    if( mmap->m_VirtAddr == MAP_FAILED ) {
        delete mmap;
        return nullptr;
    }
    else return mmap;
}


MMap::~MMap()
{
	munmap( m_VirtAddr, m_MappingSize );
}

void MMap::enable_signal_handler()
{
    s_SignalEnable = 0;

    struct sigaction sa;

    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;

    sigaction( SIGBUS, &sa, nullptr );
}

void MMap::disable_signal_handler()
{
    struct sigaction sa;

    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;

    sigaction( SIGBUS, &sa, nullptr );
}

void MMap::signal_handler( int )
{
    if( s_SignalEnable ) siglongjmp( s_SignalRecovery, 1);
}
