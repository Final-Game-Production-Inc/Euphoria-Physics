//
// pheffects/cloth_verlet.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "cloth_verlet.h"

#include "clothconnectivitydata.h"
#include "morphgeometry.h"
#include "tune.h"
#include "clothverletinst.h"
#include "pharticulated/articulatedbody.h"
#include "data/callback.h"
#include "data/safestruct.h"
#include "grmodel/geometry.h"
#include "math/random.h"
#include "math/simplemath.h"
#include "parser/restparserservices.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundgeom.h"
#include "phbound/boundsphere.h"
#include "phbound/boundtaperedcapsule.h"
#include "phbullet/triangleshape.h"
#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "physics/intersection.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "system/memory.h"
#include "system/task.h"
#include "system/timemgr.h"
#include "system/xtl.h"
#include "system/param.h"
#include "vector/geometry.h"
#include "vector/vector4.h"
#include "vectormath/classes.h"
#include "vectormath/classes_soa.h"
#include "vectormath/layoutconvert.h"

#include "system/timemgr.h"


#include "cloth_verlet_parser.h"


#if __XENON
#include <ppcintrinsics.h>
#endif

#define		BAD_INDEX_U16								0xffff
#define		USE_ALL_EDGES								0
#define		MIN_BEND_EDGE_STRENGTH_THRESHOLD			0.000001f


RAGE_DEFINE_CHANNEL(Cloth,rage::DIAG_SEVERITY_DISPLAY)


namespace rage {


PFD_DECLARE_GROUP_ON(DrawCloth);
PFD_DECLARE_ITEM(ClothVerts,Color_white,DrawCloth);
PFD_DECLARE_ITEM(ClothEdges,Color_white,DrawCloth);
PFD_DECLARE_ITEM(ClothCustomEdges,Color_white,DrawCloth);
PFD_DECLARE_ITEM(ClothInstanceCustomEdges,Color_white,DrawCloth);
PFD_DECLARE_ITEM(ClothVertIndices,Color_red,DrawCloth);
PFD_DECLARE_ITEM(ClothOriginalVertIndices,Color_red,DrawCloth);
PFD_DECLARE_ITEM(PickVerts,Color_red,DrawCloth);
PFD_DECLARE_ITEM(PickEdges,Color_red,DrawCloth);
PFD_DECLARE_ITEM(VertexValues,Color_red,DrawCloth);
PFD_DECLARE_ITEM(VerletCollision,Color_white,DrawCloth);
PFD_DECLARE_ITEM(VerletCollisionResponse,Color_white,DrawCloth);
PFD_DECLARE_ITEM(VerletBounds,Color_white,DrawCloth);
PFD_DECLARE_ITEM(ClothNormals,Color_red,DrawCloth);
PFD_DECLARE_ITEM(ClothDeltas,Color_green,DrawCloth);
PFD_DECLARE_ITEM(ClothFaceNormals,Color_green,DrawCloth);
PFD_DECLARE_ITEM(ClothTangents,Color_blue,DrawCloth);
PFD_DECLARE_ITEM(Ropes,Color_blue,DrawCloth);
PFD_DECLARE_ITEM(SpuDebug,Color_white,DrawCloth);
PFD_DECLARE_ITEM(SkinnedMesh,Color_white,DrawCloth);
PFD_DECLARE_ITEM(CustomBounds,Color_white,DrawCloth);
PFD_DECLARE_ITEM(DebugRecords,Color_white,DrawCloth);
PFD_DECLARE_ITEM(ClothGizmo,Color_white,DrawCloth);


//PFD_DECLARE_ITEM(TearEdges,Color_red,DrawCloth);


const char* phVerletCloth::sm_tuneFilePath = NULL;
const char* phVerletCloth::sm_MetaDataFileAppendix = "_edges";


#if __PFDRAW
void DrawEdgesColor( const Vec3V* RESTRICT vertexBuffer, const phVerletCloth& cloth, const Vector3& offset)
{
	if (PFD_ClothEdges.Begin(true))
	{
		static int r = 195;
		static int g = 175;
		static int b = 95;
		Color32 c(r,g,b);

		const int lastEdge = cloth.GetNumEdges() - cloth.GetNumLockedEdgesBack();
		const int firstEdge = cloth.GetNumLockedEdgesFront();
		const atArray<phEdgeData>& edgeData = cloth.GetEdgeList();
		for( int edgeIndex = firstEdge; edgeIndex < lastEdge; ++edgeIndex )
		{
			const phEdgeData& edge = edgeData[edgeIndex];
			int vert0Index = edge.m_vertIndices[0];
			int vert1Index = edge.m_vertIndices[1];

			Vector3 v0 = VEC3V_TO_VECTOR3( vertexBuffer[vert0Index] ) + offset;
			Vector3 v1 = VEC3V_TO_VECTOR3( vertexBuffer[vert1Index] ) + offset;
			grcDrawLine(v0,v1,c);
		}

		PFD_ClothEdges.End();
	}

}

void DrawEdges( const Vector3& viewPt, const phVerletCloth& cloth, const Vector3& offset, const Vec3V* RESTRICT vertexBuffer, const Vector3* RESTRICT normals, const int firstEdge, const int lastEdge, const float clothRadius, const float clothRadiusInv, const atArray<phEdgeData>& edgeData, const int minAlpha, const int maxAlpha, const bool enableAlpha, const bool flipAlpha, Color32 forceColor = Color_white )
{
	Assert( edgeData.GetCount() <= lastEdge );
	bool bForceColor = ( forceColor == Color_white ? false: true );

	for( int edgeIndex = firstEdge; edgeIndex < lastEdge; ++edgeIndex )
	{
		const phEdgeData& edge = edgeData[edgeIndex];
		const int vert0Index = edge.m_vertIndices[0];
		const int vert1Index = edge.m_vertIndices[1];

		Vector3 v0 = VEC3V_TO_VECTOR3( vertexBuffer[vert0Index] ) + offset;
		Vector3 v1 = VEC3V_TO_VECTOR3( vertexBuffer[vert1Index] ) + offset;

		// Note: performance edges, skip them
		if( vert0Index == vert1Index )
			continue;

		const bool isVert0Pinned = cloth.IsPinned(vert0Index);
		const bool isVert1Pinned = cloth.IsPinned(vert1Index);
		const bool atLeastOnePinned = !(isVert0Pinned || isVert1Pinned);

		const int g = 255 & (-(int)(atLeastOnePinned));
		const int b = 255 & (-(int)(atLeastOnePinned));
		int a0 = maxAlpha;
		int a1 = maxAlpha;

		if( normals )
		{
			Vector3 v0temp = viewPt - v0;
			Vector3 v1temp = viewPt - v1;
			v0temp.Normalize();
			v1temp.Normalize();

			float v0dot = v0temp.Dot( normals[vert0Index] );
			float v1dot = v1temp.Dot( normals[vert1Index] );
			if( v0dot > 0.0f && v1dot > 0.0f )
			{
				grcDrawLine(v0,v1,Color32(255,g,b));
			}
		}
		else
		{
			if( enableAlpha )
			{
				Vector3 v0temp = viewPt - v0;
				Vector3 v1temp = viewPt - v1;
				const float mag0clamped = Clamp( v0temp.Mag(), 0.0f, clothRadius );
				const float mag1clamped = Clamp( v1temp.Mag(), 0.0f, clothRadius );

				float distRatio0 = mag0clamped * clothRadiusInv;
				float distRatio1 = mag1clamped * clothRadiusInv;
				if( flipAlpha )
				{
					distRatio0 = 1.0f - distRatio0;
					distRatio1 = 1.0f - distRatio1;
				}

				a0 = (int)(distRatio0 * 255.0f);
				a1 = (int)(distRatio1 * 255.0f);
				a0 = Clamp( a0, minAlpha, maxAlpha );
				a1 = Clamp( a1, minAlpha, maxAlpha );
			}

			Color32 colorv0 = (bForceColor ? forceColor : Color32(255,g,b,a0));
			Color32 colorv1 = (bForceColor ? forceColor : Color32(255,g,b,a1));
			grcDrawLine(v0, v1, colorv0, colorv1);
		}
	}
}


void DrawClothWireframe( const Vec3V* RESTRICT vertexBuffer, const phVerletCloth& cloth, void* controllerAddress, const Vector3& offset, 
	const Vector3& viewPt, const bool useNormals, const bool enableAlpha, const bool flipAlpha, const float sphereRad, const Vector3& vertsColor )
{
	Assert( vertexBuffer );
	Assert( controllerAddress );
	char buf[16];
	sprintf( buf, "%p", controllerAddress );
	Vec3V clothCenter = Scale( Add(cloth.GetBBMax(), cloth.GetBBMin()), ScalarV(V_HALF) );
	grcDrawLabelf( (VEC3V_TO_VECTOR3(clothCenter)+offset), buf );

	const float clothRadius = Mag( Subtract(clothCenter,cloth.GetBBMax()) ).Getf();
	const float clothRadiusInv = FPInvertFast(clothRadius);

	// TODO: 
	// - get alpha values from RAG widgets
	const int minAlpha = 25;
	const int maxAlpha = 255;

	const Vector3* RESTRICT normals = useNormals ? (const Vector3* RESTRICT)cloth.GetClothData().GetNormalsPointer(): NULL;

	if( PFD_VerletBounds.Begin(true) )
	{
		grcDrawBox( VEC3V_TO_VECTOR3(cloth.GetBBMin())+offset, VEC3V_TO_VECTOR3(cloth.GetBBMax())+offset, Color_white );
		PFD_VerletBounds.End();
	}

	if (PFD_ClothVerts.Begin(true))
	{		
		grcColor(Color_red);
#if NO_PIN_VERTS_IN_VERLET
		const int numPinnedVerts = cloth.GetClothData().GetNumPinVerts();
#else
		const int numPinnedVerts = cloth.GetPinCount();
#endif
		for( int i = 0; i < numPinnedVerts; ++i )
		{
			Vec3V v = Add(vertexBuffer[i], VECTOR3_TO_VEC3V(offset));
			grcDrawSphere( sphereRad, v );
		}
		Color32 c(vertsColor);
		grcColor(c);
		for( int i = numPinnedVerts; i < cloth.GetNumVertices(); ++i )
		{
			grcDrawSphere( sphereRad, Add(vertexBuffer[i], VECTOR3_TO_VEC3V(offset)) );
		}
		PFD_ClothVerts.End();
	}

	if (PFD_ClothEdges.Begin(true))
	{
		const int lastEdge = cloth.GetNumEdges() - cloth.GetNumLockedEdgesBack();
		const int firstEdge = cloth.GetNumLockedEdgesFront();
		const atArray<phEdgeData>& edgeData = cloth.GetEdgeList();
		if( edgeData.GetCount() > 0 )
		{
			DrawEdges(viewPt, cloth, offset, vertexBuffer, normals, firstEdge, lastEdge, clothRadius, clothRadiusInv, edgeData, minAlpha, maxAlpha, enableAlpha, flipAlpha);
		}
		PFD_ClothEdges.End();
	}

	if (PFD_ClothCustomEdges.Begin(true))
	{
		const atArray<phEdgeData>& edgeData = cloth.GetCustomEdgeList();
		if( edgeData.GetCount() > 0 )
		{
			DrawEdges(viewPt, cloth, offset, vertexBuffer, normals, 0, edgeData.GetCount(), clothRadius, clothRadiusInv, edgeData, minAlpha, maxAlpha, enableAlpha, flipAlpha, Color_green1 );
		}
		PFD_ClothCustomEdges.End();
	}

	if (PFD_ClothInstanceCustomEdges.Begin(true))
	{
		const atArray<phEdgeData>& edgeData = cloth.GetInstanceCustomEdgeList();
		if( edgeData.GetCount() > 0 )
		{
			DrawEdges(viewPt, cloth, offset, vertexBuffer, normals, 0, edgeData.GetCount(), clothRadius, clothRadiusInv, edgeData, minAlpha, maxAlpha, enableAlpha, flipAlpha, Color_green);
		}
		PFD_ClothInstanceCustomEdges.End();
	}

	if (PFD_ClothVertIndices.Begin(true))
	{
		grcColor(Color_white);
		if( cloth.GetClothData().GetNormalsCount() > 0 )
		{
			const Vector3* RESTRICT normals = (Vector3*)cloth.GetClothData().GetNormalsPointer();
			Assert( normals );

#if NO_PIN_VERTS_IN_VERLET
			const int numPinnedVerts = cloth.GetClothData().GetNumPinVerts();
#else
			const int numPinnedVerts = cloth.GetPinCount();
#endif
			for( int i = 0; i < numPinnedVerts; ++i )
			{
				Vector3 v = VEC3V_TO_VECTOR3(vertexBuffer[i])+offset;
				char buf[16];
				formatf( buf, "%d", i );

				Vector3 vT = viewPt - v;					
				vT.Normalize();		
				if( vT.Dot(normals[i]) > 0.0f )
					grcDrawLabelf( v, buf );
			}

			for( int i = numPinnedVerts; i < cloth.GetNumVertices(); ++i)
			{
				Vector3 v = VEC3V_TO_VECTOR3(vertexBuffer[i])+offset;
				char buf[16];
				formatf( buf, "%d", i );

				Vector3 vT = viewPt - v;					
				vT.Normalize();		
				if( vT.Dot(normals[i]) > 0.0f )
					grcDrawLabelf( v, buf );
			}
		}
		else
		{
#if NO_PIN_VERTS_IN_VERLET
			const int numPinnedVerts = cloth.GetClothData().GetNumPinVerts();
#else
			const int numPinnedVerts = cloth.GetPinCount();
#endif
			for( int i = 0; i < numPinnedVerts; ++i )
			{
				Vector3 v = VEC3V_TO_VECTOR3(vertexBuffer[i])+offset;	
				char buf[16];
				formatf( buf, "%d", i );
				grcDrawLabelf( v, buf );
			}

			for( int i = numPinnedVerts; i < cloth.GetNumVertices(); ++i)
			{
				Vector3 v = VEC3V_TO_VECTOR3(vertexBuffer[i])+offset;
				char buf[16];
				formatf( buf, "%d", i );
				grcDrawLabelf( v, buf );
			}
		}
		PFD_ClothVertIndices.End();
	}
}

#endif //__PFDRAW

// NOTE: only for debug purposes
#if RECORDING_VERTS

void recordLine::Draw()
{
	grcDrawLine( VEC3V_TO_VECTOR3(m_V0), VEC3V_TO_VECTOR3(m_V1), Color_red );
}

void recordSphere::Draw()
{
	grcColor(m_Color);
	grcDrawSphere( m_Radius, m_Center );
}

void recordTriangle::Draw()
{
	grcDrawLine( VEC3V_TO_VECTOR3(m_V0), VEC3V_TO_VECTOR3(m_V1), Color_red );
	grcDrawLine( VEC3V_TO_VECTOR3(m_V0), VEC3V_TO_VECTOR3(m_V2), Color_red );
	grcDrawLine( VEC3V_TO_VECTOR3(m_V1), VEC3V_TO_VECTOR3(m_V2), Color_red );
}

void recordCapsule::Draw()
{
	grcDrawCapsule( m_Length, m_Radius, MAT34V_TO_MATRIX34(m_Transform), 8 );	
}

void recordCustomClothEvent::Draw(float x, float y)
{
	//	Assert( m_Frames > 0 );
	Assert( m_Text );
	Assert( m_ClothControllerName );
	const s32 width = grcDevice::GetWidth();
	const s32 height = grcDevice::GetHeight();

	char buff[256];
	if( m_EventType == 1 )
	{
		formatf(buff, "eventType = %d, pos: x=%f y=%f z=%f, rot: x=%f, y=%f, z=%f, rad = %f, len = %f   cloth: %s", 
			m_EventType, m_Position.GetXf(), m_Position.GetYf(), m_Position.GetZf(), m_Rotation.GetXf(), m_Rotation.GetYf(), m_Rotation.GetZf(), m_CapsuleRadius, m_CapsuleLength, m_ClothControllerName );
	}
	else if( m_EventType == 6 )
	{
		formatf(buff, "eventType = %d, pin radius set = %d   cloth:  %s", m_EventType, (u8)m_CapsuleLength, m_ClothControllerName );
	}
	else if( m_EventType == 12 )
	{
		formatf(buff, "eventType = %d, pose index = %d   cloth:  %s", m_EventType, (u8)m_CapsuleLength, m_ClothControllerName );
	}
	else
	{
		formatf(buff, "eventType = %d,   cloth:  %s", m_EventType, m_ClothControllerName );
	}

	grcColor(Color_white);
	grcDraw2dText( x*(float)width, y*(float)height, buff );
	//	m_Frames--;
}

#endif // RECORDING_VERTS

IMPLEMENT_PLACE(phEdgeData);

phEdgeData::phEdgeData(class datResource& UNUSED_PARAM(rsc) )
{

}


#if __DECLARESTRUCT
void phEdgeData::DeclareStruct(datTypeStruct &s)
{
	SSTRUCT_BEGIN(phEdgeData)
		SSTRUCT_CONTAINED_ARRAY(phEdgeData, m_vertIndices)
		SSTRUCT_FIELD(phEdgeData, m_EdgeLength2)
		SSTRUCT_FIELD(phEdgeData, m_Weight0)
		SSTRUCT_FIELD(phEdgeData, m_CompressionWeight)
		SSTRUCT_END(phEdgeData)
}
#endif

#if !NO_ROPE_WEIGHTS

void phEdgeData::SetRopeVertexWeights (ScalarV_In weight0, ScalarV_In weight1)
{
	// Set both vertices to be affected equally.
	m_RopeEdgeData.m_VertexWeights = 3;

	// Change the edge weights if one or both are not affected.
	ScalarV zero = ScalarV(V_ZERO);
	if (IsEqualAll(weight0,zero))
	{
		if (IsEqualAll(weight1,zero))
		{
			// Neither vertex is affected.
			m_RopeEdgeData.m_VertexWeights = 0;
		}
		else
		{
			// Only vertex 1 is affected.
			m_RopeEdgeData.m_VertexWeights = 2;
		}
	}
	else if (IsEqualAll(weight1,zero))
	{
		// Only vertex 0 is affected.
		m_RopeEdgeData.m_VertexWeights = 1;
	}
}

void phEdgeData::SetRopeVertexWeights (float weight0, float weight1)
{
	SetRopeVertexWeights(ScalarVFromF32(weight0),ScalarVFromF32(weight1));
}
#endif // !NO_ROPE_WEIGHTS


void phEdgeData::PinRopeVertex0 ()
{
#if !NO_ROPE_WEIGHTS
	if (m_RopeEdgeData.m_VertexWeights==1)
	{
		// Vertex 1 is already pinned (vert0 has weight 1 and vert1 has weight 0), so make them both pinned.
		m_RopeEdgeData.m_VertexWeights = 0;
	}
	else if (m_RopeEdgeData.m_VertexWeights==3)
	{
		// Neither vertex is already pinned, so make only vertex 0 pinned (vert0 gets weight 0 and vert1 gets weight 1).
		m_RopeEdgeData.m_VertexWeights = 2;
	}
	//	m_Weight0 = 0.0f;
#else
	if( m_Weight0 > 0.0f )
		m_Weight0 = 0.0f;
#endif
}


void phEdgeData::PinRopeVertex1 ()
{
#if !NO_ROPE_WEIGHTS
	if (m_RopeEdgeData.m_VertexWeights==2)
	{
		// Vertex 0 is already pinned (vert0 has weight 0 and vert1 has weight 1), so make them both pinned.
		m_RopeEdgeData.m_VertexWeights = 0;
	}
	else if (m_RopeEdgeData.m_VertexWeights==3)
	{
		// Neither vertex is already pinned, so make only vertex 1 pinned (vert0 gets weight 1 and vert1 gets weight 0).
		m_RopeEdgeData.m_VertexWeights = 1;
	}
	//	m_Weight0 = 1.0f;
#else
	if( m_Weight0 < 1.0f )
		m_Weight0 = 1.0f;
#endif
}


void phEdgeData::UnpinRopeVertex0 ()
{
#if !NO_ROPE_WEIGHTS
	if (m_RopeEdgeData.m_VertexWeights==0)
	{
		// Both vertices are already pinned (vert0 has weight 0 and vert1 has weight 0), so make vertex 1 pinned.
		m_RopeEdgeData.m_VertexWeights = 1;
	}
	else if (m_RopeEdgeData.m_VertexWeights==2)
	{
		// Vertex 0 is already pinned (vert0 has weight 0 and vert1 has weight 1), so make neither pinned.
		m_RopeEdgeData.m_VertexWeights = 3;
	}
#else
	if( m_Weight0 < 0.5f )
		m_Weight0 = 0.5f;
#endif
}


void phEdgeData::UnpinRopeVertex1 ()
{
#if !NO_ROPE_WEIGHTS
	if (m_RopeEdgeData.m_VertexWeights==0)
	{
		// Both vertices are already pinned (vert0 has weight 0 and vert1 has weight 0), so make vertex 0 pinned.
		m_RopeEdgeData.m_VertexWeights = 2;
	}
	else if (m_RopeEdgeData.m_VertexWeights==1)
	{
		// Vertex 1 is already pinned (vert0 has weight 1 and vert1 has weight 0), so make neither pinned.
		m_RopeEdgeData.m_VertexWeights = 3;
	}
#else
	if( !(m_Weight0 < 1.0f) )
		m_Weight0 = 0.5f;
#endif
}


// phVerletCloth

phVerletCloth::phVerletCloth(class datResource& rsc) 
	: 	m_DynamicPinList(rsc)
	,	m_BBMin(Vec3V(V_ZERO))
	,	m_BBMax(Vec3V(V_ZERO))
	,	m_ClothData(rsc)
	,	m_CustomEdgeData(rsc, true)
	,	m_EdgeData(rsc, true)
	,	m_CustomBound(rsc)
	,	m_VirtualBound(NULL)
	,	m_VirtualBoundMat( 0 )
{
	Assert16(m_CustomEdgeData.GetElements());
	if(datResource_IsDefragmentation)
	{
		clothDebugf1("Defrag phVerletCloth");
	}
#if NO_PIN_VERTS_IN_VERLET
	// TODO: temp here, remove it on resource bump !

	m_ClothData.SetNumPinVerts( *((int*)this->m_Pad001) /*m_NumPinnedVerts*/ );
	m_ClothData.SetNumVerts( *((int*)this->m_Pad002) /*m_NumVertices*/ );
#endif
}

#if __DECLARESTRUCT
void phVerletCloth::DeclareStruct(datTypeStruct &s)
{
	m_VerletClothType = NULL;
	if( m_CustomBound )
	{
		Assert( m_CustomBound->GetRefCount() == 1 );
	}

	pgBase::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(phVerletCloth, pgBase)

		SSTRUCT_IGNORE(phVerletCloth, m_VerletClothType)		
		SSTRUCT_FIELD(phVerletCloth, m_CustomBound)

#if NO_BOUND_CENTER_RADIUS
		SSTRUCT_CONTAINED_ARRAY(phVerletCloth, m_Pad007)
#else
		SSTRUCT_FIELD(phVerletCloth, m_BoundingCenterAndRadius)
#endif
		SSTRUCT_FIELD(phVerletCloth, m_BBMin)
		SSTRUCT_FIELD(phVerletCloth, m_BBMax)		
		SSTRUCT_FIELD(phVerletCloth, m_ClothData)	

		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad005 )

//		SSTRUCT_FIELD(phVerletCloth, m_GravityFactor)

		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad00 )
		SSTRUCT_FIELD(phVerletCloth, m_VirtualBound)
		
		
