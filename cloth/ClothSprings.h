#ifndef CLOTH_SPRINGS_H
#define CLOTH_SPRINGS_H

class CClothArchetype;
#include "Debug/Debug.h"
#include "vector/matrix34.h"

#define MAX_NUM_CLOTH_SPRINGS		(256)

class CClothSprings
{
public:

	friend class CCloth;

	void Init()
	{
		m_iNumSprings=0;
	}
	void Shutdown()
	{
		m_iNumSprings=0;
	}

	//Set up the constraints from the archetype.
	//Some data is shared and some is copied then mangled a bit.
	void SetArchetypeData(const CClothArchetype& archetypeCloth);
	//Transform from body space to world space.
	void Transform(const Matrix34& UNUSED_PARAM(mat)){}

	int GetNumSprings() const
	{
		return m_iNumSprings;
	}
	int GetParticleIdA(const int index) const
	{
		Assertf(index<m_iNumSprings, "Out of bounds");
		Assertf(m_paiParticleAs,"Null ptr to particle index array");
		return m_paiParticleAs[index];
	}
	int GetParticleIdB(const int index) const
	{
		Assertf(index<m_iNumSprings, "Out of bounds");
		Assertf(m_paiParticleBs,"Null ptr to particle index array");
		return m_paiParticleBs[index];
	}
	float GetRestLength(const int index) const
	{
		Assertf(index<m_iNumSprings, "Out of bounds");
		Assert(m_pafRestLengths[index]>0);
		return m_pafRestLengths[index];
	}
	float GetAllowedLengthDifference(const int index) const
	{
		Assertf(index<m_iNumSprings, "Out of bounds");
		Assert(m_pafAllowedLengthDifferences[index]>=0);
		return m_pafAllowedLengthDifferences[index];
	}

private:

	CClothSprings()
		: m_iNumSprings(0)
	{
		Init();
	}
	~CClothSprings()
	{
		Shutdown();
	}

	int m_iNumSprings;
	//Ptrs to arrays of particle indices stored in cloth archetype.
	//Particle indices belong to archetype.
	const int* m_paiParticleAs;
	const int* m_paiParticleBs;
	//Target angles and allowed range from target angle.
	//This data is copied from archetype then randomised a bit to create a bit of variety.
	const float* m_pafRestLengths;
	const float* m_pafAllowedLengthDifferences;
};

#endif //CLOTH_SPRINGS_H
