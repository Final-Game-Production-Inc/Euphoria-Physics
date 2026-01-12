//
// phcore/convexpoly.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

//
// Convex polygon (2D) routines
//

#ifndef PHCORE_CONVEXPOLY_H
#define PHCORE_CONVEXPOLY_H

namespace rage {

class Vector2;


// ConvexPolyIntersect() - Finds the extreme points of
//		the intersection of two convex polygons.
//   The two polygons are given by the sequence of vertices
//		in counterclockwise order.
//	 The values returned are given by specifying the edge(s)
//		and t value(s) of the points.
//   Return value is number of intersection points returned (or zero).

const int CPI_EE = 0;		// edge-to-edge intersection
const int CPI_VP = 1;		// vertex of A to polygonB.
const int CPI_PV = 2;		// polygonA to vertex of B.
const int CPI_NONE_EE = -1;	// No intersection, parallel edges witness
const int CPI_NONE_EV = -2;	// No intersection, edge of A and vert of B witness
const int CPI_NONE_VE = -3;	// No intersection, edge of B and vert of A witness

const int CPI_MAXPOLYSIDES = 4;


/*
PURPOSE
	Handle intersections between edges or rays and convex polygons.
*/
class phConvexPoly
{
public:
	struct Data
	{
		int IsectType;	// CPI_EE (0) - edge-to-edge
		int edgeA;		// A's edge number or vert number
		int edgeB;		// B's edge number or vert number
		float tA;		// t value for A's edge
		float tB;		// t value for B's edge
	};

public:
	static int ConvexPolyIntersect( int numA, const Vector2* vertsA,
							 int numB, const Vector2* vertsB,
							 phConvexPoly::Data* IntersectionList );

	static void PrecomputeRays( int num, const Vector2* verts,
						 Vector2* rays, float* dValues);

	static inline float SideTestA( const Vector2* pt );
	static inline float SideTestB( const Vector2* pt );
	static inline void SwitchAdvancingEdge();
	static void AdvanceV();
	static void GetvHeadOut();
	static void GetuHeadOut();
	static void GetuTailOut();
	static inline float GetHeadOut( bool switchedFlag );
	static void RecordEE();
	static void RecordUTail();
	static void RecordVTail();
	static void RecordTail( bool );
	static void RecordNoIsect( int );
	static void RecordInteriorCollides( bool );
};

} // namespace rage

#endif
