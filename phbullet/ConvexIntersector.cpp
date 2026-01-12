
#include "ConvexIntersector.h"

#include "GjkPairDetector.h"
#include "boxBoxDistance.h"
#include "btBoxBoxDetector.h"
#include "btGjkEpaPenetrationDepthSolver.h"
#include "MinkowskiPenetrationDepthSolver.h"
#include "TrianglePenetrationDepthSolver.h"
#include "SimplexSolverInterface.h"

#include "phbound/bound.h"
#include "phbound/support.h"
#include "profile/element.h"

GJK_COLLISION_OPTIMIZE_OFF()

// NOTE: If you de-optimize this file and end up crashing on load it's probably the fault of FindClosestSegAABBToTri blowing out your stack

#if VERIFY_SUPPORT_FUNCTIONS
extern void VerifyBoundSupportFunction(const rage::phBound * bound, rage::Vec3V_In dir);
#endif // VERIFY_SUPPORT_FUNCTIONS

namespace rage {


namespace phCollisionStats
{
	EXT_PF_TIMER(GetClosestPoints);
	EXT_PF_TIMER(PenetrationSolve);
	EXT_PF_COUNTER(PenetrationSolveCount);
}

using namespace phCollisionStats;

#if __BANK
int g_NumDeepPenetrationChecks = 0;
#endif

#if USE_BOX_BOX_DISTANCE
#if __BANK
bool g_UseBoxBoxDistance = true;
#endif // __BANK
#endif

#if CAPSULE_TO_CAPSULE_DISTANCE
#if __BANK
bool g_CapsuleCapsuleDistance = true;
#endif // __BANK
#endif

#if CAPSULE_TO_TRIANGLE_DISTANCE
#if __BANK
bool g_CapsuleTriangleDistance = true;
#endif // __BANK
#endif

#if DISC_TO_TRIANGLE_DISTANCE
#if __BANK
bool g_DiscTriangleDistance = true;
#endif // __BANK
#endif

#if BOX_TO_TRIANGLE_DISTANCE
#if __BANK
// PRESERVE_SAMPLE_CONTACTS_SLIDING_BEHAVIOR -- The current code breaks the sample_contacts sliding page
//  - There is a known error with the special case box/tri algorithm where edge/edge closest points are slightly non-optimal
//    when the edges are nearly parallel and the opposite face of the box is within the vertical slab of space between the
//    third triangle vertex and the two involved in the edge contact
//  - We choose to allow this for the full game because such errors are small and the end result still forms
//    good contact patches with the same macroscopic motion - while doing so nearly twice as fast
// Sample is stable with: g_BoxTriangleDistance = false;
bool g_BoxTriangleDistance = true;
#endif // __BANK
#endif

#if USE_GJK_CACHE 
#if __BANK
bool g_UseFramePersistentGJKCache = true;
bool g_UseGJKCache = true;
#endif // __BANK
#endif // USE_GJK_CACHE 

//static const Vec3V VEC3_EPSILON(FLT_EPSILON, FLT_EPSILON, FLT_EPSILON);
//static const Vec3V VEC_EPSILON_SQUARED(FLT_EPSILON*FLT_EPSILON, FLT_EPSILON*FLT_EPSILON, FLT_EPSILON*FLT_EPSILON);

#if TRACK_COLLISION_TYPE_PAIRS
u32 g_CollisionTypePairTable[phBound::NUM_BOUND_TYPES][phBound::NUM_BOUND_TYPES];

#if __SPU
void SendTypePairTableToMainMemory(u32 mainMemoryTable[][phBound::NUM_BOUND_TYPES])
{
	for (int row = 0; row < phBound::NUM_BOUND_TYPES; ++row)
	{
		for (int column = 0; column < phBound::NUM_BOUND_TYPES; ++column)
		{
			if (g_CollisionTypePairTable[row][column] > 0)
			{
				sysInterlockedAdd(&mainMemoryTable[row][column], g_CollisionTypePairTable[row][column]);
			}
		}
	}
}
#else
static size_t CollectCollisionTypePairTable(char* output, size_t size)
{
	size_t totalSize = 0;
	output[0] = '\0';
	for (int row = 0; row < phBound::NUM_BOUND_TYPES; ++row)
	{
		CompileTimeAssert(phBound::NUM_BOUND_TYPES == 15); // If this fires, adjust the Displayf here:

		char oneLine[512];
		formatf(oneLine, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\r\n",
			g_CollisionTypePairTable[row][0],
			g_CollisionTypePairTable[row][1],
			g_CollisionTypePairTable[row][2],
			g_CollisionTypePairTable[row][3],
			g_CollisionTypePairTable[row][4],
			g_CollisionTypePairTable[row][5],
			g_CollisionTypePairTable[row][6],
			g_CollisionTypePairTable[row][7],
			g_CollisionTypePairTable[row][8],
			g_CollisionTypePairTable[row][9],
			g_CollisionTypePairTable[row][10],
			g_CollisionTypePairTable[row][11],
			g_CollisionTypePairTable[row][12],
			g_CollisionTypePairTable[row][13],
			g_CollisionTypePairTable[row][14]);

		size_t length = strlen(oneLine);
		if (!Verifyf(totalSize + length + 1 < size, "Collision pair table too big to fit into provided buffer"))
		{
			return totalSize;
		}

		strcpy(output, oneLine);
		output += length;
		totalSize += length;
	}

	return totalSize;
}

void PrintCollisionTypePairTable()
{
	const int MAX_COLLISION_TYPE_PAIR_TABLE_STRING_SIZE = 2048;
	char buffer[MAX_COLLISION_TYPE_PAIR_TABLE_STRING_SIZE];
	CollectCollisionTypePairTable(buffer, MAX_COLLISION_TYPE_PAIR_TABLE_STRING_SIZE);
	Displayf("%s",buffer);
}

void DumpCollisionTypePairTable(fiStream* stream)
{
	const int MAX_COLLISION_TYPE_PAIR_TABLE_STRING_SIZE = 2048;
	char buffer[MAX_COLLISION_TYPE_PAIR_TABLE_STRING_SIZE];
	size_t size = CollectCollisionTypePairTable(buffer, MAX_COLLISION_TYPE_PAIR_TABLE_STRING_SIZE);
	stream->Write(buffer, (int)size);
}

void ClearCollisionTypePairTable()
{
	for (int row = 0; row < phBound::NUM_BOUND_TYPES; ++row)
	{
		for (int column = 0; column < phBound::NUM_BOUND_TYPES; ++column)
		{
			g_CollisionTypePairTable[row][column] = 0;
		}
	}
}
#endif // __SPU
#endif // TRACK_COLLISION_TYPE_PAIRS

#if USE_FRAME_PERSISTENT_GJK_CACHE
	bool GetGJKCacheSeparatingDirection(const DiscreteCollisionDetectorInterface::ClosestPointInput & input, Vec3V_InOut sepAxis, ScalarV_InOut sepDist);
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
PHYSICS_FORCE_INLINE void ComputePenetrationSeparation_(const DiscreteCollisionDetectorInterface::ClosestPointInput & input, Vec3V_InOut sepAxis, ScalarV_InOut sepDist)
{
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	const phConvexIntersector::PenetrationDepthSolverType pdsType = (phConvexIntersector::PenetrationDepthSolverType)input.m_pdsType;
	FastAssert(pdsType == phConvexIntersector::PDSOLVERTYPE_TRIANGLE || pdsType == phConvexIntersector::PDSOLVERTYPE_MINKOWSKI);
#if USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
	if (pdsType == phConvexIntersector::PDSOLVERTYPE_TRIANGLE && input.m_boundA->GetUseNewBackFaceCull() == 0)
#else // USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
	if (pdsType == phConvexIntersector::PDSOLVERTYPE_TRIANGLE)
#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
	{
		TrianglePenetrationDepthSolver penetrationDepthSolver;
		penetrationDepthSolver.ComputePenetrationSeparation(input.m_boundA, input.m_boundB, input.m_transformA, input.m_transformB, sepAxis, sepDist);
	}
	else
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	{
#if USE_FRAME_PERSISTENT_GJK_CACHE
		if (!GetGJKCacheSeparatingDirection(input,sepAxis,sepDist))
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
		{
			MinkowskiPenetrationDepthSolver penetrationDepthSolver;
			penetrationDepthSolver.ComputePenetrationSeparation(input.m_boundA, input.m_boundB, input.m_transformA, input.m_transformB, sepAxis, sepDist);
		}
	}
}

bool GetClosestPointsSpecial_BoxBox
#if USE_BOX_BOX_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	const phBound *boundA = input.m_boundA;
	const phBound *boundB = input.m_boundB;

