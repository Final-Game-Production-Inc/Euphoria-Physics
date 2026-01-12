//
// phcore/phmath.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_PHMATH_H
#define PHCORE_PHMATH_H

#include "vector/geometry.h"
#include "vector/matrix34.h"

namespace rage {

class phPolygon;

#define	DEFAULT_TOLERANCE	0.001f	// for a float to be near zero

namespace phLinearEq {};
namespace phMotionCtrl {};
//namespace phMathInertia {}; will replace class phMathInertia

bool CanSlerp(QuatV_In q1, QuatV_In q2);
QuatV_Out SlerpSafe(ScalarV_In t, QuatV_In q1, QuatV_In q2, QuatV_In errVal);

Vec3V_Out ComputeAxisAnglePrecise(QuatV_In q);
Vec3V_Out ComputeAxisAnglePrecise(Mat33V_In m0, Mat33V_In m1);

// to be moved to vector/geometry in namespace geomTValues
int FindTValuesLineToCircle(const Vector3& point1, const Vector3& point1to2, const Vector3& normal,
								const Vector3& center, float radius, float* edget1, Vector3* segtocircle1,
								float* distance1, float* edget2, Vector3* segtocircle2, float* distance2);

// to be moved to pheffects/forceliquid
void ReOrderVerts(Vector3* threeVertices, float* threeFloats=NULL);
bool VertOrderIsOK(const Vector3* threeVertices);

// to be moved to vector/angmath
void AngleToMatrix(const Vector3& a, Matrix34* m);

// to be moved to vector/matrix34
Matrix34 CreateRotatedMatrix(const Vector3& location, const Vector3& rotation);
void RotateMatrix(Matrix34* matrix, const Vector3& rotation);
void CreateAxiallySymmetricMatrix(const Vector3& endA, const Vector3& endB, Matrix34* mtx, Vector3* axis, float* length);

// to be placed in namespace phLinearEq
void RankDescending (const float* list, int* rankList, int numInList);
bool SolveLinear1D (float sumA, float coefX, float coefY, float sumB, float* solutionX, float* solutionY);
bool SolveLinear1D (float a0, float b0, float c0, float a1, float b1, float c1, float& x, float& y);
bool SolveLinear1D (float sumA, const float* coef, int* dummy, int numTerms, float sumB, float* solution);
bool SolveLinear1DRecurse (float sumA, const float* coef, const int* termIndices, int numTerms, float sumB, float* solution);
bool SolveLinear2D (float sumA, float coefXB, float coefYB, float coefZB, float sumB, float coefXC, float coefYC, float coefZC,
					float sumC, float* solutionX, float* solutionY, float* solutionZ);
bool SolveLinear2D (float sumA, const float* coefB, float sumB, const float* coefC, float sumC, float* dummyB, float* dummyC,
					int* dummyInt, float* dummyFl0, float* dummyFl1, int numTerms, float* solution);
bool SolveLinear2DRecurse (float sumA, float* coefB, float sumB, float* coefC, float sumC, float* angles, float* distances,
							int numTerms, float* solution, float centroidB, float centroidC, bool balancing, float biggestDist,
							bool verify=false);
bool CombineSolveLinear2DTerms (float angleT, float angleDiff, float dist0, float dist1, int combineTerm0, int combineTerm1,
								float* coefB, float* coefC, int numTerms, float biggestDist, float& tZeroToOne,
								float& combineCoefB, float& combineCoefC);
bool SolveLinear (const float* coefficients, float sum, int numTerms, float* unknowns,
					const Vector3* elements=NULL, Vector3* elementSum=NULL);

__forceinline Vec::V3Return128 SolveWithFriction(Vec::V3Param128 impulse, Vec::V3Param128 frictionPlaneProjection, Vec::V3Param128 frictionMu)
{
	// f2.x is r from Sam's friction white-paper
	Vec3V f2;
	Vec3V f = Vec3V(impulse);
	Vec3V _zero = Vec3V(V_ZERO);
	Vec3V _1000 = Vec3V(V_X_AXIS_WZERO);
	Vec3V _0110 = Vec3V( Vec::V4VConstant<U32_ZERO,U32_ONE,U32_ONE,U32_ZERO>() );

	f2 = Vec3V( Dot(Vec3V(frictionPlaneProjection), f) );

	f2 = Scale( f2, _1000 );

	// check complementarity condition.  We can push, but not pull
	// FastAssert( v.x * f2.x >= 0.0f )
	//	if( f2.x >= 0.0f )
	{
		// check to see if we are within the friction cone.  i.e. static friction
		//		if (f.x>=0.0f && m_mu.x*m_mu.x*square(f.x)>=square(f.y) + square(f.z))
		{
			// nothing to do, use the force calculated for complete constraint resistance
		}
		//		else
		{
			// need to project f back into the friction cone.  project a ray from f to f2, find the intersection on the cone surface.

			// we'll get the normal of the cone surface at the point g, where f->f2 pierces the cone
			Vec3V gn;

			// project onto collision plane
			gn = Scale( f, _0110 );

			// now we need to rotate down by the cone angle.  convieniently enough, tan(theta) = friction_coef = y/x of a right angle triangle

			Vec3V xyLen = Vec3V( Mag(gn) );

			Vec3V hLen = Scale( xyLen, Vec3V(frictionMu) );

			gn = SubtractScaled( gn, _1000, hLen );

			Vec3V hit = Vec3V( Dot( f, gn ) );

			Vec3V seg = Subtract( f, f2 );

			const Vec3V devZeroGuard = Vec3V(V_FLT_SMALL_6);

			Vec3V total = Vec3V( Dot(seg, gn) ) + devZeroGuard;

			Vec3V ratio = InvScale(hit, total);

			VecBoolV insideFrictionCone = IsGreaterThan( hit, _zero );

			ratio = SelectFT( insideFrictionCone, _zero, ratio );

			f = SubtractScaled( f, seg, ratio );

			f2 = Vec3V( SplatX(f2) );
			VecBoolV isSeparating = IsLessThan( f2, _zero );

			f = SelectFT( isSeparating, f, _zero );

			return f.GetIntrin128();

		}

	}
	//		else
	//		{
	//			f.Zero();
	//		}
}

// to be placed in namespace phMotionCtrl
float FindHomingAccel(float time, float currentPos, float currentSpeed, float targetPos=0.0f, float targetSpeed=0.0f);
Vector3 FindHomingAccel3D(float time, const Vector3& currentPos, const Vector3& currentVel,
							const Vector3& targetPos=Vector3(0.0f,0.0f,0.0f),
							const Vector3& targetVel=Vector3(0.0f,0.0f,0.0f));
Vector3 FindHomingAngAccel(const Vector3 &angle, const Vector3 &angVel, float time, float invTimeStep);
Vector3 FindHomingAngAccel3D(float time, float invTimeStep, const Matrix34& currentMtx, const Vector3&currentAngVel,
								const Matrix34& targetMtx=M34_IDENTITY, const Vector3& targetAngVel=ORIGIN);
Vector3 FindHomingAngAccel3D(float time, float invTimeStep, const Quaternion& currentQ, const Vector3& currentAngVel,
								const Quaternion& targetQ, const Vector3& targetAngVel=ORIGIN);

////////////////////////////////////////////////////////////////
// phMathInertia
//
// Functions for calculating the mass and angular inertia of
// various rigid body shapes.
//

namespace phMathInertia 
{

////////////////////////////////////////////////////////////
// angular inertia untility functions

// PURPOSE: Use a non-diagonal, symmetric inertia matrix defined by the first six arguments to find the principal axes.
// PARAMS:
//	iXX - the diagonal x component of the angular inertia matrix
//	iYY - the diagonal y component of the angular inertia matrix
//	iZZ - the diagonal z component of the angular inertia matrix
//	iXY - the off-diagonal xy component of the angular inertia matrix
//	iXZ - the off-diagonal xz component of the angular inertia matrix
//	iYZ - the off-diagonal yz component of the angular inertia matrix
//	angInertia - reference into which to put the new diagonal angular inertia matrix components
//	colliderMtx - reference into which to put the new collider matrix (same as the principal axes)
//	rotation - the matrix with which to rotate the current collider matrix to get the new collider matrix
void FindPrincipalAxes (float iXX, float iYY, float iZZ, float iXY, float iXZ, float iYZ, Vector3& angInertia, Matrix34& colliderMtx, Matrix34& rotation);

Vector3 PrincipalAxis (const Vector3& angInertia, float cubicRoot, float iXY, float iXZ, float iYZ);

// PURPOSE: Find the total mass and angular inertia from the addition of the given mass and angular inertia to the given previous total mass and angular inertia.
// PARAMS:
//	mass - the new mass to add
//	angInertia - the new angular inertia to add
//	location - the position of the added object
//	colliderMtx - the position and orientation of the previous total object
//	totalMass - the previous mass of the object, also used to write the new total
//	totalAngInertia - the previous angular inertia of the object, also used to write the new total
//	rotation - optional pointer in which to fill in the matrix needed to rotate the previous orientation to get the new principal axes
// NOTES:	This is used to assemble a composite object out of individual parts. Each part has an orientation aligned with the angular inertia's principal axes,
//			which means the angular inertia can be represented by a vector instead of a matrix. By assembling the composite object and changing its orientation,
//			the composite can have orientation axes aligned with its angular inertia principal axes.
void AddInertia (float mass, const Vector3& angInertia, const Vector3& location, Matrix34& colliderMtx, float& totalMass, Vector3& totalAngInertia, Matrix34* rotation=NULL);

// PURPOSE: Find the total mass and angular inertia from the addition of the given mass and angular inertia to the given previous total mass and angular inertia.
// PARAMS:
//	mass - the new mass to add
//	angInertia - the new angular inertia to add
//	mtx - the position and orientation of the added object
//	colliderMtx - the position and orientation of the previous total object
//	totalMass - the previous mass of the object, also used to write the new total
//	totalAngInertia - the previous angular inertia of the object, also used to write the new total
//	rotation - optional pointer in which to fill in the matrix needed to rotate the previous orientation to get the new principal axes
// NOTES:	This is used to assemble a composite object out of individual parts. Each part has an orientation aligned with the angular inertia's principal axes,
//			which means the angular inertia can be represented by a vector instead of a matrix. By assembling the composite object and changing its orientation,
//			the composite can have orientation axes aligned with its angular inertia principal axes.
void AddInertia (float mass, const Vector3& angInertia, const Matrix34& mtx, Matrix34& colliderMtx, float& totalMass, Vector3& totalAngInertia, Matrix34* rotation=NULL);

void ClampAngInertia (Vector3& angInertia);

void GetInverseInertiaMatrix (Mat33V_In colliderMatrix, Vec::V3Param128 invAngInertia, Mat33V_InOut invInertia);

// PURPOSE: Rotate an angular inertia into a different space
// PARAMS:
//   rotation - rotation matrix from the current space to the new space
//   angularInertia - the angular inertia to rotate
// RETURN: 
//   the angular inertia in the new space
__forceinline Vec3V_Out RotateAngularInertia(Mat33V_In rotation, Vec3V_In angularInertia)
{
	Mat33V absoluteRotation;
	Abs(absoluteRotation, rotation);
	return Multiply(absoluteRotation, angularInertia);
}

// PURPOSE: Compute the angular inertia caused by translating from the center of mass
// PARAMS:
//   offsetFromCenterOfGravity - the offset from the center of gravity
//   mass - the mass of the object that is being translated
// RETURN:
//   the angular inertia caused by translating the given mass by the given offset
__forceinline Vec3V_Out ComputeTranslationAngularInertia(Vec3V_In offsetFromCenterOfGravity, ScalarV_In mass)
{
	Vec3V offsetSquared = Scale(offsetFromCenterOfGravity, offsetFromCenterOfGravity);
	return Scale(Add(offsetSquared.Get<Vec::Z,Vec::X,Vec::Y>(),offsetSquared.Get<Vec::Y,Vec::Z,Vec::X>()), mass);
}


// PURPOSE: Accumulate mass properties of all the objects in the object arrays
class MassAccumulatorSimple
{
public:
	__forceinline void ZeroMass() { m_Mass = ScalarV(V_ZERO); }
	__forceinline void ZeroAngularInertia() { m_AngularInertia = Vec3V(V_ZERO); }
	__forceinline void ZeroCenterOfGravity() { m_CenterOfGravity = Vec3V(V_ZERO); }

