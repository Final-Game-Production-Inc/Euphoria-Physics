//
// pheffects/wind_optimisations.h
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_WIND_OPTIMISATIONS_H
#define PHEFFECTS_WIND_OPTIMISATIONS_H

#define WIND_OPTIMISATIONS_OFF		0

#if WIND_OPTIMISATIONS_OFF
#define WIND_OPTIMISATIONS()	OPTIMISATIONS_OFF()
#else
#define WIND_OPTIMISATIONS()
#endif	

#endif // PHEFFECTS_WIND_OPTIMISATIONS_H
