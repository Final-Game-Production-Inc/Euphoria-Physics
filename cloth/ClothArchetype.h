#ifndef CLOTH_ARCHETYPE_H
#define CLOTH_ARCHETYPE_H

#include "Debug/Debug.h"
#include "ClothMgr.h"
#include "ClothParticles.h"
#include "ClothConstraints.h"
#include "ClothSprings.h"
#include "ClothPendants.h"
#include "ClothCapsuleBounds.h"
class CDynamicEntity;
#include "entity/extensionlist.h"
#include "fwtl/pool.h"
#include "streaming/streamingmodule.h"

#include "crSkeleton/skeleton.h"
#include "crSkeleton/skeletondata.h"
#include "crSkeleton/bonedata.h"

#if NORTH_CLOTHS

#define MAX_NUM_INFINITE_MASS_PARTICLES 8

class CPlantTuning 
{
public:

	CPlantTuning() {}

	void SetTotalMass(const float fTotalMass)
	{
		m_fTotalMass=fTotalMass;
	}
	void SetDensityMultiplier(const float fDensityMultiplier)
	{
		m_fDensityMultiplier=fDensityMultiplier;
	}
	void SetDampingRate(const float fDampingRate)
	{
		m_fDampingRate=fDampingRate;
	}
	void SetCapsuleRadius(const float fCapsuleRadius)
	{
		m_fCapsuleRadius=fCapsuleRadius;
	}

	float GetTotalMass() const
	{
		return m_fTotalMass;
	}
	float GetDensityMultiplier() const
	{
		return m_fDensityMultiplier;
	}
	float GetDampingRate() const
	{
		return m_fDampingRate;
	}
	float GetCapsuleRadius() const
	{
		return m_fCapsuleRadius;
	}

	void SetModelName(const strStreamingObjectName pName);
	strStreamingObjectName GetModelName() const {return m_modelName;}

	static void Init();
	static void Shutdown();
	static void ClearPlantTuneData();
	static CPlantTuning& AddPlantTuneData(const strStreamingObjectName pName);
	static const CPlantTuning& GetPlantTuneData(const u32 key);

	static void Init(unsigned initMode);

private:

	float m_fTotalMass;
	float m_fDensityMultiplier;
	float m_fDampingRate;
	float m_fCapsuleRadius;

	strStreamingObjectName m_modelName;
};


class CClothArchetype : public fwExtension
{
public:
#if !__SPU
	EXTENSIONID_DECL(CClothArchetype, fwExtension);
#endif

	CClothArchetype()
	{
		Init();
	}
	virtual ~CClothArchetype()
	{
		Shutdown();
	}

	FW_REGISTER_CLASS_POOL(CClothArchetype);

