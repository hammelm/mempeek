#ifndef __mmap_h__
#define __mmap_h__

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <signal.h>


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

    bool has_failed();

	template< typename T > T peek( void* phys_addr );
	template< typename T > void poke( void* phys_addr, T value );

	template< typename T > void set( void* phys_addr, T value );
	template< typename T > void clear( void* phys_addr, T value );
	template< typename T > void toggle( void* phys_addr, T value );

	static void enable_signal_handler();
	static void disable_signal_handler();

private:
	MMap() {}

	static void signal_handler( int );

	uintptr_t m_PhysAddr;
	size_t m_Size;
	size_t m_PageOffset;

	void* m_VirtAddr;
	size_t m_MappingSize;

	bool m_HasFailed = false;

	static sig_atomic_t s_SignalEnable;
	static sigjmp_buf s_SignalRecovery;

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

inline bool MMap::has_failed()
{
    return m_HasFailed;
}


//////////////////////////////////////////////////////////////////////////////
// class MMap template functions
//////////////////////////////////////////////////////////////////////////////

template< typename T >
inline T MMap::peek( void* phys_addr )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);

	T ret = 0;

    m_HasFailed = false;
    s_SignalEnable = 1;
    if( sigsetjmp( s_SignalRecovery, 1 ) == 0 ) ret = *virt_addr;
    else m_HasFailed = true;
    s_SignalEnable = 0;

	return ret;
}

template< typename T >
inline void MMap::poke( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);

	m_HasFailed = false;
	s_SignalEnable = 1;
    if( sigsetjmp( s_SignalRecovery, 1 ) == 0 ) *virt_addr = value;
    else m_HasFailed = true;
    s_SignalEnable = 0;
}

template< typename T >
inline void MMap::set( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);

    m_HasFailed = false;
    s_SignalEnable = 1;
    if( sigsetjmp( s_SignalRecovery, 1 ) == 0 ) *virt_addr |= value;
    else m_HasFailed = true;
    s_SignalEnable = 0;
}

template< typename T >
inline void MMap::clear( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);

    m_HasFailed = false;
    s_SignalEnable = 1;
    if( sigsetjmp( s_SignalRecovery, 1 ) == 0 ) *virt_addr &= ~value;
    else m_HasFailed = true;
    s_SignalEnable = 0;
}

template< typename T >
inline void MMap::toggle( void* phys_addr, T value )
{
	uintptr_t offset = (uintptr_t)phys_addr - m_PhysAddr + m_PageOffset;
	volatile T* virt_addr = (T*)((uint8_t*)m_VirtAddr + offset);

    m_HasFailed = false;
    s_SignalEnable = 1;
    if( sigsetjmp( s_SignalRecovery, 1 ) == 0 ) *virt_addr ^= value;
    else m_HasFailed = true;
    s_SignalEnable = 0;
}


#endif // __mmap_h__
