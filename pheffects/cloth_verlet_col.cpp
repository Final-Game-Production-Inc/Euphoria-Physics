// 
// pheffects/cloth_verlet_col.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 


#include "cloth_verlet.h"

#include "morphgeometry.h"

#include "data/callback.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundgeom.h"
#include "phbound/boundplane.h"
#include "phbound/boundsphere.h"
#include "phbound/boundbox.h"
#include "phbound/boundtaperedcapsule.h"
#include "phbound/boundbvh.h"
#include "phbound/OptimizedBvh.h"
#include "phbound/support.h"
#include "phbound/boundribbon.h"
#include "phbound/boundsurface.h"
#include "phbound/boundgrid.h"

#include "phbullet/TriangleShape.h"

#include "phcore/segment.h"
#include "physics/collider.h"
#include "physics/collision.h"
#include "physics/overlappingpairarray.h"
#include "physics/shapetest.h"
#include "physics/simulator.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "vector/geometry.h"
#include "vectormath/classes.h"
#include "cloth_verlet_spu.h"


#if __WIN32
#pragma warning(disable:4100)
#endif

extern rage::u8*     g_instLvlIdxToMtxAddrMM;
extern rage::Mat34V* g_instLastMtxAddrMM;

//#pragma optimize("", off)

#define DRAW_ALL_COLLISION_BOUNDS		( __PFDRAW && 1 )
#define DRAW_EDGE_COLLISION				( __PFDRAW && DRAW_ALL_COLLISION_BOUNDS && 1 )