	void Init()
	{
		//Particles.
		m_iNumParticles=0;
		for(int i=0;i<MAX_NUM_CLOTH_PARTICLES;i++)
		{
			m_avParticlePositions[i].Zero();
			m_afParticleInvMasses[i]=0;
			m_afParticleRadii[i]=0;
		}
		m_fParticleDampingRate=0;

		//Separation constraints.
		m_iNumSeparationConstraints=0;
		for(int i=0;i<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS;i++)
		{
			m_aiSeparationConstraintParticleAs[i]=0;
			m_aiSeparationConstraintParticleBs[i]=0;
			m_afSeparationConstraintRestLengths[i]=0;
		}

		//Volume constraints.
		m_iNumVolumeConstraints=0;
		for(int i=0;i<MAX_NUM_CLOTH_VOLUME_CONSTRAINTS;i++)
		{
			m_aiVolumeConstraintParticleAs[i]=0;
			m_aiVolumeConstraintParticleBs[i]=0;
			m_aiVolumeConstraintParticleCs[i]=0;
			m_aiVolumeConstraintParticleDs[i]=0;
			m_afVolumeConstraintRestVolumes[i]=0;
		}

		//Springs
		m_iNumSprings=0;
		for(int i=0;i<MAX_NUM_CLOTH_SPRINGS;i++)
		{
			m_aiSpringParticleAs[i]=0;
			m_aiSpringParticleBs[i]=0;
			m_afSpringRestLengths[i]=0;
			m_afSpringAllowedLengthDifferences[i]=0;
		}

		//Capsule bounds
		m_iNumCapsuleBounds=0;
		for(int i=0;i<MAX_NUM_CLOTH_CAPSULE_BOUNDS;i++)
		{
			m_aiSkeletonNodes[i]=0;
			m_aiCapsuleBoundParticleAs[i]=0;
			m_aiCapsuleBoundParticleBs[i]=0;
			m_afCapsuleBoundLengths[i]=0;
			m_afCapsuleBoundRadii[i]=0;
		}

		//Pendants.
		m_iNumPendants=0;
		for(int i=0;i<MAX_NUM_CLOTH_PENDANTS;i++)
		{
			m_apPendantMethods[i]=0;
			m_aiPendantParticleIds[i]=0;
		}

		//Skeleton.  
		m_iNumSkeletonNodes=0;

		//Model id.
		m_modelId.Invalidate();
	}
	void Shutdown()
	{
		//Pendants.
		for(int i=0;i<m_iNumPendants;i++)
		{
			if(m_apPendantMethods[i]) delete m_apPendantMethods[i];
			m_apPendantMethods[i]=0;
		}
		m_iNumSeparationConstraints=0;
	}

	///////////////////////////////////
	//BEGIN WIDGET TUNING
	///////////////////////////////////
	void RefreshFromDefaults();
	static float sm_fDefaultMass;
	static float sm_fDefaultTipMassMultiplier;
	static float sm_fDefaultDampingRate;
	static float sm_fDefaultCapsuleRadius;
	////////////////////////////////////
	//END WIDGET TUNING
	////////////////////////////////////


	/////////////////////////////
	//BEGIN PARTICLES
	/////////////////////////////
	//Add a particle by specifying its start pos, damping rate, and its mass.
	//For infinite mass particles (particles that can't ever move) set their mass to -1 (or any negative value).
	bool AddParticle(const Vector3& vStartPos, const float fDampingRate, const float fMass, const float fRadius)
	{
		Assertf(m_iNumParticles<MAX_NUM_CLOTH_PARTICLES, "Too many particles, need to increase MAX_NUM_CLOTH_PARTICLES");
		if(m_iNumParticles<MAX_NUM_CLOTH_PARTICLES)
		{
			//Set the pos and prev pos of the new particle.
			m_avParticlePositions[m_iNumParticles]=vStartPos;
			//Set the particle inverse mass (zero if mass is infinite as denoted by -ve fMass).
			m_afParticleInvMasses[m_iNumParticles]=(fMass>0 ? 1.0f/fMass : 0);
			//Set the damping rate.
			m_fParticleDampingRate=fDampingRate;
			//Set the radius
			m_afParticleRadii[m_iNumParticles]=fRadius;
			//Increment the number of particles.
			m_iNumParticles++;
			return true;
		}
		else
		{
			return false;
		}
	}
	bool SetParticle(const int iParticleIndex, const Vector3& vStartPos, const float fDampingRate, const float fMass, const float fRadius)
	{
		Assertf(iParticleIndex<m_iNumParticles, "out of bounds");
		if(iParticleIndex<m_iNumParticles)
		{
			//Set the pos and prev pos of the new particle.
			m_avParticlePositions[iParticleIndex]=vStartPos;
			//Set the particle inverse mass (zero if mass is infinite as denoted by -ve fMass).
			m_afParticleInvMasses[iParticleIndex]=(fMass>0 ? 1.0f/fMass : 0);
			//Set the damping rate.
			m_fParticleDampingRate=fDampingRate;
			//Set the radius.
			m_afParticleRadii[iParticleIndex]=fRadius;
			//That's us finished.
			return true;
		}
		else
		{
			return false;
		}
	}
	void SetParticleInvMass(const int iParticleIndex, const float fInvMass)
	{
		Assertf(iParticleIndex<m_iNumParticles, "out of bounds");
		if(iParticleIndex<m_iNumParticles)
		{
			m_afParticleInvMasses[iParticleIndex]=fInvMass;
		}
	}
	void SetParticleDampingRate(const int iParticleIndex, const float fDampingRate)
	{
		Assertf(iParticleIndex<m_iNumParticles, "out of bounds");
		if(iParticleIndex<m_iNumParticles)
		{
			m_fParticleDampingRate=fDampingRate;
		}
	}
	void SetParticleRadius(const int iParticleIndex, const float fRadius)
	{
		Assertf(iParticleIndex<m_iNumParticles, "out of bounds");
		if(iParticleIndex<m_iNumParticles)
		{
			m_afParticleRadii[iParticleIndex]=fRadius;
		}
	}
	void SetParticlePosition(const int iParticleIndex, const Vector3& vPos)
	{
		Assertf(iParticleIndex<m_iNumParticles, "out of bounds");
		if(iParticleIndex<m_iNumParticles)
		{
			m_avParticlePositions[iParticleIndex]=vPos;
		}
	}

