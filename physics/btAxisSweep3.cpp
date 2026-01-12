
//Bullet Continuous Collision Detection and Physics Library
//Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

#include "btAxisSweep3.h"
#include "debugphysics.h"

#include "simulator.h"

#include "phcore/pool.h"
#include "profile/profiler.h"
#include "vector/vector3.h"
#include "vectormath/classes.h"

#include <assert.h>
#include <algorithm>

GJK_COLLISION_OPTIMIZE_OFF()

#define AXISSWEEP_EXTRA_CONSISTENCY_CHECKS (0 && __DEV)

#define AXISSWEEP_SPEW_DISPLAYF(format, ...) // Displayf(format, __VA_ARGS__)

// This enables code that checks for invalid AABBs (all components of max not >= corresponding components of min).
// Of course there's a minor performance hit for this but if you don't want to ensure that you're passing good data into here, this is better than
//  messing things up.
#define AXISSWEEP_PATCH_BAD_AABBS	1

#define USE_EDGE_PREFETCH		1

PHYSICS_OPTIMIZATIONS

namespace rage
{
#if 0
	// Simple little function to confirm that the sentinel (on a given set of axes) is correct and consistent.
	static void CheckSentinel(const btAxisSweep3 *axisSweep, int axisMin, int axisMax)
	{
		const int maxHandles = axisSweep->m_maxHandles;
		const int numHandlesInUse = axisSweep->m_numHandles;
		const int maxEdgeIndex = numHandlesInUse * 2 + 1;
		const btAxisSweep3::Edge *edges[3] = { axisSweep->m_pEdges[0], axisSweep->m_pEdges[1], axisSweep->m_pEdges[2] };
		const btAxisSweep3::Handle *handles = axisSweep->m_pHandles;
		const btAxisSweep3::Handle &sentinelHandle = handles[maxHandles];

		for(int axis = axisMin; axis <= axisMax; ++axis)
		{
			// Make sure that the edges are in the correct spots.
			if(edges[axis][0].m_pos != 0)
			{
				__debugbreak();
			}
			if(edges[axis][0].m_handle != maxHandles)
			{
				__debugbreak();
			}
			if(edges[axis][maxEdgeIndex].m_pos != 0xffff)
			{
				__debugbreak();
			}
			if(edges[axis][maxEdgeIndex].m_handle != maxHandles)
			{
				__debugbreak();
			}

			// Make sure that the sentinel knows where it's edges are.
			if(sentinelHandle.m_minEdgeIndices[axis] != 0)
			{
				__debugbreak();
			}
			if(sentinelHandle.m_maxEdgeIndices[axis] != maxEdgeIndex)
			{
				__debugbreak();
			}
		}
	}
#endif

namespace BroadphaseCollisionStats
{
	EXT_PF_TIMER(SortPairs);
	EXT_PF_TIMER(TestOverlaps);
};

using namespace BroadphaseCollisionStats;


const btAxisSweep3::Handle btAxisSweep3::Handle::UNUSED(0xffff, 0x0);

namespace phSweepAndPruneStats
{
    PF_PAGE(PHSweepAndPrune,"ph SweepAndPrune");

    PF_GROUP(SweepAndPrune);
    PF_LINK(PHSweepAndPrune,SweepAndPrune);

    PF_TIMER(SAPAddHandle,SweepAndPrune);
    PF_TIMER(SAPRemoveHandle,SweepAndPrune);

    PF_TIMER(SAPAddPair,SweepAndPrune);
    PF_TIMER(SAPRemovePair,SweepAndPrune);

	PF_TIMER(SAPUpdateHandle,SweepAndPrune);
	PF_COUNTER(SAPUpdateHandleNumCalls,SweepAndPrune);

#if USE_GLOBAL_SAP_UPDATE
	PF_TIMER(SAPDoSort,SweepAndPrune);
	PF_COUNTER(SAPDoSortNumCalls,SweepAndPrune);
#endif
};

using namespace phSweepAndPruneStats;


bool DefaultKeepPairsFunc(int UNUSED_PARAM(levelIndex0), int UNUSED_PARAM(levelIndex1))
{
	return true;
}


btAxisSweep3::btAxisSweep3(phLevelNew* level,u16 maxHandles, u16 maxPairs, Vector3::Vector3Param worldMin, Vector3::Vector3Param worldMax, btOverlappingPairCache *existingPairCache )
:phBroadPhase( maxPairs, existingPairCache )
, m_Level(level)
, m_NumSorted(0)
, m_KeepPairFunc(MakeFunctorRet(DefaultKeepPairsFunc))
{

#if __ASSERT
	m_threadId = sysIpcGetCurrentThreadId();
#endif

	m_worldAabbMin.Set(worldMin);
	m_worldAabbMax.Set(worldMax);

	// 1 handle is reserved as sentinel
	// - Note: I'm not sure if this assert really is quite valid. 1 of the handles
	//   may be reserved, but it seems like the code below adds that on top of what
	//   the user requests already. /FF
	Assert(maxHandles > 1 && maxHandles < 32767);
	// This is probably quite a bit larger than it needs to be.

	Vector3 aabbSize = m_worldAabbMax - m_worldAabbMin;

	m_quantize = Vector3(65535.0f,65535.0f,65535.0f) / aabbSize;
	m_InvQuantize = aabbSize / Vector3(65535.0f,65535.0f,65535.0f);

	// allocate handles buffer and put all handles on free list
	m_pHandles = rage_new Handle[maxHandles + 1];
	for (int handle = 0; handle < maxHandles + 1; ++handle)
	{
		m_pHandles[handle] = Handle::UNUSED;
	}
	m_maxHandles = maxHandles;
	m_numHandles = 0;

	{
	// allocate edge buffers
	for (int i = 0; i < 3; i++)

		// This used to be maxHandles * 2, but I changed it to (maxHandles + 1) * 2
		// with Eugene's approval. Without this, I found that trying to insert
		// maxHandles objects (which the user could reasonably expect to be able to)
		// would cause an array overrun in addHandle(). /FF
		m_pEdges[i] = rage_new Edge[(maxHandles + 1)* 2];
	}
	//removed overlap management

	// make boundary sentinels
	
	for (int axis = 0; axis < 3; axis++)
	{
		m_pHandles[maxHandles].m_minEdgeIndices[axis] = 0;
		m_pHandles[maxHandles].m_maxEdgeIndices[axis] = 1;

		m_pEdges[axis][0].m_pos = 0;
		m_pEdges[axis][0].m_handle = maxHandles;
		m_pEdges[axis][1].m_pos = 0xffff;
		m_pEdges[axis][1].m_handle = maxHandles;
	}
}

btAxisSweep3::~btAxisSweep3()
{
    while (m_numHandles)
    {
        // There might be a better way to do this...but edge #1 should always be an in-use handle.
        // This is definitely not a very efficient way to remove all the handles, but it does seem to work.
        removeHandle(m_pEdges[0][1].m_handle);
    }

	for (int i = 2; i >= 0; i--)
		delete[] m_pEdges[i];
	delete[] m_pHandles;

}

inline void btAxisSweep3::quantize(Vector3::Ref vout, Vector3::Param min, Vector3::Param max) const
{
#if __XENON 
	Vector3 minscaled = (Vector3(min) - m_worldAabbMin) * m_quantize;
	Vector3 maxscaled = (Vector3(max) - m_worldAabbMin) * m_quantize;
	__vector4 minfixed = __vcfpuxws(minscaled, 0);
	__vector4 maxfixed = __vcfpuxws(maxscaled, 0);
	__vector4 packed = __vpkuwus(minfixed, maxfixed);
	__vector4 masked = __vand(packed, __vspltish(~1));
	__vector4 maxflags = __vsldoi(__vspltish(0), __vspltish(1), 8);
	__vector4 flagged = __vor(masked, maxflags);
	vout = flagged;
#elif __PS3
	vector float minscaled = (min - m_worldAabbMin.xyzw) * m_quantize.xyzw;
	vector float maxscaled = (max - m_worldAabbMin.xyzw) * m_quantize.xyzw;
	vector unsigned int minfixed = vec_ctu(minscaled, 0);
	vector unsigned int maxfixed = vec_ctu(maxscaled, 0);
	vector unsigned short packed = vec_vpkuwus(minfixed, maxfixed);
	vector unsigned short masked = vec_and(packed, vec_splat_u16(~1));
	vector unsigned short maxflags = vec_sld(vec_splat_u16(0), vec_splat_u16(1), 8);
	vector unsigned short flagged = vec_or(masked, maxflags);
	vout = (__vector4)flagged;
#else
	Vector3 clampedMin, clampedMax;
	clampedMin.Max(m_worldAabbMin, min);
	clampedMax.Max(m_worldAabbMin, max);
	clampedMin.Min(m_worldAabbMax, clampedMin);
	clampedMax.Min(m_worldAabbMax, clampedMax);
	Vector3 scaledMin = (clampedMin - m_worldAabbMin) * m_quantize;
	Vector3 scaledMax = (clampedMax - m_worldAabbMin) * m_quantize;
	u16* out = (u16*)&vout;
	out[0] = (unsigned short)((int)scaledMin.x & ~1);
	out[1] = (unsigned short)((int)scaledMin.y & ~1);
	out[2] = (unsigned short)((int)scaledMin.z & ~1);
	out[4] = (unsigned short)((int)scaledMax.x | 1);
	out[5] = (unsigned short)((int)scaledMax.y | 1);
	out[6] = (unsigned short)((int)scaledMax.z | 1);
#endif
}


void btAxisSweep3::addHandle(const Vector3& center,float radius, u16 pOwner)
{
	addHandle( center - Vector3(radius,radius,radius), center + Vector3(radius, radius, radius ), pOwner );
}

void btAxisSweep3::addHandle(const Vector3& aabbMin,const Vector3& aabbMax, u16 pOwner)
{
    PF_FUNC(SAPAddHandle);
	// quantize the bounds
	Vector3 vminmax;
	unsigned short* min = (u16*)&vminmax;
	unsigned short* max = min+4;

	QuantizeAndCheck(vminmax, aabbMin, aabbMax);

	// If any of these fail but the above assert didn't then that indicates something went wrong with the quantization.
	Assert(min[0] < max[0]);
	Assert(min[1] < max[1]);
	Assert(min[2] < max[2]);

	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

	// allocate a handle
    ++m_numHandles;
	Handle* pHandle = getHandle(pOwner);

	Assert(*pHandle == Handle::UNUSED);

	// compute current limit of edge arrays
	// Because m_numHandles has already been incremented at this point, we don't need to add one to take into account the sentinel object here.
	u16 limit = m_numHandles * 2;

	// insert new edges just inside the max boundary edge
	for (int axis = 2; axis >= 0; axis--)
	{
		// Make sure that the sentinel gets its edge moved.
		Assert(m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] == limit - 1);
		m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] += 2;

