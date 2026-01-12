//
// physics/constraintbase.h
//
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_BASE_H
#define PHYSICS_CONSTRAINT_BASE_H

#include "contact.h"
#include "constrainthandle.h"

#include "atl/array.h"
#include "atl/bitset.h"
#include "grprofile/drawcore.h"
#include "vector/quaternion.h"
#include "vector/vector3.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#if __ASSERT
#define ConstraintAssert(x, fmt, ...) (void)(Likely(x) || ConstraintAssertFunc(__FILE__,__LINE__,"%s: " fmt,#x,##__VA_ARGS__) || (__debugbreak(),0))
#else // __ASSERT
#define ConstraintAssert(x, fmt, ...)
#endif // __ASSERT

namespace rage {

class phManifold;

class phConstraintBase
{
public:

	enum Type {
		//Basic types (one phContact internally)
		HALFSPACE,		//Removes 1 relative DOF complementarily
		DISTANCE,		//Removes 1 relative DOF complementarily
		SPHERICAL,		//Removes 3 relative DOFs
		ROTATION,		//Removes 1 relative DOF complementarily
		FIXEDROTATION,	//Removes 3 relative DOFs
		
		ATTACHMENT,		//Attaches two components, optionally with a permitted separation distance and optionally with their rotations locked
		PRISMATIC,		
		HINGE,
		FIXED,			//Removes all relative DOFs (or just the three rotational DOFs)
		CYLINDRICAL		//Removes 2 relative DOFs
	};

	struct Params
	{
		// One of the instances the constraint is attached to
		phInst* instanceA;

		// The other instance involved; typically if this is NULL then the constraint is attached to a fixed point
		phInst* instanceB;

		// The component of object A the constraint attaches to (important for articulated objects)
		u16 componentA;

		// The component of object B the constraint attaches to (important for articulated objects)
		u16 componentB;

		// Set this to zero to have object B react as if object A were infinitely massive
		float massInvScaleA;

		// Set this to zero to have object A react as if object B were infinitely massive
		float massInvScaleB;

		// Whether this constraint will become no longer enforced as the result of a large impulse
		bool breakable;

		// The impulse that will cause this constraint to break
		float breakingStrength;

		// Whether this constraint should use pushes. If false, separation bias error correction is used.
		bool usePushes;

		// Whether this constraint should get "extra" iterations in the solver
		bool extraIterations;

		// The separation bias on this constraint...meaningless unless pushes are disabled
		float separateBias;

		// Don't set this: it records the derived class type these params are for
		const Type type;

	protected:
		Params(Type _type)
			: instanceA(NULL)
			, instanceB(NULL)
			, componentA(0)
			, componentB(0)
			, massInvScaleA(1.0f)
			, massInvScaleB(1.0f)
			, breakable(false)
			, breakingStrength(10000.0f)
			, usePushes(true)
			, extraIterations(false)
			, separateBias(1.0f)
			, type(_type)
		{
		}

		Params(const Params& other, const Type thisType)
			: instanceA(other.instanceA)
			, instanceB(other.instanceB)
			, componentA(other.componentA)
			, componentB(other.componentB)
			, massInvScaleA(other.massInvScaleA)
			, massInvScaleB(other.massInvScaleB)
			, breakable(other.breakable)
			, breakingStrength(other.breakingStrength)
			, usePushes(other.usePushes)
			, extraIterations(other.extraIterations)
			, separateBias(other.separateBias)
			, type(thisType)
		{
		}
		
	private:
		const Params& operator=(const Params&)
		{
			Assert(0);
			return *this;
		}
	};

	virtual ~phConstraintBase() { }

#if !__FINAL
	// Record the location in the code where this constraint was inserted
	virtual void SetOwner(const char* file, int line);
	const char* GetOwnerFile() const { return m_OwnerFile; }
	int GetOwnerLine() const { return m_OwnerLine; }
#endif // !__FINAL

// user interface
public: 

	virtual void SetWorldPosA(Vec3V_In /*worldPosA*/) { Errorf("Invalid call(SetWorldPosA) to phConstraint base class"); };
	virtual void SetWorldPosB(Vec3V_In /*worldPosB*/) { Errorf("Invalid call(SetWorldPosB) to phConstraint base class"); };

	// For enabling breaking. breakingStrength is the impulse that will cause the constraint to break
	virtual void SetBreakable(bool breakable=true, float breakingStrength=10000.0f);
	virtual void SetBroken(bool broken=true);
	bool IsBreakable() { return m_Breakable; }
	bool IsBroken() { return m_Broken; }

// interface for simulator
public :