namespace rage {

EXT_PFD_DECLARE_ITEM(VerletCollision);
EXT_PFD_DECLARE_ITEM(VerletCollisionResponse);

#define		TRIANGLE_MARGIN					0.04f

extern bool PointOnTriangle(Vec3V_In p, Vec3V_In _a, Vec3V_In _b, Vec3V_In _c);


#if __SPU
const u32 g_BoundSizeTable[phBound::NUM_BOUND_TYPES] = {
#undef BOUND_TYPE_INC
#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) sizeof(className),
#include "phbound/boundtypes.inc"
#undef BOUND_TYPE_INC
};
#endif


namespace phClothStats
{
	EXT_PF_TIMER(ClothCollisions);
	EXT_PF_TIMER(ClothCollisionsSphere);
	EXT_PF_TIMER(ClothCollisionsCapsule);
	EXT_PF_TIMER(ClothCollisionsTaperedCapsule);
	EXT_PF_TIMER(ClothCollisionsGeometry);
};

using namespace phClothStats;

#if POLYGON_INDEX_IS_U32
u16 Index_to_u16(phPolygon::Index i) 
{
	Assert(i <= 65535);
	return (u16) i;
}
#else
#define Index_to_u16(x) x
#endif

/*ScalarV_Out ClosestPtSegmentSegment( Vec3V_In p1, Vec3V_In q1, Vec3V_In p2, Vec3V_In q2, ScalarV_InOut s, ScalarV_InOut t, Vec3V_InOut c1, Vec3V_InOut c2 )
{
	Vec3V d1 = Subtract( q1, p1 );
	Vec3V d2 = Subtract( q2, p2 );
	Vec3V r  = Subtract( p1, p2 );
	ScalarV a = Dot( d1, d1 );
	ScalarV e = Dot( d2, d2 );
	ScalarV f = Dot( d2, r );

	ScalarV _eps(V_FLT_EPSILON);
	ScalarV _zero(V_ZERO);
	ScalarV _one(V_ONE);
	if( IsLessThanOrEqualAll(a, _eps) != 0 && IsLessThanOrEqualAll(e, _eps) != 0 )
	{
		s = t = _zero;
		c1 = p1;
		c2 = p2;
		return Dot( Subtract(c1,c2), Subtract(c1,c2) );
	}

	if( IsLessThanOrEqualAll(a, _eps) != 0  )
	{
		s = _zero;
		t = Scale(f,Invert(e));
		t = Clamp(t,_zero,_one);
	}
	else
	{
		ScalarV c = Dot( d1, r );
		if( IsLessThanOrEqualAll(e, _eps) != 0  )
		{
			t = _zero;
			s = Clamp( Scale(Negate(c),Invert(a)), _zero, _one );
		}
		else
		{
			ScalarV b = Dot( d1, d2 );
			ScalarV denom = Subtract(Scale(a,e), Scale(b,b));
			if( IsEqualAll(denom, _zero) == 0  )
			{
				s = Clamp( Scale(Subtract(Scale(b,f), Scale(c,e)), Invert(denom)), _zero, _one );
			}
			else
			{
				s = _zero;
			}
			t = Scale(AddScaled(f,b,s),Invert(e));
			if( IsLessThanAll(t, _zero) != 0  )
			{
				t = _zero;
				s = Clamp(Scale(Negate(c),Invert(a)), _zero, _one);
			}
			else if( IsGreaterThanAll(t, _one) != 0 )
			{
				t = _one;
				s = Clamp(Scale(Subtract(b,c),Invert(a)), _zero, _one);
			}
		}
	}

	c1 = AddScaled(p1, d1, s);
	c2 = AddScaled(p2, d2, t);
	return Dot(Subtract(c1, c2), Subtract(c1, c2));
}*/


#if __SPU
Vec3V_Out GetCompressedVertexSPU( const phBoundGeometry* bound, int idx )
{
	const CompressedVertexType* pVerts = bound->GetCompressedVertexPointer();
	const CompressedVertexType* compressedVertexPointer = &pVerts[3 * idx];

	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the address.
	const CompressedVertexType* ppuDMAAddress = reinterpret_cast<const CompressedVertexType*>((int)(compressedVertexPointer) & ~15);

	u8 vertexBuffer[32];
	cellDmaLargeGet(vertexBuffer, (uint64_t)(ppuDMAAddress), 32, DMA_TAG(16), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(16));

	const int bufferOffset = (int)(compressedVertexPointer) & 15;	
	const CompressedVertexType* spuCompressedVector = reinterpret_cast<const CompressedVertexType*>(&vertexBuffer[bufferOffset]);

	return bound->DecompressVertex(spuCompressedVector);
}
#endif

#define MAX_CULLED_PRIMITIVES		64


void phVerletCloth::CullPrimitives(const phBound* RESTRICT pBound, Mat34V_In boundPose, Vec3V* RESTRICT pClothVerts, Vec3V* RESTRICT pPrevClothVerts, Vec3V_In parentOffset)
{
	Assert( pBound );
	Assert( pClothVerts );
	Assert( pPrevClothVerts );

	phBoundCuller culler;			
	const int maxCulledPrimitives = MAX_CULLED_PRIMITIVES;
	Mat34V matIdentity(V_IDENTITY);

#if __SPU
	phPolygon::Index* culledPolyList = sysScratchAllocObj<phPolygon::Index>(maxCulledPrimitives);
	phPolygon* polygon = sysScratchAllocObj<phPolygon>(1);	

	phBoundBVH* bvhBound = (phBoundBVH*)(pBound);
	u8 bvhStructureBuffer[sizeof(phOptimizedBvh)];
	const phOptimizedBvh *ppuAddress = bvhBound->GetBVH();
	Assert(((int)(ppuAddress) & 15) == 0);
	Assert(((int)(&bvhStructureBuffer[0]) & 15) == 0);

	sysDmaLargeGetAndWait(bvhStructureBuffer, (uint64_t)(ppuAddress), sizeof(phOptimizedBvh), DMA_TAG(15));
	bvhBound->SetBVH(reinterpret_cast<phOptimizedBvh *>(bvhStructureBuffer));
#else
	phPolygon::Index culledPolyList[maxCulledPrimitives];
	const phBoundBVH* RESTRICT bvhBound = static_cast<const phBoundBVH*>(pBound);
#endif

	culler.SetArrays(culledPolyList,maxCulledPrimitives);

	const int beginEdge = GetNumLockedEdgesFront();
	const int endEdge = GetNumVertices() - GetNumLockedEdgesBack()-1;

	for( int edgeIdx = beginEdge; edgeIdx < endEdge; ++edgeIdx )
	{				
		const phEdgeData& verletEdge = GetEdge( edgeIdx );
		const int idx0 = verletEdge.m_vertIndices[0];
		const int idx1 = verletEdge.m_vertIndices[1];
		Vec3V p1 = pClothVerts[idx0];
		Vec3V p2 = pClothVerts[idx1];
		Vec3V clothWorldCentroid = Scale(Add(p1,p2), ScalarV(V_HALF));
		ScalarV boundRadius = Scale(Mag( Subtract(p1, p2) ),ScalarV(V_HALF));
//		float boundRadius = verletEdge.m_EdgeLength2 * 0.25f;


		culler.Reset();
		bvhBound->CullSpherePolys(culler,clothWorldCentroid,boundRadius);
		int numCulledPolygons = culler.GetNumCulledPolygons();
		for (int culledPolygonIndex=0; culledPolygonIndex<numCulledPolygons; culledPolygonIndex++)				
		{
			phPolygon::Index polygonIndex = culler.GetCulledPolygonIndex(culledPolygonIndex);


			const phPolygon* ppuPolygonAddress = ((const phPolygon*)bvhBound->GetPolygonPointer() + polygonIndex);
#if __SPU			
			sysDmaLargeGetAndWait(polygon, (uint64_t)(ppuPolygonAddress), sizeof(phPolygon), 0);
#else
			const phPolygon* polygon = const_cast<phPolygon*>(ppuPolygonAddress);
#endif
			Assert( polygon );
			const phPrimitive* bvhPrimitive = &polygon->GetPrimitive();
			switch( bvhPrimitive->GetType() )
			{
				case PRIM_TYPE_POLYGON:
				{
					const int triVertIndex0 = polygon->GetVertexIndex(0);
					const int triVertIndex1 = polygon->GetVertexIndex(1);
					const int triVertIndex2 = polygon->GetVertexIndex(2);
#if COMPRESSED_VERTEX_METHOD == 0
					const Vec3V triVert0 = bvhBound->GetVertex(triVertIndex0);
					const Vec3V triVert1 = bvhBound->GetVertex(triVertIndex1);
					const Vec3V triVert2 = bvhBound->GetVertex(triVertIndex2);
#else
#if __SPU
					const Vec3V triVert0 = GetCompressedVertexSPU( bvhBound, triVertIndex0 );
					const Vec3V triVert1 = GetCompressedVertexSPU( bvhBound, triVertIndex1 );
					const Vec3V triVert2 = GetCompressedVertexSPU( bvhBound, triVertIndex2 );
#else
					const Vec3V triVert0 = bvhBound->GetCompressedVertex(triVertIndex0);
					const Vec3V triVert1 = bvhBound->GetCompressedVertex(triVertIndex1);
					const Vec3V triVert2 = bvhBound->GetCompressedVertex(triVertIndex2);
#endif
#endif
					const Vec3V polyNormal = polygon->ComputeUnitNormal(triVert0, triVert1, triVert2);
					TriangleShape triangle(triVert0, triVert1, triVert2);
					triangle.m_PolygonNormal = polyNormal;
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
					triangle.m_EdgeNormals[0] = polyNormal;
					triangle.m_EdgeNormals[1] = polyNormal;
					triangle.m_EdgeNormals[2] = polyNormal;
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
					triangle.SetMargin(TRIANGLE_MARGIN);


					TriangleTransformed tTriangle;
					tTriangle.polyIdx = polygonIndex;
					
					TransformTriangle(&triangle, boundPose, tTriangle.pt[0], tTriangle.pt[1], tTriangle.pt[2], tTriangle.n, tTriangle.pos);

					if( DetectAndEnforceOneTriangle( pClothVerts[idx1], tTriangle.pt[0], tTriangle.pt[1], tTriangle.pt[2], tTriangle.n, tTriangle.pos ) )
					{
						pPrevClothVerts[idx1] = pClothVerts[idx1];
					}


#if !__SPU
					triangle.Release(false);
#endif
					break;
				}

				case PRIM_TYPE_CAPSULE:
				{
					const phPrimCapsule& capsulePrim = bvhPrimitive->GetCapsule();
					const int endIndex0 = capsulePrim.GetEndIndex0();
					const int endIndex1 = capsulePrim.GetEndIndex1();

#if COMPRESSED_VERTEX_METHOD == 0
					Vec3V capsuleEnd0 = bvhBound->GetVertex(endIndex0);
					Vec3V capsuleEnd1 = bvhBound->GetVertex(endIndex1);
#else
#if __SPU
					Vec3V capsuleEnd0 = GetCompressedVertexSPU( bvhBound, endIndex0 );
					Vec3V capsuleEnd1 = GetCompressedVertexSPU( bvhBound, endIndex1 );
#else				
					Vec3V capsuleEnd0 = bvhBound->GetCompressedVertex(endIndex0);
					Vec3V capsuleEnd1 = bvhBound->GetCompressedVertex(endIndex1);
#endif
#endif
					Vec3V capsuleShaft(capsuleEnd1 - capsuleEnd0);
					ScalarV capsuleLengthV = MagFast(capsuleShaft);
					phBoundCapsule capsuleBound;
					capsuleBound.SetCapsuleSize(capsulePrim.GetRadiusV(), capsuleLengthV);

					// Create a matrix to orient the capsule in the local space of the bound.
					Mat34V localCapsuleMatrix(V_IDENTITY);
					const VecBoolV maskY = VecBoolV(V_F_T_F_F);
					const Vec3V result = SelectFT(maskY,capsuleShaft,Vec3V(V_ZERO));
					if (!IsZeroAll(result))
					{
						localCapsuleMatrix.SetCol0(Normalize(Cross(capsuleShaft,Vec3V(V_Y_AXIS_WONE))));
						localCapsuleMatrix.SetCol1(Normalize(capsuleShaft));
						localCapsuleMatrix.SetCol2(Cross(localCapsuleMatrix.GetCol0(),localCapsuleMatrix.GetCol1()));
					}

					localCapsuleMatrix.SetCol3(Average(capsuleEnd0,capsuleEnd1));

					Mat34V boundPartPose;
					Transform(boundPartPose,boundPose,localCapsuleMatrix);

					ScalarV boundRadius = capsuleBound.GetRadiusAroundCentroidV();
					Vec3V boundCentroid = capsuleBound.GetWorldCentroid(boundPartPose);	
					ScalarV capsuleLength = capsuleBound.GetLengthV();
					ScalarV capsuleRadius = capsuleBound.GetRadiusV();
					DetectAndEnforceOneCapsule( GetFlag(FLAG_COLLIDE_EDGES), boundRadius,boundCentroid,capsuleLength,capsuleRadius,boundPartPose,parentOffset);
#if !__SPU
					capsuleBound.Release(false);
#endif
					break;
				}						
				case PRIM_TYPE_BOX:
				{
#if 0
					// TODO: cache boxes

					Assert(0);

					const phPrimBox &boxPrim = *reinterpret_cast<const phPrimBox *>(bvhPrimitive);
					const int vertIndex0 = boxPrim.GetVertexIndex(0);
					const int vertIndex1 = boxPrim.GetVertexIndex(1);
					const int vertIndex2 = boxPrim.GetVertexIndex(2);
					const int vertIndex3 = boxPrim.GetVertexIndex(3);
					Vec3V vert0, vert1, vert2, vert3;

	#if COMPRESSED_VERTEX_METHOD == 0
					vert0 = bvhBound->GetVertex(vertIndex0);
					vert1 = bvhBound->GetVertex(vertIndex1);
					vert2 = bvhBound->GetVertex(vertIndex2);
					vert3 = bvhBound->GetVertex(vertIndex3);
	#else
		#if __SPU
					vert0 = GetCompressedVertexSPU( bvhBound, vertIndex0 );
					vert1 = GetCompressedVertexSPU( bvhBound, vertIndex1 );
					vert2 = GetCompressedVertexSPU( bvhBound, vertIndex2 );
					vert2 = GetCompressedVertexSPU( bvhBound, vertIndex3 );
		#else
					vert0 = bvhBound->GetCompressedVertex(vertIndex0);
					vert1 = bvhBound->GetCompressedVertex(vertIndex1);
					vert2 = bvhBound->GetCompressedVertex(vertIndex2);
					vert3 = bvhBound->GetCompressedVertex(vertIndex3);

		#endif
	#endif
//  						Vec3V a = Transform(boundPose,vert0);
//  						Vec3V b = Transform(boundPose,vert1);
//  						Vec3V c = Transform(boundPose,vert2);
//  						Vec3V d = Transform(boundPose,vert3);
// 
// 	#if __PFDRAW
// 						if( PFD_VerletCollision.Begin(true) )
// 						{		
// 							grcDrawSphere( 0.05f, a );
// 							grcDrawSphere( 0.05f, b );
// 							grcDrawSphere( 0.05f, c );
// 							grcDrawSphere( 0.05f, d );
// 
// 							PFD_VerletCollision.End();
// 						}
// 	#endif
#endif //0
					break;
				}
				case PRIM_TYPE_SPHERE:
				{
// TODO: disable for now
//					clothDebugf1("Unsupported Bvh primitive type BVH_PRIM_TYPE_SPHERE" ); 
					break;
				}
				case PRIM_TYPE_CYLINDER:
				{
// TODO: disable for now
					clothDebugf1("Unsupported Bvh primitive type BVH_PRIM_TYPE_CYLINDER" ); 
					break;
				}
				default:
				{
// TODO: disable for now
//					clothDebugf1("Unsupported Bvh primitive type UNKNOWN" ); 
					break;
				}
			} // end switch

		} // end for 
	} // end for

#if __SPU
	Assert( culledPolyList );
	sysScratchReset( (u8*)culledPolyList );
#endif
}


void phVerletCloth::DetectAndEnforceOne( const bool bCollideEdges, const phBound* RESTRICT bound, Mat34V_In boundPose, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset )
{
	switch (bound->GetType())
	{
#if USED_IN_GTAV
	case phBound::SPHERE:
		{
	#if __SPU
			// NOTE: character cloth doesn't need to collide with spheres
			if( !ENVIRONMENT_ROPE && !ENVIRONMENT_CLOTH )
				break;
	#endif
			DetectAndEnforceOneSphere(static_cast<const phBoundSphere*>(bound),boundPose );
			break;
		}
#endif // USED_IN_GTAV

	case phBound::CAPSULE:
		{
			const phBoundCapsule* pCapsuleBound = static_cast<const phBoundCapsule*>(bound);
#if USE_CAPSULE_EXTRA_EXTENTS
			if( Unlikely(pCapsuleBound->GetHalfHeight() > 0.0f) )
			{
				CollideCapsuleWithExtraExtents(bCollideEdges, pCapsuleBound, boundPose, parentOffset );
			}
			else
#endif
			{
				ScalarV boundRadius = pCapsuleBound->GetRadiusAroundCentroidV();
				Vec3V boundCentroid = pCapsuleBound->GetWorldCentroid(boundPose);	
				ScalarV capsuleLength = pCapsuleBound->GetLengthV();
				ScalarV capsuleRadius = pCapsuleBound->GetRadiusV();
				DetectAndEnforceOneCapsule(bCollideEdges,boundRadius,boundCentroid,capsuleLength,capsuleRadius,boundPose,parentOffset );
			}
			break;
		}

#if USE_TAPERED_CAPSULE
	case phBound::TAPERED_CAPSULE:
		{
			clothErrorf("Collision vs tapered capsule is not supported !");		
			//			DetectAndEnforceOneTaperedCapsule(static_cast<const phBoundTaperedCapsule*>(bound),boundPose);
			break;
		}
#endif

#if USED_IN_GTAV
	case phBound::GEOMETRY:
		{
			Vec3V* RESTRICT pClothVerts = (Vec3V*) m_ClothData.GetVertexPointer();
			Vec3V* RESTRICT pPrevClothVerts = (Vec3V*) m_ClothData.GetVertexPrevPointer();
			DetectAndEnforceOneGeometry( static_cast<const phBoundGeometry*>(bound), boundPose, pClothVerts, pPrevClothVerts );
			break;
		}

	case phBound::BOX:
		{
// TODO: This is causing problems at the moment !!!
//			Vec3V* RESTRICT pClothVerts = (Vec3V*) m_ClothData.GetVertexPointer();
//			Vec3V* RESTRICT pPrevClothVerts = (Vec3V*) m_ClothData.GetVertexPrevPointer();
//			DetectAndEnforceOneBox( bound, boundPose, pClothVerts, pPrevClothVerts );
			break;
		}
#endif //USED_IN_GTAV

	case phBound::BVH:
		{
#if __SPU
			// NOTE: character cloth doesn't need to collide with geometries ... for now
			if( !ENVIRONMENT_ROPE && !ENVIRONMENT_CLOTH )
				break;
#endif

			if( !GetFlag(FLAG_IS_ROPE) )
				break;

			Vec3V* RESTRICT pClothVerts = (Vec3V*) m_ClothData.GetVertexPointer();
			Vec3V* RESTRICT pPrevClothVerts = (Vec3V*) m_ClothData.GetVertexPrevPointer();

			CullPrimitives(bound, boundPose, pClothVerts, pPrevClothVerts, parentOffset);

			break;
		} // end case phBound::BVH:

	case phBound::COMPOSITE:
		{
			const phBoundComposite& compositeBound = *static_cast<const phBoundComposite*>(bound);
			Mat34V	boundPartPose;
			int numBoundParts = compositeBound.GetNumBounds();
#if __SPU

			phBound* partBoundArray[numBoundParts] ;
			sysDmaLargeGetAndWait(partBoundArray, (u32)compositeBound.GetBoundArray(), sizeof(phBound*) * numBoundParts, 0);
			for (int boundPartIndex=0; boundPartIndex<numBoundParts; ++boundPartIndex)
			{
				const phBound* compositePart = partBoundArray[boundPartIndex];
				if( compositePart )
				{

					u8 boundBufferBase[sizeof(phBound)] ;
					//	u8* boundBuffer = (u8*)sysScratchAllocObj<phBound>(1);		// allocate from the scratch pad not the stack
					sysDmaGetAndWait(boundBufferBase, (u32)compositePart, sizeof(phBound), 0);

					const int boundType = ((const phBound*)boundBufferBase)->GetType();
					const int boundSize = g_BoundSizeTable[boundType];
					if( !boundSize )
					{
#if 0//!__DEV
						switch( boundType )
						{
						case phBound::SPHERE:			Displayf("Cloth is colliding with unsupported bound of type phBoundSphere"); break;
						case phBound::CAPSULE:			Displayf("Cloth is colliding with unsupported bound of type phBoundCapsule"); break;
#if USE_TAPERED_CAPSULE
						case phBound::TAPERED_CAPSULE:	Displayf("Cloth is colliding with unsupported bound of type phBoundTaperedCapsule"); break;
#endif
						case phBound::BOX:				Displayf("Cloth is colliding with unsupported bound of type phBoundBox"); break;
						case phBound::GEOMETRY:			Displayf("Cloth is colliding with unsupported bound of type phBoundGeometry"); break;
#if USE_GEOMETRY_CURVED
						case phBound::GEOMETRY_CURVED:	Displayf("Cloth is colliding with unsupported bound of type phBoundCurvedGeom"); break;
#endif
#if USE_GRIDS
						case phBound::GRID:				Displayf("Cloth is colliding with unsupported bound of type phBoundGrid"); break;
#endif
#if USE_RIBBONS
						case phBound::RIBBON:			Displayf("Cloth is colliding with unsupported bound of type phBoundRibbon"); break;
#endif
						case phBound::BVH:				Displayf("Cloth is colliding with unsupported bound of type phBoundBVH"); break;
#if USE_SURFACES
						case phBound::SURFACE:			Displayf("Cloth is colliding with unsupported bound of type phBoundSurface"); break;
#endif
						case phBound::COMPOSITE:		Displayf("Cloth is colliding with unsupported bound of type phBoundComposite"); break;
						case phBound::TRIANGLE:			Displayf("Cloth is colliding with unsupported bound of type TriangleShape"); break;
						};
#endif
						return;
					}
					u8 boundBuffer[boundSize] ;
					if( customMatrices )
					{
						boundPartPose = customMatrices[boundPartIndex];
						sysDmaGetAndWait(boundBuffer, (u32)compositePart, boundSize, 0);
					}
					else
					{
						Mat34V bodyTaPart;
						sysDmaGet( &bodyTaPart, (u32)&compositeBound.GetCurrentMatrix(boundPartIndex), sizeof(Mat34V), 0);
						sysDmaGet( boundBuffer, (u32)compositePart, boundSize, 0);
						sysDmaWait( 1 << 0 );

						Transform(boundPartPose,boundPose,bodyTaPart);
					}

					compositePart = (const phBound*)boundBuffer;
					DetectAndEnforceOne( bCollideEdges, compositePart, boundPartPose, NULL, parentOffset );
				}
			}
#else //__SPU
			for (int boundPartIndex=0; boundPartIndex < numBoundParts; ++boundPartIndex)
			{
				const phBound* RESTRICT compositePart = compositeBound.GetBound(boundPartIndex);
				if( compositePart )
				{
					if( customMatrices )
						boundPartPose = customMatrices[boundPartIndex];
					else
						Transform(boundPartPose,boundPose,compositeBound.GetCurrentMatrix(boundPartIndex));
					DetectAndEnforceOne( bCollideEdges, compositePart, boundPartPose, NULL, parentOffset );
				}
			}
#endif //__SPU

			break;
		}

	case phBound::TRIANGLE:
		{
#if __SPU
			// NOTE: character cloth doesn't include this into the job ... for now
			if( !ENVIRONMENT_ROPE && !ENVIRONMENT_CLOTH)
				break;
#endif

			Vec3V* RESTRICT clothVerts = (Vec3V*) m_ClothData.GetVertexPointer();

			const TriangleShape* pTriangleBound = static_cast<const TriangleShape*>(bound);
			Vec3V planePos = Transform(boundPose, pTriangleBound->m_vertices1[0]);
			Vec3V planeNormal = Transform3x3(boundPose, pTriangleBound->m_PolygonNormal);	


#if DRAW_ALL_COLLISION_BOUNDS
			bool canDrawVerletCollision = GetFlag(FLAG_ENABLE_DEBUG_DRAW);
			if( canDrawVerletCollision )
			{
				if( PFD_VerletCollision.Begin(true) )
				{		
					Vec3V rightV = Normalize( Cross( planeNormal, Vec3V(V_Y_AXIS_WZERO) ) );
					//Vec3V upV = Normalize( Cross( planeNormal, rightV ) );
										
					grcColor( Color32(0.54f,0.17f,0.89f, 0.75f) );	// Color_BlueViolet;
					grcDrawCircle( 2.0f, VEC3V_TO_VECTOR3(planePos), VEC3V_TO_VECTOR3(planeNormal), VEC3V_TO_VECTOR3(rightV), 11, false, true );

					grcColor( Color_red );
					grcDrawLine( VEC3V_TO_VECTOR3(planePos), VEC3V_TO_VECTOR3( Add(planePos, planeNormal)), Color_red, Color_green );

					PFD_VerletCollision.End();
				}
			}
#endif

			const int beginVert = GetNumLockedEdgesFront();
			const int endVert = GetNumVertices() - GetNumLockedEdgesBack();
			for( int vertIndex = beginVert; vertIndex < endVert; ++vertIndex )
			{
				DetectAndEnforceOnePlane( clothVerts[vertIndex], planeNormal, planePos, parentOffset );
			}
			break;
		}

	case phBound::PLANE:
		{
#if __SPU
			// NOTE: character cloth doesn't include this into the job ... for now
			if( !ENVIRONMENT_ROPE && !ENVIRONMENT_CLOTH)
				break;
#endif

			Vec3V* RESTRICT clothVerts = (Vec3V*) m_ClothData.GetVertexPointer();

			const phBoundPlane* pPlaneBound = static_cast<const phBoundPlane*>(bound);
			Vec3V planePos = Transform(boundPose, pPlaneBound->GetPosition() );
			Vec3V planeNormal = Transform3x3(boundPose, pPlaneBound->GetNormal () );	


#if DRAW_ALL_COLLISION_BOUNDS
			bool canDrawVerletCollision = GetFlag(FLAG_ENABLE_DEBUG_DRAW);
			if( canDrawVerletCollision )
			{
				if( PFD_VerletCollision.Begin(true) )
				{		
					Vec3V rightV = Normalize( Cross( planeNormal, Vec3V(V_Y_AXIS_WZERO) ) );
					//Vec3V upV = Normalize( Cross( planeNormal, rightV ) );
										
					grcColor( Color32(0.54f,0.17f,0.89f, 0.75f) );	// Color_BlueViolet;
					grcDrawCircle( 2.0f, VEC3V_TO_VECTOR3(planePos), VEC3V_TO_VECTOR3(planeNormal), VEC3V_TO_VECTOR3(rightV), 11, false, true );

					grcColor( Color_red );
					grcDrawLine( VEC3V_TO_VECTOR3(planePos), VEC3V_TO_VECTOR3( Add(planePos, planeNormal)), Color_red, Color_green );

					PFD_VerletCollision.End();
				}
			}
#endif

			const int numEdgesLockedFront = GetNumLockedEdgesFront();
			const int beginVert = (numEdgesLockedFront ? numEdgesLockedFront: GetClothData().GetNumPinVerts());
			const int endVert = GetNumVertices() - GetNumLockedEdgesBack();
			for( int vertIndex = beginVert; vertIndex < endVert; ++vertIndex )
			{
				DetectAndEnforceOnePlane( clothVerts[vertIndex], planeNormal, planePos, parentOffset );
			}
			break;
		}

	case phBound::CYLINDER:
		{
			const phBoundCylinder* pBoundCylinder = static_cast<const phBoundCylinder*>(bound);

			ScalarV boundRadius = pBoundCylinder->GetRadiusAroundCentroidV();
			Vec3V boundCentroid = pBoundCylinder->GetWorldCentroid(boundPose);	
			ScalarV capsuleLength = pBoundCylinder->GetHeightV();
			ScalarV capsuleRadius = pBoundCylinder->GetRadiusV();
			DetectAndEnforceOneCapsule(bCollideEdges,boundRadius,boundCentroid,capsuleLength,capsuleRadius,boundPose,parentOffset );

			break;
		}
	default:
		{
			break;
		}		
	}
}

int QuickRejectList(int nVertCount, ScalarV_In radiusSqr, Vec3V_In center, const Vec3V* RESTRICT pVerts, int* RESTRICT pOutValues, int nPin, Vec3V_In offset)
{
	int nOutCount = 0;
	const int nVert8 = (((nVertCount-nPin) >> 3) << 3) + nPin;
	int i;

	const Vec3V offsetFromCenter = Subtract(offset,center);

#if __XENON // dot products are fast

	for( i = nPin; i < nVert8; i += 8 )
	{
		const Vec3V localVertex0 = pVerts[i+0];
		const Vec3V localVertex1 = pVerts[i+1];
		const Vec3V localVertex2 = pVerts[i+2];
		const Vec3V localVertex3 = pVerts[i+3];
		const Vec3V localVertex4 = pVerts[i+4];
		const Vec3V localVertex5 = pVerts[i+5];
		const Vec3V localVertex6 = pVerts[i+6];
		const Vec3V localVertex7 = pVerts[i+7];

		const ScalarV lengthSqr0 = MagSquared(Add(localVertex0,offsetFromCenter));
		const ScalarV lengthSqr1 = MagSquared(Add(localVertex1,offsetFromCenter));
		const ScalarV lengthSqr2 = MagSquared(Add(localVertex2,offsetFromCenter));
		const ScalarV lengthSqr3 = MagSquared(Add(localVertex3,offsetFromCenter));
		const ScalarV lengthSqr4 = MagSquared(Add(localVertex4,offsetFromCenter));
		const ScalarV lengthSqr5 = MagSquared(Add(localVertex5,offsetFromCenter));
		const ScalarV lengthSqr6 = MagSquared(Add(localVertex6,offsetFromCenter));
		const ScalarV lengthSqr7 = MagSquared(Add(localVertex7,offsetFromCenter));

		pOutValues[nOutCount] = i+0; nOutCount += IsLessThanAll(lengthSqr0,radiusSqr);
		pOutValues[nOutCount] = i+1; nOutCount += IsLessThanAll(lengthSqr1,radiusSqr);
		pOutValues[nOutCount] = i+2; nOutCount += IsLessThanAll(lengthSqr2,radiusSqr);
		pOutValues[nOutCount] = i+3; nOutCount += IsLessThanAll(lengthSqr3,radiusSqr);
		pOutValues[nOutCount] = i+4; nOutCount += IsLessThanAll(lengthSqr4,radiusSqr);
		pOutValues[nOutCount] = i+5; nOutCount += IsLessThanAll(lengthSqr5,radiusSqr);
		pOutValues[nOutCount] = i+6; nOutCount += IsLessThanAll(lengthSqr6,radiusSqr);
		pOutValues[nOutCount] = i+7; nOutCount += IsLessThanAll(lengthSqr7,radiusSqr);
	}

#else // not __XENON (transposed to avoid dot-products (TODO -- we could use _mm_movemask_ps on pc?))

	const Vec4V offsetFromCenter_X(offsetFromCenter.GetX());
	const Vec4V offsetFromCenter_Y(offsetFromCenter.GetY());
	const Vec4V offsetFromCenter_Z(offsetFromCenter.GetZ());

	const Vec4V radiusSqrV(radiusSqr);

	const VecBoolV _oneInt = VecBoolV(ScalarVConstant<1>().GetIntrin128());

#if !(__SPU || RSG_PC)
	Vec::Vector_4V* tempDst = (Vec::Vector_4V*)Alloca(Vec::Vector_4V, nVert8 - nPin);
	const u32* tempSrc = (const u32*)tempDst;
#endif // !(__SPU || RSG_PC)

	for( i = nPin; i < nVert8; i += 8 )
	{
		const Vec3V localVertex0 = pVerts[i+0];
		const Vec3V localVertex1 = pVerts[i+1];
		const Vec3V localVertex2 = pVerts[i+2];
		const Vec3V localVertex3 = pVerts[i+3];
		const Vec3V localVertex4 = pVerts[i+4];
		const Vec3V localVertex5 = pVerts[i+5];
		const Vec3V localVertex6 = pVerts[i+6];
		const Vec3V localVertex7 = pVerts[i+7];

		Vec4V localVertex_X0123,localVertex_Y0123,localVertex_Z0123;
		Vec4V localVertex_X4567,localVertex_Y4567,localVertex_Z4567;

		Transpose3x4to4x3(localVertex_X0123,localVertex_Y0123,localVertex_Z0123,localVertex0,localVertex1,localVertex2,localVertex3);
		Transpose3x4to4x3(localVertex_X4567,localVertex_Y4567,localVertex_Z4567,localVertex4,localVertex5,localVertex6,localVertex7);

		const Vec4V v_X0123 = Add(localVertex_X0123,offsetFromCenter_X);
		const Vec4V v_Y0123 = Add(localVertex_Y0123,offsetFromCenter_Y);
		const Vec4V v_Z0123 = Add(localVertex_Z0123,offsetFromCenter_Z);
		const Vec4V v_X4567 = Add(localVertex_X4567,offsetFromCenter_X);
		const Vec4V v_Y4567 = Add(localVertex_Y4567,offsetFromCenter_Y);
		const Vec4V v_Z4567 = Add(localVertex_Z4567,offsetFromCenter_Z);

		const Vec4V lengthSqr_0123 = AddScaled(AddScaled(Scale(v_X0123,v_X0123),v_Y0123,v_Y0123),v_Z0123,v_Z0123);
		const Vec4V lengthSqr_4567 = AddScaled(AddScaled(Scale(v_X4567,v_X4567),v_Y4567,v_Y4567),v_Z4567,v_Z4567);

		const Vec::Vector_4V comparison_0123 = And(IsLessThan(lengthSqr_0123,radiusSqrV),_oneInt).GetIntrin128();
		const Vec::Vector_4V comparison_4567 = And(IsLessThan(lengthSqr_4567,radiusSqrV),_oneInt).GetIntrin128();

#if __SPU || RSG_PC
		// since we don't care about LHS stalls, just use the comparisons immediately
		pOutValues[nOutCount] = i+0; nOutCount += Vec::GetXi(comparison_0123);
		pOutValues[nOutCount] = i+1; nOutCount += Vec::GetYi(comparison_0123);
		pOutValues[nOutCount] = i+2; nOutCount += Vec::GetZi(comparison_0123);
		pOutValues[nOutCount] = i+3; nOutCount += Vec::GetWi(comparison_0123);
		pOutValues[nOutCount] = i+4; nOutCount += Vec::GetXi(comparison_4567);
		pOutValues[nOutCount] = i+5; nOutCount += Vec::GetYi(comparison_4567);
		pOutValues[nOutCount] = i+6; nOutCount += Vec::GetZi(comparison_4567);
		pOutValues[nOutCount] = i+7; nOutCount += Vec::GetWi(comparison_4567);
#else
		tempDst[0] = comparison_0123;
		tempDst[1] = comparison_4567;

		tempDst += 2;
#endif
	}

#if !(__SPU || RSG_PC)
	// avoid LHS stalls by reading results in a separate loop
	for( i = nPin; i < nVert8; i += 8 )
	{
		pOutValues[nOutCount] = i+0; nOutCount += tempSrc[0];
		pOutValues[nOutCount] = i+1; nOutCount += tempSrc[1];
		pOutValues[nOutCount] = i+2; nOutCount += tempSrc[2];
		pOutValues[nOutCount] = i+3; nOutCount += tempSrc[3];
		pOutValues[nOutCount] = i+4; nOutCount += tempSrc[4];
		pOutValues[nOutCount] = i+5; nOutCount += tempSrc[5];
		pOutValues[nOutCount] = i+6; nOutCount += tempSrc[6];
		pOutValues[nOutCount] = i+7; nOutCount += tempSrc[7];

		tempSrc += 8;
	}
#endif // !(__SPU || RSG_PC)

#endif // not __XENON

	// and the rest
	for( ; i < nVertCount; i++ )
	{
		const Vec3V localVertex = pVerts[i];
		const ScalarV lengthSqr = MagSquared(Add(localVertex,offsetFromCenter));
		pOutValues[nOutCount] = i;
		nOutCount += IsLessThanAll(lengthSqr,radiusSqr);
	}

	return nOutCount;
}

#if USED_IN_GTAV
void phVerletCloth::DetectAndEnforceOneSphere( const phBoundSphere* RESTRICT bound, Mat34V_In boundPose )
{
	PF_FUNC(ClothCollisionsSphere);

	Vec3V* RESTRICT clothVertexPositions = m_ClothData.GetVertexPointer();
	Assert( clothVertexPositions );

	ScalarV boundRadius = bound->GetRadiusAroundCentroidV();
	ScalarV combinedRadiusSquared = Scale(boundRadius,boundRadius);
	Vec3V centroid = bound->GetWorldCentroid(boundPose);

	const int nVert = GetNumVertices();
#if NO_PIN_VERTS_IN_VERLET
	const int nPins = GetClothData().GetNumPinVerts();
#else
	const int nPins = GetPinCount();
#endif

	// Cull test all of the verts vs the bound
#if __SPU
	sysScratchScope s;
	int* pCulled = sysScratchAllocObj<int>(nVert);
#else
	int* RESTRICT pCulled = Alloca(int, nVert);
#endif
	const ScalarV _zero(V_ZERO);
	const ScalarV _neg1(V_NEGONE);
	const int nRemaining = QuickRejectList(nVert, combinedRadiusSquared, centroid, clothVertexPositions, pCulled, 0, Vec3V(V_ZERO));
	for (int culledVertexIndex=0; culledVertexIndex<nRemaining; culledVertexIndex++)
	{
		const int vertexIndex = pCulled[culledVertexIndex];
		if( vertexIndex < nPins )
			continue;
		const Vec3V localVertex = clothVertexPositions[vertexIndex];
		const Vec3V dir = Subtract(localVertex,centroid);
		const ScalarV oneOverDistance = InvMag(dir);
		const ScalarV depth = Max(_zero,AddScaled(_neg1,boundRadius,oneOverDistance));
		clothVertexPositions[vertexIndex] = AddScaled(localVertex,dir,depth);
	}

#if __SPU
	Assert( pCulled );
	sysScratchReset( (u8*)pCulled );
#endif
}

#endif // USED_IN_GTAV


#ifdef __SNC__
#pragma control postopt=5
#endif


#define FAST_DetectAndEnforceOneCapsule	( __XENON || __PS3 ) // TODO -- is this faster on pc as well?

#if USE_CAPSULE_EXTRA_EXTENTS

// TODO: majority of the code is dup on DetectAndEnforceOneCapsule ... combine both functions once all is working fine

void phVerletCloth::CollideCapsuleWithExtraExtents(const bool bCollideEdges, const phBoundCapsule* RESTRICT capsuleBound, Mat34V_In boundPose, Vec3V_In parentOffset )
{
	PF_FUNC(ClothCollisionsCapsule);

	Vec3V* RESTRICT clothVertexPositions = m_ClothData.GetVertexPointer();
	Assert( clothVertexPositions );

	ScalarV boundRadius = capsuleBound->GetRadiusAroundCentroidV();
	ScalarV combinedRadiusSquared = Scale(boundRadius,boundRadius);

	Mat34V boundCapsuleMat = boundPose;			// rotate or translate the boundCapsule by some custom offset if needed ... shouldn't be needed
	Vec3V boundCentroid = capsuleBound->GetWorldCentroid(boundCapsuleMat);	
	Vec3V unitAxis = boundCapsuleMat.GetCol1();

	ScalarV capsuleLength = capsuleBound->GetLengthV();
	ScalarV capsuleRadius = capsuleBound->GetRadiusV();

	Vec3V end0 = SubtractScaled(boundCentroid,unitAxis,Scale(ScalarV(V_HALF),capsuleLength));
	ScalarV combinedCapsuleRadius2 = Scale(capsuleRadius,capsuleRadius);

#if DRAW_ALL_COLLISION_BOUNDS
	float halfHeigth = capsuleBound->GetHalfHeight();
	Assert( halfHeigth > 0.0f );
	bool canDrawVerletCollision = GetFlag(FLAG_ENABLE_DEBUG_DRAW);
	if( canDrawVerletCollision )
	{		
		canDrawVerletCollision = PFD_VerletCollision.Begin(true);
		if( canDrawVerletCollision )
		{
			grcDrawCapsule( capsuleLength.Getf(), capsuleRadius.Getf(), MAT34V_TO_MATRIX34(boundCapsuleMat), 8, false, halfHeigth );
		}
	}
#endif

	const int nVert = GetNumVertices();
#if NO_PIN_VERTS_IN_VERLET
	const int nPins = GetClothData().GetNumPinVerts();
#else
	const int nPins = GetPinCount();
#endif


#if 0 //FAST_DetectAndEnforceOneCapsule

	ScalarV _zero(V_ZERO);
	int i;
	const int nVert8 = ((nVert-nPins)>>3)<<3;
	for ( i = nPins; i < nVert8; i+=8 )
	{
		const int   i0 = i,
			i1 = i+1,
			i2 = i+2,
			i3 = i+3,
			i4 = i+4,
			i5 = i+5,
			i6 = i+6,
			i7 = i+7;

		Vec3V clothVertex0 = Add( clothVertexPositions[i0], parentOffset ) ;
		Vec3V clothVertex1 = Add( clothVertexPositions[i1], parentOffset ) ;
		Vec3V clothVertex2 = Add( clothVertexPositions[i2], parentOffset ) ;
		Vec3V clothVertex3 = Add( clothVertexPositions[i3], parentOffset ) ;
		Vec3V clothVertex4 = Add( clothVertexPositions[i4], parentOffset ) ;
		Vec3V clothVertex5 = Add( clothVertexPositions[i5], parentOffset ) ;
		Vec3V clothVertex6 = Add( clothVertexPositions[i6], parentOffset ) ;
		Vec3V clothVertex7 = Add( clothVertexPositions[i7], parentOffset ) ;

		Vec3V endToSphere0 = Subtract(clothVertex0,end0);
		Vec3V endToSphere1 = Subtract(clothVertex1,end0);
		Vec3V endToSphere2 = Subtract(clothVertex2,end0);
		Vec3V endToSphere3 = Subtract(clothVertex3,end0);
		Vec3V endToSphere4 = Subtract(clothVertex4,end0);
		Vec3V endToSphere5 = Subtract(clothVertex5,end0);
		Vec3V endToSphere6 = Subtract(clothVertex6,end0);
		Vec3V endToSphere7 = Subtract(clothVertex7,end0);

		ScalarV closestPointAlongShaft0 = Dot(endToSphere0,unitAxis);
		ScalarV closestPointAlongShaft1 = Dot(endToSphere1,unitAxis);
		ScalarV closestPointAlongShaft2 = Dot(endToSphere2,unitAxis);
		ScalarV closestPointAlongShaft3 = Dot(endToSphere3,unitAxis);
		ScalarV closestPointAlongShaft4 = Dot(endToSphere4,unitAxis);
		ScalarV closestPointAlongShaft5 = Dot(endToSphere5,unitAxis);
		ScalarV closestPointAlongShaft6 = Dot(endToSphere6,unitAxis);
		ScalarV closestPointAlongShaft7 = Dot(endToSphere7,unitAxis);

		BoolV test10 = IsLessThan(capsuleLength,closestPointAlongShaft0);
		BoolV test11 = IsLessThan(capsuleLength,closestPointAlongShaft1);
		BoolV test12 = IsLessThan(capsuleLength,closestPointAlongShaft2);
		BoolV test13 = IsLessThan(capsuleLength,closestPointAlongShaft3);
		BoolV test14 = IsLessThan(capsuleLength,closestPointAlongShaft4);
		BoolV test15 = IsLessThan(capsuleLength,closestPointAlongShaft5);
		BoolV test16 = IsLessThan(capsuleLength,closestPointAlongShaft6);
		BoolV test17 = IsLessThan(capsuleLength,closestPointAlongShaft7);

		BoolV test20 = IsGreaterThan(_zero,closestPointAlongShaft0);
		BoolV test21 = IsGreaterThan(_zero,closestPointAlongShaft1);
		BoolV test22 = IsGreaterThan(_zero,closestPointAlongShaft2);
		BoolV test23 = IsGreaterThan(_zero,closestPointAlongShaft3);
		BoolV test24 = IsGreaterThan(_zero,closestPointAlongShaft4);
		BoolV test25 = IsGreaterThan(_zero,closestPointAlongShaft5);
		BoolV test26 = IsGreaterThan(_zero,closestPointAlongShaft6);
		BoolV test27 = IsGreaterThan(_zero,closestPointAlongShaft7);

		closestPointAlongShaft0 = SelectFT(test20,SelectFT(test10,closestPointAlongShaft0,capsuleLength),_zero);
		closestPointAlongShaft1 = SelectFT(test21,SelectFT(test11,closestPointAlongShaft1,capsuleLength),_zero);
		closestPointAlongShaft2 = SelectFT(test22,SelectFT(test12,closestPointAlongShaft2,capsuleLength),_zero);
		closestPointAlongShaft3 = SelectFT(test23,SelectFT(test13,closestPointAlongShaft3,capsuleLength),_zero);
		closestPointAlongShaft4 = SelectFT(test24,SelectFT(test14,closestPointAlongShaft4,capsuleLength),_zero);
		closestPointAlongShaft5 = SelectFT(test25,SelectFT(test15,closestPointAlongShaft5,capsuleLength),_zero);
		closestPointAlongShaft6 = SelectFT(test26,SelectFT(test16,closestPointAlongShaft6,capsuleLength),_zero);
		closestPointAlongShaft7 = SelectFT(test27,SelectFT(test17,closestPointAlongShaft7,capsuleLength),_zero);

		Vec3V closestPointOnShaft0 = AddScaled(end0,unitAxis,closestPointAlongShaft0);
		Vec3V closestPointOnShaft1 = AddScaled(end0,unitAxis,closestPointAlongShaft1);
		Vec3V closestPointOnShaft2 = AddScaled(end0,unitAxis,closestPointAlongShaft2);
		Vec3V closestPointOnShaft3 = AddScaled(end0,unitAxis,closestPointAlongShaft3);
		Vec3V closestPointOnShaft4 = AddScaled(end0,unitAxis,closestPointAlongShaft4);
		Vec3V closestPointOnShaft5 = AddScaled(end0,unitAxis,closestPointAlongShaft5);
		Vec3V closestPointOnShaft6 = AddScaled(end0,unitAxis,closestPointAlongShaft6);
		Vec3V closestPointOnShaft7 = AddScaled(end0,unitAxis,closestPointAlongShaft7);

		// See if that closest point is close enough to the sphere.
		Vec3V dir0 = Subtract(clothVertex0,closestPointOnShaft0);
		Vec3V dir1 = Subtract(clothVertex1,closestPointOnShaft1);
		Vec3V dir2 = Subtract(clothVertex2,closestPointOnShaft2);
		Vec3V dir3 = Subtract(clothVertex3,closestPointOnShaft3);
		Vec3V dir4 = Subtract(clothVertex4,closestPointOnShaft4);
		Vec3V dir5 = Subtract(clothVertex5,closestPointOnShaft5);
		Vec3V dir6 = Subtract(clothVertex6,closestPointOnShaft6);
		Vec3V dir7 = Subtract(clothVertex7,closestPointOnShaft7);

		ScalarV distance20 = MagSquared(dir0);
		ScalarV distance21 = MagSquared(dir1);
		ScalarV distance22 = MagSquared(dir2);
		ScalarV distance23 = MagSquared(dir3);
		ScalarV distance24 = MagSquared(dir4);
		ScalarV distance25 = MagSquared(dir5);
		ScalarV distance26 = MagSquared(dir6);
		ScalarV distance27 = MagSquared(dir7);

		Vec3V normal0 = Scale( dir0, InvSqrt(distance20) );
		Vec3V normal1 = Scale( dir1, InvSqrt(distance21) );
		Vec3V normal2 = Scale( dir2, InvSqrt(distance22) );
		Vec3V normal3 = Scale( dir3, InvSqrt(distance23) );
		Vec3V normal4 = Scale( dir4, InvSqrt(distance24) );
		Vec3V normal5 = Scale( dir5, InvSqrt(distance25) );
		Vec3V normal6 = Scale( dir6, InvSqrt(distance26) );
		Vec3V normal7 = Scale( dir7, InvSqrt(distance27) );

		ScalarV distance0 = Sqrt( distance20 );
		ScalarV distance1 = Sqrt( distance21 );
		ScalarV distance2 = Sqrt( distance22 );
		ScalarV distance3 = Sqrt( distance23 );
		ScalarV distance4 = Sqrt( distance24 );
		ScalarV distance5 = Sqrt( distance25 );
		ScalarV distance6 = Sqrt( distance26 );
		ScalarV distance7 = Sqrt( distance27 );

		ScalarV depth0 = Max(Subtract(capsuleRadius,distance0),_zero);
		ScalarV depth1 = Max(Subtract(capsuleRadius,distance1),_zero);
		ScalarV depth2 = Max(Subtract(capsuleRadius,distance2),_zero);
		ScalarV depth3 = Max(Subtract(capsuleRadius,distance3),_zero);
		ScalarV depth4 = Max(Subtract(capsuleRadius,distance4),_zero);
		ScalarV depth5 = Max(Subtract(capsuleRadius,distance5),_zero);
		ScalarV depth6 = Max(Subtract(capsuleRadius,distance6),_zero);
		ScalarV depth7 = Max(Subtract(capsuleRadius,distance7),_zero);

		clothVertexPositions[i0] = AddScaled(clothVertexPositions[i0], normal0, depth0 );
		clothVertexPositions[i1] = AddScaled(clothVertexPositions[i1], normal1, depth1 );
		clothVertexPositions[i2] = AddScaled(clothVertexPositions[i2], normal2, depth2 );
		clothVertexPositions[i3] = AddScaled(clothVertexPositions[i3], normal3, depth3 );
		clothVertexPositions[i4] = AddScaled(clothVertexPositions[i4], normal4, depth4 );
		clothVertexPositions[i5] = AddScaled(clothVertexPositions[i5], normal5, depth5 );
		clothVertexPositions[i6] = AddScaled(clothVertexPositions[i6], normal6, depth6 );
		clothVertexPositions[i7] = AddScaled(clothVertexPositions[i7], normal7, depth7 );
	}
	for ( ; i < nVert; ++i )
	{
		// NOTE: character cloth is simulated in local space ( but is oriented ) when doing collision verts need to be translated to world space
		const Vec3V clothVertex = Add( clothVertexPositions[i], parentOffset ) ;

		Vec3V endToSphere = Subtract(clothVertex,end0);
		ScalarV closestPointAlongShaft = Dot(endToSphere,unitAxis);

		BoolV test1 = IsLessThan(capsuleLength,closestPointAlongShaft);
		BoolV test2 = IsGreaterThan(ScalarV(V_ZERO),closestPointAlongShaft);
		closestPointAlongShaft = SelectFT(test2,SelectFT(test1,closestPointAlongShaft,capsuleLength),_zero);
		Vec3V closestPointOnShaft = AddScaled(end0,unitAxis,closestPointAlongShaft);

		// See if that closest point is close enough to the sphere.
		Vec3V dir = Subtract(clothVertex,closestPointOnShaft);
		ScalarV distance2 = MagSquared(dir);

		Vec3V normal = Scale( dir, InvSqrt(distance2) );
		ScalarV distance = Sqrt( distance2 );
		ScalarV depth = Max(Subtract(capsuleRadius,distance),_zero);

		clothVertexPositions[i] = AddScaled(clothVertexPositions[i], normal, depth );
	}


#else // FAST_

	// 1st Pass - Build a list of all the verts close enough to this bound to really care about

 #if __SPU
	sysScratchScope s;
	int* pCulled = sysScratchAllocObj<int>(nVert);
 #else
	int* RESTRICT pCulled = Alloca(int,nVert);
 #endif

	Vec3V capsuleCenter = boundCapsuleMat.GetCol3();
	ScalarV capsuleCenterZ = capsuleCenter.GetZ();
	//	ScalarV halfHeightSqr = ScalarVFromF32(halfHeight*halfHeight);
	//	Vec3V tempV = Subtract( capsuleCenter, clothVertex );
	//	ScalarV radSqr = Dot(tempV, tempV);
	//	BoolV res0 = SelectFT( IsLessThan( radSqr,halfHeightSqr ), );

	const int remaining = QuickRejectList(nVert, combinedRadiusSquared, boundCentroid, clothVertexPositions, pCulled, nPins, parentOffset);

	// 2nd Pass - Build a list of all the verts that actually collide
 #if __SPU
	Vec3V* pCullDir2 = sysScratchAllocObj<Vec3V>(remaining);
	ScalarV* pCullDistance2 = sysScratchAllocObj<ScalarV>(remaining);
	int* pCulled2 = sysScratchAllocObj<int>(remaining);
 #else

	Vec3V* RESTRICT pCullDir2 = Alloca(Vec3V,remaining);
	ScalarV* RESTRICT pCullDistance2 = Alloca(ScalarV,remaining);
	int* RESTRICT pCulled2 = Alloca(int,remaining);
 #endif

	ScalarV _zero(V_ZERO);
	int nPass2Count = 0;

	for (int culledVertexIndex=0; culledVertexIndex<remaining; culledVertexIndex++)
	{
		const int vertexIndex = pCulled[culledVertexIndex];
		// Find the point on the shaft that is closest to the sphere.

		// NOTE: character cloth is simulated in local space ( but is oriented ) when doing collision verts need to be translated to world space
		Vec3V clothVertex = Add( clothVertexPositions[vertexIndex], parentOffset ) ;
		clothVertex.SetZ( capsuleCenterZ );

		Vec3V endToSphere = Subtract(clothVertex,end0);
// TODO: rename the following to: ... distance from point to the bound center
		ScalarV closestPointAlongShaft = Dot(endToSphere,unitAxis);

		BoolV test1 = IsLessThan(capsuleLength,closestPointAlongShaft);
		BoolV test2 = IsGreaterThan(ScalarV(V_ZERO),closestPointAlongShaft);
		closestPointAlongShaft = SelectFT(test2,SelectFT(test1,closestPointAlongShaft,capsuleLength),_zero);
		Vec3V closestPointOnShaft = AddScaled(end0,unitAxis,closestPointAlongShaft);


		// See if that closest point is close enough to the sphere.
		Vec3V dir = Subtract(clothVertex,closestPointOnShaft);
		ScalarV distance2 = MagSquared(dir);

		u32 r0;
		SCALARV_TO_VECTOR3(distance2).IsLessThanVR4(SCALARV_TO_VECTOR3(combinedCapsuleRadius2),r0);
		r0 &= VEC3_CMP_VAL;
		r0 >>= 7;

		pCullDir2[nPass2Count] = dir;
		pCullDistance2[nPass2Count] = distance2;
		pCulled2[nPass2Count] = vertexIndex;
		nPass2Count += r0;
	}

	// 3rd Pass - Resolve all the collisions from pass2
	for (int collidingVertexIndex=0; collidingVertexIndex<nPass2Count; collidingVertexIndex++)
	{
		const int vertexIndex = pCulled2[collidingVertexIndex];
		Vec3V normal = Scale( pCullDir2[collidingVertexIndex], InvSqrt( pCullDistance2[collidingVertexIndex] ) ) ;
		ScalarV distance = Sqrt( pCullDistance2[collidingVertexIndex] );
		ScalarV depth = Max(Subtract(capsuleRadius,distance),_zero);
		clothVertexPositions[vertexIndex] = AddScaled( clothVertexPositions[vertexIndex], normal, depth );
	}

 #if __SPU
	Assert( pCulled );
	sysScratchReset( (u8*)pCulled );
 #endif

#endif // FAST_DetectAndEnforceOneCapsule


	if( GetFlag(FLAG_IS_ROPE) || bCollideEdges )
	{
		const phEdgeData* pEdgeData = &(GetEdge(0));		

		// TODO: we might want to unroll this for better performance ... 
		// get some real env cloth capsule custom bounds cases in game !
		// -svetli

		ScalarV _half(V_HALF);

		for( int i = 0; i < m_NumEdges; ++i)
		{
			const phEdgeData* RESTRICT edge = &pEdgeData[i];
			const u16 edgeVert0 = edge->m_vertIndices[0];
			const u16 edgeVert1 = edge->m_vertIndices[1];

			Vec3V v0 = clothVertexPositions[edgeVert0];
			Vec3V v1 = clothVertexPositions[edgeVert1];

			Vec3V vertexPos0 = Add( v0, parentOffset );
			Vec3V vertexPos1 = Add( v1, parentOffset );

			ScalarV newBoundCentroidZ = Scale(Add(vertexPos0, vertexPos1), _half).GetZ();
			boundCentroid.SetZ(newBoundCentroidZ);

			Vec3V capsuleEnd0 = SubtractScaled(boundCentroid,unitAxis,Scale(_half,capsuleLength));
			Vec3V capsuleEnd1 = AddScaled(boundCentroid,unitAxis,Scale(_half,capsuleLength));

			Vec3V pointA2 = vertexPos1 - vertexPos0;

			Vec3V pointB1 = capsuleEnd0 - vertexPos0;
			Vec3V pointB2 = capsuleEnd1 - vertexPos0;

			Vec3V pointA, pointB;
			geomPoints::FindClosestSegSegToSeg(pointA.GetIntrin128Ref(), pointB.GetIntrin128Ref(), pointA2.GetIntrin128(), pointB1.GetIntrin128(), pointB2.GetIntrin128());

			Vec3V newSeg = Subtract(pointA,pointB);
			Vec3V normalSeq = Normalize(newSeg);
			ScalarV newSegMag = Mag(newSeg);
			BoolV testCond = IsLessThan( Scale(newSegMag,newSegMag),combinedCapsuleRadius2);
			ScalarV diff = Subtract( capsuleRadius, newSegMag );			
			ScalarV depthSeq = SelectFT( testCond, _zero, diff );

			clothVertexPositions[edgeVert0] = AddScaled( v0, normalSeq, depthSeq );
			clothVertexPositions[edgeVert1] = AddScaled( v1, normalSeq, depthSeq );

 #if  DRAW_EDGE_COLLISION
			if( canDrawVerletCollision && testCond.Getb() )
			{
				grcColor(Color_red);
				grcDrawSphere( 0.05f, Add(pointA, vertexPos0) );
				grcColor(Color_green);
				grcDrawSphere( 0.05f, Add(pointB, vertexPos0) );
			}
 #endif
		}
	}

 #if DRAW_ALL_COLLISION_BOUNDS
	if( canDrawVerletCollision )
	{
		PFD_VerletCollision.End();
	}
 #endif
}
#endif // USE_CAPSULE_EXTRA_EXTENTS


void phVerletCloth::DetectAndEnforceOneCapsule( const bool bCollideEdges, ScalarV_In boundRadius, Vec3V_In boundCentroid, ScalarV_In capsuleLength, ScalarV_In capsuleRadius, Mat34V_In boundPose, Vec3V_In parentOffset )
{
	PF_FUNC(ClothCollisionsCapsule);

	Vec3V* RESTRICT clothVertexPositions = m_ClothData.GetVertexPointer();
	Assert( clothVertexPositions );

	Mat34V boundCapsuleMat = boundPose;			// rotate or translate the boundCapsule by some custom offset if needed ... shouldn't be needed
	Vec3V unitAxis = boundCapsuleMat.GetCol1();

//	ScalarV boundRadius = capsuleBound->GetRadiusAroundCentroidV();
	ScalarV combinedRadiusSquared = Scale(boundRadius,boundRadius);
//	Vec3V boundCentroid = capsuleBound->GetWorldCentroid(boundCapsuleMat);	
//	ScalarV capsuleLength = capsuleBound->GetLengthV();
//	ScalarV capsuleRadius = capsuleBound->GetRadiusV();

	Vec3V end0 = SubtractScaled(boundCentroid,unitAxis,Scale(ScalarV(V_HALF),capsuleLength));
	ScalarV combinedCapsuleRadius2 = Scale(capsuleRadius,capsuleRadius);

#if DRAW_ALL_COLLISION_BOUNDS
	bool canDrawVerletCollision = GetFlag(FLAG_ENABLE_DEBUG_DRAW);
	if( canDrawVerletCollision )
	{
		canDrawVerletCollision = PFD_VerletCollision.Begin(true);
		if( canDrawVerletCollision )
		{
			grcDrawCapsule( capsuleLength.Getf(), capsuleRadius.Getf(), MAT34V_TO_MATRIX34(boundCapsuleMat), 8 );
		}
	}
#endif


#if ENABLE_SPU_DEBUG_DRAW	

	Vec3V	col0 = boundCapsuleMat.GetCol0(),
			col1 = boundCapsuleMat.GetCol1(),
			col2 = boundCapsuleMat.GetCol2(),
			col3 = boundCapsuleMat.GetCol3();

	SpuDebugCapsule( (void*)&col0
					,(void*)&col1
					,(void*)&col2
					,(void*)&col3
					,(void*)&capsuleLength, (void*)&capsuleRadius );
#endif

	const int nVert = GetNumVertices();
#if NO_PIN_VERTS_IN_VERLET
	const int nPins = GetClothData().GetNumPinVerts();
#else
	const int nPins = GetPinCount();
#endif


#if FAST_DetectAndEnforceOneCapsule

	const Vec3V parentOffsetMinusEnd0 = Subtract(parentOffset, end0);
#if !__XENON
	const Vec4V parentOffsetMinusEnd0_X(parentOffsetMinusEnd0.GetX());
	const Vec4V parentOffsetMinusEnd0_Y(parentOffsetMinusEnd0.GetY());
	const Vec4V parentOffsetMinusEnd0_Z(parentOffsetMinusEnd0.GetZ());

	const Vec4V unitAxis_X(unitAxis.GetX());
	const Vec4V unitAxis_Y(unitAxis.GetY());
	const Vec4V unitAxis_Z(unitAxis.GetZ());

	const Vec4V capsuleLengthV(capsuleLength);
#endif // !__XENON
	const Vec4V capsuleRadiusV(capsuleRadius);

	const Vec4V _zeroV(V_ZERO);
	const Vec4V _neg1V(V_NEGONE);
	const ScalarV _zero(V_ZERO);
	const ScalarV _neg1(V_NEGONE);

	int i;
	const int nVert8 = ((nVert-nPins)>>3)<<3;
	for ( i = nPins; i < nVert8; i+=8 )
	{
		const int   i0 = i,
					i1 = i+1,
					i2 = i+2,
					i3 = i+3,
					i4 = i+4,
					i5 = i+5,
					i6 = i+6,
					i7 = i+7;

		const Vec3V localVertex0 = clothVertexPositions[i0];
		const Vec3V localVertex1 = clothVertexPositions[i1];
		const Vec3V localVertex2 = clothVertexPositions[i2];
		const Vec3V localVertex3 = clothVertexPositions[i3];
		const Vec3V localVertex4 = clothVertexPositions[i4];
		const Vec3V localVertex5 = clothVertexPositions[i5];
		const Vec3V localVertex6 = clothVertexPositions[i6];
		const Vec3V localVertex7 = clothVertexPositions[i7];

#if __XENON // dot products are fast

		const Vec3V clothVertex0 = Add(localVertex0,parentOffset);
		const Vec3V clothVertex1 = Add(localVertex1,parentOffset);
		const Vec3V clothVertex2 = Add(localVertex2,parentOffset);
		const Vec3V clothVertex3 = Add(localVertex3,parentOffset);
		const Vec3V clothVertex4 = Add(localVertex4,parentOffset);
		const Vec3V clothVertex5 = Add(localVertex5,parentOffset);
		const Vec3V clothVertex6 = Add(localVertex6,parentOffset);
		const Vec3V clothVertex7 = Add(localVertex7,parentOffset);

		const Vec3V endToSphere0 = Add(localVertex0,parentOffsetMinusEnd0);
		const Vec3V endToSphere1 = Add(localVertex1,parentOffsetMinusEnd0);
		const Vec3V endToSphere2 = Add(localVertex2,parentOffsetMinusEnd0);
		const Vec3V endToSphere3 = Add(localVertex3,parentOffsetMinusEnd0);
		const Vec3V endToSphere4 = Add(localVertex4,parentOffsetMinusEnd0);
		const Vec3V endToSphere5 = Add(localVertex5,parentOffsetMinusEnd0);
		const Vec3V endToSphere6 = Add(localVertex6,parentOffsetMinusEnd0);
		const Vec3V endToSphere7 = Add(localVertex7,parentOffsetMinusEnd0);

		const ScalarV distanceAlongShaft0 = Dot(endToSphere0,unitAxis);
		const ScalarV distanceAlongShaft1 = Dot(endToSphere1,unitAxis);
		const ScalarV distanceAlongShaft2 = Dot(endToSphere2,unitAxis);
		const ScalarV distanceAlongShaft3 = Dot(endToSphere3,unitAxis);
		const ScalarV distanceAlongShaft4 = Dot(endToSphere4,unitAxis);
		const ScalarV distanceAlongShaft5 = Dot(endToSphere5,unitAxis);
		const ScalarV distanceAlongShaft6 = Dot(endToSphere6,unitAxis);
		const ScalarV distanceAlongShaft7 = Dot(endToSphere7,unitAxis);

		const ScalarV closestDistanceAlongShaft0 = Clamp(distanceAlongShaft0,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft1 = Clamp(distanceAlongShaft1,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft2 = Clamp(distanceAlongShaft2,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft3 = Clamp(distanceAlongShaft3,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft4 = Clamp(distanceAlongShaft4,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft5 = Clamp(distanceAlongShaft5,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft6 = Clamp(distanceAlongShaft6,_zero,capsuleLength);
		const ScalarV closestDistanceAlongShaft7 = Clamp(distanceAlongShaft7,_zero,capsuleLength);

		const Vec3V dir0 = SubtractScaled(endToSphere0,unitAxis,closestDistanceAlongShaft0);
		const Vec3V dir1 = SubtractScaled(endToSphere1,unitAxis,closestDistanceAlongShaft1);
		const Vec3V dir2 = SubtractScaled(endToSphere2,unitAxis,closestDistanceAlongShaft2);
		const Vec3V dir3 = SubtractScaled(endToSphere3,unitAxis,closestDistanceAlongShaft3);
		const Vec3V dir4 = SubtractScaled(endToSphere4,unitAxis,closestDistanceAlongShaft4);
		const Vec3V dir5 = SubtractScaled(endToSphere5,unitAxis,closestDistanceAlongShaft5);
		const Vec3V dir6 = SubtractScaled(endToSphere6,unitAxis,closestDistanceAlongShaft6);
		const Vec3V dir7 = SubtractScaled(endToSphere7,unitAxis,closestDistanceAlongShaft7);
#if 1
		// transposed is probably faster, even on xenon
		Vec4V dir_X0123,dir_Y0123,dir_Z0123;
		Vec4V dir_X4567,dir_Y4567,dir_Z4567;

		Transpose3x4to4x3(dir_X0123,dir_Y0123,dir_Z0123,dir0,dir1,dir2,dir3);
		Transpose3x4to4x3(dir_X4567,dir_Y4567,dir_Z4567,dir4,dir5,dir6,dir7);

		const Vec4V oneOverDistance_0123 = InvSqrt(AddScaled(AddScaled(Scale(dir_X0123,dir_X0123),dir_Y0123,dir_Y0123),dir_Z0123,dir_Z0123));
		const Vec4V oneOverDistance_4567 = InvSqrt(AddScaled(AddScaled(Scale(dir_X4567,dir_X4567),dir_Y4567,dir_Y4567),dir_Z4567,dir_Z4567));

		const Vec4V depth_0123 = Max(_zeroV,AddScaled(_neg1V,capsuleRadiusV,oneOverDistance_0123));
		const Vec4V depth_4567 = Max(_zeroV,AddScaled(_neg1V,capsuleRadiusV,oneOverDistance_4567));

		clothVertexPositions[i0] = AddScaled(localVertex0,dir0,depth_0123.GetX());
		clothVertexPositions[i1] = AddScaled(localVertex1,dir1,depth_0123.GetY());
		clothVertexPositions[i2] = AddScaled(localVertex2,dir2,depth_0123.GetZ());
		clothVertexPositions[i3] = AddScaled(localVertex3,dir3,depth_0123.GetW());
		clothVertexPositions[i4] = AddScaled(localVertex4,dir4,depth_4567.GetX());
		clothVertexPositions[i5] = AddScaled(localVertex5,dir5,depth_4567.GetY());
		clothVertexPositions[i6] = AddScaled(localVertex6,dir6,depth_4567.GetZ());
		clothVertexPositions[i7] = AddScaled(localVertex7,dir7,depth_4567.GetW());
#else
		const ScalarV oneOverDistance0 = InvMag(dir0);
		const ScalarV oneOverDistance1 = InvMag(dir1);
		const ScalarV oneOverDistance2 = InvMag(dir2);
		const ScalarV oneOverDistance3 = InvMag(dir3);
		const ScalarV oneOverDistance4 = InvMag(dir4);
		const ScalarV oneOverDistance5 = InvMag(dir5);
		const ScalarV oneOverDistance6 = InvMag(dir6);
		const ScalarV oneOverDistance7 = InvMag(dir7);

		const ScalarV depth0 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance0));
		const ScalarV depth1 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance1));
		const ScalarV depth2 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance2));
		const ScalarV depth3 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance3));
		const ScalarV depth4 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance4));
		const ScalarV depth5 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance5));
		const ScalarV depth6 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance6));
		const ScalarV depth7 = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance7));

		clothVertexPositions[i0] = AddScaled(localVertex0,dir0,depth0);
		clothVertexPositions[i1] = AddScaled(localVertex1,dir1,depth1);
		clothVertexPositions[i2] = AddScaled(localVertex2,dir2,depth2);
		clothVertexPositions[i3] = AddScaled(localVertex3,dir3,depth3);
		clothVertexPositions[i4] = AddScaled(localVertex4,dir4,depth4);
		clothVertexPositions[i5] = AddScaled(localVertex5,dir5,depth5);
		clothVertexPositions[i6] = AddScaled(localVertex6,dir6,depth6);
		clothVertexPositions[i7] = AddScaled(localVertex7,dir7,depth7);
