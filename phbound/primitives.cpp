// 
// phbound/primitives.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "primitives.h"

#include "phbound/boundpolyhedron.h"
#include "phcore/material.h"
#include "phcore/phmath.h" 
#include "phcore/segment.h"

#include "data/resource.h"
#include "data/struct.h"
#include "math/simplemath.h"
#include "string/string.h"


using namespace rage;

//=============================================================================
// phPolygon

bool phPolygon::sm_InResourceConstructor = false;

#if __DEV || __TOOL
bool phPolygon::sm_MessagesEnabled = true;
#endif


IMPLEMENT_PLACE(phPolygon);

#if __DECLARESTRUCT
void phPolygon::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(phPolygon);

	STRUCT_FIELD(m_Area);
	STRUCT_CONTAINED_ARRAY(m_VertexIndices);
	STRUCT_CONTAINED_ARRAY(m_NeighboringPolygons);

	STRUCT_END();
}
#endif


void phPolygon::InitTriangle (phPolygon::Index i0, phPolygon::Index i1, phPolygon::Index i2, Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2 ASSERT_ONLY(, bool assertOnZeroArea))
{
	// Possibly this should go back to setting the whole m_VertexIndices values?
	//SetVertexIndex(0, i0);
	//SetVertexIndex(1, i1);
	//SetVertexIndex(2, i2);
	// Yes - Since this is initializing, we want to set all bits, this should leave the vertex normal codes set to zero
	//  - (i.e. Initialized and awaiting further setup)
	m_VertexIndices[0] = i0;
	m_VertexIndices[1] = i1;
	m_VertexIndices[2] = i2;

	//there are cases where we end up with zero area triangles
	//In some cases it's harmless as we end up calling WeldVertices() after making this function
	//which should group up all the vertices that are closeby and so
	// all the zero area triangles go away. So we pass false (for disabling
	//the assert). By default it is true
	CalculateNormalAndArea(vert0, vert1, vert2 ASSERT_ONLY(, assertOnZeroArea));
}


////////////////////////////////////////////////////////////
// resource creation

// Change the Set method below if the class members change.
#if VECTORIZED_PADDING
// TODO: Clean up this class definition if we use 16-byte Vector3s
//CompileTimeAssert(sizeof(phPolygon)==32 + 8 * sizeof(phPolygon::Index));
#else
// CompileTimeAssert(sizeof(phPolygon)==16 + 8 * sizeof(phPolygon::Index));
#endif


void phPolygon::Set (const phPolygon & poly)
{
	memcpy(this, &poly, sizeof(phPolygon));
}

void phPolygon::Rotate()
{
	phPolygon::Index temp;
	temp = m_VertexIndices[0];
	m_VertexIndices[0] = m_VertexIndices[1];
	m_VertexIndices[1] = m_VertexIndices[2];
	m_VertexIndices[2] = temp;

	temp = m_NeighboringPolygons[0];
	m_NeighboringPolygons[0] = m_NeighboringPolygons[1];
	m_NeighboringPolygons[1] = m_NeighboringPolygons[2];
	m_NeighboringPolygons[2] = temp;
}

void phPolygon::RemoveVertex(int index)
{
	int indexLimit = 2;

	for (int i = index; i < indexLimit; i++)
	{
		m_VertexIndices[i] = m_VertexIndices[i+1];
		m_NeighboringPolygons[i] = m_NeighboringPolygons[i+1];
	}

	m_VertexIndices[indexLimit] = 0;
	m_NeighboringPolygons[indexLimit] = 0;
}


void phPolygon::CalculateNormalAndArea(const Vec3V* vertices)
{
	CalculateNormalAndArea(vertices[0], vertices[1], vertices[2]);
}


