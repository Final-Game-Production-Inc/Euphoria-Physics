//
// phbound/boundpolyhedron.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "boundpolyhedron.h"

#include "support.h"

#include "data/resource.h"
#include "data/struct.h"
#include "math/simplemath.h"
#include "phbullet/CollisionMargin.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "system/alloca.h"
#include "system/memory.h"
#include "system/timemgr.h"						// We shouldn't really need this and it's only in use in one place.
#include "vector/geometry.h"

#if !__FINAL && !__TOOL && !__RESOURCECOMPILER
#include "fragment/cacheheap.h"
#include "fragment/manager.h"
#endif

#if __PFDRAW
#include "boundbvh.h"
#include "boundculler.h"
#endif // __PFDRAW

#if __NMDRAW
#include "nmext/NMRenderBuffer.h"
#endif

#if !__FINAL && !__TOOL && !__RESOURCECOMPILER
// Hack to work around not linking the fragment cache allocator into the physics samples.
void ValidateFragCachePointer(const void * ptr, const char * ptrName);
#endif

namespace rage {

#if !__SPU

//////////////////////////////////////////////////////////////
// bound profiler variables
//EXT_PFD_DECLARE_GROUP(BoundIndices);
EXT_PFD_DECLARE_ITEM(BvhVertexIndices);
EXT_PFD_DECLARE_ITEM(NonBvhVertexIndices);
EXT_PFD_DECLARE_ITEM(BvhPolygonIndices);
EXT_PFD_DECLARE_ITEM(NonBvhPolygonIndices);
EXT_PFD_DECLARE_ITEM(ComponentIndices);
EXT_PFD_DECLARE_ITEM(Quads);
EXT_PFD_DECLARE_ITEM(FlattenQuads);
EXT_PFD_DECLARE_ITEM(PolyNeighbors);
EXT_PFD_DECLARE_ITEM_SLIDER(PolyNeighborsLength);
EXT_PFD_DECLARE_ITEM_SLIDER(PolyNeighborsDensity);
EXT_PFD_DECLARE_ITEM(DrawBoundMaterialNames);
EXT_PFD_DECLARE_ITEM(SolidBoundLighting);
EXT_PFD_DECLARE_ITEM(SolidBoundRandom);
EXT_PFD_DECLARE_ITEM(MarginOriginalPolygons);
EXT_PFD_DECLARE_ITEM(MarginOriginalPolygonsSolid);
EXT_PFD_DECLARE_ITEM(MarginShrunkPolygons);
EXT_PFD_DECLARE_ITEM(MarginExpandedShrunkPolygons);
EXT_PFD_DECLARE_ITEM(MarginExpandedShrunkRoundEdges);
EXT_PFD_DECLARE_ITEM(MarginExpandedShrunkRoundCorners);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDistanceOpacity);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundSolidOpacity);
EXT_PFD_DECLARE_ITEM_SLIDER(ThinPolyTolerance);
EXT_PFD_DECLARE_ITEM_SLIDER(BigPrimTolerance);
EXT_PFD_DECLARE_GROUP(PolygonDensity);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);
EXT_PFD_DECLARE_ITEM_SLIDER_INT_FULL(NumRecursions);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(RecusiveWeightingStyle);
EXT_PFD_DECLARE_ITEM_SLIDER(ColorMidPoint);
EXT_PFD_DECLARE_ITEM_SLIDER_FULL(ColorExpBias);
EXT_PFD_DECLARE_ITEM_SLIDER(SmallPolygonArea);
EXT_PFD_DECLARE_ITEM_SLIDER(MediumPolygonArea);
EXT_PFD_DECLARE_ITEM_SLIDER(LargePolygonArea);
EXT_PFD_DECLARE_ITEM(PolygonAngleDensity);
EXT_PFD_DECLARE_ITEM(EdgeAngles);
EXT_PFD_DECLARE_ITEM(MaxNeighborAngleEnabled);
EXT_PFD_DECLARE_ITEM_SLIDER_FULL(MaxNeighborAngle);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);
EXT_PFD_DECLARE_ITEM(IncludePolygons);
EXT_PFD_DECLARE_ITEM(SphereCull);

namespace phBoundStats
{
	EXT_PF_TIMER(Poly_Sphere);
	EXT_PF_TIMER(Poly_Capsule);
	EXT_PF_TIMER(Poly_Box);
	EXT_PF_TIMER(Poly_Poly);
};

using namespace phBoundStats;

#define CHECK_COLLISION_VERTS_IN_RSC_CONSTRUCTOR	0


//////////////////////////////////////////////////////////////

#if POLYGON_INDEX_IS_U32
const unsigned int phBoundPolyhedron::INVALID_VERTEX    = 0xFFFFFFFF;
const unsigned int phBoundPolyhedron::INVALID_POLYGON   = 0xFFFFFFFF;
#else // POLYGON_INDEX==u32
const unsigned int phBoundPolyhedron::INVALID_VERTEX    = 0xFFFF;
const unsigned int phBoundPolyhedron::INVALID_POLYGON   = 0xFFFF;
#endif // POLYGON_INDEX==u32
const unsigned int phBoundPolyhedron::MAX_NUM_MATERIALS = 0xFFFE;

bool phBoundPolyhedron::sm_ComputeNeighbors=true;

#if __PFDRAW
float phBoundPolyhedron::sm_TypeFlagDensityScaling[32] = { 0 };
float phBoundPolyhedron::sm_IncludeFlagDensityScaling[32] = { 0 };
#endif // __PFDRAW

////////////////////////////////////////////////////////////////////////////////

#endif	// !__SPU

#if OCTANT_MAP_SUPPORT_ACCEL
bool phBoundPolyhedron::sm_UseOctantMap = true;
#endif

phBoundPolyhedron::phBoundPolyhedron ()
#if COMPRESSED_VERTEX_METHOD == 0
	: m_Vertices(NULL)
	, m_ShrunkVertices(NULL)
#else
	: m_VerticesPad(0)
	, m_CompressedShrunkVertices(NULL)
#endif
	, m_Polygons(NULL)
#if COMPRESSED_VERTEX_METHOD > 0
	, m_CompressedVertices(NULL)
#endif
	, m_NumPerVertexAttribs(0)
	, m_VertexAttribs(NULL)
	, m_UseActiveComponents(false)
	, m_IsFlat(false)
	, m_NumConvexHullVertices(0)
#if OCTANT_MAP_SUPPORT_ACCEL
	, m_OctantVertCounts(NULL)
	, m_OctantVerts(NULL)
#endif // OCTANT_MAP_SUPPORT_ACCEL
{
#if OCTANT_MAP_SUPPORT_ACCEL
	sysMemZeroWords<3+3>(m_Pad0);
#else
	sysMemZeroWords<1+3>(m_Pad0);
#endif // OCTANT_MAP_SUPPORT_ACCEL

#if USE_OCTANT_MAP_INDEX_U16
	Assert(sizeof(OctantMapIndexType) == 2);
	m_Flags |= OCTANT_MAP_INDEX_IS_U16;
#endif // USE_OCTANT_MAP_INDEX_U16
}

#if !__SPU
phBoundPolyhedron::phBoundPolyhedron (datResource & rsc)
	: phBound(rsc)
	, m_IsFlat(false)
{
#if COMPRESSED_VERTEX_METHOD == 0
	rsc.PointerFixup(m_Vertices);
#endif
	rsc.PointerFixup(m_Polygons);
#if COMPRESSED_VERTEX_METHOD == 0
    rsc.PointerFixup(m_ShrunkVertices);
#else
	rsc.PointerFixup(m_CompressedShrunkVertices);
#endif
#if COMPRESSED_VERTEX_METHOD > 0
	rsc.PointerFixup(m_CompressedVertices);
#endif
	rsc.PointerFixup(m_VertexAttribs);

#if OCTANT_MAP_SUPPORT_ACCEL
	if (m_OctantVertCounts)
	{
		rsc.PointerFixup(m_OctantVertCounts);
		rsc.PointerFixup(m_OctantVerts);

		for (int octant = 0; octant < 8; ++octant)
		{
#if __ASSERT
			const OctantMapIndexType vertCount = GetOctantVertCount(octant);
			Assert(vertCount > 0 && vertCount <= (OctantMapIndexType)m_NumConvexHullVertices);
#endif // __ASSERT
			rsc.PointerFixup(m_OctantVerts[octant]);
		}
	}
#endif // OCTANT_MAP_SUPPORT_ACCEL

#if __ASSERT && CHECK_COLLISION_VERTS_IN_RSC_CONSTRUCTOR
	if (GetType() == phBound::GEOMETRY)
	{
		Assert(GetMargin() >= CONVEX_MINIMUM_MARGIN);

		// Figure out which set of vertices is going to be used for collision and then check to make sure that those vertices, when expanded by the
		//   margin, don't stick outside of the bounding box.
		const int numVertices = GetNumVertices();
		const Vec3V marginAsVec3V = Vec3V(GetMarginV());
#if COMPRESSED_VERTEX_METHOD == 0
		const Vec3V boxReduction = marginAsVec3V;
#else	// COMPRESSED_VERTEX_METHOD == 0
		const Vec3V boxReduction = Subtract(marginAsVec3V, m_UnQuantizeFactor);
#endif	// COMPRESSED_VERTEX_METHOD == 0
		const Vec3V reducedBoundingBoxMin = Add(m_BoundingBoxMin, boxReduction);
		const Vec3V reducedBoundingBoxMax = Subtract(m_BoundingBoxMax, boxReduction);
		if(GetShrunkVertexPointer() != NULL)
		{
			for(int vertIndex = 0; vertIndex < numVertices; ++vertIndex)
			{
				const Vec3V shrunkVert = GetShrunkVertex(vertIndex);
				Assert(IsLessThanOrEqualAll(shrunkVert, reducedBoundingBoxMax));
				Assert(IsGreaterThanOrEqualAll(shrunkVert, reducedBoundingBoxMin));
			}
		}
		else
		{
			for(int vertIndex = 0; vertIndex < numVertices; ++vertIndex)
			{
				const Vec3V vert = GetVertex(vertIndex);
				Assert(IsLessThanOrEqualAll(vert, reducedBoundingBoxMax));
				Assert(IsGreaterThanOrEqualAll(vert, reducedBoundingBoxMin));
			}
		}
	}
#endif	// __ASSERT && CHECK_COLLISION_VERTS_IN_RSC_CONSTRUCTOR
}

#if OCTANT_MAP_SUPPORT_ACCEL

template<class OFFSET_TYPE> __forceinline OFFSET_TYPE Align(const OFFSET_TYPE offset, const size_t alignment)
{
	FastAssert(((alignment-1)&alignment) == 0 && alignment > 0);	// Alignment must be a power of 2.
	//const size_t aligned_val = ((size_t(val) - 1) & ~(size_t(alignment) - 1)) + alignment;
	const size_t aligned_val = (size_t(offset) + size_t(alignment - 1)) & ~(size_t(alignment) - 1);
	return (OFFSET_TYPE)aligned_val;
}

__forceinline int Max(const int s1, const int s2)
{
	return s1 >= s2 ? s1 : s2;
}

void phBoundPolyhedron::OctantMapAllocateAndCopy(OctantMapIndexType * octantVertCounts, OctantMapIndexType ** octantVerts)
{
	// This function allocates the memory for the octant map and copies everthing.

#if USE_OCTANT_MAP_SINGLE_ALLOCATION
	// Use one allocation for everything.

	// Assume alignment are powers of 2, else need to compute the lcm.
	const size_t alignOfVertCount = __alignof(OctantMapIndexType);
	const size_t alignOfVertList = __alignof(OctantMapIndexType*);
	const size_t alignOfVertIndex = __alignof(OctantMapIndexType);
	const size_t sizeOfVertCount = sizeof(OctantMapIndexType);
	const size_t sizeOfVertList = sizeof(OctantMapIndexType*);
	const size_t sizeOfVertIndex = sizeof(OctantMapIndexType);

	const size_t totalVertsNeeded = octantVertCounts[0] + octantVertCounts[1] + octantVertCounts[2] + octantVertCounts[3] + 
									octantVertCounts[4] + octantVertCounts[5] + octantVertCounts[6] + octantVertCounts[7];

#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
	// Allocate the maximum possible size for the index lists.
	const size_t vertIndexListSize = 4 * m_NumVertices;
	Assert(totalVertsNeeded <= vertIndexListSize);
#else // USE_OCTANT_MAP_PERMANENT_ALLOCATION
	const size_t vertIndexListSize = totalVertsNeeded;
#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION

	// The first part of the allocation is for the vert counts.
	// The second part is for the vert lists.
	// The third part is for the indices.

	const size_t vertCountOffset = 0;
	const size_t vertListOffset = Align(vertCountOffset + sizeOfVertCount * 8, alignOfVertList);
	const size_t vertIndexOffset = Align(vertListOffset + sizeOfVertList * 8, alignOfVertIndex);

//#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
	const size_t bufferSize = vertIndexOffset + sizeOfVertIndex * vertIndexListSize + alignOfVertIndex * 8;	// Add some extra for alignment.
//#else // USE_OCTANT_MAP_PERMANENT_ALLOCATION
//	size_t bufferSize = vertIndexOffset;
//	for (int octant = 0 ; octant < 8 ; octant++)
//	{
//		const OctantMapIndexType vertCount = octantVertCounts[octant];
//		Assert(vertCount > 0 && vertCount < (OctantMapIndexType)m_NumVertices);
//		bufferSize = Align(bufferSize + sizeOfVertIndex * vertCount,alignOfVertIndex);
//	}
//#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION
	const size_t bufferAlignment = Max(Max(alignOfVertCount,alignOfVertList),alignOfVertIndex);		// Assume alignments are powers of 2, else need to compute the lcm.

	u8 * octantMapBuffer;
#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
	if (m_OctantVertCounts)
	{
		// The buffer is already allocated.
		octantMapBuffer = (u8*)m_OctantVertCounts;
	}
	else
#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION
	{
		Assert(m_OctantVertCounts == NULL);
		octantMapBuffer = rage_aligned_new(bufferAlignment) u8[bufferSize];
	}

	m_OctantVertCounts = (OctantMapIndexType*)(octantMapBuffer + vertCountOffset);

#if !__FINAL && !__TOOL && !__RESOURCECOMPILER
	// It's critical that this allocation comes out of the frag cache heap!
	ValidateFragCachePointer(m_OctantVertCounts,"m_OctantVertCounts");
#endif

	m_OctantVerts = (OctantMapIndexType**)(octantMapBuffer + vertListOffset);
	OctantMapIndexType * octacntVertIndices = (OctantMapIndexType*)(octantMapBuffer + vertIndexOffset);

	// Copy the counts and indices. Fix up the index list pointers.
	//sysMemCpy(octacntVertIndices,totalVertsList,sizeOfVertIndex*totalVertsNeeded);
	for (int octant = 0 ; octant < 8 ; octant++)
	{
		const OctantMapIndexType vertCount = octantVertCounts[octant];
		Assert(vertCount > 0 && vertCount <= (OctantMapIndexType)m_NumVertices);
		m_OctantVertCounts[octant] = vertCount;
		m_OctantVerts[octant] = octacntVertIndices;
		octacntVertIndices = Align(octacntVertIndices+vertCount,alignOfVertIndex);
		Assert((u8*)octacntVertIndices <= octantMapBuffer + bufferSize);
		sysMemCpy(m_OctantVerts[octant], octantVerts[octant], sizeof(OctantMapIndexType) * vertCount);
	}

#else // USE_OCTANT_MAP_SINGLE_ALLOCATION

	// Allocate the vert counts.
	m_OctantVertCounts = rage_new OctantMapIndexType[8];

#if !__FINAL && !__TOOL && !__RESOURCECOMPILER
	// It's critical that this allocation comes out of the frag cache heap!
	ValidateFragCachePointer(m_OctantVertCounts,"m_OctantVertCounts");
#endif

	// Allocate the vert lists.
	m_OctantVerts = rage_new OctantMapIndexType*[8];
	for (int octant = 0; octant < 8; ++octant)
	{
		// Allocate the indices. Copy the counts and indices.
		const OctantMapIndexType vertCount = octantVertCounts[octant];
		Assert(vertCount > 0 && vertCount <= (OctantMapIndexType)m_NumVertices);
		m_OctantVertCounts[octant] = vertCount;
		m_OctantVerts[octant] = rage_new OctantMapIndexType[vertCount];
		sysMemCpy(m_OctantVerts[octant], octantVerts[octant], sizeof(OctantMapIndexType) * vertCount);
	}

#endif // USE_OCTANT_MAP_SINGLE_ALLOCATION
}

void phBoundPolyhedron::OctantMapDelete()
{
	if (m_OctantVertCounts)
	{
		Assert(m_OctantVerts);
#if USE_OCTANT_MAP_SINGLE_ALLOCATION
		delete [] ((u8*)m_OctantVertCounts);
#else // USE_OCTANT_MAP_SINGLE_ALLOCATION
		for (int octant = 0 ; octant < 8 ; octant++)
		{
			Assert(m_OctantVerts[octant]);
			delete [] m_OctantVerts[octant];
		}
		delete [] m_OctantVerts;
		delete [] m_OctantVertCounts;
#endif // USE_OCTANT_MAP_SINGLE_ALLOCATION
		m_OctantVertCounts = NULL;
		m_OctantVerts = NULL;
	}
}
#endif // OCTANT_MAP_SUPPORT_ACCEL

#define STRUCT_DYNAMIC_ARRAY_STACK_COUNT(field,ct)		\
	do { if (field) for (int i=0; i<(int)(ct); i++) ::rage::datSwapper(field[i]); \
			s.AddField((size_t)&field-(size_t)this,sizeof(field),#field); \
			::rage::datSwapper((void*&)field); } while (0)

#if __DECLARESTRUCT
void phBoundPolyhedron::DeclareStruct(datTypeStruct &s)
{
#if __RESOURCECOMPILER
	const size_t maxPrimitives = (128 * g_rscVirtualLeafSize) / sizeof(phPolygon);
	if((u32)m_NumPolygons > maxPrimitives)
	{
		Warningf("Bound has %d primitives.  Bounds with more than %d will not resource successfully.  Please reduce the number of primitives.", m_NumPolygons, maxPrimitives);
	}
#endif	// __RESOURCECOMPILER

	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundPolyhedron);
#if COMPRESSED_VERTEX_METHOD == 0
	STRUCT_DYNAMIC_ARRAY(m_Vertices,m_NumVertices);
	STRUCT_DYNAMIC_ARRAY(m_ShrunkVertices,m_NumVertices);
#else
	STRUCT_FIELD(m_VerticesPad);
	STRUCT_DYNAMIC_ARRAY_MULT(m_CompressedShrunkVertices,m_NumVertices,3);
#endif

	STRUCT_FIELD(m_UseActiveComponents);
	STRUCT_FIELD(m_IsFlat);
	STRUCT_FIELD(m_NumPerVertexAttribs);
	STRUCT_FIELD(m_NumConvexHullVertices);

	// It's not really necessary to have this branch here as the second branch actually handles both cases fine.
	if(GetType() != phBound::BVH)
	{
		STRUCT_DYNAMIC_ARRAY(m_Polygons,m_NumPolygons);
	}
	else
	{
		// We're a BVH bound.  The m_Polygons array could contain primitives other than just polygons so we need to ensure that each of those
		//   gets DeclareStruct'd correctly.
		const int numPolygons = m_NumPolygons;
		for(int polyIndex = 0; polyIndex < numPolygons; ++polyIndex)
		{
			const phPrimitive &curPrim = m_Polygons[polyIndex].GetPrimitive();
			switch(curPrim.GetType())
			{
			case PRIM_TYPE_POLYGON:
				{
					::rage::datSwapper(curPrim.GetPolygon());
					break;
				}
			case PRIM_TYPE_SPHERE:
				{
					::rage::datSwapper(curPrim.GetSphere());
					break;
				}
			case PRIM_TYPE_CAPSULE:
				{
					::rage::datSwapper(curPrim.GetCapsule());
					break;
				}
			case PRIM_TYPE_BOX:
				{
					::rage::datSwapper(curPrim.GetBox());
					break;
				}
			default:
				{
					Assert(curPrim.GetType() == PRIM_TYPE_CYLINDER);
					::rage::datSwapper(curPrim.GetCylinder());
					break;
				}
			}
		}
		s.AddField((size_t)&(m_Polygons)-(size_t)this,sizeof(m_Polygons),"m_Polygons");
		::rage::datSwapper((void*&)m_Polygons);
	}

#if COMPRESSED_VERTEX_METHOD == 0
	STRUCT_CONTAINED_ARRAY(m_MorePad);
#elif COMPRESSED_VERTEX_METHOD == 1
	STRUCT_DYNAMIC_ARRAY_MULT(m_CompressedVertices,m_NumVertices,3);
#elif COMPRESSED_VERTEX_METHOD == 2
	STRUCT_FIELD(m_UnQuantizeFactor);
	STRUCT_FIELD(m_BoundingBoxCenter);
	STRUCT_DYNAMIC_ARRAY_MULT(m_CompressedVertices,m_NumVertices,3);
#else
	Assert(false);
#endif

	STRUCT_DYNAMIC_ARRAY_MULT(m_VertexAttribs,m_NumVertices,m_NumPerVertexAttribs);

#if OCTANT_MAP_SUPPORT_ACCEL
	if (m_OctantVerts)
	{
		for (int octant = 0; octant < 8; ++octant)
		{
			const OctantMapIndexType vertCount = m_OctantVertCounts[octant];
			Assert(vertCount > 0 && vertCount <= (OctantMapIndexType)m_NumVertices);
			for (OctantMapIndexType i = 0; i < vertCount; ++i)
			{
				::rage::datSwapper(m_OctantVerts[octant][i]);
			}
		}
	}

	STRUCT_DYNAMIC_ARRAY_NOCOUNT(m_OctantVertCounts, 8);
	STRUCT_DYNAMIC_ARRAY_NOCOUNT((char*&)m_OctantVerts, 8);
#endif // OCTANT_MAP_SUPPORT_ACCEL

	STRUCT_FIELD(m_NumVertices);
	STRUCT_FIELD(m_NumPolygons);

#if OCTANT_MAP_SUPPORT_ACCEL
	STRUCT_IGNORE(m_Pad0[0]);
	STRUCT_IGNORE(m_Pad0[1]);
	STRUCT_IGNORE(m_Pad0[2]);
#else // OCTANT_MAP_SUPPORT_ACCEL
	STRUCT_IGNORE(m_Pad0);
#endif // OCTANT_MAP_SUPPORT_ACCEL

	STRUCT_END();
}
#endif // __DECLARESTRUCT


