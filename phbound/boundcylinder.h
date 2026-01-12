//
// phbound/boundcylinder.h
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_CYLINDER_H
#define PHBOUND_CYLINDER_H


////////////////////////////////////////////////////////////////
// external defines

#include "bound.h"

#include "phcore/constants.h"
#include "phcore/materialmgr.h"
#include "phcore/phmath.h"


namespace rage {

	//=============================================================================
	// phBoundCylinder
	// PURPOSE
	//   A physics bound in the shape of a cylinder.
	// <FLAG Component>
	//
	class phBoundCylinder : public phBound
	{
	public:
		//=========================================================================
		// Construction

		phBoundCylinder();
		PH_NON_SPU_VIRTUAL ~phBoundCylinder();

#if !__SPU
		phBoundCylinder (datResource &rsc);										// construct in resource

#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

		virtual void Copy (const phBound* original);
#endif	// !__SPU

		static const bool ms_RequiresDestructorOnStack = false;

		//=========================================================================
		// Accessors
		Vec3V_Out GetHalfExtents() const											{ return Subtract(GetBoundingBoxMax(),GetCentroidOffset()); }
		float GetRadius() const														{ return GetHalfExtents().GetXf(); }
		ScalarV_Out GetRadiusV() const												{ return GetHalfExtents().GetX(); }
		float GetHalfHeight() const													{ return GetHalfExtents().GetYf(); }
		ScalarV_Out GetHalfHeightV() const											{ return GetHalfExtents().GetY(); }
		float GetHeight() const														{ return GetBoundingBoxSize().GetYf(); }
		ScalarV_Out GetHeightV() const												{ return GetBoundingBoxSize().GetY(); }

		void SetCylinderRadiusAndHalfHeight(float radius, float halfHeight);
		void SetCylinderRadiusAndHalfHeight(ScalarV_In radius, ScalarV_In halfHeight);

		void SetCentroidOffset(Vec3V_In offset);									// set the centroid to be at offset
		void ShiftCentroidOffset(Vec3V_In offsetDelta);							// translate the centroid by offsetDelta

		phMaterialMgr::Id GetMaterialIdFromPartIndex (int UNUSED_PARAM(partIndex)) const { return GetPrimitiveMaterialId(); }
		void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) { SetPrimitiveMaterialId(materialId); }

#if !__SPU
		virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
		virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return  GetPrimitiveMaterialId(); }

		// <COMBINE phBound::GetMaterialIdFromPartIndex>
#else
		phMaterialMgr::Id GetMaterialId (int UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
#endif	// !__SPU

		//=========================================================================
		// Operations

#if !__SPU
		// <COMBINE phBound::CanBecomeActive>
		virtual bool CanBecomeActive() const;
#endif	// !__SPU

		// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
		void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const;

		// Test a segment against this cylinder all input and output is in the cylinder's local space
		// Params:
		//   point - first point of segment
		//   segment - vector from first to second point of segment
		//   segmentT1 - output parameter, if return value is 1 or 2 this will be the T value of the first intersection
		//   normal1 - output parameter, if return value is 1 or 2 this will be the normal of the first intersection
		//   segmentT2 - output parameter, if return value is 2 this will be the T value of the second intersection
		//   normal2 - output parameter, if return value is 2 this will be the normal of the second intersection
		// Return:
		//   number of intersections (0, 1, or 2)
		int TestAgainstSegment(Vec3V_In point, Vec3V_In segment, ScalarV_InOut segmentT1, Vec3V_InOut normal1, ScalarV_InOut segmentT2, Vec3V_InOut normal2)  const;

#if !__SPU
		//=========================================================================
		// Debugging
#if __PFDRAW
		virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = phBound::ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif // __PFDRAW

#if __NMDRAW
		virtual void NMRender(Mat34V_In mtx) const;
#endif // __NMDRAW
#endif	// !__SPU

#if !__SPU
	protected:
#endif	// !__SPU
		//=========================================================================
		// load / save
		bool Load_v110(fiAsciiTokenizer & token);
#if !__FINAL && !IS_CONSOLE
		bool Save_v110(fiAsciiTokenizer & token);
#endif	// !__FINAL && !IS_CONSOLE

		//=========================================================================
		// Protected operations.
		void CalculateExtents();												// calculate the bounding sphere and box

		// Pass the radius and half height as parameters to avoid potential LHSs.
		void CalculateCylinderExtents(ScalarV_In radius, ScalarV_In halfHeight);

		u8 m_Pad[16];
	};

	__forceinline phBoundCylinder::phBoundCylinder()
	{
		m_Type = CYLINDER;
		SetCylinderRadiusAndHalfHeight(ScalarV(V_ONE),ScalarV(V_ONE));
		SetMaterial(phMaterialMgr::DEFAULT_MATERIAL_ID);
		phBound::SetCentroidOffset(Vec3V(V_ZERO));
	}

	__forceinline phBoundCylinder::~phBoundCylinder()
	{
	}


#if !__SPU
	////////////////////////////////////////////////////////////////
	// resources

	inline phBoundCylinder::phBoundCylinder (datResource &rsc) : phBound(rsc)
	{
	}