#if NO_PIN_VERTS_IN_VERLET
		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad001 )
#else
		SSTRUCT_FIELD(phVerletCloth, m_NumPinnedVerts)
#endif

		SSTRUCT_FIELD(phVerletCloth, m_NumEdges)

#if NO_VERTS_IN_VERLET
		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad002 )
#else
		SSTRUCT_FIELD(phVerletCloth, m_NumVertices)
#endif
		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad01 )
		SSTRUCT_FIELD(phVerletCloth, m_nIterations)

		SSTRUCT_FIELD(phVerletCloth, m_Flags)

		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad006 )

		SSTRUCT_FIELD(phVerletCloth, m_CustomEdgeData)	
		SSTRUCT_FIELD(phVerletCloth, m_EdgeData)
		SSTRUCT_FIELD(phVerletCloth, m_PedBound0)
		SSTRUCT_FIELD(phVerletCloth, m_PedBoundMatrix0)
		SSTRUCT_FIELD(phVerletCloth, m_CollisionInst)
		SSTRUCT_FIELD(phVerletCloth, m_DynamicPinList)	
		SSTRUCT_FIELD(phVerletCloth, m_NumLockedEdgesFront)
		SSTRUCT_FIELD(phVerletCloth, m_NumLockedEdgesBack)			
		SSTRUCT_FIELD(phVerletCloth, m_ClothWeight )

		SSTRUCT_CONTAINED_ARRAY( phVerletCloth, m_Pad0 )				 		
		SSTRUCT_FIELD(phVerletCloth, m_VirtualBoundMat)
		SSTRUCT_FIELD(phVerletCloth, m_PedBound1)
		SSTRUCT_FIELD(phVerletCloth, m_PedBoundMatrix1)

		SSTRUCT_END(phVerletCloth)
}
#endif


IMPLEMENT_PLACE(phVerletCloth);



phVerletCloth::phVerletCloth()
 :	  m_NumLockedEdgesFront( 0 )
	, m_NumLockedEdgesBack( 0 )
	, m_CustomBound( NULL )
	, m_VerletClothType( NULL )
#if !NO_PIN_VERTS_IN_VERLET
	, m_NumPinnedVerts( 0 )
#endif
	, m_NumEdges(0)
	, m_BBMin( Vec3V(V_ZERO) )
	, m_BBMax( Vec3V(V_ZERO) )
	, m_nIterations( DEFAULT_VERLET_ITERS )
