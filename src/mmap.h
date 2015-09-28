#ifndef __mmap_h__
#define __mmap_h__

#include <stdint.h>
#include <stddef.h>


//////////////////////////////////////////////////////////////////////////////
// class MMap
//////////////////////////////////////////////////////////////////////////////

class MMap {
public:
	MMap( void* phys_addr, size_t size );
	~MMap();

	template< typename T > T peek( void* phys_addr );
	template< typename T > void poke( void* phys_addr, T value );

	template< typename T > void set( void* phys_addr, T value );
	template< typename T > void clear( void* phys_addr, T value );
	template< typename T > void toggle( void* phys_addr, T value );

private:
	uintptr_t m_PhysAddr;
	size_t m_PageOffset;

	void* m_VirtAddr;
	size_t m_MappingSize;
};


//////////////////////////////////////////////////////////////////////////////
// class MMap template functions
//////////////////////////////////////////////////////////////////////////////

template< typename T >
inline T MMap::peek( void* phys_addr )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);
	return *virt_addr;
}

template< typename T >
inline void MMap::poke( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);
	*virt_addr = value;
}

template< typename T >
inline void MMap::set( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);
	*virt_addr |= value;
}

template< typename T >
inline void MMap::clear( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);
	*virt_addr &= ~value;
}

template< typename T >
inline void MMap::toggle( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);
	*virt_addr ^= value;
}


#endif // __mmap_h__
