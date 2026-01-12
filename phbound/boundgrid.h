//
// phbound/boundgrid.h
//
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDGRID_H
#define PHBOUND_BOUNDGRID_H


/////////////////////////////////////////////////////////////////
// external defines

#include "boundbvh.h"

#include "atl/delegate.h"
#include "system/memory.h"			// usemembucket

namespace rage {

#if USE_GRIDS

/////////////////////////////////////////////////////////////////
// phBoundGrid
//
// A phBoundGrid is a bound that holds
// multiple phBoundOctree's in a 2D grid 
// of cells.  This bound is useful for 
// holding large complicated fixed worlds.
// <FLAG Component>

// typedef so that clients 
//	typedef phBoundGrid<phBoundOctree> phBoundOTGrid;

class phBoundGrid : public phBound
{
public:
	phBoundGrid ();														// constructor
	~phBoundGrid ();														// destructor

	phBoundGrid (datResource & rsc);										// construct in resource

#if !__SPU

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

#endif // !__SPU

	////////////////////////////////////////////////////////////
	// initialization
    template <class GridCellType>
#if !__SPU
	void InitGrid (float cellDim, int minFirstAxis, int maxFirstAxis, int minSecondAxis, int maxSecondAxis);
#endif	// !__SPU

	// create the grid
	void DeleteGrid ();												// destroy the grid
	void CalculateExtents ();												// calculate the box/sphere extents

	////////////////////////////////////////////////////////////
	// accessors
	int GetMinFirstAxis () const													{ return m_MinFirstAxis; }
	int GetMaxFirstAxis () const													{ return m_MaxFirstAxis; }
	int GetMinSecondAxis () const													{ return m_MinSecondAxis; }
	int GetMaxSecondAxis () const													{ return m_MaxSecondAxis; }

	// PURPOSE: Get the cell size.
	// RETURN: the cell size
	float GetCellSize () const;

	// PURPOSE: Get the inverse of the cell size.
	// RETURN: the inverse of the cell size
	float GetInverseCellSize () const;

#if !__SPU
	bool CullSpherePolys (const Vector3& center, float radius, const phBoundBVH** culledOctrees, int* polyOffsets, int* vertexOffsets, phBoundCuller& culler) const;
#endif // !__SPU

	////////////////////////////////////////////////////////////
	// construction
#if __TOOL
	// PURPOSE: Create the octrees from the geometry bounds. This should only be called after calling AddGeometry.
	// RETURN: true if all of the octrees in the grid were successfully initialized, false if any were not
	// NOTES:	AddGeometry and AddGeometryToCell will cause an assert failure if they are called after calling InitOctrees.
	bool InitOctrees (void);

	// PURPOSE: Put the given geometry bound in the octree grid.
	// PARAMS:
	//	boundGeom - the geometry bound to put in the octree grid
	void AddGeometry (phBoundGeometry * boundGeom);

	// PURPOSE: Put the given geometry bound in the specified octree in the grid.
	// PARAMS:
	//	boundGeom - the geometry bound to put in the octree
	//	cellIndex - the index number of the octree into which to put the geometry bound
	void AddGeometryToCell (phBoundGeometry * geom, int cellIndex);

	// PURPOSE: Put the specified polygon from the given geometry bound in the specified octree in the grid.
	// PARAMS:
	//	geom - the geometry bound to put in the octree
	//	pIndex - the index number of the polygon to put in the octree
	//	cIndex - the index number of the octree into which to put the polygon from the geometry bound
	//	vertexMap - the list of vertices in the geometry bound
	//	materialMap - the list of materials in the geometry bound
	void AddPolygonToCell (phBoundGeometry * geom, int pIndex, int cIndex, phPolygon::Index * vertexMap, phMaterialMgr::Id * materialMap);
#endif

	const phMaterial & GetMaterial (phMaterialIndex ) const;
	phMaterialMgr::Id GetMaterialId (phMaterialIndex ) const;

	// <COMBINE phBound::GetMaterialIdFromPartIndex>
	phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent (int partIndex, int component) const;

#if !__SPU
	virtual bool IsPolygonal (int component) const;
#endif

	phBoundBVH* GetOctree (int absFirstAxis, int absSecondAxis);							// get the octree at absX,absZ
	const phBoundBVH* GetOctree (int absFirstAxis, int absSecondAxis) const;				// get the octree at absX,absZ
	phBoundBVH* GetOctree (int cellIndex);								// get the octree at this index
	const phBoundBVH* GetOctree (int cellIndex) const;					// get the octree at this index

#if __SPU
    const atArray< datOwner<phBoundBVH> >& GetOctrees() const;
#endif // __SPU

#if !__SPU
	typedef  atDelegate< void ( const phBoundBVH* ) >    phBoundOctreeQueryDelegate;

