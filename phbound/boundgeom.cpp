//
// phbound/boundgeom.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundgeom.h"

#include "primitives.h"
#include "support.h"

#include "atl/bitset.h"
#include "data/resource.h"
#include "data/struct.h"
#include "diag/output.h"
#include "file/asset.h"
#include "file/token.h"
#include "phbullet/CollisionMargin.h"
#include "phcore/convexpoly.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "string/string.h"
#include "system/alloca.h"
#include "system/memory.h"
#include "vector/geometry.h"
#include "physics/physicsprofilecapture.h"

#include <algorithm>
#include <functional>

#if RSG_TOOL || __RESOURCECOMPILER
#define SORT_CONVEX_HULL_VERTS_TO_FRONT	0
#else
#define SORT_CONVEX_HULL_VERTS_TO_FRONT	0
#endif
#define TEST_CONVEX_HULL_CODE			(0 && SORT_CONVEX_HULL_VERTS_TO_FRONT)
#if TEST_CONVEX_HULL_CODE
#include "math/random.h"
#endif

namespace rage {

CompileTimeAssert(sizeof(phBoundGeometry) <= phBound::MAX_BOUND_SIZE);

#if RSG_TOOL || __RESOURCECOMPILER
Vector3 phBoundGeometry::sm_CG = Vector3(0.0f,0.0f,0.0f);
int phBoundGeometry::sm_NumVertices=0;
int phBoundGeometry::sm_NumPerVertexAttribs=0;
int phBoundGeometry::sm_NumEdges=0;
int phBoundGeometry::sm_NumMaterialsToLoad=0;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	int phBoundGeometry::sm_NumMaterialColorsToLoad=0;
#endif
int phBoundGeometry::sm_NumPolygons=0;
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA	// HACK_GTA4 (post gta4) - new code
	int phBoundGeometry::sm_SecondSurface=0;
	float phBoundGeometry::sm_SecondSurfaceMaxHeight=0;
#endif
float phBoundGeometry::sm_Margin=0.0f;
bool phBoundGeometry::sm_LoadedCG=false;
bool phBoundGeometry::sm_LoadedMargin=false;
bool phBoundGeometry::sm_PreCheckedSizes=false;
#endif //RSG_TOOL || __RESOURCECOMPILER

bool phBoundGeometry::sm_SplitQuadsBendingOut=false;

//////////////////////////////////////////////////////////////
// bound profiler variables

namespace phBoundStats
{
	EXT_PF_TIMER(Poly_Box);
	EXT_PF_TIMER(TP_Poly);
};

using namespace phBoundStats;


/////////////////////////////////////////////////////////////////
// phBoundGeometry

phBoundGeometry::phBoundGeometry ()
{
	m_Type = GEOMETRY;

	m_NumMaterials = 0;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	m_NumMaterialColors = 0;
#endif

	// Shouldn't this really be taken care of in the phBoundPolyhedron constructor?
	m_NumVertices = 0;
	m_NumPolygons = 0;
	m_NumConvexHullVertices = 0;

#if COMPRESSED_VERTEX_METHOD == 0
	m_Vertices = NULL;
#endif
	m_Polygons = NULL;
#if COMPRESSED_VERTEX_METHOD > 0
	m_CompressedVertices = NULL;
#endif

	m_NumPerVertexAttribs = 0;
	m_VertexAttribs = NULL;

	m_MaterialIds = 0;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	m_MaterialColors = 0;
#endif

	m_PolyMatIndexList = NULL;

	memset(pad, 0, sizeof(pad));

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	m_SecondSurfaceVertexDisplacements=0;
	m_NumSecondSurfaceVertexDisplacements=0;
#endif
}


phBoundGeometry::~phBoundGeometry ()
{
	/*
	  If this geometry has been streamed we don't want
	  to release any memory (pointers are actually
	  addresses within static (ish) pages of managed memory).
	  */
	if (ComponentsStreamed())
		return;

	/*
	  NOTE: mutable pointers since phBoundGeometry
	  controls its own components
	  */
	phPolygon * geomPolys = (phPolygon*)m_Polygons;
	delete [] geomPolys;

#if COMPRESSED_VERTEX_METHOD == 0
	Vector3 * geomVerts = (Vector3*)m_Vertices;
	delete [] geomVerts;
#endif

#if COMPRESSED_VERTEX_METHOD > 0
	delete [] m_CompressedVertices;
#endif

	if(m_VertexAttribs) delete [] m_VertexAttribs;

	delete [] m_MaterialIds;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	if(m_MaterialColors) delete [] m_MaterialColors;
#endif
#if COMPRESSED_VERTEX_METHOD == 0
    delete [] m_ShrunkVertices;
#else
	delete [] m_CompressedShrunkVertices;
#endif

	delete [] m_PolyMatIndexList;

#if OCTANT_MAP_SUPPORT_ACCEL
	OctantMapDelete();
#endif

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	if(m_SecondSurfaceVertexDisplacements) delete [] m_SecondSurfaceVertexDisplacements;
#endif
}

const char* g_BoundNameAssertContext = NULL;

#if __RESOURCECOMPILER
const char*& phBoundGeometry::GetBoundNameAssertContextStr()
{
	return g_BoundNameAssertContext;
}
#endif // __RESOURCECOMPILER

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		void phBoundGeometry::Init (int numverts, int numvertattribs, int nummaterials, int nummaterialcolors, int numpolys, int secondSurface, bool polysInTempMem)
	#else
		void phBoundGeometry::Init (int numverts, int numvertattribs, int nummaterials, int numpolys, int secondSurface, bool polysInTempMem)
	#endif
#else //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		void phBoundGeometry::Init (int numverts, int numvertattribs, int nummaterials, int nummaterialcolors, int numpolys, bool polysInTempMem)
	#else
		void phBoundGeometry::Init (int numverts, int numvertattribs, int nummaterials, int numpolys, bool polysInTempMem)
	#endif
#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
{
	sysMemUseMemoryBucket bucket(sm_MemoryBucket);

	m_NumVertices=numverts;
	m_NumConvexHullVertices=(u16)(numverts);
#if !POLYGON_INDEX_IS_U32
	if(numpolys < 0 || numpolys >= 65535)
		Errorf("Object %s: You have too many or to little polygons you can only have up to %d you have %d.", g_BoundNameAssertContext, 65535, numpolys);
#endif
	if(numverts < 0 || numverts >= 32767)
		Errorf("Object %s: You have too many or to little vertices you can only have up to %d you have %d.", g_BoundNameAssertContext, 32767, numverts);
#if __ASSERT
	if (numpolys >= 256 && GetType() != phBound::BVH)
		Assertf(0, "Object %s: %d is too many primitives for a non-BVH geometry bound to have.  Please correct this.", g_BoundNameAssertContext, numpolys);
	if (numverts >= 256 && GetType() != phBound::BVH)
		Assertf(0, "Object %s: %d is too many vertices for a non-BVH geometry bound to have.  Please correct this.", g_BoundNameAssertContext, numverts);
#endif
	m_NumPolygons=numpolys;
	Assert(nummaterials>=0 && nummaterials<=255);
	m_NumMaterials=(u8)nummaterials;

#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	Assert(nummaterialcolors>=0 && nummaterialcolors<=255);
	m_NumMaterialColors=(u8)nummaterialcolors;
#endif

	Assert(numvertattribs>=0 && numvertattribs<256);
	m_NumPerVertexAttribs = (u8)numvertattribs;

    // Create the arrays for vertices, edges, polygons and materials.  The number of each must be greater than
	// zero except for edges; if there are zero edges here then the array is created in ComputeEdges or not at all.
#if 0 && RSG_TOOL
	Assert( m_NumVertices>0 && m_NumMaterials>0 && m_NumPolygons>0);
#endif

#if COMPRESSED_VERTEX_METHOD == 0
	Assert(!m_Vertices && !m_Polygons && !m_MaterialIds);
#else
	Assert(!m_Polygons && !m_MaterialIds);
#endif

#if COMPRESSED_VERTEX_METHOD > 0
	Assert(!m_CompressedVertices);
#endif
	Assert(!m_VertexAttribs);

	// Call the aligned version of new by putting WIN32_ONLY((32)) in the line below, so we get aligned phPolygons. We need that
    // because phPolygons are not allowed to straddle two cache lines for compatibility with IBM software cache on SPU.
	if (polysInTempMem)
	{
		sysMemStartTemp();
	}

    m_Polygons = rage_new phPolygon[m_NumPolygons];
	m_PolyMatIndexList = rage_new u8[m_NumPolygons];

	if (polysInTempMem)
	{
		sysMemEndTemp();
	}

	m_MaterialIds = rage_new phMaterialMgr::Id[Max(m_NumMaterials, (u8)(4))];
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	if(m_NumMaterialColors)
	{
		m_MaterialColors = rage_new u32[Max(m_NumMaterialColors, (u8)(4))];
	}
#endif

	if(m_NumPerVertexAttribs>0)
	{
		m_VertexAttribs = rage_new u32[Fourtify(m_NumVertices*m_NumPerVertexAttribs)];
	}

#if COMPRESSED_VERTEX_METHOD > 0
	{
#if COMPRESSED_VERTEX_METHOD == 1
		m_CompressedVertices = rage_new float[(m_NumVertices * 3 + 4) & ~3];
#elif COMPRESSED_VERTEX_METHOD == 2
		m_CompressedVertices = rage_new CompressedVertexType[m_NumVertices * 3];
#endif
	}
#endif
#if COMPRESSED_VERTEX_METHOD == 0
	{
		m_Vertices = rage_new Vec3V[Fourtify(m_NumVertices)];
	}
#endif

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA

	if(secondSurface && m_NumVertices>0)
	{
		m_NumSecondSurfaceVertexDisplacements = m_NumVertices;
		m_SecondSurfaceVertexDisplacements = rage_new float[m_NumSecondSurfaceVertexDisplacements];
		int i;
		for(i=0;i<m_NumSecondSurfaceVertexDisplacements;i++)
		{
			m_SecondSurfaceVertexDisplacements[i]=0.0f;
		}
	}
	else
	{
		m_SecondSurfaceVertexDisplacements = 0;
		m_NumSecondSurfaceVertexDisplacements=0;
	}
#endif
}


bool phBoundGeometry::PostLoadCompute ()
{
	bool noErrors = true;

	CalculateExtents();

	return noErrors;
}

const phMaterial& phBoundGeometry::GetMaterial (phMaterialIndex i) const
{
	Assert(i<m_NumMaterials);
	return MATERIALMGR.GetMaterial(m_MaterialIds[i]);
}

void phBoundGeometry::GetMaterialName (phMaterialIndex i, char* name, int size) const
{
	Assert(i<m_NumMaterials);
	MATERIALMGR.GetMaterialName(m_MaterialIds[i], name, size);
}

/*
int phBoundGeometry::GetPolygonMaterialIndex (int polygonIndex) const
{
	return m_PolyMatIndexList[polygonIndex];
}
*/


/////////////////////////////////////////////////////////////////
// resources


phBoundGeometry::phBoundGeometry (datResource & rsc) : phBoundPolyhedron(rsc)
{
	rsc.PointerFixup(m_MaterialIds);
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	rsc.PointerFixup(m_MaterialColors);
#endif

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	rsc.PointerFixup(m_SecondSurfaceVertexDisplacements);
	Assert(0==m_SecondSurfaceVertexDisplacements || m_NumVertices==m_NumSecondSurfaceVertexDisplacements);
#endif

	rsc.PointerFixup(m_PolyMatIndexList);

#if USE_OCTANT_MAP_INDEX_U16
	if ((m_Flags & OCTANT_MAP_INDEX_IS_U16) == 0)
	{
		if (m_OctantVertCounts)
		{
			// Compress the octant counts and indices.
			u32 * octantVertCountsU32 = (u32*)m_OctantVertCounts;
			for (int octant = 0 ; octant < 8 ; octant++)
			{
				const u32 count = octantVertCountsU32[octant];
				FastAssert(count < 0xFFFF);
				m_OctantVertCounts[octant] = (u16)count;

				u32 * octantVertsU32 = (u32*)(m_OctantVerts[octant]);
				u16 * octantVertsU16 = m_OctantVerts[octant];
				for (u32 vertIndex = 0 ; vertIndex < count ; vertIndex++)
				{
					const u32 index = octantVertsU32[vertIndex];
					FastAssert(index < 0xFFFF);
					octantVertsU16[vertIndex] = (u16)index;
				}
			}
		}
		m_Flags |= OCTANT_MAP_INDEX_IS_U16;
	}
#endif // USE_OCTANT_MAP_INDEX_U16
}

#if __DECLARESTRUCT
void phBoundGeometry::DeclareStruct(datTypeStruct &s)
{
	phBoundPolyhedron::DeclareStruct(s);
	STRUCT_BEGIN(phBoundGeometry);
	STRUCT_DYNAMIC_ARRAY(m_MaterialIds,m_NumMaterials);
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	STRUCT_DYNAMIC_ARRAY(m_MaterialColors,m_NumMaterialColors);
#endif
	STRUCT_CONTAINED_ARRAY(pad1);
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	STRUCT_DYNAMIC_ARRAY(m_SecondSurfaceVertexDisplacements, m_NumSecondSurfaceVertexDisplacements);
	STRUCT_FIELD(m_NumSecondSurfaceVertexDisplacements);
#endif
	STRUCT_FIELD_VP(m_PolyMatIndexList); // count on this was already swapped but since it's an array of u8s we don't need to swap contents, so only swap pointer

	STRUCT_FIELD(m_NumMaterials);
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	STRUCT_FIELD(m_NumMaterialColors);
#endif
	STRUCT_CONTAINED_ARRAY(pad);
	STRUCT_END();
}
#endif // __DECLARESTRUCT


/////////////////////////////////////////////////////////////////
// creation

void phBoundGeometry::RemoveUnusedVerts ()
{
	// TODO
}


void phBoundGeometry::RemoveDegeneratePolys (float tolerance)
{
	if ( !GetBoundFlag(DELETE_BAD_POLYGONS) &&
		  GetBoundFlag(ALLOW_BAD_POLYGONS) &&
		 !GetBoundFlag(WARN_BAD_POLYGONS) )
	{
		// if we aren't going to delete, assert, or warn, then just return
		return;
	}

	const ScalarV toleranceAsScalarV(ScalarVFromF32(tolerance));
	const ScalarV oneAsScalarV(V_ONE);
	const ScalarV lowerTolerance = Subtract(oneAsScalarV, toleranceAsScalarV);
	const ScalarV upperTolerance = Add(oneAsScalarV, toleranceAsScalarV);
	const ScalarV lowerToleranceSquared = Scale(lowerTolerance, lowerTolerance);
	const ScalarV upperToleranceSquared = Scale(upperTolerance, upperTolerance);

	// get a mutable polygon pointer (ok since phBoundGeometry polygons are non-const)
	phPolygon * geomPolys = (phPolygon*)m_Polygons;

	u8* polyMatIndexList = const_cast<u8*>(m_PolyMatIndexList);

	int goodIndex, testIndex;

	for (goodIndex=0, testIndex=0; testIndex<m_NumPolygons; testIndex++)
	{
		phPolygon & testPoly = geomPolys[testIndex];

		if(GetType() == phBound::BVH)
		{
			const phPrimitive *testPrimitive = &testPoly.GetPrimitive();
			if(testPrimitive->GetType() != PRIM_TYPE_POLYGON)
			{
				if (goodIndex != testIndex)
				{
					Assert(goodIndex<testIndex);
					geomPolys[goodIndex] = testPoly; // copy polygon data

					polyMatIndexList[goodIndex] = polyMatIndexList[testIndex];

				}
				goodIndex++;
				continue;
			}
		}

		// Identify bad polygons by unit normals that have squared magnitudes that differ by more than the given tolerance.
		// A negative test is used so that NaN normals will result in bad polygons (they always fail comparisons - as long as the compiler doesn't invert them!).
		const Vec3V polyUnitNormal = GetPolygonUnitNormal(testIndex);
		const ScalarV polyMagSquared = MagSquared(polyUnitNormal);
		const bool polyNormalNotTooSmall = (IsGreaterThanAll(polyMagSquared, lowerToleranceSquared) != 0);
		const bool polyNormalNotTooBig = (IsLessThanAll(polyMagSquared, upperToleranceSquared) != 0);
		bool badPoly = !(polyNormalNotTooSmall && polyNormalNotTooBig);

		if (!badPoly || !GetBoundFlag(DELETE_BAD_POLYGONS))
		{
			// good poly, or keeping bad
			Assert(!badPoly || GetBoundFlag(ALLOW_BAD_POLYGONS));

			if (goodIndex != testIndex)
			{
				Assert(goodIndex<testIndex);
				geomPolys[goodIndex] = testPoly; // copy polygon data

				polyMatIndexList[goodIndex] = polyMatIndexList[testIndex];
			}
			goodIndex++;
		}
#if !__NO_OUTPUT
		else if (badPoly)
		{
			// bad polygon, skipping
			if (GetBoundFlag(WARN_BAD_POLYGONS))
			{
#if __RESOURCECOMPILER
				char desc[256];
				sprintf(desc, " (%s, '%s')", s_currentBoundFilename, s_currentFragChildBoneName);
#else
				const char* desc = "";
#endif
				Warningf("phBoundGeom:Load_v110(ASCII token) -%s Polygon %d has a non-unit normal: <%f, %f, %f>", desc, testIndex, polyUnitNormal.GetXf(), polyUnitNormal.GetYf(), polyUnitNormal.GetZf());

				int v[3] = {testPoly.GetVertexIndex(0),testPoly.GetVertexIndex(1),testPoly.GetVertexIndex(2)};

				{
					Warningf(" verts (%d,%d,%d)",v[0],v[1],v[2]);
					Warningf(" 0(%.3f,%.3f,%.3f) 1(%.3f,%.3f,%.3f) 2(%.3f,%.3f,%.3f)",GetVertex(v[0]).GetXf(),GetVertex(v[0]).GetYf(),GetVertex(v[0]).GetZf(),GetVertex(v[1]).GetXf(),GetVertex(v[1]).GetYf(),GetVertex(v[1]).GetZf(),GetVertex(v[2]).GetXf(),GetVertex(v[2]).GetYf(),GetVertex(v[2]).GetZf());
				}
			}
		}
#endif
	}

	Assert(goodIndex<=m_NumPolygons);
	m_NumPolygons=goodIndex;
}

/////////////////////////////////////////////////////////////////
// load / save
#if RSG_TOOL || __RESOURCECOMPILER
void phBoundGeometry::LoadGeomStats_v110 (fiAsciiTokenizer & token)
{
	token.MatchIToken("verts:");
	sm_NumVertices = token.GetInt();
	if (sm_NumVertices==0)
	{
		if (GetBoundFlag(WARN_ZERO_VERTICES))
		{
			Warningf("phBoundGeom:Load(token,version) found zero vertices in bound");
		}
		goto finished_preload;
	}

#if 0 // BS#1680840: TODO
	if(token.CheckToken("pervertattribs:"))
	{
		sm_NumPerVertexAttribs = token.GetInt();
		Assert(sm_NumPerVertexAttribs < 256);
	}
	else
	{
		sm_NumPerVertexAttribs = 0;
	}
#else
	sm_NumPerVertexAttribs = 0;
#endif

	if (token.CheckToken("centroid:"))
	{
		Vector3 offset;
		token.GetVector(offset);
		// phBound::Offset is found in CalculateExtents() after loading.
		if (phBound::MessagesEnabled())
		{
			Warningf("phBoundGeometry::LoadGeomStats_v110(%s) -- 'centroid:' not supported",token.GetName());
		}
	}


	sm_LoadedCG = false;
	if (token.CheckToken("cg:"))
	{
		token.GetVector(sm_CG);
		sm_LoadedCG = true;
	}

    sm_LoadedMargin = false;
    if (token.CheckToken("margin:"))
    {
        sm_Margin = token.GetFloat();
        sm_LoadedMargin = true;
    }

	token.MatchIToken("materials:");
	sm_NumMaterialsToLoad = token.GetInt();

#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	sm_NumMaterialColorsToLoad = 0;
	if (token.CheckToken("materialcolors:"))
	{
		sm_NumMaterialColorsToLoad = token.GetInt();
	}
#endif
	if (token.CheckIToken("pervertattribs:"))
	{
		sm_NumPerVertexAttribs = token.GetInt();
	}


	if (token.CheckIToken("edges:"))
	{
		sm_NumEdges = token.GetInt();
	}

	token.MatchIToken("polys:");
	sm_NumPolygons = token.GetInt();

finished_preload:;
	//Note we're pre checked
	sm_PreCheckedSizes = true;
}
#endif //RSG_TOOL || __RESOURCECOMPILER

void ExpandBoundingSphereForSphere(Vec3V_InOut boundingSphereCenter, ScalarV_InOut boundingSphereRadius, Vec3V_In newSphereCenter, ScalarV_In newSphereRadius)
{
	const ScalarV vsZero(V_ZERO);
	const Vec3V boundingCenterToNewCenter = Subtract(newSphereCenter, boundingSphereCenter);
	const ScalarV vsBoundingCenterToNewCenter = Mag(boundingCenterToNewCenter);
	const ScalarV vsAmountStickingOut = Subtract(Add(vsBoundingCenterToNewCenter, newSphereRadius), boundingSphereRadius);
	if(IsGreaterThanAll(vsAmountStickingOut, vsZero))
	{
		// We need to expand the bounding sphere to account for this sphere.  There are two distinct situations to handle here - when the sphere we're
		//   adding completely contains the previous bound sphere and when it doesn't.
		if(IsLessThanOrEqualAll(Subtract(Add(vsBoundingCenterToNewCenter, boundingSphereRadius), newSphereRadius), vsZero))
		{
			// The new sphere completely contains the old sphere.
			boundingSphereCenter = newSphereCenter;
			boundingSphereRadius = newSphereRadius;
		}
		else
		{
			// It should not be possible for boundingCenterToNewCenter to be zero here.  If that's the case, we either don't need to update the bounding
			//   sphere (failing the first if-statement above) or the new sphere completely contains the bounding sphere (passing the second if-statement
			//   above).
			const Vec3V directionToMoveCenter = Normalize(boundingCenterToNewCenter);
			const ScalarV adjustmentAmount = Scale(ScalarV(V_HALF), vsAmountStickingOut);
			boundingSphereCenter = Add(boundingSphereCenter, Scale(directionToMoveCenter, adjustmentAmount));
			boundingSphereRadius = Add(boundingSphereRadius, adjustmentAmount);

			ASSERT_ONLY(const Vec3V newBoundingCenterToNewCenter = Subtract(newSphereCenter, boundingSphereCenter));
			ASSERT_ONLY(const ScalarV vsNewAmountStickingOut = Subtract(Add(Mag(newBoundingCenterToNewCenter), newSphereRadius), boundingSphereRadius));
			Assert(IsLessThanOrEqualAll(vsNewAmountStickingOut, ScalarV(V_FLT_SMALL_2)));
		}
	}
}

#if RSG_TOOL || __RESOURCECOMPILER
bool phBoundGeometry::Load_v110 (fiAsciiTokenizer & token)
{
	// NOTE: mutable pointers since phBoundGeometry
	// controls its own components
	phPolygon * geomPolys;

	bool readEdgeNormals;

	ASSERT_ONLY(const int boundType = GetType());

	sysMemUseMemoryBucket bucket(sm_MemoryBucket);
	if (!sm_PreCheckedSizes)
	{
		LoadGeomStats_v110(token);
	}
	sm_PreCheckedSizes = false;

	//Pre-load gets some info into static variables
	m_NumVertices = sm_NumVertices;
	m_NumPolygons = sm_NumPolygons;
	m_NumPerVertexAttribs = (u8)sm_NumPerVertexAttribs;
	bool loadedCG = sm_LoadedCG;
	int numMaterialsToLoad = sm_NumMaterialsToLoad;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	const int numMaterialColorsToLoad = sm_NumMaterialColorsToLoad;
#endif
	if (!m_NumVertices)
	{
		if (GetBoundFlag(WARN_ZERO_VERTICES))
		{
			Warningf("phBoundGeom:Load() - found zero vertices in bound file");
		}

		// TODO: Need to verify that this sets the bounding box properly.
		CalculateBoundingBox(NULL,NULL);
		CalculateExtents();

#if COMPRESSED_VERTEX_METHOD == 0
		Assert(m_Vertices == NULL);
		m_Vertices = NULL;
#endif

		m_NumPolygons = 0;
		Assert(m_Polygons == NULL);
		m_Polygons = NULL;
#if COMPRESSED_VERTEX_METHOD > 0
		Assert(m_CompressedVertices == NULL);
		m_CompressedVertices = NULL;
#endif
		Assert(m_VertexAttribs == NULL);
		m_VertexAttribs = NULL;

		m_PolyMatIndexList = NULL;

		return true;
	}

	if (sm_LoadedCG)
	{
		SetCGOffset(RCC_VEC3V(sm_CG));
	}

    if (sm_LoadedMargin)
    {
        SetMargin(sm_Margin);
    }

	// There used to be an early quit with a warning here when the number of polygons is zero.
	// Curved geometry bounds can have zero (non-curved) polygons now.

	if (token.CheckIToken("readedgenormals:"))
	{
		readEdgeNormals = token.GetInt()==1 ? true : false;
	}
	else
	{
		readEdgeNormals = false;
	}

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	if(token.CheckIToken("secondSurface:"))
	{
		sm_SecondSurface = token.GetInt();
	}
	else
	{
		sm_SecondSurface = 0;
	}
	sm_SecondSurfaceMaxHeight=0;
	if(token.CheckIToken("secondSurfaceMaxHeight:"))
	{
		sm_SecondSurfaceMaxHeight=token.GetFloat();
		Assert(sm_SecondSurfaceMaxHeight<=SECOND_SURFACE_MAX_ALLOWED_DISPLACEMENT);
	}
#endif


#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
#define TEMP_HACK 0
#if TEMP_HACK 
	sm_SecondSurface=1;
#endif
#endif

	if (numMaterialsToLoad == 0)
	{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init (m_NumVertices, m_NumPerVertexAttribs, 1, numMaterialColorsToLoad, m_NumPolygons, sm_SecondSurface, true);
	#else
		Init (m_NumVertices, m_NumPerVertexAttribs, 1, m_NumPolygons, sm_SecondSurface, true);
	#endif
#else //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init (m_NumVertices, m_NumPerVertexAttribs, 1, numMaterialColorsToLoad, m_NumPolygons, true);
	#else
		Init (m_NumVertices, m_NumPerVertexAttribs, 1, m_NumPolygons, true);
	#endif
#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...
	}
	else
	{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init (m_NumVertices, m_NumPerVertexAttribs, numMaterialsToLoad, numMaterialColorsToLoad, m_NumPolygons, sm_SecondSurface, true);
	#else
		Init (m_NumVertices, m_NumPerVertexAttribs, numMaterialsToLoad, m_NumPolygons, sm_SecondSurface, true);
	#endif
#else //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init (m_NumVertices, m_NumPerVertexAttribs, numMaterialsToLoad, numMaterialColorsToLoad, m_NumPolygons, true);
	#else
		Init (m_NumVertices, m_NumPerVertexAttribs, numMaterialsToLoad, m_NumPolygons, true);
	#endif
#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...
	}

	geomPolys = (phPolygon *)m_Polygons;

	if (numMaterialsToLoad == 0)
	{
		// set a default material
		m_MaterialIds[0] = phMaterialMgr::DEFAULT_MATERIAL_ID;
	}

	// Unfortunately we can't just read these vertices right into a compressed vertex because we need to know the bounding
	//   box before we can compress, and we need to see all of the vertices to know the bounding box.  So we'll just read
	//   them into a temporary array for now and then delete it once we're done with it.
	sysMemStartTemp();
	Vec3V *tempVertArray = rage_new Vec3V[m_NumVertices];
	sysMemEndTemp();

	// Load the vertices into the temporary array and determine the bounding box that they require.
	int i;
	Vec3V boundingBoxMin(V_FLT_MAX), boundingBoxMax(V_NEG_FLT_MAX);
	for(i=0;i<m_NumVertices;i++)
	{
		token.MatchIToken("v");
		Vec3V tempVec;
		tempVec.SetXf(token.GetFloat());
		tempVec.SetYf(token.GetFloat());
		tempVec.SetZf(token.GetFloat());
		tempVertArray[i] = tempVec;

		boundingBoxMin = Min(boundingBoxMin, tempVec);
		boundingBoxMax = Max(boundingBoxMax, tempVec);
	}

	const ScalarV svMargin = GetMarginV(); 
	boundingBoxMin -= Vec3V(svMargin);
	boundingBoxMax += Vec3V(svMargin);

	if(m_NumPerVertexAttribs>0)
	{
		for (i=0; i<m_NumVertices; i++)
		{
			Color32 color;
			if(token.CheckIToken("va", true))
			{
				color.SetRed	(token.GetInt());
				color.SetGreen	(token.GetInt());
				color.SetBlue	(token.GetInt());
				color.SetAlpha	(token.GetInt());
			}
			this->SetVertexAttrib(i, 0, color);
		}
	}

#if TEST_CONVEX_HULL_CODE
	// Some test cases for sorting convex hull points.
	Vector3 temp2[20];

	temp2[0].Set(-1.0f, 1.0f, 0.0f);
	temp2[1].Set(1.0f, 1.0f, 0.0f);
	temp2[2].Set(1.0f, -1.0f, 0.0f);
	temp2[3].Set(-1.0f, -1.0f, 0.0f);

	// Four coplanar points.
	Assert(SortConvexHullPointsToFront(temp2, NULL, 4) == 4);

	temp2[0].Set(-1.0f, 1.0f, -1.0f);
	temp2[1].Set(-1.0f, -1.0f, -1.0f);
	temp2[2].Set(1.0f, 0.0f, -1.0f);
	temp2[3].Set(0.0f, 0.0f, 1.0f);

	Assert(SortConvexHullPointsToFront(temp2, NULL, 4) == 4);

	temp2[0].Set(-1.0f, 1.0f, -1.0f);
	temp2[1].Set(-1.0f, -1.0f, -1.0f);
	temp2[2].Set(1.0f, 1.0f, -1.0f);
	temp2[3].Set(1.0f, -1.0f, -1.0f);
	temp2[4].Set(-1.0f, 1.0f, 1.0f);

	Assert(SortConvexHullPointsToFront(temp2, NULL, 5) == 5);

	temp2[0].Set(-1.0f, 1.0f, 0.0f);
	temp2[1].Set(1.0f, 1.0f, 0.0f);
	temp2[2].Set(1.0f, -1.0f, 0.0f);
	temp2[3].Set(-1.0f, -1.0f, 0.0f);
	temp2[4].Set(0.0f, 0.0f, 0.0f);
	temp2[5].Set(-1.0f, 1.0f, 10.0f);
	temp2[6].Set(1.0f, 1.0f, 10.0f);
	temp2[7].Set(1.0f, -1.0f, 10.0f);
	temp2[8].Set(-1.0f, -1.0f, 10.0f);

	// Cube with an extra vertex on one of the faces.
	Assert(SortConvexHullPointsToFront(temp2, NULL, 9) == 8);

	temp2[9].Set(0.0f, 0.0f, 2.0f);

	// Cube with an extra vertex on one face *and* an extra vertex in the interior.
	Assert(SortConvexHullPointsToFront(temp2, NULL, 10) == 8);

	temp2[0].Set(0.0f, 0.0f, 0.0f);
//	temp2[1].Set(2.0f, 1.0f, 0.0f);
	temp2[1].Set(2.9f, 0.1f, 0.0f);
	temp2[2].Set(3.0f, 3.0f, 0.0f);
	temp2[3].Set(3.0f, 0.0f, 0.0f);
	temp2[4].Set(2.5f, 0.5f, 1.0f);

	// Something.
	Assert(SortConvexHullPointsToFront(temp2, NULL, 5) == 4);

	temp2[0].Set(0.0f, 0.0f, 0.0f);
	temp2[1].Set(2.0f, 1.0f, 0.0f);
	temp2[2].Set(3.0f, 3.0f, 0.0f);
	temp2[3].Set(0.0f, 0.0f, 1.0f);
	temp2[4].Set(3.0f, 0.0f, 0.0f);
	temp2[5].Set(2.0f, 1.0f, 1.0f);
	temp2[6].Set(3.0f, 3.0f, 1.0f);
	temp2[7].Set(3.0f, 0.0f, 1.0f);

	// Something.
	int retVal = SortConvexHullPointsToFront(temp2, NULL, 8);
	Assert(retVal == 6);

	const float kfTemp = 1.0f;
	temp2[0].Set(0.0f, 0.0f, 0.0f);
	temp2[1].Set(0.0f, 1.0f, 0.0f);
	temp2[2].Set(1.0f, 1.0f, 0.0f);
	temp2[3].Set(2.0f, 2.0f, 0.0f);
	temp2[4].Set(2.0f, 0.0f, 0.0f);
	temp2[5].Set(0.0f, 0.0f, kfTemp);
	temp2[6].Set(0.0f, 1.0f, kfTemp);
	temp2[7].Set(1.0f, 1.0f, kfTemp);
	temp2[8].Set(2.0f, 2.0f, kfTemp);
	temp2[9].Set(2.0f, 0.0f, kfTemp);

	/*int */retVal = SortConvexHullPointsToFront(temp2, NULL, 10);
	Assert(retVal == 8);

	temp2[0].Set(0.0f, 0.0f, 0.0f);
	temp2[1].Set(0.0f, 1.0f, 0.0f);
	temp2[2].Set(1.0f, 1.0f, 0.0f);
	temp2[3].Set(2.0f, 2.0f, 0.0f);
	temp2[4].Set(4.0f, 2.0f, 0.0f);
	temp2[5].Set(5.0f, 1.0f, 0.0f);
	temp2[6].Set(5.0f, 0.0f, 0.0f);
	temp2[7].Set(0.0f, 0.0f, 1.0f);
	temp2[8].Set(0.0f, 1.0f, 1.0f);
	temp2[9].Set(1.0f, 1.0f, 1.0f);
	temp2[10].Set(2.0f, 2.0f, 1.0f);
	temp2[11].Set(4.0f, 2.0f, 1.0f);
	temp2[12].Set(5.0f, 1.0f, 1.0f);
	temp2[13].Set(5.0f, 0.0f, 1.0f);

	// Car shaped point set.
	retVal = SortConvexHullPointsToFront(temp2, NULL, 14);
	Assert(retVal == 12);

	// Infinite loop ... debug code only.
	mthRandom localRandom;
	int iterCount = 0;
	while(true)
	{
		const int numVerts = 7;
		for(int i = 0; i < numVerts; ++i)
		{
			temp2[i].x = -10.0f + 20.0f * localRandom.GetFloat();
			//temp2[i].z = -0.1f + 0.2f * localRandom.GetFloat();
			temp2[i].y = -10.0f + 20.0f * localRandom.GetFloat();
			//temp2[i].z = -10.0f + 20.0f * localRandom.GetFloat();
			temp2[i].z = -1.0f + 2.0f * localRandom.GetFloat();
			//temp2[i].z = -0.1f + 0.2f * localRandom.GetFloat();
			if(temp2[i].z < 0.0f)
			{
				temp2[i].z = -1.0f;
			}
			else
			{
				temp2[i].z = +1.0f;
			}
		}
		// This particular iteration seems to be problematic.
		if(iterCount == 5878)
		{
			int x = 34;
			++x;
		}
		SortConvexHullPointsToFront(temp2, NULL, numVerts);
		++iterCount;
	}
#endif
	u16 *remappedVertices = NULL;
#if SORT_CONVEX_HULL_VERTS_TO_FRONT
	if(boundType != phBound::BVH)
	{
		remappedVertices = Alloca(u16, m_NumVertices);
		m_NumConvexHullVertices = (u16)(SortConvexHullPointsToFront(tempVertArray, remappedVertices, m_NumVertices));
	}
	else
#endif
	{
		// This really shouldn't be necessary and shouldn't make any difference - nobody should ever look at this value for a BVH.
		m_NumConvexHullVertices = (u16)(m_NumVertices);
	}

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
#if TEMP_HACK
	Assert(sm_SecondSurface);
	if(sm_SecondSurface)
	{
		Assert(m_NumSecondSurfaceVertexDisplacements==m_NumVertices);
		Assert(m_SecondSurfaceVertexDisplacements);
		for(i=0;i<m_NumSecondSurfaceVertexDisplacements;i++)
		{
			Assert(m_SecondSurfaceVertexDisplacements);
			m_SecondSurfaceVertexDisplacements[i]=0.5f;
		}
	}
#else
	if(sm_SecondSurface)
	{
		Assert(m_NumSecondSurfaceVertexDisplacements==m_NumVertices);
		Assert(m_SecondSurfaceVertexDisplacements);
		for(i=0;i<m_NumSecondSurfaceVertexDisplacements;i++)
		{
			Assert(m_SecondSurfaceVertexDisplacements);
			token.MatchIToken("f");
			const float fUnclamped=token.GetFloat();
			Assertf(fUnclamped>=0 && fUnclamped<=1.0f, "Displacement not in range (0,1)");
			const float fDisplacement=Clamp(fUnclamped,0.0f,1.0f);
			Assert(fDisplacement>=0.0f && fDisplacement<=1.0f);
			m_SecondSurfaceVertexDisplacements[i]=fDisplacement;
			m_SecondSurfaceVertexDisplacements[i]*=sm_SecondSurfaceMaxHeight;
		}
	}
#endif//TEMP_HACK
#endif//HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA

	Vec3V *tempShrunkVertArray = NULL;
    if (token.CheckToken("shrunk:"))
    {
		// Unfortunately we can't just read these vertices right into a compressed vertex because we need to know the bounding box before we can
		//   compress, and we need to see all of the vertices, and the primitives, to know the bounding box.  So we'll just read them into a temporary
		//   array for now and then delete it once we're done with it.
		sysMemStartTemp();
		tempShrunkVertArray = rage_new Vec3V[m_NumVertices];
		sysMemEndTemp();

		int i;
        for(i=0;i<m_NumVertices;i++)
        {
            token.MatchToken("v");
			tempShrunkVertArray[i].SetXf(token.GetFloat());
			tempShrunkVertArray[i].SetYf(token.GetFloat());
			tempShrunkVertArray[i].SetZf(token.GetFloat());
        }
    }

	for (i=0; i<numMaterialsToLoad; i++)
	{
		m_MaterialIds[i] = GetMaterialIdFromFile(token);
	#if __ASSERT && HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		// parsed matColorIDs should be in sync with color palette:
		const u32 matColorID = (u32(u64(m_MaterialIds[i])>>40))&0x000000ff;
		Assertf(matColorID <= m_NumMaterialColors,"phBoundGeometry: Invalid material color index found (%d) at index %d. Material colors palette size: %d.", matColorID, i, m_NumMaterialColors);
	#endif
	}

#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	for (i=0; i<numMaterialColorsToLoad; i++)
	{
		Color32 color;
		if(token.CheckIToken("materialcolor", true))
		{
			color.SetRed	(token.GetInt());
			color.SetGreen	(token.GetInt());
			color.SetBlue	(token.GetInt());
			color.SetAlpha	(token.GetInt());
		}
		m_MaterialColors[i] = color.GetColor();
	}
#endif

	if (token.CheckToken("vertexmaterial:") || token.CheckToken("vertedgematerial:"))
	{
		token.GetInt();
	}

	/// JUSTIN : This is going to be able to go away.
	for (i=0; i<sm_NumEdges; i++)
	{
		token.MatchIToken("edge");
		// Eat the edges
		token.GetInt();
		token.GetInt();

		if (readEdgeNormals)
		{
			Vector3 normal;
			token.GetVector(normal);
		}
	}

	sysMemStartTemp();
	// Make an extra polygon array for storing an extra triangle for every quadrangle in the file.
	phPolygon* quadHalves = rage_new phPolygon[m_NumPolygons];
	u8* quadMatIndexList = rage_new u8[m_NumPolygons];
	int numQuads = 0;
	sysMemEndTemp();

	char temp[256];
	for (i=0; i<m_NumPolygons; i++)
	{
		// Found out if the polygon is a triangle or a quadrangle.
		token.GetToken(temp,256);
		int v[4],m;
		bool isQuad=(strcmp(temp,"quad")==0);
		if(isQuad || (strcmp(temp,"tri")==0))
		{
			// Get the vertices.
			v[0] = token.GetInt();
			v[1] = token.GetInt();
			v[2] = token.GetInt();
			v[3] = isQuad ? token.GetInt() : 0;

			if(remappedVertices != NULL)
			{
				v[0] = remappedVertices[v[0]];
				v[1] = remappedVertices[v[1]];
				v[2] = remappedVertices[v[2]];
				v[3] = isQuad ? remappedVertices[v[3]] : 0;
			}

			// Get the material index.
			m = token.GetInt();
			Assert(m<=255);

			const bool eatIt = false;
			if (!token.CheckToken("tri",eatIt) && !token.CheckToken("quad",eatIt) && !token.CheckToken(""))
			{
				// Eat the edge numbers, which are ignored
				const bool mustExist = false;
				token.GetInt(mustExist);
				token.GetInt(mustExist);
				token.GetInt(mustExist);
				if(isQuad)
				{
					// One more edge if we're a quad
					token.GetInt(mustExist);
				}
			}

			if (isQuad)
			{
				if (SplitQuad012(static_cast<phPolygon::Index>(v[0]),static_cast<phPolygon::Index>(v[1]),static_cast<phPolygon::Index>(v[2]),static_cast<phPolygon::Index>(v[3])))
				{
					InitPolygonTriNoVerts(geomPolys[i], static_cast<phPolygon::Index>(v[0]), static_cast<phPolygon::Index>(v[1]), static_cast<phPolygon::Index>(v[2]));
					InitPolygonTriNoVerts(quadHalves[numQuads], static_cast<phPolygon::Index>(v[2]), static_cast<phPolygon::Index>(v[3]), static_cast<phPolygon::Index>(v[0]));
				}
				else
				{
					InitPolygonTriNoVerts(geomPolys[i], static_cast<phPolygon::Index>(v[1]), static_cast<phPolygon::Index>(v[2]), static_cast<phPolygon::Index>(v[3]));
					InitPolygonTriNoVerts(quadHalves[numQuads], static_cast<phPolygon::Index>(v[3]), static_cast<phPolygon::Index>(v[0]), static_cast<phPolygon::Index>(v[1]));
				}

				// Set the material index in the list of split quad material indices and increment the number of quads.
				quadMatIndexList[numQuads] = (u8)m;
				numQuads++;
			}
			else
			{
				InitPolygonTriNoVerts(geomPolys[i], static_cast<phPolygon::Index>(v[0]), static_cast<phPolygon::Index>(v[1]), static_cast<phPolygon::Index>(v[2]));
			}

			if (m_NumMaterials > 0)
			{
				// Set the material index.  IMPORTANT: Call SetMaterialId after InitTriangle or InitQuad.
				Assert(m<=m_NumMaterials);

				m_PolyMatIndexList[i] = (u8)m;
			}
		}
		else
		{
			// A potentially non-polygonal primitive.
			phPrimitive *curPrimitive = &geomPolys[i].GetPrimitive();
			if(strcmp(temp, "sphere") == 0)
			{
				AssertMsg(boundType == phBound::BVH, "phBoundGeometry::Load(): Found 'sphere' in non-BVH geometry bound.  Only BVH bounds can have non-polygonal primitives.");
				int centerIndex = token.GetInt();
				float radius = token.GetFloat();

				curPrimitive->SetType(PRIM_TYPE_SPHERE);
				phPrimSphere *spherePrim = &curPrimitive->GetSphere();
				spherePrim->SetCenterIndexAndRadius((u16)(centerIndex), radius);

				// Get the material index.
				m = token.GetInt();
				Assert(m<=m_NumMaterials);
				m_PolyMatIndexList[i] = (u8)m;
				// Adjust the bounding box in case this primitive sticks out.
				const ScalarV radiusAsScalarV = ScalarVFromF32(radius);
				boundingBoxMin = Min(boundingBoxMin, tempVertArray[centerIndex] - Vec3V(radiusAsScalarV));
				boundingBoxMax = Max(boundingBoxMax, tempVertArray[centerIndex] + Vec3V(radiusAsScalarV));
			}
			else if(strcmp(temp, "capsule") == 0)
			{
				AssertMsg(boundType == phBound::BVH, "phBoundGeometry::Load(): Found 'capsule' in non-BVH geometry bound.  Only BVH bounds can have non-polygonal primitives.");
				int endIndex0 = token.GetInt();
				int endIndex1 = token.GetInt();
				float radius = token.GetFloat();

				curPrimitive->SetType(PRIM_TYPE_CAPSULE);
				phPrimCapsule *capsulePrim = &curPrimitive->GetCapsule();
				capsulePrim->SetEndIndicesAndRadius((u16)(endIndex0), (u16)(endIndex1), radius);

				// Get the material index.
				m = token.GetInt();
				Assert(m<=m_NumMaterials);
				m_PolyMatIndexList[i] = (u8)m;

				// Adjust the bounding box in case this primitive sticks out.
				const ScalarV radiusAsScalarV = ScalarVFromF32(radius);
				boundingBoxMin = Min(boundingBoxMin, tempVertArray[endIndex0] - Vec3V(radiusAsScalarV));
				boundingBoxMax = Max(boundingBoxMax, tempVertArray[endIndex0] + Vec3V(radiusAsScalarV));
				boundingBoxMin = Min(boundingBoxMin, tempVertArray[endIndex1] - Vec3V(radiusAsScalarV));
				boundingBoxMax = Max(boundingBoxMax, tempVertArray[endIndex1] + Vec3V(radiusAsScalarV));
			}
			else if(strcmp(temp, "box") == 0)
			{
				AssertMsg(boundType == phBound::BVH, "phBoundGeometry::Load(): Found 'box' in non-BVH geometry bound.  Only BVH bounds can have non-polygonal primitives.");
				int vertexIndex0 = token.GetInt();
				int vertexIndex1 = token.GetInt();
				int vertexIndex2 = token.GetInt();
				int vertexIndex3 = token.GetInt();

				const Vec3V vert0(tempVertArray[vertexIndex0]);
				const Vec3V vert1(tempVertArray[vertexIndex1]);
				const Vec3V vert2(tempVertArray[vertexIndex2]);
				const Vec3V vert3(tempVertArray[vertexIndex3]);

#if !__NO_OUTPUT
				// Check that the vertices actually do specify opposite diagonals.  Note that this code does its check *before* vertices are
				//   quantized.  If boxes are passing the check here but then failing at run-time that could mean that quantization is to blame.
				const Vec3V diagonalLengths0(Dist(vert0, vert1), Dist(vert0, vert2), Dist(vert0, vert3));
				const Vec3V diagonalLengths1(Dist(vert2, vert3), Dist(vert1, vert3), Dist(vert1, vert2));
				const Vec3V diagonalLengthDiffs(Subtract(diagonalLengths0, diagonalLengths1));
				const Vec3V relativeThreshold = Scale(Scale(ScalarV(V_SIX),ScalarV(V_FLT_SMALL_3)), diagonalLengths0);
				const Vec3V absoluteThreshold = Vec3V(Scale(ScalarV(V_TWO), ScalarV(V_FLT_SMALL_2)));
				const Vec3V finalThreshold = Max(relativeThreshold, absoluteThreshold);
				const bool isOppositeDiagonals = (IsLessThanAll(Abs(diagonalLengthDiffs), finalThreshold) != 0);
				if(!isOppositeDiagonals)
				{
					// Temporarily downgraded this check to a warning - JWR
					Warningf("Box primitive with specified vertices <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f> is not a valid box!  The vertices specified are not opposite diagonals (%f, %f, %f) / (%f, %f, %f).", vert0.GetXf(), vert0.GetYf(), vert0.GetZf(), vert1.GetXf(), vert1.GetYf(), vert1.GetZf(), vert2.GetXf(), vert2.GetYf(), vert2.GetZf(), vert3.GetXf(), vert3.GetYf(), vert3.GetZf(), diagonalLengths0.GetXf(), diagonalLengths0.GetYf(), diagonalLengths0.GetZf(), diagonalLengths1.GetXf(), diagonalLengths1.GetYf(), diagonalLengths1.GetZf());
				}
#endif // !__NO_OUTPUT

				curPrimitive->SetType(PRIM_TYPE_BOX);
				phPrimBox *boxPrim = &curPrimitive->GetBox();
				boxPrim->SetVertexIndices((u16)(vertexIndex0), (u16)(vertexIndex1), (u16)(vertexIndex2), (u16)(vertexIndex3));

				// Get the material index.
				m = token.GetInt();
				Assert(m<=m_NumMaterials);
				m_PolyMatIndexList[i] = (u8)m;

				// Adjust the bounding box in case this primitive sticks out.
				Mat34V localBoxMatrix;
				Vec3V boxSize;
				ScalarV maxMargin;
				geomBoxes::ComputeBoxDataFromOppositeDiagonals(vert0, vert1, vert2, vert3, localBoxMatrix, boxSize, maxMargin);
				const Vec3V aabbHalfWidth = geomBoxes::ComputeAABBExtentsFromOBB(localBoxMatrix.GetMat33ConstRef(), Scale(ScalarV(V_HALF), boxSize));
				const Vec3V aabbCenter = localBoxMatrix.GetCol3();
				boundingBoxMin = Min(boundingBoxMin, aabbCenter - aabbHalfWidth);
				boundingBoxMax = Max(boundingBoxMax, aabbCenter + aabbHalfWidth);
			}
			else if(strcmp(temp, "cylinder") == 0)
			{
				AssertMsg(boundType == phBound::BVH, "phBoundGeometry::Load(): Found 'cylinder' in non-BVH geometry bound.  Only BVH bounds can have non-polygonal primitives.");
				int endIndex0 = token.GetInt();
				int endIndex1 = token.GetInt();
				float radius = token.GetFloat();

				curPrimitive->SetType(PRIM_TYPE_CYLINDER);
				phPrimCylinder *cylinderPrim = &curPrimitive->GetCylinder();
				cylinderPrim->SetEndIndicesAndRadius((u16)(endIndex0), (u16)(endIndex1), radius);

				// Get the material index.
				m = token.GetInt();
				Assert(m<=m_NumMaterials);
				m_PolyMatIndexList[i] = (u8)m;

				// Adjust the bounding box in case this primitive sticks out.
				const ScalarV radiusAsScalarV = ScalarVFromF32(radius);
				boundingBoxMin = Min(boundingBoxMin, tempVertArray[endIndex0] - Vec3V(radiusAsScalarV));
				boundingBoxMax = Max(boundingBoxMax, tempVertArray[endIndex0] + Vec3V(radiusAsScalarV));
				boundingBoxMin = Min(boundingBoxMin, tempVertArray[endIndex1] - Vec3V(radiusAsScalarV));
				boundingBoxMax = Max(boundingBoxMax, tempVertArray[endIndex1] + Vec3V(radiusAsScalarV));
			}
			else
			{
				Quitf(ERR_PHY_GEOM,"phBoundGeometry::Load() - Found unrecognized primitive type '%s'", temp);
				return false;
			}
		}
	}

	// Make a new polygon list to include the existing polygons plus all the extra split quad halves.
	const int numTriangles = m_NumPolygons + numQuads;
	Assertf(numTriangles < 0x10000, "Too many triangles (%d) in the bounds file '%s' - max is 65535. Remember that quads count as two triangles.", numTriangles, token.GetName());
	phPolygon* triangles = rage_new phPolygon[numTriangles];

	// Make a new polygon material index list.
	u8* triangleMatIndexList = rage_new u8[numTriangles];

	// Copy the polygons and the material indices into the new lists.
	for (int polyIndex=0; polyIndex<m_NumPolygons; polyIndex++)
	{
		triangles[polyIndex] = m_Polygons[polyIndex];
		triangleMatIndexList[polyIndex] = m_PolyMatIndexList[polyIndex];
	}

	// Copy the quad halves and their material indices into the new lists.
	for (int quadIndex=0; quadIndex<numQuads; quadIndex++)
	{
		int triIndex = m_NumPolygons+quadIndex;
		triangles[triIndex] = quadHalves[quadIndex];
		triangleMatIndexList[triIndex] = quadMatIndexList[quadIndex];
	}

	// Delete the old polygon and material index lists.
	sysMemStartTemp();
	delete[] m_Polygons;
	delete[] m_PolyMatIndexList;

	// Delete the list of quad halves and the list of material indices.
	delete[] quadHalves;
	delete[] quadMatIndexList;

	// Delete this and reallocate it with the correct size
	sysMemEndTemp();

	// Set the new number of polygons and reassign the polygon list pointer and the material index list pointer.
	m_NumPolygons = numTriangles;
	m_Polygons = triangles;
	m_PolyMatIndexList = triangleMatIndexList;

	// Now that we've figured everything out (in particular we've figured out the final bounding box, which allows us to figure out how to compress the
	//   vertices if appropriate), let's fin
	// First, set the bounding box.
	SetBoundingBoxMin(boundingBoxMin);
	SetBoundingBoxMax(boundingBoxMax);

#if COMPRESSED_VERTEX_METHOD > 0
	CalculateQuantizationValues();
#endif

	const int numVertices = m_NumVertices;
	for(int vertIndex = 0; vertIndex < numVertices; ++vertIndex)
	{
		SetVertex(vertIndex, tempVertArray[vertIndex]);
	}

#if COMPRESSED_VERTEX_METHOD == 0
	if(m_Vertices != NULL)
	{
		Vector3 *vertices = reinterpret_cast<Vector3*>(const_cast<Vec3V *>(m_Vertices));
		// Set the extra (unused) vertices to zero to avoid triggering NAN math in fourtified functions.
		for (i=m_NumVertices; i<Fourtify(m_NumVertices); i++)
		{
			vertices[i].Zero();
		}
	}
#endif

	if(tempShrunkVertArray != NULL)
	{
#if COMPRESSED_VERTEX_METHOD == 0
		m_ShrunkVertices = rage_new Vec3V[numVertices];
#else	// COMPRESSED_VERTEX_METHOD == 0
		m_CompressedShrunkVertices = rage_new CompressedVertexType[3 * numVertices];
#endif	// COMPRESSED_VERTEX_METHOD == 0

		for(int vertIndex = 0; vertIndex < numVertices; ++vertIndex)
		{
			SetShrunkVertex(vertIndex, tempShrunkVertArray[vertIndex]);
		}

		sysMemStartTemp();
		delete [] tempShrunkVertArray;
		sysMemEndTemp();
	}

#if 0 // BS#1680840: TODO - load/set per-vertex attribs here:
	if(m_NumPerVertexAttribs > 0)
	{
		for(int i=0; i<m_numVertices; i++)
		{
			for(int att=0; att<m_NumPerVertexAttribs; att++)
			{
				Color32 colorValue = 0;
				SetVertexAttrib(i, att, colorValue);
			}
		}
	}//if(m_NumPerVertexAttribs > 0)...
#endif

	// Calculate a bounding sphere based on the vertices.  This has to be called after the vertices are finalized.
	Vec3V boundingSphereCenter;
	ScalarV boundingSphereRadius;
	CalculateBoundingSphere(boundingSphereCenter, boundingSphereRadius);

	// Loop over the primitives and compute their areas.  Because this accesses the vertices this needs to be done after the vertices are finalized.
	// For BVH bounds we will also check for non-triangular primitives and make sure that the bounding sphere contains them.  It would have been nice
	//   to do this earlier, when loading the primitives (where we update the bounding box to account for the primitives) but we really should
	//   probably wait until the vertices are finalized.  As an alternative, we could compute the bounding sphere with pre-finalized vertices and then
	//   add one quantization unit to the radius.
	for(int primIndex = 0; primIndex < numTriangles; ++primIndex)
	{
		phPolygon &curPoly = triangles[primIndex];
		const phPrimitive &curPrim = curPoly.GetPrimitive();
		const PrimitiveType primType = curPrim.GetType();

		Assert(boundType == phBound::BVH || primType == PRIM_TYPE_POLYGON);
		switch(primType)
		{
			case PRIM_TYPE_POLYGON:
			{
				const int vertIndex0 = curPoly.GetVertexIndex(0);
				const int vertIndex1 = curPoly.GetVertexIndex(1);
				const int vertIndex2 = curPoly.GetVertexIndex(2);
				const Vec3V vert0 = GetVertex(vertIndex0);
				const Vec3V vert1 = GetVertex(vertIndex1);
				const Vec3V vert2 = GetVertex(vertIndex2);
				curPoly.CalculateNormalAndArea(vert0, vert1, vert2);
				break;
			}
			case PRIM_TYPE_SPHERE:
			{
				Assert(boundType == phBound::BVH);
				const phPrimSphere &spherePrim = curPrim.GetSphere();
				const int centerIndex = spherePrim.GetCenterIndex();
				const Vec3V spherePrimCenter = GetVertex(centerIndex);
				const ScalarV spherePrimRadius = spherePrim.GetRadiusV();
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, spherePrimCenter, spherePrimRadius);
				break;
			}
			case PRIM_TYPE_CAPSULE:
			{
				Assert(boundType == phBound::BVH);
				const phPrimCapsule &capsulePrim = curPrim.GetCapsule();
				const int endIndex0 = capsulePrim.GetEndIndex0();
				const int endIndex1 = capsulePrim.GetEndIndex1();
				const Vec3V capsuleEnd0 = GetVertex(endIndex0);
				const Vec3V capsuleEnd1 = GetVertex(endIndex1);
				const ScalarV capsulePrimRadius = capsulePrim.GetRadiusV();
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, capsuleEnd0, capsulePrimRadius);
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, capsuleEnd1, capsulePrimRadius);
				break;
			}
			case PRIM_TYPE_BOX:
			{
				Assert(boundType == phBound::BVH);
				const phPrimBox &boxPrim = curPrim.GetBox();
				const int vertIndex0 = boxPrim.GetVertexIndex(0);
				const int vertIndex1 = boxPrim.GetVertexIndex(1);
				const int vertIndex2 = boxPrim.GetVertexIndex(2);
				const int vertIndex3 = boxPrim.GetVertexIndex(3);
				const Vec3V vert0 = GetVertex(vertIndex0);
				const Vec3V vert1 = GetVertex(vertIndex1);
				const Vec3V vert2 = GetVertex(vertIndex2);
				const Vec3V vert3 = GetVertex(vertIndex3);
				const ScalarV vsHalf(V_HALF);
				const Vec3V vert0b = Scale(vert1 + vert2 + vert3 - vert0, vsHalf);
				const Vec3V vert1b = Scale(vert0 + vert2 + vert3 - vert1, vsHalf);
				const Vec3V vert2b = Scale(vert0 + vert1 + vert3 - vert2, vsHalf);
				const Vec3V vert3b = Scale(vert0 + vert1 + vert2 - vert3, vsHalf);
				const ScalarV vsZero(V_ZERO);
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, vert0b, vsZero);
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, vert1b, vsZero);
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, vert2b, vsZero);
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, vert3b, vsZero);
				break;
			}
			default:
			{
				Assert(boundType == phBound::BVH);
				Assert(primType == PRIM_TYPE_CYLINDER);
				const phPrimCylinder &cylinderPrim = curPrim.GetCylinder();
				const int endIndex0 = cylinderPrim.GetEndIndex0();
				const int endIndex1 = cylinderPrim.GetEndIndex1();
				const Vec3V cylinderEnd0 = GetVertex(endIndex0);
				const Vec3V cylinderEnd1 = GetVertex(endIndex1);
				const ScalarV cylinderPrimRadius = cylinderPrim.GetRadiusV();
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, cylinderEnd0, cylinderPrimRadius);
				ExpandBoundingSphereForSphere(boundingSphereCenter, boundingSphereRadius, cylinderEnd1, cylinderPrimRadius);
				break;
			}
		}

	}

	// At long last let's commit the bounding sphere that we've determined.
	m_RadiusAroundCentroid = boundingSphereRadius.Getf();
	// I think this should be here but I'm not positive.
	//m_RadiusAroundCentroid += GetMargin();
	phBound::SetCentroidOffset(boundingSphereCenter);

	// This depends on the vertices being finalized and the polygons having their areas calculated.
	RemoveDegeneratePolys(0.4f);

	// If we're not a BVH bound, let's compute the volume of this bound using the triangles.  If we are a BVH, we won't need the volume to be computed, and it's
	//   likely that the volume is undefined anyway due to the polygons not forming a closed surface (or surfaces).
	if(GetType() != phBound::BVH)
	{
		Vec3V angInertia;
		const phPolygon *polygons = m_Polygons;
		phMathInertia::FindGeomAngInertia(1.0f, &(RCC_VECTOR3(*tempVertArray)), polygons, m_NumPolygons, VEC3V_TO_VECTOR3(GetCGOffset()), RC_VECTOR3(angInertia));
		m_VolumeDistribution.SetXYZ(angInertia);

		ScalarV scaledVolume(V_ZERO);	// Since it happens to be more computationally convenient, we'll actually accumulate six times the area and then divide by six at the end.
		for(int polyIndex = 0; polyIndex < m_NumPolygons; ++polyIndex)
		{
			const phPolygon &poly = polygons[polyIndex];
			const ScalarV newScaledVolume = Dot(GetPolygonNonUnitNormal(polyIndex), tempVertArray[poly.GetVertexIndex(0)]);
			scaledVolume = Add(scaledVolume, newScaledVolume);
		}
		m_VolumeDistribution.SetW(Scale(scaledVolume, Scale(ScalarV(V_THIRD), ScalarV(V_HALF))));
	}

	sysMemStartTemp();
	delete [] tempVertArray;
	sysMemEndTemp();

	// This has to happen after RemoveDegeneratePolys() because we don't want to make neighbors out of polygons that are just going to be removed.
	ComputeNeighbors(token.GetName());

	//////////////////////////////////////////////////////////////////////////
	// Has to happen after ComputeNeighbors because we use neighbor connectivity to make iterating around polys that own vertices easy
	sysMemStartTemp();
	Vec3V* vertexNormals = rage_new Vec3V[m_NumVertices];
	sysMemEndTemp();
	
	// Compute and compress into each polygon
	ComputeVertexNormals(vertexNormals);
	SetCompressedVertexNormals(vertexNormals);

	sysMemStartTemp();
	delete [] vertexNormals;
	sysMemEndTemp();
	//////////////////////////////////////////////////////////////////////////

	if (GetType() == phBound::GEOMETRY)
	{
		const float DEFAULT_GEOM_MARGIN = 0.04f;
		SetMarginAndShrink(DEFAULT_GEOM_MARGIN);

		// This probably has wait to happen until the vertices have been finalized.
		ComputeOctantMap();
	}

	// Use centroid as center-of-gravity if 'cg:' not specified in data file.
	if(!loadedCG)
	{
		// No center of gravity offset was specified, so use m_CentroidOffset as default m_CGOffset.
		SetCGOffset(GetCentroidOffset());
	}

	return true;
}

