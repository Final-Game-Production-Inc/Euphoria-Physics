// 
// vector/vector3_win32.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTOR3_WIN32_H
#define VECTOR_VECTOR3_WIN32_H


// Default Vector3 Implementations


namespace rage
{

inline __m128 NewtonRaphsonRecip(__m128 val)
{
	// r = 1 / val
	// nms = - ((r * val) - 1)
	// m = (r * nms) + r

	__m128 recip = _mm_rcp_ps(val);
	__m128 nms = _mm_sub_ps(_mm_setzero_ps(), _mm_sub_ps(_mm_mul_ps(recip, val), _mm_set1_ps(1.0f)));
	return _mm_add_ps(_mm_mul_ps(recip, nms), recip);
}

inline __m128 NewtonRaphsonRsqrt(__m128 val)
{
	// rsqrt = 1 / sqrte(val)
	// squaredEstimate = (rsqrt * rsqrt)
	// halfEstimate = (0.5 * rsqrt)
	// nms = -((val * squaredEstimate) - 1)
	// m = (nms * halfEstimate) + rsqrt

	__m128 rsqrt = _mm_rsqrt_ps(val);
	__m128 squaredEstimate = _mm_mul_ps(rsqrt, rsqrt);
	__m128 halfEstimate = _mm_mul_ps(_mm_set1_ps(0.5f), rsqrt);
	__m128 nms = _mm_sub_ps(_mm_setzero_ps(), _mm_sub_ps(_mm_mul_ps(val, squaredEstimate), _mm_set1_ps(1.0f)));
	return _mm_add_ps(_mm_mul_ps(nms, halfEstimate), rsqrt);
}

inline __m128 NewtonRaphsonSqrt(__m128 val)
{
	// rsqrt = 1 / sqrte(val)
	// squaredEstimate = (rsqrt * rsqrt)
	// halfEstimate = (0.5 * rsqrt)
	// nms = -((val * squaredEstimate) - 1)
	// m = ((nms * halfEstimate) + rsqrt) * val;

	__m128 rsqrt = _mm_rsqrt_ps(val);
	__m128 squaredEstimate = _mm_mul_ps(rsqrt, rsqrt);
	__m128 halfEstimate = _mm_mul_ps(_mm_set1_ps(0.5f), rsqrt);
	__m128 nms = _mm_sub_ps(_mm_setzero_ps(), _mm_sub_ps(_mm_mul_ps(val, squaredEstimate), _mm_set1_ps(1.0f)));
	return _mm_mul_ps(_mm_add_ps(_mm_mul_ps(nms, halfEstimate), rsqrt), val);
}

	//=============================================================================
	// Implementations

	inline Vector3::Vector3( _ZeroType )
	{
		xyzw = _vzerofp;
	}

