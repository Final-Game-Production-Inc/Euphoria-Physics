// 
// pheffects/clothdata.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//


#include "clothdata.h"
#include "data/safestruct.h"
#include "math/simplemath.h"
#include "phcore/segment.h"
#include "system/cache.h"
#include "system/memory.h"
#include "vector/geometry.h"

#if __BANK && !__RESOURCECOMPILER
#include "parser/restparserservices.h"
#endif

#include "clothdata_parser.h"

//#pragma optimize("" ,off)

#if !__SPU
PARAM(clothxmlformat, "load/save cloth data in vec3v or u32 format");
#endif

namespace rage {

#if !__SPU
const char* phClothData::sm_tuneFilePath = "common:/data/cloth/";
const char* phClothData::sm_MetaDataFileAppendix = "_data";
#endif


phClothData::phClothData () 
	: m_Flags( FLAG_COMPRESSION )
	, m_SwitchDistanceUp( 9999.0f )
	, m_SwitchDistanceDown( 9999.0f ) 
#if NO_PIN_VERTS_IN_VERLET
	, m_NumPinVerts(0)
#endif
#if NO_VERTS_IN_VERLET
	, m_NumVerts(0)
#endif
{
}

phClothData::phClothData(class datResource& rsc) 
	: m_VertexPinnedPositions(rsc,true)
	, m_VertexInitialNormals(rsc,true)
	, m_VertexPositions(rsc,true)
	, m_VertexPrevPositions(rsc,true)
{
}

phClothData::~phClothData () 
{
	clothDebugf1("Delete phClothData");
}

#if __DECLARESTRUCT
void phClothData::DeclareStruct(datTypeStruct &s)
{
	pgBase::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE( phClothData, pgBase )
		SSTRUCT_FIELD( phClothData, m_VertexPinnedPositions )
		SSTRUCT_FIELD( phClothData, m_VertexInitialNormals )
		SSTRUCT_FIELD( phClothData, m_VertexPositions )
		SSTRUCT_FIELD( phClothData, m_VertexPrevPositions )

		SSTRUCT_FIELD( phClothData, m_NumVerts )
		SSTRUCT_FIELD( phClothData, m_NumPinVerts )
#if RSG_CPU_X64
		SSTRUCT_CONTAINED_ARRAY( phClothData, m_Padding )
#endif
		SSTRUCT_FIELD( phClothData, m_SwitchDistanceUp )
		SSTRUCT_FIELD( phClothData, m_SwitchDistanceDown )
		SSTRUCT_FIELD( phClothData, m_Flags )
		SSTRUCT_END( phClothData)
}
#endif

IMPLEMENT_PLACE(phClothData);


#if !__SPU

// TODO: we want to be able to deal with instance of the cloth,
// the name should have something unique about the cloth instance ... position ( for non-moving env cloth ) ?

void phClothData::Load(const char* filePath, const char* clothName)
{
	Assert(clothName);
	char buff[256];
	formatf( buff, "%s%s%s", filePath, clothName, phClothData::sm_MetaDataFileAppendix );
	parSettings s = parSettings::sm_StrictSettings; 
	s.SetFlag(parSettings::USE_TEMPORARY_HEAP, true); 

#if !__FINAL && !__RESOURCECOMPILER
	int xmlIdxType = !PARAM_clothxmlformat.Get(xmlIdxType) ? 1: 0;
	if( !xmlIdxType )
		PARSER.LoadObject( buff, "xml", *(phClothDataDebug*)this, &s);
	else
#endif
		PARSER.LoadObject( buff, "xml", *this, &s);

#if !__FINAL && !__RESOURCECOMPILER
	const char* xmlDesc[] = { "Vec3V format", "U32 format" };
	clothDebugf1("Loaded cloth data from file(%s): %s", xmlDesc[xmlIdxType], buff);
#endif
}

void phClothData::Save(void* 
#if !__FINAL
	clothName
#endif
	)
{
#if !__FINAL
	Assert( clothName );
	char buff[256];
	formatf( buff, "%s%s%s", phClothData::sm_tuneFilePath, (char*)clothName, phClothData::sm_MetaDataFileAppendix );

	int xmlIdxType = !PARAM_clothxmlformat.Get(xmlIdxType) ? 1: 0;
	if( !xmlIdxType )
		AssertVerify(PARSER.SaveObject( buff, "xml", (phClothDataDebug*)this, parManager::XML));
	else
		AssertVerify(PARSER.SaveObject( buff, "xml", this, parManager::XML));

	const char* xmlDesc[] = { "Vec3V format", "U32 format" };
	clothDebugf1("Saved cloth data to file(%s): %s", xmlDesc[xmlIdxType], buff);

#endif // !__FINAL
}

#if __BANK && !__RESOURCECOMPILER
phClothData* s_CurrentClothData = NULL;
bool s_RegisterClothDataRestInterface = false;

void phClothData::RegisterRestInterface(const char* controllerName)
{
	if( !s_RegisterClothDataRestInterface )
	{		
		s_RegisterClothDataRestInterface = true;
		s_CurrentClothData = this;
		parRestRegisterSingleton("Physics/Cloth/ClothData", *s_CurrentClothData, NULL);

		clothDebugf1("Registered Physics/Cloth/ClothData REST interface for: %s", controllerName);
	}
}

void phClothData::UnregisterRestInterface(const char* controllerName)
{
	if( s_RegisterClothDataRestInterface )
	{		
		s_RegisterClothDataRestInterface = false;
		s_CurrentClothData = NULL;
		REST.RemoveAndDeleteService("Physics/Cloth/ClothData");

		clothDebugf1("Unregistered Physics/Cloth/ClothData REST interface for: %s", controllerName);
	}
}
#endif // __BANK

#endif // !__SPU


#if CLOTH_INSTANCE_FROM_DATA
void phClothData::InstanceFromData(int vertsCapacity)
{
	// m_SwitchDistanceUp - default for now, no cloth lods
	// m_SwitchDistanceDown - default for now, no cloth lods

	// TODO: generate mesh from some code ... or read it through REST ?
	//	m_NumPinVerts
	//	m_NumVerts

	Assert( !m_VertexPositions.GetCount() );
	m_VertexPositions.Resize(vertsCapacity);
	Assert( m_VertexPositions.GetCount() );

// TODO: need only by character cloth
	Assert( !m_VertexInitialNormals.GetCount() );
	m_VertexInitialNormals.Resize(vertsCapacity);
	Assert( m_VertexInitialNormals.GetCount() );
	

	for (int i=0; i < m_NumVerts; ++i)
	{
//		m_VertexPositions[i] = copyme->m_VertexPositions[i];
		m_VertexPositions[i].SetW( MAGIC_ZERO );
	}

}
#endif // CLOTH_INSTANCE_FROM_DATA



#if NO_PIN_VERTS_IN_VERLET
void phClothData::InstanceFromTemplate( const phClothData* copyme )
#else
void phClothData::InstanceFromTemplate( const phClothData* copyme, const int numPinVerts )
#endif
{
	Assert( copyme );

	m_SwitchDistanceUp = copyme->m_SwitchDistanceUp;
	m_SwitchDistanceDown = copyme->m_SwitchDistanceDown;
	m_Flags = copyme->m_Flags;

#if NO_PIN_VERTS_IN_VERLET
	m_NumPinVerts = copyme->m_NumPinVerts;
#endif

#if NO_VERTS_IN_VERLET
	m_NumVerts = copyme->m_NumVerts;
#endif

 #if NO_PIN_VERTS_IN_VERLET
	const int numPinVerts = copyme->m_NumPinVerts;
 #endif

	if( numPinVerts )
	{
		m_VertexPinnedPositions.Resize(numPinVerts);
		const Vec3V* RESTRICT copyVerts = (copyme->GetPinVertexCapacity() > 0) ? copyme->m_VertexPinnedPositions.GetElements(): copyme->m_VertexPositions.GetElements();
		Assert( copyVerts );
		for (int i=0; i < numPinVerts; ++i)
		{
			m_VertexPinnedPositions[i] = copyVerts[i];
			m_VertexPinnedPositions[i].SetW( MAGIC_ZERO );
		}
	}

	const int vertCount = copyme->m_VertexPositions.GetCount();
	Assert( vertCount );
	Assert( !m_VertexPositions.GetCount() );
	m_VertexPositions.Resize(vertCount);
	Assert( m_VertexPositions.GetCount() );

	for (int i=0; i < vertCount; ++i)
	{
		m_VertexPositions[i] = copyme->m_VertexPositions[i];
		m_VertexPositions[i].SetW( MAGIC_ZERO );
	}

	const int normalsCount = copyme->m_VertexInitialNormals.GetCount();
	if( normalsCount)
	{
		Assert( !m_VertexInitialNormals.GetCount() );
		m_VertexInitialNormals.Resize(normalsCount);
		Assert( m_VertexInitialNormals.GetCount() );
		for (int i=0; i < normalsCount; ++i)
		{
			m_VertexInitialNormals[i] = copyme->m_VertexInitialNormals[i];
		}
	}

	clothDebugf1("phClothData::InstanceFromTemplate: vert count %d", vertCount);
}


void phClothData::Init( const Vec3V* vertices, int numVertices, Vec3V_In gravity, bool allocNormals, int extraVerts )
{
	int totalVerts = numVertices + extraVerts;
	if( allocNormals )
		m_VertexInitialNormals.Resize(totalVerts);

	Assert( totalVerts );
	m_VertexPositions.Resize( totalVerts );

	if (IsFlagSet(FLAG_NO_COMPRESSION))
	{
		m_VertexPrevPositions.Resize( totalVerts );
		Vec3V v(V_ZERO);
		v.SetW( MAGIC_ZERO );
		for (int vertexIndex = 0; vertexIndex < totalVerts; ++vertexIndex)
		{
			SetVertexPrevPosition( vertexIndex, v );
		}
	}

	for (int i=0; i < totalVerts; ++i)
	{
		m_VertexPositions[i] = vertices[i];			
	}

	if( allocNormals )
	{
		for (int i=0; i < totalVerts; ++i)
			m_VertexInitialNormals[i] = gravity;
	}
}


void phClothData::TransformVertexPositions( Mat34V_In T, const int vertIdxStart )
{
	const int numVertices = m_VertexPositions.GetCount();

	Vec3V* RESTRICT verts = m_VertexPositions.GetElements();
	Assert( !numVertices || verts );
	const int numVerts8 = ((numVertices-vertIdxStart) >> 3)<<3;
	int i;
	for ( i = vertIdxStart; i < numVerts8; i += 8 )
	{
		PrefetchDC( verts + 8 );
		const int	i0 = i,
			i1 = i+1,
			i2 = i+2,
			i3 = i+3,
			i4 = i+4,
			i5 = i+5,
			i6 = i+6,
			i7 = i+7;

		Vec3V v0 = Transform( T, verts[i0] );
		Vec3V v1 = Transform( T, verts[i1] );
		Vec3V v2 = Transform( T, verts[i2] );
		Vec3V v3 = Transform( T, verts[i3] );
		Vec3V v4 = Transform( T, verts[i4] );
		Vec3V v5 = Transform( T, verts[i5] );
		Vec3V v6 = Transform( T, verts[i6] );
		Vec3V v7 = Transform( T, verts[i7] );

		v0.SetW( MAGIC_ZERO );
		v1.SetW( MAGIC_ZERO );
		v2.SetW( MAGIC_ZERO );
		v3.SetW( MAGIC_ZERO );
		v4.SetW( MAGIC_ZERO );
		v5.SetW( MAGIC_ZERO );
		v6.SetW( MAGIC_ZERO );
		v7.SetW( MAGIC_ZERO );

		verts[i0] = v0;
		verts[i1] = v1;
		verts[i2] = v2;
		verts[i3] = v3;
		verts[i4] = v4;
		verts[i5] = v5;
		verts[i6] = v6;
		verts[i7] = v7;
	}
	for (; i<numVertices; ++i)
	{
		verts[i] = Transform( T, verts[i] );
		verts[i].SetW( MAGIC_ZERO );
	}
}

// Changed the name from SWAP to SWAP_ due to a name collision in linearalgebra.h.
#define SWAP_(TYPE,A,B) { TYPE savedA = A; A = B; B = savedA; }

void phClothData::SwapVertex (int oldIndex, int newIndex, phClothConnectivityData* connectivity, bool hasNormals)
{
	if( oldIndex!=newIndex )
	{	
		SWAP_( Vec3V, m_VertexPositions[oldIndex], m_VertexPositions[newIndex] );

		const int numPinVerts = m_VertexPinnedPositions.GetCapacity();
		if( numPinVerts )
		{
			if( newIndex < numPinVerts )
			{
				m_VertexPinnedPositions[newIndex] = m_VertexPositions[newIndex];
			}
		}

		// NOTE: prev positons are used by vehicle cloth damage, and not used by non-vehicle cloth
		const int numDamageOffsets = m_VertexPrevPositions.GetCapacity();
		if( numDamageOffsets > 0 )
		{
			Assert( numDamageOffsets == numPinVerts );
			if( newIndex < numDamageOffsets )
			{
				m_VertexPrevPositions[newIndex] = m_VertexPositions[newIndex];
			}
		}

		if( hasNormals )
		{
			SWAP_( Vec3V, m_VertexInitialNormals[oldIndex], m_VertexInitialNormals[newIndex] );
		}

		if (connectivity)
		{
			// Swap the old and new vertices in all the connecting edges.
			const int numEdges = connectivity->m_EdgeToVertexIndices.GetCount();
			for (int i = 0; i < numEdges; i++)
			{
				if (connectivity->m_EdgeToVertexIndices[i][0] == (u16)oldIndex)
				{
					connectivity->m_EdgeToVertexIndices[i][0] = (u16)newIndex;
				}
				else if (connectivity->m_EdgeToVertexIndices[i][0] == (u16)newIndex)
				{
					connectivity->m_EdgeToVertexIndices[i][0] = (u16)oldIndex;
				}

				if (connectivity->m_EdgeToVertexIndices[i][1] == (u16)oldIndex)
				{
					connectivity->m_EdgeToVertexIndices[i][1] = (u16)newIndex;
				}
				else if (connectivity->m_EdgeToVertexIndices[i][1] == (u16)newIndex)
				{
					connectivity->m_EdgeToVertexIndices[i][1] = (u16)oldIndex;
				}
			}

			for (int i = 0; i < MAX_EDGES_ON_VERTEX + 1; i++)
			{
				u16 temp = connectivity->m_VertexToEdgeIndices[oldIndex][i];
				connectivity->m_VertexToEdgeIndices[oldIndex][i] = connectivity->m_VertexToEdgeIndices[newIndex][i];
				connectivity->m_VertexToEdgeIndices[newIndex][i] = temp;
			}

			// we could get to triangle from edges... but that's too much work.
			int nTri = connectivity->m_TriangleToVertexIndices.GetCount();
			for (int iTri = 0; iTri < nTri; iTri++)
			{
				for (int ii = 0; ii < 3; ii++)
				{
					if( connectivity->m_TriangleToVertexIndices[iTri][ii] == (u16)newIndex )
					{
						connectivity->m_TriangleToVertexIndices[iTri][ii] = (u16)oldIndex;
					}
					else if( connectivity->m_TriangleToVertexIndices[iTri][ii] == (u16)oldIndex )
					{
						connectivity->m_TriangleToVertexIndices[iTri][ii] = (u16)newIndex;
					}
				}
			}
		}
	}
}


int phClothData::FindVertsNearSegment( int numVertices, Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In radiusSquared, int* verts, int maxNearVerts, Vec3V_In vOffset) const
{
	Vec3V segment = Subtract(segmentEnd,segmentStart);
	/// ScalarV nearestDist2 = ScalarV(V_FLT_LARGE_8);

	int firstIndex = 0;
	int count = 0;
	for (int vertexIndex=0; vertexIndex<numVertices; vertexIndex++)
	{
		Vec3V vertexPosition = Add( GetVertexPosition(vertexIndex), vOffset );
		Vec3V startToVertex = Subtract(segmentStart,vertexPosition);
		Vec3V vertexToEnd = Subtract(vertexPosition,segmentEnd);
		ScalarV toVertexDotSeg = Dot(startToVertex,segment);
		ScalarV toEndDotSeg = Dot(vertexToEnd,segment);
		Vec3V nearestPoint = AddScaled(segmentStart,segment,Scale(toVertexDotSeg,Invert(Add(toVertexDotSeg,toEndDotSeg))));
		ScalarV dist2 = DistSquared(nearestPoint,vertexPosition);
		if( IsLessThanAll(dist2,radiusSquared) )
		{
			int nextVert = (firstIndex+count)%maxNearVerts;
			bool queueFull = (count >= maxNearVerts) ? true: false;
			verts[nextVert] = vertexIndex;
			count = queueFull ? count: (count+1);

			firstIndex += (int)queueFull;
			firstIndex = (firstIndex >= maxNearVerts) ? 0: firstIndex;
		}
	}

	return count;
}


void phClothData::VerifyMesh( phClothConnectivityData * ASSERT_ONLY(connectivity) )
{
#if __DEV && __ASSERT

	if( connectivity )
	{
		const int nEdges = connectivity->m_EdgeToTriangleIndices.GetCount();
		for( int iEdge = 0; iEdge < nEdges; iEdge++ )
		{
			int t0 = connectivity->GetEdgeToTriangleIndices(iEdge)[0];
			int t1 = connectivity->GetEdgeToTriangleIndices(iEdge)[1];

			if( t0 != BAD_POLY_INDEX ){
				ASSERT_ONLY(int e0 =) connectivity->GetTriangleEdges(t0)[0];
				ASSERT_ONLY(int e1 =) connectivity->GetTriangleEdges(t0)[1];
				ASSERT_ONLY(int e2 =) connectivity->GetTriangleEdges(t0)[2];

				//				Assert( iEdge == e0 || iEdge == e1 || iEdge == e2 );

				Assert( e0 != e1 && e0 != e2 );
				Assert( e1 != e0 && e1 != e2 );
				Assert( e2 != e1 && e2 != e0 );
			}

			if( t1 != BAD_POLY_INDEX ){
				ASSERT_ONLY(int e01 =) connectivity->GetTriangleEdges(t1)[0];
				ASSERT_ONLY(int e11 =) connectivity->GetTriangleEdges(t1)[1];
				ASSERT_ONLY(int e21 =) connectivity->GetTriangleEdges(t1)[2];

				//				Assert( iEdge == e01 || iEdge == e11 || iEdge == e21 );

				Assert( e01 != e11 && e01 != e21 );
				Assert( e11 != e01 && e11 != e21 );
				Assert( e21 != e11 && e21 != e01 );
			}
		}
	}

	if( connectivity )
	{
		const int nVerts = connectivity->m_VertexToEdgeIndices.GetCount();
		for( int iVert = 0; iVert < nVerts; iVert++ )
		{
			int nEdge = connectivity->GetVertexToEdgeIndices(iVert)[MAX_EDGES_ON_VERTEX];
			for( int iEdge = 0; iEdge < nEdge; iEdge++ )
			{
				ASSERT_ONLY(int edgeFromVert = connectivity->GetVertexToEdgeIndices(iVert)[iEdge];)
					Assert(edgeFromVert>=0);

				ASSERT_ONLY(int v0 = connectivity->m_EdgeToVertexIndices[edgeFromVert][0];)
					ASSERT_ONLY(int v1 = connectivity->m_EdgeToVertexIndices[edgeFromVert][1];)

					Assert( v0 == iVert || v1 == iVert );

			}
		}
	}

	if(0)
	{
		for( int iTri = 0; iTri < connectivity->GetNumTriangles(); iTri++ )
		{
			const u16* ti0 = connectivity->GetTriangleIndices( iTri );
			Vec3V v0 = GetVertexPosition(ti0[0]);
			Vec3V v1 = GetVertexPosition(ti0[0]);
			Vec3V v2 = GetVertexPosition(ti0[0]);

			Vec3V e10 = Subtract(v1,v0);
			Vec3V e20 = Subtract(v2,v0);

			Vec3V n = Cross( e10, e20 );

			Assert( Mag(n).Getf() > FLT_EPSILON );
		}
	}
#endif
}


int phClothData::GetVertexOppositeEdge (int triIndex, int edgeIndex, const phClothConnectivityData& connectivity) const
{
	const u16 *aVert = connectivity.GetTriangleIndices( triIndex );

	const int cullVert0 = connectivity.m_EdgeToVertexIndices[edgeIndex][0];
	const int cullVert1 = connectivity.m_EdgeToVertexIndices[edgeIndex][1];

	if( aVert[0] != cullVert0 && aVert[0] != cullVert1 )
	{
		return aVert[0];
	}

	if( aVert[1] != cullVert0 && aVert[1] != cullVert1 )
	{
		return aVert[1];
	}

	if( aVert[2] != cullVert0 && aVert[2] != cullVert1 )
	{
		return aVert[2];
	}

	AssertMsg( 0 , "something is awry" );

	return BAD_POLY_INDEX;
}


}	// namespace rage