	const phBoundBox* boxA = static_cast<const phBoundBox*>(boundA);
	Vec3V halfA, centerA;
	boxA->GetBoundingBoxHalfWidthAndCenter(halfA, centerA);
	Mat34V boxMatA;
	boxMatA.Set3x3(input.m_transformA.GetMat33());
	boxMatA.SetCol3(Transform(input.m_transformA, centerA));

	const phBoundBox* boxB = static_cast<const phBoundBox*>(boundB);
	Vec3V halfB, centerB;
	boxB->GetBoundingBoxHalfWidthAndCenter(halfB, centerB);
	Mat34V boxMatB;
	boxMatB.Set3x3(input.m_transformB.GetMat33());
	boxMatB.SetCol3(Transform(input.m_transformB, centerB));

	Vec3V normal, pointA, pointB;
	ScalarV dist = boxBoxDistance(normal, pointA, pointB, halfA, boxMatA, halfB, boxMatB);

	Vec3V worldPointOnA = Transform(boxMatA, pointA);
	Vec3V worldPointOnB = Transform(boxMatB, pointB);

	Vec3V localPointOnA = UnTransformOrtho( input.m_transformA, worldPointOnA );
	Vec3V localPointOnB = UnTransformOrtho( input.m_transformB, worldPointOnB );

	output.AddContactPoint((-normal).GetIntrin128(), 
		dist.GetIntrin128(), boxA->GetIndexFromBound(), boxB->GetIndexFromBound()
		// These are filler, but this usage also shouldn't be needing these parameters later anyway 
		, localPointOnA.GetIntrin128(), localPointOnB.GetIntrin128()
#if TRACK_COLLISION_TIME
		, 0.0f
#endif
		);

	return true;

#else // USE_BOX_BOX_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput&, DiscreteCollisionDetectorInterface::SimpleResult&)
{
	return false;
#endif // USE_BOX_BOX_DISTANCE
}

bool GetClosestPointsSpecial_CapsuleCapsule
#if CAPSULE_TO_CAPSULE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	const phBoundCapsule* capsuleA = static_cast<const phBoundCapsule*>(input.m_boundA);
	const phBoundCapsule* capsuleB = static_cast<const phBoundCapsule*>(input.m_boundB);

	ScalarV marginA = capsuleA->GetMarginV();
	ScalarV marginB = capsuleB->GetMarginV();

	Vec3V capAxisA = input.m_transformA.GetCol1();
	Vec3V pointA2 = capAxisA * capsuleA->GetLengthV(marginA);
	Vec3V offset = input.m_transformA.GetCol3() - pointA2 * ScalarV(V_HALF);
	Vec3V halfLengthB = input.m_transformB.GetCol1() * capsuleB->GetHalfLengthV(marginB);
	Vec3V directionB = input.m_transformB.GetCol3();
	Vec3V pointB1 = directionB - halfLengthB - offset;
	Vec3V pointB2 = directionB + halfLengthB - offset;

	Vec3V pointA, pointB;
	geomPoints::FindClosestSegSegToSeg(pointA.GetIntrin128Ref(), pointB.GetIntrin128Ref(), pointA2.GetIntrin128(), pointB1.GetIntrin128(), pointB2.GetIntrin128());

	Vec3V delta = pointB - pointA;
	ScalarV dist = Mag(delta);

	// Turned some branches into selects - Which means these checks appear in reverse logical order
	//  - We are effectively just protecting ourselves against divide by zero for cases of touching, aligned, or even aligned and axis aligned capsules
	Vec3V tempDir = Cross(capAxisA, Vec3V(V_X_AXIS_WZERO));
	ScalarV tempDirMagSqrd = MagSquared(tempDir);
	Vec3V norm = Scale(tempDir, InvSqrt(tempDirMagSqrd));

