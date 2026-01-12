// 
// curve/curvenurbs.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_CURVENURBS_H
#define CURVE_CURVENURBS_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_CURVE_CODE

// #include "curve.h" Do not include the curve.h this has to be included before we get here.
// This is done for PS3 to compile
#ifndef CURVE_CURVE_H
#error "Do not include curvenurbs.h directly, include curve.h instead."
#endif

#include "data/resourcehelpers.h"
#include "vector/vectort.h"
#include "vector/matrixt.h"
#include "mathext/linearalgebra.h"

namespace rage {

class datResource;


// Helper classes for the curves to resource easier
class CurveNurbsKnot : public datBase
{
public:
	CurveNurbsKnot() {}
	virtual ~CurveNurbsKnot() {}
	CurveNurbsKnot(datResource & ) {}
	DECLARE_PLACE(CurveNurbsKnot);

#if	__DECLARESTRUCT
	void				DeclareStruct	(datTypeStruct &s);
#endif

	unsigned int Kv;				// knot vector
	float Kt;						// knot vector t values
};

// Some precalculated constants to avoid divisions every time we want to evaluate a curve.
class CurveNurbsBlendFunctionConst : public datBase
{
public:
	CurveNurbsBlendFunctionConst() {}
	virtual ~CurveNurbsBlendFunctionConst() {}
	CurveNurbsBlendFunctionConst(datResource & ) {}

	DECLARE_PLACE(CurveNurbsBlendFunctionConst);

#if	__DECLARESTRUCT
	void				DeclareStruct	(datTypeStruct &s);
#endif

