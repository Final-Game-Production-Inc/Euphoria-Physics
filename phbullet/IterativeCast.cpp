// 
// phbullet/IterativeCast.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "iterativecast.h"

#include "phbound/bound.h"
#include "phbound/support.h"

#include "phbullet/VoronoiSingleSimplexSolver.h"
#include "phbullet/VoronoiDegenerateSimplexSolver.h"

#define ITERATIVE_CAST_MAX_ITERATIONS 32

namespace rage
{
	struct CastBase
	{
		__forceinline CastBase(){}

		// PURPOSE:
		//   Construct the cast
		// PARAMS:
		//   input - The input structure of the cast
		//   localizationOffset - vector to be subtracted from sweep to reduce FP error
		//   localBoundMargin - The margin of the bound being casted against
		//   errorTolerance - The tolerance for error when determining an intersection.
		//                    The larger this is the fewer iterations it will take to find an intersection. Especially with curved shapes.
		CastBase(const CastBaseInput& input, Vec3V_In localizationOffset, ScalarV_In localBoundMargin, ScalarV_In errorTolerance);

		// PURPOSE:
		//   Compute an initial separating axis pointing away from the local bound. 
		// PARAMS:
		//   localBound - reference to the bound being casted against
		//   localizationOffset - vector to be subtracted from sweep to reduce FP error
		// RETURN:
		//   The separating axis from the bound to the cast. 
		Vec3V_Out ComputeInitialSeparatingAxis(const phBound& localBound, Vec3V_In localizationOffset);

		// PURPOSE:
		//   Compute a new separating axis between the cast and the local bound
		// PARAMS:
		//   supportLocalBound - the support point on the local bound from the last separating axis
		//   separatingAxis - out parameter that should be filled with the new separating axis
		// RETURN:
		//   0 means the cast and the bound are colliding, separatingAxis doesn't need to be set
		//    in that case. Non-zero means we found a new separating axis. 
		int ComputeNewSeparatingAxis(Vec3V_In supportLocalBound, Vec3V_InOut separatingAxis);

		// PURPOSE:
		//   Compute the separation between the cast and the local bound along the separating axis
		// PARAMS:
		//   separatingAxis - the separating axis we're getting separation along
		//   supportLocalBound -  the support point on the local bound computed with separatingAxis
		// RETURN:
		//   Separation between the cast and local bound along the separating axis. 
		// NOTE:
		//   This gets called each iteration. 
		//   If the separation is positive, there is room to move the cast forward. At that point, either
		//     the cast is moving towards the bound along the axis and we can move it until the gap is closed,
		//     or the cast is moving away from the bound and we can return no intersection.
		ScalarV_Out ComputeSeparationAlongAxis(Vec3V_In separatingAxis, Vec3V_In supportLocalBound);

		// PURPOSE: 
		//   Compute the velocity of the cast along the separating axis
		// PARAMS:
		//   separatingAxis - the separating axis we're getting the velocity along
		// RETURN:
		//   The velocity of the cast projected onto the separating axis
		// NOTE:
		//   This gets called whenever a separating axis is found where the cast and the bound don't overlap.
		//   If the velocity along the axis is negtive, the cast is moving away from the bound and the algorithm
		//     will stop and return no intersection.
		__forceinline ScalarV_Out ComputeVelocityAlongAxis(Vec3V_In separatingAxis)
		{
			return Dot(separatingAxis,m_NegativeSegment);
		}

		// PURPOSE: 
		//   Move the cast forward to the current [0-1] time
		// PARAMS:
		//   time - new time in the cast
		// NOTES:
		//   This is called whenever we find a valid separating axis and move the cast forward.
		__forceinline void UpdateTime(ScalarV_In time)
		{
			// Update the cast's position to the new time
			m_CurrentPosition = AddScaled(m_InitialPosition, m_Segment, time);
		}