//	, m_GravityFactor( DEFAULT_VERLET_GRAVITY_FACTOR )

	, m_Flags(0)

	, m_ClothWeight(DEFAULT_CLOTH_WEIGHT)
	, m_PedBoundMatrix0(NULL)
	, m_PedBound0(NULL)
	, m_PedBoundMatrix1(NULL)
	, m_PedBound1(NULL)
{	
	m_CollisionInst.Reserve(CLOTH_MAX_COLLISION_OBJECTS);

#if __BANK
	m_Pad0[0] = 1;
#endif
}


phVerletCloth::~phVerletCloth() 
{
	clothDebugf1("Delete phVerletCloth");

	if (m_CustomBound)
	{		
		if (phConfig::IsRefCountingEnabled()) 
		{
			clothDebugf1("CustomBound ref count before delete: %d", m_CustomBound->GetRefCount());
#if !__RESOURCECOMPILER
			// since this here is called only for type cloth ref count should be 1
			Assert( m_CustomBound->GetRefCount() == 1 );
#endif
			m_CustomBound->Release();
		}
		else
		{
			delete m_CustomBound;
		}
	}

	// TODO: virtual bound should have been detached explicitly ... just temporary like this till ped cloth collision is properly fixed
	//	Assert( !m_VirtualBound );
	if( m_VirtualBound )
	{
		DetachVirtualBound();
	}
}

void phVerletCloth::Shutdown()
{
	clothDebugf1("Shutdown phVerletCloth");

	m_VerletClothType = NULL;
}




#if !__XENON
#define XMemCpy sysMemCpy
#define XMemSet memset
#endif


#if CLOTH_INSTANCE_FROM_DATA
void phVerletCloth::InstanceFromData(int vertsCapacity, int edgesCapacity)
{
	// m_VerletClothType -  not needed
	// m_ClothWeight - default
	// m_BBMin - default ( irrelevant at this stage )
	// m_BBMax - default ( irrelevant at this stage )

	// m_nIterations - default
	// m_NumLockedEdgesFront - default
	// m_NumLockedEdgesBack - default

	m_ClothData.InstanceFromData(vertsCapacity);

// TODO: should be set by the mesh generation functionality
	// m_NumEdges
	Assert( !m_EdgeData.GetCount() );
	m_EdgeData.Resize(edgesCapacity);
	Assert( m_EdgeData.GetCount() );
}
#endif // CLOTH_INSTANCE_FROM_DATA


void phVerletCloth::InstanceFromTemplate( const phVerletCloth* copyme )
{
	Assert( copyme );
	m_VerletClothType	= copyme->GetVerletClothType();
	m_ClothWeight		= copyme->GetClothWeight();
	m_BBMin				= copyme->m_BBMin;
	m_BBMax				= copyme->m_BBMax;

//	m_GravityFactor		= copyme->m_GravityFactor;
#if !NO_PIN_VERTS_IN_VERLET
	m_NumPinnedVerts	= copyme->m_NumPinnedVerts;
#endif
	m_NumEdges			= copyme->m_NumEdges;
#if !NO_VERTS_IN_VERLET
	m_NumVertices		= copyme->m_NumVertices;	
#endif

	m_nIterations		= copyme->m_nIterations;

#if !NO_BOUND_CENTER_RADIUS
	m_BoundingCenterAndRadius	= copyme->m_BoundingCenterAndRadius;
#endif

	m_NumLockedEdgesFront		= copyme->m_NumLockedEdgesFront;
	m_NumLockedEdgesBack		= copyme->m_NumLockedEdgesBack;

#if NO_PIN_VERTS_IN_VERLET
	m_ClothData.InstanceFromTemplate( &copyme->GetClothData() );

	// TODO: temp here, remove it on resource bump !

	m_ClothData.SetNumPinVerts( *((int*)copyme->m_Pad001) /*copyme->m_NumPinnedVerts*/ );
	m_ClothData.SetNumVerts( *((int*)copyme->m_Pad002) /*copyme->m_NumVertices*/ );
#else
	m_ClothData.InstanceFromTemplate( &copyme->GetClothData(), m_NumPinnedVerts );
#endif
}

void phVerletCloth::AttachVirtualBoundCapsule(const float capsuleRadius, const float capsuleLen, Mat34V_In boundMat, const int boundIdx/*, const Mat34V* transformMat*/ )
{
	phBoundCapsule*	virtualBound = rage_new phBoundCapsule();
	Assert( virtualBound );

	virtualBound->SetCapsuleSize( capsuleRadius, capsuleLen );	

	AttachBound( virtualBound, boundMat, boundIdx/*, transformMat*/ );
}

// TODO: GEOMETRY VIRTUAL BOUND IS WORK IN PROGRESS
Mat34V iMat(V_IDENTITY);

void phVerletCloth::AttachVirtualBoundGeometry(const u32 numVerts, const u32 numPolys, const Vec3V* RESTRICT verts, const phPolygon::Index* RESTRICT triangles, const int boundIdx)
{
	Assert( numVerts );
	Assert( numPolys );
	Assert( verts );
	Assert( triangles );

	phBoundGeometry* virtualBound = rage_new phBoundGeometry();
	Assert( virtualBound );

	const int numPerVertAttribs=0;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	virtualBound->Init(numVerts, numPerVertAttribs, 1, 0, numPolys, 0);
#else
	virtualBound->Init(numVerts, numPerVertAttribs, 1, numPolys, 0);
#endif
	virtualBound->SetMaterial(phMaterialMgr::DEFAULT_MATERIAL_ID);

	virtualBound->CalculateBoundingBox( (const Vector3*)verts, NULL);

#if COMPRESSED_VERTEX_METHOD > 0
	virtualBound->CalculateQuantizationValues();
#endif

	for(u32 i=0; i<numVerts; i++)
	{
		virtualBound->SetVertex(i, verts[i]);
	}

	phPolygon poly;
	for(u32 i=0; i<numPolys; i++)
	{
		const phPolygon::Index* tri = &triangles[i*3];
		poly.InitTriangle(tri[0], tri[1], tri[2], verts[tri[0]], verts[tri[1]], verts[tri[2]] );
		virtualBound->SetPolygon(i, poly);
		virtualBound->SetPolygonMaterialIndex(i, 0);
	}	

	CreateVirtualBound( 1, &iMat );
	AttachBound( virtualBound, iMat, boundIdx/*, &iMat*/ );
}

void phVerletCloth::CreateVirtualBound(int numBounds, const Mat34V* transformMat)
{
	phBoundComposite*	virtualCompositeBound = rage_new phBoundComposite();
	Assert( virtualCompositeBound );

	virtualCompositeBound->Init(numBounds, true);
	virtualCompositeBound->SetNumBounds(numBounds);

	m_VirtualBound = virtualCompositeBound;
#if __PS3
	m_VirtualBoundMat = (u32)(const_cast<Mat34V*>(transformMat));
#else
	m_VirtualBoundMat =		 (const_cast<Mat34V*>(transformMat));
#endif

	m_nIterations = DEFAULT_VERLET_ITERS_W_COLLISION;
}

void phVerletCloth::AttachBound( phBound* virtualBound, Mat34V_In boundMat, const int boundIdx )
{
	Assert( virtualBound );
	Assert( m_VirtualBound );

	((phBoundComposite*)m_VirtualBound.ptr)->SetBound( boundIdx, virtualBound );
	((phBoundComposite*)m_VirtualBound.ptr)->SetCurrentMatrix( boundIdx, boundMat );

	Assert( virtualBound->GetRefCount() == 2 );
	virtualBound->Release();
}

void phVerletCloth::DetachVirtualBound()
{
	m_nIterations = DEFAULT_VERLET_ITERS;

	if( m_VirtualBound )
	{
		if (phConfig::IsRefCountingEnabled()) 
		{
			clothDebugf1("DetachVirtualBound: m_VirtualBound ref count before delete: %d", m_VirtualBound->GetRefCount() );
#if !__RESOURCECOMPILER
			Assert( m_VirtualBound->GetRefCount() == 1 );
#endif
			m_VirtualBound->Release();
		}
		else
		{
			delete m_VirtualBound;
		}
		m_VirtualBound = NULL;
	}
	m_VirtualBoundMat = 0;
}

void phVerletCloth::SetLength (float fNewLength)
{
	// Use a minimum edge length of 1cm.
	Assert(GetFlag(phVerletCloth::FLAG_IS_ROPE));
	int numedges = GetNumEdges();
	float edgelen2 = Max(0.0001f,square(fNewLength/(float)numedges));
	for(int i=0;i<numedges;i++)
	{
		this->m_EdgeData[i].m_EdgeLength2 = edgelen2;
	}
}

void phVerletCloth::SwapVertexInEdges( int indexA, int indexB )
{
	for (int edgeIndex = 0; edgeIndex < GetNumEdges(); ++edgeIndex)
	{
		phEdgeData& edge = GetEdge(edgeIndex);
		if (edge.m_vertIndices[0] == (u16)indexA)
		{
			edge.m_vertIndices[0] = (u16)indexB;
		}
		else if (edge.m_vertIndices[0] == (u16)indexB)
		{
			edge.m_vertIndices[0] = (u16)indexA;
		}

		if (edge.m_vertIndices[1] == (u16)indexA)
		{
			edge.m_vertIndices[1] = (u16)indexB;
		}
		else if (edge.m_vertIndices[1] == (u16)indexB)
		{
			edge.m_vertIndices[1] = (u16)indexA;
		}
	}
}

void phVerletCloth::SwapVertex( int oldIndex, int newIndex, phClothConnectivityData* connectivity, bool hasNormals )
{
	m_ClothData.SwapVertex( oldIndex, newIndex, connectivity, hasNormals );

	for (int i=0; i<GetCustomEdgeList().GetCount(); i++)
	{
		phEdgeData& customEdge = GetCustomEdge(i);
		if (customEdge.m_vertIndices[0]==oldIndex)
		{
			customEdge.m_vertIndices[0] = (u16)newIndex;
		}
		else if (customEdge.m_vertIndices[0]==newIndex)
		{
			customEdge.m_vertIndices[0] = (u16)oldIndex;
		}
		if (customEdge.m_vertIndices[1]==oldIndex)
		{
			customEdge.m_vertIndices[1] = (u16)newIndex;
		}
		else if (customEdge.m_vertIndices[1]==newIndex)
		{
			customEdge.m_vertIndices[1] = (u16)oldIndex;
		}
	}
}

#if __DEV
typedef float RageTweakFloat;
#else
typedef const float RageTweakFloat;
#endif

#if USE_ALL_EDGES
#define CUSTOM_UNUSED_PARAM(x)	x
#else
#define CUSTOM_UNUSED_PARAM(x)
#endif

static RageTweakFloat sfDispMag = 0.1f; 
void phVerletCloth::ApplyImpulse ( const atBitSet& /*unpinnableList*/, Vec3V_In impulse, Vec3V_In position, atFunctor4<void,int,int,int,bool>* CUSTOM_UNUSED_PARAM(swapEventHandler), Vec3V_In vOffset)
{
	// Make a segment going through the cloth.
	Vec3V segmentStart = position;
	ScalarV impulseMag = Mag(impulse);
	if (IsGreaterThanAll(impulseMag,ScalarV(V_ZERO)))
	{
		Vec3V direction = Scale(impulse,Invert(impulseMag));
		Vec3V segmentEnd = AddScaled(segmentStart,direction,ScalarVFromF32(40.0f /* *MAX_CLOTH_RADIUS */));

		// Find the vertex closest to the segment.
		const int maxNumVerts = 12;
		int nearVerts[maxNumVerts];
		const float fSearchRadius = (0.1f * GetRadius(GetCenter()) );
		ScalarV nearRadiusSquared = ScalarVFromF32(fSearchRadius*fSearchRadius);
		const int numVerts = GetNumVertices();
		int numNearVerts = m_ClothData.FindVertsNearSegment(numVerts,segmentStart,segmentEnd,nearRadiusSquared,nearVerts,maxNumVerts,vOffset);
		if (numNearVerts>0)
		{
			ScalarV dispMag = Scale(impulseMag,ScalarVFromF32(sfDispMag));
			ScalarV maxDispMag(V_HALF);
			dispMag = SelectFT(IsGreaterThan(dispMag,maxDispMag),dispMag,maxDispMag);
			Vec3V displacement = Scale(direction,dispMag);

#if USE_ALL_EDGES
			int vertexToUnpin = -1;
			ScalarV distThreshold( V_FLT_MAX );
#endif
			for (int nearVertIndex=0; nearVertIndex<numNearVerts; nearVertIndex++)
			{
				const int vertexIndex = nearVerts[nearVertIndex];
				Vec3V vPos = m_ClothData.GetVertexPosition(vertexIndex);

#if USE_ALL_EDGES
				if( vertexIndex < m_NumPinnedVerts )
				{					
					// TODO: this is just temp proof of concept
					// correct solution is to convert the verts to screen space and compare dist in screen space
					// Svetli
					Vec3V tempV = Subtract( position, vPos );
					ScalarV tempDT = Dot(tempV, tempV);
					if( IsLessThanAll( tempDT, distThreshold ) )
					{
						vertexToUnpin = vertexIndex;
						distThreshold = tempDT;
					}
				}				
#endif			

				if( GetFlag(FLAG_IS_ROPE) )
				{
					m_ClothData.SetVertexPosition( vertexIndex,  Add(vPos, displacement) );
				}
				else
				{
					Vec3V deltaPos = Add( Vec3V(UnpackV1010102(vPos.GetIntrin128())), displacement);				
					m_ClothData.SetVertexPosition( vertexIndex, Vec3V( PackV1010102( vPos.GetIntrin128Ref(), deltaPos.GetIntrin128()) ) );
				}
			}
#if USE_ALL_EDGES
			if( vertexToUnpin > -1 )
			{			
				sysMemStartDebug();
				atArray< int > selectedVerts;
				selectedVerts.Resize(1);
				selectedVerts[0] = vertexToUnpin;
				if( swapEventHandler )
				{
					// TODO: lodindex is 0... i.e. there is no support for cloth LODs yet
					DynamicUnPinVerts( unpinnableList, 0, 1, selectedVerts, swapEventHandler );
				}
				selectedVerts.Reset();
				sysMemEndDebug();
			}
#endif
		}
	}
}


