//
// phbound/boundgeom.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDGEOM_H
#define PHBOUND_BOUNDGEOM_H

/////////////////////////////////////////////////////////////////
// external defines

#include "boundpolyhedron.h"
#if __SPU
#include "system/dma.h"
#include <cell/dma.h>
#endif // __SPU

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
#define SECOND_SURFACE_MAX_ALLOWED_DISPLACEMENT (1.0f)
#endif

namespace rage {


/////////////////////////////////////////////////////////////////
// phBoundGeometry

/*
PURPOSE
	A class to represent a physics bound with generalized vertex locations and edge/polygon topology.  These are represented internally by a 'polygon soup', 
	that is, an arbitrary set of polygons without any particular topology to them.  In practice, there shouldn't be any 'stray' polygons (polygons that 
	don't have neighbors on all their sides), at least where the bound is collidable (ie, a pyramid that is going to be submerged in the ground needn't have 
	a bottom).
*/
class phBoundGeometry : public phBoundPolyhedron
{
public:
	phBoundGeometry ();
	PH_NON_SPU_VIRTUAL ~phBoundGeometry ();

#if __RESOURCECOMPILER
	static const char*& GetBoundNameAssertContextStr();
#endif // __RESOURCECOMPILER

	////////////////////////////////////////////////////////////
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		void Init (int numverts, int numvertattribs, int nummaterials, int nummaterialcolors, int numpolys, int secondSurface, bool polysInTempMem = false);
	#else
		void Init (int numverts, int numvertattribs, int nummaterials, int numpolys, int secondSurface, bool polysInTempMem = false);
	#endif
#else //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		void Init (int numverts, int numvertattribs, int nummaterials, int nummaterialcolors, int numpolys, bool polysInTempMem = false);
	#else
		void Init (int numverts, int numvertattribs, int nummaterials, int numpolys, bool polysInTempMem = false);
	#endif
#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA...

#if !__SPU
#if !__FINAL// && !IS_CONSOLE
	bool Save (const char *) const;
	bool Save (fiAsciiTokenizer & token) const;
	/////////////////////////////////////////////////////////////

	void CombineBounds (const phBoundGeometry* const bnds[],int numBnds);
#endif

	bool SplitQuad012 (phPolygon::Index vertIndex0, phPolygon::Index vertIndex1, phPolygon::Index vertIndex2, phPolygon::Index vertIndex3) const;

	// PURPOSE: Weld nearby vertices.
	// PARAMS:
	//	epsilon - the maximum distance between vertices to be considered at the same location and get welded together
	// NOTES:
	//   Welding vertices together after computing the convex hull will result in the convex hull being reset to include all of the vertices.  Compute the
	//    convex hull *after* welding nearby vertices together if you plan to do both.
	bool WeldVertices (float epsilon, int* polygonRemap = NULL);

	virtual bool PostLoadCompute ();

	/////////////////////////////////////////////////////////////
	// geometry calculations
	void RemoveUnusedVerts ();												// scan for unused vertexes and remove
	void RemoveDegeneratePolys (float tolerance = 0.9f);					// scan for degenerate polys and remove them (doesn't remove orphaned edges or verts)
	void CalculatePolyNormals ();
	void ComputeOctantMap ();
	void RecomputeOctantMap ();

	// PURPOSE: Calculate the bounding box and bounding sphere.
	// NOTES:	This is a public version of the protected virtual phBound::CalculateExtents for geometry bounds.
	void CalculateGeomExtents ();

	// PURPOSE: Calculate the bounding box of this geometry bound based on supplied vertex arrays
	// PARAMS:
	//   vertices - Array of non-shrunken vertices. (These will not have margin added to them)
	//   shrunkenVertices - Array of shrunken vertices. (These will have the margin added to them)
	// NOTES:
	//   Not supplying either vertex array will result in a margin sized bounding box
	//   The arrays MUST be the same size as this geometry bound's arrays
	void CalculateBoundingBox(const Vector3 *vertices, const Vector3 *shrunkenVertices);

	virtual void Copy (const phBound* original);

	// <COMBINE phBound::CanBecomeActive>
	virtual bool CanBecomeActive () const;

#endif // !__SPU

	// <COMBINE phBound::FindElementIndex>
	int FindElementIndex (Vector3::Param localNormal, Vector3::Param localContact, int* vertexIndices, int numVertices) const;

#if __BANK
	// <COMBINE phBound::FindElementIndex>
	int FindElementIndexOld (Vector3::Param localNormal, int* vertexIndices, int numVertices) const;
#endif

