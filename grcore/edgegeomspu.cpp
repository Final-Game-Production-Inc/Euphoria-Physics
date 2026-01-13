// 
// grcore/edgegeomspu.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#if __SPU 
#include "grcore/grcorespu.h"
#include "edgegeomspu.h"

#include <cell/dma.h>
#include <cell/atomic.h>
#include <cell/spurs/common.h>
#include <cell/spurs/job_chain.h>
#include <cell/spurs/job_context.h>
#include <vmx2spu.h>
#include <spu_printf.h> 
#include <string.h>
 
#include "edge/geom/edgegeom.h"
#include "grcore/edgegeomspu.h"
#include "grcore/edgegeomvtx.h"
#include "grcore/effect_values.h"
#include "system/typeinfo.h"
#include "system/ppu_symbol.h"
#include "vectormath/classes.h"

using namespace rage;
using namespace rage::edgegeomspujob;

#include "grcore/edge_callbacks.cpp"

#define TASK_SPU_DEBUG_SPEW (0) 
#define DISABLE_EDGE_GEOMETRY (0)

uint32_t g_VertexShaderInputMask = 1;

void rageSetVertexDataArrayOffset(CellGcmContextData *thisContext, const uint8_t index,
								const uint8_t location, const uint32_t offset)
{
	if (g_VertexShaderInputMask & (1 << index))
	{
		cellGcmSetVertexDataArrayOffsetUnsafeInline(thisContext, index, location, offset);
	}
}

static void rageSetVertexDataArrayOffsetsByDescription(EdgeGeomSpuContext *ctx, CellGcmContextData *gcmCtx,
													   EdgeGeomLocation *loc, EdgeGeomVertexStreamDescription *streamDesc)
{
	// Write Attribute Addresses and formats
	uint32_t offset = loc->offset;
	uint32_t location = loc->location;
	for(uint32_t iAttr=0; iAttr<streamDesc->numAttributes; ++iAttr)
	{
		EdgeGeomAttributeBlock &attr = streamDesc->blocks[iAttr].attributeBlock;
		rageSetVertexDataArrayOffset(gcmCtx, attr.vertexProgramSlotIndex,
			location, offset + attr.offset);
	}
}

void rageSetVertexDataArrays(EdgeGeomSpuContext *ctx, CellGcmContextData *gcmCtx, EdgeGeomLocation *loc)
{
	// Write attribute offsets
	uint32_t offset = loc->offset;
	uint32_t location = loc->location;
	switch(ctx->spuConfigInfo.outputVertexFormatId)
	{
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3:
		rageSetVertexDataArrayOffset(gcmCtx, 0, location, offset);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_X11Y11Z10N:
		rageSetVertexDataArrayOffset(gcmCtx, 0,  location, offset);
		rageSetVertexDataArrayOffset(gcmCtx, 2,  location, offset + 12);
		rageSetVertexDataArrayOffset(gcmCtx, 14, location, offset + 16);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_I16Nc4:
		rageSetVertexDataArrayOffset(gcmCtx, 0,  location, offset);
		rageSetVertexDataArrayOffset(gcmCtx, 2,  location, offset + 12);
		rageSetVertexDataArrayOffset(gcmCtx, 14, location, offset + 16);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_X11Y11Z10N_X11Y11Z10N:
		rageSetVertexDataArrayOffset(gcmCtx, 0,  location, offset);
		rageSetVertexDataArrayOffset(gcmCtx, 2,  location, offset + 12);
		rageSetVertexDataArrayOffset(gcmCtx, 14, location, offset + 16);
		rageSetVertexDataArrayOffset(gcmCtx, 15, location, offset + 20);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4:
		rageSetVertexDataArrayOffset(gcmCtx, 0, location, offset);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4_X11Y11Z10N_X11Y11Z10N:
		rageSetVertexDataArrayOffset(gcmCtx, 0,  location, offset);
		rageSetVertexDataArrayOffset(gcmCtx, 2,  location, offset + 16);
		rageSetVertexDataArrayOffset(gcmCtx, 14, location, offset + 20);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4_X11Y11Z10N_I16Nc4:
		rageSetVertexDataArrayOffset(gcmCtx, 0,  location, offset);
		rageSetVertexDataArrayOffset(gcmCtx, 2,  location, offset + 16);
		rageSetVertexDataArrayOffset(gcmCtx, 14, location, offset + 20);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4_X11Y11Z10N_X11Y11Z10N_X11Y11Z10N:
		rageSetVertexDataArrayOffset(gcmCtx, 0,  location, offset);
		rageSetVertexDataArrayOffset(gcmCtx, 2,  location, offset + 16);
		rageSetVertexDataArrayOffset(gcmCtx, 14, location, offset + 20);
		rageSetVertexDataArrayOffset(gcmCtx, 15, location, offset + 24);
		break;
	case EDGE_GEOM_RSX_VERTEX_FORMAT_EMPTY:
		break;
	default:
		Assert(!ctx->customFormatInfo.setVertexDataArraysCallback);
		if (ctx->customFormatInfo.outputStreamDesc != 0)
		{
			rageSetVertexDataArrayOffsetsByDescription(ctx, gcmCtx, loc,
				ctx->customFormatInfo.outputStreamDesc);
		}
		else
		{
			// ERROR
		}
		break;
	}
}

#if ENABLE_EDGE_CULL_CLIPPED_TRIANGLES

#define EDGE_CULL_CLIPPED_TRIANGLES_USE_SI_INTRINSICS   1
#if !EDGE_CULL_CLIPPED_TRIANGLES_USE_SI_INTRINSICS

__forceinline unsigned FilterCulledTriangles(u16 *indices, unsigned numIndices, const Vec4V *positions)
{
	struct tri { u16 indices[3]; };
	const qword sizeof_tri = si_il(sizeof(tri)); // 6
	tri* dst = (tri*)indices;
	tri* src = dst;
	for (unsigned i=0; i<numIndices; i+=3)
	{
		const u16 index0 = src->indices[0];
		const u16 index1 = src->indices[1];
		const u16 index2 = src->indices[2];

		src++;

		dst->indices[0] = index0;
		dst->indices[1] = index1;
		dst->indices[2] = index2;

		const qword p0 = (qword)positions[index0].GetIntrin128();
		const qword p1 = (qword)positions[index1].GetIntrin128();
		const qword p2 = (qword)positions[index2].GetIntrin128();

		const qword advance = si_rotqbyi(si_ceqi(si_and(si_and(p0, p1), p2), 0), 12);

		// advance dst if bitwise AND of flags is zero
		dst = (tri*)si_to_ptr(si_a(si_from_ptr(dst), si_and(sizeof_tri, advance)));
	}
	return (u16*)dst - indices;
}

