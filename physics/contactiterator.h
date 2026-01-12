//
// physics/impact.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONTACT_ITERATOR_H
#define PHYSICS_CONTACT_ITERATOR_H

#include "collider.h"
#include "contact.h"
#include "manifold.h"
#include "vectormath/classes.h"


namespace rage {


	////////////////////////////////////////////////////////////////
	// phContactIterator

	// PURPOSE: A convenient interface passed back to the user for accessing arrays of impact information
	// <FLAG Component>
	class phContactConstIterator
	{
	public:
		phContactConstIterator ();
		phContactConstIterator (phManifold** manifoldsA, int numManifoldsA, phManifold** manifoldsB, int numManifoldsB, phInst* inst);
		phContactConstIterator (const phContactConstIterator& impact);

		const phManifold& GetRootManifold(int impactNumber) const;
		const phManifold& GetRootManifold() const;
		const phManifold& ComputeManifold(int impactNumber, int compositeManifold) const;
		const phManifold& ComputeManifold() const;
		const phManifold& GetCachedManifold() const;
		const phContact& GetContact() const;

		bool AtEnd() const;
		phContactConstIterator& operator++ ();
		phContactConstIterator operator++ (int);
		phContactConstIterator NextManifold();

		void NextActiveContact();

		int CountElements() const;

		void Reset();
		void ResetToFirstActiveContact();

		phCollider* GetMyCollider () const;
		phCollider* GetOtherCollider () const;

		phInst* GetInstanceA() const;
		phInst* GetInstanceB() const;

		phInst* GetMyInstance () const;
		phInst* GetOtherInstance () const;

		int GetMyElement () const;
		int GetOtherElement () const;
		int GetMyComponent () const;
		int GetOtherComponent () const;
		Vec3V_Out GetMyPosition () const;
		Vec3V_Out GetOtherPosition () const;

		// TODO -- Delete the old vector library versions (CL 2628091)
		void GetMyNormal (Vector3& normal) const;
		void GetOtherNormal (Vector3& normal) const;
		void GetMyNormal (Vec3V_InOut normal) const;
		void GetOtherNormal (Vec3V_InOut normal) const;
		phMaterialMgr::Id GetMyMaterialId() const;
		phMaterialMgr::Id GetOtherMaterialId() const;
		float GetDepth () const;
		ScalarV_Out GetDepthV () const;
		float GetFriction () const;
		float GetElasticity () const;
		bool IsForce () const;
		bool IsConstraint () const;
		void GetImpulse (Vector3& impulse) const;
		void GetMyImpulse (Vector3& impulse) const;
		void GetOtherImpulse (Vector3& impulse) const;

		Vector3 GetRelVelocity () const;
		bool IsDisabled() const;

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		float GetDistToTopSurface() const;
#endif

#if __DEBUGLOG
		void DebugReplay() const;
#endif

	protected:
		void SkipToNextActiveContact(const phManifold* manifold, int contactNumber);
		void SkipToNextContact(const phManifold* manifold, int contactNumber);
		void SkipToNextContactOld();

		phManifold** m_ManifoldsA;
		phManifold** m_ManifoldsB;
		const phManifold* m_CachedManifold;
		phInst* m_Instance;
		int m_ImpactNumber;
		int m_ContactNumber;
		int m_CompositeManifold;
		int m_NumImpactsA;
		int m_NumImpactsB;

		friend class phCachedContactConstIterator;
	};

	class phContactIterator : public phContactConstIterator
	{
	public:
		phContactIterator ();
		phContactIterator (phContact** contactGroup, int numContacts, phInst* inst);
		phContactIterator (phManifold** manifoldsA, int numManifoldsA, phManifold** manifoldsB, int numManifoldsB, phInst* inst);
		phContactIterator (const phContactIterator& impact);

		phManifold& GetRootManifold() const;
		phManifold& ComputeManifold() const;
		phManifold& GetCachedManifold() const;
		phContact& GetContact() const;

		phContactIterator& operator++ ();
		phContactIterator operator++ (int);

		void SetDepth(ScalarV_In depth) const;
		void SetDepth(float depth) const;
		void SetFriction(float friction) const;
		void SetElasticity(float elasticity) const;
		void SetRelVelocity(const Vector3& relVel) const;
// TODO -- Delete the old vector library versions (CL 2628091)
		void SetMyNormal (const Vector3& normal) const;
		void SetOtherNormal (const Vector3& normal) const;
		void SetMyNormal (Vec3V_In normal) const;
		void SetOtherNormal (Vec3V_In normal) const;
		void SetMyPositionLocal (Vec3V_In position) const;
		void SetOtherPositionLocal (Vec3V_In position) const;
		void SetMyPosition (Vec3V_In position) const;
		void SetOtherPosition (Vec3V_In position) const;
		// Legacy functions that don't make lasting changes to the contact (These positions will be overwritten at the next contact refresh)
		void SetMyPositionDoNotUse (Vec3V_In position) const;
		void SetOtherPositionDoNotUse (Vec3V_In position) const;

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		void SetDistToTopSurface(ScalarV_In dist) const;
#endif

		void DisableImpact () const;
		void ClearAllImpacts() const;
		void RemoveContact();

		void RemoveContactWithFinalize();
		void FinalizeRemoveContacts();

		// PURPOSE: Set the post-collision target velocity of this iterator's object in the current contact.
		void SetMyTargetVelocity (Vec3V_In targetVelocity) const;

		// PURPOSE: Modify how the solver looks at the mass of these two objects for this manifold only
		// PARAMS:
		//   myMassInvScale - multiplier for the inverse mass of this object
		//   otherMassInvScale - multiplier for the inverse mass of the other object
		// NOTES:
		//   -The scales should not be negative. 
		//   -An inverse mass of 0 makes an object act as if it has infinite mass. Setting both to 0 is bad.
		//   -The default mass inv scales on a manifold are 1 and 1. 
		void SetMassInvScales(float myMassInvScale, float otherMassInvScale) const;
		void SetMyMassInvScale(float myMassInvScale) const;
		void SetOtherMassInvScale(float otherMassInvScale) const;
	};


	inline phContactConstIterator::phContactConstIterator ()
		: m_ManifoldsA(NULL)
		, m_ManifoldsB(NULL)
		, m_Instance(NULL)
		, m_ImpactNumber(0)
		, m_ContactNumber(0)
		, m_CompositeManifold(-1)
		, m_NumImpactsA(0)
		, m_NumImpactsB(0)
		, m_CachedManifold(NULL)
	{
	}

	inline phContactConstIterator::phContactConstIterator (phManifold** manifoldsA, int numManifoldsA, phManifold** manifoldsB, int numManifoldsB, phInst* inst)
		: m_ManifoldsA(manifoldsA)
		, m_ManifoldsB(manifoldsB)
		, m_Instance(inst)
		, m_ImpactNumber(0)
		, m_ContactNumber(0)
		, m_CompositeManifold(-1)
		, m_NumImpactsA(numManifoldsA)
		, m_NumImpactsB(numManifoldsB)
	{
		m_CachedManifold = &GetRootManifold(0);
		SkipToNextContact(m_CachedManifold, 0);
	}

	inline phContactConstIterator::phContactConstIterator (const phContactConstIterator& other)
		: m_ManifoldsA(other.m_ManifoldsA)
		, m_ManifoldsB(other.m_ManifoldsB)
		, m_Instance(other.m_Instance)
		, m_ImpactNumber(other.m_ImpactNumber)
		, m_ContactNumber(other.m_ContactNumber)
		, m_CompositeManifold(other.m_CompositeManifold)
		, m_NumImpactsA(other.m_NumImpactsA)
		, m_NumImpactsB(other.m_NumImpactsB)
		, m_CachedManifold(other.m_CachedManifold)
	{
	}

	inline int phContactConstIterator::CountElements() const
	{
		int result = 0;

		int numImpactsA = m_NumImpactsA;
		int numImpactsB = m_NumImpactsB;

		for(int rootManifoldIndex = 0; rootManifoldIndex < numImpactsA; ++rootManifoldIndex)
		{
			const phManifold &curRootManifold = *m_ManifoldsA[rootManifoldIndex];
			if(!curRootManifold.CompositeManifoldsEnabled())
			{
				result += curRootManifold.GetNumContacts();
			}
			else
			{
				const int numCompositeManifolds = curRootManifold.GetNumCompositeManifolds();
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < numCompositeManifolds; ++compositeManifoldIndex)
				{
					const phManifold *curCompositeManifold = curRootManifold.GetCompositeManifold(compositeManifoldIndex);
					result += curCompositeManifold->GetNumContacts();
				}
			}
		}