void phVerletCloth::PinVerts (  int lodIndex, atArray<int>& vertIndices, phClothConnectivityData* connectivity, atFunctor4<void,int,int,int,bool>* swapEventHandler, float* perVertexCompression, bool allocNormals )
{
	int numVerts = GetNumVertices();

#if NO_PIN_VERTS_IN_VERLET
	int currentNumPinVerts = GetClothData().GetNumPinVerts();
#else
	int currentNumPinVerts = m_NumPinnedVerts;
#endif
	// Get the first unpinned vertex, so that it can be swapped with the next newly pinned vertex.
	int nextUnpinnedVertex = currentNumPinVerts;

	// Get the total number of pinned and to-be-pinned vertices.
	int numPinnedVerts = currentNumPinVerts + vertIndices.GetCount();
	for (int pinnedVertIndex=currentNumPinVerts; pinnedVertIndex<numPinnedVerts; pinnedVertIndex++)
	{
		// Get the index of the vertex to be pinned.
		int vertexIndex = vertIndices[pinnedVertIndex];

		if( vertexIndex >= numVerts )
		{
			char buff[256];
			sprintf( buff, " Trying to pin vertex with index: %d. Total vertex count is: %d. Most likely some vertices have been weld. Pinning will be incorrect !", vertexIndex, numVerts );
			AssertMsg( vertexIndex < numVerts, buff );
			continue;
		}

		// Move the vertex to be pinned to the start of the vertex list.
		SwapVertex( vertexIndex, nextUnpinnedVertex, connectivity, allocNormals);
		if (perVertexCompression)
		{
			float swapter = perVertexCompression[vertexIndex];
			perVertexCompression[vertexIndex] = perVertexCompression[nextUnpinnedVertex];
			perVertexCompression[nextUnpinnedVertex] = swapter;
		}

		if (swapEventHandler)
		{
			(*swapEventHandler)(vertexIndex,nextUnpinnedVertex, lodIndex,false);
		}

		// Try to find the newly taken vertex index in the remaining list of vertices to be pinned.
		for (int i = pinnedVertIndex + 1; i < numPinnedVerts; i++ )
		{
			if (vertIndices[i]==nextUnpinnedVertex)
			{				
				vertIndices[i] = vertexIndex;	// The newly taken vertex index is still to be pinned, so correct its index in the list.
			}
		}		
		vertIndices[pinnedVertIndex] = nextUnpinnedVertex;	// Correct the newly pinned vertex index in the list of vertices to be pinned.		
		nextUnpinnedVertex++;		// Go to the next unpinned vertex.
	}
#if NO_PIN_VERTS_IN_VERLET
	GetClothData().SetNumPinVerts(numPinnedVerts);
#else
	m_NumPinnedVerts = numPinnedVerts;
#endif

	InitEdgeData(perVertexCompression, connectivity->m_EdgeToVertexIndices, 0 );
}


void phVerletCloth::DynamicUnPinVerts( 	Mat34V_In /*frame*/	, const atBitSet& unpinnableList, const int lodIndex, const int numVertsToUnpin, atArray<int>& vertIndicesToUnPin, atFunctor4<void,int,int,int,bool>* swapEventHandler, float* perVertexCompression, bool hasNormals )
{
#if NO_PIN_VERTS_IN_VERLET
	int numPinVerts = GetClothData().GetNumPinVerts();
#else
	int numPinVerts = m_NumPinnedVerts;
#endif

	if( !numVertsToUnpin || numPinVerts == 1 )	
		return;
	Assert( numVertsToUnpin > 0 && numVertsToUnpin <= numPinVerts );

	for (int i=0; i < numVertsToUnpin; ++i)
	{		
		const int vertexIndex = vertIndicesToUnPin[i] - i;
		Assert( vertexIndex > -1 && vertexIndex < numPinVerts );

		// NOTE: if vertex is locked just continue to the next one
		if( !unpinnableList.IsSet(vertexIndex) )
			continue;

		const int lastPinnedVertex = numPinVerts-1;				// Move the vertex to be unpinned to the start of the unpinned section of the vertex list.
		const int numEdges = m_NumEdges;
		for (int edgeIndex=0; edgeIndex<numEdges; ++edgeIndex)
		{
			phEdgeData& edge = GetEdge(edgeIndex);
			const int idx0 = edge.m_vertIndices[0];
			const int idx1 = edge.m_vertIndices[1];

			// Note: performance fill-in edge, skip it
			if( idx0 == idx1 )
				continue;

			if( idx0 == vertexIndex )
			{
				if( IsPinned(idx1) )
					edge.m_Weight0 = 1.0f;
				else
					edge.m_Weight0 = 0.5f;
			}

			if( idx1 == vertexIndex )
			{
				if( IsPinned(idx0) )
					edge.m_Weight0 = 0.0f;
				else
					edge.m_Weight0 = 0.5f;
			}
		}

		const int numCustomEdges = GetCustomEdgeList().GetCount();
		for (int edgeIndex=0; edgeIndex<numCustomEdges; ++edgeIndex)
		{
			phEdgeData& edge = GetCustomEdge(edgeIndex);
			const int idx0 = edge.m_vertIndices[0];
			const int idx1 = edge.m_vertIndices[1];

			// Note: performance fill-in edge, skip it
			if( idx0 == idx1 )
				continue;

			if( idx0 == vertexIndex )
			{
				if( IsPinned(idx1) )
					edge.m_Weight0 = 1.0f;
				else
					edge.m_Weight0 = 0.5f;
			}

			if( idx1 == vertexIndex )
			{
				if( IsPinned(idx0) )
					edge.m_Weight0 = 0.0f;
				else
					edge.m_Weight0 = 0.5f;
			}
		}

		if( lastPinnedVertex > vertexIndex )
		{
			// NOTE: swap only if this is not the same vertex
			SwapVertex( vertexIndex, lastPinnedVertex, 0, hasNormals );
			if (perVertexCompression)
			{
				float swapter = perVertexCompression[vertexIndex];
				perVertexCompression[vertexIndex] = perVertexCompression[lastPinnedVertex];
				perVertexCompression[lastPinnedVertex] = swapter;
			}

			if (swapEventHandler)
			{
				(*swapEventHandler)( vertexIndex, lastPinnedVertex, lodIndex, true );
			}

		}
#if NO_PIN_VERTS_IN_VERLET
		--numPinVerts;
		GetClothData().SetNumPinVerts(numPinVerts);
#else
		--m_NumPinnedVerts;
#endif
	}
}


void phVerletCloth::DynamicPinVerts ( Mat34V_In /*frame*/, const int lodIndex, const int numVertsToPin, atArray<int>& vertIndicesToPin, atFunctor4<void,int,int,int,bool>* swapEventHandler, float* perVertexCompression, bool allocNormals )
{

#if NO_PIN_VERTS_IN_VERLET
	int numPinVerts = GetClothData().GetNumPinVerts();
#else
	int numPinVerts = m_NumPinnedVerts;
#endif
	Assert( numVertsToPin > 0 && numVertsToPin < (GetNumVertices() - numPinVerts) );
	for (int i=0; i < numVertsToPin; ++i)
	{		
		const int vertexIndex = vertIndicesToPin[i];
		const int firstUnPinnedVertex = numPinVerts;


		const int numEdges = m_NumEdges;
		for (int edgeIndex=0; edgeIndex<numEdges; ++edgeIndex)
		{
			phEdgeData& edge = GetEdge(edgeIndex);
			const int idx0 = edge.m_vertIndices[0];
			const int idx1 = edge.m_vertIndices[1];

			// Note: performance fill-in edge, skip it
			if( idx0 == idx1 )
				continue;

			if( idx0 == vertexIndex )
			{
				edge.m_Weight0 = 0.0f;
			}
			else if( idx1 == vertexIndex )
			{
				edge.m_Weight0 = 1.0f;
			}
		}

		// Note: don't swap if vertexIndex == firstUnPinnedVertex
		if( vertexIndex > firstUnPinnedVertex )
		{
			// NOTE: swap only if this is not the same vertex
			SwapVertex(vertexIndex,firstUnPinnedVertex, 0, allocNormals);
			if (perVertexCompression)
			{
				float swapter = perVertexCompression[vertexIndex];
				perVertexCompression[vertexIndex] = perVertexCompression[firstUnPinnedVertex];
				perVertexCompression[firstUnPinnedVertex] = swapter;
			}

			if (swapEventHandler)
			{
				(*swapEventHandler)( vertexIndex, firstUnPinnedVertex, lodIndex, true );
			}

		}
#if NO_PIN_VERTS_IN_VERLET
		++numPinVerts;
		GetClothData().SetNumPinVerts(numPinVerts);
#else
		++m_NumPinnedVerts;
#endif
	}
}


void phVerletCloth::DynamicPinVertex (int vertexIndex)
{
	m_DynamicPinList.Set(vertexIndex);
	if (GetFlag(FLAG_IS_ROPE))
	{
		for (int edgeIndex=0; edgeIndex<m_NumEdges; edgeIndex++)
		{
			phEdgeData& edge = GetEdge(edgeIndex);
			if (edge.m_vertIndices[0]==vertexIndex)
			{
				edge.PinRopeVertex0();
			}
			else if (edge.m_vertIndices[1]==vertexIndex)
			{
				edge.PinRopeVertex1();
			}
		}
	}
}


void phVerletCloth::DynamicUnpinVertex (int vertexIndex)
{
	m_DynamicPinList.Clear(vertexIndex);
	if (GetFlag(FLAG_IS_ROPE))
	{
		for (int edgeIndex=0; edgeIndex<m_NumEdges; edgeIndex++)
		{
			phEdgeData& edge = GetEdge(edgeIndex);
			if (edge.m_vertIndices[0]==vertexIndex)
			{
				edge.UnpinRopeVertex0();
			}
			else if (edge.m_vertIndices[1]==vertexIndex)
			{
				edge.UnpinRopeVertex1();
			}
		}
	}
}


