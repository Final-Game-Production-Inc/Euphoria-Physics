//
// phbound/boundribbon.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDRIBBON_H
#define PHBOUND_BOUNDRIBBON_H

#include "bound.h"

#include "atl/array.h"
#include "curve/curve.h"
#include "phcore/materialmgr.h"

// phIntersection was moved to physics from phCore and phBoundRibbon is the only thing in phBound to reference it
// Rather than erase the functions just define them out so we can switch over to a non phIntersection interface later if we want.
#define PHBOUNDRIBBON_USE_PHINTERSECTION (0)

namespace rage {

#if USE_RIBBONS

class fiTokenizer;
class fiAsciiTokenizer;
class phBoundPolyhedron;
#if PHBOUNDRIBBON_USE_PHINTERSECTION
class phIntersection;
#endif // PHBOUNDRIBBON_USE_PHINTERSECTION


#define MAX_NUM_CURVES	12		// maximum for physdraw and exporting only, so no asserts on this

/*
PURPOSE
	A class to represent a (one-sided) physics bound that is defined by two splines.  It is also known as a 'ribbon' bound.
*/
class phBoundRibbon : public phBound
{
public:
	////////////////////////////////////////////////////////////
	phBoundRibbon ();
	virtual ~phBoundRibbon ();

	////////////////////////////////////////////////////////////
	// resources
	phBoundRibbon (datResource & rsc);										// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	void SetCurves (int numCurves, cvCurve<Vector3>** curves);
	void SetNumCurves (int numCurves);
	void SetCurve (int curveIndex, cvCurve<Vector3>* curve);
	void AddCurve (cvCurve<Vector3>* curve);

private:
	void CalculateSplittingTree();
	bool RetrieveSegmentIdx(int nVertexIdx, Vector3 const *const pavecVertices, int *const panSegmentIdx, int &rnSegmentIdx) const;
//	void RetrieveSplineData(const float kfApproxT, const int knVertexIdx, const Vector3 *const pavecVertices,
//		Vector3 *const pavecTangents, Vector3 *const pavecNormals, int *const panSegmentIdx, float *const pafDepths, Vector3 avecSplinePos[2]) const;
	void RetrieveSplineData(const float kfApproxT, const Vector3 &krvecVertex, Vector3 &rvecTangent, Vector3 &rvecNormal, 
		const int knSegmentIdx, float &rfDepth, Vector3 avecSplinePos[2]) const;
	bool CheckEdgeAgainstSlab(const Vector3 &krvecVertex0, const Vector3 &krvecVertex1, const int knSegmentIdx0, const int knSegmentIdx1, bool bPermitInteriorEdges) const;
	int LocatePoint(const Vector3 &rvecTestPoint) const;
	float CalculateApproxT(const Vector3 &rvecTestPoint, const int nSegmentIdx) const;

	bool InterpolateIsectPos (const float* pointDepth, const Vector3* pointPos, const int* pointCurve, const float* splineT,
								int index0, int index1, float* t, Vector3* position, Vector3* normal) const;
	float FindDepthUnderCurve (const Vector3 &rvecTestPoint, const Vector3 &rvecSplinePos0, const Vector3 &rvecSplinePos1, const Vector3 &rvecSplineDir0, Vector3 *pvecNormal) const;

public:
	virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) { SetPrimitiveMaterialId(materialId); }
	// <COMBINE phBound::GetMaterialIdFromPartIndex>
	phMaterialMgr::Id GetMaterialIdFromPartIndex (int UNUSED_PARAM(partIndex), int UNUSED_PARAM(component)=0) const { return GetMaterialId(0); }

	// array accessors
	cvCurve<Vector3>* GetSplineCurve (int curveIndex) const { return m_SplineCurve[curveIndex]; }

public:

	void ReadCurve (fiTokenizer& token, atArray<float>* floatArray);
	int GetNumCurves () const { return m_nNumCurves; }
	float GetInvNumSpaces () const { return m_fInvNumSpaces; }

	////////////////////////////////////////////////////////////
	// visualization
#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif // __PFDRAW

#if __ASSERT
	bool LegitNodeIndex (int nodeIndex) const { return (nodeIndex>=0 && nodeIndex<m_nNumNodes); }
#endif

protected:
	////////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);
#endif

