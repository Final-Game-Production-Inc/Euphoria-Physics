//
// phbound/boundsurface.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//
#include "boundsurface.h"

#include "boundcapsule.h"
#include "boundpolyhedron.h"
#include "liquidimpactset.h"
#include "surfacegrid.h"

#include "data/struct.h"
#include "grcore/viewport.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "system/timemgr.h"
#include "atl/array_struct.h"

namespace rage {

#if USE_SURFACES

#define USE_BOUNDING_BOX_FOR_POLYS	1
#define LERP_HEIGHTS	1

EXT_PFD_DECLARE_GROUP(Bounds);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);

PFD_DECLARE_SUBGROUP(phBoundSurface, Bounds);
PFD_DECLARE_ITEM(DepthQueries,Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(SurfaceBox,Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(WaterVelocity,Color_pink, phBoundSurface);
PFD_DECLARE_ITEM_SLIDER(WaterVelocityLength, Color_pink, phBoundSurface, 10.0f, 10000.0f, 0.1f);
PFD_DECLARE_ITEM(IgnoreLiquidImpactsCapsule, Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(IgnoreLiquidImpactsBoundingBox, Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(IgnoreLiquidImpactsBox, Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(IgnoreLiquidImpactsPoly, Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(IgnoreLiquidImpactsSphere, Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(IgnoreLiquidImpactsSphere2, Color_pink, phBoundSurface);
PFD_DECLARE_ITEM(DebugLiquidImpactsTreatCapsuleAsCapsules, Color_pink, phBoundSurface);

IMPLEMENT_PLACE(phBoundSurface);

phBoundSurface::phBoundSurface()
: m_PeakWaveHeight(2.5f)
, m_SurfaceGrid(NULL)
{
	m_Type = SURFACE;
	m_SurfaceGrid = NULL;
}

phBoundSurface::phBoundSurface(datResource &rsc)
: phBound(rsc)
, m_VelocityGrid(rsc)
, m_OffsetGrid(rsc)
{
}

#if __DECLARESTRUCT
void phBoundSurface::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(phBoundSurface, phBound)
	SSTRUCT_FIELD(phBoundSurface, m_VelocityGrid)
	SSTRUCT_FIELD(phBoundSurface, m_OffsetGrid)
	SSTRUCT_FIELD(phBoundSurface, m_PeakWaveHeight)
	SSTRUCT_FIELD(phBoundSurface, m_Spacing)
	SSTRUCT_FIELD(phBoundSurface, m_MinElevation)
	SSTRUCT_FIELD(phBoundSurface, m_MaxElevation)
	SSTRUCT_FIELD(phBoundSurface, m_CellX)
	SSTRUCT_FIELD(phBoundSurface, m_CellY)

	SSTRUCT_FIELD(phBoundSurface, m_MinX)
	SSTRUCT_FIELD(phBoundSurface, m_MaxX)
	SSTRUCT_FIELD(phBoundSurface, m_MinZ)
	SSTRUCT_FIELD(phBoundSurface, m_MaxZ)
	SSTRUCT_FIELD_VP(phBoundSurface, m_SurfaceGrid)
	SSTRUCT_IGNORE(phBoundSurface, m_Pad)
	SSTRUCT_END(phBoundSurface)
}
#endif // __DECLARESTRUCT

phBoundSurface::~phBoundSurface()
{
}

const phMaterial &phBoundSurface::GetMaterial(phMaterialIndex UNUSED_PARAM(nMaterialIndex)) const
{
	Assert(false);
	phMaterial *Material = NULL;
	return *Material;
}

phMaterialMgr::Id phBoundSurface::GetMaterialId(phMaterialIndex UNUSED_PARAM(nMaterialIndex)) const
{
	Assert(false);
	return 0;
}

bool phBoundSurface::FindLiquidImpactsToSphere(phLiquidImpactSet &ImpactSet) const
{
#if __PFDRAW
	if(PFD_IgnoreLiquidImpactsSphere.GetEnabled())
	{
		return false;
	}
#endif

	Assert(ImpactSet.m_BoundA == static_cast<const phBound*>(this));
	// The other bound does not have to be a phBoundSphere.

	// Get the liquid position, and verify that its matrix is axis-aligned.
	const Vector3 &kSurfacePos = ImpactSet.m_CurrentA->d;
	Assert(ImpactSet.m_CurrentA->a.IsClose(XAXIS, SMALL_FLOAT));
	Assert(ImpactSet.m_CurrentA->b.IsClose(YAXIS, SMALL_FLOAT));
	Assert(ImpactSet.m_CurrentA->c.IsClose(ZAXIS, SMALL_FLOAT));

	// Get the sphere's bound, radius, position and orientation, and the sphere center in world coordinates.
	const phBound *BoundSphere = ImpactSet.m_BoundB;
	float SphereRadius = BoundSphere->GetRadiusAroundCentroid();
	const Matrix34 *MatrixSphere = ImpactSet.m_CurrentB;
	Vector3 SphereCenter;
	SphereCenter = VEC3V_TO_VECTOR3(BoundSphere->GetWorldCentroid(RCC_MAT34V(*MatrixSphere)));

	if((m_PeakWaveHeight + kSurfacePos.y) < (SphereCenter.y - SphereRadius))
	{
		// The sphere can not reach any point lower than the maximum wave height.
		return false;
	}

	// Find the depth, impact position on the sphere, and normal pointing into the liquid.
	phLiquidImpactData ImpactData;
	SphereCenter.Subtract(kSurfacePos);		// Now SphereCenter is in the surface's local coordinate system.

	if(FindLiquidImpactsToSphere(SphereCenter, SphereRadius, ImpactData))
	{
		ImpactData.m_ForcePos.Add(kSurfacePos);
		ImpactSet.AddImpactData(ImpactData);

		return true;
	}
	else
	{
		return false;
	}
}


bool phBoundSurface::FindLiquidImpactsToCapsule(phLiquidImpactSet &ImpactSet) const
{
	// There are currently issues with the capsule impact math. In the __PFDRAW builds, allow
	// the user to enable the problem code but default the behavior to using the more accurate
	// poly version of this function.
	// In other builds, just use the poly version until this is corrected.
#if __PFDRAW
	if(PFD_IgnoreLiquidImpactsCapsule.GetEnabled())
	{
		return false;
	}
	if(!PFD_DebugLiquidImpactsTreatCapsuleAsCapsules.GetEnabled())
	{
		return FindLiquidImpactsToPoly(ImpactSet);
	}

	Assert(ImpactSet.m_BoundA == static_cast<const phBound*>(this));
	Assert(ImpactSet.m_BoundB->GetType() == phBound::CAPSULE);

	// Get the liquid position, and verify that its matrix is axis-aligned.
	const Vector3 &kSurfacePos = ImpactSet.m_CurrentA->d;
	Assert(ImpactSet.m_CurrentA->a.IsClose(XAXIS, SMALL_FLOAT));
	Assert(ImpactSet.m_CurrentA->b.IsClose(YAXIS, SMALL_FLOAT));
	Assert(ImpactSet.m_CurrentA->c.IsClose(ZAXIS, SMALL_FLOAT));

	// Get the capsule's bound, radius, position and orientation, and the center in world coordinates.
	const phBoundCapsule *BoundCapsule = static_cast<phBoundCapsule *>(ImpactSet.m_BoundB);
	float CapsuleRadius = BoundCapsule->GetRadius();
	const Matrix34 *MatrixCapsule = ImpactSet.m_CurrentB;
	Vector3 SphereCenter;

	phLiquidImpactData ImpactData;
	bool FoundImpact = false;
	const int knNumSpheresToTest = Max(1 + (int)(0.5f * BoundCapsule->GetLength() / CapsuleRadius), 2);
	const float kOONumSpheresToTest = 1.0f / (float)(knNumSpheresToTest);
	const float kOONumSpheresToTestMinusOne = 1.0f / (float)(knNumSpheresToTest - 1);
	const float kVolumeScaleFactor = (0.75f * BoundCapsule->GetLength() / CapsuleRadius + 1.0f) * kOONumSpheresToTest;
	for(int PartIndex = 0; PartIndex < knNumSpheresToTest; ++PartIndex)
	{
		// Get the center point of each of the spheres on the ends.
		SphereCenter = VEC3V_TO_VECTOR3(BoundCapsule->GetWorldCentroid(RCC_MAT34V(*MatrixCapsule)));
		SphereCenter.SubtractScaled(MatrixCapsule->b, 0.5f * BoundCapsule->GetLength());
		SphereCenter.AddScaled(MatrixCapsule->b, BoundCapsule->GetLength() * (float)(PartIndex) * kOONumSpheresToTestMinusOne);

		if((m_PeakWaveHeight + kSurfacePos.y) < (SphereCenter.y - CapsuleRadius))
		{
			// The sphere can not reach any point lower than the maximum wave height.
			continue;
		}

		SphereCenter.Subtract(kSurfacePos);
		if(FindLiquidImpactsToSphere(SphereCenter, CapsuleRadius, ImpactData))
		{
			// We calculated the submerged volume based on the spheres at the ends, but the capsule is more than just the spheres at
			//   the ends.  If we don't account for that, then the buoyant forces won't be large enough to lift the whole capsule, so
			//   we make an approximation to submerged volume of the capsule by scaling by the ratio of the "volume of half of the
			//   capsule" to "the volume of the sphere on one end".
			ImpactData.m_SubmergedVolume *= kVolumeScaleFactor;
			ImpactData.m_ForcePos.Add(kSurfacePos);
			ImpactSet.AddImpactData(ImpactData);
			FoundImpact = true;
		}
	}

	return FoundImpact;
#else
	return FindLiquidImpactsToPoly(ImpactSet);
#endif
}


bool phBoundSurface::FindLiquidImpactsToBox(phLiquidImpactSet &ImpactSet) const
{
#if __PFDRAW
	if(PFD_IgnoreLiquidImpactsBox.GetEnabled())
	{
		return false;
	}
#endif

	return FindLiquidImpactsToPoly(ImpactSet);
}

bool phBoundSurface::FindLiquidImpactsToPoly(phLiquidImpactSet &ImpactSet) const
{
#if __PFDRAW
	if(PFD_IgnoreLiquidImpactsPoly.GetEnabled())
	{
		return false;
	}
#endif

	sysTimer oTimer;
	Assert(ImpactSet.m_BoundA == static_cast<const phBound*>(this));

	// Get the bound position, and verify that its matrix is axis-aligned.
	const Vector3 &krvecSurfacePos = ImpactSet.m_CurrentA->d;
	Assert(ImpactSet.m_CurrentA->a.IsClose(XAXIS,SMALL_FLOAT));
	Assert(ImpactSet.m_CurrentA->b.IsClose(YAXIS,SMALL_FLOAT));
	Assert(ImpactSet.m_CurrentA->c.IsClose(ZAXIS,SMALL_FLOAT));

	// Get the polyhedron's bound, orientation and position.
	const phBound *kpBound = static_cast<const phBound*>(ImpactSet.m_BoundB);
	const Matrix34 *pmtxPolyCurrent = ImpactSet.m_CurrentB;

	Vector3 vecBoxHalfWidths, vecBoxCenter;
	if(!BoxIntersectsBoundingBox(krvecSurfacePos, kpBound, pmtxPolyCurrent, vecBoxHalfWidths, vecBoxCenter))
	{
		// The box can not reach any point lower than the maximum wave height.
		return false;
	}

	bool FoundSubmergedVert = false;

#if 1
#if USE_BOUNDING_BOX_FOR_POLYS
	FoundSubmergedVert = true;
#else
	Assert(ImpactSet.m_BoundB->IsPolygonal());
	const phBoundPolyhedron *kpPolyBound = static_cast<const phBoundPolyhedron*>(ImpactSet.m_BoundB);
	Vector3 vertex;
	int nVertIndex, nNumVertices = kpPolyBound->GetNumVertices();
	for(nVertIndex = 0; nVertIndex < nNumVertices; ++nVertIndex)
	{
		// Transform the polyhedron vertices to world coordinates.
		pmtxPolyCurrent->Transform(kpPolyBound->GetVertex(nVertIndex), vertex);
		vertex.Subtract(krvecSurfacePos);

		// Find the depth in the liquid at every vertex in the polyhedron.  Any vertices over MAX_NUM_VERTS
		// will not contribute to buoyancy, so objects with too many vertices will not float properly.
		if(GetDepth(vertex) > 0.0f)
		{
			FoundSubmergedVert = true;
			break;
		}
	}
#endif
#else

	// no sense in testing all the verts if we are just doing bounding boxes anyway.
	Vector3 avecVertices[MAX_NUM_VERTS];
	float afDepth[MAX_NUM_VERTS];
	int nVertIndex, nNumVertices = Min(kpPolyBound->GetNumVertices(), MAX_NUM_VERTS);
	for(nVertIndex = 0; nVertIndex < nNumVertices; ++nVertIndex)
	{
		// Transform the polyhedron vertices to world coordinates.
		pmtxPolyCurrent->Transform(kpPolyBound->GetVertex(nVertIndex), avecVertices[nVertIndex]);
		avecVertices[nVertIndex].Subtract(krvecSurfacePos);

		// Find the depth in the liquid at every vertex in the polyhedron.  Any vertices over MAX_NUM_VERTS
		// will not contribute to buoyancy, so objects with too many vertices will not float properly.
		afDepth[nVertIndex] = GetDepth(avecVertices[nVertIndex]);
		FoundSubmergedVert = FoundSubmergedVert || afDepth[nVertIndex] > 0.0f;
	}
#endif

	if(!FoundSubmergedVert)
	{
		// This isn't quite the same check 
		// This checks the actual transormed vertices of the polyhedron against the actual vertices of the surface.
		// The above check checks the bounding box of the polyhderon against the worst case bounding box of the bound.
		return false;
	}

#if USE_BOUNDING_BOX_FOR_POLYS
	return FindLiquidImpactsToBoundingBox(ImpactSet);
#else
	// Test the bound faces for impacts with the liquid.
	// The buoyancy is the impulse location (x,y,z) and impulse magnitude (w).
	phLiquidImpactData ImpactData, TempImpactData;
	Vector3 clippedVerts[8],polyVerts[4];
	float clippedDepths[8],polyDepths[4];
	int nNumPolyVerts;
	bool submerged;
	for(int nPolyIndex = kpPolyBound->GetNumPolygons() - 1; nPolyIndex >= 0; --nPolyIndex)
	{
		// Get the polygon, the number of vertices, the vertex locations and the vertex depths.
		const phPolygon& polygon = kpPolyBound->GetPolygon(nPolyIndex);
		nNumPolyVerts = polygon.GetNumVerts();
		FoundSubmergedVert = false;
		for(int polyVertIndex = 0; polyVertIndex < nNumPolyVerts; polyVertIndex++)
		{
			nVertIndex = polygon.GetVertexIndex(polyVertIndex);
			polyVerts[polyVertIndex].Set(avecVertices[nVertIndex]);
			polyDepths[polyVertIndex] = afDepth[nVertIndex];
			FoundSubmergedVert = FoundSubmergedVert || polyDepths[polyVertIndex] > 0.0f;
		}

		if(!FoundSubmergedVert)
		{
			continue;
		}

		// Reset the buoyancy data.
		// TODO: Replace this with a Clear() call.
		ImpactData.m_ForcePos.Zero();
		ImpactData.m_SubmergedVolume = 0.0f;
		ImpactData.m_SubmergedArea = 0.0f;
		submerged = false;

		// Cut the polygon to fit inside the liquid borders.  There is a maximum of 8 clipped vertices, for
		// a quad covering three border corners.
		//		PF_START(Liquid_ClipAgainstLiquidBorders);
		int numClippedVerts = ClipAgainstBorders(nNumPolyVerts,polyVerts,polyDepths,clippedVerts,clippedDepths);
		//		PF_STOP(Liquid_ClipAgainstLiquidBorders);

		Assert(numClippedVerts >= 3);
		if(numClippedVerts>=3)
		{
			// Triangulate the clipped polygon and compute the buoyancy.
			for(nVertIndex = 1; numClippedVerts - nVertIndex > 3; ++nVertIndex)
			{
				Vector3 tri[3] = { clippedVerts[0], clippedVerts[nVertIndex], clippedVerts[nVertIndex + 1] };
				float triDepth[3] = { clippedDepths[0], clippedDepths[nVertIndex], clippedDepths[nVertIndex + 1] };

				// See if this triangle in the polyhedron touches the bound.
				if(TempImpactData.SetFromTriangle(tri, triDepth))
				{
					phLiquidImpactData OldImpactData = ImpactData;
					ImpactData.CombineImpactDatas(OldImpactData, TempImpactData);
					submerged = true;
				}
			}

			// Clean up last remaining triangle or quadrangle of the polygon.
			if((numClippedVerts - nVertIndex) == 3)
			{	
				Vector3 quad[4] = { clippedVerts[0], clippedVerts[nVertIndex], 
					clippedVerts[nVertIndex + 1], clippedVerts[nVertIndex + 2] };
				float quadDepth[4] = { clippedDepths[0], clippedDepths[nVertIndex],
					clippedDepths[nVertIndex + 1], clippedDepths[nVertIndex + 2] };

				// See if this quadrangle in the polyhedron touches the bound.
				if(TempImpactData.SetFromQuadrangle(quad, quadDepth))
				{
					phLiquidImpactData OldImpactData = ImpactData;
					ImpactData.CombineImpactDatas(OldImpactData, TempImpactData);
					submerged = true;
				}
			}
			else
			{
				Assert(numClippedVerts - nVertIndex == 2);
				Vector3 tri[3] = { clippedVerts[0], clippedVerts[nVertIndex], clippedVerts[nVertIndex + 1] };
				float triDepth[3] = { clippedDepths[0], clippedDepths[nVertIndex], clippedDepths[nVertIndex + 1] };

				// See if this triangle in the polyhedron touches the bound.
				if(TempImpactData.SetFromTriangle(tri, triDepth))
				{
					phLiquidImpactData OldImpactData = ImpactData;
					ImpactData.CombineImpactDatas(OldImpactData, TempImpactData);
					submerged = true;
				}
			}

			// At this point we know that the polygon has at least one vertex that is submerged, 
			if(submerged)
			{
				/////////////////////////////////////////////////////////////////////////////////////////////////////
				// Make last-minute additions or adjustments to the impact data and then add it to the impact set.

				// Get the polygon normal and impulse position in world coordinates.
				pmtxPolyCurrent->Transform3x3(kpPolyBound->GetPolygonUnitNormal(nPolyIndex), ImpactData.m_Normal);
				ImpactData.m_ForcePos.Add(krvecSurfacePos);

				ImpactData.m_PartIndex = (u16)(nPolyIndex);

				// Finally add the impact data to the impact set.
				ImpactSet.AddImpactData(ImpactData);

				//
				/////////////////////////////////////////////////////////////////////////////////////////////////////
			}
		}
	}

	// This has to be true because we checked all of the vertices earlier and at least one was submerged.
	return true;
#endif
}


bool phBoundSurface::FindLiquidImpactsToCurvedGeometry (phLiquidImpactSet& impactSet) const
{
#if USE_BOUNDING_BOX_FOR_POLYS
	return FindLiquidImpactsToPoly(impactSet);
#else
	return false;
#endif
}


bool phBoundSurface::FindLiquidImpactsToBoundingBox(phLiquidImpactSet &ImpactSet) const
{
#if __PFDRAW
	if(PFD_IgnoreLiquidImpactsBoundingBox.GetEnabled())
	{
		return false;
	}
#endif
	// 
	phBound &OtherBound = *ImpactSet.m_BoundB;
	Vector3 BoundingBoxHalfWidth, BoundingBoxOffset;
	OtherBound.GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(BoundingBoxHalfWidth), RC_VEC3V(BoundingBoxOffset));

	Vector3 BoundingBoxVerts_MS[8];
	float VertexDepths[8];
	for(int VertexIndex = 0; VertexIndex < 8; ++VertexIndex)
	{
		// Get the bounding box vertices in world space.
		BoundingBoxVerts_MS[VertexIndex].x = ((VertexIndex & 1) == 0) ? -BoundingBoxHalfWidth.x : BoundingBoxHalfWidth.x;
		BoundingBoxVerts_MS[VertexIndex].y = ((VertexIndex & 2) == 0) ? -BoundingBoxHalfWidth.y : BoundingBoxHalfWidth.y;
		BoundingBoxVerts_MS[VertexIndex].z = ((VertexIndex & 4) == 0) ? -BoundingBoxHalfWidth.z : BoundingBoxHalfWidth.z;
		BoundingBoxVerts_MS[VertexIndex].Add(BoundingBoxOffset);
		ImpactSet.m_CurrentB->Transform(BoundingBoxVerts_MS[VertexIndex]);

#if __PFDRAW
		const Vector3 drawStart(BoundingBoxVerts_MS[VertexIndex]);
#endif

		// Now transform the bounding box vertices into the surface's local space.
		BoundingBoxVerts_MS[VertexIndex].Subtract(ImpactSet.m_CurrentA->d);

		// Get the depth of the vertices.
		VertexDepths[VertexIndex] = GetDepth(BoundingBoxVerts_MS[VertexIndex]);
		LIQUID_ASSERT_LEGIT(VertexDepths[VertexIndex]);

#if __PFDRAW
		if(VertexDepths[VertexIndex] > 0.0f)
		{
			const Vector3 drawEnd(drawStart.x, drawStart.y + VertexDepths[VertexIndex], drawStart.z);
			PF_DRAW_LINE(DepthQueries, drawStart, drawEnd);
		}
#endif
	}

	const float kForceScaleFactor = 0.70f;//(ImpactSet.m_BoundB->GetVolume()) / (8.0f * BoundingBoxHalfWidth.x * BoundingBoxHalfWidth.y * BoundingBoxHalfWidth.z);
//	const float kOtherMass = 1.0f;
//	const float kForceScaleFactor = kOtherMass / (8.0f * BoundingBoxHalfWidth.x * BoundingBoxHalfWidth.y * BoundingBoxHalfWidth.z * 0.300f)
//	kForceScaleFactor;

	bool foundSubmerged = false;

	// Vector3 FaceCenter;
	phLiquidImpactData ImpactData;
	ImpactData.m_PartIndex = 0;
	for(int Axis = 0; Axis < 3; ++Axis)
	{
#if 1
		int NumSubmerged[2] = { 0, 0 };
		int VertexIndices[2][4];
		int NumFound[2] = { 0, 0 };
		bool allSubmerged = true;
		for(int VertexIndex = 0; VertexIndex < 8; ++VertexIndex)
		{
			bool submerged = VertexDepths[VertexIndex] > 0.0f;
			allSubmerged &= submerged;
			NumSubmerged[((VertexIndex >> Axis) & 1)] += submerged ? 1 : 0;
			VertexIndices[((VertexIndex >> Axis) & 1)][NumFound[((VertexIndex >> Axis) & 1)]] = VertexIndex;
			NumFound[((VertexIndex >> Axis) & 1)] += 1;
		}
		Assert(NumFound[0] == 4);
		Assert(NumFound[1] == 4);

		ImpactData.m_CompletelySubmerged &= allSubmerged;

		const Vector3 &kFaceNormal = Axis == 0 ? ImpactSet.m_CurrentB->a : Axis == 1 ? ImpactSet.m_CurrentB->b : ImpactSet.m_CurrentB->c;

		Vector3 FaceVertices[4];
		float FaceDepths[4];
		for(int AxisDir = 0; AxisDir < 2; ++AxisDir)
		{
			if(NumSubmerged[AxisDir] == 0)
			{
				continue;
			}

			SwapEm(VertexIndices[AxisDir][2], VertexIndices[AxisDir][3]);

			for(int FaceVertexIndex = 0; FaceVertexIndex < 4; ++FaceVertexIndex)
			{
				FaceVertices[FaceVertexIndex].Set(BoundingBoxVerts_MS[VertexIndices[AxisDir][FaceVertexIndex]]);
				FaceDepths[FaceVertexIndex] = VertexDepths[VertexIndices[AxisDir][FaceVertexIndex]];
				LIQUID_ASSERT_LEGIT(FaceVertices[FaceVertexIndex]);
				LIQUID_ASSERT_LEGIT(FaceDepths[FaceVertexIndex]);
			}

			if(ImpactData.SetFromQuadrangle(FaceVertices, FaceDepths))
			{
				ImpactData.m_ForcePos.Add(ImpactSet.m_CurrentA->d);
				ImpactData.m_SubmergedArea *= kForceScaleFactor;
				ImpactData.m_Normal.Set(AxisDir == 0 ? -kFaceNormal : kFaceNormal);
				ImpactData.m_PartIndex = 0;
				ImpactSet.AddImpactData(ImpactData);

				foundSubmerged = true;
			}
		}
#else
		// We need to find all of the vertices that are part of this face.
		float AverageDepths[2] = { 0.0f, 0.0f };
		int NumSubmerged[2] = { 0, 0 };
		int VertexIndices[2][4];
		int NumFound[2] = { 0, 0 };
		bool allSubmerged = true;
		for(int VertexIndex = 0; VertexIndex < 8; ++VertexIndex)
		{
			AverageDepths[((VertexIndex >> Axis) & 1)] += VertexDepths[VertexIndex];
			bool submerged = VertexDepths[VertexIndex] > 0.0f;
			allSubmerged &= submerged;
			NumSubmerged[((VertexIndex >> Axis) & 1)] += submerged ? 1 : 0;
			VertexIndices[((VertexIndex >> Axis) & 1)][NumFound[((VertexIndex >> Axis) & 1)]] = VertexIndex;
			NumFound[((VertexIndex >> Axis) & 1)] += 1;
		}
		Assert(NumFound[0] == 4);
		Assert(NumFound[1] == 4);

		AverageDepths[0] *= 0.25f;
		AverageDepths[1] *= 0.25f;

		float FaceArea = Axis != 0 ? 2.0f * BoundingBoxHalfWidth[0] : 1.0f;
		FaceArea *= Axis != 1 ? 2.0f * BoundingBoxHalfWidth[1] : 1.0f;
		FaceArea *= Axis != 2 ? 2.0f * BoundingBoxHalfWidth[2] : 1.0f;

		Vector3 ForcePos;

		const Vector3 &kFaceNormal = Axis == 0 ? ImpactSet.m_CurrentB->a : Axis == 1 ? ImpactSet.m_CurrentB->b : ImpactSet.m_CurrentB->c;

		for(int AxisDir = 0; AxisDir < 2; ++AxisDir)
		{
			float FinalDepth = AverageDepths[AxisDir];
			float FinalFaceArea = FaceArea;

			const int *FaceVertexIndices = VertexIndices[AxisDir];
			int DepthOrder[4] = { 0, 1, 2, 3 };

			if(NumSubmerged[AxisDir] > 0 && NumSubmerged[AxisDir] < 4)
			{
				// We need to find out which vertices are the most submerged, etc.
				bool FoundOutOfOrder;
				do
				{
					FoundOutOfOrder = false;
					for(int Pos = 0; Pos < 3; ++Pos)
					{
						if(VertexDepths[FaceVertexIndices[DepthOrder[Pos]]] < VertexDepths[FaceVertexIndices[DepthOrder[Pos + 1]]])
						{
							SwapEm(DepthOrder[Pos], DepthOrder[Pos + 1]);
							FoundOutOfOrder = true;
						}
					}
				}
				while(FoundOutOfOrder);
				Assert(VertexDepths[FaceVertexIndices[DepthOrder[0]]] >= VertexDepths[FaceVertexIndices[DepthOrder[1]]]);
				Assert(VertexDepths[FaceVertexIndices[DepthOrder[1]]] >= VertexDepths[FaceVertexIndices[DepthOrder[2]]]);
				Assert(VertexDepths[FaceVertexIndices[DepthOrder[2]]] >= VertexDepths[FaceVertexIndices[DepthOrder[3]]]);

				Assert(fabs(VertexDepths[FaceVertexIndices[DepthOrder[3]]] - VertexDepths[FaceVertexIndices[DepthOrder[2]]] - VertexDepths[FaceVertexIndices[DepthOrder[1]]] + VertexDepths[FaceVertexIndices[DepthOrder[0]]]) <= 0.0001f);
			}

			switch(NumSubmerged[AxisDir])
			{
				case 0:
				{
					// None of these vertices is submerged, so we should skip this face.
					continue;
					break;
				}
				case 1:
				{
					// Now let's make an adjustment to the area to compensate for the fact that only one corner is submerged.
					const float kAreaFactor = 0.5f * square(VertexDepths[FaceVertexIndices[DepthOrder[0]]]) / ((VertexDepths[FaceVertexIndices[DepthOrder[0]]] - VertexDepths[FaceVertexIndices[DepthOrder[1]]]) * (VertexDepths[FaceVertexIndices[DepthOrder[0]]] - VertexDepths[FaceVertexIndices[DepthOrder[2]]]));
					Assert(kAreaFactor >= 0.0f && kAreaFactor <= 1.0f);
					FinalFaceArea = FaceArea * kAreaFactor;

					FinalDepth = 0.3333333333f * VertexDepths[FaceVertexIndices[DepthOrder[0]]];

					// TEMP: This is just temporary until I replace it with the correct implementation.
//					ForcePos.Average(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]], BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[3]]]);
//					ForcePos.Add(ImpactSet.m_CurrentA->d);

					// These vectors are all in the surface's local space.
					Vector3 BasisX, BasisY;
					BasisX.Subtract(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[1]]], BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]]);
					BasisY.Subtract(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[2]]], BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]]);

					ForcePos.Set(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]]);
					const float kD0 = VertexDepths[FaceVertexIndices[DepthOrder[0]]];
					const float kD1 = VertexDepths[FaceVertexIndices[DepthOrder[1]]];
					const float kD2 = VertexDepths[FaceVertexIndices[DepthOrder[2]]];
					ForcePos.AddScaled(BasisX, 0.33333333f * kD0 / (kD0 - kD1));
					ForcePos.AddScaled(BasisY, 0.33333333f * kD0 / (kD0 - kD2));
					Assert(ForcePos.y <= 0.0f);
					ForcePos.Add(ImpactSet.m_CurrentA->d);

					foundSubmerged = true;
					break;
				}
				case 2:
				{
					const float kAreaFactor = 0.5f * (VertexDepths[FaceVertexIndices[DepthOrder[0]]] / (VertexDepths[FaceVertexIndices[DepthOrder[0]]] - VertexDepths[FaceVertexIndices[DepthOrder[2]]]) + VertexDepths[FaceVertexIndices[DepthOrder[1]]] / (VertexDepths[FaceVertexIndices[DepthOrder[1]]] - VertexDepths[FaceVertexIndices[DepthOrder[3]]]));
const float kAreaFactorNew = 0.5f * (VertexDepths[FaceVertexIndices[DepthOrder[0]]] + VertexDepths[FaceVertexIndices[DepthOrder[1]]]) / (VertexDepths[FaceVertexIndices[DepthOrder[0]]] - VertexDepths[FaceVertexIndices[DepthOrder[2]]]);
Assert(kAreaFactorNew <= 1.0001f * kAreaFactor && kAreaFactorNew >= 0.9999f * kAreaFactor);
					Assert(kAreaFactor >= 0.0f && kAreaFactor <= 1.0f);
					FinalFaceArea = FaceArea * kAreaFactor;

					FinalDepth = 0.25f * (VertexDepths[FaceVertexIndices[DepthOrder[0]]] + VertexDepths[FaceVertexIndices[DepthOrder[1]]]);

					// These vectors are all in the surface's local space.
					Vector3 BasisX, BasisY;
					BasisX.Subtract(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[1]]], BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]]);
					BasisY.Subtract(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[2]]], BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]]);

					const float kA1 = VertexDepths[FaceVertexIndices[DepthOrder[0]]];
					const float kA2 = VertexDepths[FaceVertexIndices[DepthOrder[1]]];

					ForcePos.Set(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]]);
					ForcePos.AddScaled(BasisX, 2.0f * kA2 / (kA1 + 3.0f * kA2));

					const float kValue = (square(kA1) + kA1 * kA2 + square(kA2)) / (3.0f * (kA1 + kA2));
					const float kValue2 = kValue / (VertexDepths[FaceVertexIndices[DepthOrder[0]]] - VertexDepths[FaceVertexIndices[DepthOrder[2]]]);
					ForcePos.AddScaled(BasisY, kValue2);
					Assert(ForcePos.y <= 0.0f);
					ForcePos.Add(ImpactSet.m_CurrentA->d);

					foundSubmerged = true;
					break;
				}
				case 3:
				{
					// Now let's make an adjustment to the area to compensate for the fact that one corner of the rectangle is not submerged.
					const float kAreaFactor = 1.0f - 0.5f * square(VertexDepths[FaceVertexIndices[DepthOrder[3]]]) / ((VertexDepths[FaceVertexIndices[DepthOrder[3]]] - VertexDepths[FaceVertexIndices[DepthOrder[1]]]) * (VertexDepths[FaceVertexIndices[DepthOrder[3]]] - VertexDepths[FaceVertexIndices[DepthOrder[2]]]));
					Assert(kAreaFactor > 0.0f && kAreaFactor <= 1.0f);
					FinalFaceArea = FaceArea * kAreaFactor;

					FinalDepth = AverageDepths[AxisDir] - 0.3333333333f * VertexDepths[VertexIndices[AxisDir][DepthOrder[3]]];

					// TEMP: This is just temporary until I replace it with the correct implementation.
					ForcePos.Average(BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[0]]], BoundingBoxVerts_MS[FaceVertexIndices[DepthOrder[3]]]);
					ForcePos.Add(ImpactSet.m_CurrentA->d);

					foundSubmerged = true;
					break;
				}
				case 4:
				{
					// All of these vertices are submerged.  Excellent!
					foundSubmerged = true;
					break;
				}
			}

			Assert(FinalDepth > 0.0f);
			ImpactData.m_Normal.Set(AxisDir == 0 ? -kFaceNormal : kFaceNormal);
			ImpactData.m_ForcePos.AddScaled(ImpactSet.m_CurrentB->d, ImpactData.m_Normal, BoundingBoxHalfWidth[Axis]);
			ImpactData.m_ForcePos.Add(BoundingBoxOffset);
			Assert(NumSubmerged[AxisDir] == 1 || NumSubmerged[AxisDir] == 2 || NumSubmerged[AxisDir] == 4 || ImpactData.m_ForcePos.Dist2(ForcePos) <= square(0.001f));
			if(NumSubmerged[AxisDir] < 3)
			{
				ImpactData.m_ForcePos.Set(ForcePos);
			}
			Assert(ImpactData.m_ForcePos.y <= ImpactSet.m_CurrentA->d.y);
			ImpactData.m_SubmergedArea = FinalFaceArea;
			Assert(ImpactData.m_SubmergedArea >= 0.0f);
			ImpactData.m_SubmergedVolume = -FinalDepth * FinalFaceArea * ImpactData.m_Normal.y;
			Assert(FinalDepth >= 0.0f);
			ImpactSet.AddImpactData(ImpactData);
		}
