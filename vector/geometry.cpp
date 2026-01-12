//
// vector/geometry.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "math/channel.h"
#include "math/simplemath.h"
#include "system/typeinfo.h"
#include "vector/geometry.h"
#include "vector/matrix34.h"
#include "vectormath/legacyconvert.h"

#include <string.h>

using namespace rage;

#define		FAST_GEOMETRY		1

void geomSpheres::ComputeBoundSphere (int numverts,const float verts[], u32 stride, Vector4& sphere)
{
    float rad;
    Vector3 center;
    geomBoxes::ComputeBoundInfo(numverts,verts,stride,NULL,NULL,&center,&rad);
    sphere.Set(center.x,center.y,center.z,rad);
}


int geomSpheres::FindImpactPolygonToSphere (const Vector3& sphereCenter, float sphereRadius, const Vector3* vertices,
											int numVertices, const Vector3& polyNormal, Vector3& spherePosition,
											Vector3& polyPosition, int& idNum, Vector3& normal, float& depth)
{
	Vector3 polySide;
		// A vector from the first vertex in an edge to the second vertex in an edge.
	Vector3 edgeNormal;
		// The edge normal, i.e. a vector that points into the polygon, perpendicular
		// to the edge & the polygon's normal vector.
		// (Note that, most of the time, edge-normals are defined to point outside the
		// polygon. This one doesn't.)
	Vector3 vert1ToCenter;
		// A vector from the first vertex in an edge, to the sphere center.  Alternately,
		// one could consider this the location of the sphere center, in the first vertex's
		// "coordinate system".
	Vector3 vert2ToCenter;
		// A vector from the second vertex in an edge, to the sphere center.  Alternately,
		// one could consider this the location of the sphere center, in the second vertex's
		// "coordinate system".
	float distanceSphereCenterToPlane;
		// The distance from the sphere center to the polygon's plane.  (This is calculated
		// near the beginning, as a "quick reject", but the value is needed later.)
	float distance;
		// Other distances that get calculated along the way.
	int index, nextIndex = 0;
		// The index of the first & second vertices in an edge, respectively.
	int insideThisEdge[4];
		// For each edge of the polygon, this tracks where the sphere center is relative
		// to the edge.  -1 means the first vertex is closest to the sphere center, -2
		// means the second vertex is closest to the sphere center, and other values
		// mean that neither vertex is the closest polygon feature to the sphere center.
	bool allInside=true;
		// True if the sphere's center is inside of all of the triangle's edges.

	// If the sphere doesn't intersect with the triangle's plane, we can stop now.
	// But if it does intersect, we may need that distance later.
	vert1ToCenter.Subtract(sphereCenter,vertices[0]);
	distanceSphereCenterToPlane=polyNormal.Dot(vert1ToCenter);
	if(fabsf(distanceSphereCenterToPlane)>sphereRadius)
	{
		return GEOM_NO_IMPACT;
	}

	// Determine where the sphere center is, relative to the polygon's edges.
	for(index=0;index<numVertices;index++)
	{
		// Calculate the index of the other vertex in the edge about to be tested.
		nextIndex = (index+1)%numVertices;

		// Calculate the location of the sphere center, in the first vertex's
		// "coordinate system".
		vert1ToCenter.Subtract(sphereCenter,vertices[index]);

		// Generate a vector that represents the current edge of the polygon.
		polySide.Subtract(vertices[nextIndex],vertices[index]);

		// Generate the edge's normal vector.  (It doesn't need to be normalized,
		// so don't pay for the square root or the division.)
		edgeNormal.Cross(polyNormal,polySide);

		// Initially, we don't know the relation between this edge and the sphere.
		insideThisEdge[index]=0;

		// Is the sphere center behind this edge?
		if(edgeNormal.Dot(vert1ToCenter)>=0.0f)
		{
			// The point on the polygon's plane closest to the sphere center is inside this edge.
			insideThisEdge[index]=1;
			continue;
		}

		// The point on the plane closest to the sphere center is outside this edge.
		// That means the projection of the sphere center onto the triangle's plane is
		// not inside the triangle.
		allInside=false;

		// Is the sphere center behind the edge?  (That is, if the sphere center was
		// projected onto an axis defined by the current edge, would it occur earlier
		// on the axis than the first vertex.  Another way to look at this is, if a
		// plane was constructed that lied on the first vertex and whose normal vector
		// was the edge vector, would the sphere center be behind that plane.)
		if(polySide.Dot(vert1ToCenter)<0.0f)
		{
			// Yes.  That means the closest point on the edge to the sphere is the
			// first vertex.
			insideThisEdge[index]=-1;		// -1 means vertex1 is the closest edge point
			continue;
		}

		// Is the sphere center in front of the edge?  (That is, if the sphere center was
		// projected onto an axis defined by the current edge, would it occur later
		// on the axis than the second vertex.  Another way to look at this is, if a
		// plane was constructed that lied on the second vertex and whose normal vector
		// was the edge vector, would the sphere center be in front of that plane.)
		vert2ToCenter.Subtract(sphereCenter,vertices[nextIndex]);
		if(polySide.Dot(vert2ToCenter)>0.0f)
		{
			// Yes.  That means the closest point on the edge to the sphere is the
			// second vertex.
			insideThisEdge[index]=-2;		// -2 means vertex2 is the closest edge point
			continue;
		}

		// The closest point on this edge to the sphere center is not a vertex and
		// the closest plane point to the sphere center is outside this edge, so
		// a point on the interior of this edge is the closest polygon point to the
		// sphere center.  See if the sphere center is on the correct side of the edge, relative
		// to the polygon normal.
		if (polyNormal.Dot(vert1ToCenter)<=0.0f)
		{
			continue;
		}

		// Form a triangle with the 2 points of this edge & the sphere center.  Then,
		// calculate the height of the triangle from the point of view of the polygon's
		// edge being the base.  The area of a triangle is (base * height / 2); the area
		// of a triangle can also be calculated from the lengths of the sides using
		// Heron's formula.  Combine the two, and one can calculate the height of a
		// triangle from the length of the sides.  (Here, though, we calculate the square
		// of the height from the square of the lengths of the sides.)
		float a2=vert1ToCenter.Mag2();
		float b2=vert2ToCenter.Mag2();
		float c2=polySide.Mag2();
		float inverseC2=1.0f/c2;
		float distance2=0.25f*(2.0f*(a2+b2)-c2-square(a2-b2)*inverseC2);
		if (distance2>square(sphereRadius))
		{
			// The whole polygon is outside the sphere.
			return GEOM_NO_IMPACT;
		}

		// The polygon intersects the sphere, at this edge.
		// We had the distance squared; we need the distance.
		distance = SqrtfSafe(distance2);

		// This edge point is inside the sphere.
		idNum = index;		// idnum refers to the edge number in the polygon
		depth = sphereRadius-distance;

		// Calculate the location of the impact on the edge.  We do this by first
		// calculating the lerp, along polySide, of the point on the edge closest to
		// the sphere.  The lerp is calculated with the Pythagorean theorem.
		b2=(a2-distance2)*inverseC2;	// using b2 as a temp here.
		if(b2>0.0f)
		{
			polyPosition.Scale(polySide,sqrtf(b2));
		}
		else
		{
			// Roundoff errors put the impact on a vertex even though the edge test put it on an edge,
			// so put it on the first vertex.
			polyPosition.Zero();
		}
		polyPosition.Add(vertices[index]);

		// Calculate the normal vector of the collision.  That's a vector from the impact point
		// to the center of the sphere, normalized.
		normal.Subtract(sphereCenter,polyPosition);
		if (distance>VERY_SMALL_FLOAT)
		{
			normal.InvScale(distance);
		}
		else
		{
			normal.Set(polyNormal);
		}

		// Calculate the location of the impact on the sphere.  Start at the location of the impact
		// on the edge, and move, in the direction of the normal, the distance of the penetration
		// depth.
		spherePosition.SubtractScaled(polyPosition,normal,depth);

		// Let our caller know that an edge collided with the sphere.
		return GEOM_EDGE;
	}

	// If the sphere's center, projected onto the triangle's plane, is inside all the edges,
	// then the sphere is colliding with the plane of the polygon, i.e. not with any of
	// the edges or the vertices, but with the flat "middle" part.
	if(allInside)
	{
		// The closest polygon point to the sphere center is inside the polygon.

		// There's no "part number" for the entire polygon, so we use zero.
		idNum = 0;

		// The penetration depth is the radius of the sphere minus the distance from the
		// sphere to the plane.  Simple enough.  But if the sphere is colliding with the
		// back side of the polygon, this leads to a depth that's larger than the sphere
		// radius, and a really big impulse.  This is what we want, though -- we want
		// objects on the back side of polygonal data to get "sucked through" to the
		// front side.
		depth = sphereRadius-distanceSphereCenterToPlane;

		// Calculate the locations of the impact, on the sphere and on the polygon.
		polyPosition.SubtractScaled(sphereCenter,polyNormal,distanceSphereCenterToPlane);
		spherePosition.SubtractScaled(sphereCenter,polyNormal,sphereRadius);

		// Since we're impacting with the polygon, use its normal for the collision.
		normal.Set(polyNormal);

		// Let our caller know the sphere collided with the entire polygon.
		return GEOM_POLYGON;
	}

	// Find the closest vertex.
	for (index=0;index<numVertices;index++)
	{
		// Get the index number of the next vertex.
		nextIndex = (index+1)%numVertices;

		// See if the sphere center is closest to the second vertex on this edge and no the second vertex on
		// the next edge, or closest to the first vertex on the next edge and not the first vertex on this edge.
		if ((insideThisEdge[index]==-2 || insideThisEdge[nextIndex]==-1) &&
			insideThisEdge[index]!=insideThisEdge[nextIndex])		//lint !e644 
		{
			break;
		}
	}

	if (index==numVertices)
	{
		// The sphere doesn't hit any vertex.
		return GEOM_NO_IMPACT;
	}

	// The impact point is the vertex itself.
	polyPosition.Set(vertices[nextIndex]);

	// The impact normal points from the vertex toward the sphere center.
	normal.Subtract(sphereCenter,polyPosition);

	// Only now can we check to see if the colliding vertex is actually in the
	// sphere.  If it's not, there was no impact.
	distance = normal.Mag2();
	if(distance>square(sphereRadius))
	{
		return GEOM_NO_IMPACT;
	}

	// OK, now we're sure the vertex collided with the sphere.  The index number
	// we pass back to our caller is the vertex number.
	idNum = nextIndex;

	// Scale the normal to be unit length.
	if (distance>VERY_SMALL_FLOAT)
	{
		distance=sqrtf(distance);
		normal.InvScale(distance);
	}
	else
	{
		distance = 0.0f;
		normal.Set(polyNormal);
	}

	// The penetration depth is the radius of the sphere, minus the distance between
	// the sphere center & the vertex.  In other words, it's the distance from the
	// vertex to the sphere's surface, measured along the line from the vertex to the
	// sphere's center.
	depth = sphereRadius-distance;

	// The impact point on the sphere is on the surface, along the impact normal that
	// we already calculated.
	spherePosition.SubtractScaled(polyPosition,normal,depth);

	// Let our caller know the sphere collided with a vertex of the polygon.
	return GEOM_VERTEX;
}


bool geomSpheres::TestSphereToPolygon (const Vector3& center, float radius, int numVerts, const Vector3** verts)
{
	mthAssertf(numVerts>=3, "Invalid polygon (verts=%d)", numVerts);
	Vector3 polygonNormal,edge;
	polygonNormal.Subtract(*verts[1],*verts[0]);
	edge.Subtract(*verts[2],*verts[0]);
	polygonNormal.Cross(edge);
	polygonNormal.Normalize();
	return geomSpheres::TestSphereToPolygon(center,radius,numVerts,verts,polygonNormal);
}


bool geomSpheres::TestSphereToPolygon (const Vector3& center, float radius, int numVerts, const Vector3** verts,
										 const Vector3& polygonNormal)
{
	mthAssertf(numVerts<=4, "TestSphereToPolygon only works with 3 or 4 verts, not %d", numVerts);		//lint !e506 constant value boolean
	int i;
	int insideThisEdge[4];
	Vector3 vert1ToCenter, vert2ToCenter, polygonSide, edgeNormal;
	float a2, b2, c2, dist, dist2;
	bool allInside=true, allOutside=true;

	if (!geomPoints::IsPointNearPlane(center,*(verts[0]),polygonNormal,radius))
	{
		return false;
	}

	for (i=0; i<numVerts; i++)
	{
		vert1ToCenter.Subtract(center,*(verts[i]));
		polygonSide.Subtract(*(verts[(i+1)%numVerts]),*(verts[i]));
		edgeNormal.Cross(polygonNormal,polygonSide);

		if(edgeNormal.Dot(vert1ToCenter)>=0.0f)
		{
			// the point on the plane closest to the sphere center is inside this edge
			insideThisEdge[i]=1;
			allOutside=false;
			continue;
		}
		// the point on the plane closest to the sphere center is outside this edge
		insideThisEdge[i]=0;
		allInside=false;
		if(polygonSide.Dot(vert1ToCenter)<0.0f)
		{
			insideThisEdge[i]=-1;			// -1 means vertex1 is the closest edge point
			continue;
		}
		vert2ToCenter.Subtract(center,*(verts[(i+1)%numVerts]));
		if(polygonSide.Dot(vert2ToCenter)>0.0f)
		{
			insideThisEdge[i]=-2;			// -2 means vertex2 is the closest edge point
			continue;
		}
		// the closest point on this edge to the sphere center is not a vertex and
		// the closest plane point to the sphere center is outside this edge, so
		// a point on this edge is the closest polygon point to the sphere center
		a2=vert1ToCenter.Mag2();
		b2=vert2ToCenter.Mag2();
		c2=polygonSide.Mag2();
		dist2=0.25f*(2.0f*(a2+b2)-c2-square(a2-b2)/c2);
		if(dist2>radius*radius)
		{
			// the whole polygon is outside the sphere
			return false;
		}
		// this edge point is inside the sphere
		return true;
	}

	if (allInside || allOutside)
	{
		// the closest polygon point to the sphere center is inside the polygon
		// dist will be the distance from the sphere center to the polygon's plane
		vert1ToCenter.Subtract(center,*(verts[0]));
		dist=polygonNormal.Dot(vert1ToCenter);
		if(fabsf(dist)>radius)
		{
			return false;
		}
		return true;
	}

	// Find the closest vertex.
	int nextI;
	for (i=0;i<numVerts;i++)
	{
		// Get the index number of the next vertex.
		nextI = (i+1)%numVerts;

		// See if the sphere center is closest to the second vertex on this edge and no the second vertex on
		// the next edge, or closest to the first vertex on the next edge and not the first vertex on this edge.
		if ((insideThisEdge[i]==-2 || insideThisEdge[nextI]==-1) &&
			insideThisEdge[i]!=insideThisEdge[nextI])		//lint !e644 
		{
			break;
		}
	}

	if (i==numVerts)
	{
		// The sphere doesn't hit any vertex.
		return false;
	}

	return (verts[(i+1)%numVerts]->Dist2(center) <= radius*radius);
}	//lint !e818 suggestion on verts to be declared as pointing to const

#define TEST_SPHERE_TO_TRIANGLE_TRANSPOSED (1)//__PS3)
#define TEST_SPHERE_TO_TRIANGLE_TEST (0 && __DEV && TEST_SPHERE_TO_TRIANGLE_TRANSPOSED)

bool geomSpheres::TestSphereToTriangle(Vec4V_In sphere, const Vec3V triangle[3], Vec3V_In triangleNormal)
{
#if !__SPU && __ASSERT
	// Area calculation
	const Vec3V edge01 = triangle[1] - triangle[0];
	// const Vec3V edge12 = triangle[2] - triangle[1];
	const Vec3V edge20 = triangle[0] - triangle[2];
	const Vec3V edgeCross = Cross(edge01, edge20);
	float triArea = Mag(edgeCross).Getf()*0.5f;

	const ScalarV magSqr = MagSquared(triangleNormal);
	Assertf(IsLessThanAll(Abs(magSqr - ScalarV(V_ONE)), ScalarV(V_FLT_SMALL_4)), "geomSpheres::TestSphereToTriangleFast triangle normal is %f,%f,%f (mag = %f, triangle area = %f) update B*1406709", VEC3V_ARGS(triangleNormal), sqrtf(magSqr.Getf()), triArea);
#endif

	const Vec3V A = triangle[0] - sphere.GetXYZ();
	const Vec3V B = triangle[1] - sphere.GetXYZ();
	const Vec3V C = triangle[2] - sphere.GetXYZ();
	const ScalarV rr = (sphere*sphere).GetW();
//	const Vec3V V = Cross(B - A, C - A);
//	const ScalarV vv = Dot(V, V);
	const ScalarV av = Dot(A, triangleNormal);//V);
	const BoolV sep1 = av*av > rr;//*vv; // sphere-plane test (assuming 'triangleNormal' is unit length)

#if TEST_SPHERE_TO_TRIANGLE_TRANSPOSED
	// this code is originally from http://realtimecollisiondetection.net/blog/?p=103
	// but modified to operate on transposed vectors to avoid dot-products and scalar ops
	Vec3V Ax_Bx_Cx;
	Vec3V Ay_By_Cy;
	Vec3V Az_Bz_Cz;
	Transpose3x3(Ax_Bx_Cx, Ay_By_Cy, Az_Bz_Cz, A, B, C);
	const Vec3V Bx_Cx_Ax = Ax_Bx_Cx.Get<Vec::Y,Vec::Z,Vec::X>();
	const Vec3V By_Cy_Ay = Ay_By_Cy.Get<Vec::Y,Vec::Z,Vec::X>();
	const Vec3V Bz_Cz_Az = Az_Bz_Cz.Get<Vec::Y,Vec::Z,Vec::X>();
	const Vec3V Cx_Ax_Bx = Ax_Bx_Cx.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V Cy_Ay_By = Ay_By_Cy.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V Cz_Az_Bz = Az_Bz_Cz.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V aa_bb_cc = AddScaled(AddScaled(Ax_Bx_Cx*Ax_Bx_Cx, Ay_By_Cy, Ay_By_Cy), Az_Bz_Cz, Az_Bz_Cz);
	const Vec3V ab_bc_ca = AddScaled(AddScaled(Ax_Bx_Cx*Bx_Cx_Ax, Ay_By_Cy, By_Cy_Ay), Az_Bz_Cz, Bz_Cz_Az);
	const Vec3V ca_ab_bc = ab_bc_ca.Get<Vec::Z,Vec::X,Vec::Y>();
	const Vec3V d1_d2_d3 = ab_bc_ca - aa_bb_cc;
	const VecBoolV sep234 = (aa_bb_cc > Vec3V(rr)) & (Min(ab_bc_ca, ca_ab_bc) > aa_bb_cc);
	const Vec3V ABx_BCx_CAx = Bx_Cx_Ax - Ax_Bx_Cx;
	const Vec3V ABy_BCy_CAy = By_Cy_Ay - Ay_By_Cy;
	const Vec3V ABz_BCz_CAz = Bz_Cz_Az - Az_Bz_Cz;
	const Vec3V e1_e2_e3 = AddScaled(AddScaled(ABx_BCx_CAx*ABx_BCx_CAx, ABy_BCy_CAy, ABy_BCy_CAy), ABz_BCz_CAz, ABz_BCz_CAz);
	const Vec3V Q1x_Q2x_Q3x = SubtractScaled(Ax_Bx_Cx*e1_e2_e3, ABx_BCx_CAx, d1_d2_d3);
	const Vec3V Q1y_Q2y_Q3y = SubtractScaled(Ay_By_Cy*e1_e2_e3, ABy_BCy_CAy, d1_d2_d3);
	const Vec3V Q1z_Q2z_Q3z = SubtractScaled(Az_Bz_Cz*e1_e2_e3, ABz_BCz_CAz, d1_d2_d3);
	const Vec3V QCx_QAx_QBx = SubtractScaled(Q1x_Q2x_Q3x, Cx_Ax_Bx, e1_e2_e3); // negated
	const Vec3V QCy_QAy_QBy = SubtractScaled(Q1y_Q2y_Q3y, Cy_Ay_By, e1_e2_e3); // negated
	const Vec3V QCz_QAz_QBz = SubtractScaled(Q1z_Q2z_Q3z, Cz_Az_Bz, e1_e2_e3); // negated
	const Vec3V qq = AddScaled(AddScaled(Q1x_Q2x_Q3x*Q1x_Q2x_Q3x, Q1y_Q2y_Q3y, Q1y_Q2y_Q3y), Q1z_Q2z_Q3z, Q1z_Q2z_Q3z);
	const Vec3V qw = AddScaled(AddScaled(Q1x_Q2x_Q3x*QCx_QAx_QBx, Q1y_Q2y_Q3y, QCy_QAy_QBy), Q1z_Q2z_Q3z, QCz_QAz_QBz);
	const VecBoolV sep567 = (qq > e1_e2_e3*e1_e2_e3*rr) & (qw < Vec3V(V_ZERO));
	const VecBoolV sep234_567 = sep234 | sep567;
	const BoolV separated = sep1 | sep234_567.GetX() | sep234_567.GetY() | sep234_567.GetZ();
#endif // TEST_SPHERE_TO_TRIANGLE_TRANSPOSED

#if !TEST_SPHERE_TO_TRIANGLE_TRANSPOSED || TEST_SPHERE_TO_TRIANGLE_TEST
	// this code is pretty much straight from http://realtimecollisiondetection.net/blog/?p=103
	const ScalarV aa = Dot(A, A);
	const ScalarV bb = Dot(B, B);
	const ScalarV cc = Dot(C, C);
	const ScalarV ab = Dot(A, B);
	const ScalarV bc = Dot(B, C);
	const ScalarV ca = Dot(C, A);
	const BoolV sep2 = (aa > rr) & (Min(ab, ca) > aa);
	const BoolV sep3 = (bb > rr) & (Min(bc, ab) > bb);
	const BoolV sep4 = (cc > rr) & (Min(ca, bc) > cc);
	const Vec3V AB = B - A;
	const Vec3V BC = C - B;
	const Vec3V CA = A - C;
	const ScalarV d1 = ab - aa;
	const ScalarV d2 = bc - bb;
	const ScalarV d3 = ca - cc;
	const ScalarV e1 = Dot(AB, AB);
	const ScalarV e2 = Dot(BC, BC);
	const ScalarV e3 = Dot(CA, CA);
	const Vec3V Q1 = SubtractScaled(A*e1, AB, d1);
	const Vec3V Q2 = SubtractScaled(B*e2, BC, d2);
	const Vec3V Q3 = SubtractScaled(C*e3, CA, d3);
	const Vec3V QC = SubtractScaled(Q1, C, e1); // negated
	const Vec3V QA = SubtractScaled(Q2, A, e2); // negated
	const Vec3V QB = SubtractScaled(Q3, B, e3); // negated
	const BoolV sep5 = (Dot(Q1, Q1) > e1*e1*rr) & (Dot(Q1, QC) < ScalarV(V_ZERO));
	const BoolV sep6 = (Dot(Q2, Q2) > e2*e2*rr) & (Dot(Q2, QA) < ScalarV(V_ZERO));
	const BoolV sep7 = (Dot(Q3, Q3) > e3*e3*rr) & (Dot(Q3, QB) < ScalarV(V_ZERO));
#if !TEST_SPHERE_TO_TRIANGLE_TRANSPOSED
	const BoolV separated = sep1 | sep2 | sep3 | sep4 | sep5 | sep6 | sep7;
#endif // !TEST_SPHERE_TO_TRIANGLE_TRANSPOSED
#endif // !TEST_SPHERE_TO_TRIANGLE_TRANSPOSED || TEST_SPHERE_TO_TRIANGLE_TEST

#if TEST_SPHERE_TO_TRIANGLE_TEST
	Assert(0.001f >= Abs<float>(aa.Getf() - aa_bb_cc.GetXf()));
	Assert(0.001f >= Abs<float>(bb.Getf() - aa_bb_cc.GetYf()));
	Assert(0.001f >= Abs<float>(cc.Getf() - aa_bb_cc.GetZf()));
	Assert(0.001f >= Abs<float>(ab.Getf() - ab_bc_ca.GetXf()));
	Assert(0.001f >= Abs<float>(bc.Getf() - ab_bc_ca.GetYf()));
	Assert(0.001f >= Abs<float>(ca.Getf() - ab_bc_ca.GetZf()));
	Assert(sep2.Getb() == sep234.GetX().Getb());
	Assert(sep3.Getb() == sep234.GetY().Getb());
	Assert(sep4.Getb() == sep234.GetZ().Getb());
	Assert(0.001f >= Abs<float>(AB.GetXf() - ABx_BCx_CAx.GetXf()));
	Assert(0.001f >= Abs<float>(AB.GetYf() - ABy_BCy_CAy.GetXf()));
	Assert(0.001f >= Abs<float>(AB.GetZf() - ABz_BCz_CAz.GetXf()));
	Assert(0.001f >= Abs<float>(BC.GetXf() - ABx_BCx_CAx.GetYf()));
	Assert(0.001f >= Abs<float>(BC.GetYf() - ABy_BCy_CAy.GetYf()));
	Assert(0.001f >= Abs<float>(BC.GetZf() - ABz_BCz_CAz.GetYf()));
	Assert(0.001f >= Abs<float>(CA.GetXf() - ABx_BCx_CAx.GetZf()));
	Assert(0.001f >= Abs<float>(CA.GetYf() - ABy_BCy_CAy.GetZf()));
	Assert(0.001f >= Abs<float>(CA.GetZf() - ABz_BCz_CAz.GetZf()));
	Assert(0.001f >= Abs<float>(d1.Getf() - d1_d2_d3.GetXf()));
	Assert(0.001f >= Abs<float>(d2.Getf() - d1_d2_d3.GetYf()));
	Assert(0.001f >= Abs<float>(d3.Getf() - d1_d2_d3.GetZf()));
	Assert(0.001f >= Abs<float>(e1.Getf() - e1_e2_e3.GetXf()));
	Assert(0.001f >= Abs<float>(e2.Getf() - e1_e2_e3.GetYf()));
	Assert(0.001f >= Abs<float>(e3.Getf() - e1_e2_e3.GetZf()));
	Assert(0.001f >= Abs<float>(Q1.GetXf() - Q1x_Q2x_Q3x.GetXf()));
	Assert(0.001f >= Abs<float>(Q1.GetYf() - Q1y_Q2y_Q3y.GetXf()));
	Assert(0.001f >= Abs<float>(Q1.GetZf() - Q1z_Q2z_Q3z.GetXf()));
	Assert(0.001f >= Abs<float>(Q2.GetXf() - Q1x_Q2x_Q3x.GetYf()));
	Assert(0.001f >= Abs<float>(Q2.GetYf() - Q1y_Q2y_Q3y.GetYf()));
	Assert(0.001f >= Abs<float>(Q2.GetZf() - Q1z_Q2z_Q3z.GetYf()));
	Assert(0.001f >= Abs<float>(Q3.GetXf() - Q1x_Q2x_Q3x.GetZf()));
	Assert(0.001f >= Abs<float>(Q3.GetYf() - Q1y_Q2y_Q3y.GetZf()));
	Assert(0.001f >= Abs<float>(Q3.GetZf() - Q1z_Q2z_Q3z.GetZf()));
	Assert(0.001f >= Abs<float>(QC.GetXf() - QCx_QAx_QBx.GetXf()));
	Assert(0.001f >= Abs<float>(QC.GetYf() - QCy_QAy_QBy.GetXf()));
	Assert(0.001f >= Abs<float>(QC.GetZf() - QCz_QAz_QBz.GetXf()));
	Assert(0.001f >= Abs<float>(QA.GetXf() - QCx_QAx_QBx.GetYf()));
	Assert(0.001f >= Abs<float>(QA.GetYf() - QCy_QAy_QBy.GetYf()));
	Assert(0.001f >= Abs<float>(QA.GetZf() - QCz_QAz_QBz.GetYf()));
	Assert(0.001f >= Abs<float>(QB.GetXf() - QCx_QAx_QBx.GetZf()));
	Assert(0.001f >= Abs<float>(QB.GetYf() - QCy_QAy_QBy.GetZf()));
	Assert(0.001f >= Abs<float>(QB.GetZf() - QCz_QAz_QBz.GetZf()));
	Assert(sep5.Getb() == sep567.GetX().Getb());
	Assert(sep6.Getb() == sep567.GetY().Getb());
	Assert(sep7.Getb() == sep567.GetZ().Getb());
#endif // TEST_SPHERE_TO_TRIANGLE_TEST

	return !separated.Getb();
}

#undef TEST_SPHERE_TO_TRIANGLE_TRANSPOSED
#undef TEST_SPHERE_TO_TRIANGLE_TEST

bool geomPoints::IsPointInFrontOfPlane (Vec3V_In point, Vec3V_In planePoint, Vec3V_In planeNormal, ScalarV_In tolerance)
{

	// Get the position of the point relative to any plane point.
	Vec3V displacement = point;
	displacement -= planePoint;

	// The point is not sufficiently behind the plane, if true.
	// The point is a distance tolerance or more in front of the plane, if false.
	return ( IsGreaterThanAll( Dot( displacement, planeNormal ), -tolerance ) != 0 );
}


bool geomPoints::IsPointBehindPlane (const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal,
										float tolerance)
{
	// Get the position of the point relative to any plane point.
	Vector3 displacement(point);
	displacement.Subtract(planePoint);

	if (displacement.Dot(planeNormal)<tolerance)
	{
		// The point is not sufficiently in front of the plane.
		return true;
	}

	// The point is a distance tolerance or more in front of the plane.
	return false;
}


bool geomPoints::IsPointNearPlane (const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal,
									float tolerance)
{
	// Get the position of the point relative to any plane point.
	Vector3 displacement(point);
	displacement.Subtract(planePoint);

	if (fabsf(displacement.Dot(planeNormal))<=tolerance)
	{
		// The point is within a distance tolerance of the plane.
		return true;
	}

	// The point is not within a distance tolerance of the plane.
	return false;
}

#if __DEV
// Straight duplicate of this inlined function in the header
// -- Assumption here is that DEV builds can be de-optimized and this code explodes in that case so we hide it behind a function call
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

void geomPoints::FindClosestPointPointBoxSurface(Vec3V_In point, Vec3V_In boxHalfSize, Vec3V_InOut pointOnBox, Vec3V_InOut normalOnBox, ScalarV_InOut depth)
{
	const Vec3V localPoint = point;
	const Vec3V localBoxHalfSize = boxHalfSize;

	Vec3V boxCornerToAbsPoint = Subtract(Abs(localPoint),localBoxHalfSize);
	VecBoolV isPointInBoxDimension = IsLessThan(boxCornerToAbsPoint,Vec3V(V_ZERO));
	BoolV isPointInsideBox = isPointInBoxDimension.GetX() & isPointInBoxDimension.GetY() & isPointInBoxDimension.GetZ();

	// Find the axis of the face the point is closest to. This is whichever element of boxCornerToAbsPoint is largest.
	VecBoolV greaterThanRight = IsGreaterThan(boxCornerToAbsPoint, boxCornerToAbsPoint.Get<Vec::Y, Vec::Z, Vec::X>());
	VecBoolV greaterOrEqualLeft = InvertBits(greaterThanRight.Get<Vec::Z, Vec::X, Vec::Y>());
	VecBoolV closestFaceAxisMask = And(greaterThanRight, greaterOrEqualLeft);
	closestFaceAxisMask.SetX(!Xor(closestFaceAxisMask.GetY(), closestFaceAxisMask.GetZ())); // Map F_F_F to T_F_F to ensure that one element is always true

	// Get the normal of the closest face
	Vec3V closestFaceAxis = And(Vec3V(closestFaceAxisMask),Vec3V(V_ONE));
	Vec3V closestFaceNormal = SelectFT(IsGreaterThan(localPoint,Vec3V(V_ZERO)),Negate(closestFaceAxis),closestFaceAxis);

	// If the point is inside the box, this will push it to the nearest surface, if the point is outside the box it will clamp it to the box surface
	Vec3V closestPointOnBoxSurface = Clamp(SelectFT(closestFaceAxisMask,localPoint,Scale(closestFaceNormal,localBoxHalfSize)),Negate(localBoxHalfSize),localBoxHalfSize);

	// Find the depth. The depth is only positive when the point is inside the box. 
	Vec3V pointToBoxSurface = Subtract(closestPointOnBoxSurface,localPoint);
	ScalarV distanceToSurface = Mag(pointToBoxSurface);
	ScalarV pointDepth = SelectFT(isPointInsideBox,Negate(distanceToSurface),distanceToSurface);

	// If the point is inside the box, the depth is positive and pointToBoxSurface points outside the box. If the point is outside the box
	//   then the depth is negative and pointToBoxSurface points inside the box, so it flips the normal. If the point is on the surface we will
	//   use the closest face normal. 
	normalOnBox = InvScaleSafe(pointToBoxSurface, pointDepth, closestFaceNormal);
	depth = pointDepth;
	pointOnBox = closestPointOnBoxSurface;
}