bool phBoundGeometry::CheckIsStreamable(fiAsciiTokenizer &token)
{
	LoadGeomStats_v110(token);
	if (!sm_NumVertices)
		return false;

#if 0 // needs pgPaging::GetLargestPageSize
	if (	(!sm_NumVertices)
		||	((int)sizeof(phPolygon) * sm_NumPolygons > pgPaging::GetLargestPageSize())
		||	((int)sizeof(Vector3) * sm_NumVertices > pgPaging::GetLargestPageSize())
		)
	{
		Errorf("Non-streamable bound");
		Errorf("Numbers:");
		Errorf("     m_Polygons: %d (Max: %d)", sm_NumPolygons, pgPaging::GetLargestPageSize() / sizeof(phPolygon));
		Errorf("     m_Vertices: %d (Max: %d)", sm_NumVertices, pgPaging::GetLargestPageSize() / sizeof(Vector3));
		return false;
	}
#endif
	return true;
}

#endif //RSG_TOOL || __RESOURCECOMPILER

#if !__FINAL && (RSG_TOOL || __RESOURCECOMPILER)
bool phBoundGeometry::Save_v110 (fiAsciiTokenizer & token)
{
	token.PutDelimiter("verts: ");
	token.Put( m_NumVertices );
	token.PutDelimiter("\n");

	if (m_NumVertices==0)
	{
		// Don't output materials, edges, polys, etc. if no vertices
		return true;
	}

	// Don't write the centroid offset, because it is not read in the Load function (it's calculated
	// after loading).
	// cg offset
	if (!IsEqualAll(GetCGOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("cg: ");
		token.Put(GetCGOffset());
		token.PutDelimiter("\n");
	}

    token.PutDelimiter("margin: ");
    token.Put(GetMargin());
    token.PutDelimiter("\n");

	token.PutDelimiter("\n");

	token.PutDelimiter("materials: ");
	token.Put(m_NumMaterials);
	token.PutDelimiter("\n");

#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	token.PutDelimiter("materialcolors: ");
	token.Put(m_NumMaterialColors);
	token.PutDelimiter("\n");
#endif

	if(m_NumPerVertexAttribs > 0)
	{
		token.PutDelimiter("pervertattribs: ");
		token.Put(m_NumPerVertexAttribs);
		token.PutDelimiter("\n");
	}

	token.PutDelimiter("edges: ");
	int nDummy = 0;
	token.Put( nDummy );
	token.PutDelimiter("\n");

	token.PutDelimiter("polys: ");
	token.Put( m_NumPolygons );
	token.PutDelimiter("\n");
	
	token.PutDelimiter("readedgenormals: ");
	token.Put( 1 );
	token.PutDelimiter("\n");
	token.PutDelimiter("\n");
	
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	token.PutDelimiter("secondSurface: ");
	if( !GetHasSecondSurface() )
		m_NumSecondSurfaceVertexDisplacements = 0;
	
	token.Put( m_NumSecondSurfaceVertexDisplacements  );

	token.PutDelimiter("\n");
	token.PutDelimiter("secondSurfaceMaxHeight: ");
	token.Put( sm_SecondSurfaceMaxHeight );
	token.PutDelimiter("\n");
	token.PutDelimiter("\n");
#endif

	int i;
	for(i=0;i<m_NumVertices;i++)
	{
		const Vector3 vert = VEC3V_TO_VECTOR3(GetVertex(i));
		token.PutDelimiter("v ");
		token.Put( vert.x );
		token.Put( vert.y );
		token.Put( vert.z );
		token.PutDelimiter("\n");
	}
	token.PutDelimiter("\n");
	
	if(m_NumPerVertexAttribs > 0)
	{
		for(i=0;i<m_NumVertices;i++)
		{
			const Color32 col = GetVertexAttrib(i);
			token.PutDelimiter("va ");
			token.Put( col.GetRed() );
			token.Put( col.GetGreen() );
			token.Put( col.GetBlue() );
			token.Put( col.GetAlpha() );
			token.PutDelimiter("\n");
		}
		token.PutDelimiter("\n");
	}

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	if( GetHasSecondSurface() )
	{
		for(i=0;i<m_NumVertices;i++)
		{
			const float flt = GetSecondSurfaceVertexDisplacement(i);
			token.PutDelimiter("f ");
			token.Put( flt );
			token.PutDelimiter("\n");
		}
		token.PutDelimiter("\n");
	}
#endif

#if COMPRESSED_VERTEX_METHOD == 0
    if (m_ShrunkVertices)
#else
	if (m_CompressedShrunkVertices)
#endif
    {
        token.PutDelimiter("shrunk:\n");
        int i;
        for(i=0;i<m_NumVertices;i++)
        {
            const Vector3 vert = VEC3V_TO_VECTOR3(GetShrunkVertex(i));
            token.PutDelimiter("v ");
            token.Put( vert.x );
            token.Put( vert.y );
            token.Put( vert.z );
            token.PutDelimiter("\n");
        }
        token.PutDelimiter("\n");
    }

	for (i=0; i<m_NumMaterials; i++)
	{
		WriteMaterialIdToFile(token, m_MaterialIds[i]);
		token.PutDelimiter("\n");
	}

#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	for( i=0; i<m_NumMaterialColors; i++ )
	{
		
		char buf[256];
		Color32 col = GetMaterialColorDirectly(i);
		sprintf(buf, "materialcolor %d %d %d %d", (int)col.GetRed(), (int)col.GetGreen(), (int)col.GetBlue(), (int)col.GetAlpha());
		token.Put(buf);
		token.PutDelimiter("\n");
	}
#endif

	for (i=0; i<m_NumPolygons; i++)
	{
		const phPrimitive *curPrimitive;
		if(GetType() == phBound::BVH)
			curPrimitive = &static_cast<phBoundBVH*>(this)->GetPrimitive(i);
		else
			curPrimitive = &GetPolygon(i).GetPrimitive();
		
		if(curPrimitive->GetType() == PRIM_TYPE_POLYGON)
		{
			const phPolygon& poly = curPrimitive->GetPolygon();
			token.PutDelimiter("tri ");
			token.Put ( (int)poly.GetVertexIndex(0) );
			token.Put ( (int)poly.GetVertexIndex(1) );
			token.Put ( (int)poly.GetVertexIndex(2) );

			token.Put ( m_PolyMatIndexList[i] );

			// There are no edge indices
			token.Put ( 0 );
			token.Put ( 0 );
			token.Put ( 0 );
			token.PutDelimiter("\n");
		}
		else if(curPrimitive->GetType() == PRIM_TYPE_SPHERE)
		{
			const phPrimSphere &spherePrim = curPrimitive->GetSphere();
			token.PutDelimiter("sphere ");
			token.Put((int)spherePrim.GetCenterIndex());
			token.Put(spherePrim.GetRadius());

			// Write out the material and the closing "\n".
			token.Put ( m_PolyMatIndexList[i] );
			token.PutDelimiter("\n");
		}
		else if(curPrimitive->GetType() == PRIM_TYPE_CAPSULE)
		{
			const phPrimCapsule &capsulePrim = curPrimitive->GetCapsule();
			token.PutDelimiter("capsule ");
			token.Put((int)capsulePrim.GetEndIndex0());
			token.Put((int)capsulePrim.GetEndIndex1());
			token.Put(capsulePrim.GetRadius());

			// Write out the material and the closing "\n".
			token.Put ( m_PolyMatIndexList[i] );
			token.PutDelimiter("\n");
		}
		else if(curPrimitive->GetType() == PRIM_TYPE_CYLINDER)
		{
			const phPrimCylinder &capsulePrim = curPrimitive->GetCylinder();
			token.PutDelimiter("cylinder ");
			token.Put((int)capsulePrim.GetEndIndex0());
			token.Put((int)capsulePrim.GetEndIndex1());
			token.Put(capsulePrim.GetRadius());

			// Write out the material and the closing "\n".
			token.Put ( m_PolyMatIndexList[i] );
			token.PutDelimiter("\n");
		}
		else
		{
			Assert(curPrimitive->GetType() == PRIM_TYPE_BOX);
			const phPrimBox &boxPrim = curPrimitive->GetBox();
			token.PutDelimiter("box ");
			token.Put((int)boxPrim.GetVertexIndex(0));
			token.Put((int)boxPrim.GetVertexIndex(1));
			token.Put((int)boxPrim.GetVertexIndex(2));
			token.Put((int)boxPrim.GetVertexIndex(3));

			// Write out the material and the closing "\n".
			token.Put ( m_PolyMatIndexList[i] );
			token.PutDelimiter("\n");
		}
	}
	token.PutDelimiter("\n");
	return true;
}

#endif	// end of #if !__FINAL// && !IS_CONSOLE


#if !__FINAL// && !IS_CONSOLE
bool phBoundGeometry::Save (const char *file) const
{
	fiStream *s;
	
	s = ASSET.Create(file,"bnd");
	if (s == NULL)
	{
		Quitf("phBoundGeom::Save() - Can't open file '%s'",file);
		return false;
	}

	fiAsciiTokenizer token;
	token.Init(file,s);

	bool rv;
	rv = Save(token);

	s->Close();

	return rv;
}


bool phBoundGeometry::Save (fiAsciiTokenizer & t) const
{
	// save only the current version... VERSION_110

	int i, j;

	t.PutDelimiter("version: 1.10\n");
	t.PutDelimiter("type: geometry\n\n");

	t.PutDelimiter("verts: ");
	t.Put(m_NumVertices);
	t.PutDelimiter("\n");
	t.PutDelimiter("materials: ");
	t.Put(m_NumMaterials);
	t.PutDelimiter("\n");
	t.PutDelimiter("edges: ");
	int nDummy = 0;
	t.Put(nDummy);
	t.PutDelimiter("\n");
	t.PutDelimiter("polys: ");
	t.Put(m_NumPolygons);
	t.PutDelimiter("\n");

	t.PutDelimiter("\n");
	for(i=0;i<m_NumVertices;i++)
	{
		t.PutDelimiter("v\t");
		t.Put(GetVertex(i));
		t.PutDelimiter("\n");
	}

#if COMPRESSED_VERTEX_METHOD == 0
    if (m_ShrunkVertices)
#else
	if(m_CompressedShrunkVertices)
#endif
    {
        t.PutDelimiter("shrunk:\n");
        int i;
        for(i=0;i<m_NumVertices;i++)
        {
            const Vector3 vert = VEC3V_TO_VECTOR3(GetShrunkVertex(i));
            t.PutDelimiter("v ");
            t.Put( vert.x );
            t.Put( vert.y );
            t.Put( vert.z );
            t.PutDelimiter("\n");
        }
        t.PutDelimiter("\n");
    }

	t.PutDelimiter("\n");
	for(i=0;i<m_NumMaterials;i++)
	{
		WriteMaterialIdToFile(t, m_MaterialIds[i]);
		t.PutDelimiter("\n");
	}

	t.PutDelimiter("\n");

	t.PutDelimiter("\n");
	for(i=0;i<m_NumPolygons;i++)
	{
		const phPolygon * p = m_Polygons+i;
		phMaterialIndex m = m_PolyMatIndexList[i];

		t.PutDelimiter("tri ");
		for (j = 0; j < POLY_MAX_VERTICES; j++)	// vert indices
		{
			t.Put((int)p->GetVertexIndex(j));
			t.PutDelimiter(" ");
		}
		t.PutDelimiter("  ");
		t.Put(m);	// material index
		t.PutDelimiter("   ");
		for (j = 0; j < POLY_MAX_VERTICES; j++)	// edge indices
		{
			t.Put ( 0 ); // There are no edge indices...
			t.PutDelimiter(" ");
		}
		t.PutDelimiter("\n");
	}

	t.PutDelimiter("\n");

	return true;
}

#endif	// end of #if !__FINAL// && !IS_CONSOLE



void phBoundGeometry::SetMaterialIdEntry (int index, phMaterialMgr::Id id)
{
	m_MaterialIds[index] = id;
}


void phBoundGeometry::SetPolygonMaterialIndex (int polygonIndex, phMaterialIndex materialIndex)
{
	m_PolyMatIndexList[polygonIndex] = (u8)materialIndex;
}

void phBoundGeometry::SetMaterial (phMaterialMgr::Id materialId,phMaterialIndex materialIndex)
{
	// Set all of the material index numbers to the new material index.
	if(materialIndex >= 0 && materialIndex < m_NumMaterials)
	{
		m_MaterialIds[materialIndex] = materialId;
	}
	else
	{
		for (int i=0; i<m_NumMaterials; i++)
		{
			m_MaterialIds[i] = materialId;
		}
	}
}

void phBoundGeometry::SwapZeroAndFirstMaterials()
{
	if (m_NumMaterials > 1)
	{
		SwapEm(m_MaterialIds[0], m_MaterialIds[1]);

		for (int polyIndex = 0; polyIndex < m_NumPolygons; ++polyIndex)
		{
			if (m_PolyMatIndexList[polyIndex] == 0)
			{
				m_PolyMatIndexList[polyIndex] = 1;
			}
			else if (m_PolyMatIndexList[polyIndex] == 1)
			{
				m_PolyMatIndexList[polyIndex] = 0;
			}
		}
	}
}

void phBoundGeometry::ScaleSize (float xScale, float yScale, float zScale)
{
#if COMPRESSED_VERTEX_METHOD == 0
	{
		// Get a mutable vertex pointer.
		Vector3* geomVerts = const_cast<Vector3*>(reinterpret_cast<const Vector3*>(m_Vertices));

		if (xScale>0.0f)
		{
			// Scale all the vertices in the x direction.
			for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
			{
				geomVerts[vertexIndex].x *= xScale;
			}
		}

		if (yScale>0.0f)
		{
			// Scale all the vertices in the y direction.
			for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
			{
				geomVerts[vertexIndex].y *= yScale;
			}
		}

		if (zScale>0.0f)
		{
			// Scale all the vertices in the z direction.
			for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
			{
				geomVerts[vertexIndex].z *= zScale;
			}
		}
	}
#endif
#if COMPRESSED_VERTEX_METHOD > 0
	{
		// For compressed-vertex polyhedral bounds, we just need to recalculate the quantization value.
		Vec3V scale(xScale, yScale, zScale);
		SetBoundingBoxMin(GetBoundingBoxMin()*scale);
		SetBoundingBoxMax(GetBoundingBoxMax()*scale);
		CalculateQuantizationValues();
	}
#endif

	// Get a mutable polygon pointer.
	phPolygon* geomPolys = const_cast<phPolygon*>(m_Polygons);

	// Recompute all the polygon normals and areas.
	for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
	{
		phPolygon &curPoly = geomPolys[polygonIndex];
		curPoly.CalculateNormalAndArea(GetVertex(curPoly.GetVertexIndex(0)), GetVertex(curPoly.GetVertexIndex(1)), GetVertex(curPoly.GetVertexIndex(2)));
	}

	Vector3 tempCGOffset = VEC3V_TO_VECTOR3(GetCGOffset());
	// Scale the center of mass offset.
	if (xScale>0.0f)
	{
		tempCGOffset.x *= xScale;
	}
	if (yScale>0.0f)
	{
		tempCGOffset.y *= yScale;
	}
	if (zScale>0.0f)
	{
		tempCGOffset.z *= zScale;
	}

	SetCGOffset(RCC_VEC3V(tempCGOffset));

	// Recompute the bound extents.
	CalculateExtents();
}


void phBoundGeometry::ScaleSize (const Vector3& scale)
{
	ScaleSize(scale.x,scale.y,scale.z);
}


#if COMPRESSED_VERTEX_METHOD == 0
void phBoundGeometry::Transform (const Matrix34& mat)
{
	// NOTE: mutable pointers since phBoundGeometry
	// controls its own components
	Vector3 * geomVerts = (Vector3*)m_Vertices;

	for(int i=0;i<m_NumVertices;i++) 
	{
		mat.Transform(geomVerts[i]);
	}

	CalculateExtents();
}
#endif


void phBoundGeometry::ScaleSize (float xyzScale)
{
	ScaleSize(xyzScale,xyzScale,xyzScale);
}


void phBoundGeometry::SetCentroidOffset (Vec3V_In offset)
{
	// NOTE: this function may not be directly undoable
	// due to roundoff errors
	Vec3V offsetDelta = offset - GetCentroidOffset();
	if (!IsZeroAll(offsetDelta))
	{
		ShiftCentroidOffset(offsetDelta);
	}
}


void phBoundGeometry::ShiftCentroidOffset (Vec3V_In offsetDelta)
{
#if COMPRESSED_VERTEX_METHOD == 0
	// NOTE: mutable pointers since phBoundGeometry
	// controls its own components
	Vector3 * geomVerts = (Vector3*)m_Vertices;

	for(int i=0;i<m_NumVertices;i++)
	{
		geomVerts[i].Add(RCC_VECTOR3(offsetDelta));
	}
#else
	m_BoundingBoxCenter = Add(m_BoundingBoxCenter, offsetDelta);
	SetBoundingBoxMin(GetBoundingBoxMin() + offsetDelta);
	SetBoundingBoxMax(GetBoundingBoxMax() + offsetDelta);
#endif

	phBound::SetCentroidOffset(GetCentroidOffset() + offsetDelta);

	CalculateExtents();
}

static bool CoLinearByAngle(const Vector3 &a, const Vector3 &b, const Vector3 &c, const float minTheta)
{
	Vector3 AB, BC;
	float dot, theta;

	AB.Subtract(b,a);
	BC.Subtract(c,b);

	AB.Normalize();
	BC.Normalize();

	dot = fabsf(Clamp(AB.Dot(BC),-1.0f,1.0f));
	theta = acosf(dot);

	return (theta < minTheta);
}


#if !__FINAL// && !IS_CONSOLE
// OPTIMIZE - it currently doesn't check for duplicate geometry
// TODO - This currently doesn't do anything to handle updating/sorting the convex hull points.  Is this ever used on non-BVH bounds?  (Or bounds
//   that aren't destined to become BVH bounds?)
void phBoundGeometry::CombineBounds(const phBoundGeometry* const bnds[],int numBnds)
{
	u8* geomMatIndexList = const_cast<u8*>(m_PolyMatIndexList);

	// figure out materials:
	int oldNumMaterials	=m_NumMaterials;
	int numMaterials	=m_NumMaterials;
	int oldNumVerts		=m_NumVertices;
	int numVerts		=m_NumVertices;
	int	numPolys		=m_NumPolygons;

	// init counters:
	int i,j,k,l;
	for (i=0;i<numBnds;i++)
	{
		numMaterials	+=bnds[i]->GetNumMaterials();
		numVerts		+=bnds[i]->GetNumVertices();
		numPolys		+=bnds[i]->GetNumPolygons();
	}

	phMaterialMgr::Id* newMaterialIndices = rage_new phMaterialMgr::Id[Max(numMaterials, 4)];
	int* bndMaterialMappings = Alloca(int,numMaterials);

	// map our current materials:
	for (i=0;i<m_NumMaterials;i++)
	{
		newMaterialIndices[i] = m_MaterialIds[i];
		bndMaterialMappings[i] = i;
	}

	// map the other bound materials (we see if we already have the material:)
	int newNumMaterials=m_NumMaterials;
	int mapping=m_NumMaterials;

	// loop through all bounds and add materials:
	for (j=0;j<numBnds;j++)
	{
		for (k=0;k<bnds[j]->GetNumMaterials();k++)
		{
			bool foundMaterial=false;

			for (l=0;l<newNumMaterials;l++)
				// material is already there, put it in the mapping:
				if (newMaterialIndices[l]==bnds[j]->m_MaterialIds[k])
				{
					bndMaterialMappings[mapping++]=l;
					foundMaterial=true;
					break;
				}

			// new material, add it to the array and add mapping:
			if (!foundMaterial)
			{
				newMaterialIndices[newNumMaterials]=bnds[j]->m_MaterialIds[k];
				bndMaterialMappings[mapping++]=newNumMaterials++;
			}
		}
	}
	
	// out with the old, in with the new:
	delete [] m_MaterialIds;
	m_MaterialIds=newMaterialIndices;
	Assert(newNumMaterials>=0 && newNumMaterials<=255);
	m_NumMaterials = (u8)newNumMaterials;

	// copy verts from this bound:
	Vector3* newVertices=rage_new Vector3[Fourtify(numVerts)];
	for (i=0;i<m_NumVertices;i++)
	{
		newVertices[i]=VEC3V_TO_VECTOR3(GetVertex(i));
	}

	// We really only need to calculate the bounding box as we go 
	Vec3V newBBMin = GetBoundingBoxMin();
	Vec3V newBBMax = GetBoundingBoxMax();

	// add verts from other bounds:
	for (j=0;j<numBnds;j++)
	{
		newBBMin = Min(newBBMin, bnds[j]->GetBoundingBoxMin());
		newBBMax = Max(newBBMax, bnds[j]->GetBoundingBoxMax());
		for (k=0;k<bnds[j]->GetNumVertices();k++)
		{
			newVertices[m_NumVertices++]=VEC3V_TO_VECTOR3(bnds[j]->GetVertex(k));
		}
	}

#if COMPRESSED_VERTEX_METHOD == 0
	{
		// NOTE: mutable pointers since phBoundGeometry
		// controls its own components
		Vector3 * geomVerts = (Vector3*)m_Vertices;
		delete [] geomVerts;
		m_Vertices = (Vec3V*)newVertices;
	}
#endif
#if COMPRESSED_VERTEX_METHOD > 0
	{
		InitQuantization(newBBMin, newBBMax);
		for(int i = 0; i < numVerts; ++i)
		{
			SetVertex(i, RCC_VEC3V(newVertices[i]));
		}
	}
#endif

	// copy polygons from this bound:
	phPolygon* newPolygons=rage_new phPolygon[numPolys];
	u8* newMaterialIds = rage_new u8[numPolys];

	for (i=0;i<m_NumPolygons;i++)
	{
		InitPolygonTri( 
			newPolygons[i], 
			m_Polygons[i].GetVertexIndex(0),
			m_Polygons[i].GetVertexIndex(1),
			m_Polygons[i].GetVertexIndex(2) );

		newMaterialIds[i] = m_PolyMatIndexList[i];
	}

	// copy polygons from other bounds:
	int offsetVert = oldNumVerts;
	int offsetMat = oldNumMaterials;
	for (j=0;j<numBnds;j++)
	{
		for (k=0;k<bnds[j]->GetNumPolygons();k++)
		{
			const phPolygon& poly=bnds[j]->m_Polygons[k];
			InitPolygonTri(
				newPolygons[m_NumPolygons], 
				static_cast<phPolygon::Index>(poly.GetVertexIndex(0)+offsetVert),
			    static_cast<phPolygon::Index>(poly.GetVertexIndex(1)+offsetVert),
			    static_cast<phPolygon::Index>(poly.GetVertexIndex(2)+offsetVert) );

			newMaterialIds[m_NumPolygons] = bnds[j]->m_PolyMatIndexList[k];

			m_NumPolygons++;

		}
		offsetVert	+=bnds[j]->GetNumVertices();
		offsetMat	+=bnds[j]->GetNumMaterials();

		m_NumConvexHullVertices = m_NumConvexHullVertices + bnds[j]->m_NumConvexHullVertices;
	}

	phPolygon * geomPolys = (phPolygon*)m_Polygons;
	delete [] geomPolys;
	m_Polygons = newPolygons;

	delete [] geomMatIndexList;
	m_PolyMatIndexList = newMaterialIds;

	PostLoadCompute();
}

#endif	// end of #if !__FINAL// && !IS_CONSOLE

class VertexSortPredicate : public std::binary_function<int, int, bool>
{
public:
	VertexSortPredicate(Vector3* vectors) : m_Vectors(vectors) { }

	bool operator()(const int& left, const int& right) const
	{
		return m_Vectors[left].x < m_Vectors[right].x;
	}

private:
	Vector3* m_Vectors;
};

struct PolyInfo
{
	int polygonIndex;
	int minVertexIndex;
	int maxVertexIndex;
};

class PolygonSortPredicate : public std::binary_function<PolyInfo, PolyInfo, bool>
{
public:
	bool operator()(const PolyInfo& left, const PolyInfo& right) const
	{
		if(left.minVertexIndex < right.minVertexIndex)
		{
			return true;
		}
		else if(left.minVertexIndex==right.minVertexIndex)
		{
			if(left.maxVertexIndex < right.maxVertexIndex)
			{
				return true;
			}
			else if(left.maxVertexIndex==right.maxVertexIndex)
			{
				return left.polygonIndex < right.polygonIndex;
			}
		}
		return false;
	}
};

bool phBoundGeometry::WeldVertices (float epsilon, int* polygonRemap)
{
	int i, j;

	// NOTE: mutable pointers since phBoundGeometry
	// controls its own components
#if COMPRESSED_VERTEX_METHOD == 0
	Vector3 * geomVerts = (Vector3*)m_Vertices;
#else
	// If there's any chance that we're using compressed vertices, we're going to work with a temporary buffer of uncompressed vertices
	//   and then right them back afterward.
	Vector3 *geomVerts = rage_new Vector3[m_NumVertices];
	for(i = 0; i < m_NumVertices; ++i)
	{
		geomVerts[i].Set(VEC3V_TO_VECTOR3(GetVertex(i)));
	}
#endif

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	float * geomSecondSurfaceVertexDisplacements = (float*)m_SecondSurfaceVertexDisplacements;
#endif
	phPolygon * geomPolys = (phPolygon*)m_Polygons;

	float epsilon2 = square(epsilon);
	bool changed = false;

	//Early out if there are no vertices to weld
	if(m_NumVertices == 0)
		return changed;

	///////////////////////////////////////////////////////////
	// VERTS

	bool changesMade = true;
	int iterCount = 0;

	while (changesMade && iterCount++ < 1000)
	{
		changesMade = false;

		int *newVertIndex = rage_new int[m_NumVertices];

		{
			int *sortedIndices = rage_new int[m_NumVertices];
			for (int vertex = 0; vertex < m_NumVertices; ++vertex)
			{
				newVertIndex[vertex] = vertex;
				sortedIndices[vertex] = vertex;
			}

			// Sort the vertices by x
			std::sort(sortedIndices, &sortedIndices[m_NumVertices], VertexSortPredicate(geomVerts));

			// first, find duplicate verts
			for (int leftIndex = 0; leftIndex < m_NumVertices-1; leftIndex++)
			{
				Vector3& leftVertex = geomVerts[sortedIndices[leftIndex]];

				// If vertices are near each other in one direction, do a proper pairwise comparison
				for (int rightIndex = leftIndex + 1;
							rightIndex < m_NumVertices && 
							geomVerts[sortedIndices[rightIndex]].x <= leftVertex.x + epsilon;
							++rightIndex)
				{
					if (leftVertex.Dist2(geomVerts[sortedIndices[rightIndex]]) < epsilon2)
					{
						// We set them both to the one with the minimum index, so that later we will know
						// that vertices are only remapped downwards
						int minIndex = Min(newVertIndex[sortedIndices[leftIndex]], newVertIndex[sortedIndices[rightIndex]]);

						newVertIndex[sortedIndices[leftIndex]] = minIndex;
						newVertIndex[sortedIndices[rightIndex]] = minIndex;
					}
				}
			}

			delete [] sortedIndices;
		}

		// find "floating verts" (vertices that are un-referenced by any primitives)
		{
			bool* vertsInUse = rage_new bool[m_NumVertices];
			memset(vertsInUse,0,sizeof(bool)*m_NumVertices);
			for (i = 0; i < m_NumPolygons; i++)
			{
				const phPrimitive &curPrimitive = geomPolys[i].GetPrimitive();
				if(curPrimitive.GetType() == PRIM_TYPE_POLYGON || GetType() != phBound::BVH)
				{
					for (j = 0; j < POLY_MAX_VERTICES; j++)
					{
						vertsInUse[newVertIndex[geomPolys[i].GetVertexIndex(j)]] = true;
					}
				}
				else
				{
					switch(curPrimitive.GetType())
					{
						case PRIM_TYPE_SPHERE:
						{
							const phPrimSphere &spherePrim = curPrimitive.GetSphere();
							vertsInUse[newVertIndex[spherePrim.GetCenterIndex()]] = true;
							break;
						}
						case PRIM_TYPE_CAPSULE:
						{
							const phPrimCapsule &capsulePrim = curPrimitive.GetCapsule();
							vertsInUse[newVertIndex[capsulePrim.GetEndIndex0()]] = true;
							vertsInUse[newVertIndex[capsulePrim.GetEndIndex1()]] = true;
							break;
						}
						case PRIM_TYPE_CYLINDER:
						{
							const phPrimCylinder &cylinderPrim = curPrimitive.GetCylinder();
							vertsInUse[newVertIndex[cylinderPrim.GetEndIndex0()]] = true;
							vertsInUse[newVertIndex[cylinderPrim.GetEndIndex1()]] = true;
							break;
						}
						default:
						{
							Assert(curPrimitive.GetType() == PRIM_TYPE_BOX);
							const phPrimBox &boxPrim = curPrimitive.GetBox();
							vertsInUse[newVertIndex[boxPrim.GetVertexIndex(0)]] = true;
							vertsInUse[newVertIndex[boxPrim.GetVertexIndex(1)]] = true;
							vertsInUse[newVertIndex[boxPrim.GetVertexIndex(2)]] = true;
							vertsInUse[newVertIndex[boxPrim.GetVertexIndex(3)]] = true;
						}
					}
				}
			}

			int newNumVerts = 0;
			for (i = 0; i < m_NumVertices; i++)
			{
				if (vertsInUse[i])
				{
					++newNumVerts;
				}
			}

			if (m_NumVertices != newNumVerts)
			{
				// get rid of floating verts
				changesMade = true;

				int offset = 0;
				for (i = 0; i < m_NumVertices; i++)
				{
					if (vertsInUse[i])
					{
						newVertIndex[i] = i-offset;
						geomVerts[i-offset] = VEC3V_TO_VECTOR3(GetVertex(i));
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
						if( m_SecondSurfaceVertexDisplacements )
							geomSecondSurfaceVertexDisplacements[i-offset] = m_SecondSurfaceVertexDisplacements[i];
#endif
					}
					else
					{
						newVertIndex[i] = newVertIndex[newVertIndex[i]];
						offset++;
					}
				}
				m_NumVertices = newNumVerts;
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
				m_NumSecondSurfaceVertexDisplacements = newNumVerts;
#endif

#if COMPRESSED_VERTEX_METHOD > 0
				// If there's any chance that we're using compressed vertices, we're going to work with a temporary buffer of uncompressed vertices
				//   and then right them back afterward.
				for(i = 0; i < m_NumVertices; ++i)
				{
					SetVertex(i, RCC_VEC3V(geomVerts[i]));
				}
#endif

				// set polygon indices appropriately
				for (i = 0; i < m_NumPolygons; i++)
				{
					phPrimitive &curPrimitive = geomPolys[i].GetPrimitive();
					if(curPrimitive.GetType() == PRIM_TYPE_POLYGON || GetType() != phBound::BVH)
					{
						for (j = 0; j < POLY_MAX_VERTICES; j++)
						{
							geomPolys[i].SetIndex(j,static_cast<phPolygon::Index>(newVertIndex[geomPolys[i].GetVertexIndex(j)]));
						}
					}
					else
					{
						switch(curPrimitive.GetType())
						{
						case PRIM_TYPE_SPHERE:
							{
								phPrimSphere &spherePrim = curPrimitive.GetSphere();
								phPolygon::Index newCenterIndex = (phPolygon::Index)newVertIndex[spherePrim.GetCenterIndex()];
								spherePrim.SetCenterIndex(newCenterIndex);
								break;
							}
						case PRIM_TYPE_CAPSULE:
							{
								phPrimCapsule &capsulePrim = curPrimitive.GetCapsule();
								phPolygon::Index newEndIndex0 = (phPolygon::Index)newVertIndex[capsulePrim.GetEndIndex0()];
								phPolygon::Index newEndIndex1 = (phPolygon::Index)newVertIndex[capsulePrim.GetEndIndex1()];
								capsulePrim.SetEndIndices(newEndIndex0, newEndIndex1);
								break;
							}
						case PRIM_TYPE_CYLINDER:
							{
								phPrimCylinder &cylinderPrim = curPrimitive.GetCylinder();
								phPolygon::Index newEndIndex0 = (phPolygon::Index)newVertIndex[cylinderPrim.GetEndIndex0()];
								phPolygon::Index newEndIndex1 = (phPolygon::Index)newVertIndex[cylinderPrim.GetEndIndex1()];
								cylinderPrim.SetEndIndices(newEndIndex0, newEndIndex1);
								break;
							}
						default:
							{
								Assert(curPrimitive.GetType() == PRIM_TYPE_BOX);
								phPrimBox &boxPrim = curPrimitive.GetBox();
								phPolygon::Index newVertexIndex0 = (phPolygon::Index)newVertIndex[boxPrim.GetVertexIndex(0)];
								phPolygon::Index newVertexIndex1 = (phPolygon::Index)newVertIndex[boxPrim.GetVertexIndex(1)];
								phPolygon::Index newVertexIndex2 = (phPolygon::Index)newVertIndex[boxPrim.GetVertexIndex(2)];
								phPolygon::Index newVertexIndex3 = (phPolygon::Index)newVertIndex[boxPrim.GetVertexIndex(3)];
								boxPrim.SetVertexIndices(newVertexIndex0, newVertexIndex1, newVertexIndex2, newVertexIndex3);
							}
						}
					}
				}

				changed = true;
			}

			delete [] vertsInUse;
		}

		delete [] newVertIndex;
	}

	///////////////////////////////////////////////////////////
	// POLYGONS
	// If we set any of the elements of newPolyIndex, that poly will be deleted at the end
	{
		int* newPolyIndex;
		
		if (polygonRemap)
		{
			newPolyIndex = polygonRemap;
		}
		else
		{
			newPolyIndex = rage_new int[m_NumPolygons];
		}

		for (i = 0; i < m_NumPolygons; i++)
		{
			newPolyIndex[i] = i;
		}

		// get rid of degenerate polygons
		for (i = 0; i < m_NumPolygons; i++)
		{
			phPolygon &A = geomPolys[i];
			phPrimitive &curPrimitive = A.GetPrimitive();
			if(curPrimitive.GetType() == PRIM_TYPE_POLYGON || GetType() != phBound::BVH)
			{
				if (A.GetVertexIndex(0) == A.GetVertexIndex(1) || A.GetVertexIndex(0) == A.GetVertexIndex(2) || A.GetVertexIndex(1) == A.GetVertexIndex(2))
				{
					// duplicate verts
					newPolyIndex[i] = -1;
				}
				else if (CoLinearByAngle(geomVerts[A.GetVertexIndex(0)],geomVerts[A.GetVertexIndex(1)],geomVerts[A.GetVertexIndex(2)],1e-6f))
				{
					// colinear points
					newPolyIndex[i] = -1;
				}
			}
			else
			{
				// TODO: Should we be doing something in here?  We don't have that many options for handling vertices getting fused together (capsules could be
				//   turned into spheres), things will probably work fine if we don't do anything, and it probably shouldn't be happening anyway.
			}
		}

		{
			// Fill out a PolyInfo array, to be used for sorting.
			PolyInfo* sortedPolys = rage_new PolyInfo[m_NumPolygons];
			int polygonInSortedListIndex = 0;
			for (int polygonInBoundIndex = 0; polygonInBoundIndex < m_NumPolygons; ++polygonInBoundIndex)
			{
				phPrimitive &curPrimitive = geomPolys[polygonInBoundIndex].GetPrimitive();
				if(curPrimitive.GetType() == PRIM_TYPE_POLYGON || GetType() != phBound::BVH)
				{
					const phPolygon &curPoly = geomPolys[polygonInBoundIndex];
					sortedPolys[polygonInSortedListIndex].polygonIndex = polygonInBoundIndex;
					sortedPolys[polygonInSortedListIndex].minVertexIndex = Min(curPoly.GetVertexIndex(0), curPoly.GetVertexIndex(1), curPoly.GetVertexIndex(2));
					sortedPolys[polygonInSortedListIndex].maxVertexIndex = Max(curPoly.GetVertexIndex(0), curPoly.GetVertexIndex(1), curPoly.GetVertexIndex(2));
					++polygonInSortedListIndex;
				}
			}

			// Sort the polygons by their minimum vertex index, then by maximum vertex index, then by polygon index
			std::sort(sortedPolys, &sortedPolys[polygonInSortedListIndex], PolygonSortPredicate());

			int rightIndex;

			// Go through the sorted list of polygons and remove any duplicates (which will now be in adjacent entries of sortedPolys thanks to the sorting).
			int countDuplicatePolysRemoved = 0;
			for (int leftIndex = 0; leftIndex < polygonInSortedListIndex - 1; leftIndex++)
			{
				if(newPolyIndex[sortedPolys[leftIndex].polygonIndex] != sortedPolys[leftIndex].polygonIndex)
				{
					// The left-index polygon was already remapped or removed.
					continue;
				}

				// If polygons have the same minimum vertex index, do a proper pairwise comparison
				for(rightIndex = leftIndex + 1;
					rightIndex < polygonInSortedListIndex &&
					sortedPolys[leftIndex].minVertexIndex == sortedPolys[rightIndex].minVertexIndex &&
					sortedPolys[leftIndex].maxVertexIndex == sortedPolys[rightIndex].maxVertexIndex;
					++rightIndex)
				{
					int indexA = sortedPolys[leftIndex].polygonIndex;
					int indexB = sortedPolys[rightIndex].polygonIndex;
					const phPolygon& A = geomPolys[indexA];
					const phPolygon& B = geomPolys[indexB];

					if(newPolyIndex[sortedPolys[leftIndex].polygonIndex] != -1 &&
						newPolyIndex[sortedPolys[rightIndex].polygonIndex] != -1 &&
						(A.GetVertexIndex(0) == B.GetVertexIndex(0) || A.GetVertexIndex(0) == B.GetVertexIndex(1) || A.GetVertexIndex(0) == B.GetVertexIndex(2)) &&
						(A.GetVertexIndex(1) == B.GetVertexIndex(0) || A.GetVertexIndex(1) == B.GetVertexIndex(1) || A.GetVertexIndex(1) == B.GetVertexIndex(2)) &&
						(A.GetVertexIndex(2) == B.GetVertexIndex(0) || A.GetVertexIndex(2) == B.GetVertexIndex(1) || A.GetVertexIndex(2) == B.GetVertexIndex(2)) &&
						VEC3V_TO_VECTOR3(GetPolygonUnitNormal(indexA)).Dot(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(indexB))) > 0.0f)
					{
						// A and B have all the same vertices, and their normals are pointing in the same direction.
						// get rid of B

						countDuplicatePolysRemoved++;

						// We set them both to the one with the minimum index, so that later we will know
						// that vertices are only remapped downwards
						int minPolyIndex = Min(sortedPolys[leftIndex].polygonIndex, sortedPolys[rightIndex].polygonIndex);

						newPolyIndex[sortedPolys[leftIndex].polygonIndex] = minPolyIndex;
						newPolyIndex[sortedPolys[rightIndex].polygonIndex] = minPolyIndex;
					}
				}
			}

			//Displayf("polys removed %d",countDuplicatePolysRemoved);

			delete [] sortedPolys;
		}

		// Find out how many polys are left
		int newNumPolys = m_NumPolygons;
		for (int polygon = 0; polygon < m_NumPolygons; ++polygon)
		{
			if (newPolyIndex[polygon] != polygon)
			{
				--newNumPolys;
			}
		}

		if (m_NumPolygons != newNumPolys)
		{
			// get rid of floating verts

			int offset = 0;
			for (i = 0; i < m_NumPolygons; i++)
			{
				if (newPolyIndex[i]==i)
				{
					newPolyIndex[i] = i - offset;
					geomPolys[i - offset] = geomPolys[i];

					// We keep a parallel array for the materials of the primitives, so we need to maintain that as well. /FF
					m_PolyMatIndexList[i - offset] = m_PolyMatIndexList[i];
				}
				else 
				{
					if (newPolyIndex[i] != -1)
					{
						newPolyIndex[i] = newPolyIndex[newPolyIndex[i]];
					}

					offset++;
				}
			}

			m_NumPolygons = newNumPolys;
			changed = true;

			// Now fix up all the neighboring polygon numbers
			for (i = 0; i < m_NumPolygons; i++)
			{
				phPrimitive &curPrimitive = geomPolys[i].GetPrimitive();
				if(curPrimitive.GetType() == PRIM_TYPE_POLYGON || GetType() != phBound::BVH)
				{
					if (geomPolys[i].GetNeighboringPolyNum(0) != INVALID_VERTEX)
					{
						geomPolys[i].SetNeighboringPolyNum(0, (phPolygon::Index)newPolyIndex[geomPolys[i].GetNeighboringPolyNum(0)]);
					}

					if (geomPolys[i].GetNeighboringPolyNum(1) != INVALID_VERTEX)
					{
						geomPolys[i].SetNeighboringPolyNum(1, (phPolygon::Index)newPolyIndex[geomPolys[i].GetNeighboringPolyNum(1)]);
					}

					if (geomPolys[i].GetNeighboringPolyNum(2) != INVALID_VERTEX)
					{
						geomPolys[i].SetNeighboringPolyNum(2, (phPolygon::Index)newPolyIndex[geomPolys[i].GetNeighboringPolyNum(2)]);
					}
				}
			}
		}

		if(newPolyIndex)
		{
			if (polygonRemap == NULL)
			{
				delete [] newPolyIndex;
			}
		}
	}

#if COMPRESSED_VERTEX_METHOD > 0
	delete [] geomVerts;
	geomVerts = NULL;
#endif

#if !__PAGING && __ASSERT
	for (int polyIndex = 0; polyIndex < GetNumPolygons(); ++polyIndex)
	{
		const phPolygon& poly = GetPolygon(polyIndex);

		Assertf(poly.VerifyUniqueVertices(),"While welding bound vertices the polygon at index '%d' was found to have duplicate vertices", polyIndex);

#if COMPRESSED_VERTEX_METHOD == 0
		for(int vert1=POLY_MAX_VERTICES-1;vert1>0;vert1--)
		{
			for(int vert2=vert1-1;vert2>=0;vert2--)
			{
				Vector3 vertex1 = VEC3V_TO_VECTOR3(GetVertex(poly.GetVertexIndex(vert1)));
				Vector3 vertex2 = VEC3V_TO_VECTOR3(GetVertex(poly.GetVertexIndex(vert2)));
				Assertf(vertex1.Dist2(vertex2) > epsilon2,
					"phBoundGeometry::WeldVertices: polygon %d has two vertices that are too close together (%f) <%f %f %f>, <%f %f %f>",
					 polyIndex, vertex1.Dist(vertex2), vertex1.x, vertex1.y, vertex1.z, vertex2.x, vertex2.y, vertex2.z);
			}
		}
#endif
	}
#endif // !__PAGING

	m_NumConvexHullVertices = (u16)(m_NumVertices);
	
	Assertf(GetType() != phBound::GEOMETRY || (m_NumPolygons != 0 && m_NumVertices != 0), "Reducing a phBoundGeometry to %i polygons and %i vertices.", m_NumPolygons, m_NumVertices);

	return changed;
}


