//Bullet Continuous Collision Detection and Physics Library
//Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/


#ifndef AXIS_SWEEP_3_H
#define AXIS_SWEEP_3_H

#include "broadphase.h"

#include "physics/leveldefs.h"
#include "system/criticalsection.h"
#include "atl/functor.h"

#define USE_GLOBAL_SAP_UPDATE 0

/// btAxisSweep3 is an efficient implementation of the 3d axis sweep and prune broadphase.
/// It uses arrays rather then lists for storage of the 3 axis. Also it operates using integer coordinates instead of floats.
/// The testOverlap check is optimized to check the array index, rather then the actual AABB coordinates/pos
namespace rage
{

class btAxisSweep3 : public phBroadPhase
{
public:
	class Edge
	{
	public:
		// Quantized position of this edge on the corresponding axis.
		unsigned short m_pos;			// low bit is min/max

		// Level index of the object to which this edge corresponds.
		unsigned short m_handle;

		u32 IsMax() const {return (u32)m_pos & 1;}

		__forceinline Edge &operator =(const Edge &otherEdge)
		{
			CompileTimeAssert(sizeof(Edge) == 4);
			u32 * RESTRICT thisAsU32 = reinterpret_cast<u32 *>(this);
			const u32 * RESTRICT otherAsU32 = reinterpret_cast<const u32 *>(&otherEdge);
			*thisAsU32 = *otherAsU32;
			return *this;
		}

		bool operator< ( const Edge &e ) const
		{
			return m_pos < e.m_pos;
		}

		bool operator> ( const Edge &e ) const
		{
			return m_pos > e.m_pos;
		}
	};

public:
	class Handle
	{
	public:
		Handle()
		{
		}

		Handle(u16 mins, u16 maxs)
		{
			m_minEdgeIndices[0] = m_minEdgeIndices[1] = m_minEdgeIndices[2] = mins;
			m_maxEdgeIndices[0] = m_maxEdgeIndices[1] = m_maxEdgeIndices[2] = maxs;
		}

		static const Handle UNUSED;

		// indexes into the edge arrays
		u16 m_minEdgeIndices[3];
        u16 m_maxEdgeIndices[3];

		bool operator ==(const Handle& other) const
		{
			return m_minEdgeIndices[0] == other.m_minEdgeIndices[0] &&
				   m_minEdgeIndices[1] == other.m_minEdgeIndices[1] &&
				   m_minEdgeIndices[2] == other.m_minEdgeIndices[2] &&
				   m_maxEdgeIndices[0] == other.m_maxEdgeIndices[0] &&
				   m_maxEdgeIndices[1] == other.m_maxEdgeIndices[1] &&
				   m_maxEdgeIndices[2] == other.m_maxEdgeIndices[2];
		}

		bool operator !=(const Handle& other) const
		{
			return !(*this == other);
		}
	};

	// Why not private?
public:
	Vector3 m_worldAabbMin;						// overall system bounds
	Vector3 m_worldAabbMax;						// overall system bounds

	// Effectively this is telling you the granularity of the fixed point position stored in the Edge objects.  So, for example, if m_quantize.x is 10, then
	//   there a single "game unit" is partitioned into 10 fixed point units (this may or may not be cut in half due to the use of the low bit in the fixed
	//   point position to encode minimum/maximum).
	Vector3 m_quantize;						// scaling factor for quantization
	Vector3 m_InvQuantize;

	u16 m_numHandles;						// number of active handles
	u16 m_maxHandles;						// max number of handles
	Handle* m_pHandles;						// handles pool

	Edge* m_pEdges[3];						// edge arrays for the 3 axes (each array has m_maxHandles * 2 + 2 sentinel entries)

    phLevelNew* m_Level;

	u32 m_NumSorted;

	bool TestSpatialOverlap(const Handle* pHandleA, const Handle* pHandleB) const;
	bool TestSpatialOverlap(const int levelIndex0, const int levelIndex1) const;
	bool TestObjectsAndStates(const int levelIndex0, const int generationId0, const int levelIndex1, const int generationId1) const;

	//Overlap* AddOverlap(unsigned short handleA, unsigned short handleB);
	//void RemoveOverlap(unsigned short handleA, unsigned short handleB);

	//void quantize(unsigned short* out, const Vector3& point, int isMax) const;
	void quantize(Vector3::Ref out, Vector3::Param min, Vector3::Param max) const;

