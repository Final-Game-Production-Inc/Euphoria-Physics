//
// phcore/conversion.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

/*
 *	File: conversion.h
 *	Synopsis: Units conversion constants
 */

#ifndef PHCORE_CONVERSION_H
#define PHCORE_CONVERSION_H

namespace rage {

// Angle
#define	PH_DEG2RAD(x)	(1.74532925199e-2f*(x))
#define	PH_RAD2DEG(x)	(57.2957795131f*(x))
#define	PH_MIN2RAD(x)	(2.90888208666e-4*(x))
#define	PH_RAD2MIN(x)	(3437.74677078f*(x))
#define	PH_SEC2RAD(x)	(4.84813681110e-6f*(x))
#define	PH_RAD2SEC(x)	(2.06264806247e5f*(x))
#define	PH_REV2RAD(x)	(6.28318530718f*(x))
#define	PH_RAD2REV(x)	(0.159154943092f*(x))

// Length
#define	PH_IN2M(x)	(2.54e-2f*(x))
#define	PH_M2IN(x)	(39.37f*(x))
#define	PH_FT2M(x)	(0.3048f*(x))
#define	PH_M2FT(x)	(3.281f*(x))
#define	PH_YD2M(x)	(0.9144f*(x))
#define	PH_M2YD(x)	(1.0936f*(x))
#define	PH_MI2M(x)	(1609*(x))
#define	PH_M2MI(x)	(6.214e-4f*(x))
#define PH_MILE2KM(x)	(1.609344f*(x))
#define PH_KM2MILE(x)	(0.6213711922f*(x))
#define PH_INCH2CM(x)	(2.54f*(x))
#define PH_CM2INCH(x)	(0.3937007874f*(x))

// Area
#define PH_SQFT2SQM(x)	(9.29e-2f*(x))
#define PH_SQM2SQFT(x)	(10.76f*(x))
#define	PH_SQIN2SQM(x)	(6.452e-4f*(x))
#define	PH_SQM2SQIN(x)	(1550.f*(x))

// Mass
#define	PH_LB2KG(x)	(0.4536f*(x))
#define	PH_KG2LB(x)	(2.205f*(x))
#define PH_TON2KG(x)	(907.2f*(x))
#define PH_KG2TON(x)	(1.102e-3f*(x))

// Speed
#define	PH_FPS2MPS(x)	(0.3048f*(x))
#define	PH_MPS2FPS(x)	(3.281f*(x))
#define PH_MPH2MPS(x)	(0.447f*(x))
#define PH_MPS2MPH(x)	(2.237f*(x))
#define	PH_KPH2MPS(x)	(0.2778f*(x))
#define	PH_MPS2KPH(x)	(3.6f*(x))

// Power
#define PH_HP2WATTS(x)	(746.0f*(x))

} // namespace rage

#endif	/* !PHCORE_CONVERSION_H } */
