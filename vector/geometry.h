//
// vector/geometry.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_GEOMETRY_H
#define VECTOR_GEOMETRY_H

#include "matrix33.h"
#include "matrix34.h"
#include "vector4.h"

#include "math/amath.h"
#include "math/constants.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"



namespace rage {



class Vector3;

// Return values for the FindImpacts...() functions.
enum { GEOM_VERTEX, GEOM_EDGE, GEOM_POLYGON, GEOM_NO_IMPACT, GEOM_END_IMPACT };

//=============================================================================
// @@geomVectors
//
// PURPOSE geomVector is a collection of geometric functions that act on Vectors.
// NOTES
//   - Some functions here could be members of Vector3 but were moved out since
//     they didn't exactly respect the "vector" abstraction of Vector3.  For 
//     example, Clamp, Min, and Max, really treat the vector as a collection of
//     three independent floats and do not preserve the direction of the vector.
//     [ignore this nonsense ^^]
// <FLAG Component>
//
namespace geomVectors
{
	Vec3V_Out GetOrthogonal (Vec3V_In u);
	inline Vec3V_Out ComputeLeanAngles (Mat34V_In parentPose, Vec3V_In leanAxis, ScalarV_In leanAngle, ScalarV_In twistAngle);

	// NOTE
	//   It looks like the matrices that are passed in here are transposes of 'normal' matrices - beware!
	inline Mat44V_Out ComputeLeanAndTwistAngles (Mat34V_In parentWorld, Mat34V_In childWorld, Mat34V_In jointWorld, Mat34V_In jointInChild, bool twistAroundChildAxis=false);

	// PURPOSE: Convert three joint angles (lean1, lean2 and twist) to a single rotation quaternion.
	// PARAMS:
	//	lean1Angle - the angle of lean in the first direction perpendicular to the joint axis (the local X direction).
	//	lean2Angle - the angle of lean in the second direction perpendicular to the joint axis (the local Y direction).
	//	twistAngle - the angle of rotation about the twist axis
	// RETURN:	a quaternion representing the given lean and twist rotations
	inline QuatV_Out LeanAndTwistToQuatV (ScalarV_In lean1Angle, ScalarV_In lean2Angle, ScalarV_In twistAngle);

	// PURPOSE: Convert the given rotation quaternion into a joint's lean and twist angles.
	// PARAMS:
	//	q - a rotation quaternion for a joint
	// RETURN:	a vector containing a joint's lean1, lean2 and twist angles with the same rotation as the given quaternion
	// NOTES:	The output vector is (lean1, lean2, twist).
	inline Vec3V_Out QuatVToLeanAndTwist (QuatV_In q);
}


//=============================================================================
// @@geomSpheres
//
// PURPOSE: geomSpheres is a collection of geometric functions using spherical shapes.
// <FLAG Component>
//
namespace geomSpheres
{
	// PURPOSE: Calculate the impact between a polygon and a sphere.
	// PARAMS:
	//	sphereCenter -	the center of the sphere to test
	//	sphereRadius -	the radius of the sphere to test
	//	vertices -		the vertices of the polygon to test
	//	numVertices -	the number of vertices in the polygon
	//	polyNormal -	the unit normal vector of the polygon
	//	spherePosition -an output parameter, where the impact point on the sphere is stored
	//	polyPosition -	an output parameter, where the impact point on the polygon is stored
	//	idNum -			an output parameter, where the index of the vertex/edge is stored
	//	normal -		an output parameter, where the normal of the intersection is stored
	// RETURN: the type of intersection (GEOM_NO_IMPACT, GEOM_EDGE, GEOM_POLYGON, GEOM_VERTEX, or GEOM_END_IMPACT)
	// NOTES: The output normal points toward the center of the sphere.
	int FindImpactPolygonToSphere (const Vector3& sphereCenter, float sphereRadius, const Vector3* vertices,
									int numVertices, const Vector3& polyNormal, Vector3& spherePosition,
									  Vector3& polyPosition, int& idNum, Vector3& normal, float& depth);

	// PURPOSE: Determine whether a sphere intersects a polygon.
	// PARAMS:
	//	center -	the center of the sphere
	//	radius -	the radius of the sphere
	//	numverts -	the number of vertices in the polygon
	//	verts -		an array of pointers to Vector3 objects, representing the vertices of the polygon
	// RETURN:	true if the sphere intersects the polygon, false otherwise
	bool TestSphereToPolygon (const Vector3& center, float radius, int numVerts, const Vector3** verts);

	// PURPOSE: Determine whether a sphere intersects a polygon.
	// PARAMS:
	//	center -		the center of the sphere
	//	radius -		the radius of the sphere
	//	numverts -		the number of vertices in the polygon
	//	verts -			an array of pointers to Vector3 objects, representing the vertices of the polygon
	//	polygonNormal -	the polygon's normal.
	// RETURN:	true if the sphere intersects the polygon, false otherwise
	bool TestSphereToPolygon (const Vector3& center, float radius, int numVerts, const Vector3** verts,
								const Vector3& polygonNormal);

	// PURPOSE: Determine whether a sphere intersects a triangle.
	// PARAMS:
	//	sphere -		the center of the sphere (xyz) and radius (w)
	//	triangle -		an array of 3 Vec3V objects, representing the vertices of the triangle
	//	triangleNormal -the triangle's normal.
	// RETURN:	true if the sphere intersects the triangle, false otherwise
	bool TestSphereToTriangle (Vec4V_In sphere, const Vec3V triangle[3], Vec3V_In triangleNormal);

	// PURPOSE: Determine whether a sphere intersects a capsule.
	// PARAMS:
	//	krvecCenter -	The center of the sphere, in world coordinates.
	//	kfRadiusSphere -The radius of the sphere.
	//	krvecP0 -		One end point of the shaft of the capsule.
	//	krvecShaftAxis -The axis pointing from krvecP0 to the other end point of the capsule.  *Must be of unit length.*
	//	kfShaftLength -	The distance between the two end points of the capsule shaft.
	//	kfRadiusCapsule -The radius of the shaft and the end caps of the capsule.
	// RETURN: True if the sphere intersects the capsule, false otherwise.
	//// Replaced usage entirely with TestSphereToSeg
	//bool TestSphereToCapsule (Vec::V3Param128 krvecCenter, Vec::V3Param128 kfRadiusSphere,
	//							Vec::V3Param128 krvecP0, Vec::V3Param128_After3Args krvecShaftAxis,
	//							Vec::V3Param128_After3Args kfShaftLength, Vec::V3Param128_After3Args kfRadiusCapsule);

	// PURPOSE: Determine whether a sphere intersects a capsule.
	// PARAMS:
	//	krvecCenter -	The center of the sphere, in world coordinates.
	//	kfRadiusSphere -The radius of the sphere.
	//	krvecP0 -		One end point of the line segment.
	//	krvecP1 -		One end point of the line segment.
	// RETURN: True if the sphere intersects the line segment, false otherwise.
	bool TestSphereToSeg (Vec3V_In krvecCenter, ScalarV_In kfRadiusSphere,
							Vec3V_In krvecP0, Vec3V_In krvecP1);

	// PURPOSE: Calculate a bounding sphere for a set of vertices.
	// PARAMS:
	//	numverts -	The number of vertices in the set.
	//	verts -		The vertices, as an array of floats
	//	stride -	Optional number of bytes between vertices.  (For normal vertex arrays, this is sizeof(Vector3).)
	//	sphere -	An output parameter, where the bounding sphere is stored.
	// NOTES:
	//	- stride is useful for certain types of packed structures.  If you're passing a normal vertex
	//	array to this routine, you'll want your stride to be sizeof(Vector3); that'll make it step through
	//	the array in the normal fashion.
	void ComputeBoundSphere (int numVerts, const float verts[], u32 stride, Vector4& sphere);
	inline void	ComputeBoundSphere (int numVerts,const Vector3 verts[],Vector4& sphere)
				{ComputeBoundSphere(numVerts,(float*)verts,sizeof(Vector3),sphere);}  //lint !e740 unusual pointer cast

	void ExpandRadiusFromMotion (Vector3::Param currentPosition, Vector3::Param lastPosition, Vector3& radius, Vector3& center);
};


//=============================================================================
// @@geomPoints
//
// PURPOSE geomPoints is a collection of geometric functions operating on points.
// <FLAG Component>
//
namespace geomPoints
{
	// PURPOSE: Return true if the point is in front if the plane.
	// PARAMS:
	//	point - The point to check.
	//	planePoint - A point on the plane.
	//	planeNormal - The plane's normal vector.
	//	tolerance - optional, how close to zero is good enough
	// RETURN: true if point is less than a distance tolerance behind the plane.
	// NOTES:	Either planeNormal needs to be a unit vector, or the tolerance value needs to be divided by the
	//			length of planeNormal.  Either one will work.
	bool IsPointInFrontOfPlane (Vec3V_In point, Vec3V_In planePoint, Vec3V_In planeNormal, ScalarV_In tolerance=ScalarV(V_FLT_SMALL_6));

	// PURPOSE: Return true if the point is behind the plane.
	// PARAMS:
	//	point - The point to check.
	//	planePoint - A point on the plane.
	//	planeNormal - The plane's normal vector.
	//	tolerance - optional, how close to zero is good enough
	// RETURN: true if point is less than a distance tolerance in front of the plane.
	// NOTES:	Either planeNormal needs to be a unit vector, or the tolerance value needs to be divided by the
	//			length of planeNormal.  Either one will work.
	bool IsPointBehindPlane (const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal,
								float tolerance=SMALL_FLOAT);

	// PURPOSE: Return true if the point is near the plane.
	// PARAMS:
	//	point - The point to check.
	//	planePoint - A point on the plane.
	//	planeNormal - The plane's normal vector.
	//	tolerance - optional, how close to zero is good enough
	// RETURN: true if point is less than or equal to a distance tolerance on either side of the plane.
	// NOTES:	Either planeNormal needs to be a unit vector, or the tolerance value needs to be divided by the
	//			length of planeNormal.  Either one will work.
	bool IsPointNearPlane (const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal,
							float tolerance=SMALL_FLOAT);

	// PURPOSE: Determine if a point is inside an axis-aligned box centered at the origin
	// PARAMS:
	//   point - the point to check
	//   boxHalfSize - the half size of the box
	// RETURN:
	//   true if the point is inside, false otherwise
	BoolV_Out IsPointInBox(Vec3V_In point, Vec3V_In boxHalfSize);

	// PURPOSE: Find the closest point on an origin-centered, axis-aligned box to another point (along with normal/depth)
	// PARAMS:
	//   point - the point's position
	//   boxHalfSize - the half size of the box
	//   pointOnBox - the closest point on the surface of the box to the given point
	//   normalOnBox - the separation normal of the box
	//   depth - how deep the point is inside the box (positive - inside, negative - outside)
	void FindClosestPointPointBoxSurface(Vec3V_In point, Vec3V_In boxHalfSize, Vec3V_InOut pointOnBox, Vec3V_InOut normalOnBox, ScalarV_InOut depth);

	// PURPOSE: Return true if the point is in the box.
	// PARAMS:
	//	point - The point to check.
	//	boxMin - The minimum extent of the box.
	//	boxMax - The maximum extent of the box.
	// RETURN: true if the given point is inside the box, otherwise false.
	bool IsPointInBox (const Vector3& point, const Vector3& boxMin, const Vector3& boxMax);

	// PURPOSE: Return true if the point is in the box.
	// PARAMS:
	//	point - The point to check.
	//	halfSize - The extent of the box.
	// RETURN: true if the given point is inside the box, otherwise false.
	// NOTES:	The box is defined by x=+/- halfSize.x, y=+/- halfSize.y, z=+/- halfSize.z
	bool IsPointInBox (const Vector3& point, const Vector3& halfSize);

	// PURPOSE: Return true if the point is in the (x,z) rectangle.
	// PARAMS:
	//	point - The point to check.
	//	halfSize - The extent of the rectangle.
	// RETURN: true if the given point is inside the rectangle, otherwise false.
	// NOTES:
	//	1.	The rectangle is defined by x=+/- halfSize.x, and z=+/- halfSize.z
	//	2.	y coordinates in the Vector3s are ignored
	bool IsPointInBox2D (const Vector3& point, const Vector3& halfSize);

	// PURPOSE: Return true if the point is in the box.
	// PARAMS:
	//	point - The point to check.
	//	boxMin - top left coordinates of the bounding box
	//	boxMax - bottom right coordinates of the bounding box
	// RETURN: true if the given point is inside the box, otherwise false
	bool IsPointInBox2D (const Vector2& point, const Vector2& boxMin, const Vector2& boxMax);

	// PURPOSE: Return true if the point is within the wedge.
	// PARAMS:
	//	rvecTestPoint - The point to check.
	//	rvecSide1 - A unit vector along one side of the wedge, perpendicular to the long axis of the wedge.
	//	rvecSide2 - A unit vector along the other side of the wedge, perpendicular to the long axis of the wedge.
	//	pmtxSideTransform - If supplied, this is a matrix, by the 3x3 portion of which the sides will be transformed prior
	//						to any calculations being performed.
	//	bUseAntiWedge - Setting this to true has the same effect as either negating rvecTestPoint or negating
	//					both rvecSide1 and rvecSide2.
	// RETURN: true if the given point is within the wedge, false otherwise.
	// NOTES:
	//	1.	The long axis of the wedge is the axis perpendicular to rvecSide1 and rvecSide2.
	//	2.	Setting bUseAntiWedge to true has the same effect as either negating rvecTestPoint, or negating both rvecSide1 and rvecSide2.
	//	3.	The angle for default value of the last argument is 3 degrees (the argument is the square of the sine in radians).
	bool IsPointInWedge (const Vector3 &rvecTestPoint, const Vector3 &rvecSide1, const Vector3 &rvecSide2,
							const Matrix34* const pmtxSideTransform, bool bUseAntiWedge=false, float sineCushion2=0.003f);

	// PURPOSE: Detect if a point is on a line segment.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point2 - The end of the line segment.
	//	testPoint - The point to test
	//	distance - optional distance from the segment
	// RETURN: true if the given point is on the line segment, false otherwise.
	bool IsPointOnSeg (const Vector3& point1, const Vector3& point2, const Vector3& testPoint, const float distance = SMALL_FLOAT);

	// PURPOSE: Find the point on a line segment that's closest to the given point.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point1To2 - The vector from the beginning to the end of the line segment.
	//	targetPoint - The point to which to find the closest point on the line segment.
	// RETURN: The closest point on the line segment to the target point.
	Vec3V_Out FindClosestPointSegToPoint(Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint);
	Vec3V_Out FindClosestPointAndTValSegToPoint(Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint, ScalarV_InOut tVal);

	// PURPOSE: Find the shortest line segment connecting two segments.
	// PARAMS:
	//	outPoint1 - Output parameter, the endpoint of the resulting segment.
	//	outPoint2 - Output parameter, the other endpoint of the resulting segment.
	//  pointA2 - The second endpoint of the input segment A.
	//  pointB1 - The first endpoint of the input segment B.
	//  pointB2 - The second endpoint of the input segment B.
	//
	// NOTES:
	//  - The first endpoint of input segment A is the origin. If your segment does not start at the origin, you'll need to translate the problem.
	//  - Don't forget to translate the solution back into world space!
	//  - This is redundant with FindTValuesSegToSeg, but sleek and branchless. I suspect we can remove FindTValuesSegToSeg after callers have converted to FindClosestSegSegToSeg.
	//  - This just calls FindClosestPointsSegToSeg, it should be removed
	void FindClosestSegSegToSeg (Vec::V3Ref128 outPoint1, Vec::V3Ref128 outPoint2, Vec::V3Param128 pointA2, Vec::V3Param128 pointB1, Vec::V3Param128 pointB2);

	// PURPOSE: Find the closest points between two segments
	// PARAMS:
	//   closestPointOnA - out parameter filled with the closest point on segment A to segment B
	//   closestPointOnB - out parameter filled with the closest point on segment B to segment A
	//   startA - the starting point of segment A
	//   segmentA - the vector from start to end of segment A
	//   startB - the starting point on segment B
	//   segmentB - the vector from start to end of segment B
	void FindClosestPointsSegToSeg(Vec3V_InOut closestPointOnA, Vec3V_InOut closestPointOnB, Vec3V_In startA, Vec3V_In segmentA, Vec3V_In startB, Vec3V_In segmentB);

	// PURPOSE: Find the shortest line segment connecting a segment and a triangle.
	// PARAMS:
	//	outPoint1 - Output parameter, the endpoint of the resulting segment. (Closest point on Tri)
	//	outPoint2 - Output parameter, the other endpoint of the resulting segment. (Closest point on Seg)
	//  pointA2 - The second vertex of the input triangle A.
	//	pointA3 - The third vertex of the input triangle A.
	//  pointB1 - The first endpoint of the input segment B.
	//  pointB2 - The second endpoint of the input segment B.
	//
	// NOTES:
	//  - The first vertex of input triangle A is the origin. If your triangle does not start at the origin, you'll need to translate the problem.
	//  - Don't forget to translate the solution back into world space!
	//  - With this version the returned points will be geometrically correct even for true intersections.
	void FindClosestSegSegToTri (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In pointB1, Vec3V_In pointB2);

	// PURPOSE: Find the shortest line segment connecting a segment and a triangle.
	// PARAMS:
	//	outPoint1 - Output parameter, the endpoint of the resulting segment. (Closest point on Tri)
	//	outPoint2 - Output parameter, the other endpoint of the resulting segment. (Closest point on Seg)
	//  pointA2 - The second vertex of the input triangle A.
	//	pointA3 - The third vertex of the input triangle A.
	//  pointB1 - The first endpoint of the input segment B.
	//  pointB2 - The second endpoint of the input segment B.
	//
	// NOTES:
	//  - The first vertex of input triangle A is the origin. If your triangle does not start at the origin, you'll need to translate the problem.
	//  - Don't forget to translate the solution back into world space!
	//  - The answer is only guaranteed for disjoint objects and will be incorrect for segments intersecting the triangle.
	//		(Often the point on the triangle returned will still be close, but the segment point will almost never be on the triangle's plane.)
	void FindClosestSegSegToTriNoIntersection (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In pointB1, Vec3V_In pointB2);

	// PURPOSE: Find the shortest line segment connecting a box(AABB) and a triangle.
	// PARAMS:
	//	outPoint1 - Output parameter, the endpoint of the resulting segment. (Closest point on Tri)
	//	outPoint2 - Output parameter, the other endpoint of the resulting segment. (Closest point on AABB)
	//  pointA1 - The first vertex of the input triangle A.
	//  pointA2 - The second vertex of the input triangle A.
	//	pointA3 - The third vertex of the input triangle A.
	//  boxExtents - The vector from the center to the maximum (component-wise) positive value corner of the input AABB.
	//
	// NOTES:
	//  - The box center is assumed to be zero. If your box is not centered at the origin, you'll need to translate the problem.
	//  - Also, the box must be axis aligned. If you have an oriented box the problem will need to be rotated into box space.
	//  - Don't forget to translate the solution back into world space!
	//  - With this version does NOT handle true intersections. The input shapes should be guaranteed disjoint before using this function.
	void FindClosestSegAABBToTri (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA1, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In boxExtents);

	// PURPOSE: Determine whether a triangle intersects an AABB.
	// PARAMS:
	//  triV0 - The first vertex of the input triangle A.
	//  triV1 - The second vertex of the input triangle A.
	//	triV2 - The third vertex of the input triangle A.
	//  boxExtents - The vector from the center to the maximum (component-wise) positive value corner of the input AABB.
	// RETURN:	true if the triangle intersects the AABB, false otherwise
	// NOTES:
	//  - The box center is assumed to be zero. If your box is not centered at the origin, you'll need to translate the problem.
	//  - Also, the box must be axis aligned. If you have an oriented box the problem will need to be rotated into box space.
	BoolV_Out TestAABBAgainstTri(Vec3V_In triV0, Vec3V_In triV1, Vec3V_In triV2, Vec3V_In boxExtents);

	BoolV_Out TestAABBAgainstTriFace(Vec3V_In triV0, Vec3V_In triNorm, Vec3V_In boxExtents);

	// PURPOSE: Find the closest point on a triangle where vertex 0 is the origin, to a given point
	// PARAMS: 
	//  point - the given point, relative to the first vertex
	//  vertex1 - the second vertex, relative to the first vertex
	//  vertex2 - the third vertex, relative to the first vertex
	// RETURN: The closest point on the triangle to the given point, relative to the first vertex
	Vec3V_Out FindClosestPointPointTriangle(Vec3V_In point, Vec3V_In vertex1, Vec3V_In vertex2);

	// PURPOSE: Find the closest point on a triangle, to a given point
	// PARAMS: 
	//   point - the given point we're trying to find the closest point on the triangle to
	//   vertex0 - the first vertex on the triangle
	//   vertex1 - the second vertex on the triangle
	//   vertex2 - the third vertex on the triangle
	// RETURN: The closest point on the triangle to the given point
	Vec3V_Out FindClosestPointPointTriangle(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2);
};


//=============================================================================
// @@geomDistances
//
// PURPOSE geomDistances is a collection of geometric functions giving distances between different shapes.
// <FLAG Component>
//
namespace geomDistances
{
	// PURPOSE: Find the shortest distance between an infinite line and the origin.
	// PARAMS:
	//	point1		- a point on the line
	//	point1To2	- a vector along the line
	//	unitLineToOrigin	- optional pointer to fill in the unit vector perpendicular from the line to the origin
	// RETURN: the distance from the origin to the closest point on the infinite line
	ScalarV_Out DistanceLineToOrigin (Vec3V_In point1, Vec3V_In point1To2);
	// TODO: remove these old vector lib functions
	float DistanceLineToOrigin (const Vector3& point1, const Vector3& point1To2, Vector3* unitLineToOrigin=NULL);
	Vector3 DistanceLineToOriginV (const Vector3& point1, const Vector3& point1To2, Vector3* unitLineToOrigin=NULL);

	// PURPOSE: Find the distance between a line and a point.
	// PARAMS:
	//	point1		- a point on the line
	//	point1To2	- a vector along the line
	//	target		- the target point
	//	unitLineToPoint	- optional pointer to fill in the unit vector perpendicular from the line to the point
	// RETURN: the distance from the target point to the closest point on the infinite line
	ScalarV_Out DistanceLineToPoint (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In target);
	float DistanceLineToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target, Vector3* unitLineToPoint=NULL);
	Vector3 DistanceLineToPointV (const Vector3& point1, const Vector3& point1To2, const Vector3& target, Vector3* unitLineToPoint=NULL);

	// PURPOSE: Find the (squared) distance between a line and a point.
	// PARAMS:
	//	point1 - A point on the line.
	//	point1To2 - The slope of the line.
	//	target - The point being tested against the line.
	// RETURN: The distance between the the line and the point.
	float Distance2LineToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target);

	// PURPOSE: Find the distance between a line segment and a point.
	// PARAMS:
	//	point1 - A point on the line segment.
	//	point1To2 - The slope of the line segment.
	//	target - The point being tested against the line segment.
	// RETURN: The distance between the the line segment and the point.
	float DistanceSegToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target);

	// PURPOSE: Find the distance squared between a line segment and a point.
	// PARAMS:
	//	point1 - A point on the line segment.
	//	point1To2 - The slope of the line segment.
	//	target - The point being tested against the line segment.
	// RETURN: The square of the distance between the the line segment and the point.
	ScalarV_Out Distance2SegToPointV (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In target);
	ScalarV_Out Distance2AndTValSegToPointV (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In target, ScalarV_InOut tVal);
	float Distance2SegToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target);

	// PURPOSE: Find the distance between two lines (at their nearest point).
	// PARAMS:
	//	point1 - A point on the first line.
	//	point1To2 - The slope of the first line.
	//	target1 - A point on the second line.
	//	target1To2 - The slope of the second line.
	//	outUnitTargetToLine - An output parameter for the unit vector between the closest points from
	//	the second line to the first line.
	//	tolerance - A small number, indicating allowable floating-point roundoff error.
	// RETURN: The distance between the two lines, at their nearest point.
	float DistanceLineToLine (Vector3::Param point1, Vector3::Param point1To2, Vector3::Param target1,
								Vector3::Param target1To2, Vector3* outUnitTargetToLine=NULL, Vector3::Param tolerance=VEC3_SMALL_FLOAT);

	// PURPOSE: Find the distance between two lines (at their nearest point). Vector version.
	// PARAMS:
	//	point1 - A point on the first line.
	//	point1To2 - The slope of the first line.
	//	target1 - A point on the second line.
	//	target1To2 - The slope of the second line.
	//	outUnitTargetToLine - An output parameter for the unit vector between the closest points from
	//	the second line to the first line.
	//	tolerance - A small number, indicating allowable floating-point roundoff error.
	// RETURN: The distance between the two lines, at their nearest point, in each element of a Vector3.
	Vector3 DistanceLineToLineV (Vector3::Param point1, Vector3::Param point1To2, Vector3::Param target1,
								Vector3::Param target1To2, Vector3* outUnitTargetToLine=NULL, Vector3::Param tolerance=VEC3_SMALL_FLOAT);

	// PURPOSE: Find the distance between two parallel lines.
	// PARAMS:
	//	point1 - A point on the first line.
	//	target1 - A point on the second line.
	//	target1To2 - The slope of the second line; actually the slope of both lines (since they're parallel).
	//	outUnitTargetToLine - An output parameter for the unit vector between the parallel lines.
	// RETURN: The distance between the two lines.
	float DistanceParallelLineToLine (const Vector3& point1, const Vector3& target1,
										const Vector3& target1To2, Vector3* outUnitTargetToLine=NULL);

	// PURPOSE: Find the distance between two parallel lines.
	// PARAMS:
	//	point1 - A point on the first line.
	//	target1 - A point on the second line.
	//	target1To2 - The slope of the second line; actually the slope of both lines (since they're parallel).
	//	outUnitTargetToLine - An output parameter for the unit vector between the parallel lines.
	// RETURN: The distance between the two lines, in each element of a Vector3.
	Vector3 DistanceParallelLineToLineV (const Vector3& point1, const Vector3& target1,
		const Vector3& target1To2, Vector3* outUnitTargetToLine);

	// PURPOSE: Find the distance between a line and the Y axis.
	// PARAMS:
	//	point1 - A point on the line.
	//	point1To2 - The slope of the line.
	//	unitAxisToLine - An output parameter, containing a unit vector between the closest points from
	//	the Y axis to the line.
	// RETURN: The distance between the Y axis and the line, at their nearest point.
	float DistanceLineToYAxis (const Vector3& point1, const Vector3& point1To2, Vector3& unitLineToAxis);

	// PURPOSE: Find the shortest line segment connecting a flat disc and a triangle.
	// PARAMS:
	//	outPoint1 - Output parameter, the endpoint of the resulting segment. (Closest point on Tri)
	//	outPoint2 - Output parameter, the other endpoint of the resulting segment. (Closest point on Disc)
	//  pointA2 - The second vertex of the input triangle A.
	//	pointA3 - The third vertex of the input triangle A.
	//  discCenter - The center point of the disc.
	//  discNorm - The normal vector to the plane the disc lies on.
	//	discRadius - The flat radius of the disc
	//
	// NOTES:
	//  - The first vertex of input triangle A is the origin. If your triangle does not start at the origin, you'll need to translate the problem.
	//  - Don't forget to translate the solution back into world space!
	//  - The answer is only guaranteed for disjoint objects and will be incorrect for segments intersecting the triangle.
	void FindClosestSegDiscToTri (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In discCenter, Vec3V_In discNorm, ScalarV_In discRadius);
};


//=============================================================================
// @@geomTValues
//
// PURPOSE geomTValues is a collection of geometric functions giving fractions of distances between different shapes.
// NOTES
//   - A t-value is the fraction of the distance along a line segment. These functions return t-values for locating
//     points on segments, such as the point on a given segment that is closest to the origin.
// <FLAG Component>
//
namespace geomTValues
{
	using namespace geomDistances;

	// PURPOSE: Find the t-value of the point on a line segment nearest to the origin.
	// PARAMS:
	//	point1 - A point on the line segment.
	//	point1To2 - The slope of the line segment.
	// RETURN: A value from 0 to 1, indicating where on the line segment (between point1 and point2) the point
	//	closest to the origin is.
	inline ScalarV_Out FindTValueSegToOrigin (Vec3V_In point1, Vec3V_In point1To2);
	float FindTValueSegToOrigin (const Vector3& point1, const Vector3& point1To2);
	Vector3 FindTValueSegToOriginV (const Vector3& point1, const Vector3& point1To2);

	// PURPOSE: Find the t-value of the point on a line segment nearest to another point.
	// PARAMS:
	//	point1 - A point on the line segment.
	//	point1To2 - The slope of the line segment.
	//	targetPoint - The point being tested against the line segment.
	// RETURN: A value from 0 to 1, indicating where on the line segment (between point1 and point2) the point
	//	closest to targetPoint is.
	inline ScalarV_Out FindTValueSegToPoint (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint);
	float FindTValueSegToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& targetPoint);
	Vector3 FindTValueSegToPointV (const Vector3& point1, const Vector3& point1To2, const Vector3& targetPoint);

	// PURPOSE: Find the t-value of the point on a line nearest to another point.
	// PARAMS:
	//	point1 - A point on the line.
	//	point1To2 - The slope of the line.
	//	targetPoint - The point being tested against the line.
	// RETURN: A value, indicating where on the line the point closest to targetPoint is.  The value
	//	returned is relative to 0 at point1 and 1 at point2.
	float FindTValueOpenSegToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& targetPoint);
	ScalarV_Out FindTValueOpenSegToPointV (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint);

	// PURPOSE: Find the t-value of the point on a line nearest to the origin.
	// PARAMS:
	//	point1 - A point on the line.
	//	point1To2 - The slope of the line.
	// RETURN: A value, indicating where on the line the point closest to the origin is.  The value
	//	returned is relative to 0 at point1 and 1 at point2.
	float FindTValueOpenSegToOrigin (const Vector3& point1, const Vector3& point1To2);
	ScalarV_Out FindTValueOpenSegToOriginV (Vec3V_In point1, Vec3V_In point1To2);

	// PURPOSE: Find the t-values of the nearest points on 2 line segments.
	// PARAMS:
	//	edge1 - A point on the first line segment.
	//	edge1To2 - The slope of the first line segment.
	//	target1 - A point on the second line segment.
	//	target1To2 - The slope of the second line segment.
	//	edgeT - An output parameter, where the t-value for the closest point on the first line
	//	segment is stored.
	//	targetT - An output parameter, where the t-value for the closest point on the second line
	//	segment is stored.
	// RETURN: True if the closest points are inside the line segments, i.e. not at the endpoints.
	// NOTES:
	//	- The backpatched edgeT and targetT are always in the range 0<=t<=1.
	bool FindTValuesSegToSeg (Vec3V_In edge1, Vec3V_In edge1To2, Vec3V_In target1, Vec3V_In target1To2, ScalarV_InOut edgeT, ScalarV_InOut targetT);
	bool FindTValuesSegToSeg (const Vector3& edge1, const Vector3& edge1To2, const Vector3& target1,
								const Vector3& target1To2, float* edgeT, float* targetT);

	// PURPOSE: Find the t-values of the nearest points on 2 line segments.
	// PARAMS:
	//	edge1 - A point on the first line segment.
	//	edge1To2 - The slope of the first line segment.
	//	target1 - A point on the second line segment.
	//	target1To2 - The slope of the second line segment.
	//	edgeT - An output parameter, where the t-value for the closest point on the first line
	//	segment is stored.
	//	targetT - An output parameter, where the t-value for the closest point on the second line
	//	segment is stored.
	// RETURN: True if the closest points are inside the line segments, i.e. not at the endpoints.
	// NOTES:
	//	- The backpatched edgeT and targetT are always in the range 0<=t<=1.
	bool FindTValuesSegToUprightSeg (const Vector3& edge1, const Vector3& edge1To2, float targetMinY, float targetMaxY,
										float* edgeT, float* targetT);


	// PURPOSE: Find the t-values of the nearest points on 2 lines.
	// PARAMS:
	//	edge1 - A point on the first line.
	//	edge1To2 - The slope of the first line.
	//	target1 - A point on the second line.
	//	target1To2 - The slope of the second line.
	//	edgeT - An output parameter, where the t-value for the closest point on the first line
	//	is stored.
	//	targetT - An output parameter, where the t-value for the closest point on the second line
	//	is stored.
	// RETURN: True if the closest points are inside the lines' segments, i.e. not at the endpoints of
	//	the segments.  (Put another way, if both edgeT and targetT are >0 and <1.)
	bool FindTValuesLineToLine (Vec3V_In edge1, Vec3V_In edge1To2, Vec3V_In target1, Vec3V_In target1To2, ScalarV_InOut edgeT, ScalarV_InOut targetT);
	bool FindTValuesLineToLine (const Vector3& edge1, const Vector3& edge1To2, const Vector3& target1,
								const Vector3& target1To2, float* edgeT, float* targetT);

	// PURPOSE: Find the t-values of the nearest points between a line and the Y axis.
	// PARAMS:
	//	point1 - A point on the line.
	//	point1to2 - The slope of the line.
	//	ypoint1 - The first point on the Y axis.
	//	ypoint2 - The second point on the Y axis.
	//	edgeT - An output parameter, where the t-value for the closest point on the line is stored.
	//	targetT - An output parameter, where the t-value for the closest point on the Y axis is stored.
	// RETURN: True if the closest points are inside the lines' segments, i.e. not at the endpoints of
	//	the segments.  (Put another way, if both edgeT and axisT are >0 and <1.)
	bool FindTValuesLineToYAxis (const Vector3& point1, const Vector3& point1to2, float ypoint1, float ypoint2,
								float* edgeT, float* axisT);

	// PURPOSE: Find the colliding points on two moving lines.
	// PARAMS:
	//	seg1A - The first point on the first line.
	//	seg1B - The second point on the first line.
	//	seg1AToB - The precalculated difference from seg1A to seg1B.
	//	seg2A - The first point on the second line.
	//	seg2B - The second point on the second line.
	//	seg2AToB - The precalculated difference from seg2A to seg2B.
	//	relDisp - The relative closing velocity between the two lines, from the point of view of the first line.
	//	closest1 - An output parameter, where the intersection point on the first line is stored.
	//	closest2 - An output parameter, where the intersection point on the second line is stored.
	// RETURN: True if the moving line segments collide, false otherwise.
	bool FindLineToMovingLineIsect (const Vector3& seg1A, const Vector3& seg1B, const Vector3& seg1AToB,
								const Vector3& seg2A, const Vector3& seg2B, const Vector3& seg2AToB,
								const Vector3& relDisp, Vector3& closest1, Vector3& closest2);

	// PURPOSE: Find the t-values of the intersections between a line segment and the edges of a box face.
	//			(A box face is a 2D rectangle located somewhere in 3D space.)
	// PARAMS:
	//	point1 - A point on the line segment.
	//	point1to2 - The slope of the line segment.
	//	facenormal - The normal of the box face to test.  Used to select the box face to test.
	//	boxhalfwidths - The half-widths that describe an axially-aligned box centered at the origin.
	//	t1 - The t value indicating where the line segment enters the box face.
	//	t2 - The t value indicating where the line segment exits the box face.
	//	edgeindex1 - A number from 0 to 3, indicating which rectangle edge is intersected by the line
	//	segment at t1.
	//	edgeindex2 - A number from 0 to 3, indicating which rectangle edge is intersected by the line
	//	segment at t2.
	// RETURN: True if any part of the segment is inside the rectangle.
	bool FindTValuesLineToBoxFace (const Vector3& point1, const Vector3& point1to2, const Vector3& facenormal,
									const Vector3& boxhalfwidths, float* t1, float* t2, int* edgeindex1, int* edgeindex2);

	// PURPOSE: Add an intersection to a list of two intersections.
	// PARAMS:
	//	t - The T value of the intersection to add.
	//	segmentT1 - A pointer to the first intersection's T value in the list being constructed.
	//	segmentT2 - A pointer to the second intersection's T value in the list being constructed.
	//	normal - The normal vector of the intersection to add.
	//	normal1 - A pointer to the first intersection's normal vector in the list being constructed.
	//	normal2 - A pointer to the second intersection's normal vector in the list being constructed.
	//	numfound - The number of items in the list already (i.e. before the function was called).
	//	index - The index of the intersection to add.
	//	index1 - A pointer to the first intersection's index in the list being constructed.
	//	index2 - A pointer to the second intersection's index in the list being constructed.
	// RETURN: The number of intersections in the list now.
	// NOTES:
	//	- The index value can represent anything; it's commonly used to represent vertex indices, edge
	//	indices, and polygon numbers.
	//	- This routine is not the most efficient way to assemble a list of intersections, but it's
	//	certainly a convenient way.
	int AddIntersection (float t, float* segmentT1, float* segmentT2, const Vector3& normal,
							Vector3* normal1, Vector3* normal2, int numfound, int index, int* index1, int* index2);

	// PURPOSE: Add an intersection to a list of two intersections.
	// PARAMS:
	//	t - The T value of the intersection to add.
	//	t1 - A pointer to the first intersection's T value in the list being constructed.
	//	t2 - A pointer to the second intersection's T value in the list being constructed.
	//	index - The index of the intersection to add.
	//	index1 - A pointer to the first intersection's index in the list being constructed.
	//	index2 - A pointer to the second intersection's index in the list being constructed.
	//	NumFound - The number of items in the list already (i.e. before the function was called).
	// RETURN: The number of intersections in the list now.
	// NOTES:
	//	- The index value can represent anything; it's commonly used to represent vertex indices, edge
	//	indices, and polygon numbers.
	//	- This routine is not the most efficient way to assemble a list of intersections, but its
	//	certainly a convenient way.
	int AddIntersection (float T, float* t1, float* t2, int index, int* index1, int* index2, int NumFound);

	// PURPOSE: Calculate the first root of a quadratic function
	// PARAMS:
	//	a - the 'a' constant in the quadratic equation
	//  halfB - half of the 'b' constant in the quadratic equation
	//  c - the 'c' constant in the quadratic equation
	//  root - the first (minimum) root of this quadratic equation
	// RETURN: A BoolV set to V_TRUE if the first root is valid
	// NOTES:
	//   Quadratic equation: a*t^2 + b*t + c
	//   If 'a' is negative, root will be the larger of the two roots
	BoolV_Out RealQuadraticFirstRoot(ScalarV_In a, ScalarV_In halfB, ScalarV_In c, ScalarV_InOut root);

	// PURPOSE: Calculate the roots of a quadratic function
	// PARAMS:
	//	a - the 'a' constant in the quadratic equation
	//  halfB - half of the 'b' constant in the quadratic equation
	//  c - the 'c' constant in the quadratic equation
	//  root - the first (minimum) root of this quadratic equation
	//  root - the second (maximum) root of this quadratic equation
	// RETURN: A BoolV set to V_TRUE if the roots are valid
	// NOTES:
	//   Quadratic equation: a*t^2 + b*t + c
	//   If 'a' is positive, root1 will always be less than root2. If 'a' is negative then root2 will always be less than
	//     root1. 
	BoolV_Out RealQuadraticRoots(ScalarV_In a, ScalarV_In halfB, ScalarV_In c, ScalarV_InOut root1, ScalarV_InOut root2);
};