	__forceinline bool IncludeObject(int UNUSED_PARAM(objectIndex)) { return true; }

	__forceinline void ScaleCenterOfGravityByMass() { m_CenterOfGravity = InvScaleSafe(m_CenterOfGravity, m_Mass, Vec3V(V_ZERO)); }
	__forceinline void AddMass(int UNUSED_PARAM(objectIndex), ScalarV_In mass) { m_Mass = Add(m_Mass,mass); }
	__forceinline void AddAngularInertia(int UNUSED_PARAM(objectIndex), Vec3V_In angularInertia) {m_AngularInertia = Add(m_AngularInertia, angularInertia); }
	__forceinline void AddMassAndCenterOfGravity(int UNUSED_PARAM(objectIndex), ScalarV_In mass, Vec3V_In centerOfGravity) { m_Mass = Add(m_Mass,mass); m_CenterOfGravity = AddScaled(m_CenterOfGravity,centerOfGravity,mass); }

	__forceinline ScalarV_Out GetFullMass(int UNUSED_PARAM(objectIndex)) { return m_Mass; }
	__forceinline Vec3V_Out GetFullAngularInertia(int UNUSED_PARAM(objectIndex)) { return m_AngularInertia; }
	__forceinline Vec3V_Out GetFullCenterOfGravity(int UNUSED_PARAM(objectIndex)) { return m_CenterOfGravity; }

