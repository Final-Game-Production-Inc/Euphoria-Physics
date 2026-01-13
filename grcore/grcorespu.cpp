// 
// grcore/grcorespu.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "grcorespu.h"

#if __SPU

#include "math/intrinsics.h"
#include "vectormath/classes.h"

#include "system/criticalsection_spu.h"

namespace rage {

inline qword* gcmReserveQwordsAligned(u32 count)
{
#if 1
	if (GCM_CONTEXT->current + ((count<<2)+3) > GCM_CONTEXT->end)
		gcmCallback(GCM_CONTEXT,0);
	ALIGN_CONTEXT_TO_16(GCM_CONTEXT);
	qword *dest = (qword*)GCM_CONTEXT->current;
	GCM_CONTEXT->current += (count<<2);
#else
    vec_uint4 context = *(vec_uint4*)GCM_CONTEXT;
    u32 current = spu_extract(context,2);
    qword* end = (qword*)spu_extract(context,1);
    // align to next qword
    qword* dest = (qword*)((current+15)&~15);
    // pad previous qword with nops
    dest[-1] = spu_and(dest[-1], spu_slqwbyte((qword)(-1),15&-current));     
    if (__builtin_expect(dest + count > end, 0))
    {
        gcmCallback(GCM_CONTEXT,0);
        context = *(vec_uint4*)GCM_CONTEXT;
        dest = (qword*)spu_extract(context,2);        
    }
    *(vec_uint4*)GCM_CONTEXT = spu_insert((u32)&dest[count], context, 2);
#endif
    return dest;
}

void grcState__SetWorldFast(const spuMatrix44& mtx)
{
    qword* dest = gcmReserveQwordsAligned(14);
    dest[0] = (qword)(vec_uint4){0,0,CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD, 33),CELL_GCM_ENDIAN_SWAP(0)};
    dest[9] = (qword)(vec_uint4){0,0,CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD, 17),CELL_GCM_ENDIAN_SWAP(8)};
    spuMatrix44& cmtx = *(spuMatrix44*)&dest[1];
    spuMatrix44& worldView = *(spuMatrix44*)&dest[5];
    spuMatrix44& worldViewProj = *(spuMatrix44*)&dest[10];

    // Clean up the W column.
    cmtx.a = spu_insert(0.0f, mtx.a, 3);
    cmtx.b = spu_insert(0.0f, mtx.b, 3);
    cmtx.c = spu_insert(0.0f, mtx.c, 3);
    cmtx.d = spu_insert(1.0f, mtx.d, 3);

    // Compute world dot view and send it down
    worldView.Transform(pSpuGcmState->View, cmtx);

    // Compute world dot view dot projection and send it down
    worldViewProj.Transform(pSpuGcmState->Proj, worldView);

    // Tell Edge
//    spuMatrix43 &ew = (spuMatrix43&)pSpuGcmState->EdgeWorld;
//    ew.Transpose(mtx);

	// This is somewhat redundant with grcViewport::RegenerateDevice,
	// but this code path is used for "internal" matrix changes in drawables
	// and that intentionally bypasses the viewport code.
	union_cast<spuMatrix43*>(&pSpuGcmState->EdgeWorld)->Transpose(mtx);

	pSpuGcmState->LocalLRTB.Transform(pSpuGcmState->FrustumLRTB,cmtx);

	spuMatrix44 &destVPM = *union_cast<spuMatrix44*>( pSpuGcmState->EdgeInfo.viewProjectionMatrix );
#if HACK_GTA4
	if(spuGcmShadowWarpEnabled(pSpuGcmState->shadowType))
	{
		destVPM.Transpose(pSpuGcmState->shadowMatrix);
	}
	else
#endif // HACK_GTA4
	{
		spuMatrix44 viewProj;
		viewProj.Transform(pSpuGcmState->Proj,pSpuGcmState->View);
		destVPM.Transpose(viewProj);
	}
}

