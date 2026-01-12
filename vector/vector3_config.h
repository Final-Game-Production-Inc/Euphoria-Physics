//
// vector/vector3_config.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_VECTOR3_CONFIG_H
#define VECTOR_VECTOR3_CONFIG_H

// PURPOSE: If VECTORIZED is non-zero, the vectorized implementations will be used if possible
#define VECTORIZED			(__XENON || __PS3)

// PURPOSE: If VECTORIZED_PADDING is non zero, classes with Vector3 members will be padded to a 16 byte alignment
#define VECTORIZED_PADDING	(1 || VECTORIZED)

#if VECTORIZED_PADDING
#define VECTOR_ALIGN	ALIGNAS(16)
#else
#define VECTOR_ALIGN
#endif
 
#endif // ndef VECTOR_VECTOR3_CONFIG_H
