//
// pheffects/clothconnectivitydata.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//


#include "clothconnectivitydata.h"

#include "math/simplemath.h"
#include "phcore/segment.h"
#include "system/memory.h"
#include "vector/geometry.h"

namespace rage {


template<> indxA< u16, 2>::indxA(class datResource& UNUSED_PARAM(rsc) )
{
}
template<> IMPLEMENT_PLACE( indxA2 );

#if __DECLARESTRUCT
template<> void indxA< u16, 2>::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN( indxA2 );
	STRUCT_CONTAINED_ARRAY(m_ints);
	STRUCT_END();
}
#endif


template<> indxA< u16, 3 >::indxA(class datResource& UNUSED_PARAM(rsc) )
{
}
template<> IMPLEMENT_PLACE( indxA3 );

#if __DECLARESTRUCT
template<> void indxA< u16, 3 >::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN( indxA3 );
	STRUCT_CONTAINED_ARRAY(m_ints);
	STRUCT_END();
}
#endif

template<> indxA< u16,MAX_EDGES_ON_VERTEX+1>::indxA(class datResource& UNUSED_PARAM(rsc) )
{
}
typedef indxA<u16,MAX_EDGES_ON_VERTEX+1>	indxAN;

template<> IMPLEMENT_PLACE(indxAN);

#if __DECLARESTRUCT
template<> void indxA<u16,MAX_EDGES_ON_VERTEX+1>::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN( indxAN );
	STRUCT_CONTAINED_ARRAY(m_ints);
	STRUCT_END();
}
#endif

phClothConnectivityData::phClothConnectivityData(class datResource& rsc) :
m_TriangleToEdgeIndices(rsc,true)
, m_EdgeToTriangleIndices(rsc,true)
, m_VertexToEdgeIndices(rsc,true)
, m_TriangleToVertexIndices(rsc,true)
{
}


#if __DECLARESTRUCT
void phClothConnectivityData::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(phClothConnectivityData);
	STRUCT_FIELD(m_TriangleToEdgeIndices);
	STRUCT_FIELD(m_EdgeToTriangleIndices);
	STRUCT_FIELD(m_VertexToEdgeIndices);
	STRUCT_FIELD(m_TriangleToVertexIndices);
	STRUCT_END();
}
#endif

IMPLEMENT_PLACE(phClothConnectivityData);

void phClothConnectivityData::Init( int nVert, int nEdge, int nTri )
{
	m_TriangleToEdgeIndices.Resize( nTri );
	m_EdgeToTriangleIndices.Resize( nEdge );
	m_VertexToEdgeIndices.Resize( nVert );
	m_TriangleToVertexIndices.Resize( nTri );

	// Set the number of edges and the number of body polygons for each vertex to zero.
	for (int vertexIndex=0; vertexIndex<nVert; vertexIndex++)
	{
		u16* edgeIndices = m_VertexToEdgeIndices[vertexIndex];
		edgeIndices[MAX_EDGES_ON_VERTEX] = 0;
	}

	m_EdgeToVertexIndices.Resize( nEdge );
}


int phClothConnectivityData::FindCommonEdge (int vertexIndex0, int vertexIndex1) const
{
	const u16* edgeIndices0 = m_VertexToEdgeIndices[vertexIndex0];
	u16 numEdgesOnVertex0 = edgeIndices0[MAX_EDGES_ON_VERTEX];
	const u16* edgeIndices1 = m_VertexToEdgeIndices[vertexIndex1];
	u16 numEdgesOnVertex1 = edgeIndices1[MAX_EDGES_ON_VERTEX];
	int commonEdgeIndex = BAD_INDEX;
	u16 vertexEdgeIndex0 = 0;
	while (vertexEdgeIndex0<numEdgesOnVertex0 && commonEdgeIndex==BAD_INDEX)
	{
		u16 vertexEdgeIndex1 = 0;
		while (vertexEdgeIndex1<numEdgesOnVertex1 && commonEdgeIndex==BAD_INDEX)
		{
			if (edgeIndices0[vertexEdgeIndex0]==edgeIndices1[vertexEdgeIndex1])
			{
				commonEdgeIndex = edgeIndices0[vertexEdgeIndex0];
			}

			vertexEdgeIndex1++;
		}

		vertexEdgeIndex0++;
	}

	return commonEdgeIndex;
}


void phClothConnectivityData::SetEdgeToTriangleIndices (int edgeIndex, int triangleIndex0, int triangleIndex1)
{
	u16* edgeToTriangleIndices = m_EdgeToTriangleIndices[edgeIndex];
	edgeToTriangleIndices[0] = (u16)triangleIndex0;
	edgeToTriangleIndices[1] = (u16)triangleIndex1;
}

