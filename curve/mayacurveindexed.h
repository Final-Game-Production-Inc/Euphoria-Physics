// 
// curve/mayacurveindexed.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_MAYACURVEINDEXED_H
#define CURVE_MAYACURVEINDEXED_H

#include "mayacurve.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "data/struct.h"

namespace rage {

class datResource;

/*
PURPOSE
	A way of sharing control vertices and knot vectors between curves.

	The idea is that your game uses this class if there are a lot of fixed
	curves in memory (hundreds or more.) Tools and plugins probably want the
	flexibility of using mayaCurve by itself, where each curve gets its own
	data, and CVs can be moved.
NOTES
	Use SetControlVertexArray() and SetKnotVectorArray() for the index lookups
	to succeed.

	Clients need to do some work to set up the array pointed to by m_pCVIndexes
	which provides opportunity to share (weld) control vertices so that only
	the unique ones are stored.

	Typically a client should also store only the unique knot vectors for the
	curve set and index each with m_KVIndex.

	TODO: This class inherits a knot vector and control vertex pointer from
	mayaCurve which are unused. Maybe they could be unionized with the data in here.
*/
class mayaCurveIndexed : public mayaCurve
{
public:
	mayaCurveIndexed() : m_pCVIndexes(NULL), m_KVIndex(0) {}
	mayaCurveIndexed(datResource &rsc);

	virtual ~mayaCurveIndexed() {}

	// PURPOSE: Set the base for the contents of m_pCVIndexes to look at
	static void SetControlVertexArray(const Vector3 *array) { s_pControlVertices = array; }
	static const Vector3 *GetControlVertexArray() { return s_pControlVertices; }

	// PURPOSE: Set the base for m_KVIndex to look at
	static void SetKnotVectorArray(const mayaKnotVector *array) { s_pKnotVectors = array; }
	static const mayaKnotVector *GetKnotVectorArray() { return s_pKnotVectors; }

	// PURPOSE: Returns the knot vector
	virtual const mayaKnotVector &GetKnotVector() const { return s_pKnotVectors[ m_KVIndex ]; }

	// PURPOSE: Returns a control vertex
	virtual const Vector3 &GetCV(int i) const { return s_pControlVertices[ m_pCVIndexes[i] ]; }

	// PURPOSE: Returns a control vertex index
	int GetCVIndex(int i) const { FastAssert(i>=0 && i<=m_N); return m_pCVIndexes[i]; }


	// NOTES: Left public for clients to set up
	u16 *m_pCVIndexes;  // Pointer to an array of indexes, which index an array of control vertices
	u16 m_KVIndex;      // Index into an array of knot vectors

	datPadding<2> m_Padding;

#if	__DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

protected:
	static const Vector3 *s_pControlVertices;
	static const mayaKnotVector *s_pKnotVectors;
};

}  // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

#endif  // CURVE_MAYACURVEINDEXED_H
