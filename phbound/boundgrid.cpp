//
// phbound/boundotgrid.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundgrid.h"

#include "boundbvh.h"
#include "boundculler.h"

#include "atl/array_struct.h"
#include "grcore/viewport.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "vector/colors.h"
#include "vector/geometry.h"

namespace rage {

#if USE_GRIDS

#if !__SPU

/////////////////////////////////////////////////////////////////

#define OT_DEBUG_DRAW 0

////////////////////////////////////////////////////////////////
// profiling variables

EXT_PFD_DECLARE_GROUP(Octrees);
EXT_PFD_DECLARE_ITEM(OctreeGridCellBoundaries);
EXT_PFD_DECLARE_ITEM(OctreePolyCount);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);


/////////////////////////////////////////////////////////////////

phBoundGrid::phBoundGrid ()
{
	m_Type = OCTREEGRID;

	m_CellDim = 0.0f;
	m_InvCellDim = 0.0f;

	m_MinFirstAxis = 0;
	m_MaxFirstAxis = 0;
	m_CellsFirstAxis = 0;

	m_MinSecondAxis = 0;
	m_MaxSecondAxis = 0;
	m_CellsSecondAxis = 0;

	m_DrawCell = -1;

	// Set the first axis to X and the second axis to Z, so that Y is up.
	// This can be changed by calling SetUpDirectionX() or SetUpDirectionZ().
	m_FirstAxis = 0;
	m_SecondAxis = 2;

	memset(pad, 0, sizeof(pad));
}


phBoundGrid::~phBoundGrid ()
{
	DeleteGrid();
}

void phBoundGrid::DeleteGrid ()
{
	if( phConfig::IsRefCountingEnabled() )
	{
		int numCells = m_Octrees.GetCount();
		for(int i = 0; i < numCells; ++i)
		{
			m_Octrees[i]->Release();
		}
	}
	else
	{
		int numCells = m_Octrees.GetCount();
		for(int i = 0; i < numCells; ++i)
		{
			delete m_Octrees[i];
		}
	}

    m_Octrees.Reset();
}


void phBoundGrid::CalculateExtents ()
{
	Vec3V boundingBoxMin = Vec3V(V_FLT_MAX);
	Vec3V boundingBoxMax = Vec3V(V_NEG_FLT_MAX);

	int numCells = m_Octrees.GetCount();

	for(int i = 0; i < numCells; ++i)
	{
		if (m_Octrees[i])
		{
			boundingBoxMin = Min(boundingBoxMin, m_Octrees[i]->GetBoundingBoxMin());
			boundingBoxMax = Max(boundingBoxMax, m_Octrees[i]->GetBoundingBoxMax());
		}
	}

	boundingBoxMin[m_FirstAxis] = Max(boundingBoxMin[m_FirstAxis], m_MinFirstAxis*m_CellDim);
	boundingBoxMin[m_SecondAxis] = Max(boundingBoxMin[m_SecondAxis], m_MinSecondAxis*m_CellDim);

	boundingBoxMax[m_FirstAxis] = Min(boundingBoxMax[m_FirstAxis], (m_MaxFirstAxis+1)*m_CellDim);
	boundingBoxMax[m_SecondAxis] = Min(boundingBoxMax[m_SecondAxis], (m_MaxSecondAxis+1)*m_CellDim);

	SetBoundingBoxMin(boundingBoxMin);
	SetBoundingBoxMax(boundingBoxMax);

	CalculateSphereFromBoundingBox();
}


/////////////////////////////////////////////////////////////////
// load/save