	__forceinline ScalarV_Out GetMass() { return m_Mass; }
	__forceinline Vec3V_Out GetAngularInertia() { return m_AngularInertia; }
	__forceinline Vec3V_Out GetCenterOfGravity() { return m_CenterOfGravity; }

	__forceinline void SetMass(ScalarV_In mass) { m_Mass = mass; }
	__forceinline void SetAngularInertia(Vec3V_In angularInertia) { m_AngularInertia = angularInertia; }
	__forceinline void SetCenterOfGravity(Vec3V_In centerOfGravity) { m_CenterOfGravity = centerOfGravity; }

private:
	ScalarV m_Mass; 
	Vec3V m_AngularInertia;
	Vec3V m_CenterOfGravity;
};

// PURPOSE: Accumulate mass properties of the object's whose corresponding bit is set in the given bitset
template <typename BitSet>
class MassAccumulatorBitSet : public MassAccumulatorSimple
{
public:
	MassAccumulatorBitSet(const BitSet& includeObjects) : m_IncludeObjects(includeObjects) {}
	MassAccumulatorBitSet(const MassAccumulatorBitSet& other) : m_IncludeObjects(other.m_IncludeObjects) {}

	__forceinline bool IncludeObject(int objectIndex) { return m_IncludeObjects.IsSet(objectIndex); }

	__forceinline void AddMass(int objectIndex, ScalarV_In mass) { if(IncludeObject(objectIndex)) MassAccumulatorSimple::AddMass(objectIndex,mass); }
	__forceinline void AddAngularInertia(int objectIndex, Vec3V_In angularInertia) { if(IncludeObject(objectIndex)) MassAccumulatorSimple::AddAngularInertia(objectIndex,angularInertia); }
	__forceinline void AddMassAndCenterOfGravity(int objectIndex, ScalarV_In mass, Vec3V_In centerOfGravity) { if(IncludeObject(objectIndex)) MassAccumulatorSimple::AddMassAndCenterOfGravity(objectIndex,mass,centerOfGravity); }
private:
	const BitSet& m_IncludeObjects;
};

// PURPOSE: Accumulate mass properties of the object's whose corresponding bit is clear in the given bitset
template <typename BitSet>
class MassAccumulatorNotBitSet : public MassAccumulatorSimple
{
public:
	MassAccumulatorNotBitSet(const BitSet& dontIncludeObjects) : m_DontIncludeObjects(dontIncludeObjects) {}
	MassAccumulatorNotBitSet(const MassAccumulatorNotBitSet& other) : m_DontIncludeObjects(other.m_DontIncludeObjects) {}

	__forceinline bool IncludeObject(int objectIndex) { return m_DontIncludeObjects.IsClear(objectIndex); }

	__forceinline void AddMass(int objectIndex, ScalarV_In mass) { if(IncludeObject(objectIndex)) MassAccumulatorSimple::AddMass(objectIndex,mass); }
	__forceinline void AddAngularInertia(int objectIndex, Vec3V_In angularInertia) { if(IncludeObject(objectIndex)) MassAccumulatorSimple::AddAngularInertia(objectIndex,angularInertia); }
	__forceinline void AddMassAndCenterOfGravity(int objectIndex, ScalarV_In mass, Vec3V_In centerOfGravity) { if(IncludeObject(objectIndex)) MassAccumulatorSimple::AddMassAndCenterOfGravity(objectIndex,mass,centerOfGravity); }
private:
	const BitSet& m_DontIncludeObjects;
};

// PURPOSE: Accumulate mass properties of all objects, grouping the properties together based on the given map. 
template <typename IndexType>
class MassAccumulatorMapper
{
public:
	MassAccumulatorMapper(IndexType numGroups, IndexType* objectIndexToGroupIndex, ScalarV_Ptr masses, Vec3V_Ptr angularInertias, Vec3V_Ptr centersOfGravity) :
		m_NumGroups(numGroups), 
		m_ObjectIndexToGroupIndex(objectIndexToGroupIndex),
		m_Masses(masses),
		m_AngularInertias(angularInertias),
		m_CentersOfGravity(centersOfGravity) {}
	__forceinline void ZeroMass() { memset(m_Masses,0,sizeof(ScalarV)*m_NumGroups); }
	__forceinline void ZeroAngularInertia() { memset(m_AngularInertias,0,sizeof(Vec3V)*m_NumGroups);  }
	__forceinline void ZeroCenterOfGravity() { memset(m_CentersOfGravity,0,sizeof(Vec3V)*m_NumGroups);  }