		// Copy the old max edge of the sentinel into the new max edge of the sentinel.
		m_pEdges[axis][limit + 1] = m_pEdges[axis][limit - 1];

		m_pEdges[axis][limit-1].m_pos = min[axis];
		m_pEdges[axis][limit-1].m_handle = pOwner;

		m_pEdges[axis][limit].m_pos = max[axis];
		m_pEdges[axis][limit].m_handle = pOwner;

		pHandle->m_minEdgeIndices[axis] = limit - 1;
		pHandle->m_maxEdgeIndices[axis] = limit;

		// Now that we've inserted the new edges at the upper end of the edge list let's sort them downward into the correct spots.  Don't bother updating
		//   the overlap list until we're sorting the final axis.
		const bool updateOverlaps = (axis == 0);
		if(updateOverlaps)
		{
			sortMinDownAddNewPairs(pHandle, pOwner, min[axis], axis, pHandle->m_minEdgeIndices[axis], max[axis]);
		}
		else
		{
			sortMinDownNoNewPairs(pHandle, pOwner, min[axis], axis, pHandle->m_minEdgeIndices[axis]);
		}
		sortMaxDown(pHandle, pOwner, max[axis], axis, pHandle->m_maxEdgeIndices[axis]);

		// At this point this axis should be consistent, so we can check it.
#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
		verifyAxis(axis);
#endif
	}

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}


class SAPEdgeSorterGT : public std::binary_function<btAxisSweep3::Edge, btAxisSweep3::Edge, bool>
{
public:
	bool operator()(const btAxisSweep3::Edge &left, const btAxisSweep3::Edge &right) const
	{
		return left > right;
	}
};

class SAPEdgeSorterLT : public std::binary_function<btAxisSweep3::Edge, btAxisSweep3::Edge, bool>
{
public:
	bool operator()(const btAxisSweep3::Edge &left, const btAxisSweep3::Edge &right) const
	{
		return left < right;
	}
};



void btAxisSweep3::addHandles( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandle )
{
	PF_FUNC(SAPAddHandle);
	// quantize the bounds

	Vector3 *aminmax = Alloca( Vector3, nHandle );

	u16 iHandle;
	for( iHandle = 0; iHandle < nHandle; iHandle++ )
	{
		QuantizeAndCheck(aminmax[iHandle], aabbMin[iHandle], aabbMax[iHandle]);
	}

	// sort the min and max 

	// allocate handles
//	Handle* apHandle = Alloca( Handle, nHandle );

	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

	// add one handle for sentinal
	u16 oldLastElement = ( m_numHandles + 1 ) * 2 - 1;

/*	for( iHandle = 0; iHandle < nHandle; iHandle++ )
	{		
		++m_numHandles;
		apHandle[ iHandle ] = getHandle(pOwner[ iHandle ]);
	}
*/

	m_numHandles = m_numHandles + nHandle;

	u16 newLastElement = oldLastElement + nHandle*2;

	Edge *addEdgesMin = Alloca( Edge, nHandle );
	Edge *addEdgesMax = Alloca( Edge, nHandle );

	// insert new edges just inside the max boundary edge
	for (int axis = 2; axis >= 0; axis--)
	{
		// Make sure that the sentinel gets its edge moved.
		m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] += 2*nHandle;

		// Copy the old max edge of the sentinel into the new max edge of the sentinel.
		m_pEdges[axis][newLastElement + 1] = m_pEdges[axis][oldLastElement];

		u16 newMinStart = oldLastElement + nHandle;
		u16 newMaxStart = oldLastElement;

		// since we are sorting the max down before the min we do our initial insertion inside out.
		for( iHandle = 0; iHandle < nHandle; iHandle++ )
		{
			u16 *min = (u16*)(aminmax + iHandle);
			u16 *max = min + 4;

//			m_pEdges[axis][newMinStart + iHandle].m_pos = min[axis];
//			m_pEdges[axis][newMinStart + iHandle].m_handle = pOwner[iHandle];

//			m_pEdges[axis][newMaxStart + iHandle].m_pos = max[axis];
//			m_pEdges[axis][newMaxStart + iHandle].m_handle = pOwner[iHandle];

			Handle *pHandle = getHandle( (u16)pOwner[ iHandle ] );

			//!me this is just overwritten, I think
			pHandle->m_minEdgeIndices[axis] = newMinStart + iHandle;
			pHandle->m_maxEdgeIndices[axis] = newMaxStart + iHandle;

			addEdgesMin[iHandle].m_pos = min[axis];
			addEdgesMin[iHandle].m_handle = (u16)pOwner[ iHandle ];

			addEdgesMax[iHandle].m_pos =  max[axis];
			addEdgesMax[iHandle].m_handle = (u16)pOwner[ iHandle ];
		}

		// sort the axis for efficient batch percolation
		std::sort(addEdgesMax, addEdgesMax + nHandle, SAPEdgeSorterGT());
		std::sort(addEdgesMin, addEdgesMin + nHandle, SAPEdgeSorterGT());

		// Now that we've inserted the new edges at the upper end of the edge list let's sort them downward into the correct spots.  Don't bother updating
		//   the overlap list until we're sorting the final axis.
		const bool updateOverlaps = (axis == 0);
		sortEdgesIn(axis, addEdgesMax, false, nHandle, oldLastElement);
		sortEdgesIn(axis, addEdgesMin, updateOverlaps, nHandle, oldLastElement + nHandle);

		// At this point this axis should be consistent, so we can check it.
#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
		verifyAxis(axis);
#endif
	}

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}


void btAxisSweep3::prepareCacheForTransfer()
{
	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

	m_pairCache->sort();

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}

void btAxisSweep3::getHandles( Vector3 **aabbMin, Vector3 **aabbMax, int **pOwner, u16 &nHandles ) const
{
	u16 iHandle;
	u16 iHandleOut;
	for( iHandle = 0, iHandleOut = 0; iHandle < m_maxHandles; iHandle++ )
	{
		if( m_pHandles[iHandle] != Handle::UNUSED )
		{
			phInst *inst = m_Level->GetInstance(iHandle);
			Assert( inst );

			Vec3V obbExtents, obbCenter;
			inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(obbExtents, obbCenter);
			Mat34V_ConstRef tempMat = inst->GetMatrix();
			const Vec3V aabbExtents = geomBoxes::ComputeAABBExtentsFromOBB(tempMat.GetMat33ConstRef(), obbExtents);
			const Vec3V aabbCenter = Transform(tempMat, obbCenter);

			const Vec3V maxs = aabbCenter + aabbExtents;
			const Vec3V mins = aabbCenter - aabbExtents;

			(*aabbMin)[iHandleOut] = RCC_VECTOR3(mins);
			(*aabbMax)[iHandleOut] = RCC_VECTOR3(maxs);
			(*pOwner)[iHandleOut] = iHandle;
			iHandleOut++;
		}
	}

	Assert( m_numHandles == iHandleOut );
	nHandles = iHandleOut;
}

u16 btAxisSweep3::getNumHandles() const
{ 
	return m_numHandles; 
}


void btAxisSweep3::addHandlesNoNewPairs( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandle )
{
	PF_FUNC(SAPAddHandle);

	// quantize the bounds

	Vector3 *aminmax = Alloca( Vector3, nHandle );

	u16 iHandle;
	for( iHandle = 0; iHandle < nHandle; iHandle++ )
	{
		QuantizeAndCheck(aminmax[iHandle], aabbMin[iHandle], aabbMax[iHandle]);
	}

	// allocate handles

	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

	// add one handle for sentinal
	u16 oldLastElement = ( m_numHandles + 1 ) * 2 - 1;

	m_numHandles = m_numHandles + nHandle;

	u16 newLastElement = oldLastElement + nHandle*2;

	Edge *addEdgesMinMax = Alloca( Edge, nHandle*2 );

	// insert new edges just inside the max boundary edge
	for (int axis = 0; axis < 3; axis++)
	{

		// Make sure that the sentinel gets its edge moved.
		m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] += 2*nHandle;

		// Copy the old max edge of the sentinel into the new max edge of the sentinel.
		m_pEdges[axis][newLastElement + 1] = m_pEdges[axis][oldLastElement];

		u16 newMinMaxStart = oldLastElement;

		int iOwnerHandle = 0;
		// unlike the addHandles regular version, we just add edges in a straightforwards sorted manner
		for( iHandle = 0; iHandle < nHandle*2; iHandle +=2, iOwnerHandle++ )
		{
			u16 *min = (u16*)(aminmax + iOwnerHandle);
			u16 *max = min + 4;

			//			m_pEdges[axis][newMinStart + iHandle].m_pos = min[axis];
			//			m_pEdges[axis][newMinStart + iHandle].m_handle = pOwner[iHandle];

			//			m_pEdges[axis][newMaxStart + iHandle].m_pos = max[axis];
			//			m_pEdges[axis][newMaxStart + iHandle].m_handle = pOwner[iHandle];

			Handle *pHandle = getHandle( (u16)pOwner[ iOwnerHandle ] );

			//!me this is just overwritten, I think.  So why pay the cache hit?
			pHandle->m_minEdgeIndices[axis] = newMinMaxStart + iHandle;
			pHandle->m_maxEdgeIndices[axis] = newMinMaxStart + iHandle + 1;

			addEdgesMinMax[iHandle].m_pos = min[axis];
			addEdgesMinMax[iHandle].m_handle = (u16)pOwner[ iOwnerHandle ];

			addEdgesMinMax[iHandle+1].m_pos =  max[axis];
			addEdgesMinMax[iHandle+1].m_handle = (u16)pOwner[ iOwnerHandle ];
		}

		// sort the axis for efficient batch percolation
		std::sort(addEdgesMinMax, addEdgesMinMax + nHandle*2, SAPEdgeSorterGT());

		// Now that we've inserted the new edges at the upper end of the edge list let's sort them downward into the correct spots.  Don't bother updating
		//   the overlap list until we're sorting the final axis.
		sortEdgesIn(axis, addEdgesMinMax, false, nHandle*2, oldLastElement);

		// At this point this axis should be consistent, so we can check it.
#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
		verifyAxis(axis);
#endif
	}

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}


