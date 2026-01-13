// 
// system/obfuscatedtypes.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "obfuscatedtypes.h"

#include "memory.h"

void FastObfuscatedPointerDataGet(void *dst, void *src, size_t sz)
{
	memcpy(dst, src, sz);
}
void FastObfuscatedPointerDataSet(void *dst, void *src, size_t sz)
{
	memcpy(dst, src, sz);
}