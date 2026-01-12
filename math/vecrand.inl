// ======================
// math/vecrand.inl
// (c) 2011 RockstarNorth
// ======================

namespace rage {
namespace Vec {

#define XorShiftRLR31_ARGS 11,13,20
#define XorShiftRLR31_INIT 0x00000001,0x7e50015b,0x09c5f917,0x2291212a

#define XorShiftLRL32_ARGS 11,21,13
#define XorShiftLRL32_INIT 0x00000001,0xf4fed124,0x18973670,0xa76d4499

#define XorShiftRLR61_ARGS 11,28,39

template <int a, int b, int c> static __forceinline Vector_4V_Out V4Rand_XorShiftRLR31(Vector_4V_InOut v)
{
	const Vector_4V mask = V4VConstant(V_80000000);
	const Vector_4V one  = V4VConstant(V_ONE);

	Vector_4V x = v;

	x = V4Xor (x, V4ShiftRight<a>(x)); // Mersenne-prime XorShiftRLR (period 2^^31-1)
	x = V4Xor (x, V4ShiftLeft <b>(x));
	x = V4Andc(x, mask);
	x = V4Xor (x, V4ShiftRight<c>(x));
	v = x;
	x = V4ShiftRight<8>(x     ); // shift bits into mantissa
	x = V4Or           (x, one); // bitwise-OR 1.0f so range is now [1..2) in floating-point
	x = V4Subtract     (x, one); // subtract 1.0f, range is [0..1)

	return x;
}

template <int a, int b, int c> static __forceinline Vector_4V_Out V4Rand_XorShiftLRL32(Vector_4V_InOut v)
{
	const Vector_4V one = V4VConstant(V_ONE);

	Vector_4V x = v;

	x = V4Xor(x, V4ShiftLeft <a>(x)); // XorShiftLRL (period 2^32-1, not a prime)
	x = V4Xor(x, V4ShiftRight<b>(x));
	x = V4Xor(x, V4ShiftLeft <c>(x));
	v = x;
	x = V4ShiftRight<9>(x     ); // shift bits into mantissa
	x = V4Or           (x, one); // bitwise-OR 1.0f so range is now [1..2) in floating-point
	x = V4Subtract     (x, one); // subtract 1.0f, range is [0..1)

	return x;
}

__forceinline Vector_4V_Out V4RandInit()
{
	return V4VConstant<XorShiftRLR31_INIT>();
}

__forceinline Vector_4V_Out V4Rand(Vector_4V_InOut seed)
{
	return V4Rand_XorShiftRLR31<XorShiftRLR31_ARGS>(seed);
}

} // namespace Vec

// ================================================================================================
// utilities

template <typename T, int n> static __forceinline T XorL(T x) { return x ^ (x << n); }
template <typename T, int n> static __forceinline T XorR(T x) { return x ^ (x >> n); }

template <typename T, int n> static __forceinline T InvertXorL(T x) { int i = n; while (i < (int)sizeof(T)*8) { x ^= (x << i); i <<= 1; } return x; }
template <typename T, int n> static __forceinline T InvertXorR(T x) { int i = n; while (i < (int)sizeof(T)*8) { x ^= (x >> i); i <<= 1; } return x; }

#define XORSHIFT_LRL_DEFAULT u32,13,17,5

template <typename T, int a, int b, int c> static __forceinline T XorShiftLRL(T x)
{
	x = XorL<T,a>(x);
	x = XorR<T,b>(x);
	x = XorL<T,c>(x);
	return x;
}

template <typename T, int a, int b, int c> static __forceinline T InvertXorShiftLRL(T x)
{
	x = InvertXorL<T,c>(x);
	x = InvertXorR<T,b>(x);
	x = InvertXorL<T,a>(x);
	return x; // note that InvertXorShiftLRL<T,a,b,c>(XorShiftLRL<T,a,b,c>(x)) == x
}

// reverse the bits used to represent entity id's in the render target - this massively improves the contrast when viewing the target in testbed maps
__forceinline u32 ReverseBits32(u32 a)
{
	a = ((a >> 1) & 0x55555555UL) | ((a & 0x55555555UL) << 1);
	a = ((a >> 2) & 0x33333333UL) | ((a & 0x33333333UL) << 2);
	a = ((a >> 4) & 0x0f0f0f0fUL) | ((a & 0x0f0f0f0fUL) << 4);
	a = ((a >> 8) & 0x00ff00ffUL) | ((a & 0x00ff00ffUL) << 8);
	return (a >> 0x10) | (a << 0x10);
}

__forceinline u32 ScrambleBits32(u32 a)
{
	return XorShiftLRL<XORSHIFT_LRL_DEFAULT>(ReverseBits32(a));
}

__forceinline u32 UnscrambleBits32(u32 a)
{
	return ReverseBits32(InvertXorShiftLRL<XORSHIFT_LRL_DEFAULT>(a));
}

} // namespace rage
