#ifndef INC_CLOTH_H
#define INC_CLOTH_H

//rage
#include "physics/inst.h"
#include "physics/instbehavior.h"
#include "vector/vector3.h"

//game
#include "Debug/Debug.h"
#include "ClothMgr.h"
#include "ClothParticles.h"
#include "ClothConstraints.h"
#include "ClothSprings.h"
#include "ClothPendants.h"
#include "ClothCapsuleBounds.h"
#include "fwtl/pool.h"

#if NORTH_CLOTHS

class phInstBehaviourCloth;
class CClothArchetype;

//A cloth is just a system of constrained particles.
//A cloth is made up of 
//(i)	an array of particles that have mass, position, and velocity.
//(ii)	an array of constraints that couple the particles together, forcing them to have a fixed separation.
//(iii) an array of torsional springs that attempt to maintain an angle between three cloth particles.
//(iv)	an array of pendants attached to individual particles that override the ballistic update of particle position/velocity.
//		pendants allow particles to be attached to other entities or just to follow prescribed motions.
//(v)	an array of contacts that override the ballistic update of particle position/velocity for particles
//		by forcing the particle to remain on the surface of the bound it is touching for the remainder of the cloth update.
//(vi)  arrays of data that describe how to update the rage capsule bounds used for collision detection.
//(vii) a skeleton node map that describes how to pose a skeleton from the cloth particles.

//Instances of the same cloth type share data describing their topology with a common CClothArchetype.
//The class CClothMgr stores all cloths and cloth archetypes and manage cloth integration using a CClothUpdater.
//The CClothMgr class also manages the dependency of cloths on archetypes to make sure that archetypes aren't illegally.
//Rage integration is achieved using phInstCloth (a sub-class of phInst) and phInstBehaviourCloth (a sub-class of phInstBehaviour).

class CCloth
{
public:

	friend class CClothMgr;
	friend class CClothUpdater;
	friend class CClothPendants;
	friend class phInstBehaviourCloth;

	FW_REGISTER_CLASS_POOL(CCloth);

	void Init()
	{
		m_pClothArchetype=NULL;
		m_particles.Init();
		m_separationConstraints.Init();
		m_springs.Init();
		m_pendants.Init();
		m_bIsAsleep=true;
		m_bJustWokenUp=false;
		m_fNotMovingTime=0;
		m_pRageBehaviour=0;
	}
	void Shutdown()
	{
		m_pClothArchetype=NULL;
		m_particles.Shutdown();
		m_separationConstraints.Shutdown();
		m_springs.Shutdown();
		m_pendants.Shutdown();
		m_bIsAsleep=true;
		m_bJustWokenUp=false;
		m_fNotMovingTime=0;
		m_pRageBehaviour=0;
	}

	//Initialise a cloth from the archetype data.
	void SetArchetype(const CClothArchetype* archetypeCloth);
	const CClothArchetype* GetArchetype() const {return m_pClothArchetype;}

	//Transform the cloth particles and store the transform.
	void Transform(const Matrix34& transform);
	const Matrix34& GetTransform() const {return m_transform;}
	//Set ptr to rage representation of cloth.
	void SetRageBehaviour(phInstBehaviourCloth* pRageBehaviour) {m_pRageBehaviour=pRageBehaviour;}
	phInstBehaviourCloth* GetRageBehaviour() const {return m_pRageBehaviour;}

	const CClothParticles& GetParticles() const 
	{
		return m_particles;
	}
	const CClothSeparationConstraints& GetSeparationConstraints() const
	{
		return m_separationConstraints;
	}
	const CClothVolumeConstraints& GetVolumeConstraints() const
	{
		return m_volumeConstraints;
	}

	const CClothPendants& GetPendants() const
	{
		return m_pendants;
	}
	const CClothSprings& GetSprings() const
	{
		return m_springs;
	}
	const CClothCapsuleBounds& GetCapsuleBounds() const
	{
		return m_capsuleBounds;
	}

	//Update the matrices used to pose the skeleton from the particle positions.
	void UpdateSkeletonMatrices(const CClothArchetype& archetype);
#if !__SPU
	//Pose the skeleton from the updated matrices.
	void PoseSkeleton(crSkeleton& skeleton) const;
#else 
	//Get the matrices used to pose the skeleton.
	int GetNumSkeletonNodes() const;
	const Matrix34* GetSkeletonMatrixArrayPtr() const;
#endif

	//Call once per update of the cloth.
	//Either increment the count or set it back to zero depending on whether any cloth particle
	//moved in the last update.
	void IncrementNotMovingTime(const float dt) 
	{
		m_fNotMovingTime+=dt;
	}
	void ResetNotMovingTime()
	{
		m_fNotMovingTime=0;
	}
	float GetNotMovingTime() const
	{
		return m_fNotMovingTime;
	}

	//Set to sleep or wake it up.
	//Set to sleep when the cloth hasn't moved for a number of updates.
	//Wake up when cloth is hit by another instance.
	void SetToSleep()
	{
		m_fNotMovingTime=0;
		m_bIsAsleep=true;
		m_bJustWokenUp=false;
	}
	void SetToAwake()
	{
		m_fNotMovingTime=0;
		m_bIsAsleep=false;
		m_bJustWokenUp=true;
	}
	bool IsAsleep() const
	{
		return m_bIsAsleep;
	}
	bool GetIsJustWokenUp() const
	{
		return m_bJustWokenUp;
	}
	void SetIsJustWokenUp(const bool bIsJustWokenUp)
	{
		m_bJustWokenUp=bIsJustWokenUp;
	}

private:

	CCloth()
	{
		Init();
	}
	~CCloth()
	{
		Shutdown();
	}

	CClothParticles& GetParticles()
	{
		return m_particles;
	}
	CClothSeparationConstraints& GetSeparationConstraints() 
	{
		return m_separationConstraints;
	}
	CClothVolumeConstraints& GetVolumeConstraints() 
	{
		return m_volumeConstraints;
	}
	CClothPendants& GetPendants()
	{
		return m_pendants;
	}
	CClothSprings& GetSprings() 
	{
		return m_springs;
	}
	CClothCapsuleBounds& GetCapsuleBounds()
	{
		return m_capsuleBounds;
	}

	const CClothArchetype* m_pClothArchetype;						//Archetype of cloth, contains all non-instance data
	CClothParticles m_particles;							//System of particles.
	CClothSeparationConstraints m_separationConstraints;	//System of separation constraints.
	CClothVolumeConstraints m_volumeConstraints;			//System of volume conservation constraints.
	CClothSprings m_springs;								//System of torsion springs that stop cloth folding.
	CClothPendants m_pendants;								//System of pendants that move particles around.
	CClothCapsuleBounds m_capsuleBounds;					//Capsule bounds used for collision.
	bool m_bIsAsleep;										//Is the cloth awake or asleep.
	bool m_bJustWokenUp;									//Was the cloth just woken up.
	float m_fNotMovingTime;									//Number of frames that cloth hasn't moved.
	Matrix34 m_transform;									//Start transform of cloth.
	phInstBehaviourCloth* m_pRageBehaviour;					//Rage representation of cloth.
};


#endif //NORTH_CLOTHS

#endif //INC_CLOTH_H