//=============================================================================
// @@geomSegments
//
// PURPOSE geomSegments is a collection of geometric functions that use segments.
// <FLAG Component>
//
namespace geomSegments
{
	// PURPOSE: Find the intersection between a directed segment and a triangle
	// PARAMS: 
	//  start - start of the segment
	//  startToEnd - vector from the start to the end of the segment
	//  end - end of the segment
	//  normal - normalized normal of the triangle
	//  vertex0 - first vertex on the triangle
	//  vertex1 - second vertex on the triangle
	//  vertex2 - third vertex on the triangle
	//  fractionAlongSegment - out parameter filled with the fraction along the segment where it intersects the triangle (only valid if function returns true)
	// RETURN:
	//  true if the segment hits the triangle's front face, false otherwise. 
	bool SegmentTriangleIntersectDirected (	Vec3V_In start, Vec3V_In startToEnd, Vec3V_In end,
		Vec3V_In normal, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, 
		ScalarV_InOut fractionAlongSegment);

	// This version assumes that vertex0 of the triangle is at the origin - If not, your input will need shifting
	BoolV_Out SegmentTriangleIntersectDirected (Vec3V_In segStart, Vec3V_In startToEnd, Vec3V_In segEnd,
												Vec3V_In triNorm, Vec3V_In v1, Vec3V_In v2, 
												ScalarV_InOut fractionAlongSegment);

	// PURPOSE: Find the intersection between an undirected segment and a triangle
	// PARAMS: 
	//  start - start of the segment
	//  startToEnd - vector from the start to the end of the segment
	//  normal - normalized normal of the triangle
	//  vertex0 - first vertex on the triangle
	//  vertex1 - second vertex on the triangle
	//  vertex2 - third vertex on the triangle
	//  fractionAlongSegment - out parameter filled with the fraction along the segment where it intersects the triangle (only valid if function returns true)
	// RETURN:
	//  true if the segment hits the triangle, false otherwise. 
	bool SegmentTriangleIntersectUndirected (	Vec3V_In start, Vec3V_In startToEnd,
												Vec3V_In normal, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, 
												ScalarV_InOut fractionAlongSegment);

	// This version assumes that vertex0 of the triangle is at the origin - If not, your input will need shifting
	BoolV_Out SegmentTriangleIntersectUndirected (	Vec3V_In segStart, Vec3V_In startToEnd,
													Vec3V_In triNorm, Vec3V_In v1, Vec3V_In v2, 
													ScalarV_InOut fractionAlongSegment);

	// PURPOSE: Find the t-values of the intersections between a line segment and a sphere.
	// PARAMS:
	//	sphereCenter - The center of the sphere.
	//	segmentA - The beginning of the line segment.
	//	segmentB - The end of the line segment.
	//	sphereRadius - The radius of the sphere.
	//	segmentT1 - optional output parameter, where the t value of the segment's first intersection with the sphere is stored
	//	segmentT2 - optional output parameter, where the t value of the segment's second intersection with the sphere is stored
	//	directed - optional to detect only hits going into the sphere from segmentA to segmentB
	// RETURN: The number of intersections between the segment and sphere (either 0, 1, or 2).
	// NOTES:
	//	- This function is a preprocessor for the other, more efficient SegmentToSphereIntersections().
	int SegmentToSphereIntersections (const Vector3& sphereCenter, const Vector3& segmentA, const Vector3& segmentB, float sphereRadius,
										float* segmentT1=NULL, float* segmentT2=NULL, bool directed=false);

	// PURPOSE: Find the t-values of the intersections between a line segment and a sphere.
	// PARAMS:
	//	centerToOne - A vector from the center of the sphere to the beginning of the line segment.
	//	oneToTwo - A vector from the beginning of the line segment to the end of the line segment.
	//	sphereRadius2 - The squared radius of the sphere.
	//	segmentT1 - optional output parameter, where the t value of the segment's first intersection with the sphere is stored.
	//	segmentT2 - optional output parameter, where the t value of the segment's second intersection with the sphere is stored.
	//	directed - optional to detect only hits going into the sphere from segmentA to segmentB
	// RETURN: The number of intersections between the segment and sphere (either 0, 1, or 2).
	int SegmentToSphereIntersections (const Vector3& centerToOne, const Vector3& oneToTwo, float sphereRadius2,
										float* segmentT1=NULL, float* segmentT2=NULL, bool directed=false);

	// PURPOSE: Find the t-values of the intersections between a line segment and a sphere.
	// PARAMS:
	//	centerToOne - A vector from the center of the sphere to the beginning of the line segment.
	//	oneToTwo - A vector from the beginning of the line segment to the end of the line segment.
	//	sphereRadius2 - The squared radius of the sphere.
	//	segmentT1 - output parameter, where the t value of the segment's first intersection with the sphere is stored.
	//	segmentT2 - output parameter, where the t value of the segment's second intersection with the sphere is stored.
	//	directed - optional to detect only hits going into the sphere from segmentA to segmentB
	// RETURN: The number of intersections between the segment and sphere (either 0, 1, or 2).
	int SegmentToSphereIntersections (Vec3V_In centerToOne, Vec3V_In oneToTwo, ScalarV_In sphereRadius2, ScalarV_InOut segmentT1, ScalarV_InOut segmentT2, bool directed=false);

	// PURPOSE: Find the t-values of the intersections between a line segment and a sphere.
	// PARAMS:
	//	sphereCenter - The center of the sphere.
	//	segmentA - The beginning of the line segment.
	//	segmentB - The end of the line segment.
	//	sphereRadius - The radius of the sphere.
	//	segmentT1 - output parameter, where the t value of the segment's first intersection with the sphere is stored
	//	segmentT2 - output parameter, where the t value of the segment's second intersection with the sphere is stored
	//	directed - optional to detect only hits going into the sphere from segmentA to segmentB
	// RETURN: The number of intersections between the segment and sphere (either 0, 1, or 2).
	// NOTES:
	//	- This function is a preprocessor for the other, more efficient SegmentToSphereIntersections().
	int SegmentToSphereIntersections (Vec3V_In sphereCenter, Vec3V_In segmentA, Vec3V_In segmentB, ScalarV_In sphereRadius, ScalarV_InOut segmentT1, ScalarV_InOut segmentT2, bool directed=false);

	// PURPOSE: Find the t-values of the intersections between a line segment and a hemisphere.
	// PARAMS:
	//	centertoone - A vector from the center of the sphere to the beginning of the line segment.
	//	onetotwo - A vector from the beginning of the line segment to the end of the line segment.
	//	sphereRadius2 - The squared radius of the sphere.
	//	above - If true, the test is done with the half of the sphere above the XZ plane (i.e. y > 0).
	//	If false, the test is done with the half of the sphere below the XZ plane (i.e. y < 0).
	//	segmentT1 - optional output parameter, where the t value of the segment's first intersection with
	//	the hemisphere is stored.
	//	segmentT2 - optional output parameter, where the t value of the segment's second intersection with
	//	the hemisphere is stored.
	// RETURN: The number of intersections between the segment and hemisphere (either 0, 1, or 2).
	int SegmentToHemisphereIntersections (const Vector3& centerToOne, const Vector3& oneToTwo, float sphereRadius2,
											bool above, float* segmentT1=NULL, float* segmentT2=NULL);

	// PURPOSE: Find the t-value of the intersection between a line segment and a circular disk.  (The disk is
	//	assumed to be in a plane with constant y (i.e. aligned with the XZ plane), and the disk is assumed to
	//	be centered at (x=0,z=0).)
	// PARAMS:
	//	centerToOne - A vector from the center of the disk to the beginning of the line segment.
	//	oneToTwo - A vector from the beginning of the line segment to the end of the line segment.
	//	radius2 - The (squared) radius of the disk.
	//	segmentT - An output parameter, where the t value of the segment's intersection with
	//	the disk is stored.
	// RETURN: True if the segment intersects the disk, false otherwise.
	// NOTES:
	//	- The way the parameters are written, the disk's "constant y" has been factored out, and doesn't
	//	actually appear in the calculations.
	bool SegmentToDiskIntersection (const Vector3& centerToOne, const Vector3& oneToTwo, float radius2,
									float* segmentT);

	// PURPOSE: Find the t-values of the intersection between a directed line segment and a circular disc with margin.
	// The disc is assumed to be centered at the origin.
	// PARAMS:
	//	segStart - Position vector of the segment relative to the disc center
	//	segDir - Direction of the segment(Ray)
	//	discNorm - A vector defining the normal to the plane on which the flat disc lies (Assumed to already be normalized)
	//	discRadius - Flat radius of the disc (Maximal distance to the edge of the flat disc from the origin)
	//	discMargin - Distance from the surface of the flat disc that defines the surface of the full shape
	//	segmentT - An output parameter containing intersection times
	// RETURN: Bool vector with true elements for each component of segmentT that contains a valid intersection time
	// NOTES: Pass a discMargin of ZERO for intersection against the flat disc
	VecBoolV_Out SegmentDirectedToDiscIntersection (Vec3V_In segStart, Vec3V_In segDir, Vec3V_In discNorm, ScalarV_In discRadius, ScalarV_In discMargin,
											Vec4V_InOut segmentT);

	// PURPOSE: Find the t-values of the intersections between a line segment and an upright, infinitely
	//	tall cylinder.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point1to2 - The slope of the line segment.
	//	length - The length of the cylinder.  (The cylinder is considered to extend from y=-length/2
	//	to y=+length/2.)
	//	radius2 - The (squared) radius of the cylinder.
	//	segmentT1 - An output parameter, where the t value of the segment's first intersection with
	//	the cylinder is stored.
	//	segmentT2 - An output parameter, where the t value of the segment's second intersection with
	//	the cylinder is stored.
	//	cylinderT1 - An output parameter, indicating where the first intersection point is, once it's
	//	been projected onto the cylinder's axis.
	//	cylinderT2 - An output parameter, indicating where the second intersection point is, once it's
	//	been projected onto the cylinder's axis.
	// RETURN: The number of intersections between the segment and cylinder (either 0, 1, or 2).
	// NOTES:
	//	- The cylinder t values are from 0 (meaning y=-length/2) to 1 (meaning y=+length/2).  They indicate the
	//	t value of the point on the cylinder's axis closest to the intersection point.
	int SegmentToUprightCylIsects (const Vector3& point1, const Vector3& point1to2, float length, float radius2,
									float* segmentT1, float* segmentT2, float* cylinderT1, float* cylinderT2);

	// PURPOSE: Find the t-values of the intersections between a line segment and an infinitely
	//	long cylinder.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point1to2 - The slope of the line segment.
	//	cyl1 - The beginning of the cylinder.  A point on the cylinder's axis.
	//	cyl1to2 - The slope of the cylinder.
	//	radius - The radius of the cylinder.
	//	segmentT1 - An output parameter, where the t value of the segment's first intersection with
	//	the cylinder is stored.
	//	segmentT2 - An output parameter, where the t value of the segment's second intersection with
	//	the cylinder is stored.
	// RETURN: The number of intersections between the segment and cylinder (either 0, 1, or 2).
	int SegmentToInfCylIsects (const Vector3& point1, const Vector3& point1to2, const Vector3& cyl1,
								const Vector3& cyl1to2, float radius, float* segmentT1, float* segmentT2);

	// PURPOSE: Put two intersections into the proper order.
	// PARAMS:
	//	segmentT1 - A pointer to the first intersection's T value in the list being constructed.
	//	segmentT2 - A pointer to the second intersection's T value in the list being constructed.
	//	cylT1 - A pointer to the first intersection's cylindrical T value in the list being constructed.
	//	cylT2 - A pointer to the second intersection's cylindrical T value in the list being constructed.
	//	index1 - A pointer to the first intersection's index in the list being constructed.
	//	index2 - A pointer to the second intersection's index in the list being constructed.
	// NOTES:
	//	- This routine's oddly-named calling convention comes from its only current use, in phBoundCylinder.
	//	If other code finds use for this routine, the parameter names can be clarified.
	void OrderIntersections (float* segmentT1, float* segmentT2, float* cylt1, float* cylt2,
								int* index1, int* index2);

	// PURPOSE: Determine whether a segment intersects a triangle.
	// PARAMS:
	//	start - The beginning of the segment.
	//	end - The end of the segment.
	//	v0,v1,v2 - The vertices of the triangle.
	// RETURN: True if the segment intersects the triangle, false otherwise.
	// NOTES:
	//	- This routine does an "undirected" test, i.e. it doesn't matter if the segment intersects
	//	first with the front side of the triangle or the back side.
	bool TestSegmentToTriangle (const Vector3 & start, const Vector3 & end,
							const Vector3 & v0, const Vector3 & v1, const Vector3 & v2);

	// PURPOSE: Tests whether a ray intersects a plane.
	// PARAMS:
	//	origin - The origin of the ray.
	//	ray - The slope of the ray.
	//	plane - The plane, as a Vector4.  The x/y/z components represent the unit normal vector,
	//	and the w component represents the distance from the origin.
	//	t - An output parameter, where the T value of the intersection is stored.
	//	backfaceCulling - True if intersections through the back of the plane are not allowed, false if the
	//	ray can pass through either side of the plane.
	// RETURN: True if the ray intersects the plane, false otherwise.
	bool CollideRayPlane (const Vector3 & origin, const Vector3 & ray, const Vector4 & plane, float * t, bool backfaceCulling=false);

	BoolV_Out CollideRayPlane(Vec3V_In origin, Vec3V_In ray, Vec4V_In plane, ScalarV_InOut t);
	BoolV_Out CollideRayPlaneNoBackfaceCullingV(Vec3V_In origin, Vec3V_In ray, Vec4V_In plane, ScalarV_InOut t);

	// PURPOSE: Tests whether a ray intersects a triangle.
	// PARAMS:
	//	origin - The origin of the ray.
	//	ray - The slope of the ray.
	//	vert0, vert1, vert2 - The three vertices of the triangle.
	//	t - An output parameter, where the T value of the intersection is stored.
	//	backfaceCulling - True if intersections through the back of the plane are not allowed, false if the
	//	ray can pass through either side of the plane.
	// RETURN: True if the ray intersects the triangle, false otherwise.
	bool CollideRayTriangle (const Vector3 & v1, const Vector3 & v2, const Vector3 & v3,
								const Vector3 & origin, const Vector3 & ray, float * t,
								bool backfaceCulling=false);

	// PURPOSE: Calculate the impact between an edge and a shaft.  (A shaft is a cylinder with no
	//	ends, but it has a finite length.)
	// PARAMS:
	//	shaftEnd1 - The first endpoint of the shaft to test against.
	//	shaftEnd1 - The second endpoint of the shaft to test against.
	//	vertex1 - The first endpoint of the edge to test against.
	//	vertex2 - The second endpoint of the edge to test against.
	//	radius - The radius of the shaft.
	//	idNum - An output parameter, where the index of the intersected vertex/edge is stored.
	//	impactNormal - An output parameter, where the normal of the intersection is stored.
	//	impactDepth - An output parameter, where the depth of the intersection is stored.
	//	impactAxisPoint - An output parameter, where the point on the shaft's axis closest to the intersection
	//	point is stored.
	//	shaftT - An output parameter, where a range from 0 to 1 indicates where impactAxisPoint is relative
	//	to shaftEnd1 (0) and shaftEnd2 (1).
	// RETURN: GEOM_NO_IMPACT, GEOM_EDGE, GEOM_VERTEX, or GEOM_END_IMPACT, depending on what happened.
	// NOTES:
	//	- The backpatched normal always points toward the axis of the shaft.
	int FindImpactEdgeToShaft (const Vector3& shaftEnd1, const Vector3& shaftEnd2, const Vector3& vertex1,
								const Vector3& vertex2, float radius, int& idNum, Vector3& impactNormal,
								float& impactDepth, Vector3& impactAxisPoint, float& shaftT);

	// PURPOSE: Test a directed segment against a capsule centered at the origin, aligned to the Y-axis
	// PARAMS:
	//  segmentStart - the start position of the segment
	//  segmentStartToEnd - vector from the start to the end of the segment
	//  capsuleLength - the length of the capsule (along the Y-axis)
	//  capsuleRadius - the radius of the capsule
	//  positionOnCapsule - out parameter filled with the intersection position on the capsule (only valid if the function returns true)
	//  normalOnCapsule - out parameter filled with the intersection normal on the capsule (only valid if the function returns true)
	//  fractionAlongSegment - out parameter filled with the fraction along the segment where the segment first intersects the capsule
	// RETURN: BoolV set to true if there is an intersection, false otherwise. 
	// NOTES:
	//   If the initial segment position intersects the capsule this will return false 
	BoolV_Out SegmentCapsuleIntersectDirected(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, 
												ScalarV_In capsuleLength, ScalarV_In capsuleRadius,
												Vec3V_InOut positionOnCapsule, Vec3V_InOut normalOnCapsule, ScalarV_InOut fractionAlongSegment);
};

namespace geomCapsule
{
	// PURPOSE: Intersect a capsule with an AABB at the origin
	// PARAMS:
	//  capsuleSegmentStart - the start of the capsule's segment
	//  capsuleSegmentEnd - the end of the capsule's segment
	//  capsuleRadius - the radius of the capsule
	//  aabbHalfExtents - the half extents of the AABB
	//  positionOnBox - out parameter filled with the point on the box furthest along the separating axis 
	//  normalOnBox - out parameter filled with the collision normal on the box (the separating axis)
	//  depth - out parameter filled with the depth of the intersection (positive means there was intersection)
	// RETURN: true if they intersect false otherwise. 
	// NOTE: The out parameters will all be valid regardless of the return value. 
	BoolV_Out CapsuleAABBIntersect(	Vec3V_In capsuleSegmentStart, Vec3V_In capsuleSegmentEnd, ScalarV_In capsuleRadius,
									Vec3V_In aabbHalfExtents, 
									Vec3V_InOut positionOnBox, Vec3V_InOut normalOnBox, ScalarV_InOut depth);

	// PURPOSE: Intersect a capsule with another capsule aligned with the Y-axis centered at the origin
	// PARAMS: 
	//  segmentStartA - the start of the segment on capsule A
	//  segmentA - the start-to-end segment on capsule A
	//  radiusA - the radius of capsule A
	//  halfLengthB - half of the segment length of capsule B
	//  radiusB - the radius of capsule B
	//  positionOnB - out parameter filled with the point on capsule B furthest along the separating axis
	//  normalOnB - out parameter filled with the collision normal on capsule B (the separating axis)
	//  depth - out parameter filled with the depth of the intersection (positive means there was an intersection)
	// RETURN: true if there is an intersection, false otherwise
	// NOTE: The out parameters will be valid regardless of the return value
	BoolV_Out CapsuleAlignedCapsuleIntersect(	Vec3V_In segmentStartA, Vec3V_In segmentA, ScalarV_In radiusA, 
												ScalarV_In halfLengthB, ScalarV_In radiusB,
												Vec3V_InOut positionOnB, Vec3V_InOut normalOnB, ScalarV_InOut depth);

	// PURPOSE: Intersect a capsule with a sphere
	// PARAMS:
	//  capsuleSegmentStart - the start of the capsule's segment
	//  capsuleSegment - the start-to-end segment of the capsule
	//  capsuleRadius - the radius of the capsule
	//  sphereCenter - the center of the sphere
	//  sphereRadius - the radius of the sphere
	//  positionOnSphere - out parameter filled with the point on the sphere furthest along the separating axis
	//  normalOnSphere - out parameter filled with the collision normal on the sphere (the separating axis)
	//  depth - out parameter filled with the depth of the intersection (positive means there was an intersection)
	// RETURN: true if there is an intersection, false otherwise
	BoolV_Out CapsuleSphereIntersect(	Vec3V_In capsuleSegmentStart, Vec3V_In capsuleSegment, ScalarV_In capsuleRadius,
										Vec3V_In sphereCenter, ScalarV_In sphereRadius,
										Vec3V_InOut positionOnSphere, Vec3V_InOut normalOnSphere, ScalarV_InOut depth);
}

namespace geomTaperedSweptSphere
{
	// PURPOSE: Find the intersection between a tapered swept sphere and a triangle
	// PARAMS:
	//	segmentStart - Position of the center of the initial sphere
	//  segmentEnd - Position of the center of the final sphere
	//  initialRadius - Radius of the initial sphere
	//  finalRadius - Radius of the final sphere
	//  vertex0 - first vertex of the triangle
	//  vertex1 - second vertex of the triangle
	//  vertex2 - third vertex of the triangle
	//  triangleFaceNormal - normal of the triangle (this test will work on back faces)
	//  positionOnTriangle - intersection position on triangle (undefined if return value is false)
	//  normalOnTriangle - intersection normal on triangle (undefined if return value is false)
	//  fractionAlongSegment - the fraction between the initial and final spheres where the intersection happened (undefined if return value is false)
	// RETURN: BoolV set to V_TRUE if there was an intersection. If it is V_FALSE, the InOut parameters are undefined
	// NOTES: 
	//  -This function will work on back facing triangles, it is up to the user to cull those
	//  -This function will set the InOut parameters even if there is no intersection
	BoolV_Out TaperedSweptSphereTriangleIntersect(Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In initialRadius, ScalarV_In finalRadius,
		Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_In triangleFaceNormal,
		Vec3V_InOut positionOnTriangle, Vec3V_InOut normalOnTriangle, ScalarV_InOut fractionAlongSegment);
}

namespace geomSweptSphere
{
	// PURPOSE: Find the intersection between a swept sphere and a triangle
	// PARAMS:
	//	segmentStart - Position of the center of the initial sphere
	//  segmentEnd - Position of the center of the final sphere
	//  radius - Radius of the initial sphere
	//  vertex0 - first vertex of the triangle
	//  vertex1 - second vertex of the triangle
	//  vertex2 - third vertex of the triangle
	//  triangleFaceNormal - normal of the triangle (this test will work on back faces)
	//  positionOnTriangle - intersection position on triangle (undefined if return value is false)
	//  normalOnTriangle - intersection normal on triangle (undefined if return value is false)
	//  fractionAlongSegment - the fraction between the initial and final spheres where the intersection happened (undefined if return value is false)
	// RETURN: BoolV set to V_TRUE if there was an intersection. If it is V_FALSE, the InOut parameters are undefined
	// NOTES: 
	//  -This function will work on back facing triangles, it is up to the user to cull those
	//  -This function will set the InOut parameters even if there is no intersection
	BoolV_Out SweptSphereTriangleIntersect(	Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In radius,
											Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_In triangleFaceNormal,
											Vec3V_InOut positionOnTriangle, Vec3V_InOut normalOnTriangle, ScalarV_InOut fractionAlongSegment);

	// PURPOSE: Find the intersection between a swept-sphere and an axis-aligned box at the origin
	// PARAMS:
	//  segmentStart - the center of the start of the swept sphere
	//  segmentStartToEnd - vector that the swept-sphere moves along
	//  radius - radius of the swept-sphere
	//  boxHalfExtents - the half extents of the box
	//  positionOnBox - out parameter filled with the intersection position on the box (only valid if the function returns true)
	//  normalOnBox - out parameter filled with the intersection normal on the box (only valid if the function returns true)
	//  fractionAlongSegment - out parameter filled with the fraction along the sweep where the swept-sphere first intersects the box
	// RETURN: BoolV set to true if there is an intersection, false otherwise. 
	// NOTES:
	//   If the initial sphere intersects the box this will return false
	BoolV_Out SweptSphereBoxIntersect(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, ScalarV_In radius,
										Vec3V_In boxHalfExtents,
										Vec3V_InOut positionOnBox, Vec3V_InOut normalOnBox, ScalarV_InOut fractionAlongSegment);

	// PURPOSE: Find the intersection between a swept-sphere and a sphere centered at the origin
	// PARAMS:
	//  segmentStart - the center of the start of the swept sphere
	//  segmentStartToEnd - vector that the swept-sphere moves along
	//  sweptSphereRadius - the radius of the swept sphere
	//  sphereRadius - the radius of the sphere at the origin
	//  positionOnSphere - out parameter filled with the intersection position on the sphere (only valid if the function returns true)
	//  normalOnSphere - out parameter filled with the intersection normal on the sphere (only valid if the function returns true)
	//  fractionAlongSegment - out parameter filled with the fraction along the sweep where the swept-sphere first intersects the sphere
	// RETURN: BoolV set to true if there is an intersection, false otherwise. 
	// NOTES:
	//   If the initial sphere intersects the sphere this will return false
	BoolV_Out SweptSphereSphereIntersect(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, ScalarV_In sweptSphereRadius,
											ScalarV_In sphereRadius,
											Vec3V_InOut positionOnSphere, Vec3V_InOut normalOnSphere, ScalarV_InOut fractionAlongSegment);

	// PURPOSE: Find the intersection between a swept-sphere and a capsule centered at the origin, aligned to the Y-axis
	// PARAMS:
	//  segmentStart - the center of the start of the swept sphere
	//  segmentStartToEnd - vector that the swept-sphere moves along
	//  sweptSphereRadius - the radius of the swept sphere
	//  capsuleLength - the length of the capsule (along the Y-axis)
	//  capsuleRadius - the radius of the capsule
	//  positionOnCapsule - out parameter filled with the intersection position on the capsule (only valid if the function returns true)
	//  normalOnCapsule - out parameter filled with the intersection normal on the capsule (only valid if the function returns true)
	//  fractionAlongSegment - out parameter filled with the fraction along the sweep where the swept-sphere first intersects the capsule
	// RETURN: BoolV set to true if there is an intersection, false otherwise. 
	// NOTES:
	//   If the initial sphere intersects the capsule this will return false
	BoolV_Out SweptSphereCapsuleIntersect(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, ScalarV_In sweptSphereRadius,
											ScalarV_In capsuleLength, ScalarV_In capsuleRadius,
											Vec3V_InOut positionOnCapsule, Vec3V_InOut normalOnCapsule, ScalarV_InOut fractionAlongSegment);
}

//=============================================================================
// @@geomBoxes
//
// PURPOSE geomBoxes is a collection of geometric functions that use box shapes.
// <FLAG Component>
//
namespace geomBoxes
{
	inline void ComputeBoxDataFromOppositeDiagonals(Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2, Vec3V_In vert3, Mat34V_InOut mtx, Vec3V_InOut boxSize, ScalarV_InOut maxMargin);

	// PURPOSE: Compute an orthogonal matrix from three vectors
	// PARAMS:
	//	axis0 - first column of basis
	//	axis1 - second column of basis
	//	axis2 - third column of basis
	//	axisLengths - The axis with the largest length will stay the same in the basis. The second largest axis will be modified for alignment, but not as much as the smallest.
	// NOTES:
	//  - None of the vectors need to be normalized, however this will break with zero vectors.
	//  - The output bases may not be in the same order as the input
	//  -- Additionally the axisLengths will have been permuted to match any change in order to the output bases
	inline void ComputeOrthonormalBasisFromLooseVectors(Vec3V_InOut axis0, Vec3V_InOut axis1, Vec3V_InOut axis2, Vec3V_InOut axisLengths);

	// PURPOSE: Determine whether a capsule intersects a box.
	// PARAMS:
	//	a_rSegA - The beginning of the capsule's segment.
	//	a_rSegB - The end of the capsule's segment.
	//	a_rSegAB - The difference between a_rSegB and a_rSegA.
	//	a_fRadius - The radius of the capsule.
	//	a_rBoxMin - The minimum extent of the box to test against.
	//	a_rBoxMax - The maximum extent of the box to test against.
	// RETURN: True if the capsule intersects the box, false otherwise.
	// NOTES:
	//	- This function may return false positives (hence the "FP" suffix on the name).  This routine is
	//	presumably faster than one that doesn't generate false positives.
	bool TestCapsuleToAlignedBoxFP (Vec::V3Param128_After3Args a_rSegA, Vec::V3Param128_After3Args a_rSegB,
									Vec::V4ParamSplatted128 a_fRadius, Vec::V3Param128 a_rBoxMin, Vec::V3Param128 a_rBoxMax);

	// PURPOSE: Determine whether a triangle intersects a box.
	// PARAMS:
	//	p0 - The first vertex of the triangle to test.
	//	p1 - The second vertex of the triangle to test.
	//	p2 - The third vertex of the triangle to test.
	//	min - The minimum extent of the box to test against.
	//	max - The maximum extent of the box to test against.
	//	a_pNormal - If not NULL, points to the triangle's normal vector.
	// RETURN: True if the triangle intersects the box, false otherwise.
	// NOTES:
	//	- This function may return false positives (hence the "FP" suffix on the name).  This routine is
	//	presumably faster than one that doesn't generate false positives.
	bool TestTriangleToAlignedBoxFP (const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& min, const Vector3& max);