		// PURPOSE: 
		//   Get the closest point on the local bound to the cast, and the vector from the local bound's closest point to the
		//     cast's closest point. 
		// PARAMS:
		//   positionOnLocalBound - Out parameter that should be set to the closest position on the local bound to the cast
		//   separationVector - Out parameter that should be set to the vector from the closest point on the local bound to the 
		//                       closest point on the cast
		// NOTE:
		//   This is called once at the end of a successful cast to fill the results structure
		void GetPositionAndSeparationVectorOnLocalBound(Vec3V_InOut positionOnLocalBound, Vec3V_InOut separationVector);

		Vec3V m_InitialPosition;					// The initial position of the cast
		Vec3V m_Segment;							// The segment from the initial position to the final position
		Vec3V m_NegativeSegment;					// The negative segment
		ScalarV m_TotalMargin;						// The sum of the margins around the cast and bound
		ScalarV m_IntersectionToleranceSquared;		// If the marginless cast and bound are within this distance there is an intersection
		Vec3V m_CurrentPosition;					// The current position of the cast
	};


	struct CastPoint : public CastBase
	{
		__forceinline CastPoint(){}
		__forceinline CastPoint(const CastPointInput& input, Vec3V_In localizationOffset, ScalarV_In localBoundMargin, ScalarV_In errorTolerance)
		{
			m_InitialPosition = m_CurrentPosition = (input.m_InitialPosition - localizationOffset);
			m_Segment = input.m_Segment;
			m_NegativeSegment = Negate(m_Segment);
			m_TotalMargin = localBoundMargin;
			const ScalarV intersectionTolerance = Add(localBoundMargin,errorTolerance);
			m_IntersectionToleranceSquared = Scale(intersectionTolerance,intersectionTolerance);
		}

		__forceinline Vec3V_Out ComputeInitialSeparatingAxis(const phBound& localBound, Vec3V_In localizationOffset)
		{
			// TODO: 
			//   Is the better initial supporting vertex worth the LocalGetSupportingVertexWithoutMargin call?
			const Vec3V supportLocalBound = Subtract(localBound.LocalGetSupportingVertexWithoutMarginNotInlined(m_NegativeSegment.GetIntrin128()),localizationOffset);
			return NormalizeSafe(Subtract(m_InitialPosition, supportLocalBound), Vec3V(V_X_AXIS_WZERO));
		}

		__forceinline ScalarV_Out ComputeSeparationAlongAxis(Vec3V_In separatingAxis, Vec3V_In supportLocalBound)
		{
			// The distance between the cast and bound must take the margins into account
			const Vec3V supportToCast = Subtract(m_CurrentPosition, supportLocalBound);
			return Subtract(Dot(separatingAxis, supportToCast), m_TotalMargin);
		}

		__forceinline void GetPositionAndSeparationVectorOnLocalBound(Vec3V_InOut positionOnLocalBound, Vec3V_InOut separationVector)
		{
			positionOnLocalBound = m_ClosestPositionOnLocalBound;
			separationVector = Subtract(m_CurrentPosition,m_ClosestPositionOnLocalBound);
		}

		__forceinline int ComputeNewSeparatingAxis(Vec3V_In supportLocalBound, Vec3V_InOut separatingAxis)
		{
			// Add the bound's support vertex to the simplex and find the new supporting vertex
			m_SimplexSolver.AddVertex(supportLocalBound,separatingAxis);

			m_SimplexSolver.ComputeClosestPointAndNormalOnSimplex(m_CurrentPosition,m_ClosestPositionOnLocalBound,separatingAxis);
			return IsGreaterThanAll(DistSquared(m_ClosestPositionOnLocalBound,m_CurrentPosition), m_IntersectionToleranceSquared);
		}

		Vec3V m_ClosestPositionOnLocalBound;
		VoronoiSingleSimplexSolver m_SimplexSolver;	// Simplex solver used to generate new separating axiis
	};