bool phBoundGrid::Load_v110 (fiAsciiTokenizer & token)
{
	sysMemUseMemoryBucket bucket(sm_MemoryBucket);
	if (!LoadDef_v110(token))
	{
		return false;
	}

	Assert(m_Octrees.GetCount());

	//Start extents gather here
	Vec3V boundingBoxMin = Vec3V(V_FLT_MAX);
	Vec3V boundingBoxMax = Vec3V(V_NEG_FLT_MAX);

	bool oldWarn = phBound::GetBoundFlag(phBound::WARN_ZERO_VERTICES);
	phBound::SetBoundFlag(phBound::WARN_ZERO_VERTICES,false);

	int firstAxisIndex,secondAxisIndex;
	for (secondAxisIndex=m_MinSecondAxis; secondAxisIndex<=m_MaxSecondAxis; secondAxisIndex++)
	{
		for (firstAxisIndex=m_MinFirstAxis; firstAxisIndex<=m_MaxFirstAxis; firstAxisIndex++)
		{
			int i = CellIndexAbsolute(firstAxisIndex,secondAxisIndex);
			if (!m_Octrees[i]->Load_v110(token))
			{
				phBound::SetBoundFlag(phBound::WARN_ZERO_VERTICES,oldWarn);
				return false;
			}

			boundingBoxMin = Min(boundingBoxMin, m_Octrees[i]->GetBoundingBoxMin());
			boundingBoxMax = Max(boundingBoxMax, m_Octrees[i]->GetBoundingBoxMax());
		}
	}

	boundingBoxMin[m_FirstAxis] = Max(boundingBoxMin[m_FirstAxis], m_MinFirstAxis*m_CellDim);
	boundingBoxMin[m_SecondAxis] = Max(boundingBoxMin[m_SecondAxis], m_MinSecondAxis*m_CellDim);

	boundingBoxMax[m_FirstAxis] = Min(boundingBoxMax[m_FirstAxis], (m_MaxFirstAxis+1)*m_CellDim);
	boundingBoxMax[m_SecondAxis] = Min(boundingBoxMax[m_SecondAxis], (m_MaxSecondAxis+1)*m_CellDim);

	SetBoundingBoxMin(boundingBoxMin);
	SetBoundingBoxMax(boundingBoxMax);

	CalculateSphereFromBoundingBox();

	phBound::SetBoundFlag(phBound::WARN_ZERO_VERTICES,oldWarn);
	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundGrid::Save_v110 (fiAsciiTokenizer & token)
{
	if (!SaveDef_v110(token))
	{
		return false;
	}
	token.Put("\n");

	int firstAxis, secondAxis;
	for (secondAxis=m_MinSecondAxis; secondAxis<=m_MaxSecondAxis; secondAxis++)
	{
		for (firstAxis=m_MinFirstAxis; firstAxis<=m_MaxFirstAxis; firstAxis++)
		{
			if (!SaveCell_v110(token,CellIndexAbsolute(firstAxis,secondAxis)))
			{
				return false;
			}
			token.Put("\n");
		}
	}

	return true;
}


#endif	// end of #if !__FINAL && !IS_CONSOLE

bool phBoundGrid::LoadDef_v110 (fiAsciiTokenizer & token)
{
	token.MatchToken("CellDim:");
	float cellDim = token.GetFloat();

	token.MatchToken("MinX:");
	int minFistAxis = token.GetInt();

	token.MatchToken("MaxX:");
	int maxFirstAxis = token.GetInt();

	token.MatchToken("MinZ:");
	int minSecondAxis = token.GetInt();

	token.MatchToken("MaxZ:");
	int maxSecondAxis = token.GetInt();

	if (token.CheckToken("xUp"))
	{
		SetUpDirectionX();
	}
	else if (token.CheckToken("zUp"))
	{
		SetUpDirectionZ();
	}

    if (token.CheckIToken("BVH"))
    {
        InitGrid<phBoundBVH>(cellDim,minFistAxis,maxFirstAxis,minSecondAxis,maxSecondAxis);
    }
    else
    {
        token.CheckIToken("Octree");
		InitGrid<phBoundBVH>(cellDim,minFistAxis,maxFirstAxis,minSecondAxis,maxSecondAxis);
    }

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundGrid::SaveDef_v110 (fiAsciiTokenizer & token)
{
	Assert(m_Octrees.GetCount());

	token.PutDelimiter("\n");

	token.PutDelimiter("CellDim: ");
	token.Put(m_CellDim);
	token.PutDelimiter("\n");

	token.PutDelimiter("MinX: ");
	token.Put(m_MinFirstAxis);
	token.PutDelimiter("\n");

	token.PutDelimiter("MaxX: ");
	token.Put(m_MaxFirstAxis);
	token.PutDelimiter("\n");

	token.PutDelimiter("MinZ: ");
	token.Put(m_MinSecondAxis);
	token.PutDelimiter("\n");

	token.PutDelimiter("MaxZ: ");
	token.Put(m_MaxSecondAxis);
	token.PutDelimiter("\n");

	if (m_FirstAxis==0 && m_SecondAxis==1)
	{
		token.PutDelimiter("zUp");
	}
	else if (m_FirstAxis==1 && m_SecondAxis==2)
	{
		token.PutDelimiter("xUp");
	}

    if (m_Octrees[0]->GetType() == phBound::BVH)
    {
        token.PutDelimiter("BVH");
    }

	return true;
}


bool phBoundGrid::SaveCell_v110 (fiAsciiTokenizer & token, int index)
{
	Assert(m_Octrees.GetCount());
	return m_Octrees[index]->Save_v110(token);
}

#endif	// end of #if !__FINAL && !IS_CONSOLE

/////////////////////////////////////////////////////////////////
// testing
#endif // !__SPU

// TestProbe intersection helper function
int phBoundGrid::GetFirstCellOnSegment (const Vector3& segStart, const Vector3& segStartToEnd) const
{
	// Find the cell index numbers along the x and z axes that contain the segment's start point.
	int cellIndex = CellIndex(segStart);
	if (cellIndex!=INVALID_CELL)
	{
		// The segment start point is inside an octree grid cell, so return the cell index number.
		return cellIndex;
	}

	// The segment start point is not inside an octree grid cell. Try to find the first cell the segment enters.
	float dispFirstAxis,dispSecondAxis;
	float fabsSegFirstAxis = fabsf(segStartToEnd[m_FirstAxis]);
	float fabsSegSecondAxis = fabsf(segStartToEnd[m_SecondAxis]);
	int maxNumSteps;
	if (fabsSegFirstAxis>=fabsSegSecondAxis && fabsSegFirstAxis>SMALL_FLOAT)
	{
		// The segment is directed more along x than along z, so change x by one cell size,
		// and change z to stay on the segment.
		dispFirstAxis = segStartToEnd[m_FirstAxis]>0.0f ? m_CellDim : -m_CellDim;
		dispSecondAxis = dispFirstAxis*segStartToEnd[m_SecondAxis]/segStartToEnd[m_FirstAxis];
		maxNumSteps = int(fabsSegFirstAxis/m_CellDim)+1;
	}
	else if (fabsSegSecondAxis>SMALL_FLOAT)
	{
		// The segment is directed more along z than along x, so change z by one cell size,
		// and change x to stay on the segment.
		dispSecondAxis = segStartToEnd[m_SecondAxis]>0.0f ? m_CellDim : -m_CellDim;
		dispFirstAxis = dispSecondAxis*segStartToEnd[m_FirstAxis]/segStartToEnd[m_SecondAxis];
		maxNumSteps = int(fabsSegSecondAxis/m_CellDim)+1;
	}
	else
	{
		// The segment has nearly zero length and does not start in a cell, so it does not touch the octree grid.
		return INVALID_CELL;
	}

	// Move along the segment to find the first cell it enters.
	Vector3 segPosition(segStart);
	int numSteps = 0;
	while (cellIndex==INVALID_CELL && numSteps<maxNumSteps)
	{
		segPosition[m_FirstAxis] += dispFirstAxis;
		segPosition[m_SecondAxis] += dispSecondAxis;
		cellIndex = CellIndex(segPosition);
		numSteps++;
	}

	// Return the index of the first cell hit by the segment, or INVALID_CELL if the segment does not touch
	// the octree grid.
	return cellIndex;
}


// TestProbe intersection helper function
int phBoundGrid::GetNextCellOnSegment (int index, const Vector3& segStart, const Vector3& segStartToEnd) const
{
	float tFirstAxis,tSecondAxis;

	int absFirstAxis,absSecondAxis;
	CellCoordinatesAbsolute(index,absFirstAxis,absSecondAxis);

	// x intercept
	if (segStartToEnd[m_FirstAxis] > 0.0f)
	{
		tFirstAxis = ((absFirstAxis+1)*m_CellDim - segStart[m_FirstAxis]) / segStartToEnd[m_FirstAxis];
	}
	else if (segStartToEnd[m_FirstAxis] < 0.0f)
	{
		tFirstAxis = (absFirstAxis*m_CellDim - segStart[m_FirstAxis]) / segStartToEnd[m_FirstAxis];
	}
	else // segStartToEnd[m_FirstAxis] == 0.0f
	{
		tFirstAxis = 2.0f;
	}

	// z intercept
	if (segStartToEnd[m_SecondAxis] > 0.0f)
	{
		tSecondAxis = ((absSecondAxis+1)*m_CellDim - segStart[m_SecondAxis]) / segStartToEnd[m_SecondAxis];
	}
	else if (segStartToEnd[m_SecondAxis] < 0.0f)
	{
		tSecondAxis = (absSecondAxis*m_CellDim - segStart[m_SecondAxis]) / segStartToEnd[m_SecondAxis];
	}
	else // segStartToEnd[m_SecondAxis] == 0.0f
	{
		tSecondAxis = 2.0f;
	}

	if (tFirstAxis >= 1.0f && tSecondAxis >= 1.0f)
	{
		return INVALID_CELL;
	}

	if (tFirstAxis < tSecondAxis)
	{
		// x wins
		if (segStartToEnd[m_FirstAxis] > 0.0f)
		{
			if (absFirstAxis < m_MaxFirstAxis)
			{
				absFirstAxis++;
			}
			else
			{
				return INVALID_CELL;
			}
		}
		else
		{
			if (absFirstAxis > m_MinFirstAxis)
			{
				absFirstAxis--;
			}
			else
			{
				return INVALID_CELL;
			}
		}
	}
	else
	{
		// z wins
		if (segStartToEnd[m_SecondAxis] > 0.0f)
		{
			if (absSecondAxis < m_MaxSecondAxis)
			{
				absSecondAxis++;
			}
			else
			{
				return INVALID_CELL;
			}
		}
		else
		{
			if (absSecondAxis > m_MinSecondAxis)
			{
				absSecondAxis--;
			}
			else
			{
				return INVALID_CELL;
			}
		}
	}

	return CellIndexAbsolute(absFirstAxis,absSecondAxis);
}

#if !__SPU

void phBoundGrid::QueryOctreesInSphere(const Vector4& sphere, phBoundOctreeQueryDelegate functor)
{
	Assert(m_Octrees.GetCount());

	float radius = sphere.w;
	Vector3 center;
	sphere.GetVector3(center);

	int minFirstAxis = Max(m_MinFirstAxis,(int)floorf((center[m_FirstAxis]-radius)*m_InvCellDim));
	int maxFirstAxis = Min(m_MaxFirstAxis,(int)floorf((center[m_FirstAxis]+radius)*m_InvCellDim));
	int minSecondAxis = Max(m_MinSecondAxis,(int)floorf((center[m_SecondAxis]-radius)*m_InvCellDim));
	int maxSecondAxis = Min(m_MaxSecondAxis,(int)floorf((center[m_SecondAxis]+radius)*m_InvCellDim));

	// loop over all the intersected cells and call TestSphere on the contained octrees

	// firstAxis and secondAxis are absolute cell indices, not relative to m_MinFirstAxis, m_MinSecondAxis
	// TODO: only test against intersected cells, not the full rectangle

	int firstAxis, secondAxis, cellIndex;
	for (firstAxis=minFirstAxis; firstAxis<=maxFirstAxis; firstAxis++)
	{
		for (secondAxis=minSecondAxis; secondAxis<=maxSecondAxis; secondAxis++)
		{
			// intersected cell: firstAxis, secondAxis
			cellIndex = CellIndexAbsolute(firstAxis,secondAxis);
			phBoundBVH *pOctree = GetOctree(cellIndex);
			if (!pOctree)
			{
				continue;
			}

			functor(pOctree);
		}
	}
}


// Cull the polygons in the octrees with the given sphere, and give back a list of culled octrees.
// The sphere radius must be no greater than half the cell dimension, so that no more than four octrees
// will ever be included.
// Note: vertexOffsets no longer serves any useful purpose and should probably be removed.
bool phBoundGrid::CullSpherePolys (const Vector3& center, float radius, const phBoundBVH** culledOctrees, int* polyOffsets, int* vertexOffsets, phBoundCuller& culler) const
{
	Assert(m_Octrees.GetCount());
	Assert(radius<0.5f*m_CellDim);

	bool result = false;
	int minFirstAxis = Max(m_MinFirstAxis,(int)floorf((center[m_FirstAxis]-radius)*m_InvCellDim));
	int maxFirstAxis = Min(m_MaxFirstAxis,(int)floorf((center[m_FirstAxis]+radius)*m_InvCellDim));
	int minSecondAxis = Max(m_MinSecondAxis,(int)floorf((center[m_SecondAxis]-radius)*m_InvCellDim));
	int maxSecondAxis = Min(m_MaxSecondAxis,(int)floorf((center[m_SecondAxis]+radius)*m_InvCellDim));

	// loop over all the intersected cells and call CullSpherePolys on the contained octrees

	// firstAxis and secondAxis are absolute cell indices, not relative to m_MinFirstAxis, m_MinSecondAxis
	int firstAxis, secondAxis, cellIndex;
	int numCellsHit = 0;
	for (firstAxis=minFirstAxis; firstAxis<=maxFirstAxis; firstAxis++)
	{
		for (secondAxis=minSecondAxis; secondAxis<=maxSecondAxis; secondAxis++)
		{
			// intersected cell: firstAxis, secondAxis
			cellIndex = CellIndexAbsolute(firstAxis,secondAxis);

			const phBoundBVH *pOctree = GetOctree(cellIndex);
			if (!pOctree)
				continue;

#if (ENABLE_DRAW_PHYS && OT_DEBUG_DRAW)
			// draw the cell
			phDrawPhys::sph->DrawBox(Vector3(x*CellDim,0,z*CellDim),Vector3((x+1)*CellDim,CellDim,(z+1)*CellDim),M34_IDENTITY,DP_SPH_HIT_ELEMENT);

			// draw the octree
			pOctree->DrawPhysics(phDrawPhys::sph,DP_SPH_HIT_ELEMENT,M34_IDENTITY);
#endif

			polyOffsets[numCellsHit] = culler.GetNumCulledPolygons();
			vertexOffsets[numCellsHit] = 0;//culler.GetNumCulledVertices();

			// test against the octree
			pOctree->CullSpherePolys(culler,RCC_VEC3V(center),ScalarVFromF32(radius));
			if (culler.GetNumCulledPolygons()>0)
			{
				result = true;
				culledOctrees[numCellsHit] = pOctree;
				numCellsHit++;
				Assert(numCellsHit<=4);
			}
		}
	}

	return result;
}


#if __TOOL
bool phBoundGrid::InitOctrees (void)
{
	return true;
}
void phBoundGrid::AddGeometry (phBoundGeometry* geom)
{
	Assert(geom);
	int minFirstAxis = Max(m_MinFirstAxis,(int)floorf(geom->GetBoundingBoxMin()[m_FirstAxis]*m_InvCellDim));
	int maxfirstAxis = Min(m_MaxFirstAxis,(int)floorf(geom->GetBoundingBoxMax()[m_FirstAxis]*m_InvCellDim));
	int minSecondAxis = Max(m_MinSecondAxis,(int)floorf(geom->GetBoundingBoxMin()[m_SecondAxis]*m_InvCellDim));
	int maxSecondAxis = Min(m_MaxSecondAxis,(int)floorf(geom->GetBoundingBoxMax()[m_SecondAxis]*m_InvCellDim));
	for (int secondAxis=minSecondAxis; secondAxis<=maxSecondAxis; secondAxis++)
	{
		for (int firstAxis=minFirstAxis; firstAxis<=maxfirstAxis; firstAxis++)
		{
			if (phBound::MessagesEnabled())
			{
				Displayf("Adding geometry to cell %d, %d...", firstAxis, secondAxis);
			}

			AddGeometryToCell(geom,CellIndexAbsolute(firstAxis,secondAxis));
		}
	}
}

void phBoundGrid::AddGeometryToCell (phBoundGeometry * geom, int cellIndex)
{
	Assert(geom);

	phPolygon::Index * vertexMap = rage_new phPolygon::Index [geom->GetNumVertices()];
	memset(vertexMap,phBoundPolyhedron::INVALID_VERTEX,sizeof(phPolygon::Index)*geom->GetNumVertices());

	int numMat = geom->GetNumMaterials();
	phMaterialMgr::Id * materialMap=NULL;
	if (numMat)
	{
		materialMap = rage_new phMaterialMgr::Id [numMat];
#if PH_MATERIAL_ID_64BIT
		u64 matId64 = phMaterialMgr::MATERIAL_NOT_FOUND;
		memset(materialMap,(int)matId64,sizeof(phMaterialMgr::Id)*numMat);
#else
		memset(materialMap,phMaterialMgr::MATERIAL_NOT_FOUND,sizeof(phMaterialMgr::Id)*numMat);
#endif // PH_MATERIAL_ID_64BIT
	}
	else
	{
		materialMap = rage_new phMaterialMgr::Id;
#if PH_MATERIAL_ID_64BIT
		u64 matId64 = phMaterialMgr::MATERIAL_NOT_FOUND;
		memset(materialMap,(int)matId64,sizeof (phMaterialMgr::Id));
#else 
		memset(materialMap,phMaterialMgr::MATERIAL_NOT_FOUND,sizeof (phMaterialMgr::Id));
#endif // PH_MATERIAL_ID_64BIT
	}

	// cell boundaries
	int firstAxis,secondAxis;
	CellCoordinatesAbsolute(cellIndex,firstAxis,secondAxis);
	Vector3 cellMin, cellMax;
	cellMin[m_FirstAxis] = firstAxis * m_CellDim;
	cellMax[m_FirstAxis] = (firstAxis+1) * m_CellDim;
	int upAxis = 3-m_FirstAxis-m_SecondAxis;
	cellMin[upAxis] = 0.0f;
	cellMax[upAxis] = 0.0f;
	cellMin[m_SecondAxis] = secondAxis * m_CellDim;
	cellMax[m_SecondAxis] = (secondAxis+1) * m_CellDim;

	const Vector3 * verts[POLY_MAX_VERTICES];
	const Vector3 * geomVerts = (const Vector3*)geom->GetVertexPointer();
	const phPolygon * poly;

	for (int i=geom->GetNumPolygons()-1; i>=0; i--)
	{
		poly = &geom->GetPolygon(i);
		verts[0] = geomVerts+poly->GetVertexIndex(0);
		verts[1] = geomVerts+poly->GetVertexIndex(1);
		verts[2] = geomVerts+poly->GetVertexIndex(2);

		if (geom2D::Test2DPolyVsAlignedRect(POLY_MAX_VERTICES,verts,cellMin,cellMax,m_FirstAxis,m_SecondAxis))
		{
			AddPolygonToCell(geom,i,cellIndex,vertexMap,materialMap);
		}
		else
		{
			int err;
			err = 0;
		}
	}

	delete [] vertexMap;
	delete [] materialMap;
}

void phBoundGrid::AddPolygonToCell (phBoundGeometry * geom, int polyIndex, int cellIndex, phPolygon::Index * vertexMap, phMaterialMgr::Id * materialMap)
{
	Assert(geom);
	int i;
	u32 j;

	const phPolygon & srcPoly = geom->GetPolygon(polyIndex);

	phBoundBVH* octree = m_Octrees[cellIndex];

	// 1) add the vertices as necessary
	for (i=POLY_MAX_VERTICES-1; i>=0; i--)
	{
		j = srcPoly.GetVertexIndex(i);
		if ((int)vertexMap[j]==phBoundPolyhedron::INVALID_VERTEX)
		{
			// If we're crossing an order-of-magnitude boundary, reallocate
			if (((octree->m_NumVertices + 1) & octree->m_NumVertices) == 0)
			{
				Vector3* newVertices = rage_new Vector3[(octree->m_NumVertices + 1) * 2];

				for (int index = 0; index < octree->m_NumVertices; ++index)
				{
					newVertices[index] = RCC_VECTOR3(octree->m_Vertices[index]);
				}

				delete [] octree->m_Vertices;
				octree->m_Vertices = (Vec3V*)newVertices;
			}

			// NOTE: verts could be compared to all previously added verts
			//       to prevent duplication across different geometries
			Assert((u32)octree->m_NumVertices<(u32)phBoundPolyhedron::MAX_NUM_VERTICES);
			octree->SetVertex(octree->m_NumVertices, geom->GetVertex(j));
			vertexMap[j] = (phPolygon::Index)octree->m_NumVertices;
			octree->m_NumVertices++;
		}
	}

	// 3) add the polygon
	phPolygon newPoly;
	newPoly.Set(srcPoly);

	for (i=POLY_MAX_VERTICES-1; i>=0; i--)
	{
		newPoly.SetIndex(i,vertexMap[srcPoly.GetVertexIndex(i)]);
	}

	// 4) add the Materials as necessary
	j = geom->GetPolygonMaterialIndex(polyIndex);

	if (materialMap[j] == phMaterialMgr::MATERIAL_NOT_FOUND)
	{
		Assert(octree->m_NumMaterials<phBoundPolyhedron::MAX_NUM_MATERIALS);

		phMaterialMgr::Id newMaterial;
		newMaterial = geom->GetMaterialId(j);

		for (int index = 0; index < octree->m_NumMaterials; ++index)
		{
			if (octree->GetMaterialId(index) == newMaterial)
			{
				materialMap[j] = index;
				break;
			}
		}

		// If it wasn't found, add it!
		if (materialMap[j] == phMaterialMgr::MATERIAL_NOT_FOUND)
		{
			// If we're crossing an order-of-magnitude boundary, reallocate
			if (((octree->m_NumMaterials + 1) & octree->m_NumMaterials) == 0)
			{
				phMaterialMgr::Id* newMaterials = rage_new phMaterialMgr::Id[(octree->m_NumMaterials + 1) * 2];

				for (int index = 0; index < octree->m_NumMaterials; ++index)
					newMaterials[index] = octree->m_MaterialIds[index];

				delete [] octree->m_MaterialIds;
				octree->m_MaterialIds = newMaterials;
			}

			octree->m_MaterialIds[octree->m_NumMaterials] = newMaterial;
			materialMap[j] = octree->m_NumMaterials++;
		}
	}

	Assert(materialMap[j] <= u8(-1));

	newPoly.SetArea(0);
	geom->SetPolygonMaterialIndex(polyIndex, u8(materialMap[j]));

	Assert((u32)octree->m_NumPolygons<(u32)phBoundPolyhedron::MAX_NUM_POLYGONS);

	// If we're crossing an order-of-magnitude boundary, reallocate
	if (((octree->m_NumPolygons + 1) & octree->m_NumPolygons) == 0)
	{
		phPolygon* newPolygons = rage_new phPolygon[(octree->m_NumPolygons + 1) * 2];

		for (int index = 0; index < octree->m_NumPolygons; ++index)
		{
			newPolygons[index] = octree->m_Polygons[index];
		}

		delete [] octree->m_Polygons;
		octree->m_Polygons = newPolygons;
	}

	octree->SetPolygon(octree->m_NumPolygons, newPoly);
	octree->m_NumPolygons++;
}
#endif // __TOOL

phMaterialMgr::Id phBoundGrid::GetMaterialIdFromPartIndexAndComponent (int partIndex, int component) const
{
	Assert(component>=0);
	const phBoundBVH *pOctree = GetOctree(component);
	if (!pOctree)
	{
		return phMaterialMgr::DEFAULT_MATERIAL_ID;
	}

	return pOctree->GetMaterialIdFromPartIndex(partIndex);
}


// This returns true if the argument is given as an octree index number in the grid.
// If no index number is given, this will return false.
bool phBoundGrid::IsPolygonal (int component) const
{
	if (component>=0)
	{
		const phBoundBVH *pOctree = GetOctree(component);
		if (!pOctree)
		{
			return false;
		}

		Assert(pOctree->IsPolygonal());
		return true;
	}

	return false;
}


/////////////////////////////////////////////////////////////////
// development

#if __PFDRAW
void phBoundGrid::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int whichPolys, phMaterialFlags highlightFlags, unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	if (m_Octrees.GetCount())
	{
		int minCellFirstAxis, maxCellFirstAxis, minCellSecondAxis, maxCellSecondAxis;
#if 0
		if (phDrawPhys::IsInterestLimited()) // TODO DRAWPHYS
		{
			Vector3 interestCenter;
			float interestRadius;
			phDrawPhys::GetMaxInterest(interestRadius,interestCenter);
			minCellFirstAxis = Clamp((int)floorf((interestCenter[m_FirstAxis]-interestRadius)*m_InvCellDim),m_MinFirstAxis,m_MaxFirstAxis);
			maxCellFirstAxis = Clamp((int)floorf((interestCenter[m_FirstAxis]+interestRadius)*m_InvCellDim),m_MinFirstAxis,m_MaxFirstAxis);
			minCellSecondAxis = Clamp((int)floorf((interestCenter[m_SecondAxis]-interestRadius)*m_InvCellDim),m_MinSecondAxis,m_MaxSecondAxis);
			maxCellSecondAxis = Clamp((int)floorf((interestCenter[m_SecondAxis]+interestRadius)*m_InvCellDim),m_MinSecondAxis,m_MaxSecondAxis);
		}
		else
#endif
		{
			minCellFirstAxis = m_MinFirstAxis;
			maxCellFirstAxis = m_MaxFirstAxis;
			minCellSecondAxis = m_MinSecondAxis;
			maxCellSecondAxis = m_MaxSecondAxis;
		}

		Vector3 InstCenter;
		grcViewport *CurViewport = grcViewport::GetCurrent();

		if (CurViewport && (m_DrawCell < 0))
		{
			float drawDistance = PFD_BoundDrawDistance.GetValue();

			int cellFirstAxis, cellSecondAxis;
			for (cellFirstAxis = minCellFirstAxis; cellFirstAxis <= maxCellFirstAxis; cellFirstAxis++)
			{
				for (cellSecondAxis = minCellSecondAxis; cellSecondAxis <= maxCellSecondAxis; cellSecondAxis++)
				{
					const phBoundBVH *pOctree = GetOctree(CellIndexAbsolute(cellFirstAxis,cellSecondAxis));
					if (!pOctree)
					{
						continue;
					}

					// Let's check the bounding sphere of this octree versus the viewport and don't bother trying to draw it if it won't be visible anyway.
					InstCenter = VEC3V_TO_VECTOR3(pOctree->GetWorldCentroid(mtxIn));
                    if (grcViewport::GetCurrent())
                    {
					    Vector3 cameraDirection(InstCenter);
					    cameraDirection.Subtract(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition()));
					    cameraDirection.SubtractScaled(g_UnitUp, cameraDirection.Dot(g_UnitUp));
					    if (cameraDirection.Mag2() > square(drawDistance + pOctree->GetRadiusAroundCentroid()))
					    {
						    continue;
					    }
                    }

					if(CurViewport->IsSphereVisible(InstCenter.x, InstCenter.y, InstCenter.z, pOctree->GetRadiusAroundCentroid()) == cullOutside)
					{
						continue;
					}

					// Now let's check the bounding box of this octree versus the viewport and don't bother trying to draw it if it won't be visible anyway.
					if (!CurViewport->IsAABBVisible(pOctree->GetBoundingBoxMin().GetIntrin128(),pOctree->GetBoundingBoxMax().GetIntrin128(),CurViewport->GetFrustumLRTB()))
					{
						continue;
					}

					pOctree->Draw(mtxIn, colorMaterials, solid, whichPolys, highlightFlags);
				}
			}
		}
		else
		{
			Assert(m_DrawCell>=0 && m_DrawCell<(m_CellsFirstAxis*m_CellsSecondAxis));
			const phBoundBVH *pOctree = GetOctree(m_DrawCell);
			if (pOctree)
			{
				pOctree->Draw(mtxIn, colorMaterials, solid, whichPolys, highlightFlags);
			}
		}

#if __PFDRAW
		if (PFDGROUP_Octrees.GetEnabled())
		{
			grcWorldIdentity();
			bool oldLighting = grcLighting(false);

			// draw the cell boundaries	
			int firstAxis, secondAxis;

			if (m_DrawCell<0)
			{
				for (firstAxis = m_MinFirstAxis; firstAxis <= m_MaxFirstAxis; firstAxis++)
				{
					for (secondAxis = m_MinSecondAxis; secondAxis <= m_MaxSecondAxis; secondAxis++)
					{
						DrawCellInfo(mtxIn, firstAxis, secondAxis);
					}
				}
			}
			else
			{
				Assert(m_DrawCell>=0 && m_DrawCell<(m_CellsFirstAxis*m_CellsSecondAxis));
				CellCoordinatesAbsolute(m_DrawCell,firstAxis,secondAxis);
				DrawCellInfo(mtxIn, firstAxis, secondAxis);
			}

			grcLighting(oldLighting);
		}
#endif // __PFDRAW
	}
}

