//
// vector/quantize.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_QUANTIZE_H
#define VECTOR_QUANTIZE_H

#include "vector/vector4.h"


namespace rage {


////////////////////////////////////////////////////////////////
// packing functions

// PURPOSE: Pack a float in [0,1] to 8 bits.
// PARAMS:
//	f -	a float between 0 and 1
// RETURN: An 8-bit quantization of the given floating point value.
u8 Pack8_0to1 (float f);

// PURPOSE: Pack a float in [-1,1] to 8 bits.
// PARAMS:
//	f -	a float between -1 and 1
// RETURN: An 8-bit quantization of the given floating point value.
u8 Pack8_n1to1 (float f);

// PURPOSE: Pack a float in [-2,2] to 8 bits.
// PARAMS:
//	f -	a float between -2 and 2
// RETURN: An 8-bit quantization of the given floating point value.
u8 Pack8_n2to2 (float f);

// PURPOSE: Pack a float in [0,1] to 16 bits.
// PARAMS:
//	f -	a float between 0 and 1
// RETURN: A 16-bit quantization of the given floating point value.
u16 Pack16_0to1 (float f);

// PURPOSE: Pack a float in [-1,1] to 16 bits.
// PARAMS:
//	f -	a float between -1 and 1
// RETURN: A 16-bit quantization of the given floating point value.
u16 Pack16_n1to1 (float f);

// PURPOSE: Pack a float in [-2,2] to 16 bits.
// PARAMS:
//	f -	a float between -2 and 2
// RETURN: A 16-bit quantization of the given floating point value.
u16 Pack16_n2to2 (float f);

// PURPOSE: Pack a float in [-PI,PI] to low 10 bits.
// PARAMS:
//	f -	a float between -PI and PI
// RETURN: A 10-bit quantization of the given floating point value.
u32 Pack10_nPItoPI (float f);

// PURPOSE: Pack a float in [-PI,PI] to low 11 bits.
// PARAMS:
//	f -	a float between -PI and PI
// RETURN: An 11-bit quantization of the given floating point value.
u32 Pack11_nPItoPI (float f);

// PURPOSE: Pack a float in [-PI,PI] to low 21 bits.
// PARAMS:
//	f -	a float between -PI and PI
// RETURN: An 21-bit quantization of the given floating point value.
u64 Pack21_nPItoPI (float f);

// PURPOSE: Pack a float in [-PI,PI] to low 22 bits.
// PARAMS:
//	f -	a float between -PI and PI
// RETURN: An 22-bit quantization of the given floating point value.
u64 Pack22_nPItoPI (float f);

// PURPOSE: Pack a Vector4 with x,y,z and w in [-1,1] to 32 bits.
// PARAMS:
//	dest -	a reference to a u32 in which to put the quantized Vector4
//	src -	a Vector4 with all elements between -1 and 1
void PackNormalTo8s (u32 & dest, const Vector4 & src);

// PURPOSE: Pack a Vector4 with x,y,z and w in [-1,1] to 64 bits.
// PARAMS:
//	dest -	a reference to a u64 in which to put the quantized Vector4
//	src -	a Vector4 with all elements between -1 and 1
void PackNormalTo16s (u64 & dest, const Vector4 & src);	

// PURPOSE: 10-bit mask
const u32 MASK10bits = 0x000003FF;

// PURPOSE: 11-bit mask
const u32 MASK11bits = 0x000007FF;

// PURPOSE: 21-bit mask
const u64 MASK21bits = 0x001FFFFF;

// PURPOSE: 22-bit mask
const u64 MASK22bits = 0x003FFFFF;

// PURPOSE: Pack a Vector3 with x,y and z in [-PI,PI] to 32 bits.
// PARAMS:
//	src -	a Vector3 with all elements between -PI and PI
// RETURN: A 32-bit quantization of the given Vector3.
u32 PackEulersTo32 (const Vector3 & src);

u64 PackEulersTo64 (const Vector3 & src);


////////////////////////////////////////////////////////////////
// unpacking floats

// PURPOSE: Unpack 8 bits to a float in [0,1].
// PARAMS:
//	b -	an 8-bit packed floating point value
// RETURN: A floating point value from the 8-bit quantization.
inline float Unpack8_0to1 (u8 b)
{
	return ((float) b) * (1.0f / 256.0f);
}

// PURPOSE: Unpack 8 bits to a float in [-1,1].
// PARAMS:
//	b -	an 8-bit packed floating point value
// RETURN: A floating point value from the 8-bit quantization.
inline float Unpack8_n1to1 (u8 b)
{
	return ((float) b - 128.0f) * (1.0f / 128.0f);
}

// PURPOSE: Unpack 8 bits to a float in [-2,2].
// PARAMS:
//	b -	an 8-bit packed floating point value
// RETURN: A floating point value from the 8-bit quantization.
inline float Unpack8_n2to2 (u8 b)
{
	return ((float) b - 128.0f) * (1.0f / 64.0f);
}

// PURPOSE: Unpack 16 bits to a float in [0,1].
// PARAMS:
//	c -	a 16-bit packed floating point value
// RETURN: A floating point value from the 16-bit quantization.
inline float Unpack16_0to1 (u16 c)
{
	return ((float) c) * (1.0f / 65536.0f);
}

// PURPOSE: Unpack 16 bits to a float in [-1,1].
// PARAMS:
//	c -	a 16-bit packed floating point value
// RETURN: A floating point value from the 16-bit quantization.
inline float Unpack16_n1to1 (u16 c)
{
	return ((float) c - 32768.0f) * (1.0f / 32768.0f);
}

// PURPOSE: Unpack 16 bits to a float in [-2,2].
// PARAMS:
//	c -	a 16-bit packed floating point value
// RETURN: A floating point value from the 16-bit quantization.
inline float Unpack16_n2to2 (u16 c)
{
	return ((float) c - 32768.0f) * (1.0f / 16384.0f);
}

// PURPOSE: Unpack low-10 bits to a float in [-PI,PI].
// PARAMS:
//	src -	a low-10-bit packed floating point value
// RETURN: A floating point value from the low-10-bit quantization.
inline float Unpack10_nPItoPI (u32 src)
{
	return -(((float) src / 1024.0f) * (2.0f * PI) - PI);
}

// PURPOSE: Unpack low-11 bits to a float in [-PI,PI].
// PARAMS:
//	src -	a low-11-bit packed floating point value
// RETURN: A floating point value from the low-11-bit quantization.
inline float Unpack11_nPItoPI (u32 src)
{
	return -(((float) src / 2048.0f) * (2.0f * PI) - PI);
}

// PURPOSE: Unpack low-21 bits to a float in [-PI,PI].
// PARAMS:
//	src -	a low-21-bit packed floating point value
// RETURN: A floating point value from the low-21-bit quantization.
inline float Unpack21_nPItoPI (u64 src)
{
	return -(((float) src / 2097152.0f) * (2.0f * PI) - PI);
}

// PURPOSE: Unpack low-22 bits to a float in [-PI,PI].
// PARAMS:
//	src -	a low-22-bit packed floating point value
// RETURN: A floating point value from the low-22-bit quantization.
inline float Unpack22_nPItoPI (u64 src)
{
	return -(((float) src / 4194304.0f) * (2.0f * PI) - PI);
}


////////////////////////////////////////////////////////////////
// unpacking vectors

// PURPOSE: Unpack 32 bits to a Vector3 with x,y and z in [-PI,PI].
// PARAMS:
//	dest -	a reference to a Vector3 in which to put the unpacked u32
//	src -	a u32-packed Vector3
inline void UnpackEulersFrom32 (Vector3 & dest, u32 src)
{
	dest.x = Unpack11_nPItoPI(src & MASK11bits);
	dest.y = Unpack11_nPItoPI((src >> 11) & MASK11bits);
	dest.z = Unpack10_nPItoPI((src >> 22) & MASK10bits);
}

inline void UnpackEulersFrom64 (Vector3 & dest, u64 src)
{
	dest.x = Unpack21_nPItoPI(src & MASK21bits);
	dest.y = Unpack22_nPItoPI((src >> 21) & MASK22bits);
	dest.z = Unpack21_nPItoPI((src >> 43) & MASK21bits);
}

// PURPOSE: Unpack 32 bits to a Vector4 with x,y,z and w in [-1,1].
// PARAMS:
//	dest -	a reference to a Vector4 in which to put the unpacked u32
//	src -	a u32-packed Vector4
inline void UnpackNormalFrom8s (Vector4 & dest, u32 src)
{
	dest.x = Unpack8_n1to1((u8)(src));
	dest.y = Unpack8_n1to1((u8)(src >> 8));
	dest.z = Unpack8_n1to1((u8)(src >> 16));
	dest.w = Unpack8_n1to1((u8)(src >> 24));
}

// PURPOSE: Unpack 64 bits to a Vector4 with x,y,z and w in [-1,1].
// PARAMS:
//	dest -	a reference to a Vector4 in which to put the unpacked u64
//	src -	a u64-packed Vector4
inline void UnpackNormalFrom16s (Vector4 & dest, u64 src)
{
	dest.x = Unpack16_n1to1((u16)(src));
	dest.y = Unpack16_n1to1((u16)(src >> 16));
	dest.z = Unpack16_n1to1((u16)(src >> 32));
	dest.w = Unpack16_n1to1((u16)(src >> 48));
}

}	// namespace rage

#endif // ndef VECTOR_QUANTIZE_H