	// PURPOSE: Determine whether a polygon intersects an oriented box.
	// PARAMS:
	//	v0,v1,v2 - The first 3 vertices of the polygon to test.
	//	v3 - if it's a triangle just duplicate one of the v0,v1,v2 in v3
	//	normal - The polygon's normal vector.
	//	boxCenter - The center of the oriented bounding box.
	//	boxAxes - An array of 3 Vector3s, containing the coordinate axes of the oriented bounding box.
	//	boxHalfExtents - An array of 3 floats, containing the half-extents, along the axes, of the box.
	// RETURN: True if the polygon intersects the box, false otherwise.
	// NOTES:
	//	- This function may return false positives (hence the "FP" suffix on the name).  This routine is
	//	presumably faster than one that doesn't generate false positives.
	bool TestPolygonToOrientedBoxFP (Vector3::Vector3Param v0, Vector3::Vector3Param v1, Vector3::Vector3Param v2,
									Vector3::Vector3Param v3, Vector3::Vector3Param normal,
									Vector3::Vector3Param boxCenter, const Matrix34 &boxAxes, Vector3::Vector3Param boxHalfExtents);

	// PURPOSE: A version of TestPolygonToOrientedBoxFP for triangles
	bool TestPolygonToOrientedBoxFP (Vector3::Vector3Param v0, Vector3::Vector3Param v1, Vector3::Vector3Param v2,
		Vector3::Vector3Param normal,Vector3::Vector3Param boxCenter, const Matrix34 &boxAxes, Vector3::Vector3Param boxHalfExtents);

	// PURPOSE: Determine whether a triangle intersects a box.
	// PARAMS:
	//	v1 - The first vertex of the triangle to test.
	//	v2 - The second vertex of the triangle to test.
	//	v3 - The third vertex of the triangle to test.
	//	boxMin - The minimum extent of the box to test against.
	//	boxMax - The maximum extent of the box to test against.
	// RETURN: True if the triangle intersects the box, false otherwise.
	bool TestTriangleToAlignedBox (const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& boxMin, const Vector3& boxMax);

	// PURPOSE: Determine whether a segment intersects an axially-aligned box centered at the origin.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point2 - The end of the line segment.
	//	halfSize - The maximum extent of the box.  (The minimum extent is -halfSize.)
	// RETURN: True if the segment intersects the box, false otherwise.
	bool TestSegmentToCenteredAlignedBox (const Vector3 & point1, const Vector3 & point2, const Vector3& halfSize);

	VecBoolV_Out TestSegmentToCenteredAlignedBox(Vec3V_In point1, Vec3V_In point2, Vec3V_In halfSize);

	// PURPOSE: Determine whether a segment intersects an axially-aligned box centered at the origin.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point1to2 - The difference between point2 and point1 (i.e. the slope of the line segment).
	//	halfSize - The maximum extent of the box.  (The minimum extent is -halfSize.)
	//	paT - If not NULL, *paT receives the t value for where the segment enters the box.
	//	pbT - If not NULL, *pbT receives the t value for where the segment exits the box.
	// RETURN: True if the segment intersects the box, false otherwise.
	bool TestSegmentToCenteredAlignedBox (const Vector3 & point1, const Vector3 & point1to2,
											const Vector3 & halfSize, float *paT, float *pbT);

	// PURPOSE: Determine whether a segment intersects an axially-aligned box centered at the origin.
	// PARAMS:
	//	point1 - The beginning of the line segment.
	//	point1to2 - The difference between point2 and point1 (i.e. the slope of the line segment).
	//	halfSize - The maximum extent of the box.  (The minimum extent is -halfSize.)
	//	minT - The smallest t that qualifies as an intersection.
	//	maxT - The biggest t that qualifies as an intersection.
	//	paT - If not NULL, *paT receives the t value for where the segment enters the box.
	//	pbT - If not NULL, *pbT receives the t value for where the segment exits the box.
	// RETURN: True if the segment intersects the box, false otherwise.
	bool TestSegmentToCenteredAlignedBox (const Vector3 & point1, const Vector3 & point1to2,
											const Vector3 & halfSize, float minT, float maxT,
											float *paT, float *pbT);

	// PURPOSE: Storage for values that are precomputed from a segment for use in TestSegmentToBox
	struct PrecomputedSegmentToBox
	{
		Vec::Vector_4V  segCenter;
		Vec::Vector_4V  absSegCenter;
		Vec::Vector_4V  absSegHalfExtents;
		Vec::Vector_4V  signedSegHalfExtents;
	};

	// PURPOSE: Precompute values from a segment for use in TestSegmentToBox
	inline void PrecomputeSegmentToBox(PrecomputedSegmentToBox *tmp, Vec::Vector_4V_In p0, Vec::Vector_4V_In p1);

	// PURPOSE: Test if a line segment and box intersect.
	// PARAMS:
	//  tmp - precomputed values from line segment
	//  boxCenter - center of aabb
	//  boxHalfExtents - half extents of aabb
	// RETURN: True iff line segment touches or intersects the box.
	inline int TestSegmentToBox(const PrecomputedSegmentToBox *tmp, Vec::Vector_4V_In boxCenter, Vec::Vector_4V_In boxHalfExtents);

	// PURPOSE: Test if a line segment and box intersect.
	// PARAMS:
	//  p0 - start of line segment (closed interval, ie, p0 is part of the segment)
	//  p1 - end of line segment (closed interval, ie, p1 is part of the segment)
	//  boxCenter - center of aabb
	//  boxHalfExtents - half extents of aabb
	// RETURN: True iff line segment touches or intersects the box.
	// NOTES:
	//  - This is just a convenient wrapper around PrecomputeSegmentToBox() and TestSegmentToBox(PrecomputedSegmentToBox&,Vector_4V_In,Vector_4V_In).
	//  When the same line segment is used to test against multiple boxes, this wrapper should not be used.
	//  - Currently named with NEW sufix to ensure it doesn't clash with the other TestSegmentToBox overloads that used to have lots of default
	//  arguments.  This sufix should be removed sometime (eg. after integrating back to RAGE head).
	inline int TestSegmentToBox(Vec::Vector_4V_In p0, Vec::Vector_4V_In p1, Vec::Vector_4V_In boxCenter, Vec::Vector_4V_In_After3Args boxHalfExtents);

	// PURPOSE: Test if a line segment and box intersect.
	// PARAMS:
	//  p0 - start of line segment (closed interval, ie, p0 is part of the segment)
	//  p1 - end of line segment (closed interval, ie, p1 is part of the segment)
	//  boxMin - min point of box
	//  boxMax - max point of box
	// RETURN: True iff line segment touches or intersects the box.
	// NOTES:
	//	- Same as TestSegmentToBox only takes the min/max of a box instead of center and extents.
	inline int TestSegmentToBoxMinMax(Vec::Vector_4V_In p0, Vec::Vector_4V_In p1, Vec::Vector_4V_In boxMin, Vec::Vector_4V_In_After3Args boxMax);

	// PURPOSE: Find the intersections between a line segment and a box.
	// PARAMS:
	//	point1 - the beginning of the line segment
	//	point1to2 - the offset from the beginning to the end of the line segment
	//	boxMin - the minimum extent of the box
	//	boxMax - the maximum extent of the box
	//	segmentT1 - optional fraction of the distance along the segment of the segment's first intersection with the box
	//	segmentT2 - optional fraction of the distance along the segment of the segment's second intersection with the box
	//	normal1 - optional outward normal of the segment's first intersection with the box
	//	normal2 - optional outward normal of the segment's second intersection with the box
	//	index1 - optional face index of the segment's first intersection with the box
	//	index2 - optional face index of the segment's second intersection with the box
	// RETURN: The number of intersections between the segment and box (either 0, 1, or 2).
	// NOTES:
	//	- The face indices are taken from the enum definition at the top of geometry.h.  The faces and their
	//	numbers are 0 (x+), 1 (x-), 2 (y+), 3 (y-), 4 (z+), and 5 (z-).
	int TestSegmentToBox (const Vector3& point1, const Vector3& point1to2, const Vector3& boxMin, const Vector3& boxMax,
							float* segmentT1, Vector3* normal1, int* index1,
							float* segmentT2, Vector3* normal2, int* index2);

	// PURPOSE: Find the intersections between a line segment and a box.
	// PARAMS:
	//	point1 - the beginning of the line segment
	//	point1to2 - the offset from the beginning to the end of the line segment
	//	boxHalfWidths - the half extent of the box along each dimension
	//	segmentT1 - optional fraction of the distance along the segment of the segment's first intersection with the box
	//	segmentT2 - optional fraction of the distance along the segment of the segment's second intersection with the box
	//	normal1 - optional outward normal of the segment's first intersection with the box
	//	normal2 - optional outward normal of the segment's second intersection with the box
	//	index1 - optional face index of the segment's first intersection with the box
	//	index2 - optional face index of the segment's second intersection with the box
	// RETURN: The number of intersections between the segment and box (either 0, 1, or 2).
	// NOTES:
	//	- The face indices are taken from the enum definition at the top of geometry.h.  The faces and their
	//	numbers are 0 (x+), 1 (x-), 2 (y+), 3 (y-), 4 (z+), and 5 (z-).
	int TestSegmentToBox (Vec3V_In point1, Vec3V_In point1to2, Vec3V_In boxHalfWidths, ScalarV_Ptr segmentT1, Vec3V_Ptr normal1, int* index1, ScalarV_Ptr segmentT2,
							Vec3V_Ptr normal2, int* index2);

	// PURPOSE: Determine whether a sphere intersects a box.
	// PARAMS:
	//	sphereCenterWorld - The center of the sphere, in world coordinates.
	//	radius - The radius of the sphere.
	//	boxMin - The minimum extent of the box being tested against, in local box coordinates.
	//	boxMax - The maximum extent of the box being tested against, in local box coordinates.
	//	boxMatrix - A matrix that transforms the box into world coordinates.
	//	dist2 - An optional output parameter; if not NULL, it indicates where the distance between the
	//	sphere and the box is stored.
	// RETURN: True if the sphere intersects the box, false otherwise.
	// NOTES:
	//	- If the sphere intersects the box, *dist2 will be set to zero.
	bool TestSphereToBox (const Vector3 &sphereCenterWorld, float radius, const Vector3 &boxMin, const Vector3 &boxMax, const Matrix34 &boxMatrix, float *dist2=NULL);

	bool TestSphereToBox (Vec3V_In sphereCenterWorld, ScalarV_In radius, Vec3V_In boxMin, Vec3V_In boxMax, const Matrix34 &boxMatrix);


	// PURPOSE: Determine whether a sphere intersects an AABB
	// PARAMS:
	//	sphereCenterRad - A v4 containing the sphere's center and radius (in the w component)
	//  boxMin - The minimum extent of the box being tested
	//  boxMax - The maximum extent of the box being tested
	// RETURNS: True if the sphere intersects the box, false otherwise
	inline bool TestSphereToAABB(Vec::V4Param128 sphereCenterRad, Vec::V3Param128 boxMin, Vec::V3Param128 boxMax);

	// PURPOSE: Determine whether a sphere intersects an AABB
	// PARAMS:
	//  sphereCenter - the center of the sphere
	//  sphereRadius - the radius of the sphere
	//  boxCenter - the center of the box
	//  boxHalfSize - the half size of the box
	// RETURN: BoolV set to true if they intersect, false otherwise
	BoolV_Out TestSphereToAABB_CenterHalfSize(Vec3V_In sphereCenter, ScalarV_In sphereRadius, Vec3V_In boxCenter, Vec3V_In boxHalfSize);

	// PURPOSE: Determine whether a box is above, below, or intersects a plane.
	// PARAMS:
	//	p0 - A point on the plane.
	//	vtx - The 8 vertices of the box.
	//	normal - The plane's normal vector.
	// RETURN: -1 if the box is below the plane, 0 if the box intersects the plane, and 1 if the box is
	//	above the plane.
	// NOTES:
	//	- The box's vertices can be in any order.
	int TestBoxToPlane (const Vector3 &p0,Vector3 vtx[8], const Vector3 &normal);

    // PURPOSE: Determine whether an oriented box intersects an oriented box.
    // PARAMS:
    //	boxAHalfWidths - The half widths of box A, must have a zero in the W component
    //	boxBHalfWidths - The half widths of box B, must have a zero in the W component
    //	aToB - Transform from A to B.
    // RETURN: True if one box intersects the other, false otherwise
    bool TestBoxToBoxOBB (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB);

    // PURPOSE: Determine whether an box intersects an box, comparing an AABB around A in B's space vs. B, 
    //          and an AABB around B in A's space vs A.
    // PARAMS:
    //	boxAHalfWidths - The half widths of box A, must have a zero in the W component
    //	boxBHalfWidths - The half widths of box B, must have a zero in the W component
    //	aToB - Transform from A to B.
    // RETURN: True if one box intersects the other, false otherwise
    // NOTES:
    //  This is almost as good as the full OBB test TestBoxToBoxOBB. The only time this returns a false
    //  positive is if the planes of box A penetrate all the planes of box B when extended to infinity 
    //  (and vice versa).
    bool TestBoxToBoxOBBFaces (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB);

	// PURPOSE: Determine whether any of the face normal constitute a separating axis between two boxes.
	// PARAMS:
	//  boxAHalfWidths, boxBHalfWidths - the half widths of the respective boxes.  It is *not* necessary to have a zero in the w component.
	//  bOrientationInA, bPositionInA - the orientation and position of the second box with respect to the first.  ***NOTE*** that this reversed
	//    with respect to how it is currently passed in for the previous TestBoxToBoxOBBFaces() function.
	BoolV_Out TestBoxToBoxOBBFaces (Vec3V_In boxAHalfWidths, Vec3V_In boxBHalfWidths, QuatV_In bOrientationInA, Vec3V_In bPositionInA);

	// PURPOSE: Calculate a bounding box for a set of vertices.
	// PARAMS:
	//	numverts - The number of vertices in the set.
	//	verts - The vertices.
	//	min - An output parameter; if not NULL, it's where the bounding-box's minimum extent is stored.
	//	max - An output parameter; if not NULL, it's where the bounding-box's maximum extent is stored.
	//	center - An output parameter; if not NULL, it's where the bounding-box's center is stored.
	void ComputeBoundBox (int numverts,const Vector3 *verts,Vector3 *min,Vector3 *max,Vector3 *center);

	void MergeAABBs(Vec3V_In center0, Vec3V_In halfExtents0, Vec3V_In center1, Vec3V_In halfExtents1, Vec3V_InOut localExtentsOut, Vec3V_InOut localCenterOut);

	// PURPOSE: Find the size of the minimum AABB enclosing an OBB
	// PARAMS:
	//  matrix/unit[X/Y/Z]Axis - The orientation of the OBB
	//  halfExtents - the half extents of the OBB
	// RETURN:
	//  a vector describing the half extents of the AABB
	Vec3V_Out ComputeAABBExtentsFromOBB(Vec3V_In unitXAxis, Vec3V_In unitYAxis, Vec3V_In unitZAxis, Vec3V_In halfExtents);
	Vec3V_Out ComputeAABBExtentsFromOBB(Mat33V_In matrix, Vec3V_In halfExtents);
	Vec3V_Out ComputeAABBExtentsFromOBB(QuatV_In orientation, Vec3V_In halfExtents);

	void ComputeAABBFromSweptOBB(Mat34V_In current, Mat34V_In last, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut);
	void ComputeAABBFromSweptOBB(QuatV_In currentOrientation, Vec3V_In currentPosition, QuatV_In lastOrientation, Vec3V_In lastPosition, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut);

    // PURPOSE: Make a OBB larger to take into a count the objects last transform
    // PARAMS:
    //  current - The orientation and position of the OBB
    //  last - The orientation and position where the OBB was last
    //  localExtentsInOut - On input, the half extents of the OBB, on output the half extents of the expanded OBB
    //  localCenterInOut - On input, the center of the OBB in local space, on output the center of the expanded OBB, also still in local space
    // NOTES:
    //  This function does not compute an AABB based on the current matrix, it leaves the center an extents in "current" space. To do both,
    //  call this function and follow it up with ComputeAABBExtentsFromOBB plus a Transform.
    void ExpandOBBFromMotionOldAndSlow (MAT34V_DECL(current), MAT34V_DECL2(last), Vec3V_Ref localExtentsInOut, Vec3V_Ref localCenterInOut);

	void ExpandOBBFromMotion(Mat34V_In relativeMatrix, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut);
	void ExpandOBBFromMotion(Mat34V_In current, Mat34V_In last, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut);

	void ExpandOBBFromMotion(QuatV_In relativeQuat, Vec3V_In relativePosition, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut);
	void ExpandOBBFromMotion(QuatV_In currentOrientation, Vec3V_In currentPosition, QuatV_In lastOrientation, Vec3V_In lastPosition, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut);

	// PURPOSE: Calculate a bounding sphere for a set of vertices.
	// PARAMS:
	//	numverts - The number of vertices in the set.
	//	verts - The vertices.
	//	stride - The number of bytes between vertices.  (For normal vertex arrays, this is sizeof(Vector3).)
	//	sphere - An output parameter, where the bounding sphere is stored.
	// NOTES:
	//	- stride is useful for certain types of packed structures.  If you're passing a normal vertex
	//	array to this routine, you'll want your stride to be sizeof(Vector3); that'll make it step through
	//	the array in the normal fashion.
	void ComputeBoundInfo (int numVerts, const float verts[], u32 stride,Vector3* boxmin, Vector3* boxmax,
							Vector3* center, float* radius);


	bool TestAABBtoAABB( Vector3::Vector3Param minA, Vector3::Vector3Param maxA, Vector3::Vector3Param minB, Vector3::Vector3Param maxB );

	// PURPOSE: Determine if two axis-aligned bounding boxes intersect.
	// PARAMS:
	//  centerA - the center of box A
	//  halfSizeA - the half size of box A
	//  centerB - the center of box B
	//  halfSizeB - the half size of box B
	// RETURN: BoolV set to true if the boxes intersect, false otherwise
	BoolV_Out TestAABBtoAABB_CenterHalfSize( Vec3V_In centerA, Vec3V_In halfSizeA, Vec3V_In centerB, Vec3V_In halfSizeB );

	// PURPOSE: six normal vectors in local coordinates for six box faces
	Vec3V_Out GetFaceNormal (int faceIndex);
};


//=============================================================================
// @@geom2D
//
// PURPOSE geom2D is a collection of two-dimensional geometric functions.
// <FLAG Component>
//
namespace geom2D
{
	// PURPOSE: Test whether a 2D point is inside a 2D triangle.
	// PARAMS:
	//	pointA, pointB - the point's 2D coordinates
	//	tri1A, tri1B - the 2D coordinates of the triangle's first vertex
	//	tri2A, tri2B - the 2D coordinates of the triangle's second vertex
	//	tri3A, tri3B - the 2D coordinates of the triangle's third vertex
	// RETURN: True if the point is inside the triangle, false otherwise.
	bool Test2DPointVsTri (float pointA, float pointB, float tri1A, float tri1B, float tri2A, float tri2B, float tri3A, float tri3B);

	// PURPOSE: Test whether a 2D point is inside a 2D triangle.
	// PARAMS:
	//	point - The point.
	//	v1,v2,v3 - The vertices of the triangle.
	// RETURN: True if the point is inside the triangle, false otherwise.
	// NOTES:
	//	- This takes X and Z as the 2 dimensions and calls the version above.
	bool Test2DPointVsTri (const Vector3& point, const Vector3& v1, const Vector3& v2, const Vector3& v3);

	// PURPOSE: Test whether a 2D segment intersects a 2D axially-aligned rectangle centered at the origin.
	// PARAMS:
	//	point1A, point1B - the 2D coordinates of the segment's first vertex
	//	point2A, point2B - the 2D coordinates of the segment's second vertex
	//	halfSizeA, halfSizeB - the 2D extent of the rectangle
	// RETURN: True if the segment intersects the rectangle, false otherwise.
	bool Test2DSegmentVsCenteredAlignedRect (float point1A, float point1B, float point2A, float point2B, float halfSizeA, float halfSizeB);

	// PURPOSE: Test whether a 2D segment intersects a 2D axially-aligned rectangle centered at the origin.
	// PARAMS:
	//	point1 - The beginning of the segment.
	//	point2 - The end of the segment.
	//	halfSize - The extent of the rectangle.
	// RETURN: True if the segment intersects the rectangle, false otherwise.
	// NOTES:
	//	- This takes X and Z as the 2 dimensions and calls the version above.
	bool Test2DSegmentVsCenteredAlignedRect (const Vector3& point1, const Vector3& point2, const Vector3& halfSize);

	// PURPOSE: Test whether a 2D triangle intersects a 2D axially-aligned rectangle.
	// PARAMS:
	//	tri1A, tri1B - the 2D coordinates of the triangle's first vertex
	//	tri2A, tri2B - the 2D coordinates of the triangle's second vertex
	//	tri3A, tri3B - the 2D coordinates of the triangle's third vertex
	//	rectMinA, rectMinB - the 2D coordinates of the rectangle's minimum corner
	//	rectMaxA, rectMaxB - the 2D coordinates of the rectangle's maximum corner
	// RETURN: True if the triangle intersects the rectangle, false otherwise.
	bool Test2DTriVsAlignedRect (float tri1A, float tri1B, float tri2A, float tri2B, float tri3A, float tri3B, float rectMinA, float rectMinB, float rectMaxA, float rectMaxB);

	// PURPOSE: Test whether a 2D triangle intersects a 2D axially-aligned rectangle.
	// PARAMS:
	//	v0,v1,v2 - The vertices of the triangle.
	//	rectMin, rectMax - The extent of the rectangle.
	//	firstAxis - optional index number of the first axis to use (0==X, 1==Y, 2==Z, default is X)
	//	secondAxis - optional index number of the second axis to use (0==X, 1==Y, 2==Z, default is Z)
	// RETURN: True if the triangle intersects the rectangle, false otherwise.
	bool Test2DTriVsAlignedRect (const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& rectMin, const Vector3& rectMax, int firstAxis=0, int secondAxis=2);

	// PURPOSE: Test whether a 2D polygon intersects a 2D axially-aligned rectangle.
	// PARAMS:
	//	numVerts - The number of vertices in the polygon.  Must be 3 or 4.
	//	verts - An array of pointers to the vertices of the polygon.
	//	rectMin, rectMax - The extent of the rectangle.
	//	firstAxis - optional index number of the first axis to use (0==X, 1==Y, 2==Z, default is X)
	//	secondAxis - optional index number of the second axis to use (0==X, 1==Y, 2==Z, default is Z)
	// RETURN: True if the polygon intersects the rectangle, false otherwise.
	// NOTES:
	//	- Despite its name, this routine cannot accept arbitrary polygons, just triangles and
	//	quadrilaterals.
	bool Test2DPolyVsAlignedRect (int numVerts, const Vector3** verts, const Vector3& rectMin, const Vector3& rectMax, int firstAxis=0, int secondAxis=2);

	// PURPOSE: Test whether two 2D lines intersect each other.
	// PARAMS:
	//	hitT1Out - An output parameter, giving the t-value of the intersection on the first line.
	//	hitT2Out - An output parameter, giving the t-value of the intersection on the second line.
	//	start1 - The beginning of the first line segment.
	//	end1 - The end of the first line segment.
	//	start2 - The beginning of the second line segment.
	//	end2 - The end of the second line segment.
	//	directedTest - True if this is a directed test (i.e. line segment 2 isn't allowed to start
	//				behind line segment 1).  Defaults to false.
	// RETURN: True if the line segments intersect each other, false otherwise.
	bool Test2DLineVsLine (float& hitT1Out, float& hitT2Out, const Vector2& start1, const Vector2& end1,
							const Vector2& start2, const Vector2& end2, bool directedTest=false);

	// PURPOSE: Test whether two 2D line segments intersect each other.
	// PARAMS:
	//	hitT1Out - An output parameter, giving the t-value of the intersection on the first line.
	//	hitT2Out - An output parameter, giving the t-value of the intersection on the second line.
	//	start1 - The beginning of the first line segment.
	//	end1 - The end of the first line segment.
	//	start2 - The beginning of the second line segment.
	//	end2 - The end of the second line segment.
	//	directedTest - True if this is a directed test (i.e. line segment 2 isn't allowed to start
	//				behind line segment 1).  Defaults to false.
	// RETURN: True if the line segments intersect each other, false otherwise.
	bool Test2DSegVsSeg (float& hitT1Out, float& hitT2Out, const Vector2& start1, const Vector2& end1,
							const Vector2& start2, const Vector2& end2, bool directedTest=false);
											// Directed test checks if line2 starts behind line1 or not

	// PURPOSE: Faster version of the function above
	// NOTES: This version doesn't have the "directed" test, and the output 't' values are undefined
	// if the function returns false.
	bool Test2DSegVsSegFast (float& hitT1Out, float& hitT2Out, float start1x, float start1y, float end1x, float end1y,
		float start2x, float start2y, float end2x, float end2y);

	// PURPOSE: Test whether a 2d line segment intersects with a polygon
	// PARAMS:
	//	tEnterOut - the t-value for where the segment 'enters' the polygon (the closest to segStart), or 0.0f if the segment starts inside
	//  tLeaveOut - the t-value for where the segment 'leaves' the polygon (the closes to segEnd) or 1.0f if the segment ends inside
	//  segStart - the starting point of the segment
	//	segEnd - the ending point of the segment
	//	verts - An array of verts forming a convex polygon. They must be in clockwise order.
	//	numVerts - The number of verts in the polygon
	// RETURNS:
	//	True if the segment intersects or is contained inside the polygon, false otherwise
	// NOTES:
	//	tEnterOut and tLeaveOut are always set to something, but only contain useful values if the function returns true.
	bool Test2DSegVsPolygon(float& tEnterOut, float& tLeaveOut, Vec2f_In segStart, Vec2f_In segEnd, const Vec2f* __restrict verts, int numVerts);
	VecBoolV_Out Test4x2DSegVsPolygon(Vec4V_InOut tEnter, Vec4V_InOut tLeave, Vec4V_In segStartX, Vec4V_In segStartY, Vec4V_In segEndX, Vec4V_In segEndY, const Vec2V* __restrict verts, int numVerts);

	// PURPOSE: Test whether two rectangles intersect each other.
	// PARAMS:
	//	centerA - the center of the first rectangle in world coordinates
	//	unitHeadingA - the direction of the length dimension of the first rectangle in world coordinates
	//	lengthA - the length of the first rectangle
	//	widthA - the width of the first rectangle
	//	centerB - the center of the second rectangle in world coordinates
	//	unitHeadingB - the direction of the length dimension of the second rectangle in world coordinates
	//	lengthB - the length of the second rectangle
	//	widthB - the width of the second rectangle
	// RETURN: true if the rectangles intersect each other, false otherwise.
	// NOTES:
	//	- This is a 2-dimensional test; y coordinates of the Vector3s are ignored.
	bool RectangleTouchesRectangle (const Vector3& centerA, const Vector3& unitHeadingA, float lengthA, float widthA,
									const Vector3& centerB, const Vector3& unitHeadingB, float lengthB, float widthB);

	// PURPOSE: Tests whether a 2D point is inside a 2D polygon.
	// PARAMS:
	//	x - The x coordinate of the point to test.
	//	y - The y coordinate of the point to test.
	//	perimeter - The vertices that make up the polygon.
	//	nperimverts - The number of vertices that make up the polygon.
	// RETURN: True if the 2D point is inside the 2D polygon, false otherwise.
	// NOTES:
	//	- This routine assumes the polygon is convex and closed.
	//	- The original comments to this routine said the polygon had to be clockwise.  The normal
	//	right-handed coordinate system assumes polygons are counterclockwise.  I'm not sure what
	//	this routine expects.
	bool IsPointInRegion (float x, float y, const Vector3* perimeter, int nperimverts);

	// PURPOSE: See which side of the given edge the given point is on, in two dimensions.
	// PARAMS:
	//	x, z -	the point's coordinates
	//	ax, az -	the edge's starting position
	//	bx, bz -	the edge's ending position
	// RETURN: zero if the point is on the edge, positive if it is inside and negative if outside
	// NOTES:	Inside the edge means it is on the left side looking from a to b, outside is on the right side.
	float TestEdgeFlat (float x, float z, float ax, float az, float bx, float bz);

	// PURPOSE: See the given point is inside the given quadrilateral.
	// PARAMS:
	//	x, z -	the point's coodinates
	//	ax, az -	the quadrilateral's first corner
	//	bx, bz -	the quadrilateral's second corner
	//	cx, cz -	the quadrilateral's third corner
	//	dx, dz -	the quadrilateral's fourth corner
	// RETURN: true if the point is inside this quad or if it lies on side a-b or on side a-c, but not on point b or c.
	// NOTES:	This returns true if the point is one one of two opposite edges and false if it is on one of the other
	//			two opposite edges so that a grid can be used and any point will be in exactly one quad.
	bool IntersectPointFlat (float x, float z, float ax, float az, float bx, float bz,
								float cx, float cz, float dx, float dz);

	// PURPOSE: Calculate the intersection points between two circles.
	// PARAMS:
	//  x0, y0	 - Center of first circle
	//  r0		 - Radius of first circle
	//  x1, y1	 - Center of second circle
	//  r1		 - Radius of second circle
	//  xi1, yi1 - First intersection point
	//  xi2, yi2 - Second intersection point
	// RETURN: true if the circles intersected
	// NOTES:  If the two circles intersect in only one point, then xi1 and yi1 will equal
	//         xi2 and yi2.
	bool IntersectTwoCircles (float x0, float y0, float r0, float x1, float y1, float r1,
							  float &xi1, float &yi1, float &xi2, float &yi2);

	// PURPOSE: Sweeps a polygon along a specified direction and returns the new, expanded polygon
	// NOTES: This will add 2 additional vertices to the polygon. Make sure you have room!
	//		This function assumes the polygon is in clockwise winding order. 
	void SweepPolygon(Vec2f* __restrict inPoints, Vec2f* __restrict outPoints, int numStartingPoints, Vec2f sweepVector);

	// PURPOSE: Determine if two lines intersect on the XY plane
	// RETURN: Returns TRUE if they intersect and stores the intersection point in "intersectionPoint"
	// - To clarify: intersectionPoint gets written to with *something* regardless of the return value
	// - intersectionPoint - Will be the point on line1 that projects onto the Z plane at the 2D intersection point of the two input lines
	//     - Note that this is *not* neccessarily the closest point on line1 to that 2D intersection
	// NOTES: Input is assumed to already be flattened into XY plane
	// - However, the only thing that would actually change is the output intersection point
	//   Which would become the 3D point of line 1 that intersects the (line2 cross zAxis) halfspace
	//   And may, in fact, be the desired end result
	bool TestLineToLineFastXY(Vec3V_In start1, Vec3V_In slope1,
		Vec3V_In start2, Vec3V_In slope2, Vec3V_InOut intersectionPoint);
};


//=============================================================================
// Implementations

inline Vec3V_Out geomVectors::ComputeLeanAngles (Mat34V_In parentPose, Vec3V_In leanAxis, ScalarV_In leanAngle, ScalarV_In twistAngle)
{
	// Transpose the parent matrix.
	Mat34V parentTranspose;
	Transpose3x3(parentTranspose,parentPose);

	// Get the two lean axes and angles.
	Vec3V lean1Axis = parentTranspose.GetCol1();
	Vec3V lean2Axis = -parentTranspose.GetCol0();
	ScalarV leanMtx22a = Dot(leanAxis,lean1Axis);
	ScalarV leanMtx22c = Dot(leanAxis,lean2Axis);

	// Return the two lean angles and the twist angle in a vector.
	return Vec3V(Scale(leanAngle,leanMtx22a),Scale(leanAngle,leanMtx22c),twistAngle);
}


inline Vec3V_Out geomVectors::GetOrthogonal (Vec3V_In u)
{
	// ORIGINAL CODE:
	//if ( u.x > 0.5f || u.x < -0.5f || u.y > 0.5f || u.y < -0.5f )
	//{
	//	v.Set ( u.y, -u.x, 0.0f );
	//}
	//else
	//{
	//	v.Set ( 0.0f, u.z, -u.y);
	//}
	//v.Normalize();

	Vec::Vector_4V v_u = u.GetIntrin128();
	Vec::Vector_4V v_neg_u = Vec::V4Negate( v_u );

	// We want to compare (a > b) with:
	//     ---------------------------
	// a = | u.x | -0.5 | u.y | -0.5 |
	//     ---------------------------
	// with:
	//     ---------------------------
	// b = | 0.5 |  u.x | 0.5 |  u.y |
	//     ---------------------------

	Vec::Vector_4V v_zero = Vec::V4VConstant(V_ZERO);
	Vec::Vector_4V v_half = Vec::V4VConstant(V_HALF);
	Vec::Vector_4V v_negHalf = Vec::V4VConstant(V_NEGHALF);
	Vec::Vector_4V a = Vec::V4MergeXY( v_u, v_negHalf );
	Vec::Vector_4V b = Vec::V4MergeXY( v_half, v_u );
	Vec::Vector_4V isGt = Vec::V4IsGreaterThanV( a, b );

	// Combine the results.
	Vec::Vector_4V resX = Vec::V4SplatX( isGt );
	Vec::Vector_4V resY = Vec::V4SplatY( isGt );
	Vec::Vector_4V resZ = Vec::V4SplatZ( isGt );
	Vec::Vector_4V resW = Vec::V4SplatW( isGt );
	Vec::Vector_4V res = Vec::V4Or( Vec::V4Or( resX, resY ), Vec::V4Or( resZ, resW ) );

	// Generate result A to select from.
	Vec::Vector_4V v_uy = Vec::V4SplatY( v_u );
	Vec::Vector_4V v_uy_0_uy_0 = Vec::V4MergeXY( v_uy, v_zero );
	Vec::Vector_4V resultA = Vec::V4MergeXY( v_uy_0_uy_0, v_neg_u );

	// Generate result B to select from.
	Vec::Vector_4V v_uz = Vec::V4SplatZ( v_u );
	Vec::Vector_4V v_0_nux_0_nuy = Vec::V4MergeXY( v_zero, v_neg_u );
	Vec::Vector_4V resultB = Vec::V4MergeZW( v_0_nux_0_nuy, v_uz );

	// Select the result.
	Vec::Vector_4V result = Vec::V4SelectFT( res, resultB, resultA );

	// Normalize and return.
	Vec::Vector_4V normalizedResult = Vec::V3Normalize( result );
	return Vec3V( normalizedResult );
}


