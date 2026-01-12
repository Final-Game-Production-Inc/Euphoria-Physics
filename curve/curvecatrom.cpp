//
// curve/curvecatrom.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "curve.h"

#if ENABLE_UNUSED_CURVE_CODE

#include "vector/matrix44.h"

namespace rage {

const Matrix44 cvCurveCatRom::sm_CatRomMatrix
	(-0.5f,  1.5f, -1.5f,  0.5f, 
	  1.0f, -2.5f,  2.0f, -0.5f,
	 -0.5f,  0.0f,  0.5f,  0.0f,
	  0.0f,  1.0f,  0.0f,  0.0f);

//=============================================================================

cvCurveCatRom::cvCurveCatRom()
{
    m_Type = CURVE_TYPE_CATROM;
	m_InitialTangent.Zero();
	m_FinalTangent.Zero();
}


cvCurveCatRom::~cvCurveCatRom()
{
}


void cvCurveCatRom::AllocateVerticesDerived()
{
	Assert(m_Cubics.GetCapacity()==0);
	m_Cubics.Reserve(m_Vertices.GetCapacity());
	m_Cubics.Resize(0);
}


void cvCurveCatRom::PostInitVerts()
{
	GenerateCubics();
}


float cvCurveCatRom::SolveSegment(Vector3& posOut, int seg, float t, float, Vector3* tangentOut)
{
	Assert(m_Cubics.GetCount()==m_Vertices.GetCount());
	Assert(seg>=0 && seg<m_Vertices.GetCount() - (m_Looping ? 0 : 1));
	Vector3 v(t*t*t,t*t,t);
	m_Cubics[seg].Transform(v,posOut);
	if (tangentOut)
	{
		v.Set(3.0f*t*t,2.0f*t,1.0f);
		m_Cubics[seg].Transform3x3(v, *tangentOut);
		tangentOut->Normalize();
	}
	return 0.0f;
}


void cvCurveCatRom::SolveSegmentFast(Vector3 &pos, Vector3& tan, int seg, float t) const
{
	float t2 = t * t;

	pos.Set(t*t2,t2,t);
	m_Cubics[seg].Transform(pos);

	tan.Set(3.0f*t2,2.0f*t,1.0f);
	m_Cubics[seg].Transform3x3(tan);
}


void cvCurveCatRom::Move(int &seg, float &t, float dist, float ) const
{
	Assert(seg>=0 && seg<m_Vertices.GetCount());
	Vector3 vel;
	Vector3 v(3.0f*t*t,2.0f*t,1.0f);
	m_Cubics[seg].Transform3x3(v,vel);
	float speed = vel.Mag();
	float delta = dist/speed;
	t += delta;
	while(t>1.0f)
	{
		t-=1.0f;
		seg++;
		if(seg>=m_Vertices.GetCount() - (m_Looping ? 0 : 1))
		{
			seg=0;
		}
	}
	while (t < 0.0f)
	{
		t+=1.0f;
		seg--;
		if(seg<0)
		{
			seg=m_Vertices.GetCount() - (m_Looping ? 1 : 2);
		}
	}
}

void cvCurveCatRom::GenerateCubics()
{
	int numVerts = m_Vertices.GetCount();
	m_Cubics.Resize(numVerts);

	Assertf(numVerts>=2,"aiCurveCatrom::GenerateCubics()- NumVerts (%d) must be at least 2",numVerts);

	Matrix44 geom, cube;

	if(!m_Looping)
	{
		Vector3 tmp;

		if (m_InitialTangent.Mag2() > VERY_SMALL_FLOAT)
			tmp.SubtractScaled(m_Vertices[1], m_InitialTangent, 2.f);
		else
			tmp.Lerp(2.0f,m_Vertices[1],m_Vertices[0]);

		geom.a.SetVector3ClearW(tmp);
	}
	else
	{
		geom.a.SetVector3ClearW(m_Vertices[numVerts - 1]);
	}

	geom.b.SetVector3ClearW(m_Vertices[0]);
	geom.c.SetVector3ClearW(m_Vertices[1]);

	for(int i=0; i<numVerts; i++)
	{
		if (!m_Looping && i == numVerts - 2)
		{
			Vector3 tmp;

			if (m_FinalTangent.Mag2() > VERY_SMALL_FLOAT)
				tmp.AddScaled(m_Vertices[numVerts-2], m_FinalTangent, 2.f);
			else
				tmp.Lerp(2.0f,m_Vertices[numVerts-2],m_Vertices[numVerts-1]);

			geom.d.SetVector3ClearW(tmp);
		}
		else
		{
			geom.d.SetVector3ClearW(m_Vertices[(i + 2) % numVerts]);
		}

		cube.Dot(geom, sm_CatRomMatrix);

		cube.a.GetVector3(m_Cubics[i].a);
		cube.b.GetVector3(m_Cubics[i].b);
		cube.c.GetVector3(m_Cubics[i].c);
		cube.d.GetVector3(m_Cubics[i].d);

		geom.a = geom.b;
		geom.b = geom.c;
		geom.c = geom.d;
	}
}

float cvCurveCatRom::SolveDerivatives(Vector3 &vel, Vector3 &accl, int seg, float t) const
{
	Assert(m_Cubics.GetCount()==m_Vertices.GetCount());
	Assert(seg>=0 && seg<m_Vertices.GetCount() - (m_Looping ? 0 : 1));
	Assert(t >= 0.f && t <= 1.f);

	vel.Set(3.0f*t*t,2.0f*t,1.0f);
	m_Cubics[seg].Transform3x3(vel);
	accl.Set(6.0f*t,2.0f,0.0f);
	m_Cubics[seg].Transform3x3(accl);

	return 0.0f;
}

float cvCurveCatRom::CalcCurvature(int seg, float t) const
{
	// K(t) = ||f'(t) x f''(t)|| / ||f'(t)||^3

	Vector3 vel, accl, cross;
	SolveDerivatives(vel, accl, seg, t);
	float speed = vel.Mag();
	if (speed < SMALL_FLOAT)
		return LARGE_FLOAT;
	cross.Cross(vel, accl);
	return cross.Mag() / power3(speed);
}

float cvCurveCatRom::CalcXZCurvature(int seg, float t) const
{
	// K(t) = ||f'(t) x f''(t)|| / ||f'(t)||^3

	Vector3 vel, accl;
	SolveDerivatives(vel, accl, seg, t);
	float speed = vel.XZMag();
	if (speed < SMALL_FLOAT)
		return Sign(vel.CrossY(accl)) * LARGE_FLOAT;
	return vel.CrossY(accl) / power3(speed);
}

IMPLEMENT_PLACE	(cvCurveCatRom);

cvCurveCatRom::cvCurveCatRom (datResource &rsc)
	: cvCurve<Vector3>(rsc), m_Cubics(rsc, true)
{
}

#if	__DECLARESTRUCT

void	cvCurveCatRom::DeclareStruct	(datTypeStruct &s)
{
	cvCurve<Vector3>::DeclareStruct(s);

	STRUCT_BEGIN(cvCurveCatRom);
	STRUCT_FIELD(m_Cubics);
	STRUCT_FIELD(m_InitialTangent);
	STRUCT_FIELD(m_FinalTangent);
	STRUCT_END();
}

#endif

} // namespace rage

#endif // ENABLE_UNUSED_CURVE_CODE

