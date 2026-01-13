// 
// system/endian.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_ENDIAN_H
#define SYSTEM_ENDIAN_H

namespace rage
{

//PURPOSE
//  This class provides a set of endian utility functions.
//NOTES
//  In some of the functions there is code of the form:
//
//      return __BE ? val : Swap( val );
//
//  Becuase __BE is a constant the conditional should be optimized out
//  in release builds.
//
namespace sysEndian
{
    //PURPOSE
    //  Returns true if the host machine is big endian.
    inline bool IsBig();

    //PURPOSE
    //  Returns true if the host machine is little endian.
    inline bool IsLittle();

    //PURPOSE
    //  Swaps the bytes of some standard types.
	//Notes: Add a new overload of this function if you need to swap a new POD type.
	// If you are doing all your swaps in-place you could also specialize SwapMe below instead.
    inline u64 Swap( const u64 val );
    inline s64 Swap( const s64 val );
    inline u32 Swap( const u32 val );
    inline s32 Swap( const s32 val );
    inline u16 Swap( const u16 val );
    inline s16 Swap( const s16 val );
    inline u8  Swap( const u8 val ) { return val; }
    inline s8  Swap( const s8 val ) { return val; }

	// These are not safe; if the bit pattern happens to match a QNAN it may get rewritten by the FPU if it ever gets loaded into a floating-point register!
	// They are intentionally unimplemented so the compiler won't silently cast to an integer version.  Any compiler we currently use
	// will warn on this anyway, but this would be more future-proof.  (For now we're relying on the compiler warning since that catches the problem sooner,
	// but I confirmed the game and ragebuilder still link with these overloads declared but not implemented).
    // double Swap( const double val );
    // float Swap( const float val );


	// In-place versions of the above functions:

	//PURPOSE
	//	Swaps the bytes of some standard types in place.
	//Notes: Specialize this if you need to swap a new type in-place, and don't want or need 
	// to swap via a return value. If you do need that, overload Swap() instead.
	template<typename _Type> void SwapMe(_Type &val);
	template<> inline void SwapMe(u64 &val) { val = Swap(val); }
	template<> inline void SwapMe(s64 &val) { val = Swap(val); }
	template<> inline void SwapMe(u32 &val) { val = Swap(val); }
	template<> inline void SwapMe(s32 &val) { val = Swap(val); }
	template<> inline void SwapMe(u16 &val) { val = Swap(val); }
	template<> inline void SwapMe(s16 &val) { val = Swap(val); }
	template<> inline void SwapMe(u8 &/*val*/) { }
	template<> inline void SwapMe(s8 &/*val*/) { }
	template<> inline void SwapMe(float & val) { (u32&)val = Swap((u32&)val); }
	template<> inline void SwapMe(double & val) { (u64&)val = Swap((u64&)val); }

	template <typename _Type> inline void BtoNMe(_Type& val) { if (!__BE) SwapMe(val); }
	template <typename _Type> inline void NtoBMe(_Type& val) { if (!__BE) SwapMe(val); }
	template <typename _Type> inline void LtoNMe(_Type& val) { if (__BE) SwapMe(val); }
	template <typename _Type> inline void NtoLMe(_Type& val) { if (__BE) SwapMe(val); }
	template <typename _Type> inline void BtoLMe(_Type& val) { SwapMe(val); }
	template <typename _Type> inline void LtoBMe(_Type& val) { SwapMe(val); }

	//PURPOSE
	//  Big- to native-endian.
	inline u64 BtoN(u64 val) { return __BE ? val : Swap(val); }
	inline s64 BtoN(s64 val) { return __BE ? val : Swap(val); }
	inline u32 BtoN(u32 val) { return __BE ? val : Swap(val); }
	inline s32 BtoN(s32 val) { return __BE ? val : Swap(val); }
	inline u16 BtoN(u16 val) { return __BE ? val : Swap(val); }
	inline s16 BtoN(s16 val) { return __BE ? val : Swap(val); }
	inline u8 BtoN(u8 val) { return __BE ? val : Swap(val); }
	inline s8 BtoN(s8 val) { return __BE ? val : Swap(val); }

	//PURPOSE
	//  Native- to big-endian.
	inline u64 NtoB(u64 val) { return __BE ? val : Swap(val); }
	inline s64 NtoB(s64 val) { return __BE ? val : Swap(val); }
	inline u32 NtoB(u32 val) { return __BE ? val : Swap(val); }
	inline s32 NtoB(s32 val) { return __BE ? val : Swap(val); }
	inline u16 NtoB(u16 val) { return __BE ? val : Swap(val); }
	inline s16 NtoB(s16 val) { return __BE ? val : Swap(val); }
	inline u8 NtoB(u8 val) { return __BE ? val : Swap(val); }
	inline s8 NtoB(s8 val) { return __BE ? val : Swap(val); }

