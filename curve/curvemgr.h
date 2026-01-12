// 
// curve/curvemgr.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_CURVEMGR_H
#define CURVE_CURVEMGR_H

#include "curve.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "data/base.h"

namespace rage {

class fiAsciiTokenizer;

// PURPOSE
//   cvCurveMgr loads and controls access for a set of curves, so they can be used in multiple places
//   without having multiple copies of the same curve.
// <FLAG Component>
class cvCurveMgr : public cvCurveMgrBase
{
public:
	static void CreateInstance  ();
	static void DestroyInstance ();
    static void SetInstance(cvCurveMgr * pCurveMgr);

	cvCurveMgr ();
    cvCurveMgr (datResource &rsc);
   ~cvCurveMgr ();

	DECLARE_PLACE	(cvCurveMgr);

#if	__DECLARESTRUCT
	void				DeclareStruct	(rage::datTypeStruct &s);
#endif

    // The save and load functions
	bool Save(const char* fileName="", const char* fileExt="");
    bool Save(fiAsciiTokenizer & token);
    bool Load(const char* fileName="", const char* fileExt="");
    bool Load(fiAsciiTokenizer & token);

    // Adding curves does not support resources be careful
    cvCurve<Vector3> * AddCurve(fiAsciiTokenizer & token);

    // Accessors
    int     GetCurveCount()     const { return m_CurveArray.GetCount(); }
    cvCurve<Vector3> * GetCurve(int curve) const { return m_CurveArray[curve]; }
    int     GetCurveIndex(const cvCurve<Vector3> * pCurve) const { for ( int i=0; i<m_CurveArray.GetCount(); i++) if (pCurve == m_CurveArray[i]) return i; return -1; }

protected:

    // Allocate a curve of the given type
    cvCurve<Vector3> * CreateCurve(int curveType);

    atArray <datOwner <cvCurve<Vector3> > > m_CurveArray;

	// Flag for resourcing
	bool m_bResourced;

	// PURPOSE: Pad to fill up the remaining bytes
	u8 pad[3];

};

} // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

////////////////////////////////////////////////////////////////////////////////
#endif
