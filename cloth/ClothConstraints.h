#ifndef CLOTH_CONSTRAINTS_H
#define CLOTH_CONSTRAINTS_H

#include "ClothParticles.h"
#include "Debug/Debug.h"

#define MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS MAX_NUM_CLOTH_PARTICLES*4//Only allowing 4 constraints per particle
#define MAX_NUM_CLOTH_VOLUME_CONSTRAINTS MAX_NUM_CLOTH_PARTICLES*4//Only allowing 4 constraints per particle

//Array of cloth constraints.
class CClothSeparationConstraints
{
public:

	friend class CCloth;

	void Init()
	{
		m_iNumConstraints=0;
		m_paiParticleAs=0;
		m_paiParticleBs=0;
		m_pafRestLengths=0;
		/*
		for(int i=0;i<MAX_NUM_CLOTH_CONSTRAINTS;i++)
		{
			m_afRestLengths[i]=0;
		}
		*/
	}
	void Shutdown()
	{
		m_iNumConstraints=0;
		m_paiParticleAs=0;
		m_paiParticleBs=0;
		m_pafRestLengths=0;
	}

	//Set up the constraints from the archetype.
	//Some data is shared and some is copied then mangled a bit.
	void SetArchetypeData(const class CClothArchetype& clothArchetype);
	//Transform from body space to world space.
	void Transform(const Matrix34& UNUSED_PARAM(mat)){}

	//Get all the constraint data.
	int GetNumConstraints() const
	{
		return m_iNumConstraints;
	}
	int GetParticleIndexA(const int iConstraintIndex) const 
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		Assertf(m_paiParticleAs, "Null ptr to array of particle indices");
		return m_paiParticleAs[iConstraintIndex];
	}
	int GetParticleIndexB(const int iConstraintIndex) const 
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		Assertf(m_paiParticleBs, "Null ptr to array of particle indices");
		return m_paiParticleBs[iConstraintIndex];
	}
	float GetRestLength(const int iConstraintIndex) const
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		return m_pafRestLengths[iConstraintIndex];
	}

private:

	CClothSeparationConstraints()
		: m_iNumConstraints(0)
	{
		Init();
	}

	~CClothSeparationConstraints()
	{
		Shutdown();
	}

	//Number of constraints in cloth.
	int m_iNumConstraints;

	//Arrays of indices storing id of particle A and particle B for each constraint.
	//Particle indices belong to archetype.
	const int* m_paiParticleAs;
	const int* m_paiParticleBs;

	//Array of rest lengths of each constraint.
	const float* m_pafRestLengths;
};

class CClothVolumeConstraints
{
public:

	friend class CCloth;

	//Compute the volume of a tetrahedron.
	static float ComputeVolume(const Vector3& v1, const Vector3& v2, const Vector3& v3, const Vector3& v4)
	{
#define ONE_SIXTH (0.166666667f)
		const Vector3 v21=v2-v1;
		const Vector3 v31=v3-v1;
		const Vector3 v41=v4-v1;
		Vector3 cross;
		cross.Cross(v21,v31);
		const float fVol=cross.Dot(v41)*ONE_SIXTH;
		Assertf(fVol>0, "Vol should be +ve");
		return fVol;
	}

	static ScalarV ComputeVolume(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3, const Vec3V& v4)
	{
#define ONE_SIXTH (0.166666667f)
		const ScalarV oneSixth=ScalarVFromF32(ONE_SIXTH);
		const Vec3V v21=v2-v1;
		const Vec3V v31=v3-v1;
		const Vec3V v41=v4-v1;
		const Vec3V cross=Cross(v21,v31);
		const ScalarV fVol=Dot(cross,v41)*oneSixth;
		return fVol;
	}


	void Init()
	{
		m_iNumConstraints=0;
		m_paiParticleAs=0;
		m_paiParticleBs=0;
		m_paiParticleCs=0;
		m_paiParticleDs=0;
		m_pafRestVolumes=0;
	}
	void Shutdown()
	{
		m_iNumConstraints=0;
		m_paiParticleAs=0;
		m_paiParticleBs=0;
		m_paiParticleCs=0;
		m_paiParticleDs=0;
		m_pafRestVolumes=0;
	}

	//Set up the constraints from the archetype.
	//Some data is shared and some is copied then mangled a bit.
	void SetArchetypeData(const class CClothArchetype& clothArchetype);
	//Transform from body space to world space.
	void Transform(const Matrix34& UNUSED_PARAM(mat)){}

	//Get all the constraint data.
	int GetNumConstraints() const
	{
		return m_iNumConstraints;
	}
	int GetParticleIndexA(const int iConstraintIndex) const 
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		Assertf(m_paiParticleAs, "Null ptr to array of particle indices");
		return m_paiParticleAs[iConstraintIndex];
	}
	int GetParticleIndexB(const int iConstraintIndex) const 
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		Assertf(m_paiParticleBs, "Null ptr to array of particle indices");
		return m_paiParticleBs[iConstraintIndex];
	}
	int GetParticleIndexC(const int iConstraintIndex) const 
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		Assertf(m_paiParticleCs, "Null ptr to array of particle indices");
		return m_paiParticleCs[iConstraintIndex];
	}
	int GetParticleIndexD(const int iConstraintIndex) const 
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		Assertf(m_paiParticleDs, "Null ptr to array of particle indices");
		return m_paiParticleDs[iConstraintIndex];
	}
	float GetRestVolume(const int iConstraintIndex) const
	{
		Assertf(iConstraintIndex<m_iNumConstraints, "Constraint index out of bounds");
		return m_pafRestVolumes[iConstraintIndex];
	}


private:

	CClothVolumeConstraints()
		: m_iNumConstraints(0)
	{
		Init();
	}

	~CClothVolumeConstraints()
	{
		Shutdown();
	}

	//Number of constraints in cloth.
	int m_iNumConstraints;

	//Arrays of indices storing id of particle A//B/C/D for each constraint.
	//Particle indices belong to archetype.
	const int* m_paiParticleAs;
	const int* m_paiParticleBs;
	const int* m_paiParticleCs;
	const int* m_paiParticleDs;

	//Array of rest lengths of each constraint.
	const float* m_pafRestVolumes;
};

#endif //CLOTH_CONSTRAINTS_H