void phBoundPolyhedron::ComputeNeighbors(const char* OUTPUT_ONLY(filename))
{
	if (!sm_ComputeNeighbors || m_NumPolygons == 0)
		return;

	phPolygon *geomPolys = (phPolygon *)(m_Polygons);

	sysMemStartTemp();

	// Build a reverse map from vertices to the polygons that reference them
    atArray<u32>* vertexPolys = rage_new atArray<u32>[m_NumVertices];

	for(int polyIndex = 0; polyIndex < m_NumPolygons; ++polyIndex)
	{
		const phPrimitive &bvhPrimitive = geomPolys[polyIndex].GetPrimitive();
		if(GetType() == phBound::BVH && bvhPrimitive.GetType() != PRIM_TYPE_POLYGON)
		{
			continue;
		}

		geomPolys[polyIndex].SetNeighboringPolyNum(0, (phPolygon::Index)(-1));
		geomPolys[polyIndex].SetNeighboringPolyNum(1, (phPolygon::Index)(-1));
		geomPolys[polyIndex].SetNeighboringPolyNum(2, (phPolygon::Index)(-1));

		for (int vertex = 0; vertex < POLY_MAX_VERTICES; ++vertex)
		{
            vertexPolys[geomPolys[polyIndex].GetVertexIndex(vertex)].Grow() = polyIndex;
		}
	}

	int numFound = 0;

	for(u32 polyIndex = 0; polyIndex < (u32)(m_NumPolygons - 1); ++polyIndex)
	{
		const phPrimitive &bvhPrimitive = geomPolys[polyIndex].GetPrimitive();
		if(GetType() == phBound::BVH && bvhPrimitive.GetType() != PRIM_TYPE_POLYGON)
		{
			continue;
		}

		phPolygon* poly1 = &geomPolys[polyIndex];
		int numVerts1 = POLY_MAX_VERTICES;

		// Loop through the vertex indices on the first polygon to see if there is a match on the second polygon.
		for (int vertexIndex1 = 0; vertexIndex1 < numVerts1; ++vertexIndex1)
		{
            atArray<u32>& polysForVert = vertexPolys[poly1->GetVertexIndex(vertexIndex1)];
            int numPolys = polysForVert.GetCount();
            for (int vertexPolyIndex = 0; vertexPolyIndex < numPolys; ++vertexPolyIndex)
			{
				// Don't consider ourselves, or any polygons with a lower index
                if (polysForVert[vertexPolyIndex] > polyIndex)
				{
                    phPolygon* poly2 = &geomPolys[polysForVert[vertexPolyIndex]];
					int numVerts2 = POLY_MAX_VERTICES;

					// Loop through the vertices of the second polygon to look for a match.
					int vertexIndex2;
					for (vertexIndex2 = 0; vertexIndex2 < numVerts2; ++vertexIndex2)
					{
						if(poly1->GetVertexIndex(vertexIndex1) == poly2->GetVertexIndex(vertexIndex2))
						{
							// Two verts match.
							break;
						}
					}

					if (vertexIndex2 < numVerts2)
					{
						// Look to see if the corresponding vertices match up.
						int vertexIndex1b = vertexIndex1 != numVerts1 - 1 ? vertexIndex1 + 1 : 0;
						int vertexIndex2b = vertexIndex2 != 0 ? vertexIndex2 - 1 : numVerts2 - 1;
						if(poly1->GetVertexIndex(vertexIndex1b) == poly2->GetVertexIndex(vertexIndex2b))
						{
							bool poly1HasNeighbor = poly1->GetNeighboringPolyNum(vertexIndex1) != (phPolygon::Index)(-1);
							bool poly2HasNeighbor = poly2->GetNeighboringPolyNum(vertexIndex2b) != (phPolygon::Index)(-1);

							if(poly1HasNeighbor || poly2HasNeighbor)
							{
								// At least one of them already has a neighbor, let's not try to give them another one.
								if (phBound::MessagesEnabled())
								{
									Warningf("More than two polygons are incident at the same edge in file '%s'.", filename);

								#if !__FINAL
									Vec3V rvecVertex1 = GetVertex(poly1->GetVertexIndex(vertexIndex1));
									Vec3V rvecVertex2 = GetVertex(poly1->GetVertexIndex(vertexIndex1b));
									Warningf("- Model space coordinates for the edge are: (%f, %f, %f) and (%f, %f, %f)", rvecVertex1.GetXf(), rvecVertex1.GetYf(), rvecVertex1.GetZf(), rvecVertex2.GetXf(), rvecVertex2.GetYf(), rvecVertex2.GetZf());
								#endif

								}
							}
							else
							{
								// Tell the polygons that they have neighbors.
                                poly1->SetNeighboringPolyNum(vertexIndex1, (u16)(polysForVert[vertexPolyIndex]));
								poly2->SetNeighboringPolyNum(vertexIndex2b, (u16)(polyIndex));
								++numFound;
							}
						}
					}
				}
			}
		}
	}

	delete [] vertexPolys;

	sysMemEndTemp();
}

void phBoundPolyhedron::ComputeVertexNormals(Vec3V_Ptr vertexNormalsOut) const
{
	// Should only be dealing with tris at this point
	CompileTimeAssert(POLY_MAX_VERTICES == 3);

	sysMemSet(vertexNormalsOut, 0, sizeof(Vec3V) * m_NumVertices);

	for (int polygonIndex = 0; polygonIndex < m_NumPolygons; polygonIndex++)
	{
		const phPolygon& polygon = m_Polygons[polygonIndex];

		const rage::phPrimitive *curPrim = reinterpret_cast<const rage::phPrimitive *>(&polygon);
		if(curPrim->GetType() == rage::PRIM_TYPE_POLYGON)
		{
			// Not using GetPolygonUnitNormal because I don't trust m_Area
			const Vec3V thisTriVert[3] = { GetVertex(polygon.GetVertexIndex(0)), GetVertex(polygon.GetVertexIndex(1)), GetVertex(polygon.GetVertexIndex(2)) };
			const Vec3V thisTriNorm = Normalize(polygon.ComputeNonUnitNormal(thisTriVert[0], thisTriVert[1], thisTriVert[2]));

			// Add this triangle's contribution to the vertex normals
			for (int polyVertexIndex = 0; polyVertexIndex < 3; polyVertexIndex++)
			{
				int boundVertexIndex = polygon.GetVertexIndex(polyVertexIndex);
				vertexNormalsOut[boundVertexIndex] = Add(vertexNormalsOut[boundVertexIndex], thisTriNorm);
			}
		}
	}

	// Normalize the summed contributions of all polys that share any one vertex
	for (int vertexIndex = 0; vertexIndex < m_NumVertices; vertexIndex++)
	{
		vertexNormalsOut[vertexIndex] = NormalizeSafe(vertexNormalsOut[vertexIndex], Vec3V(V_Z_AXIS_WZERO), Vec3V(V_FLT_SMALL_6));
	}
}

void phBoundPolyhedron::SetCompressedVertexNormals(Vec3V_Ptr vertexNormals)
{
	// Should only be dealing with tris at this point
	FastAssert(POLY_MAX_VERTICES == 3);

	phPolygon *polygons = const_cast<phPolygon *>(m_Polygons);
	for (int polygonIndex = 0; polygonIndex < m_NumPolygons; polygonIndex++)
	{
		phPolygon& polygon = polygons[polygonIndex];

		const rage::phPrimitive *curPrim = reinterpret_cast<rage::phPrimitive *>(&polygon);
		if(curPrim->GetType() == rage::PRIM_TYPE_POLYGON)
		{
			const int triVertIndex[3] = { polygon.GetVertexIndex(0), polygon.GetVertexIndex(1), polygon.GetVertexIndex(2) };
			const Vec3V triVert[3] = { GetVertex(triVertIndex[0]), GetVertex(triVertIndex[1]), GetVertex(triVertIndex[2]) };
			const Vec3V triNorm = Normalize(polygon.ComputeNonUnitNormal(triVert[0], triVert[1], triVert[2])); // Because I don't trust m_Area

			// For later
			Vec3V triCenter = Scale(Add(Add(triVert[0], triVert[1]), triVert[2]), ScalarV(V_THIRD));

			// Calculate real vertex normals for each triangle vertex
			Vec3V actualVertNorm[3] = { vertexNormals[triVertIndex[0]], vertexNormals[triVertIndex[1]], vertexNormals[triVertIndex[2]] };
			for(int i = 0; i < 3; i++)
			{
				// Currently we smash it down to a single bit, choosing between two possible options
				// - Done by taking the closer of our own face normal or a direction out from the triangle barycenter through the vertex in question
				Vec3V normOutFromCenter = Normalize(Subtract(triVert[i], triCenter));
				Vec3V avgNormOutFromCenter = Normalize(Add(normOutFromCenter, triNorm));
				u32 useNormOut = GenerateMaskNZ(IsGreaterThanAll( Dot(avgNormOutFromCenter, actualVertNorm[i]), Dot(triNorm, actualVertNorm[i]) ));
				//
				polygon.SetVertexNormalCode(i, useNormOut);
			}
		}
	}
}
#endif	// !__SPU


/////////////////////////////////////////////////////////////////////////////////
// draw physics functions

#define SORT_DEBUGGING 0

#if !__SPU
#if __PFDRAW || (__WIN32PC && !__FINAL)

#if SORT_DEBUGGING
static Vector3 s_PolyNormal(VEC3_ZERO);
static Vector3 s_LastVertexAdded(VEC3_ZERO);
static Vector3 s_TriVert0(VEC3_ZERO);
static Vector3 s_TriVert1(VEC3_ZERO);
static Vector3 s_TriVert2(VEC3_ZERO);
#endif

Vec3V_Out ComputeQuadraticConstants(ScalarV_In x0, ScalarV_In x1, ScalarV_In x2)
{
	// We have a quadratic equation of the form
	//   ax^2 + bx + c = y
	// We want to know what a, b, and c are.
	// We know this:
	//  a*x0^2 + b*x0 + c = 1
	//  b*x1^2 + b*x1 + c = 1/2
	//  c*x2^2 + b*x2 + c = 0
	Mat33V quadraticMatrix = Mat33V(Vec3V(x0*x0,x1*x1,x2*x2),
									Vec3V(x0,x1,x2),
									Vec3V(V_ONE));
	Mat33V invQuadraticMatrix;
	InvertFull(invQuadraticMatrix,quadraticMatrix);
	return Multiply(invQuadraticMatrix,Vec3V(ScalarV(V_ONE),ScalarV(V_HALF),ScalarV(V_ZERO)));
}

__forceinline float phBoundPolyhedron::RatePolygon(phPolygon::Index polygonIndex, ScalarV_In minArea, ScalarV_In maxArea, Vec3V_In quadraticConstants) const
{
	const phPolygon& polygon = GetPolygon(polygonIndex);

	if (PFD_GETENABLED(PFD_MaxNeighborAngleEnabled))
	{
		const Vec3V normal(GetPolygonUnitNormal(polygonIndex));

		for (int neighborIndex = 0; neighborIndex < 3; neighborIndex++)
		{
			const phPolygon::Index neighborPolygonIndex = polygon.GetNeighboringPolyNum(neighborIndex);

			if (neighborPolygonIndex == BAD_POLY_INDEX)
			{
				return 1.0f;
			}
			else
			{
				const Vec3V neighborNormal(GetPolygonUnitNormal(neighborPolygonIndex));
				const float angle = acosf(Clamp<float>(Dot(normal, neighborNormal).Getf(), -1.0f, 1.0f))*RtoD;

				if (angle > PFD_GETVALUE(PFD_MaxNeighborAngle))
				{
					return 1.0f;
				}
			}
		}
	}

	const ScalarV clampedArea = Clamp(ScalarVFromF32(polygon.GetArea()),minArea,maxArea);
	return Dot(quadraticConstants,Vec3V(Scale(clampedArea,clampedArea),clampedArea,ScalarV(V_ONE))).Getf();
}

float phBoundPolyhedron::RatePolygonRecursive(phPolygon::Index polygonIndex, int numRecursions) const
{
	Assert(numRecursions >= 0);
	ScalarV smallPolygonArea = ScalarVFromF32(PFD_GETVALUE(PFD_SmallPolygonArea));
	ScalarV mediumPolygonArea = ScalarVFromF32(PFD_GETVALUE(PFD_MediumPolygonArea));
	ScalarV largePolygonArea = ScalarVFromF32(PFD_GETVALUE(PFD_LargePolygonArea));
	if(IsLessThanAll(largePolygonArea,mediumPolygonArea + ScalarV(V_FLT_SMALL_1)) || IsLessThanAll(mediumPolygonArea,smallPolygonArea + ScalarV(V_FLT_SMALL_2)))
	{
		smallPolygonArea = ScalarV(V_FLT_SMALL_4);
		mediumPolygonArea = ScalarV(V_ONE);
		largePolygonArea = ScalarV(V_TEN);
	}
	const Vec3V quadraticConstants = ComputeQuadraticConstants(largePolygonArea,mediumPolygonArea,smallPolygonArea);

#if 0 && __DEV
	// this is useful if we need to dump the parameters used for polygon density rendering (i.e. to use them for the offline density image)
	static bool bDumpRatePolygonParams = false;
	if (bDumpRatePolygonParams)
	{
		bDumpRatePolygonParams = false;

		Displayf("smallPolygonArea=%f", smallPolygonArea.Getf());
		Displayf("mediumPolygonArea=%f", mediumPolygonArea.Getf());
		Displayf("largePolygonArea=%f", largePolygonArea.Getf());
		Displayf("quadraticConstants=%f,%f,%f", VEC3V_ARGS(quadraticConstants));

		for (int i = 0; i < 32; i++)
		{
			if (sm_TypeFlagDensityScaling[i] != 0.0f) { Displayf("sm_TypeFlagDensityScaling[%d]=%f", i, sm_TypeFlagDensityScaling[i]); }
		}
		for (int i = 0; i < 32; i++)
		{
			if (sm_IncludeFlagDensityScaling[i] != 0.0f) { Displayf("sm_IncludeFlagDensityScaling[%d]=%f", i, sm_IncludeFlagDensityScaling[i]); }
		}
	}
#endif // __DEV

	if(numRecursions == 0)
	{
		// early out here so we don't make that giant bitset
		return RatePolygon(polygonIndex,smallPolygonArea,largePolygonArea,quadraticConstants);
	}

	const int maxNumTotalPolygons = VERT_INDEX_MASK; // hopefully we can assume no meshes have more polys than verts AND have thousands of polygons
	const int maxNumProcessedPolygons = 200;

	// Bitset of polygon's we've already processed
	atFixedBitSet<maxNumTotalPolygons> processedPolygonBits;

	// Array of polygons that are expanding
	atFixedArray<phPolygon::Index,maxNumProcessedPolygons> expandingPolygons;

	const int maxNumPolySets = 5;
	const int numPolySets = Min(maxNumPolySets,numRecursions + 1);
	float polySetRatings[maxNumPolySets];

	// Start the algorithm off by pushing the initial polygon onto the expansion list and marking it as processed
	processedPolygonBits.Set(polygonIndex);
	expandingPolygons.Push(polygonIndex);
	int firstExpandingPolyIndex = 0;
	for(int polySetIndex = 0; polySetIndex < numPolySets; ++polySetIndex)
	{
		// The expanding polygons are the ones that were added on the last iteration of the loop
		int endExpandingPolyIndex = expandingPolygons.GetCount();
		int numExpandingPolygons = endExpandingPolyIndex-firstExpandingPolyIndex;
		if(numExpandingPolygons == 0)
		{
			// If there are no polygons to expand, just use the previous expansion's average
			polySetRatings[polySetIndex] = polySetRatings[polySetIndex-1];
		}
		else
		{
			float polySetRatingSum = 0;
			for(int expandingPolyIndex = firstExpandingPolyIndex; expandingPolyIndex < endExpandingPolyIndex; ++expandingPolyIndex)
			{
				// Rate each expanding polygon and add its rating to the total sum
				const phPolygon& expandingPolygon = GetPolygon(expandingPolygons[expandingPolyIndex]);
				polySetRatingSum += RatePolygon(expandingPolygons[expandingPolyIndex],smallPolygonArea,largePolygonArea,quadraticConstants);

				// Go through each neighbor and add it to the expanding list if it isn't already processed
				for(int neighborIndex = 0; neighborIndex < 3; ++neighborIndex)
				{
					phPolygon::Index neighborPolygonIndex = expandingPolygon.GetNeighboringPolyNum(neighborIndex);
					if(neighborPolygonIndex != BAD_POLY_INDEX && !processedPolygonBits.GetAndSet(neighborPolygonIndex))
					{
						if(!expandingPolygons.IsFull())
						{
							expandingPolygons.Push(neighborPolygonIndex);
						}
					}
				}
			}
			// Compute the average rating for this set
			polySetRatings[polySetIndex] = polySetRatingSum/(float)numExpandingPolygons;
		}
		firstExpandingPolyIndex = endExpandingPolyIndex;
	}

	// Loop over all the sets and come up with a total average, weighting the polygons closer to the initial polygon as higher
	float weightedSum = 0;
	float totalWeights = 0;
	int weightingStyle = PFD_GETVALUE(PFD_RecusiveWeightingStyle);
	for(int polySetIndex = 0; polySetIndex < numPolySets; ++polySetIndex)
	{
		float weight = 1.0f;
		switch(weightingStyle)
		{
		case 0:
			weight = 1.0f; break; // Every set is equal: 1, 1, ... , 1
		case 1:
			weight = (float)(numPolySets-polySetIndex); break; // Each set is weighted linearly more than the set expanding from it: n, n-1, ... , 1
		case 2:
			weight = (float)(1 << ((numPolySets-polySetIndex)-1)); break; // Each set is weighted twice as much as the set expanded from it: 2^(n-1), 2^(n-2), ... , 2^(n-n)
		default: Assert(false);
		}
		weightedSum += weight * polySetRatings[polySetIndex];
		totalWeights += weight;
	}
	return weightedSum / totalWeights;
}

Color32 phBoundPolyhedron::ComputeDensityColor(float value) const
{
	const ScalarV midPoint = ScalarVFromF32(PFD_GETVALUE(PFD_ColorMidPoint));
	const ScalarV midPointToOne = Subtract(ScalarV(V_ONE),midPoint);

	// Go from pink to red to green
	const Vec3V colorAtZero =		Vec3V(0.4f,0.0f,0.6f);
	const Vec3V colorAtMidPoint =	Lerp(midPoint,Vec3V(V_X_AXIS_WZERO),Vec3V(V_Y_AXIS_WZERO));
	const Vec3V colorAtOne =		Vec3V(V_Y_AXIS_WZERO);

	const ScalarV finalAreaRating = ScalarV(powf(Clamp<float>(value, 0.0f, 1.0f), powf(2.0f, PFD_GETVALUE(PFD_ColorExpBias))));

	const Vec3V color = SelectFT(	IsGreaterThan(finalAreaRating,midPoint),
									Lerp(InvScaleSafe(finalAreaRating,midPoint,ScalarV(V_ZERO)),colorAtZero,colorAtMidPoint),
									Lerp(InvScaleSafe(Subtract(finalAreaRating,midPoint),midPointToOne,ScalarV(V_ZERO)),colorAtMidPoint,colorAtOne));

	return Color32(Clamp(color,Vec3V(V_ZERO),Vec3V(V_ONE)));
}

