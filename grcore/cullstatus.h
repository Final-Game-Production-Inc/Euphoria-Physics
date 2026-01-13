// 
// grcore/cullstatus.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_CULLSTATUS_H 
#define GRCORE_CULLSTATUS_H 

namespace rage {

/*
	Zero means object is completely off screen; otherwise it is at 
	least partially visible.
*/
enum grcCullStatus {
	cullOutside, 		// Completely off screen
	cullClipped, 		// Partially on screen
	cullInside 		// Completely on screen
};



} // namespace rage

#endif // GRCORE_CULLSTATUS_H 