#else   // EDGE_CULL_CLIPPED_TRIANGLES_USE_SI_INTRINSICS

__forceinline unsigned FilterCulledTriangles(u16 *indices, unsigned numIndices, const Vec4V *positions)
{
	Assert(((u32)indices & 15) == 0);
	qword pSrcIndices = si_from_ptr(indices);
	pSrcIndices = si_shufb(pSrcIndices, pSrcIndices, si_ila(0x10203));
	qword pDstIndices = pSrcIndices;

	qword pPos = si_from_ptr(positions);

	qword shuf_sldo = {
		0x00,0x01,0x02,0x03,  0x04,0x05,0x06,0x07,
		0x08,0x09,0x0a,0x0b,  0x0c,0x0d,0x0e,0x0f,
	};

	// This loop can trash one quadword past the end if no triangles are culled.
	// Simply save and restore this quadword to prevent any issues.  This can
	// overlap the skinning matrices if they exist, or else the vertex data
	// (Edge has a fixed buffer layout in local store).
	//
	// To safetly handle the very rare case of non-skinned geometry, where no
	// triangles (except maybe the last) are culled, and the last triangle
	// indexes vertex zero, we need to restore the saved quadword every loop,
	// not just at the end of the loop.
	//
	qword endPtr = si_ai(si_a(pSrcIndices, si_from_uint(numIndices*2)), 15);
	qword savedEnd = si_lqd(endPtr, 0);

	qword dstIndices0;
	qword sixByte = si_ilh(0x0606);
	qword maskFC00 = si_fsmbi(0xfc00);
	qword mask3333 = si_fsmbi(0x3333);
	qword shuf0C0C0C0C = si_iohl(si_ilhu(0x8080), 0x0405);
	qword srcIndices0 = si_lqd(pSrcIndices, 0);
	qword srcIndices1 = si_lqd(pSrcIndices, 16);
	qword srcQuad = pSrcIndices;
	for (unsigned i=0; i<numIndices; i+=3)
	{
		// Rotate the source indices for this triangle into the first 6-bytes.
		qword srcIndices = si_shufb(srcIndices0, srcIndices1, shuf_sldo);

		// Because we are overwriting the input index stream, load ahead to
		// ensure we never trash input before it is read.
		qword srcIndices2 = si_lqd(pSrcIndices, 32);

		// The flags for each vertex being outside of a plane are stored in the
		// w component of the position stream.  Triangle is kept if the logical
		// and of the three sets of flags is zero.
		qword posOffset0 = si_rotmi(srcIndices, -12);   // don't care about the least significant 4-bits, since loads are rounded down
		qword posOffset1 = si_shli(si_and(srcIndices, mask3333), 4);
		qword posOffset2 = si_shli(si_shufb(srcIndices, srcIndices, shuf0C0C0C0C), 4);
		qword pos0 = si_lqx(pPos, posOffset0);
		qword pos1 = si_lqx(pPos, posOffset1);
		qword pos2 = si_lqx(pPos, posOffset2);
		qword advance = si_shlqbyi(si_ceqi(si_and(si_and(pos0, pos1), pos2), 0), 12);

		// Load the first destination quadword, as we may need to preserve the first few bytes.
		dstIndices0 = si_lqd(pDstIndices, 0);

		// Move source pointer forward for next loop.
		pSrcIndices = si_ai(pSrcIndices, 6);
		shuf_sldo = si_andbi(si_a(shuf_sldo, sixByte), 31);
		qword nextSrcQuad = si_andi(pSrcIndices, -16);
		qword isNewSrcQuad = si_clgt(nextSrcQuad, srcQuad);
		srcQuad = nextSrcQuad;
		shuf_sldo = si_xor(shuf_sldo, si_andbi(isNewSrcQuad, 16));
		srcIndices0 = si_selb(srcIndices0, srcIndices1, isNewSrcQuad);
		srcIndices1 = si_selb(srcIndices1, srcIndices2, isNewSrcQuad);

		// Rotate the indices into the correct alignment for the destination,
		// then mask and store.
		qword negDstPtr = si_sfi(si_andi(pDstIndices, 15), 0);
		qword dstIndices = si_rotqby(srcIndices, negDstPtr);
		qword dstMask0 = si_rotqmby(maskFC00, negDstPtr);
		dstIndices0 = si_selb(dstIndices0, dstIndices, dstMask0);
		si_stqd(dstIndices0, pDstIndices, 0);
		si_stqd(dstIndices,  pDstIndices, 16);
		pDstIndices = si_a(pDstIndices, si_andi(advance, 6));

		// Restore the potentially trashed quadword incase it is used by the
		// next loop iteration.
		si_stqd(savedEnd, endPtr, 0);
	}

	return (si_to_uint(pDstIndices) - (u32)indices)/sizeof(u16);
}

#endif // EDGE_CULL_CLIPPED_TRIANGLES_USE_SI_INTRINSICS

#if EDGE_NUM_TRIANGLE_CLIP_PLANES > 4