inline Mat44V_Out geomVectors::ComputeLeanAndTwistAngles (Mat34V_In parentWorld, Mat34V_In childWorld, Mat34V_In jointWorld, Mat34V_In jointInChild, bool twistAroundChildAxis)
{
	// Transpose the input matrices to make this function simpler, because joints use a left-handed coordinate system.
	// The transposed matrices use a right-handed coordinate system like the rest of Rage.
	Mat34V jointInChildTranspose,jointWorldTranspose;
	Transpose3x3(jointInChildTranspose,jointInChild);
	Transpose3x3(jointWorldTranspose,jointWorld);

	// Compute the twist (central) axis for the child and parent in global coordinates.
	// Since these matrices are transposed this would be like grabbing the 3rd column of a 'normal' matrix.
	Vec3V axisC = jointInChildTranspose.GetCol2();
	axisC = UnTransform3x3Ortho(childWorld,axisC);

	// Compute the lean axis as the cross product of the parent's z-axis and the child's z-axis.
	Vec3V axisP = jointWorldTranspose.GetCol2();
	Vec3V lean1Axis = Cross(axisP,axisC);

	// Create the other axes and the angles, to be computed below.
	Vec3V twistAxis;
	ScalarV twistAngle,lean1Angle,twistFactor;

	// Get the squared magnitude of the lean axis.
	ScalarV leanAxisMagSq = MagSquared(lean1Axis);
	if (IsGreaterThanOrEqualAll(leanAxisMagSq,ScalarV(V_FLT_SMALL_12)))
	{
		// The lean angle is not nearly zero, so normalize the lean axis.
		ScalarV leanAngleSine = Sqrt(leanAxisMagSq);
		lean1Axis = InvScale(lean1Axis,Vec3V(leanAngleSine));

		// Compute the lean angle.
		ScalarV leanAngleCosine = Dot(axisP,axisC);
		lean1Angle = Arctan2(leanAngleSine,leanAngleCosine);

		// Compute the twist axis as half way between the parent and child z-axes (it's changed later if twistAroundChildAxis is true).
		twistAxis = Add(axisP,axisC);
		ScalarV twistAxisMag = Mag(twistAxis);
		if (IsGreaterThanOrEqualAll(twistAxisMag,ScalarV(V_FLT_SMALL_12)))
		{
			// The twist axis is not nearly zero. Compute the twist factor as 1.0/cos(CurrentLeanAngle/2.0)
			twistFactor = InvScale(ScalarV(V_TWO),twistAxisMag);

			// Normalize the twist axis.
			twistAxis = Scale(twistAxis,Scale(Vec3V(V_HALF),Vec3V(twistFactor)));

			// Find the second lean axis (perpendicular to the lean and twist axes)
			Vec3V lean2Axis = Cross(lean1Axis,axisP);// was twistAxis instead of axisP

			// Get the joint's x-axis in world coordinates.
			// (This is not the same as jointWorld.GetCol0() or jointWorldTranspose.GetCol0() so something must be misnamed here,
			//  probably the input matrix jointWorld is not what it says it is).
			Vec3V jointWorldX = UnTransform3x3Ortho(childWorld,jointInChildTranspose.GetCol0());

			ScalarV jointXdotLean2 = Dot(lean2Axis,jointWorldX);
			ScalarV sinTimesAxisZ = Scale(leanAngleSine,jointXdotLean2);
			ScalarV cosTimesAxisZ = Scale(leanAngleCosine,jointXdotLean2);
			ScalarV axisY = Dot(axisP,jointWorldX);
			ScalarV newY = AddScaled(sinTimesAxisZ,leanAngleCosine,axisY);
			ScalarV newZ = SubtractScaled(cosTimesAxisZ,leanAngleSine,axisY);

			Vec3V unleaned = AddScaled(jointWorldX,axisP,Subtract(newY,axisY));
			unleaned = AddScaled(unleaned,lean2Axis,Subtract(newZ,jointXdotLean2));

			jointWorldX = Transform3x3(jointWorld,unleaned);
			twistAngle = Arctan2(SplatY(jointWorldX),SplatX(jointWorldX));
		}
		else
		{
			// The angle is nearly PI. This only happens here in from roundoff error, since normSq should have been zero above.
			lean1Angle = ScalarV(V_ZERO);
			twistAxis = axisP;
			twistAngle = ScalarV(V_ZERO);
			twistFactor = ScalarV(V_ONE);
		}
	}
	else
	{
		// The lean angle is nearly zero.
		lean1Angle = ScalarV(V_ZERO);
		lean1Axis = GetOrthogonal(axisP);			// Arbitrary orthogonal axis for the LeanAxis
		twistAxis = axisP;
		Vec3V& axisXC = axisC;			// Reuse this temporary variable
		axisXC = jointInChildTranspose.GetCol0();
		axisXC = UnTransform3x3Ortho( childWorld, axisXC );		// Child's x-axis is global coordinates
		ScalarV xCos = Dot(axisXC,jointWorldTranspose.GetCol0());
		ScalarV xSin = Dot(axisXC,jointWorldTranspose.GetCol1());
		twistAngle = Arctan2(xSin,xCos);
		twistFactor = ScalarV(V_ONE);
	}

	if (twistAroundChildAxis)
	{
		// Change the twist axis to match the child body part's axis.
		twistAxis = axisC;
	}

	Vec3V thirdAxis = Cross(lean1Axis,twistAxis);

	Vec3V computedLeanAngles = ComputeLeanAngles(parentWorld,lean1Axis,lean1Angle,twistAngle);
	Vec4V angles = Vec4V(lean1Angle,SplatY(computedLeanAngles),twistAngle,twistFactor);

	Mat44V axesAndAngles;
	axesAndAngles.SetCol0(Vec4V(lean1Axis));
	axesAndAngles.SetCol1(Vec4V(thirdAxis));
	axesAndAngles.SetCol2(Vec4V(twistAxis));
	axesAndAngles.SetCol3(angles);
	return axesAndAngles;
}


inline QuatV_Out geomVectors::LeanAndTwistToQuatV (ScalarV_In lean1Angle, ScalarV_In lean2Angle, ScalarV_In twistAngle)
{
	// Compute the quaternion for the twist only.
	ScalarV half = ScalarV(V_HALF);
	ScalarV halfTwistAngle = Scale(twistAngle,half);
	ScalarV zero = ScalarV(V_ZERO);
	QuatV qTwist = QuatV(zero,zero,Sin(halfTwistAngle),Cos(halfTwistAngle));

	// Compute the quaternion for the lean only.
	ScalarV leanAngle = Sqrt(Add(Scale(lean1Angle,lean1Angle),Scale(lean2Angle,lean2Angle)));
	ScalarV halfLeanAngle = Scale(leanAngle,half);
	BoolV nearZeroAngle = IsLessThan(halfLeanAngle,ScalarV(V_FLT_EPSILON));
	ScalarV sineOverAngle = SelectFT(nearZeroAngle,Scale(Sin(halfLeanAngle),Invert(halfLeanAngle)),ScalarV(V_ONE));
	ScalarV alpha = Scale(half,sineOverAngle);
	QuatV qLean = QuatV(-Scale(alpha,lean2Angle),Scale(alpha,lean1Angle),zero,Cos(halfLeanAngle));

	// Multiply and return the lean and twist rotations.
	return Multiply(qLean,qTwist);
}


inline Vec3V_Out geomVectors::QuatVToLeanAndTwist (QuatV_In q)
{
	Vec3V u = Transform(q,Vec3V(V_Z_AXIS_WZERO));
	ScalarV uX = SplatX(u);
	ScalarV uY = SplatY(u);
	ScalarV sineTheta = Sqrt(Add(Scale(uX,uX),Scale(uY,uY)));
	ScalarV zero = ScalarV(V_ZERO);
	if (IsGreaterThanAll(sineTheta,ScalarV(V_FLT_EPSILON)))
	{
		ScalarV theta = Arctan2(sineTheta,SplatZ(u));
		ScalarV inverseSineTheta = Invert(sineTheta);
		ScalarV uXoverS = Scale(uX,inverseSineTheta);
		ScalarV uYoverS = Scale(uY,inverseSineTheta);
		ScalarV halfTheta = Scale(theta,ScalarV(V_HALF));
		ScalarV sineHalfTheta = Sin(halfTheta);
		QuatV unLean = QuatV(Scale(sineHalfTheta,uYoverS),-Scale(sineHalfTheta,uXoverS),zero,Cos(halfTheta));
		QuatV qTwistOnly = Multiply(unLean,q);
		ScalarV sineTwist = SplatZ((Vec4V)qTwistOnly);
		ScalarV cosineTwist = SplatW((Vec4V)qTwistOnly);
		Vec3V leanLeanTwist = Vec3V(Scale(uXoverS,theta),Scale(uYoverS,theta),Scale(ScalarV(V_TWO),Arctan2(sineTwist,cosineTwist)));
		return leanLeanTwist;
	}

	ScalarV sineTwist = SplatZ((Vec4V)q);
	ScalarV cosineTwist = SplatW((Vec4V)q);
	Vec3V leanLeanTwist = Vec3V(zero,zero,Scale(ScalarV(V_TWO),Arctan2(sineTwist,cosineTwist)));
	return leanLeanTwist;
}


inline void geomSpheres::ExpandRadiusFromMotion (Vector3::Param currentPosition, Vector3::Param lastPosition, Vector3& radius, Vector3& center)
{
	// Find the motion since the last frame.
	Vector3 displacement(currentPosition);
	displacement.Subtract(lastPosition);
	Vector3 halfDisplacement = displacement.MagV();
	halfDisplacement.Multiply(VEC3_HALF);

	// Move and enlarge the object's sphere radius to avoid missing collisions when object spheres pass through each other but do not overlap
	// at the end of any frame.
	// Warning: this is incorrect for rotating objects with large centroid offsets, because the instance displacement is used instead
	// of the centroid displacement. Finding the centroid displacement would require another transformation for all objects.
	// So fast-rotating objects with large centroid offsets can miss collisions.
	radius.Add(halfDisplacement);
	center.SubtractScaled(displacement,VEC3_HALF);
}


__forceinline BoolV_Out geomPoints::IsPointInBox(Vec3V_In point, Vec3V_In boxHalfSize)
{
	const VecBoolV isPointInsidePlane = IsLessThanOrEqual(Abs(point),boxHalfSize);
	return isPointInsidePlane.GetX() & isPointInsidePlane.GetY() & isPointInsidePlane.GetZ();
}

inline bool geomPoints::IsPointInBox (const Vector3& point, const Vector3& boxMin, const Vector3& boxMax)
{
	return (point.x <= boxMax.x && point.x >= boxMin.x &&
			point.y <= boxMax.y && point.y >= boxMin.y &&
			point.z <= boxMax.z && point.z >= boxMin.z);
}


inline bool geomPoints::IsPointInBox(const Vector3& point, const Vector3& halfSize)
{
	if(point.x>halfSize.x || point.x<-halfSize.x) return false;
	if(point.y>halfSize.y || point.y<-halfSize.y) return false;
	if(point.z>halfSize.z || point.z<-halfSize.z) return false;
	return true;
}


inline bool geomPoints::IsPointInBox2D (const Vector3& point, const Vector3& halfSize)
{
	if (point.x>halfSize.x || point.x<-halfSize.x) return false;
	if (point.z>halfSize.z || point.z<-halfSize.z) return false;
	return true;
}


inline bool geomPoints::IsPointInBox2D (const Vector2& point, const Vector2& boxMin, const Vector2& boxMax)
{
	return (point.x <= boxMax.x && point.x >= boxMin.x &&
			point.y <= boxMax.y && point.y >= boxMin.y);
}


inline bool geomPoints::IsPointInWedge (const Vector3 &rvecTestPoint, const Vector3 &rvecSide1,
										const Vector3 &rvecSide2, const Matrix34 * const pmtxSideTransform,
										bool bUseAntiWedge, float sineCusion2)
{
	// Hard assert if it's a non-unit normal, non-zero normal.  DebugAssert only if it's a zero normal.
	DebugAssert(rvecSide1.Mag2() > square(0.01f));
	DebugAssert(rvecSide2.Mag2() > square(0.01f));
	FastAssert(fabs(rvecSide1.Mag2() - 1.0f) <= square(0.01f) || rvecSide1.Mag2() <= square(0.01f));
	FastAssert(fabs(rvecSide2.Mag2() - 1.0f) <= square(0.01f) || rvecSide2.Mag2() <= square(0.01f));

	Vector3 vecTestPoint;
	if(pmtxSideTransform != NULL)
	{
		vecTestPoint.Set(rvecTestPoint);
		vecTestPoint.Dot3x3Transpose(*pmtxSideTransform);
	}
	else
	{
		vecTestPoint.Set(rvecTestPoint);
	}

	Vector3 vecEdge;
	vecEdge.Add(rvecSide1, rvecSide2);

	// Reject the point if it's completely on the wrong side to be in the wedge.
	// This isn't just an early out, the second test below will return a false positive for vectors 
	float fPointDotEdge = vecTestPoint.Dot(vecEdge);
	if(bUseAntiWedge ? fPointDotEdge > 0.0f : fPointDotEdge < 0.0f)
	{
		return false;
	}

	// A test point that is far enough out of the plane of the two side vectors could have a positive dot product
	// between its cross products with the side vectors, so project it onto the plane before finding the cross products.
	Vector3 planeTestPoint(vecTestPoint);
	Vector3 planeNormal(rvecSide1);
	planeNormal.Cross(rvecSide2);
	float planeNormalMag2 = planeNormal.Mag2();
	if (planeNormalMag2>SMALL_FLOAT)
	{
		planeNormal.Scale(invsqrtf(planeNormalMag2));
		planeTestPoint.SubtractScaled(planeNormal,planeNormal.Dot(planeTestPoint));
	}

	// Find the cross products of the test point with the wedge sides.
	Vector3 vecPointCrossSide1, vecPointCrossSide2;
	vecPointCrossSide1.Cross(planeTestPoint,rvecSide1);
	vecPointCrossSide2.Cross(planeTestPoint,rvecSide2);


	float fDot = vecPointCrossSide1.Dot(vecPointCrossSide2);

	float error2 = planeTestPoint.Mag2()*sineCusion2;
	return ((fDot <= 0.0f) || (vecPointCrossSide1.Mag2() <= error2) || (vecPointCrossSide2.Mag2() <= error2));
}


inline bool geomPoints::IsPointOnSeg (const Vector3& point1, const Vector3& point2, const Vector3& testPoint, float distance)
{
	// The three given points can make a triangle.  If the area of the triangle is zero, all three points are co-linear
	Vector3 edge1 = point2 - point1;
	Vector3 edge2 = testPoint - point1;
	float edge1Length2 = edge1.Mag2();
	if (fabs(edge1Length2) > VERY_SMALL_FLOAT)
	{
		// point1 and point2 are not too close, so find the triangle area.
		Vector3 normal;
		normal.Cross(edge1, edge2);
		float area2 = normal.Mag2() / edge1Length2;
		if ( area2 > square(distance) )
		{
			return false;
		}
	}

	// The test point is nearly on the same line, or the given segment has nearly zero length, so see if it is within the segment.
	float edge2Length2 = edge2.Mag2();
	float edge3Length2 = testPoint.Dist2(point2);
	float extendedEdge1Length2 = edge1Length2+distance;
	return ( edge2Length2 <= extendedEdge1Length2 && edge3Length2 <= extendedEdge1Length2 );
}

__forceinline Vec3V_Out geomPoints::FindClosestPointSegToPoint(Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint)
{
	Vec3V p1ToTarget = Subtract(targetPoint, point1);

	ScalarV segAxisLenSqrd = Dot(point1To2, point1To2);
	ScalarV t = SelectFT( IsGreaterThan(segAxisLenSqrd, ScalarV(V_FLT_SMALL_12)), ScalarV(V_ZERO), Saturate(InvScale(Dot(point1To2, p1ToTarget), segAxisLenSqrd)) );
	Vec3V closestPointOnSeg = AddScaled(point1, point1To2, t);

	return closestPointOnSeg;
}

__forceinline Vec3V_Out geomPoints::FindClosestPointAndTValSegToPoint(Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint, ScalarV_InOut tVal)
{
	Vec3V p1ToTarget = Subtract(targetPoint, point1);

	ScalarV segAxisLenSqrd = Dot(point1To2, point1To2);
	ScalarV t = SelectFT( IsGreaterThan(segAxisLenSqrd, ScalarV(V_FLT_SMALL_12)), ScalarV(V_ZERO), Saturate(InvScale(Dot(point1To2, p1ToTarget), segAxisLenSqrd)) );
	Vec3V closestPointOnSeg = AddScaled(point1, point1To2, t);

	tVal = t;
	return closestPointOnSeg;
}

// This function ensures that the the output is between min and max even if the input is a QNAN
__forceinline ScalarV_Out ClampSafe(ScalarV_In v, ScalarV_In min, ScalarV_In max)
{
	return SelectFT(IsGreaterThan(v,min),min,Min(v,max));
}

__forceinline void geomPoints::FindClosestPointsSegToSeg(Vec3V_InOut closestPointOnA, Vec3V_InOut closestPointOnB, Vec3V_In startA, Vec3V_In segmentA, Vec3V_In startB, Vec3V_In segmentB)
{
	// Constants
	const ScalarV svZero(V_ZERO);
	const ScalarV svOne(V_ONE);

	// Compute some values that will get reused
	const Vec3V segmentC = Subtract(startB,startA);
	const ScalarV AdotB = Dot(segmentA,segmentB);
	const ScalarV AdotC = Dot(segmentA,segmentC);
	const ScalarV magSqA = MagSquared(segmentA);
	const ScalarV BdotC = Dot(segmentB,segmentC);
	const ScalarV magSqB = MagSquared(segmentB);

	// Compute the T-value on segment A where the two infinite lines come closest
	// NOTE: 'tA' might end up as a QNAN but we handle it later
	const ScalarV tA = InvScale(SubtractScaled(Scale(AdotC,magSqB),BdotC,AdotB), 
								SubtractScaled(Scale(magSqA,magSqB),AdotB,AdotB));

	// Find the min and max T-values of segment B projected onto segment A. The final T-value must be between them. 
	// NOTE: 'tMaxA' and 'tMinA' might end up as QNANs but we handle it later
	const ScalarV AdotEndB = Add(AdotC,AdotB);
	const ScalarV tMaxA = InvScale(Max(AdotC,AdotEndB),magSqA);
	const ScalarV tMinA = InvScale(Min(AdotC,AdotEndB),magSqA);

	// First clamp tA to the projected segment B. If 'tA' is QNAN this will return 'tMinA' or 'tMaxA', any value in that range
	//   is acceptable if the lines are parallel. Next clamp that value between 0 and 1, if 'tMinA' or 'tMaxA' is QNAN then it
	//   means that segment A has length 0 and the T-value can be anything between 0 and 1. 
	const ScalarV tFinalA = ClampSafe(ClampSafe(tA,tMinA,tMaxA),svZero,svOne);

	// Using the final T-value, find the closest point on segment A to segment B
	const Vec3V localClosestPointOnA = AddScaled(startA,segmentA,tFinalA);

	// Rather than do all that math again just use the closest point on segment A to find the closest point on segment B. This also
	//   ensures that if the lines are parallel the two points we chose line up. 
	const ScalarV tFinalB = ClampSafe(InvScale(Dot(Subtract(localClosestPointOnA,startB),segmentB),magSqB),svZero,svOne);

	// Fill in the final points
	closestPointOnA = localClosestPointOnA;
	closestPointOnB = AddScaled(startB,segmentB,tFinalB);
}

__forceinline void geomPoints::FindClosestSegSegToSeg (Vec::V3Ref128 outPoint1, Vec::V3Ref128 outPoint2, Vec::V3Param128 pointA2, Vec::V3Param128 pointB1, Vec::V3Param128 pointB2)
{
	FindClosestPointsSegToSeg(Vec3V_InOut(outPoint1),Vec3V_InOut(outPoint2),Vec3V(V_ZERO),Vec3V(pointA2),Vec3V(pointB1),Subtract(Vec3V(pointB2),Vec3V(pointB1)));
}

__forceinline void FindClosestSegSegToTriInternalHelper (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In pointB1, Vec3V_In pointB2,
														 Vec3V_InOut v0, Vec3V_InOut v1, ScalarV_InOut triDot11, ScalarV_InOut triDot12, ScalarV_InOut triDot22, Vec3V_InOut segAxis, ScalarV_InOut invDenom)
{
	// This helper is intended for use within this file only. It combines the duplicated code of the two other SegToTri tests.
	//  There are a number of extra returned parameters so that the extended version can continue using the same already computed values.

	// Temps for pair selection
	Vec3V closestVec;
	ScalarV distSqrd;
	BoolV closer;
	ScalarV closestDistSqrd(V_FLT_MAX);

	// Test line seg against all edges of the triangle
	segAxis = Subtract(pointB2, pointB1);
	//
	const Vec3V u = segAxis;
	v0 = pointA2;
	const Vec3V w0 = pointB1;
	v1 = pointA3;
	//Vec3V w1 = pointB1; same as w0
	const Vec3V v2 = Subtract(pointA3, pointA2);
	const Vec3V w2 = Subtract(pointB1, pointA2);

	const Vec3V a = Vec3V(Dot(u,u));        // always >= 0
	const Vec3V b( Dot(u,v0), Dot(u,v1), Dot(u,v2) );
	const Vec3V c( Dot(v0,v0), Dot(v1,v1), Dot(v2,v2) );        // always >= 0
	const Vec3V d( Vec2V(Dot(u,w0)), Dot(u,w2) );
	const Vec3V e( Dot(v0,w0), Dot(v1,w0), Dot(v2,w2) );
	const Vec3V lengthSquaredProduct = Scale(a, c);
	const Vec3V D = SubtractScaled(lengthSquaredProduct, b, b);       // always >= 0
	Vec3V sc, sN, sD = D;      // sc = sN / sD, default sD = D >= 0
	Vec3V tc, tN, tD = D;      // tc = tN / tD, default tD = D >= 0

	sN = SubtractScaled(Scale(b,e), c, d);
	tN = SubtractScaled(Scale(a,e), b, d);

	const VecBoolV sN0 = IsLessThan(sN, Vec3V(V_ZERO));
	const VecBoolV sNsD = IsGreaterThan(sN, sD);

	// Clamp sN to the line segment
	sN = SelectFT(sN0, SelectFT(sNsD, sN, sD), Vec3V(V_ZERO));
	tN = SelectFT(sN0, SelectFT(sNsD, tN, Add(e, b)), e);
	tD = SelectFT(sN0, SelectFT(sNsD, tD, c), c);

	// To call this 'degenerate' isn't quite right - I wouldn't call two capsules that are parallel to each other (or nearly so) degenerate but they
	//   do represent a configuration that requires an alternate method for determining closest points.  This BoolV is what is used for selecting
	//   between those two results.
	// Should this threshold be even smaller?  Possibly.  We really don't want to be too quick to declare the capsules to be parallel but I don't have
	//   any clear basis for reducing this further.  This tolerance (0.0001) means that 1 - cos^2(angle) < 0.0001 => cos^2(angle) < 0.9999 => angle < about 0.5
	//   degrees, which seems about right.
	const VecBoolV degenerate = IsLessThan(D, Scale(lengthSquaredProduct, ScalarV(V_FLT_SMALL_4)));
	sN = SelectFT(degenerate, sN, Vec3V(V_ZERO));
	sD = SelectFT(degenerate, sD, Vec3V(V_ONE));
	tN = SelectFT(degenerate, tN, e);
	tD = SelectFT(degenerate, tD, c);

	const VecBoolV tN0 = IsLessThan(tN, Vec3V(V_ZERO));
	const VecBoolV tNtD = IsGreaterThan(tN, tD);

	tN = SelectFT(tN0, SelectFT(tNtD, tN, tD), Vec3V(V_ZERO)); // Clamp tN to the line segment

	const VecBoolV d0 = IsLessThan(Negate(d), Vec3V(V_ZERO));
	const VecBoolV da = IsGreaterThan(Negate(d), a);
	const Vec3V sNtN0 = SelectFT(d0, SelectFT(da, Negate(d), sD), Vec3V(V_ZERO));	//  sN if tN < 0

	const Vec3V bd = Subtract(b, d);
	const VecBoolV db0 = IsLessThan( bd, Vec3V(V_ZERO) );
	const VecBoolV dba = IsGreaterThan( bd, a );
	const Vec3V sNtNtD = SelectFT(db0, SelectFT(dba, bd, sD), Vec3V(V_ZERO));	//  sN if tN > tD

	sN = SelectFT(tN0, SelectFT(tNtD, sN, sNtNtD), sNtN0);
	sD = SelectFT((tN0 & !d0 & !da) | (tNtD & !db0 & !dba), sD, a);

	// finally do the division to get sc and tc
	sc = SelectFT(IsLessThan(Abs(sN), Vec3V(V_FLT_SMALL_6)), InvScale(sN, sD), Vec3V(V_ZERO));
	tc = SelectFT(IsLessThan(Abs(tN), Vec3V(V_FLT_SMALL_6)), InvScale(tN, tD), Vec3V(V_ZERO));

	// Use t values to compute segs' closest points
	const Vec3V segTestClosestOnSeg0 = AddScaled(pointB1, segAxis, sc.GetX());
	const Vec3V segTestClosestOnTri0 = Scale(v0, tc.GetX());
	const Vec3V segTestClosestOnSeg1 = AddScaled(pointB1, segAxis, sc.GetY());
	const Vec3V segTestClosestOnTri1 = Scale(v1, tc.GetY());
	const Vec3V segTestClosestOnSeg2 = AddScaled(pointB1, segAxis, sc.GetZ());
	const Vec3V segTestClosestOnTri2 = AddScaled(v0, v2, tc.GetZ()); // Note v0 = pointA2

	// Start testing for actual closest pair
	closestVec = Subtract(segTestClosestOnSeg0, segTestClosestOnTri0);
	closestDistSqrd = Dot(closestVec, closestVec);
	outPoint1 = segTestClosestOnTri0;
	outPoint2 = segTestClosestOnSeg0;

	closestVec = Subtract(segTestClosestOnSeg1, segTestClosestOnTri1);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, segTestClosestOnTri1);
	outPoint2 = SelectFT(closer, outPoint2, segTestClosestOnSeg1);

	closestVec = Subtract(segTestClosestOnSeg2, segTestClosestOnTri2);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, segTestClosestOnTri2);
	outPoint2 = SelectFT(closer, outPoint2, segTestClosestOnSeg2);

	//////////////////////////////////////////////////////////////////////////
	// Test seg endpoints against face of triangle
	// Find barycentric coords of two line endpoints on triangle's plane
	triDot11 = c.GetX();
	triDot12 = Dot(v0, v1);
	triDot22 = c.GetY();

	const ScalarV triSegDot10 = e.GetX();
	const ScalarV triSegDot20 = Dot(v1, w0);

	const ScalarV triSegDot11 = Dot(v0, pointB2);
	const ScalarV triSegDot21 = Dot(v1, pointB2);

	invDenom = Invert(SubtractScaled( Scale(triDot11, triDot22), triDot12, triDot12 ));
	Vec4V endpointBary(triSegDot10, triSegDot20, triSegDot11, triSegDot21);
	endpointBary = Scale(Subtract( Scale(endpointBary, Vec4V(triDot22, triDot11, triDot22, triDot11)), 
		Scale(endpointBary.Get<Vec::Y, Vec::X, Vec::W, Vec::Z>(), triDot12) ), invDenom);

	// Use UVs to generate the closest points on the triangle's plane
	const Vec3V faceTestClosestOnTri0 = AddScaled(Scale(v0, endpointBary.GetX()), v1, endpointBary.GetY());
	const Vec3V faceTestClosestOnTri1 = AddScaled(Scale(v0, endpointBary.GetZ()), v1, endpointBary.GetW());

	// vectors to test against for triangle containment of the barycentric projected endpoints
	const Vec3V triContain0(endpointBary.GetXY(), Add(endpointBary.GetX(), endpointBary.GetY()));
	const Vec3V triContain1(endpointBary.GetZW(), Add(endpointBary.GetZ(), endpointBary.GetW()));

	// Testing for actual closest pair
	// The somewhat clearer closer test had been:
	// 	closer = IsLessThanAll(distSqrd, closestDistSqrd) && IsGreaterThanOrEqualAll(triContain0, Vec3V(V_ZERO)) && IsLessThanOrEqualAll(triContain0, Vec3V(V_ONE));
	//
	closestVec = Subtract(pointB1, faceTestClosestOnTri0);
	distSqrd = Dot(closestVec, closestVec);
	VecBoolV temp = IsLessThan(Vec4V(Vec3V(V_ZERO), distSqrd), Vec4V(triContain0, closestDistSqrd)) & IsLessThan(Vec4V(triContain0, distSqrd), Vec4V(Vec3V(V_ONE), closestDistSqrd));
	closer = temp.GetX() & temp.GetY() & temp.GetZ() & temp.GetW();
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, faceTestClosestOnTri0);
	outPoint2 = SelectFT(closer, outPoint2, pointB1);

	closestVec = Subtract(pointB2, faceTestClosestOnTri1);
	distSqrd = Dot(closestVec, closestVec);
	temp = IsLessThan(Vec4V(Vec3V(V_ZERO), distSqrd), Vec4V(triContain1, closestDistSqrd)) & IsLessThan(Vec4V(triContain1, distSqrd), Vec4V(Vec3V(V_ONE), closestDistSqrd));
	closer = temp.GetX() & temp.GetY() & temp.GetZ() & temp.GetW();
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, faceTestClosestOnTri1);
	outPoint2 = SelectFT(closer, outPoint2, pointB2);
}

__forceinline void geomPoints::FindClosestSegSegToTri (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In pointB1, Vec3V_In pointB2)
{
	// This function is a fairly straightforward check of the closest points between the the segment endpoints and the face of the triangle
	//  and between the segment and the segments formed by the three edges of the triangle. Making for five total computed closest points
	//  to select between.
	// The endpoints calculate their projected barycentric coordinates on the triangle, giving us the resulting closest points on the
	//  triangle's plane as well as a trivial containment test. (To check if that closest point is within the bounds of the triangle.)
	// The segment tests are a nearly direct copy of the above FindClosestSegSegToSeg function. However, most of the work has been
	//  parallelized into vector rather than scalar objects, so we perform all three seg vs seg tests for only slightly more than
	//  just one.
	// Finally the actual closest points are selected by comparing squared distances between the potential pairs.

	// Duplicated work now moved into this helper
	Vec3V v0, v1, segAxis;
	ScalarV triDot11, triDot12, triDot22, invDenom;
	FindClosestSegSegToTriInternalHelper(outPoint1, outPoint2, pointA2, pointA3, pointB1, pointB2, v0, v1, triDot11, triDot12, triDot22, segAxis, invDenom);

	////////////////////////////////////////////////////////////////////////
	// (This is the extra work to handle the case of true intersection)
	// Find intersection with triangle's plane and test that for triangle containment
	Vec3V triNorm = Cross(v0, v1);
	ScalarV tVal = Scale(Negate(Dot(triNorm, pointB1)), Invert(Dot(triNorm, segAxis)));

	Vec3V segMidArm = AddScaled(pointB1, segAxis, tVal);
	ScalarV segDot12 = Dot(v0, segMidArm);
	ScalarV segDot22 = Dot(v1, segMidArm);
	ScalarV segU2 = Scale( SubtractScaled(Scale(triDot22, segDot12), triDot12, segDot22), invDenom);
	ScalarV segV2 = Scale( SubtractScaled(Scale(triDot11, segDot22), triDot12, segDot12), invDenom);

	Vec3V faceTestClosestOnTriMid = AddScaled(Scale(v0, segU2), v1, segV2);
	Vec3V segBary(segU2, segV2, Add(segU2, segV2));

	// An intersection with the triangle's plane that is also within the triangle is a true intersection
	//  and guaranteed to be the closest point.
	VecBoolV temp = IsLessThan(Vec4V(V_ZERO), Vec4V(segBary, tVal)) & IsLessThan(Vec4V(segBary, tVal), Vec4V(V_ONE));
	BoolV closer = temp.GetX() & temp.GetY() & temp.GetZ() & temp.GetW();
	//closestDistSqrd = ScalarV(V_ZERO); // True intersection, cannot get any closer
	outPoint1 = SelectFT(closer, outPoint1, faceTestClosestOnTriMid);
	outPoint2 = SelectFT(closer, outPoint2, faceTestClosestOnTriMid);
}

__forceinline void geomPoints::FindClosestSegSegToTriNoIntersection (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In pointB1, Vec3V_In pointB2)
{
	// Duplicated work now moved into this helper
	Vec3V v0, v1, segAxis;
	ScalarV triDot11, triDot12, triDot22, invDenom;
	FindClosestSegSegToTriInternalHelper(outPoint1, outPoint2, pointA2, pointA3, pointB1, pointB2, v0, v1, triDot11, triDot12, triDot22, segAxis, invDenom);
}

