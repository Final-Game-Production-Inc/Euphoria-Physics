// ======================
// math/vecrand.cpp
// (c) 2011 RockstarNorth
// ======================

#include "math/vecrand.h"

#include "grcore/debugdraw.h" // temporary!

namespace rage {

// ================================================================================================
// ================================================================================================
// ================================================================================================

#if __DEV // Vector_4V_proxy and everything in rng_util is dev-only for now

namespace Vec {

template <typename T> class Vector_4V_proxy
{
public:
	__forceinline Vector_4V_proxy() {}
	__forceinline Vector_4V_proxy(Vector_4V_In v_) : m_v128(v_) {}

	__forceinline Vector_4V_proxy(T x, T y)
	{
		CompileTimeAssert(sizeof(Vector_4V) == 2*sizeof(T));
		m_v[0] = x;
		m_v[1] = y;
	}
	__forceinline Vector_4V_proxy(T x, T y, T z, T w)
	{
		CompileTimeAssert(sizeof(Vector_4V) == 4*sizeof(T));
		m_v[0] = x;
		m_v[1] = y;
		m_v[2] = z;
		m_v[3] = w;
	}
	__forceinline Vector_4V_proxy(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1)
	{
		CompileTimeAssert(sizeof(Vector_4V) == 4*sizeof(T));
		m_v[0] = x0;
		m_v[1] = y0;
		m_v[2] = z0;
		m_v[3] = w0;
		m_v[4] = x1;
		m_v[5] = y1;
		m_v[6] = z1;
		m_v[7] = w1;
	}

	__forceinline operator Vector_4V_Out() const { return m_v128; }

	__forceinline       T& operator[](int i)       { return m_v[i]; }
	__forceinline const T& operator[](int i) const { return m_v[i]; }

private:
	union
	{
		Vector_4V m_v128;
		T         m_v[sizeof(Vector_4V)/sizeof(T)];
	};
};

} // namespace Vec

// ================================================================================================

namespace rng_util {

template <int a, int b, int c> __forceinline u32 XorShiftRLR31(u32 x)
{
	x ^= (x >> a);
	x ^= (x << b); x &= 0x7fffffff;
	x ^= (x >> c);

	return x;
}

template <int a, int b, int c> __forceinline u32 XorShiftLRL32(u32 x)
{
	x ^= (x << a);
	x ^= (x >> b);
	x ^= (x << c);

	return x;
}

} // namespace rng_util

// ================================================================================================

namespace rng_util {

template <typename T, int N> class GM2 // GF2 matrix, where N <= sizeof(T)*8
{
public:
	inline GM2();
	inline GM2(T (*gen)(T));

	inline void SetIdentity();
	inline bool IsIdentity() const;

	inline T    Mul(T b) const;
	inline void Mul(const GM2<T,N>& b);
	inline void Sqr();
	inline void Exp(int k);
	inline T    ExpMul(T x, int k) const; // faster than calling Exp(k) and then Mul(x)

	T v[N];
};

template <typename T, int N> inline GM2<T,N>::GM2()
{
}

template <typename T, int N> inline GM2<T,N>::GM2(T (*gen)(T))
{
	for (int i = 0; i < N; i++)
	{
		v[i] = gen(T(1) << i);
	}
}

template <typename T, int N> inline void GM2<T,N>::SetIdentity()
{
	for (int i = 0; i < N; i++)
	{
		v[i] = T(1) << i;
	}
}

template <typename T, int N> inline bool GM2<T,N>::IsIdentity() const
{
	for (int i = 0; i < N; i++)
	{
		if (v[i] != (T(1) << i))
		{
			return false;
		}
	}

	return true;
}

template <typename T> __forceinline T BitSplat(T x, int i);

template <> __forceinline u16 BitSplat<u16>(u16 x, int i) { return (u16)( (s16(x) << (15 - i)) >> 15 ); }
template <> __forceinline u32 BitSplat<u32>(u32 x, int i) { return (u32)( (s32(x) << (31 - i)) >> 31 ); }
template <> __forceinline u64 BitSplat<u64>(u64 x, int i) { return (u64)( (s64(x) << (63 - i)) >> 63 ); }

template <typename T, int N> inline T GM2<T,N>::Mul(T b) const
{
	T y = T(0);

	for (int i = 0; i < N; i++)
	{
		y ^= (v[i] & BitSplat<T>(b, i));
	}

	return y;
}

template <typename T, int N> inline void GM2<T,N>::Mul(const GM2<T,N>& b)
{
	FastAssert(this != &b);

	for (int i = 0; i < N; i++)
	{
		v[i] = b.Mul(v[i]);
	}
}

template <typename T, int N> inline void GM2<T,N>::Sqr()
{
	GM2<T,N> b = *this;
	Mul(b);
}

template <typename T, int N> inline void GM2<T,N>::Exp(int k)
{
	GM2<T,N> b = *this;

	SetIdentity();

	while (k != 0)
	{
		if (k & 1)
		{
			Mul(b);
		}

		k >>= 1;
		b.Sqr();
	}
}

template <typename T, int N> inline T GM2<T,N>::ExpMul(T x, int k) const // faster than calling Exp(k) and then Mul(x)
{
	GM2<T,N> b = *this;

	while (k != 0)
	{
		if (k & 1)
		{
			x = b.Mul(x);
		}

		k >>= 1;
		b.Sqr();
	}

	return x;
}

} // namespace rng_util

// ================================================================================================

namespace rng_util {

class GMV // 32x32 GF2 matrix using vectors
{
public:
	enum { N = 32, M = (N+3)/4 };