	struct CastSphere : public CastPoint
	{
		__forceinline CastSphere(){}
		__forceinline CastSphere(const CastSphereInput& input, Vec3V_In localizationOffset, ScalarV_In localBoundMargin, ScalarV_In errorTolerance)
		{
			m_InitialPosition = m_CurrentPosition = (input.m_InitialPosition - localizationOffset);
			m_Segment = input.m_Segment;
			m_NegativeSegment = Negate(m_Segment);
			m_TotalMargin = Add(localBoundMargin, input.m_SphereRadius);
			const ScalarV intersectionTolerance = Add(m_TotalMargin,errorTolerance);
			m_IntersectionToleranceSquared = Scale(intersectionTolerance,intersectionTolerance);
		}
	};

	struct CastScalingSphere : public CastSphere
	{
		__forceinline CastScalingSphere(){}
		__forceinline CastScalingSphere(const CastScalingSphereInput& input, Vec3V_In localizationOffset, ScalarV_In localBoundMargin, ScalarV_In errorTolerance)
		{
			m_InitialPosition = m_CurrentPosition = (input.m_InitialPosition - localizationOffset);
			m_Segment = input.m_Segment;
			m_NegativeSegment = Negate(m_Segment);
			m_SphereRadiusGrowth = input.m_SphereRadiusGrowth;
			m_InitialTotalMargin = Add(localBoundMargin, input.m_SphereRadius);
			m_TotalMargin = m_InitialTotalMargin;
			m_ErrorTolerance = errorTolerance;
			const ScalarV intersectionTolerance = Add(m_TotalMargin,errorTolerance);
			m_IntersectionToleranceSquared = Scale(intersectionTolerance,intersectionTolerance);
		}

		__forceinline ScalarV_Out ComputeVelocityAlongAxis(Vec3V_In separatingAxis)
		{
			// Since the sphere growth is in all directions, add it directly to the velocity of the sphere's center along
			//   the separating axis
			return Add(CastPoint::ComputeVelocityAlongAxis(separatingAxis), m_SphereRadiusGrowth);
		}

		__forceinline void UpdateTime(ScalarV_In time)
		{
			CastPoint::UpdateTime(time);

			// Grow the sphere and intersection tolerance to match the new cast time
			m_TotalMargin = AddScaled(m_InitialTotalMargin, m_SphereRadiusGrowth, time);
			const ScalarV intersectionTolerance = Add(m_TotalMargin, m_ErrorTolerance);
			m_IntersectionToleranceSquared = Scale(intersectionTolerance,intersectionTolerance);
		}

		ScalarV m_SphereRadiusGrowth;
		ScalarV m_InitialTotalMargin;
		ScalarV m_ErrorTolerance;
	};

	struct CastBound : public CastBase
	{
		__forceinline CastBound(const phBound& castBound) : m_CastBound(castBound) {}
		__forceinline CastBound(const CastBoundInput& input, Vec3V_In localizationOffset, ScalarV_In localBoundMargin, ScalarV_In errorTolerance) : m_CastBound(input.m_Bound)
		{
			m_InitialPosition = m_CurrentPosition = (input.m_InitialPosition - localizationOffset);
			m_Segment = input.m_Segment;
			m_NegativeSegment = Negate(m_Segment);
			m_TotalMargin = Add(localBoundMargin, m_CastBound.GetMarginV());
			const ScalarV intersectionTolerance = Add(m_TotalMargin,errorTolerance);
			m_IntersectionToleranceSquared = Scale(intersectionTolerance,intersectionTolerance);
			Transpose(m_CastBoundFromLocalBoundNormal, input.m_Rotation);
			m_LocalBoundFromCastBound = input.m_Rotation;
		}