#endif

#else // not __XENON (dot product aren't fast)

		Vec4V localVertex_X0123,localVertex_Y0123,localVertex_Z0123;
		Vec4V localVertex_X4567,localVertex_Y4567,localVertex_Z4567;

		Transpose3x4to4x3(localVertex_X0123,localVertex_Y0123,localVertex_Z0123,localVertex0,localVertex1,localVertex2,localVertex3);
		Transpose3x4to4x3(localVertex_X4567,localVertex_Y4567,localVertex_Z4567,localVertex4,localVertex5,localVertex6,localVertex7);

		const Vec4V endToSphere_X0123 = Add(localVertex_X0123,parentOffsetMinusEnd0_X);
		const Vec4V endToSphere_Y0123 = Add(localVertex_Y0123,parentOffsetMinusEnd0_Y);
		const Vec4V endToSphere_Z0123 = Add(localVertex_Z0123,parentOffsetMinusEnd0_Z);
		const Vec4V endToSphere_X4567 = Add(localVertex_X4567,parentOffsetMinusEnd0_X);
		const Vec4V endToSphere_Y4567 = Add(localVertex_Y4567,parentOffsetMinusEnd0_Y);
		const Vec4V endToSphere_Z4567 = Add(localVertex_Z4567,parentOffsetMinusEnd0_Z);

		const Vec4V distanceAlongShaft_0123 = AddScaled(AddScaled(Scale(endToSphere_X0123,unitAxis_X),endToSphere_Y0123,unitAxis_Y),endToSphere_Z0123,unitAxis_Z);
		const Vec4V distanceAlongShaft_4567 = AddScaled(AddScaled(Scale(endToSphere_X4567,unitAxis_X),endToSphere_Y4567,unitAxis_Y),endToSphere_Z4567,unitAxis_Z);

		const Vec4V closestDistanceAlongShaft_0123 = Clamp(distanceAlongShaft_0123,_zeroV,capsuleLengthV);
		const Vec4V closestDistanceAlongShaft_4567 = Clamp(distanceAlongShaft_4567,_zeroV,capsuleLengthV);

		const Vec4V dir_X0123 = SubtractScaled(endToSphere_X0123,unitAxis_X,closestDistanceAlongShaft_0123);
		const Vec4V dir_Y0123 = SubtractScaled(endToSphere_Y0123,unitAxis_Y,closestDistanceAlongShaft_0123);
		const Vec4V dir_Z0123 = SubtractScaled(endToSphere_Z0123,unitAxis_Z,closestDistanceAlongShaft_0123);
		const Vec4V dir_X4567 = SubtractScaled(endToSphere_X4567,unitAxis_X,closestDistanceAlongShaft_4567);
		const Vec4V dir_Y4567 = SubtractScaled(endToSphere_Y4567,unitAxis_Y,closestDistanceAlongShaft_4567);
		const Vec4V dir_Z4567 = SubtractScaled(endToSphere_Z4567,unitAxis_Z,closestDistanceAlongShaft_4567);

		const Vec4V oneOverDistance_0123 = InvSqrt(AddScaled(AddScaled(Scale(dir_X0123,dir_X0123),dir_Y0123,dir_Y0123),dir_Z0123,dir_Z0123));
		const Vec4V oneOverDistance_4567 = InvSqrt(AddScaled(AddScaled(Scale(dir_X4567,dir_X4567),dir_Y4567,dir_Y4567),dir_Z4567,dir_Z4567));

		const Vec4V depth_0123 = Max(_zeroV,AddScaled(_neg1V,capsuleRadiusV,oneOverDistance_0123));
		const Vec4V depth_4567 = Max(_zeroV,AddScaled(_neg1V,capsuleRadiusV,oneOverDistance_4567));

		localVertex_X0123 = AddScaled(localVertex_X0123,dir_X0123,depth_0123);
		localVertex_Y0123 = AddScaled(localVertex_Y0123,dir_Y0123,depth_0123);
		localVertex_Z0123 = AddScaled(localVertex_Z0123,dir_Z0123,depth_0123);
		localVertex_X4567 = AddScaled(localVertex_X4567,dir_X4567,depth_4567);
		localVertex_Y4567 = AddScaled(localVertex_Y4567,dir_Y4567,depth_4567);
		localVertex_Z4567 = AddScaled(localVertex_Z4567,dir_Z4567,depth_4567);

		Transpose4x3to3x4(clothVertexPositions[i0],clothVertexPositions[i1],clothVertexPositions[i2],clothVertexPositions[i3],localVertex_X0123,localVertex_Y0123,localVertex_Z0123);
		Transpose4x3to3x4(clothVertexPositions[i4],clothVertexPositions[i5],clothVertexPositions[i6],clothVertexPositions[i7],localVertex_X4567,localVertex_Y4567,localVertex_Z4567);

