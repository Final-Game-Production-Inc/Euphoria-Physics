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




#ifndef GJK_PAIR_DETECTOR_H
#define GJK_PAIR_DETECTOR_H

#include "DiscreteCollisionDetectorInterface.h"
#include "vectormath/classes.h"
#include "phbullet/CollisionMargin.h"

#include "SimplexSolverInterface.h"

#define USE_GJK_EARLY_EXIT 1
#define USE_FULL_SIMPLEX_COPY 0		// Experimental optimization

namespace rage {

class phBound;

#define CachedSeparatingAxisDefault Vec3V( V_Z_AXIS_WONE )
#define SimplexTooCloseFixedEpsilon ScalarVFromF32(.001f)
#define PENETRATION_CHECK_EXTRA_MARGIN_SV ScalarVFromF32(PENETRATION_CHECK_EXTRA_MARGIN)
#if !USE_NEW_SIMPLEX_SOLVER
#define DEGENERATE_PENETRATION_LIMIT ScalarVFromF32(.01f)
#endif // !USE_NEW_SIMPLEX_SOLVER

/// GjkPairDetector uses GJK to implement the DiscreteCollisionDetectorInterface
class GjkPairDetector : public DiscreteCollisionDetectorInterface
{

public:
	VoronoiSimplexSolver m_simplexSolver;

	GjkPairDetector() {}

	~GjkPairDetector() {}

	__forceinline void GetLocalPoints(Vec3V * CCD_RESTRICT localPointA_Out, Vec3V * CCD_RESTRICT localPointB_Out) const
	{
		FastAssert(localPointA_Out);
		FastAssert(localPointB_Out);
		FastAssert(localPointA_Out != localPointB_Out);
		FastAssert(m_simplexSolver.m_numVertices > 0 && m_simplexSolver.m_numVertices <= 3);
		Vec3V localPointA = m_simplexSolver.m_SimplexCoords[0] * m_simplexSolver.m_LocalA[0];
		Vec3V localPointB = m_simplexSolver.m_SimplexCoords[0] * m_simplexSolver.m_LocalB[0];
		#undef LOOP_
		#define LOOP_(i) localPointA += m_simplexSolver.m_SimplexCoords[i] * m_simplexSolver.m_LocalA[i]; localPointB += m_simplexSolver.m_SimplexCoords[i] * m_simplexSolver.m_LocalB[i];
		switch (m_simplexSolver.m_numVertices)
		{
			case 3:
				LOOP_(2);
			case 2:
				LOOP_(1);
		}
		#undef LOOP_
		*localPointA_Out = localPointA;
		*localPointB_Out = localPointB;
	}

	Vec3V_Out CalcInitialSeparatingAxis (const Matrix34& currentA, const phBound* boundA, const Matrix34& currentB, const phBound* boundB);

	void DoGjkIterations(const ClosestPointInput & input, const bool isSeparated, const bool isFirstIteration, ClosestPointOutput * CCD_RESTRICT output);

private:
};

} // namespace rage

#endif //GJK_PAIR_DETECTOR_H
