//
// phcore/segment.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_SEGMENT_H
#define PHCORE_SEGMENT_H


#include "vector/matrix34.h"
#include "vectormath/classes.h"


namespace rage {


//=============================================================================
// phSegment
// PURPOSE
//   phSegment defines a line segment between two Vector3 points.  Segments
//   are often used in intersections tests against phBound objects.
// <FLAG Component>
//
class phSegment
{
public:
	// PURPOSE: Default constructor.  Segment endpoints are uninitialized.
	phSegment() { }

	// PURPOSE: Construct with the given beginning and ending points of this segment.
	// PARAMS:
	//	a - the beginning point of this segment
	//	b - the ending point of this segment
	phSegment (const Vector3& a, const Vector3& b) : A(a), B(b) { }

	// PURPOSE: Set the beginning and ending points of this segment.
	// PARAMS:
	//	a - the new beginning point of this segment
	//	b - the new ending point of this segment
	void Set (const Vector3& a, const Vector3& b);

	// PURPOSE: Compute the length of this segment.
	// RETURN: the length of this segment
	float GetLength () const;

	// PURPOSE: Compute the squared length of this segment.
	// RETURN: the squared length of this segment
	float GetLength2 () const;

	// PURPOSE: Compute the inverse length of this segment.
	// RETURN: the inverse length of this segment
	float GetInvLength () const;

	// PURPOSE: Make this segment the given world segment transformed into the given matrix's coordinate system (untransform by the given matrix).
	// PARAMS:
	//	worldSegment - the segment in world coordinates from which to compute this segment
	//	objectMatrix - the coordinate system into which to transform the given world segment to get this segment
	void Localize (const phSegment& worldSegment, const Matrix34& objectMatrix);

	// PURPOSE: Put this segment into the given matrix's coordinate system (untransform by the given matrix).
	// PARAMS:
	//	objectMatrix - the coordinate system into which to transform this segment (untransform by this matrix)
	void Localize (const Matrix34& objectMatrix);

	// PURPOSE: Transform this segment from the given matrix's coordinate system (transform by the given matrix).
	// PARAMS:
	//	objectMatrix - the coordinate system from which to transform this segment (transform by this matrix)
	void Transform (const Matrix34& objectMatrix);

public:
	//=========================================================================
	// Data

	// PURPOSE: The first endpoint of the segment; the start if the segment is directed.
	Vector3 A;

	// PURPOSE: The second endpoint of the segment; the end if the segment is directed.
	Vector3 B;
};


//=============================================================================
// Implementations

inline void phSegment::Set (const Vector3& a,const Vector3& b)
{
	A.Set(a);
	B.Set(b);
}


inline float phSegment::GetLength () const
{
	return A.Dist(B);
}


inline float phSegment::GetLength2 () const
{
	return A.Dist2(B);
}


inline float phSegment::GetInvLength () const
{
	return A.InvDist(B);
}


inline void phSegment::Localize (const phSegment& worldSegment, const Matrix34& objectMatrix)
{
	objectMatrix.UnTransform(worldSegment.A,A);
	objectMatrix.UnTransform(worldSegment.B,B);
}


inline void phSegment::Localize (const Matrix34& objectMatrix)
{
	objectMatrix.UnTransform(A);
	objectMatrix.UnTransform(B);
}


inline void phSegment::Transform (const Matrix34& objectMatrix)
{
	objectMatrix.Transform(A);
	objectMatrix.Transform(B);
}


//=============================================================================
// phSegment
// PURPOSE
//   phSegment defines a line segment between two Vector3 points.  Segments
//   are often used in intersections tests against phBound objects.
// <FLAG Component>
//
class phSegmentV
{
public:
	// PURPOSE: Default constructor.  Segment endpoints are uninitialized.
	phSegmentV () { }

	// PURPOSE: Construct with the given beginning and ending points of this segment.
	// PARAMS:
	//	start - the beginning point of this segment
	//	end - the ending point of this segment
	phSegmentV (Vec3V_In start, Vec3V_In end) : m_Start(start), m_End(end) { }