	// PURPOSE: Call the callback once for each octree in the sphere
	// PARAMS
	//   sphere - x, y, z determine sphere's position, while w determines the radius
	//   callback - The delegate to call for each poly found
	void QueryOctreesInSphere(const Vector4& sphere, phBoundOctreeQueryDelegate functor);

#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
	virtual void DrawCellInfo(Mat34V_In mtx, int firstAxis, int secondAxis) const;
	virtual void DrawNormals(Mat34V_In mtx, int normalType = FACE_NORMALS, int whichPolys = ALL_POLYS, float length = 1.0f, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;

	void ClearDrawCell ()													{ m_DrawCell = -1; }
	void NextDrawCell ()													{ m_DrawCell = (m_DrawCell>=0) ? (m_DrawCell+1) : 0; m_DrawCell = m_DrawCell % (m_CellsFirstAxis*m_CellsSecondAxis); }
	void PrevDrawCell ()													{ m_DrawCell = (m_DrawCell>=0) ? (m_DrawCell+(m_CellsFirstAxis*m_CellsSecondAxis-1)) : 0; m_DrawCell = m_DrawCell % (m_CellsFirstAxis*m_CellsSecondAxis); }
	void SetDrawCell (int drawCell)											{ m_DrawCell = drawCell; }
	void SetDrawCell (const Vector3& position)								{ SetDrawCell(CellIndex(position)); }
	int GetDrawCell () const												{ return m_DrawCell; }
#endif // __PFDRAW

#endif // !__SPU

	// intersecting
	static const int INVALID_CELL = -1;

	// PURPOSE: Get the index number of the first cell along the given segment.
	// PARAMS:
	//	segStart - the starting point of the segment
	//	segStartToEnd - the segment vector from its starting point to its ending point
	// RETURN: the index number of the first octree along the segment
	// NOTES:
	//	The segment vector is included in case the segment starts outside and enters the octree grid.
	int GetFirstCellOnSegment (const Vector3& segStart, const Vector3& segStartToEnd) const;

	// PURPOSE: Get the index number of the next cell along the given segment, after the given cell index.
	// PARAMS:
	//	index - the index number of the current cell along the segment
	//	segStart - the starting point of the segment
	//	segStartToEnd - the segment vector from its starting point to its ending point
	// RETURN: the index number of the next octree along the segment after the given cell
	int GetNextCellOnSegment (int index, const Vector3& segStart, const Vector3& segStartToEnd) const;

	// PURPOSE: Find the index number of the cell with the given coordinates.
	// PARAMS:
	//	absFirstAxis - the cell's absolute coordinate along the first axis
	//	absSecondAxis - the cell's absolute coordinate along the second axis.
	int CellIndexAbsolute (int absFirstAxis, int absSecondAxis) const;

	// PURPOSE: Find the coordinates for the given cell index.
	// PARAMS:
	//	index - the index number of the cell for which to find coordinates
	//	absFirst - the absolute coordinate along the first axis of the cell with the given index
	//	absSecond - the absolute coordinate along the second axis of the cell with the given index
	void CellCoordinatesAbsolute (int index, int& absFirst, int& absSecond) const;

	// PURPOSE: Compute the extents of the specified cell.
	void ComputeCellExtents (int cellIndex, Vector3& cellMin, Vector3& cellMax) const;

	// PURPOSE: Change the octree grid's up direction to X.
	// NOTES:
	//	1.	The default state is y up.
	//	2.	This should be called before any geometry is added to the octree grid.
	void SetUpDirectionX ();

	// PURPOSE: Change the octree grid's up direction to Z.
	// NOTES:
	//	1.	The default state is y up.
	//	2.	This should be called before any geometry is added to the octree grid.
	void SetUpDirectionZ ();

	// PURPOSE: Return the "first axis" of the octree grid
	// RETURNS: 0, 1, or 2, depending on the up direction for this grid
	int GetFirstAxis() const;

	// PURPOSE: Return the "second axis" of the octree grid
	// RETURNS: 0, 1, or 2, depending on the up direction for this grid
	int GetSecondAxis() const;

	// PURPOSE: Get the total number of octrees in the grid.
	// RETURN: the total number of octrees in the grid
	int GetNumOctrees () const;


protected:

	/////////////////////////////////////////////////////////////
	// the grid structure

	// PURPOSE: the cell size and its inverse
	float m_CellDim, m_InvCellDim;

	// PURPOSE: the minimum cell coordinate along the first axis
	int m_MinFirstAxis;

	// PURPOSE: the maximum cell coordinate along the first axis
	int m_MaxFirstAxis;

	// PURPOSE: the number of cells along the direction of the first axis (default first axis is X)
	int m_CellsFirstAxis;

	// PURPOSE: the minimum cell coordinate along the second axis
	int m_MinSecondAxis;

	// PURPOSE: the maximum cell coordinate along the second axis
	int m_MaxSecondAxis;

	// PURPOSE: the number of cells along the direction of the second axis (default second axis is Z)
	int m_CellsSecondAxis;

	// PURPOSE: the octrees in the grid cells
	atArray<datOwner<phBoundBVH> > m_Octrees;

	// PURPOSE: the current cell to draw (-1 -> draw all)
	// NOTES:	this had to be moved out of __PFDRAW to support of resourcing
	int m_DrawCell;

	// PURPOSE: the index number of the first axis in the plane of the grid (0==X, 1==Y and 2==Z)
	// NOTES:
	//	1.	The default value is 0 (X axis).
	u8 m_FirstAxis;

	// PURPOSE: the index number of the second axis in the plane of the grid (0==X, 1==Y and 2==Z)
	// NOTES:
	//	1.	The default value is 2 (Z axis).
	u8 m_SecondAxis;

	u8 pad[2];

	/////////////////////////////////////////////////////////////
	// load/save
	bool Load_v110 (fiAsciiTokenizer & token);								// load, ascii, v1.10

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);								// save, ascii, v1.10
#endif