	inline GMV();
	inline GMV(u32 (*gen)(u32));

	inline void SetIdentity();
	inline bool IsIdentity() const;

	inline u32                Mul(u32               b) const;
	inline Vec::Vector_4V_Out Mul(Vec::Vector_4V_In b) const; // 4x parallel multiply
	inline Vec::Vector_4V_Out Xor(Vec::Vector_4V_In x) const; // internal

//	inline Vec::Vector_4V_Out Mul_Expanded(Vec::Vector_4V_In b) const; // expanded version of GMV::Mul with GMV::Xor included .. maybe there's a way to optimise this?

	inline void Mul(const GMV& b);
	inline void Sqr();
	inline void Exp(u32 k);

	inline u32                ExpMul(u32               x, u32 k) const; // faster than calling Exp(k) and then Mul(x)
	inline Vec::Vector_4V_Out ExpMul(Vec::Vector_4V_In x, u32 k) const; // faster than calling Exp(k) and then Mul(x)

	Vec::Vector_4V v[M];
};

inline GMV::GMV()
{
}

inline GMV::GMV(u32 (*gen)(u32))
{
	for (int j = 0; j < M; j++)
	{
		v[j] = Vec::Vector_4V_proxy<u32>(
			gen(1ul << (0 + j*4)),
			gen(1ul << (1 + j*4)),
			gen(1ul << (2 + j*4)),
			gen(1ul << (3 + j*4))
		);
	}
}

inline void GMV::SetIdentity()
{
	for (int j = 0; j < M; j++)
	{
		v[j] = Vec::Vector_4V_proxy<u32>(
			(1ul << (0 + j*4)),
			(1ul << (1 + j*4)),
			(1ul << (2 + j*4)),
			(1ul << (3 + j*4))
		);
	}
}

inline bool GMV::IsIdentity() const
{
	for (int j = 0; j < M; j++)
	{
		const Vec::Vector_4V_proxy<u32> temp = v[j];

		if (temp[0] != (1ul << (0 + j*4))) { return false; }
		if (temp[1] != (1ul << (1 + j*4))) { return false; }
		if (temp[2] != (1ul << (2 + j*4))) { return false; }
		if (temp[3] != (1ul << (3 + j*4))) { return false; }
	}

	return true;
}

inline u32 GMV::Mul(u32 b) const
{
	const Vec::Vector_4V_proxy<u32> temp = Xor(Vec::V4LoadScalar32IntoSplatted(b));

	const u32 x0 = temp[0];
	const u32 x1 = temp[1];
	const u32 x2 = temp[2];
	const u32 x3 = temp[3];

	return x0 ^ x1 ^ x2 ^ x3;
}

inline Vec::Vector_4V_Out GMV::Mul(Vec::Vector_4V_In b) const
{
	Vec::Vector_4V x0,x1,x2,x3;
	Vec::Vector_4V y0,y1,y2,y3;

	x0 = Xor(Vec::V4SplatX(b));
	x1 = Xor(Vec::V4SplatY(b));
	x2 = Xor(Vec::V4SplatZ(b));
	x3 = Xor(Vec::V4SplatW(b));

	// 4x4 transpose
	y0 = Vec::V4MergeXY(x0, x2);
	y1 = Vec::V4MergeXY(x1, x3);
	y2 = Vec::V4MergeZW(x0, x2);
	y3 = Vec::V4MergeZW(x1, x3);
	x0 = Vec::V4MergeXY(y0, y1);
	x1 = Vec::V4MergeZW(y0, y1);
	x2 = Vec::V4MergeXY(y2, y3);
	x3 = Vec::V4MergeZW(y2, y3);

	x0 = Vec::V4Xor(x0, x1);
	x2 = Vec::V4Xor(x2, x3);

	x0 = Vec::V4Xor(x0, x2);

	return x0;
}

inline Vec::Vector_4V_Out GMV::Xor(Vec::Vector_4V_In x_) const
{
#if 1 || RSG_CPU_INTEL
	Vec::Vector_4V_proxy<u32> temp(x_);
	temp[0] >>= 0;
	temp[1] >>= 1;
	temp[2] >>= 2;
	temp[3] >>= 3;
	const Vec::Vector_4V x = temp;
#else
	const Vec::Vector_4V x = Vec::V4ShiftRight(x_, Vec::V4VConstant<0,1,2,3>()); // relies on vector shift function which is currently in fwmaths
#endif

	Vec::Vector_4V y0 = Vec::V4And(v[0], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*0>(x)));
	Vec::Vector_4V y1 = Vec::V4And(v[1], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*1>(x)));
	Vec::Vector_4V y2 = Vec::V4And(v[2], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*2>(x)));
	Vec::Vector_4V y3 = Vec::V4And(v[3], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*3>(x)));
	Vec::Vector_4V y4 = Vec::V4And(v[4], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*4>(x)));
	Vec::Vector_4V y5 = Vec::V4And(v[5], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*5>(x)));
	Vec::Vector_4V y6 = Vec::V4And(v[6], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*6>(x)));
	Vec::Vector_4V y7 = Vec::V4And(v[7], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*7>(x)));

	y0 = Vec::V4Xor(y0, y1);
	y2 = Vec::V4Xor(y2, y3);
	y4 = Vec::V4Xor(y4, y5);
	y6 = Vec::V4Xor(y6, y7);

	y0 = Vec::V4Xor(y0, y2);
	y4 = Vec::V4Xor(y4, y6);

	y0 = Vec::V4Xor(y0, y4);

	return y0;
}

