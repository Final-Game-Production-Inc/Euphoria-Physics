// 
// curve/mayacurve.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "mayacurve.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/safestruct.h"
#include "file/token.h"
#include "grcore/im.h"
#include "spatialdata/aabb.h"
#include "system/memory.h"
#include "vector/geometry.h"

#include "profile/profiler.h"
#include "grprofile/ekg.h"  // pfEKGMgr

using namespace rage;

IMPLEMENT_PLACE(mayaCurve);

#define CURVE_OPTIMIZED		1
#define CURVE_VECTORIZED_3	1 // This is "even more optimized"!
#define CURVE_VECTORIZED_4	1 //
#define GETKNOT_BINARYSEARCH 1

#define CURVE_EPSILON (0.000001f)
#define CURVE_INVEPSILON (1.0f-CURVE_EPSILON);

namespace mayaCurveTimers
{
	PF_PAGE(Curves, "Curve Timers");

	PF_GROUP(UpdateLoop);
	PF_LINK(Curves, UpdateLoop);

	PF_TIMER(PointAndSlope1, UpdateLoop);
	PF_TIMER(PointAndSlope2, UpdateLoop);
	PF_TIMER(Point1, UpdateLoop);
	PF_TIMER(Point2, UpdateLoop);
	PF_TIMER(PointAndTangent1, UpdateLoop);
	PF_TIMER(PointAndTangent2, UpdateLoop);
	PF_TIMER(PointSlopeTangent1, UpdateLoop);
	PF_TIMER(PointSlopeTangent2, UpdateLoop);
	PF_TIMER(Tangent1, UpdateLoop);
	PF_TIMER(Slope1, UpdateLoop);
	PF_TIMER(NearestPoint, UpdateLoop);
	PF_TIMER(NearestPointInRange, UpdateLoop);
};
using namespace mayaCurveTimers;

bool mayaCurve::Init(const Vector3 *cv, int cvCount, const float *kv, int kvCount, int polydegree, mayaCurveForm form)
{
#if __DEV
	if (cvCount <= 0)
	{
		Errorf("Bad number of control vertices (%d)", cvCount);
		return false;
	}
	if (form == FORM_UNKNOWN)
	{
		Errorf("Uninitialized curve form");
		return false;
	}
	if (form == FORM_CLOSED)
	{
		Errorf("Closed curves are not supported by the Maya importer/exporter");
		return false;
	}
	if (!cv)
	{
		Errorf("No curve control vertex array");
		return false;
	}
	if (polydegree < 1 || polydegree > 7)
	{
		Errorf("Unsupported curve degree %d", polydegree);
		return false;
	}
#endif

	m_N = static_cast<s16>(cvCount - 1);
	m_Degree = static_cast<s8>(polydegree);
	m_Form = static_cast<s8>(form);

	unsigned int nOutputVerts = cvCount;

	m_pCVs = rage_new Vector3[nOutputVerts];

	// Load up the original curve data.
	for (int i = 0; i < cvCount; i++)
		m_pCVs[i] = cv[i];

	// We pass in the actual knot vector reported by the Maya API.
	m_KnotVector.Init(kv, kvCount, m_N, m_Degree+1);

	ComputeParametricRange();

	return true;
}

void mayaCurve::Shutdown()
{
	m_KnotVector.Shutdown();
	delete[] m_pCVs;
	m_pCVs = NULL;

	m_N = -1;
	m_Degree = 0;
	m_Form = FORM_UNKNOWN;
	m_TRange = 0.0f;
	m_TMax = 0.0f;
}

Vector3 mayaCurve::GetCurvePointAndSlope(float t, float slopeIn, float &slopeOut) const
{
	PF_FUNC(PointAndSlope1);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t);
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	// See "Computer Graphics Second Edition" p.497 Eq(11.43).
	Vector3 res, slopeVec;
	res.Zero();
	slopeVec.Zero();
//	float w = 0.0f;  // WEIGHTS

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope2;
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector(), slope2);
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		slopeVec.AddScaled(v, slope2);
//		w += GetWeight(knot) * b;  // WEIGHTS
		knot++;
	}

//	res.InvScale(w);  // WEIGHTS

	slopeOut = slopeVec.Mag() * m_TRange;
	if (slopeIn > 0.0f)
	{
		// Return a slope half way between this slope and the previous (input) slope.
		slopeOut = 0.5f * (slopeIn + slopeOut);
	}

	return res;
}

Vector3 mayaCurve::GetCurvePointAndSlope(float t, float slopeIn, float &slopeOut, int knotIndexIn, int &knotIndexOut) const
{
	PF_FUNC(PointAndSlope2);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t, knotIndexIn);
	knotIndexOut = knot;
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	// See "Computer Graphics Second Edition" p.497 Eq(11.43).
	Vector3 res, slopeVec;
	res.Zero();
	slopeVec.Zero();
//	float w = 0.0f;  // WEIGHTS

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope2;
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector(), slope2);
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		slopeVec.AddScaled(v, slope2);
//		w += GetWeight(knot) * b;  // WEIGHTS
		knot++;
	}

//	res.InvScale(w);  // WEIGHTS

	slopeOut = slopeVec.Mag() * m_TRange;
	if (slopeIn > 0.0f)
	{
		// Return a slope half way between this slope and the previous (input) slope.
		slopeOut = 0.5f * (slopeIn + slopeOut);
	}

	return res;
}


Vector3 mayaCurve::GetCurvePoint(float t) const
{
	PF_FUNC(Point1);
	t = Clamp(t, 0.0f, 1.0f);

	if (m_Degree == 1)  // Matches the optimization in GetNearestPoint().
	{
		if (t == 1.0f)
			t = CURVE_INVEPSILON;

		t *= GetSpanCount();  // Convert t from [0,1] to curve segment.

		int seg = static_cast<int>(t);
		Assert(seg < GetCVCount() - 1);

		t -= static_cast<float>(seg);  // t = frac(t)

		Vector3 v = GetCV(seg) + (t * (GetCV(seg + 1) - GetCV(seg)));
		return v;
	}

	// Find i.
	int knot = GetKnotBeforeOrAtT(t);
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	Vector3 res;
	res.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector());
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		knot++;
	}

	return res;
}

Vector3 mayaCurve::GetCurvePoint(float t, int knotIndexIn, int &knotIndexOut) const
{
	PF_FUNC(Point2);
	t = Clamp(t, 0.0f, 1.0f);

	if (m_Degree == 1)  // Matches the optimization in GetNearestPoint().
	{
		if (t == 1.0f)
			t = CURVE_INVEPSILON;

		t *= GetSpanCount();  // Convert t from [0,1] to curve segment.

		int seg = static_cast<int>(t);
		Assert(seg < GetCVCount() - 1);

		t -= static_cast<float>(seg);  // t = frac(t)

		Vector3 v = GetCV(seg) + (t * (GetCV(seg + 1) - GetCV(seg)));
		return v;
	}

	// Find i.
	int knot = GetKnotBeforeOrAtT(t, knotIndexIn);
	knotIndexOut = knot;
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	Vector3 res;
	res.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector());
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		knot++;
	}

	return res;
}