void phVerletCloth::DynamicUnpinAll ()
{
	int numVerts = GetNumVertices();
	for (int vertexIndex=0; vertexIndex < numVerts; vertexIndex++)
	{
		if (IsDynamicPinned(vertexIndex))
		{
			DynamicUnpinVertex(vertexIndex);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// (Not linked by the main game.)
// int CountConnectionsInBucket( int edgeIndex, int inBucketIndex, int numEdges, phEdgeData *edgeData, int nullVertIndex, const int spacing = 8 )
// {
// 
// 	const int bucketStart = (inBucketIndex) & ~(spacing-1); 
// 	const int bucketEnd = Min(bucketStart + spacing, numEdges);
// 
// 	int v0 = edgeData[edgeIndex].m_vertIndices[0];
// 	int v1 = edgeData[edgeIndex].m_vertIndices[1];
// 
// 	// it's a NULL edge. doesn't interact with anything else.
// 	if( v0 == nullVertIndex )
// 	{
// 		return 0;
// 	}
// 
// 	int nConnect = 0;
// 	int i;
// 	for( i = bucketStart; i < bucketEnd; i++ )
// 	{
// 		phEdgeData &check = edgeData[i];
// 
// 		// it's a NULL edge. doesn't interact with anything else.
// 		if( i != edgeIndex && check.m_vertIndices[0] != nullVertIndex && ( v0 == check.m_vertIndices[0] || v0 == check.m_vertIndices[1] || v1 == check.m_vertIndices[0] || v1 == check.m_vertIndices[1] ))
// 		{
// 			nConnect++;
// 		}
// 	}
// 
// 	return nConnect;
// 
// }


int SpaceEdges (int numEdges, phEdgeData* edgeData, int* newEdgeOrder, int nullVertIndex)
{
	// Sort the edges to keep neighboring edges apart from each other, to remove load hit stores from the UpdateEdges loop.
	// This is required for UpdateEdgesSoAVec3VBy8s, or the cloth will blow up.
	sysMemStartTemp();
	phEdgeData* tempEdges = rage_new phEdgeData[numEdges];
	sysMemEndTemp();

	sysMemCpy(tempEdges,&(edgeData[0]),sizeof(phEdgeData)*numEdges);
	memset(&(edgeData[0]),0xFF,sizeof(phEdgeData)*numEdges);

	const int spacing = 8;
	u32* last = Alloca(u32,spacing);

	int nLast = 0;

	int nextTempEdge = 0;
	int nSkippedEdge = -1;
	int edgeIndex;
	for (edgeIndex=0; edgeIndex<numEdges; edgeIndex++)
	{
		if (nextTempEdge>=numEdges)
		{
			break;
		}

		// Get the next temp edge vertices.
		u16 v0 = tempEdges[nextTempEdge].m_vertIndices[0];
		u16 v1 = tempEdges[nextTempEdge].m_vertIndices[1];

		// Make sure we haven't used this edge yet.
		if (v0==0xFFFF && v1==0xFFFF)
		{
			// This edge has already been placed, so skip it.
			edgeIndex--;
			nextTempEdge++;
			continue;
		}

		// Check to see if this edge has verts that match any of the previous edges added to this bucket
		bool bLast = false;
		// check for null edge, just pass through if it's NULL since it doesn't affect anyone else
		if( v0 != nullVertIndex )
		{
			for( int j = 0; j < nLast; j++ )
			{

				u16* pLast = (u16*)&last[j];
				if ((v0==pLast[0] || v0==pLast[1] || v1==pLast[0] || v1==pLast[1]) && !(v0==0 && v1==0))
				{
					bLast = true;
					break;
				}
			}
		}
		else
		{
			// NULL out this edge for the "last list".  set it to something that wont == any real edge
			v0 = 0xFFFF;
			v1 = 0xFFFF;
		}

		// This edge has a vertex that matches one of the edges in this target bucket
		if (bLast)
		{
			if (nSkippedEdge<0)
			{
				nSkippedEdge = nextTempEdge;
			}

			nextTempEdge++;
			edgeIndex--;
			continue;
		}

		// update our last list, so it records all verticies added to this bucket
		last[nLast] = (u32)v0 << 16 | (u32)v1;
		nLast = (nLast + 1)%spacing;

		// Copy the edge to the list
		sysMemCpy(&edgeData[edgeIndex], &tempEdges[nextTempEdge], sizeof(phEdgeData));
		if( newEdgeOrder ) newEdgeOrder[edgeIndex]=nextTempEdge;
		memset(&tempEdges[nextTempEdge], 0xFF, sizeof(phEdgeData));
		nextTempEdge++;

		// If we have skipped an edge, jump back to it
		if( nSkippedEdge >= 0 )
		{
			nextTempEdge = nSkippedEdge;
			nSkippedEdge = -1;
		}
	}

	// These don't fit well with the sort, so add them to the end.
	nextTempEdge = nSkippedEdge;
	nSkippedEdge = -1;

	int extraEdges = 0;

	for ( ; edgeIndex<numEdges; edgeIndex++)
	{
		u16 v0 = tempEdges[nextTempEdge].m_vertIndices[0];
		u16 v1 = tempEdges[nextTempEdge].m_vertIndices[1];
		if( v0 == 0xFFFF && v1 == 0xFFFF )
		{
			// This edge has already been placed, skip it
			edgeIndex--;
			nextTempEdge++;
			continue;
		}

		extraEdges++;

		// Copy the edge to the list
		sysMemCpy(&edgeData[edgeIndex], &tempEdges[nextTempEdge], sizeof(phEdgeData));
		if( newEdgeOrder ) newEdgeOrder[edgeIndex]=nextTempEdge;
		memset(&tempEdges[nextTempEdge], 0xFF, sizeof(phEdgeData));
		nextTempEdge++;
	}

	sysMemStartTemp();
	delete [] tempEdges;
	sysMemEndTemp();

	return extraEdges;
}

void phVerletCloth::InitEdgeData (const float* perVertexCompression, const atArray<indxA2>& edgeToVertexIndices, const int numExtraEdges )
{
	int numEdgesInClothData = m_NumEdges;
	m_NumEdges = 0;

	const phClothData& clothTypeData = m_ClothData;
	int numVerts = GetNumVertices();
	int edgeIndex;
	for (edgeIndex=0; edgeIndex<numEdgesInClothData; edgeIndex++)
	{
#if !USE_ALL_EDGES
		int iv0 = edgeToVertexIndices[edgeIndex][0];
		int iv1 = edgeToVertexIndices[edgeIndex][1];

		// skip double pinned edges		
		if( !(IsPinned(iv0) && IsPinned(iv1)) )
#endif
		{
			m_NumEdges++;
		}
	}

	// Pad the number of edges to an integer multiple of 8, of this cloth is not rope.
	int nPaddedEdges = GetFlag(FLAG_IS_ROPE) ? m_NumEdges : (m_NumEdges + 7) & ~7;
	int nExtra = nPaddedEdges - m_NumEdges;

	//do all operations on the edge data on this temporary array
	sysMemStartTemp();
	atArray<phEdgeData>& tempEdgeData = *(rage_new atArray<phEdgeData>(m_NumEdges, m_NumEdges + nExtra));
	sysMemEndTemp();

	int iEdgeData = 0;
	for (edgeIndex=0; edgeIndex<numEdgesInClothData; edgeIndex++)
	{
		int iv0 = edgeToVertexIndices[edgeIndex][0];
		int iv1 = edgeToVertexIndices[edgeIndex][1];

		Vec3V v0 = clothTypeData.m_VertexPositions[iv0];
		Vec3V v1 = clothTypeData.m_VertexPositions[iv1];

		Vec3V e = Subtract(v0,v1);
		float edgeLength2 = Dot(e,e).Getf();
		// TODO: obsolete , remove it
		//		if (GetFlag(FLAG_IS_ROPE))
		//		{			
		//			edgeLength2 /= square(1.0f+ROPE_STRETCH_FRACTION);	// Shorten rope edge rest lengths because they stretch when the rope hangs.
		//		}

		float weight0 = 0.5f;
#if !NO_ROPE_WEIGHTS
		float weight1 = 0.5f;
#endif
		if( IsPinned(iv0) )
		{
			weight0 = 0.0f;
		}
		else if( IsPinned(iv1) )
		{
#if !NO_ROPE_WEIGHTS
			if (GetFlag(FLAG_IS_ROPE))
			{
				weight1 = 0.0f;
			}
			else
#endif
			{
				weight0 = 1.0f;
			}
		}

		float compressionWeight = DEFAULT_VERLET_COMPRESSION_STIFFNESS;//m_CompressionStiffnessScale;

		if( perVertexCompression != NULL )
		{
			compressionWeight = ( perVertexCompression[iv0] > perVertexCompression[iv1] ) ? perVertexCompression[iv0] : perVertexCompression[iv1];
		}

#if !USE_ALL_EDGES
		//!me should really be skipping edges like this
		if( IsPinned(iv0) && IsPinned(iv1) )
		{
			compressionWeight = 0.0f;
			edgeLength2 = LARGE_FLOAT;
		}

		// skip double pinned edges

		if( !(IsPinned(iv0) && IsPinned(iv1)) )
#endif
		{						
			tempEdgeData[iEdgeData].m_EdgeLength2 = edgeLength2;
#if !NO_ROPE_WEIGHTS
			if (GetFlag(FLAG_IS_ROPE))
			{
				tempEdgeData[iEdgeData].SetRopeVertexWeights(weight0,weight1);
			}
			else
#endif
			{
				tempEdgeData[iEdgeData].m_Weight0 = weight0;
			}
			tempEdgeData[iEdgeData].m_CompressionWeight = compressionWeight;
			tempEdgeData[iEdgeData].m_vertIndices[0] = (u16)edgeToVertexIndices[edgeIndex][0];
			tempEdgeData[iEdgeData].m_vertIndices[1] = (u16)edgeToVertexIndices[edgeIndex][1];
			iEdgeData++;
		}
	}

	if (!GetFlag(phVerletCloth::FLAG_IS_ROPE))
	{
		// Pad out the edge array to a multiple of 8 so the update edges loop can execute without branches
		for( int i = 0; i < nExtra; i++ )
		{
			sysMemStartTemp();
			phEdgeData& dummyEdge = tempEdgeData.Grow();
			sysMemEndTemp();
			dummyEdge.Init();			
		}

		m_NumEdges = nPaddedEdges;

		// Space the edges out so that no vertex appears twice in any group of 8 edges. This prevents load-hit-stores when updating edges,
		// and also makes it possible to update them in groups of 8.
		int extraEdges = tempEdgeData.GetCount()>0 ? SpaceEdges(m_NumEdges,&(tempEdgeData[0]),NULL,numVerts) : 0;

		// TODO: this is wasting alot of edges especially for small cloth pieces
		// try if instead of while - Svetli 
		while(extraEdges>0)
		{
			// add 8 null edges to end
			for ( int i=0; i<8; i++)
			{
				sysMemStartTemp();
				phEdgeData& dummyEdge = tempEdgeData.Grow();
				sysMemEndTemp();
				dummyEdge.Init();
			}

			int newSpaceStart = m_NumEdges - extraEdges;
			// point to first element of the bucket
			newSpaceStart = newSpaceStart & ~7;

			m_NumEdges += 8;

			// TODO: let's try spacing only once - svetli !!!
			// Try spacing them out again ...
			extraEdges = SpaceEdges(m_NumEdges-newSpaceStart,&(tempEdgeData[newSpaceStart]),NULL,numVerts);
		}
	}

	Assert(tempEdgeData.GetCount()==m_NumEdges);

	m_EdgeData.Resize( m_NumEdges + numExtraEdges );

	if (m_NumEdges>0)
	{
		sysMemCpy(&(m_EdgeData[0]), &(tempEdgeData[0]), sizeof(phEdgeData) * m_NumEdges);
	}

	for( int i = m_NumEdges; i < (m_NumEdges + numExtraEdges); i++ )
	{
		sysMemStartTemp();
		phEdgeData& dummyEdge = m_EdgeData[i];
		sysMemEndTemp();
		dummyEdge.Init();
	}

	sysMemStartTemp();
	delete &tempEdgeData;
	sysMemEndTemp();
}

void phVerletCloth::AddCustomEdge(const u16 vtxIndex0, const u16 vtxIndex1, const float edgeLen2, const float compressionWeight)
{
	phEdgeData& edge = m_CustomEdgeData.Grow(1);
	edge.m_vertIndices[0] = vtxIndex0;
	edge.m_vertIndices[1] = vtxIndex1;
	edge.m_EdgeLength2 = edgeLen2;
	edge.m_Weight0 = 0.5f;
	edge.m_CompressionWeight = compressionWeight;
}

void phVerletCloth::RemoveCustomEdge(const u32 edgeIdx)
{
	Assert( edgeIdx < (u32)m_CustomEdgeData.GetCount() );
	m_CustomEdgeData.Delete( edgeIdx );
}
//


void phVerletCloth::AddCustomEdges( const float *perVertexBendStrength )
{
	sysMemStartTemp();
	const int MAX_CUSTOM_EDGES = 4096;
	phEdgeData* tempBendSprings = rage_new phEdgeData[MAX_CUSTOM_EDGES];
	sysMemEndTemp();

	Vector3 offset(Vector3::ZeroType);

	const atArray<phEdgeData>& edgeData = GetEdgeList();
	int numVerts = GetNumVertices();

#if NO_PIN_VERTS_IN_VERLET
	int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
	int numPinnedVerts = m_NumPinnedVerts;
#endif
	// Loop over the cloth edges again, to initialize the bending spring edges.
	int nCustomEdge = 0;
	for( int v0 = numPinnedVerts; v0 < numVerts; ++v0 )
	{
		Vec3V vI = m_ClothData.GetVertexPosition( v0 );
		const int MAX_EDGES_TO_DEBUG = 48;
		int edgesFound[MAX_EDGES_TO_DEBUG];
		int edgesFoundCount = SearchEdges( (const Vector3* RESTRICT)m_ClothData.GetVertexPointer(), offset, v0, edgesFound, edgeData );
		Assert( edgesFoundCount < MAX_EDGES_TO_DEBUG );
		if( edgesFoundCount == 1 )
			continue;

		for( int i = 0; i < edgesFoundCount; ++i )
		{
			const phEdgeData& edge0 = GetEdge( edgesFound[i] );
			const int idx0 = ( edge0.m_vertIndices[0] == v0 ) ? edge0.m_vertIndices[1]: edge0.m_vertIndices[0];

			if( IsPinned(idx0) )
				continue;

			int v1 = -1;	
			ScalarV bestAngle = ScalarV(V_ONE);

			for( int j = 0; j < edgesFoundCount; ++j )
			{			
				const phEdgeData& edge1 = GetEdge( edgesFound[j] );
				const int idx1 = ( edge1.m_vertIndices[0] == v0 ) ? edge1.m_vertIndices[1]: edge1.m_vertIndices[0];

				if( IsPinned(idx1) )
					continue;

				Vec3V vI0 = m_ClothData.GetVertexPosition( idx0 );
				Vec3V vI1 = m_ClothData.GetVertexPosition( idx1 );

				// TODO: I assume here no edge has 0 length, assert or something like it for just in case - svetli
				Vec3V tempEdge0 = Normalize( Subtract( vI, vI0 ) );
				Vec3V tempEdge1 = Normalize( Subtract( vI, vI1 ) );
				ScalarV res = Dot( tempEdge0, tempEdge1 );
				if( IsLessThanAll(res, bestAngle) != 0 )
				{
					bestAngle = res;
					v1 = idx1;
				}
			}

			ScalarV bestAngleThreshold = ScalarVFromF32(-0.9f);
			if( IsLessThanAll(bestAngleThreshold, bestAngle) != 0 )
			{
				continue;
			}

			Assert( v1 != -1 );

			// Get the new bend edge and set its vertices.

			Assert( nCustomEdge < MAX_CUSTOM_EDGES );
			bool foundEdge = false;
			for(int k = 0; k < nCustomEdge; ++k)
			{
				const phEdgeData& bendEdge = tempBendSprings[ k ];
				foundEdge = (bendEdge.m_vertIndices[0] == idx0 && bendEdge.m_vertIndices[1] == v1) 
					||	(bendEdge.m_vertIndices[0] == v1 && bendEdge.m_vertIndices[1] == idx0);
				if( foundEdge )
					break;
			}

			if( foundEdge )
				continue;

			if( perVertexBendStrength )
			{
				if( perVertexBendStrength[idx0] < MIN_BEND_EDGE_STRENGTH_THRESHOLD || perVertexBendStrength[v1] < MIN_BEND_EDGE_STRENGTH_THRESHOLD )
				{
					continue;
				}
			}			

			phEdgeData& bend = tempBendSprings[ nCustomEdge++ ];
			bend.m_vertIndices[0] = (u16)idx0;
			bend.m_vertIndices[1] = (u16)v1;

			// Set the bend spring length scale to the default.
			// TODO: why is this here ?!?
			// 			float bsls = m_BendSpringLengthScale;
			// 			if (bendSpringLengthScale)
			// 			{
			// 				// Bend spring length scales were provided, so use the largest one from the vertices on this bend spring edge.
			// 				bsls = ( bendSpringLengthScale[idx0] > bendSpringLengthScale[v1] ) ? bendSpringLengthScale[idx0] : bendSpringLengthScale[v1];
			// 			}

			// Set the bend spring edge length squared, and multiply it by the square of the length scale.

			Vec3V vTemp = Subtract(m_ClothData.GetVertexPosition(v1), m_ClothData.GetVertexPosition(idx0));
			bend.m_EdgeLength2 = Dot(vTemp, vTemp).Getf();

			// TODO: why is this here ?!?
			//			bend.m_EdgeLength2 *= square(bsls);

			if (!perVertexBendStrength)
			{
				// No bend strength was given, so use the default.
				bend.m_CompressionWeight = DEFAULT_VERLET_BENDSPRING_STIFFNESS;//m_BendSpringStiffness;
			}
			else
			{
				// Bend spring strengths were provided, so use the largest one from the vertices on this bend spring edge.
				bend.m_CompressionWeight = (perVertexBendStrength[v0]>perVertexBendStrength[v1] ? perVertexBendStrength[v0] : perVertexBendStrength[v1]);
			}

			bend.m_Weight0 = 0.5f;
			if (IsPinned(v0))
			{
				bend.m_Weight0 = 0.0f;
			}
			else if (IsPinned(v1))
			{
				bend.m_Weight0 = 1.0f;
			}
		}
	}

	// Make the number of added edges an integer multiple of 8.
	int nPaddedEdges = (nCustomEdge + 7) & ~7;

	int nTotalEdges = nPaddedEdges;
	if( m_CustomEdgeData.GetCapacity() < nTotalEdges )
	{
		if( m_CustomEdgeData.GetCapacity() > 0 )
		{
			m_CustomEdgeData.Reset();
		}
		m_CustomEdgeData.Reserve( nTotalEdges );
	}

	m_CustomEdgeData.Resize( nTotalEdges );
	for(int i = 0; i < nCustomEdge; ++i)
	{
		m_CustomEdgeData[i] = tempBendSprings[i];
	}

	for (int i= nCustomEdge; i < nTotalEdges; i++)
	{
		phEdgeData& dummyEdge = m_CustomEdgeData[i];
		dummyEdge.Init();
	}

	sysMemStartTemp();
	Assert( tempBendSprings );
	delete[] tempBendSprings;
	sysMemEndTemp();
}


void phVerletCloth::Init ()
{
	int numVerts = GetClothData().GetVertexCapacity();
	AssertMsg(m_NumEdges>0 && numVerts>0 , "Load or initialize with cloth with a shape first.");
	m_DynamicPinList.Init(numVerts);
}


void phVerletCloth::InitWithBound( atArray<indxA2>& edgeToVertexIndices, phBoundGeometry& clothBound, bool allocNormals, int extraVerts)
{
	int numBoundVerts = clothBound.GetNumVertices();
#if COMPRESSED_VERTEX_METHOD == 0
	const Vec3V* vertices = (const Vec3V*)clothBound.GetVertexPointer();
#else
	// NOTE: if we need to do verts padding, should be done here - svetli
	// numVerts = (numBoundVerts + 7 ) &~7 ... then allocate numVerts

	Vec3V *vertices = Alloca(Vec3V, numBoundVerts);
	int i;
	for(i = 0; i < numBoundVerts; ++i)
	{
		vertices[i] = clothBound.GetVertex(i);
		vertices[i].SetW( ScalarV(V_ZERO) );
	}

#endif
	InitWithPolylist( edgeToVertexIndices, vertices, numBoundVerts, clothBound.GetPolygonPointer(), clothBound.GetNumPolygons(), allocNormals, extraVerts );
}


int phVerletCloth::CountEdges (const phPolygon* triangles, int numTriangles)
{
	int numEdges = 0;
	for (int triangleIndex=0; triangleIndex<numTriangles; triangleIndex++)
	{
		// Get the triangle and loop over its neighbors.
		const phPolygon& triangle = triangles[triangleIndex];
		for (int neighborIndex=0; neighborIndex<3; neighborIndex++)
		{
			// Get the neighbor triangle index.
			int neighborTriangleIndex = triangle.GetNeighboringPolyNum(neighborIndex);

			// See if the neighbor triangle index is greater, or if there is no neighbor.
			// This includes the case neighborTriangleIndex==(phPolygon::Index)(-1) because it's 65535.
			if (neighborTriangleIndex>triangleIndex)
			{
				// The neighbor index is greater than this triangle index, or there is no neighbor, so count this edge.
				numEdges++;
			}
		}
	}

	return numEdges;
}


// TODO: something must be seriosly wrong with the cloth if clamp is needed, remove this functions

bool phVerletCloth::ClampCompositeExtents (phBoundComposite& compositeBound, float maxRadius)
{
	// Get the bound's current radius;
	float radius = compositeBound.GetRadiusAroundCentroid();
	if (radius<maxRadius)
	{
		// The current radius is below the limit, so don't do anything.
		return false;
	}

	// Set the bound's radius to the limit.
	compositeBound.SetRadiusAroundCentroid(ScalarVFromF32(maxRadius));

	// Set the bounding box to something reasonable.
	Vector3 centroidOffset(VEC3V_TO_VECTOR3(compositeBound.GetCentroidOffset()));
	Vector3 boxHalfSize(radius,radius,radius);
	Vector3 boxMin(centroidOffset);
	boxMin.Subtract(boxHalfSize);
	Vector3 boxMax(centroidOffset);
	boxMax.Add(boxHalfSize);
	compositeBound.SetBoundingBoxMinMax(RCC_VEC3V(boxMin),RCC_VEC3V(boxMax));

	// Return true to indicate that the bound's extents were clamped.
	return true;
}


void phVerletCloth::InitWithPolylist (atArray<indxA2>& edgeToVertexIndices, const Vec3V* vertices, int numVertices, const phPolygon* polygons, int numPolygons, bool allocNormals, int extraVerts)
{	
	m_NumEdges = CountEdges(polygons,numPolygons);	// Find the number of edges in the cloth (the bound does not store edges).

	// Create the cloth triangles, edges and vertices. Initialize the cloth data.
	m_ClothData.Init(vertices,numVertices,Vec3V(V_ZERO), allocNormals, extraVerts);

#if NO_VERTS_IN_VERLET
	m_ClothData.SetNumVerts(numVertices);
#else
	m_NumVertices = numVertices;
#endif

	for (int vertexIndex=0; vertexIndex<numVertices; ++vertexIndex)
	{
		m_ClothData.m_VertexPositions[vertexIndex] = vertices[vertexIndex];
	}

	if( allocNormals )
	{
		for (int vertexIndex=0; vertexIndex<numVertices; ++vertexIndex)
			m_ClothData.m_VertexInitialNormals[vertexIndex] = Vec3V(V_ZERO);
	}

	// Initialize the cloth edges from the bound.
	int edgeIndex = 0;
	for (int triangleIndex=0; triangleIndex<numPolygons; triangleIndex++)
	{
		const phPolygon& triangle = polygons[triangleIndex];

		AssertMsg( numPolygons == 1 || (!(triangle.GetNeighboringPolyNum(0) == (phPolygon::Index)(-1) && triangle.GetNeighboringPolyNum(1) == (phPolygon::Index)(-1) && triangle.GetNeighboringPolyNum(2) == (phPolygon::Index)(-1))) ,
			"You must have connectivity information for triangles, please call phBound::ComputeNeighbors() before creating the cloth");

		for (int neighborIndex=0; neighborIndex<3; neighborIndex++)
		{
			int neighborTriangleIndex = triangle.GetNeighboringPolyNum(neighborIndex);
			int usedNeighborIndex = (phPolygon::Index)(-1);
			for (int otherTriangleIndex=0; otherTriangleIndex<numPolygons; otherTriangleIndex++)
			{
				if (neighborTriangleIndex==otherTriangleIndex)
				{
					usedNeighborIndex = otherTriangleIndex;
					break;
				}
			}

			if (usedNeighborIndex==(phPolygon::Index)(-1))
			{
				neighborTriangleIndex = (phPolygon::Index)(-1);
			}

			if (neighborTriangleIndex>triangleIndex || neighborTriangleIndex==(phPolygon::Index)(-1))
			{
				// The neighbor index is greater, or there is no neighbor in the cloth, so make an edge (so that each edge is only made once).
				int vertexA = triangle.GetVertexIndex(neighborIndex);
				int vertexB = triangle.GetVertexIndex((neighborIndex+1)%3);

				u16* vertexIndices = edgeToVertexIndices[edgeIndex];
				vertexIndices[0] = (u16)vertexA;
				vertexIndices[1] = (u16)vertexB;

				edgeIndex++;

				if( allocNormals )
				{
					// Add these two triangle normals to the two vertex initial normals.
					Vec3V triangleNormal = triangle.ComputeUnitNormal((const Vec3V*)vertices);
					Vec3V neighborNormal = neighborTriangleIndex==(phPolygon::Index)(-1) ? Vec3V(V_ZERO) : polygons[neighborTriangleIndex].ComputeUnitNormal((const Vec3V*)vertices);
					Vec3V normalA = Add(m_ClothData.m_VertexInitialNormals[vertexA],triangleNormal);
					Vec3V normalB = Add(m_ClothData.m_VertexInitialNormals[vertexB],triangleNormal);
					normalA = Add(normalA,neighborNormal);
					normalB = Add(normalB,neighborNormal);
					m_ClothData.m_VertexInitialNormals[vertexA] = normalA;
					m_ClothData.m_VertexInitialNormals[vertexB] = normalB;
				}
			}
		}
	}

	if( allocNormals )
	{
		for (int vertexIndex=0; vertexIndex<numVertices; ++vertexIndex)
			m_ClothData.m_VertexInitialNormals[vertexIndex] = NormalizeSafe(m_ClothData.m_VertexInitialNormals[vertexIndex], Vec3V(V_X_AXIS_WZERO));
	}
	Assert(edgeIndex==m_NumEdges);
	Init();
}

void phVerletCloth::InitDiamond (atArray<indxA2>& edgeToVertexIndices, const Vector2& size, int numSquareRows, int numSquareCols, const Vector3& position, const Vector3& rotation, bool allocNormals)
{
	atArray<Vector3> Vertices;
	atArray<phPolygon> Triangles;

	// Find and set the numbers of elements.
	Vector2 squareSize(size.x/(float)(2*numSquareCols),size.y/(float)(2*numSquareRows));
	int numSquares = 4*numSquareRows*numSquareCols;
	int MaxNumTriangles = 2*numSquares;
	m_NumEdges = 3*numSquares+2*numSquareRows+2*numSquareCols;
#if NO_VERTS_IN_VERLET
	int numVerts = numSquares+2*numSquareRows+2*numSquareCols+1;
#else
	m_NumVertices = numSquares+2*numSquareRows+2*numSquareCols+1;
	int numVerts = m_NumVertices;
#endif
	int NumTriangles = MaxNumTriangles;

	Vertices.Resize(numVerts);
	Triangles.Resize(NumTriangles);


	// Make the cloth matrix and the transformed normal.
	Matrix34 clothMatrix(CreateRotatedMatrix(position,rotation));
	Vector3 clothNormal;
	clothMatrix.Transform3x3(ZAXIS,clothNormal);

	// Initialize the vertices.
	Vector3 localPosition,worldPosition;
	float left = -0.5f*size.x;
	float top = 0.5f*size.y;
	int rowIndex,colIndex,vertexIndex;
	for (rowIndex=0; rowIndex<=2*numSquareRows; rowIndex++)
	{
		for (colIndex=0; colIndex<=2*numSquareCols; colIndex++)
		{
			vertexIndex = rowIndex*(2*numSquareCols+1)+colIndex;
			Assert(vertexIndex>=0 && vertexIndex<numVerts);
			localPosition.Set(left+squareSize.x*(float)colIndex,top-squareSize.y*(float)rowIndex,0.0f);
			clothMatrix.Transform(localPosition,worldPosition);
			Vertices[vertexIndex] = worldPosition;
		}
	}

	// Initialize the triangles.
	const Vector3* vertices = &(Vertices[0]);
	int triangleIndex0,triangleIndex1,triangleIndex2;
	int /*edgeIndex0,edgeIndex1,edgeIndex2,*/vertexIndex1,vertexIndex0,vertexIndex2;
	for (rowIndex=0; rowIndex<2*numSquareRows; rowIndex++)
	{
		for (colIndex=0; colIndex<2*numSquareCols; colIndex++)
		{
			int triangleIndex = rowIndex*4*numSquareCols+2*colIndex;
			Assert(triangleIndex>=0 && triangleIndex<NumTriangles);
			int topTri = rowIndex>0 ? (rowIndex<=numSquareRows ? (rowIndex-1)*4*numSquareCols+2*colIndex+1 : (rowIndex-1)*4*numSquareCols+2*colIndex) : BAD_POLY_INDEX;
			int bottomTri = rowIndex<2*numSquareRows-1 ? (rowIndex<numSquareRows-1 ? (rowIndex+1)*4*numSquareCols+2*colIndex : (rowIndex+1)*4*numSquareCols+2*colIndex+1) : BAD_POLY_INDEX;
			int leftTri = colIndex>0 ? (colIndex<=numSquareCols ? triangleIndex-1 : triangleIndex-2) : BAD_POLY_INDEX;
			int rightTri = colIndex<2*numSquareCols-1 ? (colIndex<numSquareCols-1 ? triangleIndex+2 : triangleIndex+3) : BAD_POLY_INDEX;
			// int topEdge = rowIndex*2*numSquareCols+colIndex;
			// int bottomEdge =(rowIndex+1)*2*numSquareCols+colIndex;
			// int leftEdge = (2*numSquareRows+1)*2*numSquareCols+rowIndex*(2*numSquareCols+1)+colIndex;
			// int rightEdge = leftEdge+1;
			// int diagEdge = (2*numSquareRows+1)*2*numSquareCols+2*numSquareRows*(2*numSquareCols+1)+rowIndex*(2*numSquareCols)+colIndex;
			int topLeftVert = rowIndex*(2*numSquareCols+1)+colIndex;
			int topRightVert = rowIndex*(2*numSquareCols+1)+colIndex+1;
			int bottomLeftVert = (rowIndex+1)*(2*numSquareCols+1)+colIndex;
			int bottomRightVert = (rowIndex+1)*(2*numSquareCols+1)+colIndex+1;
			triangleIndex0 = triangleIndex+1;
			triangleIndex1 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? topTri : leftTri) : (rowIndex<numSquareRows ? rightTri : bottomTri);
			triangleIndex2 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? leftTri : bottomTri) : (rowIndex<numSquareRows ? topTri : rightTri);
			// edgeIndex0 = diagEdge;
			// edgeIndex1 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? topEdge : leftEdge) : (rowIndex<numSquareRows ? rightEdge : bottomEdge);
			// edgeIndex2 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? leftEdge : bottomEdge) : (rowIndex<numSquareRows ? topEdge : rightEdge);
			vertexIndex0 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? bottomLeftVert : bottomRightVert) : (rowIndex<numSquareRows ? topLeftVert : topRightVert);
			vertexIndex1 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? topRightVert : topLeftVert) : (rowIndex<numSquareRows ? bottomRightVert : bottomLeftVert);
			vertexIndex2 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? topLeftVert : bottomLeftVert) : (rowIndex<numSquareRows ? topRightVert : bottomRightVert);
			//			Triangles[triangleIndex].Init(equilTension,vertices,triangleIndex0,triangleIndex1,triangleIndex2,edgeIndex0,edgeIndex1,edgeIndex2,vertexIndex0,vertexIndex1,vertexIndex2);
			Triangles[triangleIndex].InitTriangle((u16)vertexIndex0,(u16)vertexIndex1,(u16)vertexIndex2,RCC_VEC3V(vertices[vertexIndex0]),RCC_VEC3V(vertices[vertexIndex1]),RCC_VEC3V(vertices[vertexIndex2]));
			Triangles[triangleIndex].SetNeighboringPolyNum( 0, ( phPolygon::Index )triangleIndex0 );
			Triangles[triangleIndex].SetNeighboringPolyNum( 1, ( phPolygon::Index )triangleIndex1 );
			Triangles[triangleIndex].SetNeighboringPolyNum( 2, ( phPolygon::Index )triangleIndex2 );
			//!me	Triangles[triangleIndex].SetMaterialIndex(0);

			triangleIndex+=1;
			Assert(triangleIndex>=0 && triangleIndex<NumTriangles);
			triangleIndex0 = triangleIndex-1;
			triangleIndex1 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? bottomTri : rightTri) : (rowIndex<numSquareRows ? leftTri : topTri);
			triangleIndex2 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? rightTri : topTri) : (rowIndex<numSquareRows ? bottomTri : leftTri);
			// edgeIndex0 = diagEdge;
			// edgeIndex1 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? bottomEdge : rightEdge) : (rowIndex<numSquareRows ? leftEdge : topEdge);
			// edgeIndex2 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? rightEdge : topEdge) : (rowIndex<numSquareRows ? bottomEdge : leftEdge);
			vertexIndex0 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? topRightVert : topLeftVert) : (rowIndex<numSquareRows ? bottomRightVert : bottomLeftVert);
			vertexIndex1 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? bottomLeftVert : bottomRightVert) : (rowIndex<numSquareRows ? topLeftVert : topRightVert);
			vertexIndex2 = colIndex<numSquareCols ? (rowIndex<numSquareRows ? bottomRightVert : topRightVert) : (rowIndex<numSquareRows ? bottomLeftVert : topLeftVert);
			//			Triangles[triangleIndex].Init(equilTension,vertices,triangleIndex0,triangleIndex1,triangleIndex2,edgeIndex0,edgeIndex1,edgeIndex2,vertexIndex0,vertexIndex1,vertexIndex2);
			Triangles[triangleIndex].InitTriangle((u16)vertexIndex0,(u16)vertexIndex1,(u16)vertexIndex2,RCC_VEC3V(vertices[vertexIndex0]),RCC_VEC3V(vertices[vertexIndex1]),RCC_VEC3V(vertices[vertexIndex2]));
			Triangles[triangleIndex].SetNeighboringPolyNum( 0, ( phPolygon::Index )triangleIndex0 );
			Triangles[triangleIndex].SetNeighboringPolyNum( 1, ( phPolygon::Index )triangleIndex1 );
			Triangles[triangleIndex].SetNeighboringPolyNum( 2, ( phPolygon::Index )triangleIndex2 );
			//!me	Triangles[triangleIndex].SetMaterialIndex(0);
		}
	}

	InitWithPolylist( edgeToVertexIndices, (const Vec3V*)vertices, numVerts, &(Triangles[0]), NumTriangles, allocNormals, 0 );
}


