// 
// curve/mayacurve.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_MAYACURVE_H
#define CURVE_MAYACURVE_H

#include "mayaknot.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "vector/vector3.h"
#include "vectormath/scalarv.h"

#include "data/base.h"
#include "math/simplemath.h"

#ifndef NON_SPU_ONLY
#	if __SPU
#		define NON_SPU_ONLY(x)
#	else
#		define NON_SPU_ONLY(x) x
#	endif
#endif

namespace rage {

class datResource;
class fiTokenizer;
class spdAABB;
class Color32;
class Vector3;

enum mayaCurveForm
{
	// These should mean the same thing as Maya's 'form' curve attribute.
	// These must match rexObjectGenericNurbsCurve::Type

	FORM_UNKNOWN = -1,
	FORM_OPEN = 0,	// Open curve
	FORM_CLOSED,	// This is tricky to set up in Maya. These are not supported by the current Maya importer/exporter.
	FORM_PERIODIC,	// This is what you get when you choose "Close curve" in Maya. The curve will repeat smoothly.

	FORM_NUMFORMS
};

/*
PURPOSE
	A class for use with Maya-style NURBS curves. It should support
	any degree (including Maya's 1, 2, 3, 5, 7) with any form (open or periodic),
	and with any type of knot vector (with or without multiple end knots).

NOTES
	The control vertex weights (their w components) are ignored. Every test curve
	and every test nurbs patch I've ever looked at has weights of all 1. The
	code to handle weights is commented out, if we ever need it.

	This class is written for generality and flexibility. It is meant to be
	correct rather than fast. For example, there are no pre-calculated constants
	that exploit the fact that the control points may not move. The only
	obvious optimization is in GetCurvePoint() which checks for degree 1 cases.
	Some parts are unoptimized for clarity - PLEASE PRESERVE READABILITY where possible.

	TODO: There's a fair chance that the optimizations that exist in cvCurveNurbs
	may make it into this class at some point for the common degree 1-3 cases.
*/
class mayaCurve
{
#if __SPU
	char	m_VPtrPad[sizeof(datBase)];
#endif

public:
	mayaCurve() : m_pCVs(NULL), m_TRange(0.0f), m_TMax(0.0f), m_N(-1),
		m_Form(FORM_UNKNOWN), m_Degree(0) {}

	mayaCurve(datResource &rsc);
	NON_SPU_ONLY(virtual) ~mayaCurve();

	DECLARE_PLACE(mayaCurve);

	mayaCurve(const mayaCurve &);  // Copy ctor
	const mayaCurve &operator=(const mayaCurve &);  // Assignment
	bool operator==(const mayaCurve &) const;  // Equality
	bool operator!=(const mayaCurve &) const;  // Inequality
	bool operator<(const mayaCurve &) const;   // Less than

	// PURPOSE: Returns the knot vector
	NON_SPU_ONLY(virtual) const mayaKnotVector &GetKnotVector() const { return m_KnotVector; }

	// PURPOSE: Returns the i'th control vertex
	NON_SPU_ONLY(virtual) const Vector3 &GetCV(int i) const { FastAssert(m_pCVs && i>=0 && i<=m_N); return m_pCVs[i]; }

	// PURPOSE: Set a control vertex. Not available for mayaCurveIndexed.
	void SetCV(int i, const Vector3 &v) { FastAssert(m_pCVs && i>=0 && i<=m_N); m_pCVs[i]=v; }

/*
PURPOSE
	Initialize a curve.
PARAMS
	'cv' is the array of control vertices
	'cvCount' is the number of control vertices
	'kv' is a vector of knots as reported by the Maya API
	'kvCount' is the length of the knot vector
	'polyDegree' is the polynomial degree for each segment of the curve
	'form' is the curve form. Currently only FORM_OPEN and FORM_PERIODIC are supported
RETURNS
	'true' if successful
NOTES
	See also, mayaKnotVector::Init()
*/
	bool Init(const Vector3 *cv, int cvCount, const float *kv, int kvCount, int polyDegree, mayaCurveForm form);

