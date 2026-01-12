// 
// phbullet/IterativeCast.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_ITERATIVE_CAST_H
#define PHYSICS_ITERATIVE_CAST_H

#include "vectormath/classes.h"

// If the casting shape is within this distance of the bound then we will treat them as colliding
// Lowering this value has the potential to slow down the algorithm and increases the chances of not finding
//   a solution. 1cm was chosen somewhat arbitrarily so it might be worth changing. 
#define ITERATIVE_CAST_INTERSECTION_TOLERANCE 0.01f

namespace rage
{
	class phBound;

	// CastBase is just a base class with some functionality shared between all current casts,
	//   it is not possible to cast this.
	struct CastBase;
	class CastBaseInput
	{
	public:
		CastBaseInput(Vec3V_In initialPosition, Vec3V_In segment) :	
			m_InitialPosition(initialPosition),
			m_Segment(segment)
		{}

		typedef CastBase CastType;
		friend struct CastBase;
	protected:
		Vec3V m_InitialPosition;
		Vec3V m_Segment;
	};

	// CastPoint will cast a point from a start position, along a given segment
	struct CastPoint;
	class CastPointInput : public CastBaseInput
	{
	public:
		// PURPOSE:
		//   Construct the input required for a point cast
		// PARAMS:
		//   initialPosition - The initial position of the point
		//   segment - The segment from the initial point position to the final point position
		CastPointInput(Vec3V_In initialPosition, Vec3V_In segment) : CastBaseInput(initialPosition,segment)
		{}

		typedef CastPoint CastType;
		friend struct CastPoint;
	};


	// CastSphere will cast a sphere from a start position, along a given segment
	struct CastSphere;
	class CastSphereInput : public CastPointInput
	{
	public:
		// PURPOSE:
		//   Construct the input required for a sphere cast
		// PARAMS:
		//   initialPosition - The initial position of the sphere in the local bound's space
		//   segment - The segment from the initial sphere position to the final sphere position in the local bound's space
		//   sphereRadius - The radius of the sphere (this must be non-negative)
		CastSphereInput(Vec3V_In initialPosition, Vec3V_In segment, ScalarV_In sphereRadius) :	
			CastPointInput(initialPosition, segment),
			m_SphereRadius(sphereRadius)
		{
			Assertf(IsGreaterThanOrEqualAll(sphereRadius,ScalarV(V_ZERO)), "Trying to cast a sphere of negative radius. Radius = %f", sphereRadius.Getf());
		}

		typedef CastSphere CastType;
		friend struct CastSphere;
	protected:
		ScalarV m_SphereRadius;
	};

	// CastScalingSphere will cast a sphere that grows or shrink
	struct CastScalingSphere;
	class CastScalingSphereInput : public CastSphereInput
	{
	public:
		// PURPOSE:
		//   Construct the input required for a scaling sphere cast
		// PARAMS:
		//   initialPosition - The initial position of the sphere in the local bound's space
		//   segment - The segment from the initial sphere position to the final sphere position in the local bound's space
		//   initialSphereRadius - The initial radius of the sphere (this must be non-negative)
		//   sphereRadiusGrowth - The value by which the spheres radius increases during the cast (this cannot cause the sphere to get a negative radius during the cast)
		CastScalingSphereInput(Vec3V_In initialPosition, Vec3V_In segment, ScalarV_In initialSphereRadius, ScalarV_In sphereRadiusGrowth) :
			CastSphereInput(initialPosition, segment, initialSphereRadius),
			m_SphereRadiusGrowth(sphereRadiusGrowth)
		{
			Assertf(IsGreaterThanOrEqualAll(Add(initialSphereRadius,sphereRadiusGrowth), ScalarV(V_ZERO)), "Trying to cast an scaling sphere that gets a negative radius during the cast. Radius = %f, Growth = %f", initialSphereRadius.Getf(), sphereRadiusGrowth.Getf());
		}

		typedef CastScalingSphere CastType;
		friend struct CastScalingSphere;
	protected:
		ScalarV m_SphereRadiusGrowth;
	};

