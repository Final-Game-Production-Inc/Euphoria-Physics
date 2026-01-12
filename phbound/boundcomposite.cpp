//
// phbound/boundcomposite.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundcomposite.h"
#if PHBOUNDCOMPOSITE_BUILD_BVH
#include "OptimizedBvh.h"
#endif
#include "support.h"				// For GetMaterialId

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "file/token.h"
#include "phbullet/SimdTransformUtil.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "physics/archetype.h"		// For TYPE_FLAGS_ALL
#include "physics/collisionoverlaptest.h"
#include "physics/levelnew.h"		// For LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
#include "grprofile/drawmanager.h"
#include "system/cache.h"			// For PrefetchDC

#include "boundcomposite_parser.h"

#define PHBOUNDCOMPOSITE_MIN_NUM_BOUNDS_TO_BUILD_BVH	(5)

#define THREADSAFE_BVH_UPDATE (LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE)

namespace rage {

CompileTimeAssert(sizeof(phBoundComposite) <= phBound::MAX_BOUND_SIZE);

EXT_PFD_DECLARE_ITEM(ComponentIndices);
EXT_PFD_DECLARE_ITEM(DrawSingleComponent);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(SelectComponentToDraw);
EXT_PFD_DECLARE_ITEM(CullBoxes);
EXT_PFD_DECLARE_ITEM(AnimateFromLast);
EXT_PFD_DECLARE_ITEM(BVHHierarchy);
EXT_PFD_DECLARE_ITEM_SLIDER(NodeDepth);
EXT_PFD_DECLARE_ITEM(BVHHierarchyNodeIndices);

////////////////////////////////////////////////////////////////
#if !__SPU

#if __RESOURCECOMPILER
phBoundComposite::CompositeBoundTypeAndIncludeFlagsHook phBoundComposite::sm_TypeAndIncludeFlagsHookFunctor;
#endif // __RESOURCECOMPILER

phBoundComposite::phBoundComposite ()
{
	m_Type = COMPOSITE;

	m_Bounds = NULL;
	m_CurrentMatrices = NULL;
	m_LastMatrices = NULL;
    m_LocalBoxMinMaxs = NULL;
	m_TypeAndIncludeFlags = NULL;
	m_OwnedTypeAndIncludeFlags = NULL;
	phBound::SetCentroidOffset(Vec3V(V_ZERO));
	m_BVHStructure = NULL;
	SetMargin(0);
}


void phBoundComposite::Init (int numBounds, bool allowInternalMotion)
{
	Assert(numBounds>0 && numBounds<0xFFFF);
	m_NumBounds = static_cast<u16>(numBounds);
	m_MaxNumBounds = static_cast<u16>(numBounds);

	m_Bounds = rage_new datOwner<phBound>[numBounds];
	
	m_CurrentMatrices = rage_new Mat34V[m_MaxNumBounds];

    m_LocalBoxMinMaxs = rage_new Vec3V[m_NumBounds * 2];

	for (int i=0; i<m_MaxNumBounds; i++)
	{
		m_CurrentMatrices[i] = Mat34V(V_IDENTITY);
        m_Bounds[i] = NULL;
	}
	
	m_LastMatrices = m_CurrentMatrices;

	if (allowInternalMotion)
	{
		AllocateLastMatrices();
	}
}

void phBoundComposite::Shutdown ()
{
	if (m_Bounds)
	{
		RemoveBounds();
		delete[] m_Bounds;
		m_Bounds = 0;
	}
	m_MaxNumBounds = 0;

	if (m_LastMatrices != m_CurrentMatrices)
	{
		delete[] m_LastMatrices;
	}
	m_LastMatrices = 0;
	delete[] m_CurrentMatrices;
	m_CurrentMatrices = 0;

	delete[] m_LocalBoxMinMaxs;
	m_LocalBoxMinMaxs = 0;
	// want to move across to rage/dev branch
	delete[] m_OwnedTypeAndIncludeFlags;
	m_OwnedTypeAndIncludeFlags = 0;
}

void phBoundComposite::AllocateLastMatrices()
{
	Assert(m_LastMatrices == m_CurrentMatrices);

	m_LastMatrices = rage_new Mat34V[m_MaxNumBounds];

	for (int i=0; i<m_MaxNumBounds; i++)
	{
		m_LastMatrices[i] = Mat34V(V_IDENTITY);
	}
}


phBoundComposite::~phBoundComposite()
{
	Shutdown();

#if PHBOUNDCOMPOSITE_BUILD_BVH
	delete m_BVHStructure;
#endif
}

void phBoundComposite::CopyMatricesFromComposite (const phBoundComposite& other)
{
	const int numOtherBounds = other.GetNumBounds();
	Assert(m_NumBounds <= numOtherBounds);
	const int numBoundsToCopy = m_NumBounds < numOtherBounds ? m_NumBounds: numOtherBounds;
	memcpy(m_CurrentMatrices, other.GetCurrentMatrices(), numBoundsToCopy * sizeof(Matrix34));

	if (m_LastMatrices != m_CurrentMatrices)
	{
		memcpy(m_LastMatrices, other.GetLastMatrices(), numBoundsToCopy * sizeof(Matrix34));
	}
}

void phBoundComposite::AdjustToNewInstMatrix(Mat34V_In currInstMat, Mat34V_In newInstMat, bool lastMatricesToo)
{
	Matrix34 worldBound, newBoundMat, transposeMat;

	for (int i=0; i<m_NumBounds; i++)
	{
		// Determine bound mat in current world space
		const Matrix34 &boundMat = RCC_MATRIX34(GetCurrentMatrix(i));
		worldBound.Dot(boundMat, RCC_MATRIX34(currInstMat));

		// Determine the bound in newInstMat-space
		newBoundMat = worldBound;
		transposeMat.Transpose3x4(RCC_MATRIX34(newInstMat));
		newBoundMat.Dot(transposeMat);

		// Set the new bound matrix
		SetCurrentMatrix(i, RCC_MAT34V(newBoundMat));

		if (lastMatricesToo)
		{
			// Determine bound mat in current world space
			const Matrix34 &boundMat = RCC_MATRIX34(GetLastMatrix(i));
			worldBound.Dot(boundMat, RCC_MATRIX34(currInstMat));

			// Determine the bound in newInstMat-space
			newBoundMat = worldBound;
			transposeMat.Transpose3x4(RCC_MATRIX34(newInstMat));
			newBoundMat.Dot(transposeMat);

			// Set the new bound matrix
			SetLastMatrix(i, RCC_MAT34V(newBoundMat));
		}

		// debug - new world mat shouldn't change
		//Matrix34 newWorldBound;
		//newWorldBound.Dot(newBoundMat, newInstMat);
	}
}

/////////////////////////////////////////////////////////////////
// load / save

bool phBoundComposite::Load_v110 (fiAsciiTokenizer & token)
{
	token.MatchToken("NumBounds:");

    int numBounds = token.GetInt();

    bool allowInternalMotion = false;
    if (token.CheckToken("allowInternalMotion"))
    {
        allowInternalMotion = true;
    }

	Init(numBounds, allowInternalMotion);

	// centroid offset
	Vector3 temp;
	if (token.CheckToken("centroid:"))
	{
		token.GetVector(temp);
		SetCentroidOffset(RCC_VEC3V(temp));
	}
	
	// center of gravity offset
	bool cgOffsetInBoundFile = false;
	if (token.CheckToken("cg:"))
	{
		token.GetVector (temp);
		SetCGOffset(RCC_VEC3V(temp));
		cgOffsetInBoundFile = true;
	}

#if HACK_GTA4 // Changes to allow using type and include flags on (map) composites to signal special bounds like rivers, high/low detail, etc.
	// bound flags
	// We use these flags to signify special bounds like rivers, high/low detail map bounds, etc. If we don't find a child bound
	// in the composite with a non-zero bound flag then we don't allocate individual part type and include flags.
	u32* tempTypeAndIncludeFlags = Alloca(u32, 2*m_NumBounds);
	bool needToAllocateTypeAndIncludeFlags = false;
	for(u16 i=0; i<m_NumBounds; ++i)
	{
		if(token.CheckToken("boundflags:"))
		{
			u32 childBoundFlag = token.GetInt();
			if(childBoundFlag != 0)
			{
				needToAllocateTypeAndIncludeFlags = true;
			}

			tempTypeAndIncludeFlags[i*2] = childBoundFlag;		// Define type flag for this bound.
			tempTypeAndIncludeFlags[i*2 + 1] = TYPE_FLAGS_ALL;	// Include flags defining other bound type that this bound will collide with.
		}
	}
	if(needToAllocateTypeAndIncludeFlags)
	{
#if __RESOURCECOMPILER
		AllocateTypeAndIncludeFlags();
		for(u16 i=0; i<m_NumBounds; ++i)
		{
			u32 nBoundTypeFlag = tempTypeAndIncludeFlags[i*2];
			u32 nTypeFlags = 0;
			u32 nIncludeFlags = 0;
			bool bContainsMover = false;
			if(sm_TypeAndIncludeFlagsHookFunctor.IsValid()) 
			{
				sm_TypeAndIncludeFlagsHookFunctor(nBoundTypeFlag, nTypeFlags, nIncludeFlags, bContainsMover);
			}
			SetTypeFlags(i, nTypeFlags);
			SetIncludeFlags(i, nIncludeFlags);
		}
#endif // __RESOURCECOMPILER
	}
#endif // HACK_GTA4

	// materials
	phMaterialMgr::Id material = phMaterialMgr::DEFAULT_MATERIAL_ID;
	bool materialSet = false;
	if (token.CheckToken("materials:"))
	{
		int numMaterialsToLoad;
		numMaterialsToLoad = token.GetInt();

		if (numMaterialsToLoad==1)
		{
			material = phBound::GetMaterialIdFromFile(token);
			materialSet = true;
		}
	}

	for(int i=0; i<m_NumBounds; i++)
	{
        if (token.CheckToken("bound:", false))
        {
		    token.MatchToken("bound:");
		    int partIndex=token.GetInt();
            Assert(partIndex<m_NumBounds);
		    phBound* partBound;
		    if (token.CheckToken("name:"))
		    {
			    char partName[RAGE_MAX_PATH];
			    token.GetToken(partName,RAGE_MAX_PATH);
			    partBound = phBound::Load(partName);
		    }
		    else
		    {
			    partBound = phBound::Load(token);
		    }
		    SetBound(partIndex,partBound);
			Assert(partBound);
		    partBound->Release(); // for consistent refcount.
		    if(token.CheckToken("matrix:"))
		    {
			    Mat34V partLocal;
			    token.GetVector(RC_VECTOR3(partLocal.GetCol0Ref()));
			    token.GetVector(RC_VECTOR3(partLocal.GetCol1Ref()));
			    token.GetVector(RC_VECTOR3(partLocal.GetCol2Ref()));
			    token.GetVector(RC_VECTOR3(partLocal.GetCol3Ref()));
			    SetCurrentMatrix(partIndex,partLocal);
		    }
		    else if(token.CheckToken("position:"))
		    {
			    Mat34V partLocal;
				partLocal.Set3x3(Mat33V(V_IDENTITY));
			    token.GetVector(RC_VECTOR3(partLocal.GetCol3Ref()));
			    SetCurrentMatrix(partIndex,partLocal);
		    }
        }
	}

    if (allowInternalMotion)
    {
        UpdateLastMatricesFromCurrent();
    }

	if(materialSet)
	{
		// A material was given in the composite file, so apply it to all the parts.
		for(int i=0; i<m_NumBounds; i++)
		{
			if(GetBound(i))
			{
				GetBound(i)->SetMaterial(material);
			}
		}
	}

	if (!cgOffsetInBoundFile)
	{
		// The center of gravity was not specified in the bound file, so calculate it.
		CalcCenterOfGravity();
	}

	CalculateExtents();
	UpdateBvh(true);
	PostLoadCompute();

	// Compute and set the volume distribution.
	const float density = 1.0f;
	ComputeCompositeAngInertia(density);

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundComposite::Save_v110 (fiAsciiTokenizer & token)
{
	int actualNum = 0;

	int index;
	for(index=0;index<m_NumBounds;index++)
	{
		if (m_Bounds[index])
		{
			actualNum = Max(actualNum, index + 1);
		}
	}

	token.PutDelimiter("\n");
	token.PutDelimiter("NumBounds: ");
	token.Put(actualNum);
	token.PutDelimiter("\n");

	if (m_LastMatrices != m_CurrentMatrices)
	{
		token.PutDelimiter("allowInternalMotion");
		token.PutDelimiter("\n");
	}

	if(!IsEqualAll(GetCentroidOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("centroid: ");
		token.Put(GetCentroidOffset());
		token.PutDelimiter("\n");
	}

	if(!IsEqualAll(GetCGOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("cg: ");
		token.Put(GetCGOffset());
		token.PutDelimiter("\n");
	}

	token.PutDelimiter("\n");
	
#if HACK_GTA4 // Changes to allow using type and include flags on (map) composites to signal special bounds like rivers, high/low detail, etc.
	for(index=0; index<m_NumBounds;index++)
	{
		token.Put("boundflags: ");
		token.Put((int)GetTypeFlags(index));
		token.PutDelimiter("\n");
	}
#endif // HACK_GTA4

	for(index=0;index<m_NumBounds;index++)
	{
		if (m_Bounds[index])
		{
			token.PutDelimiter("\n");
			token.PutDelimiter("\n");
			token.PutDelimiter("bound: ");
			token.Put(index);
			token.PutDelimiter("\n");
			phBound::Save(token,m_Bounds[index],VERSION_110);
			token.PutDelimiter("matrix: ");
			const Matrix34& mtx = RCC_MATRIX34(m_CurrentMatrices[index]);
			token.Put(mtx.a);
			token.PutDelimiter("\n");
			token.Put(mtx.b);
			token.PutDelimiter("\n");
			token.Put(mtx.c);
			token.PutDelimiter("\n");
			token.Put(mtx.d);
			token.PutDelimiter("\n");
		}
	}
	return true;
}
#endif	// end of #if !__FINAL && !IS_CONSOLE


#if PHBOUNDCOMPOSITE_BUILD_BVH
	bool phBoundComposite::PostLoadCompute()
	{
		FastAssert(m_BVHStructure == NULL);

		AllocateAndBuildBvhStructure(GetMaxNumBounds());

		return true;
	}

void phBoundComposite::AllocateAndBuildBvhStructure(int numLeafNodes)
{
	Assert(m_BVHStructure == NULL);
	if(numLeafNodes > PHBOUNDCOMPOSITE_MIN_NUM_BOUNDS_TO_BUILD_BVH)
	{
		m_BVHStructure = rage_new phOptimizedBvh;

		// Need to set the extents before we can quantize anything.
		const Vec3V boundingBoxMin = GetBoundingBoxMin();
		const Vec3V boundingBoxMax = GetBoundingBoxMax();
		m_BVHStructure->SetExtents(boundingBoxMin.GetIntrin128(), boundingBoxMax.GetIntrin128());

		// Make one BvhPrimitiveData for each of our bounds.
		BvhPrimitiveData *bvhPrimitives = Alloca(BvhPrimitiveData, numLeafNodes);
		const int actualNumPrimitives = BuildPrimitiveDataFromPrimitives(bvhPrimitives);

		m_BVHStructure->BuildFromPrimitiveData(bvhPrimitives, actualNumPrimitives, numLeafNodes, NULL, 1, false);
	}
}

void phBoundComposite::AllocateAndCopyBvhStructure(const phOptimizedBvh &bvhStructureToCopy)
{
	Assert(m_BVHStructure == NULL);
	m_BVHStructure = rage_new phOptimizedBvh;
	m_BVHStructure->AllocateAndCopyFrom(bvhStructureToCopy);
}
#endif


void phBoundComposite::SetBound (int i, phBound* bound, bool deleteAtZero)
{
	AssertMsg(!datResource_IsDefragmentation,"Calling phBoundComposite::SetBound during a defrag isn't going to do what you want.");

	Assert(i >= 0 && i <= m_MaxNumBounds);
	if (bound!=NULL)
	{
		// Add a reference count to the new bound.
		bound->AddRef();
	}

	if (m_Bounds[i]!=NULL)
	{
		// Pointer might not be known if it's only been resource constructed
		SAFE_REMOVE_REF_IF_KNOWN(m_Bounds[i]);
		// Remove a reference count from the old bound.
		m_Bounds[i]->Release(deleteAtZero);
	}

	// Replace the old bound with the new bound.
	m_Bounds[i] = bound;
	SAFE_ADD_KNOWN_REF(m_Bounds[i]);
}

void phBoundComposite::SetBounds(const phBoundComposite& compositeBound, bool deleteAtZero)
{
	int numBoundsToSet = Min(GetNumBounds(),compositeBound.GetNumBounds());
	for(int componentIndex = 0; componentIndex < numBoundsToSet; ++componentIndex)
	{
		SetBound(componentIndex, compositeBound.GetBound(componentIndex), deleteAtZero);
	}
}

void phBoundComposite::SetActiveBounds(const phBoundComposite& compositeBound, bool deleteAtZero)
{
	int numBoundsToSet = Min(GetNumBounds(),compositeBound.GetNumBounds());
	for(int componentIndex = 0; componentIndex < numBoundsToSet; ++componentIndex)
	{
		if(GetBound(componentIndex))
		{
			SetBound(componentIndex, compositeBound.GetBound(componentIndex), deleteAtZero);
		}
	}
}

void phBoundComposite::RemoveBounds()
{
	int numBounds = GetNumBounds();
	for(int componentIndex = 0; componentIndex < numBounds; ++componentIndex)
	{
		RemoveBound(componentIndex);
	}
}

void phBoundComposite::CalculateExtents ()
{
	CalculateCompositeExtents();
}

#endif

void phBoundComposite::CalculateCompositeExtents (bool onlyAdjustForInternalMotion, bool ignoreInternalMotion)
{
	int numActiveBounds = 0;
	int activeBoundIndex = 0;
	if (onlyAdjustForInternalMotion==false)
	{
		phBound* bound;
#if __SPU
		u8 boundBuffer[sizeof(phBound)] ;
		bound = reinterpret_cast<phBound*>(boundBuffer);
#endif // __SPU

		// Get the box extents for all the active bound parts.
		// If the argument given is true, then the bound parts can move but are assumed to have the same local extents.
		float newVolume = 0.0f;
		for (int boundIndex = 0; boundIndex < m_NumBounds; ++boundIndex)
		{
			if (m_Bounds[boundIndex])
			{
#if __SPU
				cellDmaLargeGet(boundBuffer, (uint64_t)GetBound(boundIndex), sizeof(phBound), DMA_TAG(0), 0, 0);
				cellDmaWaitTagStatusAll(DMA_MASK(0));
#else // __SPU
				bound = GetBound(boundIndex);
#endif // __SPU

				int twiceBoundIndex = boundIndex << 1;
				m_LocalBoxMinMaxs[twiceBoundIndex] = bound->GetBoundingBoxMin();
				m_LocalBoxMinMaxs[twiceBoundIndex + 1] = bound->GetBoundingBoxMax();
				newVolume += bound->GetVolume();
				numActiveBounds++;
				activeBoundIndex = boundIndex;
			}
		}
		m_VolumeDistribution.SetWf(newVolume);
	}

	// Compute the box extents for this composite from all its parts.
	if(ignoreInternalMotion)
	{
		CalculateBoundBoxNoInternalMotion();
	}
	else
	{
		CalculateBoundBox();
	}

	// See if there is only one bound part.
#if HACK_GTA4
	if(false)
#else
	if (numActiveBounds==1 && IsEqualAll(m_LastMatrices[activeBoundIndex],m_CurrentMatrices[activeBoundIndex]))
#endif
	{
		// This composite has only one bound part, so use its radius instead of making it enclose the box.
		m_RadiusAroundCentroid = m_Bounds[activeBoundIndex]->GetRadiusAroundCentroid();

		// Recompute the centroid offset to match the bound part's centroid.
		Vec3V centroidOffset = Transform(m_CurrentMatrices[activeBoundIndex],m_Bounds[activeBoundIndex]->GetCentroidOffset());
		SetCentroidOffset(centroidOffset);
	}
	else
	{
		// Make the enclosing radius half the box extents diagonal.
		m_RadiusAroundCentroid = Mag(GetBoundingBoxMax() - GetCentroidOffset()).Getf();
	}
}

void phBoundComposite::CopyCompositeExtents(const phBoundComposite& compositeBound)
{
	Assert(GetNumBounds() == compositeBound.GetNumBounds());
	SetBoundingBoxMax(compositeBound.GetBoundingBoxMax());
	SetBoundingBoxMin(compositeBound.GetBoundingBoxMin());
	SetCentroidOffset(compositeBound.GetCentroidOffset());
	SetRadiusAroundCentroid(compositeBound.GetRadiusAroundCentroidV());
	m_VolumeDistribution = compositeBound.m_VolumeDistribution;
	sysMemCpy(m_LocalBoxMinMaxs, compositeBound.m_LocalBoxMinMaxs, GetNumBounds()*2*sizeof(Vec3V));
}

#if __DEV && !__SPU
bool phBoundComposite::VerifyAllComponentBoundingBoxesAreValid() const
{
	for(unsigned component = 0; component < m_NumBounds; ++component)
	{
		phBound* subBound = GetBound(component);

		if(subBound)
		{
			bool localBoundingBoxEqualsComponentBoundingBox = DoesLocalBoundingBoxEqualComponentBoundingBox(component);
			Assertf(localBoundingBoxEqualsComponentBoundingBox, "Bound %i of type %s does not match the local bounding box", component, subBound->GetTypeString());

			bool boundingBoxContainsComponent = DoesBoundingBoxContainComponent(component);
			Assertf(boundingBoxContainsComponent, "Bound %i of type %s is not contained by the composite's bounding box", component, subBound->GetTypeString());

			bool componentBoundingBoxContainsSupports = DoesComponentBoundingBoxContainSupports(component);
			Assertf(componentBoundingBoxContainsSupports, "Bound %i of type %s does not contain its supports with its bounding box.", component, subBound->GetTypeString());

			if(!localBoundingBoxEqualsComponentBoundingBox || !boundingBoxContainsComponent || !componentBoundingBoxContainsSupports)
			{
				return false;
			}
		}
	}

	return true;
}

bool phBoundComposite::DoesLocalBoundingBoxEqualComponentBoundingBox(int component) const
{
	if(component >= 0 && component < GetNumBounds())
	{
		phBound* subBound = GetBound(component);
		if(subBound)
		{
			int twiceComponentIndex = component << 1;
			const Vec3V& boxMin = m_LocalBoxMinMaxs[twiceComponentIndex];
			const Vec3V& boxMax = m_LocalBoxMinMaxs[twiceComponentIndex + 1];

			Vec3V subBoundMin = subBound->GetBoundingBoxMin();
			Vec3V subBoundMax = subBound->GetBoundingBoxMax();

			return IsEqualAll(subBoundMin, boxMin) && IsEqualAll(subBoundMax, boxMax);
		}
	}

	return true;
}

bool phBoundComposite::DoesBoundingBoxContainComponent(int component) const
{	
	if(component >= 0 && component < GetNumBounds())
	{
		phBound* subBound = GetBound(component);
		if(subBound)
		{
			int twiceComponentIndex = component << 1;

			const Vec3V& boxMin = m_LocalBoxMinMaxs[twiceComponentIndex];
			const Vec3V& boxMax = m_LocalBoxMinMaxs[twiceComponentIndex + 1];
			const Mat34V& c = m_CurrentMatrices[component];
			Vec3V center = Average(boxMax, boxMin);
			Vec3V extents = boxMax - center;
			// TODO: This isn't taking into account the safe last matrix.
			COT_ExpandBoundOBBFromMotion(c, m_LastMatrices[component], subBound, extents, center);
			extents = geomBoxes::ComputeAABBExtentsFromOBB(c.GetMat33ConstRef(), extents);
			center = Transform(c, center);

			Vec3V expandedBoxMin = center - extents;
			Vec3V expandedBoxMax = center + extents;

			return IsLessThanOrEqualAll(expandedBoxMax, GetBoundingBoxMax() + Vec3V(V_FLT_SMALL_2)) && IsGreaterThanOrEqualAll(expandedBoxMin, GetBoundingBoxMin()-Vec3V(V_FLT_SMALL_2));
		}
	}

	return true;
}

bool phBoundComposite::DoesComponentBoundingBoxContainSupports(int component) const
{	
	if(component >= 0 && component < GetNumBounds())
	{
		phBound* subBound = GetBound(component);
		if(subBound)
		{
			return subBound->DoesBoundingBoxContainsSupports();
		}
	}
	return true;
}
#endif // __DEV && !__SPU

#if !PHBOUNDCOMPOSITE_BUILD_BVH
void phBoundComposite::UpdateBvh(bool)
{
}
#else // !PHBOUNDCOMPOSITE_BUILD_BVH


void phBoundComposite::UpdateBvh(bool fullRebuild)
{
	bool safeToWriteToBvh = THREADSAFE_BVH_UPDATE;

#if !__TOOL && !THREADSAFE_BVH_UPDATE && !__SPU
	// if it isn't threadsafe to write to the BVH, only the main thread can
	safeToWriteToBvh = safeToWriteToBvh || sysThreadType::IsUpdateThread();
#endif // !__TOOL && !THREADSAFE_BVH_UPDATE && !__SPU

	if(m_BVHStructure != NULL && safeToWriteToBvh)
	{
#if __SPU
		// Get the BVH from PPU
		u8 bvhStructureBuffer[sizeof(phOptimizedBvh)] ;
		phOptimizedBvh* ppuBVHStructure = m_BVHStructure;
		m_BVHStructure = reinterpret_cast<phOptimizedBvh*>(bvhStructureBuffer);
		cellDmaLargeGet(bvhStructureBuffer, (uint64_t)(ppuBVHStructure), sizeof(phOptimizedBvh), DMA_TAG(0), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(0));
#endif // __SPU

		// We have a BVH structure so we need to rebuild it to match whatever changes might have been made to the bounds.
		// Need to set the extents before we can quantize anything.
		const Vec3V boundingBoxMin = GetBoundingBoxMin();
		const Vec3V boundingBoxMax = GetBoundingBoxMax();
		m_BVHStructure->SetExtents(boundingBoxMin.GetIntrin128(), boundingBoxMax.GetIntrin128());

		// Make one BvhPrimitiveData for each of our bounds.
		const int numBounds = GetNumBounds();
		BvhPrimitiveData *bvhPrimitives = Alloca(BvhPrimitiveData, numBounds);
		const int numNonNullBounds = BuildPrimitiveDataFromPrimitives(bvhPrimitives, !fullRebuild);
		Assert(numNonNullBounds != 0);

		if(fullRebuild)
		{
			m_BVHStructure->BuildFromPrimitiveDataNoAllocate(bvhPrimitives, numNonNullBounds, phOptimizedBvh::INVALID_MIN_NODE_COUNT, NULL, 1, false);
		}
		else
		{
			m_BVHStructure->UpdateFromPrimitiveData(bvhPrimitives);
		}

#if __SPU
		// Set the PPU BVH to the updated SPU BVH
		cellDmaLargePut(bvhStructureBuffer, (uint64_t)(ppuBVHStructure), sizeof(phOptimizedBvh), DMA_TAG(0), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(0));
		m_BVHStructure = ppuBVHStructure;
#endif // __SPU
	}
}
#endif // !PHBOUNDCOMPOSITE_BUILD_BVH


void phBoundComposite::CalculateBoundBox()
{
	PrefetchDC(&m_CurrentMatrices[m_NumBounds-1]);
	PrefetchDC(&m_LastMatrices[m_NumBounds-1]);
	PrefetchDC(&m_LocalBoxMinMaxs[(m_NumBounds-1)*2]);

	Vec3V boundingBoxMin = Vec3V(V_FLT_MAX);
	Vec3V boundingBoxMax = Vec3V(V_NEG_FLT_MAX);
	bool foundSubBound = false;
	for (int boundIndex = m_NumBounds-1; boundIndex >= 0; boundIndex--)
	{
		PrefetchDC(&m_CurrentMatrices[boundIndex-2]);
		PrefetchDC(&m_LastMatrices[boundIndex-2]);
		PrefetchDC(&m_LocalBoxMinMaxs[(boundIndex-4)*2]);

		if (m_Bounds[boundIndex])
		{
			const Mat34V& c = m_CurrentMatrices[boundIndex];
			int twiceBoundIndex = boundIndex << 1;
			const Vec3V boxMin = m_LocalBoxMinMaxs[twiceBoundIndex];
			const Vec3V boxMax = m_LocalBoxMinMaxs[twiceBoundIndex + 1];

			Vec3V center = Average(boxMax, boxMin);
            Vec3V extents = boxMax - center;

			// TODO: This isn't taking into account the safe last matrix.
			COT_ExpandBoundOBBFromMotion(c, m_LastMatrices[boundIndex], m_Bounds[boundIndex], extents, center);

			extents = geomBoxes::ComputeAABBExtentsFromOBB(c.GetMat33ConstRef(), extents);
			center = Transform(c, center);

            const Vec3V expandedBoxMin = center - extents;
            const Vec3V expandedBoxMax = center + extents;

            boundingBoxMin = Min(boundingBoxMin, expandedBoxMin);
            boundingBoxMax = Max(boundingBoxMax, expandedBoxMax);

			foundSubBound = true;
		}
	}

	if(!foundSubBound)
	{
		// We have no sub-bounds!
		boundingBoxMin = Vec3V(V_ZERO);
		boundingBoxMax = Vec3V(V_ZERO);
	}

	Assert(IsLessThanOrEqualAll(boundingBoxMin, boundingBoxMax));
	SetBoundingBoxMax(boundingBoxMax);
	SetBoundingBoxMin(boundingBoxMin);
	phBound::SetCentroidOffset(ComputeBoundingBoxCenter());
}

void phBoundComposite::CalculateBoundBoxNoInternalMotion() 
{ 
	Assert(m_NumBounds);
	PrefetchDC(&m_CurrentMatrices[m_NumBounds-1]); 
	PrefetchDC(&m_LocalBoxMinMaxs[(m_NumBounds-1)*2]); 
	Vec3V boundingBoxMin = Vec3V(V_FLT_MAX); 
	Vec3V boundingBoxMax = Vec3V(V_NEG_FLT_MAX); 
	bool foundSubBound = false;
	for (int boundIndex = m_NumBounds-1; boundIndex >= 0; boundIndex--) 
	{ 
		PrefetchDC(&m_CurrentMatrices[boundIndex-2]); 
		PrefetchDC(&m_LocalBoxMinMaxs[(boundIndex-4)*2]); 
		if(m_Bounds[boundIndex])
		{
			const Mat34V& c = m_CurrentMatrices[boundIndex]; 
			int twiceBoundIndex = boundIndex << 1; 
			const Vec3V boxMin = m_LocalBoxMinMaxs[twiceBoundIndex]; 
			const Vec3V boxMax = m_LocalBoxMinMaxs[twiceBoundIndex + 1]; 
			Vec3V center = Average(boxMax, boxMin); 
			Vec3V extents = boxMax - center;
			extents = geomBoxes::ComputeAABBExtentsFromOBB(c.GetMat33ConstRef(), extents); 
			center = Transform(c, center);  
			const Vec3V expandedBoxMin = center - extents; 
			const Vec3V expandedBoxMax = center + extents; 
			boundingBoxMin = Min(boundingBoxMin, expandedBoxMin); 
			boundingBoxMax = Max(boundingBoxMax, expandedBoxMax); 
			foundSubBound = true;
		}
	} 

	if(!foundSubBound)
	{
		// We have no sub-bounds!
		boundingBoxMin = Vec3V(V_ZERO);
		boundingBoxMax = Vec3V(V_ZERO);
	}

	Assertf(IsLessThanOrEqualAll(boundingBoxMin, boundingBoxMax), "Min: <%f, %f, %f>, Max: <%f, %f, %f>",VEC3V_ARGS(boundingBoxMin),VEC3V_ARGS(boundingBoxMax)); 
	SetBoundingBoxMax(boundingBoxMax); 
	SetBoundingBoxMin(boundingBoxMin); 
	phBound::SetCentroidOffset(Average(boundingBoxMax, boundingBoxMin)); 
}

#if __ASSERT
bool phBoundComposite::CheckCachedMinMaxConsistency() const
{
	bool allMatch = true;
	const int numBounds = GetNumBounds();
	for(int curBoundIndex = 0; curBoundIndex < numBounds; ++curBoundIndex)
	{
		const phBound *curBound = GetBound(curBoundIndex);
		if(curBound != NULL)
		{
			Vec3V componentBoundingBoxMin = m_LocalBoxMinMaxs[curBoundIndex * 2 + 0];// curBound->GetBoundingBoxMin();
			Vec3V componentBoundingBoxMax = m_LocalBoxMinMaxs[curBoundIndex * 2 + 1];// curBound->GetBoundingBoxMax();
			Vec3V boundMin = curBound->GetBoundingBoxMin();
			Vec3V boundMax = curBound->GetBoundingBoxMax();
			const bool minsMatch = Verifyf(IsEqualAll(componentBoundingBoxMin, boundMin), "Composite cached min doesn't equal current bound min - <%f, %f, %f> / <%f, %f, %f>", componentBoundingBoxMin.GetXf(), componentBoundingBoxMin.GetYf(), componentBoundingBoxMin.GetZf(), boundMin.GetXf(), boundMin.GetYf(), boundMin.GetZf());
			const bool maxsMatch = Verifyf(IsEqualAll(componentBoundingBoxMax, boundMax), "Composite cached max doesn't equal current bound max - <%f, %f, %f> / <%f, %f, %f>", componentBoundingBoxMax.GetXf(), componentBoundingBoxMax.GetYf(), componentBoundingBoxMax.GetZf(), boundMax.GetXf(), boundMax.GetYf(), boundMax.GetZf());
			allMatch &= minsMatch;
			allMatch &= maxsMatch;
		}
	}

	return allMatch;
}
#endif	// __ASSERT


#if PHBOUNDCOMPOSITE_BUILD_BVH
int phBoundComposite::BuildPrimitiveDataFromPrimitives(BvhPrimitiveData *primitiveData, bool formatForUpdate) const
{
	const int numBounds = GetNumBounds();
	int nonNullBoundIndex = 0;
	for(int curBoundIndex = 0; curBoundIndex < numBounds; ++curBoundIndex)
	{
		const phBound *curBound = GetBound(curBoundIndex);
		if(curBound != NULL)
		{
			Vec3V componentBoundingBoxMin = m_LocalBoxMinMaxs[curBoundIndex * 2 + 0];// curBound->GetBoundingBoxMin();
			Vec3V componentBoundingBoxMax = m_LocalBoxMinMaxs[curBoundIndex * 2 + 1];// curBound->GetBoundingBoxMax();

#if !__SPU
			// Doing these asserts on the SPU would require extra DMAs
			ASSERT_ONLY(Vec3V boundMin = curBound->GetBoundingBoxMin());
			ASSERT_ONLY(Vec3V boundMax = curBound->GetBoundingBoxMax());
			Assertf(IsEqualAll(componentBoundingBoxMin, boundMin), "Composite cached min doesn't equal current bound min - <%f, %f, %f> / <%f, %f, %f>", componentBoundingBoxMin.GetXf(), componentBoundingBoxMin.GetYf(), componentBoundingBoxMin.GetZf(), boundMin.GetXf(), boundMin.GetYf(), boundMin.GetZf());
			Assertf(IsEqualAll(componentBoundingBoxMax, boundMax), "Composite cached max doesn't equal current bound max - <%f, %f, %f> / <%f, %f, %f>", componentBoundingBoxMax.GetXf(), componentBoundingBoxMax.GetYf(), componentBoundingBoxMax.GetZf(), boundMax.GetXf(), boundMax.GetYf(), boundMax.GetZf());
#endif // __SPU

			Vec3V componentOBBHalfExtents = ScalarV(V_HALF) * Subtract(componentBoundingBoxMax, componentBoundingBoxMin);
			Vec3V componentOBBCentroid = Average(componentBoundingBoxMin, componentBoundingBoxMax);

			Mat34V_ConstRef currentMatrix = GetCurrentMatrix(curBoundIndex);
			Mat34V_ConstRef lastMatrix = GetLastMatrix(curBoundIndex);
			// TODO: This isn't taking into account the safe last matrix.
			COT_ExpandBoundOBBFromMotion(currentMatrix, lastMatrix, curBound, componentOBBHalfExtents, componentOBBCentroid);
			const Vec3V componentAABBHalfExtents = geomBoxes::ComputeAABBExtentsFromOBB(currentMatrix.GetMat33ConstRef(), componentOBBHalfExtents);
			const Vec3V componentAABBCentroid = Transform(currentMatrix, componentOBBCentroid);
			componentBoundingBoxMin = Subtract(componentAABBCentroid, componentAABBHalfExtents);
			componentBoundingBoxMax = Add(componentAABBCentroid, componentAABBHalfExtents);

			// Quantize the AABB and stuff that data into the node.
			BvhPrimitiveData &curPrimitiveData = primitiveData[nonNullBoundIndex];
			curPrimitiveData.Clear();		// This is unnecessary - it only clears the primitive index which we end up setting a few lines down.
			m_BVHStructure->QuantizeMin(curPrimitiveData.m_AABBMin, RCC_VECTOR3(componentBoundingBoxMin));
			m_BVHStructure->QuantizeMax(curPrimitiveData.m_AABBMax, RCC_VECTOR3(componentBoundingBoxMax));
			// NOTE: The centroid here doesn't *have* to just be the center of the AABB - if that were the case it would be pretty stupid to have it.  The
			//   centroid actually provides some information about the contained primitive that is used to resolves ties when constructing the BVH.  It
			//   would probably make sense to use the center of the bound's bounding sphere here. - Actually the centroid is currently not used for anything.
			m_BVHStructure->QuantizeClosest(curPrimitiveData.m_Centroid, RCC_VECTOR3(componentAABBCentroid));
			curPrimitiveData.m_PrimitiveIndex = (rage::phPolygon::Index)(curBoundIndex);
			++nonNullBoundIndex;
		}
		else if(formatForUpdate)
		{
			// If formatting the bvh primitives for an UpdateTree call, leave gaps in the data so that the index of a primitive is the same of its matching sub-bound
			++nonNullBoundIndex;
		}
	}

	return nonNullBoundIndex;
}
#endif

#if !__SPU
#if __DEBUGLOG
void phBoundComposite::DebugReplay() const
{
	for (int i = 0; i < m_NumBounds; i++)
	{
		if(m_Bounds[i])
		{
			const Matrix34& c = m_CurrentMatrices[i];
			const Vec3V& boxMin = m_Bounds[i]->GetBoundingBoxMin();
			const Vec3V& boxMax = m_Bounds[i]->GetBoundingBoxMax();

			diagDebugLog(diagDebugLogPhysics, 'BCCM', &c);
			diagDebugLog(diagDebugLogPhysics, 'BCbm', &boxMin);
			diagDebugLog(diagDebugLogPhysics, 'BCbM', &boxMax);
		}
	}
}
#endif
#endif

Vec3V_Out phBoundComposite::ComputeCompositeAngInertia (float density, const float* massList, const Vec3V* angInertiaList)
{
	using namespace Vec;

    // Technically we require the mass list to contain non-zero values also, that is enforced in a later assert
    Assert(density > 0.0f || massList != NULL); 

	// Add the angular inertias of all the bound parts.
	phBound* partBound;
	Vec3V partAngInertia,totalAngInertia(V_ZERO),partOffset;
	float totalMass = 0.0f;
	float partMass;
	for (int partIndex=0; partIndex<m_NumBounds; partIndex++)
	{
		partBound = m_Bounds[partIndex];
		if (partBound)
		{
			// Find the part's mass and angular inertia about its center of mass.
			if (massList)
			{
				partMass = massList[partIndex];
			}
			else
			{
				partMass = density*partBound->GetVolume();
			}

			if (angInertiaList)
			{
				partAngInertia = angInertiaList[partIndex];
			}
			else
			{
				partAngInertia = partBound->GetComputeAngularInertia(partMass);
			}

			// Find the offset of the part's center of mass from the composite's center of mass.
			// think GTA version is correct after lots of work looking at fragment and vehicle inertia calculations
			// want to get this moved to rage/dev
			partOffset = Transform(m_CurrentMatrices[partIndex], partBound->GetCGOffset());
			partOffset -= GetCGOffset();

			// Add the angular inertia from the centers of mass offset.
			Vec3V offsetLeft = partOffset.Get<Y, X, X>();
			Vec3V offsetRight = partOffset.Get<Z, Z, Y>();
			partAngInertia += ScalarV(partMass) * (Scale(offsetLeft, offsetLeft) + Scale(offsetRight, offsetRight));
// 			partAngInertia.x += partMass*(square(partOffset.y)+square(partOffset.z));
// 			partAngInertia.y += partMass*(square(partOffset.x)+square(partOffset.z));
// 			partAngInertia.z += partMass*(square(partOffset.x)+square(partOffset.y));

			// Add the part mass and angular inertia to the total mass and angular inertia.
			totalMass += partMass;
			totalAngInertia += partAngInertia;
		}
	}

	// Scale the angular inertia to get the volume distribution.
 	Vec3V volumeDistribution;
	if (totalMass>0.0f)
	{
		volumeDistribution = InvScale(totalAngInertia,ScalarV(totalMass));
	}
	else
	{
		volumeDistribution = Vec3V(V_ZERO);
	}

	m_VolumeDistribution.SetXYZ(volumeDistribution);

	return totalAngInertia;
}

#if !__SPU

/*
Purpose: Calculate and set the center of gravity of this composite bound.
Parameters: massList	- optional list of masses for the composite bound parts (default is NULL)
Notes:
1.	If the argument massList is NULL (default case), then the center of gravity is calculated with the same uniform
	density for all the composite bound parts. If the mass list is provided, then the center of gravity is calculated
	with the given mass for each composite bound part. */
void phBoundComposite::CalcCenterOfGravity (const float* massList)
{
	Vec3V centerOfGravity(ORIGIN),partCenter;
	float partWeight,totalWeight=0.0f;
	for (int partIndex=0; partIndex<m_NumBounds; partIndex++)
	{
		if (m_Bounds[partIndex])
		{
			// This composite bound part exists. Find its volume or its mass.
			if (massList)
			{
				partWeight = massList[partIndex];
			}
			else
			{
				partWeight = m_Bounds[partIndex]->GetVolume();
			}

			// Add this part's volume or mass to the total.
			totalWeight += partWeight;

			// Find the center of gravity of this composite bound part.
			partCenter = Transform(m_CurrentMatrices[partIndex], m_Bounds[partIndex]->GetCGOffset());

			// Add this weighted part center of gravity to the total.
			centerOfGravity += partCenter * ScalarV(partWeight);
		}
	}

	if (totalWeight>SMALL_FLOAT)
	{
		// Scale the total weighted center of gravity to get the bound's center of gravity.
		centerOfGravity = InvScale(centerOfGravity, ScalarV(totalWeight));
	}

	// Set the bound's center of gravity.
	SetCGOffset(centerOfGravity);
}


// Calculate the center of the bound from the bound parts and their positions and orientations.
void phBoundComposite::CalcCenterOfBound ()
{
	Vec3V ctr(V_ZERO),partCenter;
	int numBounds=0;
	for(int partIndex=0;partIndex<m_NumBounds;partIndex++)
	{
		if(m_Bounds[partIndex])
		{
			partCenter = Transform(m_CurrentMatrices[partIndex], m_Bounds[partIndex]->GetCentroidOffset());
			ctr += partCenter;
			numBounds++;
		}
	}
	if (numBounds > 0)
	{
		ctr = InvScale(ctr, ScalarV((float)numBounds));
	}
	SetCentroidOffset(ctr);
}


void phBoundComposite::AllocateTypeAndIncludeFlags()
{
	// This change needs to be taken across to rage/dev I think
	// see comments in constructor above
	Assert(m_OwnedTypeAndIncludeFlags == NULL);
	m_TypeAndIncludeFlags = m_OwnedTypeAndIncludeFlags = rage_new u32[2 * m_MaxNumBounds];
	// initialise type and include flags to ALL
	for(int i=0; i<2 * m_MaxNumBounds; i++)
		m_OwnedTypeAndIncludeFlags[i] = TYPE_FLAGS_ALL;
}


void phBoundComposite::SetTypeAndIncludeFlags(u32* flagArray)
{
	// This change needs to be taken across to rage/dev I think
	// see comments in constructor above

	// If this bound has it's own copy of typeAndInclude flags allocated, copy flags into there
	if (m_OwnedTypeAndIncludeFlags != NULL)
	{
		sysMemCpy(m_OwnedTypeAndIncludeFlags, flagArray, sizeof(u32) * 2 * m_MaxNumBounds);
	}
	// If we don't have our own flags allocated, just point directly to the ones we're passed in
	else
		m_TypeAndIncludeFlags = flagArray;
}


bool phBoundComposite::IsPolygonal (int component) const
{
	const phBound* boundPart = GetBound(component);
	if (boundPart)
	{
		return boundPart->IsPolygonal();
	}

	return false;
}


// Pointers to the Bounds and LocalMatrices arrays in phBoundComposite keep their original values
// in the clone and each phBoundComposite keeps its own lists.  Pointers to LastMatrices and
// CurrentMatrices also maintain their original values, but their arrays are not copied to the clone
// because the default values are the same as the LocalMatrices pointer.
void phBoundComposite::Copy (const phBound* original)
{
	// Verify that the clone is a composite and get a composite pointer to the clone.
	Assert(original->GetType()==phBound::COMPOSITE);
	const phBoundComposite* compositeOriginal = static_cast<const phBoundComposite*>(original);

	// Make sure the clone is a composite with at least the minimum number of parts.
	Assert(m_MaxNumBounds>=compositeOriginal->GetNumBounds());

	// Copy out pointers that will be changed when the original bound is copied to the clone.
	datOwner<phBound>* keepBoundParts = m_Bounds;
	Mat34V* keepLastMatrices = m_LastMatrices;
	Mat34V* keepCurrentMatrices = m_CurrentMatrices;
    Vec3V* keepLocalBoxMinMaxs = m_LocalBoxMinMaxs;
#if PHBOUNDCOMPOSITE_BUILD_BVH
	// Delete this in case we already have one allocated.  TODO: Maybe if m_BVHStructure isn't NULL we should just be rebuilding the BVH instead of delete'ing and
	//   re-allocating the memory?  That really depends on whether or not we expect it to be of an okay size if it has already been allocated.  The asserts above
	//   (regarding the number of bounds) make it seem like that would be the case.
	delete m_BVHStructure;
#endif
	// This change needs to be taken across to rage/dev I think
	// see comments in constructor above
	u32* keepTypeAndIncludeFlags = m_OwnedTypeAndIncludeFlags;
	int keepm_MaxNumBounds = m_MaxNumBounds;

	// Copy everything in the bound to the clone.
	*this = *compositeOriginal;
	
	if (phConfig::IsRefCountingEnabled())
	{
		SetRefCount(1);
	}

	// Reset the bound part pointers and matrix array pointers in the clone.
	m_Bounds = keepBoundParts;
	m_LastMatrices = keepLastMatrices;
	m_CurrentMatrices = keepCurrentMatrices;
    m_LocalBoxMinMaxs = keepLocalBoxMinMaxs;
	m_TypeAndIncludeFlags = keepTypeAndIncludeFlags;
	// This change needs to be taken across to rage/dev, see above
	m_OwnedTypeAndIncludeFlags = keepTypeAndIncludeFlags;
	m_MaxNumBounds = static_cast<u16>(keepm_MaxNumBounds);

	// This change needs to be taken across to rage/dev, see above
	if (compositeOriginal->m_OwnedTypeAndIncludeFlags)
	{
		if (m_OwnedTypeAndIncludeFlags == NULL)
		{
			AllocateTypeAndIncludeFlags();
		}
		else
		{
			m_TypeAndIncludeFlags = m_OwnedTypeAndIncludeFlags;
		}
	}
	else
	{
		delete [] m_OwnedTypeAndIncludeFlags;
		m_OwnedTypeAndIncludeFlags = NULL;
	}
	// Copy the lists of bound part pointers and local matrices, so that the clone has its
	// own lists instead of pointing to the original's lists.
	for (int partIndex=0;partIndex<m_NumBounds;partIndex++)
	{
#if ENABLE_KNOWN_REFERENCES
		// May not be known if it's only been resource constructed
		if (IS_REF_KNOWN(m_Bounds[partIndex]))
			SAFE_REMOVE_KNOWN_REF(m_Bounds[partIndex]);
#endif
		if (phConfig::IsRefCountingEnabled() && m_Bounds[partIndex])
		{
			// Release any bounds that were in the clone's list before copying.
			AssertMsg(m_Bounds[partIndex]->GetRefCount()>1, "Record m_NumBounds, BoundIndex and RefCount.");
			m_Bounds[partIndex]->Release(false);
		}
		m_Bounds[partIndex]=NULL;
		SetBound(partIndex,compositeOriginal->GetBound(partIndex));
		SetCurrentMatrix(partIndex,compositeOriginal->GetCurrentMatrix(partIndex));

		if(m_CurrentMatrices != m_LastMatrices)
		{
			SetLastMatrix(partIndex, compositeOriginal->GetLastMatrix(partIndex));
		}

		int partIndexShifted = partIndex << 1;
        m_LocalBoxMinMaxs[partIndexShifted] = compositeOriginal->m_LocalBoxMinMaxs[partIndexShifted];
        m_LocalBoxMinMaxs[partIndexShifted + 1] = compositeOriginal->m_LocalBoxMinMaxs[partIndexShifted + 1];

		// This change needs to be taken across to rage/dev, see above
		if (m_OwnedTypeAndIncludeFlags)
		{
			m_OwnedTypeAndIncludeFlags[partIndexShifted] = compositeOriginal->m_OwnedTypeAndIncludeFlags[partIndexShifted];
			m_OwnedTypeAndIncludeFlags[partIndexShifted + 1] = compositeOriginal->m_OwnedTypeAndIncludeFlags[partIndexShifted + 1];
		}
	}

#if PHBOUNDCOMPOSITE_BUILD_BVH
	m_BVHStructure = NULL;
	PostLoadCompute();
#endif
}


phBound* phBoundComposite::Clone () const
{
	phBoundComposite* clone = rage_new phBoundComposite();
	bool allowInternalMotion = (m_CurrentMatrices != m_LastMatrices);
	clone->Init(m_MaxNumBounds,allowInternalMotion);
	clone->Copy(this);
	return clone;
}

void phBoundComposite::CloneParts()
{
	for (int partIndex=0;partIndex<m_NumBounds;partIndex++)
	{
		if (m_Bounds[partIndex])
		{
			SetBound(partIndex, m_Bounds[partIndex]->Clone());
			m_Bounds[partIndex]->Release();
		}
	}
}

void phBoundComposite::DeleteParts()
{
	for (int partIndex=0;partIndex<m_NumBounds;partIndex++)
	{
		if (m_Bounds[partIndex])
		{
#if ENABLE_KNOWN_REFERENCES
			// May not be known yet if it's only been resource constructed
			if (IS_REF_KNOWN(m_Bounds[partIndex]))
				SAFE_REMOVE_KNOWN_REF(m_Bounds[partIndex]);
#endif
			delete m_Bounds[partIndex];
			m_Bounds[partIndex] = NULL;
		}
	}
}



const phMaterial& phBoundComposite::GetMaterial (phMaterialIndex i) const
{
	return GetMaterial(i,0);
}



// get a material from a specified component bound
// get a material from a specified component bound
const phMaterial& phBoundComposite::GetMaterial(phMaterialIndex i, int component) const
{
	const phBound* partBound = GetBound(component);
	if (partBound)
	{
		return partBound->GetMaterial(i);
	}

	return MATERIALMGR.GetDefaultMaterial();
}


phMaterialMgr::Id phBoundComposite::GetMaterialId (phMaterialIndex i) const
{
	return GetMaterialId(i,0);
}


// get a material from a specified component bound
// get a material from a specified component bound
phMaterialMgr::Id phBoundComposite::GetMaterialId(phMaterialIndex i, int component) const
{
	const phBound* partBound = GetBound(component);
	if (partBound)
	{
		return partBound->GetMaterialId(i);
	}

	return phMaterialMgr::DEFAULT_MATERIAL_ID;
}

int phBoundComposite::GetNumMaterials() const
{
	if (m_Bounds[0])
	{
		return m_Bounds[0]->GetNumMaterials();
	}

	return 0;
}


void phBoundComposite::SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex))
{
	for(int index=0;index<m_NumBounds;index++)
	{
		if(m_Bounds[index])
		{
			m_Bounds[index]->SetMaterial(materialId);
		}
	}
}

phMaterialMgr::Id phBoundComposite::GetMaterialIdFromPartIndexAndComponent (int partIndex, int component) const
{
	const phBound* boundPart = GetBound(component);
	if (boundPart)
	{
		return boundPart->GetMaterialIdFromPartIndex(partIndex);
	}

	return phMaterialMgr::DEFAULT_MATERIAL_ID;
}


bool phBoundComposite::CanBecomeActive () const
{
	for (int component=0; component<m_NumBounds; component++)
	{
		const phBound* boundPart = GetBound(component);
		if (boundPart && !boundPart->CanBecomeActive())
		{
			// This composite bound has a part that can not become active, so this composite bound can not become active.
			return false;
		}
	}

	// This composite bound does not have any parts that can not become active, so it can become active.
	return true;
}


#if __PFDRAW
float g_AnimateFromLastPhase = false;

void phBoundComposite::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int whichPolys, phMaterialFlags highlightFlags, unsigned int typeFilter, unsigned int includeFilter, unsigned int boundTypeFlags, unsigned int boundIncludeFlags) const
{
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	Matrix34 partWorld;
	for(int i=0;i<m_NumBounds;i++)
	{
		if(PFD_DrawSingleComponent.GetEnabled() && PFD_SelectComponentToDraw.GetValue() != i)
			continue;

		if(m_Bounds[i])
		{
			u32 subBoundTypeFlags = boundTypeFlags;
			u32 subBoundIncludeFlags = boundIncludeFlags;
            if (m_TypeAndIncludeFlags)
            {
				subBoundTypeFlags = GetTypeFlags(i);
                if (!(subBoundTypeFlags & typeFilter))
                {
                    continue;
                }

				subBoundIncludeFlags = GetIncludeFlags(i);
				if (!(subBoundIncludeFlags & includeFilter))
				{
					continue;
				}
            }

			Mat34V drawMatrix = m_CurrentMatrices[i];

			if (PFD_AnimateFromLast.WillDraw() && m_LastMatrices != m_CurrentMatrices)
			{
				Mat34V lastMatrix = m_LastMatrices[i];
				Mat34V interpolatedMatrix;

				Vec3V linVel = Subtract(GetBound(i)->GetCenterOfMass(drawMatrix), GetBound(i)->GetCenterOfMass(lastMatrix));
				interpolatedMatrix.Set3x3(drawMatrix);
				interpolatedMatrix.SetCol3( SubtractScaled(drawMatrix.GetCol3(), linVel, ScalarV(V_ONE) - ScalarV(g_AnimateFromLastPhase)) );

				drawMatrix = interpolatedMatrix;
			}

			partWorld.Dot(RCC_MATRIX34(drawMatrix),mtx);
			m_Bounds[i]->Draw(RCC_MAT34V(partWorld), colorMaterials, solid, whichPolys, highlightFlags, typeFilter, includeFilter, subBoundTypeFlags, subBoundIncludeFlags);
			if (PFD_ComponentIndices.GetEnabled() && PFD_ComponentIndices.GetParentsEnabled())
			{
			    grcBindTexture(NULL);
				bool oldLighting = grcLighting(false);
				u32 oldColor = grcCurrentColor;
				grcColor(PFD_ComponentIndices.GetBaseColor());

				Vec3V boundMin = m_Bounds[i]->GetBoundingBoxMin();
				Vec3V boundMax = m_Bounds[i]->GetBoundingBoxMax();
				Vec3V boxCenter = Transform(RCC_MAT34V(partWorld), Average(boundMin, boundMax));
		
				char componentIndexText[8];
				componentIndexText[7] = '\0';
				formatf(componentIndexText,7,"%i",i);
				grcDrawLabel(boxCenter,componentIndexText,true);

				grcColor(Color32(oldColor));
				grcLighting(oldLighting);
			}
		}
	}

	if(m_BVHStructure && PFD_BVHHierarchy.WillDraw())
	{
		m_BVHStructure->Draw(mtxIn, (int)PFD_NodeDepth.GetValue(),PFD_BVHHierarchyNodeIndices.WillDraw());
	}
}

void phBoundComposite::DrawSupport(Mat34V_In mtx, unsigned int typeFilter, unsigned int includeFilter) const
{
	Matrix34 partWorld;
	for(int i=0;i<m_NumBounds;i++)
	{
		if(PFD_DrawSingleComponent.GetEnabled() && PFD_SelectComponentToDraw.GetValue() != i)
			continue;

		if(m_Bounds[i])
		{
			if (m_TypeAndIncludeFlags)
			{
				if (!(GetTypeFlags(i) & typeFilter))
				{
					continue;
				}

				const u32 includeFlags = GetIncludeFlags(i);			// cloth sphere bound has 0 for include flag
				if (includeFlags && !(includeFlags & includeFilter))
				{
					continue;
				}
			}

			Mat34V drawMatrix = m_CurrentMatrices[i];

			if (PFD_AnimateFromLast.WillDraw() && m_LastMatrices != m_CurrentMatrices)
			{
				Mat34V lastMatrix = m_LastMatrices[i];
				Mat34V interpolatedMatrix;

				Vec3V linVel = Subtract(GetBound(i)->GetCenterOfMass(drawMatrix), GetBound(i)->GetCenterOfMass(lastMatrix));
				interpolatedMatrix.Set3x3(drawMatrix);
				interpolatedMatrix.SetCol3( SubtractScaled(drawMatrix.GetCol3(), linVel, ScalarV(V_ONE) - ScalarV(g_AnimateFromLastPhase)) );

				drawMatrix = interpolatedMatrix;
			}

			partWorld.Dot(RCC_MATRIX34(drawMatrix),RCC_MATRIX34(mtx));
			m_Bounds[i]->DrawSupport(RCC_MAT34V(partWorld),typeFilter,includeFilter);
			if (PFD_ComponentIndices.GetEnabled() && PFD_ComponentIndices.GetParentsEnabled())
			{
				grcBindTexture(NULL);
				bool oldLighting = grcLighting(false);
				u32 oldColor = grcCurrentColor;
				grcColor(PFD_ComponentIndices.GetBaseColor());

				char componentIndexText[8];
				componentIndexText[7] = '\0';
				formatf(componentIndexText,7,"%i",i);
				grcDrawLabel(partWorld.d,componentIndexText,true);

				grcColor(Color32(oldColor));
				grcLighting(oldLighting);
			}
		}
	}
}

void phBoundComposite::DrawNormals(Mat34V_In mtxIn, int normalType, int whichPolys, float length, unsigned int typeFilter, unsigned int includeFilter) const
{
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	Matrix34 partWorld;
	for (int i=0; i<m_NumBounds; i++)
	{
		if (m_Bounds[i])
		{
			if (m_TypeAndIncludeFlags)
			{
				if (!(GetTypeFlags(i) & typeFilter))
				{
					continue;
				}

				if (!(GetIncludeFlags(i) & includeFilter))
				{
					continue;
				}
			}

			partWorld.Dot(RCC_MATRIX34(m_CurrentMatrices[i]),mtx);
			m_Bounds[i]->DrawNormals(RCC_MAT34V(partWorld), normalType, whichPolys, length);
		}
	}
}

void phBoundComposite::DrawLast(Mat34V_In mtxIn, bool colorMaterials, bool solid, int whichPolys, phMaterialFlags highlightFlags) const
{
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	Matrix34 partWorld;
	for(int i=0;i<m_NumBounds;i++)
	{
		if(m_Bounds[i])
		{
			partWorld.Dot(RCC_MATRIX34(m_LastMatrices[i]),mtx);
			m_Bounds[i]->Draw(RCC_MAT34V(partWorld), colorMaterials, solid, whichPolys, highlightFlags);
			if (PFD_ComponentIndices.GetEnabled() && PFD_ComponentIndices.GetParentsEnabled())
			{
			    grcBindTexture(NULL);
				bool oldLighting = grcLighting(false);
				u32 oldColor = grcCurrentColor;
				grcColor(PFD_ComponentIndices.GetBaseColor());

				char componentIndexText[8];
				componentIndexText[7] = '\0';
				formatf(componentIndexText,7,"%i",i);
				grcDrawLabel(partWorld.d,componentIndexText,true);

				grcColor(Color32(oldColor));
				grcLighting(oldLighting);
			}
		}
	}
}
#endif // __PFDRAW

#if __NMDRAW
void phBoundComposite::NMRender(Mat34V_In mtx) const
{
  Matrix34 partWorld;
  for(int i=0;i<m_NumBounds;i++)
  {
    if(m_Bounds[i])
    {
      partWorld.Dot(m_CurrentMatrices[i],mtx);
      m_Bounds[i]->NMRender(partWorld);
    }
  }
}
#endif

phBoundComposite::phBoundComposite (datResource & rsc)
	: phBound(rsc)
{
	rsc.PointerFixup(m_Bounds);

	for (int i=0; i<m_MaxNumBounds; i++)
	{
		// If we're defragmenting, and it's a tracked pointer, just fix up the contents of the pointer 
		// since the defrag system will have already fixed up the pointer itself. 
		// Otherwise, if we're not defragmenting, or it's not a known pointer (because SetBound hasn't 
		// been called on this phBoundComposite yet) then we have to fix up both the pointer and its contents. 
		if(datResource_IsDefragmentation && IS_REF_KNOWN(m_Bounds[i]))
		{
			m_Bounds[i]->Place(m_Bounds[i],rsc);
		}
		else
		{
			::new (m_Bounds+i) datOwner<phBound>();
		}
	}

	if (m_LastMatrices == m_CurrentMatrices)
	{
		rsc.PointerFixup(m_LastMatrices);
	}
	else
	{
		ObjectFixup(rsc,m_LastMatrices,m_MaxNumBounds);
	}
	ObjectFixup(rsc,m_CurrentMatrices,m_MaxNumBounds);
    ObjectFixup(rsc,m_LocalBoxMinMaxs,m_NumBounds * 2);	// ObjectFixup shouldn't even be necessary here.
	rsc.PointerFixup(m_TypeAndIncludeFlags);
	rsc.PointerFixup(m_OwnedTypeAndIncludeFlags);

#if RAGE_RELEASE <= 314
	if (m_NumActiveBounds == 0)
	{
		int numActiveBounds = 0;
		for (int boundIndex=0; boundIndex<m_NumBounds; boundIndex++)
		{
			if (m_Bounds[boundIndex])
			{
				numActiveBounds++;
			}
		}
		m_NumActiveBounds = (u16)numActiveBounds;
	}
#endif

	SetMargin(0);

#if VALIDATE_COMPONENT_MATRICES
	if(!datResource_IsDefragmentation)
	{
		for (int i=0; i<m_MaxNumBounds; i++)
		{
			Assert(m_CurrentMatrices[i].IsOrthonormal3x3(ScalarV(V_FLT_SMALL_2)));
			Assert(m_LastMatrices[i].IsOrthonormal3x3(ScalarV(V_FLT_SMALL_2)));
		}
	}
#endif // VALIDATE_COMPONENT_MATRICES
}

#if __DECLARESTRUCT
void phBoundComposite::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundComposite);
	STRUCT_DYNAMIC_ARRAY(m_Bounds,m_MaxNumBounds);

	bool allowedToMove = (m_CurrentMatrices != m_LastMatrices);
	STRUCT_DYNAMIC_ARRAY(m_CurrentMatrices, m_MaxNumBounds);
	if (allowedToMove)
	{
		STRUCT_DYNAMIC_ARRAY(m_LastMatrices, m_MaxNumBounds);
	}
	else
	{
		STRUCT_FIELD_VP(m_LastMatrices);
	}

    STRUCT_DYNAMIC_ARRAY(m_LocalBoxMinMaxs, m_NumBounds * 2);
	STRUCT_FIELD_VP(m_TypeAndIncludeFlags);
	STRUCT_DYNAMIC_ARRAY(m_OwnedTypeAndIncludeFlags, m_MaxNumBounds * 2);
	STRUCT_FIELD(m_MaxNumBounds);
	STRUCT_FIELD(m_NumBounds);
	STRUCT_FIELD(m_BVHStructure);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

#endif  //!__SPU

} // namespace rage