	// PURPOSE: Reclaim the memory spent by Init() or Load()
	void Shutdown();

	// PURPOSE: Calculate the extents of this curve. Returns an invalidated AABB if this
	// curve has no CVs.
	void CalculateExtents(spdAABB &aabbOutput) const;

	// PURPOSE: Returns true if Init() or Load() have been called for this curve.
	// NOTES: Calling Shutdown() or Unload() will make a curve invalid again.
	bool IsValid() const { return m_Form!=FORM_UNKNOWN; }

	// PURPOSE: Returns the number of control vertices in this curve.
	int GetCVCount(void) const { return m_N+1; }

	// PURPOSE: Returns the last control vertex.
	const Vector3 &GetLastCV() const { return GetCV(GetCVCount()-1); }

	// PURPOSE: Returns the number of spans on the curve
	// NOTES: Spans are sections of the curve from one edit point to the next.
	int GetSpanCount(void) const { return m_N - (m_Degree-1); }

	// PURPOSE: Returns the number of edit points.
	// NOTES: Edit points are points on the curve where a knot is located.
	int GetEditPointCount(void) const { return GetSpanCount() + 1; }

	// PURPOSE: Returns the distance from the vector v to the nearest point on the curve
	// NOTES: See notes for GetNearestPoint
	float DistanceTo(const Vector3 &v) const;

	// PURPOSE: Returns the squared distance from the vector v to the nearest point on the curve.
	// NOTES: See notes for GetNearestPoint
	float Distance2To(const Vector3 &v) const;

	// Callback type for GetNearestPoint function
	// NOTES: See GetNearestPoint
	typedef Vector3 (*NearestPointOnLineCallback)(const Vector3 &, const Vector3 &, const Vector3 &, float &, float *);

/*
PURPOSE
	Find the nearest point along the curve to the input point.
PARAMS
	'v' is the input point.
RETURNS
	The closest point.
	't' is the t value of the returned point.
	'dist2' is the squared distance to the returned point, from the input point.
NOTES
	This works by searching for a closest straight line segment, followed by a
	binary search along that segment. If you have a very long curve (more than
	a dozen control vertices or so) then 'segmentSearchSteps' should probably
	be increased for more accurate results.
	I don't claim that this method is fantastically fast or anything.
*/
	Vector3 GetNearestPoint(const Vector3 &v, float &t, float &dist2, int segmentSearchSteps=4, int binarySearchSteps=10, NearestPointOnLineCallback nearestPointOnLineFn = NearestPointOnLine) const;

	Vector3 GetNearestPointInRange(const Vector3 &v, const float t0, const float t1, const int numSamples, float &bestT, float &bestDistSq) const;

/*
PURPOSE
	Return a point along the curve.
PARAMS
	't' is the [0, 1] parametric position of the point.
	'slopeIn' should be the slope returned from a previous call to this function, else 0
RETURNS
	The position is returned.
	'tangentOut' receives the tangent vector.
	'slopeOut' is the slope to feed back into this function when evaluating a
	point slightly further along the curve. It is also the slope to pass in to
	Move().
*/
	Vector3 GetCurvePoint(float t) const;
	Vector3 GetCurvePointAndSlope(float t, float slopeIn, float &slopeOut) const;
	Vector3 GetCurvePointAndTangent(float t, Vector3 &tangentOut) const;
	Vector3 GetCurvePointSlopeAndTangent(float t, float slopeIn, float &slopeOut, Vector3 &tangentOut) const;

	// PURPOSE: Use these in place of their simpler namesakes for better performance, where 't' does not vary too much between calls.
	// PARAMS: Feed 'knotIndexOut' back into 'knotIndexIn' on subsequent calls, or -1 if you don't have it yet.
	Vector3 GetCurvePoint(float t, int knotIndexIn, int &knotIndexOut) const;
	Vector3 GetCurvePointAndSlope(float t, float slopeIn, float &slopeOut, int knotIndexIn, int &knotIndexOut) const;
	Vector3 GetCurvePointSlopeAndTangent(float t, float slopeIn, float &slopeOut, Vector3 &tangentOut, int knotIndexIn, int &knotIndexOut) const;
	Vector3 GetCurvePointAndTangent(float t, Vector3 &tangent, int knotIndexIn, int &knotIndexOut) const;

