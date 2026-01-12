//
// phbound/boundbvh.cpp
//
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved.
//
#include "boundbvh.h"

#include "boundbox.h"
#include "boundcapsule.h"
#include "boundculler.h"
#include "boundcylinder.h"
#include "boundsphere.h"
#include "cullerhelpers.h"
#include "primitives.h"
#include "OptimizedBvh.h"
#include "support.h"

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "file/token.h"
#include "phbullet/CollisionMargin.h"
#include "phbullet/TriangleCallback.h"
#include "phcore/materialmgr.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/element.h"
#include "system/alloca.h"
#include "system/memory.h"
#include "system/timemgr.h"
#include "vector/geometry.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#if !__SPU
PARAM(spewandfixbadbvhpolys, "[physics] Fixes several known issue with world geometry triangles and spews about it");
PARAM(spewbadbvhpolys, "[physics] Spews about several known issue with world geometry triangles");
PARAM(fixbadbvhpolys, "[physics] Spews about several known issue with world geometry triangles");
#endif

#define TEST_BVH_CULLING 0
#define BOUNDBVH_REAL_OBB_CULLING	0
// Set this back to zero and get rid of the code once all resources have been rebuilt.
#define BOUNDBVH_CLEAR_HIGH_TYPE_BIT_IN_PRIMITIVES	0

// Enable this to make CullOBBPolys() not actually do any culling and just return all of the polygons.
#define DISABLE_CULLING_OBB	0

#if TEST_BVH_CULLING
#include "math/random.h"		// Random needed just for testing.
#endif

namespace rage {
#if !__TOOL
	CompileTimeAssert(sizeof(phBoundBVH) <= phBound::MAX_BOUND_SIZE);
#endif

///////////////////////////////////////////////////////////////////////////////////


namespace phBoundStats
{
    EXT_PF_TIMER(BVH_Sphere);
    EXT_PF_TIMER(BVH_Capsule);
    EXT_PF_TIMER(BVH_Poly);
};

using namespace phBoundStats;

namespace phCollisionStats
{
    EXT_PF_TIMER(BVHTrianglePrep);
}

using namespace phCollisionStats;

EXT_PFD_DECLARE_ITEM(BVHHierarchy);
EXT_PFD_DECLARE_ITEM(BVHHierarchyNodeIndices);
EXT_PFD_DECLARE_ITEM_SLIDER(NodeDepth);
EXT_PFD_DECLARE_ITEM(BVHSubtreeColoring);
EXT_PFD_DECLARE_ITEM(BVHNodeColoring);
EXT_PFD_DECLARE_ITEM(BVHDepthColoring);
EXT_PFD_DECLARE_ITEM(CulledPolygons);
EXT_PFD_DECLARE_ITEM(IncludePolygons);
EXT_PFD_DECLARE_ITEM_SLIDER(DensityCullRadius);
EXT_PFD_DECLARE_ITEM_SLIDER(MaxPrimitivesPerMeterSquared);
EXT_PFD_DECLARE_ITEM_SLIDER(MaxNonPolygonsPerMeterSquared);

///////////////////////////////////////////////////////////////////////////////////

#if !__SPU
phBoundBVH::phBoundBVH () : phBoundGeometry ()
, m_ActivePolygonIndices(NULL)
, m_NumActivePolygons((u16)(-1))
{
    m_Type = BVH;
    m_BVH = NULL;

    SetMargin(CONCAVE_DISTANCE_MARGIN);
    memset(m_Pad, 0, sizeof(m_Pad));
}

phBoundBVH::~phBoundBVH ()
{
    if(m_BVH != NULL)
    {
        delete m_BVH;
        m_BVH = NULL;
    }
}

#if __PFDRAW || (__WIN32PC && !__FINAL)
Vec3V_Out phBoundBVH::GetPrimitiveCenter(int primIndex) const
{
	const phPrimitive& primitive = GetPrimitive(primIndex);
	switch(primitive.GetType())
	{
	case PRIM_TYPE_BOX:
		{
			const phPrimBox& box = primitive.GetBox();
			return Scale(GetVertex(box.GetVertexIndex(0)) + GetVertex(box.GetVertexIndex(1)) + GetVertex(box.GetVertexIndex(2)) + GetVertex(box.GetVertexIndex(3)),ScalarV(V_QUARTER));
		}
	case PRIM_TYPE_POLYGON:
		{
			const phPolygon& polygon = primitive.GetPolygon();
			return Scale(GetVertex(polygon.GetVertexIndex(0)) + GetVertex(polygon.GetVertexIndex(1)) + GetVertex(polygon.GetVertexIndex(2)), ScalarV(V_THIRD));
		}
	case PRIM_TYPE_CYLINDER:
		{
			const phPrimCylinder& cylinder = primitive.GetCylinder();
			return Average(GetVertex(cylinder.GetEndIndex0()),GetVertex(cylinder.GetEndIndex1()));
		}
	case PRIM_TYPE_CAPSULE:
		{
			const phPrimCapsule& capsule = primitive.GetCapsule();
			return Average(GetVertex(capsule.GetEndIndex0()),GetVertex(capsule.GetEndIndex1()));
		}
	case PRIM_TYPE_SPHERE:
		{
			const phPrimSphere& sphere = primitive.GetSphere();
			return GetVertex(sphere.GetCenterIndex());
		}
	default: 
		{
			Assert(false);
			return Vec3V(V_ZERO);
		}
	}
}

float phBoundBVH::RatePrimitive(int primIndex, bool bIncludePolygons, float maxPrimitivesPerMeterSquared) const
{
	// Find the cull shape
	Vec3V primitiveCenter = GetPrimitiveCenter(primIndex);
	const ScalarV queryHalfExtents = ScalarVFromF32(PFD_GETVALUE(PFD_DensityCullRadius));

	// Cull primitives
	const phOptimizedBvh* bvhStructure = GetBVH();
	phBoundCuller culler;
	const int maxCulledPrimitives = 2048;
	phPolygon::Index culledPrimitives[maxCulledPrimitives];
	culler.SetArrays(culledPrimitives,maxCulledPrimitives);
	phBvhAabbOverlapCallback nodeOverlapCallback(&culler, bvhStructure);
	nodeOverlapCallback.SetAABB(VEC3V_TO_VECTOR3(Subtract(primitiveCenter,Vec3V(queryHalfExtents))),VEC3V_TO_VECTOR3(Add(primitiveCenter,Vec3V(queryHalfExtents))));
	bvhStructure->walkStacklessTree(&nodeOverlapCallback);
	
	// If we aren't counting polygons remove them from the count
	u16 numCulledPrimitives = culler.GetNumCulledPolygons();
	if(!bIncludePolygons)
	{
		u16 numCulledPolygons = 0;
		for(int culledPrimitiveIndex = 0; culledPrimitiveIndex < numCulledPrimitives; ++culledPrimitiveIndex)
		{
			if(GetPrimitive(culler.GetCulledPolygonIndexList()[culledPrimitiveIndex]).GetType() == PRIM_TYPE_POLYGON)
			{
				++numCulledPolygons;
			}
		}
		numCulledPrimitives -= numCulledPolygons;
	}

	// returning 1 means we culled 0 primitives
	// returning 0 means we culled more than the maximum primitives per meter squared
	float cullArea = Scale(Scale(queryHalfExtents,queryHalfExtents),ScalarV(V_FOUR)).Getf();
	float primitivesPerMeterSquared = (float)numCulledPrimitives/cullArea;
	return 1.0f - Clamp(primitivesPerMeterSquared/maxPrimitivesPerMeterSquared,0.0f,1.0f);
}

Color32 phBoundBVH::ComputePrimitiveDensityColor(phPolygon::Index primitiveIndex, u32 PFD_ONLY(typeFlags), u32 PFD_ONLY(includeFlags)) const
{
	float maxPrimitivesPerMeterSquared = PFD_GETENABLED(PFD_IncludePolygons)
		? PFD_GETVALUE(PFD_MaxPrimitivesPerMeterSquared)
		: PFD_GETVALUE(PFD_MaxNonPolygonsPerMeterSquared);
#if __PFDRAW
	maxPrimitivesPerMeterSquared *= GetMaxTypeIncludeFlagScale(typeFlags,includeFlags);
#endif // __PFDRAW
	return ComputeDensityColor(RatePrimitive(primitiveIndex,PFD_GETENABLED(PFD_IncludePolygons),maxPrimitivesPerMeterSquared));
}

#endif // __PFDRAW || (__WIN32PC && !__FINAL)

#if __PFDRAW

void phBoundBVH::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int whichPolys, phMaterialFlags highlightFlags, unsigned int typeFilter, unsigned int includeFilter, unsigned int boundTypeFlags, unsigned int boundIncludeFlags) const
{
	static int DepthBiasInitialized = false;
	static grcRasterizerStateHandle g_DepthBiasRasterizerState;
	static grcRasterizerStateHandle g_BorderDepthBiasRasterizerState;
	static bool UseDepthBias = false;		// TODO: Add this as a rag option.
	if (DepthBiasInitialized == false)
	{
		DepthBiasInitialized = true;
		grcRasterizerStateDesc RasterizerStateDesc;
		grcRasterizerStateDesc BorderRasterizerStateDesc;
#if __XENON
		RasterizerStateDesc.DepthBiasDX9 = 0.002f;
		BorderRasterizerStateDesc.DepthBiasDX9 = 0.01f;
#else
		RasterizerStateDesc.DepthBiasDX9 = -0.002f;
		BorderRasterizerStateDesc.DepthBiasDX9 = -0.01f;
#endif
		g_DepthBiasRasterizerState = grcStateBlock::CreateRasterizerState(RasterizerStateDesc);
		g_BorderDepthBiasRasterizerState = grcStateBlock::CreateRasterizerState(BorderRasterizerStateDesc);
	}
	const grcRasterizerStateHandle SavedRasterizerState = grcStateBlock::RS_Active;
	if (UseDepthBias)
		grcStateBlock::SetRasterizerState(g_DepthBiasRasterizerState);

	const Matrix34& mtx = RCC_MATRIX34(mtxIn);

	if(!PFD_BVHSubtreeColoring.WillDraw() && !PFD_BVHNodeColoring.WillDraw() && !PFD_BVHDepthColoring.WillDraw())
	{
		// Do the regular drawing for the bound.
		phBoundGeometry::Draw(mtxIn, colorMaterials, solid, whichPolys, highlightFlags, typeFilter, includeFilter, boundTypeFlags, boundIncludeFlags);
	}
	else
	{
		grcWorldMtx(mtx);
		Color32 oldColor(grcCurrentColor);
		const bool oldLighting = grcLighting(false);

		// Ooh, they want the extra fancy coloring.

		// Used for coloring according to subtree.
		int curSubTreeIndex = 0;
		int firstNodeIndexInNextSubtree = m_BVH->GetNumUsedSubtreeHeaders() > 1 ? m_BVH->GetSubtreeHeaders()[1].m_RootNodeIndex : m_BVH->GetNumNodes();

		// Used for coloring according to XXX
		int curNodeDepth = 0;
		int nodeIndex[100];
		bool isLeftChild[100];

		int curNodeIndex = 0;
		nodeIndex[0] = 0;
		isLeftChild[0] = true;

		while(curNodeIndex < m_BVH->GetNumNodes())
		{
			const phOptimizedBvhNode &curNode = m_BVH->GetNode(curNodeIndex);
			if(curNode.IsLeafNode() && (!PFD_BVHDepthColoring.WillDraw() || (int)(PFD_NodeDepth.GetValue()) <= curNodeDepth))
			{
				// We've got polygons; let's draw them.
				const int coloringIndex = PFD_BVHSubtreeColoring.WillDraw() ? curSubTreeIndex : PFD_BVHNodeColoring.WillDraw() ? curNodeIndex : nodeIndex[(int)(PFD_NodeDepth.GetValue())];
				u8 red = (u8)((23 * coloringIndex) & 255);
				u8 green = (u8)((107 * (coloringIndex + 1)) & 255);
				u8 blue = (u8)((83 * (coloringIndex + 2)) & 255);
				grcColor(Color32(red, green, blue));
				for (int curPrimInNodeIndex = 0 ; curPrimInNodeIndex < curNode.GetPolygonCount() ; ++curPrimInNodeIndex)
				{
					const int curPrimIndex = curNode.GetPolygonStartIndex() + curPrimInNodeIndex;
					const phPrimitive &curPrimitive = GetPrimitive(curPrimIndex);
					if (curPrimitive.GetType() == PRIM_TYPE_POLYGON)
					{
						const phPolygon &curPoly = curPrimitive.GetPolygon();
						const int numVertices = POLY_MAX_VERTICES;

						static float offset = 0.0f;
						const Vec3V normal = GetPolygonUnitNormal(curPrimIndex);
						const Vec3V offsetV = ScalarV(offset) * normal;
						const Vec3V v0 = GetVertex(curPoly.GetVertexIndex(0)) + offsetV;
						const Vec3V v1 = GetVertex(curPoly.GetVertexIndex(1)) + offsetV;
						const Vec3V v2 = GetVertex(curPoly.GetVertexIndex(2)) + offsetV;

						grcBegin(drawTris, (numVertices - 2) * 3);
						grcNormal3f(normal);
						grcVertex3f(v0);
						grcVertex3f(v1);
						grcVertex3f(v2);
						grcEnd();

						static bool drawBorder = false;
						if (drawBorder)
						{
							const grcRasterizerStateHandle SavedRasterizerState1 = grcStateBlock::RS_Active;
							if (UseDepthBias)
								grcStateBlock::SetRasterizerState(g_BorderDepthBiasRasterizerState);

							grcBegin(drawLineStrip,4);
							const Color32 saveColor = grcGetCurrentColor();
							grcColor(Color_white);
							grcVertex3f(v0);
							grcVertex3f(v1);
							grcVertex3f(v2);
							grcVertex3f(v0);
							grcColor(saveColor);
							grcEnd();

							if (UseDepthBias)
								grcStateBlock::SetRasterizerState(SavedRasterizerState1);
						}
					}
				}
			}

			// Go on to the next node.
			++curNodeIndex;
			if(curNode.IsLeafNode())
			{
				// This node was a leaf node, and so the next node we encounter will be a right-child of one of our ancestors.
				// We've bottomed out.  We need walk back up the tree until we find somebody who was a left child.
				while(curNodeDepth > 0 && !isLeftChild[curNodeDepth])
				{
					--curNodeDepth;
				}
				nodeIndex[curNodeDepth] = curNodeIndex;
				isLeftChild[curNodeDepth] = false;
			}
			else
			{
				// The next node will be our left-child.
				++curNodeDepth;
				nodeIndex[curNodeDepth] = curNodeIndex;
				isLeftChild[curNodeDepth] = true;
			}

			if(curNodeIndex >= firstNodeIndexInNextSubtree)
			{
				++curSubTreeIndex;
				firstNodeIndexInNextSubtree = curSubTreeIndex < m_BVH->GetNumUsedSubtreeHeaders() - 1 ? m_BVH->GetSubtreeHeaders()[curSubTreeIndex + 1].m_RootNodeIndex : m_BVH->GetNumNodes();
			}

		}

		grcLighting(oldLighting);
		grcColor(oldColor);
	}

	if(PFD_BVHHierarchy.WillDraw())
	{
		m_BVH->Draw(mtxIn, (int)PFD_NodeDepth.GetValue(),PFD_BVHHierarchyNodeIndices.WillDraw());
	}

	if (UseDepthBias)
		grcStateBlock::SetRasterizerState(SavedRasterizerState);
}