// Note that the way this is used by the AABBTriClosestPoints function we're doing unnecessary work in some cases just to avoid branching
// - When boxVertConditions squared length = 2 -- We've already defined a single box segment and don't need to call this function at all
// - When boxVertConditions squared length = 3 -- We've already defined a single box vert and could avoid the first 5 lines of code (Down to the ///...)
__forceinline void AABBTriHelper_ClosestBoxSegFromTriEdge(Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In triVert, Vec3V_In triArm, Vec3V_In boxExtents, Vec3V_In boxVertConditions)
{
	// Find closest point on tri seg to box center (the origin)
	const ScalarV triArmSqrdInverse = Invert(Dot(triArm, triArm));
	const Vec3V closePtTriSeg = Add(triVert, Scale(triArm, Clamp(Scale(Dot(-triVert, triArm), triArmSqrdInverse), ScalarV(V_ZERO), ScalarV(V_ONE))));
	const Vec3V closeDirTriSeg = SelectFT(IsGreaterThan(closePtTriSeg, Vec3V(V_ZERO)), Vec3V(V_NEGONE), Vec3V(V_ONE));

	// Combine result with previously calculated box vert conditions (Giving priority to existing conditions)
	const Vec3V closeDirSelTriSeg = SelectFT(IsEqual(boxVertConditions, Vec3V(V_ZERO)), boxVertConditions, closeDirTriSeg);
	outPoint1 = Scale(boxExtents, closeDirSelTriSeg);

	//////////////////////////////////////////////////////////////////////////
	// Then find second box vert by finding closest point on tri seg to first box vert and then moving along the box edge which shares the largest relative component
	const Vec3V closeToBoxPtTriSeg = Add(triVert, Scale(triArm, Clamp(Scale(Dot(Subtract(outPoint1, triVert), triArm), triArmSqrdInverse), ScalarV(V_ZERO), ScalarV(V_ONE))));
	const Vec3V boxVertToSeg = Subtract(closeToBoxPtTriSeg, outPoint1);
	// Component-wise mult leaves positive only if towards center
	Vec3V dirTowardsCenterseg = Scale(boxVertToSeg, -closeDirSelTriSeg);

	// Actually need to filter to only the components undefined by the original vert conditions
	// - Properties known about the closest box edge and therefore the above first vert selection was just an arbitrary pick, the next must share the same properties
	// -- BUT...if all were defined then we really do need to allow all possibilities
	dirTowardsCenterseg = SelectFT(IsEqual(boxVertConditions, Vec3V(V_ZERO)) | IsEqual(Vec3V(Dot(boxVertConditions, boxVertConditions)), Vec3V(V_THREE)), Vec3V(V_ZERO), dirTowardsCenterseg);
	
	// Filtering to only the largest component
	const Vec3V dirToCenterMaxCompSeg = Vec3V(Max(dirTowardsCenterseg.GetX(), Max(dirTowardsCenterseg.GetY(), dirTowardsCenterseg.GetZ())));
	// Note the possibility of adding zero - Resulting in the same point and a zero length segment (Caller needs to be able to handle that)
	const VecBoolV AdjacentSelector = IsGreaterThan(dirTowardsCenterseg, Vec3V(V_ZERO)) & IsGreaterThanOrEqual(dirTowardsCenterseg, dirToCenterMaxCompSeg);
	const Vec3V extentsScalar = SelectFT(IsGreaterThan(boxVertToSeg, Vec3V(V_ZERO)), Vec3V(V_NEGTWO), Vec3V(V_TWO));
	outPoint2 = Add(outPoint1, SelectFT(AdjacentSelector, Vec3V(V_ZERO), Scale(boxExtents, extentsScalar)));
}

__forceinline void AABBTriHelper_FindClosestSegSegToSeg3(Vec3V_InOut outPoint0_1, Vec3V_InOut outPoint0_2, Vec3V_In pointA0_1, Vec3V_In pointA0_2, Vec3V_In pointB0_1, Vec3V_In pointB0_2,
														 Vec3V_InOut outPoint1_1, Vec3V_InOut outPoint1_2, Vec3V_In pointA1_1, Vec3V_In pointA1_2, Vec3V_In pointB1_1, Vec3V_In pointB1_2,
														 Vec3V_InOut outPoint2_1, Vec3V_InOut outPoint2_2, Vec3V_In pointA2_1, Vec3V_In pointA2_2, Vec3V_In pointB2_1, Vec3V_In pointB2_2)
{
	const Vec3V u0 = pointA0_2 - pointA0_1;
	const Vec3V v0 = pointB0_2 - pointB0_1;
	const Vec3V w0 = pointA0_1 - pointB0_1;
	const Vec3V u1 = pointA1_2 - pointA1_1;
	const Vec3V v1 = pointB1_2 - pointB1_1;
	const Vec3V w1 = pointA1_1 - pointB1_1;
	const Vec3V u2 = pointA2_2 - pointA2_1;
	const Vec3V v2 = pointB2_2 - pointB2_1;
	const Vec3V w2 = pointA2_1 - pointB2_1;
	const Vec3V a = Vec3V(Dot(u0,u0), Dot(u1,u1), Dot(u2,u2)); // always >= 0
	const Vec3V b = Vec3V(Dot(u0,v0), Dot(u1,v1), Dot(u2,v2));
	const Vec3V c = Vec3V(Dot(v0,v0), Dot(v1,v1), Dot(v2,v2)); // always >= 0
	const Vec3V d = Vec3V(Dot(u0,w0), Dot(u1,w1), Dot(u2,w2));
	const Vec3V e = Vec3V(Dot(v0,w0), Dot(v1,w1), Dot(v2,w2));
	const Vec3V lengthSquaredProduct = Scale(a, c);
	const Vec3V D = lengthSquaredProduct - b * b; // always >= 0
	Vec3V sc, sN, sD = D; // sc = sN / sD, default sD = D >= 0
	Vec3V tc, tN, tD = D; // tc = tN / tD, default tD = D >= 0

	sN = (b*e - c*d);
	tN = (a*e - b*d);

	VecBoolV sN0 = IsLessThan(sN, Vec3V(V_ZERO));
	VecBoolV sNsD = IsGreaterThan(sN, sD);

	// Clamp sN to the line segment
	sN = SelectFT(sN0, SelectFT(sNsD, sN, sD), Vec3V(V_ZERO));
	tN = SelectFT(sN0, SelectFT(sNsD, tN, e + b), e);
	tD = SelectFT(sN0, SelectFT(sNsD, tD, c), c);

	// To call this 'degenerate' isn't quite right - I wouldn't call two capsules that are parallel to each other (or nearly so) degenerate but they
	//   do represent a configuration that requires an alternate method for determining closest points.  This BoolV is what is used for selecting
	//   between those two results.
	// Should this threshold be even smaller?  Possibly.  We really don't want to be too quick to declare the capsules to be parallel but I don't have
	//   any clear basis for reducing this further.  This tolerance (0.0001) means that 1 - cos^2(angle) < 0.0001 => cos^2(angle) < 0.9999 => angle < about 0.5
	//   degrees, which seems about right.
	VecBoolV degenerate = IsLessThan(D, Scale(lengthSquaredProduct, ScalarV(V_FLT_SMALL_4)));
	sN = SelectFT(degenerate, sN, Vec3V(V_ZERO));
	sD = SelectFT(degenerate, sD, Vec3V(V_ONE));
	tN = SelectFT(degenerate, tN, e);
	tD = SelectFT(degenerate, tD, c);

	VecBoolV tN0 = IsLessThan(tN, Vec3V(V_ZERO));
	VecBoolV tNtD = IsGreaterThan(tN, tD);

	tN = SelectFT(tN0, SelectFT(tNtD, tN, tD), Vec3V(V_ZERO)); // Clamp tN to the line segment

	VecBoolV d0 = IsLessThan(-d, Vec3V(V_ZERO));
	VecBoolV da = IsGreaterThan(-d, a);
	Vec3V sNtN0 = SelectFT(d0, SelectFT(da, -d, sD), Vec3V(V_ZERO)); //  sN if tN < 0

	VecBoolV db0 = IsLessThan((-d + b), Vec3V(V_ZERO));
	VecBoolV dba = IsGreaterThan((-d + b), a);
	Vec3V sNtNtD = SelectFT(db0, SelectFT(dba, -d + b, sD), Vec3V(V_ZERO)); //  sN if tN > tD

	sN = SelectFT(tN0, SelectFT(tNtD, sN, sNtNtD), sNtN0);
	sD = SelectFT((tN0 & !d0 & !da) | (tNtD & !db0 & !dba), sD, a);

	// Finally do the division to get sc and tc
	sc = SelectFT(IsLessThan(Abs(sN), Vec3V(V_FLT_SMALL_6)), sN * Invert(sD), Vec3V(V_ZERO));
	tc = SelectFT(IsLessThan(Abs(tN), Vec3V(V_FLT_SMALL_6)), tN * Invert(tD), Vec3V(V_ZERO));

	outPoint0_1 = pointA0_1 + (sc.GetX() * u0);
	outPoint0_2 = pointB0_1 + (tc.GetX() * v0);
	outPoint1_1 = pointA1_1 + (sc.GetY() * u1);
	outPoint1_2 = pointB1_1 + (tc.GetY() * v1);
	outPoint2_1 = pointA2_1 + (sc.GetZ() * u2);
	outPoint2_2 = pointB2_1 + (tc.GetZ() * v2);
}

__forceinline Vec3V_Out TriHelper_GetClosestOnTriangle(Vec3V_In point, Vec3V_In triArm01, Vec3V_In triArm02, Vec3V_In triArm12)
{
	const Vec3V triArm0p = point;
	const Vec3V triArm1p = Subtract(point, triArm01);
	const Vec3V triArm2p = Subtract(point, triArm02);

#if __PS3 // transposed to avoid dot-products

	Vec3V tx;
	Vec3V ty;
	Vec3V tz;
	Transpose3x3(tx, ty, tz, triArm0p, triArm1p, triArm2p);

	const Vec3V d135 = AddScaled(AddScaled(Scale(tx, triArm01.GetX()), ty, triArm01.GetY()), tz, triArm01.GetZ());
	const Vec3V d246 = AddScaled(AddScaled(Scale(tx, triArm02.GetX()), ty, triArm02.GetY()), tz, triArm02.GetZ());

	const ScalarV d1 = d135.GetX();
	const ScalarV d2 = d246.GetX();
	const ScalarV d3 = d135.GetY();
	const ScalarV d4 = d246.GetY();
	const ScalarV d5 = d135.GetZ();
	const ScalarV d6 = d246.GetZ();

	const Vec3V cross = Cross(d135, d246);

#else

	const ScalarV d1 = Dot(triArm01, triArm0p);
	const ScalarV d2 = Dot(triArm02, triArm0p);
	const ScalarV d3 = Dot(triArm01, triArm1p);
	const ScalarV d4 = Dot(triArm02, triArm1p);
	const ScalarV d5 = Dot(triArm01, triArm2p);
	const ScalarV d6 = Dot(triArm02, triArm2p);

	const Vec3V d351(d3, d5, d1);
	const Vec3V d624(d6, d2, d4);
	const Vec3V d513(d5, d1, d3);
	const Vec3V d462(d4, d6, d2);

	const Vec3V cross = SubtractScaled(Scale(d351, d624), d513, d462);

#endif

	const Vec4V d1245(d1, d2, d4, d5);
	const Vec4V d3636(d3, d6, d3, d6);

	const Vec4V s13s26_AB = Subtract(d1245, d3636);
	const Vec3V d12_A(d1, d2, s13s26_AB.GetZ());
	const Vec3V s13s26pAB(s13s26_AB.GetXY(), Add(s13s26_AB.GetZ(), s13s26_AB.GetW()));
	const Vec3V arm01v02w12w = InvScale(d12_A, s13s26pAB);

	const ScalarV denom = Add(cross.GetX(), Add(cross.GetY(), cross.GetZ()));
	const Vec3V crossOverDenom = InvScale(cross, denom);

	// Reverse order - selects overwrite previous values resulting in an effect as if we used the opposite order with early outs
	VecBoolV temp;
	BoolV selector;
	Vec3V closestPoint = AddScaled(Scale(triArm01, crossOverDenom.GetY()), triArm02, crossOverDenom.GetZ());
	const ScalarV vsOne(V_ONE);
	const ScalarV vsZero(V_ZERO);

	temp = IsGreaterThanOrEqual(Vec3V(vsZero, s13s26_AB.GetZW()), Vec3V(cross.GetX(), Vec2V(vsZero)));
	selector = temp.GetX() & temp.GetY() & temp.GetZ();
	closestPoint = SelectFT(selector, closestPoint, AddScaled(triArm01, triArm12, arm01v02w12w.GetZ()));

	temp = IsGreaterThanOrEqual(Vec3V(Vec2V(vsZero), d2), Vec3V(cross.GetY(), d6, vsZero));
	selector = temp.GetX() & temp.GetY() & temp.GetZ();
	closestPoint = SelectFT(selector, closestPoint, Scale(triArm02, arm01v02w12w.GetY()));

	temp = IsGreaterThanOrEqual(Vec2V(d6), Vec2V(d5, vsZero));
	selector = temp.GetX() & temp.GetY();
	closestPoint = SelectFT(selector, closestPoint, triArm02);

	temp = IsGreaterThanOrEqual(Vec3V(Vec2V(vsZero), d1), Vec3V(cross.GetZ(), d3, vsZero));
	selector = temp.GetX() & temp.GetY() & temp.GetZ();
	closestPoint = SelectFT(selector, closestPoint, Scale(triArm01, arm01v02w12w.GetX()));

	temp = IsGreaterThanOrEqual(Vec2V(d3), Vec2V(d4, vsZero));
	selector = temp.GetX() & temp.GetY();
	closestPoint = SelectFT(selector, closestPoint, triArm01);

	temp = IsGreaterThanOrEqual(Vec2V(vsZero), d1245.GetXY());
	selector = temp.GetX() & temp.GetY();
	closestPoint = SelectFT(selector, closestPoint, Vec3V(vsZero));

	return closestPoint;
}

__forceinline Vec3V_Out AABBTriHelper_GetClosestOnAABB(Vec3V_In P, Vec3V_In boxExtents)
{
	return( Clamp(P, -boxExtents, boxExtents) );
}

#if !__DEV
// There is a duplicate version in the .cpp that protects users from the insane asm instruction explosion that occurs when de-optimized
#if RSG_CPU_INTEL
inline
#else
__forceinline
#endif
void geomPoints::FindClosestSegAABBToTri (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA1, Vec3V_In pointA2, Vec3V_In pointA3, Vec3V_In boxExtents)
{
	//////////////////////////////////////////////////////////////////////////
	// This function finds the two globally closest points between a given triangle and 
	//  a box centered at the origin and aligned with the cardinal axes
	// Note that this only works for disjoint shapes! If the triangle penetrates the box correct results are unlikely.
	// The end result is selected from a set of possible solutions:
	// 1. A box vert vs tri face
	// 2. The three tri verts vs the box faces
	// 3. The triangle edges vs the box edges
	//////////////////////////////////////////////////////////////////////////

	// Temps for pair selection
	Vec3V closestVec;
	ScalarV distSqrd;
	BoolV closer;
	ScalarV closestDistSqrd(V_FLT_MAX);

	// Pre-calculate some needed intermediate values
	const Vec3V triV0 = pointA1;
	const Vec3V triV1 = pointA2;
	const Vec3V triV2 = pointA3;
	const Vec3V triArm01 = Subtract(triV1, triV0);
	const Vec3V triArm02 = Subtract(triV2, triV0);
	const Vec3V triArm12 = Subtract(triV2, triV1);
	const Vec3V triNorm = Cross(triArm01, triArm02);

	//////////////////////////////////////////////////////////////////////////
	// 1. A box vert vs the tri face
	//  - Because the triangle lies entirely in a plane, we know from the tri normal only two possible box verts that could be closest to its face
	//  -- Then, by picking the normal direction outward from the box center (the origin) through the triangle we can pick just one and test that

	// Want the tri normal outwards from the box center
	const Vec3V triNormOutFromBox = SelectFT(IsGreaterThan(Dot(triV0, triNorm), ScalarV(V_ZERO)), Negate(triNorm), triNorm);

	// Use that triangle plane to determine which box vert to test from
	//  - This will be the only vert we need to test due to the fact that the triangle lies on a plane
	Vec3V boxVertInDirOfTriClosest = SelectFT(IsGreaterThan(triNormOutFromBox, Vec3V(V_ZERO)), -boxExtents, boxExtents);
	// TODO -- Fixup closestOnTri helper to take input without tri centered at origin
	//  --- Actually pretty unlikely to help. The savings within appear to be greater than the single add and subtract needed out here.
	const Vec3V triClosestToBoxVert = Add(triV0, TriHelper_GetClosestOnTriangle(Subtract(boxVertInDirOfTriClosest, triV0), triArm01, triArm02, triArm12));
	// Extra clamp covers some penetration cases, but not all
	// - This line is unneeded and was only an attempt to work like other special case functions in convexIntersector
	//   Currently keeping it just because it does still help for nearly parallel edges and it only costs us one clamp operation
	boxVertInDirOfTriClosest = AABBTriHelper_GetClosestOnAABB(triClosestToBoxVert, boxExtents);

	closestVec = Subtract(boxVertInDirOfTriClosest, triClosestToBoxVert);
	closestDistSqrd = Dot(closestVec, closestVec);
	outPoint1 = triClosestToBoxVert;
	outPoint2 = boxVertInDirOfTriClosest;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 2. The three tri verts vs the box faces
	//  - This is just a simple projection of triangle vertices onto the box faces

	// All tri verts vs box - Need this to handle small tris with projections contained entirely on one box face
	const Vec3V boxFromTriVert0 = AABBTriHelper_GetClosestOnAABB(triV0, boxExtents);
	const Vec3V boxFromTriVert1 = AABBTriHelper_GetClosestOnAABB(triV1, boxExtents);
	const Vec3V boxFromTriVert2 = AABBTriHelper_GetClosestOnAABB(triV2, boxExtents);

	closestVec = Subtract(boxFromTriVert0, triV0);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, triV0);
	outPoint2 = SelectFT(closer, outPoint2, boxFromTriVert0);

	closestVec = Subtract(boxFromTriVert1, triV1);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, triV1);
	outPoint2 = SelectFT(closer, outPoint2, boxFromTriVert1);

	closestVec = Subtract(boxFromTriVert2, triV2);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, triV2);
	outPoint2 = SelectFT(closer, outPoint2, boxFromTriVert2);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 3. The triangle edges vs the box edges
	//  - This is where we get a little bit crazy. Unfortunately I did not figure out a clean mathematical way to describe why this works, it just seems to in practice.
	//  - We need to test all 12 box edges against all 3 triangle edges, but that would be 36 seg vs seg tests and that's just too slow
	//  - If edge vs edge really is the overall closest then there will only actually be a single seg seg pair that matters, why can't we figure out just that one to test?
	//  - Well, we can't seem to do *that* well; but it seems we *can* figure out just one box seg per tri seg. The insanity that follows is how.
	//
	//  - Basically the whole first part of this is trying to figure out some sort of requirements about the nature of the box segment (or vertex on said segment)
	//  - A set of rules seem to apply:
	//  -- For a given triangle segment in the form (A, B) as points relative to the AABB
	//     and the three cardinal axes defined as:
	//      'onaxis' for the direction parallel to the box segment in question and
	//      'offaxis' for the two other directions (These are actually parallel to the normals to the box planes that form the box seg in question)
	//  -- We classify each tri vert (A and B) into trinary states based on their halfspace positions for each cardinal axis
	//     (-1, 0, or 1) for being outside the box one way, inside, or outside the other way
	//  - If these rules are not met then the box edge in question cannot be the closest (Note that this also runs on the assumption that the two shapes are disjoint)
	//  -- Rule1: Each of A and B must have at least one offaxis flag and together must contain both flags
	//  -- Rule2: A and B's onaxis flags must be different OR 0(zero)
	//  -- Rule3: If an offaxis is shared -> Then the vert with the smaller position(distance from origin) in that axis must also have the second offaxis flag
	//
	//  -- The hard part (and most confusing code to look at) is how we take those three rejective tests and make them constructive such that we can pick a box vert or seg from them.
	//  -- What I came up with is a per tri-segment, per cardinal axis test for each tri-seg endpoint against the other endpoint
	//  --- So for example, in the X-axis testing for the A vert:
	//      (By' != 0 && (Bz != Az || Bz == 0)) || (Bz' != 0 && (By != Ay || By == 0))
	//       -- With Ax = X-axis trinary flags (-1, 0, or 1), etc for other axes
	//       -- Where By' = 0 if (By && Ay) && (Byd < Cyd) -- With Byd = The actual y component of vert B's position
	//
	//  -- After all that we ADD the (unique) results of the two tri-edge endpoints together to form the full tri-edge box requirements (conditions that must be true for the closest box edge)
	//
	//  -- Following the above rules can filter the possible axes down to either a single box vert, an edge, a plane, or nothing. (Although, 'nothing' inherently gives us some information about where those tri verts must lie)
	//  -- From there we need to get a full box edge to test. Obviously if we ended with flag representing a box edge we could just go with that but we don't like branching so we call ClosestBoxSegFromTriEdge anyway
	//  --- That function fills in the holes (zeros) of the box edge conditions we just generated with the octant of the closest point on the tri-seg from the box center (origin)
	//  ---- Which leaves us with a single box vert guaranteed to be one of the verts of the box seg closest to the given tri-seg
	//  --- Finally we need to pick an axis to move along to get from that one box vert to the other belonging to the correct closest box seg
	//  ---- To do that we find the closest point on the tri seg from the first box vert, filter to only the components undefined by the box edge conditions
	//       and pick the maximal component direction
	//
	// The rest is pretty simple -- Three vectorized seg vs seg tests are performed and the triangle point results are projected back onto the box

	// Tri verts classified into box plane halfspaces
	//  -- A box edge cannot possibly be closest feature unless tri verts span its incident box planes
	const Vec3V triHalfClass0 = SelectFT(IsGreaterThan(triV0, boxExtents), SelectFT(IsLessThan(triV0, -boxExtents), Vec3V(V_ZERO), Vec3V(V_NEGONE)), Vec3V(V_ONE));
	const Vec3V triHalfClass1 = SelectFT(IsGreaterThan(triV1, boxExtents), SelectFT(IsLessThan(triV1, -boxExtents), Vec3V(V_ZERO), Vec3V(V_NEGONE)), Vec3V(V_ONE));
	const Vec3V triHalfClass2 = SelectFT(IsGreaterThan(triV2, boxExtents), SelectFT(IsLessThan(triV2, -boxExtents), Vec3V(V_ZERO), Vec3V(V_NEGONE)), Vec3V(V_ONE));
	const Vec3V triClassedV0 = Scale(triV0, triHalfClass0);
	const Vec3V triClassedV1 = Scale(triV1, triHalfClass1);
	const Vec3V triClassedV2 = Scale(triV2, triHalfClass2);
	const Vec3V sharedHalfAdjusted01_0 = SelectFT(IsEqual(triHalfClass0, triHalfClass1) & IsLessThanOrEqual(triClassedV0, triClassedV1), triHalfClass0, Vec3V(V_ZERO));
	const Vec3V sharedHalfAdjusted01_1 = SelectFT(IsEqual(triHalfClass0, triHalfClass1) & IsLessThanOrEqual(triClassedV1, triClassedV0), triHalfClass1, Vec3V(V_ZERO));
	const Vec3V sharedHalfAdjusted02_0 = SelectFT(IsEqual(triHalfClass0, triHalfClass2) & IsLessThanOrEqual(triClassedV0, triClassedV2), triHalfClass0, Vec3V(V_ZERO));
	const Vec3V sharedHalfAdjusted02_2 = SelectFT(IsEqual(triHalfClass0, triHalfClass2) & IsLessThanOrEqual(triClassedV2, triClassedV0), triHalfClass2, Vec3V(V_ZERO));
	const Vec3V sharedHalfAdjusted12_1 = SelectFT(IsEqual(triHalfClass1, triHalfClass2) & IsLessThanOrEqual(triClassedV1, triClassedV2), triHalfClass1, Vec3V(V_ZERO));
	const Vec3V sharedHalfAdjusted12_2 = SelectFT(IsEqual(triHalfClass1, triHalfClass2) & IsLessThanOrEqual(triClassedV2, triClassedV1), triHalfClass2, Vec3V(V_ZERO));
	//
	const Vec3V halfClass0zxy = triHalfClass0.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V halfClass0yzx = triHalfClass0.Get<Vec::Y,Vec::Z,Vec::X>();
	const Vec3V halfClass1zxy = triHalfClass1.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V halfClass1yzx = triHalfClass1.Get<Vec::Y,Vec::Z,Vec::X>();
	const Vec3V halfClass2zxy = triHalfClass2.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V halfClass2yzx = triHalfClass2.Get<Vec::Y,Vec::Z,Vec::X>();
	//
	const Vec3V boxDir01_0 = SelectFT( (!IsEqual(sharedHalfAdjusted01_1.Get<Vec::Y,Vec::Z,Vec::X>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass1zxy, halfClass0zxy) | IsEqual(halfClass1zxy, Vec3V(V_ZERO)))) 
		| (!IsEqual(sharedHalfAdjusted01_1.Get<Vec::Z,Vec::X,Vec::Y>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass1yzx, halfClass0yzx) | IsEqual(halfClass1yzx, Vec3V(V_ZERO)))), 
		Vec3V(V_ZERO), triHalfClass0 );
	const Vec3V boxDir01_1 = SelectFT( (!IsEqual(sharedHalfAdjusted01_0.Get<Vec::Y,Vec::Z,Vec::X>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass0zxy, halfClass1zxy) | IsEqual(halfClass0zxy, Vec3V(V_ZERO)))) 
		| (!IsEqual(sharedHalfAdjusted01_0.Get<Vec::Z,Vec::X,Vec::Y>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass0yzx, halfClass1yzx) | IsEqual(halfClass0yzx, Vec3V(V_ZERO)))), 
		Vec3V(V_ZERO), triHalfClass1 );
	const Vec3V boxDir01 = Add(boxDir01_0, SelectFT(IsEqual(boxDir01_0, boxDir01_1), boxDir01_1, Vec3V(V_ZERO)));
	//
	const Vec3V boxDir02_0 = SelectFT( (!IsEqual(sharedHalfAdjusted02_2.Get<Vec::Y,Vec::Z,Vec::X>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass2zxy, halfClass0zxy) | IsEqual(halfClass2zxy, Vec3V(V_ZERO)))) 
		| (!IsEqual(sharedHalfAdjusted02_2.Get<Vec::Z,Vec::X,Vec::Y>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass2yzx, halfClass0yzx) | IsEqual(halfClass2yzx, Vec3V(V_ZERO)))), 
		Vec3V(V_ZERO), triHalfClass0 );
	const Vec3V boxDir02_2 = SelectFT( (!IsEqual(sharedHalfAdjusted02_0.Get<Vec::Y,Vec::Z,Vec::X>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass0zxy, halfClass2zxy) | IsEqual(halfClass0zxy, Vec3V(V_ZERO)))) 
		| (!IsEqual(sharedHalfAdjusted02_0.Get<Vec::Z,Vec::X,Vec::Y>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass0yzx, halfClass2yzx) | IsEqual(halfClass0yzx, Vec3V(V_ZERO)))), 
		Vec3V(V_ZERO), triHalfClass2 );
	const Vec3V boxDir02 = Add(boxDir02_0, SelectFT(IsEqual(boxDir02_0, boxDir02_2), boxDir02_2, Vec3V(V_ZERO)));
	//
	const Vec3V boxDir12_1 = SelectFT( (!IsEqual(sharedHalfAdjusted12_2.Get<Vec::Y,Vec::Z,Vec::X>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass2zxy, halfClass1zxy) | IsEqual(halfClass2zxy, Vec3V(V_ZERO)))) 
		| (!IsEqual(sharedHalfAdjusted12_2.Get<Vec::Z,Vec::X,Vec::Y>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass2yzx, halfClass1yzx) | IsEqual(halfClass2yzx, Vec3V(V_ZERO)))), 
		Vec3V(V_ZERO), triHalfClass1 );
	const Vec3V boxDir12_2 = SelectFT( (!IsEqual(sharedHalfAdjusted12_1.Get<Vec::Y,Vec::Z,Vec::X>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass1zxy, halfClass2zxy) | IsEqual(halfClass1zxy, Vec3V(V_ZERO)))) 
		| (!IsEqual(sharedHalfAdjusted12_1.Get<Vec::Z,Vec::X,Vec::Y>(), Vec3V(V_ZERO)) & (!IsEqual(halfClass1yzx, halfClass2yzx) | IsEqual(halfClass1yzx, Vec3V(V_ZERO)))), 
		Vec3V(V_ZERO), triHalfClass2 );
	const Vec3V boxDir12 = Add(boxDir12_1, SelectFT(IsEqual(boxDir12_1, boxDir12_2), boxDir12_2, Vec3V(V_ZERO)));

	//
	Vec3V boxVert01_0, boxVert01_1;
	AABBTriHelper_ClosestBoxSegFromTriEdge(boxVert01_0, boxVert01_1, triV0, triArm01, boxExtents, boxDir01);
	Vec3V boxVert02_0, boxVert02_1;
	AABBTriHelper_ClosestBoxSegFromTriEdge(boxVert02_0, boxVert02_1, triV0, triArm02, boxExtents, boxDir02);
	Vec3V boxVert12_0, boxVert12_1;
	AABBTriHelper_ClosestBoxSegFromTriEdge(boxVert12_0, boxVert12_1, triV1, triArm12, boxExtents, boxDir12);

	Vec3V closestOnTri01, closestOnBox01;
	Vec3V closestOnTri02, closestOnBox02;
	Vec3V closestOnTri12, closestOnBox12;
	AABBTriHelper_FindClosestSegSegToSeg3(closestOnTri01, closestOnBox01, triV0, triV1, boxVert01_0, boxVert01_1,
		closestOnTri02, closestOnBox02, triV0, triV2, boxVert02_0, boxVert02_1,
		closestOnTri12, closestOnBox12, triV1, triV2, boxVert12_0, boxVert12_1);

	// Final clamping back to box
	// - Catches some penetration cases
	closestOnBox01 = AABBTriHelper_GetClosestOnAABB(closestOnTri01, boxExtents);
	closestOnBox02 = AABBTriHelper_GetClosestOnAABB(closestOnTri02, boxExtents);
	closestOnBox12 = AABBTriHelper_GetClosestOnAABB(closestOnTri12, boxExtents);

	closestVec = Subtract(closestOnBox01, closestOnTri01);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, closestOnTri01);
	outPoint2 = SelectFT(closer, outPoint2, closestOnBox01);

	closestVec = Subtract(closestOnBox02, closestOnTri02);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, closestOnTri02);
	outPoint2 = SelectFT(closer, outPoint2, closestOnBox02);

	closestVec = Subtract(closestOnBox12, closestOnTri12);
	distSqrd = Dot(closestVec, closestVec);
	closer = IsLessThan(distSqrd, closestDistSqrd);
	closestDistSqrd = SelectFT(closer, closestDistSqrd, distSqrd);
	outPoint1 = SelectFT(closer, outPoint1, closestOnTri12);
	outPoint2 = SelectFT(closer, outPoint2, closestOnBox12);
	//////////////////////////////////////////////////////////////////////////
}
#endif

#if RSG_CPU_INTEL
inline
#else
__forceinline
#endif
BoolV_Out geomPoints::TestAABBAgainstTri(Vec3V_In triV0, Vec3V_In triV1, Vec3V_In triV2, Vec3V_In boxExtents)
{
	VecBoolV separatingAxisFound(V_FALSE);

	Vec3V triArm01 = Subtract(triV1, triV0);
	Vec3V triArm02 = Subtract(triV2, triV0);
	Vec3V triArm12 = Subtract(triV2, triV1);
	Vec3V triNorm = Cross(triArm01, triArm02);
	Vec3V triArmAbs01 = Abs(triArm01);
	Vec3V triArmAbs02 = Abs(triArm02);
	Vec3V triArmAbs12 = Abs(triArm12);

	// In order of first use
	Vec3V boxExtents_xxy = boxExtents.Get<Vec::X,Vec::X,Vec::Y>();
	Vec3V boxExtents_zyz = boxExtents.Get<Vec::Z,Vec::Y,Vec::Z>();
	Vec3V triArmAbs01_xxy = triArmAbs01.Get<Vec::X,Vec::X,Vec::Y>();
	Vec3V triArmAbs01_zyz = triArmAbs01.Get<Vec::Z,Vec::Y,Vec::Z>();
	Vec3V triV0_zxy = triV0.Get<Vec::Z,Vec::X,Vec::Y>();
	Vec3V triV1_zxy = triV1.Get<Vec::Z,Vec::X,Vec::Y>();
	Vec3V triV2_zxy = triV2.Get<Vec::Z,Vec::X,Vec::Y>();
	Vec3V triArm01_zxy = triArm01.Get<Vec::Z,Vec::X,Vec::Y>();
	Vec3V triArmAbs02_xxy = triArmAbs02.Get<Vec::X,Vec::X,Vec::Y>();
	Vec3V triArmAbs02_zyz = triArmAbs02.Get<Vec::Z,Vec::Y,Vec::Z>();
	Vec3V triArm02_zxy = triArm02.Get<Vec::Z,Vec::X,Vec::Y>();
	Vec3V triArmAbs12_xxy = triArmAbs12.Get<Vec::X,Vec::X,Vec::Y>();
	Vec3V triArmAbs12_zyz = triArmAbs12.Get<Vec::Z,Vec::Y,Vec::Z>();
	Vec3V triArm12_zxy = triArm12.Get<Vec::Z,Vec::X,Vec::Y>();

	// Edge-edge axes
	Vec3V projR_y01_z01_x01 = AddScaled(boxExtents_xxy * triArmAbs01_zyz, boxExtents_zyz, triArmAbs01_xxy);
	Vec3V triP0_y01_z01_x01 = SubtractScaled(triV0 * triV1_zxy ,triV0_zxy, triV1);			
	Vec3V triP1_y01_z01_x01 = SubtractScaled(triV2 * triArm01_zxy, triV2_zxy, triArm01);
	separatingAxisFound = IsGreaterThan(Max(-Max(triP0_y01_z01_x01, triP1_y01_z01_x01), Min(triP0_y01_z01_x01, triP1_y01_z01_x01)), projR_y01_z01_x01);

	Vec3V projR_y02_z02_x02 = AddScaled(boxExtents_xxy * triArmAbs02_zyz, boxExtents_zyz, triArmAbs02_xxy);
	Vec3V triP0_y02_z02_x02 = SubtractScaled(triV0 * triV2_zxy, triV0_zxy, triV2);
	Vec3V triP1_y02_z02_x02 = SubtractScaled(triV1 * triArm02_zxy ,triV1_zxy, triArm02);
	separatingAxisFound = separatingAxisFound | IsGreaterThan(Max(-Max(triP0_y02_z02_x02, triP1_y02_z02_x02), Min(triP0_y02_z02_x02, triP1_y02_z02_x02)), projR_y02_z02_x02);

	Vec3V projR_y12_z12_x12 = AddScaled(boxExtents_xxy * triArmAbs12_zyz, boxExtents_zyz, triArmAbs12_xxy);
	Vec3V triP0_y12_z12_x12 = SubtractScaled(triV1 * triV2_zxy, triV1_zxy, triV2);
	Vec3V triP1_y12_z12_x12 = SubtractScaled(triV0 * triArm12_zxy, triV0_zxy, triArm12);
	separatingAxisFound = separatingAxisFound | IsGreaterThan(Max(-Max(triP0_y12_z12_x12, triP1_y12_z12_x12), Min(triP0_y12_z12_x12, triP1_y12_z12_x12)), projR_y12_z12_x12);

	// Box face axes
	// - Actually - This could probably be split into the W of each of the above
	// -- TODO - Figure out if the extra overhead of splitting and packing these into the above Ws is worth the saved comparison, two maxes, and a min
	separatingAxisFound = separatingAxisFound | IsGreaterThan(Max(-Max(triV0, triV1, triV2), Min(triV0, triV1, triV2)), boxExtents);

	// Tri face axis (Can this be rolled up into the W of one of the above?)
	Vec3V triNormAbs = Abs(triNorm);
	BoolV boxTriAreDisjoint = IsGreaterThan(Abs(Dot(triV0, triNorm)), Dot(boxExtents, triNormAbs));

	// Could have skipped this by ORing ***All comparisons into BoolV along the way, but this is worth it just to be easier to debug
	return !(boxTriAreDisjoint | separatingAxisFound.GetX() | separatingAxisFound.GetY() | separatingAxisFound.GetZ());
}