	inline Vector3::Vector3(const __m128& set)
	{
		xyzw = set;
	}

#if VECTORIZED

#ifndef VECTOR3_OPERATOR_ASSIGN
#define VECTOR3_OPERATOR_ASSIGN
	VEC3_INLINE Vector3& Vector3::operator=(const Vector3& v)
	{
		xyzw = v.xyzw;
		return *this;
	}
#endif // VECTOR3_OPERATOR_ASSIGN

#ifndef VECTOR3_SET_F
#define VECTOR3_SET_F
	inline void Vector3::Set(float s)
	{
		xyzw = _mm_set1_ps(s);
	}
#endif // VECTOR3_SETF

#ifndef VECTOR3_SETSCALED
#define VECTOR3_SETSCALED
	inline void Vector3::SetScaled(Vector3Param a, float s)
	{
		__m128 temp = _mm_set1_ps(s);
		xyzw = _mm_mul_ps(a.xyzw, temp);
	}
#endif // VECTOR3_SETSCALED

#ifndef VECTOR3_ZERO
#define VECTOR3_ZERO
	inline void Vector3::Zero()
	{
		xyzw = _mm_setzero_ps();
	}
#endif // VECTOR3_ZERO

#ifndef VECTOR3_OPERATOR_BRACKET_C
#define VECTOR3_OPERATOR_BRACKET_C
	inline const float &Vector3::operator[](int i) const
	{
		return xyzw.m128_f32[i];
	}
#endif // VECTOR3_OPERATORBRACKETC

#ifndef VECTOR3_OPERATOR_BRACKET
#define VECTOR3_OPERATOR_BRACKET
	inline float &Vector3::operator[](int i)
	{
		return xyzw.m128_f32[i];
	}
#endif // VECTOR3_OPERATORBRACKET

#ifndef VECTOR3_ADD_V
#define VECTOR3_ADD_V
	inline void Vector3::Add(Vector3Param a)
	{
		xyzw = _mm_add_ps(xyzw, a.xyzw);
	}
#endif // VECTOR3_ADDV

#ifndef VECTOR3_ADD_V2
#define VECTOR3_ADD_V2
	inline void Vector3::Add(Vector3Param a, Vector3Param b)
	{
		xyzw = _mm_add_ps(b.xyzw, a.xyzw);
	}
#endif // VECTOR3_ADDV2

#ifndef VECTOR3_ADDSCALED
#define VECTOR3_ADDSCALED
	inline void Vector3::AddScaled(Vector3Param a,float s)
	{
		__m128 temp = _mm_set1_ps(s);
		xyzw = _mm_add_ps(xyzw, _mm_mul_ps(temp, a.xyzw));
	}
#endif // VECTOR3_ADDSCALED

#ifndef VECTOR3_ADDSCALED_2
#define VECTOR3_ADDSCALED_2
	inline void Vector3::AddScaled(Vector3Param a, Vector3Param b,float s)
	{
		__m128 temp = _mm_set1_ps(s);
		xyzw = _mm_add_ps(a.xyzw, _mm_mul_ps(temp, b.xyzw));
	}
#endif // VECTOR3_ADDSCALED2

#ifndef VECTOR3_SUBTRACT_V
#define VECTOR3_SUBTRACT_V
	inline void Vector3::Subtract(Vector3Param a)
	{
		xyzw = _mm_sub_ps(xyzw, a.xyzw);
	}
#endif // VECTOR3_SUBTRACTV

#ifndef VECTOR3_SUBTRACT_V2
#define VECTOR3_SUBTRACT_V2
	inline void Vector3::Subtract(Vector3Param a, Vector3Param b)
	{
		xyzw = _mm_sub_ps(a.xyzw, b.xyzw);
	}
#endif // VECTOR3_SUBTRACTV2

#ifndef VECTOR3_SUBTRACTSCALED
#define VECTOR3_SUBTRACTSCALED
	inline void Vector3::SubtractScaled(Vector3Param a,float s)
	{
		__m128 temp = _mm_set1_ps(s);
		xyzw = _mm_sub_ps(xyzw, _mm_mul_ps(temp, a.xyzw));
	}
#endif // VECTOR3_SUBTRACTSCALED

#ifndef VECTOR3_SUBTRACTSCALED_2
#define VECTOR3_SUBTRACTSCALED_2
	inline void Vector3::SubtractScaled(Vector3Param a, Vector3Param b,float s)
	{
		__m128 temp = _mm_set1_ps(s);
		xyzw = _mm_sub_ps(a.xyzw, _mm_mul_ps(temp, b.xyzw));
	}
#endif // VECTOR3_SUBTRACTSCALED2

#ifndef VECTOR3_SCALE_F
#define VECTOR3_SCALE_F
	inline void Vector3::Scale(float f)
	{
		__m128 temp = _mm_set1_ps(f);
		xyzw = _mm_mul_ps(xyzw, temp);
	}
#endif // VECTOR3_SCALEF

#ifndef VECTOR3_SCALE_VF
#define VECTOR3_SCALE_VF
	inline void Vector3::Scale(Vector3Param a,float f)
	{
		__m128 temp = _mm_set1_ps(f);
		xyzw = _mm_mul_ps(a.xyzw, temp);
	}
#endif // VECTOR3_SCALEVF

#ifndef VECTOR3_INVSCALE_F
#define VECTOR3_INVSCALE_F
	inline void Vector3::InvScale(float f)
	{
		__m128 temp = _mm_set1_ps(f);
		temp = NewtonRaphsonRecip(temp);
		xyzw = _mm_mul_ps(xyzw, temp);
	}
#endif // VECTOR3_INVSCALEF

#ifndef VECTOR3_INVSCALE_VF
#define VECTOR3_INVSCALE_VF
	inline void Vector3::InvScale(Vector3Param a,float f)
	{
		__m128 temp = _mm_set1_ps(f);
		temp = NewtonRaphsonRecip(temp);
		xyzw = _mm_mul_ps(a.xyzw, temp);
	}
#endif // VECTOR3_INVSCALEVF

#ifndef VECTOR3_MUL_V
#define VECTOR3_MUL_V
	inline void Vector3::Multiply(Vector3Param a)
	{
		xyzw = _mm_mul_ps(a.xyzw, xyzw);
	}
#endif // VECTOR3_MULV

#ifndef VECTOR3_MUL_V2
#define VECTOR3_MUL_V2
	inline void Vector3::Multiply(Vector3Param a, Vector3Param b)
	{
		xyzw = _mm_mul_ps(a.xyzw, b.xyzw);
	}
#endif // VECTOR3_MULV2

#ifndef VECTOR3_NEGATE
#define VECTOR3_NEGATE
	inline void Vector3::Negate()
	{
		xyzw = _mm_sub_ps(_mm_setzero_ps(), xyzw);
	}
#endif // VECTOR3_NEGATE

#ifndef VECTOR3_NEGATE_V
#define VECTOR3_NEGATE_V
	inline void Vector3::Negate(Vector3Param a)
	{
		xyzw = _mm_sub_ps(_mm_setzero_ps(), a.xyzw);
	}
#endif // VECTOR3_NEGATEV

#ifndef VECTOR3_INVERT
#define VECTOR3_INVERT
	inline void Vector3::Invert()
	{
		xyzw = NewtonRaphsonRecip(xyzw);
	}
#endif // VECTOR3_INVERT

#ifndef VECTOR3_INVERT_V
#define VECTOR3_INVERT_V
	inline void Vector3::Invert(Vector3Param a)
	{
		xyzw = NewtonRaphsonRecip(a.xyzw);
	}
#endif // VECTOR3_INVERTV

#ifndef VECTOR3_CROSS
#define VECTOR3_CROSS
	inline void Vector3::Cross(Vector3Param b)
	{
		__m128 l1, l2, m1, m2;
		l1 = _mm_shuffle_ps(xyzw, xyzw, _MM_SHUFFLE(3, 1, 0, 2));				// l1 = wyxz
		l2 = _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(3, 0, 2, 1));			// l2 = wxzy
		m2 = _mm_mul_ps(l1, l2);
		l1 = _mm_shuffle_ps(xyzw, xyzw, _MM_SHUFFLE(3, 0, 2, 1));				// l1 = wxzy
		l2 = _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(3, 1, 0, 2));			// l2 = wyxz
		m1 = _mm_mul_ps(l1, l2);
		xyzw = _mm_sub_ps(m1, m2);
	}
