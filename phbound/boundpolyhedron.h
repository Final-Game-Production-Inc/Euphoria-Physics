//
// phbound/boundpolyhedron.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDPOLYHEDRON_H
#define PHBOUND_BOUNDPOLYHEDRON_H

#include "bound.h"
#include "primitives.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

// these need to be accessible outside of __PFDRAW
#define DEFAULT_PFD_SmallPolygonArea				0.001f
#define DEFAULT_PFD_MediumPolygonArea				1.8f
#define DEFAULT_PFD_LargePolygonArea				4.61f
#define DEFAULT_PFD_RecusiveWeightingStyle			2
#define DEFAULT_PFD_NumRecursions					5
#define DEFAULT_PFD_ColorMidPoint					0.08f
#define DEFAULT_PFD_ColorExpBias					0.0f
#define DEFAULT_PFD_PolygonAngleDensity				false
#define DEFAULT_PFD_MaxNeighborAngleEnabled			false
#define DEFAULT_PFD_MaxNeighborAngle				10.0f

#define DEFAULT_PFD_DensityCullRadius				2.0f
#define DEFAULT_PFD_IncludePolygons					false
#define DEFAULT_PFD_MaxPrimitivesPerMeterSquared	4.0f
#define DEFAULT_PFD_MaxNonPolygonsPerMeterSquared	2.0f

#if __PFDRAW
	#define PFD_ONLY(x)				x
	#define PFD_SWITCH(_if_,_else_)	_if_
	#define PFD_GETENABLED(var)		var.GetEnabled()
	#define PFD_GETVALUE(var)		var.GetValue()
#elif __WIN32PC && !__FINAL
	#define PFD_ONLY(x)
	#define PFD_SWITCH(_if_,_else_)	_else_
	#define PFD_GETENABLED(var)		DEFAULT_##var
	#define PFD_GETVALUE(var)		DEFAULT_##var
#endif

namespace rage {

#if HACK_GTA4	// GAME_SPECIFIC
	// Increased the max number of verts in a poly bound for Gta4 from 32->128
	#define MAX_NUM_VERTS	128	// soft maximum for the number of vertices in a non-octree polyhedron (more will cause warnings)
#else
	#define MAX_NUM_VERTS	32	// soft maximum for the number of vertices in a non-octree polyhedron (more will cause warnings)
#endif

// 0 -> no compressed vertices supported
// 1 -> float array (no loss of precision)
// 2 -> 3 x 16bit quantized vertices
#if !__TOOL
// This is the #define for game projects to set to 2 if they want compressed vertices.
#define COMPRESSED_VERTEX_METHOD	2
#else
// This #define should never be changed by game projects because __TOOL builds should always be using uncompressed vertices.
#define COMPRESSED_VERTEX_METHOD	0
#endif

#if COMPRESSED_VERTEX_METHOD == 1
typedef float CompressedVertexType;
#elif COMPRESSED_VERTEX_METHOD == 2
typedef s16 CompressedVertexType;
#endif

#if OCTANT_MAP_SUPPORT_ACCEL
#define OCTANT_MAP_SUPPORT_ACCEL_ONLY(X) X
#else
#define OCTANT_MAP_SUPPORT_ACCEL_ONLY(X)
#endif

#define SUPPORT_FUNCTION_BATCH_DECOMPRESSION	(1 && COMPRESSED_VERTEX_METHOD > 0)

struct phSmallPolygon;

#if __PFDRAW
class phBoundCuller;
#endif 

/*
PURPOSE
	A base class to represent geometrically defined physics bounds.  Classes such as phBoundGeometry and phBoundBox derive from phBoundPolyhedron.
*/
class phBoundPolyhedron : public phBound
{
public:

#if POLYGON_INDEX_IS_U32
	static const unsigned int MAX_NUM_VERTICES  = 0xFFFFFFFE;
	static const unsigned int MAX_NUM_POLYGONS  = 0xFFFFFFFE;
#else
	static const unsigned int MAX_NUM_VERTICES  = 0xFFFE;
	static const unsigned int MAX_NUM_POLYGONS  = 0xFFFE;
#endif

	static const unsigned int MAX_NUM_MATERIALS;
	static const unsigned int INVALID_VERTEX;
	static const unsigned int INVALID_POLYGON;

public:
	////////////////////////////////////////////////////////////
	phBoundPolyhedron ();											// constructor

	////////////////////////////////////////////////////////////
	//// Access functions ////
	int GetNumVertices () const;
	int GetNumPolygons () const;
	int GetNumConvexHullVertices() const;
	void ClearPolysAndVerts ();

	// array pointer accessors
#if COMPRESSED_VERTEX_METHOD == 0
	const Vec3V * GetVertexPointer () const;
#endif
	const phPolygon * GetPolygonPointer () const;

#if COMPRESSED_VERTEX_METHOD == 0
	// Gta uses GetShrunkVertexPointer to check that the bound has shrunk verts allocated before calling GetShrunkVertex(i)
	// Might as well keep this, since otherwise if we ever turn compressed verts off we won't be able to compile
	// Want to move to rage/dev, so long as m_ShrunkVertices still exists there.
	const Vec3V * GetShrunkVertexPointer () const					{ return m_ShrunkVertices; }
#else
	const CompressedVertexType * GetShrunkVertexPointer () const	{ return m_CompressedShrunkVertices; }
#endif

	// data accessors
#if COMPRESSED_VERTEX_METHOD > 0
	__forceinline Vec3V_Out GetLocalSpaceOffset() const
	{
#if COMPRESSED_VERTEX_METHOD == 1
		return GetCentroidOffset();
#elif COMPRESSED_VERTEX_METHOD == 2
		return m_BoundingBoxCenter;
#endif
	}

	__forceinline Vec3V_Out DecompressVertex (const CompressedVertexType * RESTRICT compressedVector) const
	{
#if COMPRESSED_VERTEX_METHOD == 1
		return Vec3V(Vec::V4LoadUnaligned(compressedVector));
#elif COMPRESSED_VERTEX_METHOD == 2
#if 0
		Vec3V temp((float)(compressedVector[0]), (float)(compressedVector[1]), (float)(compressedVector[2]));
		temp = Scale(temp, m_UnQuantizeFactor);
		temp = Add(temp, m_BoundingBoxCenter);
		return temp;
#elif RSG_CPU_INTEL
		// Using doubles to match the apparent precision of the below (Xenon/PPU/SPU) combined mult and add intrinsic
		// TODO -- Note that this sucks for any PC builds of the game, though probably only a little bit more than the previous (above)
		//      -- May need to come up with an equivalent solution involving SSE for the PC runtime
		double vert0 = (double)(compressedVector[0]);
		double vert1 = (double)(compressedVector[1]);
		double vert2 = (double)(compressedVector[2]);
		vert0 *= (double)(m_UnQuantizeFactor.GetXf());
		vert1 *= (double)(m_UnQuantizeFactor.GetYf());
		vert2 *= (double)(m_UnQuantizeFactor.GetZf());
		vert0 += (double)(m_BoundingBoxCenter.GetXf());
		vert1 += (double)(m_BoundingBoxCenter.GetYf());
		vert2 += (double)(m_BoundingBoxCenter.GetZf());
		return(Vec3V((float)(vert0), (float)(vert1), (float)(vert2)));
#else
 		using namespace Vec;
		Vector_4V temp = V4LoadUnalignedSafe<6>(compressedVector);
		temp = V4UnpackLowSignedShort(temp);
		temp = V4IntToFloatRaw<0>(temp);
		temp = V4AddScaled(m_BoundingBoxCenter.GetIntrin128(), temp, m_UnQuantizeFactor.GetIntrin128());
		return Vec3V(temp);
#endif
#endif
	}

	__forceinline Vec3V_Out DecompressVertexLocalSpace (const CompressedVertexType * RESTRICT compressedVector) const
	{
#if COMPRESSED_VERTEX_METHOD == 1
		Vec3V temp = Vec3V(Vec::V4LoadUnaligned(compressedVector));
		temp -= GetCentroidOffset();
		return temp;
#elif COMPRESSED_VERTEX_METHOD == 2
#if 0
		Vec3V temp((float)(compressedVector[0]), (float)(compressedVector[1]), (float)(compressedVector[2]));
		temp = Scale(temp, m_UnQuantizeFactor);
		return temp;
#else
		using namespace Vec;
		Vector_4V temp = V4LoadUnalignedSafe<6>(compressedVector);
		temp = V4UnpackLowSignedShort(temp);
		temp = V4IntToFloatRaw<0>(temp);
		temp = V4Scale(temp, m_UnQuantizeFactor.GetIntrin128());
		return Vec3V(temp);
#endif
#endif
	}

