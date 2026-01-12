#ifndef CLOTH_PENDANTS_H
#define CLOTH_PENDANTS_H

class CEntity;

#include "Debug/Debug.h"
#include "Scene/RegdRefTypes.h"

#include "vector/vector3.h"

#define MAX_NUM_CLOTH_PENDANTS 4

class CClothPendantMethod
{
public:

	enum
	{
		CLOTH_PENDANT_METHOD_ROTATOR=0,
		CLOTH_PENDANT_METHOD_ENTITY_OFFSET
	};

	CClothPendantMethod(){}
	virtual ~CClothPendantMethod(){}

	virtual int GetType() const=0;
	//Update and recompute the pendant position.
	virtual void Update(const float dt, Vector3& vPendantPos)=0;

	//Transform from body space to world space.
	virtual void Transform(const Matrix34& mat)=0;

protected:

};

class CClothPendantMethodRotator : public CClothPendantMethod
{
public:

	CClothPendantMethodRotator(const Vector3& vMotionCentre, const float fRadius, const float fAngularSpeed)
		: CClothPendantMethod(),
		m_vMotionCentre(vMotionCentre),
		m_fRadius(fRadius),
		m_fAngularSpeed(fAngularSpeed),
		m_fAngle(0)
	{
	}
	CClothPendantMethodRotator(const CClothPendantMethodRotator& src)
		:  m_vMotionCentre(src.m_vMotionCentre),
		m_fRadius(src.m_fRadius),
		m_fAngularSpeed(src.m_fAngularSpeed),
		m_fAngle(0)
	{
	}
	~CClothPendantMethodRotator()
	{
	}


	CClothPendantMethodRotator& operator=(const CClothPendantMethodRotator& src)
	{
		m_vMotionCentre=src.m_vMotionCentre;
		m_fRadius=src.m_fRadius;
		m_fAngularSpeed=src.m_fAngularSpeed;
		m_fAngle=0;
		return *this;
	}
	virtual int GetType() const {return CLOTH_PENDANT_METHOD_ROTATOR;}
	//Update and recompute the pendant position.
	virtual void Update(const float dt, Vector3& vPendantPosition);
	//Transform from body space to world space.
	virtual void Transform(const Matrix34& mat);

private:

	Vector3 m_vMotionCentre;
	float m_fRadius;
	float m_fAngularSpeed;

	//Integrated angle.
	float m_fAngle;
};

class CClothPendantMethodEntityOffset : public CClothPendantMethod
{
public:

	CClothPendantMethodEntityOffset(CEntity* pEntity, const Vector3& vOffset);
	CClothPendantMethodEntityOffset(const CClothPendantMethodEntityOffset& src);
	~CClothPendantMethodEntityOffset();

	CClothPendantMethodEntityOffset& operator=(const CClothPendantMethodEntityOffset& src);

	virtual int GetType() const {return CLOTH_PENDANT_METHOD_ENTITY_OFFSET;}
	//Update and recompute the pendant position.
	virtual void Update(const float dt, Vector3& vPendantPos);
	//Transform from body space to world space.
	virtual void Transform(const Matrix34& mat);

private:

	RegdEnt m_pEntity;
	Vector3 m_vOffset;

	Vector3 m_vPosition;
};

class CClothPendants
{
public:

	friend class CCloth;

	void Init()
	{
		m_iNumPendants=0;
		m_paiParticleIds=0;
		for(int i=0;i<MAX_NUM_CLOTH_PENDANTS;i++)
		{
			m_apMethods[i]=0;
		}
	}
	void Shutdown()
	{
		m_iNumPendants=0;
		m_paiParticleIds=0;
		for(int i=0;i<MAX_NUM_CLOTH_PENDANTS;i++)
		{
			if(m_apMethods[i]) delete m_apMethods[i];
			m_apMethods[i]=0;
		}
	}

	//Initialise from archetype data.
	void SetArchetypeData(const class CClothArchetype& clothArchetype);
	//Transform from body space to world space.
	void Transform(const Matrix34& mat)
	{
		for(int i=0;i<m_iNumPendants;i++)
		{
			Assertf(m_apMethods[i], "Null ptr to pendant method");
			if(m_apMethods[i])
			{
				m_apMethods[i]->Transform(mat);
			}
		}
	}
	//Update all pendants and store pendant positions in an array.
	void Update(const float dt, Vector3* pavPendantPositions, const int ASSERT_ONLY(iNumPendantPositions))
	{
		Assertf(m_iNumPendants<iNumPendantPositions, "Array of pendant positions needs to be bigger");
		for(int i=0;i<m_iNumPendants;i++)
		{
			if(m_apMethods[i])
			{
				m_apMethods[i]->Update(dt,pavPendantPositions[i]);
			}
		}
	}
	//Get the number of pendants.
	int GetNumPendants() const
	{
		return m_iNumPendants;
	}
	//Get the particle index of a pendant.
	int GetParticleId(const int index) const 
	{
		Assertf(index<m_iNumPendants, "Out of bounds");
		Assertf(m_paiParticleIds, "Null ptr to array of particle ids");
		return m_paiParticleIds[index];
	}

private:

	CClothPendants()
	{
		Init();
	}
	~CClothPendants()
	{
		Shutdown();
	}

	//Number of pendants.
	int m_iNumPendants;
	//Pointer to array of particle indices (one for each pendant).
	//Particle indices belong to the archetype.
	const int* m_paiParticleIds;	
	//Array of pointers to pendant methods (one method for each pendant).
	CClothPendantMethod* m_apMethods[MAX_NUM_CLOTH_PENDANTS];
};

#endif //CLOTH_PENDANTS_H