float geomDistances::DistanceLineToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target,
											Vector3* unitLineToPoint)
{
	// Get the given point on the line relative to the target point.
	Vector3 relPoint1(point1);
	relPoint1.Subtract(target);

	return DistanceLineToOrigin(relPoint1,point1To2,unitLineToPoint);
}


float geomDistances::DistanceLineToOrigin (const Vector3& point1, const Vector3& point1To2, Vector3* unitLineToOrigin)
{
	// Get the offset from the line to the origin, perpendicular to the line.
	Vector3 perpLineToOrigin(point1);
	perpLineToOrigin.Cross(point1To2);
	perpLineToOrigin.Cross(point1To2);
	float perpLineToOriginMag2 = perpLineToOrigin.Mag2();
	if (perpLineToOriginMag2>VERY_SMALL_FLOAT)
	{
		// The perpendicular displacement from the line to the origin does not have nearly zero length.
        perpLineToOrigin.Scale(invsqrtf(perpLineToOriginMag2));
	}
	else
	{
		// The perpendicular displacement from the line to the origin has nearly zero length, so use the displacement
		// from the first point to the origin.
		perpLineToOrigin.Negate(point1);
		perpLineToOriginMag2 = perpLineToOrigin.Mag2();
		if (perpLineToOriginMag2>VERY_SMALL_FLOAT)
		{
			// The first point is not nearly on the origin.
			perpLineToOrigin.Scale(invsqrtf(perpLineToOriginMag2));
		}
		else
		{
			// The first point is nearly on the origin, so use an arbitrary direction.
			perpLineToOrigin.Set(XAXIS);
		}
	}

	if (unitLineToOrigin)
	{
		unitLineToOrigin->Set(perpLineToOrigin);
	}

	// Return the shortest distance between the line and the origin, by projecting any vector
	// between the two onto the unit normal vector just found.
	return(-(perpLineToOrigin.Dot(point1)));
}


Vector3 geomDistances::DistanceLineToPointV (const Vector3& point1, const Vector3& point1To2, const Vector3& target,
										  Vector3* unitLineToPoint)
{
	// Get the given point on the line relative to the target point.
	Vector3 relPoint1(point1);
	relPoint1.Subtract(target);

	return DistanceLineToOriginV(relPoint1,point1To2,unitLineToPoint);
}

#if !__SPU
Vector3 geomDistances::DistanceLineToOriginV (const Vector3& point1, const Vector3& point1To2, Vector3* unitLineToOrigin)
{
	// Get the offset from the line to the origin, perpendicular to the line.
	Vector3 perpLineToOrigin(point1);
	perpLineToOrigin.Cross(point1To2);
	perpLineToOrigin.Cross(point1To2);
	Vector3 perpLineToOriginMag2 = perpLineToOrigin.Mag2V();

	Vector3 perpLineToOriginMag2Inv;
#if VECTORIZED
	perpLineToOriginMag2Inv = NewtonRaphsonRsqrt( perpLineToOriginMag2 );
#else
	perpLineToOriginMag2Inv = perpLineToOriginMag2.SqrtV();
	perpLineToOriginMag2Inv.Invert();
#endif

	Vector3 perpLineToOriginA;
	perpLineToOriginA = perpLineToOrigin * perpLineToOriginMag2Inv;

	////

	perpLineToOrigin.Negate(point1);
	Vector3 perpLineToOriginMag2B = perpLineToOrigin.Mag2V();

#if VECTORIZED
	perpLineToOriginMag2Inv = NewtonRaphsonRsqrt( perpLineToOriginMag2B );
#else
	perpLineToOriginMag2Inv = perpLineToOriginMag2B.SqrtV();
	perpLineToOriginMag2Inv.Invert();
#endif

	Vector3 perpLineToOriginB;
	perpLineToOriginB = perpLineToOrigin * perpLineToOriginMag2Inv;

	////

	// perpLineToOriginC = XAXIS

	Vector3 comparitor;
	comparitor = VEC3_VERY_SMALL_FLOAT.IsLessThanV(perpLineToOriginMag2B);
	perpLineToOriginB = comparitor.Select( XAXIS, perpLineToOriginB );

	comparitor = VEC3_VERY_SMALL_FLOAT.IsLessThanV(perpLineToOriginMag2);
	perpLineToOrigin = comparitor.Select( perpLineToOriginB, perpLineToOriginA );

	/*
	if (perpLineToOriginMag2.x>VERY_SMALL_FLOAT)
	{
		// The perpendicular displacement from the line to the origin does not have nearly zero length.
		perpLineToOrigin.Scale(invsqrtf(perpLineToOriginMag2.x));
	}
	else
	{
		// The perpendicular displacement from the line to the origin has nearly zero length, so use the displacement
		// from the first point to the origin.
		perpLineToOrigin.Negate(point1);
		perpLineToOriginMag2 = perpLineToOrigin.Mag2V();
		if (perpLineToOriginMag2.x>VERY_SMALL_FLOAT)
		{
			// The first point is not nearly on the origin.
			perpLineToOrigin.Scale(invsqrtf(perpLineToOriginMag2.x));
		}
		else
		{
			// The first point is nearly on the origin, so use an arbitrary direction.
			perpLineToOrigin.Set(XAXIS);
		}
	}
*/

	if (unitLineToOrigin)
	{
		unitLineToOrigin->Set(perpLineToOrigin);
	}

	// Return the shortest distance between the line and the origin, by projecting any vector
	// between the two onto the unit normal vector just found.
	Vector3 vr;
	vr = -(perpLineToOrigin.DotV(point1));

	return vr;
}
#endif // !__SPU


float geomDistances::Distance2LineToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& target)
{
	// Calculate the lerp value for the closest point on the line to the target point.
	float alpha = geomTValues::FindTValueOpenSegToPoint(point1,point1To2,target);

	// Now calculate the vector between that closest point and the target point.
	Vector3 tmp;
	tmp.AddScaled(point1,point1To2,alpha);
	tmp.Subtract(target);

	// Return the squared magnitude of that vector.
	return tmp.Mag2();
}

float geomDistances::DistanceSegToPoint(const Vector3& point1, const Vector3& point1To2, const Vector3& target)
{
	Vec3V point1_v = VECTOR3_TO_VEC3V(point1);
	Vec3V point1To2_v = VECTOR3_TO_VEC3V(point1To2);
	Vec3V target_v = VECTOR3_TO_VEC3V(target);

	// Find the closest point on the segment to the given target.
	Vec3V closest = geomPoints::FindClosestPointSegToPoint(point1_v, point1To2_v, target_v);

	// Return the distance between those two points.
	return Dist(closest, target_v).Getf();
}


Vector3 geomDistances::DistanceLineToLineV (Vector3::Param point1, Vector3::Param point1To2, Vector3::Param target1,
											Vector3::Param target1To2, Vector3* outUnitTargetToLine, Vector3::Param tolerance)
{
	// Generate the normal vector that points from the second line to the first line.
	Vector3 unitTargetToLine(target1To2);
	unitTargetToLine.Cross(point1To2);
	Vector3 crossMag2 = unitTargetToLine.Mag2V();

	// If the angle between the two lines is less than the tolerance, then they're parallel,and
	// we use a different formula to calculate the result.
	Vector3 testVec( ((Vector3)point1To2).Mag2V() );
	testVec.Multiply( ((Vector3)target1To2).Mag2V() );
	//testVec.Scale( tolerance );
	testVec *= tolerance;
	if( crossMag2.IsLessThanDoNotUse( testVec ) )
	{
		return DistanceParallelLineToLineV(point1,target1,target1To2,outUnitTargetToLine);
	}

	// The lines are not nearly parallel.
	if(crossMag2.IsGreaterThan(VEC3_ZERO))
	{
		// Normalize the line-to-line normal vector.
		unitTargetToLine *= crossMag2.RecipSqrtV();

		// Depending on how the endpoints are ordered, we may have generated the right
		// normal vector, but in the wrong direction.  Make sure it points from
		// target1-target2 to point1-point2.
		Vector3 temp;
		temp.Subtract(point1,target1);
		Vector3 targetToEdge=unitTargetToLine.DotV(temp);
		if (targetToEdge.IsLessThanDoNotUse(VEC3_ZERO))
		{
			// The vector is backwards.  Switch it around.
			targetToEdge=-targetToEdge;
			unitTargetToLine.Negate();
		}

		if (outUnitTargetToLine)
		{
			outUnitTargetToLine->Set(unitTargetToLine);
		}

		// Return the distance between the two lines.
		return targetToEdge;
	}

	// One of the segments has zero length.
	if (((Vector3)point1To2).Mag2V().IsZero())
	{
		// The line segment point1-point2 has zero length.
		return DistanceLineToPointV(target1,target1To2,point1,outUnitTargetToLine);
	}
	
	// The line segment target1-target2 has zero length.
	Vector3 distance = DistanceLineToPointV(point1,point1To2,target1,outUnitTargetToLine);

	if (outUnitTargetToLine)
	{
		outUnitTargetToLine->Negate();
	}

	return distance;
}

float geomDistances::DistanceLineToLine (Vector3::Param point1, Vector3::Param point1To2, Vector3::Param target1,
										 Vector3::Param target1To2, Vector3* outUnitTargetToLine, Vector3::Param tolerance)
{
	// This function is faster than the old implementation, so it has been replaced.
	return DistanceLineToLineV(point1, point1To2, target1, target1To2, outUnitTargetToLine, tolerance).GetX();
}


float geomDistances::DistanceParallelLineToLine (const Vector3& point1, const Vector3& target1,
													const Vector3& target1To2, Vector3* outUnitTargetToLine)
{
	// We need a perpendicular vector between these two line segments.  Start with
	// any vector between these two line segments.
	Vector3 point1ToTarget1;
	point1ToTarget1.Subtract(target1,point1);

	// Two cross-products later, and we have a vector that's perpendicular to both
	// line segments.
	Vector3 unitTargetToLine(point1ToTarget1);
	unitTargetToLine.Cross(target1To2);
	unitTargetToLine.Cross(target1To2);
	unitTargetToLine.Normalize();

	if (outUnitTargetToLine)
	{
		outUnitTargetToLine->Set(unitTargetToLine);
	}

	// Return the distance between the two parallel lines.  (The distance is calculated
	// with the usual formula; take any vector between the two lines, and project it onto
	// the axis described by the unit normal vector.)
	return (-(unitTargetToLine.Dot(point1ToTarget1)));
}

Vector3 geomDistances::DistanceParallelLineToLineV (const Vector3& point1, const Vector3& target1,
												 const Vector3& target1To2, Vector3* outUnitTargetToLine)
{
	// We need a perpendicular vector between these two line segments.  Start with
	// any vector between these two line segments.
	Vector3 point1ToTarget1;
	point1ToTarget1.Subtract(target1,point1);

	// Two cross-products later, and we have a vector that's perpendicular to both
	// line segments.
	Vector3 unitTargetToLine(point1ToTarget1);
	unitTargetToLine.Cross(target1To2);
	unitTargetToLine.Cross(target1To2);
	unitTargetToLine.Normalize();

	if (outUnitTargetToLine)
	{
		outUnitTargetToLine->Set(unitTargetToLine);
	}

	// Return the distance between the two parallel lines.  (The distance is calculated
	// with the usual formula; take any vector between the two lines, and project it onto
	// the axis described by the unit normal vector.)
	return (-(unitTargetToLine.DotV(point1ToTarget1)));
}


float geomDistances::DistanceLineToYAxis (const Vector3& point1, const Vector3& point1To2, Vector3& unitAxisToLine)
{
	// Calculate the unit normal vector, i.e. a vector from the Y axis to the line segment.
	// The formula used here is a simplified version of point1To2.Cross(Vector3(0.0f,1.0f,0.0f)).
	unitAxisToLine.Set(-point1To2.z,0.0f,point1To2.x);
	unitAxisToLine.Normalize();

	// Calculate the distance between the line segment and the Y axis, along the unit normal
	// vector.
	float lineToAxis=unitAxisToLine.Dot(point1);
	if(lineToAxis<0.0f)
	{
		// The unit normal vector is pointing the wrong way.  Flip it.
		lineToAxis=-lineToAxis;
		unitAxisToLine.Negate();
	}

	// Return the distance between the line segment and the Y axis.
	return lineToAxis;
}


float geomTValues::FindTValueSegToPoint (const Vector3& point1, const Vector3& point1To2, const Vector3& targetPoint)
{
	// Convert the problem into one happening at the origin, by translating the point on the segment.
	Vector3 targetTo1;
	targetTo1.Subtract(point1,targetPoint);

	// Now do the same problem, but at the origin.
	return FindTValueSegToOrigin(targetTo1,point1To2);
}


Vector3 geomTValues::FindTValueSegToPointV (const Vector3& point1, const Vector3& point1To2, const Vector3& targetPoint)
{
	// Convert the problem into one happening at the origin, by translating the point on the segment.
	Vector3 targetTo1;
	targetTo1.Subtract(point1,targetPoint);

	// Now do the same problem, but at the origin.
	return FindTValueSegToOriginV(targetTo1,point1To2);
}

ScalarV_Out geomTValues::FindTValueOpenSegToPointV (Vec3V_In point1, Vec3V_In point1To2, Vec3V_In targetPoint)
{
	// Convert the problem into one happening at the origin, by translating the point on the line.
	Vec3V offsetTarget = point1 - targetPoint;

	// Now do the same problem, but at the origin.
	return FindTValueOpenSegToOriginV(offsetTarget, point1To2);
}

float geomTValues::FindTValueOpenSegToPoint (const Vector3& point1, const Vector3& point1To2,
												const Vector3& targetPoint)
{
	// Convert the problem into one happening at the origin, by translating the point on the line.
	Vector3 targetTo1;
	targetTo1.Subtract(point1,targetPoint);

	// Now do the same problem, but at the origin.
	return FindTValueOpenSegToOrigin(targetTo1,point1To2);
}


Vector3 geomTValues::FindTValueSegToOriginV (const Vector3& point1, const Vector3& point1To2)
{
#if RSG_CPU_INTEL || __PSP2
	//mthAssertf(false,"Bruce broke this on non-vectorized platforms");
	//return point1 - point1To2;	// TOTALLY WRONG
	float oneDot=-(point1To2.Dot(point1));
	if(oneDot<=0.0f)
	{
		// point1 is closest
		return ORIGIN;
	}

	// Now do the same thing in the other direction.  (The formula we use to calculate twoDot
	// is a more efficient version of point1To2.Dot (point2), especially since we don't have
	// point2 explicitly stored.)
	float bothDot=point1To2.Mag2();
	if(bothDot<=oneDot)
	{
		// point2 is closest
		return VEC3_IDENTITY;
	}

	// The closest point is on the segment.  Calculate & return the lerp value for the point.
	Vector3 tValue;
	tValue.Set(oneDot/bothDot);
	return tValue;
#else
	// Imagine a plane that point1 lies on, and whose normal vector is the vector from the
	// origin to point1.  Calculate the distance between this plane and point2.  If that
	// distance is positive, then point2 is further away from the origin than point1 is.
	// But we negate the distance, so if the distance is negative, then point1 is closer to
	// the origin than point2.
	// bdawson: optimized this to avoid load-hit-stores, floating point compares, and divides.
	__vector4 oneDotV = -point1To2.DotV(point1);
	// Now do the same thing in the other direction.  (The formula we use to calculate twoDot
	// is a more efficient version of point1To2.Dot (point2), especially since we don't have
	// point2 explicitly stored.)
	__vector4 bothDotV = point1To2.DotV(point1To2);
	// Multiply by the reciprocal estimate to get a 12-bit accurate estimate of division.
	// If bothDotV is zero then vrefp will give us infinity, which is actually the correct
	// result. This calculation will occasionally give a result slightly less than 1.0 when
	// oneDotV is larger than bothDotV (fixed below) and occasionally gives a significant
	// relative error when the result is very close to 0.0, but is otherwise accurate to
	// within one part in a thousand.
	__vector4 resultV = __vmulfp(oneDotV,__vrefp(bothDotV));
	// Now we have to choose between a result of 0.0, 1.0, and our calculated ratio.
	// Clamping our divided result at 1.0 is easy enough. However, a simple __vminfp on our
	// estimated value means that sometimes we will get a result of 0.99995 when we should
	// be getting 1.0. We can avoid this with the slightly slower technique of comparing and
	// selecting.
	//resultV = __vminfp(resultV, __VECTOR4_ONE);
	// Create a mask that is all ones if oneDotV is greater than bothDotV
//	_uvector4 maxMask = __vcmpgefp(oneDotV, bothDotV);
	// Create a mask that is all ones if resultV is greater than 1
	_uvector4 maxMask = __vcmpgefp(resultV, __VECTOR4_ONE);

	resultV = __vsel(resultV, __VECTOR4_ONE, maxMask);
	// If oneDot is negative we need to return a zero. This is best done by comparing against
	// zero to generate a mask that is all ones if we are greater than or equal to zero. Then
	// we simply and with that mask to get zero or our ratio.
	// I don't use IsGreaterThan because it spends several instructions masking off the 'w'
	// component which is unnecessary in general but particularly so in this case where the 'w'
	// component is known.
	_uvector4 mask = __vcmpgefp(oneDotV, _vzerofp);
	resultV = __vand(resultV, (__vector4)mask);
	return resultV;
#endif
}

#define	VECTORIZED_FindTValueSegToOrigin

float geomTValues::FindTValueSegToOrigin (const Vector3& point1, const Vector3& point1To2)
{
	Vector3 resultV = FindTValueSegToOriginV(point1, point1To2);
#ifdef	VECTORIZED_FindTValueSegToOrigin
	return resultV.GetX();
#else
	// This scalar version now only exists in order to test the vector version.
	float oneDot=-(point1To2.Dot(point1));
	if(oneDot<=0.0f)
	{
		if (resultV.GetX() != 0)
			__debugbreak();
		// point1 is closest
		return 0.0f;
	}

	// Now do the same thing in the other direction.  (The formula we use to calculate twoDot
	// is a more efficient version of point1To2.Dot (point2), especially since we don't have
	// point2 explicitly stored.)
	float bothDot=point1To2.Mag2();
	if(bothDot<=oneDot)
	{
		if (resultV.GetX() != 1.0)
			__debugbreak();
		// point2 is closest
		return 1.0f;
	}

	// The closest point is on the segment.  Calculate & return the lerp value for the point.
	float result = oneDot/bothDot;
	if (result > resultV.GetX() * 1.001 || result < resultV.GetX() * 0.009)
		__debugbreak();
	return result;
#endif
}

ScalarV_Out geomTValues::FindTValueOpenSegToOriginV (Vec3V_In point1, Vec3V_In point1To2)
{
	// Imagine a plane that point1 lies on, and whose normal vector is the vector from the
	// origin to point1.  Calculate the distance between this plane and point2.  If that
	// distance is positive, then point2 is further away from the origin than point1 is.
	ScalarV oneDot = Dot(point1To2, point1);

	// Now do the same thing in the other direction.  (The formula we use to calculate twoDot
	// is a more efficient version of point1To2.Dot (point2) + oneDot, especially since we
	// don't have point2 explicitly stored.)
	ScalarV twoDot = MagSquared(point1To2);
	return InvScaleSafe(-oneDot, twoDot, ScalarV(V_ZERO));
}

float geomTValues::FindTValueOpenSegToOrigin (const Vector3& point1, const Vector3& point1To2)
{
	// Imagine a plane that point1 lies on, and whose normal vector is the vector from the
	// origin to point1.  Calculate the distance between this plane and point2.  If that
	// distance is positive, then point2 is further away from the origin than point1 is.
	float oneDot=point1To2.Dot(point1);

	// Now do the same thing in the other direction.  (The formula we use to calculate twoDot
	// is a more efficient version of point1To2.Dot (point2) + oneDot, especially since we
	// don't have point2 explicitly stored.)
	float twoDot=point1To2.Mag2();
	if (twoDot==0.0f)
	{
		// t-value undetermined since the segment is of zero length
		return 0.0f;
	}

	// Calculate & return the lerp value for the point.
	return (-oneDot/twoDot);
}

// Is there a standard rage type for this? It needs to be the same
// size as a float and a signed integer.
typedef int floatAsInt_t;

static __forceinline bool FloatEqualZero(floatAsInt_t floatAsInt)
{
	return (floatAsInt & 0x7FFFFFFF) == 0;
}


// A variable that is always zero in order to restrain the optimizer.
static int g_alwaysZero;

bool geomTValues::FindTValuesSegToSeg (Vec3V_In edge1, Vec3V_In edge1To2, Vec3V_In target1, Vec3V_In target1To2, ScalarV_InOut edgeT, ScalarV_InOut targetT)
{
	if (FindTValuesLineToLine(edge1,edge1To2,target1,target1To2,edgeT,targetT))
	{
		// Both the t-values are between 0 and 1.
		return true;
	}

	// One or both of the t-values is not between 0 and 1, so get the t-values for the closest points on the target segment.
	Vec3V edge2 = Add(edge1,edge1To2);
	Vec3V target2 = Add(target1,target1To2);
	ScalarV targetEdge1T = FindTValueSegToPoint(target1,target1To2,edge1);
	ScalarV targetEdge2T = FindTValueSegToPoint(target1,target1To2,edge2);
	ScalarV zero = ScalarV(V_ZERO);
	ScalarV one = ScalarV(V_ONE);
	if (IsEqualAll(targetEdge1T,zero))
	{
		// edge1 has target1 as its closest point on the target segment.
		if (IsEqualAll(targetEdge2T,targetEdge1T))
		{
			// edge2 also has target1 as its closest point on the target segment.
			edgeT = FindTValueSegToPoint(edge1,edge1To2,target1);
			targetT = targetEdge1T;
		}
		else
		{
			// edge2 does not have target1 as its closest point on the target segment.
			ScalarV edgeToTarget1 = DistanceLineToPoint(edge1,edge1To2,target1);
			if (IsEqualAll(targetEdge2T,one))
			{
				// edge2 has target2 as its closest point on the target segment.
				ScalarV edgeToTarget2 = DistanceLineToPoint(edge1,edge1To2,target2);
				if (IsLessThanAll(edgeToTarget1,edgeToTarget2))
				{
					// target1 is closer to the edge's line than target2 is, so target1 and an inside edge point are closest.
					edgeT = FindTValueSegToPoint(edge1,edge1To2,target1);
					targetT = targetEdge1T;
				}
				else
				{
					// target2 is closer to the segment's line than target1 is so target2 and an inside edge point are closest.
					edgeT = FindTValueSegToPoint(edge1,edge1To2,target2);
					targetT = targetEdge2T;
				}
			}
			else
			{
				// edge2 has an interior target point as its closest point on the target segment.
				ScalarV targetToEdge2 = DistanceLineToPoint(target1,target1To2,edge2);
				if (IsLessThanAll(edgeToTarget1,targetToEdge2))
				{
					// target1 is closer to the edge's line than edge2 is to the target's line so target1 and an inside edge point are closest
					edgeT = FindTValueSegToPoint(edge1,edge1To2,target1);
					targetT = targetEdge1T;
				}
				else
				{
					// edge2 is closer to the target's line than target1 is to the edge's line, so an inside target point and edge2 are closest
					edgeT = one;
					targetT = targetEdge2T;
				}
			}
		}
	}
	else if (IsEqualAll(targetEdge1T,one))
	{
		// edge1 has target2 as its closest point on the target segment.
		if (IsEqualAll(targetEdge2T,targetEdge1T))
		{
			// edge2 has target2 as its closest point on the target segment.
			edgeT = FindTValueSegToPoint(edge1,edge1To2,target2);
			targetT = targetEdge1T;
		}
		else
		{
			// edge2 does not have target2 as its closest point on the target segment.
			ScalarV edgeToTarget2 = DistanceLineToPoint(edge1,edge1To2,target2);
			if (IsEqualAll(edgeToTarget2,zero))
			{
				// edge2 has target1 as its closest point on the target.
				ScalarV edgeToTarget1 = DistanceLineToPoint(edge1,edge1To2,target1);
				if (IsLessThanAll(edgeToTarget1,edgeToTarget2))
				{
					// target1 is closer to the segment's line than target2 is so target1 and an inside edge point are closest.
					edgeT = FindTValueSegToPoint(edge1,edge1To2,target1);
					targetT = targetEdge2T;
				}
				else
				{
					// target2 is closer to the segment's line than target1 is so target2 and an inside edge point are closest.
					edgeT = FindTValueSegToPoint(edge1,edge1To2,target2);
					targetT = one;
				}
			}
			else
			{
				// edge2 has an interior target point as its closest point on the target segment.
				ScalarV targetToEdge2 = DistanceLineToPoint(target1,target1To2,edge2);
				if (IsLessThanAll(edgeToTarget2,targetToEdge2))
				{
					// target2 is closer to the segment's line than edge2 is to the target's line so target2 and an inside edge point are closest
					edgeT = FindTValueSegToPoint(edge1,edge1To2,target2);
					targetEdge1T = targetEdge1T;
				}
				else
				{
					// edge2 is closer to the target's line than target2 is to the segment's line so an inside target point and edge1 are closest
					edgeT = one;
					targetT = targetEdge2T;
				}
			}
		}
	}
	else if (IsEqualAll(targetEdge2T,zero))
	{
		// Segment end 2 has target1 as its closest point on the target
		if (IsLessThanAll(DistanceLineToPoint(edge1,edge1To2,target1),DistanceLineToPoint(target1,target1To2,edge1)))
		{
			// target1 is closer to the segment's line than edge1 is to the target's line so target1 and an inside edge point are closest
			edgeT = FindTValueSegToPoint(edge1,edge1To2,target1);
			targetT = zero;
		}
		else
		{
			// edge1 is closer to the target's line than target1 is to the segment's line so edge1 and an inside target point are closest
			edgeT = zero;
			targetT = targetEdge1T;
		}
	}
	else if (IsEqualAll(targetEdge2T,one))
	{
		// Segment end 2 has target2 as its closest point on the target.
		if (IsLessThanAll(DistanceLineToPoint(edge1,edge1To2,target2),DistanceLineToPoint(target1,target1To2,edge1)))
		{
			// target2 is closer to the segment's line than edge1 is to the target's line so target2 and an inside edge point are closest
			edgeT = FindTValueSegToPoint(edge1,edge1To2,target2);
			targetT = one;
		}
		else
		{
			// edge1 is closer to the target's line than target2 is to the segment's line so an inside target point and edge1 are closest
			edgeT = zero;
			targetT = targetEdge1T;
		}
	}
	else if (IsLessThanAll(DistanceLineToPoint(target1,target1To2,edge1),DistanceLineToPoint(target1,target1To2,edge2)))
	{
		// Edge point 1 is closer to the target's line than edge2 is so edge1 and an inside target point are closest.
		edgeT = zero;
		targetT = targetEdge1T;
	}
	else
	{
		// Edge point 2 is closer to the target's line than edge1 is so edge2 and an inside target point are closest.
		edgeT = one;
		targetT = targetEdge2T;
	}

	return false;
}


bool geomTValues::FindTValuesSegToSeg (const Vector3& edge1, const Vector3& edge1To2, const Vector3& target1,
										const Vector3& target1To2, float* edgeT, float* targetT)
{
	if(FindTValuesLineToLine(edge1,edge1To2,target1,target1To2,edgeT,targetT))
	{
		// both the t-values are >0 and <1
		return true;
	}
	// one or both of the t-values are ==0 or ==1, so do a test on edge1 and edge2 to get t-values
	// for the closest points on the target segment
	Vector3 edge2,target2;
	edge2.Add(edge1,edge1To2);
	target2.Add(target1,target1To2);
	float targetsT[2];
#if	__XENON || __PS3
	// nearest target segment point to edge1
	__vector4 tempV = FindTValueSegToPointV(target1,target1To2,edge1);
#if __SPU
	targetsT[0] = spu_extract(tempV, 0);
#else
	// stvewx requires that all elements have the same value, which they
	// already do.
	__stvewx(tempV, targetsT, 0);
#endif
	// nearest target segment point to edge2
	tempV = FindTValueSegToPointV(target1,target1To2,edge2);
#if __SPU
	targetsT[1] = spu_extract(tempV, 1);
#else
	__stvewx(tempV, targetsT, 4);
#endif
#else
	// nearest target segment point to edge1
	targetsT[0]=FindTValueSegToPoint(target1,target1To2,edge1);
	// nearest target segment point to edge2
	targetsT[1]=FindTValueSegToPoint(target1,target1To2,edge2);
#endif
	// Adding an always zero offset (that the compiler doesn't know is always
	// zero) forces the compiler to complete the writes to targets before
	// any of the reads start, thus avoiding load-hit-stores.
	floatAsInt_t* iTargetsT = union_cast<floatAsInt_t*>( union_cast<char*>( ( &targetsT[g_alwaysZero] ) ) );
	// bdawson -- let's get rid of floating-point compares
	// Integer representation of 1.0f to allow comparing the integer representation of floats.
	const int fOneAsInt = 0x3f800000;
	// Treat targetsT[0] as an integer, mask off the sign bit (so that +0 and -0
	// will compare equal) and then compare against an integer zero (which is
	// the same bit-representation as a floating-point +0).
	// This is efficient in this case because we're going to get a load-hit-store
	// anyway, no need to get a float-compare flush as well.
	if (FloatEqualZero(iTargetsT[0]))
	//if(targetsT[0]==0.0f)
	{
		// segment end 1 has target1 as its closest point on the target
		// Same as above.
		if (FloatEqualZero(iTargetsT[1]))
		//if(targetsT[1]==0.0f)
		{
			// segment end 2 has target1 as its closest point on the target
			// find the closest edge point to target1
			*edgeT=FindTValueSegToPoint(edge1,edge1To2,target1);
			// target1 is closest
			*targetT=0.0f;
		}
		// Integer compares are faster than float compares, if you haven't
		// 'recently' done math on the floats.
		else if (iTargetsT[1] == fOneAsInt)
		//else if(targetsT[1]==1.0f)
		{
			// segment end 2 has target2 as its closest point on the target
			if (DistanceLineToPoint(edge1,edge1To2,target1)<DistanceLineToPoint(edge1,edge1To2,target2))
			{
				// target1 is closer to the segment's line than target2 is
				// so target1 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target1);
				*targetT=0.0f;
			}
			else
			{
				// target2 is closer to the segment's line than target1 is
				// so target2 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target2);
				*targetT=1.0f;
			}
		}
		else
		{
			// segment end 2 has an interior target point as its closest point on the target
			if(DistanceLineToPoint(edge1,edge1To2,target1)<DistanceLineToPoint(target1,target1To2,edge2))
			{
				// target1 is closer to the segment's line than edge2 is to the target's line
				// so target1 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target1);
				*targetT=0.0f;
			}
			else
			{
				// Edge2 is closer to the target's line than target1 is to the segment's line
				// so an inside target point and edge2 are closest
				*edgeT=1.0f;
				*targetT=targetsT[1];
			}
		}
	}
	// Integer compares are faster than float compares, if you haven't
	// 'recently' done math on the floats.
	else if (iTargetsT[0] == fOneAsInt)
	//else if(targetsT[0]==1.0f)
	{
		// segment end 1 has target2 as its closest point on the target
		// See comments above.
		if (FloatEqualZero(iTargetsT[1]))
		//if(targetsT[1]==0.0f)
		{
			// segment end 2 has target1 as its closest point on the target
			if(DistanceLineToPoint(edge1,edge1To2,target1)<DistanceLineToPoint(edge1,edge1To2,target2))
			{
				// target1 is closer to the segment's line than target2 is
				// so target1 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target1);
				*targetT=0.0f;
			}
			else
			{
				// target2 is closer to the segment's line than target1 is
				// so target2 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target2);
				*targetT=1.0f;
			}
		}
		// See comments above.
		else if (iTargetsT[1] == fOneAsInt)
		//else if(targetsT[1]==1.0f)
		{
			// segment end 2 has target2 as its closest point on the target
			// find the closest edge point to target2
			*edgeT=FindTValueSegToPoint(edge1,edge1To2,target2);
			// target1 is closest
			*targetT=1.0f;
		}
		else
		{
			// segment end 2 has an interior target point as its closest point on the target
			if(DistanceLineToPoint(edge1,edge1To2,target2)<DistanceLineToPoint(target1,target1To2,edge2))
			{
				// target2 is closer to the segment's line than edge2 is to the target's line
				// so target2 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target2);
				*targetT=1.0f;
			}
			else
			{
				// edge2 is closer to the target's line than target2 is to the segment's line
				// so an inside target point and edge1 are closest
				*edgeT=1.0f;
				*targetT=targetsT[1];
			}
		}
	}
	else
	{
		// segment end 1 has an interior target point as its closest point on the target
		// See comments above.
		if (FloatEqualZero(iTargetsT[1]))
		//if(targetsT[1]==0.0f)
		{
			// segment end 2 has target1 as its closest point on the target
			if(DistanceLineToPoint(edge1,edge1To2,target1)<DistanceLineToPoint(target1,target1To2,edge1))
			{
				// target1 is closer to the segment's line than edge1 is to the target's line
				// so target1 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target1);
				*targetT=0.0f;
			}
			else
			{
				// edge1 is closer to the target's line than target1 is to the segment's line
				// so edge1 and an inside target point are closest
				*edgeT=0.0f;
				*targetT=targetsT[0];
			}
		}
		// See comments above.
		else if (iTargetsT[1] == fOneAsInt)
		//else if(targetsT[1]==1.0f)
		{
			// segment end 2 has target2 as its closest point on the target
			if(DistanceLineToPoint(edge1,edge1To2,target2)<DistanceLineToPoint(target1,target1To2,edge1))
			{
				// target2 is closer to the segment's line than edge1 is to the target's line
				// so target2 and an inside edge point are closest
				*edgeT=FindTValueSegToPoint(edge1,edge1To2,target2);
				*targetT=1.0f;
			}
			else
			{
				// edge1 is closer to the target's line than target2 is to the segment's line
				// so an inside target point and edge1 are closest
				*edgeT=0.0f;
				*targetT=targetsT[0];
			}
		}
		else
		{
			// segment end 2 has an interior target point as its closest point on the target
			if(DistanceLineToPoint(target1,target1To2,edge1)<DistanceLineToPoint(target1,target1To2,edge2))
			{
				// edge1 is closer to the target's line than edge2 is
				// so edge1 and an inside target point are closest
				*edgeT=0.0f;
				*targetT=targetsT[0];
			}
			else {
				// edge2 is closer to the target's line than edge1 is
				// so edge2 and an inside target point are closest
				*edgeT=1.0f;
				*targetT=targetsT[1];
			}
		}
	}
	return false;
}