inline void edgeGeomCullClippedTriangles_Extended(EdgeGeomSpuContext* geomCtx, CellSpursEdgeJob* job, spuGcmStateBaseEdge* gcmState, qword& trivialRejectOut, qword& trivialAcceptOut)
{
	const qword* transposedPlanes = (const qword*)gcmState->EdgeClipPlanesTransposed;

	const qword abcd_xxxx = transposedPlanes[0];
	const qword abcd_yyyy = transposedPlanes[1];
	const qword abcd_zzzz = transposedPlanes[2];
	const qword abcd_wwww = transposedPlanes[3];

	const qword efgh_xxxx = transposedPlanes[4];
	const qword efgh_yyyy = transposedPlanes[5];
	const qword efgh_zzzz = transposedPlanes[6];
	const qword efgh_wwww = transposedPlanes[7];

#if EDGE_NUM_TRIANGLE_CLIP_PLANES > 8

	const qword ijkl_xxxx = transposedPlanes[8];
	const qword ijkl_yyyy = transposedPlanes[9];
	const qword ijkl_zzzz = transposedPlanes[10];
	const qword ijkl_wwww = transposedPlanes[11];

#endif // EDGE_NUM_TRIANGLE_CLIP_PLANES > 8

	qword* positions = (qword*)geomCtx->positionTable;

	qword trivialReject = si_il(-1); // reject if bitwise AND is nonzero
	qword trivialAccept = si_il( 0); // accept if bitwise OR is zero

	const qword zero = si_il(0);

	const qword splatx = (qword)(vec_uint4){ 0x00010203, 0x00010203, 0x00010203, 0x00010203 };
	const qword splaty = (qword)(vec_uint4){ 0x04050607, 0x04050607, 0x04050607, 0x04050607 };
	const qword splatz = (qword)(vec_uint4){ 0x08090a0b, 0x08090a0b, 0x08090a0b, 0x08090a0b };
	const qword mergew = (qword)(vec_uint4){ 0x00010203, 0x04050607, 0x08090a0b, 0x10111213 };

	const qword merge16hi = (qword)(vec_uint4){ 0x00010405, 0x08090c0d, 0x10111415, 0x18191c1d };
#if EDGE_NUM_TRIANGLE_CLIP_PLANES > 8
	const qword mergeijkl = (qword)(vec_uint4){ 0x01030507, 0x090b0d0f, 0x13171b1f, 0x80808080 };
#endif // EDGE_NUM_TRIANGLE_CLIP_PLANES > 8

	int numVertexes = geomCtx->spuConfigInfo.numVertexes;
	int i = 0;

	for (const int step = 4; i <= numVertexes - step; i += step)
	{
		const qword pos0 = positions[i + 0];
		const qword pos1 = positions[i + 1];
		const qword pos2 = positions[i + 2];
		const qword pos3 = positions[i + 3];

		const qword xxxx0 = si_shufb(pos0, pos0, splatx);
		const qword yyyy0 = si_shufb(pos0, pos0, splaty);
		const qword zzzz0 = si_shufb(pos0, pos0, splatz);

		const qword xxxx1 = si_shufb(pos1, pos1, splatx);
		const qword yyyy1 = si_shufb(pos1, pos1, splaty);
		const qword zzzz1 = si_shufb(pos1, pos1, splatz);

		const qword xxxx2 = si_shufb(pos2, pos2, splatx);
		const qword yyyy2 = si_shufb(pos2, pos2, splaty);
		const qword zzzz2 = si_shufb(pos2, pos2, splatz);

		const qword xxxx3 = si_shufb(pos3, pos3, splatx);
		const qword yyyy3 = si_shufb(pos3, pos3, splaty);
		const qword zzzz3 = si_shufb(pos3, pos3, splatz);

		const qword abcd0 = si_fma(abcd_xxxx, xxxx0, si_fma(abcd_yyyy, yyyy0, si_fma(abcd_zzzz, zzzz0, abcd_wwww)));
		const qword abcd1 = si_fma(abcd_xxxx, xxxx1, si_fma(abcd_yyyy, yyyy1, si_fma(abcd_zzzz, zzzz1, abcd_wwww)));
		const qword abcd2 = si_fma(abcd_xxxx, xxxx2, si_fma(abcd_yyyy, yyyy2, si_fma(abcd_zzzz, zzzz2, abcd_wwww)));
		const qword abcd3 = si_fma(abcd_xxxx, xxxx3, si_fma(abcd_yyyy, yyyy3, si_fma(abcd_zzzz, zzzz3, abcd_wwww)));

		const qword efgh0 = si_fma(efgh_xxxx, xxxx0, si_fma(efgh_yyyy, yyyy0, si_fma(efgh_zzzz, zzzz0, efgh_wwww)));
		const qword efgh1 = si_fma(efgh_xxxx, xxxx1, si_fma(efgh_yyyy, yyyy1, si_fma(efgh_zzzz, zzzz1, efgh_wwww)));
		const qword efgh2 = si_fma(efgh_xxxx, xxxx2, si_fma(efgh_yyyy, yyyy2, si_fma(efgh_zzzz, zzzz2, efgh_wwww)));
		const qword efgh3 = si_fma(efgh_xxxx, xxxx3, si_fma(efgh_yyyy, yyyy3, si_fma(efgh_zzzz, zzzz3, efgh_wwww)));

		const qword abcdefgh0 = si_shufb(si_fcgt(zero, abcd0), si_fcgt(zero, efgh0), merge16hi);
		const qword abcdefgh1 = si_shufb(si_fcgt(zero, abcd1), si_fcgt(zero, efgh1), merge16hi);
		const qword abcdefgh2 = si_shufb(si_fcgt(zero, abcd2), si_fcgt(zero, efgh2), merge16hi);
		const qword abcdefgh3 = si_shufb(si_fcgt(zero, abcd3), si_fcgt(zero, efgh3), merge16hi);

#if EDGE_NUM_TRIANGLE_CLIP_PLANES > 8

		const qword ijkl0 = si_fma(ijkl_xxxx, xxxx0, si_fma(ijkl_yyyy, yyyy0, si_fma(ijkl_zzzz, zzzz0, ijkl_wwww)));
		const qword ijkl1 = si_fma(ijkl_xxxx, xxxx1, si_fma(ijkl_yyyy, yyyy1, si_fma(ijkl_zzzz, zzzz1, ijkl_wwww)));
		const qword ijkl2 = si_fma(ijkl_xxxx, xxxx2, si_fma(ijkl_yyyy, yyyy2, si_fma(ijkl_zzzz, zzzz2, ijkl_wwww)));
		const qword ijkl3 = si_fma(ijkl_xxxx, xxxx3, si_fma(ijkl_yyyy, yyyy3, si_fma(ijkl_zzzz, zzzz3, ijkl_wwww)));

		const qword flags0 = si_gbb(si_shufb(abcdefgh0, si_fcgt(zero, ijkl0), mergeijkl));
		const qword flags1 = si_gbb(si_shufb(abcdefgh1, si_fcgt(zero, ijkl1), mergeijkl));
		const qword flags2 = si_gbb(si_shufb(abcdefgh2, si_fcgt(zero, ijkl2), mergeijkl));
		const qword flags3 = si_gbb(si_shufb(abcdefgh3, si_fcgt(zero, ijkl3), mergeijkl));

#else

		const qword flags0 = si_gbh(abcdefgh0);
		const qword flags1 = si_gbh(abcdefgh1);
		const qword flags2 = si_gbh(abcdefgh2);
		const qword flags3 = si_gbh(abcdefgh3);

#endif

		positions[i + 0] = si_shufb(pos0, flags0, mergew); // SetW already expects the data in the preferred slot
		positions[i + 1] = si_shufb(pos1, flags1, mergew);
		positions[i + 2] = si_shufb(pos2, flags2, mergew);
		positions[i + 3] = si_shufb(pos3, flags3, mergew);

		trivialReject = si_and(trivialReject, si_and(si_and(flags0, flags1), si_and(flags2, flags3)));
		trivialAccept = si_or (trivialAccept, si_or (si_or (flags0, flags1), si_or (flags2, flags3)));
	}

	for (const int step = 1; i <= numVertexes - step; i += step)
	{
		const qword pos0 = positions[i + 0];

		const qword xxxx0 = si_shufb(pos0, pos0, splatx);
		const qword yyyy0 = si_shufb(pos0, pos0, splaty);
		const qword zzzz0 = si_shufb(pos0, pos0, splatz);

		const qword abcd0 = si_fma(abcd_xxxx, xxxx0, si_fma(abcd_yyyy, yyyy0, si_fma(abcd_zzzz, zzzz0, abcd_wwww)));

		const qword efgh0 = si_fma(efgh_xxxx, xxxx0, si_fma(efgh_yyyy, yyyy0, si_fma(efgh_zzzz, zzzz0, efgh_wwww)));

		const qword abcdefgh0 = si_shufb(si_fcgt(zero, abcd0), si_fcgt(zero, efgh0), merge16hi);

#if EDGE_NUM_TRIANGLE_CLIP_PLANES > 8

		const qword ijkl0 = si_fma(ijkl_xxxx, xxxx0, si_fma(ijkl_yyyy, yyyy0, si_fma(ijkl_zzzz, zzzz0, ijkl_wwww)));

		const qword flags0 = si_gbb(si_shufb(abcdefgh0, si_fcgt(zero, ijkl0), mergeijkl));

#else

		const qword flags0 = si_gbh(abcdefgh0);

#endif

		positions[i + 0] = si_shufb(pos0, flags0, mergew); // SetW already expects the data in the preferred slot

		trivialReject = si_and(trivialReject, flags0);
		trivialAccept = si_or (trivialAccept, flags0);
	}

	trivialRejectOut = trivialReject;
	trivialAcceptOut = trivialAccept;
}