void phBoundGrid::DrawCellInfo(Mat34V_In mtx, int firstAxis, int secondAxis) const
{
	Matrix34 cellMtx(RCC_MATRIX34(mtx));
	cellMtx.d[m_FirstAxis] += (firstAxis+0.5f)*m_CellDim;
	cellMtx.d[m_SecondAxis] += (secondAxis+0.5f)*m_CellDim;
	int upAxis = 3-m_FirstAxis-m_SecondAxis;
	cellMtx.d[upAxis] += 0.5f*m_CellDim;

	Color32 oldColor(grcCurrentColor);
	if (PFD_OctreeGridCellBoundaries.GetEnabled())
	{
		grcDrawBox(Vector3(m_CellDim,m_CellDim,m_CellDim),cellMtx,Color_green);
	}

	const phBoundBVH* octree = GetOctree(CellIndexAbsolute(firstAxis,secondAxis));
	Assert(octree);
	if (octree->GetNumPolygons() > 0)
	{
		if (PFD_OctreePolyCount.GetEnabled())
		{
			float radius = logf(static_cast<float>(octree->GetNumPolygons())) * m_CellDim * 0.05f;
			grcDrawSphere(radius, cellMtx, 10, false, true);

			char buffer[64];
			formatf(buffer,sizeof(buffer),"%d",octree->GetNumPolygons());
			grcColor(Color_black);
			grcDrawLabel(cellMtx.d, 1, 1, buffer);
			grcColor(Color_white);
			grcDrawLabel(cellMtx.d, buffer);
		}
	}
	grcColor(oldColor);
}