Vector3 mayaCurve::GetCurvePointAndTangent(float t, Vector3 &tangent) const
{
	PF_FUNC(PointAndTangent1);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t);
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	Vector3 res, slopeVec;
	res.Zero();
	slopeVec.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope;
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector(), slope);
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		slopeVec.AddScaled(v, slope);
		knot++;
	}

	tangent = slopeVec;
	return res;
}

Vector3 mayaCurve::GetCurvePointAndTangent(float t, Vector3 &tangent, int knotIndexIn, int &knotIndexOut) const
{
	PF_FUNC(PointAndTangent2);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t, knotIndexIn);
	knotIndexOut = knot;
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	Vector3 res, slopeVec;
	res.Zero();
	slopeVec.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope;
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector(), slope);
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		slopeVec.AddScaled(v, slope);
		knot++;
	}

	tangent = slopeVec;
	return res;
}


Vector3 mayaCurve::GetCurvePointSlopeAndTangent(float t, float slopeIn, float &slopeOut, Vector3 &tangentOut) const
{
	PF_FUNC(PointSlopeTangent1);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t);
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	// See "Computer Graphics Second Edition" p.497 Eq(11.43).
	Vector3 res, slopeVec;
	res.Zero();
	slopeVec.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope;
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector(), slope);
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		slopeVec.AddScaled(v, slope);
		knot++;
	}

	slopeOut = slopeVec.Mag() * m_TRange;
	if (slopeIn > 0.0f)
	{
		// Return a slope half way between this slope and the previous (input) slope.
		slopeOut = 0.5f * (slopeIn + slopeOut);
	}

	tangentOut = slopeVec;

	return res;
}

Vector3 mayaCurve::GetCurvePointSlopeAndTangent(float t, float slopeIn, float &slopeOut, Vector3 &tangentOut, int knotIndexIn, int &knotIndexOut) const
{
	PF_FUNC(PointSlopeTangent2);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t, knotIndexIn);
	knotIndexOut = knot;
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	// See "Computer Graphics Second Edition" p.497 Eq(11.43).
	Vector3 res, slopeVec;
	res.Zero();
	slopeVec.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope;
		float b = GetBlendFunction(knot, GetOrder(), t, GetKnotVector(), slope);
		Vector3 v = GetCV(knot);
		res.AddScaled(v, b);
		slopeVec.AddScaled(v, slope);
		knot++;
	}

	slopeOut = slopeVec.Mag() * m_TRange;
	if (slopeIn > 0.0f)
	{
		// Return a slope half way between this slope and the previous (input) slope.
		slopeOut = 0.5f * (slopeIn + slopeOut);
	}

	tangentOut = slopeVec;

	return res;
}

Vector3 mayaCurve::GetCurveTangent(float t) const
{
	PF_FUNC(Tangent1);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t);
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	Vector3 slopeVec;
	slopeVec.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope = GetSlope(knot, GetOrder(), t, GetKnotVector());
		Vector3 v = GetCV(knot);
		slopeVec.AddScaled(v, slope);
		knot++;
	}

	return slopeVec;
}

float mayaCurve::GetCurveSlope(float t, float slopeIn) const
{
	PF_FUNC(Slope1);

	t = Clamp(t, 0.0f, 1.0f);

	// Find i.
	int knot = GetKnotBeforeOrAtT(t);
	knot -= m_Degree;

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	Vector3 slopeVec;
	slopeVec.Zero();

	// For all the relevant control vertices at this span;
	for (int k = 0; k < GetOrder(); k++)
	{
		float slope2 = GetSlope(knot, GetOrder(), t, GetKnotVector());
		Vector3 v = GetCV(knot);
		slopeVec.AddScaled(v, slope2);
		knot++;
	}

	float slopeOut = slopeVec.Mag() * m_TRange;
	if (slopeIn > 0.0f)
	{
		// Return a slope half way between this slope and the previous (input) slope.
		slopeOut = 0.5f * (slopeIn + slopeOut);
	}

	return slopeOut;
}

float mayaCurve::GetBlendFunction(int i, int j, float t, const mayaKnotVector &kv, float &slopeOut) const
{
	float Bij;

	Assert(j >= 1 && j <= GetOrder());

	if (j == 1)
	{
		Bij = ((kv.GetKnot(i) <= t) && t < kv.GetKnot(i+1)) ? 1.0f : 0.0f;
		slopeOut = 0.0f;
	}
	else
	{
		float ti = kv.GetKnot(i);
		float tiplusjminus1 = kv.GetKnot(i+j-1);
		float numer1 = t - ti;
		float denom1 = tiplusjminus1 - ti;

		float tiplusj = kv.GetKnot(i+j);
		float tiplus1 = kv.GetKnot(i+1);

		float numer2 = tiplusj - t;
		float denom2 = tiplusj - tiplus1;

		float Bijminus1, Biplus1jminus1, gradient1, gradient2;

		if (denom1 != 0.0f && denom2 != 0.0f)
		{
			Bijminus1 = GetBlendFunction(i+0, j-1, t, kv, gradient1);
			Biplus1jminus1 = GetBlendFunction(i+1, j-1, t, kv, gradient2);

			float frac1 = numer1 / denom1;
			float frac2 = numer2 / denom2;

			Bij = (frac1 * Bijminus1) +
			      (frac2 * Biplus1jminus1);

			slopeOut = ((1.0f / denom1) * Bijminus1)      + (frac1 * gradient1) -
			        ((1.0f / denom2) * Biplus1jminus1) + (frac2 * gradient2);
		}
		else if (denom1 == 0.0f && denom2 == 0.0f)
		{
			Bij = 0.0f;
			slopeOut = 0.0f;
		}
		else if (denom2 == 0.0f)
		{
			Bijminus1 = GetBlendFunction(i+0, j-1, t, kv, gradient1);
			float frac1 = numer1 / denom1;
			Bij = frac1 * Bijminus1;
			slopeOut = ((1.0f / denom1) * Bijminus1) + (frac1 * gradient1) /* - 0.0f */;
		}
		else
		{
			Biplus1jminus1 = GetBlendFunction(i+1, j-1, t, kv, gradient2);
			float frac2 = numer2 / denom2;
			Bij = frac2 * Biplus1jminus1;
			slopeOut = /* 0.0f */ - ((1.0f / denom2) * Biplus1jminus1) + (frac2 * gradient2);
		}
	}
	return Bij;
}