	// <COMBINE phBound::GetMaterialIdFromPartIndex>
	phMaterialMgr::Id GetMaterialIdFromPartIndex (int partIndex) const;

	////////////////////////////////////////////////////////////
	// accessors
	PH_NON_SPU_VIRTUAL int GetNumMaterials () const								{ return m_NumMaterials; }

#if POLY_MAX_VERTICES==3
	const u8 *GetPolygonMaterialIndexArray() const
	{
		return m_PolyMatIndexList;
	}
#endif

	const phMaterialMgr::Id *GetMaterialIdArray() const	{ return m_MaterialIds; }

#if !__SPU
	virtual const phMaterial& GetMaterial (phMaterialIndex i) const;
	virtual void GetMaterialName (phMaterialIndex i, char* name, int size) const;
	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex index) const
	{
#if __WIN32PC
		// temp hack to get around bad data that's been checked in (BS#1013358), this was crashing the heightmap/navmesh tools
		if (index >= m_NumMaterials)
		{
			return m_MaterialIds[0];
		}
#endif
		TrapLT(index,0);
		TrapGE(index,m_NumMaterials);
		return m_MaterialIds[index];
	}
	void SetMaterialIdEntry (int index, phMaterialMgr::Id id);
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	int GetNumMaterialColors() const									{ return m_NumMaterialColors;  }

	// direct array access to m_MaterialColors[]:
	Color32 GetMaterialColorDirectly(int i) const						{ FastAssert((i>=0) && (i<m_NumMaterialColors));	return Color32(m_MaterialColors[i]);	}

	// matColorId access to m_MaterialColors[]:
	// Note: matColorId=0 is default/invalid color, so valid colors start from matColorId=1:
	Color32 GetMaterialColor(int i) const								{ FastAssert((i>=1) && (i<m_NumMaterialColors+1));	return Color32(m_MaterialColors[i-1]);	}
#endif

	// <COMBINE phBoundPolyhedron::SetPolygonMaterialIndex>
	virtual void SetPolygonMaterialIndex (int polygonIndex, phMaterialIndex materialIndex);

	// <COMBINE phBoundPolyhedron::GetPolygonMaterialIndex>
	virtual phMaterialIndex GetPolygonMaterialIndex (int polygonIndex) const 
	{
		TrapLT(polygonIndex,0);
		TrapGE(polygonIndex,m_NumPolygons);
		return m_PolyMatIndexList[polygonIndex];
	}

	void SetMaterialId (phMaterialIndex index, phMaterialMgr::Id materialId) { m_MaterialIds[index] = materialId; }
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex materialIndex=-1);									// set the material for the entire bound
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	void SetMaterialColor(int i, Color32 c)									{ FastAssert(i<m_NumMaterialColors); m_MaterialColors[i] = c.GetColor(); }
#endif

	void SwapZeroAndFirstMaterials();

#if COMPRESSED_VERTEX_METHOD == 0
	void Transform (const Matrix34 & mtx);									// transform the geometry by mtx
#endif

	// PURPOSE: Change the size of the bound by scaling all of the vertex locations.
	// PARAMS:
	//	xScale - scale factor for the x direction
	//	yScale - scale factor for the y direction
	//	zScale - scale factor for the z direction
	virtual void ScaleSize (float xScale, float yScale, float zScale);

	// PURPOSE: Change the size of the bound by scaling all of the vertex locations.
	// PARAMS:
	//	xyzScale - scale factor for the the bound
	void ScaleSize (float xyzScale);

	// PURPOSE: Change the size of the bound by scaling all of the vertex locations.
	// PARAMS:
	//	scale - scale factors for all three directions
	void ScaleSize (const Vector3& scale);
#else // !__SPU

	// Post gta4 - Andrzej moved this into the header for SPU only compilation
	// to get his plant manager working on SPU
	// Want to keep somehow, move to rage/dev - need to ask Eugene?
	phMaterialIndex GetPolygonMaterialIndex (int polygonIndex) const
	{
		TrapLT(polygonIndex,0);
		TrapGE(polygonIndex,m_NumPolygons);
		return m_PolyMatIndexList[polygonIndex];
	}

	phMaterialMgr::Id GetMaterialId (int index) const
	{
		TrapLT(index,0);
		TrapGE(index,m_NumMaterials);
		return m_MaterialIds[index];
	}

	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		// direct array access to m_MaterialColors[]:
		Color32 GetMaterialColorDirectly(int i) const						{ FastAssert((i>=0) && (i<m_NumMaterialColors));	return Color32(m_MaterialColors[i]);	}

		// matColorId access to m_MaterialColors[]:
		// Note: matColorId=0 is default/invalid color, so valid colors start from matColorId=1:
		Color32 GetMaterialColor(int i) const								{ FastAssert((i>=1) && (i<m_NumMaterialColors+1));	return Color32(m_MaterialColors[i-1]);	}
	#endif