void phBoundGrid::DrawNormals(Mat34V_In mtx, int normalType, int whichPolys, float length, unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter)) const
{
	if (m_Octrees.GetCount())
	{
		int minCellFirstAxis, maxCellFirstAxis, minCellSecondAxis, maxCellSecondAxis;
		minCellFirstAxis = m_MinFirstAxis;
		maxCellFirstAxis = m_MaxFirstAxis;
		minCellSecondAxis = m_MinSecondAxis;
		maxCellSecondAxis = m_MaxSecondAxis;

		if (m_DrawCell<0)
		{
			float squaredDrawDistance = PFD_BoundDrawDistance.GetValue() * PFD_BoundDrawDistance.GetValue();

			int cellFirstAxis, cellSecondAxis;
			for (cellFirstAxis = minCellFirstAxis; cellFirstAxis <= maxCellFirstAxis; cellFirstAxis++)
			{
				for (cellSecondAxis = minCellSecondAxis; cellSecondAxis <= maxCellSecondAxis; cellSecondAxis++)
				{
					const phBoundBVH* pOctree = GetOctree(CellIndexAbsolute(cellFirstAxis,cellSecondAxis));
					if (!pOctree)
					{
						continue;
					}

                    if (grcViewport::GetCurrent())
                    {
					    Vector3 cameraDirection = VEC3V_TO_VECTOR3(pOctree->GetWorldCentroid(mtx));
					    cameraDirection.Subtract(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition()));
					    if (cameraDirection.Mag2() > squaredDrawDistance + square(pOctree->GetRadiusAroundCentroid()))
					    {
						    continue;
					    }
                    }

					pOctree->DrawNormals(mtx, normalType, whichPolys, length);
				}
			}
		}
		else
		{
			Assert(m_DrawCell>=0 && m_DrawCell<(m_CellsFirstAxis*m_CellsSecondAxis));
			const phBoundBVH *pOctree = GetOctree(m_DrawCell);
			if (pOctree)
			{
				pOctree->DrawNormals(mtx, normalType, whichPolys, length);
			}
		}
	}
}
#endif // __PFDRAW

