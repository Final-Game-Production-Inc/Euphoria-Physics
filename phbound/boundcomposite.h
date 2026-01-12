//
// phbound/boundcomposite.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDCOMPOSITE_H
#define PHBOUND_BOUNDCOMPOSITE_H

#include "bound.h"

#if __RESOURCECOMPILER
#include "atl/atfunctor.h"
#endif // __RESOURCECOMPILER
#include "diag/debuglog.h"
#include "data/struct.h"
#include "phcore/resourceversions.h"
#include "system/memory.h"


#define PHBOUNDCOMPOSITE_BUILD_BVH	1// (NEW_MIDPHASE_COLLISION_2 && 0)

// Turning this on will cause asserts whenever the user sets a non-orthonormal current/last matrix
#define VALIDATE_COMPONENT_MATRICES (0 && __ASSERT)

#if __SPU
#define SPU_PARAM(X) , X
#else // __SPU
#define SPU_PARAM(X)
#endif // __SPU

namespace rage {

class phOptimizedBvh;

#if PHBOUNDCOMPOSITE_BUILD_BVH
class BvhPrimitiveData;
#endif

////////////////////////////////////////////////////////////////
// phBoundComposite

/*
PURPOSE
	A class to represent a physics bound that is an aggregate of multiple other physics bounds.  Composite bounds may not be nested. 
*/


class phBoundComposite : public phBound
{
public:
	phBoundComposite ();
	PH_NON_SPU_VIRTUAL ~phBoundComposite ();
	void Init (int numBounds, bool allowInternalMotion = false);
	void Shutdown ();

	void AllocateLastMatrices();

#if PHBOUNDCOMPOSITE_BUILD_BVH
	// This is where we decide whether or not to actually build a BVH structure based on what bounds are currently present and is also where we allocate and fill out
	//   that structure.
	PH_NON_SPU_VIRTUAL bool PostLoadCompute();

	// Helper function used by PostLoadCompute().  Can also be called directly.
	void AllocateAndBuildBvhStructure(int numLeafNodes);

	void AllocateAndCopyBvhStructure(const phOptimizedBvh &bvhStructureToCopy);
#endif

	// materials
	PH_NON_SPU_VIRTUAL const phMaterial & GetMaterial (phMaterialIndex i) const;
	PH_NON_SPU_VIRTUAL phMaterialMgr::Id GetMaterialId (phMaterialIndex i) const;
	PH_NON_SPU_VIRTUAL const phMaterial & GetMaterial (phMaterialIndex i, int component) const;
	PH_NON_SPU_VIRTUAL phMaterialMgr::Id GetMaterialId (phMaterialIndex i, int component) const;
	PH_NON_SPU_VIRTUAL int GetNumMaterials () const;
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex materialIndex=-1);

	// <COMBINE phBound::GetMaterialIdFromPartIndex>
	PH_NON_SPU_VIRTUAL phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent (int partIndex, int component=0) const;

	// bounds
	phBound * GetBound (int index) const						{ FastAssert(index>=0 && index<m_MaxNumBounds); return m_Bounds[index]; }
	int GetNumBounds () const									{ return m_NumBounds; }

	phBound**	GetBoundArray ()									{ return (phBound**)m_Bounds; }
	const phBound**	GetBoundArray () const							{ return (const phBound**)m_Bounds; }

#if __SPU
	void		SetBoundArray (phBound** boundArray)				{ m_Bounds = (datOwner<phBound>*)boundArray; }

	void		FixCurrentMatricesPtr(Mat34V* mtxArray)				{ m_CurrentMatrices = mtxArray; }
	void		FixLastMatricesPtr(Mat34V* mtxArray)				{ m_LastMatrices = mtxArray;    }

	void		FixLocalBoxMinMaxsArray(Vec3V* minMaxArray)			{ m_LocalBoxMinMaxs = minMaxArray; }
#endif
	const Vec3V* GetLocalBoxMinMaxsArray() const					{ return m_LocalBoxMinMaxs;		}
	Mat34V*		GetCurrentMatrices()								{ return m_CurrentMatrices;		}
	Mat34V*		GetLastMatrices()									{ return m_LastMatrices;		}

	void AdjustToNewInstMatrix(Mat34V_In currInstMat, Mat34V_In newInstMat, bool lastMatricesToo = true);

	// PURPOSE: Count the total number of bounds in this composite bound.
	// RETURN:  the total number of bounds in this composite bound hierarchy (not including this bound)
	// Use GetNumBounds instead, nesting composites is no longer supported.
	DEPRECATED int GetNumNestedBounds () const;

