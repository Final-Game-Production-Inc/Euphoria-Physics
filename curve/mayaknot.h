// 
// curve/mayaknot.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_MAYAKNOT_H
#define CURVE_MAYAKNOT_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_CURVE_CODE

#include "atl/array.h"
#include "vectormath/classes.h"

namespace rage {

class datResource;
class fiTokenizer;

/*
PURPOSE
   A class for containing Maya-style knot vectors.

NOTES
	The knot vector stored actually contains two additional elements to the
	Maya version: one at the beginning, and one at the end. Somehow Maya
	doesn't require them. Some simple examples:

	<CODE>
	    0,0,0,1,1,1        Maya knot vector (multiple end knots)
	  0,0,0,0,1,1,1,1      mayaKnotVector

	   -2,-1,0,1,2,3       Maya knot vector (no multiple end knots)
	-3,-2,-1,0,1,2,3,4     mayaKnotVector
	</CODE>

	What is a knot vector? It describes the parametric value of a curve at each
	"edit point" (Maya nomenclature), "knot" or "join point". A 4-CV curve has
	two edit points, one at the beginning and one at the end of the curve.

	A 5-CV curve has three edit points, one at the beginning, the middle and
	the end. To a human being, it is the middle portion of the knot vector that
	is usually of interest:

	<CODE>
	      +------ t=0 at the first edit point
	      |
	      |   +-- t=2 at the third edit point
	      |   |
	      V   V
	0,0,0,0,1,2,2,2,2   5-CV, degree-3 curve (with three edit points)
	</CODE>

	Knot vectors may contain non-integer values; this expands or contracts the
	parametric distance between edit points, hence "non-uniform" curves.

	Functions such as mayaCurve::GetCurvePoint(float t) always expect a t in
	the range [0, 1]; the true range of t-values in the curve is abstracted away.
*/
class mayaKnotVector
{
	friend class mayaCurve; // To allow access to internal members for DMA purposes.

public:
	mayaKnotVector() : m_pKnots(NULL), m_Length(0) {}
	mayaKnotVector(datResource &rsc);
	virtual ~mayaKnotVector();

	DECLARE_PLACE(mayaKnotVector);

	mayaKnotVector(const mayaKnotVector &);  // Copy ctor
	const mayaKnotVector &operator=(const mayaKnotVector &);  // Assignment
	bool operator==(const mayaKnotVector &) const;  // Equality
	bool operator!=(const mayaKnotVector &) const;  // Inequality
	bool operator<(const mayaKnotVector &) const;   // Less than

	// PURPOSE: Returns the i'th knot
//	float operator[](int i) const { FastAssert(i>=0 && i<m_Length && m_pKnots); return m_pKnots[i]; }

	// PURPOSE: Returns the i'th knot
	float GetKnot(int i) const { FastAssert(i>=0 && i<m_Length && m_pKnots); return m_pKnots[i]; }
	
	__forceinline
	ScalarV_Out GetKnotV(int i) const
	{
		FastAssert(i>=0 && i<m_Length && m_pKnots);
		return ScalarVFromF32(m_pKnots[i]);
	}


/*
PURPOSE
	Initialize the knot vector.
PARAMS
	'kv' is knot vector as reported by the Maya API
	'kvCount' is the number of knots in the vector
	'n' is the number of control vertices - 1
	'order' is the polynomial order of each curve span, or degree+1
NOTES
	Of course you can invent the knot vector to pass in yourself. For example,
	for curves with "multiple end knots" and "uniform knot spacing":

	<CODE>
	Degree   # Control Verts  Maya Knot Vector
	  1              2             (0,1)
	  2              3             (0,0,1,1)
	  3              4             (0,0,0,1,1,1)
	</CODE>
*/
	void Init(const float *kv, int kvCount, int n, int order);

	// PURPOSE: Reclaim the memory spent by Init() or Load()
	void Shutdown();

	// PURPOSE: Write a representation of this vector
	void Save(fiTokenizer &tok) const;

	// PURPOSE: Loads a knot vector
	void Load(fiTokenizer &tok);

	// PURPOSE: Unload a knot vector
	void Unload() { Shutdown(); }

	// PURPOSE: Returns the length of the vector
	int GetLength(void) const { return m_Length; }

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	void Normalize(int cvCount, int degree, int editPointCount) const;

protected:
	float *m_pKnots;  // The array of knots.
	int m_Length;     // The number of knots in the array.
};

}  // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

#endif  // CURVE_MAYAKNOT_H
