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

#include "GjkPairDetector.h"
#include "ConvexPenetrationDepthSolver.h"

#include "phbound/bound.h"
#include "phbound/support.h"
#include "physics/simulator.h"
#include "grprofile/drawmanager.h"
#include "profile/element.h"
#include "vectormath/classes.h"
#include "GjkSimplexCache.h"
#include "physics/physicsprofilecapture.h"

GJK_COLLISION_OPTIMIZE_OFF()

#if __SPU
extern ScalarV g_allowedPenetration;
#endif

namespace rage {

namespace phCollisionStats
{
    EXT_PF_TIMER(SupportFunction);
	EXT_PF_TIMER(NP_GJK_Time);
	EXT_PF_COUNTER(NP_GJK_Calls);
	EXT_PF_COUNTER(NP_GJK_Iters);
	EXT_PF_TIMER(NP_GJK_CacheSetup);
	EXT_PF_TIMER(NP_GJK_IterTime);
}

using namespace phCollisionStats;

const float rel_error = 1.0e-3f;
const float rel_error2 = rel_error * rel_error;
//float maxdist2 = 1.0e30f;

#if __BANK
extern int g_NumDeepPenetrationChecks;
#endif // __BANK

Vec3V_Out GjkPairDetector::CalcInitialSeparatingAxis (const Matrix34& currentA, const phBound* boundA, const Matrix34& currentB, const phBound* boundB)
{
	Vec3V upVect = VECTOR3_TO_VEC3V(g_UnitUp);

	// Default the normalized separating axis to the world up vector.
	Vec3V cachedSeparatingAxis = upVect;

	if (boundA && !phBound::IsTypeConcave(boundA->GetType()) && boundB && !phBound::IsTypeConcave(boundB->GetType()))
	{
		// Make the separating axis go from the centroid of object 1 to the centroid of object 0 (the direction is arbitrary).
		Vec3V worldCentroid0 = boundA->GetWorldCentroid(RCC_MAT34V(currentA));
		Vec3V worldCentroid1 = boundB->GetWorldCentroid(RCC_MAT34V(currentB));
		cachedSeparatingAxis = worldCentroid0 - worldCentroid1;
		cachedSeparatingAxis = NormalizeSafe(cachedSeparatingAxis, upVect);
	}

	// Make the length of the separating axis the sum of the collision margins minus the allowed penetration, with a minimum length of 1cm.
	float margin0 = (boundA ? boundA->GetMargin() : 0.5f);
	float margin1 = (boundB ? boundB->GetMargin() : 0.5f);
	float separatingAxisLength = Max(margin0+margin1-phSimulator::GetAllowedPenetration(),0.01f);
	cachedSeparatingAxis = Scale( cachedSeparatingAxis, Vec3VFromF32(separatingAxisLength) );

	return cachedSeparatingAxis;
}

#define GJK_PF_FUNC1(x) //PF_FUNC(x)
#define GJK_PF_START1(x) //PF_START(x)
#define GJK_PF_STOP1(x) //PF_STOP(x)
#define GJK_PF_INCREMENT1(x) //PF_INCREMENT(x)

__forceinline void ConvertIndex(const int boundType, Vec::Vector_4V_In indexV, int & index)
{
	if (boundType == phBound::BOX)
	{
		Vec::V3ResultToIndexZYX(index,indexV);
	}
	else
	{
		Vec::V4StoreScalar32FromSplatted(index,indexV);
	}
}

#define REL_ERROR_SQUARED_V ScalarVFromF32(rel_error2)
#define AXIS_DEGENERATE_LIMIT ScalarVFromF32(0.0001f)

__forceinline ScalarV GetMinSeparationSq(const Vec3V verts[4], Vec3V_In supportPoint, const int vertCount)
{
	FastAssert(vertCount >= 0 && vertCount <= 3);
	ScalarV minSeparationSq = ScalarV(V_FLT_MAX);
	switch(vertCount)
	{
	case 3:
		minSeparationSq = Min(minSeparationSq,MagSquared(supportPoint-verts[2]));
	case 2:
		minSeparationSq = Min(minSeparationSq,MagSquared(supportPoint-verts[1]));
	case 1:
		minSeparationSq = Min(minSeparationSq,MagSquared(supportPoint-verts[0]));
	}
	return minSeparationSq;
}

__forceinline ScalarV_Out CalcSimplexTooCloseEpsilon(const Vec3V verts[4], Vec3V_In vEPSILON, const int vertCount)
{
	FastAssert(vertCount >= 1 && vertCount <= 3);
	Vec3V sum = Abs(verts[0]);
	switch(vertCount)
	{
	case 3:
		sum += Abs(verts[2]);
	case 2:
		sum += Abs(verts[1]);
	}
	const ScalarV simplexTooCloseEpsilon = Dot(sum,vEPSILON) + SimplexTooCloseFixedEpsilon;
	return simplexTooCloseEpsilon;
}

void GjkPairDetector::DoGjkIterations(const ClosestPointInput & input, const bool isSeparated, const bool isFirstIteration, ClosestPointOutput * CCD_RESTRICT output)
{
	//PPC_STAT_TIMER_SCOPED(GJKTimer);
	//PPC_STAT_COUNTER_INC(GJKCounter,1);

#if USE_NEW_SIMPLEX_SOLVER
	FastAssert(IsGreaterThanAll(input.m_sepThreshold,SimplexTooCloseFixedEpsilon) != 0);
	const ScalarV v_flt_small_4(V_FLT_SMALL_4);
	const ScalarV v_flt_small_5(V_FLT_SMALL_5);
#endif // USE_NEW_SIMPLEX_SOLVER

#if !USE_NEW_SIMPLEX_SOLVER
	const ScalarV vEPSILON = ScalarV(V_FLT_EPSILON);
#endif // #if !USE_NEW_SIMPLEX_SOLVER

	GJK_PF_FUNC1(NP_GJK_Time);
	GJK_PF_INCREMENT1(NP_GJK_Calls);

	const phBound * CCD_RESTRICT m_minkowskiA = input.m_boundA;
	const phBound * CCD_RESTRICT m_minkowskiB = input.m_boundB;

	const ScalarV v_zero(V_ZERO);
	const ScalarV v_one(V_ONE);
	const ScalarV v_fltmax(V_FLT_MAX);

	ScalarV squaredDistance = v_fltmax;
	bool checkSimplex = false;

#if USE_NEW_SIMPLEX_SOLVER
	bool needsPenetrationSolve = !isSeparated;
#else // USE_NEW_SIMPLEX_SOLVER
	(void)isSeparated;
	bool degenerateSimplex = false;
#endif // USE_NEW_SIMPLEX_SOLVER
	
	int curIter = 0;
	const int MAX_ITER = 1000;

	ScalarV cachedSeparatingAxisLengthSquared;
#define STORE_CACHED_AXIS_LENGTH 0
#if STORE_CACHED_AXIS_LENGTH
	#define CACHED_AXIS_LENGTH_ONLY(x) x
#else
	#define CACHED_AXIS_LENGTH_ONLY(x)
#endif
	CACHED_AXIS_LENGTH_ONLY(ScalarV cachedSeparatingAxisLength);

	const Mat34V transA = input.m_transformA;
	const Mat34V transB = input.m_transformB;
	const Vec3V separationOffset = transA.GetCol3() - transB.GetCol3(); 

	// Reset the simplex solver.
	GJK_PF_START1(NP_GJK_CacheSetup);
#if USE_GJK_CACHE
	if (m_simplexSolver.m_hasCache)
	{
#if __BANK
		FastAssert(g_UseGJKCache == true);
#endif
		FastAssert(m_simplexSolver.m_numVertices >= 0 && m_simplexSolver.m_numVertices <= 3);
		m_simplexSolver.ResetCache(m_simplexSolver.m_numVertices);
		if (m_simplexSolver.IsSimplexValid())
		{
			GJK_PF_INCREMENT1(NP_GJK_Iters);
			//PPC_STAT_COUNTER_INC(GJKIterationCounter,1);
			//PPC_STAT_COUNTER_INC(GJKSeedSimplexCounter,1);

			#undef LOOP_
			#define LOOP_(vert_i) \
			{ \
				const Vec3V worldPointOnA_ = Transform3x3( transA, m_simplexSolver.m_LocalA[vert_i] ); \
				const Vec3V worldPointOnB_ = Transform3x3( transB, m_simplexSolver.m_LocalB[vert_i] ); \
				m_simplexSolver.m_SimplexSeparation[vert_i] = (worldPointOnA_ - worldPointOnB_) + separationOffset; \
			}

#if USE_FULL_SIMPLEX_COPY
			LOOP_(0);
			LOOP_(1);
			LOOP_(2);
#else // USE_FULL_SIMPLEX_COPY
			switch(m_simplexSolver.m_numVertices)
			{
			case 3:
				LOOP_(2);
			case 2:
				LOOP_(1);
			case 1:
				LOOP_(0);
			}
#endif // USE_FULL_SIMPLEX_COPY
			#undef LOOP_

#if !USE_NEW_SIMPLEX_SOLVER
			if (m_simplexSolver.m_numVertices == 3 && isFirstIteration)
			{
				// Check for a degenerate simplex.
				const Vec3V v0 = m_simplexSolver.m_SimplexSeparation[0];
				const Vec3V v1 = m_simplexSolver.m_SimplexSeparation[1];
				const Vec3V v2 = m_simplexSolver.m_SimplexSeparation[2];
				const Vec3V e0 = v1 - v0;
				const Vec3V e1 = v2 - v0;
				const ScalarV thresh(V_FLT_SMALL_4);
				const ScalarV ne0Sq = MagSquared(e0);
				const ScalarV ne1Sq = MagSquared(e1);
				const ScalarV dotP = Dot(e0,e1);
				const ScalarV coSq_thresh(ScalarVFromF32(0.9999f));
				const BoolV degenerate = IsGreaterThanOrEqual(dotP * dotP, coSq_thresh * ne0Sq * ne1Sq) | IsLessThanOrEqual(ne0Sq,thresh) | IsLessThanOrEqual(ne1Sq,thresh);
				if (degenerate.Getb())
					m_simplexSolver.m_numVertices = 2;
			}
#else // !USE_NEW_SIMPLEX_SOLVER
			(void)isFirstIteration;
#endif // !USE_NEW_SIMPLEX_SOLVER

#if USE_NEW_SIMPLEX_SOLVER
			const int result = m_simplexSolver.UpdateClosestVectorAndPoints(v_fltmax,false,true);
			(void)result;
			FastAssert(result == VoronoiSimplexSolver::VALID);
			cachedSeparatingAxisLengthSquared = MagSquared(m_simplexSolver.m_cachedV);
			CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = Sqrt(cachedSeparatingAxisLengthSquared));
			squaredDistance = cachedSeparatingAxisLengthSquared;
			curIter = 1;
			if (isSeparated == false)
			{
				const ScalarV simplexTooCloseEpsilon = CalcSimplexTooCloseEpsilon(m_simplexSolver.m_SimplexSeparation,Vec3V(v_flt_small_5),m_simplexSolver.m_numVertices);
				const ScalarV simplexTooCloseEpsilonSq = simplexTooCloseEpsilon * simplexTooCloseEpsilon;
				if (IsLessThanAll(squaredDistance,simplexTooCloseEpsilonSq))
				{
					FastAssert(isSeparated == false);
					checkSimplex = true;
					needsPenetrationSolve = true;
				}
			}
#else // USE_NEW_SIMPLEX_SOLVER
			if (Likely(m_simplexSolver.UpdateClosestVectorAndPoints()))
			{
				cachedSeparatingAxisLengthSquared = MagSquared(m_simplexSolver.m_cachedV);
				CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = Sqrt(cachedSeparatingAxisLengthSquared));
				squaredDistance = cachedSeparatingAxisLengthSquared;
				curIter = 1;
				if (IsLessThanAll(cachedSeparatingAxisLengthSquared,vEPSILON))
				{
					// Degenerate case, the origin is too close to the simplex.
					FastAssert(isSeparated == false);
					degenerateSimplex = true;
					checkSimplex = true;
				}
			}
			else
			{
				Displayf("Invalid GJK execution path!");
				// If we get here then there was some roundoff in the simplex solver(3 vert case) that caused 1 or more
				// convex coefficients to be less than zero. We'll reset the simplex and run GJK from scratch.
				m_simplexSolver.ResetCache(0);
				m_simplexSolver.m_cachedV = CachedSeparatingAxisDefault;
				cachedSeparatingAxisLengthSquared = v_one;
				CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = v_one);
			}
#endif // USE_NEW_SIMPLEX_SOLVER
		}
		else
		{
			//PPC_STAT_COUNTER_INC(GJKSeedSupportDirCounter,1);
			// Use the cached support direction.
			cachedSeparatingAxisLengthSquared = MagSquared(m_simplexSolver.m_cachedV);
			CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = Sqrt(cachedSeparatingAxisLengthSquared));
			if (Unlikely(IsGreaterThanAll(cachedSeparatingAxisLengthSquared,v_zero) == 0))
			{
				Displayf("Degenerate GJK support direction cache!");
				// Degenerate case, the cached support direction was zero.
				m_simplexSolver.m_cachedV = CachedSeparatingAxisDefault;	// TODO: Compute a better initial support direction.
				cachedSeparatingAxisLengthSquared = v_one;
				CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = v_one;)
			}
		}
	}
	else
	{
		m_simplexSolver.ResetCache(0);
		m_simplexSolver.m_cachedV = CachedSeparatingAxisDefault;	// TODO: Compute a better initial support direction.
		cachedSeparatingAxisLengthSquared = v_one;
		CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = v_one);