	tempDir = Cross(capAxisA, Vec3V(V_Z_AXIS_WZERO));
	tempDirMagSqrd = MagSquared(tempDir);
	norm = SelectFT( IsGreaterThan(tempDirMagSqrd, ScalarV(V_FLT_SMALL_12)), norm, Scale(tempDir, InvSqrt(tempDirMagSqrd)) );

	tempDir = Cross(capAxisA, input.m_transformB.GetCol1());
	tempDirMagSqrd = MagSquared(tempDir);
	norm = SelectFT( IsGreaterThan(tempDirMagSqrd, ScalarV(V_FLT_SMALL_12)), norm, Scale(tempDir, InvSqrt(tempDirMagSqrd)) );

	// This last one's true result is what we would have naively done
	norm = SelectFT( IsGreaterThan(dist, ScalarV(V_FLT_SMALL_6)), norm, Scale(delta, Invert(dist)) );
	//
	Assert(0 == Vec::V3IsZeroAll(norm.GetIntrin128()));

	Vec3V worldPointOnA = Add(AddScaled(pointA, norm, marginA), offset);
	Vec3V worldPointOnB = Add(SubtractScaled(pointB, norm, marginB), offset);

	ScalarV marginDist = dist - marginA - marginB;

	Vec3V localPointOnA = UnTransformOrtho( input.m_transformA, worldPointOnA );
	Vec3V localPointOnB = UnTransformOrtho( input.m_transformB, worldPointOnB );

	output.AddContactPoint((-norm).GetIntrin128(), 
		marginDist.GetIntrin128(), capsuleA->GetIndexFromBound(), capsuleB->GetIndexFromBound()
		// These are filler, but this usage also shouldn't be needing these parameters later anyway 
		, localPointOnA.GetIntrin128(), localPointOnB.GetIntrin128()
#if TRACK_COLLISION_TIME
		, 0.0f
#endif
		);

	return true;

#else // CAPSULE_TO_CAPSULE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput&, DiscreteCollisionDetectorInterface::SimpleResult&)
{
	return false;
#endif // CAPSULE_TO_CAPSULE_DISTANCE
}

bool GetClosestPointsSpecial_CapsuleTriangle
#if CAPSULE_TO_TRIANGLE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	const phBoundCapsule* capsule = static_cast<const phBoundCapsule*>(input.m_boundA);
	const TriangleShape* triangle = static_cast<const TriangleShape*>(input.m_boundB);

	ScalarV marginCap = capsule->GetMarginV();
	ScalarV capHalfLen = capsule->GetHalfLengthV(marginCap);
	ScalarV marginTri = triangle->GetMarginV();

	Vec3V offset = input.m_transformB.GetCol3();
	Vec3V vert1 = Transform3x3(input.m_transformB, triangle->m_vertices1[1]);
	Vec3V vert2 = Transform3x3(input.m_transformB, triangle->m_vertices1[2]);

	Vec3V halfLength = Scale(input.m_transformA.GetCol1(), capHalfLen);
	Vec3V capCenter = input.m_transformA.GetCol3();
	Vec3V point1 = Subtract(Subtract(capCenter, halfLength), offset);
	Vec3V point2 = Subtract(Add(capCenter, halfLength), offset);

	Vec3V pointOnTri, pointOnSeg;
	geomPoints::FindClosestSegSegToTri(pointOnTri, pointOnSeg, vert1, vert2, point1, point2);

	Vec3V delta = Subtract(pointOnSeg, pointOnTri);
	ScalarV dist = Mag(delta);
	ScalarV marginDist = Subtract(dist, Add(marginTri, marginCap));
	ScalarV invDist = InvertSafe(dist);
	Vec3V normal = Scale(delta, invDist);

#define USE_TRIANGLE_NORMAL_TO_DETERMINE_PENETRATION 1

	// Overlapping closest points means penetration
#if USE_TRIANGLE_NORMAL_TO_DETERMINE_PENETRATION
	Vec3V triangleNorm = Transform3x3(input.m_transformB, triangle->m_PolygonNormal);
	if( IsLessThanOrEqualAll(dist, ScalarV(V_FLT_SMALL_5)) | (IsLessThanAll(Dot(delta, triangleNorm), ScalarV(V_ZERO)) & IsLessThanAll(marginDist, ScalarV(V_ZERO))) )
#else
	if(IsLessThanOrEqualAll(dist, ScalarV(V_FLT_SMALL_5)))
#endif
	{
		Vec3V offsetDir;
		ScalarV offsetDist;
		ComputePenetrationSeparation_(input,offsetDir,offsetDist);

		// Use the best axis from ComputePenetrationDepth to separate the segment and recompute new closest points
		Vec3V depenOffset = Scale(offsetDir, offsetDist);
		Vec3V offPoint1 = Add(point1, depenOffset);
		Vec3V offPoint2 = Add(point2, depenOffset);
		Vec3V offPointOnTri, offPointOnSeg;
		geomPoints::FindClosestSegSegToTriNoIntersection(offPointOnTri, offPointOnSeg, vert1, vert2, offPoint1, offPoint2);

		Vec3V offResDelta = Subtract(offPointOnSeg, offPointOnTri);
		ScalarV offResDist = Mag(offResDelta);
		Assertf(!IsEqualAll(offResDist, ScalarV(V_ZERO)), "Capsule/Tri penetration depth solver has failed. Still contacting after depenetration.");
		ScalarV offResInvDist = Invert(offResDist);
		Vec3V offResNormal = Scale(offResDelta, offResInvDist);

		// Correcting for the direction we separated the objects
		const ScalarV normalAdjustmentFactor = Dot(offsetDir, offResNormal);
		const ScalarV correctedMinPenAlongNorm = Scale(offsetDist, normalAdjustmentFactor) - offResDist;

		pointOnSeg = SubtractScaled(offPointOnTri, offResNormal, correctedMinPenAlongNorm);
		pointOnTri = offPointOnTri;

		// Use normal from separated test
		normal = offResNormal;
		// distance apart is negative because these are penetrating points
		dist = Negate(correctedMinPenAlongNorm);
		marginDist = Subtract(dist, Add(marginTri, marginCap));
	}