	void SetNumParticles(const int iNumParticles)
	{
		m_iNumParticles=iNumParticles;
		Assertf(m_iNumParticles<MAX_NUM_CLOTH_PARTICLES, "Too many particles, need to increase MAX_NUM_CLOTH_PARTICLES");
	}
	int GetNumParticles() const
	{
		return m_iNumParticles;
	}
	const Vector3& GetParticlePosition(const int index) const
	{
		Assertf(index>=0 && index<m_iNumParticles, "Out of bounds");
		return m_avParticlePositions[index];
	}
	float GetParticleDampingRate(const int ASSERT_ONLY(index)) const
	{
		Assertf(index>=0 && index<m_iNumParticles, "Out of bounds");
		return m_fParticleDampingRate;
	}
	float GetParticleInvMass(const int index) const
	{
		Assertf(index>=0 && index<m_iNumParticles, "Out of bounds");
		return m_afParticleInvMasses[index];
	}
	float GetParticleRadius(const int index) const
	{
		Assertf(index<m_iNumParticles, "Out of bounds");
		return m_afParticleRadii[index];
	}
	const float* GetPtrToParticleRadii() const
	{
		return m_afParticleRadii;
	}
	const Vector3* GetPtrToParticlePositionArray() const
	{
		return m_avParticlePositions;
	}
	/////////////////////////////
	//END PARTICLES
	/////////////////////////////


	/////////////////////////////
	//BEGIN SEPARATION CONSTRAINTS
	/////////////////////////////
	//Add a separation constraint to the archetype.
	void AddSeparationConstraint(const int iParticleA, const int iParticleB, const float fRestLength)
	{
		Assertf(m_iNumSeparationConstraints<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS, "Out of bounds");
		if(m_iNumSeparationConstraints<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS)
		{
			Assertf(iParticleA<m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleB<m_iNumParticles, "Particle index b is out of bounds");
			Assertf(fRestLength>0, "Constraint rest length is zero");
			m_aiSeparationConstraintParticleAs[m_iNumSeparationConstraints]=iParticleA;
			m_aiSeparationConstraintParticleBs[m_iNumSeparationConstraints]=iParticleB;
			m_afSeparationConstraintRestLengths[m_iNumSeparationConstraints]=fRestLength;
			Assertf(m_aiSeparationConstraintParticleAs[m_iNumSeparationConstraints]!=m_aiSeparationConstraintParticleBs[m_iNumSeparationConstraints], "Constraint particle indices must be different");
			m_iNumSeparationConstraints++;
		}
	}
	//Reset the rest length of a separation constraint.
	void SetSeparationConstraintRestLength(const int iConstraintId, const float fRestLength)
	{
		Assertf(fRestLength>0, "Constraint rest length is zero");
		Assertf(iConstraintId>=0 && iConstraintId<m_iNumSeparationConstraints, "Illegal constraint id");
		if(iConstraintId<m_iNumSeparationConstraints)
		{
			m_afSeparationConstraintRestLengths[iConstraintId]=fRestLength;
		}
	}