void phPolygon::CalculateNormalAndArea(Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2 ASSERT_ONLY(, bool assertOnZeroArea))
{
	Vec3V normal(ComputeNonUnitNormal(vert0, vert1, vert2));
	ScalarV twiceArea = Mag(normal);

	// Set the area and restore the material index.
	SetArea((ScalarV(V_HALF)*twiceArea).Getf());

	//there are cases where we end up with zero area triangles
	//In some cases it's harmless as we end up calling WeldVertices() after making this function
	//which should group up all the vertices that are closeby and so
	// all the zero area triangles go away. So we pass false (for disabling
	//the assert). By default it is true
#if __ASSERT
	if(assertOnZeroArea)
	{
		const Vec3V newUnitNormal = ComputeUnitNormal(vert0,vert1,vert2);
		if(!IsLessThanAll(Abs(Subtract(Mag(newUnitNormal),ScalarV(V_ONE))),ScalarVFromF32(0.03f)))
			Warningf("Computing non-normalized normal for polygon."
				"\n\tNormal: %f, %f, %f, |%f|"
				"\n\tOriginal Area: %f"
				"\n\tFinal Area: %f"
				"\n\tvertex 0: %f, %f, %f"
				"\n\tvertex 1: %f, %f, %f"
				"\n\tvertex 2: %f, %f, %f",
				VEC3V_ARGS(newUnitNormal),Mag(newUnitNormal).Getf(),
				twiceArea.Getf()/2.0f,
				GetArea(),
				VEC3V_ARGS(vert0),VEC3V_ARGS(vert1),VEC3V_ARGS(vert2));

	}
#endif
}


void phPolygon::ComputeArea (const Vec3V* vertices)
{
	CalculateNormalAndArea(vertices[0], vertices[1], vertices[2]);
}


void phPolygon::ComputeArea (Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2)
{
	CalculateNormalAndArea(vert0, vert1, vert2);
}


bool phPolygon::VerifyUniqueVertices () const
{
	for(int vert1=POLY_MAX_VERTICES-1;vert1>0;vert1--)
	{
		for(int vert2=vert1-1;vert2>=0;vert2--)
		{
			if(m_VertexIndices[vert1]==m_VertexIndices[vert2])
			{
				// The polygon has at least one duplicate vertex.
				return false;
			}
		}
	}
	return true;
}


#if POLY_MAX_VERTICES==4
void phPolygon::VerifyValidQuad(const Vec3V* UNUSED_PARAM(vertices))
{
	// TODO : Replace this.
	Assert(IsQuad());
/*	Vec3V a,b,c;
	a.Subtract(vertices[Index[0]], vertices[Index[3]]);
	b.Subtract(vertices[Index[2]], vertices[Index[3]]);
	c.Cross(a,b);
	float mag2=MagSquared(c);
	bool pass=true;
	if(mag2>1.0e-12f)
	{
		// The second triangle (not used when calculating the normal vector) has some area.
		c.InvScale(sqrtf(mag2));
		if(c.Dot(Normal)>PARALLEL_COSINE)
		{
			// The second triangle is nearly in the same plane as the first.
			int prevVertIndex=3;
			for(int vertIndex=0;vertIndex<4;vertIndex++)
			{
				a.Subtract(vertices[Index[(vertIndex+1)%4]],vertices[Index[vertIndex]]);
				b.Subtract(vertices[Index[prevVertIndex]],vertices[Index[vertIndex]]);
				c.Cross(a,b);
				if(c.Dot(Normal)<0.0f)
				{
					pass=false;
				}
				prevVertIndex=vertIndex;
			}
		}
		else
		{
			pass=false;
		}
	}
	else
	{
		pass=false;
	}
	if(!pass)
	{
		m_Area=0.0f;
		Normal.Zero();
	}*/
}
#endif