	// PURPOSE: Count the number of non-NULL bounds in this composite bound.
	// RETURN: The number of non-NULL bounds in this composite bound.
	// NOTES:
	//	- m_NumBounds is the number of possible bounds in this composite; it is possible for some of them to be NULL.
	int GetNumActiveBounds () const
	{
		int numActiveBounds = 0;
		for (int boundIndex=0; boundIndex<m_NumBounds; boundIndex++)
		{
			if (m_Bounds[boundIndex])
			{
				numActiveBounds++;
			}
		}

		Assert(numActiveBounds<=m_NumBounds && numActiveBounds<=m_MaxNumBounds);
		return numActiveBounds;
	}

	phBound* GetFirstActiveBound( int& boundIndex ) const
	{
		for( int i = 0; i < m_NumBounds; ++i )
		{
			if( m_Bounds[i] )
			{
				boundIndex = i;
				return m_Bounds[i];
			}
		}
		return NULL;
	}

	phBound* GetLastActiveBound( int& boundIndex )
	{
		for( int i = (m_NumBounds-1); i >= 0; --i )
		{
			if( m_Bounds[i] )
			{
				boundIndex = i;
				return m_Bounds[i];
			}
		}
		return NULL;
	}

	int GetMaxNumBounds () const								{ return m_MaxNumBounds; }

	// PURPOSE: Set one of the physics bound pointers for this composite bound.
	// PARAMS:
	//	index - the index number of the component bound to set in this composite bound
	//	bound - pointer to the phBound that will be used by this archetype
	//	deleteAtZero - optional to tell whether to delete the existing bound if the new one is NULL and this is the last reference to the existing one
	// NOTES:
	//	- The reference count of the new bound is incremented if the new bound is not NULL.
	//	- The reference count of the old bound is decremented if the old bound is not NULL.
	void SetBound (int index, phBound* bound, bool deleteAtZero=true);

	// PURPOSE: Set all of the sub-bounds on this bound to the sub-bounds on the given bound
	// PARAMS:
	//  compositeBound - reference to bound to with sub-bounds to copy
	//  deleteAtZero - optional to tell whether to delete the existing bound if the new one is NULL and this is the last reference to the existing one
	// NOTES:
	//  This will copy the minimum number of bounds between the this bound and compositeBound
	void SetBounds(const phBoundComposite& compositeBound, bool deleteAtZero=true);

	// PURPOSE: Set all of the sub-bounds on this bound to the sub-bounds on the given bound IF the sub-bound on this bound isn't NULL
	// PARAMS:
	//  compositeBound - reference to bound to with sub-bounds to copy
	//  deleteAtZero - optional to tell whether to delete the existing bound if the new one is NULL and this is the last reference to the existing one
	// NOTES:
	//  This will copy the minimum number of bounds between the this bound and compositeBound
	//  Any NULL sub-bound on this composite won't be updated
	void SetActiveBounds(const phBoundComposite& compositeBound, bool deleteAtZero=true);

	// PURPOSE: Remove the given sub-bound from this composite
	// PARAMS:
	//   index - index of sub-bound to remove
	void RemoveBound (int index)								{ SetBound(index,NULL); }

	// PURPOSE: Remove all of the sub-bounds on this composite 
	void RemoveBounds ();

	void SetNumBounds (int numBounds)							{ FastAssert(numBounds>=0 && numBounds<=m_MaxNumBounds); m_NumBounds = (u16)numBounds; }

	// PURPOSE: Find out whether one or more of the components may be BVHs.
	// RETURN:	true if this composite currently or at some previous time contained a BVH as a component
	bool GetContainsBVH () const
	{
		const int numBounds = GetNumBounds();
		for(int componentIndex = 0; componentIndex < numBounds; ++componentIndex)
		{
			const phBound *curBound = GetBound(componentIndex);
			if(curBound != NULL && curBound->GetType() == phBound::BVH)
			{
				return true;
			}
		}

		return false;
	}

#if __ASSERT
	bool CheckCachedMinMaxConsistency() const;
#endif	// __ASSERT

#if PHBOUNDCOMPOSITE_BUILD_BVH
	__forceinline const phOptimizedBvh *GetBVHStructure() const					{ return m_BVHStructure; }
#else
	__forceinline const phOptimizedBvh *GetBVHStructure() const					{ return NULL; }
#endif
	__forceinline bool HasBVHStructure() const									{ return GetBVHStructure() != NULL; }
	