void phBoundBVH::DrawActivePolygons(Mat34V_In mtxIn) const
{
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	if (PFD_CulledPolygons.Begin())
	{
		grcWorldMtx(mtx);

		for (int i = 0; i < m_NumActivePolygons; ++i)
		{
			int polygonIndex = m_ActivePolygonIndices[i];
			Assert(polygonIndex>=0 && polygonIndex<m_NumPolygons);

			const phPolygon& polygon = m_Polygons[polygonIndex];
			int numVertices = POLY_MAX_VERTICES;

			grcBindTexture(NULL);
			grcBegin(drawTris,(numVertices - 2) * 3);
			grcNormal3f(GetPolygonUnitNormal(polygonIndex));
			grcVertex3fv(&GetVertex(polygon.GetVertexIndex(0))[0]);
			grcVertex3fv(&GetVertex(polygon.GetVertexIndex(1))[0]);
			grcVertex3fv(&GetVertex(polygon.GetVertexIndex(2))[0]);

			grcEnd();
		}

		PFD_CulledPolygons.End();
	}
}
#endif // __PFDRAW


#if __DEV
    void phBoundBVH::Validate ()
    {
        //		if (m_Octree!=NULL)
        //			return;
        //		m_Octree->Validate();
    }
#endif

#if !__SPU
	bool phBoundBVH::CanBecomeActive () const
	{
		return true;
	}