__forceinline BoolV_Out geomPoints::TestAABBAgainstTriFace(Vec3V_In triV0, Vec3V_In triNorm, Vec3V_In boxExtents)
{
	// TODO - See about factoring out the need to generate the triangle normal in the (TestAABBAgainstTri) function above since this one already requires a normalized one
	Vec3V triNormAbs = Abs(triNorm);
	return IsGreaterThan(Abs(Dot(triV0, triNorm)), Dot(boxExtents, triNormAbs));
}

__forceinline Vec3V_Out geomPoints::FindClosestPointPointTriangle(Vec3V_In point, Vec3V_In vertex1, Vec3V_In vertex2)
{
	return TriHelper_GetClosestOnTriangle(point,vertex1,vertex2,Subtract(vertex2,vertex1));
}

__forceinline Vec3V_Out geomPoints::FindClosestPointPointTriangle(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2)
{
	// Transform everything to be relative to vertex 0, get the closest point, then add vertex 0 to the result
	return Add(FindClosestPointPointTriangle(Subtract(point,vertex0),Subtract(vertex1,vertex0),Subtract(vertex2,vertex0)), vertex0);
}
//===================================================================


inline ScalarV_Out geomDistances::DistanceLineToOrigin (Vec3V_In point1, Vec3V_In point1To2)
{
	Vec3V MagIsTwiceTriArea = Cross(point1,point1To2);
	ScalarV TwiceTriAreaSqrd = Dot(MagIsTwiceTriArea, MagIsTwiceTriArea);
	ScalarV point1To2MagSqrd = Dot(point1To2, point1To2);
	ScalarV TriHeightSqrdInverse = InvScale(point1To2MagSqrd, TwiceTriAreaSqrd);
	// We would not need this select if we could enforce two preconditions:
	// 1. point1To2 has a magnitude greater than (near) ZERO
	// 2. The origin is not co-linear with the infinite line represented by point1 and point1To2
	// Number 1 is plausible to require but number 2 would probably be a costly hassle that duplicates the work done here
	// Therefore we're trying to differentiate and provide meaningful results for all of 1, 2, and the regular cases
	// If 2 occurs than TwiceTriAreaSqrd is zero and TriHeightSqrdInverse has divided by zero, but we also know
	//  that zero is the actual correct answer so we can test for that and return a correct result
	// If 1 occurs than both numerator and denominator of TriHeightSqrdInverse are zero which will actually be caught the
	//  same as for 2 and such an answer of zero distance will be sort of correct since an infinite line defined by
	//  co-centric points describes all of 3-space -- But really seems like something we should warn folks about, so we assert
	Assertf(IsGreaterThanAll(point1To2MagSqrd, ScalarV(V_FLT_SMALL_12)) != 0, "Invalid input points for infinite line in DistanceLineToOrigin -- They are co-centric");
	return SelectFT( IsGreaterThan(TwiceTriAreaSqrd, ScalarV(V_FLT_SMALL_12)), ScalarV(V_ZERO), InvSqrt(TriHeightSqrdInverse) );
}


inline ScalarV_Out geomDistances::DistanceLineToPoint (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In target)
{
	// Get the given point on the line relative to the target point.
	Vec3V relPoint1 = Subtract(point1,target);
	return DistanceLineToOrigin(relPoint1,point1To2);
}


inline float geomDistances::Distance2SegToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target)
{
	Vec3V point1V = VECTOR3_TO_VEC3V(point1);
	Vec3V point1To2V = VECTOR3_TO_VEC3V(point1To2);
	Vec3V targetV = VECTOR3_TO_VEC3V(target);

	// Find the closest point on the segment to the target point.
	Vec3V closest = geomPoints::FindClosestPointSegToPoint(point1V, point1To2V, targetV);

	// Return the squared magnitude of that vector.
	return DistSquared(closest, targetV).Getf();
}

inline ScalarV_Out geomDistances::Distance2SegToPointV (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In target)
{
	// Find the closest point on the segment to the target point.
	const Vec3V closest = geomPoints::FindClosestPointSegToPoint(point1, point1To2, target);

	// Return the squared distance to the target
	return DistSquared(closest, target);
}

inline ScalarV_Out geomDistances::Distance2AndTValSegToPointV (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In target, ScalarV_InOut tVal)
{
	// Find the closest point on the segment to the target point.
	const Vec3V closest = geomPoints::FindClosestPointAndTValSegToPoint(point1, point1To2, target, tVal);

	// Return the squared distance to the target
	return DistSquared(closest, target);
}

__forceinline Vec3V_Out DiscTriHelper_GetClosestOnCircle(Vec3V_In Point, Vec3V_In circleCenter, Vec3V_In circleNormal, ScalarV_In circleRadius)
{
	Vec3V circleCenterToPoint = Subtract(Point, circleCenter);
	Vec3V circleCenterToPointProjected = Subtract(circleCenterToPoint, Scale(circleNormal, Dot(circleNormal, circleCenterToPoint)));
	Vec3V circleEdgeAlongToPointDir = Scale(Normalize(circleCenterToPointProjected), circleRadius);
	Vec3V closestOnCircleFromPoint = SelectFT(IsGreaterThan(Dot(circleCenterToPointProjected, circleCenterToPointProjected), Dot(circleEdgeAlongToPointDir, circleEdgeAlongToPointDir)), circleCenterToPointProjected, circleEdgeAlongToPointDir);
	return Add(closestOnCircleFromPoint, circleCenter);
}

//

#if RSG_CPU_INTEL
inline
#else
__forceinline
#endif
void geomDistances::FindClosestSegDiscToTri (Vec3V_InOut outPoint1, Vec3V_InOut outPoint2, Vec3V_In pointA2, 
														   Vec3V_In pointA3, Vec3V_In discCenter, Vec3V_In discNorm, ScalarV_In discRadius)
{
	Vec3V closestOnTri;
	Vec3V closestOnCircle;
	Vec3V triArm01 = pointA2;
	Vec3V triArm02 = pointA3;
	Vec3V triArm12 = Subtract(pointA3, pointA2);

	// First find the intersection of the two planes (we will be starting at a point on the circle closest to this line)
	Vec3V triNorm = NormalizeSafe(Cross(triArm01, triArm02), Vec3V(V_Z_AXIS_WZERO));
	Vec3V intLineDir = Cross(triNorm, discNorm);
	ScalarV denom = Dot(intLineDir, intLineDir);
	BoolV denomIsValid = IsGreaterThanOrEqual(denom, ScalarV(V_FLT_SMALL_6));
	Vec3V intPoint = SelectFT(denomIsValid, discCenter, InvScale(Cross(Negate(Scale(triNorm, Dot(discNorm, discCenter))), intLineDir), denom));
	// Then project circle center onto this line (Also find the intersections of this line with the circle edges, if any)
	Vec3V circleCenterFromIntLine = Subtract(discCenter, intPoint);
	ScalarV circleCenterFromIntLineDotLineDir = Dot(circleCenterFromIntLine, intLineDir);
	ScalarV circleCenterTonIntLine = SelectFT(denomIsValid, ScalarV(V_ZERO), InvScale(circleCenterFromIntLineDotLineDir, denom));
	Vec3V circleCenterHeightOffIntLine = Subtract(circleCenterFromIntLine, Scale(intLineDir, circleCenterTonIntLine));
	ScalarV circleCenterHeightOffIntLineSqrd = Dot(circleCenterHeightOffIntLine, circleCenterHeightOffIntLine);
	ScalarV circleEdgeTdelta = Sqrt(SelectFT(denomIsValid, ScalarV(V_ZERO), InvScale(Clamp( Subtract(Scale(discRadius, discRadius), circleCenterHeightOffIntLineSqrd),ScalarV(V_ZERO),ScalarV(V_FLT_MAX) ), denom)));
	ScalarV circleEdgeTa = Add(circleCenterTonIntLine, circleEdgeTdelta);
	ScalarV circleEdgeTb = Subtract(circleCenterTonIntLine, circleEdgeTdelta);
	Vec3V circleCenterOnIntLine = Add(intPoint, Scale(intLineDir, circleCenterTonIntLine));
	Vec3V circleEdgeOnIntLineA = Add(intPoint, Scale(intLineDir, circleEdgeTa));
	Vec3V circleEdgeOnIntLineB = Add(intPoint, Scale(intLineDir, circleEdgeTb));

	// Then we will select the closest to the triangle to start our iterations
	Vec3V pointOnTri = TriHelper_GetClosestOnTriangle(circleEdgeOnIntLineA, triArm01, triArm02, triArm12);
	Vec3V relVec = Subtract(pointOnTri, circleEdgeOnIntLineA);
	ScalarV closestDistSqrd = Dot(relVec, relVec);
	closestOnTri = pointOnTri;

	pointOnTri = TriHelper_GetClosestOnTriangle(circleEdgeOnIntLineB, triArm01, triArm02, triArm12);
	relVec = Subtract(pointOnTri, circleEdgeOnIntLineB);
	ScalarV distSqrd = Dot(relVec, relVec);
	closestOnTri = SelectFT(IsGreaterThanOrEqual(distSqrd, closestDistSqrd), pointOnTri, closestOnTri);
	closestDistSqrd = SelectFT(IsGreaterThanOrEqual(distSqrd, closestDistSqrd), distSqrd, closestDistSqrd);

	pointOnTri = TriHelper_GetClosestOnTriangle(circleCenterOnIntLine, triArm01, triArm02, triArm12);
	relVec = Subtract(pointOnTri, circleCenterOnIntLine);
	distSqrd = Dot(relVec, relVec);
	// If very close to same distances take the center (Which is why we check this last, and with tolerances)
	closestDistSqrd = Add(closestDistSqrd, ScalarV(V_FLT_SMALL_3));
	closestOnTri = SelectFT(IsGreaterThanOrEqual(distSqrd, closestDistSqrd), pointOnTri, closestOnTri);

	closestOnCircle = DiscTriHelper_GetClosestOnCircle(closestOnTri, discCenter, discNorm, discRadius);

#if __SPU && __ASSERT
	// The unrolling takes too much stack space for SPU in builds with less optimization
	for(int i = 0; i < 3; i++)
	{
		closestOnTri = TriHelper_GetClosestOnTriangle(closestOnCircle, triArm01, triArm02, triArm12);
		closestOnCircle = DiscTriHelper_GetClosestOnCircle(closestOnTri, discCenter, discNorm, discRadius);
	}
#else
	// Unrolled loop of 3 iterations
	closestOnTri = TriHelper_GetClosestOnTriangle(closestOnCircle, triArm01, triArm02, triArm12);
	closestOnCircle = DiscTriHelper_GetClosestOnCircle(closestOnTri, discCenter, discNorm, discRadius);
	closestOnTri = TriHelper_GetClosestOnTriangle(closestOnCircle, triArm01, triArm02, triArm12);
	closestOnCircle = DiscTriHelper_GetClosestOnCircle(closestOnTri, discCenter, discNorm, discRadius);
	closestOnTri = TriHelper_GetClosestOnTriangle(closestOnCircle, triArm01, triArm02, triArm12);
	closestOnCircle = DiscTriHelper_GetClosestOnCircle(closestOnTri, discCenter, discNorm, discRadius);
#endif

	// Done - return
	outPoint1 = closestOnTri;
	outPoint2 = closestOnCircle;
}

// Input is assumed to already be flattened into XY plane
// - However, the only thing that would actually change is the output intersection point
//   Which would become the 3D point of line 1 that intersects the (line2 cross zAxis) halfspace
__forceinline bool geom2D::TestLineToLineFastXY(Vec3V_In start1, Vec3V_In dir1,
								  Vec3V_In start2, Vec3V_In dir2, Vec3V_InOut intersectionPoint)
{
	// We're treating only a 2D portion of the input so we can pull junk like this to make a perpendicular vector
	Vec3V perp2(-dir2.GetY(), dir2.GetX(), ScalarV(V_ZERO));
	Vec3V offset = Subtract(start1, start2);

	ScalarV dir1DotPerp2 = Dot(dir1, perp2);
	// We would have negated the divisor(dir1DotPerp2) in order to get the real T value, so this is negative T
	ScalarV negT = InvScale(Dot(offset, perp2), dir1DotPerp2);

	// Subtract rather than add since the T is negated
	intersectionPoint = Subtract(start1, Scale(dir1, negT));

	// Return FALSE when near zero (indicating parallel lines)
	return( IsBetweenNegAndPosBounds(dir1DotPerp2, ScalarV(V_FLT_EPSILON)) == 0 );
}

// Note that the axes of the output matrix may not be in the same order as the input
__forceinline void geomBoxes::ComputeOrthonormalBasisFromLooseVectors(Vec3V_InOut axis0, Vec3V_InOut axis1, Vec3V_InOut axis2, Vec3V_InOut axisLengths)
{
	// Determine the order of the vectors based on the vector bias
	VecBoolV greaterThanRight = IsGreaterThan(axisLengths, axisLengths.Get<Vec::Y, Vec::Z, Vec::X>());
	VecBoolV greaterOrEqualLeft = InvertBits(greaterThanRight.Get<Vec::Z, Vec::X, Vec::Y>());
	VecBoolV isLargest = And(greaterThanRight, greaterOrEqualLeft);
	isLargest.SetX(!Xor(isLargest.GetY(), isLargest.GetZ())); // Map F_F_F to T_F_F to ensure that one element is always true
	VecBoolV isSmallest = InvertBits(Or(greaterThanRight, greaterOrEqualLeft));
	isSmallest.SetY(!Xor(isSmallest.GetX(), isSmallest.GetZ())); // Map F_F_F to F_T_F to ensure that one element is always true
	VecBoolV isMiddle = InvertBits(Or(isLargest, isSmallest));

	// Calculate which axis is largest, middle, and smallest
	Vec3V largestAxis = (Vec3V(isLargest.GetX()) & axis0) | (Vec3V(isLargest.GetY()) & axis1) | (Vec3V(isLargest.GetZ()) & axis2);
	Vec3V middleAxis = (Vec3V(isMiddle.GetX()) & axis0) | (Vec3V(isMiddle.GetY()) & axis1) | (Vec3V(isMiddle.GetZ()) & axis2);
	Vec3V smallestAxis = (Vec3V(isSmallest.GetX()) & axis0) | (Vec3V(isSmallest.GetY()) & axis1) | (Vec3V(isSmallest.GetZ()) & axis2);

	// Ensure the largest vector is normalized
	largestAxis = Normalize(largestAxis);
	// Overwrite the smallest vector with the normalized cross product of the largest and second largest
	smallestAxis = Normalize(Cross(largestAxis, middleAxis));
	// Overwrite the second largest vector with the cross product of the largest and new smallest vector (They are both orthogonal and normalized, so normalization isn't needed)
	middleAxis = Cross(smallestAxis, largestAxis);

	// Assign the ortho-normalized vectors back out
	axis0 = largestAxis;
	axis1 = middleAxis;
	axis2 = smallestAxis;

	// The lengths need to be permuted to match the end locations of the bases relative to the input order
	axisLengths = Vec3V(Vec3V(isLargest).GetX() & axisLengths.GetX() | Vec3V(isLargest).GetY() & axisLengths.GetY() | Vec3V(isLargest).GetZ() & axisLengths.GetZ(), 
		Vec3V(isMiddle).GetX() & axisLengths.GetX() | Vec3V(isMiddle).GetY() & axisLengths.GetY() | Vec3V(isMiddle).GetZ() & axisLengths.GetZ(),
		Vec3V(isSmallest).GetX() & axisLengths.GetX() | Vec3V(isSmallest).GetY() & axisLengths.GetY() | Vec3V(isSmallest).GetZ() & axisLengths.GetZ() );
}

__forceinline void geomBoxes::ComputeBoxDataFromOppositeDiagonals(Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2, Vec3V_In vert3, Mat34V_InOut mtx, Vec3V_InOut boxSize, ScalarV_InOut maxMargin)
{
#if __ASSERT && !__SPU
	// Check that the vertices actually do specify opposite diagonals.
	const Vec3V diagonalLengths0(Dist(vert0, vert1), Dist(vert0, vert2), Dist(vert0, vert3));
	const Vec3V diagonalLengths1(Dist(vert2, vert3), Dist(vert1, vert3), Dist(vert1, vert2));
	const Vec3V diagonalLengthDiffs(Subtract(diagonalLengths0, diagonalLengths1));
	// We use two different tolerances here and pick the least restrictive one in each case.  A fixed tolerance breaks down when box sizes change.  For example,
	//   a fixed tolerance that adequately catches medium-sized boxes that are sheared would be too restrictive for large boxes and too permissive for small boxes.
	//   A relative tolerance, however, can be very problematic for small boxes where the tolerance would become too tight and, due to precision or quantization,
	//   it would be effectively impossible for any box to be good enough.
	// The error we're getting here is due to quantization. For a 1000m bounding box, 1 quantum is ~1.5cm. This means each vertex can shift by 0.75cm in each direction. 
	//   The worst case would be two vertices move away from each other by the maximum amount, and the other two move towards eachother by the maximum amount. This would
	//   account for a difference in lengths of |quantum|*2, or for a 1000m bounding box, ~5.2cm. This is an absolute worse case, but it should be possible. So far I haven't
	//   found any box where the diagonal lengths are off by more than 2.7cm, so setting the absolute threshold to 6cm should work. Ideally we would calculate the threshold
	//   based on the size of the geometry bound's box, but that is more complicated than necessary just for a debugging function. 
	const Vec3V relativeThreshold = Scale(Scale(ScalarV(V_SIX),ScalarV(V_FLT_SMALL_3)), diagonalLengths0);
	const Vec3V absoluteThreshold = Vec3V(Scale(ScalarV(V_SIX), ScalarV(V_FLT_SMALL_2)));
	const Vec3V finalThreshold = Max(relativeThreshold, absoluteThreshold);
	const bool isOppositeDiagonals = (IsLessThanAll(Abs(diagonalLengthDiffs), finalThreshold) != 0);
	Assertf(isOppositeDiagonals, "Box primitive with specified vertices <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f> is not a valid box!  The vertices specified are not opposite diagonals (%f, %f, %f) / (%f, %f, %f).", vert0.GetXf(), vert0.GetYf(), vert0.GetZf(), vert1.GetXf(), vert1.GetYf(), vert1.GetZf(), vert2.GetXf(), vert2.GetYf(), vert2.GetZf(), vert3.GetXf(), vert3.GetYf(), vert3.GetZf(), diagonalLengths0.GetXf(), diagonalLengths0.GetYf(), diagonalLengths0.GetZf(), diagonalLengths1.GetXf(), diagonalLengths1.GetYf(), diagonalLengths1.GetZf());
#endif	// __ASSERT && !__SPU

	const ScalarV vecHalf(V_HALF);
	const ScalarV vecQuarter(V_QUARTER);

	// TODO: Precision could be slightly improved here by making this of the form (A - B) + (X - Y).  Also there are some common subexpressions.
	Vec3V twiceBoxX = vert1 + vert3 - vert0 - vert2;
	Vec3V twiceBoxY = vert0 + vert3 - vert1 - vert2;
	Vec3V twiceBoxZ = vert2 + vert3 - vert0 - vert1;

	Vec3V twiceBoxSize (MagFast(twiceBoxX), MagFast(twiceBoxY), MagFast(twiceBoxZ));

	// Note that this function now uses entirely inOut parameters to pass the results
	// - This is done to keep the work in vector registers instead of going out to memory
	ComputeOrthonormalBasisFromLooseVectors(twiceBoxX, twiceBoxY, twiceBoxZ, twiceBoxSize);
	mtx.SetCol0(twiceBoxX);
	mtx.SetCol1(twiceBoxY);
	mtx.SetCol2(twiceBoxZ);
	mtx.SetCol3(Scale(vecQuarter, vert0 + vert1 + vert2 + vert3));

	boxSize = Scale(vecHalf, twiceBoxSize);
	maxMargin = vecQuarter * vecHalf * MinElement(boxSize);

#if __ASSERT && !__SPU
	// Check for a degenerate box, in particular, a box with no thickness in at least one direction.
	const bool isValidSizedBox = (IsGreaterThanOrEqualAll(boxSize, Vec3V(V_FLT_SMALL_4)) != 0);
	Assertf(isValidSizedBox, "Box primitive with specified vertices <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f> is not a valid box!  The box size is <%f, %f, %f> which is too small in at least one dimension.  This is probably an art error.", vert0.GetXf(), vert0.GetYf(), vert0.GetZf(), vert1.GetXf(), vert1.GetYf(), vert1.GetZf(), vert2.GetXf(), vert2.GetYf(), vert2.GetZf(), vert3.GetXf(), vert3.GetYf(), vert3.GetZf(), boxSize.GetXf(), boxSize.GetYf(), boxSize.GetZf());
#endif	// __ASSERT && !__SPU
}


#if __XENON || __PS3

#if __XENON
typedef __vector4 bitfield128;
#else
typedef _uvector4 bitfield128;
#endif


inline bool geomBoxes::TestPolygonToOrientedBoxFP (Vector3::Vector3Param v0, Vector3::Vector3Param v1, Vector3::Vector3Param v2,
											Vector3::Vector3Param v3, Vector3::Vector3Param normal, Vector3::Vector3Param boxCenter,
											const Matrix34 &boxAxes, Vector3::Vector3Param boxHalfExtents)
{

	Vector3 displacement;
	// The displacement between the box center and one of the triangle's vertices.
	Vector3 edge0, edge1, edge2;
	// Vectors between the first vertex in the polygon and all other vertices.
	// (edge2 is unused if the polygon is a triangle.)
	Vector3 p0, p1, p2, p3;
	// The projected vertices, and their min/max values.
	Vector3 r, minusR;
	// The oriented bounding box's radius of projection.

	// This test is based on separating axes.  We try the polygon's normal vector and the three
	// coordinate axes.  For completeness, we'd also have to try all 9 or 12 possible cross-products
	// between the box axes and the polygon edges.

	// Calculate the displacement between the box center and one of the triangle's vertices.
	displacement.Subtract (v0, boxCenter);

	// Calculate the edge vectors.
	edge0.Subtract (v1, v0);
	edge1.Subtract (v2, v0);
	edge2.Subtract (v3, v0);

	// Determine whether the polygon's normal is a separating axis.
	p0.DotV (normal,displacement);

	{
		Vector3 normalPrime;
		boxAxes.UnTransform3x3(normal, normalPrime);
		normalPrime.Abs();
		r.DotV(boxHalfExtents,normalPrime);

		//		r = boxHalfExtents[0] * fabsf (normal.Dot (boxAxes.a))
		//			+ boxHalfExtents[1] * fabsf (normal.Dot (boxAxes.b))
		//			+ boxHalfExtents[2] * fabsf (normal.Dot (boxAxes.c));
	}


	bitfield128 comparitor;

	// (p0.x > r.x || p0.x < minusR.x)
	comparitor = __vcmpbfp( p0, r );

	// Determine whether the box's axes are separating axes.
	{
		Vector3 pMin, pMax;

		// "edges" are really relative displacements from p0
		boxAxes.UnTransform3x3(displacement,p0);		

		// do all three axis tests at once.  axis 0 => .x axis 1 => .y, etc.
		boxAxes.UnTransform3x3(edge0,p1);
		boxAxes.UnTransform3x3(edge1,p2);
		boxAxes.UnTransform3x3(edge2,p3);
		p1.Add(p0);
		p2.Add(p0);
		p3.Add(p0);

		pMin.Min(p0,p1);
		pMax.Max(p0,p1);
		pMin.Min(pMin,p2);
		pMax.Max(pMax,p2);
		pMin.Min(pMin,p3);
		pMax.Max(pMax,p3);

		r = boxHalfExtents;
		minusR.Negate(r);

		{
			//(pMin.x > r.x || pMax.x < minusR.x)
			bitfield128 comparitor2, comparitor3;

			comparitor2 = __vcmpgtfp( pMin, r );
			comparitor3 = __vcmpgtfp( minusR, pMax );

			comparitor = __vor( comparitor, comparitor2 );
			comparitor = __vor( comparitor, comparitor3 );
		}
	}

	// mask off the w component so it doesn't factor into the test
	comparitor = __vand( comparitor, bitfield128(Vector3(VEC3_ANDW).xyzw) );

#if __SPU
	return _vequal( comparitor, (bitfield128)(0) ) == 1;
#else
	return _vequal( comparitor, bitfield128(VECTOR4_ORIGIN.xyzw) ) == 1;
#endif

}

inline bool geomBoxes::TestPolygonToOrientedBoxFP (Vector3::Vector3Param v0, Vector3::Vector3Param v1, Vector3::Vector3Param v2,
											Vector3::Vector3Param normal, Vector3::Vector3Param boxCenter,
											const Matrix34 &boxAxes, Vector3::Vector3Param boxHalfExtents)
{

	Vector3 displacement;
	// The displacement between the box center and one of the triangle's vertices.
	Vector3 edge0, edge1;
	// Vectors between the first vertex in the polygon and all other vertices.
	// (edge2 is unused if the polygon is a triangle.)
	Vector3 p0, p1, p2;
	// The projected vertices, and their min/max values.
	Vector3 r, minusR;
	// The oriented bounding box's radius of projection.

	// This test is based on separating axes.  We try the polygon's normal vector and the three
	// coordinate axes.  For completeness, we'd also have to try all 9 or 12 possible cross-products
	// between the box axes and the polygon edges.

	// Calculate the displacement between the box center and one of the triangle's vertices.
	displacement.Subtract (v0, boxCenter);

	// Calculate the edge vectors.
	edge0.Subtract (v1, v0);
	edge1.Subtract (v2, v0);

	// Determine whether the polygon's normal is a separating axis.
	p0.DotV (normal,displacement);

	{
		Vector3 normalPrime;
		boxAxes.UnTransform3x3(normal, normalPrime);
		normalPrime.Abs();
		r.DotV(boxHalfExtents,normalPrime);

		//		r = boxHalfExtents[0] * fabsf (normal.Dot (boxAxes.a))
		//			+ boxHalfExtents[1] * fabsf (normal.Dot (boxAxes.b))
		//			+ boxHalfExtents[2] * fabsf (normal.Dot (boxAxes.c));
	}

	bitfield128 comparitor;
	// (p0.x > r.x || p0.x < minusR.x)
	comparitor = __vcmpbfp( p0, r );

	// Determine whether the box's axes are separating axes.
	{
		Vector3 pMin, pMax;

		// "edges" are really relative displacements from p0
		boxAxes.UnTransform3x3(displacement,p0);		

		// do all three axis tests at once.  axis 0 => .x axis 1 => .y, etc.
		boxAxes.UnTransform3x3(edge0,p1);
		boxAxes.UnTransform3x3(edge1,p2);
		p1.Add(p0);
		p2.Add(p0);

		pMin.Min(p0,p1);
		pMax.Max(p0,p1);
		pMin.Min(pMin,p2);
		pMax.Max(pMax,p2);

		r = boxHalfExtents;
		minusR.Negate(r);

		{
			//(pMin.x > r.x || pMax.x < minusR.x)
			bitfield128 comparitor2, comparitor3;

			comparitor2 = __vcmpgtfp( pMin, r );
			comparitor3 = __vcmpgtfp( minusR, pMax );

			comparitor = __vor( comparitor, comparitor2 );
			comparitor = __vor( comparitor, comparitor3 );
		}
	}

	// mask off the w component so it doesn't factor into the test
	comparitor = __vand( comparitor, bitfield128(Vec4V(V_MASKXYZ).GetIntrin128()) );

#if __SPU
    return _vequal( comparitor, (bitfield128)(0) ) == 1;
#else
	return _vequal( comparitor, bitfield128(Vec3V(V_ZERO).GetIntrin128()) ) == 1;
#endif

}


#endif // __XENON || __PS3

inline void geomBoxes::PrecomputeSegmentToBox(PrecomputedSegmentToBox *tmp, Vec::Vector_4V_In p0, Vec::Vector_4V_In p1)
{
	using namespace rage::Vec;

	const Vector_4V half   = V4VConstant(V_HALF);
	const Vector_4V halfP1 = V4Scale(p1, half);
	const Vector_4V segCenter            = V4AddScaled(halfP1, p0, half);
	const Vector_4V signedSegHalfExtents = V4SubtractScaled(halfP1, p0, half);
	tmp->segCenter            = segCenter;
	tmp->signedSegHalfExtents = signedSegHalfExtents;
	tmp->absSegCenter         = V4Abs(segCenter);
	tmp->absSegHalfExtents    = V4Abs(signedSegHalfExtents);
}

inline int geomBoxes::TestSegmentToBox(const PrecomputedSegmentToBox *tmp, Vec::Vector_4V_In boxCenter, Vec::Vector_4V_In boxHalfExtents)
{
	// Seperating axis test.
	// See Akenine-Moller, Haines. "Real-Time Rendering", 2nd ed. Section 13.6.3.

	using namespace rage::Vec;

	// Translate segment relative to box center.
	Vector_4V segCenter = V4Subtract(tmp->segCenter, boxCenter);
	Vector_4V absSegCenter = V4Abs(segCenter);

	// Test for seperating axis using aabb axes.
	Vector_4V sumHalfExtents = V4Add(tmp->absSegHalfExtents, boxHalfExtents);
	Vector_4V disjoint0 = V4IsGreaterThanV(absSegCenter, sumHalfExtents);

	// Test for seperating axis using segment axes.
	// Almost a cross product, but not quite.
	Vector_4V ahyzx = V4Permute<Y,Z,X,W>(boxHalfExtents);
	Vector_4V ahzxy = V4Permute<Z,X,Y,W>(boxHalfExtents);
	Vector_4V shyzx = V4Permute<Y,Z,X,W>(tmp->absSegHalfExtents);
	Vector_4V shzxy = V4Permute<Z,X,Y,W>(tmp->absSegHalfExtents);
	Vector_4V t = V4Scale(ahyzx, shzxy);
	t = V4AddScaled(t, ahzxy, shyzx);
	Vector_4V crossSegCenterSignedHalfExtent = V3Cross(segCenter, tmp->signedSegHalfExtents);
	Vector_4V disjoint1 = V4IsGreaterThanV(V4Abs(crossSegCenterSignedHalfExtent), t);

#if __XENON || __PS3
	// Fill all the bytes of disjoint with bytes from the first three
	// words of disjoint0 and disjoint1.  Each of these input words must
	// be represented by at least one byte in the output.  Other than
	// that, we don't really care which byte goes where.  Note also that
	// we don't care about the fact that a permute control will be
	// loaded from memory, this function will be called a lot of times,
	// so we should only get one cache miss.
	Vector_4V disjoint = V4BytePermuteTwo<0x00,0x10,0x04,0x14, 0x08,0x18,0x00,0x10, 0x04,0x14,0x08,0x18, 0x00,0x10,0x04,0x14>(disjoint0, disjoint1);
#else
	Vector_4V disjoint = V4Or(disjoint0, disjoint1);
	disjoint = V4Permute<X,Y,Z,Z>(disjoint);
#endif

	return V4IsEqualIntAll(disjoint, V4VConstant(V_ZERO));
}

inline int geomBoxes::TestSegmentToBox(Vec::Vector_4V_In p0, Vec::Vector_4V_In p1, Vec::Vector_4V_In boxCenter, Vec::Vector_4V_In_After3Args boxHalfExtents)
{
	PrecomputedSegmentToBox tmp;
	PrecomputeSegmentToBox(&tmp, p0, p1);
	return TestSegmentToBox(&tmp, boxCenter, boxHalfExtents);
}

inline int geomBoxes::TestSegmentToBoxMinMax(Vec::Vector_4V_In p0, Vec::Vector_4V_In p1, Vec::Vector_4V_In boxMin, Vec::Vector_4V_In_After3Args boxMax)
{
	const Vec::Vector_4V half = Vec::V4VConstant(V_HALF);
	Vec::Vector_4V halfMax = Vec::V4Scale(boxMax, half);
	Vec::Vector_4V boxCenter = Vec::V4AddScaled(halfMax, boxMin, half);
	Vec::Vector_4V boxHalfExtents = Vec::V4SubtractScaled(halfMax, boxMin, half);

	return TestSegmentToBox(p0, p1, boxCenter, boxHalfExtents);
}

#define VERIFY_TEST_BOX_BOX_VECTORIZATION 0

#if __PPU || __XENON || __SPU // Branchless, vector pipeline version


#if __XENON					// Keep old version

