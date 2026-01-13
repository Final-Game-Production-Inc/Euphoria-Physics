// 
// system/spuintrinsics.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SPUINTRINSICS_H 
#define SYSTEM_SPUINTRINSICS_H 

#if __SPU

// TODO: This file is stupid, remove it.
// A0-AF are the bytes of the first parameter to si_shufb, numbered from left
// to right in big-endian notation (also C array order).  Likewise B0-BF are
// the bytes in the second parameter to si_shufb.  The third parameter to
// si_shufb contains one of these constants below in each byte slot.
enum {
	SHUF_A0, SHUF_A1, SHUF_A2, SHUF_A3,
	SHUF_A4, SHUF_A5, SHUF_A6, SHUF_A7,
	SHUF_A8, SHUF_A9, SHUF_AA, SHUF_AB,
	SHUF_AC, SHUF_AD, SHUF_AE, SHUF_AF,

	SHUF_B0, SHUF_B1, SHUF_B2, SHUF_B3,
	SHUF_B4, SHUF_B5, SHUF_B6, SHUF_B7,
	SHUF_B8, SHUF_B9, SHUF_BA, SHUF_BB,
	SHUF_BC, SHUF_BD, SHUF_BE, SHUF_BF,

	SHUF_00 = 0x80,
	SHUF_FF = 0xC0,
	SHUF_80 = 0xE0
};

#include <spu_internals.h>
#define MAKE_SHUFFLE(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p) (qword){SHUF_##a,SHUF_##b,SHUF_##c,SHUF_##d,SHUF_##e,SHUF_##f,SHUF_##g,SHUF_##h,SHUF_##i,SHUF_##j,SHUF_##k,SHUF_##l,SHUF_##m,SHUF_##n,SHUF_##o,SHUF_##p}
#define MAKE_QWORD_s8(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p) (qword)(vector signed char){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}
#define MAKE_QWORD_u8(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p) (qword)(vector unsigned char){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}
#define MAKE_QWORD_s16(a,b,c,d, e,f,g,h) (qword)(vector signed short){a,b,c,d,e,f,g,h}
#define MAKE_QWORD_u16(a,b,c,d, e,f,g,h) (qword)(vector unsigned short){a,b,c,d,e,f,g,h}
#define MAKE_QWORD_s32(a,b,c,d) (qword)(vector signed int){a,b,c,d}
#define MAKE_QWORD_u32(a,b,c,d) (qword)(vector unsigned int){a,b,c,d}
#endif	// !__SPU

#endif // SYSTEM_SPUINTRINSICS_H 
