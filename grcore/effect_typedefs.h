// 
// grcore/effect_typedefs.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef GRCORE_EFFECT_TYPEDEFS_H
#define GRCORE_EFFECT_TYPEDEFS_H

namespace rage {

typedef enum grcEffectGlobalVar__ { grcegvNONE } grcEffectGlobalVar;
typedef enum grcEffectVar__ { grcevNONE } grcEffectVar;
typedef enum grcEffectTechnique__ { grcetNONE } grcEffectTechnique;
typedef enum grcEffectAnnotation__ { grceaNONE } grcEffectAnnotation;

enum grceRenderState {
	grcersZENABLE,					// 0
	grcersFILLMODE,					// 1
	grcersZWRITEENABLE,				// 2
	grcersALPHATESTENABLE,			// 3
	grcersSRCBLEND,					// 4
	grcersDESTBLEND,				// 5
	grcersCULLMODE,					// 6
	grcersZFUNC,					// 7
	grcersALPHAREF,					// 8
	grcersALPHAFUNC,				// 9
	grcersALPHABLENDENABLE,			// 10
	grcersSTENCILENABLE,			// 11
	grcersSTENCILFAIL,				// 12
	grcersSTENCILZFAIL,				// 13
	grcersSTENCILPASS,				// 14
	grcersSTENCILFUNC,				// 15
	grcersSTENCILREF,				// 16
	grcersSTENCILMASK,				// 17
	grcersSTENCILWRITEMASK,			// 18
	grcersCOLORWRITEENABLE,			// 19
	grcersCOLORWRITEENABLE1,		// 20
	grcersCOLORWRITEENABLE2,		// 21
	grcersCOLORWRITEENABLE3,		// 22
	grcersBLENDOP,					// 23
	grcersBLENDOPALPHA,				// 24
	grcersSEPARATEALPHABLENDENABLE,	// 25
	grcersSRCBLENDALPHA,			// 26
	grcersDESTBLENDALPHA,			// 27
	grcersHIGHPRECISIONBLENDENABLE,	// 28
	grcersSLOPESCALEDEPTHBIAS,	// 29 (float)
	grcersDEPTHBIAS,			// 30 (float)
	grcersBLENDFACTOR,			// 31
	grcersALPHATOMASK,			// 32
	grcersALPHATOMASKOFFSETS,	// 33
	grcersHALFPIXELOFFSET,		// 34
	grcersTWOSIDEDSTENCIL,		// 35
	grcersBACKSTENCILFAIL,		// 36
	grcersBACKSTENCILZFAIL,		// 37
	grcersBACKSTENCILPASS,		// 38
	grcersBACKSTENCILFUNC,		// 39
	grcersBACKSTENCILREF,		// 40
	grcersBACKSTENCILMASK,		// 41
	grcersBACKSTENCILWRITEMASK,	// 42
	grcersANTIALIASINGMASK,		// 43

	grcersRASTERIZERSTATEKEY,	// 44
	grcersDEPTHSTENCILSTATEKEY,	// 45
	grcersBLENDSTATEKEY,		// 46

#if __WIN32PC
	grcersMULTISAMPLEAA,		// 47
	// DX10 render states.
	grcersDx9COUNT,				// 48
	grcersALPHABLENDENABLE1 = grcersDx9COUNT,
	grcersALPHABLENDENABLE2,	// 49
	grcersALPHABLENDENABLE3,	// 50
	grcersALPHABLENDENABLE4,	// 51
	grcersALPHABLENDENABLE5,	// 52
	grcersALPHABLENDENABLE6,	// 53
	grcersALPHABLENDENABLE7,	// 54

	grcersCOLORWRITEENABLE4,	// 55
	grcersCOLORWRITEENABLE5,	// 56
	grcersCOLORWRITEENABLE6,	// 57
	grcersCOLORWRITEENABLE7,	// 58

	grcersFRONTCOUNTERCLOCKWISE,// 59
	grcersDEPTHCLIPENABLE,		// 60
	grcersDEPTHBIASCLAMP,		// 61
	grcersANTIALIASEDLINEENABLE,// 62
#endif // __WIN32PC

	grcersCOUNT
};

enum grceSamplerState {
	grcessADDRESSU,					// 0
	grcessADDRESSV,					// 1
	grcessADDRESSW,					// 2
	grcessBORDERCOLOR,				// 3
	grcessMAGFILTER,				// 4
	grcessMINFILTER,				// 5
	grcessMIPFILTER,				// 6
	grcessMIPMAPLODBIAS,			// 7
	grcessMAXMIPLEVEL,				// 8
	grcessMAXANISOTROPY,			// 9
	grcessTRILINEARTHRESHOLD,		// 10
	grcessMINMIPLEVEL,				// 11
	grcessTEXTUREZFUNC,				// 12

	// DX10 sampler states.
	grcessCOMPAREFUNC,				// 13

	grcessALPHAKILL,				// 14 (PS3-specific)

	grcessCOUNT
};

// Extend DX9 renderstates so DX10 can be setup using SetRenderState calls if desired.
typedef enum _RAGE_D3DRENDERSTATETYPE
{
	RAGERS_ALPHABLENDENABLE1		= 209+1,		// last valid renderstate + 1	D3DRS_BLENDOPALPHA == 209
	RAGERS_ALPHABLENDENABLE2,
	RAGERS_ALPHABLENDENABLE3,
	RAGERS_ALPHABLENDENABLE4,
	RAGERS_ALPHABLENDENABLE5,
	RAGERS_ALPHABLENDENABLE6,
	RAGERS_ALPHABLENDENABLE7,

	RAGERS_COLORWRITEENABLE4,
	RAGERS_COLORWRITEENABLE5,
	RAGERS_COLORWRITEENABLE6,
	RAGERS_COLORWRITEENABLE7,

	RAGERS_FRONTCOUNTERCLOCKWISE,
	RAGERS_DEPTHCLIPENABLE,
	RAGERS_DEPTHBIASCLAMP,
	RAGERS_ANTIALIASEDLINEENABLE,
	RAGERS_ALPHACOVERAGE,

	RAGERS_MAXRENDERSTATES,							// must be second last

	RAGERS_FORCE_DWORD				= 0x7fffffff
} RAGE_D3DRENDERSTATETYPE;

typedef enum _RAGE_D3DSAMPLERSTATETYPE
{
	RAGESAMP_MAXLOD					= 9,			// alias for D3DSAMP_MAXMIPLEVEL
	RAGESAMP_MINMIPLEVEL			= 13+1,			// last valid sampler state + 1		D3DSAMP_DMAPOFFSET == 13
	RAGESAMP_MINLOD					= RAGESAMP_MINMIPLEVEL,	// alias
	RAGESAMP_COMPAREFUNC,

	RAGESAMP_MAXSTATES,								// must be second last

	RAGESAMP_FORCE_DWORD			= 0x7fffffff
} RAGE_D3DSAMPLERSTATETYPE;

}	// namespace rage

#endif	// GRCORE_EFFECT_TYPEDEFS_H