	__forceinline Vec3V_Out GetCompressedVertex (int i) const
	{
		TrapLT(i,0);
		TrapGE(i,m_NumVertices);
		const CompressedVertexType * RESTRICT compressedVertex = m_CompressedVertices + i * 3;
		return DecompressVertex(compressedVertex);
	}
#endif

	Vec3V_Out GetVertex (int i) const;

	Vec3V_Out GetShrunkVertex(int i) const
	{
		FastAssert(i >= 0);
		FastAssert(i < m_NumVertices);
		register Vec3V retVal;
#if COMPRESSED_VERTEX_METHOD > 0
		FastAssert(m_CompressedShrunkVertices != NULL);
		retVal = DecompressVertex(m_CompressedShrunkVertices + 3 * i);
#else
		FastAssert(m_ShrunkVertices);
		retVal = m_ShrunkVertices[i];
#endif
		return retVal;
	}


#if COMPRESSED_VERTEX_METHOD > 0
	// Give the SPU some help to do what it needs to do.
	// It sure would be nice to hide some of these implementation details, but I guess this isn't that bad for now.
	const CompressedVertexType *GetCompressedVertexPointer() const
	{
		return m_CompressedVertices;
	}

#if __SPU
	void SetCompressedVertexPointer(CompressedVertexType* compressedVertexPointer) 
	{
		m_CompressedVertices = compressedVertexPointer;
	}
#endif

#endif

#if __SPU && (COMPRESSED_VERTEX_METHOD == 0)
	void SetVertexPointer(const Vec3V *vertices)
	{
		m_Vertices = vertices;
	}
#endif


	// Swaps the vertices at the two specified indices.  Not that this *does* not swap shrunk vertices, if they exist.
	void SwapVertex(const int v0, const int v1)
	{
		// Note: When using compressed vertices this could be done slightly faster by avoiding the decompression/compression.
		Vec3V temp(GetVertex(v0));
		SetVertex(v0, GetVertex(v1));
		SetVertex(v1, temp);
	}


	int		GetNumPerVertexAttribs() const								{ return m_NumPerVertexAttribs; }
	
	Color32 GetVertexAttrib(int i, int attrib=0) const
	{
		FastAssert(m_NumPerVertexAttribs>0);
		TrapLT(i,0);
		TrapGE(i,m_NumVertices);
		return Color32( m_VertexAttribs[i*m_NumPerVertexAttribs + attrib] );
	}

	void SetVertexAttrib(int i, int attrib, Color32 val)
	{
		FastAssert(m_NumPerVertexAttribs>0);
		TrapLT(i,0);
		TrapGE(i,m_NumVertices);
		m_VertexAttribs[i*m_NumPerVertexAttribs + attrib] = val.GetColor();
	}

	// PURPOSE: Get a reference to the specified polygon.
	// PARAMS:
	//	polyIndex - the index number of the polygon to get
	// RETURN:	a reference to the polygon with the given index
	__forceinline const phPolygon & GetPolygon (int polyIndex) const
	{
		FastAssert(polyIndex>=0);
		FastAssert(polyIndex<m_NumPolygons);
		return m_Polygons[polyIndex];
	}

	// PURPOSE: Get all the vertex locations for the given polygon.
	// PARAMS:
	//	polygon - reference to the polygon whose vertex locations to get
	//	v0 - reference to fill in the first vertex location
	//	v1 - reference to fill in the second vertex location
	//	v2 - reference to fill in the third vertex location
	// NOTES:	When the polygon is a triangle, the fourth vertex location is not filled in.
	void GetPolygonVertices (const phPolygon& polygon, Vec3V_InOut v0, Vec3V_InOut v1, Vec3V_InOut v2) const;

#if __TOOL || __RESOURCECOMPILER
	// PURPOSE: Get a mutable reference to the specified polygon.
	// PARAMS:
	//	polyIndex - the index number of the polygon to get
	// RETURN:	a reference to the polygon with the given index
	// NOTES:	This is for combining polygonal objects when exporting.
	phPolygon & GetPolygon (int polyIndex)
	{
		Assertf(polyIndex>=0,"phBoundPolyhedron::GetPolygon() has polygon index %i",polyIndex);
		Assertf(polyIndex<m_NumPolygons,"phBoundPolyhedron::GetPolygon() has polygon index %i and only %i polygons.",polyIndex,m_NumPolygons);
		//Assertf(m_Polygons[polyIndex].GetPrimitive().GetType() == PRIM_TYPE_POLYGON, "phBoundPolyhedron::GetPolygon() is trying to access a %s at index %i, call phBoundBvh::GetPrimitive() for non polygons.", m_Polygons[polyIndex].GetPrimitive().GetTypeName(), polyIndex);
		return *((phPolygon*)m_Polygons+polyIndex);
	}
#endif // __TOOL || __RESOURCECOMPILER

	// PURPOSE: Set the position of the specified vertex.
	// PARAMS:
	//	vertexIndex - the index number of the vertex to set
	//	localPosition - the position in the object's coordinate system of the specified vertex
	void SetVertex (int vertexIndex, Vec3V_In localPosition);

	// PURPOSE: Set the position of the specified shrunk vertex.
	// PARAMS:
	//	vertexIndex - the index number of the vertex to set
	//	localPosition - the position in the object's coordinate system of the specified vertex
	// RETURN: Boolean indicating whether or not this call actually changed the compressed vector
	bool SetShrunkVertex(const int shrunkVertexIndex, Vec3V_In localPosition) const;

	// PURPOSE: Set the specified polygon.
	// PARAMS:
	//	polygonIndex - the index number of the polygon to set
	//	polygon - reference to the polygon information to copy into this bound's polygon
	void SetPolygon (int polygonIndex, const phPolygon& polygon);

	// PURPOSE: Use the specified vertices from this bound to initialize a triangle phPolygon.
	// PARAMS:
	//   poly - the polygon to be initialized
	//   v0, v1, v2 - the indices of the vertices to use 
	void InitPolygonTri(phPolygon &poly, phPolygon::Index v0, phPolygon::Index v1, phPolygon::Index v2) const;

	// PURPOSE: This is the same as InitPolygonTri() except that it won't try and look into the bound to get vertices and compute triangle area.
	void InitPolygonTriNoVerts(phPolygon &poly, phPolygon::Index v0, phPolygon::Index v1, phPolygon::Index v2) const;

	// PURPOSE: Use the specified vertex from this bound and a radius to initialize a sphere primitive.
	// PARAMS:
	//   poly - the polygon (primitive) to be initialized
	//   sphereCenter - index of the vertex to use for the center
	//   radius - the radius of the sphere
	// NOTES:
	//   This function takes a 'phPolygon' but that is merely for convenience because phBoundPolyhedron maintains an array of phPolygons - phPolygon
	//     doubles as 'generic primitive' right now (classes will be adjusted in the future).
	void InitSphere(phPolygon &poly, phPolygon::Index sphereCenter, float radius) const;

	// PURPOSE: Use the specified vertices from this bound and a radius to initialize a capsule primitive.
	// PARAMS:
	//   poly - the polygon (primitive) to be initialized
	//   end0, end1 - indices of the vertices to use for the endpoints
	//   radius - the radius of the capsule
	// NOTES:
	//   This function takes a 'phPolygon' but that is merely for convenience because phBoundPolyhedron maintains an array of phPolygons - phPolygon
	//     doubles as 'generic primitive' right now (classes will be adjusted in the future).
	void InitCapsule(phPolygon &poly, phPolygon::Index end0, phPolygon::Index end1, float radius) const;

