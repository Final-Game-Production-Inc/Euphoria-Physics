//
// phbound/liquidimpactdata.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_LIQUIDIMPACTDATA_H
#define PHBOUND_LIQUIDIMPACTDATA_H

#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "vector/vector3.h"

#define VALIDATE_LIQUID_INPUTS 0
#if VALIDATE_LIQUID_INPUTS 
#include "vectormath/vectorassert.h"
#define LIQUID_LEGIT_VALUE (1e+10f)
#define LIQUID_ASSERT_LEGIT(x) FastAssertMagnitude(x, LIQUID_LEGIT_VALUE)
#else
#define LIQUID_ASSERT_LEGIT(x)
#endif

namespace rage {

/*
PURPOSE
phLiquidImpactData holds information for a single point of collision between a liquid and another object.
<FLAG Component>
*/
class phLiquidImpactData
{
public:
	inline phLiquidImpactData();

	inline bool SetFromTriangle(const Vector3 *ThreeVertices, const float *ThreeDepths);
	inline bool SetFromQuadrangle(const Vector3 *FourVertices, const float *FourDepths);
	inline bool SetFromSphere(const float *FourDepths);		// NOT IMPLEMENTED!!!

	inline void CombineImpactDatas(const phLiquidImpactData &kImpactDataA, const phLiquidImpactData &kImpactDataB);

