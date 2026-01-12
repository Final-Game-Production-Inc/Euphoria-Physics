//
// pheffects/clothconnectivitydata.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_CLOTHCONNECTIVITYDATA_H
#define PHEFFECTS_CLOTHCONNECTIVITYDATA_H

#define	MAX_EDGES_ON_VERTEX			22

#include "atl/array.h"
#include "atl/array_struct.h"
#include "atl/bitset.h"
#include "data/struct.h"
#include "phbound/boundgeom.h"
#include "phbound/primitives.h"
#include "vector/vector3.h"


namespace rage {

template<class T, int N>
class indxA
{
public:
	typedef indxA<T,N>  _Type_;

	DECLARE_PLACE( _Type_ );

	indxA<T,N>(class datResource& rsc);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	operator const	T*()  const	{ return m_ints; }
	operator		T*()		{ return m_ints; }

	indxA<T,N>(){}

	indxA<T,N>( const _Type_ &other )
	{
		for( int i = 0; i < N; i++ )
			m_ints[i] = other.m_ints[i];
	}
	T operator[](int i) const
	{
		return m_ints[i];
	}

	T &operator[](int i)
	{
		return m_ints[i];
	}
	T m_ints[N];
};

typedef indxA< u16, 2> indxA2;
typedef indxA< u16, 3> indxA3;



class phClothConnectivityData
{
public:

	DECLARE_PLACE(phClothConnectivityData);

	phClothConnectivityData(class datResource& rsc);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	phClothConnectivityData(){}

	void Init( int nVert, int nEdge, int nTri );

	int GetTriangleVertexIndex( int iTriangle, int triVert ) const { return m_TriangleToVertexIndices[iTriangle][triVert]; }
	const u16* GetVertexToEdgeIndices (int vertexIndex) const { return m_VertexToEdgeIndices[vertexIndex]; }
	const u16* GetEdgeToTriangleIndices (int edgeIndex) const { return m_EdgeToTriangleIndices[edgeIndex]; }
	int GetNumTriangles () const { return m_TriangleToEdgeIndices.GetCount(); }
	const u16* GetTriangleEdges( int iTriangle ) const { return m_TriangleToEdgeIndices[iTriangle]; }
	const u16* GetTriangleIndices( int iTriangle ) const { return m_TriangleToVertexIndices[iTriangle]; }

	void SetEdgeToTriangleIndices (int edgeIndex, int triangleIndex0, int triangleIndex1);
	void ConnectEdgeToVertices (int edgeIndex, int vertexIndex0, int vertexIndex1);
	void ConnectTriangle (int triangleIndex, int vertexIndex0, int vertexIndex1, int vertexIndex2);
	int FindCommonEdge (int vertexIndex0, int vertexIndex1) const;

	void BuildConnectivity( const phPolygon *polys, int nPoly );

	// triangle information
	atArray<indxA3> m_TriangleToEdgeIndices;
	// edge information
	atArray<indxA2> m_EdgeToTriangleIndices;
	// The last element in each m_VertexToEdgeIndices array holds the number of edges touching the vertex.
	atArray<indxA< u16, MAX_EDGES_ON_VERTEX+1> > m_VertexToEdgeIndices;
	atArray<indxA3> m_TriangleToVertexIndices;

	atArray<indxA2>	m_EdgeToVertexIndices;
};

} // namespace rage

#endif // end of #ifndef PHEFFECTS_CLOTHCONNECTIVITYDATA_H