inline bool geomBoxes::TestBoxToBoxOBB (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB)
{
#if __PPU || __XENON || __SPU
	// Make sure boxBHalfWidths.w is set to 0.0f before calling TestBoxToBoxOBB.
	FastAssert(Vector3(boxBHalfWidths).w==0.0f);
#endif

	Matrix34 bToA;
    bToA.Transpose(aToB);

    // Compute translation vector t in a's coordinate frame
    Vector3 t;
    aToB.UnTransform3x3(aToB.d, t);
    t.Negate();

#if __PPU || __XENON || __SPU
    t.And(VEC3_ANDW);
#else
    t.w = 0.0f;
#endif

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    Matrix34 abs;
    abs.a.Abs(aToB.a);
    abs.a.Add(VEC3_SMALL_FLOAT);
    abs.b.Abs(aToB.b);
    abs.b.Add(VEC3_SMALL_FLOAT);
    abs.c.Abs(aToB.c);
    abs.c.Add(VEC3_SMALL_FLOAT);

    Matrix34 absTranspose;
    absTranspose.Transpose(abs);

    // Test axes L = A0, L = A1, L = A2
    //for (int i = 0; i < 3; i++) {
    //    ra = a.e[i];
    //    rb = b.e[0] * abs[i][0] + b.e[1] * abs[i][1] + b.e[2] * abs[i][2];
    //    if (Abs(t[i]) > ra + rb) return false;
    //}

    Vector3 rb012;
    abs.UnTransform3x3(boxBHalfWidths, rb012);
    rb012.Add(boxAHalfWidths);

    Vector3 absT;
    absT.Abs(t);

    Vector3 controlV = absT.IsLessThanV(rb012);
    controlV.And(VEC3_ANDW);

    // Test axes L = B0, L = B1, L = B2
    //for (int i = 0; i < 3; i++) {
    //    ra = a.x * abs.GetVector(0)[i] + a.y * abs.GetVector(1)[i] + a.z * abs.GetVector(2)[i];
    //    rb = b[i];
    //    if (Abs(t[0] * aToB.GetVector(0)[i] + t[1] * aToB.GetVector(1)[i] + t[2] * aToB.GetVector(2)[i]) > ra + rb) return false;
    //}

    Vector3 ra012;
    absTranspose.UnTransform3x3(boxAHalfWidths, ra012);
    ra012.Add(boxBHalfWidths);

    Vector3 absTTransformed;
    bToA.UnTransform3x3(t, absTTransformed);
    absTTransformed.Abs();

    Vector3 abttlt = absTTransformed.IsLessThanV(ra012);

    controlV.And(abttlt);

    if (!controlV.IsTrueTrueTrue())
    {
        return false;
    }

    Matrix34 bMat0;

    bMat0.a.Permute<VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Y, VEC_PERM_W>(boxBHalfWidths);
    bMat0.b.Permute<VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X, VEC_PERM_W>(boxBHalfWidths);
    bMat0.c.Permute<VEC_PERM_Y, VEC_PERM_X, VEC_PERM_W, VEC_PERM_W>(boxBHalfWidths);

    Matrix34 bMat;
    bMat.Dot3x3(abs, bMat0);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    // Test axis L = A0 x B0
    float ra0t = boxAHalfWidths.y * abs.GetVector(2)[0] + boxAHalfWidths.z * abs.GetVector(1)[0];
    float rb0t = boxBHalfWidths.y * abs.GetVector(0)[2] + boxBHalfWidths.z * abs.GetVector(0)[1];
    float rt0t = Abs(t[2] * aToB.GetVector(1)[0] - t[1] * aToB.GetVector(2)[0]);
    bool test00 = rt0t < ra0t + rb0t;

    // Test axis L = A0 x B1
    float ra1t = boxAHalfWidths.y * abs.GetVector(2)[1] + boxAHalfWidths.z * abs.GetVector(1)[1];
    float rb1t = boxBHalfWidths.x * abs.GetVector(0)[2] + boxBHalfWidths.z * abs.GetVector(0)[0];
    float rt1t = Abs(t[2] * aToB.GetVector(1)[1] - t[1] * aToB.GetVector(2)[1]);
    bool test01 = rt1t < ra1t + rb1t;

    // Test axis L = A0 x B2
    float ra2t = boxAHalfWidths.y * abs.GetVector(2)[2] + boxAHalfWidths.z * abs.GetVector(1)[2];
    float rb2t = boxBHalfWidths.x * abs.GetVector(0)[1] + boxBHalfWidths.y * abs.GetVector(0)[0];
    float rt2t = Abs(t[2] * aToB.GetVector(1)[2] - t[1] * aToB.GetVector(2)[2]);
    bool test02 = rt2t < ra2t + rb2t;
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    Vector3 t0;
    t0.Permute<VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Y, VEC_PERM_W>(t);
    const Vector3 minusOneZ(1.0f, 1.0f, -1.0f);
    t0.Multiply(minusOneZ);
    Vector3 a0;
    a0.Permute<VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Y, VEC_PERM_W>(boxAHalfWidths);

    Vector3 ra0;
    absTranspose.UnTransform3x3(a0, ra0);

    Vector3 rab0;
    rab0.Add(ra0, bMat.a);

    Vector3 abstt0;
    bToA.UnTransform3x3(t0, abstt0);
    abstt0.Abs();

    Vector3 abttlt0 = abstt0.IsLessThanV(rab0);

    controlV.And(abttlt0);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    FastAssert(fabs(ra0t - ra0.x) <= ra0t * 0.001f);
    FastAssert(fabs(ra1t - ra0.y) <= ra1t * 0.001f);
    FastAssert(fabs(ra2t - ra0.z) <= ra2t * 0.001f);

    FastAssert(fabs(rb0t - bMat.a.x) <= rb0t * 0.001f);
    FastAssert(fabs(rb1t - bMat.a.y) <= rb1t * 0.001f);
    FastAssert(fabs(rb2t - bMat.a.z) <= rb2t * 0.001f);

    FastAssert(fabs(rt0t - abstt0.x) <= rt0t * 0.001f);
    FastAssert(fabs(rt1t - abstt0.y) <= rt1t * 0.001f);
    FastAssert(fabs(rt2t - abstt0.z) <= rt2t * 0.001f);

    // Test axis L = A1 x B0
    float ra0t2 = boxAHalfWidths.x * abs.GetVector(2)[0] + boxAHalfWidths.z * abs.GetVector(0)[0];
    float rb0t2 = boxBHalfWidths.y * abs.GetVector(1)[2] + boxBHalfWidths.z * abs.GetVector(1)[1];
    float rt0t2 = Abs(t[0] * aToB.GetVector(2)[0] - t[2] * aToB.GetVector(0)[0]);
    bool test10 = rt0t2 < ra0t2 + rb0t2;

    // Test axis L = A1 x B1
    float ra1t2 = boxAHalfWidths.x * abs.GetVector(2)[1] + boxAHalfWidths.z * abs.GetVector(0)[1];
    float rb1t2 = boxBHalfWidths.x * abs.GetVector(1)[2] + boxBHalfWidths.z * abs.GetVector(1)[0];
    float rt1t2 = Abs(t[0] * aToB.GetVector(2)[1] - t[2] * aToB.GetVector(0)[1]);
    bool test11 = rt1t2 < ra1t2 + rb1t2;

    // Test axis L = A1 x B2
    float ra2t2 = boxAHalfWidths.x * abs.GetVector(2)[2] + boxAHalfWidths.z * abs.GetVector(0)[2];
    float rb2t2 = boxBHalfWidths.x * abs.GetVector(1)[1] + boxBHalfWidths.y * abs.GetVector(1)[0];
    float rt2t2 = Abs(t[0] * aToB.GetVector(2)[2] - t[2] * aToB.GetVector(0)[2]);
    bool test12 = rt2t2 < ra2t2 + rb2t2;
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    Vector3 t1;
    t1.Permute<VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X, VEC_PERM_W>(t);
    const Vector3 minusOneX(-1.0f, 1.0f, 1.0f);
    t1.Multiply(minusOneX);
    Vector3 a1;
    a1.Permute<VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X, VEC_PERM_W>(boxAHalfWidths);

    Vector3 ra1;
    absTranspose.UnTransform3x3(a1, ra1);

    Vector3 rab1;
    rab1.Add(ra1, bMat.b);

    Vector3 abstt1;
    bToA.UnTransform3x3(t1, abstt1);
    abstt1.Abs();

    Vector3 abttlt1 = abstt1.IsLessThanV(rab1);

    controlV.And(abttlt1);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    FastAssert(fabs(ra0t2 - ra1.x) <= ra0t2 * 0.001f);
    FastAssert(fabs(ra1t2 - ra1.y) <= ra1t2 * 0.001f);
    FastAssert(fabs(ra2t2 - ra1.z) <= ra2t2 * 0.001f);

    FastAssert(fabs(rb0t2 - bMat.b.x) <= rb0t2 * 0.001f);
    FastAssert(fabs(rb1t2 - bMat.b.y) <= rb1t2 * 0.001f);
    FastAssert(fabs(rb2t2 - bMat.b.z) <= rb2t2 * 0.001f);

    FastAssert(fabs(rt0t2 - abstt1.x) <= rt0t2 * 0.001f);
    FastAssert(fabs(rt1t2 - abstt1.y) <= rt1t2 * 0.001f);
    FastAssert(fabs(rt2t2 - abstt1.z) <= rt2t2 * 0.001f);

    // Test axis L = A2 x B0
    float ra0t3 = boxAHalfWidths.x * abs.GetVector(1)[0] + boxAHalfWidths.y * abs.GetVector(0)[0];
    float rb0t3 = boxBHalfWidths.y * abs.GetVector(2)[2] + boxBHalfWidths.z * abs.GetVector(2)[1];
    float rt0t3 = Abs(t[1] * aToB.GetVector(0)[0] - t[0] * aToB.GetVector(1)[0]);
    bool test20 = rt0t3 < ra0t3 + rb0t3;

    // Test axis L = A2 x B1
    float ra1t3 = boxAHalfWidths.x * abs.GetVector(1)[1] + boxAHalfWidths.y * abs.GetVector(0)[1];
    float rb1t3 = boxBHalfWidths.x * abs.GetVector(2)[2] + boxBHalfWidthsb.z * abs.GetVector(2)[0];
    float rt1t3 = Abs(t[1] * aToB.GetVector(0)[1] - t[0] * aToB.GetVector(1)[1]);
    bool test21 = rt1t3 < ra1t3 + rb1t3;

    // Test axis L = A2 x B2
    float ra2t3 = boxAHalfWidths.x * abs.GetVector(1)[2] + boxAHalfWidths.y * abs.GetVector(0)[2];
    float rb2t3 = boxBHalfWidths.x * abs.GetVector(2)[1] + boxBHalfWidths.y * abs.GetVector(2)[0];
    float rt2t3 = Abs(t[1] * aToB.GetVector(0)[2] - t[0] * aToB.GetVector(1)[2]);
    bool test22 = rt2t3 < ra2t3 + rb2t3;
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    Vector3 t2;
    t2.Permute<VEC_PERM_Y, VEC_PERM_X, VEC_PERM_W, VEC_PERM_W>(t);
    const Vector3 minusOneY(1.0f, -1.0f, 1.0f);
    t2.Multiply(minusOneY);
    Vector3 a2;
    a2.Permute<VEC_PERM_Y, VEC_PERM_X, VEC_PERM_W, VEC_PERM_W>(boxAHalfWidths);

    Vector3 ra2;
    absTranspose.UnTransform3x3(a2, ra2);

    Vector3 rab2;
    rab2.Add(ra2, bMat.c);

    Vector3 abstt2;
    bToA.UnTransform3x3(t2, abstt2);
    abstt2.Abs();

    Vector3 abttlt2 = abstt2.IsLessThanV(rab2);

    controlV.And(abttlt2);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    FastAssert(fabs(ra0t3 - ra2.x) <= ra0t3 * 0.001f);
    FastAssert(fabs(ra1t3 - ra2.y) <= ra1t3 * 0.001f);
    FastAssert(fabs(ra2t3 - ra2.z) <= ra2t3 * 0.001f);

    FastAssert(fabs(rb0t3 - bMat.c.x) <= rb0t3 * 0.001f);
    FastAssert(fabs(rb1t3 - bMat.c.y) <= rb1t3 * 0.001f);
    FastAssert(fabs(rb2t3 - bMat.c.z) <= rb2t3 * 0.001f);

    FastAssert(fabs(rt0t3 - abstt2.x) <= rt0t3 * 0.001f);
    FastAssert(fabs(rt1t3 - abstt2.y) <= rt1t3 * 0.001f);
    FastAssert(fabs(rt2t3 - abstt2.z) <= rt2t3 * 0.001f);
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    if (!controlV.IsTrueTrueTrue())
    {
#if VERIFY_TEST_BOX_BOX_VECTORIZATION
        FastAssert(test00 == false || test01 == false || test02 == false ||
            test10 == false || test11 == false || test12 == false ||
            test20 == false || test21 == false || test22 == false);
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

        return false;
    }

    return true;
}

#else		// PS3 common

inline bool geomBoxes::TestBoxToBoxOBB (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB)
{
#if __PPU || __XENON || __SPU
	// Make sure boxBHalfWidths.w is set to 0.0f before calling TestBoxToBoxOBB.
	FastAssert(Vector3(boxBHalfWidths).w==0.0f);
#endif

	Matrix34 bToA;
    bToA.Transpose(aToB);

    // Compute translation vector t in a's coordinate frame
    Vector3 t;
    //aToB.UnTransform3x3(aToB.d, t);
	bToA.Transform3x3(aToB.d,t);
    t.Negate();

#if __PPU || __XENON || __SPU
    t.And(VEC3_ANDW);
#else
    t.w = 0.0f;
#endif

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    Matrix34 abs;
    abs.a.Abs(aToB.a);
    abs.a.Add(VEC3_SMALL_FLOAT);
    abs.b.Abs(aToB.b);
    abs.b.Add(VEC3_SMALL_FLOAT);
    abs.c.Abs(aToB.c);
    abs.c.Add(VEC3_SMALL_FLOAT);

    Matrix34 absTranspose;
    absTranspose.Transpose(abs);

    // Test axes L = A0, L = A1, L = A2
    //for (int i = 0; i < 3; i++) {
    //    ra = a.e[i];
    //    rb = b.e[0] * abs[i][0] + b.e[1] * abs[i][1] + b.e[2] * abs[i][2];
    //    if (Abs(t[i]) > ra + rb) return false;
    //}

    Vector3 rb012;
    //abs.UnTransform3x3(boxBHalfWidths, rb012);
	absTranspose.Transform3x3(boxBHalfWidths, rb012);
    rb012.Add(boxAHalfWidths);

    Vector3 absT;
    absT.Abs(t);

    Vector3 controlV = absT.IsLessThanV(rb012);
    controlV.And(VEC3_ANDW);

    // Test axes L = B0, L = B1, L = B2
    //for (int i = 0; i < 3; i++) {
    //    ra = a.x * abs.GetVector(0)[i] + a.y * abs.GetVector(1)[i] + a.z * abs.GetVector(2)[i];
    //    rb = b[i];
    //    if (Abs(t[0] * aToB.GetVector(0)[i] + t[1] * aToB.GetVector(1)[i] + t[2] * aToB.GetVector(2)[i]) > ra + rb) return false;
    //}

    Vector3 ra012;
    //absTranspose.UnTransform3x3(boxAHalfWidths, ra012);
    abs.Transform3x3(boxAHalfWidths, ra012);
    ra012.Add(boxBHalfWidths);

    Vector3 absTTransformed;
    //bToA.UnTransform3x3(t, absTTransformed);
    aToB.Transform3x3(t, absTTransformed);
    absTTransformed.Abs();

    Vector3 abttlt = absTTransformed.IsLessThanV(ra012);

    controlV.And(abttlt);

    if (!controlV.IsTrueTrueTrue())
    {
        return false;
    }

    Matrix34 bMat0;

    bMat0.a.Permute<VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Y, VEC_PERM_W>(boxBHalfWidths);
    bMat0.b.Permute<VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X, VEC_PERM_W>(boxBHalfWidths);
    bMat0.c.Permute<VEC_PERM_Y, VEC_PERM_X, VEC_PERM_W, VEC_PERM_W>(boxBHalfWidths);

    Matrix34 bMat;
    bMat.Dot3x3(abs, bMat0);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    // Test axis L = A0 x B0
    float ra0t = boxAHalfWidths.y * abs.GetVector(2)[0] + boxAHalfWidths.z * abs.GetVector(1)[0];
    float rb0t = boxBHalfWidths.y * abs.GetVector(0)[2] + boxBHalfWidths.z * abs.GetVector(0)[1];
    float rt0t = Abs(t[2] * aToB.GetVector(1)[0] - t[1] * aToB.GetVector(2)[0]);
    bool test00 = rt0t < ra0t + rb0t;

    // Test axis L = A0 x B1
    float ra1t = boxAHalfWidths.y * abs.GetVector(2)[1] + boxAHalfWidths.z * abs.GetVector(1)[1];
    float rb1t = boxBHalfWidths.x * abs.GetVector(0)[2] + boxBHalfWidths.z * abs.GetVector(0)[0];
    float rt1t = Abs(t[2] * aToB.GetVector(1)[1] - t[1] * aToB.GetVector(2)[1]);
    bool test01 = rt1t < ra1t + rb1t;

    // Test axis L = A0 x B2
    float ra2t = boxAHalfWidths.y * abs.GetVector(2)[2] + boxAHalfWidths.z * abs.GetVector(1)[2];
    float rb2t = boxBHalfWidths.x * abs.GetVector(0)[1] + boxBHalfWidths.y * abs.GetVector(0)[0];
    float rt2t = Abs(t[2] * aToB.GetVector(1)[2] - t[1] * aToB.GetVector(2)[2]);
    bool test02 = rt2t < ra2t + rb2t;
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    Vector3 t0;
    t0.Permute<VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Y, VEC_PERM_W>(t);
    const Vector3 minusOneZ(1.0f, 1.0f, -1.0f);
    t0.Multiply(minusOneZ);
    Vector3 a0;
    a0.Permute<VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Y, VEC_PERM_W>(boxAHalfWidths);

    Vector3 ra0;
    //absTranspose.UnTransform3x3(a0, ra0);
	abs.Transform3x3(a0, ra0);

    Vector3 rab0;
    rab0.Add(ra0, bMat.a);

    Vector3 abstt0;
    //bToA.UnTransform3x3(t0, abstt0);
    aToB.Transform3x3(t0, abstt0);
    abstt0.Abs();

    Vector3 abttlt0 = abstt0.IsLessThanV(rab0);

    controlV.And(abttlt0);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    FastAssert(fabs(ra0t - ra0.x) <= ra0t * 0.001f);
    FastAssert(fabs(ra1t - ra0.y) <= ra1t * 0.001f);
    FastAssert(fabs(ra2t - ra0.z) <= ra2t * 0.001f);

    FastAssert(fabs(rb0t - bMat.a.x) <= rb0t * 0.001f);
    FastAssert(fabs(rb1t - bMat.a.y) <= rb1t * 0.001f);
    FastAssert(fabs(rb2t - bMat.a.z) <= rb2t * 0.001f);

    FastAssert(fabs(rt0t - abstt0.x) <= rt0t * 0.001f);
    FastAssert(fabs(rt1t - abstt0.y) <= rt1t * 0.001f);
    FastAssert(fabs(rt2t - abstt0.z) <= rt2t * 0.001f);

    // Test axis L = A1 x B0
    float ra0t2 = boxAHalfWidths.x * abs.GetVector(2)[0] + boxAHalfWidths.z * abs.GetVector(0)[0];
    float rb0t2 = boxBHalfWidths.y * abs.GetVector(1)[2] + boxBHalfWidths.z * abs.GetVector(1)[1];
    float rt0t2 = Abs(t[0] * aToB.GetVector(2)[0] - t[2] * aToB.GetVector(0)[0]);
    bool test10 = rt0t2 < ra0t2 + rb0t2;

    // Test axis L = A1 x B1
    float ra1t2 = boxAHalfWidths.x * abs.GetVector(2)[1] + boxAHalfWidths.z * abs.GetVector(0)[1];
    float rb1t2 = boxBHalfWidths.x * abs.GetVector(1)[2] + boxBHalfWidths.z * abs.GetVector(1)[0];
    float rt1t2 = Abs(t[0] * aToB.GetVector(2)[1] - t[2] * aToB.GetVector(0)[1]);
    bool test11 = rt1t2 < ra1t2 + rb1t2;

    // Test axis L = A1 x B2
    float ra2t2 = boxAHalfWidths.x * abs.GetVector(2)[2] + boxAHalfWidths.z * abs.GetVector(0)[2];
    float rb2t2 = boxBHalfWidths.x * abs.GetVector(1)[1] + boxBHalfWidths.y * abs.GetVector(1)[0];
    float rt2t2 = Abs(t[0] * aToB.GetVector(2)[2] - t[2] * aToB.GetVector(0)[2]);
    bool test12 = rt2t2 < ra2t2 + rb2t2;
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    Vector3 t1;
    t1.Permute<VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X, VEC_PERM_W>(t);
    const Vector3 minusOneX(-1.0f, 1.0f, 1.0f);
    t1.Multiply(minusOneX);
    Vector3 a1;
    a1.Permute<VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X, VEC_PERM_W>(boxAHalfWidths);

    Vector3 ra1;
    //absTranspose.UnTransform3x3(a1, ra1);
	abs.Transform3x3(a1, ra1);

    Vector3 rab1;
    rab1.Add(ra1, bMat.b);

    Vector3 abstt1;
    //bToA.UnTransform3x3(t1, abstt1);
    aToB.Transform3x3(t1, abstt1);
    abstt1.Abs();

    Vector3 abttlt1 = abstt1.IsLessThanV(rab1);

    controlV.And(abttlt1);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    FastAssert(fabs(ra0t2 - ra1.x) <= ra0t2 * 0.001f);
    FastAssert(fabs(ra1t2 - ra1.y) <= ra1t2 * 0.001f);
    FastAssert(fabs(ra2t2 - ra1.z) <= ra2t2 * 0.001f);

    FastAssert(fabs(rb0t2 - bMat.b.x) <= rb0t2 * 0.001f);
    FastAssert(fabs(rb1t2 - bMat.b.y) <= rb1t2 * 0.001f);
    FastAssert(fabs(rb2t2 - bMat.b.z) <= rb2t2 * 0.001f);

    FastAssert(fabs(rt0t2 - abstt1.x) <= rt0t2 * 0.001f);
    FastAssert(fabs(rt1t2 - abstt1.y) <= rt1t2 * 0.001f);
    FastAssert(fabs(rt2t2 - abstt1.z) <= rt2t2 * 0.001f);

    // Test axis L = A2 x B0
    float ra0t3 = boxAHalfWidths.x * abs.GetVector(1)[0] + boxAHalfWidths.y * abs.GetVector(0)[0];
    float rb0t3 = boxBHalfWidths.y * abs.GetVector(2)[2] + boxBHalfWidths.z * abs.GetVector(2)[1];
    float rt0t3 = Abs(t[1] * aToB.GetVector(0)[0] - t[0] * aToB.GetVector(1)[0]);
    bool test20 = rt0t3 < ra0t3 + rb0t3;

    // Test axis L = A2 x B1
    float ra1t3 = boxAHalfWidths.x * abs.GetVector(1)[1] + boxAHalfWidths.y * abs.GetVector(0)[1];
    float rb1t3 = boxBHalfWidths.x * abs.GetVector(2)[2] + boxBHalfWidthsb.z * abs.GetVector(2)[0];
    float rt1t3 = Abs(t[1] * aToB.GetVector(0)[1] - t[0] * aToB.GetVector(1)[1]);
    bool test21 = rt1t3 < ra1t3 + rb1t3;

    // Test axis L = A2 x B2
    float ra2t3 = boxAHalfWidths.x * abs.GetVector(1)[2] + boxAHalfWidths.y * abs.GetVector(0)[2];
    float rb2t3 = boxBHalfWidths.x * abs.GetVector(2)[1] + boxBHalfWidths.y * abs.GetVector(2)[0];
    float rt2t3 = Abs(t[1] * aToB.GetVector(0)[2] - t[0] * aToB.GetVector(1)[2]);
    bool test22 = rt2t3 < ra2t3 + rb2t3;
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    Vector3 t2;
    t2.Permute<VEC_PERM_Y, VEC_PERM_X, VEC_PERM_W, VEC_PERM_W>(t);
    const Vector3 minusOneY(1.0f, -1.0f, 1.0f);
    t2.Multiply(minusOneY);
    Vector3 a2;
    a2.Permute<VEC_PERM_Y, VEC_PERM_X, VEC_PERM_W, VEC_PERM_W>(boxAHalfWidths);

    Vector3 ra2;
    //absTranspose.UnTransform3x3(a2, ra2);
	abs.Transform3x3(a2, ra2);

    Vector3 rab2;
    rab2.Add(ra2, bMat.c);

    Vector3 abstt2;
    //bToA.UnTransform3x3(t2, abstt2);
    aToB.Transform3x3(t2, abstt2);
    abstt2.Abs();

    Vector3 abttlt2 = abstt2.IsLessThanV(rab2);

    controlV.And(abttlt2);

#if VERIFY_TEST_BOX_BOX_VECTORIZATION
    FastAssert(fabs(ra0t3 - ra2.x) <= ra0t3 * 0.001f);
    FastAssert(fabs(ra1t3 - ra2.y) <= ra1t3 * 0.001f);
    FastAssert(fabs(ra2t3 - ra2.z) <= ra2t3 * 0.001f);

    FastAssert(fabs(rb0t3 - bMat.c.x) <= rb0t3 * 0.001f);
    FastAssert(fabs(rb1t3 - bMat.c.y) <= rb1t3 * 0.001f);
    FastAssert(fabs(rb2t3 - bMat.c.z) <= rb2t3 * 0.001f);

    FastAssert(fabs(rt0t3 - abstt2.x) <= rt0t3 * 0.001f);
    FastAssert(fabs(rt1t3 - abstt2.y) <= rt1t3 * 0.001f);
    FastAssert(fabs(rt2t3 - abstt2.z) <= rt2t3 * 0.001f);
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

    if (!controlV.IsTrueTrueTrue())
    {
#if VERIFY_TEST_BOX_BOX_VECTORIZATION
        FastAssert(test00 == false || test01 == false || test02 == false ||
            test10 == false || test11 == false || test12 == false ||
            test20 == false || test21 == false || test22 == false);
#endif // VERIFY_TEST_BOX_BOX_VECTORIZATION

        return false;
    }

    return true;
}


#endif

#else // Written in basic RAGE Vector3 calls

/*
From Ericson, Real-Time Collision Detection, 2005

=== Section 4.4.1: =============================================================

int TestOBBOBB(OBB &a, OBB &b)
{
    float ra, rb;
    Matrix33 R, AbsR;

    // Compute rotation matrix expressing b in a's coordinate frame
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R[i][j] = Dot(a.u[i], b.u[j]);

    // Compute translation vector t
    Vector t = b.c - a.c;
    // Bring translation into a's coordinate frame
    t = Vector(Dot(t, a.u[0]), Dot(t, a.u[2]), Dot(t, a.u[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            AbsR[i][j] = Abs(R[i][j]) + EPSILON;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a.e[i];
        rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
        if (Abs(t[i]) > ra + rb) return 0;
    }

    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
        rb = b.e[i];
        if (Abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return 0;
    }

    // Test axis L = A0 x B0
    ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
    rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
    if (Abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;

    // Test axis L = A0 x B1
    ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
    rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
    if (Abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;

    // Test axis L = A0 x B2
    ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
    rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
    if (Abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;

    // Test axis L = A1 x B0
    ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
    rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
    if (Abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;

    // Test axis L = A1 x B1
    ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
    rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
    if (Abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;

    // Test axis L = A1 x B2
    ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
    rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
    if (Abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;

    // Test axis L = A2 x B0
    ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
    rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
    if (Abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;

    // Test axis L = A2 x B1
    ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
    rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
    if (Abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;

    // Test axis L = A2 x B2
    ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
    rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
    if (Abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;

    // Since no separating axis found, the OBBs must be intersecting
    return 1;
}
*/

__forceinline bool geomBoxes::TestBoxToBoxOBB (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB)
{
#if __PPU || __XENON || __SPU
	// Make sure boxBHalfWidths.w is set to 0.0f before calling TestBoxToBoxOBB.
	FastAssert(boxBHalfWidths.w==0.0f);
#endif

	// Compute translation vector t in a's coordinate frame
    Vector3 t;
    t.x = -aToB.a.Dot(aToB.d);
    t.y = -aToB.b.Dot(aToB.d);
    t.z = -aToB.c.Dot(aToB.d);

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    Matrix34 abs;
    abs.a.Abs(aToB.a);
    abs.a.Add(VEC3_SMALL_FLOAT);
    abs.b.Abs(aToB.b);
    abs.b.Add(VEC3_SMALL_FLOAT);
    abs.c.Abs(aToB.c);
    abs.c.Add(VEC3_SMALL_FLOAT);

    // Test axes L = A0, L = A1, L = A2
    float raax = boxAHalfWidths.x;
    float rbax = boxBHalfWidths.Dot(abs.a);
    if (Abs(t.x) > raax + rbax) return false;

    float raay = boxAHalfWidths.y;
    float rbay = boxBHalfWidths.Dot(abs.b);
    if (Abs(t.y) > raay + rbay) return false;

    float raaz = boxAHalfWidths.z;
    float rbaz = boxBHalfWidths.Dot(abs.c);
    if (Abs(t.z) > raaz + rbaz) return false;

    // Test axes L = B0, L = B1, L = B2
    float rabx = boxAHalfWidths.x * abs.a.x + boxAHalfWidths.y * abs.b.x + boxAHalfWidths.z * abs.c.x;
    float rbbx = boxBHalfWidths.x;
    if (Abs(t.x * aToB.a.x + t.y * aToB.b.x + t.z * aToB.c.x) > rabx + rbbx) return false;

    float raby = boxAHalfWidths.x * abs.a.y + boxAHalfWidths.y * abs.b.y + boxAHalfWidths.z * abs.c.y;
    float rbby = boxBHalfWidths.y;
    if (Abs(t.x * aToB.a.y + t.y * aToB.b.y + t.z * aToB.c.y) > raby + rbby) return false;

    float rabz = boxAHalfWidths.x * abs.a.z + boxAHalfWidths.y * abs.b.z + boxAHalfWidths.z * abs.c.z;
    float rbbz = boxBHalfWidths.z;
    if (Abs(t.x * aToB.a.z + t.y * aToB.b.z + t.z * aToB.c.z) > rabz + rbbz) return false;

    // Test axis L = A0 x B0
    float ra0t = boxAHalfWidths.y * abs.GetVector(2)[0] + boxAHalfWidths.z * abs.GetVector(1)[0];
    float rb0t = boxBHalfWidths.y * abs.GetVector(0)[2] + boxBHalfWidths.z * abs.GetVector(0)[1];
    float rt0t = Abs(t[2] * aToB.GetVector(1)[0] - t[1] * aToB.GetVector(2)[0]);
    if (rt0t >= ra0t + rb0t) return false;

    // Test axis L = A0 x B1
    float ra1t = boxAHalfWidths.y * abs.GetVector(2)[1] + boxAHalfWidths.z * abs.GetVector(1)[1];
    float rb1t = boxBHalfWidths.x * abs.GetVector(0)[2] + boxBHalfWidths.z * abs.GetVector(0)[0];
    float rt1t = Abs(t[2] * aToB.GetVector(1)[1] - t[1] * aToB.GetVector(2)[1]);
    if (rt1t >= ra1t + rb1t) return false;

    // Test axis L = A0 x B2
    float ra2t = boxAHalfWidths.y * abs.GetVector(2)[2] + boxAHalfWidths.z * abs.GetVector(1)[2];
    float rb2t = boxBHalfWidths.x * abs.GetVector(0)[1] + boxBHalfWidths.y * abs.GetVector(0)[0];
    float rt2t = Abs(t[2] * aToB.GetVector(1)[2] - t[1] * aToB.GetVector(2)[2]);
    if (rt2t >= ra2t + rb2t) return false;

    // Test axis L = A1 x B0
    float ra0t2 = boxAHalfWidths.x * abs.GetVector(2)[0] + boxAHalfWidths.z * abs.GetVector(0)[0];
    float rb0t2 = boxBHalfWidths.y * abs.GetVector(1)[2] + boxBHalfWidths.z * abs.GetVector(1)[1];
    float rt0t2 = Abs(t[0] * aToB.GetVector(2)[0] - t[2] * aToB.GetVector(0)[0]);
    if (rt0t2 >= ra0t2 + rb0t2) return false;

    // Test axis L = A1 x B1
    float ra1t2 = boxAHalfWidths.x * abs.GetVector(2)[1] + boxAHalfWidths.z * abs.GetVector(0)[1];
    float rb1t2 = boxBHalfWidths.x * abs.GetVector(1)[2] + boxBHalfWidths.z * abs.GetVector(1)[0];
    float rt1t2 = Abs(t[0] * aToB.GetVector(2)[1] - t[2] * aToB.GetVector(0)[1]);
    if (rt1t2 >= ra1t2 + rb1t2) return false;

    // Test axis L = A1 x B2
    float ra2t2 = boxAHalfWidths.x * abs.GetVector(2)[2] + boxAHalfWidths.z * abs.GetVector(0)[2];
    float rb2t2 = boxBHalfWidths.x * abs.GetVector(1)[1] + boxBHalfWidths.y * abs.GetVector(1)[0];
    float rt2t2 = Abs(t[0] * aToB.GetVector(2)[2] - t[2] * aToB.GetVector(0)[2]);
    if (rt2t2 >= ra2t2 + rb2t2) return false;

    // Test axis L = A2 x B0
    float ra0t3 = boxAHalfWidths.x * abs.GetVector(1)[0] + boxAHalfWidths.y * abs.GetVector(0)[0];
    float rb0t3 = boxBHalfWidths.y * abs.GetVector(2)[2] + boxBHalfWidths.z * abs.GetVector(2)[1];
    float rt0t3 = Abs(t[1] * aToB.GetVector(0)[0] - t[0] * aToB.GetVector(1)[0]);
    if (rt0t3 >= ra0t3 + rb0t3) return false;

    // Test axis L = A2 x B1
    float ra1t3 = boxAHalfWidths.x * abs.GetVector(1)[1] + boxAHalfWidths.y * abs.GetVector(0)[1];
    float rb1t3 = boxBHalfWidths.x * abs.GetVector(2)[2] + boxBHalfWidths.z * abs.GetVector(2)[0];
    float rt1t3 = Abs(t[1] * aToB.GetVector(0)[1] - t[0] * aToB.GetVector(1)[1]);
    if (rt1t3 >= ra1t3 + rb1t3) return false;

    // Test axis L = A2 x B2
    float ra2t3 = boxAHalfWidths.x * abs.GetVector(1)[2] + boxAHalfWidths.y * abs.GetVector(0)[2];
    float rb2t3 = boxBHalfWidths.x * abs.GetVector(2)[1] + boxBHalfWidths.y * abs.GetVector(2)[0];
    float rt2t3 = Abs(t[1] * aToB.GetVector(0)[2] - t[0] * aToB.GetVector(1)[2]);
    if (rt2t3 >= ra2t3 + rb2t3) return false;

    return true;
}


