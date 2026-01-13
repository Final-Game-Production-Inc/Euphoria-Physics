// 
// grcore/adapter_common.cpp
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved.
// 

#include "grcore/d3dwrapper.h"

#if __WIN32PC

#include "grcore/device.h"
#include "grcore/adapter.h"
#include "grcore/adapter_d3d11.h"

namespace rage {

grcAdapterManager*			grcAdapterManager::sm_pInstance = NULL;
atArray<grcAdapter*>		grcAdapterManager::sm_aAdapters;
s32							grcAdapterManager::sm_OculusAdapter = -1;
char						grcAdapterManager::sm_OculusOutputMonitorName[MAX_ADAPTER_NAME_LEN] = { 0 };

grcAdapter::grcAdapter() : sm_OculusMonitor(-1)
{
}

grcAdapter::~grcAdapter()
{
}

void grcAdapterManager::InitClass(u32 uFormat)
{
	if (sm_pInstance == NULL)
	{
		grcAdapterManager* pAdapterManager = NULL;

#if __D3D11
		if (GRCDEVICE.GetDxFeatureLevel() >= 1000 || true)
		{
			pAdapterManager = rage_new grcAdapterManagerD3D11();
		}
#endif // __D3D11
/*		if (!pAdapterManager)
		{
			pAdapterManager = rage_new grcAdapterManagerD3D9();
		}*/

		AssertMsg(pAdapterManager, "Failed to create adapter manager");
		if (pAdapterManager == NULL)
			return;

		sm_pInstance = pAdapterManager;

		// Enumerate adapters.
		sm_pInstance->Enumerate(uFormat);
	}
}

void grcAdapterManager::InitClass(u32 uFormat, u32 
#if __D3D11								  
								  uDXVersion
#endif // __D3D11							  
								  )
{
	if (sm_pInstance == NULL)
	{
		grcAdapterManager* pAdapterManager = NULL;

#if __D3D11
		if (uDXVersion >= 1000)
		{
			pAdapterManager = rage_new grcAdapterManagerD3D11();
		}
#endif // __D3D11
/*		if (!pAdapterManager && uDXVersion <= 9000)
		{
			pAdapterManager = rage_new grcAdapterManagerD3D9();
		}*/

		AssertMsg(pAdapterManager, "Failed to create adapter manager");
		if (pAdapterManager == NULL)
			return;

		sm_pInstance = pAdapterManager;

		// Enumerate adapters.
		sm_pInstance->Enumerate(uFormat);
	}
}

void grcAdapterManager::ShutdownClass()
{
	for (int adapterIndex = 0; adapterIndex < (int)sm_aAdapters.size(); ++adapterIndex)
	{
		delete sm_aAdapters[adapterIndex];
	}

	sm_aAdapters.clear();

	if (sm_pInstance)
	{
		delete sm_pInstance;
		sm_pInstance = NULL;
	}
}

void grcAdapterManager::SetOculusMonitor(const char* pszOculusMonitor)
{
	formatf(sm_OculusOutputMonitorName, sizeof(sm_OculusOutputMonitorName) - 1, pszOculusMonitor);
}

grcAdapterManager::grcAdapterManager()
{
}

grcAdapterManager::~grcAdapterManager()
{
}

const grcAdapter* grcAdapterManager::GetAdapter(u32 uAdapter) const
{
	AssertMsg(uAdapter < (u32) sm_aAdapters.size(), "Invalid adapter");

	if (uAdapter < (u32)sm_aAdapters.size())
		return sm_aAdapters[uAdapter];

	return NULL;
}

const grcAdapter* grcAdapterManager::GetOculusAdapter() const 
{ 
	Assertf(sm_OculusAdapter < sm_aAdapters.GetCount(), "Oculus Adapter %d outside of available Adapters %d", sm_OculusAdapter, sm_aAdapters.GetCount()); 
	if (sm_OculusAdapter >= 0)
		return sm_aAdapters[sm_OculusAdapter]; 

	return NULL;
}

} // namespace rage

#endif // __WIN32PC