void phClothConnectivityData::ConnectEdgeToVertices (int edgeIndex, int vertexIndex0, int vertexIndex1)
{

	// Get the list of edge index numbers for the first vertex, and make sure there is one available.
	u16* edgeIndices = m_VertexToEdgeIndices[vertexIndex0];
	Assert(edgeIndices[MAX_EDGES_ON_VERTEX]<MAX_EDGES_ON_VERTEX);

	// Add the given edge index to the list of edges connecting the first vertex.
	edgeIndices[edgeIndices[MAX_EDGES_ON_VERTEX]] = (u16)edgeIndex;
	edgeIndices[MAX_EDGES_ON_VERTEX]++;

	// Get the list of edge index numbers for the second vertex, and make sure there is one available.
	edgeIndices = m_VertexToEdgeIndices[vertexIndex1];
	Assert(edgeIndices[MAX_EDGES_ON_VERTEX]<MAX_EDGES_ON_VERTEX);

	// Add the given edge index to the list of edges connecting the first vertex.
	edgeIndices[edgeIndices[MAX_EDGES_ON_VERTEX]] = (u16)edgeIndex;
	edgeIndices[MAX_EDGES_ON_VERTEX]++;
}


void phClothConnectivityData::ConnectTriangle (int triangleIndex, int vertexIndex0, int vertexIndex1, int vertexIndex2)
{

	// Find and set the connecting edge indices.
	u16* edgeIndices = m_TriangleToEdgeIndices[triangleIndex];
	edgeIndices[0] = (u16)FindCommonEdge(vertexIndex0,vertexIndex1);
	edgeIndices[1] = (u16)FindCommonEdge(vertexIndex1,vertexIndex2);
	edgeIndices[2] = (u16)FindCommonEdge(vertexIndex2,vertexIndex0);

	u16* triIndices = m_TriangleToVertexIndices[triangleIndex];
	triIndices[0] = (u16)vertexIndex0;
	triIndices[1] = (u16)vertexIndex1;
	triIndices[2] = (u16)vertexIndex2;
}


void phClothConnectivityData::BuildConnectivity( const phPolygon *polys, int nPoly )
{

	// Set the numbers of triangles and vertices in the cloth.
	int MaxNumTriangles = nPoly;
	int NumTriangles = MaxNumTriangles;

	// Find the number of edges in the cloth (the bound does not store edges).
	int NumEdges = 0;
	for (int triangleIndex=0; triangleIndex<NumTriangles; triangleIndex++)
	{
		const phPolygon& triangle = polys[triangleIndex];
		for (int neighborIndex=0; neighborIndex<3; neighborIndex++)
		{
			int neighborTriangleIndex = triangle.GetNeighboringPolyNum(neighborIndex);
			int usedNeighborIndex = (phPolygon::Index)(-1);
			for (int otherTriangleIndex=0; otherTriangleIndex<NumTriangles; otherTriangleIndex++)
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
				NumEdges++;
			}
		}
	}

	// Create the cloth triangles, edges and vertices.

	// Initialize the cloth edges from the bound.
	int edgeIndex = 0;
	for (int triangleIndex=0; triangleIndex<NumTriangles; triangleIndex++)
	{
		const phPolygon& triangle = polys[triangleIndex];

		AssertMsg( NumTriangles == 1 || (!(triangle.GetNeighboringPolyNum(0) == (phPolygon::Index)(-1) && triangle.GetNeighboringPolyNum(1) == (phPolygon::Index)(-1) && triangle.GetNeighboringPolyNum(2) == (phPolygon::Index)(-1))), 
			"You must have connectivity information for triangles, please call phBound::ComputeNeighbors() before creating the cloth" );

		for (int neighborIndex=0; neighborIndex<3; neighborIndex++)
		{
			int neighborTriangleIndex = triangle.GetNeighboringPolyNum(neighborIndex);
			int usedNeighborIndex = (phPolygon::Index)(-1);
			for (int otherTriangleIndex=0; otherTriangleIndex<NumTriangles; otherTriangleIndex++)
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
				SetEdgeToTriangleIndices(edgeIndex,triangleIndex,usedNeighborIndex);
				ConnectEdgeToVertices(edgeIndex,triangle.GetVertexIndex(neighborIndex),triangle.GetVertexIndex((neighborIndex+1)%3));
				edgeIndex++;
			}
		}

		// Set the triangle-edge and triangle-vertex connections. This also calculates the triangle normal and area,
		// even though it's already stores in the temporary triangle list.
		ConnectTriangle(triangleIndex,triangle.GetVertexIndex(0),triangle.GetVertexIndex(1),triangle.GetVertexIndex(2));
	}
	Assert(edgeIndex==NumEdges);

}

}	// namespace rage