		for(int rootManifoldIndex = 0; rootManifoldIndex < numImpactsB; ++rootManifoldIndex)
		{
			const phManifold &curRootManifold = *m_ManifoldsB[rootManifoldIndex];
			if(!curRootManifold.CompositeManifoldsEnabled())
			{
				result += curRootManifold.GetNumContacts();
			}
			else
			{
				const int numCompositeManifolds = curRootManifold.GetNumCompositeManifolds();
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < numCompositeManifolds; ++compositeManifoldIndex)
				{
					const phManifold *curCompositeManifold = curRootManifold.GetCompositeManifold(compositeManifoldIndex);
					result += curCompositeManifold->GetNumContacts();
				}
			}
		}

		return result;
	}


	__forceinline const phManifold& phContactConstIterator::GetRootManifold(int impactNumber) const
	{
		phManifold* manifold;
		if (impactNumber < m_NumImpactsA)
		{
			TrapLT(impactNumber, 0);
			manifold = m_ManifoldsA[impactNumber];
		}
		else
		{
			TrapGT(impactNumber - m_NumImpactsA, m_NumImpactsB);
			manifold = m_ManifoldsB[impactNumber - m_NumImpactsA];
		}

		return *manifold;
	}

	__forceinline const phManifold& phContactConstIterator::GetRootManifold() const
	{
		return GetRootManifold(m_ImpactNumber);
	}

	__forceinline const phManifold& phContactConstIterator::ComputeManifold(int impactNumber, int compositeManifold) const
	{
		const phManifold& manifold = GetRootManifold(impactNumber);

		if (compositeManifold >= 0)
		{
			return *manifold.GetCompositeManifold(compositeManifold);
		}
		else
		{
			return manifold;
		}
	}

	__forceinline const phManifold& phContactConstIterator::ComputeManifold() const
	{
		return ComputeManifold(m_ImpactNumber, m_CompositeManifold);
	}

	__forceinline const phManifold& phContactConstIterator::GetCachedManifold() const
	{
		return *m_CachedManifold;
	}

	__forceinline const phContact& phContactConstIterator::GetContact() const
	{
		return GetCachedManifold().GetContactPoint(m_ContactNumber);
	}

	inline bool phContactConstIterator::AtEnd() const
	{
		return m_ImpactNumber >= m_NumImpactsA + m_NumImpactsB;
	}

	inline void phContactConstIterator::SkipToNextContact(const phManifold* manifold, int contactNumber)
	{
		bool validContact = contactNumber < manifold->GetNumContacts();

		if(!validContact)
		{
			int rootManifoldIndex = m_ImpactNumber;
			int compositeManifoldIndex = m_CompositeManifold;

			// We've run past the last contact of our previous manifold, let's see if there are new manifolds to explore within this root manifold.
			const phManifold *rootManifold = &GetRootManifold(rootManifoldIndex);
			int numCompositeManifoldsInRoot = rootManifold->GetNumCompositeManifolds();

			do 
			{
				if(++compositeManifoldIndex < numCompositeManifoldsInRoot)
				{
					// We've still got another composite manifold to look at, let's go there.
					manifold = rootManifold->GetCompositeManifold(compositeManifoldIndex);
				}
				else
				{
					// We've run past the last composite manifold in our root manifold, let's see if there is a new root manifold to go to.
					int totalNumRootManifolds = m_NumImpactsA + m_NumImpactsB;
					const bool rootManifoldsLeft = ++rootManifoldIndex < totalNumRootManifolds;
					if(!rootManifoldsLeft)
					{
						// Out of contacts, out of composite manifolds and out of root manifolds.  Nowhere left to go but out.
						m_ImpactNumber = totalNumRootManifolds;
						return;
					}

					rootManifold = &GetRootManifold(rootManifoldIndex);
					numCompositeManifoldsInRoot = rootManifold->GetNumCompositeManifolds();
					// Unfortunately we really do have to branch here because we can't call GetCompositeManifold() if the root manifold isn't composite.
					if(numCompositeManifoldsInRoot > 0)
					{
						// The next root manifold is a composite with sub-manifolds.  Let's move to its first sub-manifold.
						compositeManifoldIndex = 0;
						manifold = rootManifold->GetCompositeManifold(0);
					}
					else
					{
						// The next root manifold is not a composite.
						compositeManifoldIndex = -1;
						manifold = rootManifold;
					}
				}

				// At this point we're at a new manifold that we can check for contacts.  Usually this will fail and we will leave the loop immediately.
			} while (manifold->GetNumContacts() == 0);

			// Write back our changes.
			m_ContactNumber = 0;    // Since we only get to here if we had to move to a new manifold, we know that we'll always be on contact zero.
			m_ImpactNumber = rootManifoldIndex;
			m_CompositeManifold = compositeManifoldIndex;
			m_CachedManifold = manifold;
		}

#if !__OPTIMIZED
		Assert(manifold == &ComputeManifold());
#endif // !__OPTIMIZED
	}


	inline void phContactConstIterator::SkipToNextContactOld()
	{
		const phManifold* manifold = &GetCachedManifold();

		if (!manifold)
		{
			manifold = &ComputeManifold();
		}

#if !__OPTIMIZED
		Assert(manifold == &ComputeManifold());
#endif // !__OPTIMIZED

		int contactNumber = m_ContactNumber;
		int impactNumber = m_ImpactNumber;
		int compositeManifold = m_CompositeManifold;

		// Unless we're at the end, find out whether the current leaf manifold has any contacts left
		int totalNumImpacts = m_NumImpactsA + m_NumImpactsB;
		while (impactNumber < totalNumImpacts && (manifold->CompositeManifoldsEnabled() || contactNumber >= manifold->GetNumContacts()))
		{
			// If the current root manifold is of the composite type...
			const phManifold& rootManifold = GetRootManifold(impactNumber);

			if (rootManifold.CompositeManifoldsEnabled())
			{
				// ...first try to go to the next composite manifold...
				compositeManifold++;
				contactNumber = 0;

				// ...and if that doesn't work, go to the next root manifold.
				if (compositeManifold >= rootManifold.GetNumCompositeManifolds())
				{
					++impactNumber;
					compositeManifold = -1;
					contactNumber = 0;
				}
			}
			else
			{
				// If it's not a composite type manifold, just go to the next root manifold.
				++impactNumber;
				compositeManifold = -1;
				contactNumber = 0;
			}

			manifold = &ComputeManifold(impactNumber, compositeManifold);
		}

		m_ContactNumber = contactNumber;
		m_ImpactNumber = impactNumber;
		m_CompositeManifold = compositeManifold;
		m_CachedManifold = manifold;

#if !__OPTIMIZED
		Assert(manifold == &ComputeManifold());
#endif // !__OPTIMIZED
	}

	inline phContactConstIterator& phContactConstIterator::operator++ ()
	{
		const int newContactNumber = ++m_ContactNumber;

		SkipToNextContact(&GetCachedManifold(), newContactNumber);

		return *this;
	}

	inline phContactConstIterator phContactConstIterator::operator++ (int)
	{
		phContactConstIterator copy(*this);
		++(*this);
		return copy;
	}

	inline phContactConstIterator phContactConstIterator::NextManifold()
	{
		int newContactNumber = GetCachedManifold().GetNumContacts();

		SkipToNextContact(&GetCachedManifold(), newContactNumber);

		return *this;
	}

	inline void phContactConstIterator::SkipToNextActiveContact(const phManifold* manifold, int contactNumber)
	{		
		bool validContact = contactNumber < manifold->GetNumContacts() && manifold->GetContactPoint(contactNumber).IsContactActive();

		if(!validContact)
		{
			int rootManifoldIndex = m_ImpactNumber;
			int compositeManifoldIndex = m_CompositeManifold;

			// The current contact isn't active, we need to iterate until we find an active one. 
			const phManifold *rootManifold = &GetRootManifold(rootManifoldIndex);
			int numCompositeManifoldsInRoot = rootManifold->GetNumCompositeManifolds();

			// Iterate over the contacts in this manifold
			do 
			{
				++contactNumber;

				// If we ran out of contacts keep grabbing manifolds until we find one with contacts. 
				while(contactNumber >= manifold->GetNumContacts())
				{
					// Get the next manifold
					if(++compositeManifoldIndex < numCompositeManifoldsInRoot)
					{
						// We've still got another composite manifold to look at, let's go there.
						manifold = rootManifold->GetCompositeManifold(compositeManifoldIndex);
					}
					else
					{
						// We've run past the last composite manifold in our root manifold, let's see if there is a new root manifold to go to.
						int totalNumRootManifolds = m_NumImpactsA + m_NumImpactsB;
						const bool rootManifoldsLeft = ++rootManifoldIndex < totalNumRootManifolds;
						if(!rootManifoldsLeft)
						{
							// Out of contacts, out of composite manifolds and out of root manifolds.  Nowhere left to go but out.
							m_ImpactNumber = totalNumRootManifolds;
							return;
						}

						rootManifold = &GetRootManifold(rootManifoldIndex);
						numCompositeManifoldsInRoot = rootManifold->GetNumCompositeManifolds();
						// Unfortunately we really do have to branch here because we can't call GetCompositeManifold() if the root manifold isn't composite.
						if(numCompositeManifoldsInRoot > 0)
						{
							// The next root manifold is a composite with sub-manifolds.  Let's move to its first sub-manifold.
							compositeManifoldIndex = 0;
							manifold = rootManifold->GetCompositeManifold(0);
						}
						else
						{
							// The next root manifold is not a composite.
							compositeManifoldIndex = -1;
							manifold = rootManifold;
						}
					}
					contactNumber = 0;
				}
			} while (!manifold->GetContactPoint(contactNumber).IsContactActive());

			// Write back our changes.
			m_ContactNumber = contactNumber;    // Since we only get to here if we had to move to a new manifold, we know that we'll always be on contact zero.
			m_ImpactNumber = rootManifoldIndex;
			m_CompositeManifold = compositeManifoldIndex;
			m_CachedManifold = manifold;
		}

#if !__OPTIMIZED
		Assert(manifold == &ComputeManifold());
#endif // !__OPTIMIZED
	}

	inline void phContactConstIterator::NextActiveContact()
	{
		const int newContactNumber = ++m_ContactNumber;
		SkipToNextActiveContact(&GetCachedManifold(), newContactNumber);
	}

	inline void phContactConstIterator::Reset()
	{
		m_CompositeManifold = -1;
		m_ImpactNumber = 0;
		m_ContactNumber = 0;
		m_CachedManifold = &ComputeManifold();

		SkipToNextContact(m_CachedManifold, 0);
	}

	inline void phContactConstIterator::ResetToFirstActiveContact()
	{
		m_CompositeManifold = -1;
		m_ImpactNumber = 0;
		m_ContactNumber = 0;
		m_CachedManifold = &ComputeManifold();

		SkipToNextActiveContact(m_CachedManifold, 0);
	}

	inline phCollider* phContactConstIterator::GetMyCollider () const
	{
		const phManifold& rootManifold = GetRootManifold();
		return m_Instance == rootManifold.GetInstanceA() ? rootManifold.GetColliderA() : rootManifold.GetColliderB();
	}

	inline phCollider* phContactConstIterator::GetOtherCollider () const
	{
		const phManifold& rootManifold = GetRootManifold();
		return m_Instance == rootManifold.GetInstanceA() ? rootManifold.GetColliderB() : rootManifold.GetColliderA();
	}

	__forceinline phInst* phContactConstIterator::GetInstanceA () const
	{
#if !__OPTIMIZED
		Assert(&GetCachedManifold() == &ComputeManifold());
#endif // !__OPTIMIZED

		return GetCachedManifold().GetInstanceA();
	}

	__forceinline phInst* phContactConstIterator::GetInstanceB () const
	{
#if !__OPTIMIZED
		Assert(&GetCachedManifold() == &ComputeManifold());
#endif // !__OPTIMIZED

		return GetCachedManifold().GetInstanceB();
	}

	__forceinline phInst* phContactConstIterator::GetMyInstance () const
	{
		return m_Instance;
	}

	__forceinline phInst* phContactConstIterator::GetOtherInstance () const
	{
		return m_Instance == GetInstanceA() ? GetInstanceB() : GetInstanceA();
	}

	__forceinline int phContactConstIterator::GetMyElement () const
	{
		const phContact& contact = GetContact();

		return m_Instance == GetInstanceA() ? contact.GetElementA() : contact.GetElementB();
	}

	__forceinline int phContactConstIterator::GetOtherElement () const
	{
		const phContact& contact = GetContact();

		return m_Instance == GetInstanceA() ? contact.GetElementB() : contact.GetElementA();
	}

	__forceinline int phContactConstIterator::GetMyComponent () const
	{
#if !__OPTIMIZED
		Assert(&GetCachedManifold() == &ComputeManifold());
#endif // !__OPTIMIZED

		const phManifold& manifold = GetCachedManifold();

		return m_Instance == manifold.GetInstanceA() ? manifold.GetComponentA() : manifold.GetComponentB();
	}

	__forceinline int phContactConstIterator::GetOtherComponent () const
	{
#if !__OPTIMIZED
		Assert(&GetCachedManifold() == &ComputeManifold());
#endif // !__OPTIMIZED

		const phManifold& manifold = GetCachedManifold();

		return m_Instance == manifold.GetInstanceA() ? manifold.GetComponentB() : manifold.GetComponentA();
	}

	__forceinline Vec3V_Out phContactConstIterator::GetMyPosition () const
	{
		const phContact& contact = GetContact();

		return m_Instance == GetInstanceA() ? contact.GetWorldPosA() : contact.GetWorldPosB();
	}

	__forceinline Vec3V_Out phContactConstIterator::GetOtherPosition () const
	{
		const phContact& contact = GetContact();

		return m_Instance == GetInstanceA() ? contact.GetWorldPosB() : contact.GetWorldPosA();
	}