	// PURPOSE: Find the slope at 't'
	// PARAMS: 'slopeIn' will provide more accurate results
	// RETURNS: The slope
	float GetCurveSlope(float t, float slopeIn = 0.0f) const;


	// PURPOSE: Find the tangent at 't'
	// RETURNS: The (non-normalized) tangent
	Vector3 GetCurveTangent(float t) const;

/*
PURPOSE
	Move along a curve by a specified distance.
PARAMS
	't' is the [0, 1] parametric position of the point
	'distance' is the signed distance to move
	'slope' is the value returned from an earlier call to GetCurvePointAndSlope()
RETURNS
	The new value of t
NOTES
	The idea is that 'distance' is never very big; the return value will become
	less accurate the greater the distance is, and the more wiggly the curve is.
	The return value may be outside the range [0, 1]. This may be used to detect
	when a point has moved off the end of a curve.

	In case it isn't obvious, you probably want code that looks like this:

	<CODE>
	float m_T;
	float m_Slope=0.0f;
	awMayaCurve *m_pCurve;

	// Each update:
	Vector3 pos = m_pCurve->GetCurvePointAndSlope(m_T, m_Slope, m_Slope);
	m_T = m_pCurve->Move(m_T, distanceToMove, m_Slope);
	</CODE>
*/
	float Move(float t, float distance, float slope) const;


/*
PURPOSE
	Move along a curve by a specified distance.
PARAMS
	't' is the [0, 1] parametric position of the point
	'distance' is the signed the distance to move
NOTES
	This is a lot more expensive than the other Move() function.
*/
	float Move(float t, float distance) const;


	// PURPOSE: Returns the [0,1] parameter as measured from the beginning of the curve
	float GetTFromStart(float meters) const;

	// PURPOSE: Returns the [0,1] parameter as measured from the end of the curve
	float GetTFromEnd(float meters) const;

	// NOTES: The first and last edit points will be the same in the case of a periodic curve
	Vector3 GetEditPoint(int i) const;

	// PURPOSE: Returns the last edit point
	Vector3 GetLastEditPoint(void) const { return GetEditPoint( GetEditPointCount()-1 ); }

	// PURPOSE: Returns the order of the curve (quadratic curve is 3rd order, cubic is 4th order, etc.)
	int GetOrder(void) const { return m_Degree+1; }

	// PURPOSE: Returns the polynomial degree of the curve (quadratic curve is 2nd degree, cubic is 3rd degree, etc.)
	int GetDegree(void) const { return m_Degree; }

	// PURPOSE: Returns the form of the curve (typically either FORM_OPEN or FORM_PERIODIC)
	mayaCurveForm GetForm(void) const { return static_cast<mayaCurveForm>(m_Form); }

#if (__DEV || __BANK) && !__SPU
/*
PURPOSE
	Curve visualization
PARAMS
	curve - draw the curve itself
	cvs - draw a box at every control vertex location
	eps - draw a mark at every edit point (knot) location
	drawArrow - draw an arrow in the middle of the curve pointing in the t=0 -> t=1 direction.
*/
	void Draw(bool curve, bool cvs, bool eps, bool drawArrow) const;

	// NOTES: 'steps' is ignored for degree 1 curves
	void DrawCurve(Color32 color, int steps) const;
	void DrawControlVertices(Color32 color) const;
	void DrawEditPoints(Color32 color) const;
	void DrawArrow(Color32 color) const;
	void DrawArrow(Color32 color, const Matrix34& camMatrix, const float arrowSize = 0.5f) const;
	void DrawKnotVector(Color32 color, bool drawEditPoints) const;
#endif

	// PURPOSE: Returns the length of the knot vector
	int GetKnotCount(void) const { return GetKnotVector().GetLength(); }