#endif
	}

	return foundSubmerged;
}


#if __PFDRAW

static inline float GetGridLocalCoordinate(int index)
{
	return (float) ((index * 2) - phSurfaceGrid::kSurfaceGridLength);	// HARD CODED 2 UNIT BECAUSE OF THIS! (and other stuff..)
}

void phBoundSurface::Draw(Mat34V_In kInstanceMatrixIn, bool UNUSED_PARAM(colorMaterials)/* = false*/, bool solid, int UNUSED_PARAM(whichPolys)/* = ALL_POLYS*/, phMaterialFlags UNUSED_PARAM(highlightFlags)/* = 0*/, unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	const Matrix34& kInstanceMatrix = RCC_MATRIX34(kInstanceMatrixIn);
	sysTimer oTimer;

	Vector3 PolyVerts[4];

//	Vector3 DirLight(0.436436f, 0.872872f, 0.218218f);
//	Assert(DirLight.Mag2() < square(1.01f) && DirLight.Mag2() > square(0.99f));
//	DirLight.Scale(0.5f);

	grcWorldMtx(kInstanceMatrix);

	grcViewport *CurViewPort = grcViewport::GetCurrent();
	Vector3 cameraWorldLoc = VEC3V_TO_VECTOR3(CurViewPort->GetCameraPosition());
	Vector3 cameraLoc = kInstanceMatrix.d -cameraWorldLoc;
	
	// TODO: This will have to get changed once we adjust the instance location to be the center of the bound and not the center of the surface.
	Vector3 Vertices[8];
	float top = m_MaxElevation;
	float bottom = m_MinElevation;

	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMin().GetXf(), top, GetBoundingBoxMin().GetZf()), Vertices[0]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMax().GetXf(), top, GetBoundingBoxMin().GetZf()), Vertices[1]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMax().GetXf(), top, GetBoundingBoxMax().GetZf()), Vertices[2]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMin().GetXf(), top, GetBoundingBoxMax().GetZf()), Vertices[3]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMin().GetXf(), bottom, GetBoundingBoxMin().GetZf()), Vertices[4]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMax().GetXf(), bottom, GetBoundingBoxMin().GetZf()), Vertices[5]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMax().GetXf(), bottom, GetBoundingBoxMax().GetZf()), Vertices[6]);
	kInstanceMatrix.Transform(Vector3(GetBoundingBoxMin().GetXf(), bottom, GetBoundingBoxMax().GetZf()), Vertices[7]);

	float maxDrawDistance = 32 + PFD_BoundDrawDistance.GetValue();
	float maxDraw2 = maxDrawDistance*maxDrawDistance;
	bool visible = false;
	for (int cornerIndex = 0; cornerIndex< 8; cornerIndex++)
	{
		Vector3 delta = Vertices[cornerIndex] - cameraWorldLoc;
		
		if(delta.Mag2() < maxDraw2)
		{
			visible = true;
			break;
		}
	}
	if(!visible)
	{
		return;
	}

	if(CurViewPort->IsPointSetVisible((Vec3V*)Vertices, 8))
	{
		if(PFD_SurfaceBox.GetEnabled())
		{
			grcWorldIdentity();
			if(m_SurfaceGrid)
			{
				if((m_SurfaceGrid->GetCellX() + m_SurfaceGrid->GetCellZ()) % 2)
				{
					grcColor(Color_pink);
				}
				else
				{
					grcColor(Color_GreenYellow);
				}
			}
			else
			{
				grcColor(Color_red);
			}

			grcBegin(drawLineStrip, 5);
			grcVertex3f(Vertices[0]);
			grcVertex3f(Vertices[1]);
			grcVertex3f(Vertices[2]);
			grcVertex3f(Vertices[3]);
			grcVertex3f(Vertices[0]);
			grcEnd();


			grcBegin(drawLines, 2);
			grcVertex3f(Vertices[0]);
			grcVertex3f(Vertices[4]);
			grcEnd();

			grcBegin(drawLines, 2);
			grcVertex3f(Vertices[1]);
			grcVertex3f(Vertices[5]);
			grcEnd();

			grcBegin(drawLines, 2);
			grcVertex3f(Vertices[2]);
			grcVertex3f(Vertices[6]);
			grcEnd();

			grcBegin(drawLines, 2);
			grcVertex3f(Vertices[3]);
			grcVertex3f(Vertices[7]);
			grcEnd();

			grcBegin(drawLineStrip, 5);
			grcVertex3f(Vertices[4]);
			grcVertex3f(Vertices[5]);
			grcVertex3f(Vertices[6]);
			grcVertex3f(Vertices[7]);
			grcVertex3f(Vertices[4]);
			grcEnd();

			grcWorldMtx(kInstanceMatrix);
		}

		if(m_SurfaceGrid != NULL)
		{

			static Vector3 lightVector(0.436436f, 0.872872f, 0.218218f);

			// Create a grid of elevations, sized one larger than the grid so we can make triangles that
			// hook into adjacent grids.
			// This is a simplification of the earlier debug renderer because this isn't bothering to calculate
			// normals.
			float minElevation = 1000000.0f;
			float maxElevation = -1000000.0f;

			Vector3 vertexLocs[phSurfaceGrid::kSurfaceGridLength+1][phSurfaceGrid::kSurfaceGridLength+1];
			bool validLocs[phSurfaceGrid::kSurfaceGridLength+1][phSurfaceGrid::kSurfaceGridLength+1];
			Vector2 velocities[phSurfaceGrid::kSurfaceGridLength+1][phSurfaceGrid::kSurfaceGridLength+1];
			for(int gridX = 0; gridX < phSurfaceGrid::kSurfaceGridLength; gridX++)
			{
				float x = GetGridLocalCoordinate(gridX);
				for(int gridZ = 0; gridZ < phSurfaceGrid::kSurfaceGridLength; gridZ++)
				{
					Vector3& vertexLoc = vertexLocs[gridX][gridZ];
					bool& validLoc = validLocs[gridX][gridZ];
					vertexLoc.x = x;
					vertexLoc.y = m_SurfaceGrid->GetElevation(gridX, gridZ,validLoc);
					vertexLoc.z = GetGridLocalCoordinate(gridZ);
					if(vertexLoc.y > maxElevation)
					{
						maxElevation = vertexLoc.y;
					}
					if(vertexLoc.y < minElevation)
					{
						minElevation = vertexLoc.y;
					}
					
					velocities[gridX][gridZ] = m_SurfaceGrid->GetVelocity(gridX, gridZ);
				}
			}

			// find the edges, which may be defined by adjacent grids, so use the 'find' function to allow it to look for it.
			// Yes it could be a bit faster by looking up the edge grid and calling GetElevation directly, etc, but it *is* debug drawing..
			float edge = GetGridLocalCoordinate(phSurfaceGrid::kSurfaceGridLength);

			for(int index = 0; index < phSurfaceGrid::kSurfaceGridLength; index++)
			{
				Vector3& edgeXVertexLoc = vertexLocs[index][phSurfaceGrid::kSurfaceGridLength];
				bool& edgeXValidLoc = validLocs[index][phSurfaceGrid::kSurfaceGridLength];
				edgeXVertexLoc.x = GetGridLocalCoordinate(index);
				edgeXVertexLoc.y = m_SurfaceGrid->FindElevation(index, phSurfaceGrid::kSurfaceGridLength,edgeXValidLoc);
				edgeXVertexLoc.z = edge;
				Vector3& edgeZVertexLoc = vertexLocs[phSurfaceGrid::kSurfaceGridLength][index];
				bool& edgeZValidLoc = validLocs[phSurfaceGrid::kSurfaceGridLength][index];
				edgeZVertexLoc.x = edge;
				edgeZVertexLoc.y = m_SurfaceGrid->FindElevation(phSurfaceGrid::kSurfaceGridLength, index,edgeZValidLoc);
				edgeZVertexLoc.z = GetGridLocalCoordinate(index);
			}

			// Now the corner, which may be defined by yet another grid
			Vector3& cornerVertexLoc = vertexLocs[phSurfaceGrid::kSurfaceGridLength][phSurfaceGrid::kSurfaceGridLength];
			bool cornerValidLoc = validLocs[phSurfaceGrid::kSurfaceGridLength][phSurfaceGrid::kSurfaceGridLength];
			cornerVertexLoc.x = edge;
			cornerVertexLoc.y = m_SurfaceGrid->FindElevation(phSurfaceGrid::kSurfaceGridLength, phSurfaceGrid::kSurfaceGridLength, cornerValidLoc);
			cornerVertexLoc.z = edge;

			int colorMode = (m_SurfaceGrid->GetCellX() + m_SurfaceGrid->GetCellZ()) % 2;
			Vector3 currentColor;
			if(colorMode)
			{
				currentColor = VEC3V_TO_VECTOR3(Color_SpringGreen.GetRGB());
			}
			else
			{
				currentColor = VEC3V_TO_VECTOR3(Color_magenta.GetRGB());
			}

			// This could be made faster by doing the interior using GetElevations, etc, but this is debug drawing afterall..
			Vector3 vertexNormals[phSurfaceGrid::kSurfaceGridLength+1][phSurfaceGrid::kSurfaceGridLength+1];
			Vector3 vertexColors[phSurfaceGrid::kSurfaceGridLength+1][phSurfaceGrid::kSurfaceGridLength+1];
			for(int gridX = 0; gridX <= phSurfaceGrid::kSurfaceGridLength; gridX++)
			{
				for(int gridZ = 0; gridZ <= phSurfaceGrid::kSurfaceGridLength; gridZ++)
				{
					Vector3& normal(vertexNormals[gridX][gridZ]);
					m_SurfaceGrid->CalculateNormal(gridX, gridZ, 2.0f, normal);

					Vector3& color(vertexColors[gridX][gridZ]);
					float dot = normal.Dot(lightVector);
					color.Set(dot * 0.45f, dot * 0.45f, dot * 0.8f);
					color.Multiply(currentColor);
				}
			}


			// Do the actual drawing.
			grcDrawMode mode = solid ? drawTriStrip : drawLineStrip;
			for(int gridZ = 0; gridZ < phSurfaceGrid::kSurfaceGridLength; ++gridZ)
			{
				for(int gridX = 0; gridX < phSurfaceGrid::kSurfaceGridLength; ++gridX)
				{
					if(validLocs[gridX][gridZ])
					{
						Vector3 worldVertex;
						kInstanceMatrix.Transform(vertexLocs[gridX][gridZ],worldVertex);
						Vector3 delta = worldVertex - cameraWorldLoc;
						if(delta.Mag2() < maxDraw2)
						{
							grcBegin(mode, 4);
							grcColor(vertexColors[gridX][gridZ]);
							grcNormal3f(vertexNormals[gridX][gridZ]);
							grcVertex3f(vertexLocs[gridX][gridZ]);

							grcColor(vertexColors[gridX][gridZ+1]);
							grcNormal3f(vertexNormals[gridX][gridZ+1]);
							grcVertex3f(vertexLocs[gridX][gridZ+1]);

							int gridX2 = gridX + 1;
							grcColor(vertexColors[gridX2][gridZ]);
							grcNormal3f(vertexNormals[gridX2][gridZ]);
							grcVertex3f(vertexLocs[gridX2][gridZ]);

							grcColor(vertexColors[gridX2][gridZ+1]);
							grcNormal3f(vertexNormals[gridX2][gridZ+1]);
							grcVertex3f(vertexLocs[gridX2][gridZ+1]);

							grcEnd();

							if(PFD_WaterVelocity.GetEnabled())
							{
								const Vector3& loc = vertexLocs[gridX][gridZ];
								const Vector2& vel = velocities[gridX][gridZ];
								Vector3 end(loc.x + vel.x * PFD_WaterVelocityLength.GetValue(), loc.y, loc.z + vel.y * PFD_WaterVelocityLength.GetValue());
								grcBegin(drawLineStrip, 2);
								grcColor(VEC3V_TO_VECTOR3(Color_red.GetRGB()));
								grcVertex3f(vertexLocs[gridX][gridZ]);
								grcColor(VEC3V_TO_VECTOR3(Color_beige.GetRGB()));
								grcVertex3f(end);
							}
						}
					}
				}
			}
		}

		else
		{
			// Just draw a flat plane.
			PolyVerts[0].Set(GetBoundingBoxMin().GetXf(), 0.0f, GetBoundingBoxMin().GetZf());
			PolyVerts[1].Set(GetBoundingBoxMin().GetXf(), 0.0f, GetBoundingBoxMax().GetZf());
			PolyVerts[2].Set(GetBoundingBoxMax().GetXf(), 0.0f, GetBoundingBoxMax().GetZf());
			PolyVerts[3].Set(GetBoundingBoxMax().GetXf(), 0.0f, GetBoundingBoxMin().GetZf());
			grcBegin(drawTris, 6);
			{
				grcColor3f(0.85f, 0.45f, 0.40f);
				grcNormal3f(YAXIS);
				grcVertex3f(PolyVerts[0]);
				grcVertex3f(PolyVerts[1]);
				grcVertex3f(PolyVerts[2]);
				grcVertex3f(PolyVerts[0]);
				grcVertex3f(PolyVerts[2]);
				grcVertex3f(PolyVerts[3]);
			}
			grcEnd();
		}
	}
}
#endif