	Vector3 m_ForcePos;
	Vector3 m_Normal;			// Unit vector from the non-liquid object toward the liquid object.
	float m_SubmergedArea;
	float m_SubmergedVolume;
	u16 m_PartIndex;			// Index number of the polygon, edge or vertex of the non-liquid object (object B).
	u32 m_Component;			// Component number of the component hit in the non-liquid object (object B).
	bool m_UseBoundDirectionForDrag;
	bool m_CompletelySubmerged;

protected:
	inline void SubmergedTriangleBuoyancy(const Vector3 *pavecThreeVertices, float fDepthSum, const float *pafThreeDepths, float fImpulseScale);
};


phLiquidImpactData::phLiquidImpactData()
{
	m_Component = 0;
    m_Normal.Zero();
	m_UseBoundDirectionForDrag = false;
	m_CompletelySubmerged = true;
}

bool phLiquidImpactData::SetFromTriangle(const Vector3 *ThreeVertices, const float *ThreeDepths)
{
	FastAssert(ThreeDepths[0] > 0.0f || ThreeDepths[1] > 0.0f || ThreeDepths[2] > 0.0f);

	// Copy the three vertices.
	Vector3 triangleVertices[3];
	float triangleDepths[3];
	int vertIndex;
	for (vertIndex=0;vertIndex<3;vertIndex++)
	{
		triangleVertices[vertIndex].Set(ThreeVertices[vertIndex]);
		triangleDepths[vertIndex] = ThreeDepths[vertIndex];
	}

	// Reorder the copied vertices so that the third one is above the edge of the other two.
	ReOrderVerts(triangleVertices, triangleDepths);

	// Find the number of submerged vertices.
	// OneSubmerged is used if exactly one is submerged (index of the one submerged)
	// OneNot is used if exactly two are submerged (index of the one not submerged)
	Vector3 vertexShift;
	float depthSum = 0.0f;
	int numSubmerged = 0, oneSubmerged = BAD_INDEX, oneNot = BAD_INDEX;
	for(vertIndex = 0; vertIndex < 3; vertIndex++)
	{
		if(triangleDepths[vertIndex] >= 0.0f)
		{
			oneSubmerged = vertIndex;
			numSubmerged++;
			depthSum += triangleDepths[vertIndex];
		}
		else
		{
			oneNot = vertIndex;
		}
	}

	// Find the buoyancy by splitting the submerged part of the triangle into more triangles (leaving it whole
	// if all three vertices are submerged).
	Vector3 a, b;
	float fImpulseScale;
	FastAssert(numSubmerged != 0);
	switch(numSubmerged)
	{
	case 1:
		{
			// One out of three vertices is submerged.  Move the other two vertices near the surface,
			// reorder the vertices, recalculate the area and find the impulse and the impulse application point.
			for (vertIndex=0;vertIndex<3;vertIndex++)
			{
				if (vertIndex==oneSubmerged)
				{
					continue;
				}

				vertexShift.Subtract(triangleVertices[oneSubmerged],triangleVertices[vertIndex]);
				fImpulseScale = triangleDepths[vertIndex]-triangleDepths[oneSubmerged];
				if (fabsf(fImpulseScale)>VERY_SMALL_FLOAT)
				{
					vertexShift.Scale(triangleDepths[vertIndex] / fImpulseScale);
				}

				triangleVertices[vertIndex].Add(vertexShift);
				triangleDepths[vertIndex] = 0.0f;
			}

			depthSum = triangleDepths[oneSubmerged];
			//			PF_START(Liquid_ReorderVerts);
			ReOrderVerts(triangleVertices,triangleDepths);
			//			PF_STOP(Liquid_ReorderVerts);
			a.Subtract(triangleVertices[1],triangleVertices[0]);
			b.Subtract(triangleVertices[2],triangleVertices[0]);
			a.Cross(b);
			m_SubmergedArea = 0.5f * a.Mag();
			fImpulseScale = m_SubmergedArea;
			LIQUID_ASSERT_LEGIT(depthSum);
			LIQUID_ASSERT_LEGIT(fImpulseScale);
			SubmergedTriangleBuoyancy(triangleVertices, depthSum, triangleDepths, fImpulseScale);
			break;
		}
	case 2:
		{
			// Two out of three vertices are submerged.  Move the other vertex into two surface vertices and split
			// the quadrangle into two triangles.  For each of these two triangles, reorder the vertices,
			// recalculate the area, and call SubmergedTriangleBuoyancy to get the impulse and application
			// point.  Then combine the two into one impulse and application point.
			Vector3 spareVertices[3];
			float spareDepths[3];
			for (vertIndex=0;vertIndex<3;vertIndex++)
			{
				if (vertIndex==oneNot)
				{
					continue;
				}

				vertexShift.Subtract(triangleVertices[vertIndex],triangleVertices[oneNot]);
				fImpulseScale = triangleDepths[oneNot]-triangleDepths[vertIndex];
				if (fabsf(fImpulseScale)>VERY_SMALL_FLOAT)
				{
					vertexShift.Scale(triangleDepths[oneNot] / fImpulseScale);
				}

				spareVertices[vertIndex].Add(triangleVertices[oneNot],vertexShift);
				spareDepths[vertIndex] = 0.0f;
			}

			vertIndex = 0;
			while (vertIndex==oneNot)
			{
				vertIndex++;
			}

			spareVertices[oneNot].Set(triangleVertices[vertIndex]);
			spareDepths[oneNot] = triangleDepths[vertIndex];
			float spareDepthSum = spareDepths[oneNot];
			vertIndex++;

			while (vertIndex==oneNot)
			{
				vertIndex++;
			}

			triangleVertices[oneNot].Set(spareVertices[vertIndex]);
			triangleDepths[oneNot] = 0.0f;
			//			PF_START(Liquid_ReorderVerts);
			ReOrderVerts(triangleVertices,triangleDepths);
			ReOrderVerts(spareVertices,spareDepths);
			//			PF_STOP(Liquid_ReorderVerts);

			// Get the impulse scale and application point for the spareVertices triangle.
			a.Subtract(spareVertices[1],spareVertices[0]);
			b.Subtract(spareVertices[2],spareVertices[0]);
			a.Cross(b);
			phLiquidImpactData SpareImpactData;
			SpareImpactData.m_SubmergedArea = 0.5f*a.Mag();
			fImpulseScale = SpareImpactData.m_SubmergedArea;
			LIQUID_ASSERT_LEGIT(spareDepthSum);
			LIQUID_ASSERT_LEGIT(fImpulseScale);
			SpareImpactData.SubmergedTriangleBuoyancy(spareVertices, spareDepthSum, spareDepths, fImpulseScale);

			// Get the impulse and application point for the triangleVertices triangle.
			a.Subtract(triangleVertices[1],triangleVertices[0]);
			b.Subtract(triangleVertices[2],triangleVertices[0]);
			a.Cross(b);
			m_SubmergedArea = 0.5f * a.Mag();
			fImpulseScale = m_SubmergedArea;
			LIQUID_ASSERT_LEGIT(depthSum);
			LIQUID_ASSERT_LEGIT(fImpulseScale);
			SubmergedTriangleBuoyancy(triangleVertices,depthSum,triangleDepths,fImpulseScale);

			// Combine the two into a single buoyancy.
			//BuoyancyData oldBuoyancy = *pBuoyancy;
			// TODO: Eliminate the need for the extra impact data.
			phLiquidImpactData OldImpactData = *this;
			CombineImpactDatas(OldImpactData, SpareImpactData);
			break;
		}
	case 3:
		{
			// The whole triangle is submerged.
			a.Subtract(triangleVertices[1],triangleVertices[0]);
			b.Subtract(triangleVertices[2],triangleVertices[0]);
			a.Cross(b);
			m_SubmergedArea = 0.5f * a.Mag();
			fImpulseScale = m_SubmergedArea;
			LIQUID_ASSERT_LEGIT(depthSum);
			LIQUID_ASSERT_LEGIT(fImpulseScale);
			SubmergedTriangleBuoyancy(triangleVertices,depthSum,triangleDepths,fImpulseScale);
			break;
		}
	}
	LIQUID_ASSERT_LEGIT(m_SubmergedVolume);
	LIQUID_ASSERT_LEGIT(m_SubmergedArea);

	//	PF_STOP(Liquid_TriangleBuoyancy);
	return (m_SubmergedVolume > SMALL_FLOAT);
}


bool phLiquidImpactData::SetFromQuadrangle(const Vector3 *FourVertices, const float *FourDepths)
{
	FastAssert(FourDepths[0] > 0.0f || FourDepths[1] > 0.0f || FourDepths[2] > 0.0f || FourDepths[3] > 0.0f);

	// Find the buoyancy for the triangle made from the first three polygon vertices
	bool bSubmerged = (FourDepths[0] > 0.0f || FourDepths[1] > 0.0f || FourDepths[2] > 0.0f) ? SetFromTriangle(FourVertices, FourDepths) : false;

	if(FourDepths[0] > 0.0f || FourDepths[2] > 0.0f || FourDepths[3] > 0.0f)
	{
		// Find the buoyancy for the triangle made from the third, fourth and first polygon vertices
		Vector3 newTriangle[3];
		newTriangle[0].Set(FourVertices[2]);
		newTriangle[1].Set(FourVertices[3]);
		newTriangle[2].Set(FourVertices[0]);
		float newDepths[3];
		if(FourDepths != NULL)
		{
			newDepths[0] = FourDepths[2];
			newDepths[1] = FourDepths[3];
			newDepths[2] = FourDepths[0];
			FourDepths = newDepths;
		}

		phLiquidImpactData SpareImpactData;
		if(SpareImpactData.SetFromTriangle(newTriangle, FourDepths))
		{
			if(bSubmerged)
			{
				// Both triangles are submerged, so combine the two triangle buoyancies.
				//BuoyancyData oldBuoyancy = *pBuoyancy;
				phLiquidImpactData OldImpactData = *this;
				CombineImpactDatas(OldImpactData, SpareImpactData);
			}
			else
			{
				// The second triangle is submerged and the first is not.
				m_ForcePos.Set(SpareImpactData.m_ForcePos);
				m_Normal.Set(SpareImpactData.m_Normal);
				m_SubmergedArea = SpareImpactData.m_SubmergedArea;
				m_SubmergedVolume = SpareImpactData.m_SubmergedVolume;
				m_PartIndex = SpareImpactData.m_PartIndex;
				m_Component = SpareImpactData.m_Component;
				bSubmerged = true;
				LIQUID_ASSERT_LEGIT(m_SubmergedVolume);
				LIQUID_ASSERT_LEGIT(m_SubmergedArea);
				LIQUID_ASSERT_LEGIT(m_ForcePos);
			}
		}
	}

	//	PF_STOP(Liquid_QuadrangleBuoyancy);
	return bSubmerged;
}


bool phLiquidImpactData::SetFromSphere(const float *UNUSED_PARAM(FourDepths))
{
	FastAssert(false);
	return false;
}


#if 1
void phLiquidImpactData::SubmergedTriangleBuoyancy(const Vector3 *pavecThreeVertices, float fDepthSum, const float *pafThreeDepths, float fImpulseScale)
{
	// What the heck is all of this stuff doing?
	if(fDepthSum < VERY_SMALL_FLOAT)
	{
		m_ForcePos.Zero();
		m_SubmergedVolume = 0.0f;
		return;
	}

	Vector3 edge01;
	edge01.Subtract(pavecThreeVertices[1], pavecThreeVertices[0]);
	float edge1S = edge01.Mag();
	if (edge1S>1.0e-10f)
	{
		edge01.InvScale(edge1S);
	}
	else
	{
		edge01.Scale(1.0e10f);
	}

	Vector3 edge02;
	edge02.Subtract(pavecThreeVertices[2], pavecThreeVertices[0]);
	float edge2S = edge02.Dot(edge01);
	float edge2T2 = edge02.Mag2() - square(edge2S);

	if (edge2T2<0.0f)
	{
		//		PF_STOP(Liquid_SubmergedTriangleBuoyancy);
		m_ForcePos.Zero();
		m_SubmergedVolume = 0.0f;
		return;
	}

	float edge2T = sqrtf(edge2T2);
	float invDepthSum = 1.0f / fDepthSum;
	float tF = edge2T * 0.25f * (pafThreeDepths[0] + pafThreeDepths[1] + 2.0f * pafThreeDepths[2]) * invDepthSum;
	if (tF<0.0f || tF>edge2T)
	{
		tF = 0.5f*edge2T;
	}

	float sF = 0.25f*(edge1S*(pafThreeDepths[0] + 2.0f * pafThreeDepths[1] + pafThreeDepths[2]) + edge2S * (pafThreeDepths[0] + pafThreeDepths[1] +
		2.0f * pafThreeDepths[2])) * invDepthSum;

	if (sF<0.0f || sF>edge1S)
	{
		sF = edge2S;
	}

	Vector3 impulsePoint(pavecThreeVertices[1]);
	impulsePoint.Subtract(pavecThreeVertices[0]);

	if (fabsf(edge1S)>VERY_SMALL_FLOAT)
	{
		impulsePoint.Scale(sF/edge1S);
	}

	impulsePoint.Add(pavecThreeVertices[0]);
	Vector3 tempVec(pavecThreeVertices[1]);
	tempVec.Subtract(pavecThreeVertices[0]);
	if(fabsf(edge1S) > VERY_SMALL_FLOAT)
	{
		tempVec.Scale(edge2S/edge1S);
	}

	tempVec.Add(pavecThreeVertices[0]);
	tempVec.Negate();
	tempVec.Add(pavecThreeVertices[2]);

	if (fabsf(edge2T)>VERY_SMALL_FLOAT)
	{
		tempVec.Scale(tF/edge2T);
	}

	impulsePoint.Add(tempVec);

	m_ForcePos.Set(impulsePoint);
	m_SubmergedVolume = fImpulseScale * fDepthSum * 0.3333f;
	LIQUID_ASSERT_LEGIT(m_SubmergedVolume);
	LIQUID_ASSERT_LEGIT(m_ForcePos);
}
#else
void phLiquidImpactData::SubmergedTriangleBuoyancy(const Vector3 *pavecThreeVertices, float fDepthSum, const float *UNUSED_PARAM(pafThreeDepths), float fImpulseScale)
{
	m_ForcePos.Add(pavecThreeVertices[0], pavecThreeVertices[1]);
	m_ForcePos.Add(pavecThreeVertices[2]);
	m_ForcePos.Scale(0.3333333333333333f);
	m_SubmergedVolume = fImpulseScale * fDepthSum * 0.3333f;
	LIQUID_ASSERT_LEGIT(m_SubmergedVolume);
	LIQUID_ASSERT_LEGIT(m_ForcePos);
}
#endif


void phLiquidImpactData::CombineImpactDatas(const phLiquidImpactData &kImpactDataA, const phLiquidImpactData &kImpactDataB)
{
	FastAssert((&kImpactDataA != this) && (&kImpactDataB != this));

	// Set the position as the weighted sum of the two given positions.
	Vector3 vecOffset(kImpactDataA.m_ForcePos);
	vecOffset.Subtract(kImpactDataB.m_ForcePos);
	m_SubmergedVolume = kImpactDataA.m_SubmergedVolume + kImpactDataB.m_SubmergedVolume;
	if(fabsf(m_SubmergedVolume) > VERY_SMALL_FLOAT)
	{
		vecOffset.Scale(kImpactDataA.m_SubmergedVolume / m_SubmergedVolume);
	}

	m_ForcePos.Set(kImpactDataB.m_ForcePos);
	m_ForcePos.Add(vecOffset);

	// Add the two submerged areas.
	m_SubmergedArea = kImpactDataA.m_SubmergedArea + kImpactDataB.m_SubmergedArea;
	LIQUID_ASSERT_LEGIT(m_SubmergedArea);
	LIQUID_ASSERT_LEGIT(m_SubmergedVolume);
	LIQUID_ASSERT_LEGIT(m_ForcePos);

	m_CompletelySubmerged = kImpactDataA.m_CompletelySubmerged & kImpactDataB.m_CompletelySubmerged;
}

} // namespace rage

#endif // end of #ifndef PHEFFECTS_LIQUIDIMPACTDATA_H
