#ifndef CLOTH_CONTACTS_H
#define CLOTH_CONTACTS_H

#define MAX_NUM_CLOTH_CONTACTS 64

#include "Debug/Debug.h"
#include "vector/vector3.h"

class CClothContacts
{
public:

	CClothContacts()
		: m_iNumContacts(0)
	{
	}
	~CClothContacts()
	{
	}

	void AddContact(const int iParticleId, const Vector3& vParticlePos, const Vector3& vNormal)
	{
		if(m_iNumContacts<MAX_NUM_CLOTH_CONTACTS)
		{
			m_aiParticleIds[m_iNumContacts]=iParticleId;
			m_avPositions[m_iNumContacts]=vParticlePos;
			m_avNormals[m_iNumContacts]=vNormal;
			m_iNumContacts++;
		}
	}

	void FreeContacts()
	{
		m_iNumContacts=0;
	}

	int GetNumContacts() const
	{
		return m_iNumContacts;
	}
	int GetParticleId(const int index) const
	{
		Assertf(index<m_iNumContacts, "Out of bounds");
		return m_aiParticleIds[index];
	}
	const Vector3& GetPosition(const int index) const
	{
		Assertf(index<m_iNumContacts, "Out of bounds");
		return m_avPositions[index];
	}
	const Vector3& GetNormal(const int index) const
	{
		Assertf(index<m_iNumContacts, "Out of bounds");
		return m_avNormals[index];
	}

private:

	int m_iNumContacts;
	int m_aiParticleIds[MAX_NUM_CLOTH_CONTACTS];
	Vector3 m_avPositions[MAX_NUM_CLOTH_CONTACTS];
	Vector3 m_avNormals[MAX_NUM_CLOTH_CONTACTS];
};

#endif //CLOTH_CONTACTS_H
