//
// grcore/AA_shared.h
//
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_AA_SHARED_H
#define GRCORE_AA_SHARED_H

// MSAA stage lasts up until PS_Composite pass new
#define AA_BACK_BUFFER				(0 && (__D3D11 || RSG_ORBIS))

// Durango does that automatically if we pretend there is no fragmant compression
#define EQAA_DECODE_GBUFFERS		(RSG_ORBIS || (0 && RSG_DURANGO))

// Mode for debugging sample locations
#define AA_SAMPLE_DEBUG				(0)

#endif	//GRCORE_AA_SHARED_H