#endif // EDGE_NUM_TRIANGLE_CLIP_PLANES > 4

int edgeGeomCullClippedTriangles(EdgeGeomSpuContext* geomCtx, CellSpursEdgeJob* job, spuGcmStateBaseEdge* gcmState)
{
	qword trivialReject = si_il(-1); // reject if bitwise AND is nonzero
	qword trivialAccept = si_il( 0); // accept if bitwise OR is zero

#if EDGE_NUM_TRIANGLE_CLIP_PLANES > 4
	if (Likely(gcmState->EdgeClipPlaneEnable == 0xff))
	{
		edgeGeomCullClippedTriangles_Extended(geomCtx, job, gcmState, trivialReject, trivialAccept);
	}
	else
#endif // EDGE_NUM_TRIANGLE_CLIP_PLANES > 4
	{
		const qword* transposedPlanes = (const qword*)gcmState->EdgeClipPlanesTransposed;

		const qword abcd_xxxx = transposedPlanes[0];
		const qword abcd_yyyy = transposedPlanes[1];
		const qword abcd_zzzz = transposedPlanes[2];
		const qword abcd_wwww = transposedPlanes[3];

		qword* positions = (qword*)geomCtx->positionTable;

		const qword zero = si_il(0);

		const qword splatx = (qword)(vec_uint4){ 0x00010203, 0x00010203, 0x00010203, 0x00010203 };
		const qword splaty = (qword)(vec_uint4){ 0x04050607, 0x04050607, 0x04050607, 0x04050607 };
		const qword splatz = (qword)(vec_uint4){ 0x08090a0b, 0x08090a0b, 0x08090a0b, 0x08090a0b };
		const qword mergew = (qword)(vec_uint4){ 0x00010203, 0x04050607, 0x08090a0b, 0x10111213 };

		int numVertexes = geomCtx->spuConfigInfo.numVertexes;
		int i = 0;

		for (const int step = 4; i <= numVertexes - step; i += step)
		{
			const qword pos0 = positions[i + 0];
			const qword pos1 = positions[i + 1];
			const qword pos2 = positions[i + 2];
			const qword pos3 = positions[i + 3];

			const qword xxxx0 = si_shufb(pos0, pos0, splatx);
			const qword yyyy0 = si_shufb(pos0, pos0, splaty);
			const qword zzzz0 = si_shufb(pos0, pos0, splatz);

			const qword xxxx1 = si_shufb(pos1, pos1, splatx);
			const qword yyyy1 = si_shufb(pos1, pos1, splaty);
			const qword zzzz1 = si_shufb(pos1, pos1, splatz);

			const qword xxxx2 = si_shufb(pos2, pos2, splatx);
			const qword yyyy2 = si_shufb(pos2, pos2, splaty);
			const qword zzzz2 = si_shufb(pos2, pos2, splatz);

			const qword xxxx3 = si_shufb(pos3, pos3, splatx);
			const qword yyyy3 = si_shufb(pos3, pos3, splaty);
			const qword zzzz3 = si_shufb(pos3, pos3, splatz);

			const qword abcd0 = si_fma(abcd_xxxx, xxxx0, si_fma(abcd_yyyy, yyyy0, si_fma(abcd_zzzz, zzzz0, abcd_wwww)));
			const qword abcd1 = si_fma(abcd_xxxx, xxxx1, si_fma(abcd_yyyy, yyyy1, si_fma(abcd_zzzz, zzzz1, abcd_wwww)));
			const qword abcd2 = si_fma(abcd_xxxx, xxxx2, si_fma(abcd_yyyy, yyyy2, si_fma(abcd_zzzz, zzzz2, abcd_wwww)));
			const qword abcd3 = si_fma(abcd_xxxx, xxxx3, si_fma(abcd_yyyy, yyyy3, si_fma(abcd_zzzz, zzzz3, abcd_wwww)));

			const qword flags0 = si_gb(si_fcgt(zero, abcd0));
			const qword flags1 = si_gb(si_fcgt(zero, abcd1));
			const qword flags2 = si_gb(si_fcgt(zero, abcd2));
			const qword flags3 = si_gb(si_fcgt(zero, abcd3));

			positions[i + 0] = si_shufb(pos0, flags0, mergew); // SetW already expects the data in the preferred slot
			positions[i + 1] = si_shufb(pos1, flags1, mergew);
			positions[i + 2] = si_shufb(pos2, flags2, mergew);
			positions[i + 3] = si_shufb(pos3, flags3, mergew);

			trivialReject = si_and(trivialReject, si_and(si_and(flags0, flags1), si_and(flags2, flags3)));
			trivialAccept = si_or (trivialAccept, si_or (si_or (flags0, flags1), si_or (flags2, flags3)));
		}

		for (const int step = 1; i <= numVertexes - step; i += step)
		{
			const qword pos0 = positions[i + 0];

			const qword xxxx0 = si_shufb(pos0, pos0, splatx);
			const qword yyyy0 = si_shufb(pos0, pos0, splaty);
			const qword zzzz0 = si_shufb(pos0, pos0, splatz);

			const qword abcd0 = si_fma(abcd_xxxx, xxxx0, si_fma(abcd_yyyy, yyyy0, si_fma(abcd_zzzz, zzzz0, abcd_wwww)));

			const qword flags0 = si_gb(si_fcgt(zero, abcd0));

			positions[i + 0] = si_shufb(pos0, flags0, mergew); // SetW already expects the data in the preferred slot

			trivialReject = si_and(trivialReject, flags0);
			trivialAccept = si_or (trivialAccept, flags0);
		}
	}

	if (si_to_int(trivialReject) != 0) { return geomCtx->numVisibleIndexes = 0; }
	if (si_to_int(trivialAccept) == 0) { return geomCtx->numVisibleIndexes; }

	const unsigned numIndices = FilterCulledTriangles((u16*)geomCtx->indexesLs, geomCtx->numVisibleIndexes, (const Vec4V*)geomCtx->positionTable);
	geomCtx->numVisibleIndexes = numIndices;
	return numIndices;
}