bool phPrimitive::IsBig(const phBoundPolyhedron* bound, ScalarV_In tolerance) const
{
	PrimitiveType type = GetType();

	if (type == PRIM_TYPE_POLYGON)
	{
		const rage::phPolygon& poly = GetPolygon();

		const Vec3V v0 = bound->GetVertex(poly.GetVertexIndex(0));
		const Vec3V v1 = bound->GetVertex(poly.GetVertexIndex(1));
		const Vec3V v2 = bound->GetVertex(poly.GetVertexIndex(2));

		const ScalarV toleranceSquared = tolerance * tolerance;
		BoolV bool0 = IsGreaterThan(DistSquared(v0, v1), toleranceSquared);
		BoolV bool1 = IsGreaterThan(DistSquared(v1, v2), toleranceSquared);
		BoolV bool2 = IsGreaterThan(DistSquared(v2, v0), toleranceSquared);

		return IsTrue(bool0 | bool1 | bool2);
	}
	else if (type == PRIM_TYPE_BOX)
	{
		const rage::phPrimBox& boxPrim = GetBox();

		const Vec3V vertex0 = bound->GetVertex(boxPrim.GetVertexIndex(0));
		const Vec3V vertex1 = bound->GetVertex(boxPrim.GetVertexIndex(1));
		const Vec3V vertex2 = bound->GetVertex(boxPrim.GetVertexIndex(2));
		const Vec3V vertex3 = bound->GetVertex(boxPrim.GetVertexIndex(3));

		const ScalarV half(V_HALF);
		const ScalarV boxX = half * Mag(vertex1 + vertex3 - vertex0 - vertex2);
		const ScalarV boxY = half * Mag(vertex0 + vertex3 - vertex1 - vertex2);
		const ScalarV boxZ = half * Mag(vertex2 + vertex3 - vertex0 - vertex1);
		const Vec3V Width(boxX,boxY,boxZ);

		return !IsLessThanAll(Width, Vec3V(tolerance));
	}

	return false;
}

bool phPolygon::IsThin(const phBoundPolyhedron* bound, ScalarV_In tolerance) const
{
	Assert(bound);

	// See if this is a "bad" polygon, i.e. the base and the height side is unacceptably high.

	// Get the vertices of this polygon.
	// Start by assuming the polygon isn't bad.
	const Vec3V v0 = bound->GetVertex(GetVertexIndex(0));
	const Vec3V v1 = bound->GetVertex(GetVertexIndex(1));
	const Vec3V v2 = bound->GetVertex(GetVertexIndex(2));

	const Vec3V s10 = v1 - v0;
	const Vec3V s21 = v2 - v1;
	const Vec3V s02 = v0 - v2;
	const ScalarV invBaseTimesHeightSquared = InvMagSquared(Cross(s10, s21));
	const ScalarV magSquared02 = MagSquared(s02);
	const ScalarV aspect02 = magSquared02 * magSquared02 * invBaseTimesHeightSquared;
	const ScalarV magSquared10 = MagSquared(s10);
	const ScalarV aspect10 = magSquared10 * magSquared10 * invBaseTimesHeightSquared;
	const ScalarV magSquared21 = MagSquared(s21);
	const ScalarV aspect21 = magSquared21 * magSquared21 * invBaseTimesHeightSquared;

	const ScalarV maxAspect = Max(aspect02, Max(aspect10, aspect21));

	// Make sure that IsSegmentTriangleResultValid considers the center of the polygon as a valid intersection point. If it doesn't then
	//  many geometry functions won't return intersections against it and probe shapetests will fail. 
	const BoolV centerOfPolyConsideredValid = IsSegmentTriangleResultValid(s10,-s02,(s10-s02)*ScalarV(V_THIRD),ScalarV(V_HALF));

	// If the ratio between the longest side & shortest side is too high,
	// then we've got a bad polygon.
	const ScalarV maxSquareAspectRatio(tolerance * tolerance);
	if (IsTrue((maxAspect > maxSquareAspectRatio) | !centerOfPolyConsideredValid))
	{
		return true;
	}

	return false;
}


bool phPolygon::HasBadNormal(const phBoundPolyhedron* bound, ScalarV_In tolerance) const
{
	Vec3V v0 = bound->GetVertex(GetVertexIndex(0));
	Vec3V v1 = bound->GetVertex(GetVertexIndex(1));
	Vec3V v2 = bound->GetVertex(GetVertexIndex(2));
	ScalarV deviation = (MagSquared(ComputeUnitNormal(v0, v1, v2)) - ScalarV(V_ONE));
	return IsTrue(Abs(deviation) > tolerance);
}