#endif	// !__SPU


    ////////////////////////////////////////////////////////////////////////////////////
    // load/save
    bool phBoundBVH::Load_v110 (fiAsciiTokenizer &token)
    {
        const bool retVal = phBoundGeometry::Load_v110(token);

		// Skip over octree data if it's there
		// This code was commented out by R*N because that allowed the loading of composite
		// bounds containing BVH. This version allows composites to contain BVHs so it should
		// be fine to integrate this to their branch.
		bool eof = false;
		while (!token.CheckIToken("verts:", false) &&
			!token.CheckIToken("bound:", false) &&
			!token.CheckIToken("matrix:", false) &&
			!token.CheckIToken("position:", false) &&
			!eof)
		{
			char buf[512];
			int tokensLeft = token.GetToken(buf,sizeof(buf));
			eof = (tokensLeft == 0);
		}

        if(retVal)
        {
            return Build();
        }

        return false;
    }

    bool phBoundBVH::Build(phPolygon::Index * const newToOldPolygonIndexMapping/* = NULL*/, int targetPrimsPerNode/* = DEFAULT_PRIMS_PER_NODE*/)
    {
        Assert(m_BVH == NULL);
        m_BVH = rage_new phOptimizedBvh;

#if 0
		m_BVH->BuildFromGeomBound(this, newToOldPolygonIndexMapping, targetPrimsPerNode);
#else
		const Vec3V boundingBoxMin = GetBoundingBoxMin();
		const Vec3V boundingBoxMax = GetBoundingBoxMax();

		// Take all of the polygons in the other guy and make a node for each one.
		const int primitiveCount = GetNumPolygons();
		const ScalarV boundMargin = GetMarginV();

		rage::sysMemStartTemp();
		BvhPrimitiveData *bvhPrimitives = rage_new BvhPrimitiveData[primitiveCount];
		rage::sysMemEndTemp();

		m_BVH->SetExtents(boundingBoxMin.GetIntrin128(), boundingBoxMax.GetIntrin128());

		for(int curPolyIndex = 0; curPolyIndex < primitiveCount; ++curPolyIndex)
		{
			const rage::phPrimitive *curPrim = &GetPrimitive(curPolyIndex);

			////////////////////////////////////////////////////////////
			// Determine the AABB and centroid for this polygon.
			Vec3V polygonAABBMin(V_FLT_MAX);
			Vec3V polygonAABBMax(V_NEG_FLT_MAX);
			Vec3V polygonCentroid(V_ZERO);

			const rage::PrimitiveType primType = curPrim->GetType();
			if(primType == rage::PRIM_TYPE_POLYGON)
			{
				const phPolygon& curPolygon = curPrim->GetPolygon();
				const int numVertsInPoly = POLY_MAX_VERTICES;
				for(int curVertInPolyIndex = numVertsInPoly - 1; curVertInPolyIndex >= 0; --curVertInPolyIndex)
				{
					const int curVertexIndex = curPolygon.GetVertexIndex(curVertInPolyIndex);
					const Vec3V curVertex = GetVertex(curVertexIndex);

					polygonAABBMin = Min(polygonAABBMin, curVertex);
					polygonAABBMax = Max(polygonAABBMax, curVertex);
					polygonCentroid = Add(polygonCentroid, curVertex);
				}
				polygonAABBMin = Subtract(polygonAABBMin, Vec3V(boundMargin));
				polygonAABBMax = Add(polygonAABBMax, Vec3V(boundMargin));
				polygonCentroid = polygonCentroid * ScalarV(V_THIRD);
			}
			else if(primType == rage::PRIM_TYPE_SPHERE)
			{
				const rage::phPrimSphere *spherePrim = &curPrim->GetSphere();
				const Vec3V sphereCenter = GetVertex(spherePrim->GetCenterIndex());
				const ScalarV sphereRadius = spherePrim->GetRadiusV();

				polygonAABBMin = Subtract(sphereCenter, Vec3V(sphereRadius));
				polygonAABBMax = Add(sphereCenter, Vec3V(sphereRadius));
				polygonCentroid = sphereCenter;
			}
			else if(curPrim->GetType() == rage::PRIM_TYPE_CAPSULE)
			{
				const rage::phPrimCapsule *capsulePrim = &curPrim->GetCapsule();
				const Vec3V endCenter0 = GetVertex(capsulePrim->GetEndIndex0());
				const Vec3V endCenter1 = GetVertex(capsulePrim->GetEndIndex1());
				const Vec3V sphereRadius = Vec3V(capsulePrim->GetRadiusV());

				polygonAABBMin = Min(polygonAABBMin, Subtract(endCenter0, sphereRadius));
				polygonAABBMin = Min(polygonAABBMin, Subtract(endCenter1, sphereRadius));
				polygonAABBMax = Max(polygonAABBMax, Add(endCenter0, sphereRadius));
				polygonAABBMax = Max(polygonAABBMax, Add(endCenter1, sphereRadius));
				polygonCentroid = Average(endCenter0, endCenter1);
			}
			else if(curPrim->GetType() == rage::PRIM_TYPE_BOX)
			{
				const rage::phPrimBox *boxPrim = &curPrim->GetBox();
				const Vec3V vertex0 = GetVertex(boxPrim->GetVertexIndex(0));
				const Vec3V vertex1 = GetVertex(boxPrim->GetVertexIndex(1));
				const Vec3V vertex2 = GetVertex(boxPrim->GetVertexIndex(2));
				const Vec3V vertex3 = GetVertex(boxPrim->GetVertexIndex(3));

				const ScalarV half(V_HALF);
				const ScalarV quarter(half * half);
				const Vec3V boxX = Abs(quarter * (vertex1 + vertex3 - vertex0 - vertex2));
				const Vec3V boxY = Abs(quarter * (vertex0 + vertex3 - vertex1 - vertex2));
				const Vec3V boxZ = Abs(quarter * (vertex2 + vertex3 - vertex0 - vertex1));
				const Vec3V aabbHalfWidth = (boxX + boxY + boxZ);

				polygonCentroid = quarter * (vertex0 + vertex1 + vertex2 + vertex3);
				polygonAABBMin = Subtract(polygonCentroid, aabbHalfWidth);
				polygonAABBMax = Add(polygonCentroid, aabbHalfWidth);
			}
			else
			{
				Assert(curPrim->GetType() == rage::PRIM_TYPE_CYLINDER);
				const rage::phPrimCylinder *cylinderPrim = &curPrim->GetCylinder();
				const Vec3V endCenter0 = GetVertex(cylinderPrim->GetEndIndex0());
				const Vec3V endCenter1 = GetVertex(cylinderPrim->GetEndIndex1());
				const ScalarV cylinderRadius = cylinderPrim->GetRadiusV();

				const Vec3V unitCylinderShaft = Normalize(Subtract(endCenter1, endCenter0));
				const Vec3V sineRatios = Sqrt(Max(Vec3V(V_ONE) - unitCylinderShaft * unitCylinderShaft,Vec3V(V_ZERO)));
				const Vec3V discHalfDims = sineRatios * cylinderRadius;
				polygonAABBMin = Min(endCenter0,endCenter1) - discHalfDims;
				polygonAABBMax = Max(endCenter0,endCenter1) + discHalfDims;
				polygonCentroid = Average(endCenter0, endCenter1);
			}
			//
			////////////////////////////////////////////////////////////

			// Quantize the extents and stuff them into the node.
			BvhPrimitiveData &curPolygonData = bvhPrimitives[curPolyIndex];
			curPolygonData.Clear();
			m_BVH->QuantizeMin(curPolygonData.m_AABBMin, RCC_VECTOR3(polygonAABBMin));
			m_BVH->QuantizeMax(curPolygonData.m_AABBMax, RCC_VECTOR3(polygonAABBMax));
			m_BVH->QuantizeClosest(curPolygonData.m_Centroid, RCC_VECTOR3(polygonCentroid));
			curPolygonData.m_PrimitiveIndex = (rage::phPolygon::Index)(curPolyIndex);
		}

		m_BVH->BuildFromPrimitiveData(bvhPrimitives, primitiveCount, primitiveCount, newToOldPolygonIndexMapping, targetPrimsPerNode, true);
		ReorderPrimitives(bvhPrimitives);

		rage::sysMemStartTemp();
		delete [] bvhPrimitives;
		rage::sysMemEndTemp();
#endif

        return true;
    }

	void phBoundBVH::Unbuild()
	{
		delete m_BVH;
		m_BVH = NULL;
	}


	void phBoundBVH::Copy (const phBound* original)
	{
		Assert(m_BVH == NULL);
		phBoundGeometry::Copy(original);

		if (original->GetType() == phBound::BVH)
		{
			const phBoundBVH* originalBVH = static_cast<const phBoundBVH*>(original);
			m_BVH = rage_new phOptimizedBvh;
			m_BVH->Copy(originalBVH->GetBVH());
		}

#if BOUNDBVH_CLEAR_HIGH_TYPE_BIT_IN_PRIMITIVES
		phPolygon *polygons = const_cast<phPolygon *>(m_Polygons);
		const int numPrimitives = GetNumPolygons();
		for(int primIndex = 0; primIndex < numPrimitives; ++primIndex)
		{
			phPolygon &curPolygon = polygons[primIndex];
			BvhPrimitive &curPrimitive = *reinterpret_cast<BvhPrimitive *>(&curPolygon);
			Assert(!curPrimitive.CheckHighTypeBit());
			curPrimitive.ClearHighTypeBit();
		}
#endif
	}

	void phBoundBVH::ReorderPrimitives(BvhPrimitiveData *newToOldPolygonMapping)
	{
		const int numPrimitives = GetNumPolygons();

		rage::sysMemStartTemp();
		rage::phPolygon::Index *oldToNew = rage_new rage::phPolygon::Index[numPrimitives];
		rage::sysMemEndTemp();

#if __ASSERT
		// Clear out this array.  We only do this so that we can assert that we're not doubling-up or missing any elements, so that's why this is wrapped
		//   in #if __ASSERT
		for(int curPolygonIndex = 0; curPolygonIndex < numPrimitives; ++curPolygonIndex)
		{
			oldToNew[curPolygonIndex] = (rage::phPolygon::Index)(-1);
		}
#endif	// __ASSERT

		// Fill out the oldToNew mapping array.
		for(int curPolygonIndex = 0; curPolygonIndex < numPrimitives; ++curPolygonIndex)
		{
			Assert(oldToNew[newToOldPolygonMapping[curPolygonIndex].m_PrimitiveIndex] == (rage::phPolygon::Index)(-1));
			oldToNew[newToOldPolygonMapping[curPolygonIndex].m_PrimitiveIndex] = (rage::phPolygon::Index)(curPolygonIndex);
		}

		for(int curPolygonIndex = 0; curPolygonIndex < numPrimitives; ++curPolygonIndex)
		{
			Assert(oldToNew[newToOldPolygonMapping[curPolygonIndex].m_PrimitiveIndex] == (rage::phPolygon::Index)(curPolygonIndex));
			Assert(newToOldPolygonMapping[oldToNew[curPolygonIndex]].m_PrimitiveIndex == (rage::phPolygon::Index)(curPolygonIndex));
		}

		// This is a const_cast which, yes, is generally bad, but I think the real problem is that m_Polygons is a pointer to constant polygons at all.
		// I don't see why that should be.
		rage::phPolygon *nonConstPolygons = const_cast<rage::phPolygon *>(GetPolygonPointer());

		// First we need to go through and remap all of the neighbors.
		for(int curPolyIndex = 0; curPolyIndex < numPrimitives; ++curPolyIndex)
		{
			rage::phPolygon &curPoly = nonConstPolygons[curPolyIndex];
			const rage::phPrimitive *curPrim = reinterpret_cast<const rage::phPrimitive *>(&curPoly);
			if(curPrim->GetType() == rage::PRIM_TYPE_POLYGON)
			{
				for(int curNeighborIndex = 0; curNeighborIndex < POLY_MAX_VERTICES; ++curNeighborIndex)
				{
					if(curPoly.GetNeighboringPolyNum(curNeighborIndex) != (rage::phPolygon::Index)(-1))
					{
						curPoly.SetNeighboringPolyNum(curNeighborIndex, oldToNew[curPoly.GetNeighboringPolyNum(curNeighborIndex)]);
					}
				}
			}
		}

		// newPolyIndex is the index of the polygon that we're currently trying to 'fix up', ie, make correct for the new mapping.
		for(int newPolyIndex = 0; newPolyIndex < numPrimitives; ++newPolyIndex)
		{
			// Find who's supposed to be the new 'polygon #curPolyIndex' and swap with them.
			rage::phPolygon tempPoly = nonConstPolygons[newPolyIndex];
			// oldPolyIndex is the old index of the polygon whose new index is newPolyIndex.
			const int oldPolyIndex = newToOldPolygonMapping[newPolyIndex].m_PrimitiveIndex;
			Assert(oldPolyIndex >= newPolyIndex);
			Assert(oldPolyIndex < numPrimitives);
			nonConstPolygons[newPolyIndex] = nonConstPolygons[oldPolyIndex];
			nonConstPolygons[oldPolyIndex] = tempPoly;

			int tempMtlIndex = GetPolygonMaterialIndex(newPolyIndex);
			SetPolygonMaterialIndex(newPolyIndex, GetPolygonMaterialIndex(oldPolyIndex));
			SetPolygonMaterialIndex(oldPolyIndex, tempMtlIndex);

			// Let's find out which old polygon used to map to the polygon that we just displaced, and give it its new location
			rage::SwapEm(newToOldPolygonMapping[newPolyIndex], newToOldPolygonMapping[oldToNew[newPolyIndex]]);
			rage::SwapEm(oldToNew[newPolyIndex], oldToNew[oldPolyIndex]);

#if 0
			// WARNING!!! This only work if the total number of polygons is 32768 or fewer (due to the fixed-size array below).
			{
				// Testing to verify consistency.
				bool oldPolys[32768], newPolys[32768];
				for(int testPolyIndex = 0; testPolyIndex < numPrimitives; ++testPolyIndex)
				{
					oldPolys[testPolyIndex] = false;
					newPolys[testPolyIndex] = false;
				}
				for(int testPolyIndex = 0; testPolyIndex < numPrimitives; ++testPolyIndex)
				{

					// Check for consistency.
					Assert(oldToNew[newToOldPolygonMapping[testPolyIndex].m_PrimitiveIndex] == (rage::phPolygon::Index)(testPolyIndex));
					Assert(newToOldPolygonMapping[oldToNew[testPolyIndex]].m_PrimitiveIndex == (rage::phPolygon::Index)(testPolyIndex));

					// Verify no duplicates.
					Assert(!oldPolys[newToOldPolygonMapping[testPolyIndex].m_PrimitiveIndex]);
					Assert(!newPolys[oldToNew[testPolyIndex]]);

					// Mark as covered.
					oldPolys[newToOldPolygonMapping[testPolyIndex].m_PrimitiveIndex] = true;
					newPolys[oldToNew[testPolyIndex]] = true;
				}
			}
#endif
		}

		rage::sysMemStartTemp();
		delete [] oldToNew;
		rage::sysMemEndTemp();
	}