#endif // ENABLE_EDGE_CULL_CLIPPED_TRIANGLES

#if !__FINAL
inline void checkBufferEndMarker(vector signed char* bufferEnd, const vector signed char kMarker, const char* bufName)
{
	if (__builtin_expect(si_to_uint(si_gbb(si_ceqb(*bufferEnd,kMarker)))!=0xFFFF, false)) {
		spu_printf("Edge: %s buffer corrupted!\n", bufName); 
		__debugbreak();  
	}
}
#else //__FINAL
inline void checkBufferEndMarker(vector signed char* , const vector signed char , const char*)
{
}
#endif//__FINAL

#define GetJobDmaInputList(x) ((job->DmaList.x.Ea && job->DmaList.x.Size) ? ioBufferList[OffsetOf(CellSpursEdgeJob_DmaList, x) >> 3] : 0)

namespace rage {
// RAGETRACE_ONLY(pfSpuTrace* g_pfSpuTrace = 0;)
} // namespace rage

extern "C" void cellSpursJobMain2(CellSpursJobContext2* jobContext, CellSpursJob256* _job)
{
	CellSpursEdgeJob* job = (CellSpursEdgeJob*)_job;

	static EdgeGeomSpuContext geomCtx __attribute__((aligned(128)));

	// Debug/validation: adds extra markers at the end of our buffers 
	// we can do this because space has been allocated in edge_jobs.cpp	
	const vector signed char kMarker = {0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD};
	vector signed char* sBufferEnd = (vector signed char* ) ((job->Header.sizeScratch<<4) - 16 + (uintptr_t)jobContext->sBuffer);
	vector signed char* ioBufferEnd = (vector signed char* ) (job->Header.sizeInOrInOut - 16 + (uintptr_t)jobContext->ioBuffer);
	*sBufferEnd = kMarker;	
	*ioBufferEnd = kMarker;

#if TASK_SPU_DEBUG_SPEW 
	spu_printf("????????%08x: StartEdge %08x@%i\n", ~spu_readch(SPU_RdDec), (uint32_t)jobContext->eaJobDescriptor, cellSpursGetCurrentSpuId());
	spu_printf("io=%06x-%06x scratch=%06x-%06x\n", 
		(uint32_t)jobContext->ioBuffer, job->Header.sizeInOrInOut +(uint32_t)jobContext->ioBuffer,
		(uint32_t)jobContext->sBuffer, job->Header.sizeScratch * 16 + (uint32_t)jobContext->sBuffer);
#endif // TASK_SPU_DEBUG_SPEW
  
	// get our input DMA list as LS pointers
	// Retrieve IO buffers 
	void* ioBufferList[sizeof(job->DmaList) >> 3] __attribute__((aligned(16)));

	// retrieve elements of the input DMA list
	// Note: We are ignoring secondary stream fixed point offsets since they aren't used in 
	// any of our flavours and we need more space for parameters
	cellSpursJobGetPointerList(ioBufferList, &job->Header, jobContext);
	void* fixedOffsetsA									= GetJobDmaInputList(PrimaryFixedPointOffsets);
	void* fixedOffsetsB									= 0; // GetJobDmaInputList(SecondaryFixedPointOffsets);
	void* indices										= GetJobDmaInputList(Indexes0);
	void* skinMatrices									= GetJobDmaInputList(SkinningMatrices0);
	void* skinIndicesAndWeights							= GetJobDmaInputList(SkinningIndexesAndWeights0);
	void* verticesA										= GetJobDmaInputList(PrimaryVertexes0);
	void* verticesB										= GetJobDmaInputList(SecondaryVertexes0);
	EdgeGeomVertexStreamDescription* rsxOnlyStreamDesc	= (EdgeGeomVertexStreamDescription*)GetJobDmaInputList(RsxOnlyStreamDesc);
	spuGcmStateBaseEdge* gcmState						= &job->SpuGcmState;

	EdgeGeomCustomVertexFormatInfo customFormatInfo;
	memset(&customFormatInfo, 0, sizeof(EdgeGeomCustomVertexFormatInfo));
	customFormatInfo.inputStreamDescA = (EdgeGeomVertexStreamDescription*)GetJobDmaInputList(PrimarySpuInputStreamDesc);
	customFormatInfo.inputStreamDescB = (EdgeGeomVertexStreamDescription*)GetJobDmaInputList(SecondarySpuInputStreamDesc);
	customFormatInfo.outputStreamDesc = (EdgeGeomVertexStreamDescription*)GetJobDmaInputList(SpuOutputStreamDesc);

#if RAGETRACE && 0
	rage::pfSpuTrace trace(gcmState->SpuTrace);
	if( gcmState->SpuTrace )
	{
		rage::g_pfSpuTrace = &trace;
		trace.Push((rage::pfTraceCounterId*)(gcmState->traceId));
	}
#endif

	// Translate culling mode
	u8 edgeCullingMode = gcmState->EdgeViewportCullEnable? gcmState->EdgeCullMode : EDGE_GEOM_CULL_NONE;

	// compute the matrix count from the DMA size (local to this segment, can be zero)
	uint32_t matrixCount = (job->DmaList.SkinningMatrices0.Size + job->DmaList.SkinningMatrices1.Size) / 48;

	g_VertexShaderInputMask = job->VertexShaderInputMask;

	job->SpuConfigInfo.outputVertexFormatId = GetEdgeOutputVertexFormatId(&job->SpuConfigInfo, customFormatInfo.outputStreamDesc, g_VertexShaderInputMask);

	uint32_t allocSize = 0;

#if 0
	// Make sure the position-only optimization is in-place
	Assert(g_VertexShaderInputMask != 1 || verticesB == NULL);
	Assert(g_VertexShaderInputMask != 1 || fixedOffsetsB == NULL);
	//Assert(g_VertexShaderInputMask != 1 || customFormatInfo.inputStreamDescB == NULL);

	edgeGeomOptimizeVertexStreamDescription(customFormatInfo.inputStreamDescA, g_VertexShaderInputMask);
	edgeGeomOptimizeVertexStreamDescription(customFormatInfo.inputStreamDescB, g_VertexShaderInputMask);
	edgeGeomOptimizeVertexStreamDescription(customFormatInfo.outputStreamDesc, g_VertexShaderInputMask);
#endif // 0

#if HACK_GTA4
	// If the whole inputStreamB is being nixxed, it will bail during vertex decompression because it uses
	// verticesB. An assert does not a fix make. 
	if (!customFormatInfo.inputStreamDescB)
	{
		verticesB = NULL;
	}
#endif

	// validate buffer order
	uint32_t errCode = edgeGeomValidateBufferOrder(
		customFormatInfo.outputStreamDesc,
		indices, 
		skinMatrices, 
		skinIndicesAndWeights,  
		verticesA, 
		verticesB, 
		NULL,
		NULL,
		NULL,
		fixedOffsetsA, 
		fixedOffsetsB,
		customFormatInfo.inputStreamDescA,
		customFormatInfo.inputStreamDescB
	);
	if (Unlikely(errCode != 0))
	{
		spu_printf("Buffer order incorrect; errCode = 0x%08X\n", errCode);
		if (errCode & EDGE_GEOM_VALIDATE_ERROR_MASK)
		{
			spu_stop(0);
		}
	}

#ifdef EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK
	EdgeGeomCustomTransformVertexesForCullCallbackInfo transformCallbackInfo;
	transformCallbackInfo.transformCallback = &EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK;
	transformCallbackInfo.transformCallbackUserData = gcmState;
#endif // EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK

	edgeGeomInitialize(
		&geomCtx,
		&job->SpuConfigInfo,
		jobContext->sBuffer,
		job->Header.sizeScratch << 4,
		jobContext->ioBuffer,
		job->Header.sizeInOrInOut,
		jobContext->dmaTag,
		&gcmState->EdgeInfo,
		&gcmState->EdgeWorld,
		&customFormatInfo,
#ifdef EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK
		&transformCallbackInfo,
#else
		NULL,
#endif // EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK
		(u32)gcmState->Control
	);

	// these have to come before the "goto" or the compiler will complain
	uint32_t numVisibleIdxs = 0;
	const vec_float4 v_offset = (vec_float4){ job->Offset[0], job->Offset[1], job->Offset[2], 0.0f };
	DRAWABLESPU_STATS_ONLY(uint32_t waitTicks = 0);

	EdgeGeomCullingResults* cullingResults = NULL;
#if ENABLE_EDGE_CULL_DEBUGGING || defined(EDGE_GEOM_DEBUG)
	EdgeGeomCullingResults _cullingResultsScratch;
	cullingResults = &_cullingResultsScratch;
	cullingResults->numOccludedTriangles       = 0;
	cullingResults->numOutsideFrustumTriangles = 0;
	cullingResults->numOneDimensionalTriangles = 0;
	cullingResults->numZeroAreaTriangles       = 0;
	cullingResults->numNoPixelTriangles        = 0;
	cullingResults->numBackFacingTriangles     = 0;
	cullingResults->totalNumCulledTriangles    = 0;
#endif // ENABLE_EDGE_CULL_DEBUGGING || defined(EDGE_GEOM_DEBUG)

	geomCtx.debugFlags = gcmState->EdgeCullDebugFlags;

#if DISABLE_EDGE_GEOMETRY
	{
		// Early out (patch jumps to self / draw nothing)
		// we must still set the free pointer to somewhere in the inout buffer as it's used
		// for the vram sync DMA.
		edgeGeomSetFreePtr(&geomCtx, ioBufferList[0]);
		CellGcmContextData gcmCtx;
		edgeGeomBeginCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0
										#if HACK_GTA4
											,gcmState
										#endif
										);
		edgeGeomEndCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0);
		goto JobEnd;
	}