#endif //__SPU...

	void SetCentroidOffset (Vec3V_In offset);								// set the centroid to be at offset
	void ShiftCentroidOffset (Vec3V_In offset);								// move the centroid (and verts) by offset
    void SetMarginAndShrink (const float margin, const float polyOrVert = 0.0f);

	////////////////////////////////////////////////////////////
	// utility functions
	bool OverlapRegion (const Vector3 *perimeter, const int nperimverts, float cosTheta, float sinTheta);
																			// does the geometry overlap the polyline region?

	////////////////////////////////////////////////////////////
	// utility
	#if RSG_TOOL
	void Copy (phBoundGeometry * boundGeom);								// copy from another bound geometry
	// Had commented out the RSG_TOOL check for gta4 because it's used by CSmashObject at runtime
	#endif
#if HACK_GTA4 || RSG_TOOL
	void DecreaseNumPolys (int number);										// change the number of polygons w/o reallocation
#endif // HACK_GTA4 || RSG_TOOL

#if !__SPU
	////////////////////////////////////////////////////////////
	// resources
	phBoundGeometry (datResource & rsc);									// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	////////////////////////////////////////////////////////////
	// load / save
#if RSG_TOOL || __RESOURCECOMPILER
	virtual bool Load_v110 (fiAsciiTokenizer & token);								// load, ascii, v1.10
#endif

	//Statics for preload
#if RSG_TOOL || __RESOURCECOMPILER
	// These are really really bad.  Put into a class and pass that through the LoadGeomStats class
	static Vector3 sm_CG;
	static int sm_NumVertices;
	static int sm_NumPerVertexAttribs;
	static int sm_NumEdges;													// We need to keep this around to support the loading of old files.
	static int sm_NumMaterialsToLoad;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	static int sm_NumMaterialColorsToLoad;
#endif
	static int sm_NumPolygons;
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	static int sm_SecondSurface;
	static float sm_SecondSurfaceMaxHeight;
#endif
    static float sm_Margin;
	static bool sm_LoadedCG;
	static bool sm_PreCheckedSizes;
    static bool sm_LoadedMargin;

	static bool CheckIsStreamable(fiAsciiTokenizer & token);				// check array sizes (post allocation)
#endif //RSG_TOOL || __RESOURCECOMPILER

	// PURPOSE: Any quads found in assets are split into pairs of triangles during loading. When this is true, the pair chosen will be convex or flat.
	//			When this is false, the pair chosen will be concave. For flat quads, the pair chosen will be arbitrary.
	static bool sm_SplitQuadsBendingOut;

	virtual bool ComponentsStreamed() const {return false;};
	void CopyHeader (const phBoundGeometry* original);
#endif	// !__SPU

#if !__SPU
protected:
#endif
	////////////////////////////////////////////////////////////
	// geometry data
	phMaterialMgr::Id*	m_MaterialIds;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	u32*				m_MaterialColors;
#endif

#if __SPU
protected:
#endif
	u8 pad1[4];

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA

	//Array of deformations that describe a second bound surface that is formed by moving 
	//every vertex along the z-direction by the amount specified in the array.
	//If the array ptr is null then no second surface data has been loaded.  
	//For non-null array ptrs then we will have one motion value for each vertex.
	//The position of the ith vertex on the second surface is
	//		GetVertex(i) - (m_SecondSurfaceVertexDisplacements ? Vector3(0,0,m_SecondSurfaceVertexDisplacements[i]) : 0);

private:

	//The raw data.
	float* m_SecondSurfaceVertexDisplacements;
	int m_NumSecondSurfaceVertexDisplacements;//Need this for DeclareStruct, must be equal to m_NumVertices or zero.