#if __BANK
		m_simplexSolver.m_hasCache = g_UseGJKCache;		// Set has hasCache to true for subsequent calls.
#else
		m_simplexSolver.m_hasCache = true;				// Set has hasCache to true for subsequent calls.
#endif
	}
#else
	m_simplexSolver.ResetCache(0);
	m_simplexSolver.m_cachedV = CachedSeparatingAxisDefault;	// TODO: Compute a better initial support direction.
	cachedSeparatingAxisLengthSquared = v_one;
	CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = v_one);
#endif // USE_GJK_CACHE
	GJK_PF_STOP1(NP_GJK_CacheSetup);

#if __PPU
	// Pre-calculate the orthonormal inverse on PSN.
	Mat34V transposeA, transposeB;
	Transpose3x3( transposeA, transA );
	Transpose3x3( transposeB, transB );
#endif

	Vec3V m_cachedSeparatingAxis = m_simplexSolver.m_cachedV;

#if USE_GJK_EARLY_EXIT
	const ScalarV marginSum = m_minkowskiA->GetMarginV() + m_minkowskiB->GetMarginV();
	const ScalarV sepThreshold = input.m_sepThreshold + marginSum;
#endif

#define STORE_LOWER_BOUND 0
#if STORE_LOWER_BOUND
	ScalarV finalLowerBoundDist = -v_fltmax;
	ScalarV maxLowerBoundDist = -v_fltmax;
	Vec3V maxLowerBoundDir = Vec3V(v_zero);