void phVerletCloth::InitStrand ( const int totalNumVerts, atArray<indxA2>& edgeToVertexIndices, const Vec3V* vertices, int numVertices, bool allocNormals, int extraVerts, Vec3V_In gravity )
{
	SetFlag(FLAG_IS_ROPE, true);
	m_ClothData.SetIsRope();
	m_ClothData.m_VertexPositions.Resize(totalNumVerts);
	m_ClothData.m_VertexPrevPositions.Resize(totalNumVerts);

#if NO_VERTS_IN_VERLET
	m_ClothData.SetNumVerts(numVertices);
#else
	m_NumVertices = numVertices;
#endif

	m_nIterations = DEFAULT_ROPE_ITERATIONS;

	for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++ )
	{
		m_ClothData.m_VertexPositions[vertexIndex] = vertices[vertexIndex];
		m_ClothData.m_VertexPrevPositions[vertexIndex] = vertices[vertexIndex];
	}

	m_NumEdges = numVertices-1;
	m_ClothData.Init( vertices, numVertices, gravity, allocNormals, extraVerts );

	for (int edgeIndex = 0; edgeIndex < m_NumEdges; edgeIndex++ )
	{		
		u16* vertexIndices = edgeToVertexIndices[edgeIndex];
		vertexIndices[0] = (u16)edgeIndex;
		vertexIndices[1] = (u16)(edgeIndex+1);
	}

	Init();
}


