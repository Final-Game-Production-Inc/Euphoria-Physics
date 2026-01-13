// 
// grcore/instancerendererspu.h
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_INSTANCERENDERERSPU_H
#define GRCORE_INSTANCERENDERERSPU_H

#include "grcore/drawmode.h"
#include "system/codefrag_spu.h"
#include "system/taskheader.h"

struct CellGcmContextData;

namespace rage
{

class grcIndexBuffer;
struct grcVertexDeclaration;
struct spuVertexDeclaration;

struct grcInstanceRenderer
{
	struct DmaListItem
	{
		u32 Ea;
		u32 ElementSize;
		u32 Count;

		// INTERNAL USE ONLY
		u32 BatchCount;
		u32 CommandBufferHoleScratchSize;
		u32 InputInstanceScratchSize;
		u32 OutputVertexScratchSize;
	};

	struct PpuContext
	{
		DmaListItem* DmaList;
		u32 DmaListCount;
		u32 NumVertexesPerInstance;
		u32 NumIndexesPerInstance;
		grcDrawMode DrawMode;
		grcVertexDeclaration* VertexDecl;
		grcIndexBuffer* IndexBuffer;
		void* TypeData;
		u32 TypeDataSize;
		bool CacheTypeData;
	};

	struct SpuContext
	{
		u32 DmaListEa;						// + 4
		u32 DmaListCount;					// + 8
		u32 OutputBufferInfoEa;				// + 12
		u32 CommandBufferHoleEa;			// + 16
		spuVertexDeclaration* VertexDecl;	// + 20
		u32 NonZeroStreamOffsets[3];		// + 32
		void* TypeData;						// + 36
		u32 TypeDataSize;					// + 40
		u32 FrameCount;						// + 44
		u32 JumpToNextOffset;				// + 48
		u32 IndexBufferOffset;				// + 52
		u32 CommandBufferHoleSize;			// + 56
		u32 GcmControlEa;					// + 60
		u16 VertexShaderInputs;				// + 62
		u8 NumVertexesPerInstance;			// + 63
		u8 NumIndexesPerInstance;			// + 64
		u8 DrawMode;						// + 65
		u8 InputDmaTag;						// + 66
		u8 OutputDmaTag;					// + 67
		u8 Pad[13];							// + 80
	} ;
	CompileTimeAssert(sizeof(grcInstanceRenderer::SpuContext) <= sizeof(((sysTaskParameters*)0xdeadbeef)->UserData));

	struct InstanceContext
	{
		u32 InstanceIdx;
		const u8* InputInstanceData;
		u32 NumVertexesPerInstance;
		void* TypeData;
		u8* OutputVertexData;
		u8 InputDmaTag;
		u8 OutputDmaTag;
		u32 InputInstanceSize;
		u32 OutputVertexStride;
	};

#if __PPU
	static void Init(u32 ringBufferSize);
	static void Shutdown();

	static void Create(grcInstanceRenderer::PpuContext* ppuContext, const char* codeName, void* codeStart, u32 codeSize, bool isCodeFrag = true);

	static bool InitializeRingBuffer();
	static u32* AllocateRingBuffer(u32 wordCount, bool forceKick = false);
	static void FreeRingBuffer(void* ptr, u32 wordCount, CellGcmContextData* gcmCtx);
#endif // __PPU
};

#if __PPU
#define grcInstanceRenderer_Create(ppuContext, codeName) ::rage::grcInstanceRenderer::Create(ppuContext, #codeName, FRAG_INTERFACE_START(codeName), reinterpret_cast<u32>(FRAG_INTERFACE_SIZE(codeName)), true)
#define grcInstanceRenderer_Create2(ppuContext, codeName) ::rage::grcInstanceRenderer::Create(ppuContext, #codeName, TASK_INTERFACE_START(codeName), reinterpret_cast<u32>(TASK_INTERFACE_SIZE(codeName)), false)
#endif // __PPU

} // namespace rage

#endif // GRCORE_INSTANCERENDERERSPU_H