Color32 phBoundPolyhedron::ComputePolygonDensityColor(phPolygon::Index polygonIndex, u32 PFD_ONLY(typeFlags), u32 PFD_ONLY(includeFlags)) const
{
	Assert(GetPolygon(polygonIndex).GetPrimitive().GetType() == PRIM_TYPE_POLYGON);

	if (PFD_GETENABLED(PFD_PolygonAngleDensity))
	{
		return ComputePolygonAngleDensityColor(polygonIndex);
	}

	float areaRating = RatePolygonRecursive(polygonIndex,PFD_GETVALUE(PFD_NumRecursions));
#if __PFDRAW
	areaRating *= GetMaxTypeIncludeFlagScale(typeFlags,includeFlags);
#endif // __PFDRAW
	return ComputeDensityColor(areaRating);
}

Color32 phBoundPolyhedron::ComputePolygonAngleDensityColor(phPolygon::Index polygonIndex) const
{
	const Vec3V normal(GetPolygonUnitNormal(polygonIndex));
	float minDot = 1.0f;

	for (int neighbor = 0; neighbor < 3; neighbor++)
	{
		const phPolygon::Index neighborPolygonIndex = GetPolygon(polygonIndex).GetNeighboringPolyNum(neighbor);

		if (neighborPolygonIndex != BAD_POLY_INDEX)
		{
			const Vec3V neighborNormal(GetPolygonUnitNormal(neighborPolygonIndex));
			minDot = Min<float>(Dot(normal, neighborNormal).Getf(), minDot);
		}
	}

	return ComputeDensityColor(1.0f - acosf(Clamp<float>(minDot, 0.0f, 1.0f))/(PI*0.5f));
}

#endif // __PFDRAW || (__WIN32PC && !__FINAL)

#if __PFDRAW

float phBoundPolyhedron::GetMaxTypeIncludeFlagScale(u32 typeFlags, u32 includeFlags) const
{
	// Apply type/include flag scaling
	float maxTypeIncludeFlagScale = 0.0f;
	for(int i = 0; i < 32; ++i)
	{
		if(typeFlags & (1 << i))
		{
			maxTypeIncludeFlagScale = Max(maxTypeIncludeFlagScale,sm_TypeFlagDensityScaling[i]);
		}
		if(includeFlags & (1 << i))
		{
			maxTypeIncludeFlagScale = Max(maxTypeIncludeFlagScale,sm_IncludeFlagDensityScaling[i]);
		}
	}
	if(maxTypeIncludeFlagScale == 0)
	{
		maxTypeIncludeFlagScale = 1.0f;
	}
	return maxTypeIncludeFlagScale;
}


void phBoundPolyhedron::DrawInternal(int polygonIndex, Vec3V_In center) const
{
	grcColor(Color_white);

	if (PFD_DrawBoundMaterialNames.WillDraw())
	{
		// Write the polygon index number on the screen at the average location of its vertices.
		phMaterialIndex idx = GetPolygonMaterialIndex(polygonIndex);
		char name[256];
		MATERIALMGR.GetMaterialName(GetMaterialId(idx), name, 255);
		grcDrawLabelf(Vector3(center.GetXf(), center.GetYf(), center.GetZf()), name);
	}

	if ((PFD_BvhPolygonIndices.WillDraw() && GetType() == phBound::BVH) || (PFD_NonBvhPolygonIndices.WillDraw() && GetType() != phBound::BVH))
	{
		// Write the polygon index number on the screen at the average location of its vertices.
		char polygonIndexText[8];
		polygonIndexText[7] = '\0';
		formatf(polygonIndexText,7,"%i",polygonIndex);
		grcDrawLabelf(Vector3(center.GetXf(), center.GetYf(), center.GetZf()),polygonIndexText);
	}
}

float g_AttentionSphereRadius;

void phBoundPolyhedron::CullSpherePolys (phBoundCuller& culler, Vec3V_In UNUSED_PARAM(sphereCenter), ScalarV_In UNUSED_PARAM(sphereRadius)) const
{
	culler.AddCulledPolygonIndices(0,GetNumPolygons());
}

