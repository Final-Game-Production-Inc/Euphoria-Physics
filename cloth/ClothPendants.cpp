#include "ClothPendants.h"
#include "ClothArchetype.h"
#include "Scene/Entity.h"

#include "Vector/vector3.h"
#include "vector/matrix34.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

void CClothPendantMethodRotator::Update(const float dt, Vector3& vPendantPosition)
{
	//Update the angle.
	m_fAngle+=m_fAngularSpeed*dt;	
	//Compute the pendant position.
	vPendantPosition.x=m_vMotionCentre.x+m_fRadius*cosf(m_fAngle);
	vPendantPosition.y=m_vMotionCentre.y;
	vPendantPosition.z=m_vMotionCentre.z+m_fRadius*sinf(m_fAngle);
}

void CClothPendantMethodRotator::Transform(const Matrix34& mat)
{
	Vector3 vNewPos;
	mat.Transform(m_vMotionCentre,vNewPos);
	//Vector3 vNewPos=mat*m_vMotionCentre;
	m_vMotionCentre=vNewPos;
}

CClothPendantMethodEntityOffset::CClothPendantMethodEntityOffset(CEntity* pEntity, const Vector3& vOffset)
: CClothPendantMethod(),
m_pEntity(pEntity),
m_vOffset(vOffset),
m_vPosition(Vector3(VEC3_ZERO))
{
	Assertf(pEntity, "Null entity ptr");
	if(m_pEntity)
	{
		pEntity->TransformIntoWorldSpace(m_vPosition, m_vOffset);
	}
}

CClothPendantMethodEntityOffset::CClothPendantMethodEntityOffset(const CClothPendantMethodEntityOffset& src)
: m_pEntity(src.m_pEntity),
m_vOffset(src.m_vOffset),
m_vPosition(Vector3(VEC3_ZERO))
{
	Assertf(m_pEntity, "Null entity ptr");
	if(m_pEntity)
	{
		m_pEntity->TransformIntoWorldSpace(m_vPosition, m_vOffset);
	}
}

CClothPendantMethodEntityOffset::~CClothPendantMethodEntityOffset()
{
}

CClothPendantMethodEntityOffset& CClothPendantMethodEntityOffset::operator=(const CClothPendantMethodEntityOffset& src)
{
	m_pEntity=src.m_pEntity;
	m_vOffset=src.m_vOffset;
	m_vPosition=Vector3(VEC3_ZERO);

	Assertf(m_pEntity, "Null entity ptr");
	if(m_pEntity)
	{
		m_pEntity->TransformIntoWorldSpace(m_vPosition, m_vOffset);
	}

	return *this;
}


void CClothPendantMethodEntityOffset::Update(const float UNUSED_PARAM(dt), Vector3& vPendantPos)
{
	if(m_pEntity)
	{
		m_pEntity->TransformIntoWorldSpace(m_vPosition, m_vOffset);
	}
	vPendantPos=m_vPosition;
}

void CClothPendantMethodEntityOffset::Transform(const Matrix34& UNUSED_PARAM(mat))
{
}

void CClothPendants::SetArchetypeData(const CClothArchetype& clothArchetype)
{
	//Zero everything before setting up the data.
	Init();

	//Store the number of pendants.
	m_iNumPendants=clothArchetype.GetNumPendants();
	Assertf(m_iNumPendants>=0, "Num pendants must be zero or greater");
	Assertf(m_iNumPendants<MAX_NUM_CLOTH_PENDANTS, "Too many pendants");
	for(int i=0;i<m_iNumPendants;i++)
	{
		const int iType=clothArchetype.GetPendantMethod(i)->GetType();
		switch(iType)
		{
		case CClothPendantMethod::CLOTH_PENDANT_METHOD_ROTATOR:
			m_apMethods[i]=rage_new CClothPendantMethodRotator(static_cast<const CClothPendantMethodRotator&>(*clothArchetype.GetPendantMethod(i)));
			break;
		case CClothPendantMethod::CLOTH_PENDANT_METHOD_ENTITY_OFFSET:
			m_apMethods[i]=rage_new CClothPendantMethodEntityOffset(static_cast<const CClothPendantMethodEntityOffset&>(*clothArchetype.GetPendantMethod(i)));
			break;
		default:
			Assertf(false, "Unknown method type");
			m_apMethods[i]=0;
			break;
		}
	}

	//Set the ptr to the particle index array.
	m_paiParticleIds=clothArchetype.GetPtrToPedantParticleIds();
	Assertf(m_paiParticleIds, "Null ptr to particle id array");
}


#endif // NORTH_CLOTHS