public:

	//Set the motion in the z-direction of the ith vertex that will move the vertex to the second surface.
	void SetSecondSurfaceVertexDisplacement(const int iVertexIndex, const float fSSDisplacement) const
	{
		Assert(m_SecondSurfaceVertexDisplacements);
		m_SecondSurfaceVertexDisplacements[iVertexIndex] = fSSDisplacement;
	}

	//Get the motion in the z-direction of the ith vertex that will move the vertex to the second surface.
	float GetSecondSurfaceVertexDisplacement(const int iVertexIndex) const 
	{
		if(m_SecondSurfaceVertexDisplacements)
		{
			Assert(iVertexIndex<m_NumVertices);
			Assert(m_NumVertices==m_NumSecondSurfaceVertexDisplacements);
			const float f=m_SecondSurfaceVertexDisplacements[iVertexIndex];
			return f;
		}
		else
		{
			return 0;
		}
	}

	//Return the array of displacements.
	const float* GetSecondSurfaceVertDisplacements() const
	{
		return m_SecondSurfaceVertexDisplacements;
	}

	//Test if the data for the second surface exists.
	bool GetHasSecondSurface() const
	{
#if RSG_TOOL
		if(NULL==m_SecondSurfaceVertexDisplacements)
		{
			return false;
		}

		for( int i=0; i<m_NumSecondSurfaceVertexDisplacements; i++ )
		{
			if ( ( m_SecondSurfaceVertexDisplacements[i] > 0.0f ) && ( m_SecondSurfaceVertexDisplacements[i] <= 1.0f ) )
				return true;
		}
		return false;
#else
		return (m_SecondSurfaceVertexDisplacements!=NULL);
#endif
	}

	//Get an interpolated vert position somewhere between the principal surface (0) and the second surface (1)
	void ComputeSecondSurfacePosition(const int iVertexIndex, const float fInterp, Vector3& vVertexPosSecondSurface) const
	{
		Assert(m_SecondSurfaceVertexDisplacements);
		Assert(iVertexIndex>=0.0f);
		Assert(iVertexIndex<m_NumVertices);
		Assert(fInterp>=0.0f);
		Assert(fInterp<=1.0f);
		vVertexPosSecondSurface= VEC3V_TO_VECTOR3(GetVertex(iVertexIndex));
		vVertexPosSecondSurface.z-=fInterp*GetSecondSurfaceVertexDisplacement(iVertexIndex);
	}

#endif

public:

	// PURPOSE: list of index numbers into this bound's list of material ids, one for each polygon
	u8* m_PolyMatIndexList;

	// PURPOSE: Number of materials that exist on this bound.
	u8 m_NumMaterials;
#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
	u8 m_NumMaterialColors;
#endif
	// Made CalculateExtents public for gta4 because we needed to call it after dynamically modifying poly bounds
	// E.g. deformation to vehicle bounds, or ensuring ground clearance for motorbike bounds
	// Don't see any reason we couldn't move that across to rage\dev
protected:

	////////////////////////////////////////////////////////////
	// helper functions

	// PURPOSE: Calculate the bounding box and bounding sphere.
	// NOTES:	CalculateGeomExtents() is a public version of this method for geometry bounds.
	PH_NON_SPU_VIRTUAL void CalculateExtents ();

	void ChangeBvhExtentsFromEmbeddedPrim (int vertexIndex, float radius);

protected:

    void ShrinkVerticesByMargin (float margin, Vector3* shrunkVertices);
    void ShrinkPolysByMargin (float margin, Vector3* shrunkVertices);
    void ShrinkPolysOrVertsByMargin (float margin, float polyOrVert, Vector3* shrunkVertices);

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		u8 pad[6+8];
	#else
		u8 pad[11+8];
	#endif
#else
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		u8 pad[14+8];
	#else
		u8 pad[3+8];
	#endif
#endif

#if RSG_TOOL || __RESOURCECOMPILER
	static void LoadGeomStats_v110 (fiAsciiTokenizer & token);					// load, ascii, v1.10: Only counts
#if !__FINAL
	bool Save_v110 (fiAsciiTokenizer & token);								// save, ascii, v1.10
#endif
#endif //RSG_TOOL || __RESOURCECOMPILER

#if !__SPU
	// Added post gta4 - Want to keep and move to rage\dev
	// Improves volume calculation on geometry bounds with open edges
protected:
	PH_NON_SPU_VIRTUAL bool HasOpenEdges() const;
#endif
};
#if RSG_CPU_X64 && !RSG_TOOL
CompileTimeAssert(sizeof(phBoundGeometry)==304);	// double check padding and size
#endif

