//
// grcore/wrapper_gnm.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#if RSG_ORBIS

#include "wrapper_gnm.h"

#if WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES

#include "math/amath.h"

namespace rage
{

void initAsConstantBuffer(sce::Gnm::Buffer &dest,void *baseAddr,uint32_t size)
{
	FatalAssert(((uptr)baseAddr&3) == 0);
	initAsConstantBufferOptimized(dest,baseAddr,size);
	sce::Gnm::Buffer check;
	check.initAsConstantBuffer(baseAddr,size);
	FatalAssert(memcmp(&check,&dest,sizeof(check)) == 0);
}

void initAsVertexBuffer(sce::Gnm::Buffer &dest,void *baseAddr,unsigned stride,unsigned numVertices,u32 dword3,sce::Gnm::DataFormat fmt)
{
	// See https://ps4.scedev.net/forums/thread/41101/#n276337 for details on
	// the alignment and stride restrictions asserted on here.

	ASSERT_ONLY(const unsigned requiredAlignment = Min(fmt.getBytesPerElement(), 4u);)
	FatalAssert(((unsigned)(uptr)baseAddr&(requiredAlignment-1)) == 0);
	FatalAssert((stride&(requiredAlignment-1)) == 0);

	initAsVertexBufferOptimized(dest,baseAddr,stride,numVertices,dword3);

	sce::Gnm::Buffer check;
	check.initAsVertexBuffer(baseAddr,fmt,stride,numVertices);

#	if (SCE_ORBIS_SDK_VERSION & 0xffff0000) == 0x01600000
		// Hack for url:bugstar:1793360.  Don't compare the first byte of the V#
		// (contains the 8 least significant bits of the base address).  SDK1.6
		// is incorrectly rounding the value down to a 4-byte boundary.  See
		// forum link above.
		FatalAssert(memcmp((char*)&check+1,(char*)&dest+1,sizeof(check)-1) == 0);
#	else
		FatalAssert(memcmp(&check,&dest,sizeof(check)) == 0);
#	endif
}

}
// namespace rage

#endif // !__OPTIMIZED

#endif // RSG_ORBIS
