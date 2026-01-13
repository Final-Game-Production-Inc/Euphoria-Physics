//
// grprofile/drawgroup.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_DRAWGROUP_H
#define GRPROFILE_DRAWGROUP_H

#include "grprofile/drawitem.h"

namespace rage {

#if __PFDRAW

//=============================================================================
// PURPOSE
//   Contains pfDrawItemBase objects.  Groups may contain other groups.  The
//   only group that does not belong to another group is the pfDrawManager itself.
//
// <FLAG Component>
//
class pfDrawGroup : public pfDrawItemBase
{
public:
	pfDrawGroup(const char * name, pfDrawGroup * owningGroup, pfDrawManager * manager, bool startEnabled=false);

	void RegisterItem(pfDrawItemBase * itemNode);

	virtual void SetEnabled(bool v);
	virtual void SetChildrenEnabled(bool v);

#if __BANK
	virtual void AddWidgets(bkBank & bank);
	void EnableChildren();
	void DisableChildren();
#endif

#if !HACK_RDR2
protected:
#endif
	virtual void SetParentsEnabled(bool v);
protected:

	pfDrawItemBase* m_Children;
};

#endif // __PFDRAW

}	// namespace rage

#endif // GRPROFILE_DRAWGROUP_H

// <eof> grprofile/drawgroup.h