bool btAxisSweep3::isHandleAdded(unsigned short handle)
{
	return *getHandle(handle) != btAxisSweep3::Handle::UNUSED;
}


void btAxisSweep3::removeHandle(unsigned short handle)
{
    PF_FUNC(SAPRemoveHandle);
	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
	// Make sure that nothing was messed up coming into here.
	verifyAllAxes();
#endif
	Assert( handle < m_maxHandles );
	Handle* pHandle = getHandle(handle);

	if (*pHandle == Handle::UNUSED)
	{
		AssertMsg(false, "Removing a handle that was delay added but not committed.");
		return;
	}

    // We don't actually remove the overlapping pairs at this point anymore, we instead depend on the
    // fact that we will do a testOverlap for this object during iteration.
//	removeOverlappingPairsContainingProxy(handle);


	// compute current limit of edge arrays
	int limit = m_numHandles * 2;
	int axis;

	// free the handle
	--m_numHandles;

	// remove the edges by sorting them up to the end of the list
	for ( axis = 0; axis < 3; axis++)
	{
		// Push the edges of the object we're removing to the positive edges of our space.
		Edge* pEdges = m_pEdges[axis];
		u16 max = pHandle->m_maxEdgeIndices[axis];
		pEdges[max].m_pos = 0xffff;

		// This used to get done after sortMaxUp() but I moved it here to better match the general pattern of setting the positions of both
		//   edges before calling the sort functions.
		u16 i = pHandle->m_minEdgeIndices[axis];
		pEdges[i].m_pos = 0xffff;

		sortMaxUpNoNewPairs(pHandle,handle,0xffff,axis,max);

		sortMinUp(pHandle,handle,0xffff,axis,i);

		// Aha, but the edges of the object getting removed are not actually at the end of the list yet; the sentinel edge is still there.  So we need to
		//   ensure that the sentinel still is validly placed.
		pEdges[limit-1].m_handle = m_maxHandles;
		pEdges[limit-1].m_pos = 0xffff;

		Assert(m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] == (limit + 1));
		m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] = (u16)(limit - 1);
	}

	// ensure testOverlap with anything else will fail during update
	*pHandle = Handle::UNUSED;

#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
	verifyAllAxes();
#endif

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}

void btAxisSweep3::removeHandles( int *handle, int nHandle)
{
	PF_FUNC(SAPRemoveHandle);
	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
	// Make sure that nothing was messed up coming into here.
	verifyAllAxes();
#endif

#if USE_EDGE_PREFETCH
	PrefetchObject( &m_pEdges[0] );
	PrefetchObject( &m_pEdges[1] );
	PrefetchObject( &m_pEdges[2] );
#endif

	u16 *edgeIndiciesToRemove = Alloca( u16, nHandle*2 + 1 );

	int limit = (m_numHandles + 1) * 2 - 1;
	int axis;
	for( axis = 0; axis < 3; axis++)
	{
		int iHandle;
		for( iHandle = 0; iHandle < nHandle; iHandle++ )
		{
			Assert( handle[iHandle] < m_maxHandles );
			Handle *pHandle = getHandle( (unsigned short) handle[iHandle] );
			edgeIndiciesToRemove[iHandle*2] = pHandle->m_minEdgeIndices[axis];
			edgeIndiciesToRemove[iHandle*2+1] = pHandle->m_maxEdgeIndices[axis];
		}

		// add sentinel to end of list for efficiency of algorithm
		edgeIndiciesToRemove[nHandle*2] = (u16) limit;

		std::sort( edgeIndiciesToRemove, edgeIndiciesToRemove + nHandle*2 );

		removeFromAxis( axis, edgeIndiciesToRemove, nHandle*2 );


		Edge *sentinel = (m_pEdges[axis] + limit);
		*(sentinel - nHandle*2) = *sentinel;
		m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] = u16(m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] - (nHandle*2));

	}

	// ensure testOverlap with anything else will fail during update
	int iHandle;
	for( iHandle = 0; iHandle < nHandle; iHandle++ )
	{
		Handle *pHandle = getHandle( (unsigned short) handle[iHandle] );
		*pHandle = Handle::UNUSED;
	}

	m_numHandles = u16(m_numHandles - nHandle);

#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
	verifyAllAxes();
#endif

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}

#if __DEV && __ASSERT
void btAxisSweep3::verifyAllAxes() const
{
	for(int axis = 0; axis < 3; ++axis)
	{
		verifyAxis(axis);
	}
}

void btAxisSweep3::verifyAxis(int axis, bool posAreCorrect) const
{
	Assert(m_pHandles[m_maxHandles].m_minEdgeIndices[axis] == 0);
	Assert(m_pHandles[m_maxHandles].m_maxEdgeIndices[axis] == (m_numHandles * 2 + 1));

	checkEdgeVsHandle(axis, 0);
	// Ensure that the edges are correctly sorted on this axis.
	for(int edgeIndex = 1; edgeIndex < (m_numHandles + 1) * 2; ++edgeIndex)
	{
		// Ensure that the sort order is correct.
		if(posAreCorrect)
			Assert(m_pEdges[axis][edgeIndex].m_pos >= m_pEdges[axis][edgeIndex - 1].m_pos);

		// Ensure that we're in sync with the handle.
		checkEdgeVsHandle(axis, edgeIndex);
	}
}

void btAxisSweep3::checkEdgeVsHandle(int axis, int edgeIndex) const
{
	// Make sure that the handle that these edges claim to belong to agrees with them.
	u16 levelIndex = m_pEdges[axis][edgeIndex].m_handle;
	Handle *curHandle = getHandle(levelIndex);
	Assert(curHandle != NULL);
	if(m_pEdges[axis][edgeIndex].IsMax())
	{
		Assert(curHandle->m_maxEdgeIndices[axis] == edgeIndex);
	}
	else
	{
		Assert(curHandle->m_minEdgeIndices[axis] == edgeIndex);
	}

	// Make sure that the edges are in the correct order.
	Assert(curHandle->m_minEdgeIndices[axis] < curHandle->m_maxEdgeIndices[axis]);
}
#endif


inline bool btAxisSweep3::TestSpatialOverlap(const Handle* pHandleA, const Handle* pHandleB) const
{
	// TODO: This doesn't need to be all branchy like this.  See TestQuantizedAabbAgainstAabb() in OptimizedBvh.h (which should be in a different header really).
	if (pHandleA->m_maxEdgeIndices[0] < pHandleB->m_minEdgeIndices[0] || pHandleB->m_maxEdgeIndices[0] < pHandleA->m_minEdgeIndices[0] ||
		pHandleA->m_maxEdgeIndices[1] < pHandleB->m_minEdgeIndices[1] || pHandleB->m_maxEdgeIndices[1] < pHandleA->m_minEdgeIndices[1] ||
		pHandleA->m_maxEdgeIndices[2] < pHandleB->m_minEdgeIndices[2] || pHandleB->m_maxEdgeIndices[2] < pHandleA->m_minEdgeIndices[2])
	{
		return false;
	}

	return true;
}

inline bool btAxisSweep3::TestSpatialOverlap(const int levelIndex0, const int levelIndex1) const
{
	const Handle* pHandleA = getHandle(levelIndex0);
	const Handle* pHandleB = getHandle(levelIndex1);

	return TestSpatialOverlap(pHandleA, pHandleB);

}

inline bool btAxisSweep3::TestObjectsAndStates(const int levelIndex0, const int generationId0, const int levelIndex1, const int generationId1) const
{
	const phLevelNew *physLevel = m_Level;
	if(!physLevel->IsLevelIndexGenerationIDCurrent(levelIndex0, generationId0) || !physLevel->IsLevelIndexGenerationIDCurrent(levelIndex1, generationId1))
	{
		return false;
	}

	return physLevel->ShouldCollideByState(levelIndex0, levelIndex1);
}

#if USE_GLOBAL_SAP_UPDATE
static int do_merge_sort = 0;
#endif

void btAxisSweep3::updateHandle(unsigned short handle, const Vector3& center,float fRadius )
{
	updateHandle( handle, center - Vector3( fRadius,fRadius, fRadius ), center + Vector3( fRadius,fRadius, fRadius) );
}

void btAxisSweep3::updateHandle(unsigned short handle, const Vector3& aabbMin,const Vector3& aabbMax)
{
	PF_FUNC(SAPUpdateHandle);
	PF_INCREMENT(SAPUpdateHandleNumCalls);

	// quantize the new bounds
	Vector3 vminmax;
	QuantizeAndCheck(vminmax, aabbMin, aabbMax);
	u16* min = (u16*)&vminmax;
	u16* max = min + 4;

	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

	Assert( handle < m_maxHandles );
	Handle* pHandle = getHandle(handle);

	Assert(!(*pHandle == Handle::UNUSED));

#if USE_EDGE_PREFETCH
	// In my timings these prefetches seemed to neither hurt nor help, but it seems like they are more of a 'shot in the dark' than an informed prefetch.  For worlds
	//   with a small number of objects these probably work well, but they seem more likely to do harm than good for large worlds.
//	PrefetchObject( &m_pEdges[0] );
//	PrefetchObject( &m_pEdges[1] );
//	PrefetchObject( &m_pEdges[2] );
#endif

#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
	Assert(min[0] <= max[0]);
	Assert(min[1] <= max[1]);
	Assert(min[2] <= max[2]);
#endif

	// update changed edges
	Edge **edges = m_pEdges;
	for (int axis = 0; axis < 3; axis++)
	{
		// This code relies on the edge indices of minimum edges being less than the edge indices of the maximum edges, that's why we keep FastAssert()'ing
		//   on that in here.
		const u32 emin = pHandle->m_minEdgeIndices[axis];
		const u32 emax = pHandle->m_maxEdgeIndices[axis];
		FastAssert(emin < emax);

		int dmin = (int)min[axis] - (int)edges[axis][emin].m_pos;
		int dmax = (int)max[axis] - (int)edges[axis][emax].m_pos;

		edges[axis][emin].m_pos = min[axis];
		edges[axis][emax].m_pos = max[axis];

#if USE_GLOBAL_SAP_UPDATE
		if (do_merge_sort)
			continue;
#endif

		// expand (only adds overlaps)
		if (dmin < 0)
		{
			sortMinDownAddNewPairs(pHandle, handle, min[axis], axis, emin, max[axis]);
		}

		if (dmax > 0)
		{
			sortMaxUpAddNewPairs(pHandle, handle, max[axis], axis, emax, min[axis]);
		}

		// shrink (only removes overlaps)
		if (dmax < 0)
		{
			sortMaxDown(pHandle, handle, max[axis], axis, emax);
		}

		if (dmin > 0)
		{
			sortMinUp(pHandle, handle, min[axis], axis, emin);
		}
	}

#if AXISSWEEP_EXTRA_CONSISTENCY_CHECKS
	verifyAllAxes();
#endif

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}	

