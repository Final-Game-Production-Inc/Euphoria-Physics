/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#if 0
#ifndef COLLISION__DISPATCHER_H
#define COLLISION__DISPATCHER_H

#include "btDispatcher.h"
#include "phbound/bound.h"

#include "atl/pool.h"

class btIDebugDraw;
class btOverlappingPairCache;


#include "btConvexConvexAlgorithm.h"
#include "btSphereSphereCollisionAlgorithm.h"



///btCollisionDispatcher supports algorithms that handle ConvexConvex and ConvexConcave collision pairs.
///Time of Impact, Closest Points and Penetration Depth.
class btCollisionDispatcher : public btDispatcher
{
	std::vector<rage::phManifold*>	m_manifoldsPtr;
	
    btCollisionAlgorithm* m_doubleDispatch[rage::phBound::NUM_BOUND_TYPES][rage::phBound::NUM_BOUND_TYPES];
	
	btCollisionAlgorithm* internalFindAlgorithm(int proxyType0,int proxyType1);

	//default CreationFunctions, filling the m_doubleDispatch table
	btCollisionAlgorithm* m_convexConvex;
	btCollisionAlgorithm* m_convexConcave;
	btCollisionAlgorithm* m_swappedConvexConcave;
//	btCollisionAlgorithm* m_compound;
//	btCollisionAlgorithm* m_swappedCompound;
	btCollisionAlgorithm* m_empty;
    btCollisionAlgorithm* m_sphereSphere;
//    btCollisionAlgorithm* m_spuSphereSphere;

    ::rage::atPool<btConvexConvexAlgorithm> m_ConvexConvexAlgorithms;
	::rage::atPool<rage::btSphereSphereCollisionAlgorithm> m_SphereSphereAlgorithms;

// #if __PPU
//     ::rage::atPool<SpuSphereSphereCollisionAlgorithm> m_SpuSphereSphereAlgorithms;
// #endif // __PPU
// 
public:

	///registerCollisionAlgorithms allows registration of custom/alternative collision algorithms
	void	registerCollisionAlgorithm(int proxyType0,int proxyType1, btCollisionAlgorithm* algorithm);

	int	getNumManifolds() const
	{ 
		return (int) m_manifoldsPtr.size();
	}

	rage::phManifold**	getInternalManifoldPointer()
	{
		return &m_manifoldsPtr[0];
	}

	 rage::phManifold* getManifoldByIndexInternal(int index)
	{
		return m_manifoldsPtr[index];
	}

	 const rage::phManifold* getManifoldByIndexInternal(int index) const
	{
		return m_manifoldsPtr[index];
	}

	btCollisionDispatcher ();
	virtual ~btCollisionDispatcher();
/*
	virtual rage::phManifold*	getNewManifold(void* b0,void* b1);
	
	virtual void releaseManifold(rage::phManifold* manifold);

	
	///allows the user to get contact point callbacks 
//	virtual	btManifoldResult*	getNewManifoldResult(btCollisionObject* obj0,btCollisionObject* obj1,rage::phManifold* manifold);

	///allows the user to get contact point callbacks 
//	virtual	void	releaseManifoldResult(btManifoldResult*);

	virtual void clearManifold(rage::phManifold* manifold);
*/
			
    btCollisionAlgorithm* findAlgorithm(btCollisionAlgorithmConstructionInfo& ci);
	
	virtual int getUniqueId() { return RIGIDBODY_DISPATCHER;}

	

};

#endif //COLLISION__DISPATCHER_H

#endif