	// PURPOSE: Use the specified vertices from this bound to initialize a box primitive.
	// PARAMS:
	//   poly - the polygon (primitive) to be initialized
	//   vert0, vert1, vert2, vert3 - indices of the vertices of opposite diagonals of the box
	// NOTES:
	//   This function requires four vertices for the box, but they need to come from 'opposite diagonals' of the box.  In other words, the
	//     distance between vert0 and vert1 should be the same as the distance between vert2 and vert3, and the distance between vert0 and vert2
	//     should be the same as the distance between vert1 and vert3.
	//   This function takes a 'phPolygon' but that is merely for convenience because phBoundPolyhedron maintains an array of phPolygons - phPolygon
	//     doubles as 'generic primitive' right now (classes will be adjusted in the future).
	void InitBox(phPolygon &poly, phPolygon::Index vert0, phPolygon::Index vert1, phPolygon::Index vert2, phPolygon::Index vert3) const;

	// PURPOSE: Use the specified vertices from this bound and a radius to initialize a cylinder primitive.
	// PARAMS:
	//   poly - the polygon (primitive) to be initialized
	//   end0, end1 - indices of the vertices to use for the endpoints
	//   radius - the radius of the capsule
	// NOTES:
	//   This function takes a 'phPolygon' but that is merely for convenience because phBoundPolyhedron maintains an array of phPolygons - phPolygon
	//     doubles as 'generic primitive' right now (classes will be adjusted in the future).
	void InitCylinder(phPolygon &poly, phPolygon::Index end0, phPolygon::Index end1, float radius) const;

	// PURPOSE: Set the flag to tell that this bound is flat.
	// PARAMS:
	//	isFlat - optional flag to make this polygon treated as flat or not
	void SetIsFlat (bool isFlat=true);

	//
	int DetectSegmentUndirected (const phPolygon &poly, Vec4V_In segA, Vec4V_In segB) const;

	//
	int DetectSegmentDirected (const phPolygon &poly, Vec3V_In segA, Vec3V_In segB) const;

	PH_NON_SPU_VIRTUAL void CalcCGOffset (Vec3V_InOut CGOffset) const;

#if COMPRESSED_VERTEX_METHOD > 0
	// These functions are needed if you're creating a polyhedral bound with compressed vertices other than through the 'normal' means (which are loading
	//   from a file or resource or Copy()ing from another bound).  Essentially, you must call these to specify the bounding box before you can start
	//   calling SetVertex() but there was no public interface for setting that (or the quantization values derived from it).
	// As a cute little aside, when using COMPRESSED_VERTEX_METHOD == 2, the vertex locations are relative to the bounding box center so calling this
	//   after you've quantized your vertices will the scale the bound (even non-uniformly if desired) quite cheaply and easily.  However, if you choose to
	//   do that, be aware that you'd also need to recompute your bounding sphere and, if there are any instances inserted into the physics level that are
	//   currently using the bound, you'd also need to update the physics level with its new size.
	void InitQuantization(const phBoundPolyhedron *otherBound);
	void InitQuantization(Vec3V_In boundingBoxMin, Vec3V_In boundingBoxMax);
#endif

	// RETURNS: Number of points that were on the convex hull.
	static int SortConvexHullPointsToFront(Vec3V_Ptr vertices, u16 *oldToNewVertexMapping, const int numPoints);
	static void FindAndMoveInteriorPoints(Vec3V_Ptr vertices, u16 *oldToNewVertexMapping, u16 *newToOldVertexMapping, const int startIndex, 
		int &firstInteriorPoint, const phSmallPolygon *convexHullTris, const int numConvexHullTris);

#if !__SPU
	virtual bool IsPolygonal (int UNUSED_PARAM(component)) const			{ return true; }

	virtual void Copy (const phBound* original);
#endif

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const;

#if !__SPU
	////////////////////////////////////////////////////////////
	// visualization
#if __PFDRAW
	PH_NON_SPU_VIRTUAL void CullSpherePolys (phBoundCuller& culler, Vec3V_In sphereCenter, ScalarV_In sphereRadius) const;
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
	virtual void DrawNormals(Mat34V_In mtx, int normalType = FACE_NORMALS, int whichPolys = ALL_POLYS, float length = 1.0f, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;
#endif // __PFDRAW

#if __NMDRAW
  virtual void NMRender(const Matrix34& mtx) const;
#endif

	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return phMaterialMgr::DEFAULT_MATERIAL_ID; }
#endif // !__SPU

	// PURPOSE: Get the index number in the bound's list of materials for the given polygon index.
	// PARAMS:
	//	polygonIndex - the index number of the polygon for which to get the material index
	PH_NON_SPU_VIRTUAL phMaterialIndex GetPolygonMaterialIndex (int UNUSED_PARAM(polygonIndex)) const {return 0;}

	// PURPOSE: Set the index number in the bound's list of materials for the given polygon index.
	// PARAMS:
	//	polygonIndex - the index number of the polygon for which to set the material index
	//	materialIndex - the material index in this bound for the given polygon index
	PH_NON_SPU_VIRTUAL void SetPolygonMaterialIndex (int UNUSED_PARAM(polygonIndex), phMaterialIndex UNUSED_PARAM(materialIndex)) {}

	Vec3V_Out GetPolygonUnitNormal (int polygonIndex) const;

	Vec3V_Out GetPolygonNonUnitNormal (int polygonIndex) const;

#if !__SPU
	ScalarV_Out DistToPolygon(Vec3V_In point, int polyIndex, Vec3V_Ptr normal=NULL) const;
	void GetSegments (const Vec3V* transformVerts, phSegment* segments) const;

#if __ASSERT
	void AssertOnBadPolys();
#endif
	////////////////////////////////////////////////////////////
	// resources
	phBoundPolyhedron (datResource & rsc);							// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT
	void ComputeNeighbors (const char* filename);
	static void SetComputeNeighbors(bool b)		{sm_ComputeNeighbors=b;}

	void ComputeVertexNormals (Vec3V_Ptr vertexNormalsOut) const;
	void SetCompressedVertexNormals (Vec3V_Ptr vertexNormals);
#endif // !__SPU

#if OCTANT_MAP_SUPPORT_ACCEL
	static bool& GetUseOctantMapRef() { return sm_UseOctantMap; }
#endif

public:

	static bool VertexToMovingSphereIntersection (Vec3V_In axisPoint, Vec3V_In unitAxis, Vec3V_In axisEndPos, ScalarV_In radius2,
													Vec3V_In vertex, ScalarV_InOut depth, Vec3V_InOut position, Vec3V_InOut normal);
	static bool EdgeToMovingSphereIntersection (Vec3V_In axisPoint, Vec3V_In axis, Vec3V_In axisEndPos, Vec3V_In edgePoint,
												Vec3V_In edge, ScalarV_In radius, ScalarV_In axisLength, ScalarV_InOut isectDepth, Vec3V_Ref isectPosition,
												Vec3V_Ref isectNormal, Vec3V_In normal1, const Vec3V* normal2, ScalarV_In edgeCosine = ScalarV(V_ZERO));
		
	void CalculateBoundingSphere(Vec3V_InOut rvecSphereCenter, ScalarV_InOut rfSphereRadius) const;

#if COMPRESSED_VERTEX_METHOD > 0
	// Calculates quantization values based on the bounding box.
	// m_BoundingBoxMin and m_BoundingBoxMax must be set to appropriate values before this is called.
	void CalculateQuantizationValues ();
#endif

#if __PFDRAW || (__WIN32PC && !__FINAL)
	float RatePolygon(phPolygon::Index polygonIndex, ScalarV_In minArea, ScalarV_In maxArea, Vec3V_In quadraticConstants) const;
	float RatePolygonRecursive(phPolygon::Index polygonIndex, int numRecursions) const;
	Color32 ComputeDensityColor(float value) const;
	Color32 ComputePolygonDensityColor(phPolygon::Index polygonIndex, u32 typeFlags, u32 includeFlags) const;
	Color32 ComputePolygonAngleDensityColor(phPolygon::Index polygonIndex) const;
#endif // __PFDRAW || (__WIN32PC && !__FINAL)

#if __PFDRAW
	float GetMaxTypeIncludeFlagScale(u32 typeFlags, u32 includeFlags) const;
	static float sm_TypeFlagDensityScaling[32];
	static float sm_IncludeFlagDensityScaling[32];
#endif // __PFDRAW

#if !__SPU
protected:
#endif
	static bool TestVertexToMovingSphere(Vec3V_In axisPoint, Vec3V_In unitAxis, ScalarV_In radius2,
											Vec3V_In edgePoint, Vec3V_In edge);
#if !__SPU
	static bool sm_ComputeNeighbors;										// should we compute neighbors for edge collisions?
#endif

#if __PFDRAW
#if COMPRESSED_VERTEX_METHOD == 0
#else
	void DrawSolidMarginExpandedShrunkRoundEdges(Mat34V_In mtx, const CompressedVertexType* compressedVerticesToDraw, Color32 oldColor) const;
#endif
	void DrawInternal(int polygonIndex, Vec3V_In center) const;
#endif // __PFDRAW

#if OCTANT_MAP_SUPPORT_ACCEL
	static bool sm_UseOctantMap;
#endif