btBroadphasePair* btAxisSweep3::addOverlappingPair(u16 object0, u16 object1)
{
	if (m_pairCache->m_numInCache < m_pairCache->m_maxPairs)
	{
		return addOverlappingPairInternal( object0, object1 ); 
	}
	else
	{
		return NULL;
	}
}

btBroadphasePair* btAxisSweep3::addOverlappingPairInternal(u16 object0, u16 object1)
{
	if(m_Level->ShouldCollideByState(object0, object1) && m_KeepPairFunc(object0,object1))
	{
		return m_pairCache->addOverlappingPair( object0, object1 ); 
	}
	return NULL;
}

__forceinline bool TestQuantizedAabbAgainstAabb2d(u32 min00, u32 min01, u32 min10, u32 min11, u32 max00, u32 max01, u32 max10, u32 max11)
{
	int d0 = max10 - min00;
	int d1 = max11 - min01;
	int d2 = max00 - min10;
	int d3 = max01 - min11;

	// If any of the above values are negative then we want to return false.
	int combinedValue = (d0 | d1 | d2 | d3);
	const bool aabbOverlap = (combinedValue > 0);
	return aabbOverlap;
}

__forceinline bool TestQuantizedAabbAgainstAabb25d(u32 min01, u32 min02, u32 max00, u32 max01, u32 max02, u32 min10, u32 min11, u32 min12, u32 max11, u32 max12)
{
	int d0 = max00 - min10;
	int d1 = max01 - min11;
	int d2 = max11 - min01;
	int d3 = max02 - min12;
	int d4 = max12 - min02;

	// If any of the above values are negative then we want to return false.
	int combinedValue = (d0 | d1 | d2 | d3 | d4);
	const bool aabbOverlap = (combinedValue > 0);
	return aabbOverlap;
}

#if USE_GLOBAL_SAP_UPDATE
void btAxisSweep3::MergeLists(Edge * a, Edge * c, Edge * b)
{
	// merge sorted lists [a..c-1] and [c..b]
	Edge * prev_c = c - 1;
	Assert(c > a);
	Assert(c <= b);
	while ((*c) < (*prev_c)) 
	{ 
		//Assert(c <= g_end);
		Edge * const save_c = c;
		Edge const save_c_val = *c;

		const u32 c_is_max = save_c_val.IsMax();
		Handle * c_handle = &m_pHandles[save_c_val.m_handle];
		u16 * const c_index = &c_handle->m_minEdgeIndices[m_axis + c_is_max * 3];

		if (c_is_max)
		{
			do 
			{ 
				// swap save_c with prev_c.
				{
					const u32 prev_c_is_max = prev_c->IsMax();
					Handle * prev_c_handle = &m_pHandles[prev_c->m_handle];
					u16 * const prev_c_index = &prev_c_handle->m_minEdgeIndices[m_axis + prev_c_is_max * 3];
					(*prev_c_index) = (*c_index);
					(*c_index)--;
				}
				*c = *prev_c; 
				c = prev_c; 
				prev_c--; 
			} while ((prev_c >= a) && (save_c_val < (*prev_c))); 
		}
		else
		{
			const u32 movingEdgeMinIndex1 = c_handle->m_minEdgeIndices[m_axis1];
			const u32 movingEdgeMaxIndex1 = c_handle->m_maxEdgeIndices[m_axis1];
			const u32 movingEdgeMinIndex2 = c_handle->m_minEdgeIndices[m_axis2];
			const u32 movingEdgeMaxIndex2 = c_handle->m_maxEdgeIndices[m_axis2];
			const u16 finalMaxPos = m_edgelist[c_handle->m_maxEdgeIndices[m_axis]].m_pos;
			do 
			{ 
				// swap save_c with prev_c.
				{
					const u32 prev_c_is_max = prev_c->IsMax();
					Handle * prev_c_handle = &m_pHandles[prev_c->m_handle];
					u16 * const prev_c_index = &prev_c_handle->m_minEdgeIndices[m_axis + prev_c_is_max * 3];
					(*prev_c_index) = (*c_index);
					(*c_index)--;

					if (/*!c_is_max && */prev_c_is_max)
					{
						const u32 prevEdgeMinIndex1 = prev_c_handle->m_minEdgeIndices[m_axis1];
						const u32 prevEdgeMinIndex2 = prev_c_handle->m_minEdgeIndices[m_axis2];
						const u32 prevEdgeMaxIndex1 = prev_c_handle->m_maxEdgeIndices[m_axis1];
						const u32 prevEdgeMaxIndex2 = prev_c_handle->m_maxEdgeIndices[m_axis2];
						if(TestQuantizedAabbAgainstAabb2d(movingEdgeMinIndex1, movingEdgeMinIndex2, prevEdgeMinIndex1, prevEdgeMinIndex2, movingEdgeMaxIndex1, movingEdgeMaxIndex2, prevEdgeMaxIndex1, prevEdgeMaxIndex2))
						{
							// We know that we're overlapping on the other two axes and our min just crossed over the max of somebody else.  Before we declare it
							//   to be a pair we want to check the other object's min edge against our soon-to-be max edge.
							const int prevEdgeMinIndex = prev_c_handle->m_minEdgeIndices[m_axis];
							const Edge * prevEdgeMin = &m_edgelist[prevEdgeMinIndex];
							if(finalMaxPos >= prevEdgeMin->m_pos)
							{
								addOverlappingPairInternal(save_c_val.m_handle, prev_c->m_handle);
							}
						}
					}
				}

				*c = *prev_c; 
				c = prev_c; 
				prev_c--; 
			} while ((prev_c >= a) && (save_c_val < (*prev_c))); 
		}
		*c = save_c_val; 
		prev_c = save_c; 
		c = prev_c + 1; 
		if (c > b)
			break;
	}
}

void btAxisSweep3::SortList(int start_i, int end_i)
{
	int count = end_i - start_i + 1;
	int size = 1;
	int next_size = 2;
	Edge * start_p = m_edgelist + start_i;
	Edge * end_p = m_edgelist + end_i;
	while (size < count)
	{
		Edge * a = start_p;
		Edge * c = a + size;
		Edge * b = c + (size - 1);
		while (b <= end_p)
		{
			MergeLists(a,c,b);
			a += next_size;
			c += next_size;
			b += next_size;
		}
		Assert(a <= end_p + 1);
		if (a <= end_p)
		{
			c = a;
			a -= next_size;
			b = end_p;
			MergeLists(a,c,b);
		}
		size = next_size;
		next_size *= 2;
	}
}

void VerifySort(btAxisSweep3::Edge * a, btAxisSweep3::Edge * b)
{
	for (btAxisSweep3::Edge * c = a ; c < b ; c++)
		Assert(c->m_pos <= (c+1)->m_pos);
}

void btAxisSweep3::DoSort()
{
	PF_FUNC(SAPDoSort);
	PF_INCREMENT(SAPDoSortNumCalls);

	if (!do_merge_sort)
	{
		return;
	}

	const u16 start_i = 1;
	const u16 end_i = ((u16)m_numHandles+1) * 2 - 1 - 1;

	m_axis = 0;
	m_axis1 = 1;
	m_axis2 = 2;
	m_edgelist = m_pEdges[0];
	SortList(start_i,end_i);
//	VerifySort(m_pEdges[0] + start_i - 1,m_pEdges[0] + end_i + 1);

	m_axis = 1;
	m_axis1 = 0;
	m_axis2 = 2;
	m_edgelist = m_pEdges[1];
	SortList(start_i,end_i);
//	VerifySort(m_pEdges[1] + start_i - 1,m_pEdges[1] + end_i + 1);

	m_axis = 2;
	m_axis1 = 0;
	m_axis2 = 1;
	m_edgelist = m_pEdges[2];
	SortList(start_i,end_i);
//	VerifySort(m_pEdges[2] + start_i - 1,m_pEdges[2] + end_i + 1);
}
#endif // USE_GLOBAL_SAP_UPDATE

// sorting a min edge downwards can only ever *add* overlaps
void btAxisSweep3::sortMinDownAddNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex, u32 finalMaxPos)
{
	Edge * RESTRICT pEdges = m_pEdges[axis];
	Handle * RESTRICT pHandles = m_pHandles;

	// prevEdge is always the edge that we're trying to pass.
	FastAssert(edgeIndex <= (u32)m_maxHandles * 2);
	Edge * RESTRICT pPrevEdge = &pEdges[edgeIndex] - 1;
	PrefetchObject(pPrevEdge - 1);		// We know we're moving downward through the edge list so let's grab the previous one.
	u32 prevEdgePos = pPrevEdge->m_pos;
	u32 movingEdgeMinIndex0 = edgeIndex;

	if(movingEdgePos < prevEdgePos)
	{
		const u32 axis1 = IIncrementSaturateAndWrap(axis, 3);
		const u32 axis2 = 3 - axis - axis1;
		const u32 movingEdgeMinIndex1 = movingHandle->m_minEdgeIndices[axis1];
		const u32 movingEdgeMaxIndex1 = movingHandle->m_maxEdgeIndices[axis1];
		const u32 movingEdgeMinIndex2 = movingHandle->m_minEdgeIndices[axis2];
		const u32 movingEdgeMaxIndex2 = movingHandle->m_maxEdgeIndices[axis2];

		do
		{
			const u32 prevHandleIndex = pPrevEdge->m_handle;
			Handle *prevHandle = &pHandles[prevHandleIndex];

			const u32 prevEdgeIsMax = pPrevEdge->IsMax();
			FastAssert(prevHandleIndex != movingHandleIndex);
			if(prevEdgeIsMax)// && prevHandleIndex != movingHandleIndex)
			{
				const u32 prevEdgeMinIndex1 = prevHandle->m_minEdgeIndices[axis1];
				const u32 prevEdgeMinIndex2 = prevHandle->m_minEdgeIndices[axis2];
				const u32 prevEdgeMaxIndex1 = prevHandle->m_maxEdgeIndices[axis1];
				const u32 prevEdgeMaxIndex2 = prevHandle->m_maxEdgeIndices[axis2];
				if(TestQuantizedAabbAgainstAabb2d(movingEdgeMinIndex1, movingEdgeMinIndex2, prevEdgeMinIndex1, prevEdgeMinIndex2, movingEdgeMaxIndex1, movingEdgeMaxIndex2, prevEdgeMaxIndex1, prevEdgeMaxIndex2))
				{
					// We know that we're overlapping on the other two axes and our min just crossed over the max of somebody else.  Before we declare it
					//   to be a pair we want to check the other object's min edge against our soon-to-be max edge.
					const int prevEdgeMinIndex = prevHandle->m_minEdgeIndices[axis];
					const Edge *prevEdgeMin = &pEdges[prevEdgeMinIndex];
					if(finalMaxPos >= prevEdgeMin->m_pos)
					{
						addOverlappingPairInternal((u16)movingHandleIndex, (u16)prevHandleIndex);
					}
				}
			}

			// Update the edge index of the previous edge's handle to let it know its edge has moved.
			u16 *edgeIndices = prevHandle->m_minEdgeIndices;
			const u32 offset = prevEdgeIsMax * 3 + axis;
			FastAssert(edgeIndices[offset] == movingEdgeMinIndex0 - 1);
			edgeIndices[offset] = (u16)movingEdgeMinIndex0;

			*(pPrevEdge + 1) = *pPrevEdge;

			--pPrevEdge;
			PrefetchObject(pPrevEdge - 1);
			prevEdgePos = pPrevEdge->m_pos;

			--movingEdgeMinIndex0;
		}
		while(movingEdgePos < prevEdgePos);

		(pPrevEdge + 1)->m_pos = (u16)movingEdgePos;
		(pPrevEdge + 1)->m_handle = (u16)movingHandleIndex;
		movingHandle->m_minEdgeIndices[axis] = (u16)(movingEdgeMinIndex0);
	}

	FastAssert((pPrevEdge + 1)->m_pos == (u16)movingEdgePos);
	FastAssert((pPrevEdge + 1)->m_handle == (u16)movingHandleIndex);
	FastAssert(movingHandle->m_minEdgeIndices[axis] == (u16)(movingEdgeMinIndex0));
}


