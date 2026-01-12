#ifndef CLOTH_CAPSULE_BOUNDS_H
#define CLOTH_CAPSULE_BOUNDS_H

#define MAX_NUM_PARTICLES_PER_CAPSULE	(4)
#define MAX_NUM_CLOTH_CAPSULE_BOUNDS	(32)
#define MAX_NUM_CLOTH_SKELETON_BONES	MAX_NUM_CLOTH_CAPSULE_BOUNDS

class CClothParticles;
#include "vector/vector3.h"
#include "vector/matrix34.h"
#include "crSkeleton/skeleton.h"

class CClothCapsuleBounds
{
public:

	friend class CCloth;
	friend class phInstBehaviourCloth;

	void Init()
	{
		m_iNumCapsuleBounds=0;
		m_paiParticleAs=0;
		m_paiParticleBs=0;
		m_pafLengths=0;
		m_pafRadii=0;
		m_iNumSkeletonNodes=0;
	}
	void Shutdown()
	{
		m_iNumCapsuleBounds=0;
		m_paiParticleAs=0;
		m_paiParticleBs=0;
		m_pafLengths=0;
		m_pafRadii=0;
		m_iNumSkeletonNodes=0;
	}

	//Initialise from archetype.
	void SetArchetypeData(const class CClothArchetype& clothArchetype);

	//Get all the capsule bound data.
	int GetNumCapsuleBounds() const
	{
		return m_iNumCapsuleBounds;
	}
	//Compute start and end points of capsule and return its length and radius.
	void ComputeCapsule(const int index, const CClothParticles& particles, Vector3& vA, Vector3& vB, float& fLength, float& fRadius) const;

	//Compute just the start and end points of capsule.
	void ComputeCapsuleEnds(const int index, const CClothParticles& particles, Vector3& vA, Vector3& vB) const;

#if !__SPU
	//Pose a skeleton from the capsule bound matrices (ppu only)
	void PoseSkeleton(crSkeleton& skeleton) const;
#else
	//Get a pointer to the matrix array so it can be copied across to spu via dma (spu only)
	int GetNumSkeletonNodes() const {return m_iNumSkeletonNodes;}
	const Matrix34* GetPtrToSkeletonMatrixArray() const {return &m_aSkeletonMatrices[0];}
#endif

private:

private:

	CClothCapsuleBounds()
	{
		Init();
	}

	~CClothCapsuleBounds()
	{
		Shutdown();
	}

	//Number of capsule bounds in cloth.
	int m_iNumCapsuleBounds;
	//Arrays of indices storing id of particles for each capsule bound.
	//Particle indices belong to archetype.
	//Arrays of lengths and radii of capsule bounds.
	//Lengths and radii belong to archetype.
	const int* m_paiParticleAs;
	const int* m_paiParticleBs;
	const float* m_pafLengths;
	const float* m_pafRadii;

	//Map between capsule bounds and skeleton nodes.
	const int* m_paiSkeletonNodes;
	//Skeleton matrices that get passed to skeleton.
	int m_iNumSkeletonNodes;
	Matrix34 m_aSkeletonMatrices[MAX_NUM_CLOTH_SKELETON_BONES] ;
	//Update an array of skeleton matrices from the capsule bound matrices.
	void UpdateSkeletonMatrices(const CClothParticles& particles, const Matrix34& transform, const CClothArchetype& archetype);

	//Get the data used to compute a capsule bound.
	//Get the particles that are used to compute the position of each end of a capsule.
	void GetParticleIndexAs(const int index, int& i0, int& i1, int& i2, int& i3) const
	{
		Assertf(index<m_iNumCapsuleBounds, "Out of bounds");
		i0=m_paiParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*index+0];
		i1=m_paiParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*index+1];
		i2=m_paiParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*index+2];
#if 3==MAX_NUM_PARTICLES_PER_CAPSULE
		i3=-1;
#elif 4==MAX_NUM_PARTICLES_PER_CAPSULE
		i3=m_paiParticleAs[MAX_NUM_PARTICLES_PER_CAPSULE*index+3];
#endif
	}
	void GetParticleIndexBs(const int index, int& i0, int& i1, int& i2, int& i3) const
	{
		Assertf(index<m_iNumCapsuleBounds, "Out of bounds");
		i0=m_paiParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*index+0];
		i1=m_paiParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*index+1];
		i2=m_paiParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*index+2];
#if 3==MAX_NUM_PARTICLES_PER_CAPSULE
		i3=-1;
#elif 4==MAX_NUM_PARTICLES_PER_CAPSULE
		i3=m_paiParticleBs[MAX_NUM_PARTICLES_PER_CAPSULE*index+3];
#endif
	}
	//Get the length and radius of a capsule.
	float GetLength(const int index) const
	{
		Assertf(index<m_iNumCapsuleBounds, "Out of bounds");
		return m_pafLengths[index];
	}
	float GetRadius(const int index) const
	{
		Assertf(index<m_iNumCapsuleBounds, "Out of bounds");
		return m_pafRadii[index];
	}
	//Compute the two ends of a capsule from the positions of the particles.
	void ComputeCapsuleEnds(const int index, const CClothArchetype& archetype, const Matrix34& transform, Vector3& vA, Vector3& vB) const;
};


#endif