bool phBoundGeometry::SplitQuad012 (phPolygon::Index vertIndex0, phPolygon::Index vertIndex1, phPolygon::Index vertIndex2, phPolygon::Index vertIndex3) const
{
	// Find the edge vectors for this quad.
	Vector3 vertex0 = VEC3V_TO_VECTOR3(GetVertex(vertIndex0));
	Vector3 vertex1 = VEC3V_TO_VECTOR3(GetVertex(vertIndex1));
	Vector3 vertex2 = VEC3V_TO_VECTOR3(GetVertex(vertIndex2));
	Vector3 vertex3 = VEC3V_TO_VECTOR3(GetVertex(vertIndex3));
	Vector3 edge01 = vertex1-vertex0;
	Vector3 edge12 = vertex2-vertex1;
	Vector3 edge23 = vertex3-vertex2;
	Vector3 edge30 = vertex0-vertex3;

	// Find the normal vector for the (0,1,2) triangle.
	Vector3 normal012(edge01);
	normal012.Cross(edge12);
	if (normal012.Mag2()<SMALL_FLOAT)
	{
		// The triangle made by vertices (0,1,2) has nearly zero area, so split the quad into (1,2,3) and (3,0,1) instead.
		return false;
	}

	// Find the normal vector for the (2,3,0) triangle.
	Vector3 normal230(edge23);
	normal230.Cross(edge30);
	if (normal230.Mag2()<SMALL_FLOAT)
	{
		// The triangle made by vertices (2,3,0) has nearly zero area, so split the quad into (1,2,3) and (3,0,1) instead.
		return false;
	}

	// Find the normal vector for the (1,2,3) triangle.
	Vector3 normal123(edge12);
	normal123.Cross(edge23);
	if (normal123.Mag2()<SMALL_FLOAT)
	{
		// The triangle made by vertices (1,2,3) has nearly zero area, so split the quad into (0,1,2) and (2,3,0).
		return true;
	}

	// Find the normal vector for the (3,0,1) triangle.
	Vector3 normal301(edge30);
	normal301.CrossNegate(edge01);
	if (normal301.Mag2()<SMALL_FLOAT)
	{
		// The triangle made by vertices (3,0,1) has nearly zero area, so split the quad into (0,1,2) and (2,3,0).
		return true;
	}

	Vector3 crossNormals(normal230);
	crossNormals.Cross(normal012);
	Vector3 diagonal02 = vertex2-vertex0;
	if ((crossNormals.Dot(diagonal02)>=0.0f)==sm_SplitQuadsBendingOut)
	{
		// The diagonal from 0 to 2 is in front of the diagonal from 1 to 3, so split the quadrangle into (0,1,2) and (2,3,0),
		// or it's behind, and split-quads-bending-out is false (split them bending in).
		return true;
	}
	else
	{
		// The diagonal from 0 to 2 is in behind the diagonal from 1 to 3, so split the quadrangle into (1,2,3) and (3,0,1),
		// or it's in front, and split-quads-bending-out is false (split them bending in).
		return false;
	}
}