float mayaCurve::GetSlope(int i, int j, float t, const mayaKnotVector &kv) const
{
	float slope;

	Assert(j >= 1 && j <= GetOrder());

	if (j == 1)
	{
		slope = 0.0f;
	}
	else
	{
		float ti = kv.GetKnot(i);
		float tiplusjminus1 = kv.GetKnot(i+j-1);
		float numer1 = t - ti;
		float denom1 = tiplusjminus1 - ti;

		float tiplusj = kv.GetKnot(i+j);
		float tiplus1 = kv.GetKnot(i+1);

		float numer2 = tiplusj - t;
		float denom2 = tiplusj - tiplus1;

		float Bijminus1, Biplus1jminus1, gradient1, gradient2;

		if (denom1 != 0.0f && denom2 != 0.0f)
		{
			Bijminus1 = GetBlendFunction(i+0, j-1, t, kv, gradient1);
			Biplus1jminus1 = GetBlendFunction(i+1, j-1, t, kv, gradient2);

			float frac1 = numer1 / denom1;
			float frac2 = numer2 / denom2;

			slope = ((1.0f / denom1) * Bijminus1)      + (frac1 * gradient1) -
				((1.0f / denom2) * Biplus1jminus1) + (frac2 * gradient2);
		}
		else
		if (denom1 == 0.0f && denom2 == 0.0f)
		{
			slope = 0.0f;
		}
		else
		if (denom2 == 0.0f)
		{
			Bijminus1 = GetBlendFunction(i+0, j-1, t, kv, gradient1);
			float frac1 = numer1 / denom1;
			slope = ((1.0f / denom1) * Bijminus1) + (frac1 * gradient1) /* - 0.0f */;
		}
		else
		{
			Biplus1jminus1 = GetBlendFunction(i+1, j-1, t, kv, gradient2);
			float frac2 = numer2 / denom2;
			slope = /* 0.0f */ - ((1.0f / denom2) * Biplus1jminus1) + (frac2 * gradient2);
		}
	}
	return slope;
}

#if CURVE_OPTIMIZED
namespace rage
{
// Base case, Order == 1
template <>
inline float mayaCurve::GetBlendFunction_Internal<1>(int i, float t, const mayaKnotVector &kv) const
{
	return ((kv.GetKnot(i) <= t) && t < kv.GetKnot(i+1)) ? 1.0f : 0.0f;
}

// Recursive case, calls GBF<Order-1>. These 'recursive' calls should all inline when the starting order is
// a reasonable size.
template<int _Order> 
inline float mayaCurve::GetBlendFunction_Internal(int i, float t, const mayaKnotVector &kv) const
{
	float ti = kv.GetKnot(i);
	float tiplusjminus1 = kv.GetKnot(i+_Order-1);
	float numer1 = t - ti;
	float denom1 = tiplusjminus1 - ti;

	float tiplusj = kv.GetKnot(i+_Order);
	float tiplus1 = kv.GetKnot(i+1);

	float numer2 = tiplusj - t;
	float denom2 = tiplusj - tiplus1;

	if (denom1 != 0.0f)
	{
		if (denom2 != 0.0f)
		{
			return ((numer1 / denom1) * GetBlendFunction_Internal<_Order-1>(i+0, t, kv)) +
				((numer2 / denom2) * GetBlendFunction_Internal<_Order-1>(i+1, t, kv));
		}
		else
		{
			return (numer1 / denom1) * GetBlendFunction_Internal<_Order-1>(i+0, t, kv);
		}
	}
	else
	{
		if (denom2 != 0.0f)
		{
			return (numer2 / denom2) * GetBlendFunction_Internal<_Order-1>(i+1, t, kv);
		}
		else
		{
			return 0.0f;
		}
	}
}

} // namespace rage
#endif