void btAxisSweep3::sortMinDownNoNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex)
{
	Edge * RESTRICT pEdges = m_pEdges[axis];
	Handle * RESTRICT pHandles = m_pHandles;

	// pPrevEdge is always the edge that we're trying to pass.
	FastAssert(edgeIndex <= (u32)m_maxHandles * 2);
	Edge * RESTRICT pPrevEdge = &pEdges[edgeIndex] - 1;
	PrefetchObject(pPrevEdge - 1);		// We know we're moving downward through the edge list so let's grab the previous one.
	u32 prevEdgePos = pPrevEdge->m_pos;
	u32 movingEdgeMinIndex0 = edgeIndex;

	if(movingEdgePos < prevEdgePos)
	{
		do
		{
			const u32 prevHandleIndex = pPrevEdge->m_handle;
			Handle *prevHandle = &pHandles[prevHandleIndex];

			const u32 prevEdgeIsMax = pPrevEdge->IsMax();

			// Update the edge index of the previous edge's handle to let it know its edge has moved.
			u16 *edgeIndices = prevHandle->m_minEdgeIndices;
			const u32 offset = prevEdgeIsMax * 3 + axis;
			FastAssert(edgeIndices[offset] == movingEdgeMinIndex0 - 1);
			edgeIndices[offset] = (u16)movingEdgeMinIndex0;

			*(pPrevEdge + 1) = *pPrevEdge;

			--pPrevEdge;
			prevEdgePos = pPrevEdge->m_pos;

			--movingEdgeMinIndex0;
		}
		while(movingEdgePos < prevEdgePos);

		(pPrevEdge + 1)->m_pos = (u16)movingEdgePos;
		(pPrevEdge + 1)->m_handle = (u16)movingHandleIndex;
		movingHandle->m_minEdgeIndices[axis] = (u16)(movingEdgeMinIndex0);
	}

	FastAssert((pPrevEdge + 1)->m_pos == (u16)movingEdgePos);
	FastAssert((pPrevEdge + 1)->m_handle == (u16)movingHandleIndex);
	FastAssert(movingHandle->m_minEdgeIndices[axis] == (u16)(movingEdgeMinIndex0));
}

/*
template < class _Type >
class CircularQueueLocalStorePowerOf2
{
public:
	CircularQueueLocalStorePowerOf2( int nSize )
	{
		Assert( nSize > 0 );
		// bump up to smallest power of two > nSize
		int nPowerOfTwo = 0;
		while( (nSize >> (31-nPowerOfTwo)) == 0 )
		{
			nPowerOfTwo++;
		}

		nSize = 1 << nPowerOfTwo;
		m_wrapMask = 0;
		int iWrap = 0;
		while( iWrap < nPowerOfTwo )
		{
			m_wrapMask += (1 << iWrap );
			iWrap++;
		}
		m_size = nSize;
		m_elements = Alloca( _Type, nSize );
		m_A = 0;
		m_B = 1;  // always one past actual last element
	}

	void Add( _Type e )
	{
		m_elements[m_B & m_wrapMask] = e;
		Assert( m_B != m_A );  // 
		m_B++;
		Assert( m_B != m_A );  // 
	}

	_Type Remove()
	{
		const int aw = m_A & m_wrapMask;
		Assert( m_B != m_A );  // 
		m_A++;
		Assert( m_B != m_A );  // 
		return m_elements[aw];
	}

	_Type Peek() const
	{
		const int aw = m_A & m_wrapMask;
		return m_elements[aw];
	}

	bool IsEmpty() const
	{
		return ( (m_A + 1) == m_B );
	}


	_Type m_elements;
	int m_A;
	int m_B;
	int m_size;
	int m_wrapMask;

};
*/

// sorting a min edge upwards can only ever *remove* overlaps
void btAxisSweep3::sortMinUp(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex)
{
	Edge * RESTRICT pEdges = m_pEdges[axis];
	Handle * RESTRICT pHandles = m_pHandles;

	// prevEdge is always the edge that we're trying to pass.
	FastAssert(edgeIndex <= (u32)m_maxHandles * 2);
	Edge * RESTRICT pNextEdge = &pEdges[edgeIndex] + 1;
	PrefetchObject(pNextEdge + 1);		// We know we're moving upward through the edge list so let's grab the next one.
	u32 nextEdgePos = pNextEdge->m_pos;
	u32 movingEdgeMinIndex0 = edgeIndex;

	if(movingEdgePos > nextEdgePos)
	{
		do
		{
			const u32 nextHandleIndex = pNextEdge->m_handle;
			Handle *nextHandle = &pHandles[nextHandleIndex];

			const u32 nextEdgeIsMax = pNextEdge->IsMax();

			// Update the edge index of the next edge's handle to let it know its edge has moved.
			u16 *edgeIndices = nextHandle->m_minEdgeIndices;
			const u32 offset = nextEdgeIsMax * 3 + axis;
			FastAssert(edgeIndices[offset] == movingEdgeMinIndex0 + 1);
			edgeIndices[offset] = (u16)movingEdgeMinIndex0;

			*(pNextEdge - 1) = *pNextEdge;

			++pNextEdge;
			nextEdgePos = pNextEdge->m_pos;

			++movingEdgeMinIndex0;
		}
		while(movingEdgePos > nextEdgePos);

		(pNextEdge - 1)->m_pos = (u16)movingEdgePos;
		(pNextEdge - 1)->m_handle = (u16)movingHandleIndex;
		movingHandle->m_minEdgeIndices[axis] = (u16)(movingEdgeMinIndex0);
	}

	FastAssert((pNextEdge - 1)->m_pos == (u16)movingEdgePos);
	FastAssert((pNextEdge - 1)->m_handle == (u16)movingHandleIndex);
	FastAssert(movingHandle->m_minEdgeIndices[axis] == (u16)(movingEdgeMinIndex0));
}

// sorting a max edge downwards can only ever *remove* overlaps
void btAxisSweep3::sortMaxDown(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex)
{
	Edge * RESTRICT pEdges = m_pEdges[axis];
	Handle * RESTRICT pHandles = m_pHandles;

	// prevEdge is always the edge that we're trying to pass.
	FastAssert(edgeIndex <= (u32)m_maxHandles * 2);
	Edge * RESTRICT pPrevEdge = &pEdges[edgeIndex] - 1;
	PrefetchObject(pPrevEdge - 1);		// We know we're moving downward through the edge list so let's grab the previous one.
	u32 prevEdgePos = pPrevEdge->m_pos;
	u32 movingEdgeMaxIndex0 = edgeIndex;

	if(movingEdgePos < prevEdgePos)
	{
		do
		{
			const u32 prevHandleIndex = pPrevEdge->m_handle;
			Handle *prevHandle = &pHandles[prevHandleIndex];

			const u32 prevEdgeIsMax = pPrevEdge->IsMax();

			// Update the edge index of the previous edge's handle to let it know its edge has moved.
			u16 *edgeIndices = prevHandle->m_minEdgeIndices;
			const u32 offset = prevEdgeIsMax * 3 + axis;
			FastAssert(edgeIndices[offset] == movingEdgeMaxIndex0 - 1);
			edgeIndices[offset] = (u16)movingEdgeMaxIndex0;

			*(pPrevEdge + 1) = *pPrevEdge;

			--pPrevEdge;
			prevEdgePos = pPrevEdge->m_pos;

			--movingEdgeMaxIndex0;
		}
		while(movingEdgePos < prevEdgePos);

		(pPrevEdge + 1)->m_pos = (u16)movingEdgePos;
		(pPrevEdge + 1)->m_handle = (u16)movingHandleIndex;
		movingHandle->m_maxEdgeIndices[axis] = (u16)(movingEdgeMaxIndex0);
	}

	FastAssert((pPrevEdge + 1)->m_pos == (u16)movingEdgePos);
	FastAssert((pPrevEdge + 1)->m_handle == (u16)movingHandleIndex);
	FastAssert(movingHandle->m_maxEdgeIndices[axis] == (u16)(movingEdgeMaxIndex0));
}