	bool LoadDef_v110 (fiAsciiTokenizer & token);							// load the definition, ascii, v1.10

#if !__FINAL && !IS_CONSOLE
	bool SaveDef_v110 (fiAsciiTokenizer & token);							// load the definition, ascii, v1.10
	bool SaveCell_v110 (fiAsciiTokenizer & token, int index);				// save one cell, ascii, v1.10
#endif

	/////////////////////////////////////////////////////////////
	// helper functions / data

	// indexing
	int CellIndexRelative (int relFirstAxis, int relSecondAxis) const		{ return (relSecondAxis * m_CellsFirstAxis + relFirstAxis); }
	inline int CellIndex (const Vector3& point) const;						// cell by point, inlined below
	void CellCoordinatesRelative (int index, int& relFirstAxis, int& relSecondAxis) const { relFirstAxis = index % m_CellsFirstAxis; relSecondAxis = index / m_CellsFirstAxis; }
};


/////////////////////////////////////////////////////////////////
// inlined functions

#if !__SPU
template <class GridCellType>
inline void phBoundGrid::InitGrid (float cellDim, int minFirstAxis, int maxFirstAxis, int minSecondAxis, int maxSecondAxis)
{
    // Make sure the given parameters are legitimate.
    Assertf(m_Octrees.GetCapacity() == 0,"InitGrid was called twice on the same octree grid bound.");
    Assertf(cellDim>0.0f,"The octree grid cell size %f is not positive.",cellDim);
    Assertf(maxFirstAxis>=minFirstAxis,"The first min (%i) and max (%i) are inverted",minFirstAxis,maxFirstAxis);
    Assertf(maxSecondAxis>=minSecondAxis,"The second min (%i) and max (%i) are inverted",minSecondAxis,maxSecondAxis);

    sysMemUseMemoryBucket bucket(sm_MemoryBucket);

    m_CellDim = cellDim;
    m_InvCellDim = 1.0f / m_CellDim;

    m_MinFirstAxis = minFirstAxis;
    m_MaxFirstAxis = maxFirstAxis;
    m_CellsFirstAxis = m_MaxFirstAxis - m_MinFirstAxis + 1;

    m_MinSecondAxis = minSecondAxis;
    m_MaxSecondAxis = maxSecondAxis;
    m_CellsSecondAxis = m_MaxSecondAxis - m_MinSecondAxis + 1;

    AssertMsg ((double(m_CellsFirstAxis )*double(m_CellsSecondAxis)) < double(INT_MAX), "numCells exceeds integer range");
    // Maybe put in an additional assert for a sensible upper bound for the numCells..
    int numCells = m_CellsFirstAxis * m_CellsSecondAxis;
    m_Octrees.Reserve(numCells);
    m_Octrees.Resize(numCells);

    for (int cell = 0; cell < numCells; ++cell)
    {
        m_Octrees[cell] = rage_new GridCellType;
    }
}
#endif


inline int phBoundGrid::CellIndex (const Vector3& point) const
{
	int relCellSecond = (int)floorf(point[m_SecondAxis] * m_InvCellDim) - m_MinSecondAxis;
	int relCellFirst = (int)floorf(point[m_FirstAxis] * m_InvCellDim) - m_MinFirstAxis;
	return (relCellFirst>=0 && relCellFirst<m_CellsFirstAxis && relCellSecond>=0 && relCellSecond<m_CellsSecondAxis) ? relCellSecond*m_CellsFirstAxis+relCellFirst : INVALID_CELL;
}

inline float phBoundGrid::GetCellSize () const
{
	return m_CellDim;
}

inline float phBoundGrid::GetInverseCellSize () const
{
	return m_InvCellDim;
}

/////////////////////////////////////////////////////////////////
// utility

inline phBoundBVH* phBoundGrid::GetOctree (int absFirstAxis, int absSecondAxis)
{
    return m_Octrees.GetCount() ? m_Octrees[CellIndexAbsolute(absFirstAxis,absSecondAxis)] : NULL;
}


inline const phBoundBVH* phBoundGrid::GetOctree (int absFirstAxis, int absSecondAxis) const
{
    return m_Octrees.GetCount() ? m_Octrees[CellIndexAbsolute(absFirstAxis,absSecondAxis)] : NULL;
}

inline phBoundBVH* phBoundGrid::GetOctree (int cellIndex)
{
    return ((m_Octrees.GetCount() && cellIndex>=0 && cellIndex<m_CellsFirstAxis*m_CellsSecondAxis) ? m_Octrees[cellIndex] : NULL);
}


inline const phBoundBVH* phBoundGrid::GetOctree (int cellIndex) const
{
    return ((m_Octrees.GetCount() && cellIndex>=0 && cellIndex<m_CellsFirstAxis*m_CellsSecondAxis) ? m_Octrees[cellIndex] : NULL);
}

#if __SPU
inline const atArray< datOwner<phBoundBVH> >& phBoundGrid::GetOctrees() const
{
    return m_Octrees;
}
#endif // __SPU

#if !__SPU
inline const phMaterial & phBoundGrid::GetMaterial (phMaterialIndex ) const
{
    return (m_Octrees.GetCount()) ? m_Octrees[0]->GetMaterial(0) : MATERIALMGR.GetDefaultMaterial();
}

inline phMaterialMgr::Id phBoundGrid::GetMaterialId (phMaterialIndex ) const
{
    return (m_Octrees.GetCount()) ? m_Octrees[0]->GetMaterialId(0) : phMaterialMgr::DEFAULT_MATERIAL_ID;
}
#endif // !__SPU


inline void phBoundGrid::SetUpDirectionX ()
{
	// Set the first axis to the Y direction.
	m_FirstAxis = 1;

	// Set the second axis to the Z direction.
	m_SecondAxis = 2;
}

inline void phBoundGrid::SetUpDirectionZ ()
{
	// Set the first axis to the X direction.
	m_FirstAxis = 0;

	// Set the second axis to the Y direction.
	m_SecondAxis = 1;
}

inline int phBoundGrid::GetFirstAxis() const
{
	return m_FirstAxis;
}

inline int phBoundGrid::GetSecondAxis() const
{
	return m_SecondAxis;
}

inline int phBoundGrid::CellIndexAbsolute (int absFirstAxis, int absSecondAxis) const
{
	return ((absSecondAxis-m_MinSecondAxis) * m_CellsFirstAxis + (absFirstAxis-m_MinFirstAxis));
}

inline void phBoundGrid::CellCoordinatesAbsolute (int index, int& absFirst, int& absSecond) const
{
	absFirst = m_MinFirstAxis + (index % m_CellsFirstAxis); absSecond = m_MinSecondAxis + (index / m_CellsFirstAxis);
}

inline void phBoundGrid::ComputeCellExtents (int cellIndex, Vector3& cellMin, Vector3& cellMax) const
{
	int cellIndexFirstAxis, cellIndexSecondAxis;
	CellCoordinatesAbsolute(cellIndex,cellIndexFirstAxis,cellIndexSecondAxis);
	float cellMinFirstAxis = cellIndexFirstAxis*m_CellDim;
	float cellMinSecondAxis = cellIndexSecondAxis*m_CellDim;
	cellMin[m_FirstAxis] = cellMinFirstAxis;
	cellMax[m_FirstAxis] = cellMinFirstAxis+m_CellDim;
	cellMin[m_SecondAxis] = cellMinSecondAxis;
	cellMax[m_SecondAxis] = cellMinSecondAxis+m_CellDim;
	int thirdAxis = 3-m_FirstAxis-m_SecondAxis;
	cellMin[thirdAxis] = -FLT_MAX;
	cellMax[thirdAxis] = FLT_MAX;
}

inline int phBoundGrid::GetNumOctrees () const
{
    FastAssert(m_Octrees.GetCount() == m_CellsFirstAxis*m_CellsSecondAxis);
	return m_Octrees.GetCount();
}

#endif // USE_GRIDS

} // namespace rage

#endif