#endif // STORE_LOWER_BOUND

	GJK_PF_START1(NP_GJK_IterTime);
	// Fill the simplex with points on the two objects.

	if (Likely(checkSimplex == false))	// This is for the rare case that the simplex cache was degenerate.
	// I didn't indent this while loop on purpose to minimize the dif.
	while (true)
	{
		// Get the separating axis in the two local coordinate systems.
#if __PPU
		const Vec3V separatingAxisInA = Transform3x3( transposeA, -m_cachedSeparatingAxis );
		const Vec3V separatingAxisInB = Transform3x3( transposeB, m_cachedSeparatingAxis );
#else
		const Vec3V separatingAxisInA = UnTransform3x3Ortho( transA, -m_cachedSeparatingAxis );
		const Vec3V separatingAxisInB = UnTransform3x3Ortho( transB, m_cachedSeparatingAxis );
#endif

		// Get the closest point on each object along the separating axis.
		GJK_PF_START1(SupportFunction);
		//PPC_STAT_TIMER_START(GJKSupportTimer);
		phBound::SupportPoint spA;
		phBound::SupportPoint spB;
		m_minkowskiA->LocalGetSupportingVertexWithoutMarginNotInlined(separatingAxisInA.GetIntrin128(),spA);
		m_minkowskiB->LocalGetSupportingVertexWithoutMarginNotInlined(separatingAxisInB.GetIntrin128(),spB);
		//PPC_STAT_TIMER_STOP(GJKSupportTimer);
		GJK_PF_STOP1(SupportFunction);

		// Transform the closest points on the objects along the axis, into world coordinates.
		const Vec3V worldPointOnA = Transform3x3( transA, spA.m_vertex );
		const Vec3V worldPointOnB = Transform3x3( transB, spB.m_vertex );

		// Get the distance separating the two points along the axis, and see if the objects overlap along the axis.
		// axisDotBtoA is the separation between these two objects along the current separating axis, scaled by the 'magnitude' of the separating axis
		//   (remember that our separating axis is not unit).
		// This probably seems like a strange quantity to calculate, and you're right.  See the comments below to understand why it's calculated.
		const Vec3V worldPointBtoA = (worldPointOnA - worldPointOnB) + separationOffset;
		const ScalarV axisDotBtoA = Dot(m_cachedSeparatingAxis, worldPointBtoA);

#if STORE_LOWER_BOUND
		finalLowerBoundDist = axisDotBtoA / cachedSeparatingAxisLength;
		const BoolV isLarger = IsGreaterThan(finalLowerBoundDist,maxLowerBoundDist);
		maxLowerBoundDist = SelectFT(isLarger,maxLowerBoundDist,finalLowerBoundDist);
		maxLowerBoundDir = SelectFT(isLarger,maxLowerBoundDir,m_cachedSeparatingAxis);
#endif // STORE_LOWER_BOUND

#if USE_GJK_EARLY_EXIT
#if 0
		if (IsGreaterThanAll(axisDotBtoA,v_zero) != 0)
		{
			// Separation lower bound = Dot(supportDir,A-B)/Length(supportDir)
			// If the object are separated then this quantity positive.
			// Test: Dot(supportDir,A-B)/Length(supportDir) > sepThreshold
			// ===> Dot(supportDir,A-B) > sepThreshold * Length(supportDir)
			// ===> Dot(supportDir,A-B)^2 > sepThreshold^2 * Length(supportDir)^2 , if Dot(supportDir,A-B) >= 0
			const ScalarV LeftSide = axisDotBtoA * axisDotBtoA;
			const ScalarV RightSide = sepThreshold * sepThreshold * MagSquared(m_cachedSeparatingAxis);
			if (IsGreaterThanAll(LeftSide,RightSide) != 0)
			{
				// separated.
			}
		}
#else // #if 1/0
#if USE_NEW_SIMPLEX_SOLVER
		const ScalarV separationEpsilon = Dot(Abs(m_cachedSeparatingAxis) + Abs(worldPointBtoA),Vec3V(v_flt_small_5));
		if (IsGreaterThanAll(axisDotBtoA,separationEpsilon) != 0)
		//if (IsGreaterThanAll(axisDotBtoA,v_zero) != 0)
		{
			needsPenetrationSolve = false;
#else // USE_NEW_SIMPLEX_SOLVER
		if (IsGreaterThanAll(axisDotBtoA,v_zero) != 0)
		{
#endif // USE_NEW_SIMPLEX_SOLVER
#if STORE_CACHED_AXIS_LENGTH
			const ScalarV nsupportDir = cachedSeparatingAxisLength;
#else
			const ScalarV nsupportDir = Sqrt(cachedSeparatingAxisLengthSquared);
#endif
			if (IsGreaterThanAll(axisDotBtoA,sepThreshold * nsupportDir) != 0)	
			{
				// The objects are separated.
				FastAssert(IsGreaterThanAll(nsupportDir,v_zero) != 0);
				const ScalarV distance = axisDotBtoA / nsupportDir;
				const ScalarV separationWithMargin = distance - marginSum;
				output->SetSeparationParams(separationWithMargin, m_cachedSeparatingAxis / nsupportDir);
				GJK_PF_STOP1(NP_GJK_IterTime);
				return;
			}
		}
#endif // #if 1/0
#endif // USE_GJK_EARLY_EXIT

#if USE_NEW_SIMPLEX_SOLVER
		// Check if the support point is already in the simplex.
		const ScalarV minSeparationSq = GetMinSeparationSq(m_simplexSolver.m_SimplexSeparation,worldPointBtoA,m_simplexSolver.m_numVertices);
		if (IsLessThanOrEqualAll(minSeparationSq,v_flt_small_4))
		{
			checkSimplex = true;
			break;
		}
#else // USE_NEW_SIMPLEX_SOLVER
		// See if the new point is closer to the origin than any points already in the simplex.
		if (m_simplexSolver.inSimplex(VEC3V_TO_VECTOR3( worldPointBtoA )))
		{
			// The new point isn't closer to the origin than any points already in the simplex, so don't use it.
			degenerateSimplex = true;
			checkSimplex = true;
			break;
		}
#endif // USE_NEW_SIMPLEX_SOLVER

		// are we getting any closer ?
		const ScalarV f0 = squaredDistance - axisDotBtoA;
		const ScalarV f1 = Scale( squaredDistance, REL_ERROR_SQUARED_V );

		// Are we close enough
		if (IsLessThanOrEqualAll(f0, f1)!=0)
		{
#if !USE_NEW_SIMPLEX_SOLVER
			if (IsLessThanOrEqualAll(f0, v_zero)!=0)
			{
				degenerateSimplex = true;
			}
#endif // !USE_NEW_SIMPLEX_SOLVER
			checkSimplex = true;
			break;
		}

		// Add the new point to the simplex.
		// TODO: It might be better to store the vector indices and only convert them at the end.
		ConvertIndex(m_minkowskiA->GetType(),spA.m_index,m_simplexSolver.m_VertexIndexA[m_simplexSolver.m_numVertices]);
		ConvertIndex(m_minkowskiB->GetType(),spB.m_index,m_simplexSolver.m_VertexIndexB[m_simplexSolver.m_numVertices]);
		//m_simplexSolver.AddVertex(worldPointBtoA,spA.m_index,spB.m_index,spA.m_vertex,spB.m_vertex);
		m_simplexSolver.AddVertex(worldPointBtoA,spA.m_vertex,spB.m_vertex);

		// Find the closest point to the origin in the simplex, and make it the new separating axis.
#if USE_NEW_SIMPLEX_SOLVER
		const int result = m_simplexSolver.UpdateClosestVectorAndPoints(squaredDistance,!needsPenetrationSolve,false);
		if (result == VoronoiSimplexSolver::KEEP)
		{
			checkSimplex = true;
			break;
		}
		else if (result == VoronoiSimplexSolver::PENETRATING)
		{
			FastAssert(isSeparated == false);
			needsPenetrationSolve = true;
			break;
		}
		else
		{
			FastAssert(result == VoronoiSimplexSolver::VALID);
			m_cachedSeparatingAxis = m_simplexSolver.m_cachedV;
		}
#else // USE_NEW_SIMPLEX_SOLVER
		if (m_simplexSolver.UpdateClosestVectorAndPoints())
		{
			m_cachedSeparatingAxis = m_simplexSolver.m_cachedV;
		}
		else
		{
			m_simplexSolver.m_cachedV = m_cachedSeparatingAxis; // m_cachedV might get clobbered. Restore it for caching the support direction.
			degenerateSimplex = true;
			checkSimplex = true;
			break;
		}
#endif // USE_NEW_SIMPLEX_SOLVER
		
		cachedSeparatingAxisLengthSquared = MagSquared(m_cachedSeparatingAxis);
		CACHED_AXIS_LENGTH_ONLY(cachedSeparatingAxisLength = Sqrt(cachedSeparatingAxisLengthSquared));
#if !USE_NEW_SIMPLEX_SOLVER
		const ScalarV previousSquaredDistance = squaredDistance;
#endif // !USE_NEW_SIMPLEX_SOLVER
		squaredDistance = cachedSeparatingAxisLengthSquared;

#if USE_NEW_SIMPLEX_SOLVER
		// Check if the objects are too close.
		if (isSeparated == false)
		{
			const ScalarV simplexTooCloseEpsilon = CalcSimplexTooCloseEpsilon(m_simplexSolver.m_SimplexSeparation,Vec3V(v_flt_small_5),m_simplexSolver.m_numVertices);
			const ScalarV simplexTooCloseEpsilonSq = simplexTooCloseEpsilon * simplexTooCloseEpsilon;
			if (IsLessThanAll(squaredDistance,simplexTooCloseEpsilonSq))
			{
				checkSimplex = true;
				needsPenetrationSolve = true;
				break;
			}
		}
#endif // USE_NEW_SIMPLEX_SOLVER

#if !USE_NEW_SIMPLEX_SOLVER
		// This test is really just an rough approximation at seeing how much closer we're getting between iterations (since the difference between two
		//   squares is just ... the difference between two squares).  Nonetheless, the closer the squares are to each other, the closer the two iterations
		//   are so this is checking if we're actually making progress between iterations.  If we aren't, let's stop iterating because we've probably
		//   already ...? (TODO: can someone finish this comment? I'm not sure why we would break early here, or really, why we wouldn't go towards the origin
		//   each iteration... Does this mean we have a tetrahedron simplex already, so we're not making any progress? If not, why do we check a non-tetrahedral
		//   simplex, do we do anything if there is contact but not penetration? Please add any wisdom here...)
		if (IsLessThanOrEqualAll((previousSquaredDistance - squaredDistance), Scale( vEPSILON, previousSquaredDistance)) != 0)
		{
			checkSimplex = true;
			break;
		}
#endif // !USE_NEW_SIMPLEX_SOLVER

		GJK_PF_INCREMENT1(NP_GJK_Iters);
		//PPC_STAT_COUNTER_INC(GJKIterationCounter,1);
		if (Unlikely(curIter++ > MAX_ITER))
		{   
			// We get here when GJK fails to converge. This usually happens when one or both of the objects
			// is too large, which worsens numerical round-off problems. The convergence condition can fail, even
			// if using a relative epsilon.
#if __DEV
			Warningf("GjkPairDetector maxIter exceeded:%i\n",curIter);   
			Warningf("sepAxis=(%f,%f,%f), squaredDistance = %f, shapeTypeA=%i,shapeTypeB=%i\n",   
				m_cachedSeparatingAxis.GetXf(),
				m_cachedSeparatingAxis.GetYf(),
				m_cachedSeparatingAxis.GetZf(),
				squaredDistance.Getf(),
				m_minkowskiA->GetType(),   
				m_minkowskiB->GetType());
#endif // __DEV  
#if STORE_LOWER_BOUND
			const Vec3V dimsA = m_minkowskiA->GetBoundingBoxMax() - m_minkowskiA->GetBoundingBoxMin();
			const Vec3V dimsB = m_minkowskiB->GetBoundingBoxMax() - m_minkowskiB->GetBoundingBoxMin();
			Warningf("isSeparated:(%f), dimsA(%f,%f,%f), dimsB(%f,%f,%f)\n",
				lowerBoundDistance.Getf(),
				dimsA.GetXf(),dimsA.GetYf(),dimsA.GetZf(),
				dimsB.GetXf(),dimsB.GetYf(),dimsB.GetZf());
#endif // STORE_LOWER_BOUND
			break;
		}

#if !USE_NEW_SIMPLEX_SOLVER
		if (m_simplexSolver.fullSimplex())
		{
			break;
		}
#endif // !USE_NEW_SIMPLEX_SOLVER
	}
	GJK_PF_STOP1(NP_GJK_IterTime);

#if USE_GJK_CACHE
	FastAssert(m_simplexSolver.m_needsUpdate == false);
	if (m_simplexSolver.m_numVertices > 3)
	{
		FastAssert(m_simplexSolver.m_numVertices == 4);
		// If we get here then either the simplex was penetrating the origin or
		// the simplex was degenerate, meaning that the last vertex added was 
		// coplanar or colinear with the previous simplex. Either way, we want
		// to keep the 3 previous vertices for the simplex cache.
		m_simplexSolver.m_numVertices = 3;
	}
#endif

	bool isValid = false;
	Vec3V worldNormalBtoA = Vec3V(v_zero);
	ScalarV separationWithMargin = v_fltmax;
	if (checkSimplex)
	{
		// Get the collision normal on object A. This should only happen when there is no penetration, so the normal into A is the displacement from the point on B to the point on A.
		worldNormalBtoA = m_cachedSeparatingAxis;
		const ScalarV axisLength2 = cachedSeparatingAxisLengthSquared;
		//valid normal
#if !USE_NEW_SIMPLEX_SOLVER
		if (IsLessThanAll(axisLength2, AXIS_DEGENERATE_LIMIT)!=0)
		{
			degenerateSimplex = true;
		} 
#endif // !USE_NEW_SIMPLEX_SOLVER

		if (Likely(IsGreaterThanAll(axisLength2, ScalarV( 0.0f ) )!=0))
		{
			// Compute the length.
#if STORE_CACHED_AXIS_LENGTH
			const ScalarV axisLength = cachedSeparatingAxisLength;
#else
			const ScalarV axisLength = Sqrt(axisLength2);
#endif

			// Normalize the normal
			worldNormalBtoA = worldNormalBtoA / axisLength;

			// The separation is the axis length minus the margins
			separationWithMargin = axisLength - marginSum;

			isValid = true;
		}
	}

#if USE_NEW_SIMPLEX_SOLVER
	FastAssert(isValid || needsPenetrationSolve);
#endif // USE_NEW_SIMPLEX_SOLVER

	output->worldNormalBtoA = worldNormalBtoA;
	output->separationWithMargin = separationWithMargin;
	output->isValid = isValid;
#if USE_NEW_SIMPLEX_SOLVER
	output->m_needsPenetrationSolve = needsPenetrationSolve;
#else // USE_NEW_SIMPLEX_SOLVER
	const bool catchDegeneratePenetrationCase = (degenerateSimplex && (IsLessThanAll((separationWithMargin+marginSum), DEGENERATE_PENETRATION_LIMIT)!=0));
	output->m_needsPenetrationSolve = (!isValid || catchDegeneratePenetrationCase);
#endif // USE_NEW_SIMPLEX_SOLVER
	output->isSeparatedBeyondSepThresh = false;
}

} // namespace rage
