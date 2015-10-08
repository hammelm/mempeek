#include "mmap.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class MMap implementation
//////////////////////////////////////////////////////////////////////////////

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