	__forceinline bool IncludeObject(int UNUSED_PARAM(objectIndex)) { return true; }
	__forceinline IndexType GetGroupIndex(int objectIndex) { return m_ObjectIndexToGroupIndex[objectIndex]; }

	__forceinline void ScaleCenterOfGravityByMass()
	{ 
		for(IndexType groupIndex = 0; groupIndex < m_NumGroups; ++groupIndex)
		{
			m_CentersOfGravity[groupIndex] = InvScaleSafe(m_CentersOfGravity[groupIndex], m_Masses[groupIndex], Vec3V(V_ZERO));
		}
	}
	__forceinline void AddMass(int objectIndex, ScalarV_In mass)
	{ 
		IndexType groupIndex = GetGroupIndex(objectIndex);
		m_Masses[groupIndex] = Add(m_Masses[groupIndex],mass);
	}
	__forceinline void AddAngularInertia(int objectIndex, Vec3V_In angularInertia)
	{
		IndexType groupIndex = GetGroupIndex(objectIndex);
		m_AngularInertias[groupIndex] = Add(m_AngularInertias[groupIndex],angularInertia);
	}
	__forceinline void AddMassAndCenterOfGravity(int objectIndex, ScalarV_In mass, Vec3V_In centerOfGravity)
	{ 
		IndexType groupIndex = GetGroupIndex(objectIndex);
		m_Masses[groupIndex] = Add(m_Masses[groupIndex],mass);
		m_CentersOfGravity[groupIndex] = AddScaled(m_CentersOfGravity[groupIndex],centerOfGravity,mass);
	}

	__forceinline ScalarV_Out GetFullMass(int objectIndex) { return GetGroupMass(GetGroupIndex(objectIndex)); }
	__forceinline Vec3V_Out GetFullAngularInertia(int objectIndex) { return GetGroupAngularInertia(GetGroupIndex(objectIndex)); }
	__forceinline Vec3V_Out GetFullCenterOfGravity(int objectIndex) { return GetGroupCenterOfGravity(GetGroupIndex(objectIndex)); }

	__forceinline ScalarV_Out GetGroupMass(IndexType groupIndex) { return m_Masses[groupIndex]; }
	__forceinline Vec3V_Out GetGroupAngularInertia(IndexType groupIndex) { return m_AngularInertias[groupIndex]; }
	__forceinline Vec3V_Out GetGroupCenterOfGravity(IndexType groupIndex) { return m_CentersOfGravity[groupIndex]; }

	__forceinline void SetGroupMass(IndexType groupIndex, Vec3V_In mass) { m_Masses[groupIndex] = mass; }
	__forceinline void SetGroupAngularInertia(IndexType groupIndex, Vec3V_In angularInertia) { m_AngularInertias[groupIndex] = angularInertia; }
	__forceinline void SetGroupCenterOfGravity(IndexType groupIndex, Vec3V_In centerOfGravity) { m_CentersOfGravity[groupIndex] = centerOfGravity; }

private:
	ScalarV_Ptr m_Masses; 
	Vec3V_Ptr m_AngularInertias;
	Vec3V_Ptr m_CentersOfGravity;
	IndexType m_NumGroups;
	IndexType* m_ObjectIndexToGroupIndex;
};

// PURPOSE: Accumulate mass properties of all objects, grouping the properties together based on the given map. Only touches objects whose corresponding bit is set in the given bitset.
template <typename IndexType, typename BitSet>
class MassAccumulatorMapperBitSet : public MassAccumulatorMapper<IndexType>
{
public:
	MassAccumulatorMapperBitSet(IndexType numGroups, IndexType* objectIndexToGroupIndex, ScalarV_Ptr masses, Vec3V_Ptr angularInertias, Vec3V_Ptr centersOfGravity, const BitSet& includeObjects) :
		MassAccumulatorMapper<IndexType>(numGroups,objectIndexToGroupIndex,masses,angularInertias,centersOfGravity),
		m_IncludeObjects(includeObjects) {}

	__forceinline bool IncludeObject(int objectIndex) { return m_IncludeObjects.IsSet(objectIndex); }

	__forceinline void AddMass(int objectIndex, ScalarV_In mass) { if(IncludeObject(objectIndex)) AddMass(objectIndex,mass); }
	__forceinline void AddAngularInertia(int objectIndex, Vec3V_In angularInertia) { if(IncludeObject(objectIndex)) AddAngularInertia(objectIndex,angularInertia); }
	__forceinline void AddMassAndCenterOfGravity(int objectIndex, ScalarV_In mass, Vec3V_In centerOfGravity) { if(IncludeObject(objectIndex)) AddMassAndCenterOfGravity(objectIndex,mass,centerOfGravity); }

private:
	const BitSet& m_IncludeObjects;
};

// PURPOSE: Accumulate mass properties of all objects, grouping the properties together based on the given map. Only touches objects whose corresponding bit is clear in the given bitset.
template <typename IndexType, typename BitSet>
class MassAccumulatorMapperNotBitSet : public MassAccumulatorMapper<IndexType>
{
public:
	MassAccumulatorMapperNotBitSet(IndexType numGroups, IndexType* objectIndexToGroupIndex, ScalarV_Ptr masses, Vec3V_Ptr angularInertias, Vec3V_Ptr centersOfGravity, const BitSet& dontIncludeObjects) :
		MassAccumulatorMapper<IndexType>(numGroups,objectIndexToGroupIndex,masses,angularInertias,centersOfGravity),
		m_DontIncludeObjects(dontIncludeObjects) {}