	Vec3V worldPointOnTri = Add(AddScaled(pointOnTri, normal, marginTri), offset);
	Vec3V worldPointOnCap = Add(SubtractScaled(pointOnSeg, normal, marginCap), offset);

	Vec3V localPointOnTri = UnTransformOrtho( input.m_transformB, worldPointOnTri );
	Vec3V localPointOnCap = UnTransformOrtho( input.m_transformA, worldPointOnCap );

	output.AddContactPoint((normal).GetIntrin128(), 
		marginDist.GetIntrin128(), capsule->GetIndexFromBound(), triangle->GetIndexFromBound()
		// These are filler, but this usage also shouldn't be needing these parameters later anyway 
		, localPointOnCap.GetIntrin128(), localPointOnTri.GetIntrin128()
#if TRACK_COLLISION_TIME
		, 0.0f
#endif
		);

	return true;

#else // CAPSULE_TO_TRIANGLE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput&, DiscreteCollisionDetectorInterface::SimpleResult&)
{
	return false;
#endif // CAPSULE_TO_TRIANGLE_DISTANCE
}

bool GetClosestPointsSpecial_DiscTriangle
#if DISC_TO_TRIANGLE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	const phBoundDisc* disc = static_cast<const phBoundDisc*>(input.m_boundA);
	const TriangleShape* triangle = static_cast<const TriangleShape*>(input.m_boundB);

	ScalarV marginDisc = disc->GetMarginV();
	ScalarV discRadius = disc->GetRadiusV(marginDisc);
	ScalarV marginTri = triangle->GetMarginV();

	Vec3V offset = input.m_transformB.GetCol3();
	Vec3V vert1 = Transform3x3(input.m_transformB, triangle->m_vertices1[1]);
	Vec3V vert2 = Transform3x3(input.m_transformB, triangle->m_vertices1[2]);

	Vec3V discNorm = Transform3x3(input.m_transformA, disc->GetAxis());
	Vec3V discCenter = input.m_transformA.GetCol3();
	Vec3V discCenterRel = Subtract(discCenter, offset);

	Vec3V pointOnTri, pointOnDisc;
	geomDistances::FindClosestSegDiscToTri(pointOnTri, pointOnDisc, vert1, vert2, discCenterRel, discNorm, discRadius);

	Vec3V delta = Subtract(pointOnDisc, pointOnTri);
	ScalarV dist = Mag(delta);
	ScalarV marginDist = Subtract(dist, Add(marginTri, marginDisc));
	ScalarV invDist = InvertSafe(dist);
	Vec3V normal = Scale(delta, invDist);

	// Overlapping closest points means penetration
	if(IsLessThanOrEqualAll(dist, ScalarV(V_FLT_SMALL_4)))
	{
		Vec3V offsetDir;
		ScalarV offsetDist;
		ComputePenetrationSeparation_(input,offsetDir,offsetDist);

		// Use the best axis from ComputePenetrationDepth to separate the segment and recompute new closest points
		Vec3V depenOffset = Scale(offsetDir, offsetDist);
		Vec3V discCenterRelOff = Add(discCenterRel, depenOffset);
		Vec3V offPointOnTri, offPointOnDisc;
		geomDistances::FindClosestSegDiscToTri(offPointOnTri, offPointOnDisc, vert1, vert2, discCenterRelOff, discNorm, discRadius);

		Vec3V offResDelta = Subtract(offPointOnDisc, offPointOnTri);
		ScalarV offResDist = Mag(offResDelta);
		Assertf(!IsEqualAll(offResDist, ScalarV(V_ZERO)), "Disc/Tri penetration depth solver has failed. Still contacting after depenetration.");
		ScalarV offResInvDist = Invert(offResDist);
		Vec3V offResNormal = Scale(offResDelta, offResInvDist);

		// Correcting for the direction we separated the objects
		const ScalarV normalAdjustmentFactor = Dot(offsetDir, offResNormal);
		const ScalarV correctedMinPenAlongNorm = Scale(offsetDist, normalAdjustmentFactor) - offResDist;

		pointOnDisc = SubtractScaled(offPointOnTri, offResNormal, correctedMinPenAlongNorm);
		pointOnTri = offPointOnTri;

		// Use normal from separated test
		normal = offResNormal;
		// distance apart is negative because these are penetrating points
		dist = Negate(correctedMinPenAlongNorm);
		marginDist = Subtract(dist, Add(marginTri, marginDisc));
	}

	Vec3V worldPointOnTri = Add(AddScaled(pointOnTri, normal, marginTri), offset);
	Vec3V worldPointOnDisc = Add(SubtractScaled(pointOnDisc, normal, marginDisc), offset);

	Vec3V localPointOnTri = UnTransformOrtho( input.m_transformB, worldPointOnTri );
	Vec3V localPointOnDisc = UnTransformOrtho( input.m_transformA, worldPointOnDisc );

	output.AddContactPoint((normal).GetIntrin128(), 
		marginDist.GetIntrin128(), disc->GetIndexFromBound(), triangle->GetIndexFromBound()
		// These are filler, but this usage also shouldn't be needing these parameters later anyway 
		, localPointOnDisc.GetIntrin128(), localPointOnTri.GetIntrin128()
#if TRACK_COLLISION_TIME
		, 0.0f
#endif
		);

	return true;

#else // DISC_TO_TRIANGLE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput&, DiscreteCollisionDetectorInterface::SimpleResult&)
{
	return false;
#endif // DISC_TO_TRIANGLE_DISTANCE
}

bool GetClosestPointsSpecial_BoxTriangle
#if BOX_TO_TRIANGLE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	const phBoundBox* box = static_cast<const phBoundBox*>(input.m_boundA);
	const TriangleShape* triangle = static_cast<const TriangleShape*>(input.m_boundB);

	ScalarV marginBox = box->GetMarginV();
	ScalarV marginTri = triangle->GetMarginV();