// TODO -- Delete the old vector library versions (CL 2628091)
	__forceinline void phContactConstIterator::GetMyNormal (Vector3& normal) const
	{
		const phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			normal.Set(VEC3V_TO_VECTOR3(contact.GetWorldNormal()));
		}
		else
		{
			normal.Negate(VEC3V_TO_VECTOR3(contact.GetWorldNormal()));
		}
	}

	__forceinline void phContactConstIterator::GetOtherNormal (Vector3& normal) const
	{
		const phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			normal.Negate(VEC3V_TO_VECTOR3(contact.GetWorldNormal()));
		}
		else
		{
			normal.Set(VEC3V_TO_VECTOR3(contact.GetWorldNormal()));
		}
	}

	__forceinline void phContactConstIterator::GetMyNormal (Vec3V_InOut normal) const
	{
		const phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			normal = contact.GetWorldNormal();
		}
		else
		{
			normal = Negate(contact.GetWorldNormal());
		}
	}

	__forceinline void phContactConstIterator::GetOtherNormal (Vec3V_InOut normal) const
	{
		const phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			normal = Negate(contact.GetWorldNormal());
		}
		else
		{
			normal = contact.GetWorldNormal();
		}
	}

	__forceinline phMaterialMgr::Id phContactConstIterator::GetMyMaterialId() const
	{
		return GetContact().GetMaterialId(m_Instance == GetInstanceA() ? 0 : 1);
	}

	__forceinline phMaterialMgr::Id phContactConstIterator::GetOtherMaterialId() const
	{
		return GetContact().GetMaterialId(m_Instance == GetInstanceA() ? 1 : 0);
	}

	__forceinline float phContactConstIterator::GetDepth () const
	{
		return GetContact().GetDepth();
	}

	__forceinline ScalarV_Out phContactConstIterator::GetDepthV () const
	{
		return GetContact().GetDepthV();
	}

	__forceinline float phContactConstIterator::GetFriction () const
	{
		return GetContact().GetFriction();
	}

	__forceinline float phContactConstIterator::GetElasticity () const
	{
		return GetContact().GetElasticity();
	}

	__forceinline bool phContactConstIterator::IsForce () const
	{
		return false;
	}

	__forceinline bool phContactConstIterator::IsConstraint () const
	{
#if !__OPTIMIZED
		Assert(&GetCachedManifold() == &ComputeManifold());
#endif // !__OPTIMIZED

		return GetCachedManifold().IsConstraint();
	}

	inline void phContactConstIterator::GetImpulse (Vector3& impulse) const
	{
#if !__OPTIMIZED
		Assert(&GetCachedManifold() == &ComputeManifold());
#endif // !__OPTIMIZED

		const phContact& contact = GetContact();

		Vec3V contactImpulse;

		if(GetCachedManifold().IsArticulatedFixedCollision())
		{
			contactImpulse = contact.ComputeTotalArtFixImpulse();
		}
		else
		{
			contactImpulse = contact.ComputeTotalImpulse();
		}

		impulse.Set(VEC3V_TO_VECTOR3(contactImpulse));
	}

	__forceinline void phContactConstIterator::GetMyImpulse (Vector3& impulse) const
	{
		Vector3 contactImpulse; 
		GetImpulse(contactImpulse);

		if (m_Instance == GetInstanceA())
		{
			impulse.Set(contactImpulse);
		}
		else
		{
			impulse.Negate(contactImpulse);
		}
	}


	__forceinline void phContactConstIterator::GetOtherImpulse (Vector3& impulse) const
	{
		Vector3 contactImpulse; 
		GetImpulse(contactImpulse);

		if (m_Instance == GetInstanceA())
		{
			impulse.Negate(contactImpulse);
		}
		else
		{
			impulse.Set(contactImpulse);
		}
	}

	__forceinline Vector3 phContactConstIterator::GetRelVelocity () const
	{
		const phContact& contact = GetContact();
		return VEC3V_TO_VECTOR3(contact.ComputeRelVelocity( contact.GetWorldPosA(), contact.GetWorldPosB(), &GetCachedManifold() ));
	}

	__forceinline bool phContactConstIterator::IsDisabled() const
	{
		return !GetContact().IsContactActive();
	}

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
	__forceinline float phContactConstIterator::GetDistToTopSurface() const
	{
		const phContact& contact = GetContact();
		return contact.GetDistToTopSurface().Getf();
	}

	__forceinline void phContactIterator::SetDistToTopSurface(ScalarV_In dist) const
	{
		phContact& contact = GetContact();
		contact.SetDistToTopSurface(dist);
	}