void phBoundGeometry::CalculatePolyNormals ()
{
	// NOTE: mutable pointers since phBoundGeometry
	// controls its own components
	phPolygon * geomPolys = (phPolygon*)m_Polygons;

	for (int i=0; i<m_NumPolygons; i++)
	{
		phPolygon &curPoly = geomPolys[i];
		curPoly.CalculateNormalAndArea(GetVertex(curPoly.GetVertexIndex(0)), GetVertex(curPoly.GetVertexIndex(1)), GetVertex(curPoly.GetVertexIndex(2)));
	}
}

void phBoundGeometry::ComputeOctantMap ()
{
#if OCTANT_MAP_SUPPORT_ACCEL
	if (GetType() != phBound::BVH)
	{
#if USE_OCTANT_MAP_INDEX_U16
		FastAssert(m_Flags & OCTANT_MAP_INDEX_IS_U16);
#endif // USE_OCTANT_MAP_INDEX_U16
		
#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
		// Disable the octant map flag until we know we computed a valid octant map.
		m_Flags &= ~GEOMETRY_BOUND_HAS_OCTANT_MAP;
#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION

		OctantMapIndexType octantVertCounts[8];
		OctantMapIndexType* octantVerts[8];

		Assert(m_NumConvexHullVertices < (0xFFFF >> 2));
		// Only keep the octant map if on average less than half of the verts went into each octant
		const OctantMapIndexType MAX_OCTANT_VERTS = 4 * (OctantMapIndexType)m_NumConvexHullVertices;

		OctantMapIndexType * totalVertsList = Alloca(OctantMapIndexType, MAX_OCTANT_VERTS);
		OctantMapIndexType totalVertsNeeded = 0;

		OctantMapIndexType * vertsNeeded = totalVertsList;

#define PRECOMP_OCTANT_MAP_VERT_LIST 1

#if PRECOMP_OCTANT_MAP_VERT_LIST
		Vec3V * vertList = Alloca(Vec3V, m_NumConvexHullVertices);
		for (int vertIndex = 0 ; vertIndex < m_NumConvexHullVertices ; vertIndex++)
			vertList[vertIndex] = GetShrunkVertex(vertIndex);
#endif // PRECOMP_OCTANT_MAP_VERT_LIST

		const VecBoolV octantSigns[8] = 
		{
			VecBoolV(V_T_T_T_F),
			VecBoolV(V_F_T_T_F),
			VecBoolV(V_T_F_T_F),
			VecBoolV(V_F_F_T_F),
			VecBoolV(V_T_T_F_F),
			VecBoolV(V_F_T_F_F),
			VecBoolV(V_T_F_F_F),
			VecBoolV(V_F_F_F_F),
		};

		for (int octant = 0; octant < 8; ++octant)
		{
			OctantMapIndexType numVertsNeeded = 0;
			for (OctantMapIndexType vertIndex = 0; vertIndex < m_NumConvexHullVertices; ++vertIndex)
			{
				// Check to see if the newVert is shadowed by any preexisting vert in the octant. If a preexisting vert is
				// shadowed by the new vert, delete it by collapsing the vertsNeeded array.
#if PRECOMP_OCTANT_MAP_VERT_LIST
				const Vec3V newVert = vertList[vertIndex];
#else // PRECOMP_OCTANT_MAP_VERT_LIST
				const Vec3V newVert = GetShrunkVertex(vertIndex);
#endif // PRECOMP_OCTANT_MAP_VERT_LIST
				const OctantMapIndexType numVertsNeededCur = numVertsNeeded;
				bool addNewVert = true;
				ASSERT_ONLY(bool deletedPreexistingVert = false);
				numVertsNeeded = 0;
				for (OctantMapIndexType vertsNeededIndex = 0 ; vertsNeededIndex < numVertsNeededCur ; vertsNeededIndex++)
				{
					const OctantMapIndexType vert_i = vertsNeeded[vertsNeededIndex];
#if PRECOMP_OCTANT_MAP_VERT_LIST
					const Vec3V vert = vertList[vert_i];
#else // PRECOMP_OCTANT_MAP_VERT_LIST
					const Vec3V vert = GetShrunkVertex(vert_i);
#endif // PRECOMP_OCTANT_MAP_VERT_LIST
					const Vec3V direction = newVert - vert;
					const Vec3V directionScaled = SelectFT(octantSigns[octant],-direction,direction);
					if (IsLessThanOrEqualAll(directionScaled,Vec3V(V_ZERO)))
					{
						// The new vert is shadowed by a preexisting vert.
						addNewVert = false;

						// Make sure no preexisting verts were deleted.
						Assert(deletedPreexistingVert == false);
						Assert(vertsNeededIndex == numVertsNeeded);
						numVertsNeeded = numVertsNeededCur;
						break;
					}
					if (IsGreaterThanOrEqualAll(directionScaled,Vec3V(V_ZERO)) == 0)
					{
						// The preexisting vert is not shadowed by the new vert, so keep it.
						vertsNeeded[numVertsNeeded] = vert_i;
						numVertsNeeded++;
					}
					else
					{
						// The preexisting vert is shadowed by the new vert, so delete it. 
						// We delete it by not adding it back into the vertsNeeded array.

						// If we get here then it shouldn't be possible for the new vert to be shadowed by any preexisting vert.
						ASSERT_ONLY(deletedPreexistingVert = true);
					}
				}
				if (addNewVert)
				{
					const OctantMapIndexType totalVertsNeededCurrent = numVertsNeeded + totalVertsNeeded;
					if (totalVertsNeededCurrent < MAX_OCTANT_VERTS)
					{
						vertsNeeded[numVertsNeeded] = vertIndex;
						numVertsNeeded++;
					}
					else
					{
#if RSG_TOOL
						Displayf("Discarding octant map (octant verts %d, verts %d)", totalVertsNeededCurrent+1, m_NumConvexHullVertices);
#endif
						return;
					}
				}
			}

			if (!Verifyf(numVertsNeeded > 0,"Octant map generator failed(numVertices %d, numConvexHullVertices %d",m_NumVertices,m_NumConvexHullVertices))
			{
				return;
			}
			Assert(totalVertsNeeded + numVertsNeeded <= MAX_OCTANT_VERTS);
			octantVertCounts[octant] = numVertsNeeded;
			octantVerts[octant] = vertsNeeded;
			totalVertsNeeded += numVertsNeeded;
			vertsNeeded += numVertsNeeded;
		}

		Assert(totalVertsNeeded <= MAX_OCTANT_VERTS);
#if RSG_TOOL
		Displayf("Keeping octant map (octant verts %d, verts %d)", totalVertsNeeded, m_NumConvexHullVertices);
#endif
#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
		m_Flags |= GEOMETRY_BOUND_HAS_OCTANT_MAP;
#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION
		OctantMapAllocateAndCopy(octantVertCounts,octantVerts);
	}
#endif // OCTANT_MAP_SUPPORT_ACCEL
}

