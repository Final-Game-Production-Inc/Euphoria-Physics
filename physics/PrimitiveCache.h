#ifndef PRIMITIVE_CACHE_H
#define PRIMITIVE_CACHE_H

#include "phcore/constants.h"
#include "phbullet/TriangleShape.h"
#include "phbound/boundsphere.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundbox.h"
#include "phbound/boundcylinder.h"
#include "phcore/LinearMemoryAllocator.h"
#include "collision.h"
#include "physics/physicsprofilecapture.h"

namespace rage
{

#if __PS3 && !__SPU
	#define LIKELY_TARGET __attribute__((likely_target))
#else 
	#define LIKELY_TARGET 
#endif

struct NewCollisionInput;
class phInst;
class phBoundBVH;
class phOptimizedBvhNode;

#define DECLARE_PRIM_BUFFER(type) ALIGNAS(16) u8 m_primBuffer[sizeof(type)] 
#define PRIM_NEW(type) new((void*)m_primBuffer) type

#if __ASSERT && COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	#define USE_CHECK_TRIANGLE 1
	#define CHECK_TRI_ONLY(x) ,x
#else
	#define USE_CHECK_TRIANGLE 0
	#define CHECK_TRI_ONLY(x)
#endif

#define USE_PRIMITIVE_CULLING 0
#define PRIM_CACHE_TESTOVERLAP_VIRTUAL 0
#define PRIM_CACHE_COLLIDE_VIRTUAL 1
#define STORE_PRIM_TYPE ((!PRIM_CACHE_TESTOVERLAP_VIRTUAL && USE_PRIMITIVE_CULLING) || !PRIM_CACHE_COLLIDE_VIRTUAL)

struct PrimitiveBase;

struct PrimitiveLeaf
{
	struct key_t
	{
		// u16 levelindex
		// u8 component
		// u16 partindex
		u32 a1;
		u32 a2;
	};

	// Map vars.
	key_t m_key;
	PrimitiveLeaf * m_left;
	PrimitiveLeaf * m_right;
	char m_balance;

	const phBoundBVH * m_boundBVH;
	const phOptimizedBvhNode * m_curNode;
#if USE_CHECK_TRIANGLE
	const phInst * m_pInstOfBvh;
#endif // USE_CHECK_TRIANGLE
	PrimitiveBase * m_first;

	__forceinline void Init(const key_t & key, const phBoundBVH * boundBVH, const phOptimizedBvhNode * curNode CHECK_TRI_ONLY(const phInst * pInstOfBvh))
	{
		m_key = key;
		m_boundBVH = boundBVH;
		m_curNode = curNode;
#if USE_CHECK_TRIANGLE
		m_pInstOfBvh = pInstOfBvh;
#endif // USE_CHECK_TRIANGLE
	}

#if __SPU
	__forceinline void ReInit(const phBoundBVH * boundBVH, const phOptimizedBvhNode * curNode)
	{
		m_boundBVH = boundBVH;
		m_curNode = curNode;
	}
#endif // __SPU

	__forceinline const phBoundBVH * GetBoundBVH() const
	{
		return m_boundBVH;
	}

#if USE_CHECK_TRIANGLE
	__forceinline const phInst * GetInstOfBvh() const
	{
		return m_pInstOfBvh;
	}
#endif // USE_CHECK_TRIANGLE
};

#if PRIM_CACHE_RENDER
struct PrimitiveCylinder;
#endif // PRIM_CACHE_RENDER

struct PrimitiveBase
{
	NewCollisionObject collisionObject;
	const PrimitiveLeaf * m_leaf;
	PrimitiveBase * m_next;
#if STORE_PRIM_TYPE
	PrimitiveType m_primType;
#endif

	__forceinline void InitBase(const PrimitiveLeaf * leaf)
	{
		m_leaf = leaf;
		collisionObject.m_bound = NULL;
	}