void phBoundSurface::SetGridData(phSurfaceGrid *SurfaceGrid)
{
	m_SurfaceGrid = SurfaceGrid;
	if(SurfaceGrid)
	{
		SurfaceGrid->SetOffsetAndVelocityGrid(&m_OffsetGrid, &m_VelocityGrid, m_MaxElevation, m_MinElevation);
	}
}

void phBoundSurface::CalculateBoundingVolumes()
{
	m_PeakWaveHeight = m_MaxElevation;
//	float minX = -32.0f, maxX = +32.0f, minZ = -32.0f, maxZ = +32.0f;
//	if(m_SurfaceGrid)
//	{
//		m_SurfaceGrid->SetExtents(minX,maxX,minZ,maxZ);
//		m_PeakWaveHeight = m_SurfaceGrid->GetMaxOffset();
//		m_SurfaceGrid->GetLocal2DExtents(minX, maxX, minZ, maxZ);
//	}

	m_RadiusAroundCentroid = sqrt(square(32.0f) + square(32.0f) + square(m_PeakWaveHeight * 0.5f));
	const float maxDepth = 5.0f;
	SetBoundingBoxMin(Vec3V(m_MinX, -maxDepth , m_MinZ));
	SetBoundingBoxMax(Vec3V(m_MaxX, m_PeakWaveHeight, m_MaxZ));
}