#endif // not __XENON
	}
	for ( ; i < nVert; ++i )
	{
		// NOTE: character cloth is simulated in local space ( but is oriented ) when doing collision verts need to be translated to world space
		const Vec3V localVertex = clothVertexPositions[i];
		const Vec3V clothVertex = Add(localVertex,parentOffset);
		const Vec3V endToSphere = Add(localVertex,parentOffsetMinusEnd0);
		const ScalarV distanceAlongShaft = Dot(endToSphere,unitAxis);
		const ScalarV closestDistanceAlongShaft = Clamp(distanceAlongShaft,_zero,capsuleLength);
		const Vec3V dir = SubtractScaled(endToSphere,unitAxis,closestDistanceAlongShaft);
		const ScalarV oneOverDistance = InvMag(dir);
		const ScalarV depth = Max(_zero,AddScaled(_neg1,capsuleRadius,oneOverDistance));
		clothVertexPositions[i] = AddScaled(localVertex,dir,depth);
	}

#else // FAST_

	// 1st Pass - Build a list of all the verts close enough to this bound to really care about

#if __SPU
	sysScratchScope s;
	int* pCulled = sysScratchAllocObj<int>(nVert);
#else
	int* RESTRICT pCulled = Alloca(int,nVert);
#endif

	const int remaining = QuickRejectList(nVert, combinedRadiusSquared, boundCentroid, clothVertexPositions, pCulled, nPins, parentOffset);

	// 2nd Pass - Build a list of all the verts that actually collide
