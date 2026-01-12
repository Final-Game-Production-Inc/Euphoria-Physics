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

#include "TrianglePenetrationDepthSolver.h"

#include "CollisionMargin.h"
#include "GjkPairDetector.h"
#include "MinkowskiSumShape.h"
#include "PointCollector.h"
#include "TriangleShape.h"
#include "VoronoiSimplexSolver.h"

#include "phbound/support.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

using namespace rage;

#if VERIFY_SUPPORT_FUNCTIONS
extern void VerifyBoundSupportFunction(const phBound * bound, Vec3V_In dir);
#endif // VERIFY_SUPPORT_FUNCTIONS

void TrianglePenetrationDepthSolver::ComputePenetrationSeparation(
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	const phBound* convexA, const phBound* convexB, Mat34V_In transA, Mat34V_In transB,
																Vec3V_InOut sepAxis, ScalarV_InOut sepDist)
{
	//just take fixed number of orientation, and sample the penetration depth in that direction
	//Vec3V seperatingAxisInA, seperatingAxisInB;
	//Vec3V pInA, qInB, w;

	// Save locals.
	const Mat34V v_transA = transA;
	const Mat34V v_transB = transB;

	Assert(convexB->GetType() == rage::phBound::TRIANGLE);
	const rage::TriangleShape *tri = static_cast<const rage::TriangleShape *>(convexB);
	const Mat34V triangleTransform = v_transB;

// NEW_MIDPHASE
//	const Mat34V otherTransform = triangleBoundIndex != 0 ? v_transA : v_transB;

/*	
    transA.UnTransform3x3(-norm, seperatingAxisInA);
    transB.UnTransform3x3( norm, seperatingAxisInB);
	pInA = convexA->LocalGetSupportingVertexWithoutMargin(seperatingAxisInA);
	qInB = convexB->LocalGetSupportingVertexWithoutMargin(seperatingAxisInB);
	transA.Transform(pInA, minA);	
	transB.Transform(qInB, minB);
	w	= minB - minA;
	minProj = norm.Dot(w);
*/	

	const int numTestDirections = 10;
	Vec3V testDirections[numTestDirections];
	Vec3V v_normal = tri->m_PolygonNormal;
	testDirections[0] = v_normal;
#if 1 // If you toggle the old you'll have to add testDirections[7]-[9]
	testDirections[1] = tri->m_EdgeNormals[0];
	testDirections[2] = tri->m_EdgeNormals[1];
	testDirections[3] = tri->m_EdgeNormals[2];

#if __XENON
	// Pre-calculate the transpose for Xenon.
	Mat34V v_triangleTransformTransposed;
	Transpose3x3( v_triangleTransformTransposed, triangleTransform );
	testDirections[0] = UnTransform3x3Ortho( v_triangleTransformTransposed, testDirections[0] );
	testDirections[1] = UnTransform3x3Ortho( v_triangleTransformTransposed, testDirections[1] );
	testDirections[2] = UnTransform3x3Ortho( v_triangleTransformTransposed, testDirections[2] );
	testDirections[3] = UnTransform3x3Ortho( v_triangleTransformTransposed, testDirections[3] );
#else
	// Else, nothing to pre-calculate that will help the multiple transforms by the same matrix.
	testDirections[0] = Transform3x3( triangleTransform, testDirections[0] );
	testDirections[1] = Transform3x3( triangleTransform, testDirections[1] );
	testDirections[2] = Transform3x3( triangleTransform, testDirections[2] );
	testDirections[3] = Transform3x3( triangleTransform, testDirections[3] );
#endif
	testDirections[4] = Add(testDirections[1], testDirections[2]);
	testDirections[4] = Normalize(testDirections[4]);
	testDirections[5] = Add(testDirections[1], testDirections[3]);
	testDirections[5] = Normalize(testDirections[5]);
	testDirections[6] = Add(testDirections[2], testDirections[3]);
	testDirections[6] = Normalize(testDirections[6]);

	// Our binary vertex normals
	Vec3V v0 = tri->GetVertex(0);
	Vec3V v1 = tri->GetVertex(1);
	Vec3V v2 = tri->GetVertex(2);
	Vec3V triCenter = Scale(Add(Add(v0, v1), v2), ScalarV(V_THIRD));
	testDirections[7] = SelectFT( IsU32NonZero(tri->m_VertexNormalCodes[0]), v_normal, Normalize(Subtract(v0, triCenter)) );
	testDirections[8] = SelectFT( IsU32NonZero(tri->m_VertexNormalCodes[1]), v_normal, Normalize(Subtract(v1, triCenter)) );
	testDirections[9] = SelectFT( IsU32NonZero(tri->m_VertexNormalCodes[2]), v_normal, Normalize(Subtract(v2, triCenter)) );
#else
	Vec3V v0 = VECTOR3_TO_VEC3V(tri->GetVertex( 0 ));
	Vec3V v1 = VECTOR3_TO_VEC3V(tri->GetVertex( 1 ));
	Vec3V v2 = VECTOR3_TO_VEC3V(tri->GetVertex( 2 ));

	Vec3V centre = Scale(v0 + v1 + v2, Vec3V(V_THIRD));

	Vec3V e0, e1, e2;
	e0 = Subtract( v1, v0 );
	e1 = Subtract( v2, v1 );
	e2 = Subtract( v0, v2 );

	testDirections[1] = Subtract( v0, centre );
	testDirections[2] = Subtract( v1, centre );
	testDirections[3] = Subtract( v2, centre );
	testDirections[4] = Cross( e0, v_normal );
	testDirections[5] = Cross( e1, v_normal );
	testDirections[6] = Cross( e2, v_normal );
	testDirections[1] = Normalize(testDirections[1]);
	testDirections[2] = Normalize(testDirections[2]);
	testDirections[3] = Normalize(testDirections[3]);
	testDirections[4] = Normalize(testDirections[4]);
	testDirections[5] = Normalize(testDirections[5]);
	testDirections[6] = Normalize(testDirections[6]);
#endif

	// NEW_MIDPHASE
	// Quick little hack - if the second bound is also a triangle consider its face normal as possible separating axis as well.
	// This is only needed for the new midphase.
//	if(bounds[1 - triangleBoundIndex]->GetType() == phBound::TRIANGLE)
//	{
//		const TriangleShape *otherTriangle = static_cast<const TriangleShape *>(bounds[1 - triangleBoundIndex]);
//		// The Negate() is here because the test directions are the directions in which we will consider moving the "other" triangle bound, not this one.
//		testDirections[6] = Negate(Transform3x3(otherTransform, otherTriangle->m_PolygonNormal));
//	}

// 	Displayf("testDirection %d: <%f, %f, %f>", 0, testDirections[0].x, testDirections[0].y, testDirections[0].z);
// 	Displayf("testDirection %d: <%f, %f, %f>", 1, testDirections[1].x, testDirections[1].y, testDirections[1].z);
// 	Displayf("testDirection %d: <%f, %f, %f>", 2, testDirections[2].x, testDirections[2].y, testDirections[2].z);
// 	Displayf("testDirection %d: <%f, %f, %f>", 3, testDirections[3].x, testDirections[3].y, testDirections[3].z);
// 	Displayf("testDirection %d: <%f, %f, %f>", 4, testDirections[4].x, testDirections[4].y, testDirections[4].z);
// 	Displayf("testDirection %d: <%f, %f, %f>", 5, testDirections[5].x, testDirections[5].y, testDirections[5].z);
// 	Displayf("testDirection %d: <%f, %f, %f>", 6, testDirections[6].x, testDirections[6].y, testDirections[6].z);

	ScalarV minProj(V_FLT_MAX);
	Vec3V norm(V_ZERO);//, minA, minB;		// Initializing norm to zero to avoid compiler warning that it may be uninitialized after the loop
												//   below, that couldn't really happen though

	const Vec3V separationOffset = v_transA.GetCol3() - v_transB.GetCol3();
	for (int i=0; i <numTestDirections; i++)
	{
		//Vec3V pWorld, qWorld; 
		const Vec3V triNorm = testDirections[i];
		const Vec3V seperatingAxisInA = UnTransform3x3Ortho(v_transA, -triNorm);
		const Vec3V seperatingAxisInB = UnTransform3x3Ortho(v_transB, triNorm);

#if VERIFY_SUPPORT_FUNCTIONS
		VerifyBoundSupportFunction(convexA,seperatingAxisInA);
		VerifyBoundSupportFunction(convexB,seperatingAxisInB);
#endif // VERIFY_SUPPORT_FUNCTIONS

		const Vec3V pInA = convexA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
		const Vec3V qInB = convexB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
		const Vec3V pWorld = Transform3x3( v_transA, pInA );
		const Vec3V qWorld = Transform3x3( v_transB, qInB );
		const Vec3V w = (qWorld - pWorld) - separationOffset;
		const ScalarV delta = Dot(triNorm, w);
		//find smallest delta
		if( IsLessThanAll(delta, minProj) != 0 )
		{
			minProj = delta;
			norm = triNorm;
			//minA = pWorld;
			//minB = qWorld;
		}
	}

	//Assert(minProj.IsGreaterOrEqualThan(VEC3_ZERO));

	//add the margins
	ScalarV v3minProj = minProj;
	v3minProj += PENETRATION_CHECK_EXTRA_MARGIN_SV;

	//////////////////////////////////////////////////////////////////////////
	// Return direction and displacement values
	sepAxis = norm;
	sepDist = v3minProj;

#else // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER

const phBound*, const phBound*, Mat34V_In, Mat34V_In, Vec3V_InOut, ScalarV_InOut)
{
	Quitf("TriPenDepthSolver being used with COLLISION_MAY_USE_TRIANGLE_PD_SOLVER off!");
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
}