	float fFuncConst1;
	float fFuncConst2a;
	float fFuncConst2b;
	float fFuncConst2c;
	float fFuncConst3a;
	float fFuncConst3b;
	float fFuncConst3c;
	float fFuncConst4;
};

//=============================================================================
// cvCurveNurbs
//
// PURPOSE
//   cvCurveNurbs defines a type of spline curve, and is short for Nonuniform Rational B-Spline.
//   It is derived from cvCurve, which provides an interface for spline curves.
// <FLAG Component>
//
template <class _Vector> class cvCurveNurbs : public cvCurve<_Vector>
{
public:
	cvCurveNurbs();

    cvCurveNurbs(datResource &rsc);
	
	~cvCurveNurbs();

	DECLARE_PLACE	(cvCurveNurbs);

#if	__DECLARESTRUCT
	void				DeclareStruct	(datTypeStruct &s);
#endif
	
	//=========================================================================
	// IO

	virtual void Serialize(datSerialize& archive);

	//=========================================================================
	// Evaluation

	virtual float SolveSegment(_Vector& posOut, int seg, float t, float slope=0.0f, _Vector* direction=0);

	virtual void SolveSegmentSimple (_Vector& posOut, int nSeg, float fT, _Vector *pvecTangent, _Vector *pvecUnitTangent);

	virtual void SolveSegmentSimpleBatch(int nNumCurves, cvCurve<_Vector> **papCurves, _Vector *pavecPos, int nSeg, float fT,
		_Vector **papvecTangents);

	virtual void Move(int &seg, float &t, float dist, float slope=0.0f) const;

	//[CLEMENS] special case function for MF
	void SolveSegmentForOneDimension(float& pos, int dim, float t)
	{ pos = SolveSegmentForOneDimension(dim, t); }

	float SolveSegmentForOneDimension(int dim, float t);

	// PURPOSE: Solves for the t value that evaluates to u for the curve in dimension dim.
	float SolveInverseOneDimension(int dim, float u);

	// PURPOSE: Fit an exact curve to the given verticies
	// NOTE: This is extremely slow and should only be used offline.
	void InterpolateGlobal();

protected:
	// Called by SolveSegment:
	float GetBlendFunction(unsigned int i, float t) const;
	float GetBlendFunction1(unsigned int i, float t, float* slope=0) const;
	float GetBlendFunction2(unsigned int i, float t, float* slope=0) const;
	float GetBlendFunction3(unsigned int i, float t, float* slope=0) const;
	float GetBlendFunction4(unsigned int i, float t, float* slope=0) const;

	atArray <CurveNurbsKnot> m_Knots;
	atArray <CurveNurbsBlendFunctionConst> m_BlendConsts;

	float m_Blend[4];
	float m_SlopeBlend[4];	// saved for repeated curve solutions

private:
    virtual void PostInitVerts();

    // Call after initializing the verts:
	// Equivalent to "Multiple End Knots" in Maya
	void SetClampedKnots();
};


//=============================================================================
// Implementations

template <class _Vector>
cvCurveNurbs<_Vector>::cvCurveNurbs()
: cvCurve<_Vector>()
{
	cvCurve<_Vector>::m_Type = CURVE_TYPE_NURBS;
}


template <class _Vector>
cvCurveNurbs<_Vector>::~cvCurveNurbs()
{
	m_Knots.Reset();
	m_BlendConsts.Reset();
}

template <class _Vector>
void cvCurveNurbs<_Vector>::Serialize(datSerialize& archive)
{
	cvCurve<_Vector>::Serialize(archive);

	int nKnotCount = m_Knots.GetCount();
	archive << nKnotCount;

	for (int i=0; i<m_Knots.GetCount(); i++)
	{
		archive << m_Knots[i].Kv;
		archive << m_Knots[i].Kt;
	}

	int nBlendConstCount = m_BlendConsts.GetCount();
	archive << nBlendConstCount;

	for (int i=0; i<m_BlendConsts.GetCount(); i++)
	{
		archive << m_BlendConsts[i].fFuncConst1 << m_BlendConsts[i].fFuncConst1 << m_BlendConsts[i].fFuncConst2a << m_BlendConsts[i].fFuncConst2b << m_BlendConsts[i].fFuncConst2c << m_BlendConsts[i].fFuncConst3a << m_BlendConsts[i].fFuncConst3b << m_BlendConsts[i].fFuncConst3c << m_BlendConsts[i].fFuncConst4;
	}

	archive << m_Blend[0] << m_Blend[1] << m_Blend[2] << m_Blend[3];
	archive << m_SlopeBlend[0] << m_SlopeBlend[1] << m_SlopeBlend[2] << m_SlopeBlend[3];
	
}


template <class _Vector>
void cvCurveNurbs<_Vector>::PostInitVerts()
{
	SetClampedKnots();
}


template <class _Vector>
void cvCurveNurbs<_Vector>::InterpolateGlobal()
{
	// From http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/INT-APP/CURVE-INT-global.html

	short n = short(cvCurve<_Vector>::m_Vertices.GetCount());
	FastAssert(n == cvCurve<_Vector>::m_Vertices.GetCount());

	if (n == 0)
		return;

	MatrixT<float> N(n, n);

	float scale = (m_Knots.GetCount() - 7.f) / (n - 1);

	N[0][0] = 1.f;

	for (short i = 1; i < n; i++)
		N[0][i] = 0.f;

	for (short i = 1; i < n - 1; i++)
		for (int j = 0; j < n; j++)
			N[i][j] = GetBlendFunction(j,  scale * i);

	for (short i = 0; i < n - 1; i++)
		N[n - 1][i] = 0.f;

	N[n - 1][n - 1] = 1.f;

	VectorT<float> d(n,0);

	short s = short(cvCurve<_Vector>::m_Vertices[0].GetSize());
	FastAssert(s == cvCurve<_Vector>::m_Vertices[0].GetSize());

	for (int i = 0; i < s; i++)
	{
		for (short j = 0; j < n; j++)
			d[j] = cvCurve<_Vector>::m_Vertices[j][i];

		GaussJordanElimination(N,d);

		for (short j = 0; j < n; j++)
			cvCurve<_Vector>::m_Vertices[j][i] = d[j];
	}
}


template <class _Vector>
float cvCurveNurbs<_Vector>::SolveSegment(_Vector& pos, int UNUSED_PARAM(seg), float t, float slope, _Vector* direction)
{
	AssertMsg(m_Knots.GetCount() , "Knot vectors have not been computed.  Please call PostInitVerts before solving.");

	float T = t;
	float tScale = float(m_Knots.GetCount()-7);
	T *= tScale;		// convert T from [0,1] to curve segment and avoid ultimate value of T
	if (T < 0.0f)
	{
		T = 0.0f;
	}
	else if(T >= tScale)
	{
		T = tScale*(1.f-SMALL_FLOAT);
	}

	// find i
	int nodeIndex = 3;
	while (!((m_Knots[nodeIndex].Kt <= T) && (T < m_Knots[nodeIndex + 1].Kt)))
	{
		nodeIndex++;
	}

	// see Foley van Dam p.497 Eq(11.43)
	m_Blend[0] = GetBlendFunction1(nodeIndex-3,T,&m_SlopeBlend[0]);
	m_Blend[1] = GetBlendFunction2(nodeIndex-2,T,&m_SlopeBlend[1]);
	m_Blend[2] = GetBlendFunction3(nodeIndex-1,T,&m_SlopeBlend[2]);
	m_Blend[3] = GetBlendFunction4(nodeIndex,T,&m_SlopeBlend[3]);

	pos.Scale(cvCurve<_Vector>::m_Vertices[nodeIndex-3],m_Blend[0]);
	pos.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-2],m_Blend[1]);
	pos.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-1],m_Blend[2]);
	pos.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex],m_Blend[3]);

	if (direction)
	{
		direction->Scale(cvCurve<_Vector>::m_Vertices[nodeIndex-3],m_SlopeBlend[0]);
		direction->AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-2],m_SlopeBlend[1]);
		direction->AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-1],m_SlopeBlend[2]);
		direction->AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex],m_SlopeBlend[3]);
		float dirMag2 = direction->Mag2();
		if(dirMag2>SMALL_FLOAT)
		{
			direction->Normalize();
			//direction->InvScale(sqrtf(dirMag2));
		}
		else
		{
			direction->Zero();
			(*direction)[0] = 1.0f;
			//direction->Set(1.0f,0.0f,0.0f);
		}
	}

	// Find the slope, so that Move() can estimate the next t-value for constant-speed motion.
	_Vector slopeVec;
	slopeVec.Scale(cvCurve<_Vector>::m_Vertices[nodeIndex-3],m_SlopeBlend[0]);
	slopeVec.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-2],m_SlopeBlend[1]);
	slopeVec.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-1],m_SlopeBlend[2]);
	slopeVec.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex],m_SlopeBlend[3]);
	float newSlope = slopeVec.Mag()*tScale;
	if(slope>0.0f)
	{
		// Return a slope half way between this slope and the previous (input) slope.
		return 0.5f*(slope+newSlope);
	}
	return newSlope;
}



