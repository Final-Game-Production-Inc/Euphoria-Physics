#include "ClothSprings.h"
#include "ClothArchetype.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

void CClothSprings::SetArchetypeData(const CClothArchetype& clothArchetype)
{
	//Zero everything before setting up the data.
	Init();

	//Store the number of springs.
	m_iNumSprings=clothArchetype.GetNumSprings();
	Assertf(m_iNumSprings>=0, "Negative number of springs");
	Assertf(m_iNumSprings<MAX_NUM_CLOTH_SPRINGS, "Too many springs");

	//Get the ptrs to arrays of particle indices.
	m_paiParticleAs=clothArchetype.GetPtrToSpringParticleIdsA();
	m_paiParticleBs=clothArchetype.GetPtrToSpringParticleIdsB();
	Assertf(m_paiParticleAs, "Null ptr to array");
	Assertf(m_paiParticleBs, "Null ptr to array");
	Assertf(m_paiParticleAs!=m_paiParticleBs, "Two array ptrs are equal");

	m_pafRestLengths=clothArchetype.GetPtrToSpringRestLengths();
	m_pafAllowedLengthDifferences=clothArchetype.GetPtrToSpringAllowedLengthDifferrences();
	/*
	//Set up the target angles.  Copy from archetype then randomise a bit 
	//to create a bit of variety.
	for(int i=0;i<clothArchetype.GetNumSprings();i++)
	{
		m_afRestLengths[i]=clothArchetype.GetSpringRestLength(i);
		m_afAllowedLengthDifferences[i]=clothArchetype.GetSpringAllowedLengthDifference(i);
		Assertf(m_afRestLengths[i]>=0, "Illegal rest length");
		Assertf(m_afAllowedLengthDifferences[i]>=0, "Illegal allowed length difference");
	}
	*/
}

#endif // NORTH_CLOTHS