		__forceinline Vec3V_Out ComputeInitialSeparatingAxis(const phBound& localBound, Vec3V_In localizationOffset)
		{
			// TODO: 
			//   Is the better initial supporting vertex worth the LocalGetSupportingVertexWithoutMargin calls?
			const Vec3V supportLocalBound = Subtract(localBound.LocalGetSupportingVertexWithoutMarginNotInlined(m_NegativeSegment.GetIntrin128()),localizationOffset);
			const Vec3V supportCastBound = Multiply(m_LocalBoundFromCastBound, m_CastBound.LocalGetSupportingVertexWithoutMarginNotInlined(Multiply(m_CastBoundFromLocalBoundNormal, m_Segment).GetIntrin128()));
			return NormalizeSafe(Subtract(m_InitialPosition, Subtract(supportLocalBound,supportCastBound)), Vec3V(V_X_AXIS_WZERO));
		}

		__forceinline ScalarV_Out ComputeSeparationAlongAxis(Vec3V_In separatingAxis, Vec3V_In supportLocalBound)
		{
			m_SupportCastBound = m_CastBound.LocalGetSupportingVertexWithoutMarginNotInlined(Multiply(m_CastBoundFromLocalBoundNormal, Negate(separatingAxis)).GetIntrin128());
			Vec3V supportDifference = Subtract(supportLocalBound,Multiply(m_LocalBoundFromCastBound, m_SupportCastBound));

			// Add the vertex to the simplex before possibly scaling the vertices in the simplex and ruining the degeneracy check data.
			// This is only necessary for scaling bounds but the cost of moving this function here is very small, and this helps keep the code cleaner.
			m_SimplexSolver.AddVertex(supportDifference, supportLocalBound, m_SupportCastBound);

			const Vec3V supportToCast = Subtract(m_CurrentPosition, supportDifference);
			return Subtract(Dot(separatingAxis, supportToCast), m_TotalMargin);
		}

		__forceinline int ComputeNewSeparatingAxis(Vec3V_In UNUSED_PARAM(supportLocalBound), Vec3V_InOut separatingAxis)
		{
			// Add both supporting points to the simplex and compute the new separating axis
			m_SimplexSolver.UpdateClosestVectorAndPoints(m_CurrentPosition);
			separatingAxis = m_SimplexSolver.GetCachedNormal();
			return IsGreaterThanAll(m_SimplexSolver.GetCachedDistanceSquared(), m_IntersectionToleranceSquared);
		}

		__forceinline void GetPositionAndSeparationVectorOnLocalBound(Vec3V_InOut positionOnLocalBound, Vec3V_InOut separationVector)
		{
			Vec3V temp;
			m_SimplexSolver.GetClosestPoints(positionOnLocalBound, temp);
			separationVector = m_SimplexSolver.GetCachedVector();
		}

		const phBound& m_CastBound;		// The bound being cast

		Mat33V m_CastBoundFromLocalBoundNormal;
		Mat33V m_LocalBoundFromCastBound;

		Vec3V m_SupportCastBound;		// Temp storage for the support point on the cast

		VoronoiDegenerateSimplexSolver m_SimplexSolver;
	};


	struct CastScalingBound : public CastBound
	{
		__forceinline CastScalingBound(const CastScalingBoundInput& input, Vec3V_In localizationOffset, ScalarV_In localBoundMargin, ScalarV_In errorTolerance) : CastBound(input.m_Bound)
		{
			m_InitialPosition = m_CurrentPosition = (input.m_InitialPosition - localizationOffset);
			m_Segment = input.m_Segment;
			m_NegativeSegment = Negate(m_Segment);
			m_TotalMargin = Add(localBoundMargin, m_CastBound.GetMarginV());
			const ScalarV intersectionTolerance = Add(m_TotalMargin,errorTolerance);
			m_IntersectionToleranceSquared = Scale(intersectionTolerance,intersectionTolerance);

			// There is an issue where if the scale is zero, we will transform the separating axis into the bound's space
			//   and get a zero vector. This means that the support point we get is random. When computing the scaling 
			//   velocity along the separating axis, it is important that we use the correct point, or we'll get growth
			//   in the wrong direction. There is probably a better solution than just having a minimum scale, but I 
			//   couldn't come up with a cheap alternative.
			m_InitialScale = Max(input.m_InitialScale,Vec3V(V_FLT_SMALL_6));
			
			m_ScaleGrowth = input.m_ScaleGrowth;
			m_Rotation = input.m_Rotation;
			Transpose(m_InvRotation,m_Rotation);

			UpdateScale(m_InitialScale);

			Mat33V scaleGrowthMatrix;
			Mat33VFromScale(scaleGrowthMatrix,m_ScaleGrowth);
			Multiply(m_GrowthTransform, m_Rotation, scaleGrowthMatrix);
		}

