// 
// physics/debugphysics.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_DEBUGPHYSICS_H 
#define PHYSICS_DEBUGPHYSICS_H 

#if __DEV && (! __PS3) && !__RESOURCECOMPILER && !__TOOL 
// Use the first definition if you want optimizations off in debug files.
//#define PHYSICS_OPTIMIZATIONS __pragma(optimize("", off))
#define PHYSICS_OPTIMIZATIONS
#else
#define PHYSICS_OPTIMIZATIONS
#endif


#endif // PHYSICS_DEBUGPHYSICS_H 