void btAxisSweep3::sortEdgesIn(int axis, Edge *addEdges, bool updateOverlaps, u16 nAddEdge, u16 currentEnd)
{
	u32 iAdd = 0;
	u32 iCue = currentEnd;
	const u32 nEnd = currentEnd + nAddEdge;
	u32 iBump = nEnd;

	while( iAdd < nAddEdge )
	{
		Edge *edge = m_pEdges[axis];
		// AssertMsg( iAdd > 0 || edge[iAdd].m_handle != edge[iAdd-1].m_handle , "phInst added twice?  Are you using delayedSAP adding incorrectly?" ); 

		// sort down, shifting to bump postion as we go
		// iBump > 1 guards from shifting past the sentinel

		unsigned short addEdgesPos = addEdges[iAdd].m_pos;

		while( addEdgesPos <= edge[iCue].m_pos && iBump > 1 && iCue > 0  )
		{
			int cueHandle = edge[iCue].m_handle;
			int cuePos = edge[iCue].m_pos;
			edge[iBump].m_handle = (u16)cueHandle;
			edge[iBump].m_pos = (u16)cuePos;
			Handle *handle = getHandle(cueHandle);

			cuePos &= 1;	// 1 indicates "max" edge, 0 indicates "min" edge.
			// The m_maxEdgeIndices is right behind m_minEdgeIndices. Actually, let's confirm that.
			CompileTimeAssert(OffsetOf(Handle, m_maxEdgeIndices) - OffsetOf(Handle, m_minEdgeIndices) == sizeof(u16) * 3);
			cuePos *= 3;

			handle->m_minEdgeIndices[axis + cuePos] = (u16)iBump;

			iBump--;
			iCue--;
		}

		// put our element in bump position
		edge[iBump] = addEdges[iAdd];
		u32 handleId = addEdges[iAdd].m_handle;
		Handle *handle = getHandle(handleId);

		if( addEdges[iAdd].IsMax() )
		{
			handle->m_maxEdgeIndices[axis] = (u16)iBump;

			Assert(!updateOverlaps);
		}
		else
		{
			handle->m_minEdgeIndices[axis] = (u16)iBump;

			// check all pairs in bumped elements
			if( updateOverlaps && !m_Level->IsNonexistent(handleId))
			{
				int aindex = handleId;
				Assert(axis == 0);
				u32 amin1 = handle->m_minEdgeIndices[1];
				u32 amin2 = handle->m_minEdgeIndices[2];
				u32 amax0 = handle->m_maxEdgeIndices[0];
				u32 amax1 = handle->m_maxEdgeIndices[1];
				u32 amax2 = handle->m_maxEdgeIndices[2];

				for( u32 iCheck = iBump + 1; iCheck < nEnd; iCheck++ )
				{
					Edge &toCheck = edge[iCheck];
					int bindex = toCheck.m_handle;

					// if previous edge is a maximum check the bounds and add an overlap if necessary
					if (toCheck.IsMax() && (aindex != bindex))
					{
						Handle* handleb = getHandle(bindex);
						if (TestQuantizedAabbAgainstAabb25d(amin1, amin2, amax0, amax1, amax2, 
							handleb->m_minEdgeIndices[0], handleb->m_minEdgeIndices[1], handleb->m_minEdgeIndices[2],
							handleb->m_maxEdgeIndices[1], handleb->m_maxEdgeIndices[2]))
						{
							addOverlappingPairInternal((u16)aindex, (u16)bindex);
						}
					}
				}
			}
		}

		iAdd++;
		Assert(iBump > 0);
		iBump--;
	}
}

void btAxisSweep3::removeFromAxis( int axis, u16 *toRemove, int nRemove )
{
	Assert( nRemove > 1 );
	Assert( (m_pEdges[axis] + toRemove[nRemove])->m_handle == m_maxHandles );

	Edge* edgeTarget = m_pEdges[axis] + toRemove[0];

	int iRemove;
	for( iRemove = 0; iRemove < nRemove; iRemove++ )
	{
		int iA = toRemove[iRemove] + 1;
		int iB = toRemove[iRemove+1];
		
		Edge* edgeA = m_pEdges[axis] + iA;
		Edge* edgeB = m_pEdges[axis] + iB;
	
		while( edgeA < edgeB )
		{
			Handle* pHandleA = getHandle(edgeA->m_handle);
			
			if( edgeA->IsMax() )
			{
				pHandleA->m_maxEdgeIndices[axis] = u16(pHandleA->m_maxEdgeIndices[axis] - (iRemove+1));
			}
			else
			{
				pHandleA->m_minEdgeIndices[axis] = u16(pHandleA->m_minEdgeIndices[axis] - (iRemove+1));
			}

			*edgeTarget = *edgeA;
			edgeA++;
			edgeTarget++;
		}

	}
}

// sorting a max edge upwards can only ever *add* overlaps
void btAxisSweep3::sortMaxUpAddNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex, u32 finalMinPos)
{
	Edge * RESTRICT pEdges = m_pEdges[axis];
	Handle * RESTRICT pHandles = m_pHandles;

	// pNextEdge is always the edge that we're trying to pass.
	FastAssert(edgeIndex <= (u32)m_maxHandles * 2);
	Edge * RESTRICT pNextEdge = &pEdges[edgeIndex] + 1;
	PrefetchObject(pNextEdge + 1);		// We know we're moving upward through the edge list so let's grab the next one.
	u32 nextEdgePos = pNextEdge->m_pos;
	u32 movingEdgeMaxIndex0 = edgeIndex;

	if(movingEdgePos > nextEdgePos)
	{
		const u32 axis1 = IIncrementSaturateAndWrap(axis, 3);
		const u32 axis2 = 3 - axis - axis1;
		const u32 movingEdgeMinIndex1 = movingHandle->m_minEdgeIndices[axis1];
		const u32 movingEdgeMaxIndex1 = movingHandle->m_maxEdgeIndices[axis1];
		const u32 movingEdgeMinIndex2 = movingHandle->m_minEdgeIndices[axis2];
		const u32 movingEdgeMaxIndex2 = movingHandle->m_maxEdgeIndices[axis2];

		do
		{
			const u32 nextHandleIndex = pNextEdge->m_handle;
			Handle *nextHandle = &pHandles[nextHandleIndex];

			const u32 nextEdgeIsMax = pNextEdge->IsMax();
			FastAssert(nextHandleIndex != movingHandleIndex);
			if(nextEdgeIsMax == 0)
			{
				const u32 nextEdgeMinIndex1 = nextHandle->m_minEdgeIndices[axis1];
				const u32 nextEdgeMinIndex2 = nextHandle->m_minEdgeIndices[axis2];
				const u32 nextEdgeMaxIndex1 = nextHandle->m_maxEdgeIndices[axis1];
				const u32 nextEdgeMaxIndex2 = nextHandle->m_maxEdgeIndices[axis2];
				if(TestQuantizedAabbAgainstAabb2d(movingEdgeMinIndex1, movingEdgeMinIndex2, nextEdgeMinIndex1, nextEdgeMinIndex2, movingEdgeMaxIndex1, movingEdgeMaxIndex2, nextEdgeMaxIndex1, nextEdgeMaxIndex2))
				{
					// We know that we're overlapping on the other two axes and our min just crossed over the max of somebody else.  Before we declare it
					//   to be a pair we want to check the other object's min edge against our soon-to-be max edge.
					const int nextEdgeMaxIndex = nextHandle->m_maxEdgeIndices[axis];
					const Edge *nextEdgeMax = &pEdges[nextEdgeMaxIndex];
					if(finalMinPos <= nextEdgeMax->m_pos)
					{
						addOverlappingPairInternal((u16)movingHandleIndex, (u16)nextHandleIndex);
					}
				}
			}

			// Update the edge index of the next edge's handle to let it know its edge has moved.
			u16 *edgeIndices = nextHandle->m_minEdgeIndices;
			const u32 offset = nextEdgeIsMax * 3 + axis;
			FastAssert(edgeIndices[offset] == movingEdgeMaxIndex0 + 1);
			edgeIndices[offset] = (u16)movingEdgeMaxIndex0;

			*(pNextEdge - 1) = *pNextEdge;

			++pNextEdge;
			PrefetchObject(pNextEdge + 1);
			nextEdgePos = pNextEdge->m_pos;

			++movingEdgeMaxIndex0;
		}
		while(movingEdgePos > nextEdgePos);

		(pNextEdge - 1)->m_pos = (u16)movingEdgePos;
		(pNextEdge - 1)->m_handle = (u16)movingHandleIndex;
		movingHandle->m_maxEdgeIndices[axis] = (u16)(movingEdgeMaxIndex0);
	}

	FastAssert((pNextEdge - 1)->m_pos == (u16)movingEdgePos);
	FastAssert((pNextEdge - 1)->m_handle == (u16)movingHandleIndex);
	FastAssert(movingHandle->m_maxEdgeIndices[axis] == (u16)(movingEdgeMaxIndex0));
}


void btAxisSweep3::sortMaxUpNoNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex)
{
	Edge * RESTRICT pEdges = m_pEdges[axis];
	Handle * RESTRICT pHandles = m_pHandles;

	// pNextEdge is always the edge that we're trying to pass.
	FastAssert(edgeIndex <= (u32)m_maxHandles * 2);
	Edge * RESTRICT pNextEdge = &pEdges[edgeIndex] + 1;
	PrefetchObject(pNextEdge + 1);		// We know we're moving upward through the edge list so let's grab the next one.
	u32 nextEdgePos = pNextEdge->m_pos;
	u32 movingEdgeMaxIndex0 = edgeIndex;

	if(movingEdgePos > nextEdgePos)
	{
		do
		{
			const u32 nextHandleIndex = pNextEdge->m_handle;
			Handle *nextHandle = &pHandles[nextHandleIndex];

			const u32 nextEdgeIsMax = pNextEdge->IsMax();

			// Update the edge index of the next edge's handle to let it know its edge has moved.
			u16 *edgeIndices = nextHandle->m_minEdgeIndices;
			const u32 offset = nextEdgeIsMax * 3 + axis;
			FastAssert(edgeIndices[offset] == movingEdgeMaxIndex0 + 1);
			edgeIndices[offset] = (u16)movingEdgeMaxIndex0;

			*(pNextEdge - 1) = *pNextEdge;

			++pNextEdge;
			PrefetchObject(pNextEdge + 1);
			nextEdgePos = pNextEdge->m_pos;

			++movingEdgeMaxIndex0;
		}
		while(movingEdgePos > nextEdgePos);

		(pNextEdge - 1)->m_pos = (u16)movingEdgePos;
		(pNextEdge - 1)->m_handle = (u16)movingHandleIndex;
		movingHandle->m_maxEdgeIndices[axis] = (u16)(movingEdgeMaxIndex0);
	}

	FastAssert((pNextEdge - 1)->m_pos == (u16)movingEdgePos);
	FastAssert((pNextEdge - 1)->m_handle == (u16)movingHandleIndex);
	FastAssert(movingHandle->m_maxEdgeIndices[axis] == (u16)(movingEdgeMaxIndex0));
}