	__forceinline bool IncludeObject(int objectIndex) { return m_DontIncludeObjects.IsClear(objectIndex); }

	__forceinline void AddMass(int objectIndex, ScalarV_In mass) { if(IncludeObject(objectIndex)) AddMass(objectIndex,mass); }
	__forceinline void AddAngularInertia(int objectIndex, Vec3V_In angularInertia) { if(IncludeObject(objectIndex)) AddAngularInertia(objectIndex,angularInertia); }
	__forceinline void AddMassAndCenterOfGravity(int objectIndex, ScalarV_In mass, Vec3V_In centerOfGravity) { if(IncludeObject(objectIndex)) AddMassAndCenterOfGravity(objectIndex,mass,centerOfGravity); }

private:
	const BitSet& m_DontIncludeObjects;
};

// PURPOSE: Compute the total mass of the objects
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   massAccumulator - the mass(es) of this mass accumulator will be correct after this function call
template <typename MassAccumulator>
void ComputeCombinedMass(int numObjects, ScalarV_ConstPtr objectMasses, MassAccumulator& massAccumulator)
{
	massAccumulator.ZeroMass();
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		massAccumulator.AddMass(objectIndex,objectMasses[objectIndex]);
	}
}

// PURPOSE: Compute the total mass and center of gravity of the objects
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectCentersOfGravity - the centers of gravity of the objects
//   massAccumulator - the mass(es) and center(s) of gravity of this mass accumulator will be correct after this function call
template <typename MassAccumulator>
void ComputeCombinedMassAndCenterOfGravity(int numObjects, ScalarV_ConstPtr objectMasses, Vec3V_ConstPtr objectCentersOfGravity, MassAccumulator& massAccumulator)
{
	massAccumulator.ZeroMass();
	massAccumulator.ZeroCenterOfGravity();
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		massAccumulator.AddMassAndCenterOfGravity(objectIndex,objectMasses[objectIndex],objectCentersOfGravity[objectIndex]);
	}
	massAccumulator.ScaleCenterOfGravityByMass();
}


// PURPOSE: Compute the total untranslated angular inertia of the objects (this doesn't include the angular inertia from being offset from the center of mass)
// PARAMS:
//   numObjects - the number of objects
//   objectAngularInertias - the angular inertias of the objects
//   massAccumulator - the angular inertia(s) of this mass accumulator will be correct (not accounting for translational angular inertia) after this function call
template <typename MassAccumulator>
void ComputeCombinedUntranslatedAngularInertia(int numObjects, Vec3V_ConstPtr objectAngularInertias, MassAccumulator& massAccumulator)
{
	massAccumulator.ZeroAngularInertia();
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		massAccumulator.AddAngularInertia(objectIndex, objectAngularInertias[objectIndex]);
	}
}
// PURPOSE: Compute the total translated angular inertia of the objects (this only includes angular inertia coming from being offset from the total center of gravity)
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectCentersOfGravity - the centers of gravity of the objects
//   massAccumulator - the angular inertia(s) of this mass accumulator will be correct (not accounting for untranslated angular inertia) after this function call
//                     the center(s) of gravity must be set prior to calling this function
template <typename MassAccumulator>
void ComputeCombinedTranslatedAngularInertia(int numObjects, ScalarV_ConstPtr objectMasses, Vec3V_ConstPtr objectCentersOfGravity, MassAccumulator& massAccumulator)
{
	massAccumulator.ZeroAngularInertia();
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		Vec3V objectTranslationalAngularInertia = ComputeTranslationAngularInertia(Subtract(massAccumulator.GetFullCenterOfGravity(objectIndex),objectCentersOfGravity[objectIndex]),objectMasses[objectIndex]);
		massAccumulator.AddAngularInertia(objectIndex, objectTranslationalAngularInertia);
	}
}
// PURPOSE: Compute the total angular inertia of the objects
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectAngularInertias - the angular inertias of the objects
//   objectCentersOfGravity - the centers of gravity of the objects
//   massAccumulator - the angular inertia(s) of this mass accumulator will be correct after this function call
//                     the center(s) of gravity must be set prior to calling this function
template <typename MassAccumulator>
void ComputeCombinedAngularInertia(int numObjects, ScalarV_ConstPtr objectMasses, Vec3V_ConstPtr objectAngularInertias, Vec3V_ConstPtr objectCentersOfGravity, MassAccumulator& massAccumulator)
{
	massAccumulator.ZeroAngularInertia();
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		Vec3V objectTranslationalAngularInertia = ComputeTranslationAngularInertia(Subtract(massAccumulator.GetFullCenterOfGravity(objectIndex),objectCentersOfGravity[objectIndex]),objectMasses[objectIndex]);
		massAccumulator.AddAngularInertia(objectIndex, Add(objectAngularInertias[objectIndex], objectTranslationalAngularInertia));
	}
}
// PURPOSE: Compute the total mass, angular inertia, and centers of gravity of the objects
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectAngularInertias - the angular inertias of the objects
//   objectCentersOfGravity - the centers of gravity of the objects
//   massAccumulator - the mass(es), angular inertia(s), and center(s) of gravity of this mass accumulator will be correct after this function call
template <typename MassAccumulator>
void ComputeCombinedMassAngularInertiaAndCenterOfGravity(int numObjects, ScalarV_ConstPtr objectMasses, Vec3V_ConstPtr objectAngularInertias, Vec3V_ConstPtr objectCentersOfGravity, MassAccumulator& massAccumulator)
{
	ComputeCombinedMassAndCenterOfGravity(numObjects, objectMasses, objectCentersOfGravity, massAccumulator);
	ComputeCombinedAngularInertia(numObjects, objectMasses, objectAngularInertias, objectCentersOfGravity, massAccumulator);
}

