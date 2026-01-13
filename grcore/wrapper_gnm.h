//
// grcore/wrapper_gnm.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//
#ifndef GRCORE_WRAPPER_GNM_H
#define GRCORE_WRAPPER_GNM_H

#if RSG_ORBIS

#include <gnm.h>
#include "amdsouthernislands.h"
#include "system/memory.h"

#define WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES   (__ASSERT)

#if WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES
#	define WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(...)  __VA_ARGS__
#else
#	define WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(...)
#endif


namespace rage {

void *allocateSystemSharedMemory(uint32_t size,uint32_t align);
void *allocateVideoSharedMemory(uint32_t size,uint32_t align);
void freeVideoSharedMemory(void *ptr);
void* allocateVideoPrivateMemory(uint32_t size,uint32_t align);
void freeVideoPrivateMemory(void *addr);
void copyToVideoMemory(void *dest,const void *src,uint32_t bytes);
bool isAllocatedVideoMemory(void *ptr);

sysMemAllocator& getVideoPrivateMemory();


inline void initAsConstantBufferOptimized(sce::Gnm::Buffer &dest,void *baseAddr,uint32_t size)
{
	dest.m_regs[0] = AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_0(baseAddr);
	dest.m_regs[1] = AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_1(baseAddr);
	dest.m_regs[2] = AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_2(size>>4);
	dest.m_regs[3] = AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_3();
}

inline void initAsVertexBufferOptimized(sce::Gnm::Buffer &dest,void *baseAddr,unsigned stride,unsigned numVertices,u32 dword3)
{
	dest.m_regs[0] = AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_0(baseAddr);
	dest.m_regs[1] = AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_1(baseAddr,stride);
	dest.m_regs[2] = AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_2(numVertices);
	dest.m_regs[3] = dword3;
}

#if !WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES
	inline void initAsConstantBuffer(sce::Gnm::Buffer &dest,void *baseAddr,uint32_t size)
	{
		initAsConstantBufferOptimized(dest,baseAddr,size);
	}
	inline void initAsVertexBuffer(sce::Gnm::Buffer &dest,void *baseAddr,unsigned stride,unsigned numVertices,u32 dword3)
	{
		initAsVertexBufferOptimized(dest,baseAddr,stride,numVertices,dword3);
	}
#else
	void initAsConstantBuffer(sce::Gnm::Buffer &dest,void *baseAddr,uint32_t size);
	void initAsVertexBuffer(sce::Gnm::Buffer &dest,void *baseAddr,unsigned stride,unsigned numVertices,u32 dword3,sce::Gnm::DataFormat fmt);
#endif

}

#include <sdk_version.h>

#if SCE_ORBIS_SDK_VERSION < 0x00920020u
namespace sce { 
	namespace Gnm {
		typedef enum Alignment_920 {
			kAlignmentOfShaderInBytes			= 256,  ///< Alignment of shader binaries, in bytes.
			kAlignmentOfTessFactorBufferInBytes = 256,  ///< Alignment of tessellation factor buffers, in bytes.
			kAlignmentOfOcclusionQueryInBytes	= 16,   ///< Alignment of OcclusionQueryResults objects, in bytes.
			kAlignmentOfIndirectArgsInBytes     = 8,    ///< Alignment of a DrawIndirectArgs, DrawIndexIndirectArgs, and DispatchIndirectArgs objects, in bytes.
			kAlignmentOfBufferInBytes			= 4,    ///< Alignment of Buffer data, in bytes.
			kAlignmentOfFetchShaderInBytes		= 4,    ///< Alignment of fetch shader binaries, in bytes.
		} Alignment_920;
	}
}
#endif

#define gfxc (*g_grcCurrentContext)

#endif // RSG_ORBIS

#endif // GRCORE_WRAPPER_GNM_H
