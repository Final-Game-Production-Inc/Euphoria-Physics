//
// pheffects/instbehaviorliquid.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_INSTBEHAVIORLIQUID_H
#define PHEFFECTS_INSTBEHAVIORLIQUID_H

#include "math/random.h"
//#include "phbound/boundcomposite.h"
#include "phbound/boundsurface.h"
#include "phbound/liquidimpactset.h"
#include "phbound/surfacegrid.h"
#include "physics/collider.h"
#include "physics/inst.h"
#include "physics/instbehavior.h"
#include "physics/simulator.h"

#define ENABLE_SURFACE_NOISE	1
#define ENABLE_SURFACE_REACTION	1

namespace rage {

#if USE_SURFACES

	// PURPOSE:	This class was added because there may be a need for different
	//			objects in a game that collide with a phInstBehaviorLiquid
	//			to behave differently. This is achieved by the user deriving
	//			from phInstBehaviorLiquid, and overriding
	//			GetCollidingObjectTuning() to return another object than the
	//			default one.
	class phInstBehaviorLiquidCollidingObjectTuning
	{
	public:
		phInstBehaviorLiquidCollidingObjectTuning();

		float	m_DragFactorScale;

		bool	m_EnableBuoyancy;
		bool	m_EnablePushResistance;
		bool	m_EnableSlidingFriction;
		bool	m_EnableSurfaceReaction;
		bool	m_EnableFlow;
	};

	//////////////////////////////////////////////////////////////////////////
	// phInstBehaviorLiquidTuning
	//

	// PURPOSE
	//	By storing the tuning data in a separate class, applications have the option to derive from this 
	//	class with a to have game-specific tunings that work with the automatic parser.
	//	

	class phInstBehaviorLiquidTuning
	{
	public:
		phInstBehaviorLiquidTuning();

		float m_fDampening;
		float m_fPropogationSpeed;
		float m_fInstBuoyancy;
		float m_fPushDrag;
		float m_fHorizPushDragScale;
		float m_fSlideDrag;
		float m_fHorizSlideDragScale;
		float m_fSurfaceNoiseRange;
		int m_iSurfaceNoiseCount;
		float m_fMaxWaveHeight;
		bool m_bEnableSurfaceReaction;
		float m_fSplashScale;
		float m_fMaxSplash;
		float m_fMaxLocalVelocity;
		float m_fConstantPushDrag;
		float m_fConstantSlideDrag;
		Vector3 m_WaterDirection;
		float m_fWaterVelocityScale;
		float m_fDeadFloatForwardScale;
		float m_fDeadFloatToShoreScale;
		float m_fPushDragRisingFactor;
		float m_fPushDragSinkingFactor;
	};

	////////////////////////////////////////////////////////////////
	// phInstBehaviorLiquid

	// PURPOSE
	//   Derivative of phInstBehavior that alters an instance's behavior to cause liquid collisions and physical bound surface animation.
	// NOTES
	//   

	class phInstBehaviorLiquid : public phInstBehavior, public phInstBehaviorLiquidTuning
	{
	public:
		inline phInstBehaviorLiquid() : m_Tune(sm_DefaultLiquidTuning) {}
		inline phInstBehaviorLiquid(phInstBehaviorLiquidTuning& tuneData) : m_Tune(tuneData) {}
		inline ~phInstBehaviorLiquid() {}

		virtual void Reset();

		virtual bool IsActive() const;

		void SetTuningData (const phInstBehaviorLiquidTuning& liquidTuning);

		virtual void Update(float TimeStep);

		virtual bool CollideObjects(Vec::V3Param128 timestep, phInst* myInst, phCollider* myCollider, phInst* otherInst, phCollider* otherCollider, phInstBehavior* otherInstBehavior);

		virtual void NotifyCollidingObject(phInst *otherInst, phCollider *otherCollider) const;
		virtual void NotifyCollidingObjectImpacts(const phLiquidImpactData& impactData, phInst* thisInst, phInst* otherInst, phCollider* otherCollider) const;

		virtual bool CanImpactWater(phBound* otherBound, u32 typeFlags) const;

		// PURPOSE:	Get some tuning values to use for a particular colliding object.
		// RETURNS:	Reference to a phInstBehaviorLiquidCollidingObjectTuning object to use.
		// NOTES:	At this level, this just returns sm_DefaultCollidingObjectTuning, but a
		//			user can override it in a derived class.
		virtual const phInstBehaviorLiquidCollidingObjectTuning &GetCollidingObjectTuning(const phInst *otherInst, const phCollider *otherCollider) const;

		// PURPOSE:	Return the default phInstBehaviorLiquidCollidingObjectTuning object,
		//			which is what GetCollidingObjectTuning() will return if not overridden.
		//			A user can call this to get or set the default tuning values for
		//			colliding objects.
		static phInstBehaviorLiquidCollidingObjectTuning &GetDefaultCollidingObjectTuning()
		{	return sm_DefaultCollidingObjectTuning;	}

		virtual bool ActivateWhenHit() const;

		// <COMBINE: phInstBehavior::IsForceField>
		virtual bool IsForceField () const { return true; }


		// PURPOSE:	Default tuning values for colliding objects.
		static phInstBehaviorLiquidCollidingObjectTuning sm_DefaultCollidingObjectTuning;

		// PURPOSE: Default liquid tuning values
		static phInstBehaviorLiquidTuning sm_DefaultLiquidTuning;

	private:
		// These functions are private because at the moment they are only needed by this class internally
		// and this allows us to implement them in the .cpp while retaining the inline performance.
		inline bool FindLiquidImpacts(phLiquidImpactSet &ImpactSet);
		inline void ComputeAndApplyImpactResponse (const phLiquidImpactData& impactData, phInst* thisInst, phInst* otherInst, phCollider* otherCollider, Vec::V3Param128 timestep) const;

	protected:
		phInstBehaviorLiquidTuning& m_Tune;

	public:
		// Runs after we FindLiquidImpacts but before ComputeAndApplyImpactResponse
		// Games should overload to make special modifications to the impact set or perform updates/notifications
		//   better suited for the whole object rather than the per bound notification that comes in ComputeAndApplyImpactResponse
		virtual void ModifyImpactsNotifyParent (phLiquidImpactSet &UNUSED_PARAM(ImpactSet), phInst* UNUSED_PARAM(thisInst), phInst* UNUSED_PARAM(otherInst), phCollider* UNUSED_PARAM(otherCollider)) { return; }

	};

#endif // USE_SURFACES

} // namespace rage

#endif