	// Often when moving a min edge downward you are also going to subsequently move the corresponding max edge downward as well.
	// By telling sortMinDown() what that final max edge will be it can avoid adding overlapping pairs that are unnecessary.
	void sortMinDownAddNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex, u32 finalMaxPos);
	void sortMinDownNoNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex);
	void sortMinUp(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex);
	void sortMaxDown(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex);
	// Often when moving a max edge upward you are also going to subsequently move the corresponding min edge upward as well.
	// By telling sortMaxUp() what that final min edge will be it can avoid adding overlapping pairs that are unnecessary.
	void sortMaxUpAddNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex, u32 finalMinPos);
	void sortMaxUpNoNewPairs(Handle * RESTRICT movingHandle, u32 movingHandleIndex, u32 movingEdgePos, int axis, u32 edgeIndex);

	void removeFromAxis( int axis, u16 *toRemove, int nRemove );

	//void sortMaxsIn(int axis, Edge *addEdges, int nAddEdge, int currentEnd);

	void sortEdgesIn(int axis, Edge *addEdges, bool updateOverlaps, u16 nAddEdge, u16 currentEnd);

#if __DEV
	void verifyAllAxes() const;
	void verifyAxis(int axis, bool posAreCorrect = false) const;
	void checkEdgeVsHandle(int axis, int edgeIndex) const;
#endif

	inline Handle* getHandle(int index) const {return &m_pHandles[index];}
public:
	btAxisSweep3(phLevelNew* level,u16 maxHandles, u16 maxPairs, Vector3::Vector3Param worldMin, Vector3::Vector3Param worldMax, btOverlappingPairCache *existingPairCache = NULL );
	virtual ~btAxisSweep3();
	
	virtual void addHandle(const Vector3& aabbMin,const Vector3& aabbMax, u16 pOwner );
	virtual void addHandle(const Vector3& center, float fRadius,  u16 pOwner );

	virtual void addHandles( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles ); 

	virtual bool isHandleAdded(unsigned short handle);

	virtual void removeHandle(unsigned short handle);
	virtual void removeHandles(int *handle, int nHandle);
	virtual void updateHandle(unsigned short handle, const Vector3& aabbMin,const Vector3& aabbMax);
	virtual void updateHandle(unsigned short handle, const Vector3& center,float fRadius );

	virtual btBroadphasePair* addOverlappingPair(u16 object0, u16 object1);
	btBroadphasePair* addOverlappingPairInternal(u16 object0, u16 object1);

    template <class ProcessFunc>
    void processActiveOverlappingPairs();

	bool KeepPair(const btBroadphasePair & pair);

	virtual void PruneNewOverlappingPairs(const int nextStartingBroadphasePair);
	virtual void pruneActiveOverlappingPairs();
	u16 getNumHandles() const;

	virtual Vector3 getQuantize() const
	{
		return m_quantize;
	}

	virtual Vector3 GetInvQuantize() const
	{
		return m_InvQuantize;
	}

	void getHandles( Vector3 **aabbMin, Vector3 **aabbMax, int **pOwner, u16 &nHandles ) const;

	void addHandlesNoNewPairs( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles );

	void prepareCacheForTransfer();

#if USE_GLOBAL_SAP_UPDATE
	int m_axis;
	int m_axis1;
	int m_axis2;
	Edge * m_edgelist;
	void DoSort();
	void MergeLists(Edge * a, Edge * c, Edge * b);
	void SortList(int start_i, int end_i);
#endif // USE_GLOBAL_SAP_UPDATE

	void CriticalSectionLock()
	{
		m_CriticalSectionToken.Lock();
	}
	
	void CriticalSectionUnlock()
	{
		m_CriticalSectionToken.Unlock();
	}

#if __ASSERT
	const sysIpcCurrentThreadId & GetCurThreadId() const
	{
		return m_threadId;
	}
#endif // __ASSERT

	typedef Functor2Ret <bool, int, int> KeepPairFunc;
	void SetKeepPairFunc(KeepPairFunc func) { m_KeepPairFunc = func; }

private:
	sysCriticalSectionToken	m_CriticalSectionToken;

	KeepPairFunc m_KeepPairFunc;

#if __ASSERT
	sysIpcCurrentThreadId	m_threadId;
#endif

	void QuantizeAndCheck(Vector3::Ref out, Vector3::Param min, Vector3::Param max) const;
};


} // namespace rage


#endif //AXIS_SWEEP_3_H