	Vec3V boxHalfWidth, localBoxCenter;
	box->GetBoundingBoxHalfWidthAndCenter(boxHalfWidth, localBoxCenter);
	Vec3V boxShrunkHalfWidth = Subtract(boxHalfWidth, Vec3V(marginBox));
	Vec3V boxCenter = Transform(input.m_transformA, localBoxCenter);

	Mat34V boxMat = input.m_transformA;
	boxMat.SetCol3(boxCenter);
	Mat34V triMat = input.m_transformB;

	Mat34V boxMatInv;
	InvertTransformOrtho(boxMatInv, boxMat);
	Mat34V triToBoxMat;
	Transform(triToBoxMat, boxMatInv, triMat);

	Vec3V vert0 = Transform(triToBoxMat, triangle->m_vertices1[0]);
	Vec3V vert1 = Transform(triToBoxMat, triangle->m_vertices1[1]);
	Vec3V vert2 = Transform(triToBoxMat, triangle->m_vertices1[2]);

#define USE_TRIANGLE_NORMAL_TO_DETERMINE_PENETRATION_FOR_BOX 1

	// Boolean test for true penetration first
	// - Our closestPoints function needs guaranteed disjoint box and tri, so we're forced to do this work here
	//   rather than being able to wait till the last second like with capsule and disc
	Vec3V pointOnTriWorld, pointOnBoxWorld, normal;
	ScalarV marginDist;
#if USE_TRIANGLE_NORMAL_TO_DETERMINE_PENETRATION_FOR_BOX
	// This acts like the capsule/tri test's backfacing check by forcing a penetration solve if the margin expanded box would touch the back of the triangle
	Vec3V triangleNorm = Transform3x3(triToBoxMat, triangle->m_PolygonNormal);
	Vec3V marginOffset = Scale(triangleNorm, Add(marginBox, marginTri));
	// Actually have to add a little to the boxExtents so that we don't have numerical issues with the normal later on
	Vec3V boxExtents = Add(boxShrunkHalfWidth, Vec3V(V_FLT_SMALL_5));
	if( (geomPoints::TestAABBAgainstTri(vert0, vert1, vert2, boxExtents) | geomPoints::TestAABBAgainstTriFace(vert0-marginOffset, triangleNorm, boxExtents)).Getb() )
#else
	// Actually have to add a little to the boxExtents so that we don't have numerical issues with the normal later on
	if(geomPoints::TestAABBAgainstTri(vert0, vert1, vert2, Add(boxShrunkHalfWidth, Vec3V(V_FLT_SMALL_5))).Getb())
#endif
	{
		Vec3V offsetDir;
		ScalarV offsetDist;
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER 
		TrianglePenetrationDepthSolver penetrationDepthSolver;
#else // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER 
		MinkowskiPenetrationDepthSolver penetrationDepthSolver;
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER 
		// We forced our box to be centered around our local space, but it may not have been created that way
		// Shifting by the centroid offset will account for that and let TriPenDepthSolver work properly
		Mat34V boxToOffsetBox(V_IDENTITY);
		boxToOffsetBox.SetCol3(-box->GetCentroidOffset());
		penetrationDepthSolver.ComputePenetrationSeparation(box, triangle, boxToOffsetBox, triToBoxMat, offsetDir, offsetDist);

		// Use the best axis from ComputePenetrationDepth to separate the segment and recompute new closest points
		Vec3V depenOffset = Scale(offsetDir, offsetDist);
		// TriPenDepthSolver returns the way the second object should go to move out, need to negate since we're going to move the triangle
		Vec3V triPenOffset = Negate(depenOffset);

		// Shift the triangle verts
		vert0 = Add(vert0, triPenOffset);
		vert1 = Add(vert1, triPenOffset);
		vert2 = Add(vert2, triPenOffset);

		Vec3V pointOnTri, pointOnBox;
		geomPoints::FindClosestSegAABBToTri(pointOnTri, pointOnBox, vert0, vert1, vert2, boxShrunkHalfWidth);

		Vec3V delta = Subtract(pointOnBox, pointOnTri);
		ScalarV dist = Mag(delta);
		Assertf(!IsEqualAll(dist, ScalarV(V_ZERO)), "Box/Tri penetration depth solver has failed!");
		ScalarV invDist = Invert(dist);
		normal = Scale(delta, invDist);

		// You may notice some reversal of direction or the Add rather than subtract here (vs the versions in other special functions)
		//  - This is because here we moved the triangle instead of the other object but the normal always points out from the triangle
		const ScalarV correctedMinPenAlongNorm = Dot(depenOffset, normal) - dist;
		pointOnTri = AddScaled(pointOnBox, normal, correctedMinPenAlongNorm);

		// distance apart is negative because these are penetrating points
		dist = Negate(correctedMinPenAlongNorm);
		marginDist = Subtract(dist, Add(marginTri, marginBox));

		// Convert back to world space
		pointOnTriWorld = Transform(boxMat, pointOnTri);
		pointOnBoxWorld = Transform(boxMat, pointOnBox);
		// Note that since we already computed a normal, that needs transforming back out too
		// - However normals need to be transformed by the transpose of the inverse
		// -- Unless you can guarantee uniform scaling (Which we probably can do here, but it's easy enough to use the inverse we computed earlier)
		Mat34V boxMatInvTrans;
		Transpose3x3(boxMatInvTrans, boxMatInv);
		normal = Transform3x3(boxMatInvTrans, normal);
	}
	else
	{
		Vec3V pointOnTri, pointOnBox;
		geomPoints::FindClosestSegAABBToTri(pointOnTri, pointOnBox, vert0, vert1, vert2, boxShrunkHalfWidth);

		// Convert back to world space
		pointOnTriWorld = Transform(boxMat, pointOnTri);
		pointOnBoxWorld = Transform(boxMat, pointOnBox);

		Vec3V delta = Subtract(pointOnBoxWorld, pointOnTriWorld);
		ScalarV dist = Mag(delta);
		Assertf(!IsEqualAll(dist, ScalarV(V_ZERO)), "Box/Tri penetration depth detection has failed!");
		marginDist = Subtract(dist, Add(marginTri, marginBox));
		ScalarV invDist = Invert(dist);
		normal = Scale(delta, invDist);
	}