	//Get ptrs to arrays of separation constraint particle indices to pass to instances.
	const int* GetPtrToSeparationConstraintParticleIdsA() const
	{
		return m_aiSeparationConstraintParticleAs;
	}
	const int* GetPtrToSeparationConstraintParticleIdsB() const
	{
		return m_aiSeparationConstraintParticleBs;
	}
	//Get ptr to array of separation constraint rest lengths.
	const float* GetPtrToSeparationConstraintRestLengths() const
	{
		return m_afSeparationConstraintRestLengths;
	}
	//Get separation constraint rest lengths.  The instance might randomise these a bit 
	//to create a bit of variety from the same archetype.
	int GetNumSeparationConstraints() const
	{
		return m_iNumSeparationConstraints;
	}
	float GetSeparationConstraintRestLength(const int index) const
	{
		Assertf(index<m_iNumSeparationConstraints, "Out of bounds");
		return m_afSeparationConstraintRestLengths[index];
	}
	/////////////////////////////
	//END CONSTRAINTS
	/////////////////////////////

	/////////////////////////////
	//BEGIN VOLUME CONSTRAINTS
	/////////////////////////////
	void AddVolumeConstraint(const int iParticle0, const int iParticle1, const int iParticle2, const int iParticle3, const float fRestVolume)
	{
		Assertf(m_iNumVolumeConstraints<MAX_NUM_CLOTH_VOLUME_CONSTRAINTS, "Out of bounds");
		if(m_iNumVolumeConstraints<MAX_NUM_CLOTH_VOLUME_CONSTRAINTS)
		{
			Assertf(iParticle0<m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticle1<m_iNumParticles, "Particle index b is out of bounds");
			Assertf(iParticle2<m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticle3<m_iNumParticles, "Particle index b is out of bounds");
			Assertf(fRestVolume>0, "Constraint rest length is zero");
			m_aiVolumeConstraintParticleAs[m_iNumVolumeConstraints]=iParticle0;
			m_aiVolumeConstraintParticleBs[m_iNumVolumeConstraints]=iParticle1;
			m_aiVolumeConstraintParticleCs[m_iNumVolumeConstraints]=iParticle2;
			m_aiVolumeConstraintParticleDs[m_iNumVolumeConstraints]=iParticle3;
			m_afVolumeConstraintRestVolumes[m_iNumVolumeConstraints]=fRestVolume;
			m_iNumVolumeConstraints++;
		}
	}
	//Reset the rest length of a volume constraint.
	void SetVolumeConstraintRestVolume(const int iConstraintId, const float fRestVolume)
	{
		Assertf(fRestVolume>0, "Constraint rest volume is zero");
		Assertf(iConstraintId>=0 && iConstraintId<m_iNumVolumeConstraints, "Illegal constraint id");
		if(iConstraintId<m_iNumVolumeConstraints)
		{
			m_afVolumeConstraintRestVolumes[iConstraintId]=fRestVolume;
		}
	}

	//Get ptrs to arrays of volume constraint particle indices to pass to instances.
	const int* GetPtrToVolumeConstraintParticleIdsA() const
	{
		return m_aiVolumeConstraintParticleAs;
	}
	const int* GetPtrToVolumeConstraintParticleIdsB() const
	{
		return m_aiVolumeConstraintParticleBs;
	}
	const int* GetPtrToVolumeConstraintParticleIdsC() const
	{
		return m_aiVolumeConstraintParticleCs;
	}
	const int* GetPtrToVolumeConstraintParticleIdsD() const
	{
		return m_aiVolumeConstraintParticleDs;
	}

