// 
// curve/curvecatrom.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef CURVE_CURVECATROM_H
#define CURVE_CURVECATROM_H

// #include "curve.h" Do not include the curve.h this has to be included before we get here.
// This is done for PS3 to compile

#if ENABLE_UNUSED_CURVE_CODE

namespace rage {

class Vector3;
class Matrix34;
class Matrix44;
class datResource;


// PURPOSE
//   cvCurveCatRom defines a type of spline curve. It is derived from cvCurve, which provides an
//   interface for spline curves.
// <FLAG Component>
class cvCurveCatRom : public cvCurve<Vector3>
{
public:
	cvCurveCatRom();

    cvCurveCatRom(datResource &rsc);

	~cvCurveCatRom();

	DECLARE_PLACE	(cvCurveCatRom);

#if	__DECLARESTRUCT
	void				DeclareStruct	(datTypeStruct &s);
#endif
	
	virtual float SolveSegment (Vector3 &pos, int seg, float t, float slope=0.0f, Vector3* direction=0);

	float SolveDerivatives (Vector3 &vel, Vector3 &accl, int seg, float t) const;

	float CalcCurvature(int seg, float t) const;

	float CalcXZCurvature(int seg, float t) const;

	void SetInitialTangent(const Vector3& dir)	{ m_InitialTangent = dir; }

	void SetFinalTangent(const Vector3& dir)		{ m_FinalTangent = dir; }

	virtual void Move (int &seg, float &t, float dist, float slope=0.0f) const;

	void SolveSegmentFast(Vector3 &pos, Vector3& tan, int seg, float t) const;

protected:
	virtual void AllocateVerticesDerived();

	// Call after initializing the verts:
	void GenerateCubics();

	atArray<Matrix34> m_Cubics;

	static const Matrix44 sm_CatRomMatrix;

	Vector3 m_InitialTangent;

	Vector3 m_FinalTangent;

private:
    virtual void PostInitVerts();
};

} /// namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

#endif