	// CastBound will cast a convex bound
	struct CastBound;
	class CastBoundInput : public CastBaseInput
	{
	public:
		// PURPOSE:
		//   Construct the input required for a bound cast
		// PARAMS:
		//   initialPosition - The initial position of the sphere in the local bound's space
		//   segment - The segment from the initial sphere position to the final sphere position in the local bound's space
		//   rotation - the rotation matrix from the bound's space to the local bound's space
		//   bound - the bound that is being cast
		CastBoundInput(Vec3V_In initialPosition, Vec3V_In segment, Mat33V_In rotation, const phBound& bound) :
			CastBaseInput(initialPosition, segment),
			m_Rotation(rotation),
			m_Bound(bound)
		{
			Assertf(rotation.IsOrthonormal(ScalarV(V_FLT_SMALL_2)), "Non-orthonormal rotation matrix on cast bound."
																	"\n\t%f %f %f"
																	"\n\t%f %f %f"
																	"\n\t%f %f %f",
																	rotation.GetCol0().GetXf(), rotation.GetCol1().GetXf(), rotation.GetCol2().GetXf(),
																	rotation.GetCol0().GetYf(), rotation.GetCol1().GetYf(), rotation.GetCol2().GetYf(),
																	rotation.GetCol0().GetZf(), rotation.GetCol1().GetZf(), rotation.GetCol2().GetZf());
			Assertf(&bound != NULL, "Trying to cast NULL bound.");
		}

		typedef CastBound CastType;
		friend struct CastBound;
	protected:
		Mat33V m_Rotation;
		const phBound& m_Bound;
	};

	// CastScalingBound will cast a bound that scales during the cast
	struct CastScalingBound;
	class CastScalingBoundInput : public CastBoundInput
	{
	public:
		// PURPOSE:
		//   Construct the input required for a bound cast
		// PARAMS:
		//   initialPosition - The initial position of the sphere in the local bound's space
		//   segment - The segment from the initial sphere position to the final sphere position in the local bound's space
		//   initialScale - The initial scale applied to the marginless bound
		//   scaleGrowth - The difference in scale from the initial scale to the final scale
		//   rotation - the rotation matrix from the bound's space to the local bound's space
		//   bound - the bound that is being cast
		CastScalingBoundInput(Vec3V_In initialPosition, Vec3V_In segment, Vec3V_In initialScale, Vec3V_In scaleGrowth, Mat33V_In rotation, const phBound& bound) :
			CastBoundInput(initialPosition, segment, rotation, bound),
			m_InitialScale(initialScale),
			m_ScaleGrowth(scaleGrowth)
		{
			Assertf(IsGreaterThanOrEqualAll(initialScale, Vec3V(V_ZERO)), "Trying to cast a bound with negative scale. Scale=(%f, %f, %f)",initialScale.GetXf(),initialScale.GetYf(),initialScale.GetZf());
			Assertf(IsGreaterThanOrEqualAll(Add(initialScale,scaleGrowth), Vec3V(V_ZERO)), "Trying to cast a bound that gets a negative scale during the cast. Scale=(%f, %f, %f), Growth=(%f, %f, %f)",initialScale.GetXf(),initialScale.GetYf(),initialScale.GetZf(),scaleGrowth.GetXf(),scaleGrowth.GetYf(),scaleGrowth.GetZf());
		}

		  typedef CastScalingBound CastType;
		  friend struct CastScalingBound;
	protected:
		Vec3V m_InitialScale;
		Vec3V m_ScaleGrowth;		
	};

	class IterativeCastResult
	{
	public:
		Vec3V m_Position;
		Vec3V m_NormalOnLocalBound;
		ScalarV m_IntersectionTime;
	};

	// PURPOSE:
	//   Iteratively cast the given input against the given generic convex object.
	// PARAMS:
	//   castInput - The type of cast to perform
	//   localBound - The bound in local space to cast against
	//   result - Out parameter class filled with the results of the cast
	// RETURN:
	//   True if there was an intersection, false otherwise
	// NOTES:
	//   If this returns a collision but the result intersection time is 0, it means the cast and bound start out intersecting.
	//     This function doesn't try to solve the penetration so the position and normal will not be optimal. 
	template <class CastInput> bool IterativeCast(const CastInput& castInput, const phBound& localBound, IterativeCastResult& result);
} // namespace rage

#endif // PHYSICS_ITERATIVE_CAST_H