bool phBoundSurface::BoxIntersectsBoundingBox(Vector3::Vector3Param vkrvecSurfacePos, const phBound *kpPolyBound, const Matrix34 *kpmtxPolyCurrent,
							  Vector3 &rvecBoxHalfWidths, Vector3 &rvecBoxCenter) const
{
	// Get the box half-widths.
	//	PF_START(Liquid_BoxTouchesLiquidBox);
	Vector3 krvecSurfacePos(vkrvecSurfacePos);
	rvecBoxHalfWidths = VEC3V_TO_VECTOR3(kpPolyBound->GetBoundingBoxSize());
	rvecBoxHalfWidths.Scale(0.5f);

	// Get the box center location and the distances to the corners.
	rvecBoxCenter = VEC3V_TO_VECTOR3(kpPolyBound->GetWorldCentroid(RCC_MAT34V(*kpmtxPolyCurrent)));
	float fCenterToCorner;
	fCenterToCorner = fabsf(kpmtxPolyCurrent->a.y * rvecBoxHalfWidths.x) + fabsf(kpmtxPolyCurrent->b.y * rvecBoxHalfWidths.y)
		+ fabsf(kpmtxPolyCurrent->c.y * rvecBoxHalfWidths.z);

	// See if the box of the colliding object intersects the axis-aligned liquid box.
	bool bBoxTouches = false;
	if(rvecBoxCenter.y - fCenterToCorner <= m_PeakWaveHeight + krvecSurfacePos.y && rvecBoxCenter.y + fCenterToCorner >= GetBoundingBoxMin().GetYf() + krvecSurfacePos.y)
	{
		// The object's box extends below the peak wave height and above the bottom of the bound.
		fCenterToCorner = fabsf(kpmtxPolyCurrent->a.x * rvecBoxHalfWidths.x)+fabsf(kpmtxPolyCurrent->b.x * rvecBoxHalfWidths.y)
			+ fabsf(kpmtxPolyCurrent->c.x * rvecBoxHalfWidths.z);
		if(rvecBoxCenter.x - fCenterToCorner < GetBoundingBoxMax().GetXf() + krvecSurfacePos.x && rvecBoxCenter.x + fCenterToCorner > GetBoundingBoxMin().GetXf() + krvecSurfacePos.x)
		{
			// The object's box penetrates the liquid box along the x axis.
			fCenterToCorner = fabsf(kpmtxPolyCurrent->a.z * rvecBoxHalfWidths.x) + fabsf(kpmtxPolyCurrent->b.z * rvecBoxHalfWidths.y)
				+ fabsf(kpmtxPolyCurrent->c.z * rvecBoxHalfWidths.z);
			if(rvecBoxCenter.z - fCenterToCorner < GetBoundingBoxMax().GetZf() + krvecSurfacePos.z && rvecBoxCenter.z + fCenterToCorner > GetBoundingBoxMin().GetZf() + krvecSurfacePos.z)
			{
				// The object's box penetrates the liquid box along the z axis.
				bBoxTouches = true;
			}
		}
	}

	//	PF_STOP(Liquid_BoxTouchesLiquidBox);
	return bBoxTouches;
}