void phBoundPolyhedron::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int whichPolys, phMaterialFlags highlightFlags, unsigned int typeFilter, unsigned int includeFilter, unsigned int boundTypeFlags, unsigned int boundIncludeFlags) const
{
	Mat34V_In mtx = mtxIn;
	grcWorldMtx(mtx);
	Color32 oldColor(grcCurrentColor);

	const grcViewport* pViewport = grcViewport::GetCurrent();
	const Vec3V vMask2D = Vec3V(V_ONE) - Vec3V(g_UnitUp);

	Vec3V cameraPos2D(V_ZERO);
#if !DISABLE_DRAW_GRCVIEWPORT_GETCURRENT
    if (grcViewport::GetCurrent())
    {
        cameraPos2D = grcViewport::GetCurrentCameraPosition()*vMask2D;
    }
#endif

	ScalarV maxDrawDistance = ScalarV(PFD_BoundDrawDistance.GetValue());
	ScalarV squaredDrawDistance = square(maxDrawDistance);
	ScalarV inverseDrawDistance = InvertSafe(maxDrawDistance);

#if COMPRESSED_VERTEX_METHOD == 0
	const Vec3V* shrunkVertices = m_ShrunkVertices;
    if (shrunkVertices == NULL)
    {
        shrunkVertices = m_Vertices;
    }
#endif

#if COMPRESSED_VERTEX_METHOD > 0
	const CompressedVertexType *compressedVerticesToDraw = m_CompressedShrunkVertices != NULL ? m_CompressedShrunkVertices : m_CompressedVertices;
#endif

	Vec3V shrunkVertex0, shrunkVertex1, shrunkVertex2;

	if(GetType() != phBound::BOX)
	{
		ScalarV thinPolyTolerance = Expt(ScalarV(PFD_ThinPolyTolerance.GetValue()) / ScalarV(V_LOG2_TO_LOG10));
		ScalarV bigPrimTolerance = ScalarV(PFD_BigPrimTolerance.GetValue());

		bool sphereCullPolys = (PFD_SphereCull.GetEnabled() && GetType() == phBound::BVH);
		phBoundCuller culler;
		int maxNumCulledPrimitives = 0; 
		phPolygon::Index* culledPrimitiveIndices = NULL;
		if(sphereCullPolys)
		{
			maxNumCulledPrimitives = Min(DEFAULT_MAX_CULLED_POLYS,GetNumPolygons());
			culledPrimitiveIndices = Alloca(phPolygon::Index,maxNumCulledPrimitives);
			culler.SetArrays(culledPrimitiveIndices,maxNumCulledPrimitives);
			CullSpherePolys(culler,UnTransformOrtho(mtxIn,grcViewport::GetCurrentCameraPosition()),maxDrawDistance);
		}

		// Loop over each polygon and do any relevant non-textual debug rendering.
		int numCulledPolygons = sphereCullPolys ? culler.GetNumCulledPolygons() : GetNumPolygons();
		for (int culledPrimitiveIndexIndex=0; culledPrimitiveIndexIndex<numCulledPolygons; culledPrimitiveIndexIndex++)
		{
			int polygonIndex = sphereCullPolys ? culler.GetCulledPolygonIndex(culledPrimitiveIndexIndex) : culledPrimitiveIndexIndex;
			const phPolygon& polygon = m_Polygons[polygonIndex];
			const phPrimitive &bvhPrimitive = polygon.GetPrimitive();

			phMaterialIndex materialIndex = GetPolygonMaterialIndex(polygonIndex);
			phMaterialMgr::Id polyMaterialID = GetMaterialId(materialIndex);
			phMaterialFlags polyFlags = MATERIALMGR.GetFlags(polyMaterialID);

			bool isBig = false;
			if ((whichPolys & RENDER_BIG_PRIMITIVES) && bvhPrimitive.IsBig(this, bigPrimTolerance))
			{
				solid = true;
				isBig = true;
			}

			if(bvhPrimitive.GetType() == PRIM_TYPE_POLYGON || GetType() != phBound::BVH)
			{
#if COMPRESSED_VERTEX_METHOD == 0
				{
					shrunkVertex0 = shrunkVertices[polygon.GetVertexIndex(0)];
					shrunkVertex1 = shrunkVertices[polygon.GetVertexIndex(1)];
					shrunkVertex2 = shrunkVertices[polygon.GetVertexIndex(2)];
				}
#endif
#if COMPRESSED_VERTEX_METHOD > 0
				{
					shrunkVertex0 = DecompressVertex(compressedVerticesToDraw + 3 * polygon.GetVertexIndex(0));
					shrunkVertex1 = DecompressVertex(compressedVerticesToDraw + 3 * polygon.GetVertexIndex(1));
					shrunkVertex2 = DecompressVertex(compressedVerticesToDraw + 3 * polygon.GetVertexIndex(2));
				}
#endif

				if (whichPolys && !isBig)
				{
					bool thinPoly = (whichPolys & RENDER_THIN_POLYS) && polygon.IsThin(this, thinPolyTolerance);
					bool badNormalPoly = (whichPolys & RENDER_BAD_NORMAL_POLYS) && polygon.HasBadNormal(this);
					bool badNeighborPoly = (whichPolys & RENDER_BAD_NEIGHBOR_POLYS) && polygon.HasBadNeighbors(this);

					if (!thinPoly && !badNormalPoly && !badNeighborPoly)
					{
						continue;
					}

					grcColor(Color32(thinPoly ? 255 : 0, badNormalPoly ? 255 : 0, badNeighborPoly ? 255 : 0));

					// Find the average position of the polygon's vertices.
					Vec3V center(GetVertex(polygon.GetVertexIndex(0)));
					center = Add(center, GetVertex(polygon.GetVertexIndex(1)));
					center = Add(center, GetVertex(polygon.GetVertexIndex(2)));
					center = Scale(center, ScalarV(V_THIRD));
					center = Transform(mtxIn, center);

					const Mat34V prevWorldMtx(grcWorldMtx());
					grcDrawSphere(g_AttentionSphereRadius, center);
					grcWorldMtx(prevWorldMtx);
				}

				// Check to see if any of the vertices of this polygon are close enough to the camera to draw.
				bool vertsInInterest = false;
				Vec3V adjustedVerts[POLY_MAX_VERTICES],center(V_ZERO);
				int numVertices = POLY_MAX_VERTICES;
				Assert(numVertices == 3);
				for (int polyVertexIndex = 0; polyVertexIndex < numVertices; ++polyVertexIndex)
				{
					adjustedVerts[polyVertexIndex] = Transform(mtx, GetVertex(polygon.GetVertexIndex(polyVertexIndex)));
					center = Add(center, adjustedVerts[polyVertexIndex]);
					if (!pViewport || (!vertsInInterest && IsLessThanAll(DistSquared(cameraPos2D, adjustedVerts[polyVertexIndex]*vMask2D),squaredDrawDistance)))
					{
						vertsInInterest = true;
					}
				}

				// At least one of the vertices of this polygon was close enough to draw, let's check to see if the polygon as a whole is at least partially visible.
				if (vertsInInterest && pViewport && !pViewport->IsPointSetVisible(adjustedVerts, numVertices))
				{
					vertsInInterest = false;
				}

				if (!vertsInInterest || (PFDGROUP_PrimitiveDensity.GetEnabled() && !PFD_IncludePolygons.GetEnabled() && !PFDGROUP_PolygonDensity.GetEnabled()))
				{
					continue;
				}

				adjustedVerts[0] = GetVertex(polygon.GetVertexIndex(0));
				adjustedVerts[1] = GetVertex(polygon.GetVertexIndex(1));
				adjustedVerts[2] = GetVertex(polygon.GetVertexIndex(2));

#if POLY_MAX_VERTICES == 4
				// Adjust non-planar quads to appear the way they actually collide
				if (PFD_FlattenQuads.GetEnabled() && numVertices == 4 && GetType() == phBound::BVH)
				{
					{
						Vec3V diff1;
						diff1 = Subtract(adjustedVerts[0], adjustedVerts[1]);
						ScalarV adjust1;
						adjust1 = Dot(diff1, GetPolygonUnitNormal(polygonIndex));
						adjustedVerts[1] = Add(adjustedVerts[1], Vec3V(adjust1));
					}

					{
						Vec3V diff3;
						diff3 = Subtract(adjustedVerts[0], adjustedVerts[3]);
						ScalarV adjust3;
						adjust3 = Dot(diff3, GetPolygonUnitNormal(polygonIndex));
						adjustedVerts[3] = Add(adjustedVerts[3], Vec3V(adjust3));
					}
				}
#endif // POLY_MAX_VERTICES == 4

				// Find the center of the polygon (the average vertex position).
				center = Scale(center, numVertices == 3 ? ScalarV(V_THIRD) : ScalarV(V_QUARTER));
				Vec3V polyCenter = center;
#if !DISABLE_DRAW_GRCVIEWPORT_GETCURRENT
				if (grcViewport::GetCurrent())
				{
					center = Subtract(center, grcViewport::GetCurrentCameraPosition());
				}
#endif

				// See if this polygon is facing the camera.
				Vec3V normal;
				normal = Transform3x3(mtx, GetPolygonUnitNormal(polygonIndex));
				bool facingTowardsCamera = IsLessThan(Dot(normal, center), ScalarV(V_ZERO)).Getb();

				if(facingTowardsCamera)
				{
					DrawInternal(polygonIndex, polyCenter);
				}

				// Set the color and solid state.
				if (solid)
				{
					if (facingTowardsCamera || m_IsFlat)
					{
						Color32 newColor(Color_grey90);

						if (isBig)
						{
							newColor = Color_red;
						}
						else if (highlightFlags & polyFlags)
						{
							newColor = MATERIALMGR.GetDebugColor(polyMaterialID, polyFlags, highlightFlags);
						}
						else if (PFD_Quads.GetEnabled())
						{
							newColor = numVertices == 3 ? Color_red : Color_green;
						}
						else if (colorMaterials)
						{
							newColor = MATERIALMGR.GetDebugColor(MATERIALMGR.GetMaterial(polyMaterialID));
						}
						else if (PFDGROUP_PolygonDensity.GetEnabled())
						{
							newColor = ComputePolygonDensityColor((phPolygon::Index)polygonIndex,typeFilter&boundTypeFlags,includeFilter&boundIncludeFlags);
						}
						else if(GetType() == phBound::BVH && PFDGROUP_PrimitiveDensity.GetEnabled() && PFD_IncludePolygons.GetEnabled())
						{
							newColor = static_cast<const phBoundBVH*>(this)->ComputePrimitiveDensityColor((phPolygon::Index)polygonIndex,typeFilter&boundTypeFlags,includeFilter&boundIncludeFlags);
						}
						else
						{
							newColor = oldColor;
						}

						if (PFD_SolidBoundRandom.GetEnabled() && !PFDGROUP_PolygonDensity.GetEnabled() && !PFDGROUP_PrimitiveDensity.GetEnabled())
						{
							/////////////////////////////////
							// Want to randomize the colors a bit when drawing solid
							// so we can actually make out the individual polys

							// get a random seed from this poly's vertex positions, so the colors won't flicker
							float fRandomSeed = 1000.0f * (GetVertex(polygon.GetVertexIndex(0))[0] + GetVertex(polygon.GetVertexIndex(1))[0] + GetVertex(polygon.GetVertexIndex(2))[0]
														+ GetVertex(polygon.GetVertexIndex(0))[1] + GetVertex(polygon.GetVertexIndex(1))[1] + GetVertex(polygon.GetVertexIndex(2))[1]
														+ GetVertex(polygon.GetVertexIndex(0))[2] + GetVertex(polygon.GetVertexIndex(1))[2] + GetVertex(polygon.GetVertexIndex(2))[2]);
							unsigned int RandomSeed = (unsigned int)fRandomSeed;
							srand(RandomSeed);

							// calculate a random percentage to modify the current color (+/- 10%)
							float RandomColorMult = 1.0f + (0.2f*(rand() / (float)(RAND_MAX)) - 0.1f);

							// modify and clamp each color
							int nNewColorVal = int(newColor.GetRed() * RandomColorMult);
							if(nNewColorVal > 255) nNewColorVal = 255;
							newColor.SetRed(nNewColorVal);

							nNewColorVal = int(newColor.GetGreen() * RandomColorMult);
							if(nNewColorVal > 255) nNewColorVal = 255;
							newColor.SetGreen(nNewColorVal);

							nNewColorVal = int(newColor.GetBlue() * RandomColorMult);
							if(nNewColorVal > 255) nNewColorVal = 255;
							newColor.SetBlue(nNewColorVal);
							// end random colour modification
							//////////////////////////////
						}

						newColor.SetAlpha(int(255 * PFD_BoundSolidOpacity.GetValue()));

						grcBindTexture(NULL);
						grcColor(newColor);

						bool oldLighting = false;
						if (PFD_SolidBoundLighting.GetEnabled() == false)
						{
							// don't light solid poly rendering either, it's all coming out white and we don't care about lighting anyway
							oldLighting = grcLighting(false);
						}

						if (PFD_MarginOriginalPolygonsSolid.GetEnabled() || (GetType() != GEOMETRY && GetType() != BOX))
						{
							Vec3V normal(GetPolygonUnitNormal(polygonIndex));
							if (m_IsFlat && !facingTowardsCamera)
							{
								// This polygon is facing away from the camera and the bound is flat, so swap the first and second vertices and negate the normal so that it will be visible.
								SwapEm(adjustedVerts[0],adjustedVerts[2]);
								normal = Negate(normal);
							}

							grcBegin(drawTris,(numVertices - 2) * 3);
							grcNormal3f(normal);
							grcVertex3f(adjustedVerts[0]);
							grcVertex3f(adjustedVerts[1]);
							grcVertex3f(adjustedVerts[2]);
							grcEnd();
						}

						if (PFD_MarginExpandedShrunkPolygons.GetEnabled() && (GetType() == GEOMETRY || GetType() == BOX))
						{
							Vec3V normal(GetPolygonUnitNormal(polygonIndex));
							if (m_IsFlat && !facingTowardsCamera)
							{
								// This polygon is facing away from the camera and the bound is flat, so swap the first and second vertices and negate the normal so that it will be visible.
								SwapEm(shrunkVertex0,shrunkVertex2);
								normal = Negate(normal);
							}

							grcBegin(drawTris,(numVertices - 2) * 3);
							grcNormal3f(normal);
							Vec3V vertices0, vertices1, vertices2;
							vertices0 = AddScaled(shrunkVertex0, normal, GetMarginV());
							vertices1 = AddScaled(shrunkVertex1, normal, GetMarginV());
							vertices2 = AddScaled(shrunkVertex2, normal, GetMarginV());
							grcVertex3f(vertices0);
							grcVertex3f(vertices1);
							grcVertex3f(vertices2);
							grcEnd();
						}

						if (PFD_SolidBoundLighting.GetEnabled() == false)
						{
							// don't light solid poly rendering either, it's all coming out white and we don't care about lighting anyway
							grcLighting(oldLighting);
						}
					}
				}
				else if (PFD_EdgeAngles.GetEnabled())
				{
					const bool oldLighting = grcLighting(false);

					const Vec3V normal(GetPolygonUnitNormal(polygonIndex));

					for (int neighbor = 0; neighbor < 3; neighbor++)
					{
						phPolygon::Index neighborPolygonIndex = GetPolygon(polygonIndex).GetNeighboringPolyNum(neighbor);
						bool bDraw = false;

						if (neighborPolygonIndex == BAD_POLY_INDEX)
						{
							grcColor(Color32(255,0,0,255));
							bDraw = true;
						}
						else if (neighborPolygonIndex < polygonIndex)
						{
							const Vec3V neighborNormal(GetPolygonUnitNormal(neighborPolygonIndex));
							const float dot = Dot(normal, neighborNormal).Getf();
							grcColor(ComputeDensityColor(1.0f - acosf(Clamp<float>(dot, 0.0f, 1.0f))/(PI*0.5f)));
							bDraw = true;
						}

						if (bDraw)
						{
							grcBegin(drawLines, 2);
							grcVertex3f(adjustedVerts[neighbor]);
							grcVertex3f(adjustedVerts[(neighbor + 1)%3]);
							grcEnd();
						}
					}

					grcLighting(oldLighting);
				}
				else
				{
					// Draw in wireframe mode.
					Color32 newColor(oldColor);
					if (highlightFlags & polyFlags)
					{
						newColor = MATERIALMGR.GetDebugColor(polyMaterialID, polyFlags, highlightFlags);
					}
					else if (PFD_Quads.GetEnabled())
					{
						newColor = (numVertices == 3 ? Color_red : Color_green);
					}
					else if (colorMaterials)
					{
						newColor = MATERIALMGR.GetDebugColor(MATERIALMGR.GetMaterial(polyMaterialID)); 
					}
					else if (highlightFlags)
					{
						newColor = Color_grey90;
					}

					ScalarV polygonDistance = Min(Mag(center),maxDrawDistance);
					newColor.SetAlpha(int(255 * (1.0f - (1.0f-PFD_BoundDistanceOpacity.GetValue())*(polygonDistance*inverseDrawDistance).Getf())));
					grcColor(newColor);

					bool oldLighting = grcLighting(false);
					if (PFD_MarginOriginalPolygons.GetEnabled() || (GetType() != GEOMETRY && GetType() != BOX))
					{
						grcBegin(drawLineStrip,numVertices + 1);
						grcVertex3f(adjustedVerts[0]);
						grcVertex3f(adjustedVerts[1]);
						grcVertex3f(adjustedVerts[2]);
						grcVertex3f(adjustedVerts[0]);
						grcEnd();
					}

					if (PFD_MarginShrunkPolygons.GetEnabled() && (GetType() == GEOMETRY || GetType() == BOX))
					{
						grcBegin(drawLineStrip,numVertices + 1);
						/// Vec3V normal(GetPolygonUnitNormal(polygonIndex));
						grcVertex3f(shrunkVertex0);
						grcVertex3f(shrunkVertex1);
						grcVertex3f(shrunkVertex2);
						grcVertex3f(shrunkVertex0);
						grcEnd();
					}
					grcLighting(oldLighting);
				}

				if (PFD_PolyNeighbors.Begin(true, false))
				{
					const phPolygon& poly1 = m_Polygons[polygonIndex];

					int numPolyVerts = POLY_MAX_VERTICES;
					for (int neighborIndex = 0; neighborIndex < numPolyVerts; ++neighborIndex)
					{
						int polyIndex2 = poly1.GetNeighboringPolyNum(neighborIndex);

						// We will see this edge twice, so arbitrarily pick the one where poly1 is greater than poly2 so we only
						// draw it once.
						if(polygonIndex > polyIndex2 || polyIndex2 == (u16)-1)
						{
							continue;
						}

						int polyVertexIdx2 = neighborIndex < numPolyVerts - 1 ? neighborIndex + 1 : 0;
						int boundVertexIdx1 = poly1.GetVertexIndex(neighborIndex);
						int boundVertexIdx2 = poly1.GetVertexIndex(polyVertexIdx2);
						Vec3V vertex1(GetVertex(boundVertexIdx1));
						Vec3V vertex2(GetVertex(boundVertexIdx2));
						Vec3V vecNormal1(GetPolygonUnitNormal(polygonIndex));
						Vec3V vecNormal2(GetPolygonUnitNormal(polyIndex2));
						Vec3V edge = vertex2 - vertex1;

						ScalarV STITCH_DRAW_IN_DISP(PFD_PolyNeighborsLength.GetValue());

						Vec3V start1;
						start1 = Cross(edge, vecNormal1);
						start1 = Normalize(start1);
						start1 = Scale(start1, STITCH_DRAW_IN_DISP);

						Vec3V start2;
						start2 = Cross(vecNormal2, edge);
						start2 = Normalize(start2);
						start2 = Scale(start2, STITCH_DRAW_IN_DISP);

						ScalarV STITCHES_PER_METER(PFD_PolyNeighborsDensity.GetValue());
						ScalarV edgeLength = Mag(edge);
						int numStitches = (int)(edgeLength * STITCHES_PER_METER).Getf();
						numStitches = Min(numStitches, grcBeginMax >> 2);
						numStitches = Max(numStitches, 5);

						grcBegin(drawLines, numStitches * 2);

						for (int stitchIndex = 0; stitchIndex < numStitches; ++stitchIndex)
						{
							ScalarV stitchT = ScalarV(float(stitchIndex) / float(numStitches));
							Vec3V stitchDelta;
							stitchDelta = Scale(edge, stitchT);

							Vec3V stitchCenter;
							stitchCenter = Add(vertex1, stitchDelta);

							Vec3V stitchEnd1;
							stitchEnd1 = Add(stitchCenter, start1);
							grcVertex3f(stitchEnd1);

							Vec3V stitchEnd2;
							stitchEnd2 = Add(stitchCenter, start2);
							grcVertex3f(stitchEnd2);
						}

						grcEnd();
					}

					PFD_PolyNeighbors.End();
				}
			}
			else
			{
				// Don't draw 
				bool drawPrimitive = ((whichPolys == ALL_POLYS) || ((whichPolys & RENDER_BIG_PRIMITIVES) && isBig)) && (!PFDGROUP_PolygonDensity.GetEnabled() || PFDGROUP_PrimitiveDensity.GetEnabled());
				if (!drawPrimitive)
				{
					continue;
				}
				
				Color32 primitiveColor = Color_white;
				if (isBig)
				{
					primitiveColor = Color_red;
				}
				else if(GetType() == phBound::BVH && PFDGROUP_PrimitiveDensity.GetEnabled())
				{
					primitiveColor = static_cast<const phBoundBVH*>(this)->ComputePrimitiveDensityColor((phPolygon::Index)polygonIndex,typeFilter&boundTypeFlags,includeFilter&boundIncludeFlags);
				}
				grcColor(primitiveColor);

				if(bvhPrimitive.GetType() == PRIM_TYPE_SPHERE)
				{
					Mat34V sphereMatrix;
					CREATE_SIMPLE_BOUND_ON_STACK(phBoundSphere,sphereBound);
					static_cast<const phBoundBVH*>(this)->ConstructBoundFromPrimitive(bvhPrimitive.GetSphere(),sphereBound,sphereMatrix);
					Transform(sphereMatrix,mtx,sphereMatrix);
					ScalarV distToCenter = Dist(cameraPos2D, sphereMatrix.GetCol3()*vMask2D);
					ScalarV distToSphere = distToCenter - sphereBound.GetRadiusV(); 

					if (IsLessThanOrEqualAll(distToSphere * distToSphere, squaredDrawDistance))
					{
						grcDrawSphere(sphereBound.GetRadius(), sphereMatrix.GetCol3(), 20, true, solid);
						DrawInternal(polygonIndex, sphereMatrix.GetCol3());

						// grcDrawSphere over-writes this so we need to restore it.
						grcWorldMtx(mtx);
					}
				}
				else if(bvhPrimitive.GetType() == PRIM_TYPE_CAPSULE)
				{
					Mat34V capsuleMatrix;
					CREATE_SIMPLE_BOUND_ON_STACK(phBoundCapsule,capsuleBound);
					static_cast<const phBoundBVH*>(this)->ConstructBoundFromPrimitive(bvhPrimitive.GetCapsule(),capsuleBound,capsuleMatrix);
					Transform(capsuleMatrix,mtx,capsuleMatrix);
					Vec3V p0 = SubtractScaled(capsuleMatrix.GetCol3(),capsuleMatrix.GetCol1(),capsuleBound.GetHalfLengthV())*vMask2D;
					Vec3V segment = Scale(capsuleMatrix.GetCol1(),capsuleBound.GetLengthV())*vMask2D;
					ScalarV segDist = ScalarVFromF32(geomDistances::DistanceLineToPoint(RCC_VECTOR3(p0),RCC_VECTOR3(segment),RCC_VECTOR3(cameraPos2D)));
					if(IsLessThanAll(Subtract(segDist,capsuleBound.GetRadiusV()),squaredDrawDistance))
					{
						grcDrawCapsule(capsuleBound.GetLength(), capsuleBound.GetRadius(), RCC_MATRIX34(capsuleMatrix), 20, solid);
						DrawInternal(polygonIndex, capsuleMatrix.GetCol3());
						// grcDrawCapsule over-writes this so we need to restore it.
						grcWorldMtx(mtx);
					}
				}
				else if(bvhPrimitive.GetType() == PRIM_TYPE_BOX)
				{
					Mat34V boxMatrix;
					CREATE_SIMPLE_BOUND_ON_STACK(phBoundBox,boxBound);
					static_cast<const phBoundBVH*>(this)->ConstructBoundFromPrimitive(bvhPrimitive.GetBox(),boxBound,boxMatrix);
					Transform(boxMatrix,mtx,boxMatrix);

					ScalarV distToVert0V = DistSquared(cameraPos2D, Transform(boxMatrix,SelectFT(VecBoolV(V_T_T_T_T),boxBound.GetBoundingBoxMin(),boxBound.GetBoundingBoxMax()))*vMask2D);
					ScalarV distToVert1V = DistSquared(cameraPos2D, Transform(boxMatrix,SelectFT(VecBoolV(V_F_T_F_T),boxBound.GetBoundingBoxMin(),boxBound.GetBoundingBoxMax()))*vMask2D);
					ScalarV distToVert2V = DistSquared(cameraPos2D, Transform(boxMatrix,SelectFT(VecBoolV(V_F_F_T_T),boxBound.GetBoundingBoxMin(),boxBound.GetBoundingBoxMax()))*vMask2D);
					ScalarV distToVert3V = DistSquared(cameraPos2D, Transform(boxMatrix,SelectFT(VecBoolV(V_T_F_F_T),boxBound.GetBoundingBoxMin(),boxBound.GetBoundingBoxMax()))*vMask2D);
					ScalarV distToBox = Min(distToVert0V,distToVert1V,distToVert2V,distToVert3V);
					if (IsLessThanOrEqualAll(distToBox, squaredDrawDistance))
					{
						if(solid)
						{
							grcDrawSolidBox(VEC3V_TO_VECTOR3(boxBound.GetBoxSize()), RCC_MATRIX34(boxMatrix), primitiveColor);
						}
						else
						{
							grcDrawBox(VEC3V_TO_VECTOR3(boxBound.GetBoxSize()), RCC_MATRIX34(boxMatrix), primitiveColor);
						}

						DrawInternal(polygonIndex, boxMatrix.GetCol3());

						// grcDrawBox over-writes this so we need to restore it.
						grcWorldMtx(mtx);
					}
				}
				else
				{
					Mat34V cylinderMatrix;
					CREATE_SIMPLE_BOUND_ON_STACK(phBoundCylinder,cylinderBound);
					static_cast<const phBoundBVH*>(this)->ConstructBoundFromPrimitive(bvhPrimitive.GetCylinder(),cylinderBound,cylinderMatrix);
					Transform(cylinderMatrix,mtx,cylinderMatrix);
					Vec3V p0 = SubtractScaled(cylinderMatrix.GetCol3(),cylinderMatrix.GetCol1(),cylinderBound.GetHalfHeightV())*vMask2D;
					Vec3V segment = Scale(cylinderMatrix.GetCol1(),Max(cylinderBound.GetHeightV(),ScalarV(V_FLT_SMALL_6)))*vMask2D;
					ScalarV segDist = ScalarVFromF32(geomDistances::DistanceLineToPoint(RCC_VECTOR3(p0),RCC_VECTOR3(segment),RCC_VECTOR3(cameraPos2D)));
					if(IsLessThanAll(Subtract(segDist,cylinderBound.GetRadiusV()),squaredDrawDistance))
					{
						grcDrawCylinder(cylinderBound.GetHeight(), cylinderBound.GetRadius(), RCC_MATRIX34(cylinderMatrix), 20, solid);
						DrawInternal(polygonIndex, cylinderMatrix.GetCol3());
						// grcDrawCylinder over-writes this so we need to restore it.
						grcWorldMtx(mtx);
					}
				}
			}
		}
	}
	else
	{
		// We're a box!  Let's just draw a box.
		Vec3V boundingBoxSize = GetBoundingBoxSize();
		grcDrawBox(RCC_VECTOR3(boundingBoxSize), RCC_MATRIX34(mtx), Color_white);
	}

    if (solid && (GetType() == GEOMETRY || GetType() == BOX))
    {
        const int MAX_NUM_ROUND_VERTS = 128;
        Vec3V vertexNormals[MAX_NUM_ROUND_VERTS];

        if (PFD_MarginExpandedShrunkRoundCorners.GetEnabled() && PFD_MarginExpandedShrunkRoundEdges.GetEnabled() && GetNumVertices() < MAX_NUM_ROUND_VERTS)
        {
            ComputeVertexNormals(vertexNormals);
        }

#if COMPRESSED_VERTEX_METHOD == 0
        if (PFD_MarginExpandedShrunkRoundEdges.GetEnabled() && shrunkVertices != NULL)
#else
		if (PFD_MarginExpandedShrunkRoundEdges.GetEnabled() && compressedVerticesToDraw != NULL)
#endif
        {
            grcWorldMtx(mtx);
            Color32 newColor = oldColor;
            newColor.SetAlpha(int(255 * PFD_BoundSolidOpacity.GetValue()));
            grcBindTexture(NULL);
            grcColor(newColor);

            // Iterate over every edge
            for (int polyIndex1=0; polyIndex1<m_NumPolygons; polyIndex1++)
            {
                const phPolygon& poly1 = m_Polygons[polyIndex1];

                int numPolyVerts = POLY_MAX_VERTICES;
                for (int neighborIndex = 0; neighborIndex < numPolyVerts; ++neighborIndex)
                {
                    int polyIndex2 = poly1.GetNeighboringPolyNum(neighborIndex);

                    // We will see this edge twice, so arbitrarily pick the one where poly1 is greater than poly2 so we only draw it once.
                    if(polyIndex1 > polyIndex2 || polyIndex2 == (u16)-1)
                    {
                        continue;
                    }

                    int polyVertexIdx2 = neighborIndex < numPolyVerts - 1 ? neighborIndex + 1 : 0;
                    int boundVertexIdx1 = poly1.GetVertexIndex(neighborIndex);
                    int boundVertexIdx2 = poly1.GetVertexIndex(polyVertexIdx2);
#if COMPRESSED_VERTEX_METHOD == 0
                    Vec3V vertex1(shrunkVertices[boundVertexIdx1]);
                    Vec3V vertex2(shrunkVertices[boundVertexIdx2]);
#else
					Vec3V vertex1(DecompressVertex(compressedVerticesToDraw + 3 * boundVertexIdx1));
					Vec3V vertex2(DecompressVertex(compressedVerticesToDraw + 3 * boundVertexIdx2));
#endif
                    Vec3V vecNormal1(GetPolygonUnitNormal(polyIndex1));
                    Vec3V vecNormal2(GetPolygonUnitNormal(polyIndex2));

                    // Draw a ribbon of parallel quads in an arc, to connect the two edges and smooth the normals
                    const int ROUND_EDGE_TESSELLATION = 4;
                    grcBegin(drawTriStrip, 2 * (ROUND_EDGE_TESSELLATION + 1));
                    for (int tess = 0; tess <= ROUND_EDGE_TESSELLATION; ++tess)
                    {
                        Vec3V corner0, corner1, corner2, corner3;

                        ScalarV ratio = ScalarV(float(tess) / float(ROUND_EDGE_TESSELLATION));

                        Vec3V normal;
                        normal = Lerp(ratio, vecNormal1, vecNormal2);
                        normal = Normalize(normal);

                        corner0 = AddScaled(vertex1, normal, GetMarginV());
                        corner1 = AddScaled(vertex2, normal, GetMarginV());

                        grcNormal3f(normal);
                        grcVertex3f(corner1);
                        grcVertex3f(corner0);
                    }
                    grcEnd();

                    if (PFD_MarginExpandedShrunkRoundCorners.GetEnabled() && GetNumVertices() < MAX_NUM_ROUND_VERTS)
                    {
                        // For corners, draw a triangle fan at each end of each edge
                        grcBegin(drawTris, (ROUND_EDGE_TESSELLATION * 3));

						Vec3V corner[ROUND_EDGE_TESSELLATION * 3];
						Vec3V normal[ROUND_EDGE_TESSELLATION * 3];
						normal[0] = vertexNormals[boundVertexIdx1];

                        Vec3V displacedVertexA;
#if COMPRESSED_VERTEX_METHOD == 0
                        corner[0] = AddScaled(shrunkVertices[boundVertexIdx1], vertexNormals[boundVertexIdx1], GetMarginV());
#else
						corner[0] = AddScaled(DecompressVertex(compressedVerticesToDraw + 3 * boundVertexIdx1), vertexNormals[boundVertexIdx1], GetMarginV());
#endif

                        for (int tess = ROUND_EDGE_TESSELLATION; tess >= 0; --tess)
                        {
                            ScalarV ratio = ScalarV(float(tess) / float(ROUND_EDGE_TESSELLATION));

                            normal[tess + 1] = Lerp(ratio, vecNormal1, vecNormal2);
                            normal[tess + 1] = Normalize(normal[tess + 1]);

                            corner[tess + 1] = AddScaled(vertex1, normal[tess + 1], GetMarginV());
                        }

						for (int tri = 0; tri < ROUND_EDGE_TESSELLATION; tri++)
						{
                            grcNormal3f(normal[tri]);
                            grcVertex3f(corner[tri]);
                            grcNormal3f(normal[tri + 1]);
                            grcVertex3f(corner[tri + 1]);
                            grcNormal3f(normal[tri + 2]);
                            grcVertex3f(corner[tri + 2]);
						}

                        grcEnd();

                        grcBegin(drawTriFan, ROUND_EDGE_TESSELLATION + 2);

						normal[0] = vertexNormals[boundVertexIdx2];
#if COMPRESSED_VERTEX_METHOD == 0
                        corner[0] = AddScaled(shrunkVertices[boundVertexIdx2], vertexNormals[boundVertexIdx2], GetMarginV());
#else
						corner[0] = AddScaled(DecompressVertex(compressedVerticesToDraw + 3 * boundVertexIdx2), vertexNormals[boundVertexIdx2], GetMarginV());
#endif

                        for (int tess = 0; tess <= ROUND_EDGE_TESSELLATION; ++tess)
                        {
                            ScalarV ratio = ScalarV(float(tess) / float(ROUND_EDGE_TESSELLATION));

                            normal[tess + 1] = Lerp(ratio, vecNormal1, vecNormal2);
                            normal[tess + 1] = Normalize(normal[tess + 1]);

                            corner[tess + 1] = AddScaled(vertex2, normal[tess + 1], GetMarginV());
                        }

						for (int tri = 0; tri < ROUND_EDGE_TESSELLATION; tri++)
						{
                            grcNormal3f(normal[tri]);
                            grcVertex3f(corner[tri]);
                            grcNormal3f(normal[tri + 1]);
                            grcVertex3f(corner[tri + 1]);
                            grcNormal3f(normal[tri + 2]);
                            grcVertex3f(corner[tri + 2]);
						}

                        grcEnd();
                    }
                }
            }
        }
    }

	grcColor(Color_white);

	if ((PFD_BvhVertexIndices.WillDraw() && GetType() == phBound::BVH) || (PFD_NonBvhVertexIndices.WillDraw() && GetType() != phBound::BVH))
	{
		Vec3V vertexPos;
		for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
		{
			// Write the vertex index number on the screen at the vertex location.
			vertexPos = Transform(mtx, GetVertex(vertexIndex));
			if (grcViewport::GetCurrent() && IsGreaterThanAll(DistSquared(vertexPos, grcViewport::GetCurrentCameraPosition()), squaredDrawDistance))
			{
				continue;
			}

			char vertexIndexText[8];
			vertexIndexText[7] = '\0';
			formatf(vertexIndexText,7,"%i",vertexIndex);
			grcDrawLabelf(RCC_VECTOR3(vertexPos),vertexIndexText);
		}
	}

#if 0
	// Debug drawing for the points on the convex hull.  Disabled right now because I have the convex hull vertex sorting code disabled and I haven't
	//   hooked this up to a widget.
	if(GetType() != phBound::BVH)
	{
		Vec3V vertexPos;
		for (int vertexIndex=0; vertexIndex<m_NumConvexHullVertices; vertexIndex++)
		{
			// Write the vertex index number on the screen at the vertex location.
			mtx.Transform(GetVertex(vertexIndex),vertexPos);
			if (grcViewport::GetCurrent() && vertexPos.Dist2(grcViewport::GetCurrentCameraPosition()) > squaredDrawDistance)
			{
				continue;
			}

			grcDrawSphere(0.04f, vertexPos, 8, false, true);
		}
#if 0
		for(int vertexIndex=m_NumConvexHullVertices; vertexIndex<m_NumVertices; vertexIndex++)
		{
			// Write the vertex index number on the screen at the vertex location.
			mtx.Transform(GetVertex(vertexIndex),vertexPos);
			if (grcViewport::GetCurrent() && vertexPos.Dist2(grcViewport::GetCurrentCameraPosition()) > squaredDrawDistance)
			{
				continue;
			}

			grcDrawSphere(0.05f, vertexPos, 8, false, true);
		}
#endif
#if SORT_DEBUGGING
		mtx.Transform(s_LastVertexAdded, vertexPos);
		grcColor(Color_DarkGoldenrod);
		grcDrawSphere(0.07f, vertexPos, 8, false, true);

		grcWorldMtx(mtx);
		grcColor(Color_RoyalBlue);
		grcBegin(drawTri, 3);
		grcVertex3f(s_TriVert0);
		grcVertex3f(s_TriVert1);
		grcVertex3f(s_TriVert2);
		grcEnd();

		// Make the arrow end one meter out along the polygon normal.
		Vec3V arrowEnd(s_TriVert0);
		arrowEnd.AddScaled(s_PolyNormal, 1.0f);
		//mtx.Transform(arrowEnd);

		// Draw a 1m long arrow from the average position of the polygon's vertices, 1m out along the normal.
		pfDrawArrow(s_TriVert0, arrowEnd);

//		Vec3V v0, v1, v2;
//		mtx.Transform(s_TriVert0, v0);
//		mtx.Transform(s_TriVert1, v1);
//		mtx.Transform(s_TriVert2, v2);
//		grcDrawSphere(0.05f, v0, 8, false, true);
//		grcDrawSphere(0.05f, v1, 8, false, true);
//		grcDrawSphere(0.05f, v2, 8, false, true);
#endif
	}
#endif

	grcColor(oldColor);
}


