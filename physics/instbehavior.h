//
// physics/instbehavior.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_INSTBEHAVIOR_H
#define PHYSICS_INSTBEHAVIOR_H

#include "data/base.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "grprofile/drawcore.h"
#include "physics/contactiterator.h"		// Just for the declaration of phContactIterator - can't forward declare the class because it's nested in another class.

namespace rage {

	class phCollider;
	class phInst;
	class phImpact;

	////////////////////////////////////////////////////////////////
	// phInstBehavior

	// PURPOSE
	//   Base class interface for an object that will be allowed add and control behavior relating to a phInst object.
	// NOTES
	//   This class is designed to provide 'plug-in' functionality for phInst objects without having to derive from them.  They are
	//     integrated into a lot of the work that phSimulator does (see the comments on the specific methods below for details) and
	//     *need* to registered with a phSimulator in order for them to be effective.  phSimulator also has the responsibility of
	//     updating all phInstBehavior objects that have been registered with it.
	//   Objects can be created and owned by anyone, but often it will be convenient to make a manager class a given derived class
	//     to handle things like creation, pooling and addition/removal from the simulator, and also to provide a fire-and-forget
	///    interface where appropriate.  For an example of a simple manager class, see phExplosionMgr.

	class phInstBehavior : public datBase
	{
	public:
		inline phInstBehavior();
		phInstBehavior(datResource &rsc) : m_Instance(rsc) {  }
		virtual inline ~phInstBehavior();

		inline phInst * GetInstance() const;

		inline void SetInstance(phInst &Instance);

		// This should be called before a phInstBehavior is 'put into action', that is, before it is registered with the phSimulator.
		virtual void Reset() = 0;

		// If this returns false, that means that this instance behavior is done, and won't do anything further without any prompting.
		//   This will usually be used by a manager class to determine when to remove the behavior from the phSimulator.
		virtual bool IsActive() const = 0;

		// If this returns false, then constraints attached to this instance will be allowed to sleep
		virtual bool IsAsleep() const { return false; }

		// Called once per frame to give you an opportunity to make any changes that you wish to make.
		virtual void PreUpdate (float UNUSED_PARAM(TimeStep)) {}

		// Called once per frame to give you an opportunity to make any changes that you wish to make.
		virtual void Update (float TimeStep) = 0;

		// PURPOSE: Called when your instance's cull sphere is intersecting another instance's cull sphere to give you a chance to cause an alternate
		//   response.
		// RETURN: false to indicate that you have handled collision detection and response yourself and do not wish for the collision to be further
		//   handled by the physics system.  If you return false from here you should possibly also be overriding IsForceField().
		// NOTES: The instance behavior is consulted BEFORE type, state and include flags are consulted.  So, for example, even if an instance is set
		//   up to not collide with fixed objects, you will still get CollideObject() calls for that pair.
		virtual bool CollideObjects(Vec::V3Param128 timestep, phInst* myInst, phCollider* myCollider, phInst* otherInst, phCollider* otherCollider, phInstBehavior* otherInstBehavior) = 0;

		virtual void PreComputeImpacts(phContactIterator UNUSED_PARAM(impacts)) {}

		// Called before the instance's PrepareForActivation() is called.  Return false to disallow activation.  Return true to let the phInst
		//   proceed as normal.
		virtual bool ActivateWhenHit() const = 0;

		// PURPOSE: See if this instance behavior exerts forces on objects instead of having rigid collisions.
		// RETURN: true if this instance behavior exerts forces (liquid, explosions), false if it does not or if it uses collisions (rope, cloth)
		virtual bool IsForceField () const { return false; }

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s)
		{
			STRUCT_BEGIN(phInstBehavior);
			STRUCT_FIELD(m_Instance);
			STRUCT_END();
		}
#endif

#if __PFDRAW
		virtual void ProfileDraw() const { }
#endif // __PFDRAW

	protected:
		datRef<phInst> m_Instance;
	};

	phInstBehavior::phInstBehavior()
	{
		m_Instance = NULL;
	}

	phInstBehavior::~phInstBehavior()
	{
	}

	phInst * phInstBehavior::GetInstance() const
	{
		return m_Instance;
	}

	void phInstBehavior::SetInstance(phInst &Instance)
	{
		m_Instance = &Instance;
	}

} // namespace rage

#endif