int phBoundSurface::ClipAgainstBorders(const int kNumInVerts, const Vector3 *InVerts, const float *kInDepths, Vector3 *OutVerts, float *OutDepths) const
{
	Assert(InVerts != OutVerts);
	for(int nVertIndex = 0; nVertIndex < kNumInVerts; ++nVertIndex)
	{
		OutVerts[nVertIndex].Set(InVerts[nVertIndex]);
		OutDepths[nVertIndex] = kInDepths[nVertIndex];
	}

	return kNumInVerts;
}


bool phBoundSurface::FindLiquidImpactsToSphere(Vector3::Vector3Param vkSphereCenter_MS, const float kSphereRadius, phLiquidImpactData &ImpactData) const
{
#if __PFDRAW
	if(PFD_IgnoreLiquidImpactsSphere2.GetEnabled())
	{
		return false;
	}
#endif
	// These are the points at which we are going to probe the water to determine what the water surface is like.
	Vector3 DepthProbes[4];
	Vector3 kSphereCenter_MS(vkSphereCenter_MS);
	DepthProbes[0].Set(kSphereCenter_MS.x - kSphereRadius, 0.0f, kSphereCenter_MS.z);
	DepthProbes[1].Set(kSphereCenter_MS.x + kSphereRadius, 0.0f, kSphereCenter_MS.z);
	DepthProbes[2].Set(kSphereCenter_MS.x, 0.0f, kSphereCenter_MS.z - kSphereRadius);
	DepthProbes[3].Set(kSphereCenter_MS.x, 0.0f, kSphereCenter_MS.z + kSphereRadius);
	float BottomHeight = kSphereCenter_MS.y - kSphereRadius;
	int NumHits = 0, ProbeIndex;
	for(ProbeIndex = 0; ProbeIndex < 4; ProbeIndex++)
	{
		DepthProbes[ProbeIndex].x = Clamp(DepthProbes[ProbeIndex].x, GetBoundingBoxMin().GetXf(), GetBoundingBoxMax().GetXf());//-32.0f, +32.0f);
		DepthProbes[ProbeIndex].z = Clamp(DepthProbes[ProbeIndex].z, GetBoundingBoxMin().GetZf(), GetBoundingBoxMax().GetZf());//-32.0f, +32.0f);
		DepthProbes[ProbeIndex].y = GetHeight(DepthProbes[ProbeIndex].x, DepthProbes[ProbeIndex].z);
		if(DepthProbes[ProbeIndex].y > BottomHeight)
		{
			NumHits++;
		}
	}

	if(NumHits == 0)
	{
		return false;
	}

	// Construct an approximate normal for the plane of the water from the four water heights.
	ImpactData.m_Normal.Subtract(DepthProbes[1], DepthProbes[0]);
	Vector3 Temp(DepthProbes[3]);
	Temp.Subtract(DepthProbes[2]);
	ImpactData.m_Normal.Cross(Temp);
	ImpactData.m_Normal.Normalize();

	// Approximate a center point for that plane.
	Vector3 PlanePos(DepthProbes[0]);
	PlanePos.Add(DepthProbes[1]);
	PlanePos.Add(DepthProbes[2]);
	PlanePos.Add(DepthProbes[3]);
	PlanePos.Scale(0.25f);

	// Find the distance from the water plane to the center of the sphere.
	Vector3 CenterToPlane(PlanePos);
	CenterToPlane.Subtract(kSphereCenter_MS);
	float Depth = kSphereRadius - CenterToPlane.Dot(ImpactData.m_Normal);

	if(Depth <= 0.0f)
	{
		return false;
	}

	if(Depth < 2.0f * kSphereRadius)
	{
		// The sphere is partially submerged.  Find the impulse magnitude excluding density and gravity factors.
		ImpactData.m_SubmergedVolume = PI * kSphereRadius * square(Depth) * (1.0f - Depth / (3.0f * kSphereRadius));

		// Set the surface area submerged based on the depth.
		ImpactData.m_SubmergedArea = 2.0f * PI * kSphereRadius * Depth;
	}
	else
	{
		// The sphere is completely submerged.  This could really be handled by the code above if we clamped the depth,
		//   but the math simplifies a lot so we just make it a full-blown special case.
		ImpactData.m_SubmergedVolume = 1.333333333f * PI * power3(kSphereRadius);
		ImpactData.m_Normal.Negate(YAXIS);
		ImpactData.m_SubmergedArea = 4.0f * PI * square(kSphereRadius);
	}

	// Find an approximate point for the sphere's deepest penetration in the water.
	ImpactData.m_ForcePos.AddScaled(kSphereCenter_MS, ImpactData.m_Normal, kSphereRadius);
	ImpactData.m_UseBoundDirectionForDrag = true;
	return true;
}