// PURPOSE: Scale the masses of the objects so their sum is equal to the given sum
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   targetCombinedMass - the sum of the masses after the function is called
//   massAccumulator - this is used only for determining if we need to include objects
// NOTE:
//   This only works on mass accumulators derived from MassAccumulatorSimple, we might need a new mass scaler templated class if we want
//	 to scale other types
template <typename MassAccumulator>
void ScaleMassesToTargetSum(int numObjects, ScalarV_Ptr objectMasses, ScalarV_In targetCombinedMass, const MassAccumulator& massAccumulator)
{
	Assertf(IsGreaterThanAll(targetCombinedMass,ScalarV(V_FLT_SMALL_12)),"Trying to set extremely small or negative combined mass (%f).",targetCombinedMass.Getf());
	MassAccumulator tempMassAccumulator(massAccumulator);
	ComputeCombinedMass(numObjects,objectMasses,tempMassAccumulator);
	const ScalarV massScale = InvScaleSafe(targetCombinedMass,tempMassAccumulator.GetMass(),ScalarV(V_ZERO));
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		if(tempMassAccumulator.IncludeObject(objectIndex))
		{
			objectMasses[objectIndex] = Scale(objectMasses[objectIndex],massScale);
		}
	}
}

// PURPOSE: Scale the angular inertias of the objects (based around a custom center of gravity) so their sum is equal to the given sum
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectAngularInertias - the angular inertias of the objects
//   objectCentersOfGravity - the centers of gravity of the objects
//   combinedCenterOfGravity - (OPTIONAL PARAMTER) a custom center of gravity, if not supplied we will calculate it
//   targetCombinedAngularInertia - the total angular inertia of the objects after the function is called
//   massAccumulator - this is used only for determining if we need to include objects
// RETURN: true if the target angular inertia was acceptable and we scaled the object angular inertias, false if we scaled the angular inertias
//           as low as possible but didn't reach the desired amount. 
// NOTE:
//   This only works on mass accumulators derived from MassAccumulatorSimple, we might need a new mass scaler templated class if we want
//		to scale other types. 
template <typename MassAccumulator>
BoolV_Out ScaleAngularInertiasToTargetSum(int numObjects, ScalarV_ConstPtr objectMasses, Vec3V_Ptr objectAngularInertias, Vec3V_ConstPtr objectCentersOfGravity, Vec3V_In combinedCenterOfGravity, Vec3V_In targetCombinedAngularInertia, const MassAccumulator& massAccumulator)
{
	MassAccumulator tempMassAccumulator(massAccumulator);
	tempMassAccumulator.SetCenterOfGravity(combinedCenterOfGravity);

	// Compute the translated angular inertia, this is the number we aren't allowed to scale since it's controlled solely by
	//   the mass and center of gravity of the objects. 
	ComputeCombinedTranslatedAngularInertia(numObjects,objectMasses,objectCentersOfGravity,tempMassAccumulator);
	const Vec3V currentCombinedTranslatedAngularInertia = tempMassAccumulator.GetAngularInertia();
	

	// Compute the untranslated angular inertia, this is the only sum we can change
	ComputeCombinedUntranslatedAngularInertia(numObjects,objectAngularInertias,tempMassAccumulator);
	const Vec3V currentCombinedUntranslatedAngularInertia = tempMassAccumulator.GetAngularInertia();

	// If the target angular inertia is less than the unscalable translated angular inertia we can't scale the angular inertia to the user's desired sum
	// Instead we will go as low as possible by zeroing out the angular inertia in the dimensions we can't reach our goal. 
	const VecBoolV isTargetAngularInertiaValid = IsGreaterThan(targetCombinedAngularInertia,currentCombinedTranslatedAngularInertia);
	const Vec3V targetCombinedUntranslatedAngularInertia = Max(Subtract(targetCombinedAngularInertia,currentCombinedTranslatedAngularInertia),Vec3V(V_ZERO));
	const Vec3V untranslatedAngularInertiaScale = InvScaleSafe(targetCombinedUntranslatedAngularInertia,currentCombinedUntranslatedAngularInertia,Vec3V(V_ZERO));
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		if(tempMassAccumulator.IncludeObject(objectIndex))
		{
			objectAngularInertias[objectIndex] = Scale(objectAngularInertias[objectIndex],untranslatedAngularInertiaScale);
		}
	}

	return isTargetAngularInertiaValid.GetX() & isTargetAngularInertiaValid.GetY() & isTargetAngularInertiaValid.GetZ();
}
template <typename MassAccumulator>
BoolV_Out ScaleAngularInertiasToTargetSum(int numObjects, ScalarV_ConstPtr objectMasses, Vec3V_Ptr objectAngularInertias, Vec3V_ConstPtr objectCentersOfGravity, Vec3V_In targetCombinedAngularInertia, const MassAccumulator& massAccumulator)
{
	MassAccumulator tempMassAccumulator(massAccumulator);
	ComputeCombinedMassAndCenterOfGravity(numObjects,objectMasses,objectCentersOfGravity,tempMassAccumulator);
	return ScaleAngularInertiasToTargetSum(numObjects,objectMasses,objectAngularInertias,objectCentersOfGravity,tempMassAccumulator.GetCenterOfGravity(),targetCombinedAngularInertia,tempMassAccumulator);
}

