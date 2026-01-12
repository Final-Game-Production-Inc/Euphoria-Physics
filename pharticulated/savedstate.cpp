// 
// pharticulated/savedstate.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "savedstate.h"
#include "articulatedbody.h"

namespace rage {

phArticulatedBodySavedState::phArticulatedBodySavedState()
	: m_Parts(NULL)
	, m_NumParts(-1)
{
}

phArticulatedBodySavedState::~phArticulatedBodySavedState()
{
	delete [] m_Parts;
}

void phArticulatedBodySavedState::SaveState(phArticulatedBody& body)
{
	int newNumParts = body.GetNumBodyParts();
	if (newNumParts != m_NumParts)
	{
		delete [] m_Parts;
		m_Parts = rage_new phArticulatedBodyPartSavedState[newNumParts];
		m_NumParts = newNumParts;
	}

	for (int partIndex = 0; partIndex < newNumParts; ++partIndex)
	{
		m_Parts[partIndex].SaveState(&body, partIndex);
	}
}

void phArticulatedBodySavedState::RestoreState(phArticulatedBody& body)
{
	Assert(body.GetNumBodyParts() == m_NumParts);

	for (int partIndex = 0; partIndex < m_NumParts; ++partIndex)
	{
		m_Parts[partIndex].RestoreState(&body, partIndex);
	}
}

void phArticulatedBodyPartSavedState::SaveState(phArticulatedBody *body, int partIndex)
{
	m_LinVelocity = body->GetLinearVelocity(partIndex);
	m_AngVelocity = body->GetAngularVelocity(partIndex);
	m_Matrix = body->GetLink(partIndex).GetMatrix();
}

void phArticulatedBodyPartSavedState::RestoreState(phArticulatedBody *body, int partIndex)
{
	body->SetLinearVelocity(partIndex, m_LinVelocity);
	body->SetAngularVelocity(partIndex, m_AngVelocity);
	body->GetLink(partIndex).SetMatrix(RCC_MATRIX34(m_Matrix));
}

} // namespace rage