float phBoundSurface::GetDepth(Vector3::Vector3Param vkLocationParam) const
{
	Vector3 vkLocation(vkLocationParam);
	// If the point is outside the bounds, return a zero depth so that points outside the edge of the bounds
	// do not have buoyancy forces applied.
	//if ((vkLocation.x < -32.0f) || (vkLocation.x > 32.0f) || (vkLocation.z < -32.0f) || (vkLocation.z > 32.0f))
	if ((vkLocation.x < GetBoundingBoxMin().GetXf()) || (vkLocation.x > GetBoundingBoxMax().GetXf()) || (vkLocation.z < GetBoundingBoxMin().GetZf()) || (vkLocation.z > GetBoundingBoxMax().GetZf()))
	{
		return 0.0f;		
	}

	float Depth = GetHeight(vkLocation.x, vkLocation.z);
	if(!FPIsFinite(Depth))
	{
		return 0.0f;
	}
	LIQUID_ASSERT_LEGIT(Depth);
	LIQUID_ASSERT_LEGIT(vkLocation.y);
	Depth -= vkLocation.y;
	LIQUID_ASSERT_LEGIT(Depth);
	return Depth;
}

float phBoundSurface::GetHeight(float LocationX, float LocationZ) const
{
	FastAssert(LocationX >= -32.0f && LocationX <= 32.0f);
	FastAssert(LocationZ >= -32.0f && LocationZ <= 32.0f);
	float Height = 0.0f;

	if(m_SurfaceGrid != NULL)
	{
#if !LERP_HEIGHTS
		// Simply pick the grid point that is closest to the given location and use that.
		int IndexX = static_cast<int>(0.5f * LocationX + 16.0f + 0.5f);
		int IndexZ = static_cast<int>(0.5f * LocationZ + 16.0f + 0.5f);

		if(IndexX != 32 && IndexZ != 32)
		{
			Height = m_SurfaceGrid->m_CurrentGrid[IndexZ * 32 + IndexX];
		}
#else
		int IndexX = static_cast<int>(0.5f * LocationX + 16.0f);
		FastAssert((int)(0.5f * LocationX + 16.0f) == IndexX);
		int IndexZ = static_cast<int>(0.5f * LocationZ + 16.0f);
		FastAssert((int)(0.5f * LocationZ + 16.0f) == IndexZ);
		FastAssert(IndexX <= 32);
		FastAssert(IndexZ <= 32);

		float Heights[4];
		Heights[0 * 2 + 0] = m_SurfaceGrid->FindElevationSafe(IndexX, IndexZ);
		LIQUID_ASSERT_LEGIT(Heights[0]);
		Heights[0 * 2 + 1] = m_SurfaceGrid->FindElevationSafe(IndexX + 1, IndexZ);
		LIQUID_ASSERT_LEGIT(Heights[1]);
		Heights[1 * 2 + 0] = m_SurfaceGrid->FindElevationSafe(IndexX, IndexZ + 1);
		LIQUID_ASSERT_LEGIT(Heights[2]);
		Heights[1 * 2 + 1] = m_SurfaceGrid->FindElevationSafe(IndexX + 1, IndexZ + 1);
		LIQUID_ASSERT_LEGIT(Heights[3]);

		const float kOOGridSpacing = 1.0f / 2.0f;
		float XLerp = kOOGridSpacing * LocationX - (static_cast<float>(IndexX) - 16.0f);
		FastAssert(XLerp >= -0.01f && XLerp <= 1.01f);

		float ZLerp = kOOGridSpacing * LocationZ - (static_cast<float>(IndexZ) - 16.0f);
		FastAssert(ZLerp >= -0.01f && ZLerp <= 1.01f);

		float Weights[4];
		Weights[0 * 2 + 0] = (1.0f - XLerp) * (1.0f - ZLerp);
		LIQUID_ASSERT_LEGIT(Weights[0]);
		Weights[0 * 2 + 1] = XLerp * (1.0f - ZLerp);
		LIQUID_ASSERT_LEGIT(Weights[1]);
		Weights[1 * 2 + 0] = (1.0f - XLerp) * ZLerp;
		LIQUID_ASSERT_LEGIT(Weights[2]);
		Weights[1 * 2 + 1] = XLerp * ZLerp;
		LIQUID_ASSERT_LEGIT(Weights[3]);

		FastAssert(Weights[0] + Weights[1] + Weights[2] + Weights[3] > 0.99f);
		FastAssert(Weights[0] + Weights[1] + Weights[2] + Weights[3] < 1.01f);
		Height = Weights[0] * Heights[0] + Weights[1] * Heights[1] + Weights[2] * Heights[2] + Weights[3] * Heights[3];
#endif
	}

	return Height;
}