////////////////////////////////////////////////////////////////
// resources

phBoundGrid::phBoundGrid (datResource & rsc) : phBound (rsc), m_Octrees(rsc, true)
{
	//Fixup the array of phOTGridOTStreamer
	sysMemUseMemoryBucket bucket(sm_MemoryBucket);

	for(int i=0;i<(m_CellsFirstAxis*m_CellsSecondAxis);i++)
	{
	}

	if (m_FirstAxis == 0 && m_SecondAxis == 0)
	{
		// Default to Y-up if using old resources
		m_FirstAxis = 0;
		m_SecondAxis = 2;
	}
}

#if __DECLARESTRUCT
void phBoundGrid::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);

	STRUCT_BEGIN(phBoundGrid);
	STRUCT_FIELD(m_CellDim);
	STRUCT_FIELD(m_InvCellDim);
	STRUCT_FIELD(m_MinFirstAxis);
	STRUCT_FIELD(m_MaxFirstAxis);
	STRUCT_FIELD(m_CellsFirstAxis);
	STRUCT_FIELD(m_MinSecondAxis);
	STRUCT_FIELD(m_MaxSecondAxis);
	STRUCT_FIELD(m_CellsSecondAxis);
	STRUCT_FIELD(m_Octrees);
	STRUCT_FIELD(m_DrawCell);
	STRUCT_FIELD(m_FirstAxis);
	STRUCT_FIELD(m_SecondAxis);
	STRUCT_CONTAINED_ARRAY(pad);

	STRUCT_END();
}
#endif // __DECLARESTRUCT

#endif // !__SPU

#endif // USE_GRIDS

} // namespace rage
