// 
// vector/vectorn.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTORN_H
#define VECTOR_VECTORN_H

#include "vector4.h"

#include "atl/array.h"
#include "data/resource.h"
#include "data/serialize.h"

#define IMPLEMENT_VECTORN_WITH_VECTOR4

namespace rage {

//=============================================================================
// VectorN
//
// PURPOSE: A templated class storing N floats where N is known at compile time.
//
// <FLAG Component>
//

template<int N> class VectorN
{
public:
	VectorN();
	VectorN(datResource&);

	IMPLEMENT_PLACE_INLINE(VectorN);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct&);
#endif // __DECLARESTRUCT

	const float & operator[](int i) const;

	float & operator[](int i);

	void Zero();

	void Scale(float s);
	void Scale(const VectorN<N> & v, float s);

	VectorN<N>& operator +=(const VectorN<N> & v);
	VectorN<N>& operator -=(const VectorN<N> & v);
	VectorN<N>& operator /=(float f);

	void AddScaled(const VectorN<N> & v, float s);
	void AddScaled(const VectorN<N>& a, const VectorN<N>& b, float s);
	void AddScaled(const VectorN<N>& a, const VectorN<N>& b, Vector4::Param s);
	void Add(const VectorN<N> & v);
	void Subtract(const VectorN<N> & v);

	void Multiply(const VectorN<N>& v, Vector4::Param s);
	void Multiply(Vector4::Param s);

	float Mag() const;

	float Mag2() const;

	float Dist2(const VectorN<N> & v) const;

	//PURPOSE: set the vector to a normalized default vector (1,0,0,0,0,.....)
	void SetNormalizedDefault();

	void Normalize();

	void Normalize(const VectorN<N> & v);

	int GetDimension() const;

	void Print(const char *label, bool newline = true) const;

#if __ASSERT
	void AssertRange(int i) const;
#endif

	int GetSize() const { return N; }

	Vector4 Get4(int i) const {
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		return m_Vector[i];
#else
		int index = i * 4;
		return Vector4(
			index < N		? (*this)[index]	: 0.0f,
			(index+1) < N	? (*this)[index+1]	: 0.0f,
			(index+2) < N	? (*this)[index+2]	: 0.0f,
			(index+3) < N	? (*this)[index+3]	: 0.0f
		);
#endif
	}

	void Set4(int i, Vector4::Param vec)
	{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		m_Vector[i] = vec;
#else
		int index = i * 4;
		m_Vector[index] = vec.x;
		m_Vector[index+1] = vec.y;
		m_Vector[index+2] = vec.z;
		m_Vector[index+3] = vec.w;
#endif
	}

protected:
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
	atRangeArray<Vector4, ((N-1)/4)+1> m_Vector;
#else
	atRangeArray<float,N> m_Vector;
#endif 
};

#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
#define FOR_EACH_ELEMENT for(int i=0; i < (((N-1)/4)+1); i++)
#else
#define FOR_EACH_ELEMENT for(int i=0; i<N; i++)
#endif


//=============================================================================
// Implementations

// PURPOSE: Serialize a VectorN object
template<int N> datSerialize & operator<< (datSerialize& archive, VectorN<N>& v)
{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4

	if (archive.IsRead())
	{
		v.Zero();
	}

	for(int i=0; i<N; i++)
	{	
		archive << v[i];
	}
#else
	FOR_EACH_ELEMENT
	{
		archive << v[i];
	}
#endif
	return archive;
}


#if __ASSERT
template<int N> void VectorN<N>::AssertRange(int i) const
{
	mthAssertf(i>=0 && i<N, "Out of range for vector - requested %d, size %d",i,N);
}
#endif

template<int N> VectorN<N>::VectorN()
{
#if 1
	Zero();
#endif
}


template<int N> int VectorN<N>::GetDimension() const
{
	return N;
}


template<int N> const float & VectorN<N>::operator[](int i) const
{
	ASSERT_ONLY(AssertRange(i);)
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
	return m_Vector[i/4][i%4];
#else
	return m_Vector[i];
#endif
}


template<int N> float & VectorN<N>::operator[](int i)
{
	ASSERT_ONLY(AssertRange(i);)
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
	return m_Vector[i/4][i%4];
#else
	return m_Vector[i];
#endif
}


template<int N> void VectorN<N>::Zero()
{
	FOR_EACH_ELEMENT
	{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		m_Vector[i].Zero();
#else
		m_Vector[i] = 0.0f;
#endif
	}
}


template<int N> float VectorN<N>::Mag2() const
{
	float a = 0.0f;
	FOR_EACH_ELEMENT
	{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		a += m_Vector[i].Mag2();
#else
		a += square(m_Vector[i]);
#endif
	}
	return a;
}


template<int N> float VectorN<N>::Mag() const
{
	return sqrtf(Mag2());
}

template<int N> void VectorN<N>::SetNormalizedDefault()
{
	Zero();

#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
	m_Vector[0][0] = 1.0f;
#else
	m_Vector[0] = 1.0f;
#endif
}

template<int N> void VectorN<N>::Normalize()
{
	float a = Mag2();
	if(a<=SMALL_FLOAT)
	{
		SetNormalizedDefault();
		return;
	}

	a = 1.0f/sqrtf(a);
	FOR_EACH_ELEMENT
	{
		m_Vector[i] *= a;
	}
}