void phBoundGeometry::RecomputeOctantMap ()
{
#if OCTANT_MAP_SUPPORT_ACCEL
#if !USE_OCTANT_MAP_PERMANENT_ALLOCATION
	OctantMapDelete();
#endif // !USE_OCTANT_MAP_PERMANENT_ALLOCATION
	ComputeOctantMap();
#endif // OCTANT_MAP_SUPPORT_ACCEL
}


void phBoundGeometry::SetMarginAndShrink (const float margin, const float polyOrVert)
{
	Assert(polyOrVert >= 0.0f && polyOrVert <= 1.0f);

	float marginUsed = margin;

	// If the margin is greater than half the extents, then we may as well scale it right away
	Vector3 halfExtents = VEC3V_TO_VECTOR3(GetBoundingBoxSize());
	halfExtents.Scale(0.5f);

	if (marginUsed > halfExtents.x)
	{
			marginUsed = halfExtents.x;
	}

	if (marginUsed > halfExtents.y)
	{
			marginUsed = halfExtents.y;
	}

	if (marginUsed > halfExtents.z)
	{
			marginUsed = halfExtents.z;
	}

	sysMemStartTemp();
	Vector3* resultShrunk = rage_new Vector3[m_NumVertices];
	sysMemEndTemp();

	bool resultsGood = false;

	// Repeat until the margin is small enough
	while (!resultsGood && marginUsed > SMALL_FLOAT)
	{
		ShrinkPolysOrVertsByMargin(marginUsed, polyOrVert, resultShrunk);

		// Now, check out our results and make sure they don't pass through any of the polygons, either original
		// or post-shrunk
		resultsGood = true;
		for (int vertIndex = 0; resultsGood && vertIndex < m_NumVertices; ++vertIndex)
		{
			const Vec3V segmentStart = GetVertex(vertIndex);
			const Vec3V segmentEnd = RCC_VEC3V(resultShrunk[vertIndex]);
			const Vec3V segmentStartToEnd = Subtract(segmentEnd,segmentStart);
			const Vec3V segmentEndToStart = Negate(segmentStartToEnd);

			for (int polyIndex = 0; resultsGood && polyIndex < m_NumPolygons; ++polyIndex)
			{
				const phPolygon& polygon = m_Polygons[polyIndex];

				// Don't test against polygons containing the vertex we are shrinking
				if (!polygon.ContainsVertices(&vertIndex, 1))
				{
					const Vec3V originalVertex0 = GetVertex(polygon.GetVertexIndex(0));
					const Vec3V originalVertex1 = GetVertex(polygon.GetVertexIndex(1));
					const Vec3V originalVertex2 = GetVertex(polygon.GetVertexIndex(2));
					const Vec3V originalNormal = polygon.ComputeUnitNormal(originalVertex0,originalVertex1,originalVertex2);

					const Vec3V shrunkVertex0 = RCC_VEC3V(resultShrunk[polygon.GetVertexIndex(0)]);
					const Vec3V shrunkVertex1 = RCC_VEC3V(resultShrunk[polygon.GetVertexIndex(1)]);
					const Vec3V shrunkVertex2 = RCC_VEC3V(resultShrunk[polygon.GetVertexIndex(2)]);
					const Vec3V shrunkNormal = Normalize(polygon.ComputeNonUnitNormal(shrunkVertex0,shrunkVertex1,shrunkVertex2));

					// We're now using directed tests for testing the results, but with the opposite segment direction
					// This is because we don't actually care if a vertex gets moved into an existing or shrunk face
					// What we care about is making sure we don't move a vertex through the other side of our object
					//  and are therefore effectively testing against the backface of other polys
					// -- I think this means we don't need to bother excluding polys that contain the test vertex, but it's safer (and a bit faster) to leave that part as is right now
					ScalarV fractionAlongSegment;
					if(geomSegments::SegmentTriangleIntersectDirected(segmentEnd, segmentEndToStart, segmentStart, originalNormal, originalVertex0, originalVertex1, originalVertex2, fractionAlongSegment))
					{
						resultsGood = false;
					}
					
					if(geomSegments::SegmentTriangleIntersectDirected(segmentEnd, segmentEndToStart, segmentStart, shrunkNormal, shrunkVertex0, shrunkVertex1, shrunkVertex2, fractionAlongSegment))
					{
						resultsGood = false;
					}
				}
			}
		}

		if (!resultsGood)
		{
				marginUsed *= 0.5f;
		}
	}

	// Now that we like the shrunken vertices we made...
#if COMPRESSED_VERTEX_METHOD == 0
	if (m_ShrunkVertices == NULL)
#else
	if (m_CompressedShrunkVertices == NULL)
#endif
	{
		// ...allocate our shrunk vector...this time in real memory...
#if COMPRESSED_VERTEX_METHOD == 0
		m_ShrunkVertices = rage_new Vec3V[m_NumVertices];
#else
		m_CompressedShrunkVertices = rage_new CompressedVertexType[3 * m_NumVertices];
#endif
	}

	if (marginUsed != margin)
	{
		Warningf("Margin %f was too big for shrinking, %f was used instead", margin, marginUsed);
	}

	// IMPORTANT!  Make sure that this gets set before calling SetShrunkVertex() below or else asset may potentially trigger (SetShrunkVertex()
	//   will verify that the vertex is within range but we need to know the margin before we can do that.)
	marginUsed = Max(CONVEX_MINIMUM_MARGIN, marginUsed);
	SetMargin(marginUsed);

    // ...and copy the data in.
#if COMPRESSED_VERTEX_METHOD == 0
	sysMemCpy(m_ShrunkVertices, resultShrunk, m_NumVertices * sizeof(Vector3));
#else
	for(int i = 0; i < m_NumVertices; ++i)
	{
		// Clamp the shrunk vertices to be within the bounding box or we'll wrap around
		Vec3V clampedResult;
		clampedResult = Max(GetBoundingBoxMin() + Vec3V(GetMarginV()), RCC_VEC3V(resultShrunk[i]));
		clampedResult = Min(GetBoundingBoxMax() - Vec3V(GetMarginV()), clampedResult);
		SetShrunkVertex(i, clampedResult);
	}
#endif

	// Delete our temporary memory
	sysMemStartTemp();
	delete [] resultShrunk;
	sysMemEndTemp();
}