namespace rage
{

inline ScalarV_Out mayaCurve::GetBlendFunction_Order3(int i, const float& t, const mayaKnotVector &kv)
{
	//GBF<3>:
	//	ti = k(i)
	//	tiplusjminus1 = k(i+3-1) = k(i+2)
	//	numer1 = t - k(i)
	//	denom1 = k(i+2) - k(i)
	//	tiplusj = k(i+3)
	//
	//               t - k(i)                      t - k(i+3)
	//	result =   ------------  * GBF<2>(i,t) + ---------------  * GBF<2>(i+1, t)
	//             k(i+2) - k(i)                  k(i+1) - k(i+3)
	//
	//	result = range(t, k(i), k(i+2)) * GBF<2>(i,t) + range(t, k(i+3), k(i+1)) * GBF<2>(i+1, t)
	//
	//GBF<2>:
	//	ti = k(i)
	//	tiplusjminus1 = k(i+1)
	//	tiplusj = k(i+2)
	//
	//              t - k(i)                    t - k(i+2)
	//	result = ------------- * GBF<1>(i,t) + -------------    * GBF<1>(i+1, t)
	//	         k(i+1) - k(i)                 k(i+1) - k(i+2)
	//
	//	result = range(t, k(i), k(i+1)) * GBF<1>(i,t) + range(t, k(i+2), k(i+1)) * GBF<1>(i+1, t)
	//
	//GBF<1>: 
	//	1 if k(i) <= t < k(i+1)
	//	0 otherwise
	//
	//Expanded:
	//
	//	GBF<3> = range(t, k(i), k(i+2)) *		// E
	//			range(t, k(i), k(i+1)) * GBF<1>(i,t) + range(t, k(i+2), k(i+1)) * GBF<1>(i+1, t)		// A + B
	//		   + range(t, k(i+3), k(i+1)) *		// F
	//			range(t, k(i+1), k(i+2)) * GBF<1>(i+1,t) + range(t, k(i+3), k(i+2)) * GBF<1>(i+2, t)	// C + D
	//
	//Convert GBF<1>(i,t) to between(t, k(i), k(i+1):
	//
	//	GBF<3> = range(t, k(i), k(i+2)) *		// E
	//			(range(t, k(i), k(i+1)) * between(t, k(i), k(i+1)) + range(t, k(i+2), k(i+1)) * between(t, k(i+1), k(i+2)))		// A + B
	//		   + range(t, k(i+3), k(i+1)) *		// F
	//			(range(t, k(i+1), k(i+2)) * between(t, k(i+1), k(i+2)) + range(t, k(i+3), k(i+2)) * between(t, k(i+2), k(i+3)))	// C + D


	ScalarV _t = ScalarVFromF32( t );

	ScalarV _k0 = kv.GetKnotV(i);
	ScalarV _k1 = kv.GetKnotV(i+1);
	ScalarV _k2 = kv.GetKnotV(i+2);
	ScalarV _k3 = kv.GetKnotV(i+3);

	Vec4V _k0213( _k0, _k2, _k1, _k3 );
	Vec4V _k1122 = GetFromTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( Vec4V(_k1), Vec4V(_k2) );

	Vec4V _k0112( _k0, _k1, _k1, _k2 );
	Vec4V _k1223( _k1, _k2, _k2, _k3 );

	Vec4V betweens = RangeSafe( Vec4V(_t), _k0213, _k1122 );
	VecBoolV isInBetween = And( IsGreaterThanOrEqual( Vec4V(_t), _k0112 ), IsLessThan( Vec4V(_t), _k1223 ) );
	betweens = SelectFT( isInBetween, Vec4V(V_ZERO), betweens ); // == <A,B,C,D>

	Vec2V _k03 = Vec2V( _k0, _k3 );
	Vec2V _k21 = Vec2V( _k2, _k1 );
	Vec2V ranges = RangeSafe( Vec2V(_t), _k03, _k21 ); // == <E,F>

	// Now we need E*(A+B) + F*(C+D)
	ScalarV _E = SplatX( ranges );
	ScalarV _F = SplatY( ranges );
	ScalarV _A = SplatX( betweens );
	ScalarV _B = SplatY( betweens );
	ScalarV _C = SplatZ( betweens );
	ScalarV _D = SplatW( betweens );
	ScalarV _retVal = AddScaled( _E*(_A+_B), _F, (_C+_D) );

	// Make sure output is OK.
	FastAssert( FPIsFinite(_retVal.Getf()) );

	return _retVal;
}

inline ScalarV_Out mayaCurve::GetBlendFunction_Order4(int i, const float& t, const mayaKnotVector& kv)
{
	//GBF<4>:
	//	ti = k(i)
	//	tiplusjminus1 = k(i+4-1) = k(i+3)
	//	numer1 = t - k(i)
	//	denom1 = k(i+3) - k(i)
	//	tiplusj = k(i+4)
	//	...
	//
	//               t - k(i)                      t - k(i+4)
	//	result =   ------------  * GBF<3>(i,t) + ---------------  * GBF<3>(i+1, t)
	//                 k(i+3) - k(i)             k(i+1) - k(i+4)
	//
	//	result = range(t, k(i), k(i+3)) * GBF<3>(i,t) + range(t, k(i+4), k(i+1)) * GBF<3>(i+1, t)
	//
	//GBF<3>:
	//	ti = k(i)
	//	tiplusjminus1 = k(i+3-1) = k(i+2)
	//	numer1 = t - k(i)
	//	denom1 = k(i+2) - k(i)
	//	tiplusj = k(i+3)
	//
	//                  t - k(i)                    t - k(i+3)
	//	result =   ------------  * GBF<2>(i,t) + ---------------  * GBF<2>(i+1, t)
	//                 k(i+2) - k(i)             k(i+1) - k(i+3)
	//
	//	result = range(t, k(i), k(i+2)) * GBF<2>(i,t) + range(t, k(i+3), k(i+1)) * GBF<2>(i+1, t)
	//
	//GBF<2>:
	//	ti = k(i)
	//	tiplusjminus1 = k(i+1)
	//	tiplusj = k(i+2)
	//
	//              t - k(i)                    t - k(i+2)
	//	result = ------------- * GBF<1>(i,t) + -------------    * GBF<1>(i+1, t)
	//	         k(i+1) - k(i)                k(i+1) - k(i+2)
	//
	//	result = range(t, k(i), k(i+1)) * GBF<1>(i,t) + range(t, k(i+2), k(i+1)) * GBF<1>(i+1, t)
	//
	//GBF<1>: 
	//	1 if k(i) <= t < k(i+1)
	//	0 otherwise
	//
	//Expanded:
	//
	//	GBF<4> =	range(t, k(i), k(i+3)) *
	//					[	range(t, k(i), k(i+2)) *
	//						{ range(t, k(i), k(i+1)) * GBF<1>(i,t) + range(t, k(i+2), k(i+1)) * GBF<1>(i+1, t) }
	//						+
	//						range(t, k(i+3), k(i+1)) *
	//						{ range(t, k(i+1), k(i+2)) * GBF<1>(i+1,t) + range(t, k(i+3), k(i+2)) * GBF<1>(i+2, t) }
	//					]
	//				+
	//				range(t, k(i+4), k(i+1)) *
	//					[	range(t, k(i+1), k(i+3)) *
	//						{ range(t, k(i+1), k(i+2)) * GBF<1>(i+1,t) + range(t, k(i+3), k(i+2)) * GBF<1>(i+2, t) }
	//						+
	//						range(t, k(i+4), k(i+2)) *
	//						{ range(t, k(i+2), k(i+3)) * GBF<1>(i+2,t) + range(t, k(i+4), k(i+3)) * GBF<1>(i+3, t) }
	//					]
	//
	//	Convert GBF<1>(i,t) to between(t, k(i), k(i+1)):
	//
	//	GBF<4> =	range(t, k(i), k(i+3)) *				// A
	//					[	range(t, k(i), k(i+2)) *		// B
	//						{ range(t, k(i), k(i+1)) * between(t,k(i),k(i+1)) + range(t, k(i+2), k(i+1)) * between(t,k(i+1),k(i+2)) }		// G + H
	//						+
	//						range(t, k(i+3), k(i+1)) *		// C
	//						{ range(t, k(i+1), k(i+2)) * between(t,k(i+1),k(i+2)) + range(t, k(i+3), k(i+2)) * between(t,k(i+2),k(i+3)) }	// I + J
	//					]
	//				+
	//				range(t, k(i+4), k(i+1)) *				// D
	//					[	range(t, k(i+1), k(i+3)) *		// E
	//						{ range(t, k(i+1), k(i+2)) * between(t,k(i+1),k(i+2)) + range(t, k(i+3), k(i+2)) * between(t,k(i+2),k(i+3)) }	// K + L
	//						+
	//						range(t, k(i+4), k(i+2)) *		// F
	//						{ range(t, k(i+2), k(i+3)) * between(t,k(i+2),k(i+3)) + range(t, k(i+4), k(i+3)) * between(t,k(i+3),k(i+4)) }	// M + N
	//					]
	//
	// Need: A * [ B*(G+H) + C*(I+J) ] + D * [ E*(K+L) + F*(M+N) ]


	ScalarV _t = ScalarVFromF32( t );

	ScalarV _k0 = kv.GetKnotV(i);
	ScalarV _k1 = kv.GetKnotV(i+1);
	ScalarV _k2 = kv.GetKnotV(i+2);
	ScalarV _k3 = kv.GetKnotV(i+3);
	ScalarV _k4 = kv.GetKnotV(i+4);

	// Ranges.
	Vec4V _k0013( _k0, _k0, _k1, _k3 );
	Vec4V _k2331( _k2, _k3, _k3, _k1 );

	Vec2V _k44( _k4 );
	Vec2V _k12( _k1, _k2 );

	// Range-betweens.
	Vec4V _k0112( _k0, _k1, _k1, _k2 );
	Vec4V _k1223( _k1, _k2, _k2, _k3 );

	// (no swizzles needed for the above two, can check the t's 'in-between' value as-is)
	// ...

	Vec4V _k2334( _k2, _k3, _k3, _k4 );
	//Vec4V _k1223 = ...; // already exists!

	// (no swizzles needed for the above two, can check the t's 'in-between' value as-is)
	// ...

	// Ranges.
	Vec4V ranges_k0013_k2331 = RangeSafe( Vec4V(_t), _k0013, _k2331 );		// <B,A,E,C>

	Vec2V ranges_k44_k12 = RangeSafe( Vec2V(_t), _k44, _k12 );				// <D,F>

	// Range-betweens.
	Vec4V betweens_k0112_k1223 = RangeSafe( Vec4V(_t), _k0112, _k1223 );
	VecBoolV isInBetween0 = And( IsGreaterThanOrEqual( Vec4V(_t), _k0112 ), IsLessThan( Vec4V(_t), _k1223 ) );
	betweens_k0112_k1223 = SelectFT( isInBetween0, Vec4V(V_ZERO), betweens_k0112_k1223 );		// <G,I,K,M>

	Vec4V betweens_k2334_k1223 = RangeSafe( Vec4V(_t), _k2334, _k1223 );
	VecBoolV isInBetween1 = And( IsGreaterThanOrEqual( Vec4V(_t), _k1223 ), IsLessThan( Vec4V(_t), _k2334 ) );
	betweens_k2334_k1223 = SelectFT( isInBetween1, Vec4V(V_ZERO), betweens_k2334_k1223 );		// <H,J,L,N>

	// Rename them.
	Vec4V _BAEC = ranges_k0013_k2331;
	Vec2V _DF = ranges_k44_k12;
	Vec4V _GIIM = betweens_k0112_k1223;
	Vec4V _HJJN = betweens_k2334_k1223;

	// Create:			A * [ B*(G+H) + C*(I+J) ] + D * [ E*(K+L) + F*(M+N) ], where L == J and I == K,
	// so, reduced:		A * [ B*(G+H) + C*(I+J) ] + D * [ E*(I+J) + F*(M+N) ]
	Vec4V _GpH_IpJ_IpJ_MpN = _GIIM + _HJJN;
	ScalarV _B = SplatX( _BAEC );
	ScalarV _C = SplatW( _BAEC );
	ScalarV _E = SplatZ( _BAEC );
	ScalarV _F = SplatY( _DF );
	Vec4V _BCEF( _B, _C, _E, _F );
	Vec4V _GpH_IpJ_IpJ_MpN_times_B_C_E_F = _GpH_IpJ_IpJ_MpN * _BCEF;
	// Reduced:			A * [ P + Q ] + D * [ R + S ]
	ScalarV _P = SplatX( _GpH_IpJ_IpJ_MpN_times_B_C_E_F );
	ScalarV _Q = SplatY( _GpH_IpJ_IpJ_MpN_times_B_C_E_F );
	ScalarV _R = SplatZ( _GpH_IpJ_IpJ_MpN_times_B_C_E_F );
	ScalarV _S = SplatW( _GpH_IpJ_IpJ_MpN_times_B_C_E_F );
	ScalarV _A = SplatY( _BAEC );
	ScalarV _D = SplatX( _DF );
	ScalarV _retVal = AddScaled( _A * (_P + _Q), _D, ( _R + _S ) );

	// Make sure output is OK.
	FastAssert( FPIsFinite(_retVal.Getf()) );

	return _retVal;
}
} // namespace rage



