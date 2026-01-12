#ifndef CLOTH_PARTICLES_H
#define CLOTH_PARTICLES_H

#define MAX_NUM_CLOTH_PARTICLES		(128)

#include "vector/vector3.h"

class CClothParticles
{
public:

	friend class CCloth;

	void Init()
	{
		m_iNumParticles=0;
		m_fDampingRate=0;
		m_pafRadii=0;
	}
	void Shutdown()
	{
		m_iNumParticles=0;
		m_fDampingRate=0;
		m_pafRadii=0;
	}

	//Initialise from archetype.
	void SetArchetypeData(const class CClothArchetype& clothArchetype);
	//Transform from body space to world space.
	void Transform(const Matrix34& mat);


	int GetNumParticles() const
	{
		return m_iNumParticles;
	}
	const Vector3& GetPosition(const int id) const
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		return m_avPositions[id];
	}
	const Vector3& GetPositionPrev(const int id) const
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		return m_avPositionPrevs[id];
	}
	float GetInverseMass(const int id) const
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		return m_afInvMasses[id];
	}
	float GetDampingRate(const int ASSERT_ONLY(id)) const
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		return m_fDampingRate;
	}
	float GetRadius(const int id) const
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		Assertf(m_pafRadii, "Null ptr to radii array");
		return m_pafRadii[id];
	}

	void SetPosition(const int id, const Vector3& vPos)
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		if(id<m_iNumParticles)
		{
			m_avPositions[id]=vPos;
		}
	}
	void SetPositionPrev(const int id, const Vector3& vPos)
	{
		Assertf(id<m_iNumParticles,"Particle doesn't exist");
		if(id<m_iNumParticles)
		{
			m_avPositionPrevs[id]=vPos;
		}
	}
	
private:

	CClothParticles()
		: m_iNumParticles(0),
		m_fDampingRate(0)
	{
		Init();
	}
	~CClothParticles()
	{
		Shutdown();
	}

	//Number of particles in cloth.
	int m_iNumParticles;

	//Damping rate used in particle update (assume damping is same for every particle).
	float m_fDampingRate;

	//Array of particle inverse masses.
	float m_afInvMasses[MAX_NUM_CLOTH_PARTICLES];

	//Array of current positions.
	Vector3 m_avPositions[MAX_NUM_CLOTH_PARTICLES];
	//Array of previous positions.
	Vector3 m_avPositionPrevs[MAX_NUM_CLOTH_PARTICLES];

	//Array of particle radii (owned by archetype).
	const float* m_pafRadii;

};

#endif //CLOTH_PARTICLES_H