#endif // DISABLE_EDGE_GEOMETRY

	// decompress and skin (if necessary) 
	edgeGeomDecompressVertexes(&geomCtx, verticesA, fixedOffsetsA, verticesB, fixedOffsetsB);

	// apply offset (if any)
	if (Likely(vec_any_ne(v_offset, spu_splats(0.0f))))
	{
		vec_float4* __restrict__ v_positions = (vec_float4*)geomCtx.positionTable;
		for (int i = 0; i < geomCtx.spuConfigInfo.numVertexes; ++i)
		{
			v_positions[i] = vec_add(v_positions[i], v_offset);
		}
	}

	// from edgegeomspu_callbacks.cpp
#ifdef EDGE_GEOM_PRETRANSFORM_CALLBACK
	EDGE_GEOM_PRETRANSFORM_CALLBACK(jobContext, job, &geomCtx, *gcmState);
#endif // EDGE_GEOM_PRETRANSFORM_CALLBACK

#if ENABLE_EDGE_CULL_DEBUGGING || defined(EDGE_GEOM_DEBUG)
	if ((geomCtx.debugFlags & EDGE_BLENDSHAPE) != 0 || (geomCtx.debugFlags & EDGE_CULL_DEBUG_ENABLED) == 0)
#endif // ENABLE_EDGE_CULL_DEBUGGING || defined(EDGE_GEOM_DEBUG)
	{
 		if (job->NumBlendShapes && job->BlendShapesEa)
		{
 			//spu_printf("draw=%d, num=%d, ea=%x\n", gcmState->BlendShapeDrawBuffer, job->NumBlendShapes, job->BlendShapesEa);
 			//si_stopd(si_from_uint(0), si_from_uint(1), si_from_uint(2));
 			edgeGeomProcessBlendShapes(&geomCtx, gcmState->BlendShapeDrawBuffer, job->NumBlendShapes, job->BlendShapesEa + 16/*sizeof(grmGeometryEdgeBlendHeader)*/, g_VertexShaderInputMask);
 		}
	}

