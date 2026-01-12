#ifndef CLOTH_RAGE_INTERFACE_H
#define CLOTH_RAGE_INTERFACE_H

#include "physics/inst.h"
#include "physics/instbehavior.h"
#include "phBound/boundcomposite.h"

#include "Debug/Debug.h"
#include "fwtl/pool.h"

class CCloth;
class CClothParticles;
class CClothUpdater;
class CClothContacts;
class CClothCapsuleBounds;

class phInstBehaviourCloth : public phInstBehavior
{
public:

	FW_REGISTER_CLASS_POOL(phInstBehaviourCloth);

	phInstBehaviourCloth(CClothUpdater* pUpdater, CCloth* pCloth)
		: phInstBehavior(),
		m_pClothUpdater(pUpdater),
		m_pCloth(pCloth),
		m_iNumCollisionInstances(0)
	{
		Assertf(m_pCloth, "Null cloth ptr");
		Assertf(m_pClothUpdater, "Null cloth updater ptr");
	}
	virtual ~phInstBehaviourCloth()
	{
	}

	CClothUpdater* GetClothUpdater() 
	{
		return m_pClothUpdater;
	}
	CCloth* GetCloth() 
	{
		return m_pCloth;
	}

	phBoundComposite* CreateBound() const;

	void SetStartTransform(const Matrix34& startTransform);

	///////////////////////////////////////////
	//Begin Virtual functions from phInstBehaviouir
	///////////////////////////////////////////

	// This should be called before a phInstBehavior is 'put into action', that is, before it is registered with the phSimulator.
	virtual void Reset(){}

	// If this returns false, that means that this instance behavior is done, and won't do anything further without any prompting.
	//   This will usually be used by a manager class to determine when to remove the behavior from the phSimulator.
	virtual bool IsActive() const 
	{
		return true;
	}

	// If this returns false, then constraints attached to this instance will be allowed to sleep
	virtual bool IsAsleep() const { return false; }

	// Called once per frame to give you an opportunity to make any changes that you wish to make.
	virtual void PreUpdate (float TimeStep);

	// Called once per frame to give you an opportunity to make any changes that you wish to make.
	virtual void Update (float TimeStep);

	// Called when your instance's cull sphere is intersecting another instance's cull sphere to give you a chance to cause an alternate response.
	virtual bool CollideObjects(Vec::V3Param128 timeStep, phInst* UNUSED_PARAM(myInst), phCollider* UNUSED_PARAM(myCollider), phInst* UNUSED_PARAM(otherInst), phCollider* UNUSED_PARAM(otherCollider), phInstBehavior* UNUSED_PARAM(otherInstBehavior));

	// Called before the instance's ActivateWhenHit() is called.  Return false to disallow activation.  Return true to let the phInst
	//   proceed as normal.
	virtual bool ActivateWhenHit() const {return true;}

	// PURPOSE: See if this instance behavior exerts forces on objects instead of having rigid collisions.
	// RETURN: true if this instance behavior exerts forces (liquid, explosions), false if it does not or if it uses collisions (rope, cloth)
	virtual bool IsForceField () const { return false; }

	///////////////////////////////////////////
	//End Virtual functions from phInstBehaviouir
	///////////////////////////////////////////

private:

	//Use a cloth updater to update a cloth.
	CClothUpdater* m_pClothUpdater;
	CCloth* m_pCloth;

	//Number of objects colliding with cloth bounds.
	enum
	{
		MAX_NUM_COLLISION_INSTANCES=8
	};
	int m_iNumCollisionInstances;
	phInst* m_apCollisionInstances[MAX_NUM_COLLISION_INSTANCES];

	void IntersectSpheres(const phInst* pOtherInst, const CCloth& cloth, CClothContacts& contacts);
	void IntersectCapsules(const phInst* pOtherInst, const CCloth& cloth, CClothContacts& contacts);

	//Add contact points to the cloth updater after doing some collision detection.
	void GenerateContacts(CClothContacts& contacts);
	//Update the composite bound with the latest constraint and particle states.
	void UpdateCompositeBound();

	/*
	//Update the extents of the bound with the latest matrices of the composite.
	void ComputeBoundingMinMax(Vector3& vMin, Vector3& vMax) const;
	//Compute the bounding sphere that bounds the cloth particles and use this for the 
	//instance position.
	void ComputeBoundingSphere(Vector3& vCentre, float& fRadius) const;
	*/
};

class phInstCloth : public phInst
{
public:

	FW_REGISTER_CLASS_POOL(phInstCloth);

	phInstCloth(phInstBehaviourCloth* pInstBehaviourCloth)
		: phInst(),
		m_pInstBehaviourCloth(pInstBehaviourCloth)
	{
		Assertf(m_pInstBehaviourCloth, "Null ptr");
	}
	virtual ~phInstCloth()
	{
	}

	//Get the instance behaviour that is used to update the cloth.
	phInstBehaviourCloth* GetInstBehaviourCloth() 
	{
		return m_pInstBehaviourCloth;
	}
	//The instance behaviour owns the cloth ptr.
	CCloth* GetCloth()
	{
		return m_pInstBehaviourCloth->GetCloth();
	}
	//The instance behaviour owns the cloth updater ptr.
	CClothUpdater* GetClothUpdater()
	{
		return m_pInstBehaviourCloth->GetClothUpdater();
	}

private:

	phInstBehaviourCloth* m_pInstBehaviourCloth;
};


#endif //CLOTH_RAGE_INTERFACE_H
