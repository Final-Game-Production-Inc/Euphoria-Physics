//
// phbound/boundribbon.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundribbon.h"

#include "boundcapsule.h"
#include "boundpolyhedron.h"
#include "boundsphere.h"

#include "curve/curve.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "file/token.h"
#include "math/simplemath.h"
#include "phcore/material.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "profile/profiler.h"
#include "string/string.h"
#include "system/alloca.h"
#include "system/timer.h"
#include "vector/geometry.h"

#if PHBOUNDRIBBON_USE_PHINTERSECTION
#include "physics/intersection.h"
#endif // PHBOUNDRIBBON_USE_PHINTERSECTION

namespace rage {

#if USE_RIBBONS

#define COLLIDEWITHEDGES 3

////////////////////////////////////////////////////////////////

phBoundRibbon::phBoundRibbon ()
{
	m_Type = RIBBON;
	m_nNumCurves = 0;
	m_nNumNodes = 0;

	m_RadiusAroundCentroid = 1.0f;
	phBound::SetCentroidOffset(Vec3V(V_ZERO));

	m_fInvNumSpaces = 1.0f;

	m_nSplittingTreeSize = -1;
	m_pafSplittingTreeT = NULL;
	m_pavecSplittingTreeNormal = NULL;
	m_pavecSplittingTreePos = NULL;

    m_pavecSlabNormal = NULL;

    m_pafSlabHalfThickness = NULL;
    m_pafSlabOffset = NULL;

	m_pavecSphereCenters = NULL;
	m_pafSphereRadii = NULL;

	m_pavecOBDNormals = NULL;
	m_pafOBDHalfThicknesses = NULL;
	m_pafOBDOffsets = NULL;
}


phBoundRibbon::~phBoundRibbon()
{
    // If there is no curve manager the curves were created here so delete them
    if (!CURVEMGR)
    {
        for ( int i=0; i<m_nNumCurves; i++ )
        {
            delete m_SplineCurve[i];
        }
    }
	m_SplineCurve.Reset();

	delete [] m_pafSphereRadii;
	m_pafSphereRadii = NULL;

	delete [] m_pavecSphereCenters;
	m_pavecSphereCenters = NULL;

	delete [] m_pafSplittingTreeT;
	m_pafSplittingTreeT = NULL;

	delete [] m_pavecSplittingTreeNormal;
	m_pavecSplittingTreeNormal = NULL;

	delete [] m_pavecSplittingTreePos;
	m_pavecSplittingTreePos = NULL;

	delete [] m_pavecSlabNormal;
	m_pavecSlabNormal = NULL;

	delete [] m_pafSlabHalfThickness;
	m_pafSlabHalfThickness = NULL;

	delete [] m_pafSlabOffset;
	m_pafSlabOffset = NULL;

	delete [] m_pavecOBDNormals;
	m_pavecOBDNormals = NULL;

	delete [] m_pafOBDHalfThicknesses;
	m_pafOBDHalfThicknesses = NULL;

	delete [] m_pafOBDOffsets;
	m_pafOBDOffsets = NULL;
}


void phBoundRibbon::SetCurves (int numCurves, cvCurve<Vector3>** curves)
{
	SetNumCurves(numCurves);
	for (int index=0;index<m_nNumCurves;index++)
	{
		SetCurve(index,curves[index]);
	}
}


void phBoundRibbon::SetNumCurves (int numCurves)
{
	m_nNumCurves = numCurves;
	m_SplineCurve.Resize(m_nNumCurves);
	for (int i=0; i<m_nNumCurves; i++)
		m_SplineCurve[i] = NULL;
}


void phBoundRibbon::SetCurve (int curveIndex, cvCurve<Vector3>* curve)
{
	m_SplineCurve[curveIndex] = curve;

	if(m_nNumNodes == 0)
	{
		// This is the first curve to be added, so get the number of nodes and the inverse number of spaces.
		m_nNumNodes = m_SplineCurve[curveIndex]->GetNumVertices();
		m_fInvNumSpaces = 1.0f/(float)(m_nNumNodes - 1);
	}
	else
	{
		// This is not the first curve to be added, so make sure it's got the right number of nodes.
		// Make sure all the curves have the same number of nodes.
		Assert(m_SplineCurve[curveIndex]->GetNumVertices() == m_nNumNodes);
	}
}


void phBoundRibbon::AddCurve (cvCurve<Vector3>* curve)
{
	if (!m_SplineCurve.GetCapacity())
	{
		SetNumCurves(MAX_NUM_CURVES);
		m_nNumCurves = 0;
	}
	Assert(m_nNumCurves<MAX_NUM_CURVES);
	SetCurve(m_nNumCurves,curve);
	m_nNumCurves++;
}


void phBoundRibbon::CalculateExtents ()
{
	Assert(m_SplineCurve.GetCapacity() && m_nNumCurves > 0);
	// We need to set the offset, m_RadiusAroundCentroid, m_RadiusAroundLocalOrigin, m_BoundingBoxMin, and m_BoundingBoxMax.
	Vec3V boundingBoxMin = Vec3V(V_FLT_MAX);
	Vec3V boundingBoxMax = Vec3V(V_NEG_FLT_MAX);

	// Calculate the top-level bounding sphere for the bound.
	Vector3 vecSphereCenter;
	CalculateBoundingSphere(vecSphereCenter, m_RadiusAroundCentroid, 0.0f, 1.0f, &RC_VECTOR3(boundingBoxMin), &RC_VECTOR3(boundingBoxMax));

	SetBoundingBoxMin(boundingBoxMin);
	SetBoundingBoxMax(boundingBoxMax);

	SetCentroidOffset(RCC_VEC3V(vecSphereCenter));

	m_nSphereTreeDepth = 0;
	while((1 << m_nSphereTreeDepth) < (4 * (m_nNumNodes - 1)))
	{
		++m_nSphereTreeDepth;
	}
	if(m_nSphereTreeDepth > 6)
	{
		if (phBound::MessagesEnabled())
		{
			Warningf("Road has %d vertices, resulting in a tree of depth %d.  This is too many (clamping to 6).", m_nNumNodes, m_nSphereTreeDepth);
		}

		m_nSphereTreeDepth = 6;
	}

	m_nSphereTreeSize = (1 << m_nSphereTreeDepth) - 2;
	m_pavecSphereCenters = rage_new Vector3[m_nSphereTreeSize];
	m_pafSphereRadii = rage_new float[m_nSphereTreeSize];

	m_pavecOBDNormals = rage_new Vector3[m_nSphereTreeSize];
	m_pafOBDHalfThicknesses = rage_new float[m_nSphereTreeSize];
	m_pafOBDOffsets = rage_new float[m_nSphereTreeSize];

	int nCurSphereTreeIdx = 0;

	const int knTValueCnt = (1 << (m_nSphereTreeDepth - 1)) + 1;
	float *pafTValues = Alloca(float, knTValueCnt);

	float *pafSlabThicknesses = Alloca(float, knTValueCnt);
	float *pafSlabOffsets = Alloca(float, knTValueCnt);
	Vector3 *pavecSlabNormals = Alloca(Vector3, knTValueCnt);

	// Let's initially give the t-values a uniform distribution.
	float fOOSphereTreeTWidth = 1.0f / (float)(knTValueCnt - 1);
	for(int nTValueIdx = 0; nTValueIdx < knTValueCnt; ++nTValueIdx)
	{
		pafTValues[nTValueIdx] = fOOSphereTreeTWidth * (float)(nTValueIdx);
	}

	// Let's calculate the slabs based on our current t-values.
	CalculateSlabsFromTValues(pafTValues, knTValueCnt - 1, pafSlabThicknesses, pafSlabOffsets, pavecSlabNormals);

	// Now we try to adjust the t-values to achieve a uniform thickness across all of the slabs.
	for(int nIterIdx = 0; nIterIdx < 20; ++nIterIdx)
	{
		// Calculated thicknesses can have a small absolute error in them.  However, when the value of the thickness is small, the 
		//   relative error becomes large enough and the redistribution will make adjustments that we really don't want it to make.
		//   Therefore, we bump up the thickness on really thin segments (when for the purpose of redistributing t-values).
		for(int nSlabIndex = 0; nSlabIndex < knTValueCnt - 1; ++nSlabIndex)
		{
			pafSlabThicknesses[nSlabIndex] = Max(pafSlabThicknesses[nSlabIndex], 0.00001f);
		}

		// Redistribute the t-values based upon our calculated slab thicknesses.
		RedistributeTValues(pafTValues, knTValueCnt - 1, pafSlabThicknesses);

		// Let's calculate the slabs based on our current t-values.
		CalculateSlabsFromTValues(pafTValues, knTValueCnt - 1, pafSlabThicknesses, pafSlabOffsets, pavecSlabNormals);
	}

	for(int nCurSphereTreeDepth = 1; nCurSphereTreeDepth < m_nSphereTreeDepth; ++nCurSphereTreeDepth)
	{
		int nStartingTValueIdx = 0;
		int nWidth = 1 << (m_nSphereTreeDepth - nCurSphereTreeDepth - 1);
		for(int nCurSphereTreeBreadth = 0; nCurSphereTreeBreadth < (1 << nCurSphereTreeDepth); ++nCurSphereTreeBreadth)
		{
			float fT0 = pafTValues[nStartingTValueIdx];
			float fT1 = pafTValues[nStartingTValueIdx + nWidth];
			nStartingTValueIdx += nWidth;

			CalculateBoundingSphere(m_pavecSphereCenters[nCurSphereTreeIdx], m_pafSphereRadii[nCurSphereTreeIdx], fT0, fT1, NULL, NULL);
			CalculateBoundingSlab(m_pavecSphereCenters[nCurSphereTreeIdx], m_pavecOBDNormals[nCurSphereTreeIdx], m_pafOBDHalfThicknesses[nCurSphereTreeIdx], m_pafOBDOffsets[nCurSphereTreeIdx], fT0, fT1);

			++nCurSphereTreeIdx;
		}
	}
}

#if 0
void phBoundRibbon::CalculateBoundingVolumes(float fT0, float fT1, int nX, int nY)
{
	// The goal of this function is to find a good t-value at which to split before recursing further down the tree.

	Assert(nY >= 0);

	// We start off by trying the middle.
	float fSplittingT = 0.5f * (fT0 + fT1);

	int nSphereTreeIdx0 = (1 << (nY + 1)) + (2 * nX) - 2;
	int nSphereTreeIdx1 = (1 << (nY + 1)) + (2 * nX) - 1;

	CalculateBoundingSphere(m_pavecSphereCenters[nSphereTreeIdx0], m_pafSphereRadii[nSphereTreeIdx0], fT0, fSplittingT, NULL, NULL);
	CalculateBoundingSlab(m_pavecSphereCenters[nSphereTreeIdx0], m_pavecOBDNormals[nSphereTreeIdx0], m_pafOBDHalfThicknesses[nSphereTreeIdx0], m_pafOBDOffsets[nSphereTreeIdx0], fT0, fSplittingT);
	CalculateBoundingSphere(m_pavecSphereCenters[nSphereTreeIdx1], m_pafSphereRadii[nSphereTreeIdx1], fSplittingT, fT1, NULL, NULL);
	CalculateBoundingSlab(m_pavecSphereCenters[nSphereTreeIdx1], m_pavecOBDNormals[nSphereTreeIdx1], m_pafOBDHalfThicknesses[nSphereTreeIdx1], m_pafOBDOffsets[nSphereTreeIdx1], fSplittingT, fT1);

	for(int nIterIdx = 0; nIterIdx < 10; ++nIterIdx)
	{
		if(m_pafOBDHalfThicknesses[nSphereTreeIdx0] > m_pafOBDHalfThicknesses[nSphereTreeIdx1])
		{
			// Slab 0 is thicker than slab 1.  Let's adjust our splitting t-value in the negative direction.
			float fUnitSlab0Thickness = m_pafOBDHalfThicknesses[nSphereTreeIdx0] / (m_pafOBDHalfThicknesses[nSphereTreeIdx0] + m_pafOBDHalfThicknesses[nSphereTreeIdx1]);
			float fUnitInterpolate = 0.5f / fUnitSlab0Thickness;
			fSplittingT = (1.0f - fUnitInterpolate) * fT0 + (fUnitInterpolate) * fSplittingT;
		}
		else
		{
			// Slab 1 is thicker than slab 0.  Let's adjust our splitting t-value in the positive direction.
			float fUnitSlab1Thickness = m_pafOBDHalfThicknesses[nSphereTreeIdx1] / (m_pafOBDHalfThicknesses[nSphereTreeIdx0] + m_pafOBDHalfThicknesses[nSphereTreeIdx1]);
			float fUnitInterpolate = (fUnitSlab1Thickness - 0.5f) / fUnitSlab1Thickness;
			fSplittingT = (1.0f - fUnitInterpolate) * fSplittingT + (fUnitInterpolate) * fT1;
		}

		CalculateBoundingSphere(m_pavecSphereCenters[nSphereTreeIdx0], m_pafSphereRadii[nSphereTreeIdx0], fT0, fSplittingT, NULL, NULL);
		CalculateBoundingSlab(m_pavecSphereCenters[nSphereTreeIdx0], m_pavecOBDNormals[nSphereTreeIdx0], m_pafOBDHalfThicknesses[nSphereTreeIdx0], m_pafOBDOffsets[nSphereTreeIdx0], fT0, fSplittingT);
		CalculateBoundingSphere(m_pavecSphereCenters[nSphereTreeIdx1], m_pafSphereRadii[nSphereTreeIdx1], fSplittingT, fT1, NULL, NULL);
		CalculateBoundingSlab(m_pavecSphereCenters[nSphereTreeIdx1], m_pavecOBDNormals[nSphereTreeIdx1], m_pafOBDHalfThicknesses[nSphereTreeIdx1], m_pafOBDOffsets[nSphereTreeIdx1], fSplittingT, fT1);
	}

	if(nY != m_nSphereTreeDepth - 2)
	{
		CalculateBoundingVolumes(fT0, fSplittingT, 2 * nX, nY + 1);
		CalculateBoundingVolumes(fSplittingT, fT1, 2 * nX + 1, nY + 1);
	}
}
#endif

void phBoundRibbon::CalculateBoundingSphere(Vector3 &rvecSphereCenter, float &rfSphereRadius, float fT0, float fT1, Vector3 *pvecBoxMin, Vector3 *pvecBoxMax) const
{
	if(pvecBoxMin != NULL)
	{
		pvecBoxMin->Set(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	}
	if(pvecBoxMax != NULL)
	{
		pvecBoxMax->Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	}

	// Here we iteratively calculate the bounding sphere for this segment.  We make it fast by making a good first guess
	//   at the bounding sphere and then iteratively adjusting it if it's not quite right.
	float fSphereRadiusSq;

	// These are the start and end points of the two splines defining this segment.
	Vector3 avecTestPoints[2][2];
	m_SplineCurve[0]->SolveSegmentSimple(avecTestPoints[0][0], -1, fT0, NULL, NULL);
	m_SplineCurve[0]->SolveSegmentSimple(avecTestPoints[0][1], -1, fT1, NULL, NULL);
	m_SplineCurve[1]->SolveSegmentSimple(avecTestPoints[1][0], -1, fT0, NULL, NULL);
	m_SplineCurve[1]->SolveSegmentSimple(avecTestPoints[1][1], -1, fT1, NULL, NULL);

	// To start, we pick the pair of end points that are separated by the largest distance.
	float fTempDiameterSq;
	fSphereRadiusSq = 0.25f * avecTestPoints[0][0].Dist2(avecTestPoints[0][1]);
	rvecSphereCenter.Average(avecTestPoints[0][0], avecTestPoints[0][1]);
	fTempDiameterSq = avecTestPoints[0][0].Dist2(avecTestPoints[1][1]);
	if(fTempDiameterSq > 4.0f * fSphereRadiusSq)
	{
		fSphereRadiusSq = 0.25f * fTempDiameterSq;
		rvecSphereCenter.Average(avecTestPoints[0][0], avecTestPoints[1][1]);
	}
	fTempDiameterSq = avecTestPoints[1][0].Dist2(avecTestPoints[0][1]);
	if(fTempDiameterSq > 4.0f * fSphereRadiusSq)
	{
		fSphereRadiusSq = 0.25f * fTempDiameterSq;
		rvecSphereCenter.Average(avecTestPoints[1][0], avecTestPoints[0][1]);
	}
	fTempDiameterSq = avecTestPoints[1][0].Dist2(avecTestPoints[1][1]);
	if(fTempDiameterSq > 4.0f * fSphereRadiusSq)
	{
		fSphereRadiusSq = 0.25f * fTempDiameterSq;
		rvecSphereCenter.Average(avecTestPoints[1][0], avecTestPoints[1][1]);
	}

	// rfSphereRadiusSq now holds the square of half the maximum distance between any two end points of the spline.
	// rvecSphereCenter now holds the midpoint of the two spline end points that were the farthest apart.

	// Now we will walk along both curves, and ensure that all of the points on the curve are within the current
	//   bounding sphere.
	Vector3 vecTestCurvePoint, vecCenterToTestPoint;
	for(int nTIdx = 0; nTIdx <= 10 * (m_nNumNodes - 1); ++nTIdx)
	{
		float fT = (float)(nTIdx) * 0.10f * m_fInvNumSpaces * (fT1 - fT0) + fT0;
		for(int nCurveIdx = 0; nCurveIdx < m_nNumCurves; ++nCurveIdx)
		{
			m_SplineCurve[nCurveIdx]->SolveSegmentSimple(vecTestCurvePoint, -1, fT, NULL, NULL);
			vecCenterToTestPoint.Subtract(vecTestCurvePoint, rvecSphereCenter);
			float fTempRadiusSq = vecCenterToTestPoint.Mag2();
			if(fTempRadiusSq > fSphereRadiusSq)
			{
				// This point is outside of the bounding sphere.
				// No biggie, we just need to adjust the bounding sphere.
				rvecSphereCenter.AddScaled(vecCenterToTestPoint, 0.5f * (1.0f - sqrtf(fSphereRadiusSq / fTempRadiusSq)));
				fSphereRadiusSq = rvecSphereCenter.Dist2(vecTestCurvePoint);
			}

			for(int nDim = 0; nDim < 3; ++nDim)
			{
				if((pvecBoxMin != NULL) && (vecTestCurvePoint[nDim] < (*pvecBoxMin)[nDim]))
				{
					(*pvecBoxMin)[nDim] = vecTestCurvePoint[nDim];
				}
				if((pvecBoxMax != NULL) && (vecTestCurvePoint[nDim] > (*pvecBoxMax)[nDim]))
				{
					(*pvecBoxMax)[nDim] = vecTestCurvePoint[nDim];
				}
			}
		}
	}

#if __ASSERT
	// At this point, we will sample the road to make sure that it is completely within the bounding sphere.
	for(float fT = fT0; fT <= fT1; fT += m_fInvNumSpaces * 0.01f * (fT1 - fT0))
	{
		for(int nCurveIdx = 0; nCurveIdx < m_nNumCurves; ++nCurveIdx)
		{
			m_SplineCurve[nCurveIdx]->SolveSegmentSimple(vecTestCurvePoint, -1, fT, NULL, NULL);
			vecCenterToTestPoint.Subtract(vecTestCurvePoint, rvecSphereCenter);
			float fTempRadiusSq = vecCenterToTestPoint.Mag2();
			Assert(fTempRadiusSq <= square(1.0001f) * fSphereRadiusSq);
		}
	}
#endif

	rfSphereRadius = sqrtf(fSphereRadiusSq);
}


void phBoundRibbon::CalculateBoundingSlab(const Vector3 &krvecSlabCenter, Vector3 &rvecNormal, float &rfHalfThickness, float &rfOffset, const float kfT0, const float kfT1) const
{
	Vector3 avecSpline1Pos[2], avecSpline2Pos[2];
	m_SplineCurve[0]->SolveSegmentSimple(avecSpline1Pos[0], -1, kfT0, NULL, NULL);
	m_SplineCurve[0]->SolveSegmentSimple(avecSpline1Pos[1], -1, kfT1, NULL, NULL);
	m_SplineCurve[1]->SolveSegmentSimple(avecSpline2Pos[0], -1, kfT0, NULL, NULL);
	m_SplineCurve[1]->SolveSegmentSimple(avecSpline2Pos[1], -1, kfT1, NULL, NULL);

	Vector3 avecDiagonals[2];
	avecDiagonals[0].Subtract(avecSpline2Pos[1], avecSpline1Pos[0]);
	avecDiagonals[1].Subtract(avecSpline2Pos[0], avecSpline1Pos[1]);
	rvecNormal.Cross(avecDiagonals[1], avecDiagonals[0]);
	rvecNormal.Normalize();

	float afSlabExtents[2] = { 0.0f, 0.0f };
	Vector3 vecPointToSlabOrigin;
	for(float fT = kfT0; fT <= kfT1; fT += m_fInvNumSpaces * 0.25f * (kfT1 - kfT0))
	{
		float fDistToPlane;

		m_SplineCurve[0]->SolveSegmentSimple(vecPointToSlabOrigin, -1, fT, NULL, NULL);
		vecPointToSlabOrigin.Subtract(krvecSlabCenter);
		fDistToPlane = vecPointToSlabOrigin.Dot(rvecNormal);
		afSlabExtents[0] = Min(fDistToPlane, afSlabExtents[0]);
		afSlabExtents[1] = Max(fDistToPlane, afSlabExtents[1]);

		m_SplineCurve[1]->SolveSegmentSimple(vecPointToSlabOrigin, -1, fT, NULL, NULL);
		vecPointToSlabOrigin.Subtract(krvecSlabCenter);
		fDistToPlane = vecPointToSlabOrigin.Dot(rvecNormal);
		afSlabExtents[0] = Min(fDistToPlane, afSlabExtents[0]);
		afSlabExtents[1] = Max(fDistToPlane, afSlabExtents[1]);
	}

	rfHalfThickness = 0.5f * (afSlabExtents[1] - afSlabExtents[0]);
	rfOffset = 0.5f * (afSlabExtents[1] + afSlabExtents[0]);
}


// This function should be given a more generic name (and more generic parameter names).
void phBoundRibbon::RedistributeTValues(float *pafTValues, const int knSlabCnt, const float *kpafSlabHalfThicknesses) const
{
	float *pafDeltaT = Alloca(float, knSlabCnt + 1);
	pafDeltaT[0] = 0.0f;

	// First, let's find the sum of the thicknesses of all of the slabs.
	float fTotal = 0.0f;
	for(int nSlabIdx = 0; nSlabIdx < knSlabCnt; ++nSlabIdx)
	{
		fTotal += kpafSlabHalfThicknesses[nSlabIdx];
	}

	// Now, we'll try to evenly distribute that thickness across all of the slabs.
	int nCurSlabIdx = 0;
	float fLastTotal = 0.0f;
	float fCurTotal = kpafSlabHalfThicknesses[0];
	Assert(pafTValues[0] == 0.0f);
	Assert(pafTValues[knSlabCnt] == 1.0f);
	int nTValueIdx;
	for(nTValueIdx = 1; nTValueIdx < knSlabCnt; ++nTValueIdx)
	{
		// TODO: This division could be calculated once.
		float fNextTotal = (float)(nTValueIdx) * fTotal * (1.0f / (float)(knSlabCnt));

		while(fNextTotal > fCurTotal)
		{
			++nCurSlabIdx;
			fLastTotal = fCurTotal;
			fCurTotal += (kpafSlabHalfThicknesses[nCurSlabIdx]);
		}

		float fUnitInterpolate = (fNextTotal - fLastTotal) / (fCurTotal - fLastTotal);
		Assert(fUnitInterpolate >= 0.0f);
		Assert(fUnitInterpolate <= 1.0f);
		float fNewT = pafTValues[nCurSlabIdx] + (fUnitInterpolate) * (pafTValues[nCurSlabIdx + 1] - pafTValues[nCurSlabIdx]);
		Assert(fNewT > pafTValues[nTValueIdx - 1] + pafDeltaT[nTValueIdx - 1]);
		pafDeltaT[nTValueIdx] = fNewT - pafTValues[nTValueIdx];
	}

	// Now we actually apply the changes that we just calculated.
	for(nTValueIdx = 1; nTValueIdx < knSlabCnt; ++nTValueIdx)
	{
		pafTValues[nTValueIdx] += pafDeltaT[nTValueIdx];

		// Ensure that we have a strictly increasing sequence.
		Assert(pafTValues[nTValueIdx] > pafTValues[nTValueIdx - 1]);
	}
}

// 
void phBoundRibbon::CalculateSlabsFromTValues(const float *kpafTValues, const int knSlabCnt, float *pafSlabHalfThicknesses, float *pafSlabOffsets, Vector3 *pavecSlabNormals) const
{
	Vector3 avecSpline1Pos[2];
	Vector3 avecSpline2Pos[2];
	Vector3 avecDiagonals[2];
	Vector3 vecTemp;

	m_SplineCurve[0]->SolveSegmentSimple(avecSpline1Pos[1], -1, 0.0f, NULL, NULL);
	m_SplineCurve[1]->SolveSegmentSimple(avecSpline2Pos[1], -1, 0.0f, NULL, NULL);
	for(int nSlabIdx = 0; nSlabIdx < knSlabCnt; ++nSlabIdx)
	{
		avecSpline1Pos[0].Set(avecSpline1Pos[1]);
		avecSpline2Pos[0].Set(avecSpline2Pos[1]);

		m_SplineCurve[0]->SolveSegmentSimple(avecSpline1Pos[1], -1, kpafTValues[nSlabIdx + 1], NULL, NULL);
		m_SplineCurve[1]->SolveSegmentSimple(avecSpline2Pos[1], -1, kpafTValues[nSlabIdx + 1], NULL, NULL);

		avecDiagonals[0].Subtract(avecSpline2Pos[1], avecSpline1Pos[0]);
		avecDiagonals[1].Subtract(avecSpline2Pos[0], avecSpline1Pos[1]);

		pavecSlabNormals[nSlabIdx].Cross(avecDiagonals[1], avecDiagonals[0]);
		pavecSlabNormals[nSlabIdx].Normalize();

		vecTemp.Subtract(avecSpline1Pos[1], avecSpline1Pos[0]);

		float afSlabExtents[2] = { 0.0f, 0.0f };		// 0 is the minimum, 1 is the maximum.

		float fDistToPlane = vecTemp.Dot(pavecSlabNormals[nSlabIdx]);
		afSlabExtents[0] = Min(fDistToPlane, afSlabExtents[0]);
		afSlabExtents[1] = Max(fDistToPlane, afSlabExtents[1]);

		// Let's check some points along the curves to see if they're displaced more.  If they are, we'll adjust our slab thickness.
		for(float fUnitInterpolate = 0.1f; fUnitInterpolate < 1.0f; fUnitInterpolate += 0.1f)
		{
			for(int nCurveIdx = 0; nCurveIdx < 2; ++nCurveIdx)
			{
				m_SplineCurve[nCurveIdx]->SolveSegmentSimple(vecTemp, -1, Lerp(fUnitInterpolate, kpafTValues[nSlabIdx] ,kpafTValues[nSlabIdx + 1]), NULL, NULL);
				vecTemp.Subtract(avecSpline1Pos[0]);

				fDistToPlane = vecTemp.Dot(pavecSlabNormals[nSlabIdx]);
				afSlabExtents[0] = Min(1.001f * fDistToPlane, afSlabExtents[0]);
				afSlabExtents[1] = Max(1.001f * fDistToPlane, afSlabExtents[1]);
			}
		}

		pafSlabHalfThicknesses[nSlabIdx] = /*1.05f * */0.5f * (afSlabExtents[1] - afSlabExtents[0]);

		pafSlabOffsets[nSlabIdx] = 0.5f * (afSlabExtents[1] + afSlabExtents[0]);
	}
}


bool phBoundRibbon::IsInBoundingSphereHierarchy(const Vector3 &rvecSphereCenter, const float fSphereRadius, int nX, int nY, const Vector3 *const pavecBoxVertices) const
{
	// This could be implemented non-recursively.  However, improvements to this function would have the most effect in the cases
	//   where 
	Assert(nY > 0);

	int nSphereTreeIdx = (1 << nY) + nX - 2;		// The extra -1 is because we don't keep the top level bounding sphere in our tree (it's in
													//	phBound).

	Vector3 vecSphereToSlab;
	vecSphereToSlab.Subtract(rvecSphereCenter, m_pavecSphereCenters[nSphereTreeIdx]);
	if(vecSphereToSlab.Mag2() <= square(fSphereRadius + m_pafSphereRadii[nSphereTreeIdx]))
	{
		// We're inside the bounding sphere for this node.
		
		// We'll bail if the sphere doesn't intersect with the bounding disk for this node.
		float fDistToSlab = vecSphereToSlab.Dot(m_pavecOBDNormals[nSphereTreeIdx]) - m_pafOBDOffsets[nSphereTreeIdx];
		if(square(fDistToSlab) > square(fSphereRadius + m_pafOBDHalfThicknesses[nSphereTreeIdx]))
		{
			// The bounding sphere that we're testing against does not intersect with the bounding disk for this node.
			return false;
		}

		// We'll also bail if they supplied a bounding box and the bounding box doesn't intersect with the bounding disk for
		//   this node.
		if(pavecBoxVertices != NULL)
		{
			Vector3 vecSlabToVertex;
			vecSlabToVertex.Subtract(pavecBoxVertices[0], m_pavecSphereCenters[nSphereTreeIdx]);

			bool bIsIntersecting = false;

			int nFirstVertexIdx = 0;
			float fFirstDistToSlab = vecSlabToVertex.Dot(m_pavecOBDNormals[nSphereTreeIdx]) - m_pafOBDOffsets[nSphereTreeIdx];
			bIsIntersecting = square(fFirstDistToSlab) < square(m_pafOBDHalfThicknesses[nSphereTreeIdx]);
			for(int nVertexIdx = nFirstVertexIdx; !bIsIntersecting && nVertexIdx < 8; ++nVertexIdx)
			{
				vecSlabToVertex.Subtract(pavecBoxVertices[nVertexIdx], m_pavecSphereCenters[nSphereTreeIdx]);

				float fDistToSlab = vecSlabToVertex.Dot(m_pavecOBDNormals[nSphereTreeIdx]) - m_pafOBDOffsets[nSphereTreeIdx];
				bIsIntersecting = square(fDistToSlab) < square(m_pafOBDHalfThicknesses[nSphereTreeIdx]) || !SameSign(fDistToSlab, fFirstDistToSlab);
			}

			if(!bIsIntersecting)
			{
				return false;
			}
		}

		if(nY != m_nSphereTreeDepth - 1)
		{
			// We're not at a leaf node yet, so let's check our two children.
			if(IsInBoundingSphereHierarchy(rvecSphereCenter, fSphereRadius, 2 * nX, nY + 1, pavecBoxVertices))
			{
				return true;
			}
			if(IsInBoundingSphereHierarchy(rvecSphereCenter, fSphereRadius, 2 * nX + 1, nY + 1, pavecBoxVertices))
			{
				return true;
			}
		}
		else
		{
			// We're at a leaf node, so we know we're good.
			return true;
		}
	}

	return false;
}


#define I_AM_JUSTIN 0
#if I_AM_JUSTIN
static float sfFindImpactsToPolyTimer = 0.0f;
static float sfBoundRejectionTimer = 0.0f;
static float sfTotalLocatePointTimer = 0.0f;
static float sfSolveSegmentTimer = 0.0f;
static int snFindImpactsToPolyCtr = 0;
static int snBoundingSphereRejectionCtr = 0;
static int snFalsePositivesCtr = 0;

void CheckFindImpactsToPolyTiming()
{
	if(snFindImpactsToPolyCtr >= 1000)
	{
		if (phBound::MessagesEnabled())
		{
			Displayf("%f", sfFindImpactsToPolyTimer / (float)(snFindImpactsToPolyCtr));
			Displayf("%f, %f, %f", sfBoundRejectionTimer / sfFindImpactsToPolyTimer, sfTotalLocatePointTimer / sfFindImpactsToPolyTimer, sfSolveSegmentTimer / sfFindImpactsToPolyTimer);
			Displayf("%d / %d / %d / %d", snBoundingSphereRejectionCtr, snFalsePositivesCtr, snFindImpactsToPolyCtr - snBoundingSphereRejectionCtr - snFalsePositivesCtr, snFindImpactsToPolyCtr);
		}

		sfFindImpactsToPolyTimer = 0.0f;
		sfBoundRejectionTimer = 0.0f;
		sfTotalLocatePointTimer = 0.0f;
		sfSolveSegmentTimer = 0.0f;
		snFindImpactsToPolyCtr = 0;
		snBoundingSphereRejectionCtr = 0;
		snFalsePositivesCtr = 0;
	}
}
#endif


enum eSegmentIdxFlags
{
	SEGIDXFLAG_IDXMASK			=	(1 << 10) - 1,
	SEGIDXFLAG_FLAGMASK			=	3 << 10,

	SEGIDXFLAG_UNCHECKED		=	0 << 10,		// We haven't performed any checks with respect to this point.
	SEGIDXFLAG_LOCATED			=	1 << 10,		// The point has been located, but the edge to which it was connected was rejected by the slab test.
													//   This does not mean that the point is invalid.
	SEGIDXFLAG_VALID			=	2 << 10,		// The point has been located and verified as valid.
	SEGIDXFLAG_INVALID			=	3 << 10,		// The point is not on the road for whatever reason.

	SEGIDXFLAG_VERTEXHANDLED	=	1 << 12,		// This vertex has been handled, one way or another.
	SEGIDXFLAG_PENETRATINGEDGE	=	1 << 13,		// The vertex was found to be the deeper vertex of a penetrating real edge.
};



enum ePointBetween
{
	PB_BEHIND0				= 0,
	PB_BEHIND1				= 1,
	PB_BETWEEN				= 2,
};


inline ePointBetween IsPointBetween(const Vector3 &krvecTestPoint, const Vector3 &krvecPoint0, const Vector3 &krvecPoint1, float &rfNumerator, float &rfDenominator)
{
	Vector3 vecPoint0ToPoint1;
	vecPoint0ToPoint1.Subtract(krvecPoint1, krvecPoint0);

	rfDenominator = vecPoint0ToPoint1.Mag2();

	Vector3 vecTemp;
	vecTemp.Subtract(krvecTestPoint, krvecPoint0);
	rfNumerator = vecTemp.Dot(vecPoint0ToPoint1);
	if(rfNumerator < 0.0f)
	{
		return PB_BEHIND0;
	}
	else if(rfNumerator > rfDenominator)
	{
		return PB_BEHIND1;
	}
	else
	{
		return PB_BETWEEN;
	}
}

#if PHBOUNDRIBBON_USE_PHINTERSECTION
bool CheckEdgeToEdge(const float kfPreviousDepth, const Vector3 &krvecVertex0, const Vector3 &krvecVertex1, const Vector3 &krvecSplinePosT0, const Vector3 &krvecSplinePosT1, const float UNUSED_PARAM(kfDepth0), phIntersection *pIsect)
{
	Vector3 vecPolyEdge;
	vecPolyEdge.Subtract(krvecVertex1, krvecVertex0);

	Vector3 vecRibbonEdge;
	vecRibbonEdge.Subtract(krvecSplinePosT1, krvecSplinePosT0);
	Vector3 vecNormal;
	vecNormal.Cross(vecRibbonEdge, vecPolyEdge);
	vecNormal.Normalize();
	Vector3 vecPolyEdgeToRibbonEdge;
	vecPolyEdgeToRibbonEdge.Subtract(krvecSplinePosT0, krvecVertex0);
	float fDepth = vecPolyEdgeToRibbonEdge.Dot(vecNormal);
	if(fDepth < 0.0f)
	{
		vecNormal.Negate();
		fDepth = -fDepth;
	}

	if(fDepth >= kfPreviousDepth)
	{
		return false;
	}

	// The edge to edge intersection is going to give us better results.
	// We now have the depth and the normal, so all we need is the intersection point and the t-value.
	Vector3 vecBlah;
	vecBlah.Cross(vecNormal, vecRibbonEdge);
	Vector3 vecRibbonPolyEdgeT0PolyEdgeT0, vecRibbonPolyEdgeT0PolyEdgeT1;
	vecRibbonPolyEdgeT0PolyEdgeT0.Subtract(krvecVertex0, krvecSplinePosT0);
	vecRibbonPolyEdgeT0PolyEdgeT1.Subtract(krvecVertex1, krvecSplinePosT0);

	float afT[2];
	afT[0] = vecRibbonPolyEdgeT0PolyEdgeT0.Dot(vecBlah);
	afT[1] = vecRibbonPolyEdgeT0PolyEdgeT1.Dot(vecBlah);
	if(afT[0] * afT[1] > 0.0f)
	{
		return false;
	}
	float fTValue = (0.0f - afT[0]) / (afT[1] - afT[0]);
	Vector3 vecApproxIsectPoint;
	vecApproxIsectPoint.Lerp(fTValue, krvecVertex0, krvecVertex1);
	pIsect->Set(RCC_VEC3V(vecApproxIsectPoint), RCC_VEC3V(vecNormal), fTValue, fDepth, 0, (u16)(BAD_INDEX));
	return true;
}


// This function tests an edge for intersection with the road.
int phBoundRibbon::TestEdgeIndexed(int nVertexIdx0, int nVertexIdx1, Vector3 const *const pavecVertices, 
						  Vector3 *const pavecNormals, float *const pafDepths, Vector3 *const pavecTangents, int *const panSegmentIdx, 
						  phIntersection* pISect, bool& aOutside, bool bCheckEdgeToEdge) const
{
	if((panSegmentIdx[nVertexIdx0] & SEGIDXFLAG_FLAGMASK) == SEGIDXFLAG_INVALID || (panSegmentIdx[nVertexIdx1] & SEGIDXFLAG_FLAGMASK) == SEGIDXFLAG_INVALID)
	{
		// We've already tried to find at least one of these vertices before and it wasn't valid, so we
		//   reject this whole edge.
		return 0;
	}
	int anSegmentIdx[2];// = { -1, -1 };
	float afApproxT[2];

	const Vector3 &krvecVertex0 = pavecVertices[nVertexIdx0];
	const Vector3 &krvecVertex1 = pavecVertices[nVertexIdx1];

	if(!RetrieveSegmentIdx(nVertexIdx0, pavecVertices, panSegmentIdx, anSegmentIdx[0]))
	{
		return 0;
	}
	if(!RetrieveSegmentIdx(nVertexIdx1, pavecVertices, panSegmentIdx, anSegmentIdx[1]))
	{
		return 0;
	}

	if(!CheckEdgeAgainstSlab(krvecVertex0, krvecVertex1, anSegmentIdx[0], anSegmentIdx[1], false))
	{
		return 0;
	}

	Vector3 avecSplinePosT0[2];
	Vector3 avecSplinePosT1[2];

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// We've located the segment in which each endpoint lies.  Let's approximate the t-values for the points, and 
	//   then get the data from the curves for those points.
	afApproxT[0] = CalculateApproxT(krvecVertex0, anSegmentIdx[0]);
	RetrieveSplineData(afApproxT[0], krvecVertex0, pavecTangents[nVertexIdx0], pavecNormals[nVertexIdx0], anSegmentIdx[0], pafDepths[nVertexIdx0], avecSplinePosT0);

	afApproxT[1] = CalculateApproxT(krvecVertex1, anSegmentIdx[1]);
	RetrieveSplineData(afApproxT[1], krvecVertex1, pavecTangents[nVertexIdx1], pavecNormals[nVertexIdx1], anSegmentIdx[1], pafDepths[nVertexIdx1], avecSplinePosT1);
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if(pafDepths[nVertexIdx0] * pafDepths[nVertexIdx1] <= 0.0f)
	{
		// One of the end points is above the surface, and one of them is below.  Let's check for a possible intersection.
		ePointBetween aeIsInside[2];
		float afNumerator[2];
		float afDenominator[2];
		aeIsInside[0] = IsPointBetween(krvecVertex0, avecSplinePosT0[0], avecSplinePosT0[1], afNumerator[0], afDenominator[0]);
		aeIsInside[1] = IsPointBetween(krvecVertex1, avecSplinePosT1[0], avecSplinePosT1[1], afNumerator[1], afDenominator[1]);

		int nLowerEdgeIdx = pafDepths[nVertexIdx0] > pafDepths[nVertexIdx1] ? 0 : 1;
		int anVertexIdx[2]; // 0 is the lower vertex, 1 is the upper vertex. // = pafDepths[nVertexIdx0] > 0.0f ? nVertexIdx0 : nVertexIdx1;
		anVertexIdx[0] = pafDepths[nVertexIdx0] > pafDepths[nVertexIdx1] ? nVertexIdx0 : nVertexIdx1;
		anVertexIdx[1] = pafDepths[nVertexIdx0] > pafDepths[nVertexIdx1] ? nVertexIdx1 : nVertexIdx0;

		Assert(pafDepths[anVertexIdx[0]] >= 0.0f);
		Assert(pafDepths[anVertexIdx[1]] <= 0.0f);

		float fFinalDepth;
		float fUnitInterpolate;
		Vector3 vecApproxIsectPoint;
		Vector3 vecFinalNormal;

		if((aeIsInside[0] == PB_BETWEEN) && (aeIsInside[1] == PB_BETWEEN))
		{
			// Both points are inside of the bound, so we'll assume that our penetration point is also inside of the bound.
			// That means that we can do the easy calculation.
			fFinalDepth = pafDepths[anVertexIdx[0]];
			vecFinalNormal.Set(pavecNormals[anVertexIdx[0]]);
			if(nLowerEdgeIdx == 0)
			{
				fUnitInterpolate = 0.0f;
				vecApproxIsectPoint.Set(krvecVertex0);
			}
			else
			{
				fUnitInterpolate = 1.0f;
				vecApproxIsectPoint.Set(krvecVertex1);
			}


			// At this point, we could also consider an edge to edge intersection.
			if(bCheckEdgeToEdge)
			{
				int nSplineEdgeIdx = afNumerator[nLowerEdgeIdx] / afDenominator[nLowerEdgeIdx] < afNumerator[1 - nLowerEdgeIdx] / afDenominator[1 - nLowerEdgeIdx] ? 0 : 1;
				if(CheckEdgeToEdge(fFinalDepth, krvecVertex0, krvecVertex1, avecSplinePosT0[nSplineEdgeIdx], avecSplinePosT1[nSplineEdgeIdx], pafDepths[nVertexIdx0], pISect))
				{
					return 1;
				}
			}
		}
		else
		{
			// 
			if(aeIsInside[1 - nLowerEdgeIdx] == aeIsInside[nLowerEdgeIdx])
			{
				// Both end points of the edge are outside of the bound, to the same side.
				return 0;
			}

			if(aeIsInside[nLowerEdgeIdx] != PB_BETWEEN)
			{
				// The lower end of the edge is outside of the bound - let's calculate the depth of the edge at the edge of the bound.
				float fEdgeToClipTo = aeIsInside[nLowerEdgeIdx] == PB_BEHIND0 ? 0.0f : 1.0f;

				float afUnitCross[2] = { afNumerator[0] / afDenominator[0], afNumerator[1] / afDenominator[1] };
				float fUnitClipInterpolate = (fEdgeToClipTo - afUnitCross[0]) / (afUnitCross[1] - afUnitCross[0]);
				Assert(fUnitClipInterpolate >= 0.0f);
				Assert(fUnitClipInterpolate <= 1.0f);
				fFinalDepth = (1.0f - fUnitClipInterpolate) * (pafDepths[nVertexIdx0]) + (fUnitClipInterpolate) * (pafDepths[nVertexIdx1]);
				if(fFinalDepth <= 0.0f)
				{
					// The edge is still above the bound at the edge of the bound, so it doesn't intersect.
					return 0;
				}

				// At this point, we could also consider an edge to edge intersection.
				if(bCheckEdgeToEdge)
				{
					int nSplineEdgeIdx = aeIsInside[nLowerEdgeIdx] == PB_BEHIND0 ? 0 : 1;
					if(CheckEdgeToEdge(fFinalDepth, krvecVertex0, krvecVertex1, avecSplinePosT0[nSplineEdgeIdx], avecSplinePosT1[nSplineEdgeIdx], pafDepths[nVertexIdx0], pISect))
					{
						return 1;
					}
				}

				fUnitInterpolate = fUnitClipInterpolate;
				vecFinalNormal.Lerp(fUnitInterpolate, pavecNormals[nVertexIdx0], pavecNormals[nVertexIdx1]);
				vecFinalNormal.Normalize();
				vecApproxIsectPoint.Lerp(fUnitInterpolate, pavecVertices[nVertexIdx0], pavecVertices[nVertexIdx1]);
			}
			else
			{
				Assert(aeIsInside[1 - nLowerEdgeIdx] != PB_BETWEEN);

				// The upper end of the edge is outside of the bound - let's calculate the depth of the edge at the edge of the bound.
				float fEdgeToClipTo = aeIsInside[1 - nLowerEdgeIdx] == PB_BEHIND0 ? 0.0f : 1.0f;

				float fN = afNumerator[0] / afDenominator[0];
				float fP = afNumerator[1] / afDenominator[1];
				float fUnitClipInterpolate = (fEdgeToClipTo - fN) / (fP - fN);
				Assert(fUnitClipInterpolate >= 0.0f);
				Assert(fUnitClipInterpolate <= 1.0f);
				float fClipDepth = (1.0f - fUnitClipInterpolate) * (pafDepths[nVertexIdx0]) + (fUnitClipInterpolate) * (pafDepths[nVertexIdx1]);
				if(fClipDepth >= 0.0f)
				{
					// The edge is still below the bound at the edge of the bound, so it doesn't intersect.
					return 0;
				}

				fFinalDepth = pafDepths[anVertexIdx[0]];

				// At this point, we could also consider an edge to edge intersection.
				if(bCheckEdgeToEdge)
				{
					int nSplineEdgeIdx = aeIsInside[1 - nLowerEdgeIdx] == PB_BEHIND0 ? 1 : 0;
					if(CheckEdgeToEdge(fFinalDepth, krvecVertex0, krvecVertex1, avecSplinePosT0[nSplineEdgeIdx], avecSplinePosT1[nSplineEdgeIdx], pafDepths[nVertexIdx0], pISect))
					{
						return 1;
					}
				}

				vecFinalNormal.Set(pavecNormals[anVertexIdx[0]]);
				if(nLowerEdgeIdx == 0)
				{
					fUnitInterpolate = 0.0f;
					vecApproxIsectPoint.Set(krvecVertex0);
				}
				else
				{
					fUnitInterpolate = 1.0f;
					vecApproxIsectPoint.Set(krvecVertex1);
				}
			}
		}

		Assert(fFinalDepth >= 0.0f);
		aOutside = pafDepths[nVertexIdx1] > 0.0f;
		pISect->Set(RCC_VEC3V(vecApproxIsectPoint), RCC_VEC3V(vecFinalNormal), fUnitInterpolate, fFinalDepth, 0, (u16)(BAD_INDEX), GetPrimitiveMaterialId());
		return 1;
	}

	return 0;
}


bool phBoundRibbon::TestProbeIndexed(int nVertexIdx0, int nVertexIdx1, Vector3 const *const pavecVertices, 
								   Vector3 *const pavecNormals, float *const pafDepths, Vector3 *const pavecTangents, int *const panSegmentIdx,
								   phIntersection * pISect, float UNUSED_PARAM(fTValCeiling)) const
{
	bool aOutside;
	return((TestEdgeIndexed(nVertexIdx0, nVertexIdx1, pavecVertices, pavecNormals, pafDepths, pavecTangents, panSegmentIdx,
		pISect,aOutside, false) == 1) && aOutside);
}


int phBoundRibbon::TestInteriorEdgeIndexed(int nVertexIdx0, int nVertexIdx1, Vector3 const *const pavecVertices, 
								 Vector3 *const pavecNormals, float *const pafDepths, Vector3 *const pavecTangents, int *const panSegmentIdx, 
								 phIntersection* pISect) const
{
	if((panSegmentIdx[nVertexIdx0] & SEGIDXFLAG_FLAGMASK) == SEGIDXFLAG_INVALID || (panSegmentIdx[nVertexIdx1] & SEGIDXFLAG_FLAGMASK) == SEGIDXFLAG_INVALID)
	{
		// We've already tried to find at least one of these vertices before and it wasn't valid, so we
		//   reject this whole edge.
		return 0;
	}
	int anSegmentIdx[2];// = { -1, -1 };
	float afApproxT[2];

	const Vector3 &krvecVertex0 = pavecVertices[nVertexIdx0];
	const Vector3 &krvecVertex1 = pavecVertices[nVertexIdx1];

	if(!RetrieveSegmentIdx(nVertexIdx0, pavecVertices, panSegmentIdx, anSegmentIdx[0]))
	{
		return 0;
	}
	if(!RetrieveSegmentIdx(nVertexIdx1, pavecVertices, panSegmentIdx, anSegmentIdx[1]))
	{
		return 0;
	}

	if(!CheckEdgeAgainstSlab(krvecVertex0, krvecVertex1, anSegmentIdx[0], anSegmentIdx[1], true))
	{
		return 0;
	}

	Vector3 avecSplinePosT0[2];
	Vector3 avecSplinePosT1[2];

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// We've located the segment in which each endpoint lies.  Let's approximate the t-values for the points, and 
	//   then get the data from the curves for those points.
	afApproxT[0] = CalculateApproxT(krvecVertex0, anSegmentIdx[0]);
	RetrieveSplineData(afApproxT[0], krvecVertex0, pavecTangents[nVertexIdx0], pavecNormals[nVertexIdx0], anSegmentIdx[0], pafDepths[nVertexIdx0], avecSplinePosT0);
	if(pafDepths[nVertexIdx0] < 0.0f)
	{
		// This vertex is above the bound, and hence this edge can't be an interior edge.
		return 0;
	}

	afApproxT[1] = CalculateApproxT(krvecVertex1, anSegmentIdx[1]);
	RetrieveSplineData(afApproxT[1], krvecVertex1, pavecTangents[nVertexIdx1], pavecNormals[nVertexIdx1], anSegmentIdx[1], pafDepths[nVertexIdx1], avecSplinePosT1);
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if(pafDepths[nVertexIdx1] >= 0.0f)
	{
		// Both of the end points are above the surface.  Let's see which vertices, if any, are outside of the bound.
		ePointBetween aeIsInside[2];
		float afNumerator[2];
		float afDenominator[2];
		aeIsInside[0] = IsPointBetween(krvecVertex0, avecSplinePosT0[0], avecSplinePosT0[1], afNumerator[0], afDenominator[0]);
		aeIsInside[1] = IsPointBetween(krvecVertex1, avecSplinePosT1[0], avecSplinePosT1[1], afNumerator[1], afDenominator[1]);

		int nLowerEdgeIdx = pafDepths[nVertexIdx0] > pafDepths[nVertexIdx1] ? 0 : 1;
		int anVertexIdx[2]; // 0 is the lower vertex, 1 is the upper vertex. // = pafDepths[nVertexIdx0] > 0.0f ? nVertexIdx0 : nVertexIdx1;
		anVertexIdx[0] = pafDepths[nVertexIdx0] > pafDepths[nVertexIdx1] ? nVertexIdx0 : nVertexIdx1;
		anVertexIdx[1] = pafDepths[nVertexIdx0] > pafDepths[nVertexIdx1] ? nVertexIdx1 : nVertexIdx0;

		Assert(pafDepths[anVertexIdx[0]] >= 0.0f);
		Assert(pafDepths[anVertexIdx[1]] >= 0.0f);

		float fFinalDepth;
		float fUnitInterpolate;
		Vector3 vecApproxIsectPoint;
		Vector3 vecFinalNormal;

		if((aeIsInside[0] == PB_BETWEEN) && (aeIsInside[1] == PB_BETWEEN))
		{
			// Both points are inside of the bound, so we'll assume that our penetration point is also inside of the bound.
			// That means that we can do the easy calculation.
			fFinalDepth = pafDepths[anVertexIdx[0]];
			vecFinalNormal.Set(pavecNormals[anVertexIdx[0]]);
			if(nLowerEdgeIdx == 0)
			{
				fUnitInterpolate = 0.0f;
				vecApproxIsectPoint.Set(krvecVertex0);
			}
			else
			{
				fUnitInterpolate = 1.0f;
				vecApproxIsectPoint.Set(krvecVertex1);
			}

			// At this point, we could also consider an edge to edge intersection.
			int nSplineEdgeIdx = afNumerator[nLowerEdgeIdx] / afDenominator[nLowerEdgeIdx] < afNumerator[1 - nLowerEdgeIdx] / afDenominator[1 - nLowerEdgeIdx] ? 0 : 1;
			if(CheckEdgeToEdge(fFinalDepth, krvecVertex0, krvecVertex1, avecSplinePosT0[nSplineEdgeIdx], avecSplinePosT1[nSplineEdgeIdx], pafDepths[nVertexIdx0], pISect))
			{
				return 1;
			}
		}
		else
		{
			// 
			if(aeIsInside[1 - nLowerEdgeIdx] == aeIsInside[nLowerEdgeIdx])
			{
				// Both end points of the edge are outside of the bound, to the same side.
				return 0;
			}

			if(aeIsInside[nLowerEdgeIdx] != PB_BETWEEN)
			{
				// The lower end of the edge is outside of the bound - let's calculate the 
				float fEdgeToClipTo = aeIsInside[nLowerEdgeIdx] == PB_BEHIND0 ? 0.0f : 1.0f;

				float fN = afNumerator[0] / afDenominator[0];
				float fP = afNumerator[1] / afDenominator[1];
				fUnitInterpolate = (fEdgeToClipTo - fN) / (fP - fN);
				fFinalDepth = (1.0f - fUnitInterpolate) * (pafDepths[nVertexIdx0]) + (fUnitInterpolate) * (pafDepths[nVertexIdx1]);
				Assert(fFinalDepth >= 0.0f);

				// At this point, we could also consider an edge to edge intersection.
				int nSplineEdgeIdx = aeIsInside[nLowerEdgeIdx] == PB_BEHIND0 ? 0 : 1;
				if(CheckEdgeToEdge(fFinalDepth, krvecVertex0, krvecVertex1, avecSplinePosT0[nSplineEdgeIdx], avecSplinePosT1[nSplineEdgeIdx], pafDepths[nVertexIdx0], pISect))
				{
					return 1;
				}

				vecFinalNormal.Lerp(fUnitInterpolate, pavecNormals[nVertexIdx0], pavecNormals[nVertexIdx1]);
				vecFinalNormal.Normalize();
				vecApproxIsectPoint.Lerp(fUnitInterpolate, pavecVertices[nVertexIdx0], pavecNormals[nVertexIdx1]);
			}
			else
			{
				Assert(aeIsInside[1 - nLowerEdgeIdx] != PB_BETWEEN);

				// The upper end of the edge is outside of the bound but the lower is inside - that's fine.
				// That means that we can do the easy calculation.
				fFinalDepth = pafDepths[anVertexIdx[0]];

				// At this point, we could also consider an edge to edge intersection.
				int nSplineEdgeIdx = aeIsInside[1 - nLowerEdgeIdx] == PB_BEHIND0 ? 1 : 0;
				if(CheckEdgeToEdge(fFinalDepth, krvecVertex0, krvecVertex1, avecSplinePosT0[nSplineEdgeIdx], avecSplinePosT1[nSplineEdgeIdx], pafDepths[nVertexIdx0], pISect))
				{
					return 1;
				}

				vecFinalNormal.Set(pavecNormals[anVertexIdx[0]]);
				if(nLowerEdgeIdx == 0)
				{
					fUnitInterpolate = 0.0f;
					vecApproxIsectPoint.Set(krvecVertex0);
				}
				else
				{
					fUnitInterpolate = 1.0f;
					vecApproxIsectPoint.Set(krvecVertex1);
				}
			}
		}

		Assert(fUnitInterpolate >= 0.0f);
		Assert(fUnitInterpolate <= 1.0f);
		Assert(fFinalDepth >= 0.0f);

		pISect->Set(RCC_VEC3V(vecApproxIsectPoint), RCC_VEC3V(vecFinalNormal), fUnitInterpolate, fFinalDepth, 0, (u16)(BAD_INDEX), GetPrimitiveMaterialId());
		return 1;
	}

	return 0;
}
#endif // PHBOUNDRIBBON_USE_PHINTERSECTION


bool phBoundRibbon::FindIntersectionToMovingSphere(const Vector3 &rvecPos1_WS, const Vector3 &rvecPos2_WS, float fRadius, Vector3 &/*rvecHitPosRoad_WS*/,
												 Vector3 &rvecHitPosSphere_WS, float &rfDepth, Vector3 &rvecNormal, int &rnNodeIndex) const
{
	// TODO: This method could get a lot of the same optimizations that 
	// This isn't used.
	//rvecHitPosRoad_WS.Set(0.0f, 0.0f, 0.0f);

	int anSegmentIdx[2];
	float afDepth[2];
	Vector3 avecNormals[2];
	float afApproxT[2];

	anSegmentIdx[1] = LocatePoint(rvecPos2_WS);
	if((anSegmentIdx[1] == -1) || (anSegmentIdx[1] == m_nNumSegments))
	{
		// The sphere's ending position is outside the bound, off of the front or trailing edge.
		return false;
	}
	afApproxT[1] = CalculateApproxT(rvecPos2_WS, anSegmentIdx[1]);

	Vector3 avecTestPointToSplinePoint[2];

	Vector3 vecSplineDir2;
	Vector3 avecSplinePos2[2];

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Let's find the points on the two curves that correspond to the ending position of the sphere, and make 
	//   sure that it is in fact between them.
	m_SplineCurve[0]->SolveSegmentSimple(avecSplinePos2[0], 0, afApproxT[1], &vecSplineDir2, NULL);
	m_SplineCurve[1]->SolveSegmentSimple(avecSplinePos2[1], 0, afApproxT[1], NULL, NULL);

	avecTestPointToSplinePoint[0].Subtract(rvecPos2_WS, avecSplinePos2[0]);
	avecTestPointToSplinePoint[1].Subtract(rvecPos2_WS, avecSplinePos2[1]);

	if(avecTestPointToSplinePoint[0].Dot(avecTestPointToSplinePoint[1]) > 0.0f)
	{
		// rvecPos2_WS isn't between the two points on the splines that correspond to t = afApproxT[1].
		return 0;
	}
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	afDepth[1] = FindDepthUnderCurve(rvecPos2_WS, avecSplinePos2[0], avecSplinePos2[1], vecSplineDir2, &avecNormals[1]);

	if(square(afDepth[1]) <= square(fRadius))
	{
		// The sphere ends up penetrating, so we won't look any more.
		rfDepth = fRadius + afDepth[1];
		Assert(rfDepth > 0.0f);

		rvecHitPosSphere_WS.SubtractScaled(rvecPos2_WS, avecNormals[1], fRadius);
		rvecNormal.Negate(avecNormals[1]);

		rnNodeIndex = anSegmentIdx[1];

		return true;
	}
	else
	{
		if(afDepth[1] < 0.0f)
		{
			// The sphere's ending position is above the road, so it couldn't have passed through the road this frame.
			return false;
		}

		anSegmentIdx[0] = LocatePoint(rvecPos1_WS);
		if((anSegmentIdx[0] == -1) || (anSegmentIdx[0] == m_nNumSegments))
		{
			// The sphere's starting position is outside the bound, off of the front or trailing edge.
			return false;
		}
		afApproxT[0] = CalculateApproxT(rvecPos1_WS, anSegmentIdx[0]);

		Vector3 vecSplineDir1;
		Vector3 avecSplinePos1[2];

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Let's find the points on the two curves that correspond to the starting position of the sphere, and make 
		//   sure that it is in fact between them.
		m_SplineCurve[0]->SolveSegmentSimple(avecSplinePos1[0], 0, afApproxT[0], &vecSplineDir1, NULL);
		m_SplineCurve[1]->SolveSegmentSimple(avecSplinePos1[1], 0, afApproxT[0], NULL, NULL);

		avecTestPointToSplinePoint[0].Subtract(rvecPos1_WS, avecSplinePos1[0]);
		avecTestPointToSplinePoint[1].Subtract(rvecPos1_WS, avecSplinePos1[1]);

		if(avecTestPointToSplinePoint[0].Dot(avecTestPointToSplinePoint[1]) > 0.0f)
		{
			// rvecPos1_WS isn't between the two points on the splines that correspond to t = afApproxT[0].
			return 0;
		}
		//
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		afDepth[0] = FindDepthUnderCurve(rvecPos1_WS, avecSplinePos1[0], avecSplinePos1[1], vecSplineDir1, &avecNormals[0]);

		// We located the second point, let's see if we're underneath the road.
		if(afDepth[0] < 0.0f)
		{
			Assert(afDepth[0] < fRadius);

			// The sphere has passed through the road this frame.
			rfDepth = fRadius + afDepth[1];
			Assert(rfDepth > 0.0f);

			rvecHitPosSphere_WS.SubtractScaled(rvecPos2_WS, avecNormals[1], fRadius);
			rvecNormal.Negate(avecNormals[1]);

			rnNodeIndex = anSegmentIdx[1];

			return true;
		}

		return false;
	}
}


bool phBoundRibbon::RetrieveSegmentIdx(int nVertexIdx, Vector3 const *const pavecVertices, int *const panSegmentIdx, int &rnSegmentIdx) const
{
	if((panSegmentIdx[nVertexIdx] & SEGIDXFLAG_FLAGMASK) != SEGIDXFLAG_UNCHECKED)
	{
		rnSegmentIdx = panSegmentIdx[nVertexIdx] & SEGIDXFLAG_IDXMASK;
	}
	else
	{
		rnSegmentIdx = LocatePoint(pavecVertices[nVertexIdx]);
		if((rnSegmentIdx == -1) || (rnSegmentIdx == m_nNumSegments))
		{
			// The segment end point is outside the bound, off of the front or trailing edge.
			// Return 0 and mark this vertex as invalid.
			panSegmentIdx[nVertexIdx] = SEGIDXFLAG_INVALID;
			return false;
		}
		panSegmentIdx[nVertexIdx] |= (rnSegmentIdx | SEGIDXFLAG_LOCATED);
	}

	return true;
}


void phBoundRibbon::RetrieveSplineData(const float kfApproxT, const Vector3 &krvecVertex, Vector3 &rvecTangent, Vector3 &rvecNormal, 
									 const int knSegmentIdx, float &rfDepth, Vector3 avecSplinePos[2]) const
{
	if((knSegmentIdx & SEGIDXFLAG_FLAGMASK) != SEGIDXFLAG_VALID)
	{
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Let's find the points on the two curves that correspond to krvecVertex0.
	#if I_AM_JUSTIN
		Timer oTimer;
		oTimer.Reset();
	#endif
		Vector3 *apvecTangents[2] = { &rvecTangent, NULL };
		cvCurve<Vector3> * curves[2];
		for (int i=0; i<2; i++)
			curves[i] = m_SplineCurve[i];
		m_SplineCurve[0]->SolveSegmentSimpleBatch(2, curves, avecSplinePos, -1, kfApproxT, apvecTangents);
	#if I_AM_JUSTIN
		sfSolveSegmentTimer += oTimer.MsTime();
	#endif
		//
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		rfDepth = FindDepthUnderCurve(krvecVertex, avecSplinePos[0], avecSplinePos[1], rvecTangent, &rvecNormal);
	}
}


// I don't like the name of this function, it should be changed.
bool phBoundRibbon::CheckEdgeAgainstSlab(const Vector3 &krvecVertex0, const Vector3 &krvecVertex1, const int knSegmentIdx0, const int knSegmentIdx1, bool bPermitInteriorEdges) const
{
	float afDistToSlab[2];
	Vector3 vecTemp;
	vecTemp.Subtract(krvecVertex0, m_pavecSplittingTreePos[knSegmentIdx0]);
	afDistToSlab[0] = vecTemp.Dot(m_pavecSlabNormal[knSegmentIdx0]) - m_pafSlabOffset[knSegmentIdx0];
	if(square(m_pafSlabHalfThickness[knSegmentIdx0]) < square(afDistToSlab[0]))
	{
		// The first vertex is outside of the slab, so let's check to see if the whole thing might be outside.
		vecTemp.Subtract(krvecVertex1, m_pavecSplittingTreePos[knSegmentIdx1]);
		afDistToSlab[1] = vecTemp.Dot(m_pavecSlabNormal[knSegmentIdx1]) - m_pafSlabOffset[knSegmentIdx1];
		if(SameSign(afDistToSlab[0], afDistToSlab[1]) && (square(m_pafSlabHalfThickness[knSegmentIdx1]) < square(afDistToSlab[1])))
		{
			// Both edges are outside of and on the same side of the slab, so the edge can't intersect the slab (and
			//   by extension, the road).
			// We'll return 0, but won't mark either of the vertices as being invalid.
			return(false || ((bPermitInteriorEdges) && (afDistToSlab[0] <= 0.0f)));
		}
	}

	return true;
}


void phBoundRibbon::CalculateSplittingTree()
{
	// For starters, we're going to partition the road into 32 different segments.
	// In the future, we'll want to do this based on charactistics of the road such
	//   as its length, its curvature, and its twistiness.
	// Also in the future, when multiple curve support is back fully, we'll need to
	//   have several of these, and they could also potentially be different.
	m_nNumSegments = 8 * (m_nNumNodes - 1);//4 * (m_nNumNodes - 1);
	if(m_nNumSegments > 64)
	{
		if (phBound::MessagesEnabled())
		{
			Warningf("Road has %d vertices, resulting in %d segments.  This is too many (clamping to 64).", m_nNumNodes, m_nNumSegments);
		}

		m_nNumSegments = 64;
	}
	m_nSplittingTreeSize = m_nNumSegments + 1;

	m_pafSplittingTreeT = rage_new float[m_nSplittingTreeSize];
	m_pavecSplittingTreeNormal = rage_new Vector3[m_nSplittingTreeSize];
	m_pavecSplittingTreePos = rage_new Vector3[m_nSplittingTreeSize];

	m_pavecSlabNormal = rage_new Vector3[m_nNumSegments];
	m_pafSlabHalfThickness = rage_new float[m_nNumSegments];
	m_pafSlabOffset = rage_new float[m_nNumSegments];

	// Also in the future, we're going to want to pick these splitting tree t-values
	//   more intelligently.  Right now I just evenly distribute them, and that's not
	//   going to be the best way for a lot of roads.

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Let's pick the t-values at which to divide the road into segments.
	// To start with, we'll just create a uniform distribution.
	float fOOSplittingTreeSize = 1.0f / (float)(m_nNumSegments);
	int nSplittingTreeIdx;
	for(nSplittingTreeIdx = 0; nSplittingTreeIdx < m_nSplittingTreeSize - 1; ++nSplittingTreeIdx)
	{
		m_pafSplittingTreeT[nSplittingTreeIdx] = fOOSplittingTreeSize * (float)(nSplittingTreeIdx);
	}
	// We set this element explicitly rather than in the loop above because numerical inaccuracy could cause the above loop to calculate a slightly wrong value.
	m_pafSplittingTreeT[m_nNumSegments] = 1.0f;

	// We calculate the starting slabs once.
	CalculateSlabsFromTValues(m_pafSplittingTreeT, m_nNumSegments, m_pafSlabHalfThickness, m_pafSlabOffset, m_pavecSlabNormal);

	// Now we try to adjust the t-values to achieve a uniform thickness across all of the slabs.
	for(int nIterIdx = 0; nIterIdx < 20; ++nIterIdx)
	{
		// Calculated thicknesses can have a small absolute error in them.  However, when the value of the thickness is small, the 
		//   relative error becomes large enough and the redistribution will make adjustments that we really don't want it to make.
		//   Therefore, we bump up the thickness on really thin segments (when for the purpose of redistributing t-values).
		for(int nSlabIndex = 0; nSlabIndex < m_nNumSegments; ++nSlabIndex)
		{
			m_pafSlabHalfThickness[nSlabIndex] = Max(m_pafSlabHalfThickness[nSlabIndex], 0.00001f);
		}

		RedistributeTValues(m_pafSplittingTreeT, m_nNumSegments, m_pafSlabHalfThickness);

		// Let's redo the calculations based on our changes.
		CalculateSlabsFromTValues(m_pafSplittingTreeT, m_nNumSegments, m_pafSlabHalfThickness, m_pafSlabOffset, m_pavecSlabNormal);
	}
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Vector3 vecSplittingPlaneDir1, vecSplittingPlaneDir2;
	Vector3 vecTangent;
	for(nSplittingTreeIdx = 0; nSplittingTreeIdx < m_nSplittingTreeSize; ++nSplittingTreeIdx)
	{
		// Find a point on our splitting plane.
		m_SplineCurve[0]->SolveSegmentSimple(m_pavecSplittingTreePos[nSplittingTreeIdx], -1, m_pafSplittingTreeT[nSplittingTreeIdx], &vecTangent, NULL);

		// Calculate one of the basis vectors for our plane - the vector pointing from one curve to the next.
		m_SplineCurve[1]->SolveSegmentSimple(vecSplittingPlaneDir1, -1, m_pafSplittingTreeT[nSplittingTreeIdx], NULL, NULL);
		vecSplittingPlaneDir1.Subtract(m_pavecSplittingTreePos[nSplittingTreeIdx]);

		// Calculate the other basis vector for our plane - this should be the normal vector at this vertex.
		vecSplittingPlaneDir2.Cross(vecSplittingPlaneDir1, vecTangent);

		m_pavecSplittingTreeNormal[nSplittingTreeIdx].Cross(vecSplittingPlaneDir2, vecSplittingPlaneDir1);
		m_pavecSplittingTreeNormal[nSplittingTreeIdx].Normalize();
	}

	// Also, I should add in a check in here to ensure that the curve meets the requirements for
	//   working with the splitting plane system.
}


void phBoundRibbon::ReadCurve (fiTokenizer& token, atArray<float>* floatArray)
{
	// Read the header.
	token.MatchToken("version:");
	token.GetInt();

	char name[128];
	int numVerts = 0;
	token.GetToken(name,128);
	char* numVertsStr = strrchr(name,'[');
	*numVertsStr = 0;
	numVertsStr++;
	numVerts = atoi(numVertsStr);
	token.GetDelimiter("{");
	for (int index=0;index<numVerts;index++)
	{
		float* nextTerm = &floatArray->Append();
		*nextTerm = token.GetFloat();
	}
}


bool phBoundRibbon::Load_v110 (fiAsciiTokenizer& token)
{
	int curveIndex;
	token.MatchToken("numcurves:");
	SetNumCurves(token.GetInt());
	for (curveIndex=0;curveIndex<m_nNumCurves;curveIndex++)
	{
		// Read a spline curve.
        if (token.CheckToken("curve:"))
        {
            Assert(CURVEMGR);
            int curve = token.GetInt();
		    cvCurve<Vector3>* splineCurve = CURVEMGR->GetCurve(curve);
		    SetCurve(curveIndex,splineCurve);
        }
        else
        {
            if (CURVEMGR)
            {
                cvCurve<Vector3>* splineCurve = CURVEMGR->AddCurve(token);
                SetCurve(curveIndex,splineCurve);
            }
            else
            {
                int version = 2;
                cvCurveNurbs<Vector3>* splineCurve = rage_new cvCurveNurbs<Vector3>;
                splineCurve->Load(token, version);                
                SetCurve(curveIndex,splineCurve);
            }
        }
	}
	m_nNumNodes = m_SplineCurve[0]->GetNumVertices();
	
	// centroid offset
	if (token.CheckToken("centroid:"))
	{
		Vector3 offset;
		token.GetVector(offset);
		SetCentroidOffset(RCC_VEC3V(offset));
	}
	
	if (token.CheckToken("materials:"))
	{
		int numMaterials;
		numMaterials = token.GetInt();
		Assert(numMaterials<=1);
		if (numMaterials > 0)
		{
			SetPrimitiveMaterialId(GetMaterialIdFromFile(token));
		}
	}

	CalculateSplittingTree();
	CalculateExtents();

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundRibbon::Save_v110 (fiAsciiTokenizer& token)
{
	token.PutDelimiter("numcurves: ");
	token.Put(m_nNumCurves);
	for (int curveIndex=0;curveIndex<m_nNumCurves;curveIndex++)
	{
        if (CURVEMGR)
        {
            int curve = CURVEMGR->GetCurveIndex(m_SplineCurve[curveIndex]);
            token.PutDelimiter("curve: ");
            token.Put(curve);
            token.PutDelimiter("\n");
        }
        else
        {
            int version = 2;
            m_SplineCurve[curveIndex]->SaveCurve(token, version);
        }
	}

	token.PutDelimiter("\n");

	if (!IsEqualAll(GetCentroidOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("centroid: ");
		token.Put(GetCentroidOffset());
		token.PutDelimiter("\n");
	}

	token.PutDelimiter("\n");

	token.PutDelimiter("materials: ");
	token.Put(GetNumMaterials());
	token.PutDelimiter("\n");

	WriteMaterialIdToFile(token, GetPrimitiveMaterialId());
	token.PutDelimiter("\n");

	return true;
}


#endif


////////////////////////////////////////////////////////////////////////////////


bool phBoundRibbon::IsPolyhedronInBVHierarchy(const phBoundPolyhedron & polyBound, const Matrix34 & MS2WS) const
{
	Vector3 vecPolyhedronCenter = VEC3V_TO_VECTOR3(polyBound.GetWorldCentroid(RCC_MAT34V(MS2WS)));
	const Vector3 kavecBoxExtents[2] = { VEC3V_TO_VECTOR3(polyBound.GetBoundingBoxMin()), VEC3V_TO_VECTOR3(polyBound.GetBoundingBoxMax()) };

	// Calculate the volume of the bounding sphere.
	float fVolSphere = power3(polyBound.GetRadiusAroundCentroid());

	// Calculate the volume of the bounding box.
	float fVolBox = (kavecBoxExtents[1].x - kavecBoxExtents[0].x) * (kavecBoxExtents[1].y - kavecBoxExtents[0].y) * (kavecBoxExtents[1].z - kavecBoxExtents[0].z);

	// We need to make the decision of which of the polyhedron's two bounding volumes (the sphere or
	//   the box) that we want to collide with the the bounding slab(/disk) that is at each node of
	//   our sphere tree.
	// We make this decision based on the volume of the two shapes, with an advantage given to the
	//   box.  This is because the bounding box of a polyhedron, due to its flat sides, is likely to
	//   allow trivial rejects in many cases where the sphere would not (or would not until farther
	//   down the tree).
	if(fVolBox < /*1.20f * */fVolSphere)
	{
		Vector3 avecBoxVertices[8];
		avecBoxVertices[0].Set(kavecBoxExtents[0][0], kavecBoxExtents[0][1], kavecBoxExtents[0][2]);
		avecBoxVertices[1].Set(kavecBoxExtents[0][0], kavecBoxExtents[0][1], kavecBoxExtents[1][2]);
		avecBoxVertices[2].Set(kavecBoxExtents[0][0], kavecBoxExtents[1][1], kavecBoxExtents[0][2]);
		avecBoxVertices[3].Set(kavecBoxExtents[0][0], kavecBoxExtents[1][1], kavecBoxExtents[1][2]);
		avecBoxVertices[4].Set(kavecBoxExtents[1][0], kavecBoxExtents[0][1], kavecBoxExtents[0][2]);
		avecBoxVertices[5].Set(kavecBoxExtents[1][0], kavecBoxExtents[0][1], kavecBoxExtents[1][2]);
		avecBoxVertices[6].Set(kavecBoxExtents[1][0], kavecBoxExtents[1][1], kavecBoxExtents[0][2]);
		avecBoxVertices[7].Set(kavecBoxExtents[1][0], kavecBoxExtents[1][1], kavecBoxExtents[1][2]);

		avecBoxVertices[0].Dot(MS2WS);
		avecBoxVertices[1].Dot(MS2WS);
		avecBoxVertices[2].Dot(MS2WS);
		avecBoxVertices[3].Dot(MS2WS);
		avecBoxVertices[4].Dot(MS2WS);
		avecBoxVertices[5].Dot(MS2WS);
		avecBoxVertices[6].Dot(MS2WS);
		avecBoxVertices[7].Dot(MS2WS);

		if(!IsInBoundingSphereHierarchy(vecPolyhedronCenter, polyBound.GetRadiusAroundCentroid(), 0, 1, avecBoxVertices) && !IsInBoundingSphereHierarchy(vecPolyhedronCenter, polyBound.GetRadiusAroundCentroid(), 1, 1, avecBoxVertices))
		{
			return false;
		}
	}
	else
	{
		// The bounding sphere is a good enough fit that we should try intersecting it with the sphere hierarchy.
		if(!IsInBoundingSphereHierarchy(vecPolyhedronCenter, polyBound.GetRadiusAroundCentroid(), 0, 1, NULL) && !IsInBoundingSphereHierarchy(vecPolyhedronCenter, polyBound.GetRadiusAroundCentroid(), 1, 1, NULL))
		{
			return false;
		}
	}

	return true;
}

#if 0
#define	MAX_NUM_ISECTS	144	// the maximum number of intersections in any collision (more will be ignored)
bool phBoundRibbon::FindIntersectionsWithPoly	(const phBoundPolyhedron* polyBound, const Matrix34* polyCurrent,
												const Matrix34* polyLast/*, const Vector3 *const kpavecVertices_WS*/, float UNUSED_PARAM(penetration),
												phPolyIntersection* isects, int* numIsects, bool useDispEdges) const
{
	// Get the vertices of polyBound transformed to world coordinates.
	const int knPolyBoundVertexCnt = polyBound->GetNumVertices();
	const int knVertexBufferSize = useDispEdges ? 2 * knPolyBoundVertexCnt : knPolyBoundVertexCnt;
	// 
	Vector3 *pavecVertices_WS = Alloca(Vector3, knVertexBufferSize);
	Vector3 *pavecVertexNormals_WS = Alloca(Vector3, knVertexBufferSize);
	float *pafVertexDepths = Alloca(float, knVertexBufferSize);
	Vector3 *pavecVertexTangents_WS = Alloca(Vector3, knVertexBufferSize);
	int *panVertexSegmentIdx = Alloca(int, knVertexBufferSize);
	memset(panVertexSegmentIdx, SEGIDXFLAG_UNCHECKED, sizeof(int) * knVertexBufferSize);

	int nCurVertexBufferIdx;
	for (nCurVertexBufferIdx = 0; nCurVertexBufferIdx < knPolyBoundVertexCnt; ++nCurVertexBufferIdx)
	{
		polyCurrent->Transform(polyBound->GetVertex(nCurVertexBufferIdx), pavecVertices_WS[nCurVertexBufferIdx]);
		if(useDispEdges)
		{
			polyLast->Transform(polyBound->GetVertex(nCurVertexBufferIdx), pavecVertices_WS[nCurVertexBufferIdx + knPolyBoundVertexCnt]);
		}
	}

	phPolyIntersection* isectPtr = isects;
	int numLeft = MAX_NUM_ISECTS;
	int nPolyBoundPolyCnt = polyBound->GetNumPolygons();

	if(useDispEdges)
	{
		// Check the displacement edges (moving vertices) to see if any of them have passed through the bound since the last update.
		for(int nVertexIdx = 0; nVertexIdx < knPolyBoundVertexCnt; nVertexIdx++)
		{
			if(TestProbeIndexed(nVertexIdx + knPolyBoundVertexCnt, nVertexIdx, pavecVertices_WS, pavecVertexNormals_WS, pafVertexDepths, pavecVertexTangents_WS,
				panVertexSegmentIdx, isectPtr))
			{
				panVertexSegmentIdx[nVertexIdx] |= SEGIDXFLAG_VERTEXHANDLED;

				if(isectPtr->GetDepth() == 0.0f)
				{
					continue;
				}

				--numLeft;

				isectPtr->VertA.Set(pavecVertices_WS[nVertexIdx + knPolyBoundVertexCnt]);
				isectPtr->VertB.Set(pavecVertices_WS[nVertexIdx]);
				isectPtr->BoundP = this;
				isectPtr->Poly = NULL;
				isectPtr->SetPartIndex(0);
				isectPtr->EdgeNum = nVertexIdx;		// vertex number for displacement edges
				isectPtr->BoundE = this;
				isectPtr->InteriorEdgeFlag = false;
				isectPtr->DispEdgeFlag = true;
				isectPtr->VertNumA = BAD_INDEX;
				isectPtr->VertNumB = nVertexIdx;
				isectPtr++;
			}
		}
	}

	// Unfortunately, it's possible that, even if we had detected a penetrating displacement edge the last frame, the polyhedron was
	//   not corrected enough to pull that vertex out of the bound.  So, here we check to see if any of the edges of the polyhedron are 
	//   still partially submerged and are not connected to any penetrating displacement edges that we detected this frame.
	for(int nPolyIdx1 = 0; nPolyIdx1 < nPolyBoundPolyCnt - 1; ++nPolyIdx1)
	{
		const phPolygon *pPoly1 = &polyBound->GetPolygon(nPolyIdx1);
		int nPolyVertexCnt = pPoly1->GetNumVerts();
		for(int nPolyVertexIdx1 = 0; nPolyVertexIdx1 < nPolyVertexCnt; ++nPolyVertexIdx1)
		{
			int nPolyVertexIdx2 = nPolyVertexIdx1 < pPoly1->GetNumVerts() - 1 ? nPolyVertexIdx1 + 1 : 0;
			int nPolyIdx2 = pPoly1->GetNeighboringPolyNum(nPolyVertexIdx1);

			if(nPolyIdx1 > nPolyIdx2)
			{
				// These two polygons are being considered in the wrong order.
				continue;
			}

			int nBoundVertexIdx1 = pPoly1->GetVertexIndex(nPolyVertexIdx1);
			int nBoundVertexIdx2 = pPoly1->GetVertexIndex(nPolyVertexIdx2);

			if((panVertexSegmentIdx[nBoundVertexIdx1] & SEGIDXFLAG_VERTEXHANDLED) && (panVertexSegmentIdx[nBoundVertexIdx2] & SEGIDXFLAG_VERTEXHANDLED))
			{
				// We probably can also continue if only one of the vertices has been handled; we should put an assert on this farther down to
				//   test that assumption.
				// Both vertices were already found to have passed through the bound this frame; that means that the edge must have too but,
				//   since we've already handled the vertices, we don't need to look at the edge.
				continue;
			}

			int numHits = TestEdgeIndexed(nBoundVertexIdx1, nBoundVertexIdx2, pavecVertices_WS, pavecVertexNormals_WS, pafVertexDepths, 
				pavecVertexTangents_WS, panVertexSegmentIdx, isectPtr, true);

			while(numHits--)
			{
				// This edge was found to be intersecting with the ribbon bound.  Even if we reject it because it's oriented the wrong way,
				//   we still want to record that it was penetrating because we want to check correctly for interior edges later.
				if(pafVertexDepths[nBoundVertexIdx1] > 0.0f)
				{
					panVertexSegmentIdx[nBoundVertexIdx1] |= SEGIDXFLAG_PENETRATINGEDGE;
				}
				if(pafVertexDepths[nBoundVertexIdx2] > 0.0f)
				{
					panVertexSegmentIdx[nBoundVertexIdx2] |= SEGIDXFLAG_PENETRATINGEDGE;
				}

				if(isectPtr->GetDepth() == 0.0f)
				{
					continue;
				}

				const Vector3 poly1UnitNormal(polyBound->GetPolygonUnitNormal(nPolyIdx1));
				isectPtr->EdgeNormal.Set(poly1UnitNormal);
				if(nPolyIdx2 != (u16)(-1))
				{
					Vector3 vecPoly1Normal_WS, vecPoly2Normal_WS;
					vecPoly1Normal_WS.Dot3x3(poly1UnitNormal, *polyCurrent);
					const Vector3 poly2UnitNormal(polyBound->GetPolygonUnitNormal(nPolyIdx2));
					vecPoly2Normal_WS.Dot3x3(poly2UnitNormal, *polyCurrent);
					if(!geomPoints::IsPointInWedge(isectPtr->GetNormal(), poly1UnitNormal, poly2UnitNormal, polyCurrent, true))
					{
						continue;
					}
					isectPtr->EdgeNormal.Add(poly2UnitNormal);
				}
				else
				{
					if(isectPtr->GetNormal().Dot(isectPtr->EdgeNormal) > 0.0f)
					{
						continue;
					}
				}

				if(isectPtr->GetT() == 0.0f)
				{
					if(panVertexSegmentIdx[nBoundVertexIdx1] & SEGIDXFLAG_VERTEXHANDLED)
					{
						// This vertex was already taken care of, let's not handle it again.
						continue;
					}
					panVertexSegmentIdx[nBoundVertexIdx1] |= SEGIDXFLAG_VERTEXHANDLED;
				}
				else if(isectPtr->GetT() == 1.0f)
				{
					Assert(pafVertexDepths[nBoundVertexIdx2] > 0.0f);
					if(panVertexSegmentIdx[nBoundVertexIdx2] & SEGIDXFLAG_VERTEXHANDLED)
					{
						// This vertex was already taken care of, let's not handle it again.
						continue;
					}
					panVertexSegmentIdx[nBoundVertexIdx2] |= SEGIDXFLAG_VERTEXHANDLED;
				}

				--numLeft;

				// This edge of polyBound intersects this bound.
				isectPtr->VertA.Set(pavecVertices_WS[nBoundVertexIdx1]);
				isectPtr->VertB.Set(pavecVertices_WS[nBoundVertexIdx2]);
				isectPtr->BoundP = this;
				isectPtr->Poly = NULL;
				isectPtr->SetPartIndex(0);
				isectPtr->EdgeNum = (nPolyIdx1 << 2) + nPolyVertexIdx1;
				isectPtr->BoundE = polyBound;
				isectPtr->InteriorEdgeFlag = false;
				isectPtr->DispEdgeFlag = false;

				isectPtr->VertNumA = nBoundVertexIdx1;
				isectPtr->VertNumB = nBoundVertexIdx2;

				isectPtr->m_MatrixE.Set(*polyCurrent);
				isectPtr++;
			}
		}
	}
#if 1
	// Unfortunately, it's possible that, despite all our best efforts, an entire edge has completely penetrated 
	// So, here we check to see if any of the edges of the polyhedron are completely submerged but are connected to any
	//   penetrating real edges that we detected this frame.
	for(int nPolyIdx1 = 0; nPolyIdx1 < nPolyBoundPolyCnt - 1; ++nPolyIdx1)
	{
		const phPolygon *pPoly1 = &polyBound->GetPolygon(nPolyIdx1);
		int nPolyVertexCnt = pPoly1->GetNumVerts();
		for(int nPolyVertexIdx1 = 0; nPolyVertexIdx1 < nPolyVertexCnt; ++nPolyVertexIdx1)
		{
			int nPolyVertexIdx2 = nPolyVertexIdx1 < pPoly1->GetNumVerts() - 1 ? nPolyVertexIdx1 + 1 : 0;
			int nPolyIdx2 = pPoly1->GetNeighboringPolyNum(nPolyVertexIdx1);

			if(nPolyIdx1 > nPolyIdx2)
			{
				// These two polygons are being considered in the wrong order.
				continue;
			}

			int nBoundVertexIdx1 = pPoly1->GetVertexIndex(nPolyVertexIdx1);
			int nBoundVertexIdx2 = pPoly1->GetVertexIndex(nPolyVertexIdx2);

			bool bUnhandledEdge = !((panVertexSegmentIdx[nBoundVertexIdx1] & SEGIDXFLAG_VERTEXHANDLED) && (panVertexSegmentIdx[nBoundVertexIdx2] & SEGIDXFLAG_VERTEXHANDLED));
			bool bPotentialInteriorEdge = panVertexSegmentIdx[nBoundVertexIdx1] & SEGIDXFLAG_PENETRATINGEDGE || panVertexSegmentIdx[nBoundVertexIdx2] & SEGIDXFLAG_PENETRATINGEDGE;

			if(!bUnhandledEdge || !bPotentialInteriorEdge)
			{
				// Either both vertices were already found to be penetrating the bound, or neither vertex was found to be penetrating the bound.
				// In either case, we won't look for an interior edge.
				continue;
			}

			int numHits = TestInteriorEdgeIndexed(nBoundVertexIdx1, nBoundVertexIdx2, pavecVertices_WS, pavecVertexNormals_WS, pafVertexDepths, 
				pavecVertexTangents_WS, panVertexSegmentIdx, isectPtr);

			while(numHits--)
			{
				if(isectPtr->GetDepth() == 0.0f)
				{
					continue;
				}

				if(isectPtr->GetT() == 0.0f)
				{
					if(panVertexSegmentIdx[nBoundVertexIdx1] & SEGIDXFLAG_VERTEXHANDLED)
					{
						// This vertex was already taken care of, let's not handle it again.
						continue;
					}
				}
				else if(isectPtr->GetT() == 1.0f)
				{
					if(panVertexSegmentIdx[nBoundVertexIdx2] & SEGIDXFLAG_VERTEXHANDLED)
					{
						// This vertex was already taken care of, let's not handle it again.
						continue;
					}
				}

				const Vector3 poly1UnitNormal(polyBound->GetPolygonUnitNormal(nPolyIdx1));
				isectPtr->EdgeNormal.Set(poly1UnitNormal);
				if(nPolyIdx2 != (u16)(-1))
				{
					const Vector3 poly2UnitNormal(polyBound->GetPolygonUnitNormal(nPolyIdx2));
					Vector3 vecPoly1Normal_WS, vecPoly2Normal_WS;
					vecPoly1Normal_WS.Dot3x3(poly1UnitNormal, *polyCurrent);
					vecPoly2Normal_WS.Dot3x3(poly2UnitNormal, *polyCurrent);
					if(!geomPoints::IsPointInWedge(isectPtr->GetNormal(),poly1UnitNormal,poly2UnitNormal,polyCurrent,true))
					{
						continue;
					}
					isectPtr->EdgeNormal.Add(poly2UnitNormal);
				}
				else
				{
					if(isectPtr->GetNormal().Dot(isectPtr->EdgeNormal) > 0.0f)
					{
						continue;
					}
				}

				// We will reject an interior edge if the motion of the edge can't account for at least part of the penetration depth.
				// JUSTIN : Since isectPtr->tVal is likely to be 0.0f or 1.0f most of the time, check into special casing those
				//   and if it improves performance.
				Vector3 avecVertMotion[2];
				avecVertMotion[0].Subtract(pavecVertices_WS[nBoundVertexIdx1], pavecVertices_WS[nBoundVertexIdx1 + knPolyBoundVertexCnt]);
				avecVertMotion[1].Subtract(pavecVertices_WS[nBoundVertexIdx2], pavecVertices_WS[nBoundVertexIdx2 + knPolyBoundVertexCnt]);
				Vector3 vecVertMotion;
				vecVertMotion.Lerp(isectPtr->GetT(), avecVertMotion[0], avecVertMotion[1]);

				if(-vecVertMotion.Dot(isectPtr->GetNormal()) < 0.707f * isectPtr->GetDepth())
				{
					continue;
				}

				--numLeft;

				// This edge of polyBound intersects this bound.
				isectPtr->VertA.Set(pavecVertices_WS[nBoundVertexIdx1]);
				isectPtr->VertB.Set(pavecVertices_WS[nBoundVertexIdx2]);
				isectPtr->BoundP = this;
				isectPtr->Poly = NULL;
				isectPtr->SetPartIndex(0);
				isectPtr->EdgeNum = (nPolyIdx1 << 2) + nPolyVertexIdx1;
				isectPtr->BoundE = polyBound;
				isectPtr->InteriorEdgeFlag = true;
				isectPtr->DispEdgeFlag = false;

				isectPtr->VertNumA = nBoundVertexIdx1;
				isectPtr->VertNumB = nBoundVertexIdx2;

				isectPtr->m_MatrixE.Set(*polyCurrent);
				isectPtr++;
			}
		}
	}
#endif
	(*numIsects) = MAX_NUM_ISECTS-numLeft;
/*
	if ((*numIsects)>0)
	{
#if 0
		if (TwoSided)
		{
			// Make the road two-sided by choosing which side of the road polyBound should be colliding with and
			// reversing any intersections with the other side.
			Vector3 polyCenter;
			polyBound->GetCenter(polyLast,&polyCenter);
			Vector3 relPos;
			for (index=(*numIsects)-1;index>=0;index--)
			{
				phPolyIntersection& isect = isects[index];
				if (!isect.DispEdge())
				{
					relPos.Subtract(polyCenter,isect.Position);
					if (isect.Normal.Dot(relPos)<0.0f)
					{
						isect.AoutsideFlag = !isect.AoutsideFlag;
						Vector3 segDisp(isect.VertB);
						segDisp.Subtract(isect.VertA);
						isect.Depth = segDisp.Dot(isect.Normal)-isect.Depth;
						isect.Normal.Negate();
					}
				}
			}
		}
#endif
		return true;
	}

	return false;
*/
	return(*numIsects > 0);
}
#endif	// end of #if 0 to save FindIntersectionsWithPoly for future reference


int phBoundRibbon::LocatePoint(const Vector3 &rvecTestPoint) const
{
#if I_AM_JUSTIN
	Timer oTimer;
	oTimer.Reset();
#endif
	int anExtent[2];

	// These two values represent the range of segments that are still valid.
	// They are initialized to represent off of the left and right edges of the bound,
	//   respectively.
	anExtent[0] = -1;
	anExtent[1] = m_nNumSegments;

	Vector3 vecTestPointToPlanePos;

	int nCurSplittingPlane;

	// NOTE: This function is pretty simple, but it is used a whole lot and is especially significant when we
	//   are finding intersections with a polyhedron.  I can't see any way to improve it at the C++ level, so
	//   it might it be worth it to consider implementing this portion in assembly.
	do
	{
		Assert(anExtent[1] > anExtent[0]);

		nCurSplittingPlane = (anExtent[0] + anExtent[1] + 1) >> 1;

		Assert(nCurSplittingPlane >= 0);
		Assert(nCurSplittingPlane < m_nSplittingTreeSize);

		vecTestPointToPlanePos.Subtract(rvecTestPoint, m_pavecSplittingTreePos[nCurSplittingPlane]);

		if(vecTestPointToPlanePos.Dot(m_pavecSplittingTreeNormal[nCurSplittingPlane]) >= 0.0f)
		{
			// The test point is to the right of (or possibly on) this splitting plane.
			anExtent[0] = nCurSplittingPlane;
		}
		else
		{
			// The test point is to the left of this splitting plane.
			anExtent[1] = nCurSplittingPlane - 1;
		}
	}
	while(anExtent[0] != anExtent[1]);
#if I_AM_JUSTIN
	sfTotalLocatePointTimer += oTimer.MsTime();
#endif

	return anExtent[0];
}


float phBoundRibbon::CalculateApproxT(const Vector3 &rvecTestPoint, const int nSegmentIdx) const
{
	Assert(nSegmentIdx >= 0);
	Assert(nSegmentIdx < m_nNumSegments);

	// The point was located between two of the splitting planes, so let's try to approximate a t-value for the point.
	Vector3 vecTestPointToPlanePos;
	vecTestPointToPlanePos.Subtract(rvecTestPoint, m_pavecSplittingTreePos[nSegmentIdx]);
	float fDot0 = vecTestPointToPlanePos.Dot(m_pavecSplittingTreeNormal[nSegmentIdx]);

	vecTestPointToPlanePos.Subtract(rvecTestPoint, m_pavecSplittingTreePos[nSegmentIdx + 1]);
	float fDot1 = vecTestPointToPlanePos.Dot(m_pavecSplittingTreeNormal[nSegmentIdx + 1]);

	float fUnitInterpolate = fDot0 / (fDot0 - fDot1);

	float fApproxT = (1.0f - fUnitInterpolate) * m_pafSplittingTreeT[nSegmentIdx] + fUnitInterpolate * m_pafSplittingTreeT[nSegmentIdx + 1];
	return fApproxT;
}


// This is a 'special' version of FindDepthUnderCurve for when we have already called SolveSegment and don't want to make that costly
//   call again.
// Note that rvecSplineDir0 does *not* need to be unit.
float phBoundRibbon::FindDepthUnderCurve (const Vector3 &rvecTestPoint, const Vector3 &rvecSplinePos0, const Vector3 &rvecSplinePos1, const Vector3 &rvecSplineDir0, Vector3 *pvecNormal) const
{
	Vector3 vecSurfaceNormal(rvecSplinePos1);
	vecSurfaceNormal.Subtract(rvecSplinePos0);
	vecSurfaceNormal.Cross(rvecSplineDir0);
	vecSurfaceNormal.Normalize();

	if(pvecNormal != NULL)
	{
		pvecNormal->Set(vecSurfaceNormal);
	}

	Vector3 vecRelPos(rvecTestPoint);
	vecRelPos.Subtract(rvecSplinePos1);
	float fDepth = -vecRelPos.Dot(vecSurfaceNormal);

	return fDepth;
}


////////////////////////////////////////////////////////////////
// bound line drawing 

#if __PFDRAW
void phBoundRibbon::Draw(Mat34V_In mtxIn, bool colorMaterials, bool UNUSED_PARAM(solid), int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	grcWorldMtx(mtx);
	Vector3 nodePos[MAX_NUM_CURVES],lastNodePos[MAX_NUM_CURVES];
	float nodeT = 0.0f;
	int curveIndex;
	for (curveIndex=0;curveIndex<m_nNumCurves;curveIndex++)
	{
		m_SplineCurve[curveIndex]->SolveSegment(lastNodePos[curveIndex],0,nodeT);
	}

	// Set the color.
	Color32 drawColor(grcCurrentColor);
	if (colorMaterials)
	{
		// Ignore the given color, and color the capsule according to its material index.
		drawColor = MATERIALMGR.GetDebugColor(GetMaterial(0));
	}

	int nNodeIdx;
	for(nNodeIdx = 1; nNodeIdx <= (m_nNumNodes - 1) * 5; ++nNodeIdx)
	{
		nodeT = (float)(nNodeIdx) * m_fInvNumSpaces * 0.2f;
		for (curveIndex=0;curveIndex<m_nNumCurves;curveIndex++)
		{
			// Draw a segment along this spline.
			m_SplineCurve[curveIndex]->SolveSegment(nodePos[curveIndex],0,nodeT);
			grcDrawLine(lastNodePos[curveIndex],nodePos[curveIndex],drawColor);
			if (curveIndex<m_nNumCurves-1)
			{
				// Draw a segment from this spline to the next.
				grcDrawLine(lastNodePos[curveIndex],lastNodePos[curveIndex+1], drawColor);
			}
		}
		for (curveIndex=0;curveIndex<m_nNumCurves;curveIndex++)
		{
			lastNodePos[curveIndex].Set(nodePos[curveIndex]);
		}
	}

	// Draw the normals of the splitting planes.
//	Vector3 vecGreen(0.0f, 1.0f, 0.0f);
//	for(int nSplittingPlaneIdx = 0; nSplittingPlaneIdx < m_nSplittingTreeSize; ++nSplittingPlaneIdx)
//	{
//		Vector3 vecNormal;
//		vecNormal.SetScaled(m_pavecSplittingTreeNormal[nSplittingPlaneIdx], 5.0f);
//		group->DrawArrowDisp(m_pavecSplittingTreePos[nSplittingPlaneIdx], vecNormal, 0, vecGreen);
//	}

	// Draw the normals of the bounding slab for each road segment.
//	Vector3 vecOrange(1.0f, 1.0f, 0.0f);
//	for(int nSegmentIdx = 0; nSegmentIdx < m_nNumSegments; ++nSegmentIdx)
//	{
//		Vector3 vecTemp(m_pavecSlabNormal[nSegmentIdx]);
//		vecTemp.Scale(10.0f);
//		group->DrawArrowDisp(m_pavecSplittingTreePos[nSegmentIdx], vecTemp, 0, vecOrange);
//		group->DrawArrowDisp(m_pavecSplittingTreePos[nSegmentIdx + 1], vecTemp, 0, vecOrange);
//	}

	// Draw the road end boundary.
//	for (curveIndex=0;curveIndex<m_nNumCurves-1;curveIndex++)
//	{
//		// Draw a segment from this spline to the next.
//		group->DrawSegment(nodePos[curveIndex],nodePos[curveIndex+1]);
//	}
//	Vector3 vecColor(0.2f, 0.2f, 0.2f);
//	for(int nSphereTreeDepth = 0; nSphereTreeDepth < m_nSphereTreeDepth; ++nSphereTreeDepth)
//	{
//		for(int nSphereTreeX = 0; nSphereTreeX < (1 << nSphereTreeDepth); ++nSphereTreeX)
//		{
//			int nSphereTreeIdx = (1 << nSphereTreeDepth) + nSphereTreeX - 2;
//			group->DrawSphere(m_pafSphereRadii[nSphereTreeIdx], m_pavecSphereCenters[nSphereTreeIdx], 0, vecColor);
//		}
//		vecColor.Add(0.2f);
//	}
}
#endif // __PFDRAW

//
// Resource code (must be at the bottom of the file)
//

phBoundRibbon::phBoundRibbon(datResource &rsc) : phBound(rsc),

m_SplineCurve(rsc, true)

{
	/*
	rsc.PointerFixup(m_SplineCurve);
	for (int i=0; i<m_nNumCurves; i++)
	{
		rsc.PointerFixup(m_SplineCurve[i]);
	}

    rsc.PointerFixup(m_pafSplittingTreeT);
	rsc.PointerFixup(m_pavecSplittingTreeNormal);
	rsc.PointerFixup(m_pavecSplittingTreePos);

	rsc.PointerFixup(m_pavecSlabNormal);
	rsc.PointerFixup(m_pafSlabHalfThickness);
	rsc.PointerFixup(m_pafSlabOffset);

	rsc.PointerFixup(m_pavecSphereCenters);
	rsc.PointerFixup(m_pafSphereRadii);

	rsc.PointerFixup(m_pavecOBDNormals);
	rsc.PointerFixup(m_pafOBDHalfThicknesses);
	rsc.PointerFixup(m_pafOBDOffsets);
	*/

}

#if __DECLARESTRUCT
void phBoundRibbon::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);

	STRUCT_BEGIN(phBoundRibbon);
	STRUCT_FIELD(m_SplineCurve);
	STRUCT_DYNAMIC_ARRAY(m_pavecSphereCenters, m_nSphereTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pafSphereRadii, m_nSphereTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pavecOBDNormals, m_nSphereTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pafOBDHalfThicknesses, m_nSphereTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pafOBDOffsets, m_nSphereTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pafSplittingTreeT, m_nSplittingTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pavecSplittingTreeNormal, m_nSplittingTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pavecSplittingTreePos, m_nSplittingTreeSize);
	STRUCT_DYNAMIC_ARRAY(m_pavecSlabNormal, m_nNumSegments);
	STRUCT_DYNAMIC_ARRAY(m_pafSlabHalfThickness, m_nNumSegments);
	STRUCT_DYNAMIC_ARRAY(m_pafSlabOffset, m_nNumSegments);
	STRUCT_FIELD(m_nNumCurves);
	STRUCT_FIELD(m_nNumNodes);
	STRUCT_FIELD(m_fInvNumSpaces);
	STRUCT_FIELD(m_nNumSegments);
	STRUCT_FIELD(m_nSplittingTreeSize);
	STRUCT_FIELD(m_nSphereTreeDepth);
	STRUCT_FIELD(m_nSphereTreeSize);
	STRUCT_CONTAINED_ARRAY(pad);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

#endif // USE_RIBBONS

} // namespace rage