	// PURPOSE: Get the position and orientation of the composite bound part with the given index, in the object's coordinates.
	// PARAMS:
	//	partIndex - the index number of the composite bound part's matrix to get
	// RETURN:	the matrix of the given bound part in the object's coordinates (not in world space)
	Mat34V_ConstRef GetCurrentMatrix (int partIndex) const;

	// PURPOSE: Get the pevious position and orientation of the composite bound part with the given index, in the object's coordinates.
	// PARAMS:
	//	partIndex - the index number of the composite bound part's previous matrix to get
	// RETURN:	the previous matrix of the given bound part in the object's coordinates (not in world space)
	Mat34V_ConstRef GetLastMatrix (int partIndex) const;

	// PURPOSE: Get a pointer to the list of composite part matrices.
	// RETURN: a pointer to the list of composite part matrices
	const Mat34V* GetCurrentMatrices () const;

	// PURPOSE: Get a pointer to the list of composite part matrices on the previous frame.
	// RETURN: a pointer to the list of composite part matrices on the previous frame
	const Mat34V* GetLastMatrices () const;

	// PURPOSE: Bounding box min's and max's for each bound component are cached in the composite to avoid having to go into the sub-bound to get them (which
	//  might incur a cache miss or require a DMA).  These are accessors to those values.  In general you should probably prefer to use these rather than obtain
	//  them directly from the bound (and they should always be identical).
    Vec3V_ConstRef GetLocalBoxMinMaxs (int index) const;
	Vec3V_ConstRef GetLocalBoxMins (int index) const;
	Vec3V_ConstRef GetLocalBoxMaxs (int index) const;

	// PURPSOE: Set the composite part matrix with the given index.
	// PARAMS:
	//	index - the number of the composite bound part to set
	//	matrix - the new matrix for the composite bound part
	void SetCurrentMatrix (int index, Mat34V_In matrix);

	// PURPSOE: Set the composite part matrix with the given index on the previous frame.
	// PARAMS:
	//	index - the number of the composite bound part to set
	//	matrix - the new previous-frame matrix for the composite bound part
	void SetLastMatrix (int index, Mat34V_In matrix);

	void UpdateLastMatricesFromCurrent ();
	void CopyMatricesFromComposite (const phBoundComposite& other);
	void SetSphereRadius (float radius);
	void SetBoundingBoxMinMax (Vec3V_In boxMin, Vec3V_In boxMax);

	// PURPOSE: Update the composite's BVH by rebuilding or just updating the bounds
	// PARAMS:
	//   fullRebuild - If set to true this will rebuild the BVH, otherwise it will update the boxes in the BVH
	// NOTES:
	// 1. Updating a BVH is much faster than rebuilding, however if the composite parts move significantly it will slow down
	//    queries into it.
	// 2. This function is NOT thread safe and should only be called when nobody can access the BVH
	void UpdateBvh(bool fullRebuild);

	// PURPOSE: Calculate the bounding box, bounding spheres and centroid offset.
	// PARAMS:
	//	onlyAdjustForInternalMotion - optional boolean to tell whether to assume there were no changes in the bound part sizes or shapes (default false)
	// NOTES:
	//	1.	If only internal motion is specified, the the bound part box extents will not be recomputed.
	//	2.	This is provided as a public method for composite bounds because they frequently need to be configured
	//		by game-level code, after which their extents need to be recalculated. Other bound types call their own
	//		CalculateExtents only when they are initialized. 
	//  3.  When on SPU whoever calls this function is responsible for DMAing the dynamic arrays and assigning the member pointers
	void CalculateCompositeExtents (bool onlyAdjustForInternalMotion=false, bool ignoreInternalMotion=false);

	// PURPOSE: Copy the composite extent information from the given composite bound
	// PARAMS:
	//   compositeBound - bound to copy extent information from
	void CopyCompositeExtents(const phBoundComposite& compositeBound);
#if __DEV && !__SPU
	bool VerifyAllComponentBoundingBoxesAreValid() const;
	bool DoesLocalBoundingBoxEqualComponentBoundingBox(int component) const;
	bool DoesBoundingBoxContainComponent(int component) const;
	bool DoesComponentBoundingBoxContainSupports(int component) const;
#endif // __DEV && !__SPU

	Vec3V_Out ComputeCompositeAngInertia (float density, const float* massList=NULL, const Vec3V* angInertiaList=NULL);
	void CalcCenterOfGravity (const float* massList=NULL);
	void CalcCenterOfBound ();