/////////////////////////////////////////////////////////////////
// load / save
#if !__SPU

bool phBoundSurface::Load_v110 (fiAsciiTokenizer & token)
{
	//token.MatchToken("MaterialIndex:");
	//m_MaterialIndex = token.GetInt();

	token.MatchToken("BoundingBoxMin:");
	Vec3V boundingBoxMin;
	token.GetVector(RC_VECTOR3(boundingBoxMin));
	SetBoundingBoxMin(boundingBoxMin);

	token.MatchToken("BoundingBoxMax:");
	Vec3V boundingBoxMax;
	token.GetVector(RC_VECTOR3(boundingBoxMax));
	SetBoundingBoxMax(boundingBoxMax);

	token.MatchToken("Spacing:");
	m_Spacing = token.GetFloat();

	token.MatchToken("MinElevation:");
	m_MinElevation = token.GetFloat();

	token.MatchToken("MaxElevation:");
	m_MaxElevation = token.GetFloat();

	token.MatchToken("CellX:");
	m_CellX = (u16) token.GetShort();

	token.MatchToken("CellY:");
	m_CellY = (u16) token.GetShort();

	if (token.CheckToken("OffsetGrid:"))
	{
		for(int x=0;x<kPointsPerGridEdge;x++)
		{
			for(int z=0;z<kPointsPerGridEdge;z++)
			{
				m_OffsetGrid.SetValue(x,z, token.GetFloat());
			}
		}
	}

	if (token.CheckToken("VelocityGrid:"))
	{
		for(int x=0;x<kPointsPerGridEdge;x++)
		{
			for(int z=0;z<kPointsPerGridEdge;z++)
			{
				token.GetVector(m_VelocityGrid.GetValue(x,z));
			}
		}
	}

	m_PeakWaveHeight = m_MaxElevation;
	m_RadiusAroundCentroid = sqrt(square(32.0f) + square(32.0f) + square(m_PeakWaveHeight * 0.5f));

	return true;
}
#endif