inline bool grcState__BuildEdgeClipPlanes(Vec4V* transposedPlanes, int numPlanes, spuGcmStateBase* pSpuGcmState)
{
	if (pSpuGcmState->EdgeClipPlaneEnable) // valid states are 0x01 - 0x0f (1-4 clip planes), or 0xff (extended)
	{
		Mat44V worldToLocal = *(const Mat44V*)&pSpuGcmState->EdgeWorld;
		worldToLocal.SetCol3(Vec4V(V_ZERO_WONE));

		Vec4V* src = (Vec4V*)pSpuGcmState->EdgeClipPlanes;
		Vec4V* dst = transposedPlanes;

		// first 4 clip planes
		{
			// We could optimise this by using the fact that transpose(M1.M2) == transpose(M2).transpose(M1).
			// For this to work we would need to store groups of 4 worldspace planes in transposed form,
			// and have transpose(worldToLocal) available. This would be most advantageous for the extended
			// case where we are transforming 12 clip planes.

			const Vec4V temp0 = Multiply(worldToLocal, src[0]);
			const Vec4V temp1 = Multiply(worldToLocal, src[1]);
			const Vec4V temp2 = Multiply(worldToLocal, src[2]);
			const Vec4V temp3 = Multiply(worldToLocal, src[3]);

			Transpose4x4(dst[0], dst[1], dst[2], dst[3], temp0, temp1, temp2, temp3);
		}

		if (Likely(pSpuGcmState->EdgeClipPlaneEnable == 0xff)) // extended (shadows)
		{
			src += 4;
			dst += 4;

			for (int i = 4; i < numPlanes; i += 4) // compiler should unroll this, hopefully
			{
				const Vec4V temp0 = Multiply(worldToLocal, src[0]);
				const Vec4V temp1 = Multiply(worldToLocal, src[1]);
				const Vec4V temp2 = Multiply(worldToLocal, src[2]);
				const Vec4V temp3 = Multiply(worldToLocal, src[3]);

				Transpose4x4(dst[0], dst[1], dst[2], dst[3], temp0, temp1, temp2, temp3);

				src += 4;
				dst += 4;
			}
		}

		return true;
	}

	return false;
}

// Location is derived from indices, and type is always 16-bit
void myCellGcmSetDrawIndexArray(CellGcmContextData *ctxt,uint8_t mode, uint32_t count, uint32_t indicies)
{
	uint32_t startOffset;
	uint32_t startIndex;
	uint32_t misalignedIndexCount;

	startOffset = (indicies&0x1fffffff);

	// need to compute the number of indexes from starting
	// address to next 128-byte alignment

	misalignedIndexCount = (((startOffset + 127) & ~127) - startOffset) >> 1;

	// We estimate mcount with count, which may be up to 64 too high after rounding.
	// We will end up reserving an extra single word in those cases.  Also, we always
	// assume misalignedIndexCount will be nonzero.
	Assert(7 + 2 + 1 + ((count + 255) >> 8) + 2 < 2047);
	if (ctxt->current + 7 + 2 + 1 + ((count + 255) >> 8) + 2 > ctxt->end)
		gcmCallback(ctxt, 0);
	uint32_t *current = ctxt->current;

	// CELL_GCM_RESERVE(7);

	CELL_GCM_METHOD_INVALIDATE_VERTEX_FILE(current);

	// begin
	CELL_GCM_METHOD_SET_INDEX_ARRAY_OFFSET_FORMAT(current, 
		CELL_GCM_COMMAND_CAST(indicies >> 31), startOffset, CELL_GCM_COMMAND_CAST(CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16));
	CELL_GCM_METHOD_SET_BEGIN_END(current, 
		CELL_GCM_COMMAND_CAST(mode));

	startIndex = 0;
	// starting address of first index is not 128 byte aligned
	// send the mis-aligned indices thus aligning the rest to 128 byte boundary
	if (misalignedIndexCount && (misalignedIndexCount < count))
	{
		uint32_t tmp = misalignedIndexCount-1;
		// CELL_GCM_RESERVE(2);
		CELL_GCM_METHOD_DRAW_INDEX_ARRAY(current, startIndex,tmp);
		count -= misalignedIndexCount;
		startIndex += misalignedIndexCount;
	}

	// round up count to 256(0x100) counts
	uint32_t mcount = (count + 0xff)>>8;

	// CELL_GCM_RESERVE(1+mcount);

	// [startIndex, startIndex+0xff] range in DRAW_INDEX_ARRAY
	current[0] = CELL_GCM_METHOD_NI(CELL_GCM_NV4097_DRAW_INDEX_ARRAY, mcount);
	current += 1;
	while(count > 0x100)
	{
		count -= 0x100;
		current[0] = CELL_GCM_ENDIAN_SWAP(0xFF000000 | startIndex);
		current += 1;
		startIndex += 0x100;
	}

	// remainder indices
	if(count)
	{
		--count;
		current[0] = CELL_GCM_ENDIAN_SWAP((count << 24) | startIndex);
		current += 1;
	}

	// CELL_GCM_RESERVE(2);
	CELL_GCM_METHOD_SET_BEGIN_END(current, 0);
	ctxt->current = current;
}

