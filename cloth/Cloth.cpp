// rage headers

// Framework headers
#include "grcore/debugdraw.h"
#include "fwmaths/Vector.h"

// game headers
#include "Cloth/Cloth.h"
#include "Cloth/ClothArchetype.h"
#include "Cloth/ClothMgr.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

FW_INSTANTIATE_CLASS_POOL(CCloth, CClothMgr::MAX_NUM_CLOTHS, "CCloth");

void CCloth::SetArchetype(const CClothArchetype* pArchetype)
{
	Assert(pArchetype);

	m_pClothArchetype=pArchetype;

	m_particles.SetArchetypeData(*pArchetype);		
	m_separationConstraints.SetArchetypeData(*pArchetype);
	m_volumeConstraints.SetArchetypeData(*pArchetype);
	m_springs.SetArchetypeData(*pArchetype);			
	m_pendants.SetArchetypeData(*pArchetype);
	m_capsuleBounds.SetArchetypeData(*pArchetype);
}

void CCloth::Transform(const Matrix34& transform)
{
	m_transform=transform;
	m_particles.Transform(transform);
	m_separationConstraints.Transform(transform);
	m_volumeConstraints.Transform(transform);
	m_springs.Transform(transform);
	m_pendants.Transform(transform);
}

void CCloth::UpdateSkeletonMatrices(const CClothArchetype& clothArchetype)
{
	m_capsuleBounds.UpdateSkeletonMatrices(m_particles,m_transform,clothArchetype);
}

#if !__SPU
void CCloth::PoseSkeleton(crSkeleton& skeleton) const 
{
	m_capsuleBounds.PoseSkeleton(skeleton);
}
#else	
int CCloth::GetNumSkeletonNodes() const
{
	return m_capsuleBounds.GetNumSkeletonNodes();
}
const Matrix34* CCloth::GetSkeletonMatrixArrayPtr() const
{
	return m_capsuleBounds.GetPtrToSkeletonMatrixArray();
}
#endif


#endif // NORTH_CLOTHS
