#if __DECLARESTRUCT
	inline void phBoundCylinder::DeclareStruct(datTypeStruct &s)
	{
		phBound::DeclareStruct(s);
		STRUCT_BEGIN(phBoundCylinder);
		STRUCT_CONTAINED_ARRAY(m_Pad);
		STRUCT_END();
	}
#endif // __DECLARESTRUCT

	////////////////////////////////////////////////////////////////

	inline void phBoundCylinder::Copy(const phBound* original)
	{
		Assert(original->GetType() == phBound::CYLINDER);
		*this = *static_cast<const phBoundCylinder*>(original);
		SetRefCount(1);
	}
#endif	// !__SPU


	__forceinline void phBoundCylinder::SetCylinderRadiusAndHalfHeight(float radius, float halfHeight)
	{
		SetCylinderRadiusAndHalfHeight(ScalarVFromF32(radius),ScalarVFromF32(halfHeight));
	}

	__forceinline void phBoundCylinder::SetCylinderRadiusAndHalfHeight(ScalarV_In radius, ScalarV_In halfHeight)
	{
		// Clamp the collision margin at 1/8 the minimum cylinder dimension, so that small cylinders don't get too rounded.
		SetMargin(Min(GetMarginV(), Scale(Min(radius, halfHeight), ScalarV(V_QUARTER))));

		CalculateCylinderExtents(radius, halfHeight);
	}


	inline void phBoundCylinder::SetCentroidOffset(Vec3V_In offset)
	{
		phBound::SetCentroidOffset(offset);
		CalculateExtents();
	}


#if !__SPU
	inline bool phBoundCylinder::CanBecomeActive() const
	{
		return true;
	}
#endif	// !__SPU


	inline void phBoundCylinder::ShiftCentroidOffset(Vec3V_In offset)
	{
		phBound::SetCentroidOffset(GetCentroidOffset() + offset);
		CalculateExtents();
	}


	FORCE_INLINE_SIMPLE_SUPPORT void phBoundCylinder::LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const
	{
		const Vec3V supportDir = RCC_VEC3V(vec);
		const Vec3V squaredSupportDir = Scale(supportDir, supportDir);
		const ScalarV flatSquaredSupportDirMag = Add(squaredSupportDir.GetX(), squaredSupportDir.GetZ());
		const Vec3V pseudoFlatSupportDir = Scale(InvSqrtFast(flatSquaredSupportDirMag), supportDir);
		const Vec3V halfExtentsMinusMargin = Subtract(GetHalfExtents(),Vec3V(GetMarginV()));
		const ScalarV radius = halfExtentsMinusMargin.GetX();
		const ScalarV halfHeight = halfExtentsMinusMargin.GetY();

		// I switched this from a SelectFT() to an And() because that should be slightly faster.  Man I'm getting too into micro-optimizations.  The stuff below
		//   also has some more optimization opportunities (might require storing <radius, halfHeight, radius> in phBoundCylinder instead though).
//		const Vec3V xzResult = Scale(radius, SelectFT(IsLessThan(flatSquaredSupportDirMag, ScalarV(V_FLT_SMALL_5)), pseudoFlatSupportDir, Vec3V(V_ZERO)));
		const Vec3V xzResult = Scale(radius, And(Vec3V(IsGreaterThan(flatSquaredSupportDirMag, ScalarV(V_FLT_SMALL_12))), pseudoFlatSupportDir));
		const ScalarV yResult = SelectFT(IsLessThan(supportDir.GetY(), ScalarV(V_ZERO)), halfHeight, Negate(halfHeight));
		const Vec3V result = GetFromTwo<Vec::X1, Vec::Y2, Vec::Z1>(xzResult, Vec3V(yResult));
		sp.m_vertex = Add(GetCentroidOffset(), result);
		sp.m_index = DEFAULT_SUPPORT_INDEX;
	}


	inline void phBoundCylinder::CalculateExtents()
	{
		CalculateCylinderExtents(GetRadiusV(), GetHalfHeightV());
	}

	inline void phBoundCylinder::CalculateCylinderExtents(ScalarV_In radius, ScalarV_In halfHeight)
	{
		const ScalarV negatedRadius = Negate(radius);
		const ScalarV negatedHalfHeight = Negate(halfHeight);
		const Vec3V boundingBoxMin(negatedRadius, negatedHalfHeight, negatedRadius);
		const Vec3V boundingBoxMax(radius, halfHeight, radius);

		SetBoundingBoxMin(Add(boundingBoxMin, GetCentroidOffset()));
		SetBoundingBoxMax(Add(boundingBoxMax, GetCentroidOffset()));

		m_RadiusAroundCentroid = SqrtFast(Add(Scale(radius, radius), Scale(halfHeight, halfHeight))).Getf();

#if !__SPU
		// Compute the volume distribution from the cylinder shape.
		const float floatRadius = radius.Getf();
		const float floatHeight = 2.0f * halfHeight.Getf();
		phMathInertia::FindCylinderAngInertia(1.0f, floatRadius, floatHeight, &RC_VECTOR3(m_VolumeDistribution));
		m_VolumeDistribution.SetWf(PI * square(floatRadius) * (floatHeight));
#endif	// !__SPU
	}

} // namespace rage

#endif // PHBOUND_BOUNDCYLINDER_H