void DrawIndexedPrimitive(spuVertexDeclaration *vd,int primType,u32 *vertexData,u32 indexData,u32 indexCount)
{
	grcStateBlock::Flush();
	BindVertexDeclaration(GCM_CONTEXT,pSpuGcmState,vd,vertexData,g_VertexShaderInputs);

	// Perform the render
	myCellGcmSetDrawIndexArray(&ctxt,primType,indexCount,indexData);
	DRAWABLESPU_STATS_INC(GcmDrawCalls);
	DRAWABLESPU_STATS_ADD(GcmDrawIndices,indexCount);
}

void myCellGcmSetDrawArrays(CellGcmContextData *ctxt,uint8_t mode, uint32_t first, uint32_t count)
{
	uint32_t lcount;

	--count;
	lcount = count & 0xff;
	count >>= 8;

	// Make sure we don't go over the method limit
	Assert(8 + 1 + count + 2 < 2047);

	// reserve buffer size (may reserve one extra word if count is zero)
	if (ctxt->current + 8 + 1 + count + 2 > ctxt->end)
		gcmCallback(ctxt,0);
	uint32_t *current = ctxt->current;

	// hw bug workaround, send 3 invalidate vertex file
	CELL_GCM_METHOD_INVALIDATE_VERTEX_FILE_3(current);

	// Draw first batch of 1-256...
	CELL_GCM_METHOD_SET_BEGIN_END(current, CELL_GCM_COMMAND_CAST(mode));
	CELL_GCM_METHOD_DRAW_ARRAYS(current, first, lcount);
	first += lcount + 1;

	if (count) {
		current[0] = CELL_GCM_METHOD_NI(CELL_GCM_NV4097_DRAW_ARRAYS, count);
		current++;

		for(uint32_t j=0;j<count;j++){
			current[0] = CELL_GCM_ENDIAN_SWAP((first) | ((255U)<<24));
			current++;
			first += 256;
		}
	}

	// CELL_GCM_RESERVE(2);

	CELL_GCM_METHOD_SET_BEGIN_END(current, 0);
	ctxt->current = current;
}

void DrawPrimitive(spuVertexDeclaration *vd,int primType,u32 *vertexData,u32 startVertex,u32 vertexCount)
{
	grcStateBlock::Flush();
	BindVertexDeclaration(GCM_CONTEXT,pSpuGcmState,vd,vertexData,g_VertexShaderInputs);

	// Perform the render
	myCellGcmSetDrawArrays(GCM_CONTEXT,primType,startVertex,vertexCount);
	DRAWABLESPU_STATS_INC(GcmDrawCalls);
	DRAWABLESPU_STATS_ADD(GcmDrawIndices,vertexCount);
}

// PURPOSE: Make sure the instance data we are about to put to main memory is
//          correctly aligned in local store so that we don't need to block.
// PARAM:   dst     destination local store to use if the data needs expansion needs
//                  to be (((stride+15)&~15)*count) bytes, and 16-byte aligned
//          src     source local store data
//          stride  stride of source data
//          count   number of stride sized elements of source data
static void ExpandInstanceData(void *dst,const void *src,u32 stride,u32 count)
{
	const u32 srcStride = stride;
	const u32 dstStride = (stride + 15) & ~15;
	void *const dstEnd = (char*)dst + dstStride * count;
	const void *const srcEnd = (char*)src + srcStride * count;

	// Loop backwards through each of the count elements.
	qword d = si_from_ptr(dstEnd);
	qword s = si_from_ptr(srcEnd);
	do
	{
		qword d_end = d;
		d = si_sf(si_from_uint(dstStride), d);
		s = si_sf(si_from_uint(srcStride), s);

		// Generate a shuffle control that will take to input source qwords and
		// rotate them to get the appropriately aligned qword for the
		// destination.
		qword shuf = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		qword rot = si_andbi(si_shufb(s, s, si_ilh(0x0303)), 15);
		shuf = si_a(shuf, rot);

		// Loop forwards through the qwords for this element, and copy them to
		// the destination.
		qword s0, s1;
		s1 = si_lqd(s, 0);
		qword d_ = d;
		qword s_ = s;
		do
		{
			s0 = s1;
			s1 = si_lqd(s_, 16);
			s_ = si_ai(s_, 16);
			qword q = si_shufb(s0, s1, shuf);
			si_stqd(q, d_, 0);
			d_ = si_ai(d_, 16);
		}
		while (si_to_int(si_clgt(d_end, d_)) != 0);
	}
	while (si_to_ptr(d) > dst);
}