/*inline Vec::Vector_4V_Out GMV::Mul_Expanded(Vec::Vector_4V_In b) const // expanded version of GMV::Mul with GMV::Xor included .. maybe there's a way to optimise this?
{
	Vec::Vector_4V xa = Vec::V4SplatX(b);
	Vec::Vector_4V xb = Vec::V4SplatY(b);
	Vec::Vector_4V xc = Vec::V4SplatZ(b);
	Vec::Vector_4V xd = Vec::V4SplatW(b);

#if RSG_CPU_INTEL
	xa.m128_u32[0] >>= 0; xa.m128_u32[1] >>= 1; xa.m128_u32[2] >>= 2; xa.m128_u32[3] >>= 3;
	xb.m128_u32[0] >>= 0; xb.m128_u32[1] >>= 1; xb.m128_u32[2] >>= 2; xb.m128_u32[3] >>= 3;
	xc.m128_u32[0] >>= 0; xc.m128_u32[1] >>= 1; xc.m128_u32[2] >>= 2; xc.m128_u32[3] >>= 3;
	xd.m128_u32[0] >>= 0; xd.m128_u32[1] >>= 1; xd.m128_u32[2] >>= 2; xd.m128_u32[3] >>= 3;
#else
	xa = Vec::V4ShiftRight(xa, Vec::V4VConstant<0,1,2,3>());
	xb = Vec::V4ShiftRight(xb, Vec::V4VConstant<0,1,2,3>());
	xc = Vec::V4ShiftRight(xc, Vec::V4VConstant<0,1,2,3>());
	xd = Vec::V4ShiftRight(xd, Vec::V4VConstant<0,1,2,3>());
#endif

	Vec::Vector_4V ya0 = Vec::V4And(v[0], V4ShiftRightAlgebraic<31>(V4ShiftLeft<31-4*0>(xa)));
	Vec::Vector_4V yb0 = Vec::V4And(v[0], V4ShiftRightAlgebraic<31>(V4ShiftLeft<31-4*0>(xb)));
	Vec::Vector_4V yc0 = Vec::V4And(v[0], V4ShiftRightAlgebraic<31>(V4ShiftLeft<31-4*0>(xc)));
	Vec::Vector_4V yd0 = Vec::V4And(v[0], V4ShiftRightAlgebraic<31>(V4ShiftLeft<31-4*0>(xd)));

	Vec::Vector_4V ya1 = Vec::V4And(v[1], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*1>(xa)));
	Vec::Vector_4V yb1 = Vec::V4And(v[1], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*1>(xb)));
	Vec::Vector_4V yc1 = Vec::V4And(v[1], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*1>(xc)));
	Vec::Vector_4V yd1 = Vec::V4And(v[1], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*1>(xd)));

	Vec::Vector_4V ya2 = Vec::V4And(v[2], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*2>(xa)));
	Vec::Vector_4V yb2 = Vec::V4And(v[2], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*2>(xb)));
	Vec::Vector_4V yc2 = Vec::V4And(v[2], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*2>(xc)));
	Vec::Vector_4V yd2 = Vec::V4And(v[2], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*2>(xd)));

	Vec::Vector_4V ya3 = Vec::V4And(v[3], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*3>(xa)));
	Vec::Vector_4V yb3 = Vec::V4And(v[3], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*3>(xb)));
	Vec::Vector_4V yc3 = Vec::V4And(v[3], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*3>(xc)));
	Vec::Vector_4V yd3 = Vec::V4And(v[3], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*3>(xd)));

	Vec::Vector_4V ya4 = Vec::V4And(v[4], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*4>(xa)));
	Vec::Vector_4V yb4 = Vec::V4And(v[4], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*4>(xb)));
	Vec::Vector_4V yc4 = Vec::V4And(v[4], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*4>(xc)));
	Vec::Vector_4V yd4 = Vec::V4And(v[4], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*4>(xd)));

	Vec::Vector_4V ya5 = Vec::V4And(v[5], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*5>(xa)));
	Vec::Vector_4V yb5 = Vec::V4And(v[5], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*5>(xb)));
	Vec::Vector_4V yc5 = Vec::V4And(v[5], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*5>(xc)));
	Vec::Vector_4V yd5 = Vec::V4And(v[5], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*5>(xd)));

	Vec::Vector_4V ya6 = Vec::V4And(v[6], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*6>(xa)));
	Vec::Vector_4V yb6 = Vec::V4And(v[6], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*6>(xb)));
	Vec::Vector_4V yc6 = Vec::V4And(v[6], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*6>(xc)));
	Vec::Vector_4V yd6 = Vec::V4And(v[6], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*6>(xd)));

	Vec::Vector_4V ya7 = Vec::V4And(v[7], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*7>(xa)));
	Vec::Vector_4V yb7 = Vec::V4And(v[7], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*7>(xb)));
	Vec::Vector_4V yc7 = Vec::V4And(v[7], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*7>(xc)));
	Vec::Vector_4V yd7 = Vec::V4And(v[7], Vec::V4ShiftRightAlgebraic<31>(Vec::V4ShiftLeft<31-4*7>(xd)));

	ya0 = Vec::V4Xor(ya0, ya1);
	ya2 = Vec::V4Xor(ya2, ya3);
	ya4 = Vec::V4Xor(ya4, ya5);
	ya6 = Vec::V4Xor(ya6, ya7);

	ya0 = Vec::V4Xor(ya0, ya2);
	ya4 = Vec::V4Xor(ya4, ya6);

	ya0 = Vec::V4Xor(ya0, ya4);

	yb0 = Vec::V4Xor(yb0, yb1);
	yb2 = Vec::V4Xor(yb2, yb3);
	yb4 = Vec::V4Xor(yb4, yb5);
	yb6 = Vec::V4Xor(yb6, yb7);

	yb0 = Vec::V4Xor(yb0, yb2);
	yb4 = Vec::V4Xor(yb4, yb6);

	yb0 = Vec::V4Xor(yb0, yb4);

	yc0 = Vec::V4Xor(yc0, yc1);
	yc2 = Vec::V4Xor(yc2, yc3);
	yc4 = Vec::V4Xor(yc4, yc5);
	yc6 = Vec::V4Xor(yc6, yc7);

	yc0 = Vec::V4Xor(yc0, yc2);
	yc4 = Vec::V4Xor(yc4, yc6);

	yc0 = Vec::V4Xor(yc0, yc4);

	yd0 = Vec::V4Xor(yd0, yd1);
	yd2 = Vec::V4Xor(yd2, yd3);
	yd4 = Vec::V4Xor(yd4, yd5);
	yd6 = Vec::V4Xor(yd6, yd7);

	yd0 = Vec::V4Xor(yd0, yd2);
	yd4 = Vec::V4Xor(yd4, yd6);

	yd0 = Vec::V4Xor(yd0, yd4);

	Vec::Vector_4V x0 = ya0;
	Vec::Vector_4V x1 = yb0;
	Vec::Vector_4V x2 = yc0;
	Vec::Vector_4V x3 = yd0;
	Vec::Vector_4V y0,y1,y2,y3;

	// 4x4 transpose
	y0 = Vec::V4MergeXY(x0, x2);
	y1 = Vec::V4MergeXY(x1, x3);
	y2 = Vec::V4MergeZW(x0, x2);
	y3 = Vec::V4MergeZW(x1, x3);
	x0 = Vec::V4MergeXY(y0, y1);
	x1 = Vec::V4MergeZW(y0, y1);
	x2 = Vec::V4MergeXY(y2, y3);
	x3 = Vec::V4MergeZW(y2, y3);

	x0 = Vec::V4Xor(x0, x1);
	x2 = Vec::V4Xor(x2, x3);

	x0 = Vec::V4Xor(x0, x2);

	return x0;
}*/