bool phPolygon::HasBadNeighbors(phBoundPolyhedron* bound, const char* OUTPUT_ONLY(debugName), bool spew, phPolygon::Index primIndex, bool fix)
{
	Assertf(primIndex != phPolygon::Index(~0) || fix == false, "Need primIndex in order to fix polygons");

	phPolygon* polygons = const_cast<phPolygon*>(bound->GetPolygonPointer());
	const phPolygon::Index triVertIndex0 = GetVertexIndex(0);
	const phPolygon::Index triVertIndex1 = GetVertexIndex(1);
	const phPolygon::Index triVertIndex2 = GetVertexIndex(2);

	Vec3V triVert0 = bound->GetVertex(triVertIndex0);
	Vec3V triVert1 = bound->GetVertex(triVertIndex1);
	Vec3V triVert2 = bound->GetVertex(triVertIndex2);

	bool foundBad = false;

	// Test for and correct evil us's
	for(int neighborPolyi = 0; neighborPolyi < 3; neighborPolyi++)
	{
		phPolygon::Index neighborIndex = GetNeighboringPolyNum(neighborPolyi);
		if(neighborIndex != (phPolygon::Index)(-1) && neighborIndex < bound->GetNumPolygons())
		{
			phPolygon& neighborPoly = polygons[neighborIndex];
			bool isAnEvilUs = (triVertIndex0 == neighborPoly.GetVertexIndex(0) || triVertIndex0 == neighborPoly.GetVertexIndex(1) || triVertIndex0 == neighborPoly.GetVertexIndex(2))
				&& (triVertIndex1 == neighborPoly.GetVertexIndex(0) || triVertIndex1 == neighborPoly.GetVertexIndex(1) || triVertIndex1 == neighborPoly.GetVertexIndex(2))
				&& (triVertIndex2 == neighborPoly.GetVertexIndex(0) || triVertIndex2 == neighborPoly.GetVertexIndex(1) || triVertIndex2 == neighborPoly.GetVertexIndex(2));

			if(isAnEvilUs)
			{
				foundBad = true;

				if (fix)
				{
					neighborPoly.ChangeNeighborIndex(primIndex, (phPolygon::Index)(-1));
					ChangeNeighborIndex(neighborIndex, (phPolygon::Index)(-1));
				}

				if (spew)
				{
					Displayf("Bound: %s  |  Prim: %d -- Neighbor polygon shares all vertices! NeighborPrim: %d, Verts: %d, %d, %d -- Shared: << %f, %f, %f >>", debugName, primIndex, neighborIndex, triVertIndex0, triVertIndex1, triVertIndex2, triVert0.GetXf(), triVert0.GetYf(), triVert0.GetZf());
				}
			}
			else
			{
				Vec3V otherTriVert, myTriVert;
				Vec3V otherTriNorm, myTriNorm;
				switch(neighborPolyi)
				{
				case 0: otherTriVert = bound->GetVertex(neighborPoly.GetOtherVertexIndex(triVertIndex0, triVertIndex1));
					myTriVert = triVert2;
					otherTriNorm = neighborPoly.ComputeUnitNormal(triVert1, triVert0, otherTriVert);
					myTriNorm = ComputeUnitNormal(triVert0, triVert1, triVert2);
					break;
				case 1: otherTriVert = bound->GetVertex(neighborPoly.GetOtherVertexIndex(triVertIndex1, triVertIndex2));
					myTriVert = triVert0;
					otherTriNorm = neighborPoly.ComputeUnitNormal(triVert2, triVert1, otherTriVert);
					myTriNorm = ComputeUnitNormal(triVert0, triVert1, triVert2);
					break;
				default:
				case 2: otherTriVert = bound->GetVertex(neighborPoly.GetOtherVertexIndex(triVertIndex2, triVertIndex0));
					myTriVert = triVert1;
					otherTriNorm = neighborPoly.ComputeUnitNormal(triVert0, triVert2, otherTriVert);
					myTriNorm = ComputeUnitNormal(triVert0, triVert1, triVert2);
					break;
				};

				bool isCloseToEvil = (IsLessThanOrEqualAll( Mag(Add(otherTriNorm, myTriNorm)), ScalarV(V_FLT_SMALL_5) ) != 0);
				if(isCloseToEvil)
				{
					foundBad = true;

					if (fix)
					{
						neighborPoly.ChangeNeighborIndex(primIndex, (phPolygon::Index)(-1));
						ChangeNeighborIndex(neighborIndex, (phPolygon::Index)(-1));
					}

					if (spew)
					{
						Displayf("Bound: %s  |  Prim: %d -- Neighbor polygon too close to being our backface! NeighborPrim: %d, Verts: %d, %d, %d -- Our Verts: %d, %d, %d -- Shared: << %f, %f, %f >>", debugName, primIndex, neighborIndex, neighborPoly.GetVertexIndex(0), neighborPoly.GetVertexIndex(1), neighborPoly.GetVertexIndex(2), triVertIndex0, triVertIndex1, triVertIndex2, myTriVert.GetXf(), myTriVert.GetYf(), myTriVert.GetZf());
					}
				}
			}
		}
	}

	return foundBad;
}