float mayaCurve::GetBlendFunction(int i, int j, float t, const mayaKnotVector &kv) const
{
	Assert(j >= 1 && j <= GetOrder());

#if CURVE_OPTIMIZED
	switch(j)
	{
	case 1: return GetBlendFunction_Internal<1>(i,t,kv);
	case 2: return GetBlendFunction_Internal<2>(i,t,kv);
#if CURVE_VECTORIZED_3
	case 3: return GetBlendFunction_Order3(i,t,kv).Getf();
#else
	case 3: return GetBlendFunction_Internal<3>(i,t,kv);
#endif
#if CURVE_VECTORIZED_4
	case 4: return GetBlendFunction_Order4(i,t,kv).Getf();
#else
	case 4: return GetBlendFunction_Internal<4>(i,t,kv);
#endif
	default:
		// don't return, do the recursive stuff below instead.
		break;
	}
#endif

	float Bij;


	if (j == 1)
	{
		Bij = ((kv.GetKnot(i) <= t) && t < kv.GetKnot(i+1)) ? 1.0f : 0.0f;
	}
	else
	{
		float ti = kv.GetKnot(i);
		float tiplusjminus1 = kv.GetKnot(i+j-1);
		float numer1 = t - ti;
		float denom1 = tiplusjminus1 - ti;

		float tiplusj = kv.GetKnot(i+j);
		float tiplus1 = kv.GetKnot(i+1);

		float numer2 = tiplusj - t;
		float denom2 = tiplusj - tiplus1;

		if (denom1 != 0.0f && denom2 != 0.0f)
		{
			Bij = ((numer1 / denom1) * GetBlendFunction(i+0, j-1, t, kv)) +
			      ((numer2 / denom2) * GetBlendFunction(i+1, j-1, t, kv));
		}
		else if (denom1 == 0.0f && denom2 == 0.0f)
		{
			Bij = 0.0f;
		}
		else if (denom2 == 0.0f)
		{
			Bij = (numer1 / denom1) * GetBlendFunction(i+0, j-1, t, kv);
		}
		else
		{
			Bij = (numer2 / denom2) * GetBlendFunction(i+1, j-1, t, kv);
		}
	}
	return Bij;
}

Vector3 mayaCurve::GetEditPoint(int i) const
{
	Assert(i >= 0 && i < GetEditPointCount());
	float t = static_cast<float>(i) / static_cast<float>(GetSpanCount());
	return GetCurvePoint(t);
}

void mayaCurve::ComputeParametricRange(void)
{
	// This seems correct to me; the highest t value is the highest value in
	// the knot vector, that pertains to an actual edit point (and is therefore
	// not a multiplicity knot).

	const mayaKnotVector &kv = GetKnotVector();
	m_TRange = kv.GetKnot( kv.GetLength() - 1 - m_Degree );
	m_TMax = m_TRange - CURVE_EPSILON;

	// Instead of storing another float, the knot vector is adjusted so that the
	// curve always starts with a parametric value of zero. Otherwise, this would
	// be the way to do it.
//	TMin = kv.GetKnot(Degree);
}

void mayaCurve::Save(fiTokenizer &tok, bool saveControlVertices, bool saveKnotVector) const
{
	tok.Put("CURVE ");
	tok.Put("Form ");
	tok.Put(m_Form);
	tok.Put("Degree ");
	tok.Put(m_Degree);
	tok.Put("TRange ");
	tok.Put(m_TRange);
	tok.Put("TMax ");
	tok.Put(m_TMax);
	tok.Put("CVCount ");
	tok.Put(m_N+1);

	if (saveControlVertices)
	{
		int count = m_N+1;
		Assert(m_pCVs || count==0);
		for (int i=0; i<count; i++)
		{
			tok.Put("{ ");
			tok.Put(m_pCVs[i].x);
			tok.Put(m_pCVs[i].y);
			tok.Put(m_pCVs[i].z);
			tok.Put("} ");
		}
	}

	if (saveKnotVector)
		m_KnotVector.Save(tok);
}

void mayaCurve::Load(fiTokenizer &tok, bool loadControlVertices, bool loadKnotVector)
{
	tok.MatchToken("CURVE");
	m_Form = static_cast<s8>(tok.MatchInt("Form"));
	m_Degree = static_cast<u8>(tok.MatchInt("Degree"));
	m_TRange = tok.MatchFloat("TRange");
	m_TMax = tok.MatchFloat("TMax");
	int count = tok.MatchInt("CVCount");
	m_N = static_cast<s16>(count - 1);

	if (loadControlVertices)
	{
		Assert(!m_pCVs);
		m_pCVs = rage_new Vector3[count];
		for (int i=0; i<count; i++)
		{
			tok.CheckToken("{");
			tok.GetVector(m_pCVs[i]);
			tok.CheckToken("}");
		}
	}

	if (loadKnotVector)
		m_KnotVector.Load(tok);
}