#endif // VECTOR3_CROSS

#ifndef VECTOR3_CROSS_2
#define VECTOR3_CROSS_2
	inline void Vector3::Cross(Vector3Param a, Vector3Param b)
	{
		__m128 A = a.xyzw;
		__m128 B = b.xyzw;
		_asm
		{
			movaps		xmm0, xmmword ptr[A]
			movaps		xmm1, xmmword ptr[B]
			movaps		xmm2, xmm0
			movaps		xmm3, xmm1
			shufps		xmm0, xmm0, _MM_SHUFFLE(3, 1, 0, 2)		// xmm0 = a.wyxz
			shufps		xmm1, xmm1, _MM_SHUFFLE(3, 0, 2, 1)		// xmm1 = b.wxzy
			mulps		xmm0, xmm1								// xmm0 = ~(a.y * b.x)(a.x * b.z)(a.z * b.y)
			shufps		xmm2, xmm2, _MM_SHUFFLE(3, 0, 2, 1)		// xmm2 = a.wxzy
			shufps		xmm3, xmm3, _MM_SHUFFLE(3, 1, 0, 2)		// xmm1 = b.wyxz
			mulps		xmm3, xmm2								// xmm3 = ~(a.x * b.y)(a.z * b.x)(a.y * b.z)
			subps		xmm3, xmm0
			movaps		xmmword ptr[A], xmm3;
		};
		xyzw = A;
	}
#endif // VECTOR3_CROSS2