#endif

#if __DEBUGLOG
	inline void phContactConstIterator::DebugReplay() const
	{
		GetContact().DebugReplay();
	}
#endif

	inline phContactIterator::phContactIterator ()
	{
	}

	inline phContactIterator::phContactIterator (phManifold** manifoldsA, int numManifoldsA, phManifold** manifoldsB, int numManifoldsB, phInst* inst)
		: phContactConstIterator(manifoldsA, numManifoldsA, manifoldsB, numManifoldsB, inst)
	{
	}

	inline phContactIterator::phContactIterator (const phContactIterator& other)
		: phContactConstIterator(other)
	{
	}

	__forceinline phContactIterator& phContactIterator::operator++ ()
	{
		return static_cast<phContactIterator&>(phContactConstIterator::operator++());
	}

	__forceinline phContactIterator phContactIterator::operator++ (int)
	{
		phContactIterator copy(*this);
		++(*this);
		return copy;
	}

	__forceinline phContact& phContactIterator::GetContact() const
	{
		const phContact& contact = phContactConstIterator::GetContact();

		return const_cast<phContact&>(contact);
	}

	__forceinline phManifold& phContactIterator::GetCachedManifold() const
	{
		const phManifold& manifold = phContactConstIterator::GetCachedManifold();

		return const_cast<phManifold&>(manifold);
	}

	__forceinline phManifold& phContactIterator::ComputeManifold() const
	{
		const phManifold& manifold = phContactConstIterator::ComputeManifold();

		return const_cast<phManifold&>(manifold);
	}

	__forceinline phManifold& phContactIterator::GetRootManifold() const
	{
		const phManifold& contact = phContactConstIterator::GetRootManifold();

		return const_cast<phManifold&>(contact);
	}

	__forceinline void phContactIterator::SetDepth (ScalarV_In depth) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateDepth(depth.Getf()));
		GetContact().SetDepth(depth);
	}

	__forceinline void phContactIterator::SetDepth (float depth) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateDepth(depth));
		GetContact().SetDepth(depth);
	}

	__forceinline void phContactIterator::SetFriction (float friction) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateFriction(friction));
		GetContact().SetFriction(friction);
	}

	__forceinline void phContactIterator::SetElasticity (float elasticity) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateElasticity(elasticity));
		GetContact().SetElasticity(elasticity);
	}