// NOTES: Deprecated. Do not use this.
void mayaCurve::LoadFromCurvesFile(fiTokenizer &tok)
{
	char buffer[256];
	tok.GetToken(buffer, sizeof(buffer));

	ProcessNameToken(buffer);
	ProcessBoneIndex(tok.MatchIInt("boneindex"));
	tok.MatchIInt("roomindex");  // Absorb room index

	m_Degree = static_cast<u8>(tok.MatchIInt("degree"));
	m_Form = static_cast<s8>(tok.MatchIInt("form"));
	int count = tok.MatchIInt("cv");
	m_N = static_cast<s16>(count - 1);

	Assert(!m_pCVs);
	m_pCVs = rage_new Vector3[count];
	for (int i=0; i<count; i++)
	{
		tok.MatchToken("(");
		tok.GetVector(m_pCVs[i]);
		tok.MatchToken(")");
	}

	int kvCount = tok.MatchIInt("knots");

	sysMemStartTemp();
	float *knots = kvCount>0 ? rage_new float[kvCount] : NULL;
	for (int i=0; i<kvCount; i++)
		knots[i] = tok.GetFloat();
	sysMemEndTemp();

	m_KnotVector.Init(knots, kvCount, m_N, m_Degree+1);

	sysMemStartTemp();
	delete[] knots;
	sysMemEndTemp();

	// Set m_TRange, m_TMax
	ComputeParametricRange();
}

#if (__DEV || __BANK) && !__SPU
void mayaCurve::DrawCurve(Color32 color, int steps) const
{
	grcWorldIdentity();
	grcColor(color);

	if (GetDegree() > 1)
	{
		grcBegin(drawLineStrip, steps+1);
		grcVertex3f(GetCurvePoint(0.0f));
		for (int i=0; i<steps; i++)
			grcVertex3f(GetCurvePoint((float)(i+1) / (float)steps));
		grcEnd();
	}
	else
	{
		grcBegin(drawLineStrip, GetEditPointCount());
		grcVertex3f(GetEditPoint(0));
		for (int i=0; i<GetEditPointCount()-1; i++)
			grcVertex3f(GetEditPoint(i+1));
		grcEnd();
	}
}

void mayaCurve::DrawControlVertices(Color32 color) const
{
	grcWorldIdentity();
	grcColor(color);

	Matrix34 m;
	m.Identity();
	Vector3 size(0.1f, 0.1f, 0.1f);
	int cvCount = GetCVCount();

	for (int i=0; i<cvCount; i++) 
	{
		m.d = GetCV(i);
		grcDrawBox(size, m, color);
	}

	grcWorldIdentity();
	grcBegin(drawLineStrip, cvCount);
	for (int i=0; i<cvCount; i++)
		grcVertex3f(GetCV(i));
	grcEnd();
}

void mayaCurve::DrawEditPoints(Color32 color) const
{
	grcWorldIdentity();
	grcColor(color);

	for (int i=0; i<GetEditPointCount(); i++)
	{
		Vector3 a(-0.1f, 0.0f, 0.0f);
		Vector3 b( 0.1f, 0.0f, 0.0f);

		Vector3 c(0.0f, -0.1f, 0.0f);
		Vector3 d(0.0f,  0.1f, 0.0f);

		Vector3 e(0.0f, 0.0f, -0.1f);
		Vector3 f(0.0f, 0.0f,  0.1f);

		Matrix34 mat;
		mat.Identity();
		mat.d = GetEditPoint(i);

		mat.Transform(a);
		mat.Transform(b);
		mat.Transform(c);
		mat.Transform(d);
		mat.Transform(e);
		mat.Transform(f);

		grcBegin(drawLines, 6);
		grcVertex3f(a);	grcVertex3f(b);
		grcVertex3f(c);	grcVertex3f(d);
		grcVertex3f(e);	grcVertex3f(f);
		grcEnd();
	}
}

void mayaCurve::DrawArrow(Color32 color) const
{
	grcWorldIdentity();
	grcColor(color);

	Vector3 p[5];
	p[0] = GetCurvePoint(0.4f);
	p[1] = GetCurvePoint(0.5f);
	p[2] = GetCurvePoint(0.6f);

	// Shaft:
	Vector3 shaftdir = p[2] - p[0];
	shaftdir.Normalize();

	Vector3 shaftStart = p[1];
	Vector3 shaftEnd = p[1] + (shaftdir * 2.0f);

	Vector3 a = shaftStart;
	Vector3 b = shaftEnd;
	grcBegin(drawLines, 2);
	grcVertex3f(a);
	grcVertex3f(b);
	grcEnd();

	// Head:
	Vector3 f;
	f = -shaftdir;
	f.RotateY(45.0f * DtoR);
	f.Scale(2.0f);
	p[3] = shaftEnd + f;

	f = -shaftdir;
	f.RotateY(-45.0f * DtoR);
	f.Scale(2.0f);
	p[4] = shaftEnd + f;

	a = shaftEnd;
	b = p[3];
	grcBegin(drawLines, 2);
	grcVertex3f(a);
	grcVertex3f(b);
	grcEnd();

	a = shaftEnd;
	b = p[4];
	grcBegin(drawLines, 2);
	grcVertex3f(a);
	grcVertex3f(b);
	grcEnd();
}

void mayaCurve::DrawArrow(Color32 color, const Matrix34& camMatrix, const float arrowSize) const
{
	grcWorldIdentity();
	grcColor(color);

	Vector3 p[5];
	p[0] = GetCurvePoint(0.4f);
	p[1] = GetCurvePoint(0.5f);
	p[2] = GetCurvePoint(0.6f);

	// Shaft:
	Vector3 shaftdir = p[2] - p[0];
	shaftdir.Normalize();

	Vector3 shaftStart = p[1];
	Vector3 shaftEnd = p[1] + (shaftdir * 2.0f);

	Vector3 a = shaftStart;
	Vector3 b = shaftEnd;
	grcBegin(drawLines, 2);
	grcVertex3f(a);
	grcVertex3f(b);
	grcEnd();

	float comp = cosf(45.0f * DtoR) * arrowSize;

	Vector3 toPoint;
	toPoint.Subtract(camMatrix.d, shaftEnd);
	toPoint.Normalize();

	Vector3 relativeUp;
	relativeUp.Cross(toPoint, shaftdir);

	// Head:
	Vector3 f(shaftdir);
	f.Negate();
	f.Scale(comp);
	f.AddScaled(relativeUp, comp);
	p[3] = shaftEnd + f;

	f.AddScaled(relativeUp, 2.0f * -comp);
	p[4] = shaftEnd + f;

	a = shaftEnd;
	b = p[3];
	grcBegin(drawLines, 2);
	grcVertex3f(a);
	grcVertex3f(b);
	grcEnd();

	a = shaftEnd;
	b = p[4];
	grcBegin(drawLines, 2);
	grcVertex3f(a);
	grcVertex3f(b);
	grcEnd();
}

void mayaCurve::DrawKnotVector(Color32 color, bool drawEditPoints) const
{
	grcWorldIdentity();
	grcColor(color);

	int count = GetKnotCount() - (GetDegree()*2);
	for (int i=0; i<count; i++)
	{
		float knot = GetKnot(i + GetDegree());
		float t = knot / m_TRange;
		Vector3 pos = GetCurvePoint(t);

		char str[64];
		sprintf(str, "t=%.3f knot=%.3f", t, knot);
		grcDrawLabel(pos, str);
	}

	if (drawEditPoints)
		DrawEditPoints(color);
}

void mayaCurve::Draw(bool drawCurve, bool drawControlVerts, bool drawEditPoints, bool drawArrow) const
{
	if (drawCurve)
		DrawCurve(Color32(0xff808000), GetCVCount()*3);

	if (drawControlVerts)
		DrawControlVertices(Color32(0xff00ff00));

	if (drawEditPoints)
		DrawEditPoints(Color32(0xff0000ff));

	if (drawArrow)
		DrawArrow(Color32(0xffff0000));
}
#endif

