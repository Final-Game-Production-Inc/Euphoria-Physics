//  
// data/struct.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "data/struct.h"
#if __WIN32PC
#include "system/platform.h"
#endif
#include "atl/array.h"
#include "system/param.h"

typedef long long longlong;
CompileTimeAssertSize(int,4,4);
CompileTimeAssertSize(float,4,4);
CompileTimeAssertSize(double,8,8);
CompileTimeAssertSize(long,4,RSG_ORBIS? 8 : 4);
CompileTimeAssertSize(longlong,8,8);

namespace rage {

#if __DECLARESTRUCT
#if __RESOURCECOMPILER
bool g_ByteSwap = false;
#else
void datRelocatePointer(char*&) { }
#endif

void datTypeStruct::AddField(size_t offset,size_t thisSize,const char* name) {
	// Implicitly add any missing vtbl pointer
	if (offset == sizeof(void*) && m_Offset == 0)
		m_Offset = sizeof(void*);
	(void)name;
#if !__64BIT
	if (offset != m_Offset)
		Errorf("Unexpected padding before field %s in %s? (Actual offset: %d, Total counted: %d)",name?name:"<unknown>",m_Name,(int)offset,(int)m_Offset);
#endif
	m_Offset = offset + thisSize;
}

void datTypeStruct::VerifySize(size_t s) { 
	(void)s;
#if !__64BIT
	if (s != m_Offset && !(s == sizeof(void *) && !m_Offset))
		Errorf("Unexpected padding at end of structure %s? (Actual size: %d, Total counted: %d)",m_Name,(int)s,(int)m_Offset); 
#endif
}

void datSwapper(u16 &instance) {
	if (g_ByteSwap) {
		instance = (instance >> 8) | (instance<< 8);
	}
}

void datSwapper(u32 &instance) {
	if (g_ByteSwap) {
		instance = (instance >> 24) | ((instance >> 8) & 0xFF00) | ((instance << 8) & 0xFF0000) | (instance << 24);
	}
}

void datSwapper(u64 &instance) {
	if (g_ByteSwap) {
		instance = ( instance << 56 ) |
            ( ( instance << 40 ) & 0x00FF000000000000ULL ) |
            ( ( instance << 24 ) & 0x0000FF0000000000ULL ) |
            ( ( instance << 8 )  & 0x000000FF00000000ULL ) |
            ( ( instance >> 8 )  & 0x00000000FF000000ULL ) |
            ( ( instance >> 24 ) & 0x0000000000FF0000ULL ) |
            ( ( instance >> 40 ) & 0x000000000000FF00ULL ) |
            ( instance >> 56 );
	}
}

void datSwapperGeneric(u8& instance, int nSize)
{
	if( g_ByteSwap )
	{
		u8* pBeginPtr = &instance;
		u8* pEndPtr = pBeginPtr + (nSize - 1);
		for( int i = 0; i < (nSize / 2); i++ )
		{
			u8 temp = *pBeginPtr;
			*pBeginPtr++ = *pEndPtr;
			*pEndPtr-- = temp;
		}
	}
}

void datSwapper(u128& instance)
{
	if( g_ByteSwap )
	{
		u8* pBeginPtr = (u8*)&instance;
		u8* pEndPtr = pBeginPtr + 15;
		for( int i = 0; i < 8; i++ )
		{
			u8 temp = *pBeginPtr;
			*pBeginPtr++ = *pEndPtr;
			*pEndPtr-- = temp;
		}
	}
}


void datSwapArrayCount(void *pointer,int ASSERT_ONLY(count)) {
	size_t p = (size_t) pointer;
	// Attempt to discover the correct cookie offset
	if (p & 15) {
		p -= 4;
		Assertf(*(int*)p == count,"Expected %x, got %x",count,*(int*)p);
		datSwapper(*(int*)p);
	}
	else {
		p -= 16;
		Assertf(*(int*)p == count,"Expected %x, got %x",count,*(int*)p);
#if __WIN32PC
		if (g_sysPlatform == platform::PS3)
			datSwapperGeneric(reinterpret_cast<u8&>(*(u8*)p), 16);
		else
#endif // __WIN32PC
			datSwapper(*(int*)p);
	}

#if __RESOURCECOMPILER
	// We're calling this for the side effect of marking the memory as not leaked.
	void *pTemp = (void*)p;
	datRelocatePointer(pTemp);
#endif
}

#endif

}	// namespace rage