// TODO CONTACT_VALIDATION_ONLY Delete the old vector library versions (CL 2628091)
	__forceinline void phContactIterator::SetMyNormal (const Vector3& normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(RCC_VEC3V(normal)));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldNormal(RCC_VEC3V(normal));
		}
		else
		{
			contact.SetWorldNormal(-RCC_VEC3V(normal));
		}
	}

	__forceinline void phContactIterator::SetOtherNormal (const Vector3& normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(RCC_VEC3V(normal)));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldNormal(-RCC_VEC3V(normal));
		}
		else
		{
			contact.SetWorldNormal(RCC_VEC3V(normal));
		}
	}

	__forceinline void phContactIterator::SetMyNormal (Vec3V_In normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(normal));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldNormal(normal);
		}
		else
		{
			contact.SetWorldNormal(-normal);
		}
	}

	__forceinline void phContactIterator::SetOtherNormal (Vec3V_In normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(normal));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldNormal(-normal);
		}
		else
		{
			contact.SetWorldNormal(normal);
		}
	}


	__forceinline void phContactIterator::SetMyPositionLocal (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();
		if (m_Instance == GetInstanceA())
		{
			contact.SetLocalPosA(position);
			contact.ComputeWorldPointAFromLocal(m_CachedManifold);
		}
		else
		{
			contact.SetLocalPosB(position);
			contact.ComputeWorldPointBFromLocal(m_CachedManifold);
		}
	}

	__forceinline void phContactIterator::SetOtherPositionLocal (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetLocalPosB(position);
			contact.ComputeWorldPointBFromLocal(m_CachedManifold);
		}
		else
		{
			contact.SetLocalPosA(position);
			contact.ComputeWorldPointAFromLocal(m_CachedManifold);
		}
	}


	__forceinline void phContactIterator::SetMyPosition (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldPosA(position);
			contact.ComputeLocalPointAFromWorld(m_CachedManifold);
		}
		else
		{
			contact.SetWorldPosB(position);
			contact.ComputeLocalPointBFromWorld(m_CachedManifold);
		}
	}

	__forceinline void phContactIterator::SetOtherPosition (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldPosB(position);
			contact.ComputeLocalPointBFromWorld(m_CachedManifold);
		}
		else
		{
			contact.SetWorldPosA(position);
			contact.ComputeLocalPointAFromWorld(m_CachedManifold);
		}
	}

	__forceinline void phContactIterator::SetMyPositionDoNotUse (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldPosA(position);
		}
		else
		{
			contact.SetWorldPosB(position);
		}
	}

	__forceinline void phContactIterator::SetOtherPositionDoNotUse (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_Instance == GetInstanceA())
		{
			contact.SetWorldPosB(position);
		}
		else
		{
			contact.SetWorldPosA(position);
		}
	}

	__forceinline void phContactIterator::SetMyTargetVelocity (Vec3V_In targetVelocity) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateTargetVelocity(targetVelocity));
		phContact& contact = GetContact();
		if (m_Instance == GetInstanceA())
		{
			contact.SetTargetRelVelocity(targetVelocity);
		}
		else
		{
			Vec3V negatedTargetVelocity = Negate(targetVelocity);
			contact.SetTargetRelVelocity(negatedTargetVelocity);
		}
	}

	__forceinline void phContactIterator::SetMassInvScales(float myMassInvScale, float otherMassInvScale) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateMassInvScales(myMassInvScale,otherMassInvScale));
		u32 myObjectindex = (GetMyInstance() == GetCachedManifold().GetInstanceA() ? 0 : 1);
		u32 otherObjectIndex = 1 - myObjectindex;
		GetCachedManifold().SetMassInvScale(myObjectindex,myMassInvScale);
		GetCachedManifold().SetMassInvScale(otherObjectIndex,otherMassInvScale);
	}

	__forceinline void phContactIterator::SetMyMassInvScale(float myMassInvScale) const
	{
		u32 myObjectIndex = (GetMyInstance() == GetCachedManifold().GetInstanceA() ? 0 : 1);
		GetCachedManifold().SetMassInvScale(myObjectIndex,myMassInvScale);
		CONTACT_VALIDATION_ONLY(u32 otherObjectIndex = 1 - myObjectIndex);
		CONTACT_VALIDATION_ONLY(phContact::ValidateMassInvScales(myMassInvScale,GetCachedManifold().GetMassInvScale(otherObjectIndex)));
	}

	__forceinline void phContactIterator::SetOtherMassInvScale(float otherMassInvScale) const
	{
		u32 otherObjectIndex = (GetMyInstance() == GetCachedManifold().GetInstanceA() ? 1 : 0);
		GetCachedManifold().SetMassInvScale(otherObjectIndex,otherMassInvScale);
		CONTACT_VALIDATION_ONLY(u32 myObjectIndex = 1 - otherObjectIndex);
		CONTACT_VALIDATION_ONLY(phContact::ValidateMassInvScales(GetCachedManifold().GetMassInvScale(myObjectIndex),otherMassInvScale));
	}

	__forceinline void phContactIterator::DisableImpact () const
	{
		GetContact().ActivateContact(false);
	}

	__forceinline void phContactIterator::ClearAllImpacts() const
	{
		GetCachedManifold().RemoveAllContacts();
	}

	__forceinline void phContactIterator::RemoveContact()
	{
		GetCachedManifold().RemoveContactPoint(m_ContactNumber);

#if __PPU
		// Rebuild the DMA plan because, presuming we're doing this in PreComputeImpacts,
		// the DMA plan has already been generated and removing a contact invalidates it.
		phManifold& cachedManifold = GetCachedManifold();
		cachedManifold.RegenerateDmaPlan();
		if (m_CompositeManifold == 0)
		{
			phManifold& rootManifold = GetRootManifold();
			if (&rootManifold != &cachedManifold)
			{
				rootManifold.RegenerateDmaPlan();
			}
		}
#endif // __PPU

		--m_ContactNumber;
	}

	__forceinline void phContactIterator::RemoveContactWithFinalize()
	{
		GetCachedManifold().RemoveContactPoint(m_ContactNumber);

		--m_ContactNumber;
	}

	__forceinline void phContactIterator::FinalizeRemoveContacts()
	{
#if __PPU
		// Rebuild the DMA plan because, presuming we're doing this in PreComputeImpacts,
		// the DMA plan has already been generated and removing a contact invalidates it.
		phManifold& cachedManifold = GetCachedManifold();
		cachedManifold.RegenerateDmaPlan();
		if (m_CompositeManifold == 0)
		{
			phManifold& rootManifold = GetRootManifold();
			if (&rootManifold != &cachedManifold)
			{
				rootManifold.RegenerateDmaPlan();
			}
		}
#endif // __PPU
	}

	/** PURPOSE: Helper structure for phCachedContactConstIterator. Each element of this
	 *  type represents one iteration with a normal phContactConstIterator.
	 */
	struct phCachedContactIteratorElement
	{
		Vec3V m_MyPosition;
		Vec3V m_OtherPosition;
		Vec3V m_MyNormal;

		const phManifold* m_CachedManifold;
		const phManifold* m_RootManifold;
		phContact *m_Contact;
		int m_CompositeManifold;

		u16 m_MyElement;
		u16 m_OtherElement;
		u8 m_MyComponent;
		u8 m_OtherComponent;


		bool m_Disabled;
		bool m_IsInstanceA;
		bool m_Removed;
		bool m_IsConstraint;
	};

	/** PURPOSE: This is a version of phContactIterator that creates a cache of all contact points up front
	 *  and iterates over this cache rather than looking up the data at run-time. The idea is to greatly
	 *  reduce the number of L2 cache misses, as well as cheapen iteration by not calculating the next
	 *  manifold after each iteration.
	 *
	 *  Using this cached iterator comes at the cost of creating the cache up front, so it is usually best
	 *  suited for situations where contacts are iterated over more than once.
	 *  
	 *  This class is created by initializing using a normal phContact(Const)Iterator as the source. It can
	 *  then be used as a drop-in replacement, it has all the member functions of phContactConstIterator.
	 *
	 *  To create it, first create an instance, then create an array of phCachedContactConstElement objects with
	 *  as many elements as the source iterator has (use CountElements() to get this number). It is typically
	 *  a good idea to allocate this array using Alloca(). The cached iterator is then initialized using
	 *  InitFromIterator(), after which it can be used like a normal non-cached iterator.
	 */
	class phCachedContactConstIterator
	{
	private:
		// Don't try to use the copy constructor. Don't pass this by value!
		phCachedContactConstIterator(phCachedContactConstIterator &) : m_Elements(NULL, 0) {}

	public:
		phCachedContactConstIterator() : m_Elements(NULL, 0) {}

		bool AtEnd() const				{ return (size_t) m_CurrentElement >= (size_t) m_LastElement; }

		const phManifold& GetRootManifold(int impactNumber) const;
		const phManifold& GetRootManifold() const;
		const phManifold& ComputeManifold(int impactNumber, int compositeManifold) const;
		//const phManifold& ComputeManifold() const;
		const phManifold& GetCachedManifold() const;
		const phContact& GetContact() const;

		phCachedContactConstIterator& operator++ ();

		phCachedContactIteratorElement * NextActiveContact();
		phCachedContactIteratorElement * NextContact();
		phCachedContactIteratorElement * NextManifold();

	private:
		phCachedContactIteratorElement * SkipToNextContact(const phManifold* manifold, int contactNumber);
		phCachedContactIteratorElement * SkipToNextContact();

		// DO NOT USE THE POSTFIX OPERATOR! IT CREATES BAD CODE!
		// Please write "++iterator" rather than "iterator++".
		phCachedContactConstIterator operator++ (int);
	public:

		phCachedContactIteratorElement * Reset();

		phCachedContactIteratorElement * ResetToFirstActiveContact();

		phCollider* GetMyCollider () const;
		phCollider* GetOtherCollider () const;

		phInst* GetInstanceA() const;
		phInst* GetInstanceB() const;

		phInst* GetMyInstance () const;
		phInst* GetOtherInstance () const;

		int GetMyElement () const					{ return (int) m_CurrentElement->m_MyElement; }
		int GetOtherElement () const				{ return (int) m_CurrentElement->m_OtherElement; }
		int GetMyComponent () const					{ return (int) m_CurrentElement->m_MyComponent; }
		int GetOtherComponent () const				{ return (int) m_CurrentElement->m_OtherComponent; }
		Vec3V_Out GetMyPosition () const;
		Vec3V_Out GetOtherPosition () const;

// TODO -- Delete the old vector library versions (CL 2628091)
		void GetMyNormal (Vector3& normal) const;
		void GetOtherNormal (Vector3& normal) const;
		void GetMyNormal (Vec3V_InOut normal) const;
		void GetOtherNormal (Vec3V_InOut normal) const;
		phMaterialMgr::Id GetMyMaterialId() const;
		phMaterialMgr::Id GetOtherMaterialId() const;
		float GetDepth () const;
		ScalarV_Out GetDepthV () const;
		float GetFriction () const;
		float GetElasticity () const;
		bool IsForce () const;
		bool IsConstraint () const;
		void GetImpulse (Vector3& impulse) const;
		void GetMyImpulse (Vector3& impulse) const;
		void GetOtherImpulse (Vector3& impulse) const;

		Vec3V_Out GetRelVelocity () const;
		bool IsDisabled() const						{ return m_CurrentElement->m_Disabled; }

		/** PURPOSE: Initialize this cached iterator. This function will go through the source iterator and
		 *  populate the cache.
		 * 
		 *  PARAMS:
		 *   iterator - source iterator to populate this cache with
		 *   buffer - User-provided buffer, must contain "count" elements. The user is responsible for its destruction.
		 *   count - Number of elements in the iterator. Can be obtained via phContactConstIterator::CountElements().
		 */
		void InitFromIterator(phContactConstIterator &iterator, phCachedContactIteratorElement *buffer, int count);

	protected:
		bool IsRemoved() const						{ return m_CurrentElement->m_Removed; }

		void AddAllInManifold(const phManifold &rootManifold, const phManifold &manifold, int compositeManifold, bool isInstanceA, phCachedContactIteratorElement **elementPtr);

		phInst* m_Instance;
		phManifold** m_ManifoldsA;
		phManifold** m_ManifoldsB;
		int m_NumImpactsA;
		int m_NumImpactsB;

		phCachedContactIteratorElement *m_CurrentElement;

		// This is a pointer to the end of the buffer, i.e. outside the valid range.
		phCachedContactIteratorElement *m_LastElement;
		atUserArray<phCachedContactIteratorElement> m_Elements;
	};

	__forceinline const phManifold& phCachedContactConstIterator::GetRootManifold(int impactNumber) const
	{
		{
			phManifold* manifold;
			if (impactNumber < m_NumImpactsA)
			{
				TrapLT(impactNumber, 0);
				manifold = m_ManifoldsA[impactNumber];
			}
			else
			{
				TrapGT(impactNumber - m_NumImpactsA, m_NumImpactsB);
				manifold = m_ManifoldsB[impactNumber - m_NumImpactsA];
			}

			return *manifold;
		}
	}

	__forceinline const phManifold& phCachedContactConstIterator::GetRootManifold() const
	{
		return *m_CurrentElement->m_RootManifold;
	}

	__forceinline const phManifold& phCachedContactConstIterator::ComputeManifold(int impactNumber, int compositeManifold) const
	{
		const phManifold& manifold = GetRootManifold(impactNumber);

		if (compositeManifold >= 0)
		{
			return *manifold.GetCompositeManifold(compositeManifold);
		}
		else
		{
			return manifold;
		}
	}

	__forceinline const phManifold& phCachedContactConstIterator::GetCachedManifold() const
	{
		return *m_CurrentElement->m_CachedManifold;
	}

	__forceinline const phContact& phCachedContactConstIterator::GetContact() const
	{
		return *m_CurrentElement->m_Contact;
	}

	__forceinline phCachedContactIteratorElement * phCachedContactConstIterator::SkipToNextContact(const phManifold* /*manifold*/, int /*contactNumber*/)
	{
		return SkipToNextContact();
	}

	__forceinline phCachedContactIteratorElement * phCachedContactConstIterator::SkipToNextContact()
	{
		phCachedContactIteratorElement *element = m_CurrentElement;
		size_t lastElement = (size_t) m_LastElement;

		do 
		{
			element++;
		}
		while ((size_t) element < lastElement && element->m_Removed);

		PrefetchDC(element + 1);
		m_CurrentElement = element;
		return ((size_t) element < lastElement) ? element : NULL;
	}

	__forceinline phCachedContactIteratorElement * phCachedContactConstIterator::NextContact()
	{
		return SkipToNextContact();
	}

	__forceinline phCachedContactIteratorElement * phCachedContactConstIterator::NextManifold()
	{
		phCachedContactIteratorElement * element = m_CurrentElement;
		const phManifold* currentManfold = element->m_CachedManifold;

		// We can't just skip forward by the number of contacts in the manifold because the user could be in the middle of a manifold. It also gets complicated
		//   when the user is deleting contacts (in the non-const version at least). 
		size_t lastElement = (size_t) m_LastElement;
		do 
		{
			element++;
		}
		while ((size_t) element < lastElement && (element->m_Removed || currentManfold == element->m_CachedManifold));
		m_CurrentElement = element;
		return ((size_t) element < lastElement) ? element : NULL;
	}

	__forceinline phCachedContactConstIterator& phCachedContactConstIterator::operator++ ()
	{
		SkipToNextContact();
		return *this;
	}

	__forceinline phCachedContactConstIterator phCachedContactConstIterator::operator++ (int)
	{
		phCachedContactConstIterator copy(*this);
		++(*this);
		return copy;
	}

	__forceinline phCachedContactIteratorElement *  phCachedContactConstIterator::NextActiveContact()
	{
		phCachedContactIteratorElement *element = m_CurrentElement;
		size_t lastElement = (size_t) m_LastElement;

		do 
		{
			element++;
		}
		while ((size_t) element < lastElement && (element->m_Removed || element->m_Disabled));

		PrefetchDC(element + 1);
		m_CurrentElement = element;
		return ((size_t) element < lastElement) ? element : NULL;
	}

	__forceinline phCachedContactIteratorElement * phCachedContactConstIterator::Reset()
	{
		size_t lastElement = (size_t) m_LastElement;
		phCachedContactIteratorElement * element = m_Elements.GetElements();
		m_CurrentElement = element;
		if(!AtEnd() && IsRemoved())
		{
			return SkipToNextContact();
		}
		return ((size_t) element < lastElement) ? element : NULL;
	}

	__forceinline phCachedContactIteratorElement * phCachedContactConstIterator::ResetToFirstActiveContact()
	{
		size_t lastElement = (size_t) m_LastElement;
		phCachedContactIteratorElement * element = m_Elements.GetElements();
		m_CurrentElement = element;
		if(!AtEnd() && (IsDisabled() || IsRemoved()))
		{
			return NextActiveContact();
		}
		return ((size_t) element < lastElement) ? element : NULL;
	}

	__forceinline phCollider* phCachedContactConstIterator::GetMyCollider () const
	{
		return (m_CurrentElement->m_IsInstanceA) ? GetRootManifold().GetColliderA() : GetRootManifold().GetColliderB();
	}

	__forceinline phCollider* phCachedContactConstIterator::GetOtherCollider () const
	{
		return (m_CurrentElement->m_IsInstanceA) ? GetRootManifold().GetColliderB() : GetRootManifold().GetColliderA();
	}

	__forceinline phInst* phCachedContactConstIterator::GetInstanceA() const
	{
		return GetCachedManifold().GetInstanceA();
	}

	__forceinline phInst* phCachedContactConstIterator::GetInstanceB() const
	{
		return GetCachedManifold().GetInstanceB();
	}

	__forceinline phInst* phCachedContactConstIterator::GetMyInstance () const
	{
		return m_Instance;
	}

	__forceinline phInst* phCachedContactConstIterator::GetOtherInstance () const
	{
		return (m_CurrentElement->m_IsInstanceA) ? GetInstanceB() : GetInstanceA();
	}