template<int N> void VectorN<N>::Normalize(const VectorN<N> & v)
{
	float a = v.Mag2();
	if(a<=SMALL_FLOAT)
	{
		SetNormalizedDefault();
		return;
	}

	a = 1.0f/sqrtf(a);
	FOR_EACH_ELEMENT
	{
		m_Vector[i] = v.m_Vector[i] * a;
	}
}


template<int N> float VectorN<N>::Dist2(const VectorN<N> & v) const
{
	float a = 0.0f;
	FOR_EACH_ELEMENT
	{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		a += m_Vector[i].Dist2(v[i]);
#else
		a += square(m_Vector[i]-v[i]);
#endif
	}
	return a;
}


template<int N> void VectorN<N>::Scale(const VectorN<N> & v, float s)
{
	FOR_EACH_ELEMENT
	{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		m_Vector[i].SetScaled(v.m_Vector[i], s);
#else
		m_Vector[i] = v[i] * s;
#endif
	}
}


template<int N> void VectorN<N>::Scale(float s)
{
	Scale(*this, s);
}


template<int N> void VectorN<N>::AddScaled(const VectorN<N> & v, float s)
{
	FOR_EACH_ELEMENT
	{
#if __XENON && defined(IMPLEMENT_VECTORN_WITH_VECTOR4)
		__vector4 ssss;
		ssss.x = s;
		ssss = __vspltw(ssss, 0);											// temp = ssss
		m_Vector[i].xyzw = __vmaddfp(v.m_Vector[i].xyzw, ssss, m_Vector[i].xyzw);	// xyzw = (v * ssss) + xyzw
#else
		m_Vector[i] += v.m_Vector[i] * s;
#endif
	}
}

template<int N> void VectorN<N>::AddScaled(const VectorN<N> &a, const VectorN<N>& b, float s)
{
	FOR_EACH_ELEMENT
	{
#if __XENON && defined(IMPLEMENT_VECTORN_WITH_VECTOR4)
		__vector4 ssss;
		ssss.x = s;
		ssss = __vspltw(ssss, 0);
		m_Vector[i].xyzw = __vmaddfp(b.m_Vector[i].xyzw, ssss, a.m_Vector[i].xyzw);
#else
		m_Vector[i] = a.m_Vector[i] + b.m_Vector[i] * s;
#endif
	}
}

template<int N> void VectorN<N>::AddScaled(const VectorN<N> &a, const VectorN<N>& b, Vector4::Param s)
{
	FOR_EACH_ELEMENT
	{
#if defined(IMPLEMENT_VECTORN_WITH_VECTOR4)
		m_Vector[i].AddScaled(a.m_Vector[i], b.m_Vector[i], s);
#else
		m_Vector[i] = a.m_Vector[i] + (b.m_Vector[i] * s[i%4]);
#endif
	}
}


template<int N> void VectorN<N>::Multiply(const VectorN<N> &a, Vector4::Param s)
{
	FOR_EACH_ELEMENT
	{
#if defined(IMPLEMENT_VECTORN_WITH_VECTOR4)
		m_Vector[i].Multiply(a.m_Vector[i], s);
#else
		m_Vector[i] = a[i] * s[i%4];
#endif
	}
}

template<int N> void VectorN<N>::Multiply(Vector4::Param s)
{
	FOR_EACH_ELEMENT
	{
#if defined(IMPLEMENT_VECTORN_WITH_VECTOR4)
		m_Vector[i].Multiply(s);
#else
		m_Vector[i] *= s[i%4];
#endif
	}
}


template<int N> 
void VectorN<N>::Add(const VectorN<N> & v)
{
	FOR_EACH_ELEMENT
	{
		m_Vector[i] += v.m_Vector[i];
	}
}


template<int N> 
void VectorN<N>::Subtract(const VectorN<N> & v)
{
	FOR_EACH_ELEMENT
	{
		m_Vector[i] -= v.m_Vector[i];
	}
}

template<int N> VectorN<N>::VectorN(datResource&)
{
}

#if __DECLARESTRUCT
template<int N> void VectorN<N>::DeclareStruct(datTypeStruct& s)
{
	STRUCT_BEGIN(VectorN<N>)
	STRUCT_CONTAINED_ARRAY(m_Vector);
	STRUCT_END();
}
#endif // __DECLARESTRUCT


template<int N> 
VectorN<N> operator* (const VectorN<N>& v, float f)
{
	VectorN<N> result(v);
	result.Scale(f);
	return result;
}

template<int N> 
VectorN<N>& VectorN<N>::operator +=(const VectorN<N> & v)
{
	Add(v);
	return *this;
}

template<int N> 
VectorN<N>& VectorN<N>::operator -=(const VectorN<N> & v)
{
	Subtract(v);
	return *this;
}

template<int N> 
VectorN<N>& VectorN<N>::operator /=(float f)
{
	Scale(1.0f / f);
	return *this;
}


template<int N> 
VectorN<N> operator +(const VectorN<N> & a, const VectorN<N> & b)
{
	VectorN<N> result(a);
	return (result += b);
}

template<int N> 
VectorN<N> operator -(const VectorN<N> & a, const VectorN<N> & b)
{
	VectorN<N> result(a);
	return (result -= b);
}


template<int N> 
void VectorN<N>::Print(const char *label, bool newline /*= true*/) const
{
	Printf("%s: ", label);
	FOR_EACH_ELEMENT
	{
#ifdef IMPLEMENT_VECTORN_WITH_VECTOR4
		m_Vector[i].Print(false);
#else
		Printf("%f,", m_Vector[i]);
#endif
	}

	if( newline )
		Printf("\n");

}



}	// namespace rage

#endif // VECTOR_VECTORN_H
