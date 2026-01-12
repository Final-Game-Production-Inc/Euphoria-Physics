//
// viewfragnm\viewfragnm.cpp
//
// Copyright (C) Rockstar Games.  All Rights Reserved.
//
// PURPOSE: Viewer for the fragment/NaturalMotion integration

#include "nmviewer.h"

#include "system/main.h"

// main application
int Main()
{
	ragesamples::NMViewer nmViewer;
	nmViewer.Init();

	nmViewer.UpdateLoop();

	nmViewer.Shutdown();
		
	return 0;
}





