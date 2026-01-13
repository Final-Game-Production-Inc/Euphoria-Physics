//
// grcore/adapter.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_ADAPTER_H
#define GRCORE_ADAPTER_H

#if RSG_PC

#include "atl/array.h"

#define MAX_ADAPTER_NAME_LEN 256

namespace rage {

class grcDisplayWindow;

class grcAdapter {

public:
	grcAdapter();
	virtual ~grcAdapter();

	virtual int  GetOutputCount() const = 0;
	virtual u32  GetModeCount(u32 uOutput = 0) const = 0;
	virtual bool GetMode(grcDisplayWindow* pDisplayModeOut, u32 uMode, u32 uOutput = 0) const = 0;
	virtual bool GetClosestMode(grcDisplayWindow* pDisplayModeInOut, u32 uOutput = 0) const = 0;
	virtual u32  GetLargestModeIndex(u32 uOutput = 0) const = 0;
	virtual s32  GetOculusOutputIndex() const { return sm_OculusMonitor; }; // Would be nice to standard in a device independent way the output devices.

	virtual bool MatchesWith(grcAdapter *) const {return true;}

protected:
	s32 sm_OculusMonitor;
};

class grcAdapterManager {

public:
	static void InitClass(u32 uFormat);
	static void InitClass(u32 uFormat, u32 uDXVersion);
	static void ShutdownClass();

	static grcAdapterManager* GetInstance()			{ return sm_pInstance; }

	static void	SetOculusMonitor(const char* pszOculusMonitor);

	int GetAdapterCount() const						{ return (int)sm_aAdapters.size(); }
	const grcAdapter* GetAdapter(u32 uAdapter) const;

	bool HasOculusDevice() const { return (sm_OculusAdapter >= 0) ? true : false; }
	const grcAdapter* GetOculusAdapter() const;
	static const char* GetOculusOutputMonitorName() { return sm_OculusOutputMonitorName; }

	virtual bool VerifyAdapters(u32 /*uFormat*/) {return true;}

protected:
	grcAdapterManager();
	virtual ~grcAdapterManager();

	virtual void Enumerate(u32 uFormat) = 0;

	static grcAdapterManager*		sm_pInstance;
	static atArray<grcAdapter*>		sm_aAdapters;

	static s32						sm_OculusAdapter;
	static char						sm_OculusOutputMonitorName[MAX_ADAPTER_NAME_LEN];
};

} // namespace rage

#endif // RSG_PC

#endif // GRCORE_ADAPTER_H
