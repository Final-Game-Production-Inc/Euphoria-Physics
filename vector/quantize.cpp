//
// vector/quantize.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "quantize.h"

namespace rage {

////////////////////////////////////////////////////////////////
// quantizing one float (packing)

u16 Pack16_0to1 (float f)
{
	int pack = (int) (0.5f + f * 65536.0f);
	if (pack < 0) return 0;
	else if (pack > 65535) return 65535;
	else return (u16)pack;
}


u16 Pack16_n1to1 (float f)
{
	int pack = (int) (32768.5f + f * 32768.0f);
	if (pack < 0) return 0;
	else if (pack > 65535) return 65535;
	else return (u16)pack;
}


u16 Pack16_n2to2 (float f)
{
	int pack = (int) (32768.5f + f * 16384.0f);
	if (pack < 0) return 0;
	else if (pack > 65535) return 65535;
	else return (u16)pack;
}


u8 Pack8_0to1 (float f)
{
	int pack = (int) (0.5f + f * 256.0f);
	if (pack < 0) return 0;
	else if (pack > 255) return 255;
	else return (u8)pack;
}


u8 Pack8_n1to1 (float f)
{
	int pack = (int) (128.5f + f * 128.0f);
	if (pack < 0) return 0;
	else if (pack > 255) return 255;
	else return (u8)pack;
}


u8 Pack8_n2to2 (float f)
{
	int pack = (int) (128.5f + f * 64.0f);
	if (pack < 0) return 0;
	else if (pack > 255) return 255;
	else return (u8)pack;
}


u32 Pack11_nPItoPI (float f)
{
	int pack = (int) (1024.0f - (f/PI) * 1024.0f);
	if (pack < 0) return 0;
	else if (pack > 2047) return 2047;
	else return (u32)pack;
}


u32 Pack10_nPItoPI (float f)
{
	int pack = (int) (512.0f - (f/PI) * 512.0f);
	if (pack < 0) return 0;
	else if (pack > 1023) return 1023;
	else return (u32)pack;
}

u64 Pack21_nPItoPI (float f)
{
	int pack = (int) (1048576.0f - (f/PI) * 1048576.0f);
	if (pack < 0) return 0;
	else if (pack > 2097151) return 2097151;
	else return (u32)pack;
}

u64 Pack22_nPItoPI (float f)
{
	int pack = (int) (2097152.0f - (f/PI) * 2097152.0f);
	if (pack < 0) return 0;
	else if (pack > 4194303) return 4194303;
	else return (u32)pack;
}


////////////////////////////////////////////////////////////////
// quantizing vectors (packing)

void PackNormalTo8s (u32 & dest, const Vector4 & src)
{
	dest = Pack8_n1to1(src.x) | (Pack8_n1to1(src.y) << 8) | (Pack8_n1to1(src.z) << 16) | (Pack8_n1to1(src.w) << 24);
}


void PackNormalTo16s (u64 & dest, const Vector4 & src)
{
	dest = (u64)Pack16_n1to1(src.x) | ((u64)Pack16_n1to1(src.y) << 16) | ((u64)Pack16_n1to1(src.z) << 32) | ((u64)Pack16_n1to1(src.w) << 48);
}


u32 PackEulersTo32 (const Vector3 & src)
{
	u32 dest = Pack11_nPItoPI(src.x);
	dest |= (Pack11_nPItoPI(src.y) << 11);
	dest |= (Pack10_nPItoPI(src.z) << 22);
	return dest;
}

u64 PackEulersTo64 (const Vector3 & src)
{
	u64 dest = Pack21_nPItoPI(src.x);
	dest |= (Pack22_nPItoPI(src.y) << 21);
	dest |= (Pack21_nPItoPI(src.z) << 43);
	return dest;
}

}	// namespace rage