void phBoundPolyhedron::DrawNormals(Mat34V_In mtxIn, int normalType, int whichPolys, float length, unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter)) const
{
	Mat34V_ConstRef mtx = mtxIn;
	grcWorldMtx(mtx);
	Vec3V cameraDirection;
	float squaredDrawDistance = square(PFD_BoundDrawDistance.GetValue());
	ScalarV thinPolyTolerance = Expt(ScalarV(PFD_ThinPolyTolerance.GetValue()) / ScalarV(V_LOG2_TO_LOG10));

	bool sphereCullPolys = (PFD_SphereCull.GetEnabled() && GetType() == phBound::BVH);
	phBoundCuller culler;
	int maxNumCulledPrimitives = 0; 
	phPolygon::Index* culledPrimitiveIndices = NULL;
	if(sphereCullPolys)
	{
		maxNumCulledPrimitives = Min(DEFAULT_MAX_CULLED_POLYS,GetNumPolygons());
		culledPrimitiveIndices = Alloca(phPolygon::Index,maxNumCulledPrimitives);
		culler.SetArrays(culledPrimitiveIndices,maxNumCulledPrimitives);
		CullSpherePolys(culler,UnTransformOrtho(mtxIn,grcViewport::GetCurrentCameraPosition()),ScalarVFromF32(PFD_BoundDrawDistance.GetValue()));
	}

	int numCulledPrimitives = sphereCullPolys ? culler.GetNumCulledPolygons() : GetNumPolygons();
	for(int culledPrimitiveIndexIndex = 0; culledPrimitiveIndexIndex < numCulledPrimitives; ++culledPrimitiveIndexIndex)
	{
		int polyIndex1 = sphereCullPolys ? culler.GetCulledPolygonIndex(culledPrimitiveIndexIndex) : culledPrimitiveIndexIndex;
		const phPolygon& poly = m_Polygons[polyIndex1];

		if (whichPolys)
		{
			bool thinPoly = (whichPolys & RENDER_THIN_POLYS) && poly.IsThin(this, thinPolyTolerance);
			bool badNormalPoly = (whichPolys & RENDER_BAD_NORMAL_POLYS) && poly.HasBadNormal(this);
			bool badNeighborPoly = (whichPolys & RENDER_BAD_NEIGHBOR_POLYS) && poly.HasBadNeighbors(this);

			if (!thinPoly && !badNormalPoly && !badNeighborPoly)
			{
				continue;
			}
		}

		if(poly.GetArea() <= 0.0f)
		{
			// Degenerate triangle ... don't try and do anything that involves a normal with it.
			continue;
		}

		const rage::phPrimitive& prim = reinterpret_cast<const rage::phPrimitive&>(poly);
		if (prim.GetType() != PRIM_TYPE_POLYGON)
		{
			continue;
		}

		Vec3V normal;
		normal = Transform3x3(mtx, GetPolygonUnitNormal(polyIndex1));

		bool vertsInInterest = false;
		for (int vertex = 0; vertex < POLY_MAX_VERTICES && !vertsInInterest; ++vertex)
		{
			cameraDirection = Transform(mtx ,GetVertex(poly.GetVertexIndex(vertex)));
			if (!grcViewport::GetCurrent() || DistSquared(cameraDirection, grcViewport::GetCurrentCameraPosition()).Getf() < squaredDrawDistance)
			{
				vertsInInterest = true;
			}
		}

		if (!vertsInInterest)
		{
			continue;
		}

		if (normalType == FACE_NORMALS)
		{
			// Find the average position of the polygon's vertices.
			Vec3V center(GetVertex(poly.GetVertexIndex(0)));
			center = Add(center, GetVertex(poly.GetVertexIndex(1)));
			center = Add(center, GetVertex(poly.GetVertexIndex(2)));
			center = Scale(center, ScalarV(V_THIRD));

			// Make the arrow end one meter out along the polygon normal.
			Vec3V arrowEnd(center);
			arrowEnd = AddScaled(arrowEnd, GetPolygonUnitNormal(polyIndex1), ScalarV(length));

			// Draw a 1m long arrow from the average position of the polygon's vertices, 1m out along the normal.
			pfDrawArrow(RCC_VECTOR3(center), RCC_VECTOR3(arrowEnd));
		}
		else
		{
			Assert(normalType == EDGE_NORMALS);

			const phPolygon *poly1 = &GetPolygon(polyIndex1);
			int neighborIndex;
			int polyVertexCnt = POLY_MAX_VERTICES;
			for(neighborIndex = polyVertexCnt - 1; neighborIndex >= 0; neighborIndex--)
			{
				int polyIndex2 = poly1->GetNeighboringPolyNum(neighborIndex);
				if((polyIndex1 > polyIndex2))
				{
					continue;
				}

				// Find the direction of the normal.
				Vec3V vecNormal(GetPolygonUnitNormal(polyIndex1));
				if(polyIndex2 != (u16)(-1))
				{
					Vec3V vecTemp(GetPolygonUnitNormal(polyIndex2));
					vecNormal = Add(vecNormal, vecTemp);
					vecNormal = NormalizeSafe(vecNormal, Vec3V(V_X_AXIS_WZERO));
				}

				// Find the center of the edge.
				Vec3V vecCenter;
				int nPolyVertexIdx1 = neighborIndex;
				int nPolyVertexIdx2 = nPolyVertexIdx1 < polyVertexCnt - 1 ? nPolyVertexIdx1 + 1 : 0;
				int nBoundVertexIdx1 = poly1->GetVertexIndex(nPolyVertexIdx1);
				int nBoundVertexIdx2 = poly1->GetVertexIndex(nPolyVertexIdx2);
				vecCenter = Average(GetVertex(nBoundVertexIdx1), GetVertex(nBoundVertexIdx2));

				vecNormal = Scale(vecNormal, ScalarV(length));
				vecNormal = Add(vecNormal, vecCenter);
				pfDrawArrow(RCC_VECTOR3(vecCenter), RCC_VECTOR3(vecNormal));
			}
		}
	}
}

#endif	// end of #if __PFDRAW