// PURPOSE: Scale the masses and angular inertias of the objects so their totals are equal to the given totals
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectAngularInertias - the angular inertias of the objects
//   objectCentersOfGravity - the centers of gravity of the objects
//   combinedCenterOfGravity - (OPTIONAL PARAMTER) a custom center of gravity, if not supplied we will calculate it
//   targetCombinedMass - the total mass of the objects after the function is called
//   targetCombinedAngularInertia - the total angular inertia of the objects after the function is called
//   massAccumulator - this is used only for determining if we need to include objects
// RETURN: true if the target angular inertia was acceptable and we scaled the object angular inertias, false if we scaled the angular inertias
//           as low as possible but didn't reach the desired amount.  
// NOTE:
//   This only works on mass accumulators derived from MassAccumulatorSimple, we might need a new mass scaler templated class if we want
//		to scale other types. 
template <typename MassAccumulator>
BoolV_Out ScaleMassesAndAngularInertiasToTargetSum(int numObjects, ScalarV_Ptr objectMasses, Vec3V_Ptr objectAngularInertias, Vec3V_ConstPtr objectCentersOfGravity, Vec3V_In combinedCenterOfGravity, ScalarV_In targetCombinedMass, Vec3V_In targetCombinedAngularInertia, const MassAccumulator& massAccumulator)
{
	ScaleMassesToTargetSum(numObjects,objectMasses,targetCombinedMass,massAccumulator);
	return ScaleAngularInertiasToTargetSum(numObjects,objectMasses,objectAngularInertias,objectCentersOfGravity,combinedCenterOfGravity,targetCombinedAngularInertia,massAccumulator);
}
template <typename MassAccumulator>
BoolV_Out ScaleMassesAndAngularInertiasToTargetSum(int numObjects, ScalarV_Ptr objectMasses, Vec3V_Ptr objectAngularInertias, Vec3V_ConstPtr objectCentersOfGravity, ScalarV_In targetCombinedMass, Vec3V_In targetCombinedAngularInertia, const MassAccumulator& massAccumulator)
{
	ScaleMassesToTargetSum(numObjects,objectMasses,targetCombinedMass,massAccumulator);
	return ScaleAngularInertiasToTargetSum(numObjects,objectMasses,objectAngularInertias,objectCentersOfGravity,targetCombinedAngularInertia,massAccumulator);
}