	//Get ptr to array of volume constraint rest lengths.
	const float* GetPtrToVolumeConstraintRestVolumes() const
	{
		return m_afVolumeConstraintRestVolumes;
	}
	//Get volume constraint rest lengths.  The instance might randomise these a bit 
	//to create a bit of variety from the same archetype.
	int GetNumVolumeConstraints() const
	{
		return m_iNumVolumeConstraints;
	}
	float GetVolumeConstraintRestVolume(const int index) const
	{
		Assertf(index<m_iNumVolumeConstraints, "Out of bounds");
		return m_afVolumeConstraintRestVolumes[index];
	}
	/////////////////////////////
	//END VOLUME CONSTRAINTS
	/////////////////////////////


	/////////////////////////////
	//BEGIN SPRINGS
	/////////////////////////////
	//Add a spring to the archetype.
	bool AddSpring(const int iParticleA, const int iParticleB, const float fRestLength, const float fAllowedLengthDifference)
	{
		Assertf(fAllowedLengthDifference>=0, "Allowed difference must be declared positive but will used in range (-allowed,+allowed)");
		Assertf(m_iNumSprings<MAX_NUM_CLOTH_SPRINGS, "Too many springs need to increase MAX_NUM_CLOTH_SPRINGS");
		if(m_iNumSprings<MAX_NUM_CLOTH_SPRINGS)
		{
			Assertf(iParticleA<m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleB<m_iNumParticles, "Particle index b is out of bounds");
			Assertf(fRestLength>0, "Constraint rest length is zero");
			m_aiSpringParticleAs[m_iNumSprings]=iParticleA;
			m_aiSpringParticleBs[m_iNumSprings]=iParticleB;
			m_afSpringRestLengths[m_iNumSprings]=fRestLength;
			m_afSpringAllowedLengthDifferences[m_iNumSprings]=fAllowedLengthDifference;
			m_iNumSprings++;
			return true;
		}
		else
		{
			return false;
		}
	}
	//Reset the rest length of a spring.
	void SetSpringRestLength(const int iSpringId, const float fRestLength)
	{
		Assertf(fRestLength>0, "Constraint rest length is zero");
		Assertf(iSpringId>=0 && iSpringId<m_iNumSprings, "Illegal spring id");
		if(iSpringId<m_iNumSprings)
		{
			m_afSpringRestLengths[iSpringId]=fRestLength;
		}
	}
	//Get ptrs to arrays of spring particle indices to pass to instances.
	const int* GetPtrToSpringParticleIdsA() const
	{
		return m_aiSpringParticleAs;
	}
	const int* GetPtrToSpringParticleIdsB() const
	{
		return m_aiSpringParticleBs;
	}
	//Get ptrs to arrays of spring lengths and allowed length variations.
	const float* GetPtrToSpringRestLengths() const
	{
		return m_afSpringRestLengths;
	}
	const float* GetPtrToSpringAllowedLengthDifferrences() const
	{
		return m_afSpringAllowedLengthDifferences;
	}
	//Get number of springs
	int GetNumSprings() const
	{
		return m_iNumSprings;
	}
	//Get target rest length and allowed length variation.
	float GetSpringRestLength(const int index) const
	{
		Assertf(index<m_iNumSprings, "Out of bounds");
		return m_afSpringRestLengths[index];
	}
	float GetSpringAllowedLengthDifference(const int index) const
	{
		Assertf(index<m_iNumSprings, "Out of bounds");
		return m_afSpringAllowedLengthDifferences[index];
	}
	/////////////////////////////
	//END SPRINGS
	/////////////////////////////

	/////////////////////////////
	//BEGIN PENDANTS
	/////////////////////////////
	void AddPendant(CClothPendantMethod* pMethod, const int iParticleId)
	{
		Assertf(m_iNumPendants<MAX_NUM_CLOTH_PENDANTS, "Too many pendants");
		if(m_iNumPendants<MAX_NUM_CLOTH_PENDANTS)
		{
			m_apPendantMethods[m_iNumPendants]=pMethod;
			m_aiPendantParticleIds[m_iNumPendants]=iParticleId;
			m_iNumPendants++;
		}
	}
	int GetNumPendants() const
	{
		return m_iNumPendants;
	}
	const int* GetPtrToPedantParticleIds() const
	{
		return m_aiPendantParticleIds;
	}
	const CClothPendantMethod* GetPendantMethod(const int index)  const
	{
		Assertf(index<m_iNumPendants, "Out of bounds");
		return m_apPendantMethods[index];
	}
	/////////////////////////////
	//END PENDANTS
	/////////////////////////////

