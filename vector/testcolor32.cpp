//
// vector/testcolor32.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// validate algorithm used by PS2 Color32 class
// to make sure it's reversible

#include <stdio.h>

void main() {
	for (int i=0; i<256; i++) {
		int j = (i + ((i&128)>>6)) >> 1;
		int k = ((j)<<1)-(j>>6);
		printf("%3d %3d %3d\n",i,j,k);
	}
}
