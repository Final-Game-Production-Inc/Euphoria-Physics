//
// grcore/fastquad_switch.h
//
// Copyright (C) 2012 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_FASTQUAD_SWITCH_H
#define GRCORE_FASTQUAD_SWITCH_H

//This in included from both C and shader code so make sure the defines are correct.
#define	FAST_QUAD_SUPPORT	((__WIN32PC || __D3D11) && !__RESOURCECOMPILER)

#define SSAO_UNIT_QUAD		(FAST_QUAD_SUPPORT)
#define POSTFX_UNIT_QUAD	(FAST_QUAD_SUPPORT)

#endif	//GRCORE_FASTQUAD_SWITCH_H