	void SetHandle(phConstraintHandle handle) { m_Handle = handle; }

	// Return which sub-class of constraint this is
	Type GetType() { return Type(m_Type); }

	virtual const char* GetTypeName() = 0;

	// This is called on every active constraint once per frame, from phConstraintMgr::UpdateAndAddConstraintContacts
	bool Update(Vec::V3Param128 invTimeStep128, bool addToSolver);
	virtual void VirtualUpdate(Vec::V3Param128, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver) = 0;

	virtual void EnforceWorldConstraints();
	virtual void VirtualEnforceWorldConstraints(phCollider* collider);

	void RequestWorldLimitEnforcement();

	// Called by the phSimulator after constraint solving. Returns true iff constraint was broken as a result of this call.
	virtual bool UpdateBreaking() { return false; }

//	void ReplaceInstanceComponents(phInst* instance, const atFixedBitSet<phContactMgr::MAX_NUM_BREAKABLE_COMPONENTS>& componentBits, phInst* newInstance);
	// Hardcoding to 128 because phContactMgr::MAX_NUM_BREAKABLE_COMPONENTS would require me to pull in phsolver/contactmgr.h.  If it ever changes there we'll
	//   get a compile error anyway.
	void ReplaceInstanceComponents(int originalLevelIndex, const atFixedBitSet<128>& componentBits, int newLevelIndex, int newGenerationId);

#if __PFDRAW
	virtual void ProfileDraw() = 0;
#endif

	void SetAllowForceActivation(bool set) { m_AllowForceActivation = set; }
	
	bool IsAtLimit() const { return m_AtLimit; }

	void FlagForDestruction();
	bool IsFlaggedForDestruction() const { return m_FlaggedForDestruction; }

	phInst* GetInstanceA();
	phInst* GetInstanceB();
	u16		GetComponentA() const { return m_ComponentA; }
	u16		GetComponentB() const { return m_ComponentB; }
	float	GetMassInvScaleA() const { return  m_MassInvScaleA; }
	float	GetMassInvScaleB() const { return  m_MassInvScaleB; }
	bool	GetUsePushes() const { return m_UsePushes; }
	void	SetMassInvScaleA(float massInvScale) { m_MassInvScaleA = massInvScale; }
	void	SetMassInvScaleB(float massInvScale) { m_MassInvScaleB = massInvScale; }

	bool	IsInstAValid();
	bool	IsInstBValid();

	//This doesn't copy the type, but does copy everything else that was passed into the constructor of this phConstraintBase into the params
	void	ReconstructBaseParams(phConstraintBase::Params& inoutParams);

	virtual void DisableManifolds() = 0;

	void SetNeedsExtraIterations(bool extraIterations) { m_NeedsExtraIterations = extraIterations; }
	bool GetNeedsExtraIterations() { return m_NeedsExtraIterations; }

protected:

	phConstraintBase(const Params& params);

	phManifold* AllocateManifold(phInst* instanceA, phInst* instanceB, Mat33V* constraintMatrix);

	void GetWorldMatrix(const phInst* instance, int component, Mat34V_Ref matrix) const;
	QuatV_Out GetWorldQuaternion(const phInst* instance, int component) const;

	void AddManifoldToSolver(phManifold* manifold, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB);

	phContact* GetContactPoint(phManifold* manifold);

	bool BreakIfImpulseSufficient(phManifold* manifold);

	void DisableManifold(phManifold* manifold);

#if __ASSERT
	int ConstraintAssertFunc(const char* file, int line, const char* fmt, ...);
#endif // __ASSERT

private:

	const phConstraintBase& operator=(const phConstraintBase&)
	{
		Assert(0);
		return *this;
	}

private:
	u16 m_LevelIndexA;
	u16 m_LevelIndexB;
	u16 m_GenerationIdA;
	u16 m_GenerationIdB;

	phConstraintHandle m_Handle;

protected:
	u16 m_ComponentA;
	u16 m_ComponentB;

	float m_BreakingStrengthSqr;

private:
	float m_SeparateBias;
	float m_MassInvScaleA;
	float m_MassInvScaleB;

	const u8 m_Type;
	bool m_UsePushes : 1;
	bool m_FlaggedForDestruction : 1;

protected:
	bool m_Swapped : 1;
	bool m_Broken : 1;
	bool m_Breakable : 1;
	bool m_AllowForceActivation : 1;
	bool m_AtLimit : 1;
	bool m_NeedsExtraIterations : 1;

#if !__FINAL
	const char* m_OwnerFile;
	int m_OwnerLine;
	u32 m_FrameCreated;
#endif // !__FINAL
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_BASE_H