	/////////////////////////////
	//BEGIN CAPSULE BOUNDS
	/////////////////////////////
	void AddCapsuleBound
		(const int iParticleA, const int iParticleB, const float fLength, const float fRadius)
	{
		Assertf(m_iNumCapsuleBounds<MAX_NUM_CLOTH_CAPSULE_BOUNDS, "Too many capsules bounds");
		if(m_iNumCapsuleBounds<MAX_NUM_CLOTH_CAPSULE_BOUNDS)
		{
			Assertf(iParticleA<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleB<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(fLength>0, "Capsule bound with zero or -ve length");
			Assertf(fRadius>0, "Capsule bound with zero or -ve radius");
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+0]=iParticleA;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+1]=-1;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+2]=-1;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+3]=-1;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+0]=iParticleB;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+1]=-1;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+2]=-1;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+3]=-1;
			m_afCapsuleBoundLengths[m_iNumCapsuleBounds]=fLength;
			m_afCapsuleBoundRadii[m_iNumCapsuleBounds]=fRadius;
			m_iNumCapsuleBounds++;
		}
	}

#if 3==MAX_NUM_PARTICLES_PER_CAPSULE
	void AddCapsuleBound
		(const int iSkeletonNode, 
		const int iParticleA0, const int iParticleA1, const int iParticleA2, 
		const int iParticleB0, const int iParticleB1, const int iParticleB2,
		const float fLength, const float fRadius)
	{
		Assertf(m_iNumCapsuleBounds<MAX_NUM_CLOTH_CAPSULE_BOUNDS, "Too many capsules bounds");
		if(m_iNumCapsuleBounds<MAX_NUM_CLOTH_CAPSULE_BOUNDS)
		{
			Assertf(iParticleA0<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleA1<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleA2<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleB0<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(iParticleB1<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(iParticleB2<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(fLength>0, "Capsule bound with zero or -ve length");
			Assertf(fRadius>0, "Capsule bound with zero or -ve radius");
			m_aiSkeletonNodes[m_iNumCapsuleBounds]=iSkeletonNode;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+0]=iParticleA0;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+1]=iParticleA1;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+2]=iParticleA2;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+0]=iParticleB0;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+1]=iParticleB1;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+2]=iParticleB2;
			m_afCapsuleBoundLengths[m_iNumCapsuleBounds]=fLength;
			m_afCapsuleBoundRadii[m_iNumCapsuleBounds]=fRadius;
			m_iNumCapsuleBounds++;
		}
	}
#elif 4==MAX_NUM_PARTICLES_PER_CAPSULE
	void AddCapsuleBound
		(const int iSkeletonNode, 
		 const int iParticleA0, const int iParticleA1, const int iParticleA2, const int iParticleA3,
		 const int iParticleB0, const int iParticleB1, const int iParticleB2, const int iParticleB3,
		 const float fLength, const float fRadius)
	{
		Assertf(m_iNumCapsuleBounds<MAX_NUM_CLOTH_CAPSULE_BOUNDS, "Too many capsules bounds");
		if(m_iNumCapsuleBounds<MAX_NUM_CLOTH_CAPSULE_BOUNDS)
		{
			Assertf(iParticleA0<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleA1<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleA2<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleA3<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index a is out of bounds");
			Assertf(iParticleB0<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(iParticleB1<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(iParticleB2<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(iParticleB3<MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumParticles, "Particle index b is out of bounds");
			Assertf(fLength>0, "Capsule bound with zero or -ve length");
			Assertf(fRadius>0, "Capsule bound with zero or -ve radius");
			m_aiSkeletonNodes[m_iNumCapsuleBounds]=iSkeletonNode;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+0]=iParticleA0;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+1]=iParticleA1;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+2]=iParticleA2;
			m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+3]=iParticleA3;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+0]=iParticleB0;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+1]=iParticleB1;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+2]=iParticleB2;
			m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*m_iNumCapsuleBounds+3]=iParticleB3;
			m_afCapsuleBoundLengths[m_iNumCapsuleBounds]=fLength;
			m_afCapsuleBoundRadii[m_iNumCapsuleBounds]=fRadius;
			m_iNumCapsuleBounds++;
		}
	}
