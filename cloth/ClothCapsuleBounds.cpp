#include "ClothCapsuleBounds.h"
#include "ClothArchetype.h"
#include "ClothParticles.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

void CClothCapsuleBounds::SetArchetypeData(const CClothArchetype& clothArchetype)
{
	m_iNumCapsuleBounds=clothArchetype.GetNumCapsuleBounds();
	m_paiSkeletonNodes=clothArchetype.GetPtrToSkeletonNodes();
	m_paiParticleAs=clothArchetype.GetPtrToCapsuleBoundParticleAs();
	m_paiParticleBs=clothArchetype.GetPtrToCapsuleBoundParticleBs();
	m_pafLengths=clothArchetype.GetPtrToCapsuleBoundLengths();
	m_pafRadii=clothArchetype.GetPtrToCapsuleBoundRadii();
	m_iNumSkeletonNodes=clothArchetype.GetNumSkeletonBones();
	int i;
	for(i=0;i<m_iNumSkeletonNodes;i++)
	{
		m_aSkeletonMatrices[i].Identity();
	}
}

#if !__SPU
void CClothCapsuleBounds::PoseSkeleton(crSkeleton& skeleton) const
{
	int i;
	for(i=0;i<m_iNumSkeletonNodes;i++)
	{
		// We want to set the object matrices in the crSkeleton, the skeleton matrices in m_aSkeletonMatrices are already in object space
		// so just set them directly
		skeleton.GetObjectMtx(i) = RCC_MAT34V(m_aSkeletonMatrices[i]);
	}
}
#endif

void CClothCapsuleBounds::UpdateSkeletonMatrices(const CClothParticles& particles, const Matrix34& transform, const CClothArchetype& archetype)
{
	int i;
	for(i=0;i<GetNumCapsuleBounds();i++)
	{
		const int iNode=m_paiSkeletonNodes[i];
		Assertf(iNode<m_iNumSkeletonNodes, "Out of bounds skeleton node");

		//Get the ith capsule of the cloth using the cloth particles.
		// Compute capsule ends always gives us global positions

		Vector3 vA,vB;
		ComputeCapsuleEnds(i,particles,vA,vB);

		//Set the world translation of the node.
		Matrix34& partMatrix=m_aSkeletonMatrices[iNode];
		partMatrix.d=vA;

		//Set the world rotation of the node.
		//Compute the current capsule direction. 
		Vector3 vTo;
		vTo=vB-vA;
		vTo.Normalize();
		//Compute the archetype capsule direction.
		Vector3 vA0,vB0;
		ComputeCapsuleEnds(i,archetype,transform,vA0,vB0);
		Vector3 vFrom;
		vFrom=vB0-vA0;
		vFrom.Normalize();
		//Compute the matrix that transforms from the archetype direction to the current direction.
		Matrix34 mat;
		mat.Identity3x3();
		mat.RotateTo(vFrom,vTo);
		
		// Part matrices need to be in _object space_ , anim system will transform by entity matrix for us
		partMatrix = mat;
		partMatrix.d = vA - transform.d;
	}
}


void CClothCapsuleBounds::ComputeCapsule
(const int index, const CClothParticles& particles, 
 Vector3& vA, Vector3& vB, float& fLength, float& fRadius) const
{
	ComputeCapsuleEnds(index,particles,vA,vB);
	fLength=m_pafLengths[index];
	fRadius=m_pafRadii[index];
}

void CClothCapsuleBounds::ComputeCapsuleEnds
(const int index, const CClothParticles& particles, 
 Vector3& vA, Vector3& vB) const
{
	{
		int i0,i1,i2,i3;
		GetParticleIndexAs(index,i0,i1,i2,i3);
		int iNumParticles=0;
		vA.Zero();
		if(-1!=i0)
		{
			iNumParticles++;
			vA+=particles.GetPosition(i0);
		}
		if(-1!=i1)
		{
			iNumParticles++;
			vA+=particles.GetPosition(i1);
		}
		if(-1!=i2)
		{
			iNumParticles++;
			vA+=particles.GetPosition(i2);
		}
		if(-1!=i3)
		{
			iNumParticles++;
			vA+=particles.GetPosition(i3);
		}
		Assert(iNumParticles!=0);
		vA.Scale(1.0f/(1.0f*iNumParticles));
	}

	{
		int i0,i1,i2,i3;
		GetParticleIndexBs(index,i0,i1,i2,i3);
		int iNumParticles=0;
		vB.Zero();
		if(-1!=i0)
		{
			iNumParticles++;
			vB+=particles.GetPosition(i0);
		}
		if(-1!=i1)
		{
			iNumParticles++;
			vB+=particles.GetPosition(i1);
		}
		if(-1!=i2)
		{
			iNumParticles++;
			vB+=particles.GetPosition(i2);
		}
		if(-1!=i3)
		{
			iNumParticles++;
			vB+=particles.GetPosition(i3);
		}
		Assert(iNumParticles!=0);
		vB.Scale(1.0f/(1.0f*iNumParticles));
	}
}

void CClothCapsuleBounds::ComputeCapsuleEnds
(const int index, const CClothArchetype& archetype, const Matrix34& transform, Vector3& vA, Vector3& vB) const
{
	{
		int i0,i1,i2,i3;
		GetParticleIndexAs(index,i0,i1,i2,i3);
		int iNumParticles=0;
		Vector3 vA0;
		vA0.Zero();
		if(-1!=i0)
		{
			iNumParticles++;
			vA0+=archetype.GetParticlePosition(i0);
		}
		if(-1!=i1)
		{
			iNumParticles++;
			vA0+=archetype.GetParticlePosition(i1);
		}
		if(-1!=i2)
		{
			iNumParticles++;
			vA0+=archetype.GetParticlePosition(i2);
		}
		if(-1!=i3)
		{
			iNumParticles++;
			vA0+=archetype.GetParticlePosition(i3);
		}
		Assert(iNumParticles!=0);
		vA0.Scale(1.0f/(1.0f*iNumParticles));
		transform.Transform(vA0,vA);
	}

	{
		int i0,i1,i2,i3;
		GetParticleIndexBs(index,i0,i1,i2,i3);
		int iNumParticles=0;
		Vector3 vB0;
		vB0.Zero();
		if(-1!=i0)
		{
			iNumParticles++;
			vB0+=archetype.GetParticlePosition(i0);
		}
		if(-1!=i1)
		{
			iNumParticles++;
			vB0+=archetype.GetParticlePosition(i1);
		}
		if(-1!=i2)
		{
			iNumParticles++;
			vB0+=archetype.GetParticlePosition(i2);
		}
		if(-1!=i3)
		{
			iNumParticles++;
			vB0+=archetype.GetParticlePosition(i3);
		}
		Assert(iNumParticles!=0);
		vB0.Scale(1.0f/(1.0f*iNumParticles));
		transform.Transform(vB0,vB);
	}
}


#endif //NORTH_CLOTHS