	// PURPOSE: Get the bound part with the given component index.
	// PARAMS:
	//	component - the index number of the bound part to get
	//	partMatrix - matrix to get for the returned bound part, relative to the matrix of the instance that uses this composite bound
	// RETURN:	the bound part with the given component index
	const phBound* GetCompositePart (int component, Mat34V_InOut partMatrix) const;

	// PURPOSE: Don't call this, it only returns the argument.
	// NOTES:
	//	1.	This is left over from when nested composite bounds were supported.
	DEPRECATED int GetNonNestedComponent (int component) const;

	// per-component collision flags
	void AllocateTypeAndIncludeFlags();
	void SetTypeAndIncludeFlags(u32* flagArray);
	u32* GetTypeAndIncludeFlags() const;
#if __SPU 
	void SetTypeAndIncludeFlags_(u32 * TypeAndIncludeFlags);
#endif // __SPU

#if __RESOURCECOMPILER
	// PURPOSE: 
	typedef atFunctor4<void, u32, u32&, u32&, bool&> CompositeBoundTypeAndIncludeFlagsHook;
	static void SetTypeAndIncludeFlagsHook(CompositeBoundTypeAndIncludeFlagsHook functor);
#endif // __RESOURCECOMPILER

	static void SetTypeFlags(u32* flagArray, int component, u32 flags);
	static u32 GetTypeFlags(const u32* flagArray, int component);
	static void SetIncludeFlags(u32* flagArray, int component, u32 flags);
	static u32 GetIncludeFlags(const u32* flagArray, int component);

	void SetTypeFlags(int component, u32 flags);
	u32 GetTypeFlags(int component) const;
	void SetIncludeFlags(int component, u32 flags);
	u32 GetIncludeFlags(int component) const;

#if !__SPU
	virtual bool IsPolygonal (int component) const;
	void Copy (const phBound* original);
	phBound* Clone () const;
	void CloneParts();
	void DeleteParts();

#if __DEBUGLOG
	void DebugReplay() const;
#endif

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const;

