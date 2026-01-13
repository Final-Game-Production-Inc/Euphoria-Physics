//
// grcore/edge_vehicledamage.h
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_EDGE_VEHICLEDAMAGE_H
#define GRCORE_EDGE_VEHICLEDAMAGE_H

#if __SPU

#include <vec_types.h>

extern "C" void ApplyVehicleDamage(vec_float4 *__restrict__ pPos, vec_float4 *__restrict__ pNorm, vec_float4 *__restrict__ pDiffuse, rage::u32 numVertices,
								   const void *damageTex, float damageTexOffsetX, float damageTexOffsetY, float damageTexOffsetZ, float boundRadius);

#endif // __SPU

#endif // GRCORE_EDGE_VEHICLEDAMAGE_H
