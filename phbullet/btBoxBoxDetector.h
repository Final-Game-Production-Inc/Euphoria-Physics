/*
 * Box-Box collision detection re-distributed under the ZLib license with permission from Russell L. Smith
 * Original version is from Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org

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
#ifndef BOX_BOX_DETECTOR_H
#define BOX_BOX_DETECTOR_H

#if ENABLE_UNUSED_PHYSICS_CODE


namespace rage
{ 
	class phBoundBox;
};

#include "vectormath/classes.h"
#include "DiscreteCollisionDetectorInterface.h"

//hs #include "BulletCollision/NarrowPhaseCollision/btDiscreteCollisionDetectorInterface.h"

//struct ClosestPointInput
//{
//	ClosestPointInput()
//		:m_maximumDistanceSquared(float(1e30))
//	{
//	}
//
//	rage::Mat34V m_transformA;
//	rage::Mat34V m_transformB;
//	float	m_maximumDistanceSquared;
////hs	btStackAlloc* m_stackAlloc;
//};
//
//struct Result
//{
//	Result(): m_numContacts(0) {}
//
//	enum { MAX_CONTACTS = 6 }; 
//
//	void addContactPoint(
//		float normalOnBInWorld[4], 
//		float pointInWorld[4], 
//		float depth) 
//	{
//		if(m_numContacts < MAX_CONTACTS)
//		{
//			m_contacts[m_numContacts] = rage::Vec3V(pointInWorld[0], pointInWorld[1], pointInWorld[2]);
//			m_normals[m_numContacts]  = rage::Vec3V(normalOnBInWorld[0], normalOnBInWorld[1], normalOnBInWorld[2]);
//			m_depths[m_numContacts]   = depth;
//			m_numContacts++;
//		}
//	}                     
//
//	rage::Vec3V  m_contacts[MAX_CONTACTS];
//	rage::Vec3V  m_normals[MAX_CONTACTS];
//	float                    m_depths[MAX_CONTACTS];
//	int                      m_numContacts;
//};


/// btBoxBoxDetector wraps the ODE box-box collision detector
/// re-distributed under the Zlib license with permission from Russell L. Smith
struct btBoxBoxDetector //hs : public btDiscreteCollisionDetectorInterface
{
	const rage::phBoundBox* m_box1;
	const rage::phBoundBox* m_box2;

public:

	btBoxBoxDetector(const rage::phBoundBox* box1, const rage::phBoundBox* box2);

	~btBoxBoxDetector() {};

	void	getClosestPoints(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, 
								DiscreteCollisionDetectorInterface::SimpleResult& output);

};

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif //BT_BOX_BOX_DETECTOR_H
