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
#include "btCollisionDispatcher.h"

#include "btCollisionAlgorithm.h"
#include "btConvexConvexAlgorithm.h"
#include "btEmptyCollisionAlgorithm.h"
#include "btConvexConcaveCollisionAlgorithm.h"
#include "btCompoundCollisionAlgorithm.h"
#include "btSphereSphereCollisionAlgorithm.h"
#include "CollisionObject.h"
#include <algorithm>

#include "physics/simulator.h"
#include "system/spinlock.h"

int gNumManifold = 0;


btCollisionDispatcher::btCollisionDispatcher()
{
	int i;

	//default CreationFunctions, filling the m_doubleDispatch table
	m_convexConvex = rage_new btConvexConvexAlgorithm;
	m_convexConcave = rage_new rage::btConvexConcaveCollisionAlgorithm;
	m_swappedConvexConcave = rage_new rage::btConvexConcaveCollisionAlgorithm;
//    m_compound = rage_new btCompoundCollisionAlgorithm;
//    m_swappedCompound = rage_new btCompoundCollisionAlgorithm;
	m_empty = rage_new btEmptyAlgorithm;
	m_sphereSphere = rage_new rage::btSphereSphereCollisionAlgorithm;

#if __PPU
    //m_spuSphereSphere = rage_new SpuSphereSphereCollisionAlgorithm;
#endif // __PPU

    for (i=0;i<rage::phBound::NUM_BOUND_TYPES;i++)
	{
		for (int j=0;j<rage::phBound::NUM_BOUND_TYPES;j++)
		{
			m_doubleDispatch[i][j] = internalFindAlgorithm(i,j);
			Assert(m_doubleDispatch[i][j]);
		}
	}
	
	
};


void btCollisionDispatcher::registerCollisionAlgorithm(int proxyType0, int proxyType1, btCollisionAlgorithm *algorithm)
{
	m_doubleDispatch[proxyType0][proxyType1] = algorithm;
}

btCollisionDispatcher::~btCollisionDispatcher()
{
	delete m_convexConvex;
	delete m_convexConcave;
	delete m_swappedConvexConcave;
//	delete m_compound;
//	delete m_swappedCompound;
	delete m_empty;
    delete m_sphereSphere;

#if __PPU
//    delete m_spuSphereSphere;
#endif // __PPU
}

/*
rage::phManifold*	btCollisionDispatcher::getNewManifold(void* b0,void* b1) 
{ 
	gNumManifold++;
	
	//ASSERT(gNumManifold < 65535);
	

	btCollisionObject* body0 = (btCollisionObject*)b0;
	btCollisionObject* body1 = (btCollisionObject*)b1;
	
	rage::phManifold* manifold = rage_new rage::phManifold (body0,body1);
	m_manifoldsPtr.push_back(manifold);

	return manifold;
}

void btCollisionDispatcher::clearManifold(rage::phManifold* manifold)
{
	manifold->clearManifold();
}

	
void btCollisionDispatcher::releaseManifold(rage::phManifold* manifold)
{
	
	gNumManifold--;

	//printf("releaseManifold: gNumManifold %d\n",gNumManifold);

	clearManifold(manifold);

	std::vector<rage::phManifold*>::iterator i =
		std::find(m_manifoldsPtr.begin(), m_manifoldsPtr.end(), manifold);
	if (!(i == m_manifoldsPtr.end()))
	{
		std::swap(*i, m_manifoldsPtr.back());
		m_manifoldsPtr.pop_back();
		delete manifold;

	}
	
	
}
*/

btCollisionAlgorithm* btCollisionDispatcher::findAlgorithm(btCollisionAlgorithmConstructionInfo& ci)
{
    btCollisionAlgorithm* algo = m_doubleDispatch[ci.shapeType0][ci.shapeType1];

    return algo;
}


btCollisionAlgorithm* btCollisionDispatcher::internalFindAlgorithm(int proxyType0,int proxyType1)
{
    if (proxyType0 == rage::phBound::SPHERE && proxyType1 == rage::phBound::SPHERE)
    {

//	#if __PPU
//        return m_spuSphereSphere;
//	#else
        return m_sphereSphere;
//	#endif

    }

    if (rage::phBound::IsTypeConvex(proxyType0) && rage::phBound::IsTypeConcave(proxyType1))
    {
        return m_convexConcave;
    }

	return m_convexConvex;

    //if (rage::phBound::IsTypeConvex(proxyType0) && rage::phBound::IsTypeConvex(proxyType1))
	//{
	//	return m_convexConvex;
	//}

	//if (rage::phBound::IsTypeConvex(proxyType1) && rage::phBound::IsTypeConcave(proxyType0))
	//{
	//	return m_swappedConvexConcave;
	//}

	//if (rage::phBound::IsTypeComposite(proxyType0))
	//{
	//	return m_compound;
	//}
	//else
	//{
	//	if (rage::phBound::IsTypeComposite(proxyType1))
	//	{
	//		return m_swappedCompound;
	//	}
	//}

	//failed to find an algorithm
	//return m_empty;
}
#endif