bool phPolygon::HasBadNeighbors(const phBoundPolyhedron* bound, const char* debugName, bool spew) const
{
	return const_cast<phPolygon*>(this)->HasBadNeighbors(const_cast<phBoundPolyhedron*>(bound), debugName, spew);
}


int phPolygon::GetNeighborFromEdgeNormal (Vec3V_In localEdgeNormal, const Vec3V* polyVertices) const
{
	int numEdges = 3;

	Vec3V edgeVector;
	ScalarV minParallelComponent(V_FLT_MAX);
	int perpEdgeIndex = 0;
	int prevVertIndex = 0;
	for (int nextVertIndex=numEdges-1; nextVertIndex>=0; nextVertIndex--)
	{
		edgeVector = Subtract(polyVertices[nextVertIndex],polyVertices[prevVertIndex]);
		ScalarV parallelComponent = Dot(edgeVector, localEdgeNormal);
		if (IsLessThanAll(Abs(parallelComponent),minParallelComponent))
		{
			minParallelComponent = Abs(parallelComponent);
			perpEdgeIndex = nextVertIndex;
		}

		prevVertIndex = nextVertIndex;
	}

	return perpEdgeIndex;
}


int phPolygon::GetNeighborFromExteriorPoint (Vec3V_In exteriorPoint, const Vec3V* polyVertices, Vec3V_In polyNormal) const
{
	const int numEdges = 3;

	ScalarV maxDistSqrd(V_NEGONE);
	int farthestEdgeIndex = -1;
	int prevVertIndex = 0;
	for (int vertIndex=numEdges-1; vertIndex>=0; vertIndex--)
	{
		const Vec3V edgeVec = Subtract(polyVertices[vertIndex], polyVertices[prevVertIndex]);
		const Vec3V edgeToPoint = Subtract(exteriorPoint, polyVertices[prevVertIndex]);
		const Vec3V pointProjectionOnEdge = Scale(edgeVec, InvScale(Dot(edgeToPoint, edgeVec), Dot(edgeVec, edgeVec)));
		const Vec3V pointProjToPoint = Subtract(exteriorPoint, Add(pointProjectionOnEdge, polyVertices[prevVertIndex]));

		const Vec3V edgeNormal = Cross(polyNormal, edgeVec);

		const ScalarV distSqrd = Dot(pointProjToPoint, pointProjToPoint);
		if( (IsGreaterThanAll(Dot(pointProjToPoint, edgeNormal), ScalarV(V_ZERO)) & IsGreaterThanAll(distSqrd, maxDistSqrd)) != 0 )
		{
			maxDistSqrd = distSqrd;
			farthestEdgeIndex = vertIndex;
		}

		prevVertIndex = vertIndex;
	}

	return farthestEdgeIndex;
}


phPolygon::Index phPolygon::FindNeighborWithVertex2 (int vertex, int otherNeighbor) const
{
    int numNeighbors = POLY_MAX_VERTICES;
    for (int neighborIndex=numNeighbors - 1; neighborIndex<=0; neighborIndex--)
    {
        int nextNeighborIndex = (neighborIndex+numNeighbors-1)%numNeighbors;
        int polyVertex = GetVertexIndex(nextNeighborIndex);
        if (polyVertex==vertex)
        {
            phPolygon::Index neighborPoly = m_NeighboringPolygons[neighborIndex];
			return (neighborPoly!=(phPolygon::Index)otherNeighbor ? neighborPoly : m_NeighboringPolygons[nextNeighborIndex]);
        }
    }

    return (phPolygon::Index)BAD_INDEX;
}