#ifndef VECTOR3_CROSSSAFE
#define VECTOR3_CROSSSAFE
	inline void Vector3::CrossSafe(Vector3Param a, Vector3Param b)
	{
		__m128 l1, l2, m1, m2;
		l1 = _mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(3, 1, 0, 2));				// l1 = wyxz
		l2 = _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(3, 0, 2, 1));			// l2 = wxzy
		m2 = _mm_mul_ps(l1, l2);
		l1 = _mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(3, 0, 2, 1));				// l1 = wxzy
		l2 = _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(3, 1, 0, 2));			// l2 = wyxz
		m1 = _mm_mul_ps(l1, l2);
		xyzw = _mm_sub_ps(m1, m2);
		
	}
#endif // VECTOR3_CROSSAFE

#ifndef VECTOR3_CROSSNEGATE
#define VECTOR3_CROSSNEGATE
	inline void Vector3::CrossNegate(Vector3Param a)
	{
		Cross(a);
		Negate();
	}
#endif // VECTOR3_CROSSNEGATE

#ifndef VECTOR3_AVERAGE
#define VECTOR3_AVERAGE
	inline void Vector3::Average(Vector3Param a)
	{
		xyzw = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(xyzw, a.xyzw));
	}
#endif // VECTOR3_AVERAGE

#ifndef VECTOR3_AVERAGE_2
#define VECTOR3_AVERAGE_2
	inline void Vector3::Average(Vector3Param a, Vector3Param b)
	{
		xyzw = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(b.xyzw, a.xyzw));
	}
#endif // VECTOR3_AVERAGE2

#ifndef VECTOR3_ISEQUAL
#define VECTOR3_ISEQUAL
	inline bool Vector3::IsEqual(Vector3Param a) const
	{
		int nCmp = _mm_movemask_ps(_mm_cmpeq_ps(xyzw, a.xyzw));
		return ((nCmp & 0x7) == 0x7);
	}
#endif // VECTOR3_ISEQUAL

#ifndef VECTOR3_ISCLOSE
#define VECTOR3_ISCLOSE
	inline bool Vector3::IsClose(Vector3Param a,float eps) const
	{
		__m128 temp = _mm_set1_ps(eps);
		__m128 low = _mm_sub_ps(a.xyzw, temp);
		__m128 hi = _mm_add_ps(a.xyzw, temp);
		int nLow = _mm_movemask_ps(_mm_cmpge_ps(xyzw, low));
		int nHi = _mm_movemask_ps(_mm_cmple_ps(xyzw, hi));
		int nResult = (nLow & nHi) & 0x7;
		return ( nResult == 0x7 );
	}
#endif // VECTOR3_ISCLOSE

#ifndef VECTOR3_OPERATOR_PLUSEQUAL
#define VECTOR3_OPERATOR_PLUSEQUAL
	inline void Vector3::operator+=(Vector3Param V)
	{
		Add(V);
	}
#endif // VECTOR3_OPERATORPLUSEQUAL

#ifndef VECTOR3_OPERATOR_MINUSEQUAL
#define VECTOR3_OPERATOR_MINUSEQUAL
	inline void Vector3::operator-=(Vector3Param V)
	{
		Subtract(V);
	}
#endif // VECTOR3_OPERATORMINUSEQUAL

#ifndef VECTOR3_OPERATOR_TIMESEQUAL
#define VECTOR3_OPERATOR_TIMESEQUAL
	inline void Vector3::operator*=(const float f)
	{
		Scale(f);
	}
#endif // VECTOR3_OPERATORTIMESEQUAL

#ifndef VECTOR3_OPERATOR_DIVEQUAL
#define VECTOR3_OPERATOR_DIVEQUAL
	inline void Vector3::operator/=(const float f)
	{
		InvScale(f);
	}
#endif // VECTOR3_OPERATORDIVEQUAL

#endif // VECTORIZED

// Lame apology: Select defined outside of VECTORIZED because I get crashes right now if I turn on
// VECTORIZED, but this Select implementation gains me 3x speed improvements in the physics solver

#ifndef VECTOR3_SELECT
#define VECTOR3_SELECT
    inline Vector3 Vector3::Select(Vector3Param zero, Vector3Param nonZero) const
    {
#if !__OPTIMIZED
        FastAssert(((size_t)this&15)==0);
#endif

        __m128 zeroPart = _mm_andnot_ps(xyzw, zero.xyzw);
        __m128 nonZeroPart = _mm_and_ps(xyzw, nonZero.xyzw);

        return Vector3(_mm_or_ps(zeroPart, nonZeroPart));
    }
#endif

}	// namespace rage

#endif // VECTOR3_WIN32_INL