#if ENABLE_EDGE_CULL_DEBUGGING || defined(EDGE_GEOM_DEBUG)
	if ((geomCtx.debugFlags & EDGE_SKIN) != 0 || (geomCtx.debugFlags & EDGE_CULL_DEBUG_ENABLED) == 0)
#endif // ENABLE_EDGE_CULL_DEBUGGING || defined(EDGE_GEOM_DEBUG)
	{
		edgeGeomSkinVertexes(&geomCtx, skinMatrices, matrixCount, skinIndicesAndWeights);
	}
#ifdef EDGE_GEOM_POSTSKINING_CALLBACK
	EDGE_GEOM_POSTSKINING_CALLBACK(jobContext, job, &geomCtx, *gcmState, matrixCount, skinIndicesAndWeights);
#endif // EDGE_GEOM_PRETRANSFORM_CALLBACK

	edgeGeomDecompressIndexes(&geomCtx, indices);

	checkBufferEndMarker(sBufferEnd, kMarker, "scratch (decompress)");
	checkBufferEndMarker(ioBufferEnd, kMarker, "io (decompress)");

#if ENABLE_EDGE_CULL_CLIPPED_TRIANGLES
	DRAWABLESPU_STATS_ONLY(uint32_t numCullClippedIndices = 0);
#endif // ENABLE_EDGE_CULL_CLIPPED_TRIANGLES

#if ENABLE_EDGE_OCCLUSION
	if (gcmState->EdgeOccluderQuads && gcmState->EdgeOccluderQuadCount > 0)
	{
		numVisibleIdxs = edgeGeomCullOccludedTriangles(&geomCtx, gcmState->EdgeOccluderQuadCount, (u32)gcmState->EdgeOccluderQuads);
		if (numVisibleIdxs == 0)
		{
			// early out since all triangles were occluded
			CellGcmContextData gcmCtx;
			edgeGeomBeginCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0
											#if HACK_GTA4
												,gcmState
											#endif
											);
			edgeGeomEndCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0);
			goto JobEnd;
		}
	}
#endif // ENABLE_EDGE_OCCLUSION

#if ENABLE_EDGE_CULL_CLIPPED_TRIANGLES
	if (Unlikely(gcmState->ModelCullStatus != 0))
	{
		DRAWABLESPU_STATS_ONLY(numCullClippedIndices = geomCtx.numVisibleIndexes);
		if (edgeGeomCullClippedTriangles(&geomCtx,job, gcmState) == 0)
		{
			// early out since all triangles were trimmed
			CellGcmContextData gcmCtx;
			edgeGeomBeginCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0
											#if HACK_GTA4
												,gcmState
											#endif
											);
			edgeGeomEndCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0);
			goto JobEnd;
		}
	}
#endif // ENABLE_EDGE_CULL_CLIPPED_TRIANGLES

	// culling
	numVisibleIdxs = edgeGeomCullTriangles(&geomCtx, edgeCullingMode, 0, cullingResults);
	if (numVisibleIdxs == 0)
	{
		// early out since all triangles were trimmed
		CellGcmContextData gcmCtx;
		edgeGeomBeginCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0
										#if HACK_GTA4
											,gcmState
										#endif
										);
		edgeGeomEndCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0);
		goto JobEnd;
	}

	// allocate output space for vertices and indices (can block)
	EdgeGeomAllocationInfo info;
	{
		allocSize = edgeGeomCalculateDefaultOutputSize(&geomCtx, numVisibleIdxs);
		DRAWABLESPU_STATS_ONLY(uint32_t beginWait = spu_read_decrementer());
		if(!edgeGeomAllocateOutputSpace(&geomCtx, job->OutputBufferInfoEa, allocSize, &info, cellSpursGetCurrentSpuId())) {
			// no space, shouldn't happen for ring buffers, but early out anyhow (i.e. draw nothing)
			CellGcmContextData gcmCtx;
			edgeGeomBeginCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0
											#if HACK_GTA4
												,gcmState
											#endif
											);
			edgeGeomEndCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, 0, 0);
			goto JobEnd;
		}
		DRAWABLESPU_STATS_ONLY(waitTicks = beginWait - spu_read_decrementer());
	}

	// output index data
	EdgeGeomLocation idx;
	edgeGeomOutputIndexes(&geomCtx, numVisibleIdxs, &info, &idx);

