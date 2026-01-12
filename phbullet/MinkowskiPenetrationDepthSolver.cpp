/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "MinkowskiPenetrationDepthSolver.h"

#include "CollisionMargin.h"
#include "GjkPairDetector.h"
#include "MinkowskiSumShape.h"
#include "PointCollector.h"
#include "VoronoiSimplexSolver.h"

#include "phbound/support.h"
#include "vectormath/classes.h"

using namespace rage;

#define NUM_UNITSPHERE_POINTS 26
#define Vec3V_FA(x,y,z) {x,y,z,0.0f}
ALIGNAS(16) const float sPenetrationDirections[NUM_UNITSPHERE_POINTS][4]  = 
{
	Vec3V_FA( 0.000000f,  0.000000f,  1.000000f),
	Vec3V_FA( 0.000000f,  0.000000f, -1.000000f),
	Vec3V_FA( 0.000000f,  1.000000f,  0.000000f),
	Vec3V_FA( 0.000000f, -1.000000f,  0.000000f),
	Vec3V_FA( 1.000000f,  0.000000f,  0.000000f),
	Vec3V_FA(-1.000000f,  0.000000f,  0.000000f),

	Vec3V_FA( 0.707106f,  0.707106f,  0.000000f),
	Vec3V_FA( 0.707106f, -0.707106f,  0.000000f),
	Vec3V_FA(-0.707106f,  0.707106f,  0.000000f),
	Vec3V_FA(-0.707106f, -0.707106f,  0.000000f),
	Vec3V_FA( 0.000000f,  0.707106f,  0.707106f),
	Vec3V_FA( 0.000000f,  0.707106f, -0.707106f),
	Vec3V_FA( 0.000000f, -0.707106f,  0.707106f),
	Vec3V_FA( 0.000000f, -0.707106f, -0.707106f),
	Vec3V_FA( 0.707106f,  0.000000f,  0.707106f),
	Vec3V_FA( 0.707106f,  0.000000f, -0.707106f),
	Vec3V_FA(-0.707106f,  0.000000f,  0.707106f),
	Vec3V_FA(-0.707106f,  0.000000f, -0.707106f),

	Vec3V_FA( 0.577350f,  0.577350f,  0.577350f),
	Vec3V_FA( 0.577350f,  0.577350f, -0.577350f),
	Vec3V_FA( 0.577350f, -0.577350f,  0.577350f),
	Vec3V_FA( 0.577350f, -0.577350f, -0.577350f),
	Vec3V_FA(-0.577350f,  0.577350f,  0.577350f),
	Vec3V_FA(-0.577350f,  0.577350f, -0.577350f),
	Vec3V_FA(-0.577350f, -0.577350f,  0.577350f),
	Vec3V_FA(-0.577350f, -0.577350f, -0.577350f)
};

#if VERIFY_SUPPORT_FUNCTIONS
void VerifyBoundGeometrySupportFunction(const phBoundGeometry * boundGeometry, Vec3V_In dir)
{
	const ScalarV ndir = Mag(dir);
	FastAssert(IsGreaterThanAll(ndir,ScalarV(V_ZERO)));
	const Vec3V unitDir = dir / ndir;
	const bool UseOctantMap_save = phBoundGeometry::GetUseOctantMapRef();
	phBoundGeometry::GetUseOctantMapRef() = false;
	const Vec3V p0 = boundGeometry->LocalGetSupportingVertexWithoutMarginNotInlined(unitDir.GetIntrin128ConstRef());
	phBoundGeometry::GetUseOctantMapRef() = true;
	const Vec3V p1 = boundGeometry->LocalGetSupportingVertexWithoutMarginNotInlined(unitDir.GetIntrin128ConstRef());
	phBoundGeometry::GetUseOctantMapRef() = UseOctantMap_save;
	const ScalarV dist = Dot(p1-p0,unitDir);
	const ScalarV dist0 = Dot(p0,unitDir);
	const ScalarV dist1 = Dot(p1,unitDir);
	static float eps = 0.0001f;
	if (IsGreaterThanAll(Abs(dist),ScalarVFromF32(eps)))
	{
		Displayf("phBoundGeometry 0x%p has bad octant map: %f %f %f",boundGeometry,dist.Getf(),dist0.Getf(),dist1.Getf());
	}
}

void VerifyBoundSupportFunction(const phBound * bound, Vec3V_In dir)
{
	if (bound->GetType() == phBound::GEOMETRY)
	{
		const phBoundGeometry * boundGeometry = reinterpret_cast<const phBoundGeometry *>(bound);
		VerifyBoundGeometrySupportFunction(boundGeometry,dir);
	}
}
#endif // VERIFY_SUPPORT_FUNCTIONS