bool geomTValues::FindTValuesSegToUprightSeg (const Vector3& edge1, const Vector3& edge1To2, float targetMinY,
												float targetMaxY, float* edgeT, float* targetT)
{
	if (FindTValuesLineToYAxis(edge1,edge1To2,targetMinY,targetMaxY,edgeT,targetT))
	{
		// Both the t-values are between zero and one.
		return true;
	}

	// One or both of the t-values will be 0 or 1. Get the second edge point.
	Vector3 edge2(edge1);
	edge2.Add(edge1To2);

	// Find the t-value of the nearest upright target segment point to the first edge point.
	float invHeight = targetMaxY-targetMinY;
	invHeight = fabsf(invHeight)>VERY_SMALL_FLOAT ? 1.0f/invHeight : 0.0f;
	float targetT1 = edge1.y>targetMinY ? (edge1.y<targetMaxY ? (edge1.y-targetMinY)*invHeight : 1.0f) : 0.0f;

	// Find the t-value of the nearest upright target segment point to the second edge point.
	float targetT2 = edge2.y>targetMinY ? (edge2.y<targetMaxY ? (edge2.y-targetMinY)*invHeight : 1.0f) : 0.0f;

	if (targetT1<=0.0f)
	{
		// End 1 of the edge has targetMinY as its closest point on the upright target segment.
		Vector3 offsetEdge1(edge1);
		offsetEdge1.y -= targetMinY;
		if (targetT2<=0.0f)
		{
			// End 2 of the edge has targetMinY as its closest point on the upright target segment.
			// Find the closest edge point to targetMinY.
			(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
			(*targetT) = 0.0f;
		}
		else if (targetT2>=1.0f)
		{
			// End 2 of the edge has targetMaxY as its closest point on the upright target segment.
			float distanceMin = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			offsetEdge1.y = edge1.y-targetMaxY;
			float distanceMax = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			if (distanceMin<distanceMax)
			{
				// targetMinY is closer to the segment's line than targetMaxY is,
				// so targetMinY and an inside edge point are closest.
				offsetEdge1.y = edge1.y-targetMinY;
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 0.0f;
			}
			else
			{
				// targetMaxY is closer to the segment's line than targetMinY is,
				// so targetMaxY and an inside edge point are closest.
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 1.0f;
			}
		}
		else
		{
			// End 2 of the edge has an interior target point as its closest point on the upright target segment.
			float distanceMin = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			if (distanceMin<edge2.XZMag())
			{
				// targetMinY is closer to the segment's line than edge2 is to the target's line
				// so target1 and an inside edge point are closest
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 0.0f;
			}
			else
			{
				// Edge2 is closer to the target's line than target1 is to the segment's line
				// so an inside target point and edge2 are closest.
				(*edgeT) = 1.0f;
				(*targetT) = targetT2;
			}
		}
	}
	else if (targetT1>=1.0f)
	{
		// End 1 of the edge has target2 as its closest point on the upright target segment.
		Vector3 offsetEdge1(edge1);
		offsetEdge1.y -= targetMaxY;
		if (targetT2<=0.0f)
		{
			// End 2 of the edge has target1 as its closest point on the upright target segment.
			float distanceMax = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			offsetEdge1.y = edge1.y-targetMinY;
			float distanceMin = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			if (distanceMin<distanceMax)
			{
				// target1 is closer to the segment's line than target2 is
				// so target1 and an inside edge point are closest.
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 0.0f;
			}
			else
			{
				// target2 is closer to the segment's line than target1 is
				// so target2 and an inside edge point are closest.
				offsetEdge1.y = edge1.y-targetMaxY;
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 1.0f;
			}
		}
		else if (targetT2>=1.0f)
		{
			// End 2 of the edge has target2 as its closest point on the target.
			(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
			(*targetT) = 1.0f;
		}
		else
		{
			// End 2 of the edge has an interior target point as its closest point on the target.
			float distanceMax = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			if (distanceMax<edge2.XZMag())
			{
				// target2 is closer to the segment's line than edge2 is to the target's line
				// so target2 and an inside edge point are closest
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 1.0f;
			}
			else
			{
				// End 2 of the edge is closer to the target's line than target2 is to the segment's line
				// so an inside target point and edge1 are closest.
				(*edgeT) = 1.0f;
				(*targetT) = targetT2;
			}
		}
	}
	else
	{
		// End 1 of the edge has an interior target point as its closest point on the target
		if (targetT2<=0.0f)
		{
			// End 2 of the edge has target1 as its closest point on the target.
			Vector3 offsetEdge1(edge1);
			offsetEdge1.y -= targetMinY;
			float distanceMin = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			if (distanceMin<edge1.XZMag())
			{
				// target1 is closer to the segment's line than edge1 is to the target's line
				// so target1 and an inside edge point are closest
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 0.0f;
			}
			else
			{
				// edge1 is closer to the target's line than target1 is to the segment's line
				// so edge1 and an inside target point are closest
				(*edgeT) = 0.0f;
				(*targetT) = targetT1;
			}
		}
		else if (targetT2==1.0f)
		{
			// segment end 2 has target2 as its closest point on the target
			Vector3 offsetEdge1(edge1);
			offsetEdge1.y -= targetMaxY;
			float distanceMax = geomDistances::DistanceLineToOrigin(offsetEdge1,edge1To2);
			if (distanceMax<edge1.XZMag())
			{
				// target2 is closer to the segment's line than edge1 is to the target's line
				// so target2 and an inside edge point are closest
				(*edgeT) = FindTValueSegToOrigin(offsetEdge1,edge1To2);
				(*targetT) = 1.0f;
			}
			else
			{
				// edge1 is closer to the target's line than target2 is to the segment's line
				// so an inside target point and edge1 are closest
				(*edgeT) = 0.0f;
				(*targetT) = targetT1;
			}
		}
		else
		{
			// segment end 2 has an interior target point as its closest point on the target
			if(edge1.XZMag()<edge2.XZMag())
			{
				// edge1 is closer to the target's line than edge2 is
				// so edge1 and an inside target point are closest
				(*edgeT) = 0.0f;
				(*targetT) = targetT1;
			}
			else 
			{
				// edge2 is closer to the target's line than edge1 is
				// so edge2 and an inside target point are closest
				(*edgeT) = 1.0f;
				(*targetT) = targetT2;
			}
		}
	}

	return false;
}

// bdawson: optimizing to avoid load-hit-stores, float compares, and divides
// ks: We wish to improve ourselves. We will add your biological and technological distinctiveness to our own. Your culture will adapt to service ours. Resistance is futile. 
#define	VECTOR_FindTValuesLineToLine	(__XENON || __PS3)

bool geomTValues::FindTValuesLineToLine (const Vector3& edge1, const Vector3& edge1To2, const Vector3& target1,
											const Vector3& target1To2, float* edgeT, float* targetT)
{
#if	VECTOR_FindTValuesLineToLine
	// Find the squared length of the target.
	Vector3 target1To2Mag2 = target1To2.Mag2V();
	const static __vector4 vVerySmallFloat = {VERY_SMALL_FLOAT, VERY_SMALL_FLOAT, VERY_SMALL_FLOAT, VERY_SMALL_FLOAT};
	if (_vgreaterfp(target1To2Mag2, vVerySmallFloat))
	{
		// The target does not have nearly zero length. Get the inverse squared length.
		Vector3 invTarget1To2Mag2 = __vrefp(target1To2Mag2);
#else
	// Find the squared length of the target.
	float target1To2Mag2 = target1To2.Mag2();
	if (target1To2Mag2>VERY_SMALL_FLOAT)
	{
		// The target does not have nearly zero length. Get the inverse squared length.
		float invTarget1To2Mag2 = 1.0f/target1To2Mag2;
#endif

		// Find the displacement from the target starting point to the edge, perpendicular to the target.
		Vector3 perpPoint1(edge1);
		perpPoint1.Subtract(target1);
#if	VECTOR_FindTValuesLineToLine
		Vector3 dot = perpPoint1.DotV(target1To2);
#else
		float dot = perpPoint1.Dot(target1To2);
#endif
		perpPoint1.SubtractScaled(target1To2,dot*invTarget1To2Mag2);

		// Find the displacement along the edge, perpendicular to the target.
#if	VECTOR_FindTValuesLineToLine
		dot = edge1To2.DotV(target1To2);
#else
		dot = edge1To2.Dot(target1To2);
#endif
		Vector3 perpPoint1To2(edge1To2);
		perpPoint1To2.SubtractScaled(target1To2,dot*invTarget1To2Mag2);

		// Set the edge t-value (the fraction of the distance along the edge of the closest point to the target).
#if	VECTOR_FindTValuesLineToLine
		Vector3 edgeTV;
		//float perpPoint1To2Mag2 = perpPoint1To2.Mag2();
		Vector3 perpPoint1To2Mag2 = perpPoint1To2.Mag2V();
		//if (perpPoint1To2Mag2.GetX()>VERY_SMALL_FLOAT)
		if (_vgreaterfp(perpPoint1To2Mag2, vVerySmallFloat))
		{
			// The edge and the target are not nearly parallel.
			//(*edgeT) = -perpPoint1.Dot(perpPoint1To2)/perpPoint1To2Mag2.GetX();
			edgeTV = -perpPoint1.DotV(perpPoint1To2);
			// Simulate division by multiplying by the reciprocal
			edgeTV.InvScale(perpPoint1To2Mag2);
			// Using __stvewx requires having the value to be stored splatted to
			// all elements, but since our input is based on dot product data this
			// is already guaranteed.
#if __SPU			
			*edgeT = spu_extract(edgeTV.xyzw, 0);
#else
			__stvewx(edgeTV.xyzw, edgeT, 0);
#endif
		}
		else
		{
			// The edge and the target are nearly parallel. Set the edge t-value arbitrarily to 0.5.
			(*edgeT) = 0.5f;
			edgeTV = _vsplatf(0.5f);
		}

		// Find the point on the edge that is closest to the target.
		perpPoint1.AddScaled(edge1,edge1To2,edgeTV);

		// Find the target t-value.
		Vector3 tempResult = FindTValueSegToPointV(target1,target1To2,perpPoint1);
		// The result is already splatted to all elements.
#if __SPU
		*targetT = spu_extract(tempResult, 0);
#else
		__stvewx(tempResult.xyzw, targetT, 0);
#endif
#else
		float perpPoint1To2Mag2 = perpPoint1To2.Mag2();
		if (perpPoint1To2Mag2>VERY_SMALL_FLOAT)
		{
			// The edge and the target are not nearly parallel.
			(*edgeT) = -perpPoint1.Dot(perpPoint1To2)/perpPoint1To2Mag2;
		}
		else
		{
			// The edge and the target are nearly parallel. Set the edge t-value arbitrarily to 0.5.
			(*edgeT) = 0.5f;
		}

		// Find the point on the edge that is closest to the target.
		perpPoint1.AddScaled(edge1,edge1To2,*edgeT);

		// Find the target t-value.
		*targetT = FindTValueSegToPoint(target1,target1To2,perpPoint1);
#endif
	}
	else
	{
		// The target has nearly zero length. Set the target t-value arbitrarily to one half, and find the edge t-value.
#if	VECTOR_FindTValuesLineToLine
		Vector3 tempResult = FindTValueSegToPointV(edge1,edge1To2,target1);
		// The result is already splatted to all elements.
#if __SPU
		*edgeT = spu_extract(tempResult.xyzw, 0);
#else
		__stvewx(tempResult.xyzw, edgeT, 0);
#endif
#else
		(*edgeT) = FindTValueSegToPoint(edge1,edge1To2,target1);
#endif
		(*targetT) = 0.5f;

		// This function is supposed to return true if the two segments intersect along their edges.
		// In this case, the segment is too small to test, so it isn't known if the the intersection is edge-edge.
		// It seems safer to tell the caller that there wasn't an edge-edge intersection in this case since if they
		// think that there is an edge-edge intersection they might use information from an infinite line-infinite line
		// test.
		return false;
	}

	// Return true if the closest point on the edge to the target is not on an end, and the closest point on the target
	// to the edge is not on an end, false otherwise.
	floatAsInt_t* iEdgeT = (floatAsInt_t*)edgeT;
	floatAsInt_t* iTargetT = (floatAsInt_t*)targetT;
	// Integer representation of 1.0f to allow comparing the integer representation of floats.
	const int fOneAsInt = 0x3f800000;
	// Do integer compares to avoid the costs of floating-point compares. Because we
	// are just comparing to non-negative numbers
	// the integer ordering is identical to the floating-point ordering. Also, because
	// we are reading the results through a pointer anyway there is no additional
	// load-hit-store cost. Note that the results will be different for NANs, which I
	// assume doesn't matter.
	return ((*iEdgeT) > 0 && (*iEdgeT) < fOneAsInt && (*iTargetT)>0 && (*iTargetT)<fOneAsInt);
	//return ((*edgeT)>0.0f && (*edgeT)<1.0f && (*targetT)>0.0f && (*targetT)<1.0f);
}


bool geomTValues::FindTValuesLineToLine (Vec3V_In edge1, Vec3V_In edge1To2, Vec3V_In target1, Vec3V_In target1To2, ScalarV_InOut edgeT, ScalarV_InOut targetT)
{
	// Find the squared length of the target.
	ScalarV target1To2Mag2 = MagSquared(target1To2);

	// The target does not have nearly zero length. Get the inverse squared length.
	ScalarV invTarget1To2Mag2 = Invert(target1To2Mag2);

	// Find the displacement from the target starting point to the edge, perpendicular to the target.
	Vec3V perpPoint1 = Subtract(edge1,target1);
	ScalarV dot = Dot(perpPoint1,target1To2);
	perpPoint1 = SubtractScaled(perpPoint1,target1To2,Scale(dot,invTarget1To2Mag2));

	// Find the displacement along the edge, perpendicular to the target.
	dot = Dot(edge1To2,target1To2);
	Vec3V perpPoint1To2 = SubtractScaled(edge1To2,target1To2,Scale(dot,invTarget1To2Mag2));

	// Set the edge t-value (the fraction of the distance along the edge of the closest point to the target).
	ScalarV perpPoint1To2Mag2 = MagSquared(perpPoint1To2);
	ScalarV edgeTOut = -Scale(Dot(perpPoint1,perpPoint1To2),Invert(perpPoint1To2Mag2));

	// Make the edge t-value half if the edge and target are nearly parallel.
	ScalarV _small = ScalarV(V_FLT_SMALL_12);
	BoolV nearlyParallel = IsLessThan(perpPoint1To2Mag2,_small);
	ScalarV half = ScalarV(V_HALF);
	edgeTOut = SelectFT(nearlyParallel,edgeTOut,half);

	// Recompute the edge T from the target point, if the target has nearly zero length.
	BoolV zeroTargetLength = IsLessThan(target1To2Mag2,_small);
	edgeTOut = SelectFT(zeroTargetLength,edgeTOut,FindTValueSegToPoint(edge1,edge1To2,target1));

	// Find the point on the edge that is closest to the target.
	perpPoint1 = AddScaled(edge1,edge1To2,edgeTOut);

	// Find the target t-value.
	ScalarV targetTOut = FindTValueSegToPoint(target1,target1To2,perpPoint1);

	// Make the target T half if the target has nearly zero length.
	targetTOut = SelectFT(zeroTargetLength,targetTOut,half);

	// Set the edge T and target T.
	edgeT = edgeTOut;
	targetT = targetTOut;

	// Return true if the closest point on the edge to the target is not on an end, and the closest point on the target to the edge is not on an end.
	ScalarV zero = ScalarV(V_ZERO);
	ScalarV one = ScalarV(V_ONE);
	return (IsGreaterThanAll(edgeTOut,zero) && IsLessThanAll(edgeTOut,one) && IsGreaterThanAll(targetTOut,zero) && IsLessThanAll(targetTOut,one));
}


bool geomTValues::FindTValuesLineToYAxis (const Vector3& point1, const Vector3& point1to2, float ypoint1,
											float ypoint2, float* edgeT, float* axisT)
{
	float OneToTwoXZ2=point1to2.XZMag2();
	if(OneToTwoXZ2<fabsf(0.01f*point1to2.y)) {
		// the line is nearly parallel to the y axis, so give back t-values for
		// the midpoint in the overlap region or the midpoint in the gap
		float EdgeTop, EdgeBottom, AxisTop, AxisBottom;
		if(point1to2.y>0.0f) {EdgeTop=point1.y+point1to2.y; EdgeBottom=point1.y;}
		else {EdgeTop=point1.y; EdgeBottom=point1.y+point1to2.y;}
		if(ypoint2>ypoint1) {AxisTop=ypoint2; AxisBottom=ypoint1;}
		else {AxisTop=ypoint1; AxisBottom=ypoint2;}
		if(EdgeTop>AxisBottom && EdgeBottom<AxisTop) {
			// there is an overlap region
			if(EdgeBottom<AxisBottom) EdgeBottom=AxisBottom;
			if(EdgeTop>AxisTop) EdgeTop=AxisTop;
			*edgeT = (0.5f*(EdgeTop+EdgeBottom)-point1.y)/point1to2.y;
			float length = ypoint2-ypoint1;
			*axisT = fabsf(length)>VERY_SMALL_FLOAT ? (0.5f*(EdgeTop+EdgeBottom)-ypoint1)/length : 0.5f;
			return true;
		}
		else {
			// there is no overlap region
			if(EdgeTop<=AxisBottom) {
				*edgeT = (0.5f*(EdgeTop+AxisBottom)-point1.y)/point1to2.y;
				float length = ypoint2-ypoint1;
				*axisT = fabsf(length)>VERY_SMALL_FLOAT ? (0.5f*(EdgeTop+AxisBottom)-ypoint1)/length : 0.5f;
				return false;
			}
			else {
				mthAssertf(EdgeBottom>=AxisTop, "Invalid EdgeBottom: Edge Top,Bottom = %f, %f; Axis Top,Bottom = %f, %f", EdgeTop, EdgeBottom, AxisTop, AxisBottom);	//lint !e506 constant value boolean
				*edgeT = (0.5f*(EdgeBottom+AxisTop)-point1.y)/point1to2.y;
				float length = ypoint2-ypoint1;
				*axisT = fabsf(length)>VERY_SMALL_FLOAT ? (0.5f*(EdgeBottom+AxisTop)-ypoint1)/length : 0.5f;
				return false;
			}
		}
	}
	else {
		float flatMag2 = point1to2.XZMag2();
		*edgeT = flatMag2>VERY_SMALL_FLOAT ? -point1.XZDot(point1to2)/flatMag2 : 0.5f;
		float length = ypoint2-ypoint1;
		*axisT = fabsf(length)>SMALL_FLOAT ? (point1.y+(*edgeT)*point1to2.y-ypoint1)/length : 0.5f;
	}
	if(*edgeT>0 && *edgeT<1 && *axisT>0 && *axisT<1) return true;
	return false;
}


bool geomTValues::FindLineToMovingLineIsect (const Vector3& seg1A, const Vector3& seg1B, const Vector3& seg1AToB,
												const Vector3& seg2A, const Vector3& seg2B, const Vector3& seg2AToB,
												const Vector3& relDisp, Vector3& closest1, Vector3& closest2)
{
	// Find the vector perpendicular to the first segment and the relative displacement.
	Vector3 planeNormal(seg1AToB);
	planeNormal.Cross(relDisp);
	float normMag2 = planeNormal.Mag2();
	if (normMag2>VERY_SMALL_FLOAT)
	{
		// The motion of segment two relative to segment one is not nearly parallel to segment one.
		// Get the vector from the start of segment one to the end of segment two.
		Vector3 fromPlane(seg2B);
		fromPlane.Subtract(seg1A);

		// See if the end of segment two is above or below the plane of the segment one and the displacement.
		float bBelow = fromPlane.Dot(planeNormal);

		// Get the vector from the start of segment one to the start of segment two.
		fromPlane.Subtract(seg2A,seg1A);

		// See if the start of segment two is above or below the plane of the segment one and the displacement.
		float aAbove = fromPlane.Dot(planeNormal);

		if (SameSign(aAbove,bBelow) || (aAbove==0.0f && bBelow==0.0f))
		{
			// Segment two is either completely above or completely below segment one, relative to the displacement,
			// so the relative motion can not cause the segments to pass through each other.
			return false;
		}

		// Find the distances of the points of segment two above or below the plane of segment one and the displacement.
		float invNormMag = 1.0f/sqrtf(normMag2);
		aAbove *= invNormMag;
		bBelow *= invNormMag;

		// Find the fraction of the distance along segment two where it crosses that plane.
		float t2 = aAbove/(aAbove-bBelow);
		if (t2<0.0f && t2>1.0f)
		{
			// Segment two does not cross the plane of segment one and the relative displacement, so the relative
			// motion can not cause the segments to pass through each other.
			return false;
		}

		// Find the point on segment two that touches the plane of segment one and the displacement.
		Vector3 planeIsect(seg2A);
		planeIsect.AddScaled(seg2AToB,t2);

		// Normalize the plane normal (it was already used, and its lack of normalization was accounted for).
		planeNormal.Scale(invNormMag);

		// Get the plane intersection position relative to the start of segment one.
		Vector3 relIsect(planeIsect);
		relIsect.Subtract(seg1A);

		// Find segment one's edge normal vector in the plane.
		Vector3 edgeNormal(seg1AToB);
		edgeNormal.Cross(planeNormal);

		if (relIsect.Dot(edgeNormal)>0.0f)
		{
			// Segment two is moving toward segment one and hasn't reached it.
			return false;
		}

		// Get the previous plane intersection position relative to the end of segment one.
		edgeNormal.Negate();
		relIsect.Subtract(relDisp);
		relIsect.Subtract(seg1AToB);

		if (relIsect.Dot(edgeNormal)>0.0f)
		{
			// Segment two is moving away from segment one and was already moving away before the displacement.
			return false;
		}

		// Get the plane intersection position relative to the end of segment one.
		edgeNormal.Cross(relDisp,planeNormal);
		relIsect.Add(relDisp);

		if (relIsect.Dot(edgeNormal)>0.0f)
		{
			// Segment two moved past segment one off the end of segment one.
			return false;
		}

		// Get the previous plane intersection position relative to the start of segment one.
		edgeNormal.Negate();
		relIsect.Subtract(relDisp);
		relIsect.Add(seg1AToB);

		if (relIsect.Dot(edgeNormal)>0.0f)
		{
			// Segment two moved past segment one off the start of segment one.
			return false;
		}

		// Find the colliding point on segment two.
		closest2.AddScaled(seg2A,seg2AToB,t2);

		// Find the normal vector of the plane of segment two and the relative displacement.
		planeNormal.Cross(relDisp,seg2AToB);
		normMag2 = planeNormal.Mag2();
		mthAssertf(normMag2>0.0f, "Invalid (zero) plane normal");		//lint !e506 constant value boolean

		// Get the end of segment one relative to the start of segment two.
		fromPlane.Subtract(seg1B,seg2A);

		// See if the end of segment one is above or below the plane of the segment two and the displacement.
		bBelow = fromPlane.Dot(planeNormal);

		// Get the start of segment one relative to the start of segment two.
		fromPlane.Subtract(seg1A,seg2A);

		// See if the start of segment one is above or below the plane of segment two and the displacement.
		aAbove = fromPlane.Dot(planeNormal);

		// Find the distances of the points of segment one above or below the plane.
		invNormMag = 1.0f/sqrtf(normMag2);
		aAbove *= invNormMag;
		bBelow *= invNormMag;

		// Find the fraction of the distance along segment one where it crosses the plane.
		float t1 = aAbove-bBelow;
		if (fabsf(t1)>SMALL_FLOAT)
		{
			t1 = aAbove/t1;
			if (t1>=0.0f && t1<=1.0f)
			{
				// Find the colliding point on segment one.
				closest1.AddScaled(seg1A,seg1AToB,t1);
				return true;
			}
		}
	}

	// The motion of the second segment relative to the first segment is nearly parallel to the first segment,
	// so there is no segment overlap in the direction of the relative motion.
	return false;
}


bool geomTValues::FindTValuesLineToBoxFace (const Vector3& point1, const Vector3& point1to2,
											const Vector3& facenormal, const Vector3& boxhalfwidths, float* t1,
											float* t2, int* edgeindex1, int* edgeindex2)
{
	Vector2 FlatPoint1(0.f,0.f), FlatPoint1To2(0.f,0.f), SquareHalfWidths(0.f,0.f);
	float T, Inverse;
	int NumFound=0;

	if(facenormal.x>0.0f) {
		FlatPoint1.Set(point1.y, point1.z);
		FlatPoint1To2.Set(point1to2.y, point1to2.z);
		SquareHalfWidths.Set(boxhalfwidths.y, boxhalfwidths.z);
	}
	else if(facenormal.x<0.0f) {
		FlatPoint1.Set(point1.z, point1.y);
		FlatPoint1To2.Set(point1to2.z, point1to2.y);
		SquareHalfWidths.Set(boxhalfwidths.z, boxhalfwidths.y);
	}
	else if(facenormal.y>0.0f) {
		FlatPoint1.Set(point1.z, point1.x);
		FlatPoint1To2.Set(point1to2.z, point1to2.x);
		SquareHalfWidths.Set(boxhalfwidths.z, boxhalfwidths.x);
	}
	else if(facenormal.y<0.0f) {
		FlatPoint1.Set(point1.x, point1.z);
		FlatPoint1To2.Set(point1to2.x, point1to2.z);
		SquareHalfWidths.Set(boxhalfwidths.x, boxhalfwidths.z);
	}
	else if(facenormal.z>0.0f) {
		FlatPoint1.Set(point1.x, point1.y);
		FlatPoint1To2.Set(point1to2.x, point1to2.y);
		SquareHalfWidths.Set(boxhalfwidths.x, boxhalfwidths.y);
	}
	else if(facenormal.z<0.0f) {
		FlatPoint1.Set(point1.y, point1.x);
		FlatPoint1To2.Set(point1to2.y, point1to2.x);
		SquareHalfWidths.Set(boxhalfwidths.y, boxhalfwidths.x);
	}

	// check the x edges
	if(FlatPoint1To2.x!=0.0f) {
		Inverse = 1.0f/FlatPoint1To2.x;
		T = (SquareHalfWidths.x-FlatPoint1.x)*Inverse;
		if(fabsf(FlatPoint1.y+T*FlatPoint1To2.y)<=SquareHalfWidths.y) {
			// intersected the x=a edge
			NumFound = AddIntersection(T, t1, t2, 0, edgeindex1, edgeindex2, NumFound);
		}
		T = (-SquareHalfWidths.x-FlatPoint1.x)*Inverse;
		if(fabsf(FlatPoint1.y+T*FlatPoint1To2.y)<=SquareHalfWidths.y) {
			// intersected the x=-a face
			NumFound = AddIntersection(T, t1, t2, 2, edgeindex1, edgeindex2, NumFound);
		}
	}
	// check the y edges
	if(FlatPoint1To2.y!=0.0f && NumFound<2) {
		Inverse = 1.0f/FlatPoint1To2.y;
		T = (SquareHalfWidths.y-FlatPoint1.y)*Inverse;
		if(fabsf(FlatPoint1.x+T*FlatPoint1To2.x)<=SquareHalfWidths.x) {
			// intersected the y=b face
			NumFound = AddIntersection(T, t1, t2, 1, edgeindex1, edgeindex2, NumFound);
		}
		if(NumFound<2) {
			T = (-SquareHalfWidths.y-FlatPoint1.y)*Inverse;
			if(fabsf(FlatPoint1.x+T*FlatPoint1To2.x)<=SquareHalfWidths.x) {
				// intersected the y=-b face
				NumFound = AddIntersection(T, t1, t2, 3, edgeindex1, edgeindex2, NumFound);
			}
		}
	}

	if(NumFound==0) {
		// the line does not intersect the box
		*t1 = 0.5f;
		*t2 = 0.5f;
		return false;
	}
	if((*t1<=0.0f && *t2<=0.0f) || (*t1>=1.0f && *t2>=1.0f)) {
		// the segment does not intersect the box but the line does
		return false;
	}
	// the segment intersects the box
	return true;
}


int geomTValues::AddIntersection (float t, float* segmentT1, float* segmentT2, const Vector3& normal, Vector3* normal1,
									Vector3* normal2, int numfound, int index, int* index1, int* index2)
{
	if(numfound==0) {
		*segmentT1 = t;
		normal1->Set(normal);
		*index1 = index;
		*segmentT2 = t;
		normal2->Set(normal);
		*index2 = index;
		return 1;
	}
	if(t<*segmentT1) {
		*segmentT1 = t;
		normal1->Set(normal);
		*index1 = index;
		return 2;
	}
	if(t>*segmentT2) {
		*segmentT2 = t;
		normal2->Set(normal);
		*index2 = index;
		return 2;
	}
	return 2;
}


int geomTValues::AddIntersection (float t, float* t1, float* t2, int index, int* index1, int* index2, int NumFound)
{
	if(NumFound==0) {
		*t1 = t;
		*index1 = index;
		*t2 = t;
		*index2 = index;
		return 1;
	}
	if(t<*t1) {
		*t1 = t;
		*index1 = index;
		return 2;
	}
	if(t>*t2) {
		*t2 = t;
		*index2 = index;
		return 2;
	}
	return 2;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


int geomSegments::SegmentToSphereIntersections (const Vector3& sphereCenter, const Vector3& segmentA, const Vector3& segmentB, float sphereRadius,
												float* segmentT1, float* segmentT2, bool directed)
{
	Vector3 centerToOne(segmentA);
	centerToOne.Subtract(sphereCenter);
	Vector3 oneToTwo(segmentB);
	oneToTwo.Subtract(segmentA);
	return SegmentToSphereIntersections(centerToOne,oneToTwo,square(sphereRadius),segmentT1,segmentT2,directed);
}


int geomSegments::SegmentToSphereIntersections (const Vector3& centerToOne, const Vector3& oneToTwo, float sphereRadius2, float* segmentT1, float* segmentT2, bool directed)
{
	float oneToTwoMag2 = oneToTwo.Mag2();
	if (oneToTwoMag2>0.0f)
	{
		// The segment does not have zero length.
		float cToOneMag2 = centerToOne.Mag2();
		float cToOneDotOneToTwo = centerToOne.Dot(oneToTwo);
		float cToOneDotOneToTwo2 = square(cToOneDotOneToTwo);
		float rad2MinusCenterToLine2Scaled = oneToTwoMag2*(sphereRadius2-cToOneMag2)+cToOneDotOneToTwo2;
		if (rad2MinusCenterToLine2Scaled<0.0f)
		{
			// The segment's infinite line does not intersect the sphere.
			return 0;
		}

		// The segment's infinite line intersects the sphere. See if the segment starts inside the sphere.
		if (cToOneMag2<=sphereRadius2)
		{
			// The segment starts inside there sphere.
			if (directed)
			{
				// The directed segment starts inside the sphere, so there is no intersection.
				return 0;
			}
			
			// See if the segment ends inside the sphere.
			Vector3 centerToTwo(centerToOne);
			centerToTwo.Add(oneToTwo);
			if (centerToTwo.Mag2()<=sphereRadius2 || oneToTwoMag2<VERY_SMALL_FLOAT)
			{
				// The whole segment is inside the sphere, so there are no intersections with the sphere surface.
				return 0;
			}

			// The segment starts inside the sphere and ends outside.
			// There is one intersection. Find where it exits the sphere.
			if (segmentT1)
			{
				(*segmentT1) = (sqrtf(rad2MinusCenterToLine2Scaled)-cToOneDotOneToTwo)/oneToTwoMag2;
				DebugAssert((*segmentT1)>-SMALL_FLOAT && (*segmentT1)<1.0f+SMALL_FLOAT);
			}

			return 1;
		}

		// The segment starts outside the sphere and its infinite line touches the sphere, so find where it enters the sphere.
		float invOneToTwoMag2 = 1.0f/oneToTwoMag2;
		float enterT = -(sqrtf(rad2MinusCenterToLine2Scaled)+cToOneDotOneToTwo)*invOneToTwoMag2;
		if (enterT<0.0f || enterT>1.0f)
		{
			// The whole segment is outside the sphere, so there are no intersections.
			return 0;
		}

		if (segmentT1)
		{
			(*segmentT1) = enterT;
		}

		if (!directed)
		{
			// The segment enters the sphere, so find where it exits the sphere.
			float exitT = (sqrtf(rad2MinusCenterToLine2Scaled)-cToOneDotOneToTwo)*invOneToTwoMag2;
			DebugAssert(exitT>=enterT);
			if (exitT<=1.0f)
			{
				// The segment enters and exits the sphere.
				if (segmentT2)
				{
					(*segmentT2) = exitT;
				}

				return 2;
			}
		}

		// The segment starts outside and ends inside the sphere, so there is only one intersection.
		return 1;
	}

	// The segment has zero length, so it does not intersect the sphere's surface.
	return 0;
}


int geomSegments::SegmentToHemisphereIntersections (const Vector3& centerToOne, const Vector3& oneToTwo, float sphereRadius2, bool above, float* segmentT1, float* segmentT2)
{
	float oneToTwoMag2 = oneToTwo.Mag2();
	if (oneToTwoMag2>0.0f)
	{
		// The segment does not have zero length.
		float oneDot = centerToOne.Dot(oneToTwo);
		float one2 = centerToOne.Mag2();
		float invOneTwo2 = 1.0f/oneToTwoMag2;
		float rad2MinusCenterToLine2Scaled = (square(oneDot)*invOneTwo2-(one2-sphereRadius2))*invOneTwo2;
		if (rad2MinusCenterToLine2Scaled<0.0f)
		{
			// The segment's infinite line does not intersect the sphere.
			return 0;
		}

		float  aboveScale = (above ? 1.0f : -1.0f);
		Vector3 centerToTwo(centerToOne);
		centerToTwo.Add(oneToTwo);
		if (one2<=sphereRadius2)
		{
			// The first segment point is inside the sphere.
			if (centerToTwo.Mag2()<=sphereRadius2)
			{
				// Both segment endpoints are inside the sphere, so there are no intersections.
				return 0;
			}

			// Find the second intersection between the line and the sphere (the first has t<0).
			float t1 = -oneDot*invOneTwo2+sqrtf(rad2MinusCenterToLine2Scaled);
			if (segmentT1)
			{
				(*segmentT1) = t1;
			}

			if (aboveScale*(centerToOne.y+t1*oneToTwo.y)>0.0f)
			{
				// The intersection of the segment with the sphere is above the xz plane.
				return 1;
			}
			else
			{
				// The intersection of the segment with the sphere is below the xz plane.
				return 0;
			}
		}

		// Find the first intersection between the line and the sphere.
		float sqrtScaled = sqrtf(rad2MinusCenterToLine2Scaled);
		float t1 = -oneDot*invOneTwo2-sqrtScaled;
		if (t1<0.0f || t1>1.0f)
		{
			// The whole segment is outside the sphere (but the segment's infinite line passes through the sphere).
			return 0;
		}

		if (segmentT1)
		{
			(*segmentT1) = t1;
		}

		if (aboveScale*(centerToOne.y+t1*oneToTwo.y)>0.0f)
		{
			// The first intersection is above the xz plane - look for another.
			float t2 = -oneDot*invOneTwo2+sqrtScaled;
			if (t2<=1.0f && aboveScale*(centerToOne.y+t2*oneToTwo.y)>0.0f)
			{
				// Both intersections are above the xz plane.
				if (segmentT2)
				{
					(*segmentT2) = t2;
				}

				return 2;
			}

			// The second intersection beween the segment and the hemisphere is below the xz plane,
			return 1;
		}

		// the first intersection is below the xz plane, so look for the second and call it the first
		t1 = -oneDot*invOneTwo2+sqrtScaled;
		if (t1<=1.0f && aboveScale*(centerToOne.y+t1*oneToTwo.y)>0.0f)
		{
			// The intersection is within the segment and above the xz plane.
			if (segmentT1)
			{
				(*segmentT1) = t1;
			}

			return 1;
		}
	}

	// The segment has zero length, so it does not inersect the hemisphere's surface.
	return 0;
}


bool geomSegments::SegmentToDiskIntersection(const Vector3& centerToOne, const Vector3& oneToTwo, float radius2, float* segmentT)
{
	float above=centerToOne.y;
	float below=centerToOne.y+oneToTwo.y;
	if(SameSign(above,below))
	{
		// the segment does not cross the disk's plane
		return false;
	}
	(*segmentT)=above/(above-below);
	float planePointX,planePointZ;
	planePointX=centerToOne.x+(*segmentT)*oneToTwo.x;
	planePointZ=centerToOne.z+(*segmentT)*oneToTwo.z;
	if(square(planePointX)+square(planePointZ)>radius2)
	{
		// the segment crosses the disk's plane outside of the disk
		return false;
	}
	// the segment intersects the disk
	return true;
}


VecBoolV_Out SegmentDirectedToDiscIntersection (Vec3V_In segStart, Vec3V_In segDir, Vec3V_In discNorm, ScalarV_In discRadius, ScalarV_In discMargin, Vec4V_InOut segmentT)
{
	Assertf(0, "This function is not yet complete.");

	// Intersection times with the disc plane +/- the margin (think hubcaps)
	ScalarV dirProjN = Dot(segDir, discNorm);
	ScalarV startProjN = Dot(Negate(segStart), discNorm);
	Vec4V planeTs = InvScale(Add(Vec4V(startProjN), Vec4V(discMargin,-discMargin,Vec2V(V_ZERO))), Vec4V(dirProjN));

	//
	segmentT = planeTs;

	// Turns out that the edge curve is actually a lot like a torus (Identical for the part we actually want)
	//  Unfortunately that means that our closed form is also a quartic (Degree 4 Poly)
	// With: P = segStart, V = segDir, N = discNorm, R = discRadius, M = discMargin, L = (P.P - R^2 - M^2)
	//  And operators: . = dotProduct
	// Ours is:
	//  0 = [V.V]t^4 + [4(P.V)(V.V)]t^3 + [(2L-4R^2)(V.V) + 4R^2(V.N)^2 + 4(P.V)^2]t^2 + [(4L-8R^2)(P.V) + 8R^2(P.N)(V.N)]t + [L^2 - 4R^2(P.P) + 4R^2(P.N)^2]
	ScalarV A, B, C, D, E;
	ScalarV PdotP = Dot(segStart, segStart);
	ScalarV PdotV = Dot(segStart, segDir);
	ScalarV PdotN = Dot(segStart, discNorm);
	ScalarV VdotN = Dot(segDir, discNorm);
	ScalarV VdotV = Dot(segDir, segDir);
	ScalarV Rsqrd = Scale(discRadius, discRadius);
	ScalarV L = Subtract(Subtract(PdotP, Rsqrd), Scale(discMargin, discMargin));
	A = VdotV;
	B = Scale(ScalarV(V_FOUR), Scale(PdotV, VdotV));
	C = Scale(Subtract(Scale(ScalarV(V_TWO), L), Scale(ScalarV(V_FOUR), Rsqrd)), VdotV);
	C = Add(C, Scale(ScalarV(V_FOUR), Scale(Rsqrd, Scale(VdotN, VdotN))));
	C = Add(C, Scale(ScalarV(V_FOUR), Scale(PdotV, PdotV)));
	D = Scale(Subtract(Scale(ScalarV(V_FOUR), L), Scale(ScalarV(V_EIGHT), Rsqrd)), PdotV);
	D = Add(D, Scale(ScalarV(V_EIGHT), Scale(Rsqrd, Scale(PdotN, VdotN))));
	E = Subtract(Scale(L, L), Scale(ScalarV(V_FOUR), Scale(Rsqrd, PdotP)));
	E = Add(E, Scale(ScalarV(V_FOUR), Scale(Rsqrd, Scale(PdotN, PdotN))));

	// Mark backwards Ts as invalid
	return IsGreaterThanOrEqual(segmentT, Vec4V(V_ZERO));
}


int geomSegments::SegmentToUprightCylIsects (const Vector3& point1, const Vector3& point1to2, float length,
												float radius2, float* segmentT1, float* segmentT2, float* cylinderT1,
												float* cylinderT2)
{
	float Temp = point1.XZMag2()-radius2;
	bool OneInside=false;
	if(Temp<0.0f) OneInside=true;
	Vector3 Point2;
	Point2.Add(point1, point1to2);
	bool TwoInside=false;
	if(Point2.XZMag2()<radius2) TwoInside=true;
	if(OneInside && TwoInside) {
		// both segment ends are inside the infinite cylinder - no intersections
		return 0;
	}

	float OneDot, InvOneToTwoMag2, InvLength;

	OneDot = point1.XZDot(point1to2);
	InvOneToTwoMag2 = 1.0f / point1to2.XZMag2();
	InvLength = 1.0f / length;
	Temp = (OneDot*OneDot*InvOneToTwoMag2-Temp)*InvOneToTwoMag2;
	if(Temp<0.0f) {
		// the segment's infinite line does not intersect the infinite cylinder
		return 0;
	}

	Temp = sqrtf(Temp);
	if(OneInside) {
		// the segment's first end is inside the infinite cylinder and the second is outside
		// so there is one intersection, which is the second intersection of the segment's line
		*segmentT1 = -OneDot*InvOneToTwoMag2+Temp;
		*cylinderT1 = ((*segmentT1)*(point1to2.y)+point1.y)*InvLength+0.5f;
		return 1;
	}
	else {
		// both segment ends are outside the infinite cylinder
		*segmentT1 = -OneDot*InvOneToTwoMag2-Temp;
		if(*segmentT1<0.0f || *segmentT1>1.0f) {
			// the segment's infinite line intersects the infinite cylinder twice
			// but the segment does not intersect the infinite cylinder
			return 0;
		}
		*segmentT2 = -OneDot*InvOneToTwoMag2+Temp;
	}
	*cylinderT1 = ((*segmentT1)*(point1to2.y)+point1.y)*InvLength+0.5f;
	if(TwoInside) {
		// the first segment end is outside the infinite cylinder and the second end is inside
		return 1;
	}
	*cylinderT2 = ((*segmentT2)*(point1to2.y)+point1.y)*InvLength+0.5f;
	// the segment ends are on opposite sides of the infinite cylinder, with two intersections
	return 2;
}


int geomSegments::SegmentToInfCylIsects (const Vector3& point1, const Vector3& point1to2, const Vector3& cyl1,
											const Vector3& cyl1to2, float radius, float* segmentT1, float* segmentT2)
{
	float Temp = geomDistances::DistanceLineToPoint(cyl1, cyl1to2, point1)-radius;
	bool OneInside=false;
	if(Temp<0.0f) OneInside=true;
	Vector3 Point2;
	Point2.Add(point1, point1to2);
	bool TwoInside=false;
	Temp = geomDistances::DistanceLineToPoint(cyl1, cyl1to2, Point2)-radius;
	if(Temp<0.0f) TwoInside=true;
	if(OneInside && TwoInside) {
		// both segment ends are inside the infinite cylinder - no intersections
		return 0;
	}
	Vector3 PerpPoint1, PerpPoint1to2;
	PerpPoint1.Subtract(point1, cyl1);
	float dot = PerpPoint1.Dot(cyl1to2);
	float InvMag2 = 1.0f/cyl1to2.Mag2();
	PerpPoint1.SubtractScaled(cyl1to2, dot*InvMag2);
	dot = point1to2.Dot(cyl1to2);
	PerpPoint1to2.SubtractScaled(point1to2, cyl1to2, dot*InvMag2);
	float OneDot = PerpPoint1.Dot(PerpPoint1to2);
	float InvOneToTwoMag2 = 1.0f / PerpPoint1to2.Mag2();
	Temp = (OneDot*OneDot*InvOneToTwoMag2-Temp)*InvOneToTwoMag2;
	if(Temp<0.0f) {
		// the segment's infinite line does not intersect the infinite cylinder
		return 0;
	}
	Temp = sqrtf(Temp);
	if(OneInside) {
		// the segment's first end is inside the infinite cylinder and the second is outside
		// so there is one intersection, which is the second intersection of the segment's line
		*segmentT1 = -OneDot*InvOneToTwoMag2+Temp;
		return 1;
	}
	else {
		// both segment ends are outside the infinite cylinder
		*segmentT1 = -OneDot*InvOneToTwoMag2-Temp;
		if(*segmentT1<0.0f || *segmentT1>1.0f) {
			// the segment's infinite line intersects the infinite cylinder twice
			// but the segment does not intersect the infinite cylinder
			return 0;
		}
		*segmentT2 = -OneDot*InvOneToTwoMag2+Temp;
	}
	if(TwoInside) {
		// the first segment end is outside the infinite cylinder and the second end is inside
		return 1;
	}
	// the segment ends are on opposite sides of the infinite cylinder, with two intersections
	return 2;
}


void geomSegments::OrderIntersections (float* segmentT1, float* segmentT2, float* cylT1, float* cylT2,
										int* index1, int* index2)
{
	if(*segmentT1<=*segmentT2) return;
	float tempFloat=*segmentT1;
	*segmentT1=*segmentT2;
	*segmentT2=tempFloat;
	tempFloat=*cylT1;
	*cylT1=*cylT2;
	*cylT2=tempFloat;
	int tempInt=*index1;
	*index1=*index2;
	*index2=tempInt;
}

bool geomSegments::TestSegmentToTriangle (const Vector3 & start, const Vector3 & end, const Vector3 & v0,
											const Vector3 & v1, const Vector3 & v2)
{
	Vector3 s, r;
	float u, v, alpha;
	Vector3 a, b;
	Vector3 cross;

	s.Subtract(start,v0);
	r.Subtract(end,start);

	a.Subtract(v1,v0);
	b.Subtract(v2,v0);

	cross.Cross(a,b);

	alpha = r.Dot(cross);
	if (alpha==0.0f)
	{
		//mthWarningf("parallel segments always miss polygons");
		return false;
	}

	alpha = - s.Dot(cross) / alpha;

	if (alpha < 0.0f)
	{
		// too low
		return false;
	}

	if (alpha > 1.0f)
	{
		// too high
		return false;
	}

	// s is the intersection point
	s.AddScaled(r,alpha);

	if (fabsf(cross.x) > fabsf(cross.y) || fabsf(cross.z) > fabsf(cross.y))
	{
		if (fabsf(cross.x) > fabsf(cross.z))
		{
			// x safest
			alpha = 1.0f / cross.x;
			u = (s.y * b.z - s.z * b.y) * alpha;
			v = (s.z * a.y - s.y * a.z) * alpha;
		}
		else
		{
			// z safest
			alpha = 1.0f / cross.z;
			u = (s.x * b.y - s.y * b.x) * alpha;
			v = (s.y * a.x - s.x * a.y) * alpha;
		}
	}
	else
	{
		// y safest
		alpha = 1.0f / cross.y;
		u = (s.z * b.x - s.x * b.z) * alpha;
		v = (s.x * a.z - s.z * a.x) * alpha;
	}

	if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f)
	{
		mthAssertf(fabsf(-s.x+u*a.x+v*b.x) <= 0.01f, "Triangle intersection failed");	//lint !e506 constant value boolean
		mthAssertf(fabsf(-s.y+u*a.y+v*b.y) <= 0.01f, "Triangle intersection failed");	//lint !e506 constant value boolean
		mthAssertf(fabsf(-s.z+u*a.z+v*b.z) <= 0.01f, "Triangle intersection failed");	//lint !e506 constant value boolean

		return true;
	}

	return false;
}


bool geomSegments::CollideRayPlane (const Vector3& origin, const Vector3& ray, const Vector4& plane, float* t,
									bool backfaceCulling) 
{
	// A(Px + tRx)+B(Py + tRy)+C(Py + tRy)+D=0
	// APx + BPy + CPz + D = tARx + tBRy + tCRz

	// Warning: Vector4::DistanceToPlane() implementation
	// uses the plane equation Ax+By+Cz=D.
	// (which means Ax+By+Cz-D=0).
	// The w component of the plane may need to be negated
	// depending on the caller's setup.
	float norm = plane.DistanceToPlane(origin);
	if (backfaceCulling && (norm < 0.0f)) {
		return false;
	}

	float det  = ray.Dot(*(Vector3 *)&plane);  //lint !e740
	if (det == 0.0f)
		return false;
	*t = - norm / det;
	return true;

}

BoolV_Out geomSegments::CollideRayPlaneNoBackfaceCullingV(Vec3V_In origin, Vec3V_In ray, Vec4V_In plane, ScalarV_InOut t)
{
	// A(Px + tRx)+B(Py + tRy)+C(Py + tRy)+D=0
	// APx + BPy + CPz + D = tARx + tBRy + tCRz

	// Warning: Vector4::DistanceToPlane() implementation uses the plane equation Ax+By+Cz=D. (which means Ax+By+Cz-D=0).
	// The w component of the plane may need to be negated depending on the caller's setup.

	
	Vec4V norm = VECTOR4_TO_VEC4V( (VEC4V_TO_VECTOR4(plane)).DistanceToPlaneV(VEC3V_TO_VECTOR3(origin)) );

	ScalarV det  = Dot( ray, plane.GetXYZ());
	
	BoolV res = IsEqual(det,  ScalarV(V_ZERO) );			// expected F_F_F_F or T_T_T_T

	t = SelectFT( res, Scale( Negate( ScalarV(norm.GetIntrin128()) ), InvertFast( det ) ), t );
	return InvertBits( res );
}

BoolV_Out geomSegments::CollideRayPlane(Vec3V_In origin, Vec3V_In ray, Vec4V_In plane, ScalarV_InOut t)
{
	// A(Px + tRx)+B(Py + tRy)+C(Py + tRy)+D=0
	// APx + BPy + CPz + D = tARx + tBRy + tCRz

	// Warning: Vector4::DistanceToPlane() implementation uses the plane equation Ax+By+Cz=D. (which means Ax+By+Cz-D=0).
	// The w component of the plane may need to be negated depending on the caller's setup.
	
	Vec4V norm = VECTOR4_TO_VEC4V( (VEC4V_TO_VECTOR4(plane)).DistanceToPlaneV(VEC3V_TO_VECTOR3(origin)) );

	ScalarV det  = Dot( ray, plane.GetXYZ());
	
	BoolV res = IsGreaterThan(det,  ScalarV(V_ZERO) );			// expected F_F_F_F or T_T_T_T

	t = SelectFT( res, t, Scale( Negate( ScalarV(norm.GetIntrin128()) ), InvertFast( det ) ) );

	return  res;
}

bool geomSegments::CollideRayTriangle (const Vector3 & vert0, const Vector3 & vert1, const Vector3 & vert2,
										const Vector3 & origin, const Vector3 & ray, 
										float * t, bool backfaceCulling)
{
	Vector4 plane;
	plane.ComputePlane(vert0,vert1,vert2);
	if (!CollideRayPlane(origin,ray,plane,t,backfaceCulling))
	{
		return false;
	}

	Vector3 collide;
	collide.Scale(ray,*t);
	collide.Add(origin);

	Vector3 a,b;
	a.Subtract(vert2,vert0);
	b.Subtract(vert1,vert0);

	float absx = fabsf(plane.x);
	float absy = fabsf(plane.y);
	float absz = fabsf(plane.z);
	float axis = Max(absx,absy,absz);

	float u0,u1,u2,v0,v1,v2;

	if (axis==absx) {												//lint !e777 testing floats for equalitry
		u1 = a.y;	u2 = b.y;	u0 = collide.y - vert0.y;
		v1 = a.z;	v2 = b.z;	v0 = collide.z - vert0.z;
	} else if (axis==absy) {										//lint !e777 testing floats for equalitry
		u1 = a.x;	u2 = b.x;	u0 = collide.x - vert0.x;
		v1 = a.z;	v2 = b.z;	v0 = collide.z - vert0.z;
	} else {
		u1 = a.y;	u2 = b.y;	u0 = collide.y - vert0.y;
		v1 = a.x;	v2 = b.x;	v0 = collide.x - vert0.x;
	}

	float x = (u0 * v2 - u2 * v0) / (u1 * v2 - v1 * u2);
	float y = (u1 * v0 - u0 * v1) / (u1 * v2 - v1 * u2);

	float xay = x+y;
	if ((x >= 0.0f) && (y >= 0.0f) && (xay <= 1.0f))
		return true;
	else
		return false;
}


int geomSegments::FindImpactEdgeToShaft (const Vector3& shaftEnd1, const Vector3& shaftEnd2, const Vector3& vertex1,
											const Vector3& vertex2, float radius, int& idNum, Vector3& impactNormal,
											float& impactDepth, Vector3& impactAxisPoint, float& shaftT)
{
	Vector3 edge,shaftAxis;
		// Vectors that describe the edge & the axis of the shaft.
	Vector3 unitShaftToEdge;
		// A unit vector between the shaft & the edge, at their closest point.
	float edgeT;
		// Where the impact is located, described as a lerp value along the edge.

	// Calculate a vector to represent the edge.
	edge.Subtract(vertex2,vertex1);

	// Calculate a vector to represent the shaft's axis.
	shaftAxis.Subtract(shaftEnd2,shaftEnd1);

	// Determine if the edge's line and the shaft's line ever get close enough to intersect.
	// If not, exit now.
	// bdawson -- pass in a vector rather than a scalar to avoid a load-hit-store when converting.
#if __XENON
	Vector3::Param vTolerance = {0.0012217f, 0.0012217f, 0.0012217f};
#else
	Vector3 vTolerance(0.0012217f, 0.0012217f, 0.0012217f);
#endif
	float axisToEdge=geomDistances::DistanceLineToLine(vertex1,edge,shaftEnd1,shaftAxis,&unitShaftToEdge, vTolerance);
	// axisToEdge is the distance from the axis infinite line to the edge infinite line
	if(axisToEdge>radius)
	{
		// the line through vertex1 and vertex2 does not pass through the shaft
		return GEOM_NO_IMPACT;
	}

	// Locate the points, one on the edge, one on the axis, where the two segments are
	// at their closest approach.  If both of those points are interior to the edge &
	// shaft, that's enough to determine the collision.
	if(geomTValues::FindTValuesSegToSeg(vertex1,edge,shaftEnd1,shaftAxis,&edgeT,&shaftT))
	{
		// the point on the shaft axis that is closest to the edge is inside the shaft and
		// the point on the edge that is closest to the shaft axis is inside the edge
		idNum=0;		// refers to the edge (1 and 2 are verts on the edge)

		// The impact normal was calculated in the line-to-line check.  Since the
		// collision points are both inside the line segment, we know that the normal
		// vector is the same as the one calculated by the line-to-line check.
		impactNormal.Negate(unitShaftToEdge);

		// The impact depth is how close the edge is to the surface of the shaft.  That's
		// the radius of the shaft, minus how close the edge is to the axis of the shaft.
		impactDepth=radius-axisToEdge;

		// Since we have the t-value for the impact point on the axis, calculating the
		// impact point is easy.
		impactAxisPoint.AddScaled(shaftEnd1,shaftAxis,shaftT);

		// Let our caller know the edge collided with the "edge" of the shaft (i.e. the
		// main body of the shaft).
		return GEOM_EDGE;
	}

	// If the impact point on the shaft, closest to the edge, is inside the shaft, then
	// we have an impact between the edge and the shaft.
	if(shaftT>0.0f && shaftT<1.0f)
	{
		float impactNormalLength;
			// The length of the impact normal vector, before it gets normalized.

		// The point on the shaft axis that is closest to the edge is inside the shaft and
		// the point on the edge that is closest to the shaft axis is one end of the edge.

		// Figure out which point on the edge is closest to the shaft axis.  It'll be one
		// of the endpoints.  That point is also the first piece of data we need in order
		// to calculate the impact normal.
		if(edgeT<0.5f)
		{
			// (EdgeT==0.0f)
			idNum=1;
			impactNormal.Set(vertex1);
		}
		else
		{
			// (EdgeT==1.0f)
			idNum=2;
			impactNormal.Set(vertex2);
		}

		// Calculate the impact point on the shaft axis.
		impactAxisPoint.AddScaled(shaftEnd1,shaftAxis,shaftT);

		// Finish calculating the impact normal vector.  At this time, it points from the
		// shaft to the edge.
		impactNormal.Subtract(impactAxisPoint);

		// If the impact we're putting together happens at a greater distance than the
		// shaft's radius, then there isn't really an impact.
		impactNormalLength=impactNormal.Mag2();
		if(impactNormalLength>square(radius))
		{
			return GEOM_NO_IMPACT;
		}
		
		// If the impact-normal has a nonzero length, continue calculating the details of
		// the collision.  Otherwise, work around the zero length of the impact normal.
		if(impactNormalLength>0.0f)
		{
			// We actually had the squared length of the impact normal vector.  Get the length.
			impactNormalLength=sqrtf(impactNormalLength);

			// Calculate the impact depth.
			impactDepth=radius-impactNormalLength;

			// Normalize the impact normal vector, and flip it around so that it points from
			// the edge to the shaft.
			impactNormal.InvScale(-impactNormalLength);
		}
		else
		{
			// The edge end point is exactly on the shaft axis.

			// The impact's depth, therefore, is the radius of the shaft.
			impactDepth=radius;

			// Fake together a semi-plausible replacement impact-normal.
			impactNormal.Subtract(vertex1,vertex2);
			if(idNum==2)
			{
				impactNormal.Negate();
			}
			impactNormal.Normalize();
		}

		// Let our caller know that one of the endpoints of the edge collided with the shaft.
		return GEOM_VERTEX;
	}

	// the point on the shaft axis that is closest to the edge is one end of the shaft
	// in this case, the edge should intersect an end of the cylinder (or capsule) and not the shaft
	if(shaftT<0.5f)
	{
		shaftT=0.0f;
		impactAxisPoint.Set(shaftEnd1);
	}
	else
	{
		shaftT=1.0f;
		impactAxisPoint.Set(shaftEnd2);
	}

	// Let our caller know the edge collided with the ends of the shaft.  (This is the only
	// meaning for GEOM_END_IMPACT, by the way.)
	return GEOM_END_IMPACT;
}


int SegmentToCapsuleIntersectionsHelper (Vec3V_In point1, Vec3V_In point2, ScalarV_In capsuleLength, ScalarV_In capsuleRadius, ScalarV_InOut segT1, ScalarV_InOut segT2,
												 ScalarV_InOut capsuleT1, ScalarV_InOut capsuleT2)
{
	enum CapsuleParts{ BOTTOM_HEMISPHERE, TOP_HEMISPHERE, CAPSULE_SHAFT };
	// Initialize the t-values and index numbers.
	segT1 = ScalarV(V_NEGONE);
	segT2 = ScalarV(V_NEGONE);
	int index1 = -1;
	int index2 = -1;

	// Get the capsule axis half-length.
	ScalarV axisHalfLength = Scale(capsuleLength,ScalarV(V_HALF));

	// Get the segment.
	Vec3V segment = Subtract(point2,point1);

	ScalarV point1Y = SplatY(point1);
	ScalarV segmentX = SplatX(segment);
	ScalarV segmentY = SplatY(segment);
	ScalarV segmentZ = SplatZ(segment);
	ScalarV segFlatMag2 = Add(Scale(segmentX,segmentX),Scale(segmentZ,segmentZ));
	ScalarV isect1y,isect2y;
	ScalarV capsuleRadius2 = Scale(capsuleRadius,capsuleRadius);
	if (IsGreaterThanAll(segFlatMag2,ScalarV(V_FLT_SMALL_6)))
	{
		// The segment has a x-z component that is not nearly zero.
		ScalarV point1X = SplatX(point1);
		ScalarV point1Z = SplatZ(point1);
		ScalarV crossY = Subtract(Scale(segmentX,point1Z),Scale(segmentZ,point1X));
		if (IsGreaterThanAll(Scale(crossY,crossY),Scale(capsuleRadius2,segFlatMag2)))
		{
			// The segment's infinite line does not come close enough to the axis to intersect the capsule.
			return 0;
		}

		// Find the y-values of the two points where the segment's infinite line passes through the capsule's inifinite shaft.
		ScalarV p1FlatMag2 = Add(Scale(point1X,point1X),Scale(point1Z,point1Z));
		ScalarV dot = Add(Scale(point1X,segmentX),Scale(point1Z,segmentZ));
		ScalarV b2m4ac = SqrtSafe(Subtract(Scale(dot,dot),Scale(segFlatMag2,Subtract(p1FlatMag2,capsuleRadius2))));
		ScalarV inverse = Invert(segFlatMag2);
		segT1 = -Scale(Add(b2m4ac,dot),inverse);
		segT2 = Scale(Subtract(b2m4ac,dot),inverse);
		isect1y = Add(point1Y,Scale(segT1,segmentY));
		isect2y = Add(point1Y,Scale(segT2,segmentY));

		// See if the first intersection is in the capsule's shaft.
		if (IsGreaterThanOrEqualAll(isect1y,-axisHalfLength) && IsLessThanOrEqualAll(isect1y,axisHalfLength))
		{
			// The first intersection is with the capsule shaft.
			index1 = CAPSULE_SHAFT;
		}

		// See if the second intersection is in the capsule's shaft, and find out whether hemispheres should be tested
		// (if less than both intersections are with the shaft).
		bool hemisphereTest = true;
		if (IsGreaterThanOrEqualAll(isect2y,-axisHalfLength) && IsLessThanOrEqualAll(isect2y,axisHalfLength))
		{
			// The second intersection is with the capsule shaft.
			index2 = 2;
			hemisphereTest = (index1!=2);
		}

		if (hemisphereTest)
		{
			// At least one intersection is not with the capsule shaft, so check the hemispheres.
			ScalarV segMag2 = Add(segFlatMag2,Scale(segmentY,segmentY));
			inverse = Invert(segMag2);
			if (IsLessThanAll(isect1y,-axisHalfLength) || IsLessThanAll(isect2y,-axisHalfLength))
			{
				// One or both intersection might be with the bottom hemisphere.
				Vec3V tailP1 = AddScaled(point1,Vec3V(V_Y_AXIS_WZERO),axisHalfLength);
				ScalarV p1Mag2 = MagSquared(tailP1);
				dot = Dot(tailP1,segment);
				b2m4ac = Subtract(Scale(dot,dot),Scale(segMag2,Subtract(p1Mag2,capsuleRadius2)));
				if (IsGreaterThanAll(b2m4ac,ScalarV(V_FLT_SMALL_12)))
				{
					b2m4ac = Sqrt(b2m4ac);
					if (IsLessThanAll(isect1y,-axisHalfLength))
					{
						// The first intersection is with the bottom hemisphere.
						index1 = BOTTOM_HEMISPHERE;
						segT1 = -Scale(Add(b2m4ac,dot),inverse);
					}
					if (IsLessThanAll(isect2y,-axisHalfLength))
					{
						// The second intersection is with the bottom hemisphere.
						index2 = BOTTOM_HEMISPHERE;
						segT2 = Scale(Subtract(b2m4ac,dot),inverse);
					}
				}
			}

			if (IsGreaterThanAll(isect1y,axisHalfLength) || IsGreaterThanAll(isect2y,axisHalfLength))
			{
				// One or both intersection might be with the top hemisphere.
				Vec3V tailP1 = SubtractScaled(point1,Vec3V(V_Y_AXIS_WZERO),axisHalfLength);
				ScalarV p1Mag2 = MagSquared(tailP1);
				dot = Dot(tailP1,segment);
				b2m4ac = Subtract(Scale(dot,dot),Scale(segMag2,Subtract(p1Mag2,capsuleRadius2)));
				if (IsGreaterThanAll(b2m4ac,ScalarV(V_FLT_SMALL_12)))
				{
					b2m4ac = Sqrt(b2m4ac);
					if (IsGreaterThanAll(isect1y,axisHalfLength))
					{
						// The first intersection is with the top hemisphere.
						index1 = TOP_HEMISPHERE;
						segT1 = -Scale(Add(b2m4ac,dot),inverse);
					}
					if (IsGreaterThanAll(isect2y,axisHalfLength))
					{
						// The second intersection is with the top hemisphere.
						index2 = TOP_HEMISPHERE;
						segT2 = Scale(Subtract(b2m4ac,dot),inverse);
					}
				}
			}
		}
	}
	else if (IsGreaterThanAll(segmentY,ScalarV(V_FLT_SMALL_6)) || IsLessThanAll(segmentY,-ScalarV(V_FLT_SMALL_6)))
	{
		// The segment is nearly parallel to the capsule axis.
		ScalarV point2X = SplatX(point2);
		ScalarV point2Z = SplatZ(point2);
		isect2y = Subtract(capsuleRadius2,Add(Scale(point2X,point2X),Scale(point2Z,point2Z)));
		if (IsLessThanOrEqualAll(isect2y,ScalarV(V_FLT_SMALL_12)))
		{
			// The segment is on or outside the capsule surface, so there is no intersection.
			return 0;
		}

		// Find the y-values of the two intersections.
		isect2y = Sqrt(isect2y);
		isect2y = Add(isect2y,axisHalfLength);
		isect1y = -isect2y;
		index1 = BOTTOM_HEMISPHERE;
		index2 = TOP_HEMISPHERE;
		ScalarV point2Y = SplatY(point2);
		if (IsGreaterThanAll(point1Y,point2Y))
		{
			SwapEm(isect1y,isect2y);
			SwapEm(index1,index2);
		}

		ScalarV inverse = Invert(segmentY);
		segT1 = Scale(Subtract(isect1y,point1Y),inverse);
		segT2 = Scale(Subtract(isect2y,point1Y),inverse);
	}
	else
	{
		// The segment has nearly zero length, so there are no intersections.
		return 0;
	}

	ScalarV inverseLength = Invert(capsuleLength);
	if (index1!=-1 && IsGreaterThanOrEqualAll(segT1,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(segT1,ScalarV(V_ONE)))
	{
		// The first intersection is on the segment.
		capsuleT1 = (IsGreaterThanAll(capsuleLength,ScalarV(V_FLT_SMALL_6)) ? Scale(Add(isect1y,axisHalfLength),inverseLength) : ScalarV(V_HALF));
		if (index2!=-1 && IsGreaterThanOrEqualAll(segT2,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(segT2,ScalarV(V_ONE)))
		{
			// The second intersection is on the segment.
			capsuleT2 = (IsGreaterThanAll(capsuleLength,ScalarV(V_FLT_SMALL_6)) ? Scale(Add(isect2y,axisHalfLength),inverseLength) : ScalarV(V_HALF));
			if (IsGreaterThanAll(segT1,segT2))
			{
				SwapEm(segT1,segT2);
				SwapEm(capsuleT1,capsuleT2);
				SwapEm(index1,index2);
			}

			// There are two intersections of the segment with the capsule surface.
			return 2;
		}

		// There is one intersection of the segment with the capsule surface.
		return 1;
	}

	if (index2!=-1 && IsGreaterThanOrEqualAll(segT2,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(segT2,ScalarV(V_ONE)))
	{
		// The second intersection is on the segment and the first is not, so swap them.
		segT1 = segT2;
		capsuleT1 = (IsGreaterThanAll(capsuleLength,ScalarV(V_FLT_SMALL_6)) ? Scale(Add(isect2y,axisHalfLength),inverseLength) : ScalarV(V_HALF));
		index1 = index2;

		// There is one intersection of the segment with the capsule surface.
		return 1;
	}

	// There are no intersections of the segment with the capsule surface.
	return 0;
}


BoolV_Out geomSegments::SegmentCapsuleIntersectDirected(Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, 
														ScalarV_In capsuleLength, ScalarV_In capsuleRadius,
														Vec3V_InOut positionOnCapsule, Vec3V_InOut normalOnCapsule, ScalarV_InOut fractionAlongSegment)
{
	ScalarV fractionAlongSegment2;
	ScalarV boundT1,boundT2;
	// TODO: Write a better directed segment vs. capsule test
	if(SegmentToCapsuleIntersectionsHelper(segmentStart,Add(segmentStart,segmentStartToEnd),capsuleLength,capsuleRadius,fractionAlongSegment,fractionAlongSegment2,boundT1,boundT2))
	{
		// Clamp the t-value of the intersection on the capsule bound. 
		// TODO: We shouldn't have to clamp this
		boundT1 = Clamp(boundT1,ScalarV(V_ZERO),ScalarV(V_ONE));

		const Vec3V positionOnCapsuleSegment = And(Vec3V(AddScaled(Scale(capsuleLength,ScalarV(V_NEGHALF)),capsuleLength,boundT1)),Vec3V(V_MASKY));
		const Vec3V positionOnSweptSphereSegment = AddScaled(segmentStart,segmentStartToEnd,fractionAlongSegment);
		const Vec3V directionOnCapsule = Subtract(positionOnSweptSphereSegment,positionOnCapsuleSegment);

		// Don't accept intersections with normals that point away from the segment, this means that the swept-sphere starts out touching the capsule.
		// NOTE: If we were doing a directed segment-capsule test above this branch wouldn't be necessary. 
		if(IsLessThanAll(Dot(directionOnCapsule,segmentStartToEnd),ScalarV(V_ZERO)))
		{
			normalOnCapsule = Normalize(directionOnCapsule);
			positionOnCapsule = AddScaled(positionOnCapsuleSegment,normalOnCapsule,capsuleRadius);
			return BoolV(V_TRUE);
		}
	}
	return BoolV(V_FALSE);
}

__forceinline void UpdateIntersectionIfBetterDepth(ScalarV_In depth, Vec3V_In pointOnBox, Vec3V_In normal, ScalarV_InOut bestDepth, Vec3V_InOut bestPointOnBox, Vec3V_InOut bestNormal)
{
	// the lower the depth the better the intersection
	// it is assumed no negative depths are passed in
	BoolV isNewDepthBetter = IsLessThan(depth, bestDepth);
	bestDepth = SelectFT(isNewDepthBetter, bestDepth, depth);
	bestPointOnBox = SelectFT(isNewDepthBetter, bestPointOnBox, pointOnBox);
	bestNormal = SelectFT(isNewDepthBetter, bestNormal, normal);
}

__forceinline void UpdateIntersectionIfEdgeIsBetter(VecBoolV_In axisMask, Vec3V_In pointA, Vec3V_In pointB, Vec3V_In lineAtoB, Vec3V_In boxHalfWidth, ScalarV_InOut bestDepth, Vec3V_InOut bestPointOnBox, Vec3V_InOut bestNormal)
{
	Vec3V axis = SelectFT(axisMask, Vec3V(V_ZERO), Vec3V(V_ONE));

	// get the vector perpendicular to the edge and the capsule segment
	Vec3V edgeNormal = NormalizeSafe(Cross(axis, lineAtoB), Vec3V(V_ZERO));
	VecBoolV zeroEdgeNormal = IsEqual(edgeNormal, Vec3V(V_ZERO));
	BoolV isEdgeValid = !(zeroEdgeNormal.GetX() & zeroEdgeNormal.GetY() & zeroEdgeNormal.GetZ());

	// ensure that the edge normal points to the edge on the opposite side of the segment as the origin
	edgeNormal = SelectFT(IsGreaterThan(Dot(pointA, edgeNormal), ScalarV(V_ZERO)), Negate(edgeNormal), edgeNormal);

	// the best edge is the on that the edge normal points at
	//   add an edge bias to favor the one closer to the capsule in the case that two edges are equally viable
	Vec3V edgeBias = Scale(Average(pointA,pointB), ScalarV(V_FLT_SMALL_12));
	Vec3V edgeSegmentA = SelectFT(IsGreaterThan(edgeNormal + edgeBias, Vec3V(V_ZERO)), Negate(boxHalfWidth), boxHalfWidth);
	Vec3V edgeSegmentAtoB = SelectFT(axisMask, Vec3V(V_ZERO), Scale(edgeSegmentA, ScalarV(V_NEGTWO)));

	isEdgeValid = And(isEdgeValid, IsGreaterThan(MagSquared(edgeSegmentAtoB), ScalarV(V_FLT_SMALL_6)));

	// calculate the intersection between the edge and the segment
	ScalarV boxEdgeT;
	ScalarV capsuleSegmentT;
	BoolV isNewIntersectionBetter = And(isEdgeValid, BoolV(geomTValues::FindTValuesLineToLine(edgeSegmentA, edgeSegmentAtoB, pointA, lineAtoB, boxEdgeT, capsuleSegmentT)));

	Vec3V pointOnBox = Add(Scale(edgeSegmentAtoB, boxEdgeT), edgeSegmentA);
	Vec3V pointOnSegment = Add(Scale(lineAtoB, capsuleSegmentT), pointA);
	ScalarV depth = Dot(Subtract(pointOnBox, pointOnSegment), edgeNormal);

	// if the edge intersection is better than the current on, use that instead
	isNewIntersectionBetter = And(isNewIntersectionBetter, IsLessThan(depth, bestDepth));
	bestDepth = SelectFT(isNewIntersectionBetter, bestDepth, depth);
	bestPointOnBox = SelectFT(isNewIntersectionBetter, bestPointOnBox, pointOnBox);
	bestNormal = SelectFT(isNewIntersectionBetter, bestNormal, edgeNormal);
}

BoolV_Out geomCapsule::CapsuleAABBIntersect(	Vec3V_In capsuleSegmentStart, Vec3V_In capsuleSegmentEnd, ScalarV_In capsuleRadius,
												Vec3V_In aabbHalfExtents, 
												Vec3V_InOut positionOnBox, Vec3V_InOut normalOnBox, ScalarV_InOut depth)
{
	// For Capsule-Box intersection:
	//   The strategy used here is to determine if the capsule segment touches then box and
	//     then test pairs of features on each object for the best intersection.
	//   To determine if the capsule segment touches the box, a probe - box face test is done for
	//     each face. It is possible for the segment to be completely inside the box, so that case
	//     is checked for as well.
	//   In either case the best intersection can be capsule segment - box edge, so that test
	//     is done outside of the branch. For each dimension the closest box edge to the segment
	//     is found and the distance between them is the depth. In the case of parallel segment and
	//     edge, or a zero length segment, that edge is thrown out since a better intersection will
	//     always exist.
	//  If the capsule segment touches the box the only possible intersection other than edge-edge
	//    is capsule segment/point - box face.
	//  If the capsule segment doesn't touch the box, then the best intersection other than edge-edge is 
	//    capsule point - box face/edge/vertex, or capsule segment/point - box vertex.
	Vec3V boxHalfExtents = aabbHalfExtents;
	Vec3V boxNegativeHalfExtents = Negate(boxHalfExtents);

	Vec3V pointA = capsuleSegmentStart;
	Vec3V pointB = capsuleSegmentEnd;
	Vec3V lineAtoB = Subtract(pointB,pointA);
	ScalarV lineAtoBlength = Mag(lineAtoB);
	Vec3V vectorAtoB = InvScaleSafe(lineAtoB,lineAtoBlength,Vec3V(V_ZERO));

	// find the closest segment point to the center of the box
	// this is used as a backup for finding the normal if the closest segment and box points are equal
	Vec3V closestSegmentPointToBoxCenter = Add(pointA, Scale(vectorAtoB, Clamp(Dot(Negate(pointA), vectorAtoB), ScalarV(V_ZERO), lineAtoBlength)));

	Vec3V bestPointOnBox(V_ZERO);
	Vec3V bestNormal(V_ZERO);
	ScalarV bestDepth;

	// find the closest box edge to the capsule's segment
	ScalarV bestEdgeDepth(V_FLT_MAX);
	UpdateIntersectionIfEdgeIsBetter(VecBoolV(V_MASKX), pointA, pointB, lineAtoB, boxHalfExtents, bestEdgeDepth, bestPointOnBox, bestNormal);
	UpdateIntersectionIfEdgeIsBetter(VecBoolV(V_MASKY), pointA, pointB, lineAtoB, boxHalfExtents, bestEdgeDepth, bestPointOnBox, bestNormal);
	UpdateIntersectionIfEdgeIsBetter(VecBoolV(V_MASKZ), pointA, pointB, lineAtoB, boxHalfExtents, bestEdgeDepth, bestPointOnBox, bestNormal);

	// intersect the segment with all 6 box planes
	VecBoolV areSlopesValid = !IsEqual(lineAtoB, Vec3V(V_ZERO));
	Vec3V inverseSlope = Invert(lineAtoB);
	Vec3V positivePlaneT = Scale(Subtract(boxHalfExtents, pointA), inverseSlope);
	Vec3V negativePlaneT = Scale(Subtract(boxNegativeHalfExtents, pointA), inverseSlope);

	// a T value is valid if it is between 0 and 1, and the slope of the line was non-zero
	VecBoolV arePositiveTvaluesValid = And(areSlopesValid, And(IsLessThan(positivePlaneT, Vec3V(V_ONE)), IsGreaterThan(positivePlaneT, Vec3V(V_ZERO))));
	VecBoolV areNegativeTvaluesValid = And(areSlopesValid, And(IsLessThan(negativePlaneT, Vec3V(V_ONE)), IsGreaterThan(negativePlaneT, Vec3V(V_ZERO))));

	Vec3V boxHalfExtentsAndEpsilon = Add(boxHalfExtents,Vec3V(V_FLT_SMALL_6));
	BoolV anyValidIntersections =	And(arePositiveTvaluesValid.GetX(), geomPoints::IsPointInBox(And(Add(Scale(lineAtoB, positivePlaneT.GetX()),pointA),Vec3V(V_MASKYZ)), boxHalfExtentsAndEpsilon)) |
		And(arePositiveTvaluesValid.GetY(), geomPoints::IsPointInBox(And(Add(Scale(lineAtoB, positivePlaneT.GetY()),pointA),Vec3V(V_MASKXZ)), boxHalfExtentsAndEpsilon)) |
		And(arePositiveTvaluesValid.GetZ(), geomPoints::IsPointInBox(And(Add(Scale(lineAtoB, positivePlaneT.GetZ()),pointA),Vec3V(V_MASKXY)), boxHalfExtentsAndEpsilon)) |
		And(areNegativeTvaluesValid.GetX(), geomPoints::IsPointInBox(And(Add(Scale(lineAtoB, negativePlaneT.GetX()),pointA),Vec3V(V_MASKYZ)), boxHalfExtentsAndEpsilon)) |
		And(areNegativeTvaluesValid.GetY(), geomPoints::IsPointInBox(And(Add(Scale(lineAtoB, negativePlaneT.GetY()),pointA),Vec3V(V_MASKXZ)), boxHalfExtentsAndEpsilon)) |
		And(areNegativeTvaluesValid.GetZ(), geomPoints::IsPointInBox(And(Add(Scale(lineAtoB, negativePlaneT.GetZ()),pointA),Vec3V(V_MASKXY)), boxHalfExtentsAndEpsilon));

	// determine if the capsule segment ever touches the box
	VecBoolV isPointAinBox = IsLessThan(Abs(pointA), boxHalfExtents);
	VecBoolV isPointBinBox = IsLessThan(Abs(pointB), boxHalfExtents);
	BoolV isEitherSegmentEndInsideBox = Or(isPointAinBox.GetX() & isPointAinBox.GetY() & isPointAinBox.GetZ(), isPointBinBox.GetX() & isPointBinBox.GetY() & isPointBinBox.GetZ());
	BoolV segmentTouchesBox = Or(anyValidIntersections, isEitherSegmentEndInsideBox);

	if(segmentTouchesBox.Getb())
	{
		// the capsule segment touches the box
		// start out by assuming the edge intersection is best, to save a comparison
		// give the edge a very small bias so that in the case where the edge is parallel to a face, it will get chosen over a capsule end point
		bestDepth = Subtract(bestEdgeDepth, ScalarV(V_FLT_SMALL_6));

		// the point in the face-point collision is determined by which point (A or B) is the deepest into that plane
		Vec3V boxNormalsA = SelectFT(IsGreaterThan(pointA, pointB), Vec3V(V_ONE), Vec3V(V_NEGONE));
		Vec3V boxNormalsB = Negate(boxNormalsA);
		Vec3V boxFacesPointA = Scale(boxNormalsA, boxHalfExtents);
		Vec3V boxFacesPointB = Negate(boxFacesPointA);
		Vec3V depthsPointA = Subtract(Scale(boxFacesPointA, boxNormalsA), Scale(pointA, boxNormalsA));
		Vec3V depthsPointB = Subtract(Scale(boxFacesPointB, boxNormalsB), Scale(pointB, boxNormalsB));

		// check if point A on the capsule and the faces of the box is the best intersection
		UpdateIntersectionIfBetterDepth(depthsPointA.GetX(), SelectFT(VecBoolV(V_MASKX), pointA, boxFacesPointA), Scale(Vec3V(V_X_AXIS_WZERO), boxNormalsA.GetX()), bestDepth, bestPointOnBox, bestNormal);
		UpdateIntersectionIfBetterDepth(depthsPointA.GetY(), SelectFT(VecBoolV(V_MASKY), pointA, boxFacesPointA), Scale(Vec3V(V_Y_AXIS_WZERO), boxNormalsA.GetY()), bestDepth, bestPointOnBox, bestNormal);
		UpdateIntersectionIfBetterDepth(depthsPointA.GetZ(), SelectFT(VecBoolV(V_MASKZ), pointA, boxFacesPointA), Scale(Vec3V(V_Z_AXIS_WZERO), boxNormalsA.GetZ()), bestDepth, bestPointOnBox, bestNormal);

		// check if point B on the capsule and the faces of the box is the best intersection
		UpdateIntersectionIfBetterDepth(depthsPointB.GetX(), SelectFT(VecBoolV(V_MASKX), pointB, boxFacesPointB), Scale(Vec3V(V_X_AXIS_WZERO), boxNormalsB.GetX()), bestDepth, bestPointOnBox, bestNormal);
		UpdateIntersectionIfBetterDepth(depthsPointB.GetY(), SelectFT(VecBoolV(V_MASKY), pointB, boxFacesPointB), Scale(Vec3V(V_Y_AXIS_WZERO), boxNormalsB.GetY()), bestDepth, bestPointOnBox, bestNormal);
		UpdateIntersectionIfBetterDepth(depthsPointB.GetZ(), SelectFT(VecBoolV(V_MASKZ), pointB, boxFacesPointB), Scale(Vec3V(V_Z_AXIS_WZERO), boxNormalsB.GetZ()), bestDepth, bestPointOnBox, bestNormal);

		bestDepth = Add(bestDepth, capsuleRadius);
	}
	else
	{
		// the capsule segment never touches the box
		// start out by assuming the edge intersection is best, to save a comparison
		ScalarV bestDepthSquared = square(bestEdgeDepth);

		// check if point A and its closest box point is the best intersection
		Vec3V closestBoxPointToA = Clamp(pointA, boxNegativeHalfExtents, boxHalfExtents);
		Vec3V normalToPointA = Subtract(pointA, closestBoxPointToA);
		ScalarV depthAtoBoxSquared = MagSquared(normalToPointA);
		UpdateIntersectionIfBetterDepth(depthAtoBoxSquared, closestBoxPointToA, normalToPointA, bestDepthSquared, bestPointOnBox, bestNormal);

		// check if point B and its closest box point is the best intersection
		Vec3V closestBoxPointToB = Clamp(pointB, boxNegativeHalfExtents, boxHalfExtents);
		Vec3V normalToPointB = Subtract(pointB, closestBoxPointToB);
		ScalarV depthBtoBoxSquared = MagSquared(normalToPointB);
		UpdateIntersectionIfBetterDepth(depthBtoBoxSquared, closestBoxPointToB, normalToPointB, bestDepthSquared, bestPointOnBox, bestNormal);

		// check if the closest vertex to the segment and its segment point is the best intersection
		//   this will not always be the closest vertex to the segment, but in that case another intersection should always be better than capsule segment - box vertex.
		Vec3V closestVertexToSegment = SelectFT(IsGreaterThan(closestSegmentPointToBoxCenter, Vec3V(V_ZERO)), boxNegativeHalfExtents, boxHalfExtents);
		Vec3V closestSegmentPointToClosestVertex = Add(Scale(vectorAtoB, Clamp(Dot(Subtract(closestVertexToSegment, pointA), vectorAtoB), ScalarV(V_ZERO), lineAtoBlength)), pointA);
		Vec3V normalFromClosestVertex = Subtract(closestSegmentPointToClosestVertex, closestVertexToSegment);
		ScalarV depthClosestVertexToSegmentSquared = MagSquared(normalFromClosestVertex);
		UpdateIntersectionIfBetterDepth(depthClosestVertexToSegmentSquared, closestVertexToSegment, normalFromClosestVertex, bestDepthSquared, bestPointOnBox, bestNormal);

		// since these normals are calculated through positions, it is possible for a zero normal.
		//   In these cases the normal will be the face normal of the closest face to the intersection
		//   point.
		Vec3V unscaledPointOnBox = Abs(Scale(bestPointOnBox, Invert(boxHalfExtents)));
		Vec3V closestFaceNormals = SelectFT(IsGreaterThan(bestPointOnBox, Vec3V(V_ZERO)), Vec3V(V_NEGONE), Vec3V(V_ONE));
		VecBoolV lessThanRight = IsLessThan(unscaledPointOnBox, unscaledPointOnBox.Get<Vec::Y, Vec::Z, Vec::X>());
		Vec3V backupNormal = SelectFT(lessThanRight.GetX(),	SelectFT(lessThanRight.GetZ(), Vec3V(V_Z_AXIS_WZERO), Vec3V(V_X_AXIS_WZERO)),
			SelectFT(lessThanRight.GetY(), Vec3V(V_Y_AXIS_WZERO), Vec3V(V_Z_AXIS_WZERO)));

		bestNormal = NormalizeSafe(bestNormal, Scale(backupNormal, closestFaceNormals));
		bestDepth = Subtract(capsuleRadius, Sqrt(bestDepthSquared));
	}

	depth = bestDepth;
	normalOnBox = bestNormal;
	positionOnBox = bestPointOnBox;

	return IsGreaterThan(bestDepth,ScalarV(V_ZERO));
}

BoolV_Out geomCapsule::CapsuleAlignedCapsuleIntersect(	Vec3V_In segmentStartA, Vec3V_In segmentA, ScalarV_In radiusA, 
														ScalarV_In halfLengthB, ScalarV_In radiusB,
														Vec3V_InOut positionOnB, Vec3V_InOut normalOnB, ScalarV_InOut depth )
{
	const Vec3V segmentStartB = And(Vec3V(V_MASKY),Vec3V(halfLengthB));
	const Vec3V segmentB = Negate(Add(segmentStartB,segmentStartB));

	// Find the closest points between the two capsules
	Vec3V closestPointOnSegmentA;
	Vec3V closestPointOnSegmentB;
	geomPoints::FindClosestPointsSegToSeg(closestPointOnSegmentA,closestPointOnSegmentB,segmentStartA,segmentA,segmentStartB,segmentB);

	// Find the normal, this is a little tricky since we need to deal with zero length and parallel segments
	// If the vector between segments is non-zero use that as the normal
	// Otherwise it means that the segments touch each other. Use the cross product between segment A and the Y-axis (segment B is aligned with Y-axis) if it's non-zero
	// Otherwise it means the shape segment is zero or aligned with the Y-axis, either way we can use the Z-axis as a normal safely
	const Vec3V segmentBtoSegmentA = Subtract(closestPointOnSegmentA,closestPointOnSegmentB);
	const Vec3V segmentCrossProduct = Cross(segmentA,Vec3V(V_Y_AXIS_WZERO));
	const Vec3V solveDirection =	SelectFT(	IsLessThan(MagSquared(segmentBtoSegmentA),ScalarV(V_FLT_SMALL_6)),segmentBtoSegmentA,
									SelectFT(	IsLessThan(MagSquared(segmentCrossProduct),ScalarV(V_FLT_SMALL_6)),segmentCrossProduct,
												Vec3V(V_Z_AXIS_WZERO)));
	const Vec3V solveNormal = Normalize(solveDirection);

	// Just dot the normal with the vector between segments to get the depth. If the segment vector is non-zero
	//   then the normal is the normalized version of it the dot product equals the magnitude, otherwise the dot
	//   product will be zero regardless of what we dot it with. 
	depth = Subtract(Add(radiusA,radiusB), Dot(solveNormal,segmentBtoSegmentA));
	positionOnB = AddScaled(closestPointOnSegmentB,solveNormal,radiusB);
	normalOnB = solveNormal;

	return IsGreaterThan(depth,ScalarV(V_ZERO));
}


BoolV_Out geomCapsule::CapsuleSphereIntersect(	Vec3V_In capsuleSegmentStart, Vec3V_In capsuleSegment, ScalarV_In capsuleRadius,
												Vec3V_In sphereCenter, ScalarV_In sphereRadius,
												Vec3V_InOut positionOnSphere, Vec3V_InOut normalOnSphere, ScalarV_InOut depth)
{
	// Get the closest point on the segment to the sphere center
	const Vec3V closestPointOnSegment = geomPoints::FindClosestPointSegToPoint(capsuleSegmentStart,capsuleSegment,sphereCenter);

	// Find the normal, this is a little tricky since we need to deal with zero length and parallel segments
	// If the vector to the segment is non-zero use that
	// Otherwise it means that the segment touches the sphere center, we need to use a vector perpendicular to the segment, so cross it with the Y-axis and use the result if it's non-zero
	// Otherwise it means the segment is zero or aligned with the Y-axis, either way we can use the Z-axis as a normal safely
	const Vec3V segmentCrossProduct = Cross(capsuleSegment,Vec3V(V_Y_AXIS_WZERO));
	const Vec3V solveDirection =	SelectFT(	IsLessThan(MagSquared(closestPointOnSegment),ScalarV(V_FLT_SMALL_6)),closestPointOnSegment,
									SelectFT(	IsLessThan(MagSquared(segmentCrossProduct),ScalarV(V_FLT_SMALL_6)),segmentCrossProduct,
												Vec3V(V_Z_AXIS_WZERO)));
	const Vec3V solveNormal = Normalize(solveDirection);

	// Just dot the normal with the vector to the segment to get the depth. If the segment vector is non-zero
	//   then the normal is the normalized version of it the dot product equals the magnitude, otherwise the 
	//   dot product will be zero regardless of what we dot it with. 
	depth = Subtract(Add(sphereRadius,capsuleRadius), Dot(solveNormal,closestPointOnSegment));
	positionOnSphere = AddScaled(sphereCenter,solveNormal,sphereRadius);
	normalOnSphere = solveNormal;

	return IsGreaterThan(depth,ScalarV(V_ZERO));

}
__forceinline void TaperedSweptSphereVertexIntersectionHelper(Vec3V_In segmentStart, Vec3V_In segment, Vec3V_In vertex, ScalarV_In quadraticA, ScalarV_In radiusQuadraticHalfB, ScalarV_In radiusQuadraticC, ScalarV_InOut currentFraction, Vec3V_InOut currentPosition)
{
	Vec3V vertexToSegmentStart = Subtract(segmentStart, vertex);

	ScalarV newFraction;
	BoolV isIntersectionValid = geomTValues::RealQuadraticFirstRoot(quadraticA, Subtract(Dot(segment,vertexToSegmentStart), radiusQuadraticHalfB), Subtract(Dot(vertexToSegmentStart,vertexToSegmentStart), radiusQuadraticC), newFraction);
	isIntersectionValid = And(isIntersectionValid, IsLessThan(newFraction, currentFraction));

	currentFraction = SelectFT(isIntersectionValid, currentFraction, newFraction);
	currentPosition = SelectFT(isIntersectionValid, currentPosition, vertex);
}

__forceinline void TaperedSweptSphereEdgeIntersectionHelper(Vec3V_In segmentStart, Vec3V_In segment, Vec3V_In vertex0, Vec3V_In vertex1, ScalarV_In radiusQuadraticA, ScalarV_In radiusQuadraticHalfB, ScalarV_In radiusQuadraticC, ScalarV_InOut currentFraction, Vec3V_InOut currentPosition)
{
	// Calculate the segment projected onto the edge direction (remove the "edge component" from the segment)
	Vec3V edge0to1 = Subtract(vertex1, vertex0);
	ScalarV invEdgeLengthSquared = InvMagSquaredSafe(edge0to1, ScalarV(V_ZERO));
	Vec3V segmentPerpendicular = SubtractScaled(segment, edge0to1, Scale(Dot(segment,edge0to1),invEdgeLengthSquared));
	Vec3V vertex0toSegmentStart = Subtract(segmentStart, vertex0);
	Vec3V segmentStartPerpendicular = SubtractScaled(vertex0toSegmentStart, edge0to1, Scale(Dot(vertex0toSegmentStart,edge0to1), invEdgeLengthSquared));

	// Calculate the time of intersection against the infinite tube of the edge
	ScalarV newFraction;
	BoolV isIntersectionValid = geomTValues::RealQuadraticFirstRoot(Subtract(Dot(segmentPerpendicular,segmentPerpendicular),radiusQuadraticA), Subtract(Dot(segmentPerpendicular,segmentStartPerpendicular), radiusQuadraticHalfB), Subtract(Dot(segmentStartPerpendicular,segmentStartPerpendicular), radiusQuadraticC), newFraction);
	isIntersectionValid = And(isIntersectionValid, IsLessThan(newFraction, currentFraction));

	// Make sure that the intersection point is on the infinite tube is contained by the edge
	Vec3V positionOnSegment = AddScaled(segmentStart, segment, newFraction);
	ScalarV fractionAlongEdge = Scale(Dot(Subtract(positionOnSegment,vertex0), edge0to1), invEdgeLengthSquared);
	Vec3V newPosition = AddScaled(vertex0, edge0to1, fractionAlongEdge);
	isIntersectionValid = And(isIntersectionValid, And(IsGreaterThan(fractionAlongEdge, ScalarV(V_ZERO)),IsLessThan(fractionAlongEdge, ScalarV(V_ONE))));

	// If the new intersection is valid, update the output
	currentFraction = SelectFT(isIntersectionValid, currentFraction, newFraction);
	currentPosition = SelectFT(isIntersectionValid, currentPosition, newPosition);
}

BoolV_Out geomTaperedSweptSphere::TaperedSweptSphereTriangleIntersect(	Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In initialRadius, ScalarV_In finalRadius,
																		Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_In triangleFaceNormal,
																		Vec3V_InOut positionOnTriangle, Vec3V_InOut normalOnTriangle, ScalarV_InOut fractionAlongSegment)
{
	// This function is just going to brute force it. It will test the against the triangle face,
	//   then the 3 edges, then the 3 vertices. The final intersection is the one with the minimum
	//   fraction.
	// If any intersection fraction is less than 0 it means there can't be an intersection.
	fractionAlongSegment = ScalarV(V_FLT_MAX);

	Vec3V segment = Subtract(segmentEnd,segmentStart);
	ScalarV radiusGrowth = Subtract(finalRadius, initialRadius);

	// Make sure the triangle normal isn't facing the same direction as the segment
	// This lets the function handle back faces completely, since the edges and vertices already do
	Vec3V segmentStartToVertex0 = Subtract(vertex0,segmentStart);
	Vec3V triangleNormalFacingSegment = SelectFT(IsLessThan(Dot(triangleFaceNormal,segmentStartToVertex0), ScalarV(V_ZERO)), Negate(triangleFaceNormal), triangleFaceNormal);

	// Test the front face of the triangle
	// Plane Equation for tapered swept sphere
	//		N.(P + t*V) + t*(Rf-Ri) = N.v0 + Ri
	//		t*(V.N) + t*(Rf-Ri) = N.v0 - N.P + Ri
	//		t*(V.N + (Rf-Ri)) = N.(v0-P) + Ri
	//		t = (N.(v0-P) + Ri)/(V.N + (Rf-Ri))
	ScalarV denominator = Subtract(Dot(segment, triangleNormalFacingSegment), radiusGrowth);
	BoolV isPlaneCollisionValid = IsLessThan(denominator,Negate(ScalarV(V_FLT_SMALL_12)));
	ScalarV planeFractionAlongSegment = InvScale(Add(initialRadius, Dot(triangleNormalFacingSegment, segmentStartToVertex0)), denominator);
	Vec3V positionOnRaisedPlane = AddScaled(segmentStart, segment, planeFractionAlongSegment);
	Vec3V positionOnTrianglePlane = SubtractScaled(positionOnRaisedPlane, triangleNormalFacingSegment, Dot(Subtract(positionOnRaisedPlane,vertex0),triangleNormalFacingSegment));
	isPlaneCollisionValid = And(isPlaneCollisionValid, IsSegmentTriangleResultValid(Subtract(vertex1,vertex0), Subtract(vertex2,vertex0), Subtract(positionOnTrianglePlane,vertex0), planeFractionAlongSegment));
	fractionAlongSegment = SelectFT(isPlaneCollisionValid, fractionAlongSegment, planeFractionAlongSegment);
	positionOnTriangle = SelectFT(isPlaneCollisionValid, positionOnTriangle, positionOnTrianglePlane);

	// Circle/Sphere equation for tapered swept spheres
	//		|(P-v0) + t*V| = Ri + t*(Rf-Ri)
	//		|(P-v0) + t*V|^2 = (Ri + t*(Rf-Ri))^2
	//		t^2*(V.V) + 2t*((P-v0).V) + (P-v0) = t^2*((Rf-Ri)^2) + 2t*((Rf-Ri)*Ri) + Ri
	//		t^2*((V.V) - (Rf-Ri)) + 2t*(((P-v0).V) - (Rf-Ri)*Ri) + ((P-v0) - Ri) = 0
	//	At this point the quadratic formula can be used
	//		A = V.V - (Rf-Ri)
	//		half B = (P-v0).V - (Rf-Ri)*Ri
	//		C = (P-v0) - Ri
	//  V, P, and v0 can change depending on the test, but Rf and Ri will never change.
	//		This means we can calculate them first and pass them into the functions
	ScalarV radiusQuadraticA = square(radiusGrowth);
	ScalarV radiusQuadraticHalfB = Scale(initialRadius, radiusGrowth);
	ScalarV radiusQuadraticC = square(initialRadius);

	// Test the 3 edges of the triangle
	TaperedSweptSphereEdgeIntersectionHelper(segmentStart, segment, vertex0, vertex1, radiusQuadraticA, radiusQuadraticHalfB, radiusQuadraticC, fractionAlongSegment, positionOnTriangle);
	TaperedSweptSphereEdgeIntersectionHelper(segmentStart, segment, vertex1, vertex2, radiusQuadraticA, radiusQuadraticHalfB, radiusQuadraticC, fractionAlongSegment, positionOnTriangle);
	TaperedSweptSphereEdgeIntersectionHelper(segmentStart, segment, vertex2, vertex0, radiusQuadraticA, radiusQuadraticHalfB, radiusQuadraticC, fractionAlongSegment, positionOnTriangle);

	// Test the 3 vertices of the triangle
	// The quadratic A variable is the same for all vertex intersections, so calculate it first
	ScalarV vertexQuadraticA = Subtract(Dot(segment,segment),radiusQuadraticA);
	TaperedSweptSphereVertexIntersectionHelper(segmentStart, segment, vertex0, vertexQuadraticA, radiusQuadraticHalfB, radiusQuadraticC, fractionAlongSegment, positionOnTriangle);
	TaperedSweptSphereVertexIntersectionHelper(segmentStart, segment, vertex1, vertexQuadraticA, radiusQuadraticHalfB, radiusQuadraticC, fractionAlongSegment, positionOnTriangle);
	TaperedSweptSphereVertexIntersectionHelper(segmentStart, segment, vertex2, vertexQuadraticA, radiusQuadraticHalfB, radiusQuadraticC, fractionAlongSegment, positionOnTriangle);

	// The normal is the vector from the intersection position on the triangle to the center of the tapered swept sphere at time of intersection.
	// If this vector is zero, that means the radius at this point must be zero as well, so using the triangle's normal will work
	normalOnTriangle = NormalizeSafe(Subtract(AddScaled(segmentStart, segment, fractionAlongSegment), positionOnTriangle), triangleNormalFacingSegment);

	return And(IsGreaterThanOrEqual(fractionAlongSegment, ScalarV(V_ZERO)), IsLessThanOrEqual(fractionAlongSegment, ScalarV(V_ONE)));
}




__forceinline void SweptSphereVertexIntersectionHelper(Vec3V_In segmentStart, Vec3V_In segment, Vec3V_In vertex, ScalarV_In quadraticA, ScalarV_In radiusSquared, ScalarV_InOut currentFraction, Vec3V_InOut currentPosition)
{
	Vec3V vertexToSegmentStart = Subtract(segmentStart, vertex);

	ScalarV newFraction;
	BoolV isIntersectionValid = geomTValues::RealQuadraticFirstRoot(quadraticA, Dot(segment,vertexToSegmentStart), Subtract(Dot(vertexToSegmentStart,vertexToSegmentStart), radiusSquared), newFraction);
	isIntersectionValid = And(isIntersectionValid, IsLessThan(newFraction, currentFraction));

	currentFraction = SelectFT(isIntersectionValid, currentFraction, newFraction);
	currentPosition = SelectFT(isIntersectionValid, currentPosition, vertex);
}

__forceinline void SweptSphereEdgeIntersectionHelper(Vec3V_In segmentStart, Vec3V_In segment, Vec3V_In vertex0, Vec3V_In vertex1, ScalarV_In radiusSquared, ScalarV_InOut currentFraction, Vec3V_InOut currentPosition)
{
	// Calculate the segment projected onto the edge direction (remove the "edge component" from the segment)
	Vec3V edge0to1 = Subtract(vertex1, vertex0);
	ScalarV invEdgeLengthSquared = InvMagSquaredSafe(edge0to1, ScalarV(V_ZERO));
	Vec3V segmentPerpendicular = SubtractScaled(segment, edge0to1, Scale(Dot(segment,edge0to1),invEdgeLengthSquared));
	Vec3V vertex0toSegmentStart = Subtract(segmentStart, vertex0);
	Vec3V segmentStartPerpendicular = SubtractScaled(vertex0toSegmentStart, edge0to1, Scale(Dot(vertex0toSegmentStart,edge0to1), invEdgeLengthSquared));

	// Calculate the time of intersection against the infinite tube of the edge
	ScalarV newFraction;
	BoolV isIntersectionValid = geomTValues::RealQuadraticFirstRoot(Dot(segmentPerpendicular,segmentPerpendicular), Dot(segmentPerpendicular,segmentStartPerpendicular), Subtract(Dot(segmentStartPerpendicular,segmentStartPerpendicular), radiusSquared), newFraction);
	isIntersectionValid = And(isIntersectionValid, IsLessThan(newFraction, currentFraction));

	// Make sure that the intersection point is on the infinite tube is contained by the edge
	Vec3V positionOnSegment = AddScaled(segmentStart, segment, newFraction);
	ScalarV fractionAlongEdge = Scale(Dot(Subtract(positionOnSegment,vertex0), edge0to1), invEdgeLengthSquared);
	Vec3V newPosition = AddScaled(vertex0, edge0to1, fractionAlongEdge);
	isIntersectionValid = And(isIntersectionValid, And(IsGreaterThan(fractionAlongEdge, ScalarV(V_ZERO)),IsLessThan(fractionAlongEdge, ScalarV(V_ONE))));

	// If the new intersection is valid, update the output
	currentFraction = SelectFT(isIntersectionValid, currentFraction, newFraction);
	currentPosition = SelectFT(isIntersectionValid, currentPosition, newPosition);
}


BoolV_Out geomSweptSphere::SweptSphereTriangleIntersect(Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In radius,
														Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_In triangleFaceNormal,
														Vec3V_InOut positionOnTriangle, Vec3V_InOut normalOnTriangle, ScalarV_InOut fractionAlongSegment)
{
	// This function is just going to brute force it. It will test the against the triangle face,
	//   then the 3 edges, then the 3 vertices. The final intersection is the one with the minimum
	//   fraction.
	// If any intersection fraction is less than 0 it means there can't be an intersection.
	fractionAlongSegment = ScalarV(V_FLT_MAX);

	Vec3V segment = Subtract(segmentEnd,segmentStart);

	// Make sure the triangle normal isn't facing the same direction as the segment
	// This lets the function handle back faces completely, since the edges and vertices already do
	Vec3V segmentStartToVertex0 = Subtract(vertex0,segmentStart);
	Vec3V triangleNormalFacingSegment = SelectFT(IsLessThan(Dot(triangleFaceNormal,segmentStartToVertex0), ScalarV(V_ZERO)), Negate(triangleFaceNormal), triangleFaceNormal);

	// Test the front face of the triangle
	// Plane Equation for swept sphere
	//		N.(P + t*V) = N.v0 + R
	//		t*(V.N) = N.v0 - N.P + R
	//		t*(V.N) = N.(v0-P) + R
	//		t = (N.(v0-P) + R)/(V.N)
	ScalarV denominator = Dot(segment, triangleNormalFacingSegment);
	BoolV isPlaneCollisionValid = IsLessThan(denominator,Negate(ScalarV(V_FLT_SMALL_12)));
	ScalarV planeFractionAlongSegment = InvScale(Add(radius, Dot(triangleNormalFacingSegment, segmentStartToVertex0)), denominator);
	Vec3V positionOnRaisedPlane = AddScaled(segmentStart, segment, planeFractionAlongSegment);
	Vec3V positionOnTrianglePlane = SubtractScaled(positionOnRaisedPlane, triangleNormalFacingSegment, Dot(Subtract(positionOnRaisedPlane,vertex0),triangleNormalFacingSegment));
	isPlaneCollisionValid = And(isPlaneCollisionValid, IsSegmentTriangleResultValid(Subtract(vertex1,vertex0), Subtract(vertex2,vertex0), Subtract(positionOnTrianglePlane,vertex0), planeFractionAlongSegment));
	fractionAlongSegment = SelectFT(isPlaneCollisionValid, fractionAlongSegment, planeFractionAlongSegment);
	positionOnTriangle = SelectFT(isPlaneCollisionValid, positionOnTriangle, positionOnTrianglePlane);

	// Circle/Sphere equation for swept spheres
	//		|(P-v0) + t*V| = R
	//		|(P-v0) + t*V|^2 = R
	//		t^2*(V.V) + 2t*((P-v0).V) + (P-v0) = R
	//		t^2*(V.V) + 2t*((P-v0).V) + ((P-v0) - R) = 0
	//	At this point the quadratic formula can be used
	//		A = V.V
	//		half B = (P-v0).V
	//		C = (P-v0) - R
	//  V, P, and v0 can change depending on the test, but Rf and Ri will never change.
	//		This means we can calculate them first and pass them into the functions
	ScalarV radiusSquared = Scale(radius,radius);

	// Test the 3 edges of the triangle
	SweptSphereEdgeIntersectionHelper(segmentStart, segment, vertex0, vertex1, radiusSquared, fractionAlongSegment, positionOnTriangle);
	SweptSphereEdgeIntersectionHelper(segmentStart, segment, vertex1, vertex2, radiusSquared, fractionAlongSegment, positionOnTriangle);
	SweptSphereEdgeIntersectionHelper(segmentStart, segment, vertex2, vertex0, radiusSquared, fractionAlongSegment, positionOnTriangle);

	// Test the 3 vertices of the triangle
	// The quadratic A variable is the same for all vertex intersections, so calculate it first
	ScalarV vertexQuadraticA = Dot(segment,segment);
	SweptSphereVertexIntersectionHelper(segmentStart, segment, vertex0, vertexQuadraticA, radiusSquared, fractionAlongSegment, positionOnTriangle);
	SweptSphereVertexIntersectionHelper(segmentStart, segment, vertex1, vertexQuadraticA, radiusSquared, fractionAlongSegment, positionOnTriangle);
	SweptSphereVertexIntersectionHelper(segmentStart, segment, vertex2, vertexQuadraticA, radiusSquared, fractionAlongSegment, positionOnTriangle);

	// The normal is the vector from the intersection position on the triangle to the center of the tapered swept sphere at time of intersection.
	// If this vector is zero, that means the radius at this point must be zero as well, so using the triangle's normal will work
	normalOnTriangle = NormalizeSafe(Subtract(AddScaled(segmentStart, segment, fractionAlongSegment), positionOnTriangle), triangleNormalFacingSegment);

	return And(IsGreaterThanOrEqual(fractionAlongSegment, ScalarV(V_ZERO)), IsLessThanOrEqual(fractionAlongSegment, ScalarV(V_ONE)));
}


BoolV_Out geomSweptSphere::SweptSphereBoxIntersect(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, ScalarV_In radius,
													Vec3V_In boxHalfExtents,
													Vec3V_InOut positionOnBox, Vec3V_InOut normalOnBox, ScalarV_InOut fractionAlongSegment)
{
	// The idea of this test is to, instead of a swept sphere vs. box test, do a probe vs.
	//   chamfered box test. First, the probe is intersected against the box formed by 
	//   the initial box plus the radius. The position of the intersection will show
	//   what parts of the chamfered box to test against.
	Vec3V pointA = segmentStart;
	Vec3V lineAtoB = segmentStartToEnd;

	// There needs to be a minimum radius of 0.1cm or it is possible to miss edge and vertex collisions due to numerical errors
	ScalarV sphereRadius = Max(radius, ScalarV(V_FLT_SMALL_3)); 
	ScalarV sphereRadiusSquared = Scale(sphereRadius, sphereRadius);

	Vec3V innerBoxHalfExtents = boxHalfExtents;
	Vec3V negativeInnerBoxHalfExtents = Negate(innerBoxHalfExtents);

	// If the starting point is inside of the chamfered box, or the segment is going away from the box,
	//   there can be no collision. Code further on makes the assumption that this is not the case.
	Vec3V innerBoxToPointA = Subtract(pointA, Clamp(pointA, negativeInnerBoxHalfExtents, innerBoxHalfExtents));
	BoolV isInsideChamferedBox = IsLessThan(MagSquared(innerBoxToPointA), sphereRadiusSquared);
	BoolV isGoingAwayFromBox = IsGreaterThan(Dot(innerBoxToPointA, lineAtoB), ScalarV(V_ZERO));
	if(Or(isInsideChamferedBox, isGoingAwayFromBox).Getb())
	{
		return BoolV(V_FALSE);
	}

	// Intersect the line with the outer box planes nearest point A
	Vec3V quadrantPointA = SelectFT(IsGreaterThan(pointA, Vec3V(V_ZERO)), Vec3V(V_NEGONE), Vec3V(V_ONE));
	Vec3V outerBoxHalfExtents = Add(innerBoxHalfExtents, Vec3V(sphereRadius));
	VecBoolV areSlopesValid = !IsEqual(lineAtoB, Vec3V(V_ZERO)); // Test
	Vec3V inverseSlope = Invert(lineAtoB);
	Vec3V outerBoxT = Scale(Subtract(Scale(quadrantPointA, outerBoxHalfExtents), pointA), inverseSlope);

	// Find out which intersections against the outer box are valid. 
	// Negative T values are allowed since the initial point can be inside the outer box but outside of the chamfered box
	VecBoolV areIntersectionsOnBox(V_F_F_F_F);
	Vec3V outerBoxHalfExtentsAndEpsilon = Add(outerBoxHalfExtents,Vec3V(V_FLT_SMALL_6));
	areIntersectionsOnBox.SetX(geomPoints::IsPointInBox(And(AddScaled(pointA, lineAtoB, outerBoxT.GetX()),Vec3V(V_MASKYZ)), outerBoxHalfExtentsAndEpsilon)); 									
	areIntersectionsOnBox.SetY(geomPoints::IsPointInBox(And(AddScaled(pointA, lineAtoB, outerBoxT.GetY()),Vec3V(V_MASKXZ)), outerBoxHalfExtentsAndEpsilon)); 				
	areIntersectionsOnBox.SetZ(geomPoints::IsPointInBox(And(AddScaled(pointA, lineAtoB, outerBoxT.GetZ()),Vec3V(V_MASKXY)), outerBoxHalfExtentsAndEpsilon));
	VecBoolV areIntersectionsValid = And(areIntersectionsOnBox, areSlopesValid);
	outerBoxT = SelectFT(areIntersectionsValid, Vec3V(V_FLT_MAX), outerBoxT);
	ScalarV bestOuterBoxT = Min(outerBoxT.GetX(), Min(outerBoxT.GetY(), outerBoxT.GetZ()));

	// Calculate the intersection on the outer box, and get the closest point to that on the inner box
	Vec3V outerBoxPosition = AddScaled(pointA, lineAtoB, bestOuterBoxT);
	Vec3V innerBoxPosition = Clamp(outerBoxPosition, negativeInnerBoxHalfExtents, innerBoxHalfExtents);

	// Find out which dimensions of the outer box position are inside of the inner box position, this will determine if it is a vertex, edge, or face.
	// If the t-value is invalid, setting the vector to T_T_T will cause no intersection to be returned.
	VecBoolV dimensionInsideOfInnerBox = Or(IsLessThan(Abs(outerBoxPosition), innerBoxHalfExtents), VecBoolV(IsEqual(bestOuterBoxT, ScalarV(V_FLT_MAX))));

	// Count the number of dimensions inside of the box.
	// This doesn't appear to be any slower than using SelectFTs with integers stored in ScalarVs
	int dimensionsInsideOfInnerBox;
	dimensionsInsideOfInnerBox  = dimensionInsideOfInnerBox.GetX().Getb() ? 1 : 0;
	dimensionsInsideOfInnerBox += dimensionInsideOfInnerBox.GetY().Getb() ? 1 : 0;
	dimensionsInsideOfInnerBox += dimensionInsideOfInnerBox.GetZ().Getb() ? 1 : 0;

	switch(dimensionsInsideOfInnerBox)
	{
		case 0:
		{
			// A vertex section of the outer box was hit
			// The sphere could intersect any of the 3 edges next to this vertex
			//   or any of the 4 vertices on those edges

			// Find the first inner plane the probe passes
			Vec3V innerBoxT = Scale(Subtract(innerBoxPosition, outerBoxPosition), inverseSlope);
			innerBoxT = SelectFT(And(areSlopesValid, IsGreaterThan(innerBoxT, Vec3V(V_ZERO))), Vec3V(V_FLT_MAX), innerBoxT);

			// Select the axis with lowest valid T as the edge axis. This is the edge that the probe goes towards after
			//   intersecting the vertex.
			const VecBoolV isLessThanRightComponent = IsLessThan(innerBoxT, innerBoxT.Get<Vec::Y, Vec::Z, Vec::X>());
			const VecBoolV isLessThanOrEqualLeftComponent = !isLessThanRightComponent.Get<Vec::Z, Vec::X, Vec::Y>();
			dimensionInsideOfInnerBox = And(isLessThanRightComponent, isLessThanOrEqualLeftComponent);

			// Map F_F_F to F_F_T, to ensure that there is always one valid edge
			dimensionInsideOfInnerBox.SetZ(!Xor(dimensionInsideOfInnerBox.GetX(), dimensionInsideOfInnerBox.GetY()));

			// Fall through to the edge case, now that the edge axis is set
		}
		case 1:
		{
			// An edge section of the outer box was hit
			// The sphere could intersect the edge or either of the two vertices
			Vec3V edgePlaneMask = Vec3V(!dimensionInsideOfInnerBox);
			Vec3V edgeDirectionmask = Vec3V(dimensionInsideOfInnerBox);
			Vec3V edgeMidpoint = And(edgePlaneMask, innerBoxPosition);
			Vec3V edgeDirection = And(edgeDirectionmask, Vec3V(V_ONE));
			ScalarV edgeHalfLength = Dot(edgeDirection, innerBoxHalfExtents);
			Vec3V edgeToPointA = Subtract(pointA, edgeMidpoint);

			// Find the T value of the probe against the infinite tube formed by the box edge and swept sphere radius
			ScalarV edgeT;
			Vec3V projEdgeToPointA = And(edgePlaneMask, edgeToPointA);
			Vec3V projLineAtoB = And(edgePlaneMask, lineAtoB);
			BoolV isEdgeIntersectionValid = geomTValues::RealQuadraticFirstRoot(Dot(projLineAtoB, projLineAtoB), Dot(projLineAtoB, projEdgeToPointA), Subtract(Dot(projEdgeToPointA, projEdgeToPointA), sphereRadiusSquared), edgeT);

			// Don't allow intersections coming from inside of the edge
			isEdgeIntersectionValid = And(isEdgeIntersectionValid, IsGreaterThan(edgeT, ScalarV(V_ZERO)));

			// The vertex closest to the edge position is tested against. If there is no valid edge intersection, 
			//   the only vertex that could be hit is the one closest to the probe start position, so just use that.
			Vec3V edgeHitPosition = SelectFT(isEdgeIntersectionValid, pointA, AddScaled(pointA, lineAtoB, edgeT));
			ScalarV edgeHitPositionOffset = Dot(edgeHitPosition, edgeDirection);

			// Choose the vertex closest to the hit position to test against
			Vec3V vertex = AddScaled(edgeMidpoint, edgeDirection, SelectFT(IsGreaterThan(edgeHitPositionOffset, ScalarV(V_ZERO)), Negate(edgeHalfLength), edgeHalfLength));

			// Find the T value of the probe against the sphere formed by the box vertex and swept sphere radius
			Vec3V vertexToPointA = Subtract(pointA, vertex);
			ScalarV vertexT;
			BoolV isVertexIntersectionValid = geomTValues::RealQuadraticFirstRoot(Dot(lineAtoB, lineAtoB), Dot(lineAtoB, vertexToPointA), Subtract(Dot(vertexToPointA, vertexToPointA), sphereRadiusSquared), vertexT);
			Vec3V vertexHitPosition = AddScaled(pointA, lineAtoB, vertexT);
			ScalarV vertexHitPositionOffset = Dot(vertexHitPosition, edgeDirection);

			// In order to be a valid intersection, and edge intersection must be inside of the inner box, and vertex intersection must be outside
			isVertexIntersectionValid = And(isVertexIntersectionValid, IsGreaterThan(Abs(vertexHitPositionOffset), Subtract(edgeHalfLength, ScalarV(V_FLT_SMALL_4))));
			isEdgeIntersectionValid = And(isEdgeIntersectionValid, IsLessThan(Abs(edgeHitPositionOffset), Add(edgeHalfLength, ScalarV(V_FLT_SMALL_4))));

			// Invalidate T values if the intersection isn't valid. If both are invalid this will get thrown out later in the function
			edgeT = SelectFT(isEdgeIntersectionValid, ScalarV(V_FLT_MAX), edgeT);
			vertexT = SelectFT(isVertexIntersectionValid, ScalarV(V_FLT_MAX), vertexT);

			// Use the intersection with the lowest T
			BoolV useVertexIntersection = IsLessThan(vertexT, edgeT);
			fractionAlongSegment = SelectFT(useVertexIntersection, edgeT, vertexT);
			positionOnBox = Clamp(SelectFT(useVertexIntersection, edgeHitPosition, vertexHitPosition), negativeInnerBoxHalfExtents, innerBoxHalfExtents);
			normalOnBox = Normalize(Subtract(AddScaled(pointA, lineAtoB, fractionAlongSegment), positionOnBox));
			return BoolV(V_TRUE);								
		}
		case 2:
		{
			// A face section of the outer box was hit
			// There is only one dimension of the hit position not inside of the inner box.
			//   That dimension, with the sign of the starting quadrant, is the normal
			normalOnBox = And(Vec3V(!dimensionInsideOfInnerBox), quadrantPointA);
			fractionAlongSegment = bestOuterBoxT;
			positionOnBox = innerBoxPosition;
			return BoolV(V_TRUE);
		}
		case 3:
		{
			// If the outer box wasn't hit by the probe, there can be no collision
			return BoolV(V_FALSE);
		}
	}
	Assert(false);
	return BoolV(V_FALSE);
}


BoolV_Out geomSweptSphere::SweptSphereSphereIntersect(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, ScalarV_In sweptSphereRadius,
														ScalarV_In sphereRadius,
														Vec3V_InOut positionOnSphere, Vec3V_InOut normalOnSphere, ScalarV_InOut fractionAlongSegment)
{
	const ScalarV combinedRadius = Add(sphereRadius,sweptSphereRadius);

	// TODO: We can write a better segment-sphere test, especially for directed segments
	ScalarV exitFractionAlongSphere;
	if (geomSegments::SegmentToSphereIntersections(segmentStart,segmentStartToEnd,Scale(combinedRadius,combinedRadius),fractionAlongSegment,exitFractionAlongSphere,true))
	{
		// Don't accept intersections with normals that point away from the segment, this means that the swept-sphere starts out touching the sphere.
		// NOTE: If we were doing a directed segment-sphere test above this branch wouldn't be necessary. 
		const Vec3V directionOnSphere = AddScaled(segmentStart,segmentStartToEnd,fractionAlongSegment);
		if(IsLessThanAll(Dot(directionOnSphere,segmentStartToEnd),ScalarV(V_ZERO)))
		{
			normalOnSphere = Normalize(directionOnSphere);
			positionOnSphere = Scale(normalOnSphere,sphereRadius);
		}
		return BoolV(V_TRUE);
	}

	return BoolV(V_FALSE);
}

BoolV_Out geomSweptSphere::SweptSphereCapsuleIntersect(	Vec3V_In segmentStart, Vec3V_In segmentStartToEnd, ScalarV_In sweptSphereRadius,
														ScalarV_In capsuleLength, ScalarV_In capsuleRadius,
														Vec3V_InOut positionOnCapsule, Vec3V_InOut normalOnCapsule, ScalarV_InOut fractionAlongSegment)
{
	// Simplify the test by adding the swept sphere radius to the capsule radius and testing a segment against the enlarged capsule. 
	ScalarV combinedRadius = Add(capsuleRadius,sweptSphereRadius);
	Vec3V positionOnEnlargedCapsule;
	BoolV wasIntersectionFound = geomSegments::SegmentCapsuleIntersectDirected(segmentStart,segmentStartToEnd,capsuleLength,combinedRadius,positionOnEnlargedCapsule,normalOnCapsule,fractionAlongSegment);
	positionOnCapsule = SubtractScaled(positionOnEnlargedCapsule,normalOnCapsule,sweptSphereRadius);
	return wasIntersectionFound;
}

// Replaced usage entirely with TestSphereToSeg
//bool geomSpheres::TestSphereToCapsule (Vec::V3Param128 krvecCenter, Vec::V3Param128 kfRadiusSphere,
//										Vec::V3Param128 krvecP0, Vec::V3Param128_After3Args krvecShaftAxis,
//										Vec::V3Param128_After3Args kfShaftLength, Vec::V3Param128_After3Args kfRadiusCapsule)
//{
//	Vec3V v_krvecCenter(krvecCenter);
//	ScalarV v_kfRadiusSphere(kfRadiusSphere);
//	Vec3V v_krvecP0(krvecP0);
//	Vec3V v_krvecShaftAxis(krvecShaftAxis);
//	Vec3V v_kfShaftLength(kfShaftLength);
//	ScalarV v_kfRadiusCapsule(kfRadiusCapsule);
//
//	return TestSphereToSeg(v_krvecCenter, Add(v_kfRadiusSphere, v_kfRadiusCapsule), v_krvecP0, AddScaled(v_krvecP0, v_krvecShaftAxis, v_kfShaftLength));
//}

bool geomSpheres::TestSphereToSeg (Vec3V_In krvecCenter, ScalarV_In kfRadiusSphere,
									Vec3V_In krvecP0, Vec3V_In krvecP1)
{
	Vec3V segAxis = krvecP1 - krvecP0;
	Vec3V p0ToCenter = krvecCenter - krvecP0;
	// There does not appear to be an intrinsic saturate for PPC so this boils down to __vmaxfp and a min
	// - As such the behavior with INF or NaN is to propagate the special values rather than clamping
	//   Which we don't want here, so that's unfortunate...
	// Gonna just have to go with an added select =(
	ScalarV segAxisLenSqrd = Dot(segAxis, segAxis);
	ScalarV t = SelectFT( IsGreaterThan(segAxisLenSqrd, ScalarV(V_FLT_SMALL_12)), ScalarV(V_ZERO), Saturate(InvScale(Dot(segAxis, p0ToCenter), segAxisLenSqrd)) );

	Vec3V closestPointOnSeg = AddScaled(krvecP0, segAxis, t);
	return IsGreaterThanOrEqualAll(kfRadiusSphere * kfRadiusSphere, DistSquared(krvecCenter, closestPointOnSeg)) != 0;
}


bool geomBoxes::TestCapsuleToAlignedBoxFP (Vec::V3Param128_After3Args a_rSegA, Vec::V3Param128_After3Args a_rSegB,
										   Vec::V4ParamSplatted128 a_fRadius, Vec::V3Param128 a_rBoxMin, Vec::V3Param128 a_rBoxMax)
{
	Vec3V vSegA(a_rSegA), vSegB(a_rSegB);
	ScalarV vRadius(a_fRadius);
	Vec3V vBoxMin(a_rBoxMin), vBoxMax(a_rBoxMax);

	// The bounding box center, and 1/2 the box size.
	Vec3V vecBoxCenter, vecHalfSize;
	ScalarV vHalf(V_HALF);

	// Find the box center.
	vecBoxCenter = vHalf * (vBoxMin + vBoxMax);

	// Find 1/2 the box size.
	vecHalfSize = vHalf * (vBoxMax - vBoxMin);

	// Extend the box by the radius, so as to accomplish a capsule-like intersection.
	vecHalfSize += Vec3V(vRadius);

#if FAST_GEOMETRY
	VecBoolV res = TestSegmentToCenteredAlignedBox( Subtract(vSegA,vecBoxCenter), Subtract(vSegB,vecBoxCenter), vecHalfSize);
	return (IsEqualIntAll( res, VecBoolV(V_T_T_T_T) ) != 0);
#else
	// Then do the normal segment-box test.
	return TestSegmentToCenteredAlignedBox (VEC3V_TO_VECTOR3(vSegA - vecBoxCenter), VEC3V_TO_VECTOR3(vSegB - vecBoxCenter), RCC_VECTOR3(vecHalfSize));
#endif
}


bool geomBoxes::TestTriangleToAlignedBoxFP (const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& min, const Vector3& max)
{
	// Test triangle verts against xyz planes
	if(p0.x<min.x && p1.x<min.x && p2.x<min.x) return false;
	if(p0.x>max.x && p1.x>max.x && p2.x>max.x) return false;
	if(p0.y<min.y && p1.y<min.y && p2.y<min.y) return false;
	if(p0.y>max.y && p1.y>max.y && p2.y>max.y) return false;
	if(p0.z<min.z && p1.z<min.z && p2.z<min.z) return false;
	if(p0.z>max.z && p1.z>max.z && p2.z>max.z) return false;

	// Make box
	Vector3 vtx[8];
	vtx[0].Set(min.x,min.y,min.z);
	vtx[1].Set(max.x,min.y,min.z);
	vtx[2].Set(min.x,max.y,min.z);
	vtx[3].Set(max.x,max.y,min.z);
	vtx[4].Set(min.x,min.y,max.z);
	vtx[5].Set(max.x,min.y,max.z);
	vtx[6].Set(min.x,max.y,max.z);
	vtx[7].Set(max.x,max.y,max.z);

	Vector3 normal;
	Vector3 a, b;
	a.Subtract (p1,p0);
	b.Subtract (p2,p0);
	normal.Cross (a,b);

	if(TestBoxToPlane(p0,vtx,normal)!=0) return false;

	// Test edge 01
	Vector3 edge,cross;
	edge.Subtract(p1,p0);
	cross.Cross(edge,normal);
	if(TestBoxToPlane(p0,vtx,cross)==1) return false;
	if(TestBoxToPlane(p2,vtx,cross)==-1) return false;

	// Test edge 12
	edge.Subtract(p2,p1);
	cross.Cross(edge,normal);
	if(TestBoxToPlane(p1,vtx,cross)==1) return false;
	if(TestBoxToPlane(p0,vtx,cross)==-1) return false;

	// Test edge 20
	edge.Subtract(p0,p2);
	cross.Cross(edge,normal);
	if(TestBoxToPlane(p2,vtx,cross)==1) return false;
	if(TestBoxToPlane(p1,vtx,cross)==-1) return false;

	return true;
}



#if !(__PPU || __XENON || __SPU)

bool geomBoxes::TestPolygonToOrientedBoxFP (Vector3::Vector3Param v0, Vector3::Vector3Param v1, Vector3::Vector3Param v2,
											Vector3::Vector3Param v3, Vector3::Vector3Param normal, Vector3::Vector3Param boxCenter,
											const Matrix34 &boxAxes, Vector3::Vector3Param boxHalfExtents)
{
	Vector3 displacement;
	// The displacement between the box center and one of the triangle's vertices.
	Vector3 edge0, edge1, edge2;
	// Vectors between the first vertex in the polygon and all other vertices.
	// (edge2 is unused if the polygon is a triangle.)
	float p0, p1, p2, p3, pMin, pMax;
	// The projected vertices, and their min/max values.
	float r;
	// The oriented bounding box's radius of projection.
	int i;
	// Used to loop through axes/extents.

	// This test is based on separating axes.  We try the polygon's normal vector and the three
	// coordinate axes.  For completeness, we'd also have to try all 9 or 12 possible cross-products
	// between the box axes and the polygon edges.

	// Shut the stupid compiler up.
	p3 = 0.0f;
	edge2.Set (0.0f);

	// Calculate the displacement between the box center and one of the triangle's vertices.
	displacement.Subtract (v0, boxCenter);

	// Calculate the edge vectors.
	edge0.Subtract (v1, v0);
	edge1.Subtract (v2, v0);
	edge2.Subtract (v3, v0);

	// Determine whether the polygon's normal is a separating axis.
	p0 = normal.Dot (displacement);
	// p1 = p2 = p3 = pMin = pMax = p0.
	r = boxHalfExtents[0] * fabsf (normal.Dot (boxAxes.GetVector(0)))
		+ boxHalfExtents[1] * fabsf (normal.Dot (boxAxes.GetVector(1)))
		+ boxHalfExtents[2] * fabsf (normal.Dot (boxAxes.GetVector(2)));
	if (p0 > r || p0 < -r)
	{
		return false;
	}

	// Determine whether the box's axes are separating axes.
	for (i = 0; i < 3; i++)
	{
		pMin = pMax = p0 = boxAxes.GetVector(i).Dot (displacement);
		p1 = boxAxes.GetVector(i).Dot (edge0) + p0;
		p2 = boxAxes.GetVector(i).Dot (edge1) + p0;
		pMin = Min (pMin, p1);
		pMax = Max (pMax, p1);
		p3 = boxAxes.GetVector(i).Dot (edge2) + p0;
		pMin = Min (pMin, p2);
		pMax = Max (pMax, p2);
		r = boxHalfExtents[i];
		pMin = Min (pMin, p3);
		pMax = Max (pMax, p3);
		if (pMin > r || pMax < -r)
		{
			return false;
		}
	}

	// We couldn't find a separating axis between the polygon and box, so report
	// that they intersect.
	return true;
}

bool geomBoxes::TestPolygonToOrientedBoxFP (Vector3::Vector3Param v0, Vector3::Vector3Param v1, Vector3::Vector3Param v2,
											Vector3::Vector3Param normal, Vector3::Vector3Param boxCenter,
											const Matrix34 &boxAxes, Vector3::Vector3Param boxHalfExtents)
{
	Vector3 displacement;
	// The displacement between the box center and one of the triangle's vertices.
	Vector3 edge0, edge1;
	// Vectors between the first vertex in the polygon and all other vertices.
	// (edge2 is unused if the polygon is a triangle.)
	float p0, p1, p2, pMin, pMax;
	// The projected vertices, and their min/max values.
	float r;
	// The oriented bounding box's radius of projection.
	int i;
	// Used to loop through axes/extents.

	// This test is based on separating axes.  We try the polygon's normal vector and the three
	// coordinate axes.  For completeness, we'd also have to try all 9 or 12 possible cross-products
	// between the box axes and the polygon edges.

	// Calculate the displacement between the box center and one of the triangle's vertices.
	displacement.Subtract (v0, boxCenter);

	// Calculate the edge vectors.
	edge0.Subtract (v1, v0);
	edge1.Subtract (v2, v0);

	// Determine whether the polygon's normal is a separating axis.
	p0 = normal.Dot (displacement);
	// p1 = p2 = p3 = pMin = pMax = p0.
	r = boxHalfExtents[0] * fabsf (normal.Dot (boxAxes.GetVector(0)))
		+ boxHalfExtents[1] * fabsf (normal.Dot (boxAxes.GetVector(1)))
		+ boxHalfExtents[2] * fabsf (normal.Dot (boxAxes.GetVector(2)));
	if (p0 > r || p0 < -r)
	{
		return false;
	}

	// Determine whether the box's axes are separating axes.
	for (i = 0; i < 3; i++)
	{
		pMin = pMax = p0 = boxAxes.GetVector(i).Dot (displacement);
		p1 = boxAxes.GetVector(i).Dot (edge0) + p0;
		p2 = boxAxes.GetVector(i).Dot (edge1) + p0;
		pMin = Min (pMin, p1);
		pMax = Max (pMax, p1);
		pMin = Min (pMin, p2);
		pMax = Max (pMax, p2);
		r = boxHalfExtents[i];
		if (pMin > r || pMax < -r)
		{
			return false;
		}
	}

	// We couldn't find a separating axis between the polygon and box, so report
	// that they intersect.
	return true;
}

#endif


bool geomBoxes::TestTriangleToAlignedBox (const Vector3& v1, const Vector3& v2, const Vector3& v3, 
										   const Vector3& boxMin, const Vector3& boxMax)
{
	// Return true if:
	// 1) a triangle vertex is inside the box
	// 2) or an edge of the triangle intersects the box surface
	// 3) or the lowest corner to highest corner of the box intersects triangle

	// add quick rejects?

	// See if any of the vertices are inside the box.
	if ((v1.x >= boxMin.x && v1.x <= boxMax.x && 
	     v1.y >= boxMin.y && v1.y <= boxMax.y &&
	     v1.z >= boxMin.z && v1.z <= boxMax.z) ||
	    (v2.x >= boxMin.x && v2.x <= boxMax.x && 
	     v2.y >= boxMin.y && v2.y <= boxMax.y &&
	     v2.z >= boxMin.z && v2.z <= boxMax.z) ||
	    (v3.x >= boxMin.x && v3.x <= boxMax.x && 
	     v3.y >= boxMin.y && v3.y <= boxMax.y &&
	     v3.z >= boxMin.z && v3.z <= boxMax.z))
	{
		// At least one triangle vertex is inside the box.
		return true;
	}

	// See if any of the triangle's edges intersect the box.
	Vector3 v1Prime, v2Prime, v3Prime, boxCenter, halfSize;
	boxCenter.Average(boxMin,boxMax);
	halfSize.Average(-boxMin,boxMax);
	v1Prime.Subtract(v1,boxCenter);
	v2Prime.Subtract(v2,boxCenter);
	v3Prime.Subtract(v3,boxCenter);

#if FAST_GEOMETRY

	VecBoolV res = TestSegmentToCenteredAlignedBox( VECTOR3_TO_VEC3V(v1Prime),VECTOR3_TO_VEC3V(v2Prime),VECTOR3_TO_VEC3V(halfSize));
	if( IsEqualIntAll(res, VecBoolV(V_T_T_T_T)) != 0)
		return true;

			 res = TestSegmentToCenteredAlignedBox(VECTOR3_TO_VEC3V(v2Prime),VECTOR3_TO_VEC3V(v3Prime),VECTOR3_TO_VEC3V(halfSize));
	if( IsEqualIntAll(res, VecBoolV(V_T_T_T_T)) != 0)
		return true;

			 res = TestSegmentToCenteredAlignedBox(VECTOR3_TO_VEC3V(v3Prime),VECTOR3_TO_VEC3V(v1Prime),VECTOR3_TO_VEC3V(halfSize));
	if( IsEqualIntAll(res, VecBoolV(V_T_T_T_T)) != 0)
		return true;

#else
	if (TestSegmentToCenteredAlignedBox(v1Prime,v2Prime,halfSize) || 
	    TestSegmentToCenteredAlignedBox(v2Prime,v3Prime,halfSize) || 
	    TestSegmentToCenteredAlignedBox(v3Prime,v1Prime,halfSize))
	{
		// At least one of the triangle's edges intersects the box.
		return true;
	}
#endif

	// See if the segment between opposite corners of the box intersects the triangle.
	Vector3 normal;
	Vector3 edge12,edge13;
	edge12.Subtract(v2,v1);
	edge13.Subtract(v3,v1);
	normal.Cross(edge12,edge13);
	Vector3 corner1, corner2;
	corner1.x = Selectf(normal.x,boxMax.x,boxMin.x);
	corner2.x = Selectf(normal.x,boxMin.x,boxMax.x);
	corner1.y = Selectf(normal.y,boxMax.y,boxMin.y);
	corner2.y = Selectf(normal.y,boxMin.y,boxMax.y);
	corner1.z = Selectf(normal.z,boxMax.z,boxMin.z);
	corner2.z = Selectf(normal.z,boxMin.z,boxMax.z);
	return geomSegments::TestSegmentToTriangle(corner1,corner2,v1,v2,v3);
}


bool geomBoxes::TestSegmentToCenteredAlignedBox (const Vector3 & point1, const Vector3 & point2, const Vector3 & halfSize)
{
	Vector3 segMidpoint, segExtent;
		// The midpoint & extent of the segment.
	Vector3 eXd;
		// The cross-product between the segment's extent, and the displacement between the center of the
		// segment and the center of the box.
	Vector3 absDisplacement, absExtent, absEXD;
		// The absolute value of various vectors.

	// This routine does six separating-axis tests between the segment and box, which completely
	// determines whether or not they intersect.

	// Calculate the segment's midpoint and extent.
	segMidpoint.Average (point1, point2);
	segExtent.Average (-point1, point2);

	// The displacement between the center of the segment and the center of the box, since the
	// box is centered at the origin, is just the center of the segment, so there's no need for
	// a separate calculation.

	// Calculate the cross-product between the segment's extent, and the displacement between the center
	// of the segment and the center of the box.
	eXd.Cross (segExtent, segMidpoint);

	// For most of our calculations, we actually need the absolute value of these vectors.
	absDisplacement.Abs (segMidpoint);
	absExtent.Abs (segExtent);
	absEXD.Abs (eXd);

	// Extend the box by a _small amount (1.0e-6f) so that segments that are exactly on the
	// boundary are inside the box.
	Vector3 halfSizePlus(halfSize);
	halfSizePlus.Add(VEC3_SMALL_FLOAT);

	// Do the 6 separating-axis tests.
	bool condition1 = (absDisplacement.x > halfSizePlus.x + absExtent.x
		|| absDisplacement.y > halfSizePlus.y + absExtent.y
		|| absDisplacement.z > halfSizePlus.z + absExtent.z);

	bool condition2 = (absEXD.x > halfSizePlus.y * absExtent.z + halfSizePlus.z * absExtent.y);

	bool condition3 = (absEXD.y > halfSizePlus.x * absExtent.z + halfSizePlus.z * absExtent.x
		|| absEXD.z > halfSizePlus.x * absExtent.y + halfSizePlus.y * absExtent.x);

	if (condition1 || condition2 || condition3)
	{
		return false;
	}

	// All the separating-axis tests failed.  The segment must intersect the box.
	return true;
}


VecBoolV_Out geomBoxes::TestSegmentToCenteredAlignedBox(Vec3V_In point1, Vec3V_In point2, Vec3V_In halfSize)
{
	Vec3V segMidpoint = Average( point1, point2 );
	Vec3V segExtent = Average( Negate(point1), point2);
	Vec3V eXd = Cross(segExtent, segMidpoint);

	Vec3V absDisplacement = Abs(segMidpoint);
	Vec3V absExtent = Abs(segExtent);
	Vec3V absEXD = Abs(eXd);

	Vec3V halfSizePlus = Add( halfSize, Vec3V(V_FLT_SMALL_6));

	// potential result T_F_T_F ... i.e. not guarantee T_T_T_T or F_F_F_F
	VecBoolV condition1 = IsGreaterThan( absDisplacement, Add(halfSizePlus,absExtent) );	

	// potential result F_F_F_F or T_T_T_T
	VecBoolV condition2 = VecBoolV(IsGreaterThan( SplatX(absEXD), Add( Scale( SplatY(halfSizePlus), SplatZ(absExtent)), Scale(SplatZ(halfSizePlus), SplatY(absExtent))) ));

	// potential result T_F_T_F ... i.e. not guarantee T_T_T_T or F_F_F_F
	condition1 |= condition2;

	// potential result F_F_F_F or T_T_T_T
	VecBoolV condition3 = VecBoolV(Or(	IsGreaterThan( SplatY(absEXD), Add( Scale(SplatX(halfSizePlus), SplatZ(absExtent)), Scale(SplatZ(halfSizePlus), SplatX(absExtent)))),
								IsGreaterThan( SplatZ(absEXD), Add( Scale(SplatX(halfSizePlus), SplatY(absExtent)), Scale(SplatY(halfSizePlus), SplatX(absExtent)))) 
								));

	// potential result T_F_T_F ... i.e. not guarantee T_T_T_T or F_F_F_F
	condition1 |= condition3;

	VecBoolV condX( Vec::V4SplatX( condition1.GetIntrin128() ) );
	VecBoolV condY( Vec::V4SplatY( condition1.GetIntrin128() ) );
	VecBoolV condZ( Vec::V4SplatZ( condition1.GetIntrin128() ) );

	VecBoolV condXY = condX | condY;

	return InvertBits( condXY | condZ );
}


#if !__SPU	// crashes on SPU
bool geomBoxes::TestSegmentToCenteredAlignedBox (const Vector3 & point1, const Vector3 & point1to2,
												  const Vector3 & halfSize, float *paT, float *pbT)
{
	float t, aT, bT;
	bool baT, bbT;
	int aFace;
	float inverseSlope;

	aFace = -1;
	aT = -1;
	bT = 2;
	baT = bbT = false;

	// clip in X
	if (point1to2.x == 0)
	{
		if (fabsf(point1.x) > halfSize.x)
			return false;
	}
	else
	{
		inverseSlope = 1.0f / point1to2.x;
		if (point1to2.x > 0)
		{
			// clip to right side
			t = (halfSize.x - point1.x) * inverseSlope;
			bT = t;
			bbT = true;

			// clip to left side
			t = (-halfSize.x - point1.x) * inverseSlope;
			aFace = 1;
			aT = t;
			baT = true;
		}
		else
		{
			// clip to right side
			t = (halfSize.x - point1.x) * inverseSlope;
			aFace = 2;
			aT = t;
			baT = true;

			// clip to left side
			t = (-halfSize.x - point1.x) * inverseSlope;
			bT = t;
			bbT = true;
		}
	}

	if (aT > bT)
		return false;

	// clip in Y
	if (point1to2.y == 0)
	{
		if (fabsf(point1.y) > halfSize.y)
			return false;
	}
	else
	{
		inverseSlope = 1.0f / point1to2.y;
		if (point1to2.y > 0)
		{
			// clip to top side
			t = (halfSize.y - point1.y) * inverseSlope;
			if (!bbT || t < bT)
			{
				bT = t;
				bbT = true;
			}

			// clip to bottom side
			t = (-halfSize.y - point1.y) * inverseSlope;
			if (!baT || t > aT)
			{
				aFace = 3;
				aT = t;
				baT = true;
			}
		}
		else
		{
			// clip to top side
			t = (halfSize.y - point1.y) * inverseSlope;
			if (!baT || t > aT)
			{
				aFace = 4;
				aT = t;
				baT = true;
			}

			// clip to bottom side
			t = (-halfSize.y - point1.y) * inverseSlope;
			if (!bbT || t < bT)
			{
				bT = t;
				bbT = true;
			}
		}
	}

	if (aT > bT)
		return false;

	// clip in Z
	if (point1to2.z == 0)
	{
		if (fabsf(point1.z) > halfSize.z)
			return false;
	}
	else
	{
		inverseSlope = 1.0f / point1to2.z;
		if (point1to2.z > 0)
		{
			// clip to far side
			t = (halfSize.z - point1.z) * inverseSlope;
			if (!bbT || t < bT)
			{
				bT = t;
				bbT = true;
			}

			// clip to near side
			t = (-halfSize.z - point1.z) * inverseSlope;
			if (!baT || t > aT)
			{
				aFace = 5;
				aT = t;
				baT = true;
			}
		}
		else
		{
			// clip to far side
			t = (halfSize.z - point1.z) * inverseSlope;
			if (!baT || t > aT)
			{
				aFace = 6;
				aT = t;
				baT = true;
			}

			// clip to near side
			t = (-halfSize.z - point1.z) * inverseSlope;
			if (!bbT || t < bT)
			{
				bT = t;
				bbT = true;
			}
		}
	}

	if (aT > bT)
	{
		return false;
	}

	if (aFace >= 0)
	{
		if (paT != NULL)
			*paT = aT;
		if (pbT != NULL)
			*pbT = bT;
		return true;
	}

	return false;
}


bool geomBoxes::TestSegmentToCenteredAlignedBox (const Vector3 & point1, const Vector3 & point1to2,
													const Vector3 & halfSize, float minT, float maxT,
													float *paT, float *pbT)
{
	float t, aT, bT;
	bool baT, bbT;
	int aFace;
	float inverseSlope;

	aFace = -1;
	aT = -1;
	bT = 2;
	baT = bbT = false;

	// clip in X
	if (point1to2.x == 0)
	{
		if (fabsf(point1.x) > halfSize.x)
			return false;
	}
	else
	{
		inverseSlope = 1.0f / point1to2.x;
		if (point1to2.x > 0)
		{
			// clip to right side
			t = (halfSize.x - point1.x) * inverseSlope;
			
			if (t < minT)
				return false;

			if (t <= maxT)
			{
				bT = t;
				bbT = true;
			}

			// clip to left side
			t = (-halfSize.x - point1.x) * inverseSlope;

			if (t > maxT)
				return false;

			if (t >= minT)
			{
				aFace = 1;
				aT = t;
				baT = true;
			}
		}
		else
		{
			// clip to right side
			t = (halfSize.x - point1.x) * inverseSlope;

			if (t > maxT)
				return false;

			if (t >= minT)
			{
				aFace = 2;
				aT = t;
				baT = true;
			}

			// clip to left side
			t = (-halfSize.x - point1.x) * inverseSlope;

			if (t < minT)
				return false;

			if (t <= maxT)
			{
				bT = t;
				bbT = true;
			}
		}
	}

	if (aT > bT)
		return false;

	// clip in Y
	if (point1to2.y == 0)
	{
		if (fabsf(point1.y) > halfSize.y)
			return false;
	}
	else
	{
		inverseSlope = 1.0f / point1to2.y;
		if (point1to2.y > 0)
		{
			// clip to top side
			t = (halfSize.y - point1.y) * inverseSlope;

			if (t < minT)
				return false;

			if (t <= maxT && (!bbT || t < bT))
			{
				bT = t;
				bbT = true;
			}

			// clip to bottom side
			t = (-halfSize.y - point1.y) * inverseSlope;

			if (t > maxT)
				return false;

			if (t >= minT && (!baT || t > aT))
			{
				aFace = 3;
				aT = t;
				baT = true;
			}
		}
		else
		{
			// clip to top side
			t = (halfSize.y - point1.y) * inverseSlope;

			if (t > maxT)
				return false;

			if (t >= minT && (!baT || t > aT))
			{
				aFace = 4;
				aT = t;
				baT = true;
			}

			// clip to bottom side
			t = (-halfSize.y - point1.y) * inverseSlope;

			if (t < minT)
				return false;

			if (t <= maxT && (!bbT || t < bT))
			{
				bT = t;
				bbT = true;
			}
		}
	}

	if (aT > bT)
		return false;

	// clip in Z
	if (point1to2.z == 0)
	{
		if (fabsf(point1.z) > halfSize.z)
			return false;
	}
	else
	{
		inverseSlope = 1.0f / point1to2.z;
		if (point1to2.z > 0)
		{
			// clip to far side
			t = (halfSize.z - point1.z) * inverseSlope;

			if (t < minT)
				return false;

			if (t <= maxT && (!bbT || t < bT))
			{
				bT = t;
				bbT = true;
			}

			// clip to near side
			t = (-halfSize.z - point1.z) * inverseSlope;

			if (t > maxT)
				return false;

			if (t >= minT && (!baT || t > aT))
			{
				aFace = 5;
				aT = t;
				baT = true;
			}
		}
		else
		{
			// clip to far side
			t = (halfSize.z - point1.z) * inverseSlope;

			if (t > maxT)
				return false;

			if (t >= minT && (!baT || t > aT))
			{
				aFace = 6;
				aT = t;
				baT = true;
			}

			// clip to near side
			t = (-halfSize.z - point1.z) * inverseSlope;

			if (t < minT)
				return false;

			if (t <= maxT && (!bbT || t < bT))
			{
				bT = t;
				bbT = true;
			}
		}
	}

	if (aFace >= 0 && aT <= bT)
	{
		if (paT != NULL)
			*paT = aT;
		if (pbT != NULL)
			*pbT = bT;
		return true;
	}

	return false;
}
#endif


int geomBoxes::TestSegmentToBox (const Vector3& point1, const Vector3& point1to2, const Vector3& boxMin, const Vector3& boxMax,
						float* segmentT1, Vector3* normal1, int* index1, float* segmentT2, Vector3* normal2, int* index2)
{

	// Find the center and half-widths of the box.

#if FAST_GEOMETRY
	Vec3V boxCenter = Scale( Add( VECTOR3_TO_VEC3V(boxMin), VECTOR3_TO_VEC3V(boxMax)), ScalarV(V_HALF) );
	Vec3V boxHalfSize = Subtract( VECTOR3_TO_VEC3V(boxMax), boxCenter);

	// Find the segment starting point relative to the box center.
	Vec3V relPoint1 = Subtract(  VECTOR3_TO_VEC3V(point1),boxCenter);
#else
	Vector3 boxCenter;
	Vector3 boxHalfSize;
	boxCenter.Average(boxMin,boxMax);
	boxHalfSize.Subtract(boxMax,boxCenter);

	// Find the segment starting point relative to the box center.
	Vector3 relPoint1(point1);
	relPoint1.Subtract(boxCenter);
#endif

	Vec3V normal1V, normal2V;
	ScalarV segmentT1V,segmentT2V;
	int hits = TestSegmentToBox(
#if FAST_GEOMETRY
		relPoint1 ,VECTOR3_TO_VEC3V(point1to2) ,boxHalfSize
#else
		RCC_VEC3V(relPoint1) ,RCC_VEC3V(point1to2) ,RCC_VEC3V(boxHalfSize)
#endif
		,&segmentT1V,&normal1V,index1,&segmentT2V,&normal2V,index2);
	if (hits)
	{
		if (segmentT1)
		{
			(*segmentT1) = segmentT1V.Getf();
		}

		if (normal1)
		{
			normal1->Set(RC_VECTOR3(normal1V));
		}

		if (segmentT2)
		{
			(*segmentT2) = segmentT2V.Getf();
		}

		if (normal2)
		{
			normal2->Set(RC_VECTOR3(normal2V));
		}
	}

	return hits;
}


int geomBoxes::TestSegmentToBox (Vec3V_In point1, Vec3V_In point1to2, Vec3V_In boxHalfWidths, ScalarV_Ptr segmentT1, Vec3V_Ptr normal1, int* index1,
									ScalarV_Ptr segmentT2, Vec3V_Ptr normal2, int* index2)
{

	// Some constants.
	ScalarV v_one(V_ONE);
	ScalarV v_two(V_TWO);

	// Get the axis length and its inverse along each direction.
	Vec3V lengthAlongAxis = Abs(point1to2);
	Vec3V invLengthAlongAxis = InvertSafe(lengthAlongAxis);

	// See if the segment points away from or toward the center of each axis.
	VecBoolV pointsOutward = SameSign(point1to2,point1);

	// Get the segment starting point's depth below the box surface along each axis.
	Vec3V depth = Subtract(boxHalfWidths,Abs(point1));

	// See if the segment is completely outside the box along all three axes.
	VecBoolV entirelyOutside = IsLessThan(depth,Vec3V(V_ZERO));
	entirelyOutside = And(entirelyOutside,pointsOutward);
	entirelyOutside = Or(entirelyOutside,IsLessThan(depth,-lengthAlongAxis));
	if (!IsEqualIntAll(Vec3V(entirelyOutside),Vec3V(VecBoolV(V_F_F_F_F))))
	{
		// The segment is completely outside the box along all three axes.
		return 0;
	}

	// Find the fraction of the distance along each axis that the segment enters a box face plane (or -1 if it doesn't enter).
	VecBoolV negativeDepth = IsLessThan(depth,Vec3V(V_ZERO));
	Vec3V testT = Scale(depth,invLengthAlongAxis);
	Vec3V testEnterT = SelectFT(pointsOutward,-testT,-Vec3V(v_one));
	testEnterT = SelectFT(negativeDepth,-Vec3V(v_one),testEnterT);

	// Find the fraction of the distance along the segment that it last enters a box face plane (this is where it enters the box, or -1 if it doesn't).
	ScalarV enterT = Max(SplatX(testEnterT),SplatY(testEnterT),SplatZ(testEnterT));

	// Find the fraction of the distance along the segment that the segment exits a box face plane (or 2 if it doesn't exit).
	Vec3V widthMinusDepth = Subtract(Scale(boxHalfWidths,v_two),depth);
	Vec3V testExitT = SelectFT(negativeDepth,testT,Vec3V(v_two));
	testExitT = SelectFT(pointsOutward,Scale(widthMinusDepth,invLengthAlongAxis),testExitT);

	// Find the fraction of the distance along the segment that it first exits a box face plane (this is where it exits the box, or 2 if it doesn't).
	ScalarV exitT = Min(SplatX(testExitT),SplatY(testExitT),SplatZ(testExitT));

	// See if the segment exited a box face plane before it entered one.
	if (IsGreaterThanAll(enterT,exitT))
	{
		// The segment exited a box face plane and then entered another box face plane, so it must not intersect the box.
		return 0;
	}

	int numIsects = 0;

#if FAST_GEOMETRY
	BoolV test1 = IsGreaterThanOrEqual(enterT,ScalarV(V_ZERO));
	BoolV test2 = IsLessThanOrEqual(enterT,v_one);
	test1 = And( test1, test2 );
	if ( IsEqualIntAll( test1, BoolV(V_TRUE) ) )
#else
	if (IsGreaterThanOrEqualAll(enterT,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(enterT,v_one))
#endif
	{
		// The first intersection is an entry.
		numIsects = 1;
		if (segmentT1)
		{
			(*segmentT1) = enterT;
			if (normal1)
			{
				ScalarV tX = SplatX(testEnterT);
				ScalarV tY = SplatY(testEnterT);
				ScalarV tZ = SplatZ(testEnterT);
				int axisIndex = (IsGreaterThanAll(tX,tY) ? (IsGreaterThanAll(tX,tZ) ? 0 : 2) : (IsGreaterThanAll(tY,tZ) ? 1 : 2));
				int faceIndex = (point1.GetElemf(axisIndex)>0.0f ? 2*axisIndex : 2*axisIndex+1);
				(*normal1) = GetFaceNormal(faceIndex);
				if (index1)
				{
					(*index1) = faceIndex;
				}
			}
		}

		if (IsLessThanOrEqualAll(exitT,v_one))
		{
			// There is a second intersection (an exit).
			numIsects = 2;
			if (segmentT2)
			{
				(*segmentT2) = exitT;
				if (normal2)
				{
					ScalarV tX = SplatX(testExitT);
					ScalarV tY = SplatY(testExitT);
					ScalarV tZ = SplatZ(testExitT);
					int axisIndex = (IsLessThanAll(tX,tY) ? (IsLessThanAll(tX,tZ) ? 0 : 2) : (IsLessThanAll(tY,tZ) ? 1 : 2));
					int faceIndex = (point1to2.GetElemf(axisIndex)>0.0f ? 2*axisIndex : 2*axisIndex+1);
					(*normal2) = GetFaceNormal(faceIndex);
					if (index2)
					{
						(*index2) = faceIndex;
					}
				}
			}
		}
	}
	else 
	{
#if FAST_GEOMETRY
		test1 = IsGreaterThanOrEqual(exitT,ScalarV(V_ZERO));
		test2 = IsLessThanOrEqual(exitT,v_one);
		test1 = And( test1, test2 );
		if( IsEqualIntAll( test1, BoolV(V_TRUE) ) )
#else
		if (IsGreaterThanOrEqualAll(exitT,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(exitT,v_one))
#endif
		{
			// There's only one intersection, and it's an exit.
			numIsects = 1;
			if (segmentT1)
			{
				(*segmentT1) = exitT;
				if (normal1)
				{
					ScalarV tX = SplatX(testExitT);
					ScalarV tY = SplatY(testExitT);
					ScalarV tZ = SplatZ(testExitT);
					int axisIndex = (IsLessThanAll(tX,tY) ? (IsLessThanAll(tX,tZ) ? 0 : 2) : (IsLessThanAll(tY,tZ) ? 1 : 2));
					int faceIndex = (point1to2.GetElemf(axisIndex)>0.0f ? 2*axisIndex : 2*axisIndex+1);
					(*normal1) = GetFaceNormal(faceIndex);
					if (index1)
					{
						(*index1) = faceIndex;
					}
				}
			}
		}
	}

	return numIsects;
}


bool geomBoxes::TestSphereToBox (const Vector3 &sphereCenterWorld, float radius, const Vector3 &boxMin,
									const Vector3 &boxMax, const Matrix34 &boxMatrix, float *dist2)
{

// TODO: Svetli - this is horribly slow function, re-write to vectorized format and remove the branches if used 
	Vector3 center;

	boxMatrix.UnTransform(sphereCenterWorld, center);

	float r2 = square(radius);
	float dmin = 0.0f;

	for(int n=0; n<3; n++)
	{
		if(center[n] < boxMin[n])
		{
			dmin = dmin + square(center[n] - boxMin[n]);
		}
		else if(center[n] > boxMax[n])
		{
			dmin = dmin + square(center[n] - boxMax[n]);
		}
	}

	if (dist2)
	{
		*dist2 = dmin;
	}

	return dmin <= r2;
}

bool geomBoxes::TestSphereToBox (Vec3V_In sphereCenterWorld, ScalarV_In radius, Vec3V_In boxMin, Vec3V_In boxMax, const Matrix34 &boxMatrix)
{
	const Vec3V localizedSpherePos = UnTransformOrtho(RCC_MAT34V(boxMatrix), sphereCenterWorld);
	// clampedLocalizedSpherePos could instead be called "point not outside box that's closest to the sphere"
	const Vec3V clampedLocalizedSpherePos = Clamp(localizedSpherePos, boxMin, boxMax);
	const bool retVal = (IsLessThanOrEqualAll(DistSquared(localizedSpherePos, clampedLocalizedSpherePos), Scale(radius, radius)) != 0);
	return retVal;
}


int geomBoxes::TestBoxToPlane (const Vector3 &p0,Vector3 vtx[8], const Vector3 &normal)
{ 
	Vector3 temp;
	temp.Subtract(vtx[0],p0);
	float dot=temp.Dot(normal);

	for(int i=1;i<8;i++)
	{
		temp.Subtract(vtx[i],p0);
		if(temp.Dot(normal)*dot<=0.0f) return 0;
	}
	if(dot>0.0f) return 1;
	return -1;
}		//lint !e818 vtx could be declared as pointing to const


void geomBoxes::ComputeBoundBox (int numverts,const Vector3 *verts,Vector3 *min,Vector3 *max,Vector3 *center)
{
	Vector3 LocalMin,LocalMax; // rename Min to LocalMin and Max to LocalMax to avoid same name declaration as global Min, Max

	mthAssertf (numverts > 0, "Invalid vert count %d", numverts);				//lint !e506 constant value boolean
	mthAssertf (verts != NULL, "No verts array");				//lint !e506 constant value boolean

	LocalMin = LocalMax = verts[0];		//lint !e613 Asking verts error check for NULL pointer and we did! 
	for(int i=1;i<numverts;i++)
	{
		LocalMin.Min(LocalMin, verts[i]);
		LocalMax.Max(LocalMax, verts[i]);
	}
	if(min)
	{
		*min=LocalMin;
	}
	if(max)
	{
		*max=LocalMax;
	}
	if(center)
	{
		center->Average(LocalMin, LocalMax);
	}
}


void geomBoxes::ComputeBoundInfo (int numverts,const float verts[], u32 stride, Vector3 *boxmin, Vector3 *boxmax,
									Vector3 *center,float *radius)
{
    int i;

    if(numverts<=0) {
        if(boxmin) boxmin->Set(0.f,0.f,0.f);
        if(boxmax) boxmax->Set(0.f,0.f,0.f);
        if(center) center->Set(0.f,0.f,0.f);
        if(radius) *radius=0;
        return;
    }

    // Initialize max and min values //
    Vector3 xmax(0.f,0.f,0.f),xmin(0.f,0.f,0.f),ymax(0.f,0.f,0.f),ymin(0.f,0.f,0.f),zmax(0.f,0.f,0.f),zmin(0.f,0.f,0.f),cen;
    xmin.x = ymin.y = zmin.z =  100000000.0f;
    xmax.x = ymax.y = zmax.z = -100000000.0f;
    for(i = 0; i < numverts; i++) {
		Vector3& vert=(Vector3&)*(((unsigned char*)verts)+stride*(unsigned int)i);
        if(vert.x < xmin.x || (vert.x == xmin.x &&	//lint !e777
            (vert.y <= xmin.y && vert.z <= xmin.z))) xmin = vert;
        if(vert.x > xmax.x || (vert.x == xmax.x &&	//lint !e777
            (vert.y >= xmax.y && vert.z >= xmax.z))) xmax = vert;

        if(vert.y < ymin.y || (vert.y == ymin.y &&	//lint !e777
            (vert.x <= ymin.x && vert.z <= ymin.z))) ymin = vert;
        if(vert.y > ymax.y || (vert.y == ymax.y &&	//lint !e777
            (vert.x >= ymax.x && vert.z >= ymax.z))) ymax = vert;

        if(vert.z < zmin.z || (vert.z == zmin.z &&	//lint !e777
            (vert.x <= zmin.x && vert.y <= zmin.y))) zmin = vert;
        if(vert.z > zmax.z || (vert.z == zmax.z &&	//lint !e777
            (vert.x >= zmax.x && vert.y >= zmax.y))) zmax = vert;
    }

    ////// Compute initial center and radius //////
    float rad,rad_sq;

    float xspan,yspan,zspan;
    xspan=xmin.Dist2(xmax);
    yspan=ymin.Dist2(ymax);
    zspan=zmin.Dist2(zmax);

    if(xspan > yspan && xspan > zspan) {
		cen=xmin;
		cen.Add(xmax);
		cen.Scale(0.5f);
        rad_sq=cen.Dist2(xmax);
    }
    else if(yspan > xspan && yspan > zspan) {
		cen=ymin;
		cen.Add(ymax);
		cen.Scale(0.5f);
        rad_sq=cen.Dist2(ymax);
    }
    else {
		cen=zmin;
		cen.Add(zmax);
		cen.Scale(0.5f);
        rad_sq=cen.Dist2(zmax);
    }
    rad = sqrtf(rad_sq);

    ////// Iterate to solve final center and radius //////
    float old_to_p,old_to_p_sq;
    for (i = 0; i < numverts; i++) {
		Vector3& vert=(Vector3&)*(((unsigned char*)verts)+stride*(unsigned int)i);
         old_to_p_sq = cen.Dist2(vert);
        if (old_to_p_sq > rad_sq) {
            // This point is outside of current sphere //
            old_to_p = sqrtf(old_to_p_sq);

            // Calculate radius of new sphere //
            rad = (rad + old_to_p) / 2.0f;
            rad_sq = rad * rad;
			// Calculate center of new sphere //
            cen.Lerp(rad/old_to_p,vert,cen);
        }
    }

    ////// Set return values ///////
    if(boxmin) boxmin->Set(xmin.x,ymin.y,zmin.z);
    if(boxmax) boxmax->Set(xmax.x,ymax.y,zmax.z);
    if(center) center->Set(cen.x,cen.y,cen.z);
    if(radius) *radius=rad;
}

#if __SPU
Vec3V_Out geomBoxes::GetFaceNormal (int faceIndex)
{
    switch (faceIndex)
    {
    case 0:
		return Vec3V(V_X_AXIS_WZERO);
    case 1:
        return -Vec3V(V_X_AXIS_WZERO);
    case 2:
        return Vec3V(V_Y_AXIS_WZERO);
    case 3:
        return -Vec3V(V_Y_AXIS_WZERO);
    case 4:
        return Vec3V(V_Z_AXIS_WZERO);
    case 5:
        return -Vec3V(V_Z_AXIS_WZERO);
    default:
        mthAssertf(faceIndex >= 0 && faceIndex < 6, "Invalid face index %d", faceIndex);
        return Vec3V(V_ZERO);
    }
}
#else
static const Vec3V FaceNormals[6] =
{
    Vec3V(V_X_AXIS_WZERO),
    -Vec3V(V_X_AXIS_WZERO),
    Vec3V(V_Y_AXIS_WZERO),
    -Vec3V(V_Y_AXIS_WZERO),
    Vec3V(V_Z_AXIS_WZERO),
    -Vec3V(V_Z_AXIS_WZERO)
};

Vec3V_Out geomBoxes::GetFaceNormal (int faceIndex)
{
    mthAssertf(faceIndex >= 0 && faceIndex < 6, "Invalid face index %d", faceIndex);
    return FaceNormals[faceIndex];
}
#endif // __SPU

/////////////////////////////////////////////////////////////////
// intersection test routines

bool geom2D::Test2DPointVsTri (float pointA, float pointB, float tri1A, float tri1B, float tri2A, float tri2B, float tri3A, float tri3B)
{
	// Find v1 relative to v0.
	float edge1A = tri2A - tri1A;
	float edge1B = tri2B - tri1B;

	// Find v2 relative to v0.
	float edge2A = tri3A - tri1A;
	float edge2B = tri3B - tri1B;

	float crossY = edge1B * edge2A - edge1A * edge2B;

	if (crossY == 0.0f)
	{
		return false;
	}

	float alpha = 1.0f / crossY;

	// Find p relative to v0.
	float pRelA = pointA-tri1A;
	float pRelB = pointB-tri1B;

	// Find the barycentric coordinates of the point along the two edges.
	float u = (pRelB * edge2A - pRelA * edge2B) * alpha;
	float v = (pRelA * edge1B - pRelB * edge1A) * alpha;

	return (u >= 0.0f && v >= 0.0f && u + v <= 1.0f);
}


bool geom2D::Test2DPointVsTri (const Vector3& point, const Vector3& v1, const Vector3& v2, const Vector3& v3)
{
	return Test2DPointVsTri(point.x,point.z,v1.x,v1.z,v2.x,v2.z,v3.x,v3.z);
}


bool geom2D::Test2DSegmentVsCenteredAlignedRect (float point1A, float point1B, float point2A, float point2B, float halfSizeA, float halfSizeB)
{
	int aFace = -1;

	// low clip
	float aT = -1.0f;

	// high clip
	float bT = 2.0f;

	// clip in X
	float dA = point2A - point1A;
	if (dA == 0.0f)
	{
		if (fabsf(point1A) > halfSizeA)
		{
			return false;
		}
	}
	else
	{
		float inverseD = 1.0f / dA;
		if (point1A < point2A)
		{
			// clip to right side
			float t = (halfSizeA - point1A) * inverseD;
			
			if (t < 0.0f)
			{
				return false;
			}

			if (t <= 1.0f)
			{
				bT = t;
			}

			// clip to left side
			t = (-halfSizeA - point1A) * inverseD;

			if (t > 1.0f)
			{
				return false;
			}

			if (t >= 0.0f)
			{
				aFace = 1;
				aT = t;
			}
		}
		else
		{
			// clip to right side
			float t = (halfSizeA - point1A) * inverseD;

			if (t > 1.0f)
			{
				return false;
			}

			if (t >= 0.0f)
			{
				aFace = 2;
				aT = t;
			}

			// clip to left side
			t = (-halfSizeA - point1A) * inverseD;

			if (t < 0.0f)
			{
				return false;
			}

			if (t <= 1.0f)
			{
				bT = t;
			}
		}
	}

	if (aT > bT)
	{
		return false;
	}

	// clip in Z
	float dB = point2B - point1B;
	if (dB == 0.0f)
	{
		if (fabsf(point1B) > halfSizeB)
		{
			return false;
		}
	}
	else
	{
		float inverseD = 1.0f / dB;
		if (point1B < point2B)
		{
			// clip to far side
			float t = (halfSizeB - point1B) * inverseD;

			if (t < 0.0f)
			{
				return false;
			}

			if (t <= 1.0f && t < bT)
			{
				bT = t;
			}

			// clip to near side
			t = (-halfSizeB - point1B) * inverseD;

			if (t > 1.0f)
			{
				return false;
			}

			if (t >= 0.0f && t > aT)
			{
				aFace = 5;
				aT = t;
			}
		}
		else
		{
			// clip to far side
			float t = (halfSizeB - point1B) * inverseD;

			if (t > 1.0f)
			{
				return false;
			}

			if (t >= 0.0f && t > aT)
			{
				aFace = 6;
				aT = t;
			}

			// clip to near side
			t = (-halfSizeB - point1B) * inverseD;

			if (t < 0.0f)
			{
				return false;
			}

			if (t <= 1.0f && t < bT)
			{
				bT = t;
			}
		}
	}

	return ((aT > bT) ? false : (aFace >= 0 ? true : false));
}


bool geom2D::Test2DSegmentVsCenteredAlignedRect (const Vector3& point1, const Vector3& point2, const Vector3& halfSize)
{
	return Test2DSegmentVsCenteredAlignedRect(point1.x,point1.z,point2.x,point2.z,halfSize.x,halfSize.z);
}


bool geom2D::Test2DTriVsAlignedRect (float tri1A, float tri1B, float tri2A, float tri2B, float tri3A, float tri3B, float rectMinA, float rectMinB, float rectMaxA, float rectMaxB)
{
	// 1) either a triangle vertex is inside the box
	if ((tri1A >= rectMinA && tri1A <= rectMaxA && tri1B >= rectMinB && tri1B <= rectMaxB) ||
	    (tri2A >= rectMinA && tri2A <= rectMaxA && tri2B >= rectMinB && tri2B <= rectMaxB) ||
	    (tri3A >= rectMinA && tri3A <= rectMaxA && tri3B >= rectMinB && tri3B <= rectMaxB))
	{
		return true;
	}

	// 2) or an edge of the triangle intersects the box
	float boxCenterA = Lerp(0.5f,rectMinA,rectMaxA);
	float boxCenterB = Lerp(0.5f,rectMinB,rectMaxB);
	float halfSizeA = rectMaxA-boxCenterA;
	float halfSizeB = rectMaxB-boxCenterB;
	float tri1PrimeA = tri1A-boxCenterA;
	float tri1PrimeB = tri1B-boxCenterB;
	float tri2PrimeA = tri2A-boxCenterA;
	float tri2PrimeB = tri2B-boxCenterB;
	float tri3PrimeA = tri3A-boxCenterA;
	float tri3PrimeB = tri3B-boxCenterB;
	if (Test2DSegmentVsCenteredAlignedRect(tri1PrimeA,tri1PrimeB,tri2PrimeA,tri2PrimeB,halfSizeA,halfSizeB) ||
	    Test2DSegmentVsCenteredAlignedRect(tri2PrimeA,tri2PrimeB,tri3PrimeA,tri3PrimeB,halfSizeA,halfSizeB) ||
	    Test2DSegmentVsCenteredAlignedRect(tri3PrimeA,tri3PrimeB,tri1PrimeA,tri1PrimeB,halfSizeA,halfSizeB))
	{
		return true;
	}

	// 3) or the center of the box is in the triangle (or not)
	return Test2DPointVsTri(boxCenterA,boxCenterB,tri1A,tri1B,tri2A,tri2B,tri3A,tri3B);
}


bool geom2D::Test2DTriVsAlignedRect (const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& rectMin, const Vector3& rectMax, int firstAxis, int secondAxis)
{
	return Test2DTriVsAlignedRect(v0[firstAxis],v0[secondAxis],v1[firstAxis],v1[secondAxis],v2[firstAxis],v2[secondAxis],
									rectMin[firstAxis],rectMin[secondAxis],rectMax[firstAxis],rectMax[secondAxis]);
}


bool geom2D::Test2DPolyVsAlignedRect (int numVerts, const Vector3** verts, const Vector3& rectMin, const Vector3& rectMax, int firstAxis, int secondAxis)
{
	if (!Test2DTriVsAlignedRect(*verts[0],*verts[1],*verts[2],rectMin,rectMax,firstAxis,secondAxis))
	{
		if (numVerts==4)
		{
			return Test2DTriVsAlignedRect(*verts[0],*verts[2],*verts[3],rectMin,rectMax,firstAxis,secondAxis);
		}
		else
		{
			return false;
		}
	}

	return true;
} //lint !e818 verts could be declared as pointing to const


bool geom2D::Test2DLineVsLine (float& hitT1, float& hitT2, const Vector2& start1, const Vector2& end1,
								const Vector2& start2, const Vector2& end2, bool directedTest)
{
	const float L1 = end1.x - start1.x;
	const float M1 = end1.y - start1.y;

	const float L2 = end2.x - start2.x;
	const float M2 = end2.y - start2.y;
	const float threshold = 0.00001f;

	const float denominator = L1 * M2 - L2 * M1;
	if((directedTest && denominator < threshold) || fabs(denominator) < threshold) // Threshold for parallel lines
	{
		hitT1 = FLT_MAX;
		hitT2 = FLT_MAX;
		return false;
	}

	if(fabs(L2) < threshold)// Parallel to Y Axis
	{
		if(fabsf(L1) < threshold)
		{
			// Line 1 is also close to parallel to the y-axis.  
			// It might still be possible to get a good answer,
			// so further investigation may be warranted.
			return false;
		}
		hitT1 = (start2.x - start1.x) / L1;
		hitT2 = (M1 * hitT1 + start1.y - start2.y) / M2;
		return true;
	}

	if(fabs(M2) < threshold)// Parallel to X Axis
	{
		if(fabsf(M1) < threshold)
		{
			// Line 1 is also close to parallel to the x-axis.  
			// See note in above block.
			return false;
		}
		hitT1 = (start2.y - start1.y) / M1;
		hitT2 = (L1 * hitT1 + start1.x - start2.x) / L2;
		return true;
	}

	hitT1 = (M2 * (start2.x - start1.x) + L2 * (start1.y - start2.y)) / denominator;
	hitT2 = (L1 * hitT1 + start1.x - start2.x) / L2;
	return true;
}


bool geom2D::Test2DSegVsSeg (float& hitT1Out, float& hitT2Out, const Vector2& start1, const Vector2& end1,
								const Vector2& start2, const Vector2& end2, bool directedTest)
{
	bool linesIntersect = Test2DLineVsLine(hitT1Out,hitT2Out,start1,end1,start2,end2,directedTest);
	if(linesIntersect)
	{
		return (hitT1Out>=0.0f && hitT1Out<=1.0f && hitT2Out>=0.0f && hitT2Out<=1.0f);
	}
	return false;
}

// From here; http://softsurfer.com/Archive/algorithm_0104/algorithm_0104B.htm
bool geom2D::Test2DSegVsSegFast (float& hitT1Out, float& hitT2Out, float start1x, float start1y, float end1x, float end1y,
							 float start2x, float start2y, float end2x, float end2y)
{
	Vec2f start1(start1x, start1y);
	Vec2f end1(end1x, end1y);
	Vec2f start2(start2x, start2y);
	Vec2f end2(end2x, end2y);

	Vec2f u = end1 - start1;
	Vec2f v = end2 - start2;
	Vec2f vPerp(-v.GetY(), v.GetX());
	Vec2f uPerp(-u.GetY(), u.GetX());
	Vec2f w = (start1 - start2);

	float vPerpDotU = -Dot(vPerp, u);

	if (fabsf(vPerpDotU) < FLT_EPSILON)
	{
		// lines are parallel, no intersection
		return false;
	}

	// We used to divide by vPerpDotU here, now we're doing it later once we know that the 
	// line segments will intersect. Because of that we need to check that s and t are in
	// the range (0, vPerpDotU) instead of (0, 1), and to handle the case that vPerpDotU is 
	// negative we also negate the signs of s and t so we're always testing from 0 to a positive range
	float s = Dot(vPerp, w);
	float t = Dot(uPerp, w);

	// If vPerpDotU < 0, flip the signs of it, s and t
	s = Selectf(vPerpDotU, s, -s);
	t = Selectf(vPerpDotU, t, -t);
	float vPerpDotUAbs = fabs(vPerpDotU);

	// fcmps are cheaper than fdivs. So do the range tests first, and only do the div when necessary
	if (s >= 0.0f && s <= vPerpDotUAbs && t >= 0.0f && t <= vPerpDotUAbs)
	{
		hitT1Out = s / vPerpDotUAbs;
		hitT2Out = t / vPerpDotUAbs;
		return true;
	}
	
	return false;
}


// From here: http://softsurfer.com/Archive/algorithm_0111/algorithm_0111.htm
bool geom2D::Test2DSegVsPolygon(float& tEnter, float& tLeave, Vec2f_In segStart, Vec2f_In segEnd, const Vec2f* __restrict verts, int numVerts)
{
	Vec2f segDir = segEnd - segStart;

	tEnter = 0.0f;
	tLeave = 1.0f;

	Vec2f edgeStart = verts[numVerts-1];
	for(int i = 0; i < numVerts; i++)
	{
		Vec2f edgeEnd = verts[i];
		Vec2f dir = edgeEnd - edgeStart;
		// Generate an outward-facing normal
		Vec2f edgeNormal(-dir.GetY(), dir.GetX());

		float segDotNormal = Dot(segDir, edgeNormal);
		float segToEdgeDotNormal = Dot(edgeStart - segStart, edgeNormal);

		if (fabsf(segDotNormal) < SMALL_FLOAT) // If segment and edge are (basically) parallel
		{
			if (segToEdgeDotNormal < 0.0f) {
				// Segment is outside of the poly
				return false;
			}
			else
			{
				continue; // would give a bad t value below, so ignore the edge
			}
		}

		float t = segToEdgeDotNormal / segDotNormal; // Solve line-line intersection: isect = segStart + t * segDir
		// (see URL above for what's going on here, but basically treat segStart->segEnd as a ray, 
		// and depending on the edgeNormal the ray is either entering or leaving the polygon
		if (segDotNormal < 0.0f)
		{
			tEnter = Max(tEnter, t);
			if (tEnter > tLeave) { // segment 'enters' the polygon after it 'leaves' the polygon
				return false;
			}
		}
		else
		{
			tLeave = Min(tLeave, t);
			if (tLeave < tEnter) { // segment 'leaves' the polygon before it 'enters' it.
				return false;
			}
		}

		edgeStart = edgeEnd;
	}
				
	return true;
}

VecBoolV_Out geom2D::Test4x2DSegVsPolygon(Vec4V_InOut tEnter, Vec4V_InOut tLeave, Vec4V_In segStartX, Vec4V_In segStartY, Vec4V_In segEndX, Vec4V_In segEndY, const Vec2V* __restrict verts, int numVerts)
{
	Vec4V segDirX = segEndX - segStartX;
	Vec4V segDirY = segEndY - segStartY;

	tEnter = Vec4V(V_ZERO);
	tLeave = Vec4V(V_ONE);
	
	VecBoolV result(V_T_T_T_T);
	VecBoolV keepGoing(V_T_T_T_T);

	Vec2V edgeStart = verts[numVerts-1];
	for(int i = 0; i < numVerts; i++)
	{
		Vec2V edgeEnd = verts[i];
		Vec2V dir = edgeEnd - edgeStart;

		// Generate an outward-facing normal
		Vec2V edgeNormal(-dir.GetY(), dir.GetX());

		Vec4V segDotNormal = segDirX * edgeNormal.GetX() + segDirY * edgeNormal.GetY();

		Vec4V segToEdgeX = Vec4V(edgeStart.GetX()) - segStartX;
		Vec4V segToEdgeY = Vec4V(edgeStart.GetY()) - segStartY;

		Vec4V segToEdgeDotNormal = segToEdgeX * edgeNormal.GetX() + segToEdgeY * edgeNormal.GetY();

		VecBoolV segAndEdgeAreParallel = IsLessThan(Abs(segDotNormal), Vec4V(V_FLT_SMALL_6));

		VecBoolV segOutsidePoly = IsLessThan(segToEdgeDotNormal, Vec4V(V_ZERO));

		result = result & !(segAndEdgeAreParallel & segOutsidePoly); // seg is parallel to an edge, not in the poly so whole seg is outside
		keepGoing = keepGoing & !(segAndEdgeAreParallel & segOutsidePoly); 

		VecBoolV considerThisEdge = !segAndEdgeAreParallel; // If seg and edge are parallel, don't consider this edge anymore

		Vec4V tValues = segToEdgeDotNormal / segDotNormal; // Solve line-line intersection: isect = segStart + t * segDir
		// (see URL above for what's going on here, but basically treat segStart->segEnd as a ray, 
		// and depending on the edgeNormal the ray is either entering or leaving the polygon

		VecBoolV entering = IsLessThan(segDotNormal, Vec4V(V_ZERO));
		Vec4V newTEnter = SelectFT(entering, tEnter, Max(tEnter, tValues));
		Vec4V newTLeave = SelectFT(entering, Min(tLeave, tValues), tLeave);

		VecBoolV enterAfterLeave = IsGreaterThan(newTEnter, newTLeave);

		result = result & !enterAfterLeave; // Set the result to false for all where "enter After leave" is true
		keepGoing = keepGoing & !enterAfterLeave; // Stop updating these t values

		tEnter = SelectFT(keepGoing & considerThisEdge, tEnter, newTEnter);
		tLeave = SelectFT(keepGoing & considerThisEdge, tLeave, newTLeave);

		edgeStart = edgeEnd;
	}

	return result;
}

bool geom2D::RectangleTouchesRectangle (const Vector3& centerA, const Vector3& unitHeadingA, float lengthA,
										float widthA, const Vector3& centerB, const Vector3& unitHeadingB,
										float lengthB, float widthB)
{
	// Get the relative position between the rectangle centers (A to B).
	Vector3 relPos(centerB);
	relPos.Subtract(centerA);

	// Get the center of rectangle B in rectangle A's coordinate system.
	// This is relPos.Dot(unitHeadingA) in the x direction and relPos.Dot(unitHeadingA.Cross(y)) in the z direction.
	Vector3 localCenter(relPos.x*unitHeadingA.x+relPos.z*unitHeadingA.z,0.0f,
						-relPos.x*unitHeadingA.z+relPos.z*unitHeadingA.x);

	// Get the unit heading of rectangle B in rectangle A's coordinate system.
	// This is unitHeadingB.Dot(unitHeadingA) in the x direction and unitHeadingB.Dot(unitHeadingA.Cross(y)) in z.
	Vector3 localHeading(unitHeadingB.x*unitHeadingA.x+unitHeadingB.z*unitHeadingA.z,0.0f,
							-unitHeadingB.x*unitHeadingA.z+unitHeadingB.z*unitHeadingA.x);

	// Make the half size vectors for both rectangles.
	Vector3 halfSizeA(0.5f*lengthA,0.0f,0.5f*widthA);
	Vector3 halfSizeB(0.5f*lengthB,0.0f,0.5f*widthB);

	// Get the corners of rectangle B in rectangle A's coordinate system.
	Vector3 localCorner[4];
	float shiftXX = localHeading.x*halfSizeB.x;
	float shiftXZ = localHeading.x*halfSizeB.z;
	float shiftZX = localHeading.z*halfSizeB.x;
	float shiftZZ = localHeading.z*halfSizeB.z;
	localCorner[0].Set(localCenter.x+shiftXX-shiftZZ,0.0f,localCenter.z+shiftZX+shiftXZ);
	localCorner[1].Set(localCenter.x+shiftXX+shiftZZ,0.0f,localCenter.z+shiftZX-shiftXZ);
	localCorner[2].Set(localCenter.x-shiftXX+shiftZZ,0.0f,localCenter.z-shiftZX-shiftXZ);
	localCorner[3].Set(localCenter.x-shiftXX-shiftZZ,0.0f,localCenter.z-shiftZX+shiftXZ);
	int index;
	for (index=0;index<4;index++)
	{
		if (geomPoints::IsPointInBox2D(localCorner[index],halfSizeA))
		{
			// One of the corners of rectangle B is inside rectangle A, so they intersect.
			return true;
		}
	}

	// Get the relative position between the rectangle centers (B to A).
	relPos.Negate();

	// Get the center of rectangle B in rectangle A's coordinate system.
	// This is relPos.Dot(unitHeadingB) in the x direction and relPos.Dot(unitHeadingB.Cross(y)) in the z direction.
	localCenter.Set(relPos.x*unitHeadingB.x+relPos.z*unitHeadingB.z,0.0f,
						-relPos.x*unitHeadingB.z+relPos.z*unitHeadingB.x);

	// Get the unit heading of rectangle A in rectangle B's coordinate system.
	// This is unitHeadingA.Dot(unitHeadingB) in the x direction and unitHeadingA.Dot(unitHeadingB.Cross(y)) in z.
	localHeading.Set(unitHeadingA.x*unitHeadingB.x+unitHeadingA.z*unitHeadingB.z,0.0f,
							-unitHeadingA.x*unitHeadingB.z+unitHeadingA.z*unitHeadingB.x);

	// Get the corners of rectangle A in rectangle B's coordinate system.
	shiftXX = localHeading.x*halfSizeA.x;
	shiftXZ = localHeading.x*halfSizeA.z;
	shiftZX = localHeading.z*halfSizeA.x;
	shiftZZ = localHeading.z*halfSizeA.z;
	localCorner[0].Set(localCenter.x+shiftXX-shiftZZ,0.0f,localCenter.z+shiftZX+shiftXZ);
	localCorner[1].Set(localCenter.x+shiftXX+shiftZZ,0.0f,localCenter.z+shiftZX-shiftXZ);
	localCorner[2].Set(localCenter.x-shiftXX+shiftZZ,0.0f,localCenter.z-shiftZX-shiftXZ);
	localCorner[3].Set(localCenter.x-shiftXX-shiftZZ,0.0f,localCenter.z-shiftZX+shiftXZ);

	for (index=0;index<4;index++)
	{
		if (geomPoints::IsPointInBox2D(localCorner[index],halfSizeB))
		{
			// One of the corners of rectangle A is inside rectangle B, so they intersect.
			return true;
		}
	}

	// No corner of either rectangle is inside the other, so test the edges of rectangle A for intersections
	// with rectangle B.
	int lastCornerIndex = 3;
	for (index=0;index<4;index++)
	{
		if (Test2DSegmentVsCenteredAlignedRect(localCorner[lastCornerIndex],localCorner[index],halfSizeB))
		{
			return true;
		}
		lastCornerIndex = index;
	}

	// No corner of either rectangle is inside the other, and no edges of rectangle B intersect
	// rectangle A, so the rectangles do not touch.
	return false;
}


bool geom2D::IsPointInRegion(float x, float y, const Vector3* perimeter, int nperimverts)
{
	int nisect = 0;
	for (int p = 0; p < nperimverts; p++)
	{
		const Vector3 &rp0 = perimeter[p];
		const Vector3 &rp1 = perimeter[(p + 1)%nperimverts];
		// Traverse in +ve y, so ignore lines entirely
		// in other quadrants.
		if (((rp0.x < x) && (rp1.x < x)) ||
			((rp0.x > x) && (rp1.x > x)) ||
			((rp0.z < y) && (rp1.z < y)))
			continue;
		// Vertical line - counts as 2 intersects, but
		// we only care about odd/even so leave at 0.
		if (rp0.x == rp1.x)													//lint !e777 testing floats for equalitry
			continue;
		// Both ends above us, counts as one isect.
		if ((rp0.z > y) && (rp1.z > y))
			nisect++;
		// Are we below line equation for this segment.
		else if (y < rp0.z + (rp1.z - rp0.z)*(x - rp0.x)/(rp1.x - rp0.x))
			nisect++;
	}

	// Odd number of intersects, we are inside.
	return ((nisect & 1) != 0);
}


float geom2D::TestEdgeFlat (float x, float z, float ax, float az, float bx, float bz)
{
	float crossX=az-bz;
	float crossZ=bx-ax;
	float toCX=x-ax;
	float toCZ=z-az;
	float dot=crossX*toCX+crossZ*toCZ;
	return dot;
}


// Returns true if the point is inside this quad or if
// it lies on side a-b or on side a-c, but not on point b or c.
bool geom2D::IntersectPointFlat (float x, float z, float ax, float az, float bx, float bz,
									float cx, float cz, float dx, float dz)
{
	if(TestEdgeFlat(x,z,ax,az,bx,bz)<0.0f)
	{
		return false;
	}
	if(TestEdgeFlat(x,z,bx,bz,dx,dz)<=0.0f)
	{
		return false;
	}
	if(TestEdgeFlat(x,z,dx,dz,cx,cz)<=0.0f)
	{
		return false;
	}
	if(TestEdgeFlat(x,z,cx,cz,ax,az)<0.0f)
	{
		return false;
	}
	return true;
}

void geom2D::SweepPolygon(Vec2f* __restrict inPoints, Vec2f* __restrict outPoints, int numStartingPoints, Vec2f sweepVector)
{
	// find the most extreme points (the points that lie furthest from the delta axis)
	// these are the ones that get duplicated and offset
	float minDot = 0.0f;
	int minIdx = 0;
	float maxDot = 0.0f;
	int maxIdx = 0;

	// We need a perpendicular vector such that sweepVector cross perpToSweep points in the same direction as one
	// edge of the poly crossed with the next edge. Basically because we split the polygon into a side that stays where it
	// is and a side that needs to be offset, and need to know which is which. 
	Vec2f perpToDelta(sweepVector.GetY(), -sweepVector.GetX());

	for(int i = 1; i < numStartingPoints; i++)
	{
		float dot = Dot(perpToDelta, inPoints[i] - inPoints[0]);
		if (dot < minDot)
		{
			minDot = dot;
			minIdx = i;
		}
		if (dot >= maxDot)
		{
			maxDot = dot;
			maxIdx = i;
		}
	}
	
	Assertf(minIdx != maxIdx, "Couldn't find 2 points to split the polygon at. Sweep=%f, %f. Found index %d = %f, %f. Start point= %f, %f, Num points %d", 
		sweepVector.GetX(), sweepVector.GetY(),
		minIdx,
		inPoints[minIdx].GetX(), inPoints[minIdx].GetY(), 
		inPoints[0].GetX(), inPoints[0].GetY(),
		numStartingPoints);

	if (minIdx < maxIdx)
	{
		// Original spans: [0...minIdx]       (minIdx...maxIdx)         [maxIdx...numPoints-1]
		// New spans:      [0...minIdx] [minIdx+delta ... maxIdx+delta] [maxIdx...numPoints+1]
		for(int i = 0; i <= minIdx; i++)
		{
			outPoints[i] = inPoints[i];
		}
		for(int i = minIdx; i <= maxIdx; i++)
		{
			outPoints[i+1] = inPoints[i] + sweepVector;
		}
		for(int i = maxIdx; i < numStartingPoints; i++)
		{
			outPoints[i+2] = inPoints[i];
		}
	}
	else
	{
		// Original spans: [0...maxIdx]      (maxIdx...minIdx)        [minIdx...numPoints-1]
		// New spans:      [0+delta...maxIdx+delta]  [maxIdx...minIdx] [minIdx+delta...numPoints+1 + delta]
		for(int i = 0; i <= maxIdx; i++)
		{
			outPoints[i] = inPoints[i] + sweepVector;
		}
		for(int i = maxIdx; i <= minIdx; i++)
		{
			outPoints[i+1] = inPoints[i];
		}
		for(int i = minIdx; i < numStartingPoints; i++)
		{
			outPoints[i+2] = inPoints[i] + sweepVector;
		}
	}
}

#define FAST_GEOMCODE	1	// compiler generates some horrible code , so let's help a bit here


bool geom2D::IntersectTwoCircles (float x0, float y0, float r0, float x1, float y1, float r1,
								  float &xi1, float &yi1, float &xi2, float &yi2)
{
	float a, dx, dy, d, h, rx, ry, x2, y2;

	/* dx and dy are the vertical and horizontal distances between
	* the circle centers.
	*/
	dx = x1 - x0;
	dy = y1 - y0;

	/* Determine the straight-line distance between the centers. */
	d = sqrtf((dy*dy) + (dx*dx));
#if FAST_GEOMCODE
	float invD = 1 / d;
#endif
	/* Check for solvability. */
	if (d > (r0 + r1))
	{
		/* no solution. circles do not intersect. */
		return false;
	}
	if (d < fabsf(r0 - r1))
	{
		/* no solution. one circle is contained in the other */
		return false;
	}

	/* 'point 2' is the point where the line through the circle
	* intersection points crosses the line between the circle
	* centers.  
	*/

	/* Determine the distance from point 0 to point 2. */
#if FAST_GEOMCODE
	a = ((r0*r0) - (r1*r1) + (d*d)) * 0.5f * invD ;	
#else
	a = ((r0*r0) - (r1*r1) + (d*d)) / (2.0f * d) ;
#endif

	/* Determine the coordinates of point 2. */
#if FAST_GEOMCODE
	x2 = x0 + (dx * a * invD);
	y2 = y0 + (dy * a * invD);	
#else
	x2 = x0 + (dx * a/d);
	y2 = y0 + (dy * a/d);	
#endif

	/* Determine the distance from point 2 to either of the
	* intersection points.
	*/

	// I added the Max() clamping here. I was seeing some invalid
	// numbers and when I enabled floating point exceptions, I
	// encountered some here with r0*r0 - a*a being slightly below
	// 0. This was running 360 Beta. Unfortunately, when I tried
	// either adding an mthAssert() or disabling optimizations,
	// I couldn't get the problem to happen, but I checked the
	// floating point registers to make sure it was only a slightly
	// negative number. Anyway, another possibility could be to
	// reject these solutions earlier, by adding some tolerances,
	// but that seemed riskier. /FF

	h = sqrtf(Max((r0*r0) - (a*a), 0.0f));

	/* Now determine the offsets of the intersection points from
	* point 2.
	*/
#if FAST_GEOMCODE
	rx = -dy * (h * invD);
	ry = dx * (h * invD);
#else
	rx = -dy * (h/d);
	ry = dx * (h/d);
#endif
	/* Determine the absolute intersection points. */
	xi1 = x2 + rx;
	yi1 = y2 + ry;
	xi2 = x2 - rx;
	yi2 = y2 - ry;

	return true;
}