	//PURPOSE
	//  Little- to native-endian.
	inline u64 LtoN(u64 val) { return __BE ? Swap(val) : val; }
	inline s64 LtoN(s64 val) { return __BE ? Swap(val) : val; }
	inline u32 LtoN(u32 val) { return __BE ? Swap(val) : val; }
	inline s32 LtoN(s32 val) { return __BE ? Swap(val) : val; }
	inline u16 LtoN(u16 val) { return __BE ? Swap(val) : val; }
	inline s16 LtoN(s16 val) { return __BE ? Swap(val) : val; }
	inline u8 LtoN(u8 val) { return __BE ? Swap(val) : val; }
	inline s8 LtoN(s8 val) { return __BE ? Swap(val) : val; }

	//PURPOSE
	//  Native- to little-endian.
	inline u64 NtoL(u64 val) { return __BE ? Swap(val) : val; }
	inline s64 NtoL(s64 val) { return __BE ? Swap(val) : val; }
	inline u32 NtoL(u32 val) { return __BE ? Swap(val) : val; }
	inline s32 NtoL(s32 val) { return __BE ? Swap(val) : val; }
	inline u16 NtoL(u16 val) { return __BE ? Swap(val) : val; }
	inline s16 NtoL(s16 val) { return __BE ? Swap(val) : val; }
	inline u8 NtoL(u8 val) { return __BE ? Swap(val) : val; }
	inline s8 NtoL(s8 val) { return __BE ? Swap(val) : val; }

	//PURPOSE
	//  Big- to little-endian.
	inline u64 BtoL(u64 val) { return Swap(val); }
	inline s64 BtoL(s64 val) { return Swap(val); }
	inline u32 BtoL(u32 val) { return Swap(val); }
	inline s32 BtoL(s32 val) { return Swap(val); }
	inline u16 BtoL(u16 val) { return Swap(val); }
	inline s16 BtoL(s16 val) { return Swap(val); }
	inline u8 BtoL(u8 val) { return Swap(val); }
	inline s8 BtoL(s8 val) { return Swap(val); }

	//PURPOSE
	//  Little- to big-endian.
	inline u64 LtoB(u64 val) { return Swap(val); }
	inline s64 LtoB(s64 val) { return Swap(val); }
	inline u32 LtoB(u32 val) { return Swap(val); }
	inline s32 LtoB(s32 val) { return Swap(val); }
	inline u16 LtoB(u16 val) { return Swap(val); }
	inline s16 LtoB(s16 val) { return Swap(val); }
	inline u8 LtoB(u8 val) { return Swap(val); }
	inline s8 LtoB(s8 val) { return Swap(val); }


//////////////////////////////////////////
// Implementations

inline
bool
IsBig()
{
    static const union IsBig{ u32 u; u8 c[ 4 ]; } ISBIG = { 1 };

    return ISBIG.c[ 3 ] == 1;
}

inline
bool
IsLittle()
{
    return !IsBig();
}

inline
u64
Swap( const u64 val )
{
    return ( val << 56 ) |
            ( ( val << 40 ) & 0x00FF000000000000ULL ) |
            ( ( val << 24 ) & 0x0000FF0000000000ULL ) |
            ( ( val << 8 )  & 0x000000FF00000000ULL ) |
            ( ( val >> 8 )  & 0x00000000FF000000ULL ) |
            ( ( val >> 24 ) & 0x0000000000FF0000ULL ) |
            ( ( val >> 40 ) & 0x000000000000FF00ULL ) |
            ( val >> 56 );
}

inline
s64
Swap( const s64 val )
{
    return Swap( ( u64 ) val );
}

inline
u32
Swap( const u32 val )
{
    return ( val << 24 ) |
           ( ( val << 8 ) & 0x00FF0000 ) |
           ( ( val >> 8 ) & 0x0000FF00 ) |
           ( val >> 24 );
}

inline
s32
Swap( const s32 val )
{
    return Swap( ( u32 ) val );
}

inline
u16
Swap( const u16 val )
{
    return ( val << 8 ) | ( val >> 8 );
}

inline
s16
Swap( const s16 val )
{
    return Swap( ( u16 ) val );
}

}   //namespace sysEndian

}   //namespace rage

#endif  //SYSTEM_ENDIAN_H