		__forceinline ScalarV_Out ComputeVelocityAlongAxis(Vec3V_In separatingAxis)
		{
			return Dot(Subtract(m_NegativeSegment, Multiply(m_GrowthTransform, m_SupportCastBound)), separatingAxis);
		}

		class UpdateMinkowskiFunctor
		{
		public:
			__forceinline UpdateMinkowskiFunctor(Mat33V_In localBoundFromCastBound) : m_LocalBoundFromCastBound(localBoundFromCastBound) {}

			__forceinline Vec3V_Out operator()(Vec3V_In pointLocalBound, Vec3V_In pointCastBound) const
			{
				return Subtract(pointLocalBound, Multiply(m_LocalBoundFromCastBound,pointCastBound));
			}
		private:
			Mat33V m_LocalBoundFromCastBound;
		};

		__forceinline void UpdateTime(ScalarV_In time)
		{
			CastBound::UpdateTime(time);

			// Compute the new bound matrices from the new scale
			UpdateScale(AddScaled(m_InitialScale,m_ScaleGrowth,time));

			// Update the minkowski point to fit the new scale
			m_SimplexSolver.UpdateMinkowskiPoints(UpdateMinkowskiFunctor(m_LocalBoundFromCastBound));
		}

		// Helper function to update the matrices from a new scale
		__forceinline void UpdateScale(Vec3V_In newScale)
		{
			Mat33V currentCastBoundScale;
			Mat33VFromScale(currentCastBoundScale,newScale);
			Multiply(m_CastBoundFromLocalBoundNormal, currentCastBoundScale, m_InvRotation);
			Multiply(m_LocalBoundFromCastBound, m_Rotation, currentCastBoundScale);
		}

		Vec3V m_InitialScale;
		Vec3V m_ScaleGrowth;

