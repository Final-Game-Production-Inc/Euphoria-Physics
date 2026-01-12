#include "ClothConstraints.h"
#include "ClothArchetype.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

void CClothSeparationConstraints::SetArchetypeData(const CClothArchetype& clothArchetype)
{
	//Zero everything before setting up the data.
	Init();

	//Store the number of constraints.
	m_iNumConstraints=clothArchetype.GetNumSeparationConstraints();
	Assertf(m_iNumConstraints>0, "Zero constraints");
	Assertf(m_iNumConstraints<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS, "Too many constraints");

	//Get the ptrs to the arrays of particle indices.
	m_paiParticleAs=clothArchetype.GetPtrToSeparationConstraintParticleIdsA();
	m_paiParticleBs=clothArchetype.GetPtrToSeparationConstraintParticleIdsB();
	Assertf(m_paiParticleAs, "Null array ptr");
	Assertf(m_paiParticleBs, "Null array ptr");
	Assertf(m_paiParticleAs!=m_paiParticleBs, "Array ptrs equal");

	//Get ptr to array of rest lengths.
	m_pafRestLengths=clothArchetype.GetPtrToSeparationConstraintRestLengths();
	/*
	//Set up the rest lengths.  Copy from the archetype data then randomise a bit 
	//to create a bit of variety.
	for(int i=0;i<m_iNumConstraints;i++)
	{
		m_afRestLengths[i]=clothArchetype.GetConstraintRestLength(i);
		Assertf(m_afRestLengths[i]>0, "Constraint rest length must be positive");
	}
	*/
}

void CClothVolumeConstraints::SetArchetypeData(const class CClothArchetype& clothArchetype)
{
	//Zero everything before setting up the data.
	Init();

	//Store the number of constraints.
	m_iNumConstraints=clothArchetype.GetNumVolumeConstraints();
	Assertf(m_iNumConstraints>0, "Zero constraints");
	Assertf(m_iNumConstraints<MAX_NUM_CLOTH_VOLUME_CONSTRAINTS, "Too many constraints");

	//Get the ptrs to the arrays of particle indices.
	m_paiParticleAs=clothArchetype.GetPtrToVolumeConstraintParticleIdsA();
	m_paiParticleBs=clothArchetype.GetPtrToVolumeConstraintParticleIdsB();
	m_paiParticleCs=clothArchetype.GetPtrToVolumeConstraintParticleIdsC();
	m_paiParticleDs=clothArchetype.GetPtrToVolumeConstraintParticleIdsD();
	Assertf(m_paiParticleAs, "Null array ptr");
	Assertf(m_paiParticleBs, "Null array ptr");
	Assertf(m_paiParticleCs, "Null array ptr");
	Assertf(m_paiParticleDs, "Null array ptr");
	Assertf(m_paiParticleAs!=m_paiParticleBs, "Array ptrs equal");
	Assertf(m_paiParticleAs!=m_paiParticleCs, "Array ptrs equal");
	Assertf(m_paiParticleAs!=m_paiParticleDs, "Array ptrs equal");
	Assertf(m_paiParticleBs!=m_paiParticleCs, "Array ptrs equal");
	Assertf(m_paiParticleBs!=m_paiParticleDs, "Array ptrs equal");
	Assertf(m_paiParticleCs!=m_paiParticleDs, "Array ptrs equal");

	//Get ptr to array of rest volumes.
	m_pafRestVolumes=clothArchetype.GetPtrToVolumeConstraintRestVolumes();
}


#endif //NORTH_CLOTHS