#endif
	void SetCapsuleBoundRadius(const int iCapsuleBoundIndex, const float fRadius)
	{
		Assertf(iCapsuleBoundIndex<m_iNumCapsuleBounds, "Out of bounds");
		Assertf(fRadius>0, "Capsule bound with zero or -ve radius");
		if(iCapsuleBoundIndex<m_iNumCapsuleBounds)
		{
			m_afCapsuleBoundRadii[iCapsuleBoundIndex]=fRadius;
		}
	}
	int GetNumCapsuleBounds() const
	{
		return m_iNumCapsuleBounds;
	}
	const int* GetPtrToSkeletonNodes() const
	{
		return m_aiSkeletonNodes;
	}
	const int* GetPtrToCapsuleBoundParticleAs() const
	{
		return m_aiCapsuleBoundParticleAs;
	}
	const int* GetPtrToCapsuleBoundParticleBs() const
	{
		return m_aiCapsuleBoundParticleBs;
	}
	const float* GetPtrToCapsuleBoundLengths() const
	{
		return m_afCapsuleBoundLengths;
	}
	const float* GetPtrToCapsuleBoundRadii() const
	{
		return m_afCapsuleBoundRadii;
	}
	/////////////////////////////
	//END CAPSULE BOUNDS
	/////////////////////////////

	/////////////////////////////
	//BEGIN SKELETON
	/////////////////////////////
	void SetNumSkeletonBones(const int iNumSkeletonBones)
	{
		m_iNumSkeletonNodes=iNumSkeletonBones;
	}
	int GetNumSkeletonBones() const
	{
		return m_iNumSkeletonNodes;
	}
	/////////////////////////////
	//END SKELETON
	/////////////////////////////

	////////////////////////
	//BEGIN MODEL INDEX
	////////////////////////
	void SetModelId(const fwModelId modelId)
	{
		Assert(!modelId.IsValid());
		m_modelId = modelId;
	}
	int GetModelIndex() const
	{
		return (m_modelId.GetModelIndex());
	}
	////////////////////////
	//END MODEL INDEX
	////////////////////////