#endif

#if __PPU || __XENON || __SPU // Branchess, vector pipeline version

__forceinline bool geomBoxes::TestBoxToBoxOBBFaces (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB)
{
#if __PPU || __XENON || __SPU
	// Make sure boxBHalfWidths.w is set to 0.0f before calling TestBoxToBoxOBBFaces.
	FastAssert(Vector3(boxBHalfWidths).w==0.0f);
#endif

	Matrix34 bToA;
    bToA.Transpose(aToB);

    // Compute translation vector t in a's coordinate frame
    Vector3 t;
    aToB.UnTransform3x3(aToB.d, t);
    t.Negate();

#if __PPU || __XENON || __SPU
    t.And(VEC3_ANDW);
#else
    t.w = 0.0f;
#endif

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    Matrix34 abs;
    abs.a.Abs(aToB.a);
    abs.a.Add(VEC3_SMALL_FLOAT);
    abs.b.Abs(aToB.b);
    abs.b.Add(VEC3_SMALL_FLOAT);
    abs.c.Abs(aToB.c);
    abs.c.Add(VEC3_SMALL_FLOAT);

    Matrix34 absTranspose;
    absTranspose.Transpose(abs);

    // Test axes L = A0, L = A1, L = A2
    //for (int i = 0; i < 3; i++) {
    //    ra = a.e[i];
    //    rb = b.e[0] * abs[i][0] + b.e[1] * abs[i][1] + b.e[2] * abs[i][2];
    //    if (Abs(t[i]) > ra + rb) return false;
    //}

    Vector3 rb012;
    abs.UnTransform3x3(boxBHalfWidths, rb012);
    rb012.Add(boxAHalfWidths);

    Vector3 absT;
    absT.Abs(t);

    Vector3 controlV = absT.IsLessThanV(rb012);
    controlV.And(VEC3_ANDW);

    // Test axes L = B0, L = B1, L = B2
    //for (int i = 0; i < 3; i++) {
    //    ra = a.x * abs.GetVector(0)[i] + a.y * abs.GetVector(1)[i] + a.z * abs.GetVector(2)[i];
    //    rb = b[i];
    //    if (Abs(t[0] * aToB.GetVector(0)[i] + t[1] * aToB.GetVector(1)[i] + t[2] * aToB.GetVector(2)[i]) > ra + rb) return false;
    //}

    Vector3 ra012;
    absTranspose.UnTransform3x3(boxAHalfWidths, ra012);
    ra012.Add(boxBHalfWidths);

    Vector3 absTTransformed;
    bToA.UnTransform3x3(t, absTTransformed);
    absTTransformed.Abs();

    Vector3 abttlt = absTTransformed.IsLessThanV(ra012);

    controlV.And(abttlt);

    return controlV.IsTrueTrueTrue();
}

#else // Written in basic RAGE Vector3 calls

__forceinline bool geomBoxes::TestBoxToBoxOBBFaces (Vector3::Param boxAHalfWidths, Vector3::Param boxBHalfWidths, const Matrix34& aToB)
{
#if __PPU || __XENON || __SPU
	// Make sure boxBHalfWidths.w is set to 0.0f before calling TestBoxToBoxOBBFaces.
	FastAssert(boxBHalfWidths.w==0.0f);
#endif

	// Compute translation vector t in a's coordinate frame
    Vector3 t;
    t.x = -aToB.a.Dot(aToB.d);
    t.y = -aToB.b.Dot(aToB.d);
    t.z = -aToB.c.Dot(aToB.d);

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    Matrix34 abs;
    abs.a.Abs(aToB.a);
    abs.a.Add(VEC3_SMALL_FLOAT);
    abs.b.Abs(aToB.b);
    abs.b.Add(VEC3_SMALL_FLOAT);
    abs.c.Abs(aToB.c);
    abs.c.Add(VEC3_SMALL_FLOAT);

    // Test axes L = A0, L = A1, L = A2
    float raax = boxAHalfWidths.x;
    float rbax = boxBHalfWidths.Dot(abs.a);
    if (Abs(t.x) > raax + rbax) return false;

    float raay = boxAHalfWidths.y;
    float rbay = boxBHalfWidths.Dot(abs.b);
    if (Abs(t.y) > raay + rbay) return false;

    float raaz = boxAHalfWidths.z;
    float rbaz = boxBHalfWidths.Dot(abs.c);
    if (Abs(t.z) > raaz + rbaz) return false;

    // Test axes L = B0, L = B1, L = B2
    float rabx = boxAHalfWidths.x * abs.a.x + boxAHalfWidths.y * abs.b.x + boxAHalfWidths.z * abs.c.x;
    float rbbx = boxBHalfWidths.x;
    if (Abs(t.x * aToB.a.x + t.y * aToB.b.x + t.z * aToB.c.x) > rabx + rbbx) return false;

    float raby = boxAHalfWidths.x * abs.a.y + boxAHalfWidths.y * abs.b.y + boxAHalfWidths.z * abs.c.y;
    float rbby = boxBHalfWidths.y;
    if (Abs(t.x * aToB.a.y + t.y * aToB.b.y + t.z * aToB.c.y) > raby + rbby) return false;

    float rabz = boxAHalfWidths.x * abs.a.z + boxAHalfWidths.y * abs.b.z + boxAHalfWidths.z * abs.c.z;
    float rbbz = boxBHalfWidths.z;
    if (Abs(t.x * aToB.a.z + t.y * aToB.b.z + t.z * aToB.c.z) > rabz + rbbz) return false;

    return true;
}

#endif

__forceinline BoolV_Out geomBoxes::TestBoxToBoxOBBFaces(Vec3V_In boxAHalfWidths, Vec3V_In boxBHalfWidths, QuatV_In bOrientationInA, Vec3V_In bPositionInA)
{
	const Vec3V aabbBHalfExtentsInA = ComputeAABBExtentsFromOBB(bOrientationInA, boxBHalfWidths);
	const Vec3V aabbAHalfExtentsInB = ComputeAABBExtentsFromOBB(InvertNormInput(bOrientationInA), boxAHalfWidths);

	const Vec3V aPositionInB = UnTransform(bOrientationInA, bPositionInA);

	const Vec3V absBPositionInA = Abs(bPositionInA);
	const Vec3V absAToBPositionInB = Abs(aPositionInB);
	const VecBoolV intersectingInA = IsGreaterThan(Add(aabbBHalfExtentsInA, boxAHalfWidths), absBPositionInA);
	const VecBoolV intersectingInB = IsGreaterThan(Add(aabbAHalfExtentsInB, boxBHalfWidths), absAToBPositionInB);
	return (intersectingInA.GetX() & intersectingInA.GetY() & intersectingInA.GetZ()) | (intersectingInB.GetX() & intersectingInB.GetY() & intersectingInB.GetZ());
}


__forceinline void geomBoxes::MergeAABBs(Vec3V_In center0, Vec3V_In halfExtents0, Vec3V_In center1, Vec3V_In halfExtents1, Vec3V_InOut localExtentsOut, Vec3V_InOut localCenterOut)
{
	const ScalarV vsHalf(V_HALF);
	const Vec3V aabbMin = Min(Subtract(center0, halfExtents0), Subtract(center1, halfExtents1));
	const Vec3V aabbMax = Max(Add(center0, halfExtents0), Add(center1, halfExtents1));
	localExtentsOut = Scale(Subtract(aabbMax, aabbMin), vsHalf);
	localCenterOut = Scale(Add(aabbMax, aabbMin), vsHalf);
}

__forceinline Vec3V_Out geomBoxes::ComputeAABBExtentsFromOBB(Vec3V_In unitXAxis, Vec3V_In unitYAxis, Vec3V_In unitZAxis, Vec3V_In obbHalfExtents)
{
	const Vec3V absObbX = Scale(obbHalfExtents.GetX(), Abs(unitXAxis));
	const Vec3V absObbY = Scale(obbHalfExtents.GetY(), Abs(unitYAxis));
	const Vec3V absObbZ = Scale(obbHalfExtents.GetZ(), Abs(unitZAxis));
	const Vec3V aabbHalfWidth = absObbX + absObbY + absObbZ;
	return aabbHalfWidth;
}


__forceinline Vec3V_Out geomBoxes::ComputeAABBExtentsFromOBB(Mat33V_In matrix, Vec3V_In halfExtents)
{
	return ComputeAABBExtentsFromOBB(matrix.GetCol0(), matrix.GetCol1(), matrix.GetCol2(), halfExtents);
}


__forceinline Vec3V_Out geomBoxes::ComputeAABBExtentsFromOBB(QuatV_In orientation, Vec3V_In obbHalfExtents)
{
	const Vec3V absObbX = Scale(obbHalfExtents.GetX(), Abs(Transform(orientation, Vec3V(V_X_AXIS_WZERO))));
	const Vec3V absObbY = Scale(obbHalfExtents.GetY(), Abs(Transform(orientation, Vec3V(V_Y_AXIS_WZERO))));
	const Vec3V absObbZ = Scale(obbHalfExtents.GetZ(), Abs(Transform(orientation, Vec3V(V_Z_AXIS_WZERO))));
	const Vec3V aabbHalfWidth = absObbX + absObbY + absObbZ;
	return aabbHalfWidth;
}


__forceinline void geomBoxes::ComputeAABBFromSweptOBB(Mat34V_In current, Mat34V_In last, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut)
{
	const Vec3V curCenter = Transform(current, localCenterInOut);
	const Vec3V lastCenter = Transform(last, localCenterInOut);

	const Vec3V currentHalfWidth = ComputeAABBExtentsFromOBB(current.GetMat33ConstRef(), localExtentsInOut);
	const Vec3V lastHalfWidth = ComputeAABBExtentsFromOBB(last.GetMat33ConstRef(), localExtentsInOut);

	MergeAABBs(curCenter, currentHalfWidth, lastCenter, lastHalfWidth, localExtentsInOut, localCenterInOut);
}


__forceinline void geomBoxes::ComputeAABBFromSweptOBB(QuatV_In currentOrientation, Vec3V_In currentPosition, QuatV_In lastOrientation, Vec3V_In lastPosition, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut)
{
	const Vec3V curCenter = Transform(currentOrientation, currentPosition, localCenterInOut);
	const Vec3V lastCenter = Transform(lastOrientation, lastPosition, localCenterInOut);

	const Vec3V currentHalfWidth = ComputeAABBExtentsFromOBB(currentOrientation, localExtentsInOut);
	const Vec3V lastHalfWidth = ComputeAABBExtentsFromOBB(lastOrientation, localExtentsInOut);

	MergeAABBs(curCenter, currentHalfWidth, lastCenter, lastHalfWidth, localExtentsInOut, localCenterInOut);
}


__forceinline void geomBoxes::ExpandOBBFromMotionOldAndSlow (MAT34V_DECL(current), MAT34V_DECL2(last), Vec3V_Ref localExtentsInOut, Vec3V_Ref localCenterInOut)
{
	// Save locals.
	Mat34V v_current = MAT34V_ARG_GET(current);
	Mat34V v_last = MAT34V_ARG_GET(last);
	Vec3V v_localExtentsInOut = localExtentsInOut;
	Vec3V v_localCenterInOut = localCenterInOut;

    // Make the matrix that transforms from the previous coordinate system to the current coordinate system.
    Mat34V delta;
	UnTransformOrtho( delta, v_current, v_last );

	// Transform the last center position.
	Vec3V lastCenter = Transform( delta, v_localCenterInOut );

    // Make the absolute value transformation matrix, for transforming box extents.
    Mat33V deltaAbs3x3 = delta.GetMat33();
    Abs(deltaAbs3x3, deltaAbs3x3);

    // Get the previous center and extents in the current coordinate system.
    Vec3V lastExtents = Multiply( deltaAbs3x3, v_localExtentsInOut );

    // Find the current minimum and maximum extents in the current coordinate system.
    Vec3V minCurrent, maxCurrent;
    minCurrent = Subtract(v_localCenterInOut, v_localExtentsInOut);
    maxCurrent = Add(v_localCenterInOut, v_localExtentsInOut);

    // Find the previous minimum and maximum extents in the current coordinate system.
    Vec3V minLast, maxLast;
    minLast = Subtract(lastCenter, lastExtents);
    maxLast = Add(lastCenter, lastExtents);

    // Find the entire minimum and maximum extents, from the current and last minimum and maximum.
    Vec3V min, max;
    min = Min(minCurrent, minLast);
    max = Max(maxCurrent, maxLast);

    // Find the center point and the extents of the entire minimum and maximum extents.
    v_localCenterInOut = Average(max, min);
    v_localExtentsInOut = Subtract(max, v_localCenterInOut);

	// Write outputs.
	localCenterInOut = v_localCenterInOut;
	localExtentsInOut = v_localExtentsInOut;
}


__forceinline void geomBoxes::ExpandOBBFromMotion(Mat34V_In relativeMatrix, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut)
{
	// Transform the last center position.
	const Vec3V lastCenter = Transform( relativeMatrix, localCenterInOut );
	const Vec3V lastHalfExtents = geomBoxes::ComputeAABBExtentsFromOBB(relativeMatrix.GetMat33ConstRef(), localExtentsInOut);

	const Vec3V newAABBMin = Min(Subtract(localCenterInOut, localExtentsInOut), Subtract(lastCenter, lastHalfExtents));
	const Vec3V newAABBMax = Max(Add(localCenterInOut, localExtentsInOut), Add(lastCenter, lastHalfExtents));

	localCenterInOut = Average(newAABBMin, newAABBMax);
	localExtentsInOut = Scale(Subtract(newAABBMax, newAABBMin), ScalarV(V_HALF));
}


__forceinline void geomBoxes::ExpandOBBFromMotion(Mat34V_In current, Mat34V_In last, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut)
{
	// Make the matrix that transforms from the previous coordinate system to the current coordinate system.
	Mat34V delta;
	UnTransformOrtho( delta, current, last );

	ExpandOBBFromMotion(delta, localExtentsInOut, localCenterInOut);
}


__forceinline void geomBoxes::ExpandOBBFromMotion(QuatV_In relativeQuat, Vec3V_In relativePosition, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut)
{
	// Transform the last center position.
	const Vec3V lastCenter = Transform(relativeQuat, relativePosition, localCenterInOut );
	const Vec3V lastHalfExtents = geomBoxes::ComputeAABBExtentsFromOBB(relativeQuat, localExtentsInOut);

	const Vec3V newAABBMin = Min(Subtract(localCenterInOut, localExtentsInOut), Subtract(lastCenter, lastHalfExtents));
	const Vec3V newAABBMax = Max(Add(localCenterInOut, localExtentsInOut), Add(lastCenter, lastHalfExtents));

	localCenterInOut = Average(newAABBMin, newAABBMax);
	localExtentsInOut = Scale(Subtract(newAABBMax, newAABBMin), ScalarV(V_HALF));
}


__forceinline void geomBoxes::ExpandOBBFromMotion(QuatV_In currentOrientation, Vec3V_In currentPosition, QuatV_In lastOrientation, Vec3V_In lastPosition, Vec3V_InOut localExtentsInOut, Vec3V_InOut localCenterInOut)
{
	// Make the matrix that transforms from the previous coordinate system to the current coordinate system.
	QuatV relativeOrientation;
	Vec3V relativePosition;
	UnTransform(relativeOrientation, relativePosition, currentOrientation, currentPosition, lastOrientation, lastPosition);

	ExpandOBBFromMotion(relativeOrientation, relativePosition, localExtentsInOut, localCenterInOut);
}

__forceinline bool geomBoxes::TestAABBtoAABB( Vector3::Vector3Param minA, Vector3::Vector3Param maxA, Vector3::Vector3Param minB, Vector3::Vector3Param maxB )
{
/*	u32 r, r2;
	Vector3(minA).IsGreaterThanVR( maxB, r );
	Vector3(minB).IsGreaterThanVR( maxA, r2 );
	return (((r | r2) & 0x80) >> 7) == 0;*/

#if RSG_CPU_INTEL || __PSP2
	return !( 
		minA.x > maxB.x ||
		minA.y > maxB.y ||
		minA.z > maxB.z ||
		minB.x > maxA.x ||
		minB.y > maxA.y ||
		minB.z > maxA.z
		);
#else

#if __PS3

	return vec_all_ge(__vand(maxB, VEC3_ANDW), __vand(minA, VEC3_ANDW)) && vec_all_ge(__vand(maxA, VEC3_ANDW), __vand(minB, VEC3_ANDW));

#else
	u32 r = 0, r2 = 0;
	__vcmpgefpR(__vand(maxB, VEC3_ANDW), __vand(minA, VEC3_ANDW), &r );
	__vcmpgefpR(__vand(maxA, VEC3_ANDW), __vand(minB, VEC3_ANDW), &r2 );
	return ((r & r2) & (1 << 7)) != 0;
#endif

#endif
}

__forceinline BoolV_Out geomBoxes::TestAABBtoAABB_CenterHalfSize( Vec3V_In centerA, Vec3V_In halfSizeA, Vec3V_In centerB, Vec3V_In halfSizeB )
{
	// The boxes intersect if their combined half size is greater than the absolute distance between the centers in each dimension
	VecBoolV intersectOnAxis = IsGreaterThanOrEqual(Add(halfSizeA,halfSizeB),Abs(Subtract(centerA,centerB)));
	return intersectOnAxis.GetX() & intersectOnAxis.GetY() & intersectOnAxis.GetZ();
}

inline bool geomBoxes::TestSphereToAABB(Vec::V4Param128 sphereCenterRad, Vec::V3Param128 boxMin, Vec::V3Param128 boxMax)
{
	// Regression test code is commented out.
	// Feel free to remove it completely, when comfortable. /wpfeil

	//bool oldVal;
	bool newVal;

	//{
	//	// OLD CODE
	//	// Based on the code in spdSphere::Intersects, but using vector members instead of intrinsics
	//	Vector4 one = VECTOR4_IDENTITY;
	//	Vector4 radiusV = Vector4(sphereCenterRad).GetWV();
	//	Vector4 radiusV2 = radiusV * radiusV;
	//
	//	Vector4 zero; zero.Zero();
	//
	//	Vector4 distMin = Vector4(boxMin) - Vector4(sphereCenterRad);
	//	Vector4 distMax = Vector4(sphereCenterRad) - Vector4(boxMax);
	//
	//	Vector4 dists;
	//	dists.Max(distMin, distMax);
	//	dists.Max(dists, zero);
	//
	//	Vector4 dists2 = dists * dists;
	//
	//	Vector4 dmin = one.Dot3V(dists2); // add all the distances
	//
	//	oldVal = dmin.IsLessThan(radiusV2);
	//}
	//{
		// NEW CODE

		Vec4V v_sphereCenterRad(sphereCenterRad);
		Vec3V v_boxMin(boxMin);
		Vec3V v_boxMax(boxMax);

		ScalarV one(V_ONE);
		ScalarV radiusV = Vec4V(sphereCenterRad).GetW();
		ScalarV radiusV2 = radiusV * radiusV;

		Vec3V zero(V_ZERO);

		Vec3V distMin = v_boxMin - v_sphereCenterRad.GetXYZ();
		Vec3V distMax = v_sphereCenterRad.GetXYZ() - v_boxMax;

		Vec3V dists;
		dists = Max(distMin, distMax);
		dists = Max(dists, zero);

		Vec3V dists2 = dists * dists;

		ScalarV dmin = Dot( Vec3V(one), dists2 ); // add all the distances

		newVal = IsLessThanAll( dmin, radiusV2 ) != 0;
	//}

	//Assert( newVal == oldVal );

	return newVal;
}

__forceinline BoolV_Out geomBoxes::TestSphereToAABB_CenterHalfSize(Vec3V_In sphereCenter, ScalarV_In sphereRadius, Vec3V_In boxCenter, Vec3V_In boxHalfSize)
{
	// There is an intersection if the squared radius is greater than the squared distance between the sphere center and the box.
	ScalarV sphereRadiusSquared = Scale(sphereRadius,sphereRadius);
	Vec3V absCenterDifference = Abs(Subtract(sphereCenter,boxCenter));
	Vec3V absClosestPointOnBox = Min(absCenterDifference,boxHalfSize);
	return IsGreaterThanOrEqual(sphereRadiusSquared, DistSquared(absCenterDifference,absClosestPointOnBox));
}


inline int geomSegments::SegmentToSphereIntersections (Vec3V_In centerToOne, Vec3V_In oneToTwo, ScalarV_In sphereRadius2, ScalarV_InOut segmentT1, ScalarV_InOut segmentT2, bool directed)
{

	ScalarV oneToTwoMag2 = MagSquared(oneToTwo);
	if (IsGreaterThanAll(oneToTwoMag2,ScalarV(V_ZERO)))
	{
		// The segment does not have zero length.
		ScalarV cToOneMag2 = MagSquared(centerToOne);
		ScalarV cToOneDotOneToTwo = Dot(centerToOne,oneToTwo);
		ScalarV cToOneDotOneToTwo2 = Scale(cToOneDotOneToTwo,cToOneDotOneToTwo);
		ScalarV rad2MinusCenterToLine2Scaled = Add(Scale(oneToTwoMag2,Subtract(sphereRadius2,cToOneMag2)),cToOneDotOneToTwo2);
		if (IsLessThanAll(rad2MinusCenterToLine2Scaled,ScalarV(V_ZERO)))
		{
			// The segment's infinite line does not intersect the sphere.
			return 0;
		}

		// The segment's infinite line intersects the sphere. See if the segment starts inside the sphere.
		if (!IsGreaterThanAll(cToOneMag2,sphereRadius2))
		{
			// The segment starts inside there sphere.
			if (directed)
			{
				// The directed segment starts inside the sphere, so there is no intersection.
				return 0;
			}
			
			// See if the segment ends inside the sphere.
			ScalarV centerToTwo = MagSquared(Add(centerToOne,oneToTwo));
			if (!IsGreaterThanAll(centerToTwo,sphereRadius2) || IsLessThanAll(oneToTwoMag2,ScalarV(V_FLT_SMALL_12)))
			{
				// The whole segment is inside the sphere, so there are no intersections with the sphere surface.
				return 0;
			}

			// The segment starts inside the sphere and ends outside.
			// There is one intersection. Find where it exits the sphere.
			segmentT1 = InvScale(Subtract(Sqrt(rad2MinusCenterToLine2Scaled),cToOneDotOneToTwo),oneToTwoMag2);
			//segmentT1 = (sqrtf(rad2MinusCenterToLine2Scaled)-cToOneDotOneToTwo)/oneToTwoMag2;
			return 1;
		}

		// The segment starts outside the sphere and its infinite line touches the sphere, so find where it enters the sphere.
		ScalarV invOneToTwoMag2 = InvScale(ScalarV(V_ONE),oneToTwoMag2);
		ScalarV enterT = Negate(Scale(Add(Sqrt(rad2MinusCenterToLine2Scaled),cToOneDotOneToTwo),invOneToTwoMag2));
		if (IsLessThanAll(enterT,ScalarV(V_ZERO)) || IsGreaterThanAll(enterT,ScalarV(V_ONE)))
		{
			// The whole segment is outside the sphere, so there are no intersections.
			return 0;
		}

		segmentT1 = enterT;
		if (!directed)
		{
			// The segment enters the sphere, so find where it exits the sphere.
			ScalarV exitT = Scale(Subtract(Sqrt(rad2MinusCenterToLine2Scaled),cToOneDotOneToTwo),invOneToTwoMag2);
			if (!IsGreaterThanAll(exitT,ScalarV(V_ONE)))
			{
				// The segment enters and exits the sphere.
				segmentT2 = exitT;
				return 2;
			}
		}

		// The segment starts outside and ends inside the sphere, so there is only one intersection.
		return 1;
	}

	// The segment has zero length, so it does not intersect the sphere's surface.
	return 0;
}


inline int geomSegments::SegmentToSphereIntersections (Vec3V_In sphereCenter, Vec3V_In segmentA, Vec3V_In segmentB, ScalarV_In sphereRadius, ScalarV_InOut segmentT1, ScalarV_InOut segmentT2, bool directed)
{

	Vec3V centerToOne = Subtract(segmentA,sphereCenter);
	Vec3V oneToTwo = Subtract(segmentB,segmentA);
	return SegmentToSphereIntersections(centerToOne,oneToTwo,Scale(sphereRadius,sphereRadius),segmentT1,segmentT2,directed);
}


__forceinline BoolV_Out IsSegmentTriangleResultValid(Vec3V_In v1, Vec3V_In v2, Vec3V_In ptOnTriPlane, ScalarV_In tVal)
{
	// 10^-3 = 1.812 degrees
	// 10^-4 = 0.573 degrees
	// 10^-5 = 0.1812 degrees
	const ScalarV collinearTolerance = ScalarV(V_FLT_SMALL_5);
	const ScalarV vOne(V_ONE);

	ScalarV triDot11 = Dot(v1, v1);
	ScalarV triDot12 = Dot(v1, v2);
	ScalarV triDot22 = Dot(v2, v2);
	ScalarV magV1V2 = Scale(triDot11,triDot22);
	ScalarV oneMinusCosSquared = Subtract(vOne, InvScale(Scale(triDot12,triDot12),magV1V2));

	// Calculate barycentric coordinates of the ray plane intersection point
	ScalarV ptDot1 = Dot(v1, ptOnTriPlane);
	ScalarV ptDot2 = Dot(v2, ptOnTriPlane);
	ScalarV invDenom = Invert(Scale(magV1V2, oneMinusCosSquared));
	ScalarV ptU2 = Scale( Subtract(Scale(triDot22, ptDot1), Scale(triDot12, ptDot2)), invDenom);
	ScalarV ptV2 = Scale( Subtract(Scale(triDot11, ptDot2), Scale(triDot12, ptDot1)), invDenom);

	Vec4V ptBaryWt(ptU2, ptV2, Add(ptU2, ptV2), tVal);
	// Composite comparison telling us if the ray/plane intersection point is contained within both the triangle and the segment
	// Use an extremely small tolerance to prevent probes slipping through cracks. In the original case the error was ~10^-9
	const Vec4V barycentricToleranceMask = And(Vec4V(V_FLT_SMALL_6),Vec4V(V_MASKXYZ));
	VecBoolV ptContained = And( IsLessThanOrEqual(Negate(barycentricToleranceMask), ptBaryWt), IsLessThanOrEqual(ptBaryWt, Add(Vec4V(vOne),barycentricToleranceMask)) );
	BoolV nonCollinearTriangle = IsGreaterThan(oneMinusCosSquared, collinearTolerance);
	return( ptContained.GetX() & ptContained.GetY() & ptContained.GetZ() & ptContained.GetW() & nonCollinearTriangle);
}



inline bool geomSegments::SegmentTriangleIntersectDirected (	Vec3V_In start, Vec3V_In startToEnd, Vec3V_In end,
																Vec3V_In normal, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2,
																ScalarV_InOut fractionAlongSegment)
{
	Vec3V v1 = Subtract(vertex1, vertex0);
	Vec3V v2 = Subtract(vertex2, vertex0);
	Vec3V segStart = Subtract(start, vertex0);
	Vec3V segEnd = Subtract(end, vertex0);

	return( SegmentTriangleIntersectDirected(segStart, startToEnd, segEnd, normal, v1, v2, fractionAlongSegment).Getb() );
}

__forceinline BoolV_Out geomSegments::SegmentTriangleIntersectDirected (	Vec3V_In segStart, Vec3V_In startToEnd, Vec3V_In segEnd,
																			Vec3V_In triNorm, Vec3V_In v1, Vec3V_In v2,
																			ScalarV_InOut fractionAlongSegment)
{
	// Just call the undirected version and make sure the segment end is behind the triangle
	const ScalarV calculatedDepth = -Dot(segEnd, triNorm);
	return And(SegmentTriangleIntersectUndirected(segStart,startToEnd,triNorm,v1,v2,fractionAlongSegment), IsGreaterThan(calculatedDepth, ScalarV(V_ZERO)));
}


inline bool geomSegments::SegmentTriangleIntersectUndirected (	Vec3V_In start, Vec3V_In startToEnd,
																Vec3V_In normal, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, 
																ScalarV_InOut fractionAlongSegment)
{
	Vec3V v1 = Subtract(vertex1, vertex0);
	Vec3V v2 = Subtract(vertex2, vertex0);
	Vec3V segStart = Subtract(start, vertex0);

	return( SegmentTriangleIntersectUndirected(segStart, startToEnd, normal, v1, v2, fractionAlongSegment).Getb() );
}

// This version assumes that vertex0 of the triangle is at the origin - If not, your input will need shifting
__forceinline BoolV_Out geomSegments::SegmentTriangleIntersectUndirected (	Vec3V_In segStart, Vec3V_In startToEnd,
																			Vec3V_In triNorm, Vec3V_In v1, Vec3V_In v2, 
																			ScalarV_InOut fractionAlongSegment)
{
	Assertf(IsLessThanAll(Abs(Subtract(Mag(triNorm),ScalarV(V_ONE))),ScalarVFromF32(0.03f)),"Normal is not normalized"
		"\n\tNormal: %f, %f, %f, |%f|"
		"\n\tvertex 1: %f, %f, %f"
		"\n\tvertex 2: %f, %f, %f",
		triNorm.GetXf(),triNorm.GetYf(),triNorm.GetZf(),Mag(triNorm).Getf(),
		v1.GetXf(),v1.GetYf(),v1.GetZf(),
		v2.GetXf(),v2.GetYf(),v2.GetZf());

	// Find intersection of the segment's ray with the triangle's plane
	ScalarV tVal = Scale(Dot(triNorm, segStart), Invert(Dot(triNorm, -startToEnd)));
	Vec3V ptOnTriPlane = Add(segStart, Scale(startToEnd, tVal));

	// We calculate and use these values locally before setting the return values
	//  this is to avoid a LHS because the compiler isn't sure if they're aliased (since they are InOut params)
	fractionAlongSegment = tVal;
	return IsSegmentTriangleResultValid(v1,v2,ptOnTriPlane,tVal);
}


inline ScalarV_Out geomTValues::FindTValueSegToOrigin (Vec3V_In point1, Vec3V_In point1To2)
{
	// Get the part of the segment pointing directly toward the origin, scaled by the distance to point1.
	ScalarV inwardSegment = -Dot(point1To2,point1);

	// Make a flag to tell whether the segment is pointing toward the origin.
	ScalarV zero = ScalarV(V_ZERO);
	BoolV towardOrigin = IsGreaterThan(inwardSegment,zero);

	// Make a flag to tell whether the segment passes the origin (true if the closest point on the segment's infinite line to the origin is within the segment).
	ScalarV segLength2 = MagSquared(point1To2);
	BoolV passesOrigin = IsGreaterThan(segLength2,inwardSegment);

	// Set the t-value to 1 if the segment does not pass the origin; otherwise compute the fraction of the distance along the segment.
	ScalarV tValue = SelectFT(passesOrigin,ScalarV(V_ONE),Scale(inwardSegment,Invert(segLength2)));

	// Set the t-value to 0 if the segment does not point toward the origin.
	tValue = SelectFT(towardOrigin,zero,tValue);

	// Return the fraction of the distance along the segment that is closest to the origin (0 to 1).
	return tValue;
}


inline ScalarV_Out geomTValues::FindTValueSegToPoint (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint)
{
	// Translate the segment so that the effective target is the origin.
	Vec3V targetTo1 = Subtract(point1,targetPoint);

	// Find and return the closest point on the translated segment to the origin.
	return FindTValueSegToOrigin(targetTo1,point1To2);
}

__forceinline BoolV_Out geomTValues::RealQuadraticFirstRoot(ScalarV_In a, ScalarV_In halfB, ScalarV_In c, ScalarV_InOut root)
{
	ScalarV inverseA = Invert(a);
	ScalarV discriminant = Subtract(Scale(halfB, halfB), Scale(a,c));
	ScalarV discriminantRoot = Sqrt(discriminant);
	root = Scale(Subtract(Negate(halfB), discriminantRoot), inverseA);
	return And(IsGreaterThanOrEqual(discriminant, ScalarV(V_ZERO)), IsGreaterThan(Abs(a), ScalarV(V_FLT_SMALL_12)));
}

__forceinline BoolV_Out geomTValues::RealQuadraticRoots(ScalarV_In a, ScalarV_In halfB, ScalarV_In c, ScalarV_InOut root1, ScalarV_InOut root2)
{
	ScalarV inverseA = Invert(a);
	ScalarV negativeHalfB = Negate(halfB);
	ScalarV discriminant = Subtract(Scale(halfB, halfB), Scale(a,c));
	ScalarV discriminantRoot = Sqrt(discriminant);
	root1 = Scale(Subtract(negativeHalfB, discriminantRoot), inverseA);
	root2 = Scale(     Add(negativeHalfB, discriminantRoot), inverseA);
	return And(IsGreaterThanOrEqual(discriminant, ScalarV(V_ZERO)), IsGreaterThan(Abs(a), ScalarV(V_FLT_SMALL_12)));
}

}	// namespace rage

#endif // VECTOR_GEOMETRY_H