void phBoundGeometry::ShrinkVerticesByMargin (float margin, Vector3* shrunkVertices)
{
    sysMemStartTemp();
    Vector3* vertexNormals = rage_new Vector3[m_NumVertices];
    sysMemEndTemp();

    ComputeVertexNormals((Vec3V_Ptr)vertexNormals);

    for (int vertex = 0; vertex < m_NumVertices; ++vertex)
    {
        shrunkVertices[vertex].AddScaled(VEC3V_TO_VECTOR3(GetVertex(vertex)), vertexNormals[vertex], -margin);
    }

    sysMemStartTemp();
    delete [] vertexNormals;
    sysMemEndTemp();
}

void phBoundGeometry::ShrinkPolysOrVertsByMargin (float margin, float polyOrVert, Vector3* shrunkVertices)
{
    // Make some temporary memory to perform our shrinkage
    if (polyOrVert == 0.0f)
    {
        // If they only want to shrink by poly...
        ShrinkPolysByMargin(margin, shrunkVertices);
    }
    else if (polyOrVert == 1.0f)
    {
        // If they only want to shrink by vertex...
        ShrinkVerticesByMargin(margin, shrunkVertices);
    }
    else
    {
        // If they only want a mix, make some temporary memory
        sysMemStartTemp();
        Vector3* vertShrunk = rage_new Vector3[m_NumVertices];
        sysMemEndTemp();

        // Shrink both ways
        ShrinkPolysByMargin(margin, shrunkVertices);
        ShrinkVerticesByMargin(margin, vertShrunk);

		// Then combine them
        for (int vert = 0; vert < m_NumVertices; ++vert)
        {
            shrunkVertices[vert].Lerp(polyOrVert, vertShrunk[vert]);
        }

		// And clean up the temporary memory
        sysMemStartTemp();
        delete [] vertShrunk;
        sysMemEndTemp();
    }
}