#if __BANK
inline int phBoundGeometry::FindElementIndexOld (Vector3::Param localNormal, int* vertexIndices, int numVertices) const
{
	// If we don't have any materials applied, don't bother to compute the element index
	if (m_NumMaterials <= 1)
	{
		return 0;
	}

	const phPolygon* polygons = m_Polygons;

	switch (numVertices)
	{
	case 4:
	case 3:
		{
			for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
			{
				if (polygons[polygonIndex].ContainsVertices(vertexIndices,numVertices)>=3)
				{
					// This polygon contains all 3 or 4 of the given vertices, so it must be the right polygon.
					return polygonIndex;
				}
			}
		}

		// Fall through...basically just discard which ever point was last and try again with the first two verts

	case 2:
		{
			for (int polygonIndex=0; polygonIndex<m_NumPolygons; polygonIndex++)
			{
				const phPolygon& polygon = polygons[polygonIndex];
				if (polygon.ContainsVertices(vertexIndices,2)==2)
				{
					// This polygon contains both of the given vertices.
					int neighborIndex = polygon.FindNeighborWithVertices(vertexIndices[0],vertexIndices[1]);
					if (neighborIndex!=(phPolygon::Index)BAD_INDEX)
					{
						// This polygon and its neighbor contain both the given vertices. Return the index for either this polygon or its neighbor, whichever has a normal
						// vector closest to the given collision normal.

						Vector3 polyNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polygonIndex)));
						Vector3 neighborNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex)));

						return (Vector3(localNormal).DotV(polyNormal).IsGreaterThan(Vector3(localNormal).DotV(neighborNormal)) ? polygonIndex : neighborIndex);
					}
					else
					{
						// This polygon contains both the given vertices, but either they are not neighbors or there is no neighboring polygon that shares them.
						return polygonIndex;
					}
				}
			}
		}

		// Fall through...basically just pick a polygon that the first vertex was involved in

	case 1:
		{
			for (phPolygon::Index polygonIndex=0; polygonIndex<(phPolygon::Index)m_NumPolygons; polygonIndex++)
			{
				const phPolygon& polygon = polygons[polygonIndex];
				if (polygon.ContainsVertices(vertexIndices,1))
				{
					Vector3 polyNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polygonIndex)));

					// This polygon contains the given vertex.
					Vector3 bestNormDotNorm = Vector3(localNormal).DotV(polyNormal);
					int bestPolyIndex = polygonIndex;
					phPolygon::Index neighborIndex = polygon.FindNeighborWithVertex(vertexIndices[0]);
					int prevNeighborIndex = polygonIndex;
					while (neighborIndex!=polygonIndex && neighborIndex != (phPolygon::Index)BAD_INDEX)
					{
						const phPolygon& neighbor = polygons[neighborIndex];
						Vector3 neighborNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(neighborIndex)));
						Vector3 normDotNorm = Vector3(localNormal).DotV(neighborNormal);
						if (normDotNorm.IsGreaterThan(bestNormDotNorm))
						{
							bestNormDotNorm = normDotNorm;
							bestPolyIndex = neighborIndex;
						}

						phPolygon::Index nextNeighborIndex = neighbor.FindNeighborWithVertex(vertexIndices[0],prevNeighborIndex);
						prevNeighborIndex = neighborIndex;
						neighborIndex = nextNeighborIndex;
					}

					return bestPolyIndex;
				}
			}
		}
	}

	return 0;
}
#endif //__BANK

inline phMaterialMgr::Id phBoundGeometry::GetMaterialIdFromPartIndex (int partIndex) const
{
	// TODO: Change this to phBoundGeometry::GetMaterialId(phBoundGeometry::GetMaterialIndex(partIndex)) once we're sure this 
	//       assert never fires. 
	int materialId;
	if (Verifyf(partIndex<m_NumPolygons && partIndex>=0,"Invalid part index %i. Num Polys = %i",partIndex,GetNumPolygons()))
	{
#if __SPU
		// The bound stores a list of material index numbers with the same length as the list of polygons.
		materialId = sysDmaGetUInt8(uint64_t(&m_PolyMatIndexList[partIndex]), DMA_TAG(11));
#else
		// The bound stores a list of material index numbers with the same length as the list of polygons.
		materialId = m_PolyMatIndexList[partIndex];
#endif
	}
	else
	{
		// This bound has no vert-edge material, so use its first material.
		materialId = 0;
	}

	Assertf(materialId<m_NumMaterials,"requested material index is %i and there are %i materials",materialId,m_NumMaterials);
	//	return MATERIALMGR.GetMaterial(m_MaterialIds[materialId]);
	if(materialId<m_NumMaterials)
		return m_MaterialIds[materialId];
	else
		return phMaterialMgr::DEFAULT_MATERIAL_ID;
}
#if __BANK
extern bool g_UseBBoxForPolyFinding;
#endif //__BANK

} // namespace rage

#endif
