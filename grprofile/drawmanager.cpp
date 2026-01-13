//
// profile/drawmanager.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "grprofile/drawmanager.h"

#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "string/string.h"
#include "system/param.h"
#include "system/namedpipe.h"


namespace rage {

#if __PFDRAW

__THREAD bool pfDrawManager::sm_DrawImmediately = false;

PARAM(pfdrawon, "[profile] Enable top-level profile draw group");

//==============================================================
// pfDrawManager

pfDrawManager::pfDrawManager(const char * name)
	: pfDrawGroup(name,NULL,NULL)
	, m_Initialized(false)
	, m_BeginDepth(0)
	, m_PushedBatcher(NULL)
{
	m_LastDrawCamera.Identity();

#if __BANK
	m_Bank = NULL;
#endif
}


pfDrawManager::~pfDrawManager()
{
	Assertf(!m_Initialized && !m_Bank, "pfDrawManager must be Shutdown before being destroyed.");
}


void pfDrawManager::Init(int bufferSize, bool BANK_ONLY(createWidgets), grcBatcher::BufferFullMode fullMode)
{
	Assert(!m_Initialized);

	m_Batcher.Init(bufferSize, fullMode);

#if __BANK
	if(createWidgets)
	{
		m_Bank = &BANKMGR.RegisterBank("rage - Profile Draw",datCallback(MFA1(pfDrawManager::AddWidgets),this,0,true));
	}
#endif

	m_Initialized = true;

	SetEnabled(PARAM_pfdrawon.Get());
}


void pfDrawManager::Shutdown()
{
	if(m_Initialized)
	{
		m_Batcher.Init(0);
	}

#if __BANK
	if(m_Bank)
	{
		BANKMGR.DestroyBank(*m_Bank);
		m_Bank = NULL;
	}
#endif

	m_Initialized = false;
}


#if __BANK
void pfDrawManager::AddWidgets(bkBank & bank)
{
	pfDrawGroup::AddWidgets(bank);
}
#endif // __BANK


void pfDrawManager::Render(bool flush)
{
	sysCriticalSection lock(m_CriticalSection);

	Assert(m_Initialized);
	Assert(m_BeginDepth == 0);

	// Render the data.
    grcWorldIdentity();
	grcBindTexture(NULL);
	m_Batcher.Render(flush);
}


void pfDrawManager::SendDrawCameraForServer(const Matrix34 & matrix)
{
	m_LastDrawCamera.Set(matrix);
}


void pfDrawManager::Flush()
{
	sysCriticalSection lock(m_CriticalSection);

	Assert(m_Initialized);
	m_Batcher.Flush();
}


//==============================================================

pfDrawManager & GetRageProfileDraw()
{
	static pfDrawManager sDrawManager("RAGE pfDraw");
	return sDrawManager;
}

#endif // __PFDRAW

} // namespace rage

// <eof> profile/drawmanager.cpp
