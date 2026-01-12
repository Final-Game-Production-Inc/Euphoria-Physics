#include "ClothParticles.h"
#include "ClothArchetype.h"
#include "Debug/Debug.h"

#include "vector/matrix34.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

void CClothParticles::SetArchetypeData(const CClothArchetype& clothArchetype)
{
	//Zero everything before setting up the data.
	Init();

	//Store number of particles.
	m_iNumParticles=clothArchetype.GetNumParticles();
	Assertf(m_iNumParticles>0, "Must be positive number of particles");
	Assertf(m_iNumParticles<MAX_NUM_CLOTH_PARTICLES, "Too many particles");

	//Store damping rate of all particles.
	m_fDampingRate=clothArchetype.GetParticleDampingRate(0);
	Assertf(m_fDampingRate>=0, "Damping rate must be +ve");


	//Copy particle positions and inverse masses from archetype.
	for(int i=0;i<m_iNumParticles;i++)
	{
		m_avPositions[i]=clothArchetype.GetParticlePosition(i);
		m_avPositionPrevs[i]=m_avPositions[i];
		m_afInvMasses[i]=clothArchetype.GetParticleInvMass(i);
	}

	//Set ptr to particle radii.
	m_pafRadii=clothArchetype.GetPtrToParticleRadii();
}

void CClothParticles::Transform(const Matrix34& mat)
{
	for(int i=0;i<m_iNumParticles;i++)
	{
		//Vector3 vNewPos=mat*GetPosition(i);
		Vector3 vNewPos;
		mat.Transform(GetPosition(i),vNewPos);
		SetPosition(i,vNewPos);
		//vNewPos=mat*GetPositionPrev(i);
		mat.Transform(GetPositionPrev(i),vNewPos);
		SetPositionPrev(i,vNewPos);	
	}
}

#endif //NORTH_CLOTHS