#if USE_STATIC_BVH_REJECTION_DEBUG
extern void ValidateStaticCache();
#endif // USE_STATIC_BVH_REJECTION_DEBUG

__forceinline bool btAxisSweep3::KeepPair(const btBroadphasePair & pair)
{
	const u16 levelIndex0 = pair.GetObject0();
	const u16 generationId0 = pair.GetGenId0();
	const u16 levelIndex1 = pair.GetObject1();
	const u16 generationId1 = pair.GetGenId1();
	return TestObjectsAndStates(levelIndex0, generationId0, levelIndex1, generationId1) && TestSpatialOverlap(levelIndex0, levelIndex1);
}

int FindNextGTE(const btBroadphasePair & cmp, btBroadphasePair * pairs, const int minIndex, int maxIndex)
{
	FastAssert(minIndex >= 0);
	FastAssert(minIndex <= maxIndex);
	int a = minIndex;
	int b = maxIndex;
	while (a < b)
	{
		const int c = (a + b) / 2;
		FastAssert(c >= minIndex);
		FastAssert(c < maxIndex);
		if (pairs[c] < cmp)
			a = c + 1;
		else
			b = c;
	}
	FastAssert(a == b);
	FastAssert(a >= minIndex && a <= maxIndex);
	FastAssert(a == maxIndex || cmp < pairs[a] || cmp == pairs[a]);
	FastAssert(a == 0 || pairs[a-1] < cmp);
	return a;
}

#if __DEV
void VerifySort(const btBroadphasePair * pairs, const int minIndex, const int maxIndex)
{
	for (int i = minIndex ; i < maxIndex - 1 ; i++)
		FastAssert(pairs[i] < pairs[i+1]);
}
#endif // __DEV

void btAxisSweep3::PruneNewOverlappingPairs(const int nextStartingBroadphasePair)
{
	FastAssert(nextStartingBroadphasePair >= 0);
	FastAssert((u32)nextStartingBroadphasePair <= m_pairCache->m_numInCache);
	FastAssert(m_NumSorted <= m_pairCache->m_numInCache);
	if (m_NumSorted == m_pairCache->m_numInCache)
	{
		// We don't have any new pairs.
		return;
	}
	if ((u32)nextStartingBroadphasePair == m_pairCache->m_numInCache)
	{
		// We don't have any new pairs.
		return;
	}
	FastAssert(m_pairCache->m_numInCache > m_NumSorted);	// We should have at least 1 new pair. We don't necessarily have any old pairs.

	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

	btBroadphasePair fakePair; 
	fakePair.Init(0xFFFF,0xFFFF,0xFFFF,0xFFFF);
	btBroadphasePair * cache = m_pairCache->m_cache;

	// Merge any previous new pairs into the sorted list. We only need to do this if push collisions are called more than once.
	if ((u32)nextStartingBroadphasePair > m_NumSorted)
	{
		// We must have 'newer' pairs that need to be pruned. Merge the previous new pairs into the sorted list. The previous new pairs have already been pruned and sorted.

		// Scan ahead to the first old pair that is greater than or equal to the new pairs.
		const int oldPairFirst_i = FindNextGTE(cache[m_NumSorted],cache,0,m_NumSorted);

		int oldPair_i = oldPairFirst_i;
		int newPair_i = m_NumSorted;
		const int oldPairMax_i = m_NumSorted;
		const int newPairMax_i = nextStartingBroadphasePair;

		btBroadphasePair * oldPairBuffer;
		const int oldPairBufferSize = oldPairMax_i - oldPair_i;
		if (oldPairBufferSize > 0)
		{
			// Copy the old pairs into a temporary buffer.
			oldPairBuffer = Alloca(btBroadphasePair,oldPairBufferSize);
			memcpy(oldPairBuffer,m_pairCache->m_cache+oldPairFirst_i,sizeof(btBroadphasePair)*oldPairBufferSize);
		}
		else
		{
			FastAssert(oldPairBufferSize == 0);
			FastAssert(oldPair_i == oldPairMax_i);
			oldPairBuffer = NULL;
		}
		btBroadphasePair oldPairCur = (oldPair_i < oldPairMax_i) ? cache[oldPair_i] : fakePair;
		btBroadphasePair newPairCur = (newPair_i < newPairMax_i) ? cache[newPair_i] : fakePair;
		int dest_i = oldPairFirst_i;
		while (dest_i < nextStartingBroadphasePair)
		{
			if (oldPairCur < newPairCur)
			{
				FastAssert(dest_i < newPair_i);			// Make sure we're not clobbering the new pairs.
				cache[dest_i] = oldPairCur;
				dest_i++;
				oldPair_i++;
				oldPairCur = (oldPair_i < oldPairMax_i) ? oldPairBuffer[oldPair_i-oldPairFirst_i] : fakePair;
			}
			else
			{
				FastAssert(newPairCur < oldPairCur);
				FastAssert(dest_i <= newPair_i);		// Make sure we didn't somehow pass the current pair.
				cache[dest_i] = newPairCur;
				dest_i++;
				newPair_i++;
				newPairCur = (newPair_i < newPairMax_i) ? cache[newPair_i] : fakePair;
			}
		}
		m_NumSorted = nextStartingBroadphasePair;
#if __DEV
		VerifySort(m_pairCache->m_cache,0,m_NumSorted);
#endif // __DEV
	}

	PF_START(SortPairs);
	// Sort the newest pairs which have been added to the end of the array
	std::sort(m_pairCache->m_cache + m_NumSorted, m_pairCache->m_cache + m_pairCache->m_numInCache);
	PF_STOP(SortPairs);

	// Prune the newest pairs.
	int oldPair_i = 0;
	int newPair_i = m_NumSorted;
	int newPairWrite_i = m_NumSorted;
	const int oldPairMax_i = m_NumSorted;
	const int newPairMax_i = m_pairCache->m_numInCache;
	while (newPair_i < newPairMax_i)
	{
		// Scan ahead to the next old pair that is greater than or equal to the current new pair.
		oldPair_i = FindNextGTE(cache[newPair_i],cache,oldPair_i,oldPairMax_i);
#if __DEV
		if (oldPair_i < oldPairMax_i)
			FastAssert(cache[newPair_i] < cache[oldPair_i] || cache[newPair_i] == cache[oldPair_i]);
		if (oldPair_i > 0)
			FastAssert(cache[oldPair_i-1] < cache[newPair_i]);
#endif // __DEV
		const btBroadphasePair oldPairCur = (oldPair_i < oldPairMax_i) ? cache[oldPair_i] : fakePair;

		// Prune any new pairs that are duplicates of the current old pair.
		while (newPair_i < newPairMax_i && cache[newPair_i] == oldPairCur)
		{
			FastAssert(cache[newPair_i].GetManifold() == NULL);
#if USE_STATIC_BVH_REJECTION
			FastAssert(cache[newPair_i].GetCacheIndex() == btBroadphasePair::INVALID_INDEX);
#endif // USE_STATIC_BVH_REJECTION
			newPair_i++;
		}

		// Prune duplicates among the new pairs.
		while (newPair_i < newPairMax_i && cache[newPair_i] < oldPairCur)
		{
			// This new pair is not duplicated in the old pairs.
			const int save_i = newPair_i;

			// Check for duplicates in the new pairs.
			do
			{
				FastAssert(cache[newPair_i].GetManifold() == NULL);
#if USE_STATIC_BVH_REJECTION
				FastAssert(cache[newPair_i].GetCacheIndex() == btBroadphasePair::INVALID_INDEX);
#endif // USE_STATIC_BVH_REJECTION
				newPair_i++;
			} while (newPair_i < newPairMax_i && cache[save_i] == cache[newPair_i]);

			//if (KeepPair(cache[save_i]))
			{
				// Collapse the new pair array.
				cache[newPairWrite_i] = cache[save_i];
				newPairWrite_i++;
			}
		}
	}
	m_pairCache->m_numInCache = newPairWrite_i;
#if __DEV
	VerifySort(m_pairCache->m_cache,m_NumSorted,m_pairCache->m_numInCache);
#endif // __DEV

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}
}

void btAxisSweep3::pruneActiveOverlappingPairs()
{
	const bool bOnlyMainThreadIsUpdatingPhysics = m_Level->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics broadphase now!");
	}

#if USE_GLOBAL_SAP_UPDATE
	DoSort();
