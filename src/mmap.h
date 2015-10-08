#ifndef __mmap_h__
#define __mmap_h__

#include <stdint.h>
#include <stddef.h>


//////////////////////////////////////////////////////////////////////////////
// class MMap
//////////////////////////////////////////////////////////////////////////////

class MMap {
public:
    static MMap* create( void* phys_addr, size_t size );
    static MMap* create( void* phys_addr, size_t size, const char* device );

	~MMap();

	void* get_base_address();
	size_t get_size();

	template< typename T > T peek( void* phys_addr );
	template< typename T > void poke( void* phys_addr, T value );

	template< typename T > void set( void* phys_addr, T value );
	template< typename T > void clear( void* phys_addr, T value );
	template< typename T > void toggle( void* phys_addr, T value );

private:
	MMap() {}

	uintptr_t m_PhysAddr;
	size_t m_Size;
	size_t m_PageOffset;

	void* m_VirtAddr;
	size_t m_MappingSize;

	MMap( const MMap& ) = delete;
	MMap& operator=( const MMap& ) = delete;
};


//////////////////////////////////////////////////////////////////////////////
// class MMap inline functions
//////////////////////////////////////////////////////////////////////////////

inline void* MMap::get_base_address()
{
	return (void*)m_PhysAddr;
}

inline size_t MMap::get_size()
{
	return m_Size;
}


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