	__forceinline bool IsSetup()
	{
		return (collisionObject.m_bound != NULL);
	}

	virtual ~PrimitiveBase()
	{
		FastAssert(0);
	}

#if PRIM_CACHE_RENDER
	virtual const TriangleShape * GetTriangleShape() const { return NULL; }
	virtual void GetTriangleVertices(Vec3V * v0, Vec3V * v1, Vec3V * v2) const { Assert(0); (void)v0; (void)v1; (void)v2; }
	virtual const PrimitiveCylinder * GetPrimitiveCylinder() const { return NULL; }
#endif 

#if USE_PRIMITIVE_CULLING
#if PRIM_CACHE_TESTOVERLAP_VIRTUAL
	virtual int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents) = 0;
#else
	int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
#endif
#endif // USE_PRIMITIVE_CULLING

#if PRIM_CACHE_COLLIDE_VIRTUAL
	virtual void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector) = 0;
#else
	void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
#endif
};

template <class T> __forceinline void CollideTemplate(T * prim, Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, /*const phConvexIntersector::PenetrationDepthSolverType pdsType,*/ DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	if (prim->TestOverlap(boxOrientation,boxHalfExtents))
	{
		FastAssert(collisionInput.object1 == NULL);
		// object1 is uninitialized at this point. Use collisionInput.m_bvhInput to get BVH last and current matrices.
		if (prim->IsSetup() == false)
		{
			prim->Setup(collisionInput);
			FastAssert(prim->IsSetup() == true);
		}

		collisionInput.object1 = &prim->collisionObject;

		FastAssert(collisionInput.gjkCacheInfo);
		collisionInput.gjkCacheInfo->SetPartIndex(prim->GetPartIndex());

		//PPC_STAT_TIMER_START(PrimitiveCollideNarrowPhaseTimer);
		phPairwiseCollisionProcessor::ProcessPairwiseCollision(pointCollector, collisionInput);//, pdsType);
		//PPC_STAT_TIMER_STOP(PrimitiveCollideNarrowPhaseTimer);

		if (pointCollector.GetHasResult())
			prim->ProcessResult(collisionInput,pointCollector);

		collisionInput.object1 = NULL;
	}
}

struct PrimitiveTriangle : public PrimitiveBase
{
	DECLARE_PRIM_BUFFER(TriangleShape);

	Vec3V localTrianglePosition;
	Vec3V neighborNormals0, neighborNormals1, neighborNormals2;

	phPolygon curPrimitive;
	int hasNeighbor;

	TriangleShape * GetTriangle() { return reinterpret_cast<TriangleShape*>(m_primBuffer); }
	const TriangleShape * GetTriangle() const { return reinterpret_cast<const TriangleShape*>(m_primBuffer); }

#if PRIM_CACHE_RENDER
	virtual const TriangleShape * GetTriangleShape() const { return GetTriangle(); }
	virtual void GetTriangleVertices(Vec3V * v0, Vec3V * v1, Vec3V * v2) const 
	{
		const TriangleShape * tm = GetTriangle();
		*v0 = tm->m_vertices1[0] + localTrianglePosition;
		*v1 = tm->m_vertices1[1] + localTrianglePosition;
		*v2 = tm->m_vertices1[2] + localTrianglePosition;
	}
#endif 

	int GetPartIndex() const { return GetTriangle()->GetIndexFromBound(); }
	void Init(const phPrimitive & bvhPrimitive, const int curPrimIndex);
	void Setup(const NewCollisionInput & collisionInput);
	void ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);

#if PRIM_CACHE_TESTOVERLAP_VIRTUAL
	LIKELY_TARGET int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
#else
	int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
#endif

#if PRIM_CACHE_COLLIDE_VIRTUAL
	LIKELY_TARGET void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
#else
	void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
#endif
};

struct PrimitiveSphere : public PrimitiveBase
{
	DECLARE_PRIM_BUFFER(phBoundSphere);
	Vec3V localSpherePosition;
	float m_radius;