//////////////////////////////////////////////////////////////////////////
void MinkowskiPenetrationDepthSolver::ComputePenetrationSeparation (const phBound* convexA, const phBound* convexB, Mat34V_In transAIn, Mat34V_In transBIn,
															   Vec3V_InOut sepAxis, ScalarV_InOut sepDist)
{
	//just take fixed number of orientation, and sample the penetration depth in that direction
	ScalarV minProj(V_FLT_MAX);
	Vec3V minNorm = Vec3V(V_ZERO);
	const Mat34V transA(transAIn);
	const Mat34V transB(transBIn);
	const Vec3V separationOffset = transA.GetCol3() - transB.GetCol3();

	// Triangles always try their face normal
	if(convexB->GetType() == rage::phBound::TRIANGLE)
	{
		const rage::TriangleShape* tri = static_cast<const rage::TriangleShape*>(convexB);
		Vec3V testDirection = tri->m_PolygonNormal;

		for(int i = 0; i < 2; i++)
		{
			const Vec3V norm = Transform3x3(transB, testDirection);
			const Vec3V seperatingAxisInA = UnTransform3x3Ortho( transA, -norm );
			const Vec3V seperatingAxisInB = testDirection;

#if VERIFY_SUPPORT_FUNCTIONS
			VerifyBoundSupportFunction(convexA,seperatingAxisInA);
			VerifyBoundSupportFunction(convexB,seperatingAxisInB);
#endif // VERIFY_SUPPORT_FUNCTIONS

			const Vec3V pInA = convexA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
			const Vec3V qInB = convexB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
			const Vec3V pWorld = Transform3x3(transA, pInA);
			const Vec3V qWorld = Transform3x3(transB, qInB);

			const Vec3V w = (qWorld - pWorld) - separationOffset;
			const ScalarV delta = Dot(norm, w);

			//find smallest delta
			const BoolV isLessThan = IsLessThan(delta, minProj);
			minProj = SelectFT(isLessThan, minProj, delta);
			minNorm = SelectFT(isLessThan, minNorm, norm);

			// And then try the reverse direction
			testDirection = Scale(testDirection, ScalarV(V_NEGONE));
		}
	}

	for (int i = 0; i < NUM_UNITSPHERE_POINTS; i++)
	{
		const Vec3V testDirection = ((const Vec3V *)sPenetrationDirections)[i];
		const Vec3V seperatingAxisInA = -testDirection;
		const Vec3V norm = Transform3x3(transA, testDirection);
		const Vec3V seperatingAxisInB = UnTransform3x3Ortho( transB, norm );

#if VERIFY_SUPPORT_FUNCTIONS
		VerifyBoundSupportFunction(convexA,seperatingAxisInA);
		VerifyBoundSupportFunction(convexB,seperatingAxisInB);
#endif // VERIFY_SUPPORT_FUNCTIONS

		const Vec3V pInA = convexA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
		const Vec3V qInB = convexB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
		const Vec3V pWorld = Transform3x3(transA, pInA);
		const Vec3V qWorld = Transform3x3(transB, qInB);

		const Vec3V w = (qWorld - pWorld) - separationOffset;
		const ScalarV delta = Dot(norm, w);

		//find smallest delta
		const BoolV isLessThan = IsLessThan(delta, minProj);
		minProj = SelectFT(isLessThan, minProj, delta);
		minNorm = SelectFT(isLessThan, minNorm, norm);
	}

	for (int i = 0; i < NUM_UNITSPHERE_POINTS; i++)
	{
		const Vec3V testDirection = ((const Vec3V *)sPenetrationDirections)[i];
		const Vec3V norm = Transform3x3(transB, testDirection);
		const Vec3V seperatingAxisInA = UnTransform3x3Ortho( transA, -norm );
		const Vec3V seperatingAxisInB = testDirection;

#if VERIFY_SUPPORT_FUNCTIONS
		VerifyBoundSupportFunction(convexA,seperatingAxisInA);
		VerifyBoundSupportFunction(convexB,seperatingAxisInB);
#endif // VERIFY_SUPPORT_FUNCTIONS

		const Vec3V pInA = convexA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
		const Vec3V qInB = convexB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
		const Vec3V pWorld = Transform3x3(transA, pInA);
		const Vec3V qWorld = Transform3x3(transB, qInB);

		const Vec3V w = (qWorld - pWorld) - separationOffset;
		const ScalarV delta = Dot(norm, w);

		//find smallest delta
		const BoolV isLessThan = IsLessThan(delta, minProj);
		minProj = SelectFT(isLessThan, minProj, delta);
		minNorm = SelectFT(isLessThan, minNorm, norm);
	}

	minProj += PENETRATION_CHECK_EXTRA_MARGIN_SV;
	minProj = Max(minProj, ScalarV(V_ZERO));

	//////////////////////////////////////////////////////////////////////////
	// Return direction and displacement values
	sepAxis = minNorm;
	sepDist = minProj;
}
