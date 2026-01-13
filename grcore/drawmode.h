// 
// grcore/drawmode.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef GRCORE_DRAWMODE_H
#define GRCORE_DRAWMODE_H

namespace rage {

/*
	Enumerant used by grcBegin to determine the type of primitive to draw.
*/
enum grcDrawMode {
	drawPoints,			// Draw one or more single-pixel points
	drawLines,			// Draw one or more disconnected line segments
	drawLineStrip,		// Draw a single multivertex line strip
	drawTris,			// Draw one or more disconnected triangles
	drawTriStrip,		// Draw a single tristrip
	drawTriFan,			// Draw a single triangle fan (not supported on DX11
	drawQuads,			// Independent quads; ONLY SUPPORTED UNDER Xenon, PS3, and Orbis
	drawRects,			// Independent rectangles; ONLY SUPPORTED UNDER Xenon and Orbis.  Three vertices per rectangle: x1,y1,z,u1,v1 / x2,y1,z,u2,v1 / x1,y2,z,u1,v2
	drawTrisAdj,		// Triangles with adjacency information
	drawModesTotal,
};

}	// namespace rage

#endif
