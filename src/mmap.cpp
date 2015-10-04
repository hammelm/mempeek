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

MMap::MMap( void* phys_addr, size_t size )
{
    int fd = open( "/dev/mem", O_RDWR );
    if( fd < 0 ) abort();

    const int pagesize = getpagesize();

    m_PhysAddr = (uintptr_t)phys_addr;
    m_Size = size;
    m_PageOffset = m_PhysAddr % pagesize;

    const uintptr_t page_addr = m_PhysAddr - m_PageOffset;

    m_MappingSize = size + m_PageOffset;
    m_MappingSize -= m_MappingSize % pagesize;
    if( m_MappingSize < size + m_PageOffset ) m_MappingSize += pagesize;

    m_VirtAddr = mmap( 0, m_MappingSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_addr );
    if( m_VirtAddr == MAP_FAILED ) abort();

    close(fd);
}


MMap::~MMap()
{
	munmap( m_VirtAddr, m_MappingSize );
}