// PURPOSE: Scale the masses so their sum is equal to the given total, scale angular inertia by the same ammount. 
// PARAMS:
//   numObjects - the number of objects
//   objectMasses - the masses of the objects
//   objectAngularInertias - the angular inertias of the objects
//   targetCombinedMass - the total mass of the objects after the function is called
//   massAccumulator - this is used only for determining if we need to include objects
// NOTE:
//   This only works on mass accumulators derived from MassAccumulatorSimple, we might need a new mass scaler templated class if we want
//		to scale other types. 
template <typename MassAccumulator>
void ScaleMassesAndAngularInertiasToTargetSum(int numObjects, ScalarV_Ptr objectMasses, Vec3V_Ptr objectAngularInertias, ScalarV_In targetCombinedMass, const MassAccumulator& massAccumulator)
{
	MassAccumulator tempMassAccumulator(massAccumulator);
	ComputeCombinedMass(numObjects,objectMasses,tempMassAccumulator);
	const ScalarV massScale = InvScaleSafe(targetCombinedMass,tempMassAccumulator.GetMass(),ScalarV(V_ZERO));
	for(int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
	{
		if(tempMassAccumulator.IncludeObject(objectIndex))
		{
			objectMasses[objectIndex] = Scale(objectMasses[objectIndex],massScale);
			objectAngularInertias[objectIndex] = Scale(objectAngularInertias[objectIndex],massScale);
		}
	}
}
////////////////////////////////////////////////////////////
// calculate mass and angular inertia of various shapes

// *** sphere ***

// PURPOSE: Compute the mass of a sphere from its density and dimensions.
// PARAMS:
//	density - the density of the sphere in kg/cubic meters (water density = 1000)
//	radius - the radius of the sphere in meters
float FindSphereMass (float density, float radius);

// PURPOSE: Compute the angular inertia of a sphere from its density and dimensions.
// PARAMS:
//	density - the density of the sphere in kg/cubic meters (water density = 1000)
//	radius - the radius of the sphere in meters
//	angInertia - vector pointer into which to put the angular inertia
void FindSphereAngInertia (float mass, float radius, Vector3& angInertia);

// PURPOSE: Compute the mass and angular inertia of a sphere from its density and dimensions.
// PARAMS:
//	density - the density of the sphere in kg/cubic meters (water density = 1000)
//	radius - the radius of the sphere in meters
//	mass - pointer into which to put the mass
//	angInertia - vector pointer into which to put the angular inertia
void FindSphereMassAngInertia (float density, float radius, float& mass, Vector3& angInertia);

// *** capsule ***

// PURPOSE: Compute the mass of a capsule from its density and dimensions.
// PARAMS:
//	density - the density of the capsule in kg/cubic meters (water density = 1000)
//	radius - the radius of the capsule in meters
//	length - the distance between the capsule's hemisphere centers in meters (not the total length)
// NOTES:
//	The surface of a capsule is all points equidistant from a line segment. The total length is length + twice radius.
float FindCapsuleMass (float density, float radius, float length);

// PURPOSE: Compute the angular inertia of a capsule from its density and dimensions.
// PARAMS:
//	density - the density of the capsule in kg/cubic meters (water density = 1000)
//	radius - the radius of the capsule in meters
//	length - the distance between the capsule's hemisphere centers in meters (not the total length)
//	angInertia - vector pointer into which to put the angular inertia
// NOTES:
//	The surface of a capsule is all points equidistant from a line segment. The total length is length + twice radius.
void FindCapsuleAngInertia (float mass, float radius, float length, Vector3* angInertia);

// PURPOSE: Compute the mass and angular inertia of a capsule from its density and dimensions.
// PARAMS:
//	density - the density of the capsule in kg/cubic meters (water density = 1000)
//	radius - the radius of the capsule in meters
//	length - the distance between the capsule's hemisphere centers in meters (not the total length)
//	mass - pointer into which to put the mass
//	angInertia - vector pointer into which to put the angular inertia
// NOTES:
//	The surface of a capsule is all points equidistant from a line segment. The total length is length + twice radius.
void FindCapsuleMassAngInertia (float density, float radius, float length, float* mass, Vector3* angInertia);

// *** cylinder ***

// mass of a cylinder from density and dimensions
float FindCylinderMass (float density, float radius, float length);

// angular inertia of a cylinder from density and dimensions
void FindCylinderAngInertia (float mass, float radius, float length, Vector3* angInertia);

// mass and angular inertia of a cylinder from density and dimensions
void FindCylinderMassAngInertia (float density, float radius, float length, float* mass, Vector3* angInertia);

// *** box ***

// PURPOSE: Compute the mass of a box from its density and dimensions.
// PARAMS:
//	density - the density of the box in kg/cubic meters (water density = 1000)
//	sizeX - the extent of the box along x
//	sizeY - the extent of the box along y
//	sizeZ - the extent of the box along z
float FindBoxMass (float density, float sizeX, float sizeY, float sizeZ);

// PURPOSE: Compute the angular inertia of a box from its density and dimensions.
// PARAMS:
//	density - the density of the box in kg/cubic meters (water density = 1000)
//	sizeX - the extent of the box along x
//	sizeY - the extent of the box along y
//	sizeZ - the extent of the box along z
//	angInertia - vector pointer into which to put the angular inertia
void FindBoxAngInertia (float mass, float sizeX, float sizeY, float sizeZ, Vector3* angInertia);

// PURPOSE: Compute the mass and angular inertia of a box from its density and dimensions.
// PARAMS:
//	density - the density of the box in kg/cubic meters (water density = 1000)
//	sizeX - the extent of the box along x
//	sizeY - the extent of the box along y
//	sizeZ - the extent of the box along z
//	mass - pointer into which to put the mass
//	angInertia - vector pointer into which to put the angular inertia
void FindBoxMassAngInertia (float density, float sizeX, float sizeY, float sizeZ, float* mass, Vector3* angInertia);

// *** geometry **

// angular inertia of a polyhedron
void FindGeomAngInertia (float mass, const Vector3* vertex, const phPolygon* polygon, int numPolygons, const Vector3& cgOffset,
							Vector3& angInertia, Matrix34* rotation=NULL, Matrix34* colliderMatrix=NULL);

// mass and angular inertia of a polyhedron
bool FindGeomMassAngInertia (float density, const Vector3* vertex, const phPolygon* polygon, int numPolygons, const Vector3& cgOffset,
								float& mass, Vector3& angInertia, Matrix34* rotation=NULL, Matrix34* colliderMatrix=NULL);


// PURPOSE: Find the angular inertia matrix (6 floats, since it's symmetric) about the origin for the tetrahedron made by three vertex locations and the origin.
// PARAMS:
//	threeVerts - array of three vertex locations
//	height - the distance of the triangle made by the three vertex locations from the origin along the triangle normal
//	iXX - output for the x-x component of the angular inertia matrix
//	iYY - output for the y-y component of the angular inertia matrix
//	iZZ - output for the z-z component of the angular inertia matrix
//	iXY - output for the x-y and y-x components of the angular inertia matrix
//	iXZ - output for the x-z and z-x components of the angular inertia matrix
//	iYZ - output for the y-z and z-y components of the angular inertia matrix
void ComputeTetrahedronAngInertia (Vector3* threeVerts, float heignt, float* iXX, float* iYY, float* iZZ, float* iXY, float* iXZ, float* iYZ);

// PURPOSE: Find the angular inertia matrix (6 floats, since it's symmetric) about the origin for the triangle made by three vertex locations.
// PARAMS:
//	threeVerts - array of three vertex locations
//	iXX - output for the x-x component of the angular inertia matrix
//	iYY - output for the y-y component of the angular inertia matrix
//	iZZ - output for the z-z component of the angular inertia matrix
//	iXY - output for the x-y and y-x components of the angular inertia matrix
//	iXZ - output for the x-z and z-x components of the angular inertia matrix
//	iYZ - output for the y-z and z-y components of the angular inertia matrix
void ComputeTriangleAngInertia (Vector3* threeVerts, float* iXX, float* iYY, float* iZZ, float* iXY, float* iXZ, float* iYZ);



inline float FindSphereMass(float density, float radius)
{
	return 1.33333f*PI*square(radius)*radius*density*1000.0f;
}


inline void FindSphereAngInertia(float mass, float radius, Vector3& angInertia)
{
	angInertia.Set(0.4f*mass*square(radius));
}


inline void FindSphereMassAngInertia(float density, float radius, float& mass, Vector3& angInertia)
{
	mass = FindSphereMass(density,radius);
	FindSphereAngInertia(mass,radius,angInertia);
}

inline float FindCapsuleMass(float density, float radius, float length)
{
	return (1.3333333f*radius+length)*PI*square(radius)*density*1000.0f;
}


inline void FindCapsuleAngInertia(float mass, float radius, float length, Vector3* angInertia)
{
	float inverse=1.0f/(length+1.33333333333333333333f*radius);
	float radius2=square(radius);
	float radialMom=0.5f*mass*radius2*(length+1.0666666666667f*radius)*inverse;
	float length2=square(length);
	float centralMom=0.33333333333333333333f*mass*(0.25f*length*length2+radius*length2+
		2.25f*length*radius2+1.6f*radius*radius2)*inverse;
	angInertia->Set(centralMom,radialMom,centralMom);
}


inline void FindCapsuleMassAngInertia(float density, float radius, float length, float* mass, Vector3* angInertia)
{
	*mass=FindCapsuleMass(density,radius,length);
	FindCapsuleAngInertia(*mass,radius,length,angInertia);
}

inline float FindBoxMass(float density, float sizeX, float sizeY, float sizeZ)
{
	return sizeX*sizeY*sizeZ*density*1000.0f;
}


inline void FindBoxAngInertia(float mass, float sizeX, float sizeY, float sizeZ, Vector3* angInertia)
{
	float iX=mass*(square(sizeY)+square(sizeZ))/12.0f;
	float iY=mass*(square(sizeX)+square(sizeZ))/12.0f;
	float iZ=mass*(square(sizeX)+square(sizeY))/12.0f;
	angInertia->Set(iX,iY,iZ);
}


inline void FindBoxMassAngInertia(float density, float sizeX, float sizeY, float sizeZ, float* mass, Vector3* angInertia)
{
	*mass=FindBoxMass(density,sizeX,sizeY,sizeZ);
	FindBoxAngInertia(*mass,sizeX,sizeY,sizeZ,angInertia);
}

} // namespace phMathInertia

} // namespace rage

#endif // end of #ifndef PHCORE_PHMATH_H