void phVerletCloth::ScaleRopeLength (float lengthScale)
{
	float lengthScale2 = square(lengthScale);
	for (int edgeIndex=0; edgeIndex<m_NumEdges; edgeIndex++)
	{
		phEdgeData& edge = m_EdgeData[edgeIndex];
		edge.m_EdgeLength2 *= lengthScale2;
	}
}


void phVerletCloth::SetRopeLength (float length)
{
	float oldLength = 0.0f;
	int edgeIndex;
	for (edgeIndex=0; edgeIndex<m_NumEdges; edgeIndex++)
	{
		oldLength += SqrtfSafe(m_EdgeData[edgeIndex].m_EdgeLength2);
	}

	if (oldLength>SMALL_FLOAT)
	{
		float lengthScale = length/oldLength;
		ScaleRopeLength(lengthScale);
	}

	// The previous length was invalid, so make every edge have the same length.
	float edgeLength2 = square(length/(float)m_NumEdges);
	for (edgeIndex=0; edgeIndex<m_NumEdges; edgeIndex++)
	{
		m_EdgeData[edgeIndex].m_EdgeLength2 = edgeLength2;
	}
}


#if !NO_BOUND_CENTER_RADIUS
const Vector4 phVerletCloth::GetBoundingCenterAndRadius () const
{
	return m_BoundingCenterAndRadius;
}
#endif


phBoundComposite& phVerletCloth::CreateRopeBound (ScalarV_In ropeRadius)
{
	phBoundComposite& compositeBound = *(rage_new phBoundComposite);
	const int iEdgeCapacity = GetEdgeCapacity();

	compositeBound.Init(iEdgeCapacity,true);
	compositeBound.SetNumBounds(iEdgeCapacity);
	const atArray<phEdgeData>& ropeEdges = GetEdgeList();
	Matrix34 partMatrix(M34_IDENTITY);

	const int numEdges = GetNumEdges();

	int partIndex=0;
	for (; partIndex<numEdges; partIndex++)
	{
		phBoundCapsule& capsuleBound = *(rage_new phBoundCapsule);
		int vertIndex0 = ropeEdges[partIndex].m_vertIndices[0];
		int vertIndex1 = ropeEdges[partIndex].m_vertIndices[1];
		Vec3V vert0 = m_ClothData.GetVertexPosition(vertIndex0);
		Vec3V vert1 = m_ClothData.GetVertexPosition(vertIndex1);
		Vec3V edge = Subtract(vert1,vert0);
		ScalarV edgeLength = Mag(edge);
		capsuleBound.SetCapsuleSize(ropeRadius,edgeLength);
		compositeBound.SetBound(partIndex,&capsuleBound);

		Assert( capsuleBound.GetRefCount() == 2 );
		capsuleBound.Release();
	}

// NOTE: make the rest of the bounds too small ( those edges haven't been used yet )
	for (; partIndex<iEdgeCapacity; partIndex++)
	{
		phBoundCapsule& capsuleBound = *(rage_new phBoundCapsule);
		
		ScalarV edgeLength = ScalarVFromF32(0.001f);
		capsuleBound.SetCapsuleSize(ropeRadius,edgeLength);
		compositeBound.SetBound(partIndex,&capsuleBound);

		Assert( capsuleBound.GetRefCount() == 2 );
		capsuleBound.Release();
	}

	return compositeBound;
}


// phBoundComposite& phVerletCloth::CreateRopeBound (int initialNumEdges, float ropeLength, float startRadius, float endRadius)
// {
// 	phBoundComposite& compositeBound = *(rage_new phBoundComposite);
// 	compositeBound.Init(m_NumEdges, true);
// 	compositeBound.SetNumBounds(initialNumEdges);
// 	Matrix34 partMatrix(M34_IDENTITY);
// 	float invNumEdges = 1.0f/(float)initialNumEdges;
// 	float edgeLength = ropeLength*invNumEdges;
// 	partMatrix.d.x -= 0.5f*ropeLength;
// 	bool varyRadius = (endRadius>0.0f && endRadius!=startRadius);
// 	for (int partIndex=0; partIndex<m_NumEdges; partIndex++)
// 	{
// 		phBoundCapsule& capsuleBound = *(rage_new phBoundCapsule);
// 		float edgeRadius = (varyRadius ? (partIndex<initialNumEdges ? (startRadius + (endRadius-startRadius)*(float)partIndex*invNumEdges) : startRadius) : startRadius);
// 		capsuleBound.SetCapsuleSize(edgeRadius,edgeLength);
// 		compositeBound.SetBound(partIndex,&capsuleBound);
// 		compositeBound.SetCurrentMatrix(partIndex, RCC_MAT34V(partMatrix));
// 		compositeBound.SetLastMatrix(partIndex, RCC_MAT34V(partMatrix));
// 		partMatrix.d.x += edgeLength;
// 	}
// 
// 	compositeBound.CalculateCompositeExtents();
// 	compositeBound.UpdateBvh(true);
// #if !NO_CLOTH_BOUND_CLAMP
// 	ClampCompositeExtents(compositeBound,MAX_CLOTH_RADIUS);
// #endif
// 	return compositeBound;
// }