#endif // !__SPU


#if TEST_BVH_CULLING
    // Dumb little function for testing whether the culling performed by the BVH is missing any polygons or not.  It's not actually called from anywhere
    //   right now.  For my own testing I called it a bunch of times from within CullOBBPolys but that may or may not suit you.
    // Update: I put a call to this in phBoundBVH::CullOBBPolys().
    void TestBoundBVH(const phBoundBVH *boundToTest)
    {
        class phBoundBVHNodeOverlapCallback
        {
        public:
            phBoundBVHNodeOverlapCallback(const phOptimizedBvh* bvh) : m_BVH(bvh)
            {
                m_LastPolygonIndex = -1;
            }

            virtual ~phBoundBVHNodeOverlapCallback() { }

            void SetPolyList(u16 *polyList, u16 *polyCounter)
            {
                m_PolyList = polyList;
                m_PolyCounter = polyCounter;
            }

            void SetAABB(const Vector3 &aabbMin, const Vector3 &aabbMax)
            {
                m_AABBMin.Set(aabbMin);
                m_AABBMax.Set(aabbMax);
                m_BVH->QuantizeWithClamp(m_AABBMinQuant, aabbMin);
                m_BVH->QuantizeWithClamp(m_AABBMaxQuant, aabbMax);
            }

            static bool TestAabbAgainstAabb2(const rage::u16 boxMin0[3], const rage::u16 boxMax0[3], const rage::u16 boxMin1[3], const rage::u16 boxMax1[3])
            {
                bool xOverlap = (boxMin0[0] <= boxMax1[0]) && (boxMin1[0] <= boxMax0[0]);
                bool yOverlap = (boxMin0[1] <= boxMax1[1]) && (boxMin1[1] <= boxMax0[1]);
                bool zOverlap = (boxMin0[2] <= boxMax1[2]) && (boxMin1[2] <= boxMax0[2]);

                bool aabbOverlap = xOverlap && yOverlap && zOverlap;
                return aabbOverlap;
            }

            bool TestAgainstSubtree(const u16 aabbMin[3], const u16 aabbMax[3])
            {
                return TestAabbAgainstAabb2(aabbMin, aabbMax, m_AABBMinQuant, m_AABBMaxQuant);
            }

            u32 ProcessNode(const phOptimizedBvhNode* node)
            {
                if (TestAabbAgainstAabb2(m_AABBMinQuant, m_AABBMaxQuant, node->m_AABBMin, node->m_AABBMax) == false)
                {
                    return 0x00000000;
                }

                Assert(!node->IsLeafNode() || (s32)(node->GetPolygonStartIndex()) > m_LastPolygonIndex);
                for (int polyInNodeIndex = 0; polyInNodeIndex < node->GetPolygonCount(); ++polyInNodeIndex)
                {
                    m_LastPolygonIndex = (s32)(node->GetPolygonStartIndex() + polyInNodeIndex);

                    m_PolyList[*m_PolyCounter] = (u16)(node->GetPolygonStartIndex() + polyInNodeIndex);
                    ++(*m_PolyCounter);
                }

                return 0xffffffff;
            }

        private:
            u16 *m_PolyList;
            u16 *m_PolyCounter;
            s32 m_LastPolygonIndex;
            const phOptimizedBvh* m_BVH;
            Vector3 m_AABBMin, m_AABBMax;
            u16 m_AABBMinQuant[3], m_AABBMaxQuant[3];
        };

        Vector3 aabbMin, aabbMax;
        for(int axis = 0; axis < 3; ++axis)
        {
            const float float0 = (boundToTest->GetBoundingBoxMax()[axis] - boundToTest->GetBoundingBoxMin()[axis]) * g_DrawRand.GetFloat() + boundToTest->GetBoundingBoxMin()[axis];
            const float float1 = (boundToTest->GetBoundingBoxMax()[axis] - boundToTest->GetBoundingBoxMin()[axis]) * g_DrawRand.GetFloat() + boundToTest->GetBoundingBoxMin()[axis];

            aabbMin[axis] = Min(float0, float1);
            aabbMax[axis] = Max(float0, float1);
        }

        u16 polygonList[10000];
        u16 polysInList = 0;

        phBoundBVHNodeOverlapCallback nodeOverlapCallback(boundToTest->GetBVH());

        nodeOverlapCallback.SetPolyList(polygonList, &polysInList);
        nodeOverlapCallback.SetAABB(aabbMin, aabbMax);
        boundToTest->GetBVH()->walkStacklessTree(&nodeOverlapCallback);

        // Now,
        Vector3 polyAABBMin, polyAABBMax;
        for(int curPolyIndex = 0; curPolyIndex < boundToTest->GetNumPolygons(); ++curPolyIndex)
        {
            polyAABBMin.Set(+FLT_MAX, +FLT_MAX, +FLT_MAX);
            polyAABBMax.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            const phPolygon &curPoly = boundToTest->GetPolygon(curPolyIndex);
            for(int curVertInPolyIndex = 0; curVertInPolyIndex < curPoly.GetNumVerts(); ++curVertInPolyIndex)
            {
                const phPolygon::Index curVertInBoundIndex = curPoly.GetVertexIndex(curVertInPolyIndex);
                const Vector3 &vertex = boundToTest->GetVertex(curVertInBoundIndex);

                polyAABBMin.Min(polyAABBMin, vertex);
                polyAABBMax.Max(polyAABBMax, vertex);
            }

            // Now we've got the AABB for this polygon, let's see if that AABB intersects the AABB used in the cull above.
            const bool xOverlap = (polyAABBMin.x < aabbMax.x) && (aabbMin.x < polyAABBMax.x);
            const bool yOverlap = (polyAABBMin.y < aabbMax.y) && (aabbMin.y < polyAABBMax.y);
            const bool zOverlap = (polyAABBMin.z < aabbMax.z) && (aabbMin.z < polyAABBMax.z);

            bool aabbOverlap = xOverlap && yOverlap && zOverlap;
            if(aabbOverlap)
            {
                // Search through the list returned by the cull and make sure that it's there!
                int listIndex;
                for(listIndex = 0; listIndex < polysInList; ++listIndex)
                {
                    if(polygonList[listIndex] == curPolyIndex)
                    {
                        break;
                    }
                }
                Assert(listIndex != polysInList);
            }
        }
    }
