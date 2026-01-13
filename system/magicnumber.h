// 
// system/magicnumber.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_MAGICNUMBER_H
#define SYSTEM_MAGICNUMBER_H

// Create a FOURCC code portably
#define MAKE_MAGIC_NUMBER(a,b,c,d)		((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

// Use something like: Displayf("Magic number is %c%c%c%c", EXPAND_MAGIC_NUMBER);
#define EXPAND_MAGIC_NUMBER(x)	((x) & 255), (((x) >> 8) & 255), (((x) >> 16) & 255), (((x) >> 24) & 255)
#define EXPAND_STATE_KEY(x)		(((x) >> 24) & 255), (((x) >> 16) & 255), (((x) >> 8) & 255), ((x) & 255)

// a 'printable' FOURCC is one where each character is in the range [0x20..0x7e]
#define IS_PRINTABLE_MAGIC_NUMBER(x) ( \
	((x>>(8*0)) & 255) >= 0x20 && ((x>>(8*0)) & 255) <= 0x7e && \
	((x>>(8*1)) & 255) >= 0x20 && ((x>>(8*1)) & 255) <= 0x7e && \
	((x>>(8*2)) & 255) >= 0x20 && ((x>>(8*2)) & 255) <= 0x7e && \
	((x>>(8*3)) & 255) >= 0x20 && ((x>>(8*3)) & 255) <= 0x7e \
	)

#endif	// SYSTEM_MAGICNUMBER_H
