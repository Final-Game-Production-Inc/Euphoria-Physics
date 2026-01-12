// 
// physics/debugconetacts.h
// 
// Copyright (C) 1999-20011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef __PHYSICS_DEBUG_CONTACTS_H__
#define __PHYSICS_DEBUG_CONTACTS_H__
#include "debugevents.h"

#if PDR_ENABLED

#include "physics\contactiterator.h"

namespace rage {
	namespace debugPlayback	{
		void RecordContact(	const phContactIterator &impact);
		void RecordContact(	const phCachedContactIterator &impact);
		void RecordContact(const phManifold &manifold, int iContact, bool bIsInstanceAOwner);
		void ClearCurrentContact();
		void RecordModificationToContact( const phContactConstIterator &impact, const char *pModification, const char *pValue );
		void RecordModificationToContact( const phCachedContactIterator &impact, const char *pModification, const char *pValue );
		void RecordModificationToContact( const phContactConstIterator &impact, const char *pModification, float fValue );
		void RecordModificationToContact( const phCachedContactIterator &impact, const char *pModification, float fValue );
		void RecordModificationToContact( const phContactConstIterator &impact, const char *pModification, Vec3V_In vValue, bool bIs3dPosition=false );
		void RecordNMFindImpacts(const phInst* pThisInst, const phInst* pOtherInst, bool bReturned, const char *pIdentifier);
	}
}

#endif //PDR_ENABLED
#endif //__PHYSICS_DEBUG_CONTACTS_H__
