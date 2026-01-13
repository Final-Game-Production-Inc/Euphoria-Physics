//
// ragecore/gprallocation.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
//
// Note: XBOX 360 global shader register allocation
//

#ifndef __GPRALLOCATION_H
#define __GPRALLOCATION_H

#if __XENON

#include "data/base.h"
#include "grcore/device.h"

namespace rage
{

class grcGPRAllocation : public datBase
{
public:
	grcGPRAllocation();
	~grcGPRAllocation();

	// sets the number of pixel threads (0= system default, -1 = grcGPRAllocation default)
	void SetGPRAllocation(int pixelThreads);

	// returns the number of pixel threads currently used
	int GetGPRAllocation() const { return m_CurrentPixelThreads; }

	// set the default GRP pixel thread allocation, which is used when -1 is passed to SetGPRAllocation()
	void SetGPRAllocationDefault(int pixelThreads) {m_DefaultGPRAllocation = pixelThreads;}
	
	// get the default GRP pixel thread allocation, which is used when -1 is passed to SetGPRAllocation()
	int SetGPRAllocationDefault() {return m_DefaultGPRAllocation;}

	// static functions
	// keep track of the instance
	static void Init();
	static void Terminate();
	static grcGPRAllocation *GetInstance() {return sm_instance;}

private:

	// by default we want to use 96 pixel shader threads
	int m_DefaultGPRAllocation;

	// just a default value
	int m_CurrentPixelThreads;

	static grcGPRAllocation *sm_instance;			// instance


};

#define GRCGPRALLOCATION (grcGPRAllocation::GetInstance())


}	// name space rage
#endif


#endif  // GPRALLOCATION_H