template <class _Vector>
float cvCurveNurbs<_Vector>::SolveSegmentForOneDimension(int dim, float t)
{
	FastAssert(m_Knots.GetCount() && "Knot vectors have not been computed.  Please call PostInitVerts before solving.");

	float T = t;
	float tScale = float(m_Knots.GetCount()-7);
	T *= tScale;		// convert T from [0,1] to curve segment and avoid ultimate value of T
	if (T < 0.0f)
	{
		T = 0.0f;
	}
	else if(T >= tScale)
	{
		T = tScale*(1.f-SMALL_FLOAT);
	}

	// find i
	int nodeIndex = 3;
	while (!((m_Knots[nodeIndex].Kt <= T) && (T < m_Knots[nodeIndex + 1].Kt)))
	{
		nodeIndex++;
	}

	// see Foley van Dam p.497 Eq(11.43)
	m_Blend[0] = GetBlendFunction1(nodeIndex-3,T,&m_SlopeBlend[0]);
	m_Blend[1] = GetBlendFunction2(nodeIndex-2,T,&m_SlopeBlend[1]);
	m_Blend[2] = GetBlendFunction3(nodeIndex-1,T,&m_SlopeBlend[2]);
	m_Blend[3] = GetBlendFunction4(nodeIndex,  T,&m_SlopeBlend[3]);

	float pos;
	pos =  cvCurve<_Vector>::m_Vertices[nodeIndex-3][dim] * m_Blend[0];
	pos += cvCurve<_Vector>::m_Vertices[nodeIndex-2][dim] * m_Blend[1];
	pos += cvCurve<_Vector>::m_Vertices[nodeIndex-1][dim] * m_Blend[2];
	pos += cvCurve<_Vector>::m_Vertices[nodeIndex-0][dim] * m_Blend[3];

	return pos;
}


template <class _Vector>
float cvCurveNurbs<_Vector>::SolveInverseOneDimension(int dim, float u)
{
	float t = 0.f; 

	float lastFrame = SolveSegmentForOneDimension(dim, 1.0f);

	while(u > lastFrame)
	{
		u -= lastFrame;
		t += 1.0f;
	}

	int numSamples = 13;  // 2^13 == 8192 (ie close enough to the 10K sample points this algorithm replaced)
	u32 b = 0;
	float tFraction = 0.5f;
	for(int i=0; i<numSamples; i++)
	{
		float frame = SolveSegmentForOneDimension(dim, tFraction * float(b|1));
		if(frame < u)
			b |= 1;
		b <<= 1;
		tFraction *= 0.5f;
	}

	float frameLow = SolveSegmentForOneDimension(dim, tFraction * float(b));
	float frameHigh = SolveSegmentForOneDimension(dim, tFraction * float(b|1));
	t += tFraction * (float(b) + ((u - frameLow) / (frameHigh - frameLow)));

	return t;
}