	//=========================================================================
	// data

#if COMPRESSED_VERTEX_METHOD == 0
	// PURPOSE: Array of vertex positions that this bound uses.
	// NOTES:	This pointer has const access for performance because the vertex locations are normally changed only during loading.
	const Vec3V * m_Vertices;
#else
	u32 m_VerticesPad;
#endif

	// PURPOSE: Vertices that have been shrunk inwards by a margin
#if COMPRESSED_VERTEX_METHOD == 0
	Vec3V* m_ShrunkVertices;
#else
	CompressedVertexType * m_CompressedShrunkVertices;
#endif


	// PURPOSE: True if this bound supports subsets of itself being active at any time.
	// NOTES
	//   This will be true for derived bounds that are capable of culling subsets
	//   of their polygons for various purposes (e.g. octrees).
	bool m_UseActiveComponents;

	// PURPOSE: flag to tell if this bound is flat
	// NOTES:	This is only used for solid debug drawing to draw the backs of the polygons (previously this space was padding)
	bool m_IsFlat;

	// PURPOSE: amount of per-vertex Color32 attributes stored in m_VertexAttribs
	// NOTES:	0=nothing, 1=default, could be 2,3 and more in the future
	u8 m_NumPerVertexAttribs;

	// PURPOSE: The number of vertices on the convex hull of this bound.
	// NOTES
	//   This is particularly useful because the vertex arrays (m_CompressedVertices and m_CompressedShrunkVertices is this bound
	//   has compressed vertices and m_Vertices and m_ShrunkVertices otherwise) are sorted such that the vertices that are on the
	//   convex hull will come before any vertices that aren't on the convex hull.
	u16 m_NumConvexHullVertices;

	// PURPOSE: Array of polygons that compose this bound.
	// NOTES:	This pointer has const access for performance because the polygons are normally changed only during loading.
	const phPolygon * m_Polygons;

#if COMPRESSED_VERTEX_METHOD == 0
	// PURPOSE: Fill up space that would be otherwise occupied by a pointer.
	u8 m_MorePad[4];
#elif COMPRESSED_VERTEX_METHOD == 1
	CompressedVertexType *m_CompressedVertices;
#elif COMPRESSED_VERTEX_METHOD == 2
	Vec3V m_UnQuantizeFactor;
	Vec3V m_BoundingBoxCenter;
	CompressedVertexType *m_CompressedVertices;
	//Vec3V m_QuantizeFactor;
#endif

	u32 *m_VertexAttribs;

#if OCTANT_MAP_SUPPORT_ACCEL

#if USE_OCTANT_MAP_INDEX_U16
	typedef u16 OctantMapIndexType;
#else // USE_OCTANT_MAP_INDEX_U16
	typedef u32 OctantMapIndexType;
#endif // USE_OCTANT_MAP_INDEX_U16

	// PURPOSE: The number of verts in each octant, or NULL if the octant map is not enabled
	OctantMapIndexType* m_OctantVertCounts;

	// PURPOSE: The vert indices for each octant, or NULL if the octant map is not enabled
	OctantMapIndexType** m_OctantVerts;

	void OctantMapAllocateAndCopy(OctantMapIndexType * octantVertCounts, OctantMapIndexType ** octantVerts);
	void OctantMapDelete();

	// GetOctantVertCount: This is mostly a reference function that illustrates how the u16 compression works. Try not using if possible.
	OctantMapIndexType GetOctantVertCount(const int octant) const
	{
		FastAssert(octant >= 0 && octant < 8);
#if USE_OCTANT_MAP_INDEX_U16
		if (m_Flags & OCTANT_MAP_INDEX_IS_U16)
			return m_OctantVertCounts[octant];
		else
			return (OctantMapIndexType)(((u32*)m_OctantVertCounts)[octant]);
#else // USE_OCTANT_MAP_INDEX_U16
		return m_OctantVertCounts[octant];
#endif // USE_OCTANT_MAP_INDEX_U16
	}

	// GetOctantVertIndex: This is mostly a reference function that illustrates how the u16 compression works. Try not using if possible.
	OctantMapIndexType GetOctantVertIndex(const int octant, const OctantMapIndexType index)
	{
		FastAssert(octant >= 0 && octant < 8);
		FastAssert(index < GetOctantVertCount(octant));
#if USE_OCTANT_MAP_INDEX_U16
		if (m_Flags & OCTANT_MAP_INDEX_IS_U16)
			return m_OctantVerts[octant][index];
		else
			return (OctantMapIndexType)(((u32*)m_OctantVerts[octant])[index]);
#else // USE_OCTANT_MAP_INDEX_U16
		return m_OctantVerts[octant][index];
#endif // USE_OCTANT_MAP_INDEX_U16
	}

public:
	__forceinline int HasOctantMap() const
	{
#if USE_OCTANT_MAP_PERMANENT_ALLOCATION
		const int hasOctantMap = m_Flags & GEOMETRY_BOUND_HAS_OCTANT_MAP;
#else // USE_OCTANT_MAP_PERMANENT_ALLOCATION
		const int hasOctantMap = (m_OctantVertCounts != NULL);
#endif // USE_OCTANT_MAP_PERMANENT_ALLOCATION
#if __ASSERT
		if (hasOctantMap)
		{
			FastAssert(m_OctantVertCounts);
			FastAssert(m_OctantVerts);
		}
#endif // __ASSERT
		return hasOctantMap;
	}
protected:
#endif // OCTANT_MAP_SUPPORT_ACCEL

	// PURPOSE: Number of vertices in this bound.
	int m_NumVertices;

	// PURPOSE: Number of polygons in this bound.
	int m_NumPolygons;

#if OCTANT_MAP_SUPPORT_ACCEL
	u32 m_Pad0[3+3];
#else // OCTANT_MAP_SUPPORT_ACCEL
	u32 m_Pad0[1+3];
#endif // OCTANT_MAP_SUPPORT_ACCEL