void phBoundGeometry::ShrinkPolysByMargin (float margin, Vector3* shrunkVertices)
{
	atFixedBitSet<65535> vertexShrunk;

    const int MAX_POLYS_TOUCHING_VERT = 4096;
    Vector3 neighborNormals[MAX_POLYS_TOUCHING_VERT];

	// Since we've possibly already removed polygons some vertices might not be referenced any more.  To avoid any potential problems (for example somebody
	//   might want to loop over all of the shrunk vertices without checking to see if they're referenced or not) we'll just set all of shrunk vertices to
	//   the unshrunk vertices first.
	const int numVertices = m_NumVertices;
	for(int vertIndex = 0; vertIndex < numVertices; ++vertIndex)
	{
		shrunkVertices[vertIndex] = VEC3V_TO_VECTOR3(GetVertex(vertIndex));
	}

    for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
    {
        const phPolygon& polygon = m_Polygons[polygonIndex];

        for (int polyVertexIndex = 0; polyVertexIndex < POLY_MAX_VERTICES; ++polyVertexIndex)
        {
            int boundVertexIndex = polygon.GetVertexIndex(polyVertexIndex);

            // If the normal is zero, that means we have not yet computed it
            if (vertexShrunk.IsClear(boundVertexIndex))
            {
                vertexShrunk.Set(boundVertexIndex);
                int numNeighbors = 1;
                neighborNormals[0] = VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polygonIndex));
                phPolygon::Index neighborIndex = polygon.FindNeighborWithVertex(boundVertexIndex);
                int prevNeighborIndex = polygonIndex;
                Vector3 avgNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polygonIndex)));

                // Walk around all the neighbors
				while (neighborIndex!=(phPolygon::Index)polygonIndex && neighborIndex != (phPolygon::Index)BAD_INDEX)
                {
                    const phPolygon& neighbor = m_Polygons[neighborIndex];

					Assert(numNeighbors < MAX_POLYS_TOUCHING_VERT);
                    neighborNormals[numNeighbors++] = VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex));
                    avgNormal.Add(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex)));

                    phPolygon::Index nextNeighborIndex = neighbor.FindNeighborWithVertex(boundVertexIndex,prevNeighborIndex);
                    prevNeighborIndex = neighborIndex;
                    neighborIndex = nextNeighborIndex;
                }

                if (neighborIndex == (phPolygon::Index)BAD_INDEX)
                {
                    //We ran out of neighbors, walk around the vertex the other way
                    phPolygon::Index neighborIndex = polygon.FindNeighborWithVertex2(boundVertexIndex);
                    int prevNeighborIndex = polygonIndex;
                    Vector3 avgNormal(ORIGIN);

                    // Walk around all the neighbors
                    while (neighborIndex!=(phPolygon::Index)polygonIndex && neighborIndex != (phPolygon::Index)BAD_INDEX)
                    {
                        const phPolygon& neighbor = m_Polygons[neighborIndex];

						Assert(numNeighbors < MAX_POLYS_TOUCHING_VERT);
                        neighborNormals[numNeighbors++] = VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex));
                        avgNormal.Add(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex)));

                        phPolygon::Index nextNeighborIndex = neighbor.FindNeighborWithVertex2(boundVertexIndex,prevNeighborIndex);
                        prevNeighborIndex = neighborIndex;
                        neighborIndex = nextNeighborIndex;
                    }
                }

                avgNormal.Normalize();

                if (numNeighbors < 2)
                {
                    shrunkVertices[boundVertexIndex].SubtractScaled(VEC3V_TO_VECTOR3(GetVertex(boundVertexIndex)), VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polygonIndex)), margin);
                }
                else if (numNeighbors == 2)
                {
                    // Make up a new plane by crossing our existing ones
                    neighborNormals[2].Cross(neighborNormals[0], neighborNormals[1]);
                    float newNormalMag2 = neighborNormals[2].Mag2();
                    if (newNormalMag2 < 0.1f)
                    {
                        // The two planes were nearly parallel, just pretend we only had one to start with
                        shrunkVertices[boundVertexIndex].SubtractScaled(VEC3V_TO_VECTOR3(GetVertex(boundVertexIndex)), neighborNormals[0], margin);
                    }
                    else
                    {
                        // Our fake normal is fine, put that in the list and use the neighbors >= 3 case
                        neighborNormals[2].InvScale(sqrtf(newNormalMag2));
                        numNeighbors++;
                    }
                }

                if (numNeighbors >= 3)
                {
                    Vector3 proposedNewVertex = VEC3V_TO_VECTOR3(GetVertex(boundVertexIndex));
                    proposedNewVertex.AddScaled(avgNormal, -margin);
                    Vector3 threeNormals[3];

                    // Iterate over all triplets of neighbors
                    for (int a = 0; a < numNeighbors - 2; ++a)
                    {
                        for (int b = a + 1; b < numNeighbors - 1; ++b)
                        {
                            for (int c = b + 1; c < numNeighbors; ++c)
                            {
                                threeNormals[0] = neighborNormals[a];
                                threeNormals[1] = neighborNormals[b];
                                threeNormals[2] = neighborNormals[c];

                                const float PARALLEL_TOLERANCE = 0.25f;

                                Vector3 crosses[3];
                                crosses[0].Cross(threeNormals[1], threeNormals[2]);
                                crosses[1].Cross(threeNormals[2], threeNormals[0]);
                                crosses[2].Cross(threeNormals[0], threeNormals[1]);

                                // Compute intersection of three planes
                                float determinant = crosses[0].Dot(threeNormals[0]);
                                if (fabs(determinant) > PARALLEL_TOLERANCE)
                                {
                                    Vector3 satisfyingVertex;
                                    satisfyingVertex.Scale(crosses[0], -margin);

                                    satisfyingVertex.AddScaled(crosses[1], -margin);
                                    satisfyingVertex.AddScaled(crosses[2], -margin);

                                    satisfyingVertex.InvScale(determinant);
                                    
                                    satisfyingVertex.Add(VEC3V_TO_VECTOR3(GetVertex(boundVertexIndex)));

                                    // Only if the new vertex is farther away than any other proposed vertex do we take it
                                    float newDist2 = satisfyingVertex.Dist2(VEC3V_TO_VECTOR3(GetVertex(boundVertexIndex)));
                                    if (newDist2 > proposedNewVertex.Dist2(VEC3V_TO_VECTOR3(GetVertex(boundVertexIndex))))
                                    {
                                        proposedNewVertex = satisfyingVertex;
                                    }
                                }
                            }
                        }
                    }

                    shrunkVertices[boundVertexIndex] = proposedNewVertex;
                }
            }
        }
    }

	// Is there a reason for this to be here?  Normals aren't calculated from shrunk vertices so we haven't really changed anything ...
    CalculatePolyNormals();
}

void phBoundGeometry::CalculateGeomExtents ()
{
	CalculateExtents();
}

#if HACK_GTA4 || RSG_TOOL 
// Used in R*N smashobject.
void phBoundGeometry::DecreaseNumPolys (int number)
{
	Assert ( number <= m_NumPolygons );

	m_NumPolygons = number;
}
#endif // HACK_GTA4 || RSG_TOOL


bool phBoundGeometry::CanBecomeActive () const
{
	return true;
}

void phBoundGeometry::CalculateExtents()
{
	// With compressed vertex method two this is partially pointless and partially dangerous, because it uses the vertices to 
	//   calculate the bounding box, whereas the bounding box must have already been calculated for the vertices to be able to be
	//   compressed anyway.
	// Update: This is still potentially messy - if we have shrunk vertices then we base the bounding box on the shrunk vertices and the
	//   margin and, because shrunk vertices are not compressed, it's NOT true that the bounding box has to already be accurate before
	//   this function gets called.  Once shrunk vertices are also compressed things will go back to being slightly more consistent
	//   and it will be the case that the bounding box will have to be set before the shrunk vertices can be set in the first place.
#if COMPRESSED_VERTEX_METHOD == 0
	if(m_ShrunkVertices != NULL)
	{
		CalculateBoundingBox(NULL, (const Vector3*)m_ShrunkVertices);
	}
	else if(m_Vertices != NULL)
	{
		CalculateBoundingBox((const Vector3*)m_Vertices, NULL);
	}
#endif

    Vector3 vecSphereCenter;
	ScalarV radiusV(m_RadiusAroundCentroid);
    CalculateBoundingSphere(RC_VEC3V(vecSphereCenter), radiusV);
	m_RadiusAroundCentroid = radiusV.Getf();
	// I think this should be here but I'm not positive.
	//m_RadiusAroundCentroid += GetMargin();

	phBound::SetCentroidOffset(RCC_VEC3V(vecSphereCenter));

	if (m_NumVertices>0)
	{
		if (GetType() != phBound::BVH)
		{
			Vector3 angInertia;
		#if COMPRESSED_VERTEX_METHOD == 0
			const Vector3 *vertexPointer = (const Vector3*)GetVertexPointer();
		#else
			// In order to call phMathInertia::FindGeomAngInertia we need an array of Vector3's for the vertices, so we do a quick decompression
			//   here.  TODO: Decompressing a stream of vertices can be done faster than decompressing them individually.
			Vector3 *vertexPointer = Alloca(Vector3, GetNumVertices());
			for (int vertIndex = 0; vertIndex < GetNumVertices(); ++vertIndex)
			{
				vertexPointer[vertIndex].Set(VEC3V_TO_VECTOR3(GetVertex(vertIndex)));
			}
		#endif

			phMathInertia::FindGeomAngInertia(1.0f,vertexPointer,m_Polygons,m_NumPolygons,VEC3V_TO_VECTOR3(GetCGOffset()),angInertia);
			m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));

			ScalarV w(V_ZERO);
			for (int polyIndex=0;polyIndex<m_NumPolygons;polyIndex++)
			{
				const phPolygon& poly = m_Polygons[polyIndex];
				w = AddScaled(w, poly.GetAreaV(), Dot(GetPolygonUnitNormal(polyIndex), GetVertex(poly.GetVertexIndex(0))));
			}

			m_VolumeDistribution.SetW(w * ScalarVConstant<U32_THIRD>());
		}
		else
		{
			// This is a bvh bound, so check it for capsules and spheres that might increase its extents.
			for (int polyIndex=0; polyIndex<m_NumPolygons; polyIndex++)
			{
				const phPrimitive* bvhPrimitive = &m_Polygons[polyIndex].GetPrimitive();
				if (bvhPrimitive->GetType()==PRIM_TYPE_SPHERE)
				{
					// This is an embedded sphere, so see if it sticks out of the bound extents.
					const phPrimSphere& spherePrim = bvhPrimitive->GetSphere();
					float radius = spherePrim.GetRadius();
					int sphereCenterIndex = spherePrim.GetCenterIndex();
					ChangeBvhExtentsFromEmbeddedPrim(sphereCenterIndex,radius);
				}
				else if (bvhPrimitive->GetType()==PRIM_TYPE_CAPSULE)
				{
					// This is an embedded capsule, so see if it sticks out of the bound extents.
					const phPrimCapsule& capsulePrim =  bvhPrimitive->GetCapsule();
					float radius = capsulePrim.GetRadius();
					int capsuleIndexA = capsulePrim.GetEndIndex0();
					ChangeBvhExtentsFromEmbeddedPrim(capsuleIndexA,radius);
					int capsuleIndexB = capsulePrim.GetEndIndex1();
					ChangeBvhExtentsFromEmbeddedPrim(capsuleIndexB,radius);
				}
				else if (bvhPrimitive->GetType()==PRIM_TYPE_CYLINDER)
				{
					// This is an embedded capsule, so see if it sticks out of the bound extents.
					const phPrimCylinder& cylinderPrim =  bvhPrimitive->GetCylinder();
					float radius = cylinderPrim.GetRadius();
					int cylinderIndexA = cylinderPrim.GetEndIndex0();
					ChangeBvhExtentsFromEmbeddedPrim(cylinderIndexA,radius);
					int cylinderIndexB = cylinderPrim.GetEndIndex1();
					ChangeBvhExtentsFromEmbeddedPrim(cylinderIndexB,radius);
				}
			}
		}
	}

#if COMPRESSED_VERTEX_METHOD > 0
	CalculateQuantizationValues();
#endif
}


void phBoundGeometry::ChangeBvhExtentsFromEmbeddedPrim (int vertexIndex, float radius)
{
	bool outsideBox = false;
	Vector3 vertexPosition(VEC3V_TO_VECTOR3(GetVertex(vertexIndex)));
	Vector3 sphereMin(vertexPosition);
	sphereMin.Subtract(radius,radius,radius);
	Vec3V boundingBoxMin = GetBoundingBoxMin();
	if (sphereMin.x<boundingBoxMin.GetXf())
	{
		boundingBoxMin.SetXf(sphereMin.x);
		outsideBox = true;
	}
	if (sphereMin.y<boundingBoxMin.GetYf())
	{
		boundingBoxMin.SetYf(sphereMin.y);
		outsideBox = true;
	}
	if (sphereMin.z<boundingBoxMin.GetZf())
	{
		boundingBoxMin.SetZf(sphereMin.z);
		outsideBox = true;
	}
	SetBoundingBoxMin(boundingBoxMin);
	
	Vector3 sphereMax(vertexPosition);
	sphereMax.Add(radius,radius,radius);
	Vec3V boundingBoxMax = GetBoundingBoxMax();
	if (sphereMax.x>boundingBoxMax.GetXf())
	{
		boundingBoxMax.SetXf(sphereMax.x);
		outsideBox = true;
	}
	if (sphereMax.y>boundingBoxMax.GetYf())
	{
		boundingBoxMax.SetYf(sphereMax.y);
		outsideBox = true;
	}
	if (sphereMax.z>boundingBoxMax.GetZf())
	{
		boundingBoxMax.SetZf(sphereMax.z);
		outsideBox = true;
	}
	SetBoundingBoxMax(boundingBoxMax);

	float sphereRadiusAroundCentroid = vertexPosition.Dist(VEC3V_TO_VECTOR3(GetCentroidOffset())) + radius;
	if (sphereRadiusAroundCentroid>m_RadiusAroundCentroid)
	{
		m_RadiusAroundCentroid = sphereRadiusAroundCentroid;
	}

	if (outsideBox)
	{

	#if COMPRESSED_VERTEX_METHOD>0
		// Change all the compressed vertex positions to match their old values in the new bounding box extents.
		Vector3* vertexPosition = Alloca(Vector3,m_NumVertices);
		for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
		{
			vertexPosition[vertexIndex].Set(VEC3V_TO_VECTOR3(GetVertex(vertexIndex)));
		}

		// Set the new box center and recompute quantization.
		m_BoundingBoxCenter = ComputeBoundingBoxCenter();
		CalculateQuantizationValues();

		// Re-compress all the vertex positions.
		for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
		{
			SetVertex(vertexIndex,RCC_VEC3V(vertexPosition[vertexIndex]));
		}
	#endif

	}
}