#endif

	// Note that this class is similar to, but different than, the phBvhAabbOverlapCallback class in cullerhelpers.h in that this class take a phBoundBVH to
	//   process and, when not used on SPU, will also apply the AABB cull to the triangular primitives in the BVH nodes.
    class phBvhAabbOverlapCallbackTestTris
    {
    public:
        phBvhAabbOverlapCallbackTestTris(const phBoundBVH *boundBvh) : m_boundBvh(boundBvh) {};

        void SetCuller(phBoundCuller *culler)
        {
            m_culler = culler;
        }

        void SetAABB(const Vector3 &aabbMin, const Vector3 &aabbMax)
        {
            m_AABBMin.Set(aabbMin);
            m_AABBMax.Set(aabbMax);
			m_boundBvh->GetBVH()->QuantizeMin(m_AABBMinQuant, aabbMin);
			m_boundBvh->GetBVH()->QuantizeMax(m_AABBMaxQuant, aabbMax);
        }

        static bool TestAabbAgainstAabb2(const rage::s16 boxMin0[3], const rage::s16 boxMax0[3], const rage::s16 boxMin1[3], const rage::s16 boxMax1[3])
        {
            bool xOverlap = (boxMin0[0] <= boxMax1[0]) && (boxMin1[0] <= boxMax0[0]);
            bool yOverlap = (boxMin0[1] <= boxMax1[1]) && (boxMin1[1] <= boxMax0[1]);
            bool zOverlap = (boxMin0[2] <= boxMax1[2]) && (boxMin1[2] <= boxMax0[2]);

            bool aabbOverlap = xOverlap && yOverlap && zOverlap;
            return aabbOverlap;
        }

        bool TestAgainstSubtree(const s16 aabbMin[3], const s16 aabbMax[3])
        {
            return TestAabbAgainstAabb2(aabbMin, aabbMax, m_AABBMinQuant, m_AABBMaxQuant);
        }

        u32 ProcessNode(const phOptimizedBvhNode* node)
        {
            if (TestAabbAgainstAabb2(m_AABBMinQuant, m_AABBMaxQuant, node->m_AABBMin, node->m_AABBMax) == false)
            {
                return 0x00000000;
            }

            for (int polyInNodeIndex = 0; polyInNodeIndex < node->GetPolygonCount(); ++polyInNodeIndex)
            {
                const int curPolygonIndex = node->GetPolygonStartIndex() + polyInNodeIndex;
#if /*POLYS_PER_NODE != 1 && */!__SPU
                if(TestPrimitive(curPolygonIndex))
#endif
                {
                    m_culler->AddCulledPolygonIndex((u16)(curPolygonIndex));
                }
            }

            return 0xffffffff;
        }

#if !__SPU
		// We can't really do these tests on the SPU because we don't have the vertices or polygons DMA'd locally at this point.
        bool TestPrimitive(const int polygonIndex) const
        {
            Vector3 minimum(VEC3_ZERO), maximum(VEC3_ZERO);
			Vector3 aabbMin(m_AABBMin), aabbMax(m_AABBMax);
            //const phPolygon &curPolygon = m_polygons[polygonIndex];
			const phPrimitive &curPrim = m_boundBvh->GetPrimitive(polygonIndex);
			if(curPrim.GetType() == PRIM_TYPE_POLYGON)
			{
				const phPolygon &curPolygon = curPrim.GetPolygon();
				for(int vertInPolyIndex = POLY_MAX_VERTICES - 1; vertInPolyIndex >= 0; --vertInPolyIndex)
				{
					const int vertInBoundIndex = curPolygon.GetVertexIndex(vertInPolyIndex);
#if COMPRESSED_VERTEX_METHOD > 0
	                Vector3 curVertex = VEC3V_TO_VECTOR3(m_boundBvh->GetCompressedVertex(vertInBoundIndex));
#else
					Vector3 curVertex = VEC3V_TO_VECTOR3(m_boundBvh->GetVertex(vertInBoundIndex));
#endif
					minimum.Or(curVertex.IsGreaterThanV(aabbMin));
					maximum.Or(curVertex.IsLessThanV(aabbMax));
				}

	            return minimum.IsTrueTrueTrue() && maximum.IsTrueTrueTrue();
			}
			else
			{
				return true;
			}
        }
#endif

    private:
        phBoundCuller *m_culler;
		const phBoundBVH* m_boundBvh;

        Vector3 m_AABBMin, m_AABBMax;
        s16 m_AABBMinQuant[3], m_AABBMaxQuant[3];
    };

	void phBoundBVH::CullSpherePolys (phBoundCuller& culler, Vec3V_In sphereCenter, ScalarV_In sphereRadius) const
	{
		// Make sure the culler has arrays for culled polygon and vertex indices.
		AssertMsg(culler.HasArrays(),"Ignorable - Call AllocateArrays or SetArrays on the bound culler before calling CullSpherePolys. See phShapeTest<ShapeType>::TestInLevel for an example.");
		CullOBBPolys(culler, Mat34V(Mat33V(V_IDENTITY),sphereCenter), Vec3V(sphereRadius));
	}