inline void GMV::Mul(const GMV& b)
{
	FastAssert(this != &b);

	for (int j = 0; j < M; j++)
	{
		v[j] = b.Mul(v[j]);
	}
}

inline void GMV::Sqr()
{
	GMV b = *this;
	Mul(b);
}

inline void GMV::Exp(u32 k)
{
	GMV b = *this;

	SetIdentity();

	while (k != 0)
	{
		if (k & 1)
		{
			Mul(b);
		}

		k >>= 1;
		b.Sqr();
	}
}

inline u32 GMV::ExpMul(u32 x, u32 k) const
{
	GMV b = *this;

	while (k != 0)
	{
		if (k & 1)
		{
			x = b.Mul(x);
		}

		k >>= 1;
		b.Sqr();
	}

	return x;
}

inline Vec::Vector_4V_Out GMV::ExpMul(Vec::Vector_4V_In x_, u32 k) const
{
	GMV b = *this;
	Vec::Vector_4V x = x_;

	while (k != 0)
	{
		if (k & 1)
		{
			x = b.Mul(x);
		}

		k >>= 1;
		b.Sqr();
	}

	return x;
}

} // namespace rng_util

// ================================================================================================

namespace rng_util {

void _test_XorShiftRLR31SkipAhead()
{
	Displayf("_test_XorShiftRLR31SkipAhead");

#define N 31
#define XORSHIFT_GEN XorShiftRLR31<XorShiftRLR31_ARGS>

	const u32 x0 = 1234567;
	const int k = 54321;

	u32 x1 = x0;
	u32 x2 = x0;
	u32 x3 = x0;

	for (int i = 0; i < k; i++) // test1: perform XorShift sequentially k times .. (slow!)
	{
		x1 = XORSHIFT_GEN(x1);
	}

	x2 = GM2<u32,N>(XORSHIFT_GEN).ExpMul(x2, k); // test2: scalar skip-ahead
	x3 = GMV       (XORSHIFT_GEN).ExpMul(x3, k); // test3: vectorised skip-ahead

	Displayf("state[0*1000000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 0*1000000));
	Displayf("state[1*1001000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 1*1001000));
	Displayf("state[2*1002000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 2*1002000));
	Displayf("state[3*1003000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 3*1003000));

	Displayf("x1 = 0x%08x", x1);
	Displayf("x2 = 0x%08x", x2);
	Displayf("x3 = 0x%08x", x3);
	Displayf("%s", (x1 == x2 && x1 == x3) ? "passed." : "failed!");

#undef N
#undef XORSHIFT_GEN
}

void _test_XorShiftLRL32SkipAhead()
{
	Displayf("_test_XorShiftLRL32SkipAhead");

#define N 32
#define XORSHIFT_GEN XorShiftLRL32<XorShiftLRL32_ARGS>

	const u32 x0 = 1234567;
	const int k = 54321;

	u32 x1 = x0;
	u32 x2 = x0;
	u32 x3 = x0;

	for (int i = 0; i < k; i++) // test1: perform XorShift sequentially k times .. (slow!)
	{
		x1 = XORSHIFT_GEN(x1);
	}

	x2 = GM2<u32,N>(XORSHIFT_GEN).ExpMul(x2, k); // test2: scalar skip-ahead
	x3 = GMV       (XORSHIFT_GEN).ExpMul(x3, k); // test3: vectorised skip-ahead

	Displayf("state[0*1000000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 0*1000000));
	Displayf("state[1*1001000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 1*1001000));
	Displayf("state[2*1002000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 2*1002000));
	Displayf("state[3*1003000] = 0x%08x", GMV(XORSHIFT_GEN).ExpMul(1, 3*1003000));

	Displayf("x1 = 0x%08x", x1);
	Displayf("x2 = 0x%08x", x2);
	Displayf("x3 = 0x%08x", x3);
	Displayf("%s", (x1 == x2 && x1 == x3) ? "passed." : "failed!");

#undef N
#undef XORSHIFT_GEN
}

} // namespace rng_util

#endif // __DEV

} // namespace rage