#if HACK_GTA4
	#ifdef EDGE_GEOM_PRECOMPRESS_CALLBACK
		EDGE_GEOM_PRECOMPRESS_CALLBACK(jobContext, job, &geomCtx, *gcmState);
	#endif
#endif
	EdgeGeomLocation vtx;
	// compress and output vertex data
	edgeGeomCompressVertexes(&geomCtx);
	edgeGeomOutputVertexes(&geomCtx, &info, &vtx);

	// build our pushbuffer and DMA it out (edge will sync the vertex/index data internally)
	// NOTE: update spuConfigInfo.holeSize if placing additional pushbuffer commands
	// This needs to be done here as well as the Edge job management code which resides in
	// grcore/edge_jobs.cpp
	CellGcmContextData gcmCtx;
	edgeGeomBeginCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, &info, 1
									#if HACK_GTA4
										,gcmState
									#endif
									);

	rageSetVertexDataArrays(&geomCtx, &gcmCtx, &vtx);

	// Bind RSX only stream and disable unused vertex attributes.
	// rageSetVertexDataArrays will not generate a write to any channel that is already disabled in the
	// vertex outputs.  The stall hole always has room for exactly 16 vertex attributes, and we
	// hit each attribute exactly once (either enabling it and assigning it to either ringbuffer output
	// space or static RSX memory, or disabling it because it's unused).  However, the rageSetVertexDataArrays
	// call above is somewhat redundant with the code below (both end up parsing outputVertexFormatId)
	// so we could improve things a bit by switching things over to the way BindVertexFormat works,
	// or at least processing each attribute only one time.  But this code isn't really a bottleneck at present.
	// If we did switch over to BindVertexFormat, though, we could reduce the command buffer hole size by 30 
	// words across the board.

#if HACK_GTA4
	if((g_VertexShaderInputMask!=0x00000001) || !spuGcmShadowWarpEnabled(gcmState->shadowType))
#endif
	{
		if (rsxOnlyStreamDesc)
		{
			for (u32 i = 0; i < rsxOnlyStreamDesc->numAttributes; ++i)
			{
				const EdgeGeomAttributeBlock& attr = rsxOnlyStreamDesc->blocks[i].attributeBlock;
				if (g_VertexShaderInputMask & (1 << attr.vertexProgramSlotIndex))
				{
					cellGcmSetVertexDataArrayOffsetUnsafeInline(&gcmCtx, attr.vertexProgramSlotIndex,
						CELL_GCM_LOCATION_LOCAL, job->RsxOnlyVertexOffset + attr.offset);
				}
			}
		}
	}

	cellGcmSetDrawIndexArrayInline(
		&gcmCtx, 
		CELL_GCM_PRIMITIVE_TRIANGLES,
		numVisibleIdxs, 
		CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, 
		idx.location, 
		idx.offset);  

	edgeGeomEndCommandBufferHole(&geomCtx, &gcmCtx, job->CommandBufferHoleEa, &info, 1);

JobEnd:
	// Exit point (can come from early out)
	// This function currently (0.3.2) does nothing except when profiling is enabled but you 
	// should always call it for future implementations
	edgeGeomFinalize(&geomCtx);

#if DRAWABLESPU_STATS
	int32_t result;
	spuDrawableStats stats;
	uint64_t eaStats = (uint64_t) gcmState->statsBuffer;
	if (eaStats) do 
	{
		// Atomic-get the shared buffer info
		EDGE_DMA_GETLLAR(&stats, eaStats, 0, 0);
		result = EDGE_DMA_WAIT_ATOMIC_STATUS();

		stats.TotalIndices += geomCtx.spuConfigInfo.numIndexes;
#if ENABLE_EDGE_CULL_CLIPPED_TRIANGLES
		stats.CullClippedIndices += numCullClippedIndices;
#endif // ENABLE_EDGE_CULL_CLIPPED_TRIANGLES
		stats.VisibleIndices += numVisibleIdxs;
		stats.EdgeJobs++;
		if (numVisibleIdxs == 0)
			stats.TrivialRejectEdgeJobs++;
		stats.EdgeOutputWaitTicks[cellSpursGetCurrentSpuId()] += waitTicks;
		stats.EdgeOutputSpaceUsed += allocSize;

		// Write the shared buffer info back to main memory, making
		// sure nothing it hasn't been changed since it was read.
		EDGE_DMA_PUTLLC(&stats, eaStats, 0, 0);
		result = EDGE_DMA_WAIT_ATOMIC_STATUS();
	} while (result & 1);
#endif // DRAWABLESPU_STATS

#if RAGETRACE && 0
	if (gcmState->SpuTrace)
	{
		trace.Pop();
		trace.Finish(jobContext->dmaTag);
	}
#endif
	
#if TASK_SPU_DEBUG_SPEW 
	spu_printf("????????%08x: EndEdge %08x@%i\n", ~spu_readch(SPU_RdDec), (uint32_t)jobContext->eaJobDescriptor, cellSpursGetCurrentSpuId());
#endif//TASK_SPU_DEBUG_SPEW
	 
	checkBufferEndMarker(sBufferEnd, kMarker, "scratch (jobEnd)");
	checkBufferEndMarker(ioBufferEnd, kMarker, "io (jobEnd)");

	// SPURS job will finish the outstanding pushbuffer DMA (from the ioBuffer) 
	// this is the reason why ioBuffer is declared as an input AND output buffer 
} 
	
#endif		// __SPU