	// Added post gta4, to improve volume calculations
	// Want to take across to rage\dev branch
#if !__SPU
protected:
	PH_NON_SPU_VIRTUAL bool HasOpenEdges() const {return false;}
#endif
};
#if RSG_CPU_X64 && !RSG_TOOL
CompileTimeAssert(sizeof(phBoundPolyhedron)==240);	// double check padding and size
#endif

inline void phBoundPolyhedron::SetVertex (int vertexIndex, Vec3V_In localPosition)
{
#if !__TOOL
	FastAssert(vertexIndex<m_NumVertices);
#endif

	{
#if COMPRESSED_VERTEX_METHOD == 0
		*const_cast<Vec3V*>(&m_Vertices[vertexIndex]) = localPosition;
#endif
	}
#if COMPRESSED_VERTEX_METHOD > 0
	{
#if COMPRESSED_VERTEX_METHOD == 1
		const int baseIndex = 3 * vertexIndex;
		m_CompressedVertices[baseIndex] = localPosition.x;
		m_CompressedVertices[baseIndex + 1] = localPosition.y;
		m_CompressedVertices[baseIndex + 2] = localPosition.z;
#elif COMPRESSED_VERTEX_METHOD == 2
		Vec3V quantizeFactor;
		quantizeFactor = Invert(m_UnQuantizeFactor);

		// Ensure that the vertex positions aren't too close to the bounding box edges.
		Vec3V quantizedVertex = localPosition;

#if HACK_GTA4	// Should really clamp on the game side
		// The asserts below were added on the rage/dev branch, but we're hitting them in gta when
		// we deform vehicle bounds at runtime.
		// So I put the code back in to clamp the vertex positions so they don't wrap around
		// that we had for gta4
		// Need to speak to Eugene to see if we could keep that for the main rage branch,
		// or if we're doing something wrong.
#else
#if !__SPU
		Assert(quantizedVertex.IsGreaterOrEqualThan(VEC3V_TO_VECTOR3(GetBoundingBoxMin())));
		Assert(quantizedVertex.IsLessOrEqualThan(VEC3V_TO_VECTOR3(GetBoundingBoxMax())));
		Assert(GetType() == phBound::BOX || m_CompressedShrunkVertices != NULL || quantizedVertex.IsGreaterOrEqualThan(VEC3V_TO_VECTOR3(GetBoundingBoxMin() + Vec3V(GetMarginV())) - VEC3_HALF * m_UnQuantizeFactor));
		Assert(GetType() == phBound::BOX || m_CompressedShrunkVertices != NULL || quantizedVertex.IsLessOrEqualThan(VEC3V_TO_VECTOR3(GetBoundingBoxMax() - Vec3V(GetMarginV())) + VEC3_HALF * m_UnQuantizeFactor));
#endif // !__SPU
#endif // HACK_GTA4

		quantizedVertex = Subtract(quantizedVertex, m_BoundingBoxCenter);
		quantizedVertex = Scale(quantizedVertex, quantizeFactor);
		quantizedVertex += Vec3V(V_HALF);
		quantizedVertex = FloatToIntRaw<0>(quantizedVertex);
//		Assert(quantizedVertex.x < 32767.5f);
//		Assert(quantizedVertex.y < 32767.5f);
//		Assert(quantizedVertex.z < 32767.5f);
//		Assert(quantizedVertex.x >= -32768.5f);
//		Assert(quantizedVertex.y >= -32768.5f);
//		Assert(quantizedVertex.z >= -32768.5f);
		m_CompressedVertices[vertexIndex * 3] = (CompressedVertexType)(quantizedVertex.GetXi());
		m_CompressedVertices[vertexIndex * 3 + 1] = (CompressedVertexType)(quantizedVertex.GetYi());
		m_CompressedVertices[vertexIndex * 3 + 2] = (CompressedVertexType)(quantizedVertex.GetZi());
#endif
	}
#endif
}

inline bool phBoundPolyhedron::SetShrunkVertex(const int shrunkVertexIndex, Vec3V_In localPosition) const
{
	FastAssert(shrunkVertexIndex >= 0);
	FastAssert(shrunkVertexIndex <= m_NumVertices);
#if COMPRESSED_VERTEX_METHOD == 0
	FastAssert(m_ShrunkVertices != NULL);
	bool vertIsChanged = IsEqualAll(m_ShrunkVertices[shrunkVertexIndex], localPosition) == 0;
	m_ShrunkVertices[shrunkVertexIndex] = localPosition;
	return vertIsChanged;
#else
	Vec3V quantizeFactor;
	quantizeFactor = Invert(m_UnQuantizeFactor);
	Vec3V quantizedVertex = localPosition;

#if HACK_GTA4	// Should really clamp on the game side
	// Clamp the vertex positions so they don't wrap around
	// See comment for previous function
#else
	// Ensure that the shrunk vertices won't push the bound outside of the bounding box when the margin gets added on.
#if !__SPU
	Assert(quantizedVertex.IsGreaterOrEqualThan(VEC3V_TO_VECTOR3(GetBoundingBoxMin() + Vec3V(GetMarginV())) - VEC3_HALF * m_UnQuantizeFactor));
	Assert(quantizedVertex.IsLessOrEqualThan(VEC3V_TO_VECTOR3(GetBoundingBoxMax() - Vec3V(GetMarginV())) + VEC3_HALF * m_UnQuantizeFactor));
#endif
#endif //HACK_GTA4

	quantizedVertex = Subtract(quantizedVertex, m_BoundingBoxCenter);
	quantizedVertex = Scale(quantizedVertex, quantizeFactor);
	quantizedVertex += Vec3V(V_HALF);
#if COMPRESSED_VERTEX_METHOD == 1
	CompressedVertexType vertX = (CompressedVertexType)(quantizedVertex.GetXf());
	CompressedVertexType vertY = (CompressedVertexType)(quantizedVertex.GetYf());
	CompressedVertexType vertZ = (CompressedVertexType)(quantizedVertex.GetZf());
#else //COMPRESSED_VERTEX_METHOD == 1
	quantizedVertex = FloatToIntRaw<0>(quantizedVertex);
	//here COMPRESSED_VERTEX_METHOD = 2
	CompressedVertexType vertX = (CompressedVertexType)(quantizedVertex.GetXi());
	CompressedVertexType vertY = (CompressedVertexType)(quantizedVertex.GetYi());
	CompressedVertexType vertZ = (CompressedVertexType)(quantizedVertex.GetZi());
#endif //COMPRESSED_VERTEX_METHOD == 1

	bool vertIsChanged = (m_CompressedShrunkVertices[shrunkVertexIndex * 3] != vertX || 
		m_CompressedShrunkVertices[shrunkVertexIndex * 3 + 1] != vertY || 
		m_CompressedShrunkVertices[shrunkVertexIndex * 3 + 2] != vertZ);

	m_CompressedShrunkVertices[shrunkVertexIndex * 3] = vertX;
	m_CompressedShrunkVertices[shrunkVertexIndex * 3 + 1] = vertY;
	m_CompressedShrunkVertices[shrunkVertexIndex * 3 + 2] = vertZ;

	return vertIsChanged;
#endif // COMPRESSED_VERTEX_METHOD == 0
}


inline void phBoundPolyhedron::GetPolygonVertices (const phPolygon& polygon, Vec3V_InOut v0, Vec3V_InOut v1, Vec3V_InOut v2) const
{
	v0 = GetVertex(polygon.GetVertexIndex(0));
	v1 = GetVertex(polygon.GetVertexIndex(1));
	v2 = GetVertex(polygon.GetVertexIndex(2));
}


inline void phBoundPolyhedron::SetPolygon (int polygonIndex, const phPolygon& polygon)
{
#if !__TOOL
	FastAssert(polygonIndex<m_NumPolygons);
#endif

	const_cast<phPolygon*>(&m_Polygons[polygonIndex])->Set(polygon);
}


inline void phBoundPolyhedron::InitPolygonTri(phPolygon &poly, phPolygon::Index v0, phPolygon::Index v1, phPolygon::Index v2) const
{
	poly.InitTriangle(v0, v1, v2, GetVertex(v0), GetVertex(v1), GetVertex(v2));

	phPrimitive *primitive = &poly.GetPrimitive();
	primitive->SetType(PRIM_TYPE_POLYGON);
}


inline void phBoundPolyhedron::InitPolygonTriNoVerts(phPolygon &poly, phPolygon::Index v0, phPolygon::Index v1, phPolygon::Index v2) const
{
	poly.SetVertexIndices(v0, v1, v2);

	phPrimitive *primitive = &poly.GetPrimitive();
	primitive->SetType(PRIM_TYPE_POLYGON);
}


inline void phBoundPolyhedron::InitSphere(phPolygon &poly, phPolygon::Index sphereCenter, float radius) const
{
	phPrimitive &primitive = poly.GetPrimitive();
	primitive.SetType(PRIM_TYPE_SPHERE);

	phPrimSphere &spherePrim = primitive.GetSphere();
	spherePrim.SetCenterIndexAndRadius(sphereCenter, radius);
}


inline void phBoundPolyhedron::InitCapsule(phPolygon &poly, phPolygon::Index end0, phPolygon::Index end1, float radius) const
{
	phPrimitive &primitive = poly.GetPrimitive();
	primitive.SetType(PRIM_TYPE_CAPSULE);

	phPrimCapsule &capsulePrim = primitive.GetCapsule();
	capsulePrim.SetEndIndicesAndRadius(end0, end1, radius);
}


inline void phBoundPolyhedron::InitBox(phPolygon &poly, phPolygon::Index vert0, phPolygon::Index vert1, phPolygon::Index vert2, phPolygon::Index vert3) const
{
	phPrimitive &primitive = poly.GetPrimitive();
	primitive.SetType(PRIM_TYPE_BOX);

	phPrimBox &boxPrim = primitive.GetBox();
	boxPrim.SetVertexIndices(vert0, vert1, vert2, vert3);
}


inline void phBoundPolyhedron::InitCylinder(phPolygon &poly, phPolygon::Index end0, phPolygon::Index end1, float radius) const
{
	// Just be sneaky and use capsules for now until I've checked in all of the code to handle cylinders.
#if 0
	phPrimitive &primitive = poly.GetPrimitive();
	primitive.SetType(PRIM_TYPE_CAPSULE);

	phPrimCapsule &capsulePrim = primitive.GetCapsule();
	capsulePrim.SetEndIndicesAndRadius(end0, end1, radius);
#else
	phPrimitive &primitive = poly.GetPrimitive();
	primitive.SetType(PRIM_TYPE_CYLINDER);

	phPrimCylinder &cylinderPrim = primitive.GetCylinder();
	cylinderPrim.SetEndIndicesAndRadius(end0, end1, radius);
#endif
}

inline int phBoundPolyhedron::DetectSegmentUndirected (const phPolygon &poly, Vec4V_In segA, Vec4V_In segB) const
{
	Vec3V v0(GetVertex(poly.GetVertexIndex(0)));
	Vec3V v1(GetVertex(poly.GetVertexIndex(1)));
	Vec3V v2(GetVertex(poly.GetVertexIndex(2)));
	return poly.DetectSegmentUndirected(v0, v1, v2, segA, segB);
}


inline int phBoundPolyhedron::DetectSegmentDirected (const phPolygon &poly, Vec3V_In segA, Vec3V_In segB) const
{
	Vec3V v0(GetVertex(poly.GetVertexIndex(0)));
	Vec3V v1(GetVertex(poly.GetVertexIndex(1)));
	Vec3V v2(GetVertex(poly.GetVertexIndex(2)));
	return poly.DetectSegmentDirected(v0, v1, v2, segA, segB);
}



#if COMPRESSED_VERTEX_METHOD > 0
inline void phBoundPolyhedron::InitQuantization(const phBoundPolyhedron *otherBound)
{
	SetBoundingBoxMin(otherBound->GetBoundingBoxMin());
	SetBoundingBoxMax(otherBound->GetBoundingBoxMax());
	CalculateQuantizationValues();
}

inline void phBoundPolyhedron::InitQuantization(Vec3V_In boundingBoxMin, Vec3V_In boundingBoxMax)
{
	SetBoundingBoxMin(boundingBoxMin);
	SetBoundingBoxMax(boundingBoxMax);
	CalculateQuantizationValues();
}
#endif



// Note, intended to process convex hulls, won't work correctly for concave polyhedra
FORCE_INLINE_SIMPLE_SUPPORT void phBoundPolyhedron::LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 v0, SupportPoint & sp) const
{
	Vec3V vec0(v0);

#if COMPRESSED_VERTEX_METHOD == 0
	const Vec3V* vertices = m_ShrunkVertices;
    if (m_ShrunkVertices == NULL)
    {
        vertices = m_Vertices;
    }
#else
	const CompressedVertexType *compressedVertices = m_CompressedShrunkVertices;
	if(m_CompressedShrunkVertices == NULL)
	{
		compressedVertices = m_CompressedVertices;
	}
#endif

#if OCTANT_MAP_SUPPORT_ACCEL

#if __SPU
# if __BANK
#  define USE_OCTANT_MAP sm_UseOctantMap
# else
#  define USE_OCTANT_MAP true
# endif
#else
# if __BANK
#  define USE_OCTANT_MAP sm_UseOctantMap
# else
#  define USE_OCTANT_MAP true
# endif
#endif

	Vec3V supportVec(v0);
	if (Likely(USE_OCTANT_MAP && HasOctantMap()))
	{
		FastAssert(m_OctantVertCounts);
		u32 axis;

		VecBoolV octantCode = IsLessThan(supportVec, Vec3V(V_ZERO));
		ResultToIndexZYX(axis, octantCode);

#if __WIN32PC
		Vec3V supVec;
		ScalarV newDot, maxDot;

		// 1st loop iteration has been brought outside the loop, to avoid a floating point compare & a couple branches.
#if COMPRESSED_VERTEX_METHOD == 0
		supVec = vertices[m_OctantVerts[axis][0]];
#else
		supVec = DecompressVertex(compressedVertices + 3 * m_OctantVerts[axis][0]);
#endif
		maxDot = Dot(vec0, supVec);
		int supVecIndex = 0;

		// Loop iterations 1 thru (m_NumConvexHullVertices-1).
		Vec3V curVec;
		for (OctantMapIndexType i = 1; i < m_OctantVertCounts[axis]; ++i)
		{
#if COMPRESSED_VERTEX_METHOD == 0
			curVec = vertices[i];
#else
			curVec = DecompressVertex(compressedVertices + 3 * m_OctantVerts[axis][i]);
#endif
			newDot = Dot(vec0, curVec);

			if (IsTrue(newDot > maxDot))
			{
				maxDot = newDot;
				supVec = curVec;
				supVecIndex = i;
			}
		}

		sp.m_vertex = supVec;
		sp.m_index = Vec::V4LoadScalar32IntoSplatted(m_OctantVerts[axis][supVecIndex]);
		return;
#else

#define USE_BRANCHLESS_GEOMETRY_BOUND_SUPPORT 1
#if USE_BRANCHLESS_GEOMETRY_BOUND_SUPPORT

		// Process the last vertex first
		const int numVertices = m_OctantVertCounts[axis] - 1;
#if COMPRESSED_VERTEX_METHOD == 0
		register Vec3V supVec = vertices[numVertices];
#else
		const OctantMapIndexType * vertexIndexList = m_OctantVerts[axis];
		const OctantMapIndexType * vertexIndexCur = vertexIndexList + numVertices;
		Vec4V supportVertIndexV(Vec::V4LoadScalar32IntoSplatted(*vertexIndexCur));
		register Vec3V supVec = DecompressVertex(compressedVertices + (3 * (*vertexIndexCur)));
#endif
		ScalarV maxDot = Dot(supVec, vec0);

		vertexIndexCur--;

#define REORDER_SUPPORT_SELECTS 1	// Reordering the selects gives slightly better performance.

#if REORDER_SUPPORT_SELECTS	

#define PROCESS_VERTEX_A1(c) \
	const Vec4V newSupportVertIndexV##c(Vec::V4LoadScalar32IntoSplatted(*vertexIndexCur)); \
	const Vec3V newSupVec##c(DecompressVertex(compressedVertices + 3 * (*vertexIndexCur))); \
	const ScalarV newDot##c = Dot(newSupVec##c, vec0); \
	--vertexIndexCur;

#define PROCESS_VERTEX_A2(c1,c2,c3) \
	const BoolV isGreater##c3 = IsGreaterThan(newDot##c2, newDot##c1); \
	const ScalarV newDot##c3 = SelectFT(isGreater##c3, newDot##c1, newDot##c2); \
	const Vec3V newSupVec##c3 = SelectFT(isGreater##c3, newSupVec##c1, newSupVec##c2); \
	const Vec4V newSupportVertIndexV##c3 = SelectFT(isGreater##c3, newSupportVertIndexV##c1, newSupportVertIndexV##c2);

#define PROCESS_VERTEX_A3(c) \
	const BoolV isGreaterFinal = IsGreaterThan(newDot##c, maxDot); \
	maxDot = SelectFT(isGreaterFinal, maxDot, newDot##c); \
	supVec = SelectFT(isGreaterFinal, supVec, newSupVec##c); \
	supportVertIndexV = SelectFT(isGreaterFinal, supportVertIndexV, newSupportVertIndexV##c);

/*
		// This doesn't seem to help much.
		const OctantMapIndexType * vertextIndexList8 = vertexIndexList + 7;
		while (vertexIndexCur >= vertextIndexList8)
		{
			PROCESS_VERTEX_A1(0);
			PROCESS_VERTEX_A1(1);
			PROCESS_VERTEX_A1(2);
			PROCESS_VERTEX_A1(3);
			PROCESS_VERTEX_A1(4);
			PROCESS_VERTEX_A1(5);
			PROCESS_VERTEX_A1(6);
			PROCESS_VERTEX_A1(7);

			PROCESS_VERTEX_A2(0,1,8);
			PROCESS_VERTEX_A2(2,3,9);
			PROCESS_VERTEX_A2(4,5,10);
			PROCESS_VERTEX_A2(6,7,11);

			PROCESS_VERTEX_A2(8,9,12);
			PROCESS_VERTEX_A2(10,11,13);

			PROCESS_VERTEX_A2(12,13,14);

			PROCESS_VERTEX_A3(14);
		}
*/
		const OctantMapIndexType * vertextIndexList4 = vertexIndexList + 3;
		while (vertexIndexCur >= vertextIndexList4)
		{
			PROCESS_VERTEX_A1(0);
			PROCESS_VERTEX_A1(1);
			PROCESS_VERTEX_A1(2);
			PROCESS_VERTEX_A1(3);

			PROCESS_VERTEX_A2(0,1,4);
			PROCESS_VERTEX_A2(2,3,5);
			
			PROCESS_VERTEX_A2(4,5,6);
			
			PROCESS_VERTEX_A3(6);
		}

		const OctantMapIndexType * vertextIndexList2 = vertexIndexList + 1;
		while (vertexIndexCur >= vertextIndexList2)
		{
			PROCESS_VERTEX_A1(0);
			PROCESS_VERTEX_A1(1);

			PROCESS_VERTEX_A2(0,1,2);
			
			PROCESS_VERTEX_A3(2);
		}

		if (vertexIndexCur == vertexIndexList)
		{
			PROCESS_VERTEX_A1(0);

			PROCESS_VERTEX_A3(0);
		}

#else // REORDER_SUPPORT_SELECTS

#undef PROCESS_VERTEX_A
#define PROCESS_VERTEX_A \
		{ \
		const Vec4V newSupportVertIndexV(Vec::V4LoadScalar32IntoSplatted(*vertexIndexCur)); \
		const Vec3V newSupVec(DecompressVertex(compressedVertices + 3 * (*vertexIndexCur))); \
		const ScalarV newDot = Dot(newSupVec, vec0); \
		const BoolV isGreater = IsGreaterThan(newDot, maxDot); \
		maxDot = SelectFT(isGreater, maxDot, newDot); \
		supVec = SelectFT(isGreater, supVec, newSupVec); \
		supportVertIndexV = SelectFT(isGreater, supportVertIndexV, newSupportVertIndexV); \
		--vertexIndexCur; \
		}
/*
		// This doesn't seem to help much.
		const OctantMapIndexType * vertextIndexList8 = vertexIndexList + 7;
		while (vertexIndexCur >= vertextIndexList8)
		{
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
		}
*/
		const OctantMapIndexType * vertextIndexList4 = vertexIndexList + 3;
		while (vertexIndexCur >= vertextIndexList4)
		{
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
		}

		const OctantMapIndexType * vertextIndexList2 = vertexIndexList + 1;
		while (vertexIndexCur >= vertextIndexList2)
		{
			PROCESS_VERTEX_A;
			PROCESS_VERTEX_A;
		}

		if (vertexIndexCur == vertexIndexList)
		{
			PROCESS_VERTEX_A;
		}

#endif // REORDER_SUPPORT_SELECTS
		FastAssert(vertexIndexCur == vertexIndexList - 1);

		sp.m_vertex = supVec;
		sp.m_index = supportVertIndexV.GetIntrin128ConstRef();
		return;

#else // USE_BRANCHLESS_GEOMETRY_BOUND_SUPPORT

		// Process the last vertex first
		OctantMapIndexType numVertices = m_OctantVertCounts[axis] - 1;
#if COMPRESSED_VERTEX_METHOD == 0
		register Vec3V supVec = vertices[numVertices];
#else
		register Vec3V supVec = DecompressVertex(compressedVertices + (3 * m_OctantVerts[axis][numVertices]));// vertices != NULL ? vertices[numVertices] : GetCompressedVertex(numVertices);
#endif
		int supportVert = numVertices;
		ScalarV maxDot = Dot(supVec, vec0);

#define PROCESS_VERTEX_A \
		{ \
			int curVertIndex = m_OctantVerts[axis][octantVertexIndex]; \
			Vec3V vertex(DecompressVertex(compressedVertices + 3 * curVertIndex)); \
			ScalarV newDot = Dot(vertex, vec0); \
			BoolV isGreater = IsGreaterThan(newDot, maxDot); \
			maxDot = SelectFT(isGreater, maxDot, newDot); \
			supVec = SelectFT(isGreater, supVec, vertex); \
			if (IsTrue(isGreater)) \
			{ \
				supportVert = curVertIndex; \
			} \
			++octantVertexIndex; \
		}

		int octantVertexIndex = 0;

		// Duff's device
		int i = 0;
		switch (numVertices % 4)
		{
		case 0:
		for (;i<numVertices;i += 4)
		{
			PROCESS_VERTEX_A;

		case 3:
			PROCESS_VERTEX_A;

		case 2:
			PROCESS_VERTEX_A;

		default:
			PROCESS_VERTEX_A;
		}
		}

		sp.m_vertex = supVec;
		sp.m_index = Vec::V4LoadScalar32IntoSplatted(m_OctantVerts[axis][supportVert]);
		return;

#endif // USE_BRANCHLESS_GEOMETRY_BOUND_SUPPORT

#endif
	}
#endif // OCTANT_MAP_SUPPORT_ACCEL

#if RSG_CPU_INTEL

    Vec3V supVec;
    ScalarV newDot, maxDot;

	// 1st loop iteration has been brought outside the loop, to avoid a floating point compare & a couple branches.
#if COMPRESSED_VERTEX_METHOD == 0
	supVec = vertices[0];
#else
	supVec = DecompressVertex(compressedVertices);
#endif
	maxDot = Dot(vec0 ,supVec);
	int supVecIndex = 0;

	// Loop iterations 1 thru (m_NumConvexHullVertices-1).
    Vec3V curVec;
    for (int i=1;i<m_NumConvexHullVertices;i++)
    {
#if COMPRESSED_VERTEX_METHOD == 0
        curVec = vertices[i];
#else
		curVec = DecompressVertex(compressedVertices + 3 * i);
#endif
		newDot = Dot(vec0,curVec);

        if (IsGreaterThanAll(newDot, maxDot))
        {
            maxDot = newDot;
            supVec = curVec;
			supVecIndex = i;
        }
    }

    sp.m_vertex = supVec;
	sp.m_index = Vec::V4LoadScalar32IntoSplatted(supVecIndex);
	return;

#else // !__WIN32PC

    // Process the last vertex first
    int numVertices = m_NumConvexHullVertices - 1;
#if COMPRESSED_VERTEX_METHOD == 0
	register Vec3V supVec = vertices[numVertices];
#else
	register Vec3V supVec = DecompressVertex(compressedVertices + (3 * numVertices));// vertices != NULL ? vertices[numVertices] : GetCompressedVertex(numVertices);
#endif
	Vec4V supportVertIndexV(Vec::V4LoadScalar32IntoSplatted(numVertices)); 
    ScalarV maxDot = Dot(supVec, vec0);
	int curVertIndex = 0;

#if !SUPPORT_FUNCTION_BATCH_DECOMPRESSION || __SPU
#if COMPRESSED_VERTEX_METHOD == 0
#define PROCESS_VERTEX \
    { \
		const Vec3V vertex(*vPtr); \
		const Vec4V newSupportVertIndexV(Vec::V4LoadScalar32IntoSplatted(curVertIndex)); \
		const ScalarV newDot = Dot(vertex, vec0); \
		const BoolV isGreater = IsGreaterThan(newDot, maxDot); \
		maxDot = SelectFT(isGreater, maxDot, newDot); \
		supVec = SelectFT(isGreater ,supVec, vertex); \
		supportVertIndexV = SelectFT(isGreater, supportVertIndexV, newSupportVertIndexV); \
		++vPtr; \
		++curVertIndex; \
    }

	const Vec3V* vPtr = vertices;
#else
#define PROCESS_VERTEX \
	{ \
		const Vec3V vertex(DecompressVertex(vPtr)); \
		const Vec4V newSupportVertIndexV(Vec::V4LoadScalar32IntoSplatted(curVertIndex)); \
		const ScalarV newDot = Dot(vertex, vec0); \
		const BoolV isGreater = IsGreaterThan(newDot, maxDot); \
		maxDot = SelectFT(isGreater, maxDot, newDot); \
		supVec = SelectFT(isGreater ,supVec, vertex); \
		supportVertIndexV = SelectFT(isGreater, supportVertIndexV, newSupportVertIndexV); \
		vPtr += 3; \
		++curVertIndex; \
	}

	const CompressedVertexType *vPtr = compressedVertices;
#endif

#if 0
    // Simple loop
    for (int i = 0;i<numVertices;++i)
    {
        PROCESS_VERTEX;
    }
#elif 0
    // Traditional unrolling
    int i;
	for (i = numVertices;i>=8;i -= 8)
	{
        PROCESS_VERTEX;
        PROCESS_VERTEX;
        PROCESS_VERTEX;
        PROCESS_VERTEX;
        PROCESS_VERTEX;
        PROCESS_VERTEX;
        PROCESS_VERTEX;
        PROCESS_VERTEX;
    }

    switch (i)
    {
    case 7:
        PROCESS_VERTEX;

    case 6:
        PROCESS_VERTEX;

    case 5:
        PROCESS_VERTEX;

    case 4:
        PROCESS_VERTEX;

    case 3:
        PROCESS_VERTEX;

    case 2:
        PROCESS_VERTEX;

    case 1:
        PROCESS_VERTEX;

    default:
        ;
    }
#else
    // Duff's device
    int i = 0;
    switch (numVertices % 4)
    {
    case 0:
	for (;i<numVertices;i += 4)
	{
        PROCESS_VERTEX;

    case 3:
        PROCESS_VERTEX;

    case 2:
        PROCESS_VERTEX;

    default:
        PROCESS_VERTEX;
    }
    }
#endif
#else

#define PROCESS_VERTEX2 \
	{ \
		_ivector4 ixyz = __vupkhsh( _hvector4(result) ); \
		const Vec3V vertex = Vec3V(__vmaddfp(__vcsxwfp(ixyz, 0), m_UnQuantizeFactor.GetIntrin128ConstRef(), m_BoundingBoxCenter.GetIntrin128ConstRef())); \
		const Vec4V newSupportVertIndexV(Vec::V4LoadScalar32IntoSplatted(curVertIndex)); \
		const ScalarV newDot = Dot(vertex, vec0); \
		const BoolV isGreater = IsGreaterThan(newDot, maxDot); \
		maxDot = SelectFT(isGreater, maxDot, newDot); \
		supVec = SelectFT(isGreater, supVec, vertex); \
		supportVertIndexV = SelectFT(isGreater, supportVertIndexV, newSupportVertIndexV); \
	}

	Assert(((int)compressedVertices & 15) == 0);
	int i;
	__vector4 result;
	const CompressedVertexType *cvPtr = compressedVertices;
	for (i = numVertices; i >= 8; i -= 8)
	{
		// Vertex 0:
		result = __vector4(__lvlx(cvPtr, 0));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 1:
		result = __vector4(__lvlx(cvPtr + 3, 0));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 2:
		result = __vor(__vector4(__lvlx(cvPtr + 6, 0)), __vector4(__lvrx(cvPtr + 6 + 8, 0)));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 3:
		result = __vector4(__lvlx(cvPtr + 9, 0));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 4:
		result = __vector4(__lvlx(cvPtr + 12, 0));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 5:
		result = __vector4(__vor(__vector4(__lvlx(cvPtr + 15, 0)), __vector4(__lvrx(cvPtr + 15 + 8, 0))));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 6:
		result = __vector4(__lvlx(cvPtr + 18, 0));
		PROCESS_VERTEX2;
		++curVertIndex;

		// Vertex 7:
		result = __vector4(__lvlx(cvPtr + 21, 0));
		PROCESS_VERTEX2;
		++curVertIndex;

		cvPtr += 24;
	}

	curVertIndex = numVertices - 1;
	switch (i)
	{
	case 7:
		result = __vector4(__lvlx(cvPtr + 18, 0));
		PROCESS_VERTEX2;
		--curVertIndex;

	case 6:
		result = __vor(__vector4(__lvlx(cvPtr + 15, 0)), __vector4(__lvrx(cvPtr + 15 + 8, 0)));
		PROCESS_VERTEX2;
		--curVertIndex;

	case 5:
		result = __vector4(__lvlx(cvPtr + 12, 0));
		PROCESS_VERTEX2;
		--curVertIndex;

	case 4:
		result = __vector4(__lvlx(cvPtr + 9, 0));
		PROCESS_VERTEX2;
		--curVertIndex;

	case 3:
		result = __vor(__vector4(__lvlx(cvPtr + 6, 0)), __vector4(__lvrx(cvPtr + 6 + 8, 0)));
		PROCESS_VERTEX2;
		--curVertIndex;

	case 2:
		result = __vector4(__lvlx(cvPtr + 3, 0));
		PROCESS_VERTEX2;
		--curVertIndex;

	case 1:
		result = __vector4(__lvlx(cvPtr, 0));
		PROCESS_VERTEX2;
		--curVertIndex;

	default:
		;
	}
#endif

	sp.m_vertex = supVec;
	sp.m_index = supportVertIndexV.GetIntrin128ConstRef();
	return;
#endif
}

#if COMPRESSED_VERTEX_METHOD > 0
inline void phBoundPolyhedron::CalculateQuantizationValues ()
{
	{
#if COMPRESSED_VERTEX_METHOD == 2

		// Get the half-width and center of the bounding box.
		Vec3V halfWidth,center;
		GetBoundingBoxHalfWidthAndCenter(halfWidth,center);
		m_BoundingBoxCenter = center;

		// Set the unquantization factor as the half-width over 32767.
		m_UnQuantizeFactor = Scale(halfWidth,Vec3V(Vec::V4VConstant<0x38000100,0x38000100,0x38000100,0x38000100>()));
//		m_QuantizeFactor.Invert(m_UnQuantizeFactor);
#endif
	}
}
#endif


__forceinline Vec3V_Out phBoundPolyhedron::GetPolygonUnitNormal (int polygonIndex) const
{
	const phPolygon &curPoly = GetPolygon(polygonIndex);
	return curPoly.ComputeUnitNormal(GetVertex(curPoly.GetVertexIndex(0)),GetVertex(curPoly.GetVertexIndex(1)),GetVertex(curPoly.GetVertexIndex(2)));
}


__forceinline Vec3V_Out phBoundPolyhedron::GetPolygonNonUnitNormal (int polygonIndex) const
{
	const phPolygon &curPoly = GetPolygon(polygonIndex);
	return curPoly.ComputeNonUnitNormal(GetVertex(curPoly.GetVertexIndex(0)),GetVertex(curPoly.GetVertexIndex(1)),GetVertex(curPoly.GetVertexIndex(2)));
}


inline Vec3V_Out phBoundPolyhedron::GetVertex (int i) const
{
	FastAssert(i >= 0);
	FastAssert(i < m_NumVertices);
	register Vec3V retVal;
#if COMPRESSED_VERTEX_METHOD == 0
		retVal = m_Vertices[i];
#endif
#if COMPRESSED_VERTEX_METHOD > 0
		retVal = GetCompressedVertex(i);
#endif
	return retVal;
}

inline int phBoundPolyhedron::GetNumVertices () const
{
	return m_NumVertices;
}

inline int phBoundPolyhedron::GetNumPolygons () const
{
	return m_NumPolygons;
}

inline int phBoundPolyhedron::GetNumConvexHullVertices() const
{
	return m_NumConvexHullVertices;
}

inline void phBoundPolyhedron::ClearPolysAndVerts()
{
	m_NumVertices = 1;
	m_NumPolygons = 0;
	m_NumConvexHullVertices = 1;

#if OCTANT_MAP_SUPPORT_ACCEL
	m_OctantVertCounts = NULL;
#endif
}


#if COMPRESSED_VERTEX_METHOD == 0
inline const Vec3V* phBoundPolyhedron::GetVertexPointer () const
{
	return m_Vertices;
}
#endif

inline const phPolygon* phBoundPolyhedron::GetPolygonPointer () const
{
	return m_Polygons;
}

inline void phBoundPolyhedron::SetIsFlat (bool isFlat)
{
	m_IsFlat = isFlat;
}


} // namespace rage

#endif
