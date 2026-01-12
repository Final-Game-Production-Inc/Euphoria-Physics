//
// phbound/boundsurface.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDSURFACE_H
#define PHBOUND_BOUNDSURFACE_H
#include "bound.h"
#include "surfacegrid.h"

namespace rage {

class phSurfaceGrid;
class phLiquidImpactSet;
class phLiquidImpactData;

class phBoundSurface : public phBound
{
public:
	////////////////////////////////////////////////////////////
	// Constructors / destructors.
	phBoundSurface();
	PH_NON_SPU_VIRTUAL ~phBoundSurface();

#if !__SPU
	DECLARE_PLACE(phBoundSurface);
	phBoundSurface(datResource & rsc);										// Construct in resource
#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT
#endif

	PH_NON_SPU_VIRTUAL const phMaterial & GetMaterial (phMaterialIndex materialIndex) const;
	PH_NON_SPU_VIRTUAL phMaterialMgr::Id GetMaterialId (phMaterialIndex index) const;

	////////////////////////////////////////////////////////////
	// FindLiquidImpactsXXX functions.
	bool FindLiquidImpactsToSphere(phLiquidImpactSet &ImpactSet) const;
	bool FindLiquidImpactsToCapsule(phLiquidImpactSet &ImpactSet) const;
	bool FindLiquidImpactsToBox(phLiquidImpactSet &ImpactSet) const;
	bool FindLiquidImpactsToPoly(phLiquidImpactSet &ImpactSet) const;
	bool FindLiquidImpactsToCurvedGeometry (phLiquidImpactSet& impactSet) const;

	bool FindLiquidImpactsToBoundingBox(phLiquidImpactSet &ImpactSet) const;

	//============================================================================
	// Debug drawing
#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif // __PFDRAW

	void SetGridData(phSurfaceGrid *SurfaceGrid);

	phSurfaceGrid *GetGridData()
	{
		return m_SurfaceGrid;
	}

	void CalculateBoundingVolumes();

	enum
	{
		kPointsPerGridEdge = phSurfaceGrid::kSurfaceGridLength,	 
		kPointsPerGrid = phSurfaceGrid::kSurfaceGridPointCount
	};


	///////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);
#endif

	// PURPOSE:
	//	Copies the specified elevation data into this bound, setting the m_MinimumElevation and m_MaximumElevation in the process.
	//	This will allow the class to quantize elevation data.
	void SetRawData(
		const phSurfaceGrid::GridData<float>* offsetData, 
		const phSurfaceGrid::GridData<Vector2>* velocityData, 
		float spacing, 
		short cellX, 
		short cellY,
		float minElevation,
		float maxElevation,
		float minX,
		float maxX,
		float minZ,
		float maxZ);

	// PURPOSE:
	//	Internal type for the offset data. This should eventually change to a quantized value, probably u16.
	typedef phSurfaceGrid::GridData<float> OffsetGrid;

	// PURPOSE:
	//	Internal type for the velocity data. This should eventually change to a quantized value, probably u16.
	typedef phSurfaceGrid::GridData<Vector2> VelocityGrid;

	const VelocityGrid& GetVelocityGrid() const { return m_VelocityGrid; }
	VelocityGrid& GetVelocityGrid() { return m_VelocityGrid; }

	const OffsetGrid& GetOffsetGrid() const { return m_OffsetGrid; }
	OffsetGrid& GetOffsetGrid() { return m_OffsetGrid; }

	float GetMinElevation() const { return m_MinElevation; }

protected:

	bool BoxIntersectsBoundingBox(Vector3::Vector3Param krvecSurfacePos, const phBound *kpPolyBound, const Matrix34 *kpmtxPolyCurrent,
		Vector3 &rvecBoxHalfWidths, Vector3 &rvecBoxCenter) const;

	int ClipAgainstBorders(const int kNumInVerts, const Vector3 *InVerts, const float *kInDepths, Vector3 *OutVerts, float *OutDepths) const;

	bool FindLiquidImpactsToSphere(Vector3::Vector3Param SpherePos_MS, const float kSphereRadius, phLiquidImpactData &ImpactData) const;

	float GetDepth(Vector3::Vector3Param kLocation) const;
	PH_NON_SPU_VIRTUAL float GetHeight(float LocationX, float LocationZ) const;

	VelocityGrid m_VelocityGrid;
	OffsetGrid m_OffsetGrid;
	float m_PeakWaveHeight;
//	int m_MaterialIndex;

	float m_Spacing; // space between points
	float m_MinElevation;
	float m_MaxElevation;
	// The cells are used by the phLiquidMgr to determine which bounds need to be connected. This should really be using the phInst coordinates instead.
	short m_CellX;
	short m_CellY;


	// Not resourced, should go away and just use the bounding box instead.
	float m_MinX;
	float m_MaxX;
	float m_MinZ;
	float m_MaxZ;

	phSurfaceGrid *m_SurfaceGrid;
	datPadding<8> m_Pad;
};

} // namespace rage

#endif // end of PHBOUND_BOUNDSURFACE_H
