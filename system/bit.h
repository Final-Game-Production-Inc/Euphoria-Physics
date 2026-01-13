//
// system/bit.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_BIT_H
#define SYSTEM_BIT_H

#ifndef BIT_64
// PURPOSE:	Given a bit number, return a 64-bit bitmask
#define BIT_64(n)	( (u64)1 << (n) )
#endif

#ifndef BIT
// PURPOSE:	Given a bit number, return a bitmask
#define BIT(n)	(1<<(n))
#endif

// Returns the numbers of bytes needed to store numBits amount of bits.
#define NUM_BITS(numBytes) (numBytes * 8)

// PURPOSE: Predefined bitmasks for people too lazy to shift one left themselves.
#define BIT0	(1<<0)
#define BIT1	(1<<1)
#define BIT2	(1<<2)
#define BIT3	(1<<3)
#define BIT4	(1<<4)
#define BIT5	(1<<5)
#define BIT6	(1<<6)
#define BIT7	(1<<7)
#define BIT8	(1<<8)
#define BIT9	(1<<9)
#define BIT10	(1<<10)
#define BIT11	(1<<11)
#define BIT12	(1<<12)
#define BIT13	(1<<13)
#define BIT14	(1<<14)
#define BIT15	(1<<15)
#define BIT16	(1<<16)
#define BIT17	(1<<17)
#define BIT18	(1<<18)
#define BIT19	(1<<19)
#define BIT20	(1<<20)
#define BIT21	(1<<21)
#define BIT22	(1<<22)
#define BIT23	(1<<23)
#define BIT24	(1<<24)
#define BIT25	(1<<25)
#define BIT26	(1<<26)
#define BIT27	(1<<27)
#define BIT28	(1<<28)
#define BIT29	(1<<29)
#define BIT30	(1<<30)
#define BIT31	(1<<31)

namespace rage
{

template< typename T >
inline T BIT_SET( const unsigned bit, const T a ) { return a | T( T( 1 ) << bit ); }

template< typename T >
inline T BIT_CLEAR( const unsigned bit, const T a ) { return a & (~ T( T( 1 ) << bit )); }

template< typename T >
inline bool BIT_ISSET( const unsigned bit, const T a ) { return !!( a & ( T( 1 ) << bit ) ); }

// PURPOSE: Count the number of set bits in a 32-bit number.
// PARAMS: bits - the bit field
// RETURNS: Number of set bits [0..32]
inline u32 CountOnBits(u32 bits)
{
	unsigned int const w = bits - ((bits >> 1) & 0x55555555);     
	unsigned int const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);      
	unsigned int const c = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;  

	return c;
}

inline u64 CountOnBits(u64 bits) {
	bits -= (bits >> 1) & 0x5555555555555555ULL;
	bits = (bits & 0x3333333333333333ULL) + ((bits >> 2) & 0x3333333333333333ULL);
	bits = (bits + (bits >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
	return ((bits * 0x0101010101010101ULL) >> 56);
}

// Given a basic integer type, get the signed integer type of the same size
template< typename T > struct SignedType;
template<> struct SignedType<s8>    { typedef s8  T; };
template<> struct SignedType<s16>   { typedef s16 T; };
template<> struct SignedType<s32>   { typedef s32 T; };
template<> struct SignedType<s64>   { typedef s64 T; };
template<> struct SignedType<u8>    { typedef s8  T; };
template<> struct SignedType<u16>   { typedef s16 T; };
template<> struct SignedType<u32>   { typedef s32 T; };
template<> struct SignedType<u64>   { typedef s64 T; };
#define SIGNED_TYPE(TYPE) typename ::rage::SignedType<TYPE>::T

// Given a basic integer type, get the unsigned integer type of the same size
template< typename T > struct UnsignedType;
template<> struct UnsignedType<s8>  { typedef u8  T; };
template<> struct UnsignedType<s16> { typedef u16 T; };
template<> struct UnsignedType<s32> { typedef u32 T; };
template<> struct UnsignedType<s64> { typedef u64 T; };
template<> struct UnsignedType<u8>  { typedef u8  T; };
template<> struct UnsignedType<u16> { typedef u16 T; };
template<> struct UnsignedType<u32> { typedef u32 T; };
template<> struct UnsignedType<u64> { typedef u64 T; };
#define UNSIGNED_TYPE(TYPE) typename ::rage::UnsignedType<TYPE>::T

// Given a number of bits, get the signed integer type of that size
template< unsigned NUMBITS > struct SIntBits;
template<> struct SIntBits<8>       { typedef s8  T; };
template<> struct SIntBits<16>      { typedef s16 T; };
template<> struct SIntBits<32>      { typedef s32 T; };
template<> struct SIntBits<64>      { typedef s64 T; };

// Given a number of bits, get the unsigned integer type of that size
template< unsigned NUMBITS > struct UIntBits;
template<> struct UIntBits<8>       { typedef u8  T; };
template<> struct UIntBits<16>      { typedef u16 T; };
template<> struct UIntBits<32>      { typedef u32 T; };
template<> struct UIntBits<64>      { typedef u64 T; };


}   //namespace rage

#endif // SYSTEM_BIT_H