// TODO -- Delete the old vector library versions (CL 2628091)
	__forceinline void phCachedContactConstIterator::GetMyNormal (Vector3& normal) const
	{
		normal.Set(RCC_VECTOR3(m_CurrentElement->m_MyNormal));
	}

	__forceinline void phCachedContactConstIterator::GetOtherNormal (Vector3& normal) const
	{
		normal.Negate(RCC_VECTOR3(m_CurrentElement->m_MyNormal));
	}

	__forceinline Vec3V_Out phCachedContactConstIterator::GetMyPosition () const
	{
		return m_CurrentElement->m_MyPosition;
	}

	__forceinline Vec3V_Out phCachedContactConstIterator::GetOtherPosition () const
	{
		return m_CurrentElement->m_OtherPosition;
	}

	__forceinline void phCachedContactConstIterator::GetMyNormal (Vec3V_InOut normal) const
	{
		normal = m_CurrentElement->m_MyNormal;
	}

	__forceinline void phCachedContactConstIterator::GetOtherNormal (Vec3V_InOut normal) const
	{
		normal = Negate(m_CurrentElement->m_MyNormal);
	}

	__forceinline phMaterialMgr::Id phCachedContactConstIterator::GetMyMaterialId() const
	{
		const phContact& contact = GetContact();

		return contact.GetMaterialId(m_CurrentElement->m_IsInstanceA ? 0 : 1);
	}

	__forceinline phMaterialMgr::Id phCachedContactConstIterator::GetOtherMaterialId() const
	{
		const phContact& contact = GetContact();

		return contact.GetMaterialId(m_CurrentElement->m_IsInstanceA ? 1 : 0);
	}

	__forceinline float phCachedContactConstIterator::GetDepth () const
	{
		return GetContact().GetDepth();
	}

	__forceinline ScalarV_Out phCachedContactConstIterator::GetDepthV () const
	{
		return GetContact().GetDepthV();
	}

	__forceinline float phCachedContactConstIterator::GetFriction () const
	{
		return GetContact().GetFriction();
	}

	__forceinline float phCachedContactConstIterator::GetElasticity () const
	{
		return GetContact().GetElasticity();
	}

	__forceinline bool phCachedContactConstIterator::IsForce () const
	{
		return false;
	}

	__forceinline bool phCachedContactConstIterator::IsConstraint () const
	{
		return m_CurrentElement->m_IsConstraint;
	}

	__forceinline void phCachedContactConstIterator::GetImpulse (Vector3& impulse) const
	{
		const phContact& contact = GetContact();
		Vec3V contactImpulse;

		if(GetCachedManifold().IsArticulatedFixedCollision())
		{
			contactImpulse = contact.ComputeTotalArtFixImpulse();
		}
		else
		{
			contactImpulse = contact.ComputeTotalImpulse();
		}

		impulse.Set(VEC3V_TO_VECTOR3(contactImpulse));
	}

	__forceinline void phCachedContactConstIterator::GetMyImpulse (Vector3& impulse) const
	{
		Vector3 contactImpulse; 
		GetImpulse(contactImpulse);

		if (m_CurrentElement->m_IsInstanceA)
		{
			impulse.Set(contactImpulse);
		}
		else
		{
			impulse.Negate(contactImpulse);
		}
	}

	__forceinline void phCachedContactConstIterator::GetOtherImpulse (Vector3& impulse) const
	{
		Vector3 contactImpulse; 
		GetImpulse(contactImpulse);

		if (m_CurrentElement->m_IsInstanceA)
		{
			impulse.Negate(contactImpulse);
		}
		else
		{
			impulse.Set(contactImpulse);
		}
	}

	__forceinline Vec3V_Out phCachedContactConstIterator::GetRelVelocity () const
	{
		const phContact& contact = GetContact();
		return contact.ComputeRelVelocity( contact.GetWorldPosA(), contact.GetWorldPosB(), &GetCachedManifold() );
	}

	__forceinline void phCachedContactConstIterator::AddAllInManifold(const phManifold &rootManifold, const phManifold &manifold, int compositeManifold, bool isInstanceA, phCachedContactIteratorElement **elementPtr)
	{
		int contacts = manifold.GetNumContacts();
		phCachedContactIteratorElement *element = *elementPtr;

		for (int x=0; x<contacts; x++)
		{
			const phContact &contact = manifold.GetContactPoint(x);

			if (isInstanceA)
			{
				element->m_MyPosition = contact.GetWorldPosA();
				element->m_OtherPosition = contact.GetWorldPosB();
				element->m_MyNormal = contact.GetWorldNormal();
				element->m_MyElement = (u16) contact.GetElementA();
				element->m_OtherElement = (u16) contact.GetElementB();
				element->m_MyComponent = (u8) manifold.GetComponentA();
				element->m_OtherComponent = (u8) manifold.GetComponentB();
			}
			else
			{
				element->m_MyPosition = contact.GetWorldPosB();
				element->m_OtherPosition = contact.GetWorldPosA();
				element->m_MyNormal = Negate(contact.GetWorldNormal());
				element->m_MyElement = (u16) contact.GetElementB();
				element->m_OtherElement = (u16) contact.GetElementA();
				element->m_MyComponent = (u8) manifold.GetComponentB();
				element->m_OtherComponent = (u8) manifold.GetComponentA();
			}

			element->m_CachedManifold = &manifold;
			element->m_RootManifold = &rootManifold;
			element->m_Contact = const_cast<phContact *>(&contact);
			element->m_CompositeManifold = compositeManifold;

			element->m_Disabled = !contact.IsContactActive();
			element->m_IsInstanceA = isInstanceA;
			element->m_Removed = false;
			element->m_IsConstraint = manifold.IsConstraint();

			element++;
		}

		*elementPtr = element;
	}

	inline void phCachedContactConstIterator::InitFromIterator(phContactConstIterator &iterator, phCachedContactIteratorElement *buffer, int count)
	{
		// Don't initialize the array twice.
		Assert(m_Elements.GetElements() == NULL);
		::new (&m_Elements) atUserArray<phCachedContactIteratorElement>(buffer, (u16) count, true);

		phCachedContactIteratorElement *element = m_Elements.GetElements();
		phInst *instance = iterator.GetMyInstance();

		m_Instance = instance;
		m_CurrentElement = element;
		m_LastElement = element + count;
		m_ManifoldsA = iterator.m_ManifoldsA;
		m_ManifoldsB = iterator.m_ManifoldsB;
		m_NumImpactsA = iterator.m_NumImpactsA;
		m_NumImpactsB = iterator.m_NumImpactsB;


		int result = 0;

		int numImpactsA = m_NumImpactsA;
		int numImpactsB = m_NumImpactsB;

		for(int rootManifoldIndex = 0; rootManifoldIndex < numImpactsA; ++rootManifoldIndex)
		{
			const phManifold &curRootManifold = *m_ManifoldsA[rootManifoldIndex];
			if(!curRootManifold.CompositeManifoldsEnabled())
			{
				AddAllInManifold(curRootManifold, curRootManifold, -1, true, &element);
				result += curRootManifold.GetNumContacts();
			}
			else
			{
				const int numCompositeManifolds = curRootManifold.GetNumCompositeManifolds();
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < numCompositeManifolds; ++compositeManifoldIndex)
				{
					const phManifold &curCompositeManifold = *(curRootManifold.GetCompositeManifold(compositeManifoldIndex));
					AddAllInManifold(curRootManifold, curCompositeManifold, compositeManifoldIndex, true, &element);
				}
			}
		}

		for(int rootManifoldIndex = 0; rootManifoldIndex < numImpactsB; ++rootManifoldIndex)
		{
			const phManifold &curRootManifold = *m_ManifoldsB[rootManifoldIndex];
			if(!curRootManifold.CompositeManifoldsEnabled())
			{
				AddAllInManifold(curRootManifold, curRootManifold, -1, false, &element);
			}
			else
			{
				const int numCompositeManifolds = curRootManifold.GetNumCompositeManifolds();
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < numCompositeManifolds; ++compositeManifoldIndex)
				{
					const phManifold &curCompositeManifold = *(curRootManifold.GetCompositeManifold(compositeManifoldIndex));
					AddAllInManifold(curRootManifold, curCompositeManifold, compositeManifoldIndex, false, &element);
				}
			}
		}