	void CalculateExtents ();												// calculate the bounding sphere and box
#if 0
	void CalculateBoundingVolumes(float fT0, float fT1, int nX, int nY);
#endif
	void CalculateBoundingSphere(Vector3 &rvecSphereCenter, float &rfSphereRadius, float fT0, float fT1, Vector3 *pvecBoxMin, Vector3 *pvecBoxMax) const;
	void CalculateBoundingSlab(const Vector3 &krvecSlabCenter, Vector3 &rvecNormal, float &rfHalfThickness, float &rfOffset, const float kfT0, const float kfT1) const;
	void RedistributeTValues(float *pafTValues, const int knSlabCnt, const float *kpafSlabHalfThicknesses) const;
	void CalculateSlabsFromTValues(const float *kpafTValues, const int knSlabCnt, float *pafSlabHalfThicknesses, float *pafSlabOffsets, Vector3 *pavecSlabNormals) const;
	bool IsPolyhedronInBVHierarchy(const phBoundPolyhedron & polyBound, const Matrix34 & MS2WS) const;
	bool IsInBoundingSphereHierarchy(const Vector3 &rvecSphereCenter_WS, const float fSphereRadius, int nX, int nY, const Vector3 *const pavecBoxVertices) const;
#if PHBOUNDRIBBON_USE_PHINTERSECTION
	int TestEdgeIndexed(int nVertexIdx0, int nVertexIdx1, Vector3 const *const pavecVertices, Vector3 *const pavecNormals,
		float *const pafDepths, Vector3 *const pavecTangents, int *const panSegmentIdx, phIntersection* pISect, bool& aOutside, bool bCheckEdgeToEdge) const;
	bool TestProbeIndexed(int nVertexIdx0, int nVertexIdx1, Vector3 const *const pavecVertices,	Vector3 *const pavecNormals, 
		float *const pafDepths, Vector3 *const pavecTangents, int *const panSegmentIdx,	phIntersection *pISect, float fTValCeiling = 2.0f) const;
	int TestInteriorEdgeIndexed(int nVertexIdx0, int nVertexIdx1, Vector3 const *const pavecVertices, Vector3 *const pavecNormals,
		float *const pafDepths, Vector3 *const pavecTangents, int *const panSegmentIdx, phIntersection* pISect) const;
#endif // PHBOUNDRIBBON_USE_PHINTERSECTION
	bool FindIntersectionToMovingSphere (const Vector3 &rvecPos1_RS, const Vector3 &rvecPos2_RS, float fRadius, Vector3 &rvecHitPosRoad_WS,
		Vector3 &rvecHitPosSphere_WS, float &rfDepth, Vector3 &rvecNormal, int &rnNodeIndex) const;
	atArray <datRef <cvCurve <Vector3> > > m_SplineCurve;
	
	// The sphere tree is used because a single bounding sphere is a horrible approximation to the shape of a spline ribbon.
	// This sphere tree, along with the oriented disk tree below, is important because it allows us to reject entire bounds at a time.
	// It is worth it to have this somewhat complicated 
	datOwner <Vector3> m_pavecSphereCenters;
	datRef <float> m_pafSphereRadii;

	// Piggy backed on the sphere tree is an 'oriented disk' tree.  Whenever we find that an object's bounding sphere is within a given node
	//   in the sphere tree, we also check if the object's bounding box intersects with the corresponding 'oriented bounding disk' in this
	//   tree.  Only if it passes both tests do we continue further down the tree (or return true).
	datOwner <Vector3> m_pavecOBDNormals;
	datRef <float> m_pafOBDHalfThicknesses;
	datRef <float> m_pafOBDOffsets;

	// The splitting tree is a list of splitting planes used to obtain an initial approximation of the t-value of a point.
	// We do that by performing a binary search, using the planes of the splitting tree to decide the direction in which
	//   to look.
	// Also note that these arrays do not have any restrictions on size (ie, 2^n, 2^n -1, etc).
	// You might consider making this collection of arrays into an array of objects.  That might improve cache performance.
	datRef <float> m_pafSplittingTreeT;
	datOwner <Vector3> m_pavecSplittingTreeNormal;
	datOwner <Vector3> m_pavecSplittingTreePos;

	// As a further optimization, each road segment is further approximated by a 'slab' (a region bounded by two parallel
	//   planes.  This allows us to easily reject edges when both are outside of the slab to the same side.  It is important
	//   to easily and efficiently reject edges because doing the normal evaluation of an edge can be quite costly (it involves
	//   evaluating both curves at two different points, and curve evaluation is expensive).
	// You might consider making this collection of arrays into an array of objects.  That might improve cache performance.
	datOwner <Vector3> m_pavecSlabNormal;
	datRef <float> m_pafSlabHalfThickness;
	datRef <float> m_pafSlabOffset;

	int m_nNumCurves;
	int m_nNumNodes;
	float m_fInvNumSpaces;

	int m_nNumSegments;						// This is just one fewer than m_nSplittingTreeSize, so one of these should get removed later.
	int m_nSplittingTreeSize;

	
	int m_nSphereTreeDepth;					// Note that this count includes the root sphere, which is *NOT* represented in the sphere
	//   tree arrays below (it is located in phBound).
	int m_nSphereTreeSize;

	// PURPOSE: Pad to fill up the remaining bytes
	int pad[4];
};

#endif // USE_RIBBONS

} // namespace rage

#endif