int mayaCurve::GetKnotBeforeOrAtT(float t, int knotIndexIn) const
{
	// TODO: Compare floats as integers for better performance (knots are "never" negative and neither is t)

	Assert(t >= 0.0f && t <= 1.0f);

	// Convert t from [0,1) to curve segment.
	t *= m_TRange;
	t = Min(t, m_TMax);

	const mayaKnotVector &kv = GetKnotVector();


	// Optimization: see if the provided knot index is the right one.

	if (knotIndexIn>=0 && knotIndexIn+1<kv.GetLength())
	{
		if ((kv.GetKnot(knotIndexIn) <= t) && (t < kv.GetKnot(knotIndexIn+1)))
		{
			return knotIndexIn;
		}

		// Test the ones either side in case we're just stepping along.

		if (knotIndexIn+2 < kv.GetLength())
		{
			if ((kv.GetKnot(knotIndexIn+1) <= t) && (t < kv.GetKnot(knotIndexIn+2)))
				return knotIndexIn+1;
		}

		if (knotIndexIn-1 >= 0)
		{
			if ((kv.GetKnot(knotIndexIn-1) <= t) && (t < kv.GetKnot(knotIndexIn)))
				return knotIndexIn-1;
		}
	}

	// Find i.

#if GETKNOT_BINARYSEARCH

	int front = 0;
	int back = kv.GetLength()-1;

	int maxLoops=100;
	while (maxLoops>0)
	{
		int mid = (back+front)/2;

		float backVal = kv.GetKnot(back);

		if (front==mid || back==mid)
		{
			if (t == backVal)
				return back;  // At. (never actually seen this)
			return front;  // Before
		}

//		float frontVal = kv.GetKnot(front);
		float midVal = kv.GetKnot(mid);

		if (t >= midVal && t <= backVal)
		{
			front = mid;
		}
		else
//		if (t >= frontVal && t <= midVal)
		{
			back = mid;
		}
		maxLoops--;
	}

	Errorf("mayaCurve::GetKnotBeforeOrAtT() failed; if you need a quick fix, switch off GETKNOT_BINARYSEARCH");
#endif

	// Fail-safe.
	int i = 0;

	while (!((kv.GetKnot(i) <= t) && (t < kv.GetKnot(i+1))))
	{
		i++;
	}

	return i;
}

bool mayaCurve::GetLinearSpanAtT(const float fT, Vector3 & vOutP0, Vector3 & vOutP1, float & fOutSpanT) const
{
	if( m_Degree!=1 )
		return false;

	if( fT<0.0f )
		return false;

	if( fT>=1.0f )
	{
		// Special case - end of curve
		fOutSpanT = 1.0f;
		vOutP0.Set(GetCV(m_N-1));
		vOutP1.Set(GetCV(m_N));
	}
	else
	{
		fOutSpanT = fT * GetTRange();

		int iSpan = static_cast<int>(fOutSpanT);
		fOutSpanT -= static_cast<float>(iSpan);

		vOutP0.Set(GetCV(iSpan));
		vOutP1.Set(GetCV(iSpan+1));
	}

	return true;
}

float mayaCurve::Move(float t, float distance, float slope) const
{
	if (slope > SMALL_FLOAT)
	{
		// Increment t from the slope, calculated in GetCurvePointAndSlope(),
		// for constant-speed motion.
		t += distance / slope;
	}
	else
	{
		t = Move(t, distance);
	}

	return t;
}

float mayaCurve::Move(float t, float distance) const
{
	float slope = GetCurveSlope(t, 0.0f);
	if (slope<=SMALL_FLOAT)
		return t;
	t = Move(t, distance, slope);
	return t;
}

float mayaCurve::GetTFromStart(float meters) const
{
	meters = Max(0.0f, meters);
	return Move(0.0f, meters);
}

float mayaCurve::GetTFromEnd(float meters) const
{
	meters = Max(0.0f, meters);
	return Move(1.0f, -meters);
}

float mayaCurve::DistanceTo(const Vector3 &v) const
{
	float t, dist2;
	GetNearestPoint(v, t, dist2);
	return sqrtf(dist2);
}

float mayaCurve::Distance2To(const Vector3 &v) const
{
	float t, dist2;
	GetNearestPoint(v, t, dist2);
	return dist2;
}

Vector3 mayaCurve::GetNearestPoint(const Vector3 &v, float &tOut, float &dist2Out, int segmentSearchSteps, int binarySearchSteps, NearestPointOnLineCallback nearestPointOnLineFn) const
{
	PF_FUNC(NearestPoint);
	Assert(segmentSearchSteps>0 && binarySearchSteps>0);

	if (m_Degree == 1)  // Matches the optimization in GetCurvePoint().
	{
		Vector3 nearestPoint(v);
		dist2Out = FLT_MAX;
		tOut = 0.0f;

		float t1=0.0f, t2;

		float step = 1.0f/static_cast<float>(m_N);

		for (int i=0; i<static_cast<int>(m_N); i++)
		{
			const Vector3 &p1 = GetCV(i);
			const Vector3 &p2 = GetCV(i+1);

			t2 = static_cast<float>(i+1) * step;

			float segt;
			float distance2;
			Vector3 p = nearestPointOnLineFn(p1, p2, v, segt, &distance2);

			if (distance2 < dist2Out)
			{
				nearestPoint = p;
				dist2Out = distance2;
				tOut = Clamp(t1 + (segt * (t2-t1)), 0.0f, 1.0f);
			}

			t1 = t2;
		}

		return nearestPoint;
	}

	// Find the straight line segment closest to v.

	Vector3 p1, p2;
	float t1=0.0f;
	float t2;
	p1 = GetCurvePoint(t1);  // Not necessarily the same as GetCV(0)

	float low=0.0f;
	float high=1.0f;
	Vector3 plow, phigh;

	float step = 1.0f/static_cast<float>(segmentSearchSteps);

	Vector3 nearestPoint(v);
	float dist2 = FLT_MAX;

	for (int i=0; i<segmentSearchSteps; i++)
	{
		t2 = static_cast<float>(i+1) * step;
		p2 = GetCurvePoint(t2);

		float segt;
		float distance2;
		Vector3 p = nearestPointOnLineFn(p1, p2, v, segt, &distance2);

		if (distance2 < dist2)
		{
			nearestPoint = p;
			dist2 = distance2;
			low = t1;
			high = t2;
			plow = p1;
			phigh = p2;
		}
		p1 = p2;
		t1 = t2;
	}
	Assert(dist2 < FLT_MAX);

	float closest = FLT_MAX;
	Vector3 res(ORIGIN);

	float dlow = plow.Dist2(v);
	float dhigh = phigh.Dist2(v);


	// It's possible that the real midpoint between t=low and t=high is not
	// actually very near the position reported by GetCurvePoint((low+high)*0.5f),
	// even for straight curves. (This is not an error, just to do with how the
	// curve is laid out.) We decide the first binary search step by looking at
	// distances to v that are one third and two thirds along the segment,
	// instead of using its endpoints.

	float gap=(high-low)/3.0f;
	Vector3 ot = GetCurvePoint(low+gap);  // One third
	Vector3 tt = GetCurvePoint(high-gap);  // Two thirds
	bool searchDown = v.Dist2(ot) < v.Dist2(tt);

	int lowKnotIndex=-1;
	int highKnotIndex=-1;

	for (int i=0; i<binarySearchSteps; i++)
	{
		float mid=(low+high)*0.5f;
		if (searchDown)
		{
			res = plow;
			high = mid;
			tOut = low;
			dist2Out = dlow;
			phigh = GetCurvePoint(high, i<binarySearchSteps/2 ? -1 : highKnotIndex, highKnotIndex);
			dhigh = phigh.Dist2(v);
		}
		else
		{
			res = phigh;
			low = mid;
			tOut = high;
			dist2Out = dhigh;
			plow = GetCurvePoint(low, i<binarySearchSteps/2 ? -1 : lowKnotIndex, lowKnotIndex);
			dlow = plow.Dist2(v);
		}

		if (dlow<=closest)
		{
			closest=dlow;
			searchDown=true;
		}
		if (dhigh<=closest)
		{
			closest=dhigh;
			searchDown=false;
		}
	}

	return res;
}