bool phPolygon::SegEdgeCheckUndirected(Vec3V_In p0, Vec3V_In p1,
								 Vec3V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross)
{
	Vec3V edge, crossEdges, dispEdges;
	edge = Subtract(p1,p0);
	crossEdges = Cross(edge,Vec3V_ConstRef(RaySeg));		// From the collide edge to the polygon edge (or other direction)
	dispEdges = Subtract(segA,p0);		
	ScalarV segDir = Dot(dispEdges,crossEdges);
	if(IsZeroAll(segDir)) return false;
	ScalarV faceDir = Dot(Vec3V_ConstRef(edgeNormalCross),crossEdges);
	return (!SameSign(faceDir,segDir).Getb());
}

inline bool phPolygon::SegEdgeCheckUndirectedPreCalc(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In RaySeg, Vec4V_In edgeNormalCross, Vec4V_In dispEdges)
{
	Vec3V edge, crossEdges;
	edge = Subtract(p1,p0);
	crossEdges = Cross(edge,Vec3V_ConstRef(RaySeg));		// From the collide edge to the polygon edge (or other direction)
	ScalarV segDir = Dot(Vec3V_ConstRef(dispEdges),crossEdges);
	if(IsZeroAll(segDir)) return false;
	ScalarV faceDir = Dot(Vec3V_ConstRef(edgeNormalCross),crossEdges);
	return (!SameSign(faceDir,segDir).Getb());
}

inline bool phPolygon::SegEdgeCheckDirected(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross)
{
	Vec3V edge, crossEdges, dispEdges;
	edge = Subtract(p1,p0);
	crossEdges = Cross(edge,Vec3V_ConstRef(RaySeg));		// From the collide edge to the polygon edge (or other direction)
	dispEdges = Subtract(Vec3V_ConstRef(segA),p0);
	ScalarV segDir = Dot(dispEdges, crossEdges);
	if(IsZeroAll(segDir)) return false;
	ScalarV faceDir = Dot(Vec3V_ConstRef(edgeNormalCross),crossEdges);
	return And(IsGreaterThan(faceDir, ScalarV(V_ZERO)), IsLessThan(segDir, ScalarV(V_ZERO))).Getb();
}


inline bool phPolygon::SegEdgeCheckDirected(Vec3V_In p0, Vec3V_In p1,
								 Vec3V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross)
{
	Vec3V edge, crossEdges, dispEdges;
	edge = Subtract(p1,p0);
	crossEdges = Cross(edge,Vec3V_ConstRef(RaySeg));		// From the collide edge to the polygon edge (or other direction)
	dispEdges = Subtract(segA,p0);		
	ScalarV segDir = Dot(dispEdges ,crossEdges);
	if(IsZeroAll(segDir)) return false;
	ScalarV faceDir = Dot(Vec3V_ConstRef(edgeNormalCross),crossEdges);
	return And(IsGreaterThan(faceDir, ScalarV(V_ZERO)), IsLessThan(segDir, ScalarV(V_ZERO))).Getb();
}


inline bool phPolygon::SegEdgeCheckDirectedPreCalc(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In RaySeg, Vec4V_In edgeNormalCross, Vec4V_In dispEdges)
{
	Vec3V edge, crossEdges;
	edge = Subtract(p1,p0);
	crossEdges = Cross(edge,Vec3V_ConstRef(RaySeg));		// From the collide edge to the polygon edge (or other direction)
	ScalarV segDir = Dot(Vec3V_ConstRef(dispEdges),crossEdges);
	if(IsZeroAll(segDir)) return false;
	ScalarV faceDir = Dot(Vec3V_ConstRef(edgeNormalCross),crossEdges);
	return And(IsGreaterThan(faceDir, ScalarV(V_ZERO)), IsLessThan(segDir, ScalarV(V_ZERO))).Getb();
}