#if BOUNDBVH_REAL_OBB_CULLING
	class phBvhObbOverlapCallback
	{
	public:
		phBvhObbOverlapCallback(const phOptimizedBvh *bvh) : m_BVH(bvh) {};

		void SetCuller(phBoundCuller *culler)
		{
			m_culler = culler;
		}

		void SetVertexPointer(const Vector3 *vertices)
		{
			m_vertices = vertices;
		}

		void SetPolygonPointer(const phPolygon *polygons)
		{
			m_polygons = polygons;
		}

		void SetOBB(const Matrix34 &boxMatrix, const Vector3 &boxHalfExtents)
		{
			m_BoxMatrix.Set(boxMatrix);
			m_HalfWidths.Set(boxHalfExtents);

			// Construct a quantized AABB from the OBB provided.
			rage::Vector3 halfWidth = VEC3V_TO_VECTOR3(rage::geomBoxes::ComputeAABBExtentsFromOBB(RCC_MAT33V(boxMatrix), RCC_VEC3V(boxHalfExtents)));
			m_AABBMin.Subtract(boxMatrix.d, halfWidth);
			m_AABBMax.Add(boxMatrix.d, halfWidth);
			m_BVH->QuantizeWithClamp(m_AABBMinQuant, m_AABBMin);
			m_BVH->QuantizeWithClamp(m_AABBMaxQuant, m_AABBMax);
		}

		static bool TestAabbAgainstAabb2(const rage::u16 boxMin0[3], const rage::u16 boxMax0[3], const rage::u16 boxMin1[3], const rage::u16 boxMax1[3])
		{
#if 1
			bool xOverlap = (boxMin0[0] <= boxMax1[0]) && (boxMin1[0] <= boxMax0[0]);
			bool yOverlap = (boxMin0[1] <= boxMax1[1]) && (boxMin1[1] <= boxMax0[1]);
			bool zOverlap = (boxMin0[2] <= boxMax1[2]) && (boxMin1[2] <= boxMax0[2]);

			bool aabbOverlap = xOverlap && yOverlap && zOverlap;
			return aabbOverlap;
#else
			return boxMin0[0] <= boxMax1[0] && boxMin1[0] <= boxMax0[0] && boxMin0[1] <= boxMax1[1] && boxMin1[1] <= boxMax0[1] && boxMin0[2] <= boxMax1[2] && boxMin1[2] <= boxMax0[2];
#endif
		}

		bool TestAgainstObb(const rage::u16 boxMin[3], const rage::u16 boxMax[3]) const
		{
			rage::Vector3 aabbMin, aabbMax;
			m_BVH->UnQuantize(aabbMin, boxMin);
			//nodeAabbMin.Subtract(m_BVH->GetInvQuantize());
			m_BVH->UnQuantize(aabbMax, boxMax);
			//nodeAabbMax.Add(m_BVH->GetInvQuantize());

			Matrix34 tempBoxMatrix(m_BoxMatrix);
			tempBoxMatrix.d.SubtractScaled(aabbMin, 0.5f);
			tempBoxMatrix.d.SubtractScaled(aabbMax, 0.5f);
			return rage::geomBoxes::TestBoxToBoxOBBFaces(m_HalfWidths, 0.5f * (aabbMax - aabbMin), tempBoxMatrix);
		}

		bool TestAgainstSubtree(const u16 aabbMin[3], const u16 aabbMax[3])
		{
			return TestAabbAgainstAabb2(aabbMin, aabbMax, m_AABBMinQuant, m_AABBMaxQuant) && TestAgainstObb(aabbMin, aabbMax);
		}

		u32 ProcessNode(const phOptimizedBvhNode* node)
		{
			if(!TestAabbAgainstAabb2(m_AABBMinQuant, m_AABBMaxQuant, node->m_AABBMin, node->m_AABBMax) || !TestAgainstObb(node->m_AABBMin, node->m_AABBMax))
			{
				return 0x00000000;
			}

			for (int polyInNodeIndex = 0; polyInNodeIndex < node->GetPolygonCount(); ++polyInNodeIndex)
			{
				const int curPolygonIndex = node->GetPolygonStartIndex() + polyInNodeIndex;
//#if POLYS_PER_NODE != 1
				if(TestPolygon(curPolygonIndex))
//#endif
				{
					m_culler->AddCulledPolygonIndex((u16)(curPolygonIndex));
				}
			}

			return 0xffffffff;
		}

		bool TestPolygon(const int polygonIndex) const
		{
			bool xMin = false, xMax = false, yMin = false, yMax = false, zMin = false, zMax = false;
			const phPolygon &curPolygon = m_polygons[polygonIndex];
			for(int vertInPolyIndex = curPolygon.GetNumVerts() - 1; vertInPolyIndex >= 0; --vertInPolyIndex)
			{
				const int vertInBoundIndex = curPolygon.GetVertexIndex(vertInPolyIndex);
				const Vector3 &curVertex = m_vertices[vertInBoundIndex];
				xMin = xMin || curVertex.x > m_AABBMin.x;
				xMax = xMax || curVertex.x < m_AABBMax.x;
				yMin = yMin || curVertex.y > m_AABBMin.y;
				yMax = yMax || curVertex.y < m_AABBMax.y;
				zMin = zMin || curVertex.z > m_AABBMin.z;
				zMax = zMax || curVertex.z < m_AABBMax.z;
			}

			return xMin && xMax && yMin && yMax && zMin && zMax;
		}

	private:
		phBoundCuller *m_culler;
		const phOptimizedBvh *m_BVH;
		const Vector3 *m_vertices;
		const phPolygon *m_polygons;

		Matrix34 m_BoxMatrix;				// Goes from the local space of the box to the local space of this bound.
		Vector3 m_HalfWidths;
		Vector3 m_AABBMin, m_AABBMax;
		u16 m_AABBMinQuant[3], m_AABBMaxQuant[3];
	};
#endif

    void phBoundBVH::CullOBBPolys(phBoundCuller& culler, Mat34V_In boxMatrix, Vec3V_In boxHalfExtents) const
    {
		// Make sure the culler has arrays for culled polygon and vertex indices.
		AssertMsg(culler.HasArrays(),"Ignorable - Call AllocateArrays or SetArrays on the bound culler before calling CullOBBPolys. See phShapeTest<ShapeType>::TestInLevel for an example.");

#if TEST_BVH_CULLING
        TestBoundBVH(this);
#endif
#if DISABLE_CULLING_OBB
        // This is a dummy version that just adds every polygon.
        for(int curPolyIndex = 0; curPolyIndex < m_NumPolygons; ++curPolyIndex)
        {
            culler.AddCulledPolygonIndex((u16)(curPolyIndex));
        }

	#if __PFDRAW
        // Tell this bound about the results.
        m_NumActivePolygons = culler.GetNumCulledPolygons();
        m_ActivePolygonIndices = culler.GetCulledPolygonIndexList();
	#endif

#else
#if !BOUNDBVH_REAL_OBB_CULLING
		// This is the old version that just used the AABB around the OBB.
        // We're given an OBB but we need to use an AABB, so we'll find the AABB enclosing this OBB.
        Vec3V aabbHalfWidth;
        Vec3V obbX(boxMatrix.GetCol0()), obbY(boxMatrix.GetCol1()), obbZ(boxMatrix.GetCol2());
#if 1
        Vec3V scale(boxHalfExtents);
        scale = Add(scale, Vec3V(GetMarginV()));
        obbX = Scale(obbX, Vec3V(SplatX(scale)));
		obbY = Scale(obbY, Vec3V(SplatY(scale)));
		obbZ = Scale(obbZ, Vec3V(SplatZ(scale)));
		Vec3V absObbX = Abs( obbX );
		Vec3V absObbY = Abs( obbY );
		Vec3V absObbZ = Abs( obbZ );
		aabbHalfWidth = Add( Add( absObbX, absObbY ), absObbZ );
#else
		// This code produces somewhat smaller AABBs from OBBs (it adds the margin to the AABB at the end rather than to the OBB at the beginning, the
		//   difference being larger the farther from the identity matrix that culler.GetBoxData() is) but, for some reason, results in *slightly* slower
		//   execution, even though the code seems somewhat simpler to me.  That's probably due to some bad mixture of vector and scalar math so I left it
		//   in here for reference.
		obbX.Scale(culler.GetBoxData().d.x);
		obbY.Scale(culler.GetBoxData().d.y);
		obbZ.Scale(culler.GetBoxData().d.z);
		aabbHalfWidth.Set(fabs(obbX.x) + fabs(obbY.x) + fabs(obbZ.x), fabs(obbX.y) + fabs(obbY.y) + fabs(obbZ.y), fabs(obbX.z) + fabs(obbY.z) + fabs(obbZ.z));
#if __SPU
		//aabbHalfWidth.Add(m_MarginV);
#else
		//aabbHalfWidth.Add(GetMarginV());
#endif
#endif

		Vec3V aabbMin(boxMatrix.GetCol3()), aabbMax(boxMatrix.GetCol3());
		aabbMin = Subtract(aabbMin, aabbHalfWidth);
		aabbMax = Add(aabbMax, aabbHalfWidth);

		// Walk the tree using this AABB to cull.
		phBvhAabbOverlapCallbackTestTris nodeOverlapCallback(this);
		nodeOverlapCallback.SetCuller(&culler);

		nodeOverlapCallback.SetAABB(RCC_VECTOR3(aabbMin), RCC_VECTOR3(aabbMax));
#else
		// Walk the tree using this AABB to cull.
		phBvhObbOverlapCallback nodeOverlapCallback(m_BVH);
		nodeOverlapCallback.SetCuller(&culler);
		nodeOverlapCallback.SetVertexPointer(GetVertexPointer());
		nodeOverlapCallback.SetPolygonPointer(GetPolygonPointer());
		nodeOverlapCallback.SetOBB(RCC_MATRIX34(boxMatrix), RCC_VECTOR3(boxHalfExtents));
#endif
		m_BVH->walkStacklessTree(&nodeOverlapCallback);

	#if __PFDRAW
		// Tell this bound about the results.
		m_NumActivePolygons = culler.GetNumCulledPolygons();
		m_ActivePolygonIndices = culler.GetCulledPolygonIndexList();
	#endif

#endif
	}


	void phBoundBVH::CullLineSegPolys (Vector3::Vector3Param seg0, Vector3::Vector3Param seg1, phBoundCuller& culler) const
	{
		// Make sure the culler has arrays for culled polygon and vertex indices.
		AssertMsg(culler.HasArrays(),"Ignorable - Call AllocateArrays or SetArrays on the bound culler before calling CullLineSegPolys. See phShapeTest<ShapeType>::TestInLevel for an example.");

		// Walk the tree using the probe as a line segment against which to cull.
		phBvhLineSegmentOverlapCallback nodeOverlapCallback(m_BVH);
		nodeOverlapCallback.SetCuller(&culler);
        nodeOverlapCallback.SetSegment(Vec3V(seg0), Vec3V(seg1));

		m_BVH->walkStacklessTree(&nodeOverlapCallback);

	#if __PFDRAW
		// Tell this bound about the results.
		m_NumActivePolygons = culler.GetNumCulledPolygons();
		m_ActivePolygonIndices = culler.GetCulledPolygonIndexList();
	#endif
	}


	void phBoundBVH::CullCapsulePolys (Vector3::Vector3Param seg0, Vector3::Vector3Param seg1, const float capsuleRadius, phBoundCuller& culler) const
	{
		// Make sure the culler has arrays for culled polygon and vertex indices.
		AssertMsg(culler.HasArrays(),"Ignorable - Call AllocateArrays or SetArrays on the bound culler before calling CullCapsulePolys. See phShapeTest<ShapeType>::TestInLevel for an example.");

		// Walk the tree using the probe as a line segment against which to cull.
		phBvhCapsuleOverlapCallback nodeOverlapCallback(m_BVH);
		nodeOverlapCallback.SetCuller(&culler);
		nodeOverlapCallback.SetCapsule(RCC_VEC3V(seg0), RCC_VEC3V(seg1), ScalarVFromF32(capsuleRadius));
		m_BVH->walkStacklessTree(&nodeOverlapCallback);

	#if __PFDRAW
		// Tell this bound about the results.
		m_NumActivePolygons = culler.GetNumCulledPolygons();
		m_ActivePolygonIndices = culler.GetCulledPolygonIndexList();
	#endif

	}


