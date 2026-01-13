//
// grprofile/drawcore.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_DRAWCORE_H
#define GRPROFILE_DRAWCORE_H

//==============================================================
//
// The classes in these files form a system for queuing graphical
// annotations at any point in a game loop for later drawing.
// 

#include "diag/stats.h"
#include "grcore/im.h"

namespace rage {

#ifndef __PFDRAW
#define __PFDRAW ((/* PREREQUISITES: */ __IM_BATCHER && !__SPU && __BANK) && (/* WHICH BUILDS: */ __STATS || __BANK))
#endif // __PFDRAW

#ifndef __NMDRAW
#define __NMDRAW 0
#endif // __NMDRAW

#if __PFDRAW

#define PF_DRAW_ONLY(x)								x


//=============================================================================
// Macro interface for drawing primitives.

#define PF_DRAW_LINE(itemName,start,end)				PFD_##itemName.DrawLine(start,end)
#define PF_DRAW_LINE_COLOR(itemName,start,end,color)	PFD_##itemName.DrawLine(start,end,color)
#define PF_DRAW_ARROW(itemName,start,end)				PFD_##itemName.DrawArrow(start,end)
#define PF_DRAW_ARROW_COLOR(itemName,start,end,color)	PFD_##itemName.DrawArrow(start,end,color)
#define PF_DRAW_TICK(itemName,v0,size)					PFD_##itemName.DrawTick((v0), (size))
#define PF_DRAW_BOX(itemName,matrix,halfSize)			PFD_##itemName.DrawBox((matrix), (halfSize))
#define PF_DRAW_BOX_COLOR(itemName,matrix,halfSize,color)	PFD_##itemName.DrawBox((matrix), (halfSize), (color))
#define PF_DRAW_SOLID_BOX(itemName,matrix,halfSize)		PFD_##itemName.DrawBox((matrix), (halfSize), true)
#define PF_DRAW_SPHERE(itemName,radius,center)			PFD_##itemName.DrawSphere((radius), (center))
#define PF_DRAW_SPHERE_COLOR(itemName,radius,center, color)	PFD_##itemName.DrawSphere((radius), (center), (color))
#define PF_DRAW_CAMERA(itemName,matrix,fov)				PFD_##itemName.DrawCamera((matrix), (fov))
#define PF_DRAW_SOLID_SPHERE(itemName,radius,center)	PFD_##itemName.DrawSolidSphere((radius), (center), PFD_##itemName.GetBaseColor(), PFD_##itemName.GetBaseColor() * Color32(0.5f, 0.5f, 0.5f, 1.0f))
#define PF_DRAW_SCREEN_TEXT(itemName, x, y, text)		PFD_##itemName.Draw2dText((x), (y), (text))
#define PF_DRAW_WORLD_TEXT(itemName, pos, text)			PFD_##itemName.Draw2dText((pos), (text))
#define PF_DRAW_WORLD_TEXT_COLOR(itemName, pos, text, color)	PFD_##itemName.Draw2dText((pos), (text), (color))
#define PF_DRAW_CAPSULE(itemName, matrix, len, radius)	PFD_##itemName.DrawCapsule((matrix),(len),(radius))


//=============================================================================
// Drawing primitives.
// NOTES: These functions are useful for drawing IM objects, but not general
// enough to go into grcDrawX (yet at least).

void pfDrawArrow (const Vector3& start, const Vector3& end);


#else // !__PFDRAW

//=============================================================================
// Empty macro implementations if !__PFDRAW

#define PF_DRAW_ONLY(x)

#define PF_DRAW_LINE(itemName,v0,v1)
#define PF_DRAW_LINE_COLOR(itemName,v0,v1,color)
#define PF_DRAW_TICK(itemName,v0,size)
#define PF_DRAW_BOX(itemName,matrix,halfSize)	
#define PF_DRAW_BOX_COLOR(itemName,matrix,halfSize,color)	
#define PF_DRAW_SOLID_BOX(itemName,matrix,halfSize)	
#define PF_DRAW_SPHERE(itemName,radius,center)	
#define PF_DRAW_SPHERE_COLOR(itemName,radius,center,color)	
#define PF_DRAW_CAMERA(itemName,matrix,fov)		
#define PF_DRAW_SOLID_SPHERE(itemName,radius,center)
#define PF_DRAW_SCREEN_TEXT(itemName, x, y, text)	
#define PF_DRAW_WORLD_TEXT(itemName, pos, text)		
#define PF_DRAW_WORLD_TEXT_COLOR(itemName, pos, text, color)		
#define PF_DRAW_CAPSULE(itemName, matrix, len, radius)

#endif // __PFDRAW

}	// namespace rage

#endif // GRPROFILE_DRAWCORE_H

// <eof> grprofile/drawcore.h