#if __NMDRAW
void phBoundPolyhedron::NMRender(const Matrix34& mtx) const
{
	phBound::NMRender(mtx);

	if (m_Vertices==NULL)
		return;

	Vec3V colour(1, 0, 1);
	for (int i=0; i<m_NumPolygons; i++)
	{
		if (m_Polygons[i].HasBadNormal())
		{
		}
		int numVerts = m_Polygons[i].GetNumVerts();

		NMRenderBuffer::getInstance()->addLineAndTransform(
			NMDRAW_BOUNDS, 
			mtx,
			m_Vertices[m_Polygons[i].GetVertexIndex(0)], 
			m_Vertices[m_Polygons[i].GetVertexIndex(1)], 
			colour);

		NMRenderBuffer::getInstance()->addLineAndTransform(
			NMDRAW_BOUNDS, 
			mtx,
			m_Vertices[m_Polygons[i].GetVertexIndex(1)], 
			m_Vertices[m_Polygons[i].GetVertexIndex(2)], 
			colour);

		NMRenderBuffer::getInstance()->addLineAndTransform(
			NMDRAW_BOUNDS, 
			mtx,
			m_Vertices[m_Polygons[i].GetVertexIndex(2)], 
			m_Vertices[m_Polygons[i].GetVertexIndex(0)], 
			colour);
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // !__SPU

bool phBoundPolyhedron::TestVertexToMovingSphere (Vec3V_In axisPoint, Vec3V_In unitAxis, ScalarV_In radius2, Vec3V_In edgePoint, Vec3V_In edge)
{
	// axisToEdge = vector from axisPoint to edgePoint - initial sphere position to edge vertex 0
	Vec3V axisToEdge(edgePoint);
	axisToEdge = Subtract(axisToEdge, axisPoint);
	// perpAxisToEdge = vector representing initial separation of axisPoint from edgePoint, *perpendicular* to the direction of motion
	Vec3V perpAxisToEdge(axisToEdge);
	perpAxisToEdge = SubtractScaled(perpAxisToEdge, unitAxis, Dot(axisToEdge, unitAxis));
	// So distance is going to be the difference of two squares - I guess the specifics there don't really matter since we're just comparing that
	//   result against zero
	ScalarV distance;
	distance = Subtract(radius2,MagSquared(perpAxisToEdge));
	// So we're checking here if the initial separation, perpendicular to the direction of motion, is less than the radius?
	if(IsGreaterThan(distance, ScalarV(V_ZERO)).Getb())
	{
		distance = Dot(axisToEdge, unitAxis)-Sqrt(distance);
		// distance is now the distance along the axis that the sphere will travel until it makes first contact with the vertex
		if(IsGreaterThan(distance, ScalarV(V_ZERO)).Getb())
		{
			// This code is checking to see if the sphere center is 'outside' of the edge (on the other of the vertex from the other vertex
			//   in the edge)
			Vec3V sphereCenter(axisPoint);
			sphereCenter = AddScaled(sphereCenter, unitAxis,distance);
			axisToEdge = Subtract(edgePoint,sphereCenter);
			return IsGreaterThan(Dot(axisToEdge, edge), ScalarV(V_ZERO)).Getb();
		}
	}

	return false;
}


__forceinline int RealQuadratic (ScalarV_In a1Param, ScalarV_In a0Param, ScalarV_InOut solution1, ScalarV_InOut solution2, ScalarV_In toleranceParam)
{
	ScalarV b2Minus4ac = Subtract(Scale(a1Param,a1Param),Scale(a0Param,ScalarV(V_FOUR)));
	ScalarV maxAbs = Max(Abs(a1Param),Abs(a0Param));
	ScalarV nearlyZero = Scale(toleranceParam,maxAbs);
	if (IsGreaterThanAll(b2Minus4ac,nearlyZero))
	{
		// The quantity in the square root is sufficiently positive, so there are exactly two distinct real solutions.
		ScalarV sqrtb2Minus4ac = Sqrt(b2Minus4ac);
		ScalarV b = SelectFT(IsGreaterThan(a1Param,ScalarV(V_ZERO)),-sqrtb2Minus4ac, sqrtb2Minus4ac);
		ScalarV q = -Scale(Add(a1Param,b),ScalarV(V_HALF));
		solution1 = q;
		solution2 = Scale(a0Param,Invert(q));

		// Return 2 to indicate there are 2 distinct real solutions.
		return 2;
	}

	if (IsGreaterThanOrEqualAll(b2Minus4ac,ScalarV(V_ZERO)))
	{
		// The quantity in the square root is nearly zero, so there is exactly one distinct real solution.
		solution1 = -Scale(a1Param,ScalarV(V_HALF));

		// Return 1 to indicate there is only 1 distinct real solution.
		return 1;
	}

	// The quantity inside the square root is negative, so there are no real solutions.
	return 0;
}

// Find the first intersection between an edge and a sphere moving from axisPoint along the given axis.
bool phBoundPolyhedron::EdgeToMovingSphereIntersection (Vec3V_In _axisPoint, Vec3V_In _axis, Vec3V_In _axisEndPos, Vec3V_In _edgePoint,
														Vec3V_In _edgeParam, ScalarV_In _radius, ScalarV_In _axisLengthParam, ScalarV_InOut isectDepth, Vec3V_Ref _isectPosition,
														Vec3V_Ref _isectNormal, Vec3V_In _normal1, const Vec3V* _normal2, ScalarV_In _edgeCosine)
{
	// Create a bunch of commonly-used constants up-front.
	const ScalarV vsZero(V_ZERO);
	const ScalarV vsSmallFloat(V_FLT_SMALL_6);
	const ScalarV vsOne(V_ONE);
	const ScalarV vsHalf(V_HALF);

	ScalarV vsIsectDepth = isectDepth;
	// This is solved with parametric equations for the position on the edge where the moving sphere first
	// hits it, and the simultaneous position of the sphere's center.  The distance between those points is
	// the sphere radius, and if the moving sphere touches the edge and does not hit an edge end point first,
	// then the vector between those points is perpendicular to the edge.  Those two equations are enough
	// to derive the quadratic that is solved here.  If there is no solution, then both edge end points
	// are checked for intersections with the moving sphere.
	const ScalarV radius2 = (_radius * _radius);
	const ScalarV axisLength = _axisLengthParam;
	const Vec3V edge = _edgeParam;
	if(IsGreaterThanAll(axisLength, vsSmallFloat))
	{
		const ScalarV edgeLength2 = MagSquared(edge);
		if(IsGreaterThanAll(edgeLength2, vsZero))
		{
			const ScalarV edgeDotAxis = Dot(edge, _axis);
			const Vec3V axisToEdge = _edgePoint - _axisPoint;
			const ScalarV a = edgeLength2 * Scale(axisLength,axisLength) - Scale(edgeDotAxis,edgeDotAxis);
			const ScalarV absA = Abs(a);
			if(IsGreaterThanAll(absA, vsSmallFloat))
			{
				const ScalarV inverseA = Invert(a);
				const ScalarV edgeDotAxisToEdge = Dot(edge, axisToEdge);
				ScalarV b = edgeDotAxisToEdge * edgeDotAxis - edgeLength2 * Dot(axisToEdge, _axis);
				b = Add(b, b);
				const ScalarV c = edgeLength2 * (MagSquared(axisToEdge) - radius2) - square(edgeDotAxisToEdge);
				ScalarV axisT, axisT2=ScalarV(V_FLT_MAX);
				int numHits = RealQuadratic(b*inverseA,c*inverseA,axisT,axisT2,vsSmallFloat);
				if(numHits>0)
				{
 					// There is at least one real solution to the quadratic, so the sphere touches the edge along the axis line.
					// Take the solution that has the lowest non-negative t-value, or the hightest t-value if both are negative.
					// NOTE: This isn't true (nor did it need to be before) because we just care about the smallest solution in the range of [0, 1].
					axisT = Min(axisT, axisT2);

					{
						// The sphere touches the edge along the axis.
						const ScalarV vsNewDepth = axisLength * (vsOne - axisT);
						// For long edges connecting parallel polygons, this test passes a little too often because
						// of roundoff errors in RealQuadratic.

						// This code checks that the new impact will be deeper than the previous deepest and that it didn't occur in the past (with a t-value
						//   < 0.0 - in terms of depth that means that the new depth is < axisLength).
						const ScalarV vsMinDepth = Max(vsZero, vsIsectDepth);
						const ScalarV vsMaxDepth = axisLength;
						const ScalarV vsDepthRangeMidpoint = vsHalf * Add(vsMinDepth, vsMaxDepth);
						const ScalarV vsDepthRangeHalfWidth = vsHalf * Subtract(vsMaxDepth, vsMinDepth);
						const ScalarV vsAbsNewDepthMinusDepthRangeMidpoint = Abs(Subtract(vsNewDepth, vsDepthRangeMidpoint));
						if(IsLessThanAll(vsAbsNewDepthMinusDepthRangeMidpoint, vsDepthRangeHalfWidth))
						{
							// These asserts are to test the correctness of the above condition but shouldn't be necessary in the day-to-day running of this code.
							//Assert(IsLessThanAll(axisT, vsOne - vsIsectDepth * Invert(axisLength)));
							//Assert(axisT.Getf() >= 0.0f);
							//Assert(axisT.Getf() <= 1.0f);
							//Assert(vsNewDepth.Getf() > vsIsectDepth.Getf());
							// This intersection will be the deepest so far.
							const Vec3V sphereCenter = _axisPoint + _axis * axisT;

							const Vec3V modifiedAxisToEdge = Subtract(_edgePoint, sphereCenter);
							const ScalarV edgeT = Negate(Dot(edge, modifiedAxisToEdge) * Invert(edgeLength2));

							// This is just a faster way of determining if edgeT is in the range [0, 1] that only uses one comparison.
							const ScalarV absEdgeTMinusHalf = Abs(edgeT - vsHalf);
							if(IsLessThanOrEqualAll(absEdgeTMinusHalf, vsHalf))
							{
								// The moving sphere hits the edge.
								const Vec3V isectPos = AddScaled(_edgePoint, edge, edgeT);
								_isectPosition = isectPos;
								const Vec3V isectNorm = NormalizeFast(Subtract(sphereCenter, isectPos));
								_isectNormal = isectNorm;
								Vec3V edgeNormal = _normal1;
								if(_normal2)
								{
									edgeNormal = Add(edgeNormal, *_normal2);
								}

								if (IsLessThanAll(Dot(edgeNormal, _axis), vsZero) 
									|| (_normal2 ? geomPoints::IsPointInWedge(RCC_VECTOR3(isectNorm),RCC_VECTOR3(_normal1),RCC_VECTOR3(*_normal2),NULL) : IsLessThanAll(Dot(isectNorm, edgeNormal), _edgeCosine)))
								{
									// The moving sphere hit the edge from an exterior angle.
									isectDepth = vsNewDepth;
									return true;
								}

								// The moving sphere hit the edge from an interior angle.
								return false;
							}
						}
					}
				}
			}
		}

		// Either the axis and the edge are parallel (a==0.0f or edgelength==0.0f) or the moving sphere does not
		// touch the edge, so see if the moving sphere touches an edge end point.
		const Vec3V unitAxis = NormalizeFast(_axis);
		if (TestVertexToMovingSphere(_axisPoint,unitAxis,radius2,_edgePoint,edge))
		{
			return VertexToMovingSphereIntersection(_axisPoint,unitAxis,_axisEndPos,radius2,_edgePoint,isectDepth,_isectPosition,_isectNormal);
		}

		const Vec3V edgeEnd = Add(_edgePoint, edge);
		const Vec3V reversedEdge = Negate(edge);
		if(TestVertexToMovingSphere(_axisPoint,unitAxis,radius2,edgeEnd,reversedEdge))
		{
			return VertexToMovingSphereIntersection(_axisPoint,unitAxis,_axisEndPos,radius2,edgeEnd,isectDepth,_isectPosition,_isectNormal);
		}
	}

	return false;
}


// Find the first intersection between a vertex and a sphere moving from axisPoint along the given axis.
bool phBoundPolyhedron::VertexToMovingSphereIntersection (Vec3V_In axisPoint, Vec3V_In unitAxis, Vec3V_In axisEndPos, ScalarV_In radius2,
														  Vec3V_In vertex, ScalarV_InOut depth, Vec3V_InOut position, Vec3V_InOut normal)
{
	Vec3V axisToVert(vertex);
	axisToVert = Subtract(axisToVert, axisPoint);
	ScalarV distance=Dot(axisToVert, unitAxis);
	if(IsGreaterThanAll(distance, ScalarV(V_ZERO)))
	{
		// The vertex is in front of the axis starting point.
		Vec3V axisToVertPerp(axisToVert);
		axisToVertPerp = SubtractScaled(axisToVertPerp, unitAxis,distance);
		ScalarV perpDist2=MagSquared(axisToVertPerp);
		if(IsLessThanAll(perpDist2, radius2))
		{
			// The vertex is within a distance radius of the axis line.
			Vec3V vertToEnd(axisEndPos);
			vertToEnd = Subtract(vertToEnd ,vertex);
			ScalarV vertToEnd2 = MagSquared(vertToEnd);
			ScalarV impactDepth = radius2-perpDist2;
			impactDepth = Sqrt(impactDepth);
			if (IsGreaterThanAll(Dot(vertToEnd,unitAxis), ScalarV(V_ZERO)))
			{
				if(IsLessThanAll(perpDist2,vertToEnd2))
				{
					ScalarV addDepth(vertToEnd2-perpDist2);
					addDepth = Sqrt(addDepth);
					impactDepth += addDepth;
				}
			}
			else
			{
				// The vertex is beyond the end of the axis.
				if (IsGreaterThanAll(vertToEnd2,radius2))
				{
					// The vertex is not within the last sphere position.
					return false;
				}

				if(IsGreaterThanAll(vertToEnd2,perpDist2))
				{
					ScalarV subDepth(vertToEnd2-perpDist2);
					subDepth = Sqrt(subDepth);
					impactDepth -= subDepth;
				}
			}

			ScalarV newDepth = impactDepth;
			// This vertex hits the moving sphere.
			if(IsGreaterThanAll(newDepth, depth))
			{
				// This is the deepest impact so far.
				depth=newDepth;
				position = vertex;
				axisToVert = SubtractScaled(axisEndPos,unitAxis,impactDepth);
				Vec3V normalResult = axisToVert - position;
				Assert(IsTrue(MagSquared(normalResult) > ScalarV(V_ZERO)));
				normalResult = Normalize(normalResult);
				normal = normalResult;
				return true;
			}
		}
	}

	return false;
}


#if !__SPU

static const ScalarV skfSphereInflationFactor(1.0003f);
#if __ASSERT
static const ScalarV skfOOSphereInflationFactor = Invert(skfSphereInflationFactor);
#endif

void ShrinkBoundingSphereRecurse (Vec3V_InOut rvecCenter, ScalarV_InOut rfRadius, const Vec3V **const papvecPoints, unsigned int NumPointsToEnclose, unsigned int NumSurfacePoints)
{
	switch (NumSurfacePoints)
	{
		case 0:
		{
			// Don't do anything to the bounding sphere, we have no information!
			rfRadius = ScalarV(V_ZERO);
			rvecCenter = Vec3V(V_ZERO);
			break;
		}

		case 1:
		{
			rfRadius = ScalarV(V_ZERO);
			rvecCenter = *papvecPoints[-1];
			break;
		}

		case 2:
		{
			rvecCenter = Average(*papvecPoints[-1], *papvecPoints[-2]);
			rfRadius = ScalarV(V_HALF) * Dist(*papvecPoints[-1],*papvecPoints[-2]) * skfSphereInflationFactor;
			break;
		}

		case 3:
		{
			Vec3V a;
			a = Subtract(*papvecPoints[-2], *papvecPoints[-1]);
			Vec3V b;
			b = Subtract(*papvecPoints[-3], *papvecPoints[-1]);

			Vec3V vecACrossB;
			vecACrossB = Cross(a, b);
			ScalarV Denominator = ScalarV(V_TWO) * MagSquared(vecACrossB);
			const float QUITE_SMALL_FLOAT = 1.0e-9f;
			while(IsLessThanOrEqualAll(Abs(Denominator), ScalarV(QUITE_SMALL_FLOAT)))
			{
				// The three points are too close to being colinear, so make a perpendicular vector.
				Vec3V dummy;
				Vec3V ortho;
				if (IsGreaterThanAll(MagSquared(a), ScalarV(V_FLT_SMALL_12)))
				{
					ortho = Normalize(a);
					MakeOrthonormals(ortho, vecACrossB,dummy);
				}
				else
				{
					ortho = Normalize(b);
					MakeOrthonormals(ortho, vecACrossB,dummy);
				}

				Denominator = ScalarV(V_TWO) * MagSquared(vecACrossB);
			}
			Assert(IsGreaterThanAll(Abs(Denominator), ScalarV(QUITE_SMALL_FLOAT)));

			Vec3V vecACrossBCrossA;
			vecACrossBCrossA = Cross(vecACrossB, a);
			Vec3V vecBCrossACrossB;
			vecBCrossACrossB = Cross(b, vecACrossB);
			Vec3V o = (MagSquared(b) * (vecACrossBCrossA) + MagSquared(a) * (vecBCrossACrossB)) / Denominator;

			rfRadius = Mag(o) * skfSphereInflationFactor;
			rvecCenter = Add(*papvecPoints[-1], o);
			break;
		}

		default:
		{
			Assert(NumSurfacePoints == 4);
 			Vec3V a;
			a = Subtract(*papvecPoints[-2], *papvecPoints[-1]);
			Vec3V b;
			b = Subtract(*papvecPoints[-3], *papvecPoints[-1]);
			Vec3V c;
			c = Subtract(*papvecPoints[-4], *papvecPoints[-1]);

			// This prevents the while loop from infinitely looping
			// Was added by Luke, presumably to stop a crash in rage builder
			if(IsEqualAll(a,b))
			{
				a = Add(a, Vec3V(V_FLT_SMALL_6));
				b = Subtract(b, Vec3V(V_FLT_SMALL_6));
			}
			else if(IsEqualAll(a,c))
			{
				a = Add(a, Vec3V(V_FLT_SMALL_6));
				c = Subtract(c, Vec3V(V_FLT_SMALL_6));
			}
			else if(IsEqualAll(b,c))
			{
				b = Add(b, Vec3V(V_FLT_SMALL_6));
				c = Subtract(c, Vec3V(V_FLT_SMALL_6));
			}

			ScalarV Denominator = ScalarV(V_TWO) * (SplatX(a) * (SplatY(b) * SplatZ(c) - SplatZ(b) * SplatY(c)) - SplatX(b) * (SplatY(a) * SplatZ(c) - SplatZ(a) * SplatY(c)) + SplatX(c) * (SplatY(a) * SplatZ(b) - SplatZ(a) * SplatY(b)));
			while(IsLessThanOrEqualAll(Abs(Denominator), ScalarV(VERY_SMALL_FLOAT)))
			{
				// The four points are too close to being coplanar.  Let's slightly perturb one of them to avoid a bad situation.
				// In order to get a good perturbation, we perturb vertex 3 along the (non-unitized) normal of the plane containing vertices 0, 1 and 2.
				ScalarV kE(0.1f);
				Vec3V Perturbation(a);
				Perturbation = Cross(Perturbation, b);
				Perturbation = Scale(Perturbation, kE);
				c = Add(c, Perturbation);
				Denominator = ScalarV(V_TWO) * (SplatX(a) * (SplatY(b) * SplatZ(c) - SplatZ(b) * SplatY(c)) - SplatX(b) * (SplatY(a) * SplatZ(c) - SplatZ(a) * SplatY(c)) + SplatX(c) * (SplatY(a) * SplatZ(b) - SplatZ(a) * SplatY(b)));
			}
			Assert(IsGreaterThanAll(Abs(Denominator), ScalarV(VERY_SMALL_FLOAT)));

			Vec3V vecACrossB, vecCCrossA, vecBCrossC;
			vecACrossB = Cross(a, b);
			vecCCrossA = Cross(c, a);
			vecBCrossC = Cross(b, c);
			Vec3V o = (MagSquared(c) * (vecACrossB) +
				MagSquared(b) * (vecCCrossA) +
				MagSquared(a) * (vecBCrossC)) / Denominator;

			rfRadius = Mag(o) * skfSphereInflationFactor;
			rvecCenter = Add(*papvecPoints[-1], o);
			return;
		}
	}

	for(unsigned int i = 0; i < NumPointsToEnclose; i++)
	{
		ScalarV fDistToCenterSq = DistSquared(rvecCenter,*papvecPoints[i]);
		if(IsGreaterThanAll(fDistToCenterSq, square(rfRadius)))   // Signed square distance to sphere
		{
			// We've found a vertex that isn't contained within our best bounding sphere so far.  Let's shift that vertex down into position zero (remember that we have negative indices) 
			const Vec3V *pT = papvecPoints[i];
			for(unsigned int j = i; j > 0; j--)
			{
				papvecPoints[j] = papvecPoints[j - 1];
			}
			papvecPoints[0] = pT;

			// Our previous bounding sphere enclosed all the points up until this one, so we need to find a new bounding sphere.
			// Let's now try to construct the optimal bounding sphere, for the points that we have considered so far, that also includes this point as a surface point.
			ShrinkBoundingSphereRecurse(rvecCenter, rfRadius, papvecPoints + 1, i, NumSurfacePoints + 1);
		}
	}
}


void ShrinkBoundingSphere (Vec3V_InOut rvecCenter, ScalarV_InOut rfRadius, const Vec3V *const P, unsigned int p)
{
	const Vec3V **papkvecVertices = NULL;
	sysMemStartTemp();
	papkvecVertices = rage_new const Vec3V*[p];
	sysMemEndTemp();

	for(unsigned int i = 0; i < p; i++)
//		papkvecVertices[i] = &P[p - i - 1];
		papkvecVertices[i] = &P[i];

	ShrinkBoundingSphereRecurse(rvecCenter, rfRadius, papkvecVertices, p, 0);
//	rfRadius *= (1.0f / skfSphereInflationFactor);

	sysMemStartTemp();
	delete [] papkvecVertices;
	sysMemEndTemp();
}


void ShrinkBoundingSphere (Vec3V_InOut rvecCenter, ScalarV_InOut rfRadius, const Vec3V **const kpakpkvecPoints, const int knPointCnt)
{
	rvecCenter = Vec3V(V_ZERO);
	rfRadius = ScalarV(V_ZERO);
	ShrinkBoundingSphereRecurse(rvecCenter, rfRadius, kpakpkvecPoints, knPointCnt, 0);
	rfRadius *= Invert(skfSphereInflationFactor);
}


void phBoundPolyhedron::CalculateBoundingSphere(Vec3V_InOut rvecSphereCenter, ScalarV_InOut rfSphereRadius) const
{
#if COMPRESSED_VERTEX_METHOD == 0
	if(m_NumVertices > 0)
#else	// COMPRESSED_VERTEX_METHOD == 0
	Assert(IsFiniteAll(m_UnQuantizeFactor));
	if(m_NumVertices > 0 && IsFiniteAll(m_UnQuantizeFactor))
#endif	// COMPRESSED_VERTEX_METHOD == 0
	{
		// TODO: I don't think this code serves any purpose and should be removed.
		// The ordering here is x-min, x-max, y-min, y-max, z-min, z-max.
		int anExtremeCoords[6] = {0, 0, 0, 0, 0, 0};

		// To make our initial first guess, we make a pass through all of the vertices to find the minimums and maximums along
		//   each of the x, y, and z axes.
		int nVertexIdx;
		for(nVertexIdx = 1; nVertexIdx < m_NumVertices; ++nVertexIdx)
		{
			for(int nDim = 0; nDim < 3; ++nDim)
			{
				if(GetVertex(nVertexIdx)[nDim] < GetVertex(anExtremeCoords[2 * nDim])[nDim])
				{
					anExtremeCoords[2 * nDim] = nVertexIdx;
				}
				if(GetVertex(nVertexIdx)[nDim] > GetVertex(anExtremeCoords[2 * nDim + 1])[nDim])
				{
					anExtremeCoords[2 * nDim + 1] = nVertexIdx;
				}
			}
		}

		// Code inside "#if 0" to estimate the bounding sphere before starting ShrinkBoundingSphere was removed here, it can be retrieved from Perforce (28 Feb 2007).

#if 0
		// This is a little test code to test out ShrinkBoundingSphere!
		Vec3V Vertices[5];
		Vertices[0].Set(-1.0f, 0.0f, -1.0f);
		Vertices[1].Set(-1.0f, 0.0f, +1.0f);
		Vertices[2].Set(+1.0f, 0.0f, +1.0f);
		Vertices[3].Set(+1.0f, 0.0f, -1.0f);
		Vec3V Center;
		float Radius;
		ShrinkBoundingSphere(Center, Radius, Vertices, 4);
#endif

		const Vec3V *vertsToUse;
#if COMPRESSED_VERTEX_METHOD == 0
		{
			vertsToUse = m_Vertices;
		}
#endif
#if COMPRESSED_VERTEX_METHOD > 0
		{
			sysMemStartTemp();
			Vec3V *tempVertArray = rage_new Vec3V[m_NumConvexHullVertices];
			for(int vertIndex = 0; vertIndex < m_NumConvexHullVertices; ++vertIndex)
			{
				tempVertArray[vertIndex] = GetVertex(vertIndex);
			}
			vertsToUse = tempVertArray;
			sysMemEndTemp();
		}
#endif

		ShrinkBoundingSphere(rvecSphereCenter, rfSphereRadius, vertsToUse, m_NumConvexHullVertices);

#if COMPRESSED_VERTEX_METHOD > 0
		{
			sysMemStartTemp();
			delete [] vertsToUse;
			sysMemEndTemp();
		}
#endif

#if __ASSERT && !__TOOL
		{
			// At this point, we will iterate through all of the vertices to make sure that they're all within the bounding sphere.
			// Furthermore, to ensure that we at least have a reasonable sphere (it's not necessarily easy to ensure that it's the *minimal*
			//   bounding sphere) we check the number of points that are "on" the surface of the sphere.
			int NumSurfacePoints = 0;
			ScalarV fSphereRadiusSq = square(rfSphereRadius);

// NOTE: the following line is causing exception in debug PC (ragebuilder).
// compiler is generating code which is trying to load xmm register from unaligned address causing the exception
//			const ScalarV testSphereRadiusSq = square(ScalarV(0.9995f) * skfOOSphereInflationFactor) * fSphereRadiusSq;
// change budied by Russ S and Stephan B
			const ScalarV testSphereRadiusSq = ScalarV(0.9995f * 0.9995f) * skfOOSphereInflationFactor * skfOOSphereInflationFactor * fSphereRadiusSq;

			Vec3V vecTestPoint, vecCenterToTestPoint;
			for(nVertexIdx = 0; nVertexIdx < m_NumVertices; ++nVertexIdx)
			{
				vecTestPoint = GetVertex(nVertexIdx);
				vecCenterToTestPoint = Subtract(vecTestPoint, rvecSphereCenter);
				ScalarV fTempRadiusSq = MagSquared(vecCenterToTestPoint);
				const ScalarV kNumericalAccuracyFudge(1.02f);
				Assert(IsLessThanOrEqualAll(fTempRadiusSq, square(skfSphereInflationFactor) * kNumericalAccuracyFudge *fSphereRadiusSq));

				if(IsGreaterThanOrEqualAll(fTempRadiusSq*kNumericalAccuracyFudge, testSphereRadiusSq))
				{
					++NumSurfacePoints;
				}
			}

			// If this asserts, then fewer than two of the model's vertices lie on the surface of the bounding sphere.  The means that the bounding
			//   sphere is not optimal.  This is probably due to numeric imprecision in handling the coplanar or colinear cases.
			// We would expect NumSurfacePoints to be at least three for a "typical" model, often four.
			Assert(NumSurfacePoints >= 2 || NumSurfacePoints == m_NumConvexHullVertices);
		}
#endif

	}
	else
	{
		rvecSphereCenter = Vec3V(V_ZERO);
		rfSphereRadius = ScalarV(V_ZERO);
	}
}

// Added post gta4 - Want to keep and move to rage\dev
// Improves volume calculation on geometry bounds with open edges
#if !__SPU
bool phBoundGeometry::HasOpenEdges() const
{
#define MAX_NUM_EDGES 512
#define EDGE_ERROR_NO_ERROR 0
#define EDGE_ERROR_OPEN_EDGE 1
#define EDGE_ERROR_DEGENERATE_EDGE 2
#define MIN_NUM_POLYGONS 256

	int edgeVert0[MAX_NUM_EDGES];
	int edgeVert1[MAX_NUM_EDGES];
	int edgeCount[MAX_NUM_EDGES];
	int edgeError[MAX_NUM_EDGES];

	//We've been having a problem with small objects like windscreens not being modelled as volumes, but just as 
	//a few polygons with opposing normals. It's possible to end up with a negative volume when the bound isn't closed
	//so we're going to test if the bound is a closed volume and use an alternative method for computing the volume that 
	//ensures a positive volume.  Only test for open edges (test for a non-closed volume) if the bound has a small number 
	//of verts because large objects are far less likely to give a negative volume if a few edges are open.
	if(m_NumPolygons>MIN_NUM_POLYGONS)
	{
		return false;
	}

	int iNumEdges=0;
	for(int i=0;i<m_NumPolygons;i++)
	{
		const phPolygon& poly=m_Polygons[i];
		const int iNumVerts=POLY_MAX_VERTICES;
		int j0=poly.GetVertexIndex(iNumVerts-1);
		for(int j=0;j<iNumVerts;j++)
		{
			const int j1=poly.GetVertexIndex(j);

			//Now look for the edge j0->j1
			//If we've not found this edge then add it to the list and set it to be open (error) until a partner is found.
			//If we've got a duplicate in the same direction then we've got a degenerate edge (error)
			//If we've got the edge in the opposite direction then increment the count (error if count>2)
			bool bFoundEdge=false;
			for(int k=0;k<iNumEdges;k++)
			{
				if(edgeVert0[k]==j0 && edgeVert1[k]==j1)
				{
					bFoundEdge=true;
					edgeCount[k]++;
					edgeError[k]=EDGE_ERROR_DEGENERATE_EDGE;
				}
				else if(edgeVert1[k]==j0 && edgeVert0[k]==j1)
				{
					bFoundEdge=true;
					edgeCount[k]++;
					if(2==edgeCount[k])
					{
						edgeError[k]=EDGE_ERROR_NO_ERROR;
					}
					else 
					{
						Assert(edgeCount[k]>2);
						edgeError[k]=EDGE_ERROR_DEGENERATE_EDGE;
					}
				}
			}
			if(!bFoundEdge)
			{
				Assert(iNumEdges<MAX_NUM_EDGES);
				edgeVert0[iNumEdges]=j0;
				edgeVert1[iNumEdges]=j1;
				edgeCount[iNumEdges]=1;
				edgeError[iNumEdges]=EDGE_ERROR_OPEN_EDGE;
				iNumEdges++;
				Assert(iNumEdges<MAX_NUM_EDGES);
			}

			//Prepare for the next edge of the polygon.
			j0=j1;
		}
	}

	//If we have any degenerate edges then we've got a problem.
	for(int i=0;i<iNumEdges;i++)
	{
		if(edgeError[i]!=EDGE_ERROR_NO_ERROR)
		{
			return true;
		}
	}

	return false;
}
#endif


void phBoundPolyhedron::CalcCGOffset (Vec3V_InOut CGOffset) const
{
	CGOffset = Vec3V(V_ZERO);

	// Find out of this bound is completely closed by look at all of its polygons and seeing if any of them are missing a neighbor.
	bool foundMissingNeighbor = false;
	for(int polyIndex = 0; polyIndex < m_NumPolygons && !foundMissingNeighbor; polyIndex++)
	{
		const phPolygon &poly=m_Polygons[polyIndex];
		foundMissingNeighbor = foundMissingNeighbor || poly.GetNeighboringPolyNum(0) == (phPolygon::Index)(-1);
		foundMissingNeighbor = foundMissingNeighbor || poly.GetNeighboringPolyNum(1) == (phPolygon::Index)(-1);
		foundMissingNeighbor = foundMissingNeighbor || poly.GetNeighboringPolyNum(2) == (phPolygon::Index)(-1);

		++polyIndex;
	}

	if(!foundMissingNeighbor)
	{
		// The bound was found to be closed so we will treat the bound as a solid object whose boundary is the set of polygons.
		// Form a tetrahedron from each polygon and the origin and calculate some properties of it.
		ScalarV totalVolume(V_ZERO);

		for(int polyIndex = 0; polyIndex < m_NumPolygons; polyIndex++)
		{
			const phPolygon &poly=m_Polygons[polyIndex];
			Assert(poly.GetNeighboringPolyNum(0) != (phPolygon::Index)(-1));
			Assert(poly.GetNeighboringPolyNum(1) != (phPolygon::Index)(-1));
			Assert(poly.GetNeighboringPolyNum(2) != (phPolygon::Index)(-1));

			// This is actually three times volume of this tetrahedron.  However, since our total volume is also scaled by the same factor and all
			//   we're really using is the ratio of the two, this is fine.
			ScalarV volumeTetrahedron = ScalarV(poly.GetArea()) * Dot(GetPolygonUnitNormal(polyIndex),GetVertex(poly.GetVertexIndex(0)));
			totalVolume += volumeTetrahedron;

			// We calculate the CG of the tetrahedron (with the apex at the origin) as 3/4 * cgPolygon.
			Vec3V cgTetrahedron;
			cgTetrahedron = GetVertex(poly.GetVertexIndex(0));
			cgTetrahedron = Add(cgTetrahedron, GetVertex(poly.GetVertexIndex(1)));
			cgTetrahedron = Add(cgTetrahedron, GetVertex(poly.GetVertexIndex(2)));
			cgTetrahedron = Scale(cgTetrahedron, ScalarV(V_THIRD));

			// The CG of the tetrahedron will be 3/4 of the distance from the apex (the origin for us) and the CG of the base polygon.
			cgTetrahedron = Scale(cgTetrahedron, ScalarV(.75f));

			// Weight the CG of this tetrahedron according to its volume.
			cgTetrahedron = Scale(cgTetrahedron, volumeTetrahedron);

			// Accumulate!
			CGOffset = Add(CGOffset, cgTetrahedron);
		}
		CGOffset = InvScale(CGOffset, totalVolume);
	}
	else
	{
		// The bound was found to not be completely closed (at least one polygon was missing a neighbor on at least one side).
		// We will treat the bound as a 'shell' made up of the set of polygons.
		ScalarV totalArea = ScalarV(V_ZERO);

		for(int polyIndex = 0; polyIndex < m_NumPolygons; polyIndex++)
		{
			const phPolygon &poly=m_Polygons[polyIndex];
			totalArea += ScalarV(poly.GetArea());

			Vec3V cgPolygon;
			cgPolygon = GetVertex(poly.GetVertexIndex(0));
			cgPolygon = Add(cgPolygon, GetVertex(poly.GetVertexIndex(1)));
			cgPolygon = Add(cgPolygon, GetVertex(poly.GetVertexIndex(2)));
			cgPolygon = Scale(cgPolygon, ScalarV(V_THIRD));

			// Weight the CG of this polygon according to its area.
			cgPolygon = Scale(cgPolygon, ScalarV(poly.GetArea()));

			// Accumulate!
			CGOffset = Add(CGOffset, cgPolygon);
		}
		CGOffset = InvScale(CGOffset, totalArea);
	}
}


static const int s_kNumTestDirections = 42;
static const Vec3V s_kTestDirections[s_kNumTestDirections] = {
	Vec3V(0.000000f , -0.000000f,-1.000000f),
	Vec3V(0.723608f , -0.525725f,-0.447219f),
	Vec3V(-0.276388f , -0.850649f,-0.447219f),
	Vec3V(-0.894426f , -0.000000f,-0.447216f),
	Vec3V(-0.276388f , 0.850649f,-0.447220f),
	Vec3V(0.723608f , 0.525725f,-0.447219f),
	Vec3V(0.276388f , -0.850649f,0.447220f),
	Vec3V(-0.723608f , -0.525725f,0.447219f),
	Vec3V(-0.723608f , 0.525725f,0.447219f),
	Vec3V(0.276388f , 0.850649f,0.447219f),
	Vec3V(0.894426f , 0.000000f,0.447216f),
	Vec3V(-0.000000f , 0.000000f,1.000000f),
	Vec3V(0.425323f , -0.309011f,-0.850654f),
	Vec3V(-0.162456f , -0.499995f,-0.850654f),
	Vec3V(0.262869f , -0.809012f,-0.525738f),
	Vec3V(0.425323f , 0.309011f,-0.850654f),
	Vec3V(0.850648f , -0.000000f,-0.525736f),
	Vec3V(-0.525730f , -0.000000f,-0.850652f),
	Vec3V(-0.688190f , -0.499997f,-0.525736f),
	Vec3V(-0.162456f , 0.499995f,-0.850654f),
	Vec3V(-0.688190f , 0.499997f,-0.525736f),
	Vec3V(0.262869f , 0.809012f,-0.525738f),
	Vec3V(0.951058f , 0.309013f,0.000000f),
	Vec3V(0.951058f , -0.309013f,0.000000f),
	Vec3V(0.587786f , -0.809017f,0.000000f),
	Vec3V(0.000000f , -1.000000f,0.000000f),
	Vec3V(-0.587786f , -0.809017f,0.000000f),
	Vec3V(-0.951058f , -0.309013f,-0.000000f),
	Vec3V(-0.951058f , 0.309013f,-0.000000f),
	Vec3V(-0.587786f , 0.809017f,-0.000000f),
	Vec3V(-0.000000f , 1.000000f,-0.000000f),
	Vec3V(0.587786f , 0.809017f,-0.000000f),
	Vec3V(0.688190f , -0.499997f,0.525736f),
	Vec3V(-0.262869f , -0.809012f,0.525738f),
	Vec3V(-0.850648f , 0.000000f,0.525736f),
	Vec3V(-0.262869f , 0.809012f,0.525738f),
	Vec3V(0.688190f , 0.499997f,0.525736f),
	Vec3V(0.525730f , 0.000000f,0.850652f),
	Vec3V(0.162456f , -0.499995f,0.850654f),
	Vec3V(-0.425323f , -0.309011f,0.850654f),
	Vec3V(-0.425323f , 0.309011f,0.850654f),
	Vec3V(0.162456f , 0.499995f,0.850654f)
};

struct phSmallPolygon
{
	Vec3V	m_UnitNormal;
	u16		m_VertexIndices[POLY_MAX_VERTICES];
	u16		m_NeighboringPolygons[POLY_MAX_VERTICES];

	datPadding<2, u16> m_Pad;
};

void ComputeSmallPolygonNormal(phSmallPolygon &polygon, const Vec3V *vertices)
{
	Vec3V edge0, edge1;
	edge0 = Subtract(vertices[polygon.m_VertexIndices[1]], vertices[polygon.m_VertexIndices[0]]);
	edge1 = Subtract(vertices[polygon.m_VertexIndices[2]], vertices[polygon.m_VertexIndices[0]]);
	polygon.m_UnitNormal = Cross(edge0, edge1);
	polygon.m_UnitNormal = Normalize(polygon.m_UnitNormal);
}

bool IsPointVisibleToSmallPolygon(const phSmallPolygon &polygon, const Vec3V *vertices, Vec3V_InOut testPoint)
{
	Vec3V temp(testPoint - vertices[polygon.m_VertexIndices[0]]);
	// Does the tolerance need to be taken into account here?
	ScalarV distToPolygon(Dot(temp,polygon.m_UnitNormal));
	// Which one should this be?  I dunno ... it's too confusing.
	//return distToPolygon.IsGreaterThan(VEC3_ZERO);
	return IsGreaterThan(distToPolygon, ScalarV(V_FLT_SMALL_6)).Getb();
	//return distToPolygon.IsGreaterOrEqualThan(-VEC3_SMALL_FLOAT);
}

void SwapNeighbor(phSmallPolygon &polygon, int initialEdgeVertexIndex, int newNeighbor)
{
	for(int neighborInTriIndex = 0; neighborInTriIndex < 3; ++neighborInTriIndex)
	{
		if(polygon.m_VertexIndices[neighborInTriIndex] == initialEdgeVertexIndex)
		{
			polygon.m_NeighboringPolygons[neighborInTriIndex] = (u16)(newNeighbor);
			return;
		}
	}
	Assert(false);
}

int FindNeighborIndexOfPoly(const phSmallPolygon &polygon, int neighborPolyIndex)
{
	for(int neighborInPolyIndex = 0; neighborInPolyIndex < 3; ++neighborInPolyIndex)
	{
		if(polygon.m_NeighboringPolygons[neighborInPolyIndex] == neighborPolyIndex)
		{
			return neighborInPolyIndex;
		}
	}
	Assert(false);
	return -1;
}


#if __ASSERT
#define CheckConvexHullPoly(...)
#if 0
void CheckConvexHullPoly(const phSmallPolygon &testPoly, const phSmallPolygon *polygons, const Vec3V *vertices, const int numConvexHullPoints, const int firstInteriorPoint, const int numPoints)
{
	// Don't assert on anything right now.  Well then this function is pretty stupid.
	if(true)
	{
		return;
	}

#ifndef __SNC__
	const int curPolyIndex = (int) (&testPoly - polygons);
	for(int neighborInTriIndex = 0; neighborInTriIndex < 3; ++neighborInTriIndex)
	{
		// Check to make sure that the neighbors agree that we are neighbors.
		const int neighborTriIndex = testPoly.m_NeighboringPolygons[neighborInTriIndex];
		const phSmallPolygon &neighborTri = polygons[neighborTriIndex];
		Assert(neighborTri.m_NeighboringPolygons[0] == curPolyIndex || neighborTri.m_NeighboringPolygons[1] == curPolyIndex || neighborTri.m_NeighboringPolygons[2] == curPolyIndex);
	}

	for(int curVertexIndex = 0; curVertexIndex < numConvexHullPoints; ++curVertexIndex)
	{
		Vec3V curVert = vertices[curVertexIndex];
		const int vertIndex0 = testPoly.m_VertexIndices[0];
		Vec3V distToPoly = (curVert - vertices[vertIndex0]).DotV(testPoly.m_UnitNormal);
		Assert(distToPoly.IsLessOrEqualThan(VEC3_SMALL_FLOAT));
	}

	for(int curVertexIndex = firstInteriorPoint; curVertexIndex < numPoints; ++curVertexIndex)
	{
		Vec3V curVert = vertices[curVertexIndex];
		const int vertIndex0 = testPoly.m_VertexIndices[0];
		Vec3V distToPoly = (curVert - vertices[vertIndex0]).DotV(testPoly.m_UnitNormal);
		Assert(distToPoly.IsLessOrEqualThan(VEC3_SMALL_FLOAT));
	}
#endif
}
#endif


void CheckMapping(const int numPoints, const Vec3V *originalVertices, const Vec3V *verticesToCheck, u16 *oldToNewMapping, u16 *newToOldMapping)
{
	if(oldToNewMapping == NULL)
	{
		return;
	}

	for(int i = 0; i < numPoints; ++i)
	{
		Assert(oldToNewMapping[newToOldMapping[i]] == i);
		Assert(newToOldMapping[oldToNewMapping[i]] == i);

		Assert(IsEqualAll(verticesToCheck[oldToNewMapping[i]],originalVertices[i]));
		Assert(IsEqualAll(originalVertices[newToOldMapping[i]],verticesToCheck[i]));
	}
}
#endif	// __ASSERT

int phBoundPolyhedron::SortConvexHullPointsToFront(Vec3V_Ptr vertices, u16 *oldToNewVertexMapping, const int numPoints)
{
#if __ASSERT
	// Save off a copy of the vertices in their original positions so that we can assert that the mappings are correct.
	Vec3V_Ptr originalVertices = Alloca(Vec3V, numPoints);
	for(int i = 0; i < numPoints; ++i)
	{
		originalVertices[i] = vertices[i];
	}
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the mappings if needed.
	u16 *newToOldVertexMapping = NULL;
	if(oldToNewVertexMapping != NULL)
	{
		newToOldVertexMapping = Alloca(u16, numPoints);

		for(int curVertIndex = 0; curVertIndex < numPoints; ++curVertIndex)
		{
			oldToNewVertexMapping[curVertIndex] = (u16)(curVertIndex);
			newToOldVertexMapping[curVertIndex] = (u16)(curVertIndex);
		}
	}
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////

	// If they only gave us one or two points, then those points are the convex hull.
	if(numPoints < 3)
	{
		return numPoints;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// First, we need to find a triangle made up of three vertices that are on the convex hull.
	int numConvexHullPoints = 0;
	int curTestDir = 0;
	Vec3V primarySortDir = s_kTestDirections[0];
	Vec3V secondarySortDir;
	secondarySortDir = Cross(primarySortDir, s_kTestDirections[1]);
	secondarySortDir = Normalize(secondarySortDir);

#define TEST_SAMPLING 0
#if !TEST_SAMPLING
	while(numConvexHullPoints < 3 && curTestDir < s_kNumTestDirections)
#else
	while(curTestDir < s_kNumTestDirections)
#endif
	{
		int extremeVertexIndex = -1;
		ScalarV extremeVertexDistance(-V_FLT_MAX);
		ScalarV extremeVertexSecondaryDistance(-V_FLT_MAX);
		for(int curVertIndex = 0; curVertIndex < numPoints; ++curVertIndex)
		{
			ScalarV temp;
			temp = Dot(vertices[curVertIndex], primarySortDir);
			if(IsGreaterThanAll(temp ,extremeVertexDistance + ScalarV(V_FLT_SMALL_6)))
			{
				extremeVertexIndex = curVertIndex;
				extremeVertexDistance = temp;
				extremeVertexSecondaryDistance = Dot(vertices[curVertIndex], secondarySortDir);
			}
			else if(IsGreaterThanAll(temp ,extremeVertexDistance - ScalarV(V_FLT_SMALL_6)))
			{
				ScalarV secondaryDistance;
				secondaryDistance = Dot(vertices[curVertIndex], secondarySortDir);
				ASSERT_ONLY(ScalarV secondDistDiff(secondaryDistance - extremeVertexSecondaryDistance));
				ASSERT_ONLY(secondDistDiff = Abs(secondDistDiff));
				Assert(IsGreaterThanOrEqualAll(secondDistDiff, ScalarV(V_FLT_SMALL_6)));
				if(IsGreaterThanAll(secondaryDistance,extremeVertexSecondaryDistance))
				{
					extremeVertexIndex = curVertIndex;
					extremeVertexDistance = temp;
					extremeVertexSecondaryDistance = secondaryDistance;
				}
			}
		}

		if(extremeVertexIndex >= numConvexHullPoints)
		{
			// We've found a new extreme point.  Let's swap it to the front and 
			SwapEm(vertices[extremeVertexIndex], vertices[numConvexHullPoints]);

			if(oldToNewVertexMapping != NULL)
			{
				SwapEm(oldToNewVertexMapping[newToOldVertexMapping[extremeVertexIndex]], oldToNewVertexMapping[newToOldVertexMapping[numConvexHullPoints]]);
				SwapEm(newToOldVertexMapping[extremeVertexIndex], newToOldVertexMapping[numConvexHullPoints]);
				ASSERT_ONLY(CheckMapping(numPoints, originalVertices, vertices, oldToNewVertexMapping, newToOldVertexMapping));
			}

			++numConvexHullPoints;
		}
		++curTestDir;
		primarySortDir = s_kTestDirections[curTestDir];
		secondarySortDir = Cross(primarySortDir, s_kTestDirections[(curTestDir + 1) % s_kNumTestDirections]);
		secondarySortDir = Normalize(secondarySortDir);
	}

#if TEST_SAMPLING
	if(true)
	{
		return numConvexHullPoints;
	}
#endif

	Assert(numConvexHullPoints == 3);
//	if(true)
//	{
//		return numConvexHullPoints;
//	}
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////

	const int numTrisAllocated = 2 * numPoints;
	phSmallPolygon *convexHullTris = Alloca(phSmallPolygon, numTrisAllocated);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the convex hull to have two triangles.
	int numConvexHullTris = 2;

	convexHullTris[0].m_VertexIndices[0] = 0;
	convexHullTris[0].m_VertexIndices[1] = 1;
	convexHullTris[0].m_VertexIndices[2] = 2;
	convexHullTris[0].m_NeighboringPolygons[0] = 1;
	convexHullTris[0].m_NeighboringPolygons[1] = 1;
	convexHullTris[0].m_NeighboringPolygons[2] = 1;
	ComputeSmallPolygonNormal(convexHullTris[0], vertices);

	convexHullTris[1].m_VertexIndices[0] = 0;
	convexHullTris[1].m_VertexIndices[1] = 2;
	convexHullTris[1].m_VertexIndices[2] = 1;
	convexHullTris[1].m_NeighboringPolygons[0] = 0;
	convexHullTris[1].m_NeighboringPolygons[1] = 0;
	convexHullTris[1].m_NeighboringPolygons[2] = 0;
	ComputeSmallPolygonNormal(convexHullTris[1], vertices);

	ASSERT_ONLY(CheckConvexHullPoly(convexHullTris[0], convexHullTris, vertices, numConvexHullPoints, numPoints, numPoints));
	ASSERT_ONLY(CheckConvexHullPoly(convexHullTris[1], convexHullTris, vertices, numConvexHullPoints, numPoints, numPoints));
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////

	int firstInteriorPoint = numPoints;
	int curConvexHullTriIndex = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// There might already be some 'interior' points.
	FindAndMoveInteriorPoints(vertices, oldToNewVertexMapping, newToOldVertexMapping, numConvexHullPoints, firstInteriorPoint, convexHullTris, numConvexHullTris);
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////

	// When this loop finishes, all of points will have been classified as being convex hulls points or not being convex hull points.
#if SORT_DEBUGGING
	int numIterations = 0;
	const int maxIterations = 200;
	while(firstInteriorPoint > numConvexHullPoints && numIterations < maxIterations)
	{
		++numIterations;
#else
	while(firstInteriorPoint > numConvexHullPoints)
	{
#endif
		// There are still points that aren't on the interior but aren't on the convex hull, so let's find one to add to the convex hull.
		int furthestVertexIndex = -1;
		ScalarV furthestVertexPrimaryDistance = ScalarV(V_ZERO);//-VEC3_SMALL_FLOAT;//VEC3_VERY_SMALL_FLOAT;
		ScalarV furthestVertexSecondaryDistance(-V_FLT_MAX);
		while(furthestVertexIndex == -1)
		{
			// Search through all of the as-of-yet-unclassified points and find the one that's furthest away from the current convex hull polygon
			//   and add that point to the convex hull.
			phSmallPolygon &curConvexHullPolygon = convexHullTris[curConvexHullTriIndex];
			primarySortDir = curConvexHullPolygon.m_UnitNormal;
			secondarySortDir = Subtract(vertices[curConvexHullPolygon.m_VertexIndices[1]], vertices[curConvexHullPolygon.m_VertexIndices[0]]);
			secondarySortDir = Cross(secondarySortDir, primarySortDir);
			secondarySortDir = Normalize(secondarySortDir);

			for(int curVertIndex = numConvexHullPoints; curVertIndex < firstInteriorPoint; ++curVertIndex)
			{
				ScalarV temp;

				temp = Dot(Subtract(vertices[curVertIndex], vertices[curConvexHullPolygon.m_VertexIndices[0]]), primarySortDir);

				if(IsGreaterThanAll(temp, ScalarV(V_FLT_SMALL_6)))
				{
					if(IsGreaterThanAll(temp, furthestVertexPrimaryDistance + ScalarV(V_FLT_SMALL_6)))
					{
						furthestVertexIndex = curVertIndex;
						furthestVertexPrimaryDistance = temp;
						furthestVertexSecondaryDistance = Dot(vertices[curVertIndex], secondarySortDir);
					}
					else if(IsGreaterThanAll(temp ,furthestVertexPrimaryDistance - ScalarV(V_FLT_SMALL_6)))
					{
						ScalarV secondaryDistance;
						secondaryDistance = Dot(vertices[curVertIndex], secondarySortDir);
						if(IsGreaterThanAll(secondaryDistance, furthestVertexSecondaryDistance))
						{
							furthestVertexIndex = curVertIndex;
							furthestVertexPrimaryDistance = temp;
							furthestVertexSecondaryDistance = secondaryDistance;
						}
					}
				}
			}

			Assert(furthestVertexIndex == -1 || IsGreaterThanOrEqualAll(furthestVertexPrimaryDistance ,ScalarV(V_ZERO)));
			Assert(furthestVertexIndex != -1 || IsEqualAll(furthestVertexPrimaryDistance, ScalarV(V_ZERO)));

			// See if we've found an extreme point that's actually not on the face of the triangle in question.
			// Due to the fact that we have to account for floating precision errors in the above comparisons, 
			if(furthestVertexIndex != -1)
			{
				Assert(IsGreaterThanOrEqualAll(furthestVertexPrimaryDistance, ScalarV(V_FLT_SMALL_6)));
#if SORT_DEBUGGING
				s_LastVertexAdded = vertices[furthestVertexIndex];
				s_PolyNormal = primarySortDir;
				s_TriVert0 = vertices[curConvexHullPolygon.m_VertexIndices[0]];
				s_TriVert1 = vertices[curConvexHullPolygon.m_VertexIndices[1]];
				s_TriVert2 = vertices[curConvexHullPolygon.m_VertexIndices[2]];
#endif
				ASSERT_ONLY(CheckConvexHullPoly(curConvexHullPolygon, convexHullTris, vertices, numConvexHullPoints, firstInteriorPoint, numPoints));

				Assert(numConvexHullTris + 2 < numTrisAllocated);
				// We've found a new extreme point.  Let's swap it to the front and add/adjust the convex hull triangles.
				SwapEm(vertices[furthestVertexIndex], vertices[numConvexHullPoints]);

				// First, we need to check if 
				// This is one of the complications of the three-dimensional case versus the two dimensional case - while it is guaranteed that
				//   any vertex that we've marked as 
				int neighborsVisible = 0;
				int neighborIndexShifted = 1;
				for(int neighborInTriIndex = 0; neighborInTriIndex < 3; ++neighborInTriIndex)
				{
					int neighborTriIndex = curConvexHullPolygon.m_NeighboringPolygons[neighborInTriIndex];
					phSmallPolygon &neighborTriangle = convexHullTris[neighborTriIndex];
					const bool kIsNeighborVisible = IsPointVisibleToSmallPolygon(neighborTriangle, vertices, vertices[numConvexHullPoints]);
					if(kIsNeighborVisible)
					{
						const int nextIndex = (neighborInTriIndex + 1) % 3;
						const int nextNextIndex = (neighborInTriIndex + 2) % 3;
						// Verts [neighborInTriIndex] and [(neighborInTriIndex + 1) % 3] are shared between this and the neighboring triangle.  That
						//   is also the edge that needs to be split.
						//const int oldVertIndexShared0 = curConvexHullPolygon.m_VertexIndices[neighborInTriIndex];
						const int oldNextVertIndex = curConvexHullPolygon.m_VertexIndices[nextIndex];
						const int oldNextNeighborIndex = curConvexHullPolygon.m_NeighboringPolygons[nextIndex];

						curConvexHullPolygon.m_VertexIndices[nextIndex] = (u16)(numConvexHullPoints);
						curConvexHullPolygon.m_NeighboringPolygons[nextIndex] = (u16)(numConvexHullTris);
						ComputeSmallPolygonNormal(curConvexHullPolygon, vertices);

						convexHullTris[numConvexHullTris].m_VertexIndices[0] = curConvexHullPolygon.m_VertexIndices[nextNextIndex];
						convexHullTris[numConvexHullTris].m_VertexIndices[1] = (u16)(numConvexHullPoints);
						convexHullTris[numConvexHullTris].m_VertexIndices[2] = (u16)(oldNextVertIndex);
						convexHullTris[numConvexHullTris].m_NeighboringPolygons[0] = (u16)(curConvexHullTriIndex);
						convexHullTris[numConvexHullTris].m_NeighboringPolygons[1] = (u16)(numConvexHullTris + 1);
						convexHullTris[numConvexHullTris].m_NeighboringPolygons[2] = (u16)(oldNextNeighborIndex);
						ComputeSmallPolygonNormal(convexHullTris[numConvexHullTris], vertices);

						const int sharedEdgeInNeighborIndex = (neighborTriangle.m_NeighboringPolygons[0] == curConvexHullTriIndex ? 0 : neighborTriangle.m_NeighboringPolygons[1] == curConvexHullTriIndex ? 1 : 2);
						Assert(neighborTriangle.m_NeighboringPolygons[sharedEdgeInNeighborIndex] == curConvexHullTriIndex);
						Assert(neighborTriangle.m_VertexIndices[sharedEdgeInNeighborIndex] == oldNextVertIndex);
//						const int nextSharedEdgeInNeighborIndex = (sharedEdgeInNeighborIndex + 1) % 3;
						const int nextNextSharedEdgeInNeighborIndex = (sharedEdgeInNeighborIndex + 2) % 3;
						const int foo = neighborTriangle.m_NeighboringPolygons[nextNextSharedEdgeInNeighborIndex];

						neighborTriangle.m_VertexIndices[sharedEdgeInNeighborIndex] = (u16)(numConvexHullPoints);
						neighborTriangle.m_NeighboringPolygons[nextNextSharedEdgeInNeighborIndex] = (u16)(numConvexHullTris + 1);
						ComputeSmallPolygonNormal(neighborTriangle, vertices);

						convexHullTris[numConvexHullTris + 1].m_VertexIndices[0] = (u16)(oldNextVertIndex);
						convexHullTris[numConvexHullTris + 1].m_VertexIndices[1] = (u16)(numConvexHullPoints);
						convexHullTris[numConvexHullTris + 1].m_VertexIndices[2] = neighborTriangle.m_VertexIndices[nextNextSharedEdgeInNeighborIndex];
						convexHullTris[numConvexHullTris + 1].m_NeighboringPolygons[0] = (u16)(numConvexHullTris);
						convexHullTris[numConvexHullTris + 1].m_NeighboringPolygons[1] = (u16)(neighborTriIndex);
						convexHullTris[numConvexHullTris + 1].m_NeighboringPolygons[2] = (u16)(foo);
						ComputeSmallPolygonNormal(convexHullTris[numConvexHullTris + 1], vertices);

						phSmallPolygon &oldNextNeighborTri = convexHullTris[oldNextNeighborIndex];
						SwapNeighbor(oldNextNeighborTri, curConvexHullPolygon.m_VertexIndices[nextNextIndex], numConvexHullTris);

						phSmallPolygon &fooTri = convexHullTris[foo];
						SwapNeighbor(fooTri, oldNextVertIndex, numConvexHullTris + 1);

						numConvexHullTris += 2;

#if __ASSERT
						for(int i = 0; i < numConvexHullTris; ++i)
						{
							CheckConvexHullPoly(convexHullTris[i], convexHullTris, vertices, numConvexHullPoints + 1, firstInteriorPoint, numPoints);
						}
#endif

						neighborsVisible = neighborsVisible | neighborIndexShifted;
						break;
					}
					neighborIndexShifted = neighborIndexShifted << 1;
				}
				Assert(neighborsVisible == 0 || neighborsVisible == 1 || neighborsVisible == 2 || neighborsVisible == 4);

				if(neighborsVisible == 0)
				{
					// The vertex we're adding is 'visible' to only one face.  Let's add that vertex to that face and split the face into three triangles.
					Assert(curConvexHullPolygon.m_VertexIndices[0] < numConvexHullPoints);
					Assert(curConvexHullPolygon.m_VertexIndices[1] < numConvexHullPoints);
					Assert(curConvexHullPolygon.m_VertexIndices[2] < numConvexHullPoints);
					int oldVertexIndex2 = curConvexHullPolygon.m_VertexIndices[2];
					int oldNeighborIndex1 = curConvexHullPolygon.m_NeighboringPolygons[1];
					int oldNeighborIndex2 = curConvexHullPolygon.m_NeighboringPolygons[2];

					curConvexHullPolygon.m_VertexIndices[2] = (u16)(numConvexHullPoints);
					curConvexHullPolygon.m_NeighboringPolygons[1] = (u16)(numConvexHullTris);
					curConvexHullPolygon.m_NeighboringPolygons[2] = (u16)(numConvexHullTris + 1);
					ComputeSmallPolygonNormal(curConvexHullPolygon, vertices);

					convexHullTris[numConvexHullTris].m_VertexIndices[0] = curConvexHullPolygon.m_VertexIndices[1];
					convexHullTris[numConvexHullTris].m_VertexIndices[1] = (u16)(oldVertexIndex2);
					convexHullTris[numConvexHullTris].m_VertexIndices[2] = (u16)(numConvexHullPoints);
					convexHullTris[numConvexHullTris].m_NeighboringPolygons[0] = (u16)(oldNeighborIndex1);
					convexHullTris[numConvexHullTris].m_NeighboringPolygons[1] = (u16)(numConvexHullTris + 1);
					convexHullTris[numConvexHullTris].m_NeighboringPolygons[2] = (u16)(curConvexHullTriIndex);
					ComputeSmallPolygonNormal(convexHullTris[numConvexHullTris], vertices);

					convexHullTris[numConvexHullTris + 1].m_VertexIndices[0] = (u16)(oldVertexIndex2);
					convexHullTris[numConvexHullTris + 1].m_VertexIndices[1] = curConvexHullPolygon.m_VertexIndices[0];
					convexHullTris[numConvexHullTris + 1].m_VertexIndices[2] = (u16)(numConvexHullPoints);
					convexHullTris[numConvexHullTris + 1].m_NeighboringPolygons[0] = (u16)(oldNeighborIndex2);
					convexHullTris[numConvexHullTris + 1].m_NeighboringPolygons[1] = (u16)(curConvexHullTriIndex);
					convexHullTris[numConvexHullTris + 1].m_NeighboringPolygons[2] = (u16)(numConvexHullTris);
					ComputeSmallPolygonNormal(convexHullTris[numConvexHullTris + 1], vertices);

					phSmallPolygon &oldNeighbor1 = convexHullTris[oldNeighborIndex1];
					SwapNeighbor(oldNeighbor1, oldVertexIndex2, numConvexHullTris);

					phSmallPolygon &oldNeighbor2 = convexHullTris[oldNeighborIndex2];
					SwapNeighbor(oldNeighbor2, curConvexHullPolygon.m_VertexIndices[0], numConvexHullTris + 1);

					numConvexHullTris += 2;

#if __ASSERT
					for(int i = 0; i < numConvexHullTris; ++i)
					{
						CheckConvexHullPoly(convexHullTris[i], convexHullTris, vertices, numConvexHullPoints + 1, firstInteriorPoint, numPoints);
					}
#endif
				}

				if(oldToNewVertexMapping != NULL)
				{
					SwapEm(oldToNewVertexMapping[newToOldVertexMapping[numConvexHullPoints]], oldToNewVertexMapping[newToOldVertexMapping[furthestVertexIndex]]);
					SwapEm(newToOldVertexMapping[numConvexHullPoints], newToOldVertexMapping[furthestVertexIndex]);
					ASSERT_ONLY(CheckMapping(numPoints, originalVertices, vertices, oldToNewVertexMapping, newToOldVertexMapping));
				}

				++numConvexHullPoints;
			}
			else
			{
				// Try the next polygon.
				++curConvexHullTriIndex;
				Assert(curConvexHullTriIndex < numConvexHullTris || numConvexHullTris == 2);
				Assert(curConvexHullTriIndex <= numConvexHullTris);

				//AssertMsg(curConvexHullTriIndex < numConvexHullTris, "Point set is co-planar, not handling that well.");
				// Check if we've run through all of the triangles.  If we've run out of triangles but all of the vertices have not been classified,
				//   then we've got a case where all of the vertices are co-planar and that's a degenerate case for this algorithm.  To get around
				//   that, we'll simply perturb one of the vertices by a small amount (along the normal) and re-run the algorithm on that point set.
				if(curConvexHullTriIndex == numConvexHullTris)
				{
					// If this assert were to fail then it means that all of the points have actually been classified.  I don't think that the logic
					//   of these loops would permit that to happen but I figure I should probably go ahead and at least verify that.
					Assert(firstInteriorPoint > numConvexHullPoints);

					// Adjust one of the vertices and re-run the algorithm on the adjusted point set.
					u16 *secondOldToNewVertexMapping = Alloca(u16, numPoints);
					vertices[0] = AddScaled(vertices[0], primarySortDir, ScalarV(V_ONE));
					const int kRetVal = SortConvexHullPointsToFront(vertices, secondOldToNewVertexMapping, numPoints);

					// Undo the adjustment that we made and compose the old-to-new vertex index mappings together.
					vertices[secondOldToNewVertexMapping[0]] = SubtractScaled(vertices[secondOldToNewVertexMapping[0]], primarySortDir, ScalarV(V_ONE));
					if(oldToNewVertexMapping != NULL)
					{
						for(int vertIndex = 0; vertIndex < numPoints; ++vertIndex)
						{
							oldToNewVertexMapping[vertIndex] = secondOldToNewVertexMapping[oldToNewVertexMapping[vertIndex]];
						}
					}
					return kRetVal;
				}
			}
		}

		// Loop over all of the points and toss out any that are found to be in the interior.
		// TODO: Really, we only need to check the new triangles here, not all of them.
		// TODO: Also, this could really just be in the 'if' portion of the above.  It's not needed when we went into the 'else' path.
		FindAndMoveInteriorPoints(vertices, oldToNewVertexMapping, newToOldVertexMapping, numConvexHullPoints, firstInteriorPoint, convexHullTris, numConvexHullTris);
		ASSERT_ONLY(CheckMapping(numPoints, originalVertices, vertices, oldToNewVertexMapping, newToOldVertexMapping));
	}

	return numConvexHullPoints;
}


// tolerance
void phBoundPolyhedron::FindAndMoveInteriorPoints(Vec3V_Ptr vertices, u16 *oldToNewVertexMapping, u16 *newToOldVertexMapping, const int startIndex,
	int &firstInteriorPoint, const phSmallPolygon *convexHullTris, const int numConvexHullTris)
{
	ScalarV distToPolygon;
	// TODO: Really, we only need to check the three new triangles here, not all of them.
	for(int curVertIndex = startIndex; curVertIndex < firstInteriorPoint; ++curVertIndex)
	{
		bool anyInFront = false;			// Have we found any polys on which this point is on the front side?
		bool anyPlanarInterior = false;		// Have we found any polys for which this point is coplanar but on the interior?
		bool anyPlanarExterior = false;		// Have we found any polys for which this point is coplanar but not on the interior?
		for(int curTriIndex = 0; curTriIndex < numConvexHullTris && !anyInFront; ++curTriIndex)
		{
			const phSmallPolygon &curConvexHullPolygon = convexHullTris[curTriIndex];
			Vec3V temp;
			const int vertIndex0 = curConvexHullPolygon.m_VertexIndices[0];
			temp = Subtract(vertices[curVertIndex], vertices[vertIndex0]);
			distToPolygon = Dot(temp, curConvexHullPolygon.m_UnitNormal);

			if(IsLessThanAll(distToPolygon,-ScalarV(V_FLT_SMALL_6)))
			{
				// It's clearly behind the polygon.
				continue;
			}

			if(IsLessThanOrEqualAll(distToPolygon, ScalarV(V_FLT_SMALL_6)))
			{
				// It's near the surface of the polygon.  Let's throw it away if it's not 'within' the triangle.
				const int vertIndex1 = curConvexHullPolygon.m_VertexIndices[1];
				const int vertIndex2 = curConvexHullPolygon.m_VertexIndices[2];

				Vec3V edge0(vertices[vertIndex1] - vertices[vertIndex0]);
				Vec3V edge1(vertices[vertIndex2] - vertices[vertIndex1]);
				Vec3V edge2(vertices[vertIndex0] - vertices[vertIndex2]);

				Vec3V cross0(edge0);
				Vec3V cross1(edge1);
				cross0 = Cross(cross0, temp);
				temp = Subtract(vertices[curVertIndex], vertices[vertIndex1]);
				cross1 = Cross(cross1, temp);

				if(IsLessThanAll(Dot(cross0,cross1), ScalarV(V_ZERO)))
				{
					// It's on different sides of edge0 and edge1.
					anyPlanarExterior = true;
					continue;
				}

				Vec3V cross2(edge2);
				temp = Subtract(vertices[curVertIndex], vertices[vertIndex2]);
				cross2 = Cross(cross2, temp);

				if(IsLessThanAll(Dot(cross0,cross2), ScalarV(V_ZERO)))
				{
					// It's on different sides of edge0 and edge2.
					anyPlanarExterior = true;
					continue;
				}

				anyPlanarInterior = true;
				continue;
			}

			// 
			anyInFront = true;
		}

		const bool isInterior = !anyInFront && (!anyPlanarExterior || anyPlanarInterior);
		if(isInterior)
		{
			--firstInteriorPoint;
			SwapEm(vertices[curVertIndex], vertices[firstInteriorPoint]);

			if(oldToNewVertexMapping != NULL)
			{
				SwapEm(oldToNewVertexMapping[newToOldVertexMapping[curVertIndex]], oldToNewVertexMapping[newToOldVertexMapping[firstInteriorPoint]]);
				SwapEm(newToOldVertexMapping[curVertIndex], newToOldVertexMapping[firstInteriorPoint]);
			}

			// Decrement to retest the same vertex index.
			--curVertIndex;
		}
	}
}

void phBoundPolyhedron::Copy (const phBound* original)
{
	// Make sure the clone is derived from phBoundPolyhedron, and 
	Assert(original->IsPolygonal());
	const phBoundPolyhedron* polyOriginal = static_cast<const phBoundPolyhedron*>(original);

	// Copy out pointers that will be changed when this bound is copied to the clone.
#if COMPRESSED_VERTEX_METHOD == 0
	const Vec3V* vertices = m_Vertices;
#endif
	const phPolygon* polygons = m_Polygons;

#if OCTANT_MAP_SUPPORT_ACCEL
	Assert(!m_OctantVertCounts);
#endif // OCTANT_MAP_SUPPORT_ACCEL

	// Copy everything in the bound to the clone.
	*this = *polyOriginal;
	if (phConfig::IsRefCountingEnabled())
	{
		SetRefCount(1);
	}

	// Reset the bound part pointers and local matrix pointers in the clone.
#if COMPRESSED_VERTEX_METHOD == 0
	m_Vertices = vertices;
#endif
	m_Polygons = polygons;

	// Copy the lists of vertices, edges and polygons, so that the clone has its
	// own lists instead of pointing to the original's lists.
	int index;
	for (index=0;index<m_NumVertices;index++)
	{
		SetVertex(index,polyOriginal->GetVertex(index));
	}
	for (index=0;index<m_NumPolygons;index++)
	{
		SetPolygon(index,polyOriginal->m_Polygons[index]);
	}

#if OCTANT_MAP_SUPPORT_ACCEL
#if USE_OCTANT_MAP_INDEX_U16
	FastAssert(m_Flags & OCTANT_MAP_INDEX_IS_U16);
#endif // USE_OCTANT_MAP_INDEX_U16
	if (polyOriginal->HasOctantMap())
	{
		// NULL these out to prevent some asserts inside OctantMapAllocateAndCopy and to ensure the permanent allocation works correctly.
		m_OctantVertCounts = NULL;
		m_OctantVerts = NULL;
		OctantMapAllocateAndCopy(polyOriginal->m_OctantVertCounts,polyOriginal->m_OctantVerts);
		Assert(HasOctantMap());
	}
#endif // OCTANT_MAP_SUPPORT_ACCEL
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


ScalarV_Out phBoundPolyhedron::DistToPolygon(Vec3V_In point, int polyIndex, Vec3V_Ptr normal) const
{
	const phPolygon& poly=m_Polygons[polyIndex];
	*normal = GetPolygonUnitNormal(polyIndex);
	Vec3V relPos(point);
	relPos = Subtract(relPos, GetVertex(poly.GetVertexIndex(0)));
	ScalarV distToPlane = Dot(*normal,relPos);
	Vec3V pointOnPlane(point);
	pointOnPlane = AddScaled(pointOnPlane, *normal,distToPlane);
	// See if the point on the plane is inside the polygon.
	Vec3V edge;
	int polyVertIndex,vertIndex;
	int lastVertIndex=poly.GetVertexIndex(0);
	bool planePoint=true;
	for(polyVertIndex=POLY_MAX_VERTICES-1;polyVertIndex>=0;polyVertIndex--)
	{
		vertIndex=poly.GetVertexIndex(polyVertIndex);
		edge = Subtract(GetVertex(vertIndex),GetVertex(lastVertIndex));
		relPos = Subtract(point,GetVertex(vertIndex));
		edge = Cross(edge, relPos);
		if(!SameSignAll(Dot(edge,*normal),distToPlane))
		{
			planePoint=false;
			break;
		}
		lastVertIndex=vertIndex;
	}
	if(planePoint)
	{
		// The closest point on the polygon to the given point is in the polygon's face.
		return Abs(distToPlane);
	}
	ScalarV distance2=ScalarV(V_ZERO),minDist2=ScalarV(V_FLT_MAX);
	lastVertIndex=poly.GetVertexIndex(0);
	for(polyVertIndex=POLY_MAX_VERTICES-1;polyVertIndex>=0;polyVertIndex--)
	{
		vertIndex=poly.GetVertexIndex(polyVertIndex);
		edge = Subtract(GetVertex(vertIndex),GetVertex(lastVertIndex));
		ScalarV edgeT=geomTValues::FindTValueSegToPoint(GetVertex(vertIndex),edge,point);
		if(IsLessThanOrEqualAll(edgeT, ScalarV(V_ZERO)))
		{
			// The closest point on this edge is a vertex.
			relPos = Subtract(point,GetVertex(vertIndex));
		}
		else if(IsGreaterThanOrEqualAll(edgeT, ScalarV(V_ONE)))
		{
			// The closest point on this edge is a vertex.
			relPos = Subtract(point,GetVertex(lastVertIndex));
		}
		else
		{
			// The closest point on this edge is not a vertex.
			edge = Scale(edge, edgeT);
			edge = Add(edge, GetVertex(vertIndex));
			relPos = Subtract(point,edge);
		}
		distance2=MagSquared(relPos);
		if(IsLessThanAll(distance2, minDist2))
		{
			minDist2=distance2;
			*normal = relPos;
		}
		lastVertIndex=vertIndex;
	}
	ScalarV distance= Sqrt(distance2);
	*normal = InvScale(*normal, distance);
	return distance;
}


void phBoundPolyhedron::GetSegments (const Vec3V* transformVerts, phSegment* segments) const
{
	for(int polyIndex1 = GetNumPolygons() - 1; polyIndex1 >= 0; --polyIndex1)
	{
		const phPolygon *poly1 = &GetPolygon(polyIndex1);
		for(int neighborIndex = POLY_MAX_VERTICES - 1; neighborIndex >= 0; --neighborIndex)
		{
			int polyIndex2 = poly1->GetNeighboringPolyNum(neighborIndex);
			if(/*(polyIndex2 == (u16)(-1)) || */(polyIndex1 > polyIndex2))
			{
				Assert(polyIndex2 != (u16)(-1));
				continue;
			}

			int nPolyVertexIdx1 = neighborIndex;
			int nPolyVertexIdx2 = nPolyVertexIdx1 < POLY_MAX_VERTICES - 1 ? nPolyVertexIdx1 + 1 : 0;

			int nSegmentIdx = (polyIndex1 << 2) + neighborIndex;
			segments[nSegmentIdx].A.Set(RCC_VECTOR3(transformVerts[poly1->GetVertexIndex(nPolyVertexIdx1)]));
			segments[nSegmentIdx].B.Set(RCC_VECTOR3(transformVerts[poly1->GetVertexIndex(nPolyVertexIdx2)]));
		}
	}
}
#endif // !__SPU

} // namespace rage