void phBoundGeometry::CalculateBoundingBox(const Vector3 *vertices, const Vector3 *shrunkenVertices)
{
	Vec3V margin = Vec3V(GetMarginV());
	if (m_NumVertices==0 || (!vertices && !shrunkenVertices))
	{
		SetBoundingBoxMax(margin);
		SetBoundingBoxMin(-margin);
	}
	else
	{
		Vec3V max = -Vec3V(V_FLT_MAX);
		Vec3V min = Vec3V(V_FLT_MAX);

		// Find the min and max of the non-shrunken vertices, without a margin
		if(vertices)
		{
			for(int i=0; i<m_NumVertices; i++)
			{
				max = Max(RCC_VEC3V(vertices[i]), max);
				min = Min(RCC_VEC3V(vertices[i]), min);
			}
		}

		// Find the min and max of the shrunken vertices, using the margin
		if(shrunkenVertices)
		{
			Vec3V maxShrunk = -Vec3V(V_FLT_MAX);
			Vec3V minShrunk = Vec3V(V_FLT_MAX);

			for(int i=0; i<m_NumVertices; i++)
			{
				maxShrunk = Max(RCC_VEC3V(shrunkenVertices[i]), maxShrunk);
				minShrunk = Min(RCC_VEC3V(shrunkenVertices[i]), minShrunk);
			}

			max = Max(max, maxShrunk + margin);
			min = Min(min, minShrunk - margin);
		}
		else
		{
			max += margin;
			min -= margin;
		}

		// Extend the bounding box slightly so that roundoff errors don't cause vertices to be found outside the box.
		const Vec3V boxExtension(V_FLT_SMALL_3);
		SetBoundingBoxMax(max + boxExtension);
		SetBoundingBoxMin(min - boxExtension);
	}
}


void phBoundGeometry::CopyHeader (const phBoundGeometry* original)
{
	*this = *original;
}

void phBoundGeometry::Copy (const phBound* original)
{
	// Make sure the original is derived from phBoundPolyhedron.
	Assert(original->IsPolygonal());
	const phBoundGeometry* geomOriginal = static_cast<const phBoundGeometry*>(original);

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init(geomOriginal->GetNumVertices(), geomOriginal->GetNumPerVertexAttribs(), geomOriginal->GetNumMaterials(), geomOriginal->GetNumMaterialColors(), geomOriginal->GetNumPolygons(), geomOriginal->m_SecondSurfaceVertexDisplacements ? 1 : 0);
	#else
		Init(geomOriginal->GetNumVertices(), geomOriginal->GetNumPerVertexAttribs(), geomOriginal->GetNumMaterials(), geomOriginal->GetNumPolygons(), geomOriginal->m_SecondSurfaceVertexDisplacements ? 1 : 0);
	#endif
#else //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init(geomOriginal->GetNumVertices(), geomOriginal->GetNumPerVertexAttribs(), geomOriginal->GetNumMaterials(), geomOriginal->GetNumMaterialColors(), geomOriginal->GetNumPolygons());
	#else
		Init(geomOriginal->GetNumVertices(), geomOriginal->GetNumPerVertexAttribs(), geomOriginal->GetNumMaterials(), geomOriginal->GetNumPolygons());
	#endif
#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...

#if COMPRESSED_VERTEX_METHOD == 0
    Vector3* shrunkVertices = NULL;
    if (geomOriginal->m_ShrunkVertices)
    {
        shrunkVertices = rage_new Vector3[geomOriginal->GetNumVertices()];
    }
#else
	CompressedVertexType *compressedShrunkVertices = NULL;
	if (geomOriginal->m_CompressedShrunkVertices)
	{
		compressedShrunkVertices = rage_new CompressedVertexType[3 * geomOriginal->GetNumVertices()];
	}
#endif

	// Copy out pointers that will be changed when this bound is copied to the clone.
#if COMPRESSED_VERTEX_METHOD == 0
	const Vector3* vertices = (const Vector3*)m_Vertices;
#endif
	const phPolygon* polygons = m_Polygons;
	phMaterialMgr::Id* materialIndices = m_MaterialIds;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	u32* materialColours = m_MaterialColors;
#endif // HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	u32* vertexAttribs = m_VertexAttribs;

#if COMPRESSED_VERTEX_METHOD > 0
	CompressedVertexType *compressedVertices = m_CompressedVertices;
#endif

	u8* polyMatIndexList = m_PolyMatIndexList;

	u8 originalType = m_Type;
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	float* secondSurfaceVertexMovements = m_SecondSurfaceVertexDisplacements;
#endif

#if OCTANT_MAP_SUPPORT_ACCEL
	Assert(!m_OctantVertCounts);
#endif // OCTANT_MAP_SUPPORT_ACCEL

	// Copy everything in the bound to the clone.
	*this = *geomOriginal;

	if (phConfig::IsRefCountingEnabled())
	{
		SetRefCount(1);
	}
	// after we've done the geometry copy, need to restore the bound type (might be a derived class)
	m_Type = originalType;

	// Reset the bound part pointers and local matrix pointers.
#if COMPRESSED_VERTEX_METHOD == 0
	m_Vertices = (const Vec3V*)vertices;
	m_ShrunkVertices = (Vec3V*)shrunkVertices;
#else
	m_CompressedShrunkVertices = compressedShrunkVertices;
#endif
	m_Polygons = polygons;
	m_MaterialIds = materialIndices;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	m_MaterialColors = materialColours;
#endif // HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	m_VertexAttribs = vertexAttribs;

	m_PolyMatIndexList = polyMatIndexList;

#if COMPRESSED_VERTEX_METHOD > 0
	m_CompressedVertices = compressedVertices;
#endif

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	m_SecondSurfaceVertexDisplacements=secondSurfaceVertexMovements;
#endif

	m_PolyMatIndexList = polyMatIndexList;
	sysMemCpy(m_PolyMatIndexList, geomOriginal->m_PolyMatIndexList, m_NumPolygons * sizeof(*m_PolyMatIndexList));

	// Copy the lists of vertices, edges and polygons, so that the clone has its
	// own lists instead of pointing to the original's lists.
#if COMPRESSED_VERTEX_METHOD > 0
	sysMemCpy(m_CompressedVertices, geomOriginal->m_CompressedVertices, m_NumVertices * 3 * sizeof(CompressedVertexType));
#else
	int index;
	for (index=0;index<m_NumVertices;index++)
	{
		SetVertex(index,geomOriginal->GetVertex(index));
	}
#endif

	sysMemCpy((phPolygon*)m_Polygons, geomOriginal->m_Polygons, m_NumPolygons * sizeof(*m_Polygons));
	sysMemCpy(m_PolyMatIndexList, geomOriginal->m_PolyMatIndexList, m_NumPolygons * sizeof(*m_PolyMatIndexList));
	sysMemCpy(m_MaterialIds, geomOriginal->m_MaterialIds, m_NumMaterials * sizeof(*m_MaterialIds));
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	sysMemCpy(m_MaterialColors, geomOriginal->m_MaterialColors, m_NumMaterialColors * sizeof(*m_MaterialColors));
#endif
	sysMemCpy(m_VertexAttribs, geomOriginal->m_VertexAttribs, m_NumVertices * m_NumPerVertexAttribs * sizeof(u32));

#if COMPRESSED_VERTEX_METHOD == 0
	if(m_ShrunkVertices)
	{
		for (index=0;index<m_NumVertices;index++)
		{
			SetShrunkVertex(index, geomOriginal->GetShrunkVertex(index));
		}
	}
#else
	if(m_CompressedShrunkVertices)
	{
		sysMemCpy(m_CompressedShrunkVertices, geomOriginal->m_CompressedShrunkVertices, m_NumVertices * 3 * sizeof(CompressedVertexType));
	}
#endif

#if OCTANT_MAP_SUPPORT_ACCEL
#if USE_OCTANT_MAP_INDEX_U16
	FastAssert(m_Flags & OCTANT_MAP_INDEX_IS_U16);
#endif // USE_OCTANT_MAP_INDEX_U16
	if (geomOriginal->HasOctantMap())
	{
		// NULL these out to prevent some asserts inside OctantMapAllocateAndCopy and to ensure the permanent allocation works correctly.
		m_OctantVertCounts = NULL;
		m_OctantVerts = NULL;
		OctantMapAllocateAndCopy(geomOriginal->m_OctantVertCounts,geomOriginal->m_OctantVerts);
		Assert(HasOctantMap());
	}
#endif // OCTANT_MAP_SUPPORT_ACCEL

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	if(m_SecondSurfaceVertexDisplacements)
	{
		Assert(m_NumSecondSurfaceVertexDisplacements==m_NumVertices);
		Assert(m_NumSecondSurfaceVertexDisplacements==geomOriginal->m_NumSecondSurfaceVertexDisplacements);
		Assert((0==m_NumSecondSurfaceVertexDisplacements) || (m_SecondSurfaceVertexDisplacements && geomOriginal->m_SecondSurfaceVertexDisplacements));
		sysMemCpy(m_SecondSurfaceVertexDisplacements, geomOriginal->m_SecondSurfaceVertexDisplacements, m_NumSecondSurfaceVertexDisplacements * sizeof(*m_SecondSurfaceVertexDisplacements));
	}
#endif
}

////////////////////////////////////////////////////////////////

bool phBoundGeometry::OverlapRegion(const Vector3 *perimeter, const int nperimverts, float cosTheta, float sinTheta)
{
	Assert(nperimverts);

	// Run through polys, and return as soon as we
	// find onme intersecting the region.
	for (int i = 0; i < m_NumPolygons; i++)
	{
		for (int v = 0; v < POLY_MAX_VERTICES; v++)
		{
			const Vector3 rvec = VEC3V_TO_VECTOR3(GetVertex(m_Polygons[i].GetVertexIndex(v)));
			float x = cosTheta*rvec.x - sinTheta*rvec.z;
			float y = sinTheta*rvec.x + cosTheta*rvec.z;

			// Just need to know as soon as any vert found
			// within area.
			if (geom2D::IsPointInRegion(x, y, perimeter, nperimverts))
			{
				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////
#if __BANK
bool g_UseBBoxForPolyFinding = true;
#endif // __BANK

int phBoundGeometry::FindElementIndex (Vector3::Param localNormal, Vector3::Param localContact, int* vertexIndices, int numVertices) const
{
PPC_STAT_TIMER_SCOPED(FindElementIndexTimer);
PPC_STAT_COUNTER_INC(FindElementIndexCounter,1);

#if __BANK
	if(!g_UseBBoxForPolyFinding)
	{
		return FindElementIndexOld (localNormal, vertexIndices, numVertices);
	}
#endif //__BANK

	// If we don't have any materials applied, don't bother to compute the element index
	if (m_NumMaterials <= 1)
	{
		return 0;
	}

	const phPolygon* polygons = m_Polygons;
	int nMaxNumOfVerticesFound = 0;
	int nBestPolyIndex = -1;
	int pBestPolyIndices [4];
	ScalarV sMargin = GetMarginV();
	sMargin = Add(sMargin, sMargin); // double the margin for polygon bounding box test

	switch (numVertices)
	{
	case 4:
	case 3:
		{
			int pContainedIndices [4];
			for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
			{
				int nContainedVertices = polygons[polygonIndex].ContainsVertices(vertexIndices,numVertices, pContainedIndices);
				if(nContainedVertices > nMaxNumOfVerticesFound)
				{
					if(nContainedVertices >= 3)
					{
						break;
					}
					const phPolygon& polygon = polygons[polygonIndex];
					if(polygon.DetectPointInsideBoundingBox(
						GetVertex(polygon.GetVertexIndex(0)),
						GetVertex(polygon.GetVertexIndex(1)),
						GetVertex(polygon.GetVertexIndex(2)),
						VECTOR3_TO_VEC3V(localContact), sMargin))
					{
						nMaxNumOfVerticesFound = nContainedVertices;
						nBestPolyIndex = polygonIndex;
						sysMemCpy(pBestPolyIndices, pContainedIndices, nContainedVertices * sizeof(int));
					}
				}

			}

			if (nMaxNumOfVerticesFound>=3)
			{
				// This polygon contains all 3 or 4 of the given vertices, so it must be the right polygon.
				return nBestPolyIndex;
			}

		}


		// Fall through...basically just discard which ever point was last and try again with the first two verts
	case 2:
		{
			if(nMaxNumOfVerticesFound == 0)
			{
				int pContainedIndices [4];
				for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
				{
					int nContainedVertices = polygons[polygonIndex].ContainsVertices(vertexIndices,numVertices, pContainedIndices);
					if(nContainedVertices > nMaxNumOfVerticesFound)
					{
						if(nContainedVertices >= 2)
						{
							nMaxNumOfVerticesFound = nContainedVertices;
							nBestPolyIndex = polygonIndex;
							sysMemCpy(pBestPolyIndices, pContainedIndices, nContainedVertices * sizeof(int));
							break;
						}

						const phPolygon& polygon = polygons[polygonIndex];
						if(polygon.DetectPointInsideBoundingBox(
							GetVertex(polygon.GetVertexIndex(0)),
							GetVertex(polygon.GetVertexIndex(1)),
							GetVertex(polygon.GetVertexIndex(2)),
							VECTOR3_TO_VEC3V(localContact), sMargin))
						{
							nMaxNumOfVerticesFound = nContainedVertices;
							nBestPolyIndex = polygonIndex;
							sysMemCpy(pBestPolyIndices, pContainedIndices, nContainedVertices * sizeof(int));
						}
					}
				}

			}

			if (nMaxNumOfVerticesFound>=2)
			{
				// This polygon contains both of the given vertices.
				int neighborIndex = polygons[nBestPolyIndex].FindNeighborWithVertices(vertexIndices[pBestPolyIndices[0]],vertexIndices[pBestPolyIndices[1]]);
				if (neighborIndex!=(phPolygon::Index)BAD_INDEX)
				{
					bool bInsideBestPoly = false;
					bool bInsideNeighborPoly = false;
					const phPolygon& polygon = polygons[nBestPolyIndex];
					if(polygon.DetectPointInsideBoundingBox(
						GetVertex(polygon.GetVertexIndex(0)),
						GetVertex(polygon.GetVertexIndex(1)),
						GetVertex(polygon.GetVertexIndex(2)),
						VECTOR3_TO_VEC3V(localContact), sMargin))
					{
						bInsideBestPoly = true;
					}

					const phPolygon& neighborPolygon = polygons[neighborIndex];
					if(neighborPolygon.DetectPointInsideBoundingBox(
						GetVertex(neighborPolygon.GetVertexIndex(0)),
						GetVertex(neighborPolygon.GetVertexIndex(1)),
						GetVertex(neighborPolygon.GetVertexIndex(2)),
						VECTOR3_TO_VEC3V(localContact), sMargin))
					{
						bInsideNeighborPoly = true;
					}

					if(bInsideBestPoly == bInsideNeighborPoly)
					{
						// This polygon and its neighbor contain both the given vertices. Return the index for either this polygon or its neighbor, whichever has a normal
						// vector closest to the given collision normal.

						Vector3 polyNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(nBestPolyIndex)));
						Vector3 neighborNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex)));

						return (Vector3(localNormal).DotV(polyNormal).IsGreaterThan(Vector3(localNormal).DotV(neighborNormal)) ? nBestPolyIndex : neighborIndex);
					}
					else
					{
						return bInsideBestPoly ? nBestPolyIndex : neighborIndex;
					}

				}
				else
				{
					// This polygon contains both the given vertices, but either they are not neighbors or there is no neighboring polygon that shares them.
					return nBestPolyIndex;
				}
			}
		}

		// Fall through...basically just pick a polygon that the first vertex was involved in
	case 1:
		{
			if(nMaxNumOfVerticesFound == 0)
			{
				int pContainedIndices [4];
				for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
				{
					int nContainedVertices = polygons[polygonIndex].ContainsVertices(vertexIndices,numVertices, pContainedIndices);
					if(nContainedVertices)
					{
						const phPolygon& polygon = polygons[polygonIndex];
						if(polygon.DetectPointInsideBoundingBox(
							GetVertex(polygon.GetVertexIndex(0)),
							GetVertex(polygon.GetVertexIndex(1)),
							GetVertex(polygon.GetVertexIndex(2)),
							VECTOR3_TO_VEC3V(localContact), sMargin))
						{
							nMaxNumOfVerticesFound = nContainedVertices;
							nBestPolyIndex = polygonIndex;
							sysMemCpy(pBestPolyIndices, pContainedIndices, nContainedVertices * sizeof(int));
							break;
						}
					}
				}

			}

			if (nMaxNumOfVerticesFound)
			{
				const phPolygon& polygon = polygons[nBestPolyIndex];
				Vector3 polyNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(nBestPolyIndex)));

				// This polygon contains the given vertex.
				Vector3 bestNormDotNorm = Vector3(localNormal).DotV(polyNormal);
				int bestPolyIndex = nBestPolyIndex;
				phPolygon::Index neighborIndex = polygon.FindNeighborWithVertex(vertexIndices[pBestPolyIndices[0]]);
				int prevNeighborIndex = nBestPolyIndex;
				while (neighborIndex!=nBestPolyIndex && neighborIndex != (phPolygon::Index)BAD_INDEX)
				{
					const phPolygon& neighbor = polygons[neighborIndex];

					Vector3 neighborNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex)));
					Vector3 normDotNorm = Vector3(localNormal).DotV(neighborNormal);
					if (normDotNorm.IsGreaterThan(bestNormDotNorm))
					{
						if(neighbor.DetectPointInsideBoundingBox(
							GetVertex(neighbor.GetVertexIndex(0)),
							GetVertex(neighbor.GetVertexIndex(1)),
							GetVertex(neighbor.GetVertexIndex(2)),
							VECTOR3_TO_VEC3V(localContact), sMargin))
						{
							bestNormDotNorm = normDotNorm;
							bestPolyIndex = neighborIndex;
						}
					}

					phPolygon::Index nextNeighborIndex = neighbor.FindNeighborWithVertex(vertexIndices[pBestPolyIndices[0]],prevNeighborIndex);
					prevNeighborIndex = neighborIndex;
					neighborIndex = nextNeighborIndex;
				}

				return bestPolyIndex;
			}
		}
	}
	
	return 0;
}


////////////////////////////////////////////////////////////////
// utility functions

#if RSG_TOOL
void phBoundGeometry::Copy (phBoundGeometry* original)
{
	Assert(m_NumVertices==0);

	int i;

	// initialize
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init(original->GetNumVertices(),original->GetNumPerVertexAttribs(),original->GetNumMaterials(),0,original->GetNumPolygons(),original->m_SecondSurfaceVertexDisplacements ? 1 : 0);
	#else
		Init(original->GetNumVertices(),original->GetNumPerVertexAttribs(),original->GetNumMaterials(),original->GetNumPolygons(), original->m_SecondSurfaceVertexDisplacements ? 1 : 0);
	#endif // HACK_GTA4_64BIT_MATERIAL_ID_COLORS
#else
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		Init(original->GetNumVertices(),original->GetNumPerVertexAttribs(),original->GetNumMaterials(),0,original->GetNumPolygons());
	#else
		Init(original->GetNumVertices(),original->GetNumPerVertexAttribs(),original->GetNumMaterials(),original->GetNumPolygons());
	#endif // HACK_GTA4_64BIT_MATERIAL_ID_COLORS
#endif

	// NOTE: mutable pointers since phBoundGeometry
	// controls its own components
	phPolygon * geomPolys = (phPolygon*)m_Polygons;
	Vector3 * geomVerts = (Vector3*)m_Vertices;

	for (i=original->GetNumVertices()-1; i>=0; i--)
	{
		geomVerts[i].Set(VEC3V_TO_VECTOR3(original->GetVertex(i)));
	}

	for (i=original->GetNumPolygons()-1; i>=0; i--)
	{
		geomPolys[i].Set(original->GetPolygon(i));
	}

	for (i=original->GetNumMaterials()-1; i>=0; i--)
	{
		m_MaterialIds[i] = original->m_MaterialIds[i];
	}

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	for (i=original->GetNumVertices()-1; i>=0; i--)
	{
		Assert(m_SecondSurfaceVertexDisplacements && original->m_SecondSurfaceVertexDisplacements);
		m_SecondSurfaceVertexDisplacements[i]=original->m_SecondSurfaceVertexDisplacements[i];
	}
#endif
}

#endif	// end of #if RSG_TOOL

} // namespace rage

