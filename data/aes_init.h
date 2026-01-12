// 
// data/aes_init.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

namespace rage {

// These are GTA5's keys.  Other projects will need to branch this file.
#if __XENON
extern u8 unique_key_gta5_360[32];
#define AES_INIT(offset,v1,v2)  struct aes_init_##offset { aes_init_##offset() { (*(u16*)(unique_key_gta5_360+offset) = (v2|(v1<<8))); } } aes_init_obj_##offset
#define AES_INIT_0	AES_INIT( 0, 0xa1, 0xe7)
#define AES_INIT_1	AES_INIT( 2, 0x29, 0x39)
#define AES_INIT_2	AES_INIT( 4, 0x5d, 0x8a)
#define AES_INIT_3	AES_INIT( 6, 0xd1, 0x0b)
#define AES_INIT_4	AES_INIT( 8, 0x9b, 0x7b)
#define AES_INIT_5	AES_INIT(10, 0xd0, 0x11)
#define AES_INIT_6	AES_INIT(12, 0xd5, 0x28)
#define AES_INIT_7	AES_INIT(14, 0x69, 0x3d)
#define AES_INIT_8	AES_INIT(16, 0x96, 0xe2)
#define AES_INIT_9	AES_INIT(18, 0x2b, 0xd6)
#define AES_INIT_A	AES_INIT(20, 0xa2, 0x8a)
#define AES_INIT_B	AES_INIT(22, 0xab, 0xae)
#define AES_INIT_C	AES_INIT(24, 0xb4, 0xa6)
#define AES_INIT_D	AES_INIT(26, 0x9a, 0xc6)
#define AES_INIT_E	AES_INIT(28, 0xf9, 0x73)
#define AES_INIT_F	AES_INIT(30, 0x62, 0x7f)
#elif __PS3
extern u8 unique_key_gta5_ps3[32];
#define AES_INIT(offset,v1,v2)  struct aes_init_##offset { aes_init_##offset() { *(u16*)(unique_key_gta5_ps3+offset) = (v2|(v1<<8)); } } aes_init_obj_##offset
#define AES_INIT_0	AES_INIT( 0, 0x85, 0x13)
#define AES_INIT_1	AES_INIT( 2, 0x6e, 0x1e)
#define AES_INIT_2	AES_INIT( 4, 0x37, 0xfc)
#define AES_INIT_3	AES_INIT( 6, 0xbc, 0x45)
#define AES_INIT_4	AES_INIT( 8, 0x94, 0xe7)
#define AES_INIT_5	AES_INIT(10, 0xf7, 0xbc)
#define AES_INIT_6	AES_INIT(12, 0x5f, 0x18)
#define AES_INIT_7	AES_INIT(14, 0x52, 0x00)
#define AES_INIT_8	AES_INIT(16, 0xb3, 0x2a)
#define AES_INIT_9	AES_INIT(18, 0x67, 0x30)
#define AES_INIT_A	AES_INIT(20, 0x8c, 0xc1)
#define AES_INIT_B	AES_INIT(22, 0xb8, 0x33)
#define AES_INIT_C	AES_INIT(24, 0xb3, 0x2a)
#define AES_INIT_D	AES_INIT(26, 0x67, 0x30)
#define AES_INIT_E	AES_INIT(28, 0x8c, 0xc1)
#define AES_INIT_F	AES_INIT(30, 0xb8, 0x33)
#elif 0
extern u8 unique_key_gta5_avx[32];
#define AES_INIT(offset,v1,v2)  struct aes_init_##offset { aes_init_##offset() { *(u16*)(unique_key_gta5_avx+offset) = (v1|(v2<<8)); } } aes_init_obj_##offset
#define AES_INIT_0	AES_INIT( 0, 0xb1, 0x69)
#define AES_INIT_1	AES_INIT( 2, 0x8e, 0x14)
#define AES_INIT_2	AES_INIT( 4, 0x62, 0xa0)
#define AES_INIT_3	AES_INIT( 6, 0x33, 0xbb)
#define AES_INIT_4	AES_INIT( 8, 0x8e, 0xfa)
#define AES_INIT_5	AES_INIT(10, 0xb0, 0x64)
#define AES_INIT_6	AES_INIT(12, 0xb2, 0x52)
#define AES_INIT_7	AES_INIT(14, 0x2c, 0x30)
#define AES_INIT_8	AES_INIT(16, 0xa8, 0x66)
#define AES_INIT_9	AES_INIT(18, 0xce, 0x68)
#define AES_INIT_A	AES_INIT(20, 0x02, 0xc0)
#define AES_INIT_B	AES_INIT(22, 0x4c, 0x8e)
#define AES_INIT_C	AES_INIT(24, 0xac, 0xde)
#define AES_INIT_D	AES_INIT(26, 0x21, 0xbc)
#define AES_INIT_E	AES_INIT(28, 0xe8, 0xe7)
#define AES_INIT_F	AES_INIT(30, 0xb7, 0x0e)
#else
#define AES_INIT_0
#define AES_INIT_1
#define AES_INIT_2
#define AES_INIT_3
#define AES_INIT_4
#define AES_INIT_5
#define AES_INIT_6
#define AES_INIT_7
#define AES_INIT_8
#define AES_INIT_9
#define AES_INIT_A
#define AES_INIT_B
#define AES_INIT_C
#define AES_INIT_D
#define AES_INIT_E
#define AES_INIT_F
#endif

// Do not use this in the final game; instead, scatter AES_INIT_0-F calls throughout startup code
#define TESTER_INIT_KEYS	\
	AES_INIT_0; AES_INIT_1; AES_INIT_2; AES_INIT_3; \
	AES_INIT_4; AES_INIT_5; AES_INIT_6; AES_INIT_7; \
	AES_INIT_8; AES_INIT_9; AES_INIT_A; AES_INIT_B; \
	AES_INIT_C; AES_INIT_D; AES_INIT_E; AES_INIT_F;

}	// namespace rage