		Mat33V m_GrowthTransform;
		Mat33V m_Rotation;
		Mat33V m_InvRotation;
	};

template <class CastInput>
bool IterativeCast(const CastInput& castInput, const phBound& localBound, IterativeCastResult& results)
{
	const ScalarV svZero(V_ZERO);
	const ScalarV svOne(V_ONE);
	const ScalarV localBoundMargin = localBound.GetMarginV();

	// Move the cast near the origin to reduce FP error. Unlike typical GJK algorithms we don't have a near-zero simplex since
	//   we take advantage of the fact that "FindClosestPoint((LocalSimplexA + PositionA) - (LocalSimplexB + PositionB), Origin)" is the
	//   same as "FindClosesPoint(LocalSimplexA - LocalSimplexB, -(PositionA - PositionB)) + (PositionA - PositionB". This seems like more
	//   work but it means we don't need to recompute our simplex each time PositionA or PositionB changes. 
	const Vec3V localizationOffset = localBound.GetCentroidOffset();

	// Enforce a minimum margin so the cast doesn't push through the bound and cause the simplex solver to fail. This minimum won't be reflected in the final position. 
	const ScalarV svMinimumBoundMargin(V_FLT_SMALL_3);
	const ScalarV localBoundMarginForSeparation = Max(localBoundMargin,svMinimumBoundMargin);

	// Construct the cast from the inputs
	typename CastInput::CastType cast(castInput,localizationOffset,localBoundMarginForSeparation,ScalarV(ITERATIVE_CAST_INTERSECTION_TOLERANCE));

	ScalarV time = svZero;
	Vec3V separatingAxis = cast.ComputeInitialSeparatingAxis(localBound,localizationOffset);
	Vec3V normalOnLocalBound = separatingAxis;
	Vec3V supportLocalBound;
	int iterations = ITERATIVE_CAST_MAX_ITERATIONS;
	do
	{
		// Compute the separation between the cast and the bound along the separating axis. 
		supportLocalBound = Subtract(localBound.LocalGetSupportingVertexWithoutMarginNotInlined(separatingAxis.GetIntrin128()), localizationOffset);
		ScalarV separationAlongAxis = cast.ComputeSeparationAlongAxis(separatingAxis,supportLocalBound);

		// If there is any separation, we can move the cast forward to fill the gap. If there is overlap
		//   it means we need to find a better separating axis before moving forward.
		if(IsGreaterThanAll(separationAlongAxis,svZero))
		{
			// Compute the velocity of the cast along the separating axis. With the velocity and
			//   separation we can compute the time until impact. 
			ScalarV velocityAlongAxis = cast.ComputeVelocityAlongAxis(separatingAxis);
			time = Add(time, InvScale(separationAlongAxis,velocityAlongAxis));
			if(And(IsGreaterThan(velocityAlongAxis,svZero), IsLessThan(time,svOne)).Getb())
			{
				// We were able to successfully move the time of the cast forward. Inform the cast.
				cast.UpdateTime(time);

				// Keep track of the last separating axis to use as the intersection normal. 
				normalOnLocalBound = separatingAxis;
			}
			else
			{
				// If the velocity along the axis is negative it means that the cast is moving away from the bound, so
				//   there can be no intersection. 
				// If the time is greater than one it means that the cast doesn't collide with the bound within the given
				//   time frame.
				return false;
			}
		}
		// If ComputeNewSeparatingAxis returns 0 it means we're close enough to the bound, so we can stop iterating.
	}while((GenerateMaskGZ(cast.ComputeNewSeparatingAxis(supportLocalBound,separatingAxis)) & GenerateMaskGZ(--iterations)) != 0);

#if __ASSERT
	if(iterations == 0)
	{
		Warningf("IterativeCast ran out of iterations before finding an intersection.");
	}
#endif // __ASSERT

	results.m_IntersectionTime = time;
	results.m_NormalOnLocalBound = normalOnLocalBound;

	// We need to add the margin into the position on the local bound. For numerical precision 
	//   the separating axis doesn't always line up with the vector between the cast and the bound. 
	//   Choosing to use the separation vector as the margin normal guarantees that the result point
	//   is within the specified tolerance. 
	// Also add back in the localization offset
	Vec3V marginlessPositionOnBound;
	Vec3V separationVector;
	cast.GetPositionAndSeparationVectorOnLocalBound(marginlessPositionOnBound, separationVector);
	results.m_Position = Add(AddScaled(marginlessPositionOnBound, NormalizeSafe(separationVector,Vec3V(V_ZERO),Vec3V(V_FLT_SMALL_12)), localBoundMargin), localizationOffset);

	return true;
}

// Explicit instantiation, which allows us to put the code in the .cpp file
template bool IterativeCast(const CastPointInput&, const phBound&, IterativeCastResult&);
template bool IterativeCast(const CastSphereInput&, const phBound&, IterativeCastResult&);
template bool IterativeCast(const CastScalingSphereInput&, const phBound&, IterativeCastResult&);
template bool IterativeCast(const CastBoundInput&, const phBound&, IterativeCastResult&);
template bool IterativeCast(const CastScalingBoundInput&, const phBound&, IterativeCastResult&);

} // namespace rage