	// <COMBINE phBound::CanBecomeActive>
	virtual bool CanBecomeActive () const;

#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
	virtual void DrawSupport(Mat34V_In mtx, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;
	virtual void DrawNormals(Mat34V_In mtx, int normalType = FACE_NORMALS, int whichPolys = ALL_POLYS, float length = 1.0f, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;
	void DrawLast(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0) const;
#endif // __PFDRAW

#if __NMDRAW
  virtual void NMRender(Mat34V_In mtx) const;
#endif // __NMDRAW
	////////////////////////////////////////////////////////////
	// resources
	phBoundComposite (datResource & rsc);							// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

protected:
	///////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);						// load, ASCII, version 110

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);						// save, ASCII, version 110
#endif

#endif // !__SPU
	///////////////////////////////////////////////////////////
	// helper functions
	void CalculateExtents ();
	void CalculateBoundBox ();
	void CalculateBoundBoxNoInternalMotion ();

#if PHBOUNDCOMPOSITE_BUILD_BVH
	// Return value is the number of primitives found (could be less than the number of components if there are NULL components).
	int BuildPrimitiveDataFromPrimitives(BvhPrimitiveData *primitiveData, bool formatForUpdate=false) const;
#endif

	//============================================================================
	// Data

	// PURPOSE: A list of pointers to the sub-bounds that compose this composite bound.
	datOwner<phBound> * m_Bounds;

	// PURPOSE: The current matrices for each sub-bound.
	Mat34V * m_CurrentMatrices;

	// PURPOSE: The previous (last update) matrices for each sub-bound.
	Mat34V * m_LastMatrices;

    // PURPOSE: The bounding boxes, in part local space, of all of the sub-bounds
    Vec3V* m_LocalBoxMinMaxs;

	// PURPOSE: Optional per-component type and include flags
	u32* m_TypeAndIncludeFlags;

	// PURPOSE: Per-component type and include flags that were allocated by calling AllocateTypeAndIncludeFlags
	// needs to be kept, and moved to rage\dev
	u32* m_OwnedTypeAndIncludeFlags;

	// PURPOSE: Maximum number of sub-bounds that this composite bound is allocated to hold.
	u16 m_MaxNumBounds;

	// PURPOSE: Number of sub-bounds that this composite bound is currently composed of.
	u16 m_NumBounds;

#if PHBOUNDCOMPOSITE_BUILD_BVH
	datOwner<phOptimizedBvh> m_BVHStructure;
#else
	void *m_BVHStructure;
#endif

#if __RESOURCECOMPILER
protected:
	static CompositeBoundTypeAndIncludeFlagsHook sm_TypeAndIncludeFlagsHookFunctor;
#endif // __RESOURCECOMPILER

	PAR_SIMPLE_PARSABLE;
};

__forceinline Mat34V_ConstRef phBoundComposite::GetCurrentMatrix (int partIndex) const
{
	FastAssert(partIndex >= 0 && partIndex < m_MaxNumBounds);
	return m_CurrentMatrices[partIndex];
}

__forceinline Mat34V_ConstRef phBoundComposite::GetLastMatrix (int partIndex) const
{
	FastAssert(partIndex >= 0 && partIndex < m_MaxNumBounds);
	return m_LastMatrices[partIndex];
}

inline const Mat34V* phBoundComposite::GetCurrentMatrices () const
{
	return m_CurrentMatrices;
}

inline const Mat34V* phBoundComposite::GetLastMatrices () const
{
	return m_LastMatrices;
}

inline Vec3V_ConstRef phBoundComposite::GetLocalBoxMinMaxs (int index) const
{
	FastAssert(index >= 0 && index < m_MaxNumBounds * 2);
	return m_LocalBoxMinMaxs[index];
}

inline Vec3V_ConstRef phBoundComposite::GetLocalBoxMins (int index) const
{
	FastAssert(index >= 0 && index < m_MaxNumBounds);
	return m_LocalBoxMinMaxs[index << 1];
}

inline Vec3V_ConstRef phBoundComposite::GetLocalBoxMaxs (int index) const
{
	FastAssert(index >= 0 && index < m_MaxNumBounds);
	return m_LocalBoxMinMaxs[(index << 1) + 1];
}

inline void phBoundComposite::SetCurrentMatrix (int index, Mat34V_In matrix)
{
	FastAssert(index >= 0 && index < m_MaxNumBounds);
#if VALIDATE_COMPONENT_MATRICES
	Assertf(matrix.IsOrthonormal3x3(ScalarV(V_FLT_SMALL_2)),"Trying to set non-orthonormal current matrix on component %i."
															"\n\t%f, %f, %f, %f"
															"\n\t%f, %f, %f, %f"
															"\n\t%f, %f, %f, %f",
															index,
															matrix.GetCol0().GetXf(),matrix.GetCol1().GetXf(),matrix.GetCol2().GetXf(),matrix.GetCol3().GetXf(),
															matrix.GetCol0().GetYf(),matrix.GetCol1().GetYf(),matrix.GetCol2().GetYf(),matrix.GetCol3().GetYf(),
															matrix.GetCol0().GetZf(),matrix.GetCol1().GetZf(),matrix.GetCol2().GetZf(),matrix.GetCol3().GetZf());
#endif // VALIDATE_COMPONENT_MATRICES
	if(index >= 0 && index < m_MaxNumBounds)
	{
		m_CurrentMatrices[index] = matrix;
	}
}

inline void phBoundComposite::SetLastMatrix (int index, Mat34V_In matrix)
{
	FastAssert(index >= 0 && index < m_MaxNumBounds);
#if VALIDATE_COMPONENT_MATRICES
	Assertf(matrix.IsOrthonormal3x3(ScalarV(V_FLT_SMALL_2)),"Trying to set non-orthonormal last matrix on component %i."
		"\n\t%f, %f, %f, %f"
		"\n\t%f, %f, %f, %f"
		"\n\t%f, %f, %f, %f",
		index,
		matrix.GetCol0().GetXf(),matrix.GetCol1().GetXf(),matrix.GetCol2().GetXf(),matrix.GetCol3().GetXf(),
		matrix.GetCol0().GetYf(),matrix.GetCol1().GetYf(),matrix.GetCol2().GetYf(),matrix.GetCol3().GetYf(),
		matrix.GetCol0().GetZf(),matrix.GetCol1().GetZf(),matrix.GetCol2().GetZf(),matrix.GetCol3().GetZf());
#endif // VALIDATE_COMPONENT_MATRICES
	m_LastMatrices[index] = matrix;
}

inline void phBoundComposite::UpdateLastMatricesFromCurrent ()
{
	if (m_LastMatrices != m_CurrentMatrices)
	{
		sysMemCpy(m_LastMatrices, m_CurrentMatrices, m_NumBounds * sizeof(Mat34V));
	}
}


inline void phBoundComposite::SetSphereRadius (float radius)
{
	m_RadiusAroundCentroid = radius;
}


inline void phBoundComposite::SetBoundingBoxMinMax (Vec3V_In boxMin, Vec3V_In boxMax)
{
	SetBoundingBoxMin(boxMin);
	SetBoundingBoxMax(boxMax);
}


inline const phBound* phBoundComposite::GetCompositePart (int component, Mat34V_InOut partMatrix) const
{
	if (Verifyf(component>=0 && component<m_NumBounds, "Component %d out of range (0 to %d)", component, m_NumBounds))
	{
		partMatrix = GetCurrentMatrix(component);

		return m_Bounds[component];
	}

	return NULL;
}


inline int phBoundComposite::GetNonNestedComponent (int component) const
{
	return component;
}

inline int phBoundComposite::GetNumNestedBounds () const
{
	return m_NumBounds;
}

inline u32* phBoundComposite::GetTypeAndIncludeFlags() const
{
	return m_TypeAndIncludeFlags;
}

#if __SPU 
inline void phBoundComposite::SetTypeAndIncludeFlags_(u32 * TypeAndIncludeFlags)
{
	m_TypeAndIncludeFlags = TypeAndIncludeFlags;
}
#endif // __SPU

#if __RESOURCECOMPILER
inline void phBoundComposite::SetTypeAndIncludeFlagsHook(CompositeBoundTypeAndIncludeFlagsHook functor)
{
	sm_TypeAndIncludeFlagsHookFunctor=functor;
}
#endif // __RESOURCECOMPILER

/*static*/ inline void phBoundComposite::SetTypeFlags(u32* flagArray, int component, u32 flags)
{
	flagArray[component * 2] = flags;
}

/*static*/ inline u32 phBoundComposite::GetTypeFlags(const u32* flagArray, int component)
{
	return flagArray[component * 2];
}

/*static*/ inline void phBoundComposite::SetIncludeFlags(u32* flagArray, int component, u32 flags)
{
	flagArray[component * 2 + 1] = flags;
}

/*static*/ inline u32 phBoundComposite::GetIncludeFlags(const u32* flagArray, int component)
{
	return flagArray[component * 2 + 1];
}

inline void phBoundComposite::SetTypeFlags(int component, u32 flags)
{
	FastAssert(component >= 0 && component < m_MaxNumBounds);
	SetTypeFlags(m_TypeAndIncludeFlags, component, flags);
}

inline u32 phBoundComposite::GetTypeFlags(int component) const
{
	FastAssert(component >= 0 && component < m_MaxNumBounds);
	return GetTypeFlags(m_TypeAndIncludeFlags, component);
}

inline void phBoundComposite::SetIncludeFlags(int component, u32 flags)
{
	FastAssert(component >= 0 && component < m_MaxNumBounds);
	SetIncludeFlags(m_TypeAndIncludeFlags, component, flags);
}

inline u32 phBoundComposite::GetIncludeFlags(int component) const
{
	FastAssert(component >= 0 && component < m_MaxNumBounds);
	return GetIncludeFlags(m_TypeAndIncludeFlags, component);
}

#if !__SPU
// NOTE: Not normally used for collision, since we typically take composite bounds apart so they can be concave. This
// function exists for the convenience of some operations that want to make a support query, e.g. computing a bounding box.
// This method uses margins, any call to phBound::LocalGetSupport will be correct since the composite has a margin of 0
inline void phBoundComposite::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 vec, SupportPoint & sp) const
{
	Vec3V support(V_ZERO);
	ScalarV biggestDot(-ScalarV(V_FLT_LARGE_8));

	for (int i=0; i<m_NumBounds; i++)
	{
		const phBound* bound = GetBound(i);
		if(bound)
		{
			// Determine bound mat in current world space
			const Mat34V& boundMat = GetCurrentMatrix(i);
			Vec3V localVec = UnTransform3x3Ortho(boundMat, Vec3V(vec));
			Vec3V localSupport = bound->LocalGetSupportingVertex(localVec.GetIntrin128());
			Vec3V newSupport = Transform(boundMat, localSupport);
			ScalarV newDot = Dot(newSupport, Vec3V(vec));

			if (IsGreaterThanAll(newDot, biggestDot))
			{
				support = newSupport;
				biggestDot = newDot;
			}
		}
	}

	sp.m_vertex = support;
	sp.m_index = DEFAULT_SUPPORT_INDEX;
}
#endif

} // namespace rage

#endif