template <class _Vector>
void cvCurveNurbs<_Vector>::SolveSegmentSimple
(_Vector& posOut, int UNUSED_PARAM(seg), float t, _Vector* tangentOut, _Vector* unitTangentOut)
{
	Assertf(t>=0.0f && t<=1.0f, "t (%f) out of range [0,1]",t);

	// Convert T from [0,1] to curve segment and avoid ultimate value of T.
	float tScale = float(m_Knots.GetCount()-7);
	float tNew = Clamp(t*tScale,0.0f,tScale-1.0e-6f);

	// find i
	unsigned int nodeIndex = 3;
	while (m_Knots[nodeIndex].Kt > tNew || tNew >= m_Knots[nodeIndex + 1].Kt)
	{
		nodeIndex++;
	}

	// see Foley van Dam p.497 Eq(11.43)
	m_Blend[0] = GetBlendFunction1(nodeIndex - 3, tNew, &m_SlopeBlend[0]);
	m_Blend[1] = GetBlendFunction2(nodeIndex - 2, tNew, &m_SlopeBlend[1]);
	m_Blend[2] = GetBlendFunction3(nodeIndex - 1, tNew, &m_SlopeBlend[2]);
	m_Blend[3] = GetBlendFunction4(nodeIndex - 0, tNew, &m_SlopeBlend[3]);
	FastAssert(fabs(m_Blend[0] + m_Blend[1] + m_Blend[2] + m_Blend[3] - 1.0f) < 0.001f);

	posOut.Scale(cvCurve<_Vector>::m_Vertices[nodeIndex-3],m_Blend[0]);
	posOut.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-2],m_Blend[1]);
	posOut.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-1],m_Blend[2]);
	posOut.AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex],m_Blend[3]);

	// NOTE: This sure looks wrong.  TODO: Fix
	if((unitTangentOut != NULL) && (tangentOut == NULL))
	{
		tangentOut = unitTangentOut;
	}

	if(tangentOut != NULL)
	{
		tangentOut->Scale(cvCurve<_Vector>::m_Vertices[nodeIndex-3],m_SlopeBlend[0]);
		tangentOut->AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-2],m_SlopeBlend[1]);
		tangentOut->AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex-1],m_SlopeBlend[2]);
		tangentOut->AddScaled(cvCurve<_Vector>::m_Vertices[nodeIndex],m_SlopeBlend[3]);
		if(unitTangentOut != NULL)
		{
			float dirMag2 = unitTangentOut->Mag2();
			if(dirMag2>SMALL_FLOAT)
			{
				unitTangentOut->Normalize(*tangentOut);
			}
			else
			{
				unitTangentOut->Zero();
				(*unitTangentOut)[0] = 1.0f;
				//unitTangentOut->Set(1.0f,0.0f,0.0f);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////

//static float sfHeaderTime = 0.0f;
//static float sfGetBlendFunctionTime1 = 0.0f;
//static float sfGetBlendFunctionTime2 = 0.0f;
//static float sfCalculationTime = 0.0f;
//static int snFunctionCallCnt = 0;

template <class _Vector>
void cvCurveNurbs<_Vector>::SolveSegmentSimpleBatch
(int nNumCurves, cvCurve<_Vector> **papCurves, _Vector *pavecPos, 
 int UNUSED_PARAM(nSeg), float fT, _Vector **papvecTangents)
{
	//	++snFunctionCallCnt;
	//	Timer oTimer;
	//	oTimer.Reset();

	FastAssert(papCurves != NULL);
	FastAssert(papvecTangents != NULL);
	FastAssert(pavecPos != NULL);

	float fTNew = fT;
	float fTScale = float(m_Knots.GetCount()-7);
	fTNew *= fTScale;		// convert T from [0,1] to curve segment and avoid ultimate value of T
	if (fTNew < 0.0f)
		fTNew = 0.0f;

	// find i
	int nodeIndex = 3;
	while (!((m_Knots[nodeIndex].Kt <= fTNew) && (fTNew < m_Knots[nodeIndex + 1].Kt)))
		nodeIndex++;

	//	sfHeaderTime += oTimer.MsTime();
	//	oTimer.Reset();

	// see Foley van Dam p.497 Eq(11.43)
	m_Blend[0] = GetBlendFunction1(nodeIndex-3, fTNew, &m_SlopeBlend[0]);
	m_Blend[1] = GetBlendFunction2(nodeIndex-2, fTNew, &m_SlopeBlend[1]);

	//	sfGetBlendFunctionTime1 += oTimer.MsTime();
	//	oTimer.Reset();

	m_Blend[2] = GetBlendFunction3(nodeIndex-1, fTNew, &m_SlopeBlend[2]);
	m_Blend[3] = GetBlendFunction4(nodeIndex, fTNew, &m_SlopeBlend[3]);

	//	sfGetBlendFunctionTime2 += oTimer.MsTime();
	//	oTimer.Reset();

	const cvCurveNurbs* const*ppCurCurve = (const cvCurveNurbs* const*)(papCurves);

	for(int nCurveIdx = 0; nCurveIdx < nNumCurves; ++nCurveIdx, ++ppCurCurve)
	{
		//const cvCurveNurbs *pCurCurve = (const cvCurveNurbs *)(papCurves[nCurveIdx]);
		const cvCurveNurbs *pCurCurve = *ppCurCurve;

		pavecPos[nCurveIdx].Scale(pCurCurve->m_Vertices[nodeIndex-3], m_Blend[0]);
		pavecPos[nCurveIdx].AddScaled(pCurCurve->m_Vertices[nodeIndex-2], m_Blend[1]);
		pavecPos[nCurveIdx].AddScaled(pCurCurve->m_Vertices[nodeIndex-1], m_Blend[2]);
		pavecPos[nCurveIdx].AddScaled(pCurCurve->m_Vertices[nodeIndex], m_Blend[3]);

		if (papvecTangents[nCurveIdx] != NULL)
		{
			papvecTangents[nCurveIdx]->Scale(pCurCurve->m_Vertices[nodeIndex-3], m_SlopeBlend[0]);
			papvecTangents[nCurveIdx]->AddScaled(pCurCurve->m_Vertices[nodeIndex-2], m_SlopeBlend[1]);
			papvecTangents[nCurveIdx]->AddScaled(pCurCurve->m_Vertices[nodeIndex-1], m_SlopeBlend[2]);
			papvecTangents[nCurveIdx]->AddScaled(pCurCurve->m_Vertices[nodeIndex], m_SlopeBlend[3]);
		}
	}

	//	sfCalculationTime += oTimer.MsTime();
}


template <class _Vector>
void cvCurveNurbs<_Vector>::Move(int &, float &t, float dist, float slope) const
{
	if(slope>SMALL_FLOAT)
	{
		// Increment t from the slope (calculated in SolveSegment) for constant-speed motion.
		t+=dist/slope;
	}
	else
	{
		t+=0.01f;
	}

	if(t>1.0f)
	{
		t = cvCurve<_Vector>::m_Looping ? 0.0f : 1.0f;
	}
}


template <class _Vector>
float cvCurveNurbs<_Vector>::GetBlendFunction1(unsigned int i, float t, float* slope) const
{
	FastAssert(m_BlendConsts[i].fFuncConst1 != -1.0f);
	FastAssert(t >= m_Knots[i+3].Kt && t < m_Knots[i+4].Kt);

	float fT4MinusT = m_Knots[i + 4].Kt - t;
	float fOODenom = m_BlendConsts[i].fFuncConst1;
	if(slope != NULL)
	{
		*slope = -3.0f * square(fT4MinusT) * fOODenom;
	}
	return power3(fT4MinusT) * fOODenom;
}


template <class _Vector>
float cvCurveNurbs<_Vector>::GetBlendFunction2(unsigned int i, float t, float* slope) const
{
	FastAssert(m_BlendConsts[i - 1].fFuncConst2a != -1.0f);

	float t0 = m_Knots[i].Kt;
	float t1 = m_Knots[i+1].Kt;
	float t2 = m_Knots[i+2].Kt;
	float t3 = m_Knots[i+3].Kt;
	float t4 = m_Knots[i+4].Kt;
	FastAssert(t>=t2 && t<t3);
	float t3MinusT = t3-t;
	float t4MinusT = t4-t;
	//	float t3MinusT1 = t3-t1;
	//	float t3MinusT2 = t3-t2;
	//	float t4MinusT1 = t4-t1;
	float tMinusT0 = t-t0;
	float tMinusT1 = t-t1;
	float tMinusT2 = t-t2;
	//	float denom = (t3-t0)*t3MinusT1*t3MinusT2;
	float blend;

	/*	if(denom>SMALL_FLOAT)
	{
	denom=1.0f/denom;
	FastAssert(denom == m_BlendConsts[i - 1].fFuncConst2a);
	blend=tMinusT0*square(t3MinusT)*denom;
	if(slope)
	{
	*slope=(square(t3MinusT)-2.0f*tMinusT0*t3MinusT)*denom;
	}
	}
	else
	{
	blend=0.0f;
	if(slope)
	{
	*slope=0.0f;
	}
	}*/
	blend = tMinusT0 * square(t3MinusT) * m_BlendConsts[i - 1].fFuncConst2a;
	if(slope != NULL)
	{
		*slope = (square(t3MinusT) - 2.0f * tMinusT0 * t3MinusT) * m_BlendConsts[i - 1].fFuncConst2a;
	}
	/*	denom=t4MinusT1*t3MinusT2*t3MinusT1;
	if(denom>SMALL_FLOAT)
	{
	denom=1.0f/denom;
	FastAssert(denom == m_BlendConsts[i - 1].fFuncConst2b);
	blend+=t4MinusT*tMinusT1*t3MinusT*denom;
	if(slope)
	{
	*slope+=(-tMinusT1*t3MinusT+t4MinusT*t3MinusT-t4MinusT*tMinusT1)*denom;
	}
	}*/
	blend += t4MinusT * tMinusT1 * t3MinusT * m_BlendConsts[i - 1].fFuncConst2b;
	if(slope != NULL)
	{
		*slope += (-tMinusT1 * t3MinusT + t4MinusT * t3MinusT - t4MinusT * tMinusT1) * m_BlendConsts[i - 1].fFuncConst2b;
	}
	/*	denom=(t4-t1)*t3MinusT2*(t4-t2);
	if(denom>SMALL_FLOAT)
	{
	denom=1.0f/denom;
	FastAssert(denom == m_BlendConsts[i - 1].fFuncConst2c);
	blend+=square(t4MinusT)*tMinusT2*denom;
	if(slope)
	{
	*slope+=(-2.0f*t4MinusT*tMinusT2+square(t4MinusT))*denom;
	}
	}*/
	blend += square(t4MinusT) * tMinusT2 * m_BlendConsts[i - 1].fFuncConst2c;
	if(slope != NULL)
	{
		*slope += (-2.0f * t4MinusT * tMinusT2 + square(t4MinusT)) * m_BlendConsts[i - 1].fFuncConst2c;
	}
	return blend;
}


template <class _Vector>
float cvCurveNurbs<_Vector>::GetBlendFunction3(unsigned int i, float t, float* slope) const
{
	float t0 = m_Knots[i].Kt;
	float t1 = m_Knots[i+1].Kt;
	float t2 = m_Knots[i+2].Kt;
	float t3 = m_Knots[i+3].Kt;
	float t4 = m_Knots[i+4].Kt;
	FastAssert(t>=t1 && t<t2);
	float tMinusT0 = t-t0;
	float tMinusT1 = t-t1;
	//	float t2MinusT1 = t2-t1;
	//	float t3MinusT0 = t3-t0;
	//	float t3MinusT1 = t3-t1;
	float t2MinusT = t2-t;
	float t3MinusT = t3-t;
	float t4MinusT = t4-t;
	//	float denom = (t4-t1)*t3MinusT1*t2MinusT1;
	float blend;

	/*	if(denom>SMALL_FLOAT)
	{
	denom=1.0f/denom;
	FastAssert(denom == m_BlendConsts[i - 2].fFuncConst3a);
	blend=t4MinusT*square(tMinusT1)*denom;
	if(slope)
	{
	*slope=(-square(tMinusT1)+2.0f*t4MinusT*tMinusT1)*denom;
	}
	}
	else
	{
	blend=0.0f;
	if(slope)
	{
	*slope=0.0f;
	}
	}*/
	const float kfOODenom1 = m_BlendConsts[i - 2].fFuncConst3a;
	blend = t4MinusT * square(tMinusT1) * kfOODenom1;
	if(slope != NULL)
	{
		*slope = (-square(tMinusT1) + 2.0f * t4MinusT * tMinusT1) * kfOODenom1;
	}
	/*	denom=t3MinusT0*t2MinusT1*(t2-t0);
	if(denom>SMALL_FLOAT)
	{
	denom=1.0f/denom;
	FastAssert(denom == m_BlendConsts[i - 2].fFuncConst3b);
	blend+=square(tMinusT0)*t2MinusT*denom;
	if(slope)
	{
	*slope+=(2.0f*tMinusT0*t2MinusT-square(tMinusT0))*denom;
	}
	}*/
	const float kfOODenom2 = m_BlendConsts[i - 2].fFuncConst3b;
	blend += square(tMinusT0) * t2MinusT * kfOODenom2;
	if(slope != NULL)
	{
		*slope += (2.0f * tMinusT0 * t2MinusT - square(tMinusT0)) * kfOODenom2;
	}
	/*	denom=t3MinusT0*t2MinusT1*t3MinusT1;
	if(denom>SMALL_FLOAT)
	{
	denom=1.0f/denom;
	FastAssert(denom == m_BlendConsts[i - 2].fFuncConst3c);
	blend+=tMinusT0*t3MinusT*tMinusT1*denom;
	if(slope)
	{
	*slope+=(t3MinusT*tMinusT1-tMinusT0*tMinusT1+tMinusT0*t3MinusT)*denom;
	}
	}*/
	const float kfOODenom3 = m_BlendConsts[i - 2].fFuncConst3c;
	blend += tMinusT0 * t3MinusT * tMinusT1 * kfOODenom3;
	if(slope != NULL)
	{
		*slope += (t3MinusT * tMinusT1 - tMinusT0 * tMinusT1 + tMinusT0 * t3MinusT) * kfOODenom3;
	}

	return blend;
}


template <class _Vector>
float cvCurveNurbs<_Vector>::GetBlendFunction4(unsigned int i, float t, float* slope) const
{
	FastAssert(m_BlendConsts[i - 3].fFuncConst4 != -1.0f);
	FastAssert(t >= m_Knots[i].Kt && t < m_Knots[i + 1].Kt);

	float fTMinusT1 = t - m_Knots[i].Kt;
	const float fOODenom = m_BlendConsts[i - 3].fFuncConst4;
	if(slope != NULL)
	{
		*slope = 3.0f * square(fTMinusT1) * fOODenom;
	}
	return power3(fTMinusT1) * fOODenom;
}


template <class _Vector>
float cvCurveNurbs<_Vector>::GetBlendFunction(unsigned int i, float t) const
{
	// see Foley van Dam p.497 Eq(11.44)

	if ((int)(i + 4) > (m_Knots.GetCount() - 1))
	{
		Quitf("i = %d, Nkv = %d", i, m_Knots.GetCount());
	}

	float B01, B11, B21, B31;
	float B02, B12, B22;
	float B03, B13;
	float B04;
	float d1, d2;

	B01 = ((m_Knots[i + 0].Kt <= t) && (t < m_Knots[i + 1].Kt)) ? 1.0f : 0.0f;
	B11 = ((m_Knots[i + 1].Kt <= t) && (t < m_Knots[i + 2].Kt)) ? 1.0f : 0.0f;
	B21 = ((m_Knots[i + 2].Kt <= t) && (t < m_Knots[i + 3].Kt)) ? 1.0f : 0.0f;
	B31 = ((m_Knots[i + 3].Kt <= t) && (t < m_Knots[i + 4].Kt)) ? 1.0f : 0.0f;

	d1 = (m_Knots[i + 3].Kt - m_Knots[i + 2].Kt);
	d2 = (m_Knots[i + 4].Kt - m_Knots[i + 3].Kt);

	if ((d1 != 0.0f) && (d2 != 0.0f))
	{
		B22 = (((t - m_Knots[i + 2].Kt) / d1) * B21) + (((m_Knots[i + 4].Kt - t) / d2) * B31);
	}
	else if (d1 != 0.0f)
	{
		B22 = (((t - m_Knots[i + 2].Kt) / d1) * B21);
	}
	else if (d2 != 0.0f)
	{
		B22 = (((m_Knots[i + 4].Kt - t) / d2) * B31);
	}
	else
	{
		B22 = 0.0f;
	}

	d1 = (m_Knots[i + 2].Kt - m_Knots[i + 1].Kt);
	d2 = (m_Knots[i + 3].Kt - m_Knots[i + 2].Kt);

	if ((d1 != 0.0f) && (d2 != 0.0f))
	{
		B12 = (((t - m_Knots[i + 1].Kt) / d1) * B11) + (((m_Knots[i + 3].Kt - t) / d2) * B21);
	}
	else if (d1 != 0.0f)
	{
		B12 = (((t - m_Knots[i + 1].Kt) / d1) * B11);
	}
	else if (d2 != 0.0f)
	{
		B12 = (((m_Knots[i + 3].Kt - t) / d2) * B21);
	}
	else
	{
		B12 = 0.0f;
	}

	d1 = (m_Knots[i + 3].Kt - m_Knots[i + 1].Kt);
	d2 = (m_Knots[i + 4].Kt - m_Knots[i + 2].Kt);

	if ((d1 != 0.0f) && (d2 != 0.0f))
		B13 = (((t - m_Knots[i + 1].Kt) / d1) * B12) + (((m_Knots[i + 4].Kt - t) / d2) * B22);
	else
		if (d1 != 0.0f)
			B13 = (((t - m_Knots[i + 1].Kt) / d1) * B12);
		else
			if (d2 != 0.0f)
				B13 = (((m_Knots[i + 4].Kt - t) / d2) * B22);
			else
				B13 = 0.0f;


	d1 = (m_Knots[i + 1].Kt - m_Knots[i + 0].Kt);
	d2 = (m_Knots[i + 2].Kt - m_Knots[i + 1].Kt);

	if ((d1 != 0.0f) && (d2 != 0.0f))
		B02 = (((t - m_Knots[i + 0].Kt) / d1) * B01) + (((m_Knots[i + 2].Kt - t) / d2) * B11);
	else
		if (d1 != 0.0f)
			B02 = (((t - m_Knots[i + 0].Kt) / d1) * B01);
		else
			if (d2 != 0.0f)
				B02 = (((m_Knots[i + 2].Kt - t) / d2) * B11);
			else
				B02 = 0.0f;


	d1 = (m_Knots[i + 2].Kt - m_Knots[i + 0].Kt);
	d2 = (m_Knots[i + 3].Kt - m_Knots[i + 1].Kt);

	if ((d1 != 0.0f) && (d2 != 0.0f))
		B03 = (((t - m_Knots[i + 0].Kt) / d1) * B02) + (((m_Knots[i + 3].Kt - t) / d2) * B12);
	else
		if (d1 != 0.0f)
			B03 = (((t - m_Knots[i + 0].Kt) / d1) * B02);
		else
			if (d2 != 0.0f)
				B03 = (((m_Knots[i + 3].Kt - t) / d2) * B12);
			else
				B03 = 0.0f;


	d1 = (m_Knots[i + 3].Kt - m_Knots[i + 0].Kt);
	d2 = (m_Knots[i + 4].Kt - m_Knots[i + 1].Kt);

	if ((d1 != 0.0f) && (d2 != 0.0f))
		B04 = (((t - m_Knots[i + 0].Kt) / d1) * B03) + (((m_Knots[i + 4].Kt - t) / d2) * B13);
	else
		if (d1 != 0.0f)
			B04 = (((t - m_Knots[i + 0].Kt) / d1) * B03);
		else
			if (d2 != 0.0f)
				B04 = (((m_Knots[i + 4].Kt - t) / d2) * B13);
			else
				B04 = 0.0f;

	return B04;
}

template <class _Vector>
void cvCurveNurbs<_Vector>::SetClampedKnots()
{
	// set clamped knot vector

	// KV-t's and KV-entries are the same, because KV-t's are at knots
	// (join points between curve segments)

	unsigned int nKnots = cvCurve<_Vector>::m_Vertices.GetCount() - 2;  // # distinct knots (excludes multiplicity)
	m_Knots.Resize(nKnots + 6);
	
	// These arrays can/should be a lot smaller.
	m_BlendConsts.Resize(nKnots - 1);

	unsigned int i, kvi, kvj;

	kvi = 0;
	kvj = 0;

	for (i = 0; i < 3; i++)
	{
		m_Knots[kvi].Kv = 0;
		m_Knots[kvi++].Kt = cvCurve<_Vector>::m_Looping ? (float)i-3.0f : 0.0f;
	}

	for (i = 0; i < nKnots; i++)
	{
		m_Knots[kvi].Kv= kvj;
		m_Knots[kvi++].Kt = (float)kvj++;
	}

	for (i = 0; i < 3; i++)
	{
		m_Knots[kvi].Kv = kvj - 1;
		m_Knots[kvi++].Kt = cvCurve<_Vector>::m_Looping ? (float)(kvj + i) : (float)(kvj - 1);
	}

	for(int nBlendFuncIdx = 0; nBlendFuncIdx < (int)(nKnots - 1); ++nBlendFuncIdx)
	{
		float fT4 = m_Knots[nBlendFuncIdx + 4].Kt;
		float fTempVal = (fT4 - m_Knots[nBlendFuncIdx + 1].Kt) * (fT4 - m_Knots[nBlendFuncIdx + 2].Kt) * (fT4 - m_Knots[nBlendFuncIdx + 3].Kt);

		float fBlendFuncConst;
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx].fFuncConst1 = fBlendFuncConst;
	}

	for(int nBlendFuncIdx = 1; nBlendFuncIdx < (int)(nKnots - 0); ++nBlendFuncIdx)
	{
		float fBlendFuncConst, fTempVal;

		float fT4 = m_Knots[nBlendFuncIdx + 4].Kt;
		float fT3 = m_Knots[nBlendFuncIdx + 3].Kt;
		float fT2 = m_Knots[nBlendFuncIdx + 2].Kt;
		float fT1 = m_Knots[nBlendFuncIdx + 1].Kt;
		float fT0 = m_Knots[nBlendFuncIdx + 0].Kt;

		fTempVal = (fT3 - fT0) * (fT3 - fT1) * (fT3 - fT2);
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 1].fFuncConst2a = fBlendFuncConst;


		fTempVal = (fT4 - fT1) * (fT3 - fT2) * (fT3 - fT1);
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 1].fFuncConst2b = fBlendFuncConst;

		fTempVal = (fT4 - fT1) * (fT3 - fT2) * (fT4 - fT2);
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 1].fFuncConst2c = fBlendFuncConst;
	}

	for(int nBlendFuncIdx = 2; nBlendFuncIdx < (int)(nKnots + 1); ++nBlendFuncIdx)
	{
		float fBlendFuncConst, fTempVal;

		float fT4 = m_Knots[nBlendFuncIdx + 4].Kt;
		float fT3 = m_Knots[nBlendFuncIdx + 3].Kt;
		float fT2 = m_Knots[nBlendFuncIdx + 2].Kt;
		float fT1 = m_Knots[nBlendFuncIdx + 1].Kt;
		float fT0 = m_Knots[nBlendFuncIdx + 0].Kt;

		fTempVal = (fT4 - fT1) * (fT3 - fT1) * (fT2 - fT1);
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 2].fFuncConst3a = fBlendFuncConst;

		fTempVal = (fT3 - fT0) * (fT2 - fT1) * (fT2 - fT0);
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 2].fFuncConst3b = fBlendFuncConst;

		fTempVal = (fT3 - fT0) * (fT2 - fT1) * (fT3 - fT1);
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 2].fFuncConst3c = fBlendFuncConst;
	}

	for(int nBlendFuncIdx = 3; nBlendFuncIdx < (int)(nKnots + 2); ++nBlendFuncIdx)
	{
		float fT0 = m_Knots[nBlendFuncIdx + 0].Kt;
		float fTempVal = (m_Knots[nBlendFuncIdx + 1].Kt - fT0) * (m_Knots[nBlendFuncIdx + 2].Kt - fT0) * (m_Knots[nBlendFuncIdx + 3].Kt - fT0);

		float fBlendFuncConst;
		if(fTempVal != 0.0f)
		{
			fBlendFuncConst = 1.0f / fTempVal;
		}
		else
		{
			fBlendFuncConst = -1.0f;
		}
		m_BlendConsts[nBlendFuncIdx - 3].fFuncConst4 = fBlendFuncConst;
	}
}

template<class _Vector> 
IMPLEMENT_PLACE	(cvCurveNurbs<_Vector>);

template <class _Vector>
cvCurveNurbs<_Vector>::cvCurveNurbs (datResource &rsc) : cvCurve<_Vector>(rsc),

m_Knots(rsc, true),
m_BlendConsts(rsc, true)

{
}


#if	__DECLARESTRUCT

template<class _Vector> 
void	cvCurveNurbs<_Vector>::DeclareStruct	(datTypeStruct &s)
{
	cvCurve<_Vector>::DeclareStruct(s);

	STRUCT_BEGIN(cvCurveNurbs<_Vector>);
	STRUCT_FIELD(m_Knots);
	STRUCT_FIELD(m_BlendConsts);	
	STRUCT_CONTAINED_ARRAY(m_Blend);
	STRUCT_CONTAINED_ARRAY(m_SlopeBlend);
	STRUCT_END();
}

#endif

} // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

////////////////////////////////////////////////////////////////////////////////
#endif