	phBoundSphere * GetSphere()  { return reinterpret_cast<phBoundSphere*>(m_primBuffer); }
	const phBoundSphere * GetSphere() const { return reinterpret_cast<const phBoundSphere*>(m_primBuffer); }

	int GetPartIndex() const { return GetSphere()->GetIndexFromBound(); }
	void Init(const phPrimitive & bvhPrimitive, const int curPrimIndex);
	void Setup(const NewCollisionInput & collisionInput);
	void ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
	int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
	void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
};

struct PrimitiveCapsule : public PrimitiveBase
{
	DECLARE_PRIM_BUFFER(phBoundCapsule);
	Mat34V localCapsuleMatrix;
	Vec3V capsuleEnd0, capsuleEnd1;
	float m_radius;

	phBoundCapsule * GetCapsule() { return reinterpret_cast<phBoundCapsule*>(m_primBuffer); }
	const phBoundCapsule * GetCapsule() const { return reinterpret_cast<const phBoundCapsule*>(m_primBuffer); }

	int GetPartIndex() const { return GetCapsule()->GetIndexFromBound(); }
	void Init(const phPrimitive & bvhPrimitive, const int curPrimIndex);
	void Setup(const NewCollisionInput & collisionInput);
	void ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
	int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
	void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
};

struct PrimitiveBox : public PrimitiveBase
{
	DECLARE_PRIM_BUFFER(phBoundBox);
	Mat34V localBoxMatrix;
	Vec3V boxSize;

	phBoundBox * GetBox() { return reinterpret_cast<phBoundBox*>(m_primBuffer); }
	const phBoundBox * GetBox() const { return reinterpret_cast<const phBoundBox*>(m_primBuffer); }

	int GetPartIndex() const { return GetBox()->GetIndexFromBound(); }
	void Init(const phPrimitive & bvhPrimitive, const int curPrimIndex);
	void Setup(const NewCollisionInput & collisionInput);
	void ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
	int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
	void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
};

struct PrimitiveCylinder : public PrimitiveBase
{
	DECLARE_PRIM_BUFFER(phBoundCylinder);
	Mat34V localCylinderMatrix;
	Vec3V cylinderEnd0, cylinderEnd1;
	float m_radius;

	phBoundCylinder * GetCylinder() { return reinterpret_cast<phBoundCylinder*>(m_primBuffer); }
	const phBoundCylinder * GetCylinder() const { return reinterpret_cast<const phBoundCylinder*>(m_primBuffer); }

#if PRIM_CACHE_RENDER
	virtual const PrimitiveCylinder * GetPrimitiveCylinder() const { return this; }
#endif

	int GetPartIndex() const { return GetCylinder()->GetIndexFromBound(); }
	void Init(const phPrimitive & bvhPrimitive, const int curPrimIndex);
	void Setup(const NewCollisionInput & collisionInput);
	void ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
	int TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents);
	void Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector);
};

struct PrimitiveCache
{
	LinearMemoryAllocator m_buffer;
	PrimitiveLeaf * m_root;

	__forceinline PrimitiveCache()
	{
		m_buffer.NullBuffer();
		m_root = NULL;
	}

	__forceinline void Reset()
	{
		m_buffer.Reset();
		m_root = NULL;
	}

	__forceinline void * Alloc(const u32 size, const u32 alignment)
	{
		void * ptr = m_buffer.FastAlloc(size,alignment);
		FastAssert(ptr);
#if __DEV
		memset(ptr,0xFF,size);
#endif // __DEV
		return ptr;
	}

	PrimitiveLeaf * GetLeaf(const phBoundBVH * boundBVH, const phOptimizedBvhNode * curNode, const u32 levelIndex, const u16 component CHECK_TRI_ONLY(const phInst * pInstOfBvh));
};

} // namespace rage


#endif // PRIMITIVE_CACHE_H