#if __SPU
	Vec3V* pCullDir2 = sysScratchAllocObj<Vec3V>(remaining);
	ScalarV* pCullDistance2 = sysScratchAllocObj<ScalarV>(remaining);
	int* pCulled2 = sysScratchAllocObj<int>(remaining);
#else

	Vec3V* RESTRICT pCullDir2 = Alloca(Vec3V,remaining);
	ScalarV* RESTRICT pCullDistance2 = Alloca(ScalarV,remaining);
	int* RESTRICT pCulled2 = Alloca(int,remaining);
#endif

	ScalarV _zero(V_ZERO);
	int nPass2Count = 0;

	for (int culledVertexIndex=0; culledVertexIndex<remaining; culledVertexIndex++)
	{
		const int vertexIndex = pCulled[culledVertexIndex];
		// Find the point on the shaft that is closest to the sphere.

		// NOTE: character cloth is simulated in local space ( but is oriented ) when doing collision verts need to be translated to world space
		const Vec3V clothVertex = Add( clothVertexPositions[vertexIndex], parentOffset ) ;

		Vec3V endToSphere = Subtract(clothVertex,end0);
		ScalarV closestPointAlongShaft = Dot(endToSphere,unitAxis);

		BoolV test1 = IsLessThan(capsuleLength,closestPointAlongShaft);
		BoolV test2 = IsGreaterThan(ScalarV(V_ZERO),closestPointAlongShaft);
		closestPointAlongShaft = SelectFT(test2,SelectFT(test1,closestPointAlongShaft,capsuleLength),_zero);
		Vec3V closestPointOnShaft = AddScaled(end0,unitAxis,closestPointAlongShaft);


		// See if that closest point is close enough to the sphere.
		Vec3V dir = Subtract(clothVertex,closestPointOnShaft);
		ScalarV distance2 = MagSquared(dir);

		u32 r0;
		SCALARV_TO_VECTOR3(distance2).IsLessThanVR4(SCALARV_TO_VECTOR3(combinedCapsuleRadius2),r0);
		r0 &= VEC3_CMP_VAL;
		r0 >>= 7;

		pCullDir2[nPass2Count] = dir;
		pCullDistance2[nPass2Count] = distance2;
		pCulled2[nPass2Count] = vertexIndex;
		nPass2Count += r0;
	}

	// 3rd Pass - Resolve all the collisions from pass2
	for (int collidingVertexIndex=0; collidingVertexIndex<nPass2Count; collidingVertexIndex++)
	{
		const int vertexIndex = pCulled2[collidingVertexIndex];
		Vec3V normal = Scale( pCullDir2[collidingVertexIndex], InvSqrt( pCullDistance2[collidingVertexIndex] ) ) ;
		ScalarV distance = Sqrt( pCullDistance2[collidingVertexIndex] );
		ScalarV depth = Max(Subtract(capsuleRadius,distance),_zero);
		clothVertexPositions[vertexIndex] = AddScaled( clothVertexPositions[vertexIndex], normal, depth );
	}