// phBoundComposite& phVerletCloth::CreateRopeBound (float ropeRadius)
// {
// 	return CreateRopeBound( GetNumEdges(), ComputeRopeLength().Getf(), ropeRadius );
// }

ScalarV_Out phVerletCloth::ComputeRopeLength () /*const*/
{
	ScalarV ropeLength = ScalarV(V_ZERO);
	const phClothData& clothTypeData = m_ClothData;
	const int numEdges = GetNumEdges();
	const atArray<phEdgeData>& edges = GetEdgeList();
	for (int edgeIndex=0; edgeIndex<numEdges; edgeIndex++)
	{
		int vert0 = edges[edgeIndex].m_vertIndices[0];
		int vert1 = edges[edgeIndex].m_vertIndices[1];
		ScalarV edgeLength = Dist( clothTypeData.GetVertexPosition(vert1), clothTypeData.GetVertexPosition(vert0));
		ropeLength = Add(ropeLength,edgeLength);
	}

	return ropeLength;
}

Vec3V phVerletCloth::GetEdgeVec3V(int vertexIdx, int edgeIdx, Vec3V* RESTRICT pClothVertices)
{
	Assert( edgeIdx < GetNumEdges() );
	phEdgeData& edge = GetEdge(edgeIdx);
	Assert( vertexIdx == edge.m_vertIndices[0] || vertexIdx == edge.m_vertIndices[1] );

	if( vertexIdx == edge.m_vertIndices[0] )
		return Subtract( pClothVertices[edge.m_vertIndices[1]], pClothVertices[edge.m_vertIndices[0]] );
	else
		return Subtract( pClothVertices[edge.m_vertIndices[0]], pClothVertices[edge.m_vertIndices[1]] );
}


// TODO: tear is work in progress

u16 phVerletCloth::TearEdges( int vertexIdx, int edgeIdx0, int edgeIdx1, u16* pClothDisplayMap, Vec3V* RESTRICT pClothVertices )
{
// NOTE: vertexIdx should be a common vertex between edge with Idx0 and edge with Idx1
// TODO: add assert for the note above

	Assert( pClothDisplayMap );
	Assert( pClothVertices );

	int numVerts = GetNumVertices();
	const int newVertexIdx = numVerts++;							// add vertex to cloth simulation
	pClothDisplayMap[ newVertexIdx ] = (u16)newVertexIdx;			// add vertex to cloth display map	
	pClothVertices[ newVertexIdx ] = pClothVertices[ vertexIdx ];	// dup original vertex

#if NO_VERTS_IN_VERLET
	GetClothData().SetNumVerts(numVerts);
#endif

	CopyEdge( vertexIdx, newVertexIdx, edgeIdx0 );
	CopyEdge( vertexIdx, newVertexIdx, edgeIdx1 );

	for( int i = 0; i < GetNumEdges(); ++i )
	{
		const phEdgeData& edge = GetEdge(i);
		if( edge.m_vertIndices[0] != vertexIdx && edge.m_vertIndices[1] != vertexIdx )
			continue;

		Vec3V v = pClothVertices[ (edge.m_vertIndices[0] == vertexIdx ? edge.m_vertIndices[1] : edge.m_vertIndices[0] ) ];
		Vec3V v0 = pClothVertices[vertexIdx];
		if (v0.GetZf() > v.GetZf() )
		{
			DetachEdge( vertexIdx, newVertexIdx, i );
		}
	}

	return (u16)newVertexIdx;
}


void phVerletCloth::CopyEdge(int vertexIdx, int newVertexIdx, int edgeToCopyIdx)
{
	Assert( (m_NumEdges+1) < m_EdgeData.GetCapacity() );

	// Edge to copy
	// 1. Add an edge
	const int newEdgeIdx0 = m_NumEdges++;	
	phEdgeData& newEdge0 = m_EdgeData[ newEdgeIdx0 ];

	// 2. Copy edge data and set new vertex
	newEdge0 = m_EdgeData[edgeToCopyIdx];
	if( newEdge0.m_vertIndices[0] == vertexIdx )
		newEdge0.m_vertIndices[0] = (u16)newVertexIdx;
	else
		newEdge0.m_vertIndices[1] = (u16)newVertexIdx;
}

void phVerletCloth::DetachEdge( int vertexIdx, int newVertexIdx, int edgeToDetachIdx )
{
	// Edge to detach.
	// 1. Set new vertex
	phEdgeData& edgeToDetach = m_EdgeData[ edgeToDetachIdx ];
	if( edgeToDetach.m_vertIndices[0] == vertexIdx )
		edgeToDetach.m_vertIndices[0] = (u16)newVertexIdx;
	else
		edgeToDetach.m_vertIndices[1] = (u16)newVertexIdx;
}


#if __BANK && !__RESOURCECOMPILER

int phVerletCloth::PickVertices( const Vector3* RESTRICT clothVertices, const Vector3& offset, int startVertex, int endVertex, Vector3& mouseScreen, Vector3& mouseFar )
{	
	Vector3 center;
	float radius;
#if NO_BOUND_CENTER_RADIUS
	Vec3V centerV = GetCenter();
	center = VEC3V_TO_VECTOR3(centerV);
	radius = GetRadius(centerV);
#else
	GetBoundingSphere(center,radius);
#endif
	
	center += offset;

	int vertexIndexFound = -1;	

	float closetPtRadius = 0.05f * radius;
	Vector3 closestVertex = VEC3V_TO_VECTOR3( geomPoints::FindClosestPointSegToPoint( VECTOR3_TO_VEC3V(mouseScreen), VECTOR3_TO_VEC3V(mouseFar), VECTOR3_TO_VEC3V(center)) );	
	center.Subtract( closestVertex );

	if( center.Mag2() < radius*radius  )
	{
		Assert( clothVertices );
		float closetPtRadiusSQR = closetPtRadius * closetPtRadius;

		for( int i = startVertex; i < endVertex; ++i )
		{
			Vector3 vtx = clothVertices[i] + offset;
			Vector3 closestClothVertex = VEC3V_TO_VECTOR3( geomPoints::FindClosestPointSegToPoint( VECTOR3_TO_VEC3V(mouseScreen), VECTOR3_TO_VEC3V(mouseFar), VECTOR3_TO_VEC3V(vtx)) );	
			Vector3 tempVec = closestClothVertex;

			tempVec.Subtract( vtx );
			const float closestDistSQR = tempVec.Mag2();

			if( closestDistSQR < closetPtRadiusSQR )
			{
				closetPtRadiusSQR = closestDistSQR;
				vertexIndexFound = i;
			}
		}
	}

	return vertexIndexFound;
}


int phVerletCloth::PickEdges( const Vector3* RESTRICT clothVertices, const Vector3& offset, Vector3& mouseScreen, Vector3& mouseFar, int& vertexIndexFound, int* edgesFound, const atArray<phEdgeData>& edgeData )
{
	int startVertex = 0;	
	int endVertex = GetNumVertices();
	vertexIndexFound = PickVertices( clothVertices, offset, startVertex, endVertex, mouseScreen, mouseFar );
	if( vertexIndexFound != -1 )
		return SearchEdges( clothVertices, offset, vertexIndexFound, edgesFound, edgeData );
	return 0;
}



#endif	// __BANK && !__RESOURCECOMPILER

#if __PFDRAW
void phVerletCloth::DrawPickedEdges( const Vector3* RESTRICT clothVertices, const Vector3& offset, const int edgesFoundCount, const int* RESTRICT edgesFound, const atArray<phEdgeData>& edgeData, Color32* RESTRICT edgesColors )
{
	if(	PFD_PickEdges.Begin(true))
	{
		Assert( clothVertices );
		char buf[8];

		for (int i=0; i<edgesFoundCount; ++i)
		{			
			const int edgeIndex = edgesFound[i];
			const phEdgeData& edge = edgeData[ edgeIndex ];
			const int idx0 = edge.m_vertIndices[0];
			const int idx1 = edge.m_vertIndices[1];

			Vector3 v0 = clothVertices[ idx0 ] + offset;
			Vector3 v1 = clothVertices[ idx1 ] + offset;
			grcDrawLine( v0, v1, edgesColors? edgesColors[i]: Color_red );

			formatf( buf, "%d", edgeIndex );
			Vector3 mid = (v0 + v1) * 0.5f;
			grcColor( edgesColors? edgesColors[i]: Color_red );
			grcDrawLabelf( mid, buf );

			if( edgesColors )
				edgesColors[i] = Color_green;
		}

		PFD_PickEdges.End();
	}
}
#endif // #if __PFDRAW


int phVerletCloth::SearchEdges( const Vector3* RESTRICT 
#if __PFDRAW
	clothVertices
#endif
	, const Vector3& 
#if __PFDRAW
	offset
#endif
	, const int vtxIndex, int* edgesFound, const atArray<phEdgeData>& edgeData )
{
#if __ASSERT
	const int MAX_EDGES_TO_DEBUG = 48;
#endif

	Assert( vtxIndex > -1 && vtxIndex < GetNumVertices() );
	int edgesFoundCount = 0;

	const int numEdges = edgeData.GetCount();
	for (int edgeIndex=0; edgeIndex<numEdges; ++edgeIndex)
	{
		const phEdgeData* edge = &edgeData[ edgeIndex ];
		Assert( edge );
		const int idx0 = edge->m_vertIndices[0];
		const int idx1 = edge->m_vertIndices[1];

		if( (idx0 == idx1 && idx0 == vtxIndex)
			|| idx0 == vtxIndex 
			|| idx1 == vtxIndex 
			)
		{
			Assert( edgesFoundCount < MAX_EDGES_TO_DEBUG );
			edgesFound[edgesFoundCount++] = edgeIndex;
		}
	}

#if __PFDRAW
	DrawPickedEdges( clothVertices, offset, edgesFoundCount, edgesFound, edgeData );
#endif
	return edgesFoundCount;
}



#if !__SPU

// TODO: 

// - always load edges into the instance, not the type ... in this way edges can be replaced dynamically in the game
// - if the instance doesn't have custom edges then use the custom edges from the type
// - a set of 16 dummy edges in the instance is needed to be able to turn off custom edges
// - add option to save the type edges or the instance edges !


void phVerletCloth::Load(void* controllerName, const char* filePath )
{
	Assert( controllerName );

	char buff[256];
	formatf( buff, "%s%s%s", filePath, (char*)controllerName, phVerletCloth::sm_MetaDataFileAppendix );
	phVerletCloth* verletCloth = /*m_VerletClothType ? m_VerletClothType:*/ this;

	parSettings s = parSettings::sm_StrictSettings; 
	s.SetFlag(parSettings::USE_TEMPORARY_HEAP, true); 
	PARSER.LoadObject( buff, "xml", *verletCloth, &s);

	clothDebugf1("Loaded cloth edges from file: %s", buff);
}
void phVerletCloth::Save(
#if __FINAL
	void* , const char*
#else
	void* controllerName, const char* filePath
#endif
	)
{
#if !__FINAL
	Assert( controllerName );

	char buff[256];
	formatf( buff, "%s%s%s", filePath, (char*)controllerName, phVerletCloth::sm_MetaDataFileAppendix );
// TODO: use THIS when saving custom edges
	phVerletCloth* verletCloth = m_VerletClothType ? const_cast<phVerletCloth*>((const phVerletCloth*)m_VerletClothType): this;
	AssertVerify(PARSER.SaveObject( buff, "xml", verletCloth, parManager::XML));

	clothDebugf1("Saved cloth edges to file: %s", buff);
#endif
}

#if __BANK && !__RESOURCECOMPILER
phVerletCloth* s_CurrentCloth = NULL;
bool s_RegisterClothRestInterface = false;

void phVerletCloth::RegisterRestInterface(const char* controllerName)
{
	if( !s_RegisterClothRestInterface )
	{		
		s_RegisterClothRestInterface = true;
		s_CurrentCloth = const_cast<phVerletCloth*>(m_VerletClothType ? m_VerletClothType: this);
		parRestRegisterSingleton("Physics/Cloth/Verlet", *s_CurrentCloth, NULL);

		clothDebugf1("Registered Physics/Cloth/Verlet REST interface for: %s", controllerName);

		s_CurrentCloth->GetClothData().RegisterRestInterface( controllerName );
	}
}

void phVerletCloth::UnregisterRestInterface(const char* controllerName)
{
	if( s_RegisterClothRestInterface )
	{	
		s_CurrentCloth->GetClothData().UnregisterRestInterface( controllerName );

		s_RegisterClothRestInterface = false;
		s_CurrentCloth = NULL;
		REST.RemoveAndDeleteService("Physics/Cloth/Verlet");

		clothDebugf1("Unregistered Physics/Cloth/Verlet REST interface for: %s", controllerName);
	}
}
#endif // __BANK

#endif // !__SPU

}	// namespace rage