#if !__FINAL && !IS_CONSOLE
bool phBoundSurface::Save_v110 (fiAsciiTokenizer & token)
{
	//token.PutDelimiter("MaterialIndex: ");
	//token.Put(m_MaterialIndex);
	//token.PutDelimiter("\n");

	token.PutDelimiter("BoundingBoxMin: ");
	token.Put(GetBoundingBoxMin());
	token.PutDelimiter("\n");

	token.PutDelimiter("BoundingBoxMax: ");
	token.Put(GetBoundingBoxMax());
	token.PutDelimiter("\n");

	token.PutDelimiter("Spacing: ");
	token.Put(m_Spacing);
	token.PutDelimiter("\n");

	token.PutDelimiter("MinElevation: ");
	token.Put(m_MinElevation);
	token.PutDelimiter("\n");

	token.PutDelimiter("MaxElevation: ");
	token.Put(m_MaxElevation);
	token.PutDelimiter("\n");

	token.PutDelimiter("CellX: ");
	token.Put(m_CellX);
	token.PutDelimiter("\n");

	token.PutDelimiter("CellY: ");
	token.Put(m_CellY);
	token.PutDelimiter("\n");

	token.PutDelimiter("OffsetGrid:\n");

	for(int x=0;x<kPointsPerGridEdge;x++)
	{
		for(int z=0;z<kPointsPerGridEdge;z++)
		{
			token.PutDelimiter(" ");
			token.Put( m_OffsetGrid.GetValue(x,z) );
			token.PutDelimiter("\n");
		}
	}
	token.PutDelimiter("\n");

	token.PutDelimiter("VelocityGrid:\n");

	for(int x=0;x<kPointsPerGridEdge;x++)
	{
		for(int z=0;z<kPointsPerGridEdge;z++)
		{
			token.PutDelimiter(" ");
			token.Put( m_VelocityGrid.GetValue(x,z) );
			token.PutDelimiter("\n");
		}
	}
	token.PutDelimiter("\n");

	return true;
}

#endif	// end of #if !__FINAL && !IS_CONSOLE


void phBoundSurface::SetRawData(
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
	float maxZ)
{
	m_Spacing = spacing;
	m_CellX = cellX;
	m_CellY = cellY;
	m_MinElevation = minElevation;
	m_MaxElevation = maxElevation;
	m_MinX = minX;
	m_MaxX = maxX;
	m_MinZ = minZ;
	m_MaxZ = maxZ;

	SetBoundingBoxMin(Vec3V(minX, 0.0f, minZ));
	SetBoundingBoxMax(Vec3V(maxX, m_MaxElevation - m_MinElevation, maxZ));

	// TODO: This radius does not take into account reduced bounding boxes.
	m_RadiusAroundCentroid = sqrt(square(32.0f) + square(32.0f) + square(m_PeakWaveHeight * 0.5f));

	for(int x=0;x<kPointsPerGridEdge;x++)
	{
		for(int z=0;z<kPointsPerGridEdge;z++)
		{
			// Shift all points by the min elevation so all offsets are relative to that.
			float offset = (offsetData ? offsetData->GetValue(x,z) : 0.0f);// - m_MinElevation;

			// Clean up roundoff errors from the subtraction. 20.000061 - 0.000061 < 20, apparently.
			if(offset > -0.1f)
			{
				offset = Max(offset, 0.0f);
			}
			else
			{
				// tidy up the 'no water' value of -1.
				offset = -1.0f;
			}

			m_OffsetGrid.SetValue(x,z,offset);

			m_VelocityGrid.SetValue(x,z, (velocityData ? velocityData->GetValue(x,z) : Vector2(0.0f,0.0f)));
		}
	}
}

#endif // USE_SURFACES

} // namespace rage