#if __SPU
	Assert( pCulled );
	sysScratchReset( (u8*)pCulled );
#endif

#endif // FAST_DetectAndEnforceOneCapsule


	if( GetFlag(FLAG_IS_ROPE) || bCollideEdges )
	{
		const phEdgeData* pEdgeData = &(GetEdge(0));

		Vec3V capsuleEnd0 = SubtractScaled(boundCentroid,unitAxis,Scale(ScalarV(V_HALF),capsuleLength));
		Vec3V capsuleEnd1 = AddScaled(boundCentroid,unitAxis,Scale(ScalarV(V_HALF),capsuleLength));

// #if DRAW_ALL_COLLISION_BOUNDS && 0
// 		if( canDrawVerletCollision )
// 		{
// 			grcColor(Color_blue);
// 			grcDrawSphere( 0.05f, capsuleEnd0 );
// 			grcDrawSphere( 0.05f, capsuleEnd1 );
// 		}
// #endif

// TODO: we might want to unroll this for better performance ... 
// get some real env cloth capsule custom bounds cases in game !
// -svetli

		for( int i = 0; i < m_NumEdges; ++i)
		{
			const phEdgeData* RESTRICT edge = &pEdgeData[i];
			const u16 edgeVert0 = edge->m_vertIndices[0];
			const u16 edgeVert1 = edge->m_vertIndices[1];

			Vec3V v0 = clothVertexPositions[edgeVert0];
			Vec3V v1 = clothVertexPositions[edgeVert1];

			Vec3V vertexPos0 = Add(v0, parentOffset);
			Vec3V vertexPos1 = Add(v1, parentOffset);

			Vec3V pointA2 = vertexPos1 - vertexPos0;

			Vec3V pointB1 = capsuleEnd0 - vertexPos0;
			Vec3V pointB2 = capsuleEnd1 - vertexPos0;

			Vec3V pointA, pointB;
			geomPoints::FindClosestSegSegToSeg(pointA.GetIntrin128Ref(), pointB.GetIntrin128Ref(), pointA2.GetIntrin128(), pointB1.GetIntrin128(), pointB2.GetIntrin128());
		
 			Vec3V newSeg = Subtract(pointA,pointB);
 			Vec3V normalSeq = Normalize(newSeg);
 			ScalarV newSegMag = Mag(newSeg);
 			BoolV testCond = IsLessThan( Scale(newSegMag,newSegMag),combinedCapsuleRadius2);
			ScalarV diff = Subtract( capsuleRadius, newSegMag );			
 			ScalarV depthSeq = SelectFT( testCond, _zero, diff );

 			clothVertexPositions[edgeVert0] = AddScaled( v0, normalSeq, depthSeq );
 			clothVertexPositions[edgeVert1] = AddScaled( v1, normalSeq, depthSeq );

#if DRAW_EDGE_COLLISION
			if( canDrawVerletCollision && testCond.Getb() )
			{
				grcColor(Color_red);
				grcDrawSphere( 0.05f, Add(pointA, vertexPos0) );
				grcColor(Color_green);
				grcDrawSphere( 0.05f, Add(pointB, vertexPos0) );
			}
#endif
		}
	}

#if DRAW_ALL_COLLISION_BOUNDS
	if( canDrawVerletCollision )
	{
		PFD_VerletCollision.End();
	}
#endif
}


#ifdef __SNC__
#pragma control postopt=6		// Default for -Os and -O2
#endif


