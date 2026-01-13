//
// ragecore/gprallocation.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
//
// Note: XBOX 360 global shader register allocation
//


#ifndef GPRALLOCATION_CPP
#define GPRALLOCATION_CPP

#if __XENON

#include "gprallocation.h"

using namespace rage;


grcGPRAllocation *grcGPRAllocation::sm_instance = NULL;

grcGPRAllocation::grcGPRAllocation()
{
	// by default we want to use 96 pixel shader threads
	m_DefaultGPRAllocation = 96;

	// just a default value
	m_CurrentPixelThreads = -1;
}

grcGPRAllocation::~grcGPRAllocation()
{
}

void grcGPRAllocation::SetGPRAllocation(int pixelThreads)
{
	if (pixelThreads < 0) // -1 means use defaults
		pixelThreads = m_DefaultGPRAllocation;

	// check if it is not already set
	if (pixelThreads != m_CurrentPixelThreads)
	{
		m_CurrentPixelThreads = pixelThreads;

		if (m_CurrentPixelThreads == 0) // 0 means system defaults...
			GRCDEVICE.SetShaderGPRAllocation(0,0);
		else
			GRCDEVICE.SetShaderGPRAllocation(128 - m_CurrentPixelThreads, m_CurrentPixelThreads);
	}
}

//static functions
void grcGPRAllocation::Init()
{
	Assert(sm_instance == NULL);
	sm_instance = rage_new grcGPRAllocation;
}

void grcGPRAllocation::Terminate()
{
	Assert(sm_instance);
	delete sm_instance;
	sm_instance = NULL;
}

#endif

#endif  // GPRALLOCATION_CPP