Vector3 mayaCurve::GetNearestPointInRange(const Vector3 &v, const float t0, const float t1, const int numSamples, float &bestT, float &bestDistSq) const
{
	PF_FUNC(NearestPointInRange);
	Assert(t1 > t0);

	bestDistSq = FLT_MAX;
	Vector3 bestPointOnCurve;

	const float deltaT = (t1 - t0) / (float)(numSamples);

	Vector3 curSegPos0, curSegPos1, closestSegPos;
	curSegPos0 = GetCurvePoint(t0 >= 0.0f ? t0 : t0 + 1.0f);
	const float finalStartingTValue = t1 - deltaT;
	for(float curT = t0; curT <= finalStartingTValue; curT += deltaT)
	{
		const float kNextT = curT + deltaT;
		curSegPos1 = GetCurvePoint(kNextT >= 0.0f ? kNextT : kNextT + 1.0f);

		float segt;
		closestSegPos = NearestPointOnLine(curSegPos0, curSegPos1, v, segt);

		float distToClosestPointOnSegSq = closestSegPos.Dist2(v);
		if(distToClosestPointOnSegSq < bestDistSq)
		{
			bestPointOnCurve = closestSegPos;
			bestDistSq = distToClosestPointOnSegSq;
			bestT = curT + (deltaT * segt);
		}

		curSegPos0 = curSegPos1;
	}

	Assert(bestDistSq < FLT_MAX);
	return bestPointOnCurve;
}

mayaCurve::~mayaCurve()
{
	delete[] m_pCVs;
	m_pCVs = NULL;
}

mayaCurve::mayaCurve(const mayaCurve &c)
{
	m_TRange = c.m_TRange;
	m_TMax = c.m_TMax;
	m_N = c.m_N;
	m_Form = c.m_Form;
	m_Degree = c.m_Degree;
	m_KnotVector = c.m_KnotVector;

	int count=GetCVCount();
	m_pCVs = count>0 ? rage_new Vector3[count] : NULL;
	for(int i=0; i<count; i++)
		m_pCVs[i] = c.m_pCVs[i];
}

// PURPOSE: Calculate the extents of this curve.
void mayaCurve::CalculateExtents(spdAABB &aabbOutput) const
{
	int count=GetCVCount();

	aabbOutput.Invalidate();

	for (int cv=0; cv<count; cv++)
	{
		aabbOutput.GrowPoint(RCC_VEC3V(GetCV(cv)));
	}
}


const mayaCurve &mayaCurve::operator=(const mayaCurve &c)
{
	m_TRange = c.m_TRange;
	m_TMax = c.m_TMax;
	m_N = c.m_N;
	m_Form = c.m_Form;
	m_Degree = c.m_Degree;
	m_KnotVector = c.m_KnotVector;

	int count=GetCVCount();
	Assert(!m_pCVs);
	m_pCVs = count>0 ? rage_new Vector3[count] : NULL;
	for(int i=0; i<count; i++)
		m_pCVs[i] = c.m_pCVs[i];
	return *this;
}

bool mayaCurve::operator==(const mayaCurve &c) const
{
	if (m_Degree != c.m_Degree)
		return false;
	if (m_N != c.m_N)
		return false;
	if (m_Form != c.m_Form)
		return false;
	if (m_TRange != c.m_TRange)
		return false;
	if (m_TMax != c.m_TMax)
		return false;

	if (m_KnotVector != c.m_KnotVector)
		return false;

	for (int i=0; i<GetCVCount(); i++)
	{
		if (m_pCVs[i].x != c.m_pCVs[i].x)
			return false;
		if (m_pCVs[i].y != c.m_pCVs[i].y)
			return false;
		if (m_pCVs[i].z != c.m_pCVs[i].z)
			return false;
	}
	return true;
}

bool mayaCurve::operator!=(const mayaCurve &c) const
{
	return !operator==(c);
}

bool mayaCurve::operator<(const mayaCurve &c) const
{
	if (m_Degree != c.m_Degree)
		return m_Degree < c.m_Degree;
	if (m_N != c.m_N)
		return m_N < c.m_N;
	if (m_Form != c.m_Form)
		return m_Form < c.m_Form;
	if (m_TRange != c.m_TRange)
		return m_TRange < c.m_TRange;
	if (m_TMax != c.m_TMax)
		return m_TMax < c.m_TMax;

	if (m_KnotVector != c.m_KnotVector)
		return m_KnotVector < c.m_KnotVector;

	for (int i=0; i<GetCVCount(); i++)
	{
		if (m_pCVs[i].x != c.m_pCVs[i].x)
			return m_pCVs[i].x < c.m_pCVs[i].x;
		if (m_pCVs[i].y != c.m_pCVs[i].y)
			return m_pCVs[i].y < c.m_pCVs[i].y;
		if (m_pCVs[i].z != c.m_pCVs[i].z)
			return m_pCVs[i].z < c.m_pCVs[i].z;
	}
	return false;
}


mayaCurve::mayaCurve(datResource &rsc)
: m_KnotVector(rsc)
{
	// Not tested
	rsc.PointerFixup(m_pCVs);
}

#if	__DECLARESTRUCT
void mayaCurve::DeclareStruct(datTypeStruct &s)
{
	if (m_pCVs)
	{
		// We can't use STRUCT_DYNAMIC_ARRAY for m_pCVs because we don't have a
		// member with the precise CV count.
		for (int x=0; x<GetCVCount(); x++)
		{
			::rage::datSwapper(m_pCVs[x]);
		}
	}

	SSTRUCT_BEGIN(mayaCurve)
	SSTRUCT_FIELD_VP(mayaCurve, m_pCVs)
	SSTRUCT_FIELD(mayaCurve, m_KnotVector)
	SSTRUCT_FIELD(mayaCurve, m_TRange)
	SSTRUCT_FIELD(mayaCurve, m_TMax)
	SSTRUCT_FIELD(mayaCurve, m_N)
	SSTRUCT_FIELD(mayaCurve, m_Form)
	SSTRUCT_FIELD(mayaCurve, m_Degree)
	SSTRUCT_END(mayaCurve)
}
#endif

#endif // ENABLE_UNUSED_CURVE_CODE