#if USED_IN_GTAV

inline void GetBoxVerts(Vec3V* RESTRICT boxVerts, Vec3V_In boxMin, Vec3V_In boxMax, Vec3V_In centerPos )
{
	const Vec3V vMin = Add(centerPos,boxMin);
	const Vec3V vMax = Add(centerPos,boxMax);

	boxVerts[0] = vMin;
	boxVerts[1] = GetFromTwo<Vec::X2,Vec::Y1,Vec::Z1>(vMin,vMax);
	boxVerts[2] = GetFromTwo<Vec::X2,Vec::Y2,Vec::Z1>(vMin,vMax);
	boxVerts[3] = GetFromTwo<Vec::X1,Vec::Y2,Vec::Z1>(vMin,vMax);
	boxVerts[4] = GetFromTwo<Vec::X1,Vec::Y1,Vec::Z2>(vMin,vMax);
	boxVerts[5] = GetFromTwo<Vec::X2,Vec::Y1,Vec::Z2>(vMin,vMax);
	boxVerts[6] = vMax;
	boxVerts[7] = GetFromTwo<Vec::X1,Vec::Y2,Vec::Z2>(vMin,vMax);
}

void phVerletCloth::DetectAndEnforceOneBox( const phBound* RESTRICT bound, Mat34V_In boundPose, Vec3V* RESTRICT pClothVerts, Vec3V* RESTRICT pPrevClothVerts )
{
	Assert(bound);

	Vec3V* RESTRICT clothVerts = m_ClothData.GetVertexPointer();
	Assert( clothVerts );

	Vec3V vBoxMin = bound->GetBoundingBoxMin();
	Vec3V vBoxMax = bound->GetBoundingBoxMax();	
	Vec3V vCentrePos = bound->GetCentroidOffset();

	Vec3V vBoxVerts[8];
	GetBoxVerts( vBoxVerts, vBoxMin, vBoxMax, vCentrePos );

	int boxTris[12][3] = {	{0, 2, 1}, 
							{0, 3, 2},
							{1, 6, 5}, 
							{1, 2, 6},
							{4, 3, 0}, 
							{4, 7, 3},
							{4, 1, 5}, 
							{4, 0, 1},
							{3, 6, 2}, 
							{3, 7, 6},
							{7, 5, 6}, 
							{7, 4, 5}};	

	for (int i=0; i<12; i++)
	{
		Vec3V vVtx0 = vBoxVerts[boxTris[i][0]];
		Vec3V vVtx1 = vBoxVerts[boxTris[i][1]];
		Vec3V vVtx2 = vBoxVerts[boxTris[i][2]];

		Vec3V vPolyNormal = Cross(vVtx1-vVtx0, vVtx2-vVtx0);
		vPolyNormal = Normalize(vPolyNormal);

		TriangleShape triangle(vVtx0, vVtx1, vVtx2);
		triangle.m_PolygonNormal = vPolyNormal;
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
// 		triangle.m_EdgeNormals[0] = vPolyNormal;
// 		triangle.m_EdgeNormals[1] = vPolyNormal;
// 		triangle.m_EdgeNormals[2] = vPolyNormal;
#endif
		triangle.SetMargin(TRIANGLE_MARGIN);


		const int beginVert = GetNumLockedEdgesFront();
		const int endVert = GetNumVertices() - GetNumLockedEdgesBack();

		TriangleTransformed tTriangle;
		Vec3V pt0, pt1, pt2, triNormal, triPos;
		TransformTriangle(&triangle, boundPose, tTriangle.pt[0], tTriangle.pt[1], tTriangle.pt[2], tTriangle.n, tTriangle.pos);
		
		for( int vertIndex = beginVert; vertIndex < endVert; ++vertIndex )
		{
			if( DetectAndEnforceOneTriangle( pClothVerts[vertIndex], tTriangle.pt[0], tTriangle.pt[1], tTriangle.pt[2], tTriangle.n, tTriangle.pos ) )
			{
				pPrevClothVerts[vertIndex] = pClothVerts[vertIndex];
			}
		}			

#if !__SPU
		triangle.Release(false);
#endif
	}
}


void phVerletCloth::DetectAndEnforceOneGeometry( const phBoundGeometry* RESTRICT bound, Mat34V_In boundPose, Vec3V* RESTRICT pClothVerts, Vec3V* RESTRICT pPrevClothVerts )
{
	PF_FUNC(ClothCollisionsGeometry);
	Assert(bound);

	Vec3V* RESTRICT clothVerts = m_ClothData.GetVertexPointer();
	Assert( clothVerts );

	const int numPolygons = bound->GetNumPolygons();
	if( !numPolygons )			// composite bounds might have geometry bound without polys ?!?
	{
		clothErrorf("There are no polygons in the collision geometry !");
		return;
	}
	
#if __SPU
	phPolygon* polygons = sysScratchAllocObj<phPolygon>(numPolygons);
	sysDmaLargeGetAndWait(polygons, (uint64_t)bound->m_Polygons, numPolygons * sizeof(phPolygon), 0);
	Assert( polygons );
#endif

	ScalarV _zero(V_ZERO);

	for( int polygonIndex = 0; polygonIndex < numPolygons; ++polygonIndex )
	{
#if __SPU
		const phPolygon& polygon = polygons[polygonIndex];
#else
		const phPolygon& polygon = bound->GetPolygon(polygonIndex);
#endif
		const int triVertIndex0 = polygon.GetVertexIndex(0);
		const int triVertIndex1 = polygon.GetVertexIndex(1);
		const int triVertIndex2 = polygon.GetVertexIndex(2);

#if COMPRESSED_VERTEX_METHOD == 0
		const Vec3V triVert0 = bound->GetVertex(triVertIndex0);
		const Vec3V triVert1 = bound->GetVertex(triVertIndex1);
		const Vec3V triVert2 = bound->GetVertex(triVertIndex2);
#else

#if __SPU
		const Vec3V triVert0 = GetCompressedVertexSPU( bound, triVertIndex0 );
		const Vec3V triVert1 = GetCompressedVertexSPU( bound, triVertIndex1 );
		const Vec3V triVert2 = GetCompressedVertexSPU( bound, triVertIndex2 );
#else
		const Vec3V triVert0 = bound->GetCompressedVertex(triVertIndex0);
		const Vec3V triVert1 = bound->GetCompressedVertex(triVertIndex1);
		const Vec3V triVert2 = bound->GetCompressedVertex(triVertIndex2);
#endif
#endif

// Note: degenerated triangles might exist and those need to be filtered out
// Currently disabled
// 		VecBoolV res = Or( IsEqual(VECTOR3_TO_VEC3V(triVert0), VECTOR3_TO_VEC3V(triVert1)), IsEqual(VECTOR3_TO_VEC3V(triVert0), VECTOR3_TO_VEC3V(triVert2)) );
// 		if( IsEqualIntAll(res, VecBoolV(V_T_T_T_T)) != 0 )
// 			continue;

		const Vec3V polyNormal = polygon.ComputeUnitNormal(triVert0,triVert1,triVert2);
		TriangleShape triangle(triVert0,triVert1,triVert2);
		triangle.m_PolygonNormal = polyNormal;
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
//		triangle.m_EdgeNormals[0] = polyNormal;
//		triangle.m_EdgeNormals[1] = polyNormal;
//		triangle.m_EdgeNormals[2] = polyNormal;
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
		triangle.SetMargin(TRIANGLE_MARGIN);

		TriangleTransformed tTriangle;
		Vec3V pt0, pt1, pt2, triNormal, triPos;
		TransformTriangle(&triangle, boundPose, tTriangle.pt[0], tTriangle.pt[1], tTriangle.pt[2], tTriangle.n, tTriangle.pos);

		const int beginVert = GetNumLockedEdgesFront();
		const int endVert = GetNumVertices() - GetNumLockedEdgesBack();

		for( int vertIndex = beginVert; vertIndex < endVert; ++vertIndex )
		{
			if( DetectAndEnforceOneTriangle( pClothVerts[vertIndex], tTriangle.pt[0], tTriangle.pt[1], tTriangle.pt[2], tTriangle.n, tTriangle.pos ) )
			{
				pPrevClothVerts[vertIndex] = pClothVerts[vertIndex];
			}
		}			

#if !__SPU
		triangle.Release(false);
#endif

	}

#if __SPU
	Assert( polygons );
	sysScratchReset( (u8*)polygons );
#endif
}
#endif //USED_IN_GTAV

#if USE_TAPERED_CAPSULE
void phVerletCloth::DetectAndEnforceOneTaperedCapsule(const phBoundTaperedCapsule* RESTRICT bound, Mat34V_In boundPose )
{
	PF_FUNC(ClothCollisionsTaperedCapsule);

	Vec3V* RESTRICT clothVertexPositions = m_ClothData.GetVertexPointer();
	Assert( clothVertexPositions );

	ScalarV boundRadius = bound->GetRadiusAroundCentroidV();
	ScalarV combinedRadiusSquared = Scale(boundRadius,boundRadius);

	// Get the bound centroid.
	// The centroid is the center of the axis, which is only the center of the bound if the capsule is not really tapered.
	Vec3V boundCentroid = bound->GetWorldCentroid(boundPose);

	// Get the axis direction (from end 2 to end 1), the length and the two radii.
	Vec3V unitAxis = boundPose.GetCol1();
	ScalarV axisLength = bound->GetAxisLength();
	ScalarV inverseLength = InvertSafe(axisLength);
	ScalarV radius1 = bound->GetRadius1();
	ScalarV radius2 = bound->GetRadius2();

	// The bottom end has radius2, and the top end has radius1.
	Vec3V bottomEnd = SubtractScaled(boundCentroid,unitAxis,Scale(ScalarV(V_HALF),axisLength));

	ScalarV sine = bound->GetSine();
	ScalarV cosine = bound->GetCosine();
	ScalarV tangent = Scale(sine,InvertSafe(cosine));

	const int nVert = GetNumVertices();
#if NO_PIN_VERTS_IN_VERLET
	const int nPins = GetClothData().GetNumPinVerts();
#else
	const int nPins = GetPinCount();
#endif

	// Cull test all of the verts vs the bound
#if __SPU
	sysScratchScope s;
	int* pCulled = sysScratchAllocObj<int>(nVert);
#else
	int* RESTRICT pCulled = Alloca(int, nVert);
#endif

	const int nRemaining = QuickRejectList(nVert, combinedRadiusSquared, boundCentroid, clothVertexPositions, pCulled, 0, Vec3V(V_ZERO));

#if __SPU
	Vec4V* normalAndDepthList = sysScratchAllocObj<Vec4V>(nRemaining);
#else
	Vec4V* RESTRICT normalAndDepthList = Alloca(Vec4V,nRemaining);
#endif

	ScalarV scalarZero = ScalarV(V_ZERO);
	ScalarV scalarOne = ScalarV(V_ONE);
	int numCollisions = 0;
	for( int i = 0; i < nRemaining; i++ )
	{
		const int vertexIndex = pCulled[i];
		if ( vertexIndex < nPins )
		{
			continue;
		}

		// Find the point on the shaft that is closest to the vertex.
		Vec3V vertexPosition = clothVertexPositions[vertexIndex];
		Vec3V bottomEndToVertex = Subtract(vertexPosition,bottomEnd);
		ScalarV distanceAlongAxis = Dot(bottomEndToVertex,unitAxis);

		// Find the distance to the axis, and move the computed distance along the axis to the point that is in from the tapered capsule surface normal.
		Vec3V pointOnAxis = AddScaled(bottomEnd,unitAxis,distanceAlongAxis);
		ScalarV distanceToAxis = Dist(vertexPosition,pointOnAxis);
		distanceAlongAxis = AddScaled(distanceAlongAxis,distanceToAxis,tangent);

		// Find the radius at that point.
		ScalarV fractionAlongAxis = Scale(distanceAlongAxis,inverseLength);
		ScalarV radiusDiff = Subtract(radius1,radius2);
		ScalarV taperedRadius = AddScaled(radius2,fractionAlongAxis,radiusDiff);

		// Clamp the distance long the axis to the top point (the axis length).
		BoolV isUnderTop = IsLessThan(distanceAlongAxis,axisLength);
		distanceAlongAxis = SelectFT(isUnderTop,axisLength,distanceAlongAxis);
		taperedRadius = SelectFT(isUnderTop,radius1,taperedRadius);

		// Clamp the distance along the axis to the bottom point (zero).
		BoolV isUnderBottom = IsLessThan(distanceAlongAxis,scalarZero);
		distanceAlongAxis = SelectFT(isUnderBottom,distanceAlongAxis,scalarZero);
		taperedRadius = SelectFT(isUnderBottom,taperedRadius,radius2);

		// Find the colliding point on the axis (not the closest point).
		pointOnAxis = AddScaled(bottomEnd,unitAxis,distanceAlongAxis);

		// Find the normal and the squared distance from the colliding point.
		Vec3V normal = Subtract(vertexPosition,pointOnAxis);
		ScalarV distance2 = MagSquared(normal);

		// See if that closest point is close enough to the vertex.
		ScalarV squaredRadius = Scale(taperedRadius,taperedRadius);
		if (IsGreaterThanAll(squaredRadius,distance2))
		{
			pCulled[numCollisions] = vertexIndex;
			ScalarV depth = Subtract(taperedRadius,SqrtSafe(distance2));
			normalAndDepthList[numCollisions] = Vec4V(normal,depth);
			numCollisions++;
		}
	}

	ScalarV distance2,taperedRadius,distanceMod,normalMod;
	for( int i = 0; i < numCollisions; i++ )
	{
		const int vertexIndex = pCulled[i];	
		// Get the non-unit normal from the list and normalize it.
		Vec3V unitNormal = NormalizeSafe(normalAndDepthList[i].GetXYZ(), Vec3V(V_X_AXIS_WZERO));

		// Get the depth from the list and clamp it.
		ScalarV depth = SplatW(normalAndDepthList[i]);
		BoolV negativeDepth = IsLessThan(depth,scalarZero);
		depth = SelectFT(negativeDepth,depth,scalarZero);
		clothVertexPositions[vertexIndex] = AddScaled( clothVertexPositions[vertexIndex], unitNormal, depth );
	}

#if __SPU
	Assert( pCulled );
	sysScratchReset( (u8*)pCulled );
#endif
}
#endif // USE_TAPERED_CAPSULE