int phPolygon::DetectSegmentDirected (Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec3V_In segA, Vec3V_In segB) const
{
	int iRet1;
	Vec3V dir, edge1, edge2, edge3, tvec, pvec, pvec2, qvec, qvec2;
	ScalarV det;
	ScalarV u, v;
	ScalarV fEpsilon(1.0f/131072);

	// Calculate the direction vector.
	dir = Subtract (segB, segA);

	// Find vectors for the two edges sharing v0.
	edge1 = Subtract (v1, v0);
	edge2 = Subtract (v2, v0);

	// Calculate determinant.
	pvec = Cross (dir, edge2);
	det = Dot (edge1,pvec);
	if (IsLessThanAll(det, fEpsilon))
	{
		// Segment either in plane, or intersects through backside of polygon.
		return 0;
	}

	// Calculate U parameter and test bounds.
	tvec = segA - v0;
	u = Dot (tvec,pvec);
	if ( Or(IsLessThan(u, ScalarV(V_ZERO)), IsGreaterThan(u, det)).Getb())
	{
		iRet1 = 2;
	}
	else
	{
		// Calculate V parameter and test bounds.
		qvec = Cross (tvec, edge1);
		v = Dot (dir,qvec);
		if ( Or(IsLessThan(v, ScalarV(V_ZERO)), IsGreaterThan(u + v, det)).Getb())
		{
			iRet1 = 3;
		}
		else
		{
			// The segment and triangle intersect.
			return 1;
		}
	}

	return (iRet1 == 0) ? 1 : 0;
}


int phPolygon::DetectSegmentDirected (const Vec3V* vertices, Vec3V_In segA, Vec3V_In segB) const
{
	Vec3V_ConstRef v0 = vertices[GetVertexIndex(0)];
	Vec3V_ConstRef v1 = vertices[GetVertexIndex(1)];
	Vec3V_ConstRef v2 = vertices[GetVertexIndex(2)];

	return DetectSegmentDirected(v0, v1, v2, segA, segB);
}


int phPolygon::DetectSegmentUndirected (Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec4V_In segA, Vec4V_In segB) const
{
	int iRet1;
	Vec3V dir, edge1, edge2, edge3, tvec, pvec, pvec2, qvec, qvec2;
	ScalarV det, sgnDet;
	ScalarV u, v;
	ScalarV fEpsilon(1.0f/131072);

	// Calculate the direction vector.
	dir = Subtract (Vec3V_ConstRef(segB), Vec3V_ConstRef(segA));

	// Find vectors for the two edges sharing v0.
	edge1 = Subtract (v1, v0);
	edge2 = Subtract (v2, v0);

	// Calculate determinant.
	pvec = Cross (dir, edge2);
	det = Dot (edge1,pvec);

	if (IsGreaterThan(det, ScalarV(V_ZERO)).Getb())
	{
		sgnDet = ScalarV(V_ONE);
	}
	else
	{
		sgnDet = ScalarV(V_NEGONE);
		det = -det;
	}

	if (IsLessThanAll(det, fEpsilon))
	{
		return 0;
	}

	// Calculate U parameter and test bounds.
	tvec = Subtract(Vec3V_ConstRef(segA), v0);
	u = Dot (tvec,pvec) * sgnDet;
	if (Or(IsLessThan(u, ScalarV(V_ZERO)), IsGreaterThan(u, det)).Getb())
	{
		iRet1 = 2;
	}
	else
	{
		// Calculate V parameter and test bounds.
		qvec = Cross (tvec, edge1);
		v = Dot (dir,qvec) * sgnDet;
		if (Or(IsLessThan(v, ScalarV(V_ZERO)), IsGreaterThan(u + v, det)).Getb())
		{
			iRet1 = 3;
		}
		else
		{
			// The segment and triangle intersect.
			return 1;
		}
	}

	return (iRet1 == 0) ? 1 : 0;
}


int phPolygon::DetectSegmentUndirected (const Vec3V* vertices, Vec4V_In segA, Vec4V_In segB) const
{
	Vec3V_ConstRef v0 = vertices[GetVertexIndex(0)];
	Vec3V_ConstRef v1 = vertices[GetVertexIndex(1)];
	Vec3V_ConstRef v2 = vertices[GetVertexIndex(2)];

	return DetectSegmentUndirected(v0, v1, v2, segA, segB);
}



