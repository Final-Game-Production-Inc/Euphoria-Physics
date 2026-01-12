//
// vector/testgeometry.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "geometry.h"
#include "math/random.h"
#include "system/main.h"

using namespace rage;


int Main()
{
	//////////////////////////////////
	// test geomPoints::IsPointInWedge
	mthRandom rand;
	const int numTests = 10000;
	for (int index=0; index<numTests; index++)
	{
		// Make two wedge sides and a point.
		Vector3 wedgeSide1(rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f));
		wedgeSide1.NormalizeSafe();
		Vector3 wedgeSide2(rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f));
		wedgeSide2.NormalizeSafe();
		const float max = 10.0f;
		Vector3 testPoint(rand.GetRanged(-max,max),rand.GetRanged(-max,max),rand.GetRanged(-max,max));

		// Determine whether or not to use the negated point.
		const bool useAntiWedge = (rand.GetRanged(-1.0f,1.0f)>0.0f);

		// Make a random coordinate matrix.
		Matrix34 transformMtx;
		transformMtx.a.Set(rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f));
		transformMtx.a.NormalizeSafe();
		transformMtx.b.Set(rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f),rand.GetRanged(-1.0f,1.0f));
		if (transformMtx.b.Mag2()<SMALL_FLOAT)
		{
			transformMtx.b.Set(transformMtx.a.y,transformMtx.a.z,transformMtx.a.x);
		}
		transformMtx.c.Cross(transformMtx.a,transformMtx.b);
		transformMtx.c.Normalize();
		transformMtx.b.Cross(transformMtx.c,transformMtx.a);
		transformMtx.d.Zero();

		// Make a pointer to the coordinate matrix, or a NULL pointer.
		Matrix34* mtxPtr = rand.GetRanged(-1.0f,1.0f)>0.0f ? &transformMtx : NULL;

		// See if the point is in the wedge.
		const float sineCushion2 = 0.0f;
		ASSERT_ONLY(bool inWedge =) geomPoints::IsPointInWedge(testPoint,wedgeSide1,wedgeSide2,mtxPtr,useAntiWedge,sineCushion2);

		if (useAntiWedge)
		{
			// The anti-wedge was used, so negate the test point for the accuracy tests.
			testPoint.Negate();
		}

		if (mtxPtr)
		{
			// The transformation matrix was used, so put the test point in the same coordinates as the wedge.
			mtxPtr->UnTransform3x3(testPoint);
		}

		// Make sure the point is not completely on the wrong side of the wedge.
		Vector3 wedgeCenter(wedgeSide1);
		wedgeCenter.Add(wedgeSide2);
		float pointDotCenter = testPoint.Dot(wedgeCenter);
		if (pointDotCenter>SMALL_FLOAT)
		{
			// Make sure that the projection of the test point onto the perpendicular plane of the wedge has
			// opposite dot products with the sides if its in the wedge, and not opposite if it's outside the wedge.
			Vector3 planeNormal(wedgeSide1);
			planeNormal.Cross(wedgeSide2);
			float planeNormalMag2 = planeNormal.Mag2();
			if (planeNormalMag2>SMALL_FLOAT)
			{
				planeNormal.Scale(invsqrtf(planeNormalMag2));
				Vector3 planeTestPoint(testPoint);
				planeTestPoint.SubtractScaled(planeNormal,planeNormal.Dot(testPoint));
				Vector3 pointCrossSide1(planeTestPoint);
				pointCrossSide1.Cross(wedgeSide1);
				Vector3 pointCrossSide2(planeTestPoint);
				pointCrossSide2.Cross(wedgeSide2);
				ASSERT_ONLY(float dot =) pointCrossSide1.Dot(pointCrossSide2);
				mthAssertf((inWedge && dot<SMALL_FLOAT) || (!inWedge && dot>-SMALL_FLOAT), "inWedge isn't consistent");
			}
		}
		else
		{
			mthAssertf(pointDotCenter<-SMALL_FLOAT, "point * center is near zero");
		}
	}
    
	return 1;
}