#if !__FINAL && !IS_CONSOLE
	bool phBoundBVH::Save_v110 (fiAsciiTokenizer &token)
	{
		return phBoundGeometry::Save_v110(token);
	}

#endif	// end of #if !__FINAL && !IS_CONSOLE


	////////////////////////////////////////////////////////////////////////////////////
	// collision and test routines
	// see header for implementation information

#if !__SPU
	////////////////////////////////////////////////////////////////
	// resources

	phBoundBVH::phBoundBVH (datResource & rsc)
		: phBoundGeometry(rsc)
		, m_ActivePolygonIndices(NULL)
		, m_NumActivePolygons((u16)(-1))
	{
		sysMemUseMemoryBucket bucket(sm_MemoryBucket);
		SetMargin(CONCAVE_DISTANCE_MARGIN);

		Assertf(GetNumPolygons() < 32768, "Bound '%s' has too many polygons (%d)!", rsc.GetDebugName(), GetNumPolygons());

		// Enable this to rotate the quads here at resource construction time.
#if 0
		RotateQuads();
#endif

#if BOUNDBVH_CLEAR_HIGH_TYPE_BIT_IN_PRIMITIVES
		phPolygon *polygons = const_cast<phPolygon *>(m_Polygons);
		const int numPrimitives = GetNumPolygons();
		for(int primIndex = 0; primIndex < numPrimitives; ++primIndex)
		{
			phPolygon &curPolygon = polygons[primIndex];
			BvhPrimitive &curPrimitive = *reinterpret_cast<BvhPrimitive *>(&curPolygon);
			Assert(!curPrimitive.CheckHighTypeBit());
			curPrimitive.ClearHighTypeBit();
		}
#endif

#if !__SPU && !__TOOL && !__FINAL
		bool spewBadPolys = PARAM_spewandfixbadbvhpolys.Get() || PARAM_spewbadbvhpolys.Get();
		bool fixBadPolys = PARAM_spewandfixbadbvhpolys.Get() || PARAM_fixbadbvhpolys.Get();
		if(spewBadPolys || fixBadPolys)
		{
			phPolygon *polygons = const_cast<phPolygon *>(m_Polygons);
			const int numPrimitives = GetNumPolygons();
			for(phPolygon::Index primIndex = 0; primIndex < numPrimitives; ++primIndex)
			{
				phPolygon &curPolygon = polygons[primIndex];

				// Also, going to fix the areas
				rage::phPrimitive *curPrim = reinterpret_cast<rage::phPrimitive *>(&curPolygon);
				if(curPrim->GetType() == rage::PRIM_TYPE_POLYGON)
				{
					const int triVertIndex0 = curPolygon.GetVertexIndex(0);
					const int triVertIndex1 = curPolygon.GetVertexIndex(1);
					const int triVertIndex2 = curPolygon.GetVertexIndex(2);
					Assert(triVertIndex0 < GetNumVertices());
					Assert(triVertIndex1 < GetNumVertices());
					Assert(triVertIndex2 < GetNumVertices());

					Vec3V triVert0 = GetBVHVertex(triVertIndex0);
					Vec3V triVert1 = GetBVHVertex(triVertIndex1);
					Vec3V triVert2 = GetBVHVertex(triVertIndex2);

					Vec3V side12,side10,nonUnitNormal;
					side12 = Subtract(triVert2, triVert1);
					side10 = Subtract(triVert0, triVert1);
					nonUnitNormal = Cross(side12, side10);

					float newArea = Scale(Mag(nonUnitNormal), ScalarV(V_HALF)).Getf();
					float oldArea = curPolygon.GetArea();

					float newnewArea = newArea;

					if (fixBadPolys)
					{
						curPolygon.SetArea(newArea);
						newnewArea = curPolygon.GetArea();
					}

					static float threshAreaDifPercentage = 0.01f;
					if (spewBadPolys && (abs(newArea - oldArea) / newArea) > threshAreaDifPercentage)
					{
						Displayf("Bound: %s  |  Prim: %d -- Poly Area problem: Old Area = %5.8f, New Area = %5.8f, newNew Area = %5.8f, vertex zero: << %f, %f, %f >>", rsc.GetDebugName(), primIndex, oldArea, newArea, newnewArea, triVert0.GetXf(), triVert0.GetYf(), triVert0.GetZf());
					}

					curPolygon.HasBadNeighbors(this, rsc.GetDebugName(), spewBadPolys, primIndex, fixBadPolys);
				}
			}
		}
#endif // !__SPU && !__TOOL && !__FINAL
	}

	IMPLEMENT_PLACE(phBoundBVH);

#if __DECLARESTRUCT
	void phBoundBVH::DeclareStruct(datTypeStruct &s)
	{
		phBoundGeometry::DeclareStruct(s);
		STRUCT_BEGIN(phBoundBVH);
		STRUCT_FIELD(m_BVH);
		STRUCT_IGNORE(m_ActivePolygonIndices);
		STRUCT_FIELD(m_NumActivePolygons);
		STRUCT_CONTAINED_ARRAY(m_Pad);
		STRUCT_END();
	}
#endif // __DECLARESTRUCT

#endif // !__SPU

	void phBoundBVH::ConstructBoundFromPrimitive(const phPrimBox& boxPrim, phBoundBox& boxBound, Mat34V_InOut boxMatrix) const
	{
		const int vertIndex0 = boxPrim.GetVertexIndex(0);
		const int vertIndex1 = boxPrim.GetVertexIndex(1);
		const int vertIndex2 = boxPrim.GetVertexIndex(2);
		const int vertIndex3 = boxPrim.GetVertexIndex(3);
		Vec3V vert0, vert1, vert2, vert3;
#if __SPU
#if COMPRESSED_VERTEX_METHOD == 0
		cellDmaLargeGet(&vert0, (uint64_t)(&GetVertexPointer()[vertIndex0]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vert1, (uint64_t)(&GetVertexPointer()[vertIndex1]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vert2, (uint64_t)(&GetVertexPointer()[vertIndex2]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vert3, (uint64_t)(&GetVertexPointer()[vertIndex3]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
#else // COMPRESSED_VERTEX_METHOD == 0
		const CompressedVertexType *compressedVertexPointer0 = &GetCompressedVertexPointer()[3 * vertIndex0];
		const CompressedVertexType *compressedVertexPointer1 = &GetCompressedVertexPointer()[3 * vertIndex1];
		const CompressedVertexType *compressedVertexPointer2 = &GetCompressedVertexPointer()[3 * vertIndex2];
		const CompressedVertexType *compressedVertexPointer3 = &GetCompressedVertexPointer()[3 * vertIndex3];
		const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer0) & ~15);
		const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer1) & ~15);
		const CompressedVertexType *ppuDMAAddress2 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer2) & ~15);
		const CompressedVertexType *ppuDMAAddress3 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer3) & ~15);
		u8 vertexBuffer0[32], vertexBuffer1[32], vertexBuffer2[32], vertexBuffer3[32];
		cellDmaLargeGet(&vertexBuffer0[0], (uint64_t)(ppuDMAAddress0), 32, DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vertexBuffer1[0], (uint64_t)(ppuDMAAddress1), 32, DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vertexBuffer2[0], (uint64_t)(ppuDMAAddress2), 32, DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vertexBuffer3[0], (uint64_t)(ppuDMAAddress3), 32, DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
		const int bufferOffset0 = (int)(compressedVertexPointer0) & 15;
		const int bufferOffset1 = (int)(compressedVertexPointer1) & 15;
		const int bufferOffset2 = (int)(compressedVertexPointer2) & 15;
		const int bufferOffset3 = (int)(compressedVertexPointer3) & 15;
		const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer0[bufferOffset0]);
		const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer1[bufferOffset1]);
		const CompressedVertexType *spuCompressedVector2 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer2[bufferOffset2]);
		const CompressedVertexType *spuCompressedVector3 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer3[bufferOffset3]);
		vert0 = DecompressVertex(spuCompressedVector0);
		vert1 = DecompressVertex(spuCompressedVector1);
		vert2 = DecompressVertex(spuCompressedVector2);
		vert3 = DecompressVertex(spuCompressedVector3);