	Vec3V worldPointOnTri = AddScaled(pointOnTriWorld, normal, marginTri);
	Vec3V worldPointOnBox = SubtractScaled(pointOnBoxWorld, normal, marginBox);

	Vec3V localPointOnTri = UnTransformOrtho( input.m_transformB, worldPointOnTri );
	Vec3V localPointOnBox = UnTransformOrtho( input.m_transformA, worldPointOnBox );

	output.AddContactPoint((normal).GetIntrin128(), 
		marginDist.GetIntrin128(), box->GetIndexFromBound(), triangle->GetIndexFromBound()
		// These are filler, but this usage also shouldn't be needing these parameters later anyway 
		, localPointOnBox.GetIntrin128(), localPointOnTri.GetIntrin128()
#if TRACK_COLLISION_TIME
		, 0.0f
#endif
		);

	return true;

#else // BOX_TO_TRIANGLE_DISTANCE
(const DiscreteCollisionDetectorInterface::ClosestPointInput&, DiscreteCollisionDetectorInterface::SimpleResult&)
{
	return false;
#endif // BOX_TO_TRIANGLE_DISTANCE
}

#if !DISABLE_SPECIAL_CASE_COLLISION
bool phConvexIntersector::GetClosestPointsSpecialCase(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	const phBound *boundA = input.m_boundA;
	const phBound *boundB = input.m_boundB;

	const int typeShift = 4;
	CompileTimeAssert(phBound::NUM_BOUND_TYPES < (1 << typeShift));
	int testType = (boundA->GetType() << typeShift) | boundB->GetType();
	switch(testType)
	{
#if USE_BOX_BOX_DISTANCE
	case (phBound::BOX << typeShift) | phBound::BOX:
		{
#if __BANK
			const bool useBoxBoxDistance = g_UseBoxBoxDistance;
#else
			const bool useBoxBoxDistance = true;
#endif
			if(useBoxBoxDistance)
			{
				return GetClosestPointsSpecial_BoxBox(input, output);
			}
#if __BANK
			break;
#endif
		}
#endif // USE_BOX_BOX_DISTANCE

#if CAPSULE_TO_CAPSULE_DISTANCE
	case (phBound::CAPSULE << typeShift) | phBound::CAPSULE:
		{
#if __BANK
			const bool useFastCapsuleToCapsule = g_CapsuleCapsuleDistance;
#else
			const bool useFastCapsuleToCapsule = true;
#endif
			if(useFastCapsuleToCapsule)
			{
				return GetClosestPointsSpecial_CapsuleCapsule(input, output);
			}
#if __BANK
			break;
#endif
		}
#endif // CAPSULE_TO_CAPSULE_DISTANCE

#if CAPSULE_TO_TRIANGLE_DISTANCE
	case (phBound::CAPSULE << typeShift) | phBound::TRIANGLE:
		{
#if __BANK
			const bool useFastCapsuleToTriangle = g_CapsuleTriangleDistance;
#else
			const bool useFastCapsuleToTriangle = true;
#endif
			if(useFastCapsuleToTriangle)
			{
				return GetClosestPointsSpecial_CapsuleTriangle(input, output);
			}
#if __BANK
			break;
#endif
		}
#endif // CAPSULE_TO_TRIANGLE_DISTANCE

#if DISC_TO_TRIANGLE_DISTANCE
	case (phBound::DISC << typeShift) | phBound::TRIANGLE:
		{
#if __BANK
			const bool useFastDiscToTriangle = g_DiscTriangleDistance;
#else
			const bool useFastDiscToTriangle = true;
#endif
			if(useFastDiscToTriangle)
			{
				return GetClosestPointsSpecial_DiscTriangle(input, output);
			}
#if __BANK
			break;
#endif
		}
#endif // DISC_TO_TRIANGLE_DISTANCE

#if BOX_TO_TRIANGLE_DISTANCE
	case (phBound::BOX << typeShift) | phBound::TRIANGLE:
		{
#if __BANK
			const bool useFastBoxToTriangle = g_BoxTriangleDistance;
#else
			const bool useFastBoxToTriangle = true;
#endif
			if(useFastBoxToTriangle)
			{
				return GetClosestPointsSpecial_BoxTriangle(input output);
			}
#if __BANK
			break;
#endif
		}
#endif // BOX_TO_TRIANGLE_DISTANCE
	}

	return false;
}
#endif // !DISABLE_SPECIAL_CASE_COLLISION

void phConvexIntersector::GjkPenetrationSolve(const DiscreteCollisionDetectorInterface::ClosestPointInput * CCD_RESTRICT gjkInput, DiscreteCollisionDetectorInterface::ClosestPointOutput * CCD_RESTRICT gjkOutput, GjkPairDetector * CCD_RESTRICT gjkdet)
{
	FastAssert(gjkOutput->isSeparatedBeyondSepThresh == false);
	FastAssert(gjkOutput->m_needsPenetrationSolve == true);
	const phBound *boundA = gjkInput->m_boundA;
	const phBound *boundB = gjkInput->m_boundB;

	{
		const Mat34V transA = gjkInput->m_transformA;
		const Mat34V transB = gjkInput->m_transformB;

		//penetration case
		//if there is no way to handle penetrations, bail out
		{
			PF_START(PenetrationSolve);
			PF_INCREMENT(PenetrationSolveCount);
#if __BANK
			g_NumDeepPenetrationChecks++;
#endif // __BANK

			// Penetration depth case.
			Vec3V offsetDir;
			ScalarV offsetDist;

			ComputePenetrationSeparation_(*gjkInput,offsetDir,offsetDist);
#define USE_RETURNED_GJK_NORMAL_AS_SEP_TEST_DIRECTION 0
#if USE_RETURNED_GJK_NORMAL_AS_SEP_TEST_DIRECTION
			if (gjkOutput->isValid)
			{
				// Use the returned GJK normal as a test direction.

				const Vec3V separationOffset = transA.GetCol3() - transB.GetCol3();

				const Vec3V norm = gjkOutput->worldNormalBtoA;
				const Vec3V seperatingAxisInA = UnTransform3x3Ortho( transA, -norm );
				const Vec3V seperatingAxisInB = UnTransform3x3Ortho( transB, norm );

				const Vec3V pInA = boundA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
				const Vec3V qInB = boundB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
				const Vec3V pWorld = Transform3x3(transA, pInA);
				const Vec3V qWorld = Transform3x3(transB, qInB);

				const Vec3V w = (qWorld - pWorld) - separationOffset;
				const ScalarV delta = Dot(norm, w);
				const ScalarV dist = Max(delta + PENETRATION_CHECK_EXTRA_MARGIN_SV, ScalarV(V_ZERO));
				
				const BoolV isLessThan = IsLessThan(dist, offsetDist);
				offsetDist = SelectFT(isLessThan, offsetDist, dist);
				offsetDir = SelectFT(isLessThan, offsetDir, norm);
			}
#endif // USE_RETURNED_GJK_NORMAL_AS_SEP_TEST_DIRECTION

#define USE_ITERATIVE_PENETRATION_SOLVER (1 && USE_NEW_SIMPLEX_SOLVER)
#if USE_ITERATIVE_PENETRATION_SOLVER
			//const Vec3V offset = Scale(offsetDir, offsetDist);
			GjkPairDetector::ClosestPointInput cpInput(ScalarV(V_FLT_MAX),boundA,boundB,PDSOLVERTYPE_NONE,gjkInput->m_gjkCache);
			cpInput.m_transformA.Set3x3(transA);
			cpInput.m_transformB.Set3x3(transB);
			//cpInput.m_transformA.SetCol3(transA.GetCol3() + offset);
			cpInput.m_transformB.SetCol3(transB.GetCol3());
			const int MAX_ITERS = 3;
			DiscreteCollisionDetectorInterface::ClosestPointOutput gjkOutput1;
			// Use 5 degree threshold.
			//const float co_5 = cosf(5.0f * 3.1415926f / 180.0f);
			const ScalarV v_co_5(ScalarVFromF32(0.996194f));
			const ScalarV v_zero(V_ZERO);
			const Vec3V separationOffset = transA.GetCol3() - transB.GetCol3();
			for (int iter = 0 ; iter < MAX_ITERS ; iter++)
			{
				const Vec3V offset = Scale(offsetDir, offsetDist);
				cpInput.m_transformA.SetCol3(transA.GetCol3() + offset);
				gjkdet->DoGjkIterations(cpInput,true,false,&gjkOutput1);
				FastAssert(gjkOutput1.isSeparatedBeyondSepThresh == false);
#if USE_NEW_SIMPLEX_SOLVER
				FastAssert(gjkOutput1.m_needsPenetrationSolve == false);
				FastAssert(gjkOutput1.isValid == true);
#else // USE_NEW_SIMPLEX_SOLVER
				if (gjkOutput1.isValid == false)
					break;
#endif // USE_NEW_SIMPLEX_SOLVER
				const Vec3V axis = gjkOutput1.worldNormalBtoA;
				const ScalarV dotp = Dot(axis,offsetDir);
				if (IsLessThanAll(dotp,v_co_5))
				{
					// Use the newly computed normal to separate the objects again.
					const Vec3V seperatingAxisInA = UnTransform3x3Ortho(transA,-axis);
					const Vec3V seperatingAxisInB = UnTransform3x3Ortho(transB,axis);

#if VERIFY_SUPPORT_FUNCTIONS
					VerifyBoundSupportFunction(boundA,seperatingAxisInA);
					VerifyBoundSupportFunction(boundB,seperatingAxisInB);
#endif // VERIFY_SUPPORT_FUNCTIONS

					const Vec3V pInA = boundA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
					const Vec3V qInB = boundB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
					const Vec3V pWorld = Transform3x3(transA, pInA);
					const Vec3V qWorld = Transform3x3(transB, qInB);

					const Vec3V w = (qWorld - pWorld) - separationOffset;
					const ScalarV dist_ = PENETRATION_CHECK_EXTRA_MARGIN_SV + Dot(axis, w);
					const ScalarV dist = Max(dist_,v_zero);

					offsetDir = axis;
					offsetDist = dist;
				}
				else
					break;
			}
#else // USE_ITERATIVE_PENETRATION_SOLVER
			const Vec3V offset = Scale(offsetDir, offsetDist);
			GjkPairDetector::ClosestPointInput cpInput(ScalarV(V_FLT_MAX),boundA,boundB,PDSOLVERTYPE_NONE,gjkInput->m_gjkCache);
			cpInput.m_transformA.Set3x3(transA);
			cpInput.m_transformB.Set3x3(transB);
			cpInput.m_transformA.SetCol3(transA.GetCol3() + offset);
			cpInput.m_transformB.SetCol3(transB.GetCol3());

			DiscreteCollisionDetectorInterface::ClosestPointOutput gjkOutput1;
			gjkdet->DoGjkIterations(cpInput,true,false,&gjkOutput1);
			FastAssert(gjkOutput1.isSeparatedBeyondSepThresh == false);
#endif // USE_ITERATIVE_PENETRATION_SOLVER
#if USE_NEW_SIMPLEX_SOLVER
			FastAssert(gjkOutput1.m_needsPenetrationSolve == false);
			FastAssert(gjkOutput1.isValid == true);
#else // USE_NEW_SIMPLEX_SOLVER
			if (gjkOutput1.isValid)
#endif // !USE_NEW_SIMPLEX_SOLVER
			{
				const ScalarV normalAdjustmentFactor = Dot(offsetDir, gjkOutput1.worldNormalBtoA);
				const ScalarV correctedMinPenAlongNorm = Scale(offsetDist, normalAdjustmentFactor) - gjkOutput1.separationWithMargin;

				// Get the collision normal on object A, which is the displacement from the collision point on object A to the collision point on object B.
				// With overlap, this points into object A.
				const ScalarV separation = -correctedMinPenAlongNorm;

				// See if this separation is smaller than any previous valid separation.
#if !USE_NEW_SIMPLEX_SOLVER
				const bool isValid = gjkOutput->isValid;
				const ScalarV separationWithMargin = gjkOutput->separationWithMargin;
				if(!isValid || IsLessThanAll(separation, separationWithMargin))
#endif // !USE_NEW_SIMPLEX_SOLVER
				{
					// This separation is smaller than any previous valid separation, so use it.
					// Update gjkOutput with the new results.
					gjkOutput->separationWithMargin = separation;
					gjkOutput->worldNormalBtoA = gjkOutput1.worldNormalBtoA;
					gjkOutput->isValid = true;
				}

			}

			PF_STOP(PenetrationSolve);
		}
	}
}

bool phConvexIntersector::GjkClosestPoints(const DiscreteCollisionDetectorInterface::ClosestPointInput * CCD_RESTRICT gjkInput, DiscreteCollisionDetectorInterface::ClosestPointOutput * CCD_RESTRICT gjkOutput, GjkPairDetector * CCD_RESTRICT gjkdet)
{
#if !USE_NEW_SIMPLEX_SOLVER
	FastAssert(IsGreaterThanOrEqualAll(gjkInput->m_sepThreshold,DEGENERATE_PENETRATION_LIMIT) != 0);
#endif // !USE_NEW_SIMPLEX_SOLVER
	gjkdet->DoGjkIterations(*gjkInput,false,false,gjkOutput);
	if (gjkOutput->m_needsPenetrationSolve)
	{
		GjkPenetrationSolve(gjkInput,gjkOutput,gjkdet);
		return true;
	}
	else
		return false;
}

void phConvexIntersector::WriteSimpleResult(const DiscreteCollisionDetectorInterface::ClosestPointInput * CCD_RESTRICT gjkInput, const DiscreteCollisionDetectorInterface::ClosestPointOutput * CCD_RESTRICT gjkOutput, const GjkPairDetector * CCD_RESTRICT gjkdet, DiscreteCollisionDetectorInterface::SimpleResult * CCD_RESTRICT output)
{
	FastAssert(gjkOutput->isValid);
	FastAssert(gjkOutput->isSeparatedBeyondSepThresh == false);
	{
		// Declare and initialize local parameters.
		const Vec3V worldNormalBtoA = gjkOutput->worldNormalBtoA;
		const ScalarV separationWithMargin = gjkOutput->separationWithMargin;

		const Mat34V transA = gjkInput->m_transformA;
		const Mat34V transB = gjkInput->m_transformB;

		const phBound *boundA = gjkInput->m_boundA;
		const phBound *boundB = gjkInput->m_boundB;

		// TODO: change this to only calculate local normals when needed. Inside FindElementIndex, the only shape that needs it is phBound::GEOMETRY.
		const Vec3V localNormalFromA = UnTransform3x3Ortho( transA, -worldNormalBtoA );
		const Vec3V localNormalFromB = UnTransform3x3Ortho( transB, worldNormalBtoA );

		Vec3V localPointOnA;
		Vec3V localPointOnB;
		gjkdet->GetLocalPoints(&localPointOnA,&localPointOnB);
		const Vec3V marginA = Vec3V( boundA->GetMarginV() );
		const Vec3V marginB = Vec3V( boundB->GetMarginV() );
		localPointOnA += Scale(marginA,localNormalFromA);
		localPointOnB += Scale(marginB,localNormalFromB);

		const int elementA = gjkdet->m_simplexSolver.FindElementIndexP(VEC3V_TO_VECTOR3(localNormalFromA), VEC3V_TO_VECTOR3(localPointOnA), boundA);
		const int elementB = gjkdet->m_simplexSolver.FindElementIndexQ(VEC3V_TO_VECTOR3(localNormalFromB), VEC3V_TO_VECTOR3(localPointOnB), boundB);

		// Add the contact point to the manifold. This expects the world normal on B and the contact depth, but instead it gets the world normal on A and separation, which is equivalent.
		output->AddContactPoint(worldNormalBtoA.GetIntrin128(), 
			separationWithMargin.GetIntrin128(), elementA, elementB
			// These are filler, but this usage also shouldn't be needing these parameters later anyway 
			, localPointOnA.GetIntrin128(), localPointOnB.GetIntrin128()
			TRACK_COLLISION_TIME_PARAM(0.0f));
	} 
}

void phConvexIntersector::GetClosestPoints(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output)
{
	PF_FUNC(GetClosestPoints);
	FastAssert(&input != NULL);	// This assert doesn't make sense.

#if TRACK_COLLISION_TYPE_PAIRS
	const phBound *boundA = input.m_boundA;
	const phBound *boundB = input.m_boundB;
#if __SPU
	g_CollisionTypePairTable[boundA->GetType()][boundB->GetType()]++;
#else // __SPU
	sysInterlockedIncrement(&g_CollisionTypePairTable[boundA->GetType()][boundB->GetType()]);
#endif // __SPU
#endif // TRACK_COLLISION_TYPE_PAIRS

#if USE_BOX_BOX_INTERSECT
	if(boundA->GetType() == phBound::BOX && boundB->GetType() == phBound::BOX)
	{
		btBoxBoxDetector boxBoxDetector(static_cast<const phBoundBox*>(boundA), static_cast<const phBoundBox*>(boundB));
		boxBoxDetector.getClosestPoints(input, output);
		return;
	}
#endif

#if !DISABLE_SPECIAL_CASE_COLLISION
	if (GetClosestPointsSpecialCase(input,output))
		return;
#endif // !DISABLE_SPECIAL_CASE_COLLISION

	//////////////////////////////////////////////////////////////////////////
	GjkPairDetector gjk;
	DiscreteCollisionDetectorInterface::ClosestPointOutput gjkOutput;

/*	
	gjk.DoGjkIterations(input, &gjkOutput);

	GjkPenetrationSolve(input,gjkOutput,gjk);
*/
	GjkClosestPoints(&input,&gjkOutput,&gjk);

	if (gjkOutput.isValid && !gjkOutput.isSeparatedBeyondSepThresh)
		WriteSimpleResult(&input,&gjkOutput,&gjk,&output);
}


}	// namespace rage