	// PURPOSE: Returns the i'th knot
	float GetKnot(int i) const { return GetKnotVector().GetKnot(i); }

/*
PURPOSE
	Load a curve.
PARAMS
	If 'loadControlVertices' is true then control vertices are expected
	If 'loadKnotVector' is true then a knot vector is expected
NOTES
	If you are using mayaCurveIndexed then 'loadControlVertices' and
	'loadKnotVector' should probably be false.
*/
	void Load(fiTokenizer &tok, bool loadControlVertices, bool loadKnotVector);


	// PURPOSE: Unload a curve; reclaim the memory spent by Load().
	void Unload() { Shutdown(); }


/*
PURPOSE
	Save a curve.
PARAMS
	If 'saveControlVertices' is true then the knot vector will be written out
	If 'saveKnotVector' is true then the knot vector will be written out
NOTES
	Exporters should use this function.

	If your game is using mayaCurveIndexed then 'saveControlVertices' and
	'saveKnotVector' should probably be false. In this case your exporter
	should write out an array of control vertices and knot vectors separately
	by other means; see mayaCurveIndexed.
*/
	void Save(fiTokenizer &tok, bool saveControlVertices, bool saveKnotVector) const;


	// PURPOSE: Returns the index into the knot vector for the specified t value
	// PARAMS:
	//		t - The [0,1] parametric position along the curve.
	//		'knotIndexIn' if you know what it likely is, to avoid searching the knot vector.
	int GetKnotBeforeOrAtT(float t, int knotIndexIn=-1) const;

	// PURPOSE: Given a degree-1 curve, find the span endpoints for a given T value
	// PARAMS:
	//	fT - The T value
	//	vOutP0 - (output parameter) the first endpoint of the span
	//	vOutP1 - (output parameter) the second endpoint of the span
	//	vOutSpanT - (output parameter) the converted T value along p1 -> p2
	// RETURNS: True if successful, false otherwise.
	bool GetLinearSpanAtT(const float fT, Vector3 & vOutP0, Vector3 & vOutP1, float & fOutSpanT) const;

	// PURPOSE: Returns the real t-range of the curve.
	float GetTRange(void) const { return m_TRange; }

	// PURPOSE: Returns the maximum real t value allowed.
	float GetTMax(void) const { return m_TMax; }

/*
PURPOSE
	Returns the closest approach to 'testPoint' along p1p2
PARAMS
	'p1' is the first point on the line
	'p2' is the second point on the line
	'testPoint' is the point to measure distance from
	't' is the [0, 1] output parametric value of the point
	'distance2' is the distance between the nearest point and testPoint
*/
	static Vector3 NearestPointOnLine(const Vector3 &p1, const Vector3 &p2, const Vector3 &testPoint, float &tOut, float *distance2 = NULL);

/*
PURPOSE
	Returns the closest approach to 'testPoint' along p1p2 in XZ space
PARAMS
	'p1' is the first point on the line
	'p2' is the second point on the line
	'testPoint' is the point to measure distance from
	't' is the [0, 1] output parametric value of the point
	'distance2' is the distance between the nearest point and testPoint in XZ
*/
	static Vector3 NearestPointOnLineXZ(const Vector3 &p1, const Vector3 &p2, const Vector3 &testPoint, float &tOut, float *distance2 = NULL);

#if	__DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif


	// NOTES: Deprecated. Do not use this.
	void LoadFromCurvesFile(fiTokenizer &tok);

	// PURPOSE: Fills out TMax and TRange based on the knot vector
	// NOTES: Call this if you're editing the curve's knot vector manually
	void ComputeParametricRange(void);

protected:

/*
PURPOSE
	Returns the weight on a particular CV at a given t value
PARAMS:
	'i' is the CV to get the weight for
	'j' is the order of the curve
	't' is the [0,1] parametric position of the point
	'kv' is the knot vector
	'slopeOut' is the slope
*/
	float GetBlendFunction(int i, int j, float t, const mayaKnotVector &kv) const;
	float GetBlendFunction(int i, int j, float t, const mayaKnotVector &kv, float &slopeOut) const;

	template<int _Order> float GetBlendFunction_Internal(int i, float t, const mayaKnotVector& kv) const;