#endif

	// At this point in time, the pair cache (m_pairCache) should contain all pairs that are overlapping spatially *and* whose physical states (active,
	//   inactive, etc) are compatible for collision.  It probably also contains some cruft (pairs that are no longer overlapping, pairs that have been
	//   marked for removal, and pairs that refer to objects that have been deleted).

	PF_START(SortPairs);

	// Sort the new pairs which have been added to the end of the array
	std::sort(m_pairCache->m_cache + m_NumSorted, m_pairCache->m_cache + m_pairCache->m_numInCache);

	PF_STOP(SortPairs);

	// Tracking the old, sorted pairs that we need to check for overlaps but we know are in order already
	btBroadphasePair* curSortedPair = m_pairCache->m_cache;
	btBroadphasePair* lastSortedPair = m_pairCache->m_cache + m_NumSorted;

	// The new pairs we need to merge in
	btBroadphasePair* curNewPair = lastSortedPair;
	btBroadphasePair* lastNewPair = m_pairCache->m_cache + m_pairCache->m_numInCache;

	// The pair we are currently writing to, starting from the beginning of the array
	btBroadphasePair* outPair = m_pairCache->m_cache;

	// Storage for current pairs as we merge in the new pairs
	const u32 RING_BUFFER_SIZE = 4096;
	btBroadphasePair* const ringBuffer = Alloca(btBroadphasePair, RING_BUFFER_SIZE);
	btBroadphasePair* ringBegin = ringBuffer;
	btBroadphasePair* ringEnd = ringBuffer;
	btBroadphasePair* const ringMax = ringBuffer + RING_BUFFER_SIZE;

	// Put a few pairs in the ring buffer, to avoid LHSs that would occur if the ring buffer gets too small
	const int PREPOPULATE_RING_BUFFER = 10;
	CompileTimeAssert(PREPOPULATE_RING_BUFFER <= RING_BUFFER_SIZE);
	btBroadphasePair* const lastPrepopulatedPair = Min(curSortedPair + PREPOPULATE_RING_BUFFER, lastSortedPair);
	while (curSortedPair < lastPrepopulatedPair)
	{
		*ringEnd++ = *curSortedPair++;
	}

	PF_START(TestOverlaps);

	// Make a fake pair that won't ever match as a duplicate of any pair
	btBroadphasePair fakeOldPair; fakeOldPair.Init(0xffff, 0xffff, 0xffff, 0xffff);
	btBroadphasePair* oldPair = &fakeOldPair;

	while(curSortedPair < lastSortedPair || curNewPair < lastNewPair || ringBegin != ringEnd)
	{
		// Try to move a sorted pair into the ring buffer
		btBroadphasePair* ringNext = ringEnd + 1;

		// Handle ring wrap around
#if 1
		size_t ringNextWrap = GenerateMaskEq((ptrdiff_t)ringNext, (ptrdiff_t)ringMax);
		ringNext = (btBroadphasePair*)((size_t(ringBuffer) & ringNextWrap) | (size_t(ringNext) & ~ringNextWrap));
#else
		if (ringNext == ringMax)
		{
			ringNext = ringBuffer;
		}
#endif

		// If we have more sorted pairs left, and space in the ring buffer, put a pair in there
		if (curSortedPair < lastSortedPair && ringEnd != ringBegin)
		{
			*ringEnd = *curSortedPair++;
			ringEnd = ringNext;
		}

		// Choose whether the new pair will come from the ring buffer or the new pairs
		btBroadphasePair* chosenPair;

		if (curNewPair == lastNewPair || (ringBegin != ringEnd && *ringBegin < *curNewPair))
		{
			// The ring buffer pair wins
			chosenPair = ringBegin++;

			// Handle ring wrap around
#if 1
			size_t ringBeginWrap = GenerateMaskEq((ptrdiff_t)ringBegin, (ptrdiff_t)ringMax);
			ringBegin = (btBroadphasePair*)((size_t(ringBuffer) & ringBeginWrap) | (size_t(ringBegin) & ~ringBeginWrap));
#else
			if (ringBegin == ringMax)
			{
				ringBegin = ringBuffer;
			}
#endif
		}
		else
		{
			// The new pair wins
			Assert(curNewPair < lastNewPair);
			chosenPair = curNewPair++;
		}

		// Check for duplicates
		if (*oldPair == *chosenPair)
		{
			// If we have a duplicate, don't add it to the array (skip the else block)
#if USE_PRUNE_NEW_PAIRS

#if USE_STATIC_BVH_REJECTION
			phManifold * chosenManifold = chosenPair->GetManifold();
			const u16 chosenCacheIndex = chosenPair->GetCacheIndex();
			if (chosenManifold || chosenCacheIndex != btBroadphasePair::INVALID_INDEX)
			{
				FastAssert(oldPair->GetManifold() == NULL);
				FastAssert(oldPair->GetCacheIndex() == btBroadphasePair::INVALID_INDEX);
				oldPair->SetManifold(chosenManifold);
				oldPair->SetCacheIndex(chosenCacheIndex);
			}
#else // USE_STATIC_BVH_REJECTION
			// But if the duplicate has a manifold, we have to handle that
			if (phManifold* chosenManifold = chosenPair->GetManifold())
			{
				FastAssert(oldPair->GetManifold() == NULL);
				oldPair->SetManifold(chosenManifold);
			}
#endif // USE_STATIC_BVH_REJECTION

#else // USE_PRUNE_NEW_PAIRS

			// But if the duplicate has a manifold, we have to handle that
			if (phManifold* chosenManifold = chosenPair->GetManifold())
			{
				if (oldPair->GetManifold())
				{
					// If both our duplicate and out new pair have manifolds, release the second one 
					Assert(chosenManifold != oldPair->GetManifold());

					PHMANIFOLD->Release(chosenManifold);
				}
				else
				{
					// If the new pair has a manifold, move it to the old pair
					oldPair->SetManifold(chosenManifold);
				}
			}

#if USE_STATIC_BVH_REJECTION
			const u16 chosenCacheIndex = chosenPair->GetCacheIndex();
			if (chosenCacheIndex != btBroadphasePair::INVALID_INDEX)
			{
				if (oldPair->GetCacheIndex() != btBroadphasePair::INVALID_INDEX)
				{
					FastAssert(oldPair->GetCacheIndex() != chosenCacheIndex);
					DeleteStaticBvhCacheEntry(chosenCacheIndex);
				}
				else
				{
					oldPair->SetCacheIndex(chosenCacheIndex);
				}
			}
#endif // USE_STATIC_BVH_REJECTION

#endif // USE_PRUNE_NEW_PAIRS

		}
		else
		{
			if (!Verifyf(outPair < curSortedPair || curSortedPair == lastSortedPair, "Ring buffer (%d) is too small", RING_BUFFER_SIZE))
			{
				// In the mean time, take some drastic (expensive) measures. Move all the unfinished pairs to the end of the array.
				size_t remainingPairsSize = size_t(lastNewPair) - size_t(curSortedPair);
				void* dest = (void*)((size_t)(m_pairCache->m_cache + m_pairCache->m_maxPairs) - remainingPairsSize);
				memmove(dest, curSortedPair, remainingPairsSize);

				// This will give us plenty of room. Shift all the array pointers up to where they are now tucked away.
				size_t shift = size_t(dest) - size_t(curSortedPair);
				curSortedPair = (btBroadphasePair*)(size_t(curSortedPair) + shift);
				lastSortedPair = (btBroadphasePair*)(size_t(lastSortedPair) + shift);
				curNewPair = (btBroadphasePair*)(size_t(curNewPair) + shift);
				lastNewPair = (btBroadphasePair*)(size_t(lastNewPair) + shift);

				Assert(lastNewPair == m_pairCache->m_cache + m_pairCache->m_maxPairs);
				Assert(outPair < curSortedPair || curSortedPair == lastSortedPair);
			}

			if (KeepPair(*chosenPair))
			{
				oldPair = outPair;

				// Write the out pair
				*outPair++ = *chosenPair;
#if __ASSERT
				// The return value of m_KeepPairFunc should never change for a given pair of objects. Once we disable the broadphase pair
				//   we won't get another chance to re-enable it until the objects bounding boxes separate and then overlap again.
				bool userWantsToKeepPair = m_KeepPairFunc(chosenPair->GetObject0(),chosenPair->GetObject1());
				if(!userWantsToKeepPair)
				{
					phInst* instA = PHLEVEL->GetInstance(chosenPair->GetObject0());
					phInst* instB = PHLEVEL->GetInstance(chosenPair->GetObject1());
					Assertf(userWantsToKeepPair,"Keeping bad broadphase pair ('%s',0x%X,%p), ('%s',0x%X,%p).", 
						instA->GetArchetype()->GetFilename(),PHLEVEL->GetInstanceTypeFlags(chosenPair->GetObject0()),instA->GetUserData(),
						instB->GetArchetype()->GetFilename(),PHLEVEL->GetInstanceTypeFlags(chosenPair->GetObject1()),instB->GetUserData());
				}
#endif // __ASSERT
			}
			else
			{
				DESTRUCT_BP_PAIR(chosenPair);
			}
		}
	}

	// If this assert fails it means that something changed m_numInCache during these iterations, either from another thread or during the loop somehow.
	FastAssert(curSortedPair == lastSortedPair && curNewPair == lastNewPair || ringBegin == ringEnd);

	u32 numPairs = ptrdiff_t_to_int(outPair - m_pairCache->m_cache);
	m_pairCache->m_numInCache = numPairs;
	m_NumSorted = numPairs;

#if __DEV
	VerifySort(m_pairCache->m_cache,0,m_NumSorted);
#endif // __DEV

	PF_STOP(TestOverlaps);

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_CriticalSectionToken.Unlock();
	}

#if USE_STATIC_BVH_REJECTION_DEBUG
	ValidateStaticCache();
#endif // USE_STATIC_BVH_REJECTION_DEBUG
}


inline void btAxisSweep3::QuantizeAndCheck(Vector3::Ref vminmax, Vector3::Param min, Vector3::Param max) const
{
	Vector3 aabbMin(min), aabbMax(max);

#if !LEVELNEW_USE_EVERYWHERE_NODE
	Assertf(aabbMin.IsGreaterOrEqualThan(m_worldAabbMin) && aabbMin.IsLessOrEqualThan(m_worldAabbMax),
		"aabbMin (%0.2f, %0.2f, %0.2f), aabbMax (%0.2f, %0.2f, %0.2f)\n world min (%0.2f, %0.2f, %0.2f), world max (%0.2f, %0.2f, %0.2f)",
		aabbMin.x, aabbMin.y, aabbMin.z,
		aabbMax.x, aabbMax.y, aabbMax.z,
		m_worldAabbMin.x, m_worldAabbMin.y, m_worldAabbMin.z,
		m_worldAabbMax.x, m_worldAabbMax.y, m_worldAabbMax.z);
	Assertf(aabbMax.IsGreaterOrEqualThan(m_worldAabbMin) && aabbMax.IsLessOrEqualThan(m_worldAabbMax),
		"aabbMin (%0.2f, %0.2f, %0.2f), aabbMax (%0.2f, %0.2f, %0.2f)\n world min (%0.2f, %0.2f, %0.2f), world max (%0.2f, %0.2f, %0.2f)",
		aabbMin.x, aabbMin.y, aabbMin.z,
		aabbMax.x, aabbMax.y, aabbMax.z,
		m_worldAabbMin.x, m_worldAabbMin.y, m_worldAabbMin.z,
		m_worldAabbMax.x, m_worldAabbMax.y, m_worldAabbMax.z);
	Assertf(aabbMax.IsGreaterOrEqualThan(aabbMin),
		"aabbMin (%0.2f, %0.2f, %0.2f), aabbMax (%0.2f, %0.2f, %0.2f)",
		aabbMin.x, aabbMin.y, aabbMin.z,
		aabbMax.x, aabbMax.y, aabbMax.z);
#endif

#if AXISSWEEP_PATCH_BAD_AABBS
	Vector3 vFinalMin, vFinalMax;
	vFinalMin.Min(aabbMin, aabbMax);
	vFinalMax.Max(aabbMin, aabbMax);
#else
#if !__FINAL
	if(!aabbMax.IsGreaterOrEqualThan(aabbMin))
	{
		Quitf("Invalid AABB in btAxisSweep::addHandle: <%f, %f, %f>, <%f, %f, %f>", aabbMin.x, aabbMin.y, aabbMin.z, aabbMax.x, aabbMax.y, aabbMax.z);
	}
#endif
	Vector3 vFinalMin, vFinalMax;
	vFinalMin.Set(aabbMin);
	vFinalMax.Set(aabbMax);
#endif
	quantize(vminmax, vFinalMin, vFinalMax);
}


}
