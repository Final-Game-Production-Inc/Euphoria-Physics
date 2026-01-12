//
// phbound/boundbvh.h
//
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved.
//


#ifndef PHBOUND_BOUNDBVH_H
#define PHBOUND_BOUNDBVH_H

#include "boundgeom.h"


#define DEFAULT_PRIMS_PER_NODE 4


namespace rage {

	class phBoundCuller;
	class phOptimizedBvh;
	class BvhPrimitiveData;

	class phBoundBox;
	class phBoundCapsule;
	class phBoundCylinder;
	class phBoundSphere;

	/*
		PURPOSE
		A class to represent a geometric physics bound that stores its polygon data in an octree.
	*/

	class phBoundBVH : public phBoundGeometry
	{
		friend class phBoundGrid;

	public:
		phBoundBVH ();													// constructor
		PH_NON_SPU_VIRTUAL ~phBoundBVH ();								// destructor

#if !__SPU
		phBoundBVH (datResource & rsc);									// construct in resource
		DECLARE_PLACE(phBoundBVH);
#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

#endif	// end if #if !__SPU

		inline const phOptimizedBvh *GetBVH() const;

#if __SPU
		inline void SetBVH(phOptimizedBvh *newBVH);
#endif

#if __TOOL || __RESOURCECOMPILER
		phPrimitive& GetPrimitive(int primIndex)
		{
			TrapLT(primIndex, 0);
			TrapGE(primIndex, m_NumPolygons);
			return const_cast<phPrimitive&>(m_Polygons[primIndex].GetPrimitive());
		}
#endif // __TOOL || __RESOURCECOMPILER

		void ConstructBoundFromPrimitive(const phPrimBox& boxPrim, phBoundBox& boxBound, Mat34V_InOut boxMatrix) const;
		void ConstructBoundFromPrimitive(const phPrimSphere& spherePrim, phBoundSphere& sphereBound, Mat34V_InOut spherelMatrix) const;
		void ConstructBoundFromPrimitive(const phPrimCapsule& capsulePrim, phBoundCapsule& capsuleBound, Mat34V_InOut capsuleMatrix) const;
		void ConstructBoundFromPrimitive(const phPrimCylinder& cylinderPrim, phBoundCylinder& cylinderBound, Mat34V_InOut cylinderMatrix) const;

		const phPrimitive& GetPrimitive(int primIndex) const
		{
			TrapLT(primIndex, 0);
			TrapGE(primIndex, m_NumPolygons);
			return m_Polygons[primIndex].GetPrimitive();
		}

		////////////////////////////////////////////////////////////
		// debug
#if __PFDRAW || (__WIN32PC && !__FINAL)
		Vec3V_Out GetPrimitiveCenter(int primIndex) const;
		float RatePrimitive(int primIndex, bool bIncludePolygons, float maxPrimitivesPerMeterSquared) const;
		Color32 ComputePrimitiveDensityColor(phPolygon::Index primitiveIndex, u32 typeFlags, u32 includeFlags) const;
#endif // __PFDRAW || (__WIN32PC && !__FINAL)

#if __PFDRAW
		virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
		void DrawActivePolygons (Mat34V_In mtx) const;
#endif // __PFDRAW

#if __DEV
		void Validate ();													// validate that the internal state is consistent
#endif
		////////////////////////////////////////////////////////////
		// intersection test functions

#if !__SPU
		// <COMBINE phBound::CanBecomeActive>
		virtual bool CanBecomeActive () const;
#endif // !__SPU

#if !__SPU
		/////////////////////////////////////////////////////////////
		// load / save
		bool Load_v110 (fiAsciiTokenizer & token);							// load, ascii, v1.10
		bool Build(phPolygon::Index * const newToOldPolygonIndexMapping = NULL, int targetPrimsPerNode = DEFAULT_PRIMS_PER_NODE);
		void Unbuild();

		virtual void Copy (const phBound* original);
#endif	// end of #if !__SPU

        PH_NON_SPU_VIRTUAL void CullSpherePolys (phBoundCuller& culler, Vec3V_In sphereCenter, ScalarV_In sphereRadius) const;
		PH_NON_SPU_VIRTUAL void CullOBBPolys (phBoundCuller& culler, Mat34V_In boxMatrix, Vec3V_In boxHalfExtents) const;
		void CullLineSegPolys (Vector3::Vector3Param seg0, Vector3::Vector3Param seg1, phBoundCuller& culler) const;
		void CullCapsulePolys (Vector3::Vector3Param seg0, Vector3::Vector3Param seg1, const float capsuleRadius, phBoundCuller& culler) const;

		Vec3V_Out GetBVHVertex(int vertexIndex) const;

	protected:
#if !__SPU
		// Reorder the primitives based on a results of a BVH structure being built.  I can't see any reason why you wouldn't do this if you just built a BVH
		//   and I can't think of any reason that a client would want to do this manually so it's in the protected section now.
		void ReorderPrimitives(BvhPrimitiveData *newToOldPolygonMapping);
#endif

		/////////////////////////////////////////////////////////////
		// the BVH
		datOwner<phOptimizedBvh> m_BVH;

		// PURPOSE: Indices of the currently active polygons.
		// NOTES:	This is only used for debug line drawing.
		mutable phPolygon::Index* m_ActivePolygonIndices;

		// PURPOSE: Number of currently active polygons.
		// NOTES:	This is only used for debug line drawing.
		mutable u16 m_NumActivePolygons;

		u8 m_Pad[6+8];

#if !__FINAL && !IS_CONSOLE
		bool Save_v110 (fiAsciiTokenizer & token);							// save, ascii, v1.20
#endif

		// drawing helpers
#if ENABLE_DRAW_PHYS
		static const int smShouldDrawCulledPolygons;
		void DrawCulledPolys () const;
#endif
	};
#if RSG_CPU_X64 && !RSG_TOOL
CompileTimeAssert(sizeof(phBoundBVH)==336);	// double check padding and size
#endif

	__forceinline const phOptimizedBvh *phBoundBVH::GetBVH() const
	{
		return m_BVH;
	}

#if __SPU
	__forceinline void phBoundBVH::SetBVH(phOptimizedBvh *newBVH)
	{
		m_BVH = newBVH;
	}
#endif

	__forceinline Vec3V_Out phBoundBVH::GetBVHVertex(int vertexIndex) const
	{
#if COMPRESSED_VERTEX_METHOD == 0
		return GetVertex(vertexIndex);
#else
		return GetCompressedVertex(vertexIndex);
#endif
	}

} // namespace rage


#endif