	// Optimized specializations.
	// Note that we only bothered with the cases _Order==3&4 since they were used most.
	// 1&2 should also be SIMD'ized, if they ever get used!
	static ScalarV_Out GetBlendFunction_Order3(int i, const float& t, const mayaKnotVector& kv);
	static ScalarV_Out GetBlendFunction_Order4(int i, const float& t, const mayaKnotVector& kv);

/*
PURPOSE
	Returns the slope of the blending function for a particular CV and t value
PARAMS
	'i' is the CV to get the slope for
	'j' is the order of the curve
	't' is the [0,1] parametric position of the point
	'kv' is the knot vector
RETURNS
	The slope
*/
	float GetSlope(int i, int j, float t, const mayaKnotVector &kv) const; 

/*
	PURPOSE
	While parsing the file, override this function to optionally, locally store the name of the curve;
	PARAMS
	'curveName' the name of the curve being parsed
	*/
	NON_SPU_ONLY(virtual) void ProcessNameToken(const char* /*curveName*/)	{;}

	/*
	PURPOSE
	While parsing the file, override this function to optionally, locally store the bone index of the curve;
	PARAMS
	'boneIndex' the bone index referencing the skel file.
	*/
	NON_SPU_ONLY(virtual) void ProcessBoneIndex(int /*boneIndex*/)	{;}

/*
	It's easy to confuse the "polynomial degree" with the "degree of freedom" and
	the "order" of a curve. Maya uses polynomial degrees. I think it's like this:

	Name     Order  DOF  p.degree  min.CVs
	--------------------------------------
	Constant   1     1      0        1
	Linear     2     2      1*       2
	Quadratic  3     3      2*       3
	Cubic      4     4      3*       4
	Quartic    5     5      4        5

	*The common CV/EP Tool ones
*/

	Vector3 *m_pCVs;  // Pointer to control vertices (not used with mayaCurveIndexed)
	mayaKnotVector m_KnotVector;  // The knot vector (not used with mayaCurveIndexed)

	float m_TRange;  // The real t-range of the curve.
	// m_TMax is now redundant with m_TRange, there's no need for it any more.  Remove it in the next pass.
	float m_TMax;    // The maximum allowed real t value. Less than or equal to TRange
	s16 m_N;         // CV count - 1.
	s8 m_Form;       // One of mayaCurveForm. Currently only FORM_OPEN and FORM_PERIODIC are supported.
	s8 m_Degree;     // Polynomial degree of each span.
};

inline Vector3 mayaCurve::NearestPointOnLine(const Vector3 &p1, const Vector3 &p2, const Vector3 &testPoint, float &t, float *distance2)
{
	Vector3 a = p2 - p1;
	float m2 = a.Mag2();

	if (m2 != 0.0f)
	{
		Vector3 b = testPoint - p1;
		t = a.Dot(b);
		t /= m2;
		t = Clamp(t, 0.0f, 1.0f);
	}
	else
	{
		t = 0.0f;
	}

	Vector3 nearestPoint(p1 + (a * t));
	if (distance2)
	{
		Vector3 d = nearestPoint - testPoint;
		*distance2 = d.Mag2();
	}
	return nearestPoint;
}

inline Vector3 mayaCurve::NearestPointOnLineXZ(const Vector3 &p1, const Vector3 &p2, const Vector3 &testPoint, float &t, float *distance2)
{
	Vector3 a = p2 - p1;
	float m2 = a.x*a.x+a.z*a.z;

	if (m2 != 0.0f)
	{
		Vector3 b = testPoint - p1;
		t = a.x*b.x+a.z*b.z;
		t /= m2;
		t = Clamp(t, 0.0f, 1.0f);
	}
	else
	{
		t = 0.0f;
	}

	Vector3 nearestPoint(p1 + (a * t));
	if (distance2)
	{
		Vector3 d = nearestPoint - testPoint;
		d.y = 0.0f;
		*distance2 = d.Mag2();
	}
	return nearestPoint;
}

}  // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

#endif  // CURVE_MAYACURVE_H