private:

	//Particles.
	//Array of particle positions.
	int m_iNumParticles;
	Vector3 m_avParticlePositions[MAX_NUM_CLOTH_PARTICLES];
	//Damping rate of all particles.
	float m_fParticleDampingRate;
	//Array of particle inverse masses.
	float m_afParticleInvMasses[MAX_NUM_CLOTH_PARTICLES];
	//Array of particle radii.
	float m_afParticleRadii[MAX_NUM_CLOTH_PARTICLES];

	//Separation constraints.
	//Arrays of indices storing id of particle A and particle B for each constraint.
	int m_iNumSeparationConstraints;
	int m_aiSeparationConstraintParticleAs[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
	int m_aiSeparationConstraintParticleBs[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
	//Arrays of rest lengths of distance between each constraint.
	float m_afSeparationConstraintRestLengths[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];

	//Volume constraints.
	int m_iNumVolumeConstraints;
	int m_aiVolumeConstraintParticleAs[MAX_NUM_CLOTH_VOLUME_CONSTRAINTS];
	int m_aiVolumeConstraintParticleBs[MAX_NUM_CLOTH_VOLUME_CONSTRAINTS];
	int m_aiVolumeConstraintParticleCs[MAX_NUM_CLOTH_VOLUME_CONSTRAINTS];
	int m_aiVolumeConstraintParticleDs[MAX_NUM_CLOTH_VOLUME_CONSTRAINTS];
	float m_afVolumeConstraintRestVolumes[MAX_NUM_CLOTH_VOLUME_CONSTRAINTS];

	//Springs
	//Arrays of indices storing id of particles A and B of each spring.
	int m_iNumSprings;
	int m_aiSpringParticleAs[MAX_NUM_CLOTH_SPRINGS];
	int m_aiSpringParticleBs[MAX_NUM_CLOTH_SPRINGS];
	//Arrays of target angles and allowed length range.
	float m_afSpringRestLengths[MAX_NUM_CLOTH_SPRINGS];
	float m_afSpringAllowedLengthDifferences[MAX_NUM_CLOTH_SPRINGS];

	//Pendants.
	int m_iNumPendants;
	int m_aiPendantParticleIds[MAX_NUM_CLOTH_PENDANTS];
	CClothPendantMethod* m_apPendantMethods[MAX_NUM_CLOTH_PENDANTS];

	//Capsule bounds
	int m_iNumCapsuleBounds;
	int m_aiSkeletonNodes[MAX_NUM_CLOTH_PARTICLES];
	int m_aiCapsuleBoundParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*MAX_NUM_CLOTH_CAPSULE_BOUNDS];//average up to four particles to compute capsule start
	int m_aiCapsuleBoundParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*MAX_NUM_CLOTH_CAPSULE_BOUNDS];//average up to four particles to compute capsule end
	float m_afCapsuleBoundLengths[MAX_NUM_CLOTH_CAPSULE_BOUNDS];
	float m_afCapsuleBoundRadii[MAX_NUM_CLOTH_CAPSULE_BOUNDS];

	//Map that allows skeleton nodes to be related to particles.
	int m_iNumSkeletonNodes;

	//Model id that is posed with cloth made from archetype instance.
	fwModelId m_modelId;
};

class CClothArchetypeCreator
{
public:

	CClothArchetypeCreator(){}
	~CClothArchetypeCreator(){}

	//Create a plant from skeleton data
	static CClothArchetype* CreatePlant(const crSkeletonData& skeletonData, const CPlantTuning& plantTuningData);

	/*
	//Just for debugging some simple types of cloth.
	enum
	{
		TYPE_PLAYER_HAIR=0,
		TYPE_PLAYER_ANTENNA,
		TYPE_PLANT,
		TYPE_PLANT_3D
	};
	static CClothArchetype* Create(const int iType);
	*/

private:

	static void WalkSkeleton(const crBoneData* pParent,  const int iNodePositionArraySize, Vector3* avNodePositions, int& iNumNodes);
	static void WalkSkeleton(const int* paiSkeletonNodeMap, const int* paiDuplicates, const crBoneData* pGrandParent, const crBoneData* pParent, const float fDefaultDampingRate, const float fDefaultMass, const float fDefaultRadius, CClothArchetype& archetype);
	static void AddParticleLayer
		(const int iGrandParentNodeIndexNotDuplicated, const int iParentNodeIndexNotDuplicated, const int iChildNodeIndexNotDuplicated,
		 const int iParentNodeIndex,
		 const Vector3& vTranslation, 
		 const float fDefaultDampingRate, const float fDefaultMass, const float fDefaultRadius,
		 CClothArchetype& archetype);


	/*
	//Just for debugging some simple types of cloth.
	static CClothArchetype* CreatePlayerHair(const float fLength, const int iNumParticles);
	static CClothArchetype* CreatePlayerAntenna(const float fLength, const int iNumParticles);
	static CClothArchetype* CreatePlantNearPlayer(const float fLength, const float fMassAtBottom, const float fMassAtTop, const int iNumParticles);
	static CClothArchetype* CreatePlantNearPlayer3D(const float fLength, const float fWidth, const float fTargetMass, const float fMassAtBottom, const float fMassAtTop, const int iNumParticles);
	*/
};

#endif //NORTH_CLOTHS

#endif //CLOTH_ARCHETYPE_H