void phVerletCloth::DetectAndEnforceInstance( const bool bCollideEdges, Mat34V_In attachedFrame, const phInst *otherInstance, const phBound* customBound, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset )
{
	const phBound* otherBound = customBound;
	Mat34V instMat = attachedFrame;

#if __SPU
	if( !customBound )
	{
		Assert( otherInstance );

		u8 instBuffer[sizeof(phInst)] ;
		//	u8* instBuffer = (u8*)sysScratchAllocObj<phInst>(1);		// allocate from the scratch pad not the stack

		sysDmaGetAndWait(instBuffer, (u32)otherInstance, sizeof(phInst), 0);
		otherInstance = (phInst*)instBuffer;
		if( !otherInstance->GetArchetype() )
		{
			clothErrorf(" why there is no archetype ?" );
			return;
		}
		otherBound = (const phBound*)sysDmaGetUInt32((u32)((phInst*)otherInstance)->GetArchetype()->GetBoundPtr(), 0);
	}

	u8 boundBuffer1[sizeof(phBound)] ;
	//	u8* boundBuffer = (u8*)sysScratchAllocObj<phBound>(1);		// allocate from the scratch pad not the stack
	sysDmaGetAndWait(boundBuffer1, (u32)otherBound, sizeof(phBound), 0);

	const int boundType = ((const phBound*)boundBuffer1)->GetType();
	const int boundSize = g_BoundSizeTable[boundType];
	if( !boundSize )
	{
#if 0//!__DEV
		switch( boundType )
		{
		case phBound::SPHERE:			Displayf("Cloth is colliding with unsupported bound of type phBoundSphere"); break;
		case phBound::CAPSULE:			Displayf("Cloth is colliding with unsupported bound of type phBoundCapsule"); break;
 #if USE_TAPERED_CAPSULE
		case phBound::TAPERED_CAPSULE:	Displayf("Cloth is colliding with unsupported bound of type phBoundTaperedCapsule"); break;
 #endif
		case phBound::BOX:				Displayf("Cloth is colliding with unsupported bound of type phBoundBox"); break;
		case phBound::GEOMETRY:			Displayf("Cloth is colliding with unsupported bound of type phBoundGeometry"); break;
 #if USE_GEOMETRY_CURVED
		case phBound::GEOMETRY_CURVED:	Displayf("Cloth is colliding with unsupported bound of type phBoundCurvedGeom"); break;
 #endif
 #if USE_GRIDS
		case phBound::GRID:				Displayf("Cloth is colliding with unsupported bound of type phBoundGrid"); break;
 #endif
 #if USE_RIBBONS
		case phBound::RIBBON:			Displayf("Cloth is colliding with unsupported bound of type phBoundRibbon"); break;
 #endif
		case phBound::BVH:				Displayf("Cloth is colliding with unsupported bound of type phBoundBVH"); break;
 #if USE_SURFACES
		case phBound::SURFACE:			Displayf("Cloth is colliding with unsupported bound of type phBoundSource"); break;
 #endif
		case phBound::COMPOSITE:		Displayf("Cloth is colliding with unsupported bound of type phBoundComposite"); break;
		case phBound::TRIANGLE:			Displayf("Cloth is colliding with unsupported bound of type TriangleShape"); break;
		};
#endif
		return;
	}

	u8 boundBuffer[boundSize] ;
	sysDmaGetAndWait(boundBuffer, (u32)otherBound, boundSize, 0);
	otherBound = (const phBound*)boundBuffer;

	if( !customBound )
	{
		if(otherInstance->HasLastMatrix())
		{
			u32 offset16 = (u32)(g_instLvlIdxToMtxAddrMM + otherInstance->GetLevelIndex()) & 0xF;
			u8 instMatIdx = *((u8*)&instMat + offset16);
			Mat34V* mtxAddrMM = g_instLastMtxAddrMM + instMatIdx;
			sysDmaGetAndWait(&instMat, (uint64_t)mtxAddrMM, sizeof(Mat34V), 1);
		}
		else
			instMat = otherInstance->GetMatrix();
	}

#else
	if( !customBound )
	{
		Assert( otherInstance );
		if( !otherInstance->GetArchetype() )
			return;

		otherBound = otherInstance->GetArchetype()->GetBound();
		if(otherInstance->HasLastMatrix())
			instMat = PHLEVEL->GetLastInstanceMatrix(otherInstance);
		else
			instMat = otherInstance->GetMatrix();
	}
#endif

	DetectAndEnforceOne( bCollideEdges, otherBound, instMat, customMatrices, parentOffset );
}

// NOTE: the assumption here is that p on or close( very ) to the plane defined by a, b, c
bool PointOnTriangle(Vec3V_In p, Vec3V_In _a, Vec3V_In _b, Vec3V_In _c)
{
	Vec3V a = Subtract( _a, p );
	Vec3V b = Subtract( _b, p );
	Vec3V c = Subtract( _c, p );

	ScalarV ab = Dot( a, b );
	ScalarV ac = Dot( a, c );
	ScalarV bc = Dot( b, c );
	ScalarV cc = Dot( c, c );

	ScalarV _zero = ScalarV(V_ZERO);
	if( IsGreaterThanAll( _zero, Subtract( Scale(bc,ac), Scale(cc,ab) ) ) != 0 )
		return false;

	ScalarV bb = Dot( b, b );
	if( IsGreaterThanAll( _zero, Subtract( Scale(ab,bc), Scale(ac,bb) ) ) != 0 )
		return false;

	return true;
}


void phVerletCloth::TransformTriangle( const TriangleShape* RESTRICT triangleBound, Mat34V_In boundPose, Vec3V_InOut a, Vec3V_InOut b, Vec3V_InOut c, Vec3V_InOut triangleNormal, Vec3V_InOut trianglePosition )
{
	Assert( triangleBound );

	a = Transform(boundPose,triangleBound->m_vertices1[0]);
	b = Transform(boundPose,triangleBound->m_vertices1[1]);
	c = Transform(boundPose,triangleBound->m_vertices1[2]);

	ScalarV margin = triangleBound->GetMarginV();

	// NOTE: I assume normal is not 0,0,0 
	triangleNormal = Transform3x3(boundPose,triangleBound->m_PolygonNormal);
	trianglePosition = AddScaled(a, triangleNormal, margin);

// #if __PFDRAW
// 	if( PFD_VerletCollision.Begin(true) )
// 	{		
// 		grcDrawLine( VEC3V_TO_VECTOR3(a), VEC3V_TO_VECTOR3(b), Color_red );
// 		grcDrawLine( VEC3V_TO_VECTOR3(b), VEC3V_TO_VECTOR3(c), Color_red );
// 		grcDrawLine( VEC3V_TO_VECTOR3(c), VEC3V_TO_VECTOR3(a), Color_red );
// 		grcDrawLine( VEC3V_TO_VECTOR3(trianglePosition), VEC3V_TO_VECTOR3( Add(triangleNormal, trianglePosition) ), Color_red );		
// 		PFD_VerletCollision.End();
// 	}
// #endif

}

bool phVerletCloth::DetectAndEnforceOneTriangle( Vec3V_InOut vtx, Vec3V_In a, Vec3V_In b, Vec3V_In c, Vec3V_In triangleNormal, Vec3V_In trianglePosition )
{
	ScalarV d = Subtract( Dot(triangleNormal, trianglePosition), Dot(triangleNormal, vtx) );
	if( IsLessThanAll( d, ScalarV(V_ZERO) ) == 0 )
	{
		// NOTE: the dirty trick here is that PointOnTriangle assume vtx is very close to the plane defined by the 3 verts
		const bool res = PointOnTriangle( vtx, a, b, c );
		if( res )
		{
			vtx = Add(vtx, Scale(triangleNormal,d));	// use the projected vtx on the plane if there is penetration

	#if ENABLE_SPU_DEBUG_DRAW	
			SpuDebugTriangle( (void*)&a, (void*)&b, (void*)&c );
	#endif

	#if __PFDRAW
			if( PFD_VerletCollision.Begin(true) )
			{		
				grcDrawLine( VEC3V_TO_VECTOR3(a), VEC3V_TO_VECTOR3(b), Color_red );
				grcDrawLine( VEC3V_TO_VECTOR3(b), VEC3V_TO_VECTOR3(c), Color_red );
				grcDrawLine( VEC3V_TO_VECTOR3(c), VEC3V_TO_VECTOR3(a), Color_red );
				grcDrawLine( VEC3V_TO_VECTOR3(trianglePosition), VEC3V_TO_VECTOR3( Add(triangleNormal, trianglePosition) ), Color_red );
				grcDrawSphere( 0.05f, VEC3V_TO_VECTOR3(vtx) );			
				PFD_VerletCollision.End();
			}
	#endif
			return true;
		}
	}
	return false;
}


void phVerletCloth::DetectAndEnforceOnePlane( Vec3V_InOut vtx, Vec3V_In planeNormal, Vec3V_In planePos, Vec3V_In parentOffset )
{
	Vec3V v = Add( vtx, parentOffset );
	ScalarV signOnly = Dot( Subtract(v,planePos), planeNormal );
	ScalarV d = Subtract( Dot(planeNormal, planePos), Dot(planeNormal, v) );

	vtx = SelectFT( IsLessThan( signOnly, ScalarV(V_ZERO) ), vtx, AddScaled( vtx, planeNormal, d ) );	// use the projected vtx on the plane if there is penetration

#if 0 //__PFDRAW
	Vec3V a = Transform(boundPose,triangleBound->m_vertices1[0]);
	Vec3V b = Transform(boundPose,triangleBound->m_vertices1[1]);
	Vec3V c = Transform(boundPose,triangleBound->m_vertices1[2]);

	if( PFD_VerletCollision.Begin(true) )
	{		
		grcDrawLine( VEC3V_TO_VECTOR3(a), VEC3V_TO_VECTOR3(b), Color_red );
		grcDrawLine( VEC3V_TO_VECTOR3(b), VEC3V_TO_VECTOR3(c), Color_red );
		grcDrawLine( VEC3V_TO_VECTOR3(c), VEC3V_TO_VECTOR3(a), Color_red );
		grcDrawSphere( 0.05f, vtx );
		PFD_VerletCollision.End();
	}

#if ENABLE_SPU_DEBUG_DRAW	
	SpuDebugTriangle( (void*)&a, (void*)&b, (void*)&c );
	SpuDebugSphere( (void*)&vtx );
#endif

#endif
}


void phVerletCloth::DetectAndEnforceList( Mat34V_In attachedFrame, phInstDatRefArray& collisionInst, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset )
{
	PF_FUNC(ClothCollisions);

	if( const phBound* pCustomBound = GetCustomBound() )
	{
		const bool bCollideEdges = IsCollideEdges();
		DetectAndEnforceInstance( bCollideEdges, attachedFrame, NULL, pCustomBound, customMatrices, parentOffset );
	}
	else
	{
		const int numCollidingObjects = collisionInst.GetCount();
		for (int objectIndex = 0; objectIndex < numCollidingObjects; objectIndex++)
		{
#if !__SPU
			const u32 fatID = collisionInst[objectIndex];
			const int levelIndex = fatID & 0xffff;
#if LEVELNEW_GENERATION_IDS
			const int genID = (fatID >> 16) & 0xffff;
			const bool checkCollision = (levelIndex != phInst::INVALID_INDEX && PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex, genID));		
#else
			const bool checkCollision = (levelIndex != phInst::INVALID_INDEX);		
#endif
#else
			const bool checkCollision = true;
#endif

			if(	checkCollision )
			{				
#if !__SPU
				const phInst *otherInstance = reinterpret_cast<const phInst *>(PHLEVEL->GetInstance(levelIndex));	
#else
				const phInst *otherInstance = reinterpret_cast<const phInst *>(collisionInst[objectIndex]);	
#endif
				Assert(otherInstance);
				DetectAndEnforceInstance( GetFlag(FLAG_COLLIDE_EDGES), attachedFrame, otherInstance, NULL, NULL, parentOffset );
			}
		}
	}

	if( m_VirtualBound )
	{
		Assert( m_VirtualBoundMat );

#if __SPU
		Mat34V boundMat;
		// TODO: DMA m_VirtualBoundMat matrix early ??
		sysDmaGet( &boundMat, (u32)m_VirtualBoundMat, sizeof(Mat34V), 0);
		sysDmaWait( 1 << 0 );
#else
		Mat34V& boundMat = *((Mat34V*)(m_VirtualBoundMat));
#endif

		DetectAndEnforceInstance( GetFlag(FLAG_COLLIDE_EDGES), boundMat, NULL, m_VirtualBound, (GetFlag(FLAG_IGNORE_OFFSET) ? NULL: customMatrices), parentOffset );
	}

	if( m_PedBound0 )
	{
		Assert( m_PedBoundMatrix0 );

 #if __SPU
		Mat34V boundMat;
		// TODO: DMA m_PedBoundMatrix matrix early ??
		sysDmaGet( &boundMat, (u32)m_PedBoundMatrix0, sizeof(Mat34V), 0);
		sysDmaWait( 1 << 0 );
 #else
		Mat34V& boundMat = *((Mat34V*)(m_PedBoundMatrix0));
 #endif

		DetectAndEnforceInstance( GetFlag(FLAG_COLLIDE_EDGES), boundMat, NULL, m_PedBound0, (GetFlag(FLAG_IGNORE_OFFSET) ? NULL: customMatrices), parentOffset );
	}

	if( m_PedBound1 )
	{
		Assert( m_PedBoundMatrix1 );

 #if __SPU
		Mat34V boundMat;
		// TODO: DMA m_PedBoundMatrix matrix early ??
		sysDmaGet( &boundMat, (u32)m_PedBoundMatrix1, sizeof(Mat34V), 0);
		sysDmaWait( 1 << 0 );
 #else
		Mat34V& boundMat = *((Mat34V*)(m_PedBoundMatrix1));
 #endif

		DetectAndEnforceInstance( GetFlag(FLAG_COLLIDE_EDGES), boundMat, NULL, m_PedBound1, (GetFlag(FLAG_IGNORE_OFFSET) ? NULL: customMatrices), parentOffset );
	}
}


}  // namespace rage
