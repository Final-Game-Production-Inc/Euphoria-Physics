// 
// curve/curvecubic.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_CURVECUBIC_H
#define CURVE_CURVECUBIC_H

#include "curve.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "data/resourcehelpers.h"

namespace rage {

class datResource;

//=============================================================================
// cvCurve
//
// PURPOSE
//   cvCurve is a base class for any curve defined by a set of _Vector points. 
//   Virtual accessors are provided, and derived curve classes can override them 
//   to control how the curve is defined by the set of points.  
//   Examples of derived classes are cvCurveNurbs and cvCurveCatrom.
// NOTES
//   - No default implementation is provided for the base curve class methods. 
//     Implementation in derived classes is required. 
// <FLAG Component>
//
template<class _Vector> class cvCurveCubic : public cvCurve<_Vector>
{
public:
	cvCurveCubic () { }

	cvCurveCubic (datResource & rsc) : cvCurve<_Vector>(rsc) { }

	virtual ~cvCurveCubic () { }

	//=========================================================================
	// Evaluation

	// PURPOSE: Sets pos to the point on the curve at (seg,t).
	virtual float SolveSegment (_Vector& posOut, int seg, float t, float slope=0.0f, _Vector* direction=0);
};

template<class _Vector>
float cvCurveCubic<_Vector>::SolveSegment (_Vector& posOut, int seg, float t, float UNUSED_PARAM(slope), _Vector* UNUSED_PARAM(direction))
{
	int n = cvCurve<_Vector>::GetNumVertices();

	if (n < 2)
		return 0;

	if (t == 0 && seg > 0)
		t = 1, seg--;

	FastAssert(seg >= 0 && seg < (this->m_Looping ? n : n - 1));

	/*
	t *= n;
	seg = int(t);
	t -= seg;
	*/

	const _Vector& y1 = this->m_Vertices[seg];
	const _Vector& y2 = (seg < n - 1) ? this->m_Vertices[seg + 1] : this->m_Vertices[0];

	const _Vector& y0 = (seg > 0) ? this->m_Vertices[seg - 1] : this->m_Looping ? this->m_Vertices[n - 1] : (y1 - (y2 - y1));
	const _Vector& y3 = (seg < n - 2) ? this->m_Vertices[seg + 2] : this->m_Looping ? this->m_Vertices[(seg == n - 1) ? 1 : 0] : (y2 + (y2 - y1));

	float t2 = t * t;
	float t3 = t * t2;

	const _Vector& a0 = y3 - y2 - y0 + y1;
	const _Vector& a1 = y0 - y1 - a0;
	const _Vector& a2 = y2 - y0;
	const _Vector& a3 = y1;

	posOut = a0 * t3 + a1 * t2 + a2 * t + a3;

	return 0;
}

} // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

#endif