#endif // COMPRESSED_VERTEX_METHOD == 0
#else // __SPU
		vert0 = GetVertex(vertIndex0);
		vert1 = GetVertex(vertIndex1);
		vert2 = GetVertex(vertIndex2);
		vert3 = GetVertex(vertIndex3);
#endif // __SPU

		Vec3V boxSize;
		ScalarV margin;
		geomBoxes::ComputeBoxDataFromOppositeDiagonals(vert0, vert1, vert2, vert3, boxMatrix, boxSize, margin);
		boxBound.SetBoxSize(boxSize);
		boxBound.SetMargin(margin);
	}

	void phBoundBVH::ConstructBoundFromPrimitive(const phPrimSphere& spherePrim, phBoundSphere& sphereBound, Mat34V_InOut sphereMatrix) const
	{
		const int sphereCenterIndex = spherePrim.GetCenterIndex();
		Vec3V sphereCenter;
#if __SPU
#if COMPRESSED_VERTEX_METHOD == 0
		cellDmaLargeGet(&sphereCenter, (uint64_t)(&GetVertexPointer()[sphereCenterIndex]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
#else // COMPRESSED_VERTEX_METHOD == 0
		const CompressedVertexType *compressedVertexPointer = &GetCompressedVertexPointer()[3 * sphereCenterIndex];
		// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the address.
		const CompressedVertexType *ppuDMAAddress = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer) & ~15);
		u8 vertexBuffer[32];
		cellDmaLargeGet(vertexBuffer, (uint64_t)(ppuDMAAddress), 32, DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));

		const int bufferOffset = (int)(compressedVertexPointer) & 15;
		const CompressedVertexType *spuCompressedVector = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer[bufferOffset]);

		sphereCenter = DecompressVertex(spuCompressedVector);
#endif // COMPRESSED_VERTEX_METHOD == 0
#else // __SPU
		sphereCenter = GetVertex(sphereCenterIndex);
#endif // __SPU

		sphereBound.SetSphereRadius(spherePrim.GetRadiusV());
		sphereMatrix = Mat34V(Mat33V(V_IDENTITY),sphereCenter);
	}

	void ComputeShaftData(Vec3V_In v0, Vec3V_In v1, Mat34V_InOut matrix, ScalarV_InOut shaftLength)
	{
		const Vec3V shaft(v1 - v0);
		shaftLength = MagFast(shaft);
		// Create a matrix to orient the capsule in the local space of the bound.
		// Check if the shaft axis is parallel to the y-axis.
		const VecBoolV maskY = VecBoolV(V_F_T_F_F);
		const Vec3V result = SelectFT(maskY, shaft, Vec3V(V_ZERO));
		if(!IsZeroAll(result))
		{
			matrix.SetCol0(Normalize(Cross(shaft, Vec3V(V_Y_AXIS_WONE))));
			matrix.SetCol1(Normalize(shaft));
			matrix.SetCol2(Cross(matrix.GetCol0(), matrix.GetCol1()));
		}
		else
		{
			matrix.Set3x3(Mat33V(V_IDENTITY));
		}
		matrix.SetCol3(Average(v0, v1));
	}

	void phBoundBVH::ConstructBoundFromPrimitive(const phPrimCapsule& capsulePrim, phBoundCapsule& capsuleBound, Mat34V_InOut capsuleMatrix) const
	{
		Vec3V capsuleEnd0, capsuleEnd1;
		const int endIndex0 = capsulePrim.GetEndIndex0();
		const int endIndex1 = capsulePrim.GetEndIndex1();
#if __SPU
#if COMPRESSED_VERTEX_METHOD == 0
		cellDmaLargeGet(&capsuleEnd0, (uint64_t)(&GetVertexPointer()[endIndex0]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&capsuleEnd1, (uint64_t)(&GetVertexPointer()[endIndex1]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
#else // COMPRESSED_VERTEX_METHOD == 0
		const CompressedVertexType *compressedVertexPointer0 = &GetCompressedVertexPointer()[3 * endIndex0];
		const CompressedVertexType *compressedVertexPointer1 = &GetCompressedVertexPointer()[3 * endIndex1];
		const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer0) & ~15);
		const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer1) & ~15);
		u8 vertexBuffer0[32], vertexBuffer1[32];
		cellDmaLargeGet(&vertexBuffer0[0], (uint64_t)(ppuDMAAddress0), 32, DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vertexBuffer1[0], (uint64_t)(ppuDMAAddress1), 32, DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
		const int bufferOffset0 = (int)(compressedVertexPointer0) & 15;
		const int bufferOffset1 = (int)(compressedVertexPointer1) & 15;
		const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer0[bufferOffset0]);
		const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer1[bufferOffset1]);
		capsuleEnd0 = DecompressVertex(spuCompressedVector0);
		capsuleEnd1 = DecompressVertex(spuCompressedVector1);
#endif // COMPRESSED_VERTEX_METHOD == 0
#else // __SPU
		capsuleEnd0 = GetVertex(endIndex0);
		capsuleEnd1 = GetVertex(endIndex1);
#endif // __SPU
		ScalarV capsuleLength;
		ComputeShaftData(capsuleEnd0,capsuleEnd1,capsuleMatrix,capsuleLength);
		capsuleBound.SetCapsuleSize(capsulePrim.GetRadiusV(), capsuleLength);
	}

	void phBoundBVH::ConstructBoundFromPrimitive(const phPrimCylinder& cylinderPrim, phBoundCylinder& cylinderBound, Mat34V_InOut cylinderMatrix) const
	{
		Vec3V cylinderEnd0, cylinderEnd1;
		const int endIndex0 = cylinderPrim.GetEndIndex0();
		const int endIndex1 = cylinderPrim.GetEndIndex1();
#if __SPU
#if COMPRESSED_VERTEX_METHOD == 0
		cellDmaLargeGet(&cylinderEnd0, (uint64_t)(&GetVertexPointer()[endIndex0]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&cylinderEnd1, (uint64_t)(&GetVertexPointer()[endIndex1]), sizeof(Vector3), DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
#else // COMPRESSED_VERTEX_METHOD == 0
		const CompressedVertexType *compressedVertexPointer0 = &GetCompressedVertexPointer()[3 * endIndex0];
		const CompressedVertexType *compressedVertexPointer1 = &GetCompressedVertexPointer()[3 * endIndex1];
		const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer0) & ~15);
		const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer1) & ~15);
		u8 vertexBuffer0[32], vertexBuffer1[32];
		cellDmaLargeGet(&vertexBuffer0[0], (uint64_t)(ppuDMAAddress0), 32, DMA_TAG(16), 0, 0);
		cellDmaLargeGet(&vertexBuffer1[0], (uint64_t)(ppuDMAAddress1), 32, DMA_TAG(16), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(16));
		const int bufferOffset0 = (int)(compressedVertexPointer0) & 15;
		const int bufferOffset1 = (int)(compressedVertexPointer1) & 15;
		const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer0[bufferOffset0]);
		const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer1[bufferOffset1]);
		cylinderEnd0 = DecompressVertex(spuCompressedVector0);
		cylinderEnd1 = DecompressVertex(spuCompressedVector1);
#endif // COMPRESSED_VERTEX_METHOD == 0
#else // __SPU
		cylinderEnd0 = GetVertex(endIndex0);
		cylinderEnd1 = GetVertex(endIndex1);
#endif // __SPU
		ScalarV cylinderLength;
		ComputeShaftData(cylinderEnd0,cylinderEnd1,cylinderMatrix,cylinderLength);
		cylinderBound.SetCylinderRadiusAndHalfHeight(cylinderPrim.GetRadiusV(), Scale(cylinderLength,ScalarV(V_HALF)));
		cylinderBound.SetMargin(GetMarginV());
	}

} // namespace rage
