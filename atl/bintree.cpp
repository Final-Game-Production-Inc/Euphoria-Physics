//
// atl/bintree.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "bintree.h"

bool rage::atOrderLT (const char * x, const char * y)
{
	while (*x && *x == *y)
		++x,++y;
	return *x < *y;
}
