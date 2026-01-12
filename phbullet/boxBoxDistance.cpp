/*
   Copyright (C) 2006, 2008 Sony Computer Entertainment Inc.
   All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/

#include "boxBoxDistance.h"

#if ENABLE_UNUSED_PHYSICS_CODE

namespace rage {


static inline ScalarV_Out sqr( ScalarV_In a )
{
	return (a * a);
}

enum BoxSepAxisType
{
	A_AXIS, B_AXIS, CROSS_AXIS
};

enum FeatureType { F, E, V };

//-------------------------------------------------------------------------------------------------
// voronoiTol: bevels Voronoi planes slightly which helps when features are parallel.
//-------------------------------------------------------------------------------------------------

static const ScalarV voronoiTol = ScalarVFromF32(-1.0e-5f);

//-------------------------------------------------------------------------------------------------
// separating axis tests: gaps along each axis are computed, and the axis with the maximum
// gap is stored.  cross product axes are normalized.
//-------------------------------------------------------------------------------------------------

#define AaxisTest( dim, letter, first )                                                         \
{                                                                                               \
   if ( first )                                                                                 \
   {                                                                                            \
	  maxGap = gap = gapsA.Get##letter();														\
	  if ( IsGreaterThanAll(gap, distanceThreshold) ) return gap;                               \
	  axisType = A_AXIS;                                                                        \
	  faceDimA = dim;                                                                           \
	  axisA = identity.GetCol##dim();															\
   }                                                                                            \
   else                                                                                         \
   {                                                                                            \
	  gap = gapsA.Get##letter();																\
	  if ( IsGreaterThanAll(gap, distanceThreshold) ) return gap;                               \
	  else if ( IsGreaterThanAll(gap, maxGap) )                                                 \
	  {                                                                                         \
		 maxGap = gap;                                                                          \
		 axisType = A_AXIS;                                                                     \
		 faceDimA = dim;                                                                        \
		 axisA = identity.GetCol##dim();														\
	  }                                                                                         \
   }                                                                                            \
}


#define BaxisTest( dim, letter )                                                                \
{                                                                                               \
   gap = gapsB.Get##letter();                                                                   \
   if ( IsGreaterThanAll(gap, distanceThreshold) ) return gap;                                  \
   else if ( IsGreaterThanAll(gap, maxGap) )                                                    \
   {                                                                                            \
      maxGap = gap;                                                                             \
      axisType = B_AXIS;                                                                        \
      faceDimB = dim;                                                                           \
      axisB = identity.GetCol##dim();                                                           \
   }                                                                                            \
}

#define CrossAxisTest( dima, dimb, letterb )                                                    \
{                                                                                               \
   ScalarV lsqr;                                                                                \
                                                                                                \
   lsqr = lsqrs.GetCol##dima().Get##letterb();                                                  \
                                                                                                \
   if ( IsGreaterThanAll(lsqr, lsqr_tolerance) )                                                \
   {                                                                                            \
      ScalarV l_recip = InvSqrt( lsqr );                                                        \
      gap = gapsAxB.GetCol##dima().Get##letterb() * l_recip;                                    \
                                                                                                \
      if ( IsGreaterThanAll(gap, distanceThreshold) )                                           \
      {                                                                                         \
         return gap;                                                                            \
      }                                                                                         \
                                                                                                \
      if ( IsGreaterThanAll(gap, maxGap) )                                                      \
      {                                                                                         \
         maxGap = gap;                                                                          \
         axisType = CROSS_AXIS;                                                                 \
         edgeDimA = dima;                                                                       \
         edgeDimB = dimb;                                                                       \
         axisA = Cross(identity.GetCol##dima(),matrixAB.GetCol##dimb()) * l_recip;              \
      }                                                                                         \
   }                                                                                            \
}

//-------------------------------------------------------------------------------------------------
// tests whether a vertex of box B and a face of box A are the closest features
//-------------------------------------------------------------------------------------------------

inline
ScalarV_Out
VertexBFaceATest(
	bool & inVoronoi,
	ScalarV_InOut t0result,
	ScalarV_InOut t1result,
	Vec3V_In hA,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_In signsB,
	Vec3V_In scalesB )
{
	// compute a corner of box B in A's coordinate system

	Vec3V corner =
		faceOffsetAB + matrixAB.GetCol0() * scalesB.GetX() + matrixAB.GetCol1() * scalesB.GetY();

	// compute the parameters of the point on A, closest to this corner

	ScalarV t0 = corner.GetX();
	ScalarV t1 = corner.GetY();

	t0 = Min(t0, hA.GetX());
	t0 = Max(t0, -hA.GetX());
	t1 = Min(t1, hA.GetY());
	t1 = Max(t1, -hA.GetY());

	// do the Voronoi test: already know the point on B is in the Voronoi region of the
	// point on A, check the reverse.

	Vec3V facePointB =
		(faceOffsetBA + matrixBA.GetCol0() * t0 + matrixBA.GetCol1() * t1 - scalesB) * signsB;

	BoolV inVoronoiV = IsGreaterThanOrEqual(facePointB.GetX(), voronoiTol * facePointB.GetZ()) &
				  IsGreaterThanOrEqual(facePointB.GetY(), voronoiTol * facePointB.GetX()) &
				  IsGreaterThanOrEqual(facePointB.GetZ(), voronoiTol * facePointB.GetY());

	inVoronoi = IsTrue(inVoronoiV);

	t0result = t0;
	t1result = t1;

	return (sqr( corner.GetX() - t0 ) + sqr( corner.GetY() - t1 ) + sqr( corner.GetZ() ));
}

#define VertexBFaceA_SetNewMin()                \
{                                               \
   minDistSqr = distSqr;                        \
   localPointA.SetX(t0);                        \
   localPointA.SetY(t1);                        \
   localPointB.SetX( scalesB.GetX() );          \
   localPointB.SetY( scalesB.GetY() );          \
   featureA = F;                                \
   featureB = V;                                \
}

void
VertexBFaceATests(
	bool & done,
	ScalarV_InOut minDistSqr,
	Vec3V_InOut localPointA,
	Vec3V_InOut localPointB,
	FeatureType & featureA,
	FeatureType & featureB,
	Vec3V_In hA,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_InOut signsBresult,
	Vec3V_InOut scalesBresult,
	bool first )
{
	ScalarV t0, t1;
	ScalarV distSqr;

	Vec3V signsB = signsBresult;
	Vec3V scalesB = scalesBresult;

	distSqr = VertexBFaceATest( done, t0, t1, hA, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsB, scalesB );

	if ( first ) {
		VertexBFaceA_SetNewMin();
	} else {
		if ( IsTrue(distSqr < minDistSqr) ) {
			VertexBFaceA_SetNewMin();
		}
	}

	if ( done )
		goto AssignResults;

	signsB.SetX( -signsB.GetX() );
	scalesB.SetX( -scalesB.GetX() );

	distSqr = VertexBFaceATest( done, t0, t1, hA, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsB, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		VertexBFaceA_SetNewMin();
	}

	if ( done )
		goto AssignResults;

	signsB.SetY( -signsB.GetY() );
	scalesB.SetY( -scalesB.GetY() );

	distSqr = VertexBFaceATest( done, t0, t1, hA, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsB, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		VertexBFaceA_SetNewMin();
	}

	if ( done )
		goto AssignResults;

	signsB.SetX( -signsB.GetX() );
	scalesB.SetX( -scalesB.GetX() );

	distSqr = VertexBFaceATest( done, t0, t1, hA, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsB, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		VertexBFaceA_SetNewMin();
	}

AssignResults:
	signsBresult = signsB;
	scalesBresult = scalesB;
}

//-------------------------------------------------------------------------------------------------
// VertexAFaceBTest: tests whether a vertex of box A and a face of box B are the closest features
//-------------------------------------------------------------------------------------------------

inline
ScalarV_Out
VertexAFaceBTest(
	bool & inVoronoi,
	ScalarV_InOut t0result,
	ScalarV_InOut t1result,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_In signsA,
	Vec3V_In scalesA )
{
	Vec3V corner =
		faceOffsetBA + matrixBA.GetCol0() * scalesA.GetX() + matrixBA.GetCol1() * scalesA.GetY();

	ScalarV t0 = corner.GetX();
	ScalarV t1 = corner.GetY();

	t0 = Min(t0, hB.GetX());
	t0 = Max(t0, -hB.GetX());
	t1 = Min(t1, hB.GetY());
	t1 = Max(t1, -hB.GetY());

	Vec3V facePointA =
		(faceOffsetAB + matrixAB.GetCol0() * t0 + matrixAB.GetCol1() * t1 - scalesA) * signsA;

	BoolV inVoronoiV = IsGreaterThanOrEqual(facePointA.GetX(), voronoiTol * facePointA.GetZ()) &
		IsGreaterThanOrEqual(facePointA.GetY(), voronoiTol * facePointA.GetX()) &
		IsGreaterThanOrEqual(facePointA.GetZ(), voronoiTol * facePointA.GetY());

	inVoronoi = IsTrue(inVoronoiV);

	t0result = t0;
	t1result = t1;

	return (sqr( corner.GetX() - t0 ) + sqr( corner.GetY() - t1 ) + sqr( corner.GetZ() ));
}

#define VertexAFaceB_SetNewMin()                \
{                                               \
   minDistSqr = distSqr;                        \
   localPointB.SetX(t0);                        \
   localPointB.SetY(t1);                        \
   localPointA.SetX( scalesA.GetX() );          \
   localPointA.SetY( scalesA.GetY() );          \
   featureA = V;                                \
   featureB = F;                                \
}

void
VertexAFaceBTests(
	bool & done,
	ScalarV_InOut minDistSqr,
	Vec3V_InOut localPointA,
	Vec3V_InOut localPointB,
	FeatureType & featureA,
	FeatureType & featureB,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_InOut signsAresult,
	Vec3V_InOut scalesAresult,
	bool first )
{
	ScalarV t0, t1;
	ScalarV distSqr;

	Vec3V signsA = signsAresult;
	Vec3V scalesA = scalesAresult;

	distSqr = VertexAFaceBTest( done, t0, t1, hB, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsA, scalesA );

	if ( first ) {
		VertexAFaceB_SetNewMin();
	} else {
		if ( IsTrue(distSqr < minDistSqr) ) {
			VertexAFaceB_SetNewMin();
		}
	}

	if ( done )
		goto AssignResults;

	signsA.SetX( -signsA.GetX() );
	scalesA.SetX( -scalesA.GetX() );

	distSqr = VertexAFaceBTest( done, t0, t1, hB, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsA, scalesA );

	if ( IsTrue(distSqr < minDistSqr) ) {
		VertexAFaceB_SetNewMin();
	}

	if ( done )
		goto AssignResults;

	signsA.SetY( -signsA.GetY() );
	scalesA.SetY( -scalesA.GetY() );

	distSqr = VertexAFaceBTest( done, t0, t1, hB, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsA, scalesA );

	if ( IsTrue(distSqr < minDistSqr) ) {
		VertexAFaceB_SetNewMin();
	}

	if ( done )
		goto AssignResults;

	signsA.SetX( -signsA.GetX() );
	scalesA.SetX( -scalesA.GetX() );

	distSqr = VertexAFaceBTest( done, t0, t1, hB, faceOffsetAB, faceOffsetBA,
								matrixAB, matrixBA, signsA, scalesA );

	if ( IsTrue(distSqr < minDistSqr) ) {
		VertexAFaceB_SetNewMin();
	}

AssignResults:
	signsAresult = signsA;
	scalesAresult = scalesA;
}

//-------------------------------------------------------------------------------------------------
// EdgeEdgeTest:
//
// tests whether a pair of edges are the closest features
//
// note on the shorthand:
// 'a' & 'b' refer to the edges.
// 'c' is the dimension of the axis that points from the face center to the edge Center
// 'd' is the dimension of the edge Direction
// the dimension of the face normal is 2
//-------------------------------------------------------------------------------------------------

#define EdgeEdgeTest( ac, ac_letter, ad, ad_letter, bc, bc_letter, bd, bd_letter )					\
{																									\
	Vec3V edgeOffsetAB;																				\
	Vec3V edgeOffsetBA;																				\
																									\
	edgeOffsetAB = faceOffsetAB + matrixAB.GetCol##bc() * scalesB.Get##bc_letter();					\
	edgeOffsetAB.Set##ac_letter( edgeOffsetAB.Get##ac_letter() - scalesA.Get##ac_letter() );		\
																									\
	edgeOffsetBA = faceOffsetBA + matrixBA.GetCol##ac() * scalesA.Get##ac_letter();					\
	edgeOffsetBA.Set##bc_letter( edgeOffsetBA.Get##bc_letter() - scalesB.Get##bc_letter() );		\
																									\
	ScalarV dirDot = matrixAB.GetCol##bd().Get##ad_letter();										\
	ScalarV denom = ScalarV(V_ONE) - dirDot*dirDot;											\
	ScalarV edgeOffsetAB_ad = edgeOffsetAB.Get##ad_letter();										\
	ScalarV edgeOffsetBA_bd = edgeOffsetBA.Get##bd_letter();										\
																									\
	ScalarV tA = (edgeOffsetAB_ad + edgeOffsetBA_bd * dirDot) *										\
					InvertSafe(denom, ScalarV(V_ZERO));										\
																									\
	tA = Min(tA, hA.Get##ad_letter());																\
	tA = Max(tA, -hA.Get##ad_letter());																\
																									\
	ScalarV tB = tA * dirDot + edgeOffsetBA_bd;														\
																									\
	ScalarV newtB1 = -hB.Get##bd_letter();															\
	ScalarV newtA1 = newtB1 * dirDot + edgeOffsetAB_ad;												\
	newtA1 = Min(newtA1, hA.Get##ad_letter());														\
	newtA1 = Max(newtA1, -hA.Get##ad_letter());														\
																									\
	BoolV tBLessThanhBbd1 = IsLessThan(tB, -hB.Get##bd_letter());									\
																									\
	tA = SelectFT(tBLessThanhBbd1, tA, newtA1);														\
	tB = SelectFT(tBLessThanhBbd1, tB, newtB1);														\
																									\
	ScalarV newtB2 = hB.Get##bd_letter();															\
	ScalarV newtA2 = newtB2 * dirDot + edgeOffsetAB_ad;												\
	newtA2 = Min(newtA2, hA.Get##ad_letter());														\
	newtA2 = Max(newtA2, -hA.Get##ad_letter());														\
																									\
	BoolV tBGreaterThanhBbd2 = IsGreaterThan(tB, hB.Get##bd_letter());								\
																									\
	tA = SelectFT(tBGreaterThanhBbd2, tA, newtA2);													\
	tB = SelectFT(tBGreaterThanhBbd2, tB, newtB2);													\
																									\
	Vec3V edgeOffAB = ( edgeOffsetAB + matrixAB.GetCol##bd() * tB) * signsA;						\
	Vec3V edgeOffBA = ( edgeOffsetBA + matrixBA.GetCol##ad() * tA) * signsB;						\
																									\
	BoolV inVoronoiV =																				\
		IsGreaterThanOrEqual(edgeOffAB.Get##ac_letter(), voronoiTol * edgeOffAB.GetZ()) &			\
		IsGreaterThanOrEqual(edgeOffAB.GetZ(), voronoiTol * edgeOffAB.Get##ac_letter()) &			\
		IsGreaterThanOrEqual(edgeOffBA.Get##bc_letter(), voronoiTol * edgeOffBA.GetZ()) &			\
		IsGreaterThanOrEqual(edgeOffBA.GetZ(), voronoiTol * edgeOffBA.Get##bc_letter());			\
																									\
	inVoronoi = IsTrue(inVoronoiV);																	\
																									\
	edgeOffAB.Set##ad_letter(edgeOffAB.Get##ad_letter() - tA);										\
	edgeOffBA.Set##bd_letter(edgeOffBA.Get##bd_letter() - tB);										\
																									\
	tAresult = tA;																					\
	tBresult = tB;																					\
																									\
	return Dot(edgeOffAB,edgeOffAB);																\
}

ScalarV_Out
EdgeEdgeTest_0101(
	bool & inVoronoi,
	ScalarV_InOut tAresult,
	ScalarV_InOut tBresult,
	Vec3V_In hA,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_In signsA,
	Vec3V_In signsB,
	Vec3V_In scalesA,
	Vec3V_In scalesB )
{
	EdgeEdgeTest( 0, X, 1, Y, 0, X, 1, Y );
}

ScalarV_Out
EdgeEdgeTest_0110(
	bool & inVoronoi,
	ScalarV_InOut tAresult,
	ScalarV_InOut tBresult,
	Vec3V_In hA,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_In signsA,
	Vec3V_In signsB,
	Vec3V_In scalesA,
	Vec3V_In scalesB )
{
	EdgeEdgeTest( 0, X, 1, Y, 1, Y, 0, X );
}

ScalarV_Out
EdgeEdgeTest_1001(
	bool & inVoronoi,
	ScalarV_InOut tAresult,
	ScalarV_InOut tBresult,
	Vec3V_In hA,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_In signsA,
	Vec3V_In signsB,
	Vec3V_In scalesA,
	Vec3V_In scalesB )
{
	EdgeEdgeTest( 1, Y, 0, X, 0, X, 1, Y );
}

ScalarV_Out
EdgeEdgeTest_1010(
	bool & inVoronoi,
	ScalarV_InOut tAresult,
	ScalarV_InOut tBresult,
	Vec3V_In hA,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_In signsA,
	Vec3V_In signsB,
	Vec3V_In scalesA,
	Vec3V_In scalesB )
{
	EdgeEdgeTest( 1, Y, 0, X, 1, Y, 0, X );
}

#define EdgeEdge_SetNewMin( ac_letter, ad_letter, bc_letter, bd_letter )   \
{                                                                          \
   minDistSqr = distSqr;                                                   \
   localPointA.Set##ac_letter(scalesA.Get##ac_letter());                 \
   localPointA.Set##ad_letter(tA);                                        \
   localPointB.Set##bc_letter(scalesB.Get##bc_letter());                 \
   localPointB.Set##bd_letter(tB);                                        \
   otherFaceDimA = testOtherFaceDimA;                                      \
   otherFaceDimB = testOtherFaceDimB;                                      \
   featureA = E;                                                           \
   featureB = E;                                                           \
}

void
EdgeEdgeTests(
	bool & done,
	ScalarV_InOut minDistSqr,
	Vec3V_InOut localPointA,
	Vec3V_InOut localPointB,
	int & otherFaceDimA,
	int & otherFaceDimB,
	FeatureType & featureA,
	FeatureType & featureB,
	Vec3V_In hA,
	Vec3V_In hB,
	Vec3V_In faceOffsetAB,
	Vec3V_In faceOffsetBA,
	Mat33V_In matrixAB,
	Mat33V_In matrixBA,
	Vec3V_InOut signsAresult,
	Vec3V_InOut signsBresult,
	Vec3V_InOut scalesA,
	Vec3V_InOut scalesB,
	bool first )
{

	ScalarV distSqr;
	ScalarV tA, tB;

	Vec3V signsA = signsAresult;
	Vec3V signsB = signsBresult;

	int testOtherFaceDimA, testOtherFaceDimB;

	testOtherFaceDimA = 0;
	testOtherFaceDimB = 0;

	distSqr = EdgeEdgeTest_0101( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( first ) {
		EdgeEdge_SetNewMin( X, Y, X, Y );
	} else {
		if ( IsTrue(distSqr < minDistSqr) ) {
			EdgeEdge_SetNewMin( X, Y, X, Y );
		}
	}

	if ( done )
		goto AssignResults;

	signsA.SetX( -signsA.GetX() );
	scalesA.SetX( -scalesA.GetX() );

	distSqr = EdgeEdgeTest_0101( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, X, Y );
	}

	if ( done )
		goto AssignResults;

	signsB.SetX( -signsB.GetX() );
	scalesB.SetX( -scalesB.GetX() );

	distSqr = EdgeEdgeTest_0101( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, X, Y );
	}

	if ( done )
		goto AssignResults;

	signsA.SetX( -signsA.GetX() );
	scalesA.SetX( -scalesA.GetX() );

	distSqr = EdgeEdgeTest_0101( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, X, Y );
	}

	if ( done )
		goto AssignResults;

	testOtherFaceDimA = 1;
	testOtherFaceDimB = 0;
	signsB.SetX( -signsB.GetX() );
	scalesB.SetX( -scalesB.GetX() );

	distSqr = EdgeEdgeTest_1001( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, X, Y );
	}

	if ( done )
		goto AssignResults;

	signsA.SetY( -signsA.GetY() );
	scalesA.SetY( -scalesA.GetY() );

	distSqr = EdgeEdgeTest_1001( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, X, Y );
	}

	if ( done )
		goto AssignResults;

	signsB.SetX( -signsB.GetX() );
	scalesB.SetX( -scalesB.GetX() );

	distSqr = EdgeEdgeTest_1001( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, X, Y );
	}

	if ( done )
		goto AssignResults;

	signsA.SetY( -signsA.GetY() );
	scalesA.SetY( -scalesA.GetY() );

	distSqr = EdgeEdgeTest_1001( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, X, Y );
	}

	if ( done )
		goto AssignResults;

	testOtherFaceDimA = 0;
	testOtherFaceDimB = 1;
	signsB.SetX( -signsB.GetX() );
	scalesB.SetX( -scalesB.GetX() );

	distSqr = EdgeEdgeTest_0110( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, Y, X );
	}

	if ( done )
		goto AssignResults;

	signsA.SetX( -signsA.GetX() );
	scalesA.SetX( -scalesA.GetX() );

	distSqr = EdgeEdgeTest_0110( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, Y, X );
	}

	if ( done )
		goto AssignResults;

	signsB.SetY( -signsB.GetY() );
	scalesB.SetY( -scalesB.GetY() );

	distSqr = EdgeEdgeTest_0110( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, Y, X );
	}

	if ( done )
		goto AssignResults;

	signsA.SetX( -signsA.GetX() );
	scalesA.SetX( -scalesA.GetX() );

	distSqr = EdgeEdgeTest_0110( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( X, Y, Y, X );
	}

	if ( done )
		goto AssignResults;

	testOtherFaceDimA = 1;
	testOtherFaceDimB = 1;
	signsB.SetY( -signsB.GetY() );
	scalesB.SetY( -scalesB.GetY() );

	distSqr = EdgeEdgeTest_1010( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, Y, X );
	}

	if ( done )
		goto AssignResults;

	signsA.SetY( -signsA.GetY() );
	scalesA.SetY( -scalesA.GetY() );

	distSqr = EdgeEdgeTest_1010( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, Y, X );
	}

	if ( done )
		goto AssignResults;

	signsB.SetY( -signsB.GetY() );
	scalesB.SetY( -scalesB.GetY() );

	distSqr = EdgeEdgeTest_1010( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, Y, X );
	}

	if ( done )
		goto AssignResults;

	signsA.SetY( -signsA.GetY() );
	scalesA.SetY( -scalesA.GetY() );

	distSqr = EdgeEdgeTest_1010( done, tA, tB, hA, hB, faceOffsetAB, faceOffsetBA,
								 matrixAB, matrixBA, signsA, signsB, scalesA, scalesB );

	if ( IsTrue(distSqr < minDistSqr) ) {
		EdgeEdge_SetNewMin( Y, X, Y, X );
	}

AssignResults:
	signsAresult = signsA;
	signsBresult = signsB;
}


ScalarV_Out
boxBoxDistance(
	Vec3V_InOut normal,
	Vec3V_InOut boxPointA,
	Vec3V_InOut boxPointB,
	Vec3V_In boxA, Mat34V_In transformA,
	Vec3V_In boxB, Mat34V_In transformB,
	ScalarV_In distanceThreshold )
{
	Mat33V identity(V_IDENTITY);
	Vec3V ident[3];
	ident[0] = identity.GetCol0();
	ident[1] = identity.GetCol1();
	ident[2] = identity.GetCol2();

	// get relative transformations

	Mat34V transformAB, transformBA;
	Mat33V matrixAB, matrixBA;
	Vec3V offsetAB, offsetBA;

	InvertTransformOrtho(transformAB, transformA);
	Transform(transformAB, transformB);
	InvertTransformOrtho(transformBA, transformAB);

	matrixAB = transformAB.GetMat33();
	offsetAB = transformAB.GetCol3();
	matrixBA = transformBA.GetMat33();
	offsetBA = transformBA.GetCol3();

	Mat33V absMatrixAB;
	Abs(absMatrixAB, matrixAB);
	Mat33V absMatrixBA;
	Abs(absMatrixBA, matrixBA);

	// find separating axis with largest gap between projections

	BoxSepAxisType axisType;
	Vec3V axisA(V_ZERO), axisB(V_ZERO);
	ScalarV gap, maxGap;
	int faceDimA = 0, faceDimB = 0, edgeDimA = 0, edgeDimB = 0;

	// face axes

	Vec3V  gapsA   = Abs(offsetAB) - boxA - Multiply(absMatrixAB, boxB);

	AaxisTest(0,X,true);
	AaxisTest(1,Y,false);
	AaxisTest(2,Z,false);

	Vec3V  gapsB   = Abs(offsetBA) - boxB - Multiply(absMatrixBA, boxA);

	BaxisTest(0,X);
	BaxisTest(1,Y);
	BaxisTest(2,Z);

	// cross product axes

	// Prevent degeneracies
	Mat33V epsMtx(V_FLT_EPSILON);
	absMatrixAB += epsMtx;
	absMatrixBA += epsMtx;

	Mat33V lsqrs, projOffset, projAhalf, projBhalf;

	lsqrs.SetCol0( matrixBA.GetCol2() * matrixBA.GetCol2() +
				   matrixBA.GetCol1() * matrixBA.GetCol1() );
	lsqrs.SetCol1( matrixBA.GetCol2() * matrixBA.GetCol2() +
				   matrixBA.GetCol0() * matrixBA.GetCol0() );
	lsqrs.SetCol2( matrixBA.GetCol1() * matrixBA.GetCol1() +
				   matrixBA.GetCol0() * matrixBA.GetCol0() );

	projOffset.SetCol0(matrixBA.GetCol1() * offsetAB.GetZ() - matrixBA.GetCol2() * offsetAB.GetY());
	projOffset.SetCol1(matrixBA.GetCol2() * offsetAB.GetX() - matrixBA.GetCol0() * offsetAB.GetZ());
	projOffset.SetCol2(matrixBA.GetCol0() * offsetAB.GetY() - matrixBA.GetCol1() * offsetAB.GetX());

	projAhalf.SetCol0(absMatrixBA.GetCol1() * boxA.GetZ() + absMatrixBA.GetCol2() * boxA.GetY());
	projAhalf.SetCol1(absMatrixBA.GetCol2() * boxA.GetX() + absMatrixBA.GetCol0() * boxA.GetZ());
	projAhalf.SetCol2(absMatrixBA.GetCol0() * boxA.GetY() + absMatrixBA.GetCol1() * boxA.GetX());

	projBhalf.SetCol0(absMatrixAB.GetCol1() * boxB.GetZ() + absMatrixAB.GetCol2() * boxB.GetY());
	projBhalf.SetCol1(absMatrixAB.GetCol2() * boxB.GetX() + absMatrixAB.GetCol0() * boxB.GetZ());
	projBhalf.SetCol2(absMatrixAB.GetCol0() * boxB.GetY() + absMatrixAB.GetCol1() * boxB.GetX());

	Mat33V gapsAxB;
	Abs(gapsAxB, projOffset);
	Subtract(gapsAxB, gapsAxB, projAhalf);
	Mat33V projBhalfTranspose;
	Transpose(projBhalfTranspose, projBhalf);
	Subtract(gapsAxB, gapsAxB, projBhalfTranspose);

	static const ScalarV lsqr_tolerance = ScalarVFromF32(1.0e-30f);

	CrossAxisTest(0,0,X);
	CrossAxisTest(0,1,Y);
	CrossAxisTest(0,2,Z);
	CrossAxisTest(1,0,X);
	CrossAxisTest(1,1,Y);
	CrossAxisTest(1,2,Z);
	CrossAxisTest(2,0,X);
	CrossAxisTest(2,1,Y);
	CrossAxisTest(2,2,Z);

	// need to pick the face on each box whose normal best matches the separating axis.
	// will transform vectors to be in the coordinate system of this face to simplify things later.
	// for this, a permutation matrix can be used, which the next section computes.

	int dimA[3], dimB[3];

	if ( axisType == A_AXIS ) {
		axisA = SelectFT(IsLessThan(Dot(axisA,offsetAB), ScalarV(V_ZERO)), axisA, -axisA);

		axisB = Multiply(matrixBA, -axisA);

		Vec3V absAxisB = Abs(axisB);

		if ( IsTrue(IsGreaterThan(absAxisB.GetX(), absAxisB.GetY()) & IsGreaterThan(absAxisB.GetX(), absAxisB.GetZ())) )
			faceDimB = 0;
		else if ( IsGreaterThanAll(absAxisB.GetY(), absAxisB.GetZ()) )
			faceDimB = 1;
		else
			faceDimB = 2;
	} else if ( axisType == B_AXIS ) {
		axisB = SelectFT(IsLessThan(Dot(axisB,offsetBA), ScalarV(V_ZERO)), axisB, -axisB);

		axisA = Multiply(matrixAB, -axisB);

		Vec3V absAxisA = Abs(axisA);

		if ( IsTrue(IsGreaterThan(absAxisA.GetX(), absAxisA.GetY()) & IsGreaterThan(absAxisA.GetX(), absAxisA.GetZ())) )
			faceDimA = 0;
		else if ( IsGreaterThanAll(absAxisA.GetY(), absAxisA.GetZ()) )
			faceDimA = 1;
		else
			faceDimA = 2;
	}

	if ( axisType == CROSS_AXIS ) {
		axisA = SelectFT(IsLessThan(Dot(axisA,offsetAB), ScalarV(V_ZERO)), axisA, -axisA);

		axisB = Multiply(matrixBA, -axisA);

		Vec3V absAxisA = Abs(axisA);
		Vec3V absAxisB = Abs(axisB);

		dimA[1] = edgeDimA;
		dimB[1] = edgeDimB;

		if ( edgeDimA == 0 ) {
			if ( IsGreaterThanAll(absAxisA.GetY(), absAxisA.GetZ()) ) {
				dimA[0] = 2;
				dimA[2] = 1;
			} else                             {
				dimA[0] = 1;
				dimA[2] = 2;
			}
		} else if ( edgeDimA == 1 ) {
			if ( IsGreaterThanAll(absAxisA.GetZ(), absAxisA.GetX()) ) {
				dimA[0] = 0;
				dimA[2] = 2;
			} else                             {
				dimA[0] = 2;
				dimA[2] = 0;
			}
		} else {
			if ( IsGreaterThanAll(absAxisA.GetX(), absAxisA.GetY()) ) {
				dimA[0] = 1;
				dimA[2] = 0;
			} else                             {
				dimA[0] = 0;
				dimA[2] = 1;
			}
		}

		if ( edgeDimB == 0 ) {
			if ( IsGreaterThanAll(absAxisB.GetY(), absAxisB.GetZ()) ) {
				dimB[0] = 2;
				dimB[2] = 1;
			} else                             {
				dimB[0] = 1;
				dimB[2] = 2;
			}
		} else if ( edgeDimB == 1 ) {
			if ( IsGreaterThanAll(absAxisB.GetZ(), absAxisB.GetX()) ) {
				dimB[0] = 0;
				dimB[2] = 2;
			} else                             {
				dimB[0] = 2;
				dimB[2] = 0;
			}
		} else {
			if ( IsGreaterThanAll(absAxisB.GetX(), absAxisB.GetY()) ) {
				dimB[0] = 1;
				dimB[2] = 0;
			} else                             {
				dimB[0] = 0;
				dimB[2] = 1;
			}
		}
	} else {
		dimA[2] = faceDimA;
		dimA[0] = (faceDimA+1)%3;
		dimA[1] = (faceDimA+2)%3;
		dimB[2] = faceDimB;
		dimB[0] = (faceDimB+1)%3;
		dimB[1] = (faceDimB+2)%3;
	}

	Mat33V aperm_col, bperm_col;

	aperm_col.SetCol0(ident[dimA[0]]);
	aperm_col.SetCol1(ident[dimA[1]]);
	aperm_col.SetCol2(ident[dimA[2]]);

	bperm_col.SetCol0(ident[dimB[0]]);
	bperm_col.SetCol1(ident[dimB[1]]);
	bperm_col.SetCol2(ident[dimB[2]]);

	Mat33V aperm_row, bperm_row;

	Transpose(aperm_row, aperm_col);
	Transpose(bperm_row, bperm_col);

	// permute all box parameters to be in the face coordinate systems

	Mat33V matrixAB_perm;
	Multiply(matrixAB_perm, aperm_row, matrixAB);
	Multiply(matrixAB_perm, matrixAB_perm, bperm_col);
	Mat33V matrixBA_perm;
	Transpose(matrixBA_perm, matrixAB_perm);

	Vec3V offsetAB_perm, offsetBA_perm;

	offsetAB_perm = Multiply(aperm_row, offsetAB);
	offsetBA_perm = Multiply(bperm_row, offsetBA);

	Vec3V halfA_perm, halfB_perm;

	halfA_perm = Multiply(aperm_row, boxA);
	halfB_perm = Multiply(bperm_row, boxB);

	// compute the vector between the centers of each face, in each face's coordinate frame

	Vec3V signsA_perm, signsB_perm, scalesA_perm, scalesB_perm, faceOffsetAB_perm, faceOffsetBA_perm;

	VecBoolV signsA_positive = IsGreaterThan(Multiply(aperm_row, axisA), Vec3V(V_ZERO));
	signsA_perm = SelectFT(signsA_positive, Vec3V(V_NEGONE), Vec3V(V_ONE));
	VecBoolV signsB_positive = IsGreaterThan(Multiply(bperm_row, axisB), Vec3V(V_ZERO));
	signsB_perm = SelectFT(signsB_positive, Vec3V(V_NEGONE), Vec3V(V_ONE));
	scalesA_perm = signsA_perm * halfA_perm;
	scalesB_perm = signsB_perm * halfB_perm;

	faceOffsetAB_perm = offsetAB_perm + matrixAB_perm.GetCol2() * scalesB_perm.GetZ();
	faceOffsetAB_perm.SetZ( faceOffsetAB_perm.GetZ() - scalesA_perm.GetZ() );

	faceOffsetBA_perm = offsetBA_perm + matrixBA_perm.GetCol2() * scalesA_perm.GetZ();
	faceOffsetBA_perm.SetZ( faceOffsetBA_perm.GetZ() - scalesB_perm.GetZ() );

	// if boxes overlap, this will separate the faces for finding points of penetration.
	static const ScalarV ONE_POINT_OH_ONE = ScalarVFromF32(1.01f);
	BoolV maxGapLessThanZero = IsLessThan(maxGap, ScalarV(V_ZERO));
	faceOffsetAB_perm = SelectFT(maxGapLessThanZero, faceOffsetAB_perm, faceOffsetAB_perm - Multiply(aperm_row, axisA * maxGap * ONE_POINT_OH_ONE));
	faceOffsetBA_perm = SelectFT(maxGapLessThanZero, faceOffsetBA_perm, faceOffsetBA_perm - Multiply(bperm_row, axisB * maxGap * ONE_POINT_OH_ONE));

	// for each vertex/face or edge/edge pair of the two faces, find the closest points.
	//
	// these points each have an associated box feature (vertex, edge, or face).  if each
	// point is in the external Voronoi region of the other's feature, they are the
	// closest points of the boxes, and the algorithm can exit.
	//
	// the feature pairs are arranged so that in the general case, the first test will
	// succeed.  degenerate cases (parallel faces) may require up to all tests in the
	// worst case.
	//
	// if for some reason no case passes the Voronoi test, the features with the minimum
	// distance are returned.

	Vec3V localPointA_perm, localPointB_perm;
	ScalarV minDistSqr;
	bool done;

	Vec3V hA_perm( halfA_perm ), hB_perm( halfB_perm );

	localPointA_perm.SetZ( scalesA_perm.GetZ() );
	localPointB_perm.SetZ( scalesB_perm.GetZ() );
	scalesA_perm.SetZ(ScalarV(V_ZERO));
	scalesB_perm.SetZ(ScalarV(V_ZERO));

	int otherFaceDimA, otherFaceDimB;
	FeatureType featureA, featureB;

	if ( axisType == CROSS_AXIS ) {
		EdgeEdgeTests( done, minDistSqr, localPointA_perm, localPointB_perm,
					   otherFaceDimA, otherFaceDimB, featureA, featureB,
					   hA_perm, hB_perm, faceOffsetAB_perm, faceOffsetBA_perm,
					   matrixAB_perm, matrixBA_perm, signsA_perm, signsB_perm,
					   scalesA_perm, scalesB_perm, true );

		if ( !done ) {
			VertexBFaceATests( done, minDistSqr, localPointA_perm, localPointB_perm,
							   featureA, featureB,
							   hA_perm, faceOffsetAB_perm, faceOffsetBA_perm,
							   matrixAB_perm, matrixBA_perm, signsB_perm, scalesB_perm, false );

			if ( !done ) {
				VertexAFaceBTests( done, minDistSqr, localPointA_perm, localPointB_perm,
								   featureA, featureB,
								   hB_perm, faceOffsetAB_perm, faceOffsetBA_perm,
								   matrixAB_perm, matrixBA_perm, signsA_perm, scalesA_perm, false );
			}
		}
	} else if ( axisType == B_AXIS ) {
		VertexAFaceBTests( done, minDistSqr, localPointA_perm, localPointB_perm,
						   featureA, featureB,
						   hB_perm, faceOffsetAB_perm, faceOffsetBA_perm,
						   matrixAB_perm, matrixBA_perm, signsA_perm, scalesA_perm, true );

		if ( !done ) {
			VertexBFaceATests( done, minDistSqr, localPointA_perm, localPointB_perm,
							   featureA, featureB,
							   hA_perm, faceOffsetAB_perm, faceOffsetBA_perm,
							   matrixAB_perm, matrixBA_perm, signsB_perm, scalesB_perm, false );

			if ( !done ) {
				EdgeEdgeTests( done, minDistSqr, localPointA_perm, localPointB_perm,
							   otherFaceDimA, otherFaceDimB, featureA, featureB,
							   hA_perm, hB_perm, faceOffsetAB_perm, faceOffsetBA_perm,
							   matrixAB_perm, matrixBA_perm, signsA_perm, signsB_perm,
							   scalesA_perm, scalesB_perm, false );
			}
		}
	} else {
		VertexBFaceATests( done, minDistSqr, localPointA_perm, localPointB_perm,
						   featureA, featureB,
						   hA_perm, faceOffsetAB_perm, faceOffsetBA_perm,
						   matrixAB_perm, matrixBA_perm, signsB_perm, scalesB_perm, true );

		if ( !done ) {
			VertexAFaceBTests( done, minDistSqr, localPointA_perm, localPointB_perm,
							   featureA, featureB,
							   hB_perm, faceOffsetAB_perm, faceOffsetBA_perm,
							   matrixAB_perm, matrixBA_perm, signsA_perm, scalesA_perm, false );

			if ( !done ) {
				EdgeEdgeTests( done, minDistSqr, localPointA_perm, localPointB_perm,
							   otherFaceDimA, otherFaceDimB, featureA, featureB,
							   hA_perm, hB_perm, faceOffsetAB_perm, faceOffsetBA_perm,
							   matrixAB_perm, matrixBA_perm, signsA_perm, signsB_perm,
							   scalesA_perm, scalesB_perm, false );
			}
		}
	}

	// convert local points from face-local to box-local coordinate system

	boxPointA = Multiply(aperm_col, localPointA_perm);
	boxPointB = Multiply(bperm_col, localPointB_perm);

#if 0
	// find which features of the boxes are involved.
	// the only feature pairs which occur in this function are VF, FV, and EE, even though the
	// closest points might actually lie on sub-features, as in a VF contact might be used for
	// what's actually a VV contact.  this means some feature pairs could possibly seem distinct
	// from others, although their contact positions are the same.  don't know yet whether this
	// matters.

	int sA[3], sB[3];

	sA[0] = boxPointA.localPoint.getX() > 0.0f;
	sA[1] = boxPointA.localPoint.getY() > 0.0f;
	sA[2] = boxPointA.localPoint.getZ() > 0.0f;

	sB[0] = boxPointB.localPoint.getX() > 0.0f;
	sB[1] = boxPointB.localPoint.getY() > 0.0f;
	sB[2] = boxPointB.localPoint.getZ() > 0.0f;

	if ( featureA == F ) {
		boxPointA.setFaceFeature( dimA[2], sA[dimA[2]] );
	} else if ( featureA == E ) {
		boxPointA.setEdgeFeature( dimA[2], sA[dimA[2]], dimA[otherFaceDimA], sA[dimA[otherFaceDimA]] );
	} else {
		boxPointA.setVertexFeature( sA[0], sA[1], sA[2] );
	}

	if ( featureB == F ) {
		boxPointB.setFaceFeature( dimB[2], sB[dimB[2]] );
	} else if ( featureB == E ) {
		boxPointB.setEdgeFeature( dimB[2], sB[dimB[2]], dimB[otherFaceDimB], sB[dimB[otherFaceDimB]] );
	} else {
		boxPointB.setVertexFeature( sB[0], sB[1], sB[2] );
	}
#endif

	normal = Transform3x3(transformA, axisA);

	if ( IsLessThanAll(maxGap, ScalarV(V_ZERO)) ) {
		return (maxGap);
	} else {
		return (Sqrt( minDistSqr ));
	}
}

}  // namespace rage

#endif // ENABLE_UNUSED_PHYSICS_CODE