__attribute__((__noinline__)) void CopyInstanceData(uint64_t ppuAddr,void *localStore,int stride,int count)
{
	char *const freeSpuScratch = spuScratch;

	// If the source data stride is not already multiple of 16-bytes, then
	// expand it into the scratch space so that it is correctly aligned for the
	// dma put.
	void *expanded = localStore;
	if ((stride & 15) != 0) {
		u32 dstStride = (stride+15)&~15;
		expanded = spuScratch;
		Assert((char*)expanded+dstStride*count <= spuScratchTop);
		ExpandInstanceData(expanded,localStore,stride,count);
		stride = dstStride;
	}

	const u32 size = stride*count;

	// Copy the data.  Not using sysDma[Small]Put due to annoying asserts about
	// dma size (would need to select either sysDmaSmallPut or sysDmaPut
	// depending on size).
	spu_writech(MFC_LSA,   (u32)expanded);
	spu_writech(MFC_EAL,   ppuAddr);
	spu_writech(MFC_Size,  size);
	spu_writech(MFC_TagID, spuGetTag);
	spu_writech(MFC_Cmd,   MFC_PUT_CMD);

	spuScratch = freeSpuScratch;

	sysDmaWaitTagStatusAll(1<<spuGetTag);
}

__attribute__((__noinline__)) void CopyMultiInstanceData(spuCmd_grmShaderGroup__SetVarCommonBase *cmd,char *src)
{
	grcInstanceData::Entry **datas = (grcInstanceData::Entry**) spuGetDataNoWait(cmd->datas, cmd->shaderCount * sizeof(grcInstanceData::Entry*));
	spuEffectVarPair *pairs = (spuEffectVarPair*) spuGetDataNoWait(cmd->vars, cmd->varCount * sizeof(spuEffectVarPair));

	char *const freeSpuScratch = spuScratch;

	// If the source data stride is not already multiple of 16-bytes, then
	// expand it into the scratch space so that it is correctly aligned for the
	// dma put.
	void *expanded = src;
	u32 stride = cmd->stride;
	u32 arrayCount = cmd->arrayCount;
	if ((stride & 15) != 0) {
		u32 dstStride = (stride+15)&~15;
		expanded = spuScratch;
		Assert((char*)expanded+dstStride*arrayCount <= spuScratchTop);
		ExpandInstanceData(expanded,src,stride,arrayCount);
		stride = dstStride;
	}

	const u32 size = stride*arrayCount;

	// Block on dma get of datas and pairs.
	sysDmaWaitTagStatusAll(1<<spuGetTag);

	// Prefetch the data pointer for the first loop.
	qword ppuAddrQW[2];
	qword ppuAddrEA[2];
	ppuAddrEA[0] = si_from_ptr(&datas[pairs[0].index][pairs[0].var-1].Float);
	sysDmaGet(ppuAddrQW, si_to_uint(ppuAddrEA[0])&~15, 16, spuGetTag);

	const u32 varCount = cmd->varCount;
	Assert(varCount);
	for (u32 i=0; i<varCount; ++i) {
		// Prefetch the data pointer for the next loop.
		const u32 bufIdx = i&1;
		const u32 otherBufIdx = bufIdx^1;
		if (i+1 < varCount) {
			ppuAddrEA[otherBufIdx] = si_from_ptr(&datas[pairs[i+1].index][pairs[i+1].var-1].Float);
			sysDmaGet(ppuAddrQW+otherBufIdx, si_to_uint(ppuAddrEA[otherBufIdx])&~15, 16, spuGetTag+otherBufIdx);
		}

		// Wait for the data pointer for this loop.
		sysDmaWaitTagStatusAll(1<<(spuGetTag+bufIdx));
		const u32 ppuAddr = si_to_uint(si_rotqby(ppuAddrQW[bufIdx], ppuAddrEA[bufIdx]));

		// Copy the data.  Not using sysDma[Small]Put due to annoying asserts
		// about dma size (would need to select either sysDmaSmallPut or
		// sysDmaPut depending on size).
		spu_writech(MFC_LSA,   (u32)expanded);
		spu_writech(MFC_EAL,   ppuAddr);
		spu_writech(MFC_Size,  size);
		spu_writech(MFC_TagID, spuGetTag+2);
		spu_writech(MFC_Cmd,   MFC_PUT_CMD);
	}

	spuScratch = freeSpuScratch;

	sysDmaWaitTagStatusAll(1<<(spuGetTag+2));
}

} // namespace rage

#endif