	// PURPOSE: Set the beginning and ending points of this segment.
	// PARAMS:
	//	start - the new beginning point of this segment
	//	end - the new ending point of this segment
	void Set (Vec3V_In start, Vec3V_In end);

	// PURPOSE: Set the beginning and ending points of this segment.
	// PARAMS:
	//	segment - the new beginning and end points of this segment
	void Set (const phSegmentV& segment);

	// PURPOSE: Get the starting point.
	// RETURN:	the starting point of this segment
	Vec3V_Out GetStart () const;

	// PURPOSE: Get the ending point.
	// RETURN:	the ending point of this segment
	Vec3V_Out GetEnd () const;

	// PURPOSE: Compute the length of this segment.
	// RETURN: the length of this segment
	ScalarV_Out GetLength () const;

	// PURPOSE: Compute the squared length of this segment.
	// RETURN: the squared length of this segment
	ScalarV_Out GetLength2 () const;

	// PURPOSE: Compute the inverse length of this segment.
	// RETURN: the inverse length of this segment
	ScalarV_Out GetInvLength () const;

	// PURPOSE: Make this segment the given world segment transformed into the given matrix's coordinate system (untransform by the given matrix).
	// PARAMS:
	//	worldSegment - the segment in world coordinates from which to compute this segment
	//	objectMatrix - the coordinate system into which to transform the given world segment to get this segment
	void Localize (const phSegmentV& worldSegment, Mat34V_In objectMatrix);

	// PURPOSE: Put this segment into the given matrix's coordinate system (untransform by the given matrix).
	// PARAMS:
	//	objectMatrix - the coordinate system into which to transform this segment (untransform by this matrix)
	void Localize (Mat34V_In objectMatrix);

	// PURPOSE: Transform this segment from the given matrix's coordinate system (transform by the given matrix).
	// PARAMS:
	//	objectMatrix - the coordinate system from which to transform this segment (transform by this matrix)
	void Transform (Mat34V_In objectMatrix);

protected:
	//=========================================================================
	// Data

	// PURPOSE: The first endpoint of the segment; the start if the segment is directed.
	Vec3V m_Start;

	// PURPOSE: The second endpoint of the segment; the end if the segment is directed.
	Vec3V m_End;
};


//=============================================================================
// Implementations

inline void phSegmentV::Set (Vec3V_In start, Vec3V_In end)
{
	m_Start = start;
	m_End = end;
}

inline void phSegmentV::Set (const phSegmentV& segment)
{
	m_Start = segment.GetStart();
	m_End = segment.GetEnd();
}

inline Vec3V_Out phSegmentV::GetStart () const
{
	return m_Start;
}

inline Vec3V_Out phSegmentV::GetEnd () const
{
	return m_End;
}

inline ScalarV_Out phSegmentV::GetLength () const
{
	return Dist(m_Start,m_End);
}


inline ScalarV_Out phSegmentV::GetLength2 () const
{
	return DistSquared(m_Start,m_End);
}


inline ScalarV_Out phSegmentV::GetInvLength () const
{
	return InvDistSafe(m_Start,m_End);
}


inline void phSegmentV::Localize (const phSegmentV& worldSegment, Mat34V_In objectMatrix)
{
	m_Start = UnTransformFull(objectMatrix,worldSegment.GetStart());
	m_End = UnTransformFull(objectMatrix,worldSegment.GetEnd());
}


inline void phSegmentV::Localize (Mat34V_In objectMatrix)
{
	m_Start = UnTransformFull(objectMatrix,m_Start);
	m_End = UnTransformFull(objectMatrix,m_End);
}


inline void phSegmentV::Transform (Mat34V_In objectMatrix)
{
	// VMath::AoS namespace specification is necessary to avoid confusion with phSegmentV::Transform.
	m_Start = rage::Transform(objectMatrix,m_Start);
	m_End = rage::Transform(objectMatrix,m_End);
}


} // namespace rage

#endif // PHCORE_SEGMENT_H
