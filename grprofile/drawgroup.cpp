//
// profile/drawgroup.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "grprofile/drawgroup.h"

#include "bank/bank.h"
#include "vector/colors.h"


using namespace rage;

#if __PFDRAW

namespace rage
{
	extern bool g_IsStaticInitializationFinished;
}


//==============================================================
// pfDrawGroup

pfDrawGroup::pfDrawGroup
	(const char * name, pfDrawGroup * owningGroup, pfDrawManager * manager, bool startEnabled)
:	pfDrawItemBase(name,Color_white,0,owningGroup,manager)
{
	m_Enabled = startEnabled;

	if (g_IsStaticInitializationFinished) 
	{
		// This must be heap or stack constructed, so NULL out the m_Children pointer.
		// We don't want to do this for static initted objects because we might have
		// adjusted the m_Children pointer before the object's been constructed.
		m_Children = NULL;
	}
}


void pfDrawGroup::RegisterItem(pfDrawItemBase * newItem)
{
	if (!m_Children)
	{
		m_Children = newItem;
	}
	else
	{
		pfDrawItemBase* item = m_Children;
		while(item->m_Next)
		{
			item = item->m_Next;
		}
		item->m_Next = newItem;
		newItem->m_Next = NULL;
	}
}


void pfDrawGroup::SetEnabled(bool v)
{
	pfDrawItemBase::SetEnabled(v);
	pfDrawItemBase * itemNode = m_Children;
	while(itemNode)
	{
		itemNode->SetParentsEnabled(v && m_ParentsEnabled);
		itemNode = itemNode->m_Next;
	}
}


void pfDrawGroup::SetParentsEnabled(bool v)
{
	pfDrawItemBase::SetParentsEnabled(v);
	pfDrawItemBase * itemNode = m_Children;
	while(itemNode)
	{
		itemNode->SetParentsEnabled(v && m_Enabled);
		itemNode = itemNode->m_Next;
	}
}


void pfDrawGroup::SetChildrenEnabled(bool v)
{
	pfDrawItemBase * itemNode = m_Children;
	while(itemNode)
	{
		itemNode->SetEnabled(v);
		itemNode = itemNode->m_Next;
	}
}


#if __BANK
void pfDrawGroup::AddWidgets(bkBank & bank)
{
	bank.PushGroup(m_Name,false);
	pfDrawItemBase::AddWidgets(bank);
	pfDrawItemBase * itemNode = m_Children;
	while(itemNode)
	{
		itemNode->AddWidgets(bank);
		itemNode = itemNode->m_Next;
	}
	bank.PopGroup();
}
#endif // __BANK


#if __BANK
void pfDrawGroup::EnableChildren()
{
	SetChildrenEnabled(true);
}
#endif // __BANK


#if __BANK
void pfDrawGroup::DisableChildren()
{
	SetChildrenEnabled(false);
}
#endif // __BANK

#endif // __PFDRAW

// <eof> profile/drawgroup.cpp
