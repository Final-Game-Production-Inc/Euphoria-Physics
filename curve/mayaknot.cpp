// 
// curve/mayaknot.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "mayaknot.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "file/token.h"

using namespace rage;

IMPLEMENT_PLACE(mayaKnotVector);

// PURPOSE: Reconstruct the knot vector to ensure integer spacing
// PARAMS: 'cvCount', 'degree' and 'editPointCount' from the curve
// NOTES: You MUST call ComputeParametricRange() on your curve after calling this
void mayaKnotVector::Normalize(int cvCount, int degree, int editPointCount) const
{
	int length = cvCount + degree + 1;
	Assert(length==m_Length);

	bool multiple = GetKnot(0) == GetKnot(1);

	int k;
	int j=0;

	if (multiple)
	{
		k=0;
		for (int i=0; i<degree; i++)
			m_pKnots[j++]=(float)k;

		for (int i=0; i<editPointCount; i++)
			m_pKnots[j++] = (float)k++;

		k--;
		for (int i=0; i<degree; i++)
			m_pKnots[j++]=(float)k;
	}
	else
	{
		k=-degree;
		for (int i=0; i<length; i++)
			m_pKnots[j++]=(float)k++;
	}
	Assert(j==m_Length);
}


void mayaKnotVector::Init(const float *kv, int kvCount, int n, int order)
{
	// N = CVs - 1
	// Order = degree + 1

	Assert(kv && kvCount > 0);

	m_Length = (n + order + 1);
	Assert(m_Length == kvCount + 2);

	Assert(!m_pKnots);
	m_pKnots = rage_new float[m_Length];

	// Copy the knot vector.
	int i;
	for (i = 0; i < kvCount; i++)
		m_pKnots[i + 1] = kv[i];

	// Fix the parametric range to start at zero.
	int degree = order-1;
	float offset = -m_pKnots[degree];
	for (i = 0; i < kvCount; i++)
		m_pKnots[i + 1] += offset;

	m_pKnots[degree] = 0.0f;  // Not necessary?

	// Invent the first and last entries.
	if (m_Length>4)
	{
		// (0,0,1,2,3,3) becomes ( 0,0,0,1,2,3,3,3) (multiple end knots)
		// (0,1,2,3,4,5) becomes (-1,0,1,2,3,4,5,6) (no multiple end knots)
		m_pKnots[0] = m_pKnots[1] - (m_pKnots[2] - m_pKnots[1]);
		m_pKnots[m_Length-1] = m_pKnots[m_Length-2] + (m_pKnots[m_Length-2] - m_pKnots[m_Length-3]);
	}
	else
	if (m_Length==4)
	{
		// (0,1) becomes (0,0,1,1) not (-1,0,1,2)
		m_pKnots[0] = m_pKnots[1];
		m_pKnots[3] = m_pKnots[2];
	}
}

void mayaKnotVector::Shutdown()
{
	delete[] m_pKnots;
	m_pKnots = NULL;
	m_Length = 0;
}

mayaKnotVector::~mayaKnotVector()
{
	delete[] m_pKnots;
	m_pKnots = NULL;
	m_Length = 0;
}

void mayaKnotVector::Save(fiTokenizer &tok) const
{
	tok.Put("KNOTVECTOR ");
	tok.Put(m_Length);
	for (int i = 0; i < m_Length; i++)
		tok.Put(m_pKnots[i]);
}

void mayaKnotVector::Load(fiTokenizer &tok)
{
	m_Length = tok.MatchIInt("KNOTVECTOR");
	Assert(!m_pKnots);
	m_pKnots = NULL;

	if (m_Length>0)
		m_pKnots = rage_new float[m_Length];

	for (int i = 0; i < m_Length; i++)
		m_pKnots[i] = tok.GetFloat();
}

mayaKnotVector::mayaKnotVector(const mayaKnotVector &k)
{
	m_Length = k.m_Length;
	m_pKnots = m_Length>0 ? rage_new float[m_Length] : NULL;
	for (int i=0; i<m_Length; i++)
		m_pKnots[i] = k.m_pKnots[i];
}

const mayaKnotVector &mayaKnotVector::operator=(const mayaKnotVector &k)
{
	Assert(!m_pKnots);
	m_Length = k.m_Length;
	m_pKnots = m_Length>0 ? rage_new float[m_Length] : NULL;
	for (int i=0; i<m_Length; i++)
		m_pKnots[i] = k.m_pKnots[i];
	return *this;
}

bool mayaKnotVector::operator==(const mayaKnotVector &k) const
{
	if (m_Length != k.m_Length)
		return false;

	for (int i=0; i<k.m_Length; i++)
		if (m_pKnots[i] != k.m_pKnots[i])
			return false;

	return true;
}

bool mayaKnotVector::operator!=(const mayaKnotVector &c) const
{
	return !operator==(c);
}

bool mayaKnotVector::operator<(const mayaKnotVector &k) const
{
	if (m_Length != k.m_Length)
		return m_Length < k.m_Length;

	for (int i=0; i<k.m_Length; i++)
		if (m_pKnots[i] != k.m_pKnots[i])
			return m_pKnots[i] < k.m_pKnots[i];

	return false;
}

mayaKnotVector::mayaKnotVector(datResource &rsc)
{
	rsc.PointerFixup(m_pKnots);
}

#if	__DECLARESTRUCT
void mayaKnotVector::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(mayaKnotVector);
	STRUCT_DYNAMIC_ARRAY(m_pKnots, m_Length);
	STRUCT_FIELD(m_Length);
	STRUCT_END();
}
#endif

#endif // ENABLE_UNUSED_CURVE_CODE