/*
		for (int x=0; x<count; x++)
		{
			element->m_MyPosition = VECTOR3_TO_VEC3V(iterator.GetMyPosition());
			element->m_OtherPosition = VECTOR3_TO_VEC3V(iterator.GetOtherPosition());
			iterator.GetMyNormal(RC_VECTOR3(element->m_MyNormal));

			element->m_CachedManifold = &iterator.GetCachedManifold();
			element->m_RootManifold = &iterator.GetRootManifold();
			element->m_Contact = const_cast<phContact *>(&iterator.GetContact());
			element->m_CompositeManifold = iterator.m_CompositeManifold;

			Assign(element->m_MyElement, iterator.GetMyElement());
			Assign(element->m_OtherElement, iterator.GetOtherElement());
			Assign(element->m_MyComponent, iterator.GetMyComponent());
			Assign(element->m_OtherComponent, iterator.GetOtherComponent());

			element->m_Disabled = iterator.IsDisabled();
			element->m_IsInstanceA = iterator.GetInstanceA() == instance;
			element->m_Removed = false;
			element->m_IsConstraint = iterator.IsConstraint();

			element++;
			iterator++;
		}
		*/
	}

	/** PURPOSE: Non-const version of phCachedContactConstIterator.
	 */
	class phCachedContactIterator : public phCachedContactConstIterator
	{
	private:
		// Don't try to use the copy constructor. Don't pass this by value!
		phCachedContactIterator(phCachedContactIterator &) {}

	public:
		phCachedContactIterator() {}

		phManifold& GetRootManifold() const;
		//phManifold& ComputeManifold() const;
		phManifold& GetCachedManifold() const;
		phContact& GetContact() const;

		phCachedContactIterator& operator++ ();
	private:
		// DO NOT USE THE POSTFIX OPERATOR! IT CREATES BAD CODE!
		// Please write "++iterator" rather than "iterator++".
		phCachedContactIterator operator++ (int);
	public:

		void SetDepth(ScalarV_In depth) const;
		void SetDepth(float depth) const;
		void SetFriction(float friction) const;
		void SetElasticity(float elasticity) const;
// TODO -- Delete the old vector library versions (CL 2628091)
		void SetRelVelocity(const Vector3& relVel) const;
		void SetMyNormal (const Vector3& normal) const;
		void SetOtherNormal (const Vector3& normal) const;
		void SetRelVelocity(Vec3V_In relVel) const;
		void SetMyNormal (Vec3V_In normal) const;
		void SetOtherNormal (Vec3V_In normal) const;
		void SetMyPosition (Vec3V_In position) const;
		void SetOtherPosition (Vec3V_In position) const;
		// Legacy functions that don't make lasting changes to the contact (These positions will be overwritten at the next contact refresh)
		void SetMyPositionDoNotUse (Vec3V_In position) const;
		void SetOtherPositionDoNotUse (Vec3V_In position) const;

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		void SetDistToTopSurface(ScalarV_In dist) const;
#endif

		void DisableImpact () const;
		void ClearAllImpacts() const;
		void RemoveContact();

		void RemoveContactWithFinalize();
		void FinalizeRemoveContacts();

		// PURPOSE: Set the post-collision target velocity of this iterator's object in the current contact.
		void SetMyTargetVelocity (Vec3V_In targetVelocity) const;

		// PURPOSE: Modify how the solver looks at the mass of these two objects for this manifold only
		// PARAMS:
		//   myMassInvScale - multiplier for the inverse mass of this object
		//   otherMassInvScale - multiplier for the inverse mass of the other object
		// NOTES:
		//   -The scales should not be negative. 
		//   -An inverse mass of 0 makes an object act as if it has infinite mass. Setting both to 0 is bad.
		//   -The default mass inv scales on a manifold are 1 and 1. 
		void SetMassInvScales(float myMassInvScale, float otherMassInvScale) const;
		void SetMyMassInvScale(float myMassInvScale) const;
		void SetOtherMassInvScale(float otherMassInvScale) const;
	};

	__forceinline phCachedContactIterator& phCachedContactIterator::operator++ ()
	{
		return static_cast<phCachedContactIterator&>(phCachedContactConstIterator::operator++());
	}

	__forceinline phCachedContactIterator phCachedContactIterator::operator++ (int)
	{
		phCachedContactIterator copy(*this);
		++(*this);
		return copy;
	}

	__forceinline phContact& phCachedContactIterator::GetContact() const
	{
		const phContact& contact = phCachedContactConstIterator::GetContact();

		return const_cast<phContact&>(contact);
	}

	__forceinline phManifold& phCachedContactIterator::GetCachedManifold() const
	{
		const phManifold& manifold = phCachedContactConstIterator::GetCachedManifold();

		return const_cast<phManifold&>(manifold);
	}

	__forceinline phManifold& phCachedContactIterator::GetRootManifold() const
	{
		const phManifold& contact = phCachedContactConstIterator::GetRootManifold();

		return const_cast<phManifold&>(contact);
	}

	__forceinline void phCachedContactIterator::SetDepth (ScalarV_In depth) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateDepth(depth.Getf()));
		GetContact().SetDepth(depth);
	}

	__forceinline void phCachedContactIterator::SetDepth (float depth) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateDepth(depth));
		GetContact().SetDepth(depth);
	}

	__forceinline void phCachedContactIterator::SetFriction (float friction) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateFriction(friction));
		GetContact().SetFriction(friction);
	}

	__forceinline void phCachedContactIterator::SetElasticity (float elasticity) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateElasticity(elasticity));
		GetContact().SetElasticity(elasticity);
	}

	__forceinline void phCachedContactIterator::SetMyNormal (const Vector3& normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(RCC_VEC3V(normal)));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldNormal(RCC_VEC3V(normal));
			m_CurrentElement->m_MyNormal = RCC_VEC3V(normal);
		}
		else
		{
			contact.SetWorldNormal(-RCC_VEC3V(normal));
			m_CurrentElement->m_MyNormal = -RCC_VEC3V(normal);
		}
	}

	__forceinline void phCachedContactIterator::SetOtherNormal (const Vector3& normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(RCC_VEC3V(normal)));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldNormal(-RCC_VEC3V(normal));
		}
		else
		{
			contact.SetWorldNormal(RCC_VEC3V(normal));
		}
	}

	__forceinline void phCachedContactIterator::SetMyNormal (Vec3V_In normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(normal));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldNormal(normal);
			m_CurrentElement->m_MyNormal = normal;
		}
		else
		{
			contact.SetWorldNormal(-normal);
			m_CurrentElement->m_MyNormal = -normal;
		}
	}

	__forceinline void phCachedContactIterator::SetOtherNormal (Vec3V_In normal) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateNormal(normal));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldNormal(-normal);
		}
		else
		{
			contact.SetWorldNormal(normal);
		}
	}

	__forceinline void phCachedContactIterator::SetMyPosition (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldPosA(position);
			contact.ComputeLocalPointAFromWorld(&GetCachedManifold());
		}
		else
		{
			contact.SetWorldPosB(position);
			contact.ComputeLocalPointBFromWorld(&GetCachedManifold());
		}
	}

	__forceinline void phCachedContactIterator::SetOtherPosition (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldPosB(position);
			contact.ComputeLocalPointBFromWorld(&GetCachedManifold());
		}
		else
		{
			contact.SetWorldPosA(position);
			contact.ComputeLocalPointAFromWorld(&GetCachedManifold());
		}
	}

	__forceinline void phCachedContactIterator::SetMyPositionDoNotUse (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldPosA(position);
		}
		else
		{
			contact.SetWorldPosB(position);
		}

		m_CurrentElement->m_MyPosition = position;
	}

	__forceinline void phCachedContactIterator::SetOtherPositionDoNotUse (Vec3V_In position) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidatePosition(position));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetWorldPosB(position);
		}
		else
		{
			contact.SetWorldPosA(position);
		}

		m_CurrentElement->m_OtherPosition = position;
	}

	__forceinline void phCachedContactIterator::SetMyTargetVelocity (Vec3V_In targetVelocity) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateTargetVelocity(targetVelocity));
		phContact& contact = GetContact();

		if (m_CurrentElement->m_IsInstanceA)
		{
			contact.SetTargetRelVelocity(targetVelocity);
		}
		else
		{
			Vec3V negatedTargetVelocity = Negate(targetVelocity);
			contact.SetTargetRelVelocity(negatedTargetVelocity);
		}
	}

	__forceinline void phCachedContactIterator::SetMassInvScales(float myMassInvScale, float otherMassInvScale) const
	{
		CONTACT_VALIDATION_ONLY(phContact::ValidateMassInvScales(myMassInvScale,otherMassInvScale));
		u32 myObjectIndex = (GetMyInstance() == GetCachedManifold().GetInstanceA() ? 0 : 1);
		u32 otherObjectIndex = 1 - myObjectIndex;
		GetCachedManifold().SetMassInvScale(myObjectIndex,myMassInvScale);
		GetCachedManifold().SetMassInvScale(otherObjectIndex,otherMassInvScale);
	}

	__forceinline void phCachedContactIterator::SetMyMassInvScale(float myMassInvScale) const
	{
		u32 myObjectIndex = (GetMyInstance() == GetCachedManifold().GetInstanceA() ? 0 : 1);
		GetCachedManifold().SetMassInvScale(myObjectIndex,myMassInvScale);
		CONTACT_VALIDATION_ONLY(u32 otherObjectIndex = 1 - myObjectIndex;)
		CONTACT_VALIDATION_ONLY(phContact::ValidateMassInvScales(myMassInvScale,GetCachedManifold().GetMassInvScale(otherObjectIndex)));

	}

	__forceinline void phCachedContactIterator::SetOtherMassInvScale(float otherMassInvScale) const
	{
		u32 otherObjectIndex = (GetMyInstance() == GetCachedManifold().GetInstanceA() ? 1 : 0);
		GetCachedManifold().SetMassInvScale(otherObjectIndex,otherMassInvScale);
		CONTACT_VALIDATION_ONLY(u32 myObjectIndex = 1 - otherObjectIndex;)
		CONTACT_VALIDATION_ONLY(phContact::ValidateMassInvScales(GetCachedManifold().GetMassInvScale(myObjectIndex),otherMassInvScale));
	}

	__forceinline void phCachedContactIterator::DisableImpact () const
	{
		GetContact().ActivateContact(false);
		m_CurrentElement->m_Disabled = true;
	}

	__forceinline void phCachedContactIterator::ClearAllImpacts() const
	{
		GetCachedManifold().RemoveAllContacts();
	}

	__forceinline void phCachedContactIterator::RemoveContact()
	{
		Assertf(!IsRemoved(), "Attempting to remove already removed contact.");
		int contactNumber = GetCachedManifold().FindContactPointIndex(m_CurrentElement->m_Contact);
		GetCachedManifold().RemoveContactPoint(contactNumber);
#if __PPU
		// Rebuild the DMA plan because, presuming we're doing this in PreComputeImpacts,
		// the DMA plan has already been generated and removing a contact invalidates it.
		phManifold& cachedManifold = GetCachedManifold();
		cachedManifold.RegenerateDmaPlan();
		if (m_CurrentElement->m_CompositeManifold == 0)
		{
			phManifold& rootManifold = GetRootManifold();
			if (&rootManifold != &cachedManifold)
			{
				rootManifold.RegenerateDmaPlan();
			}
		}
#endif // __PPU
		m_CurrentElement->m_Removed = true;
		m_CurrentElement--;
	}

	__forceinline void phCachedContactIterator::RemoveContactWithFinalize()
	{
		Assertf(!IsRemoved(), "Attempting to remove already removed contact.");
		int contactNumber = GetCachedManifold().FindContactPointIndex(m_CurrentElement->m_Contact);
		GetCachedManifold().RemoveContactPoint(contactNumber);

		m_CurrentElement->m_Removed = true;
		m_CurrentElement--;
	}

	__forceinline void phCachedContactIterator::FinalizeRemoveContacts()
	{
#if __PPU
		// Rebuild the DMA plan because, presuming we're doing this in PreComputeImpacts,
		// the DMA plan has already been generated and removing a contact invalidates it.
		phManifold& cachedManifold = GetCachedManifold();
		cachedManifold.RegenerateDmaPlan();
		if (m_CurrentElement->m_CompositeManifold == 0)
		{
			phManifold& rootManifold = GetRootManifold();
			if (&rootManifold != &cachedManifold)
			{
				rootManifold.RegenerateDmaPlan();
			}
		}
#endif // __PPU
	}

} // namespace rage

#endif
