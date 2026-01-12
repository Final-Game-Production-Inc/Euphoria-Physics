// 
// pheffects/velocity_field.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "velocity_field.h"

#include "system/alloca.h"
#include "vector/vector3.h"

namespace rage
{

#define IDX(x, y)	(((y) * (m_nGridRes + 2)) + (x))

phVelocityField::phVelocityField() :
	m_pGrid(0),
	m_pForceField(0),
	m_nGridRes(0),
	m_nGridSize(0),
	m_fTimeStep(0),
	m_fViscosity(0),
	m_Boundary(phvfbBounce)
{
}

phVelocityField::~phVelocityField()
{
	if( m_pForceField )
		delete[] m_pForceField;
	if( m_pGrid )
		delete[] m_pGrid;
}

void phVelocityField::Init(int gridResolution, float viscosity, float timeStep, phvfBoundary boundary)
{
	// Delete any previous field
	if( m_pForceField )
		delete[] m_pForceField;
	if( m_pGrid )
		delete[] m_pGrid;

	// Setup the variables
	m_nGridRes = gridResolution;
	m_fViscosity = viscosity;
	m_fTimeStep = timeStep;
	m_Boundary = boundary;

	// Calculate size and allocate space for the field
	m_nGridSize = (m_nGridRes + 2) * (m_nGridRes + 2);
	m_pGrid = rage_new Vector3[m_nGridSize];
	m_pForceField = rage_new Vector3[m_nGridSize];

	// Clear the field
	memset(m_pGrid, 0, sizeof(Vector3) * m_nGridSize);
	memset(m_pForceField, 0, sizeof(Vector3) * m_nGridSize);
}

void phVelocityField::AddForce(int x, int y, const Vector3& force)
{
	int index = IDX(x + 1, y + 1);
	m_pForceField[index] += force;
}

void phVelocityField::Update()
{
	// Add External Forces
	for( int i = 0; i < m_nGridSize; i++ )
	{
		m_pGrid[i] += m_pForceField[i] * m_fTimeStep;
	}

	Diffuse(m_pForceField, m_pGrid, m_fViscosity, m_fTimeStep);

	Project(m_pForceField);

	Advect(m_pGrid, m_pForceField, m_fTimeStep);

	Project(m_pGrid);

	// Clear force field for next frame
	memset(m_pForceField, 0, sizeof(Vector3) * m_nGridSize);
}

const Vector3& phVelocityField::GetVelocity(int x, int y) const
{
	int index = IDX(x + 1, y + 1);
	return m_pGrid[index];
}

int phVelocityField::GetResolution() const
{
	return m_nGridRes;
}


// Private functions
void phVelocityField::Diffuse(Vector3* pOut, Vector3* pIn, float fViscosity, float fTimeStep)
{
	float fA = fTimeStep * fViscosity * m_nGridRes * m_nGridRes;
	LinSolve(pOut, pIn, fA, 1 + 4 * fA);
}

void phVelocityField::LinSolve(int nBound, float* pOut, float* pIn, float fA, float fC)
{
	for( int k = 0; k < 20; k++ )
	{
		for( int x = 1; x <= m_nGridRes; x++ )
		{
			for( int y = 1; y <= m_nGridRes; y++ )
			{
				int index = IDX(x, y);
				int left = IDX(x - 1, y);
				int right = IDX(x + 1, y);
				int top = IDX(x, y - 1);
				int bottom = IDX(x, y + 1);
				pOut[index] = (pIn[index] + fA * (pOut[left] + pOut[right] + pOut[top] + pOut[bottom])) / fC;
			}
		}
		SetBound(nBound, pOut);
	}
}

void phVelocityField::LinSolve(Vector3* pOut, Vector3* pIn, float fA, float fC)
{
	for( int k = 0; k < 20; k++ )
	{
		for( int x = 1; x <= m_nGridRes; x++ )
		{
			for( int y = 1; y <= m_nGridRes; y++ )
			{
				int index = IDX(x, y);
				int left = IDX(x - 1, y);
				int right = IDX(x + 1, y);
				int top = IDX(x, y - 1);
				int bottom = IDX(x, y + 1);
				pOut[index] = (pIn[index] + fA * (pOut[left] + pOut[right] + pOut[top] + pOut[bottom])) / fC;
			}
		}
		SetBounds(pOut);
	}
}

void phVelocityField::SetBound(int nBound, float* pField)
{
	for( int i = 1; i <= m_nGridRes; i++ )
	{
		switch( m_Boundary )
		{
			case phvfbZero:
				pField[IDX(0, i)]				= 0;
				pField[IDX(m_nGridRes + 1, i)]	= 0;
				pField[IDX(i, 0)]				= 0;
				pField[IDX(i, m_nGridRes + 1)]	= 0;
				break;
			case phvfbBounce:
				pField[IDX(0, i)]				= pField[IDX(1, i)];
				pField[IDX(m_nGridRes + 1, i)]	= pField[IDX(m_nGridRes, i)];
				pField[IDX(i, 0)]				= pField[IDX(i, 1)];
				pField[IDX(i, m_nGridRes + 1)]	= pField[IDX(i, m_nGridRes)];

				if( nBound == 1 )
				{
					pField[IDX(0, i)]				= -pField[IDX(0, i)];
					pField[IDX(m_nGridRes + 1, i)]	= -pField[IDX(m_nGridRes + 1, i)];
				}
				if( nBound == 2 )
				{
					pField[IDX(i, 0)]				= -pField[IDX(i, 0)];
					pField[IDX(i, m_nGridRes + 1)]	= -pField[IDX(i, m_nGridRes + 1)];
				}
				break;
			default:
				AssertMsg(0 , "Unknown Boundary Type");
				break;
		}
	}

	if( m_Boundary == phvfbBounce )
	{	
		pField[IDX(0, 0)]							= 0.5f * (pField[IDX(1, 0)]							+ pField[IDX(0, 1)]);
		pField[IDX(0, m_nGridRes + 1)]				= 0.5f * (pField[IDX(1, m_nGridRes + 1)]			+ pField[IDX(0, m_nGridRes)]);
		pField[IDX(m_nGridRes + 1, 0)]				= 0.5f * (pField[IDX(m_nGridRes, 0)]				+ pField[IDX(m_nGridRes + 1, 1)]);
		pField[IDX(m_nGridRes + 1, m_nGridRes + 1)]	= 0.5f * (pField[IDX(m_nGridRes, m_nGridRes + 1)]	+ pField[IDX(m_nGridRes + 1, m_nGridRes)]);
	}
}

void phVelocityField::SetBounds(Vector3* pField)
{
	for( int i = 1; i <= m_nGridRes; i++ )
	{
		switch( m_Boundary )
		{
			case phvfbZero:
				pField[IDX(0, i)]				= VEC3_ZERO;
				pField[IDX(m_nGridRes + 1, i)]	= VEC3_ZERO;
				pField[IDX(i, 0)]				= VEC3_ZERO;
				pField[IDX(i, m_nGridRes + 1)]	= VEC3_ZERO;
				break;
			case phvfbBounce:
				pField[IDX(0, i)]				= pField[IDX(1, i)];
				pField[IDX(m_nGridRes + 1, i)]	= pField[IDX(m_nGridRes, i)];
				pField[IDX(i, 0)]				= pField[IDX(i, 1)];
				pField[IDX(i, m_nGridRes + 1)]	= pField[IDX(i, m_nGridRes)];

				pField[IDX(0, i)].x					= -pField[IDX(0, i)].x;
				pField[IDX(m_nGridRes + 1, i)].x	= -pField[IDX(m_nGridRes + 1, i)].x;
				pField[IDX(i, 0)].y					= -pField[IDX(i, 0)].y;
				pField[IDX(i, m_nGridRes + 1)].y	= -pField[IDX(i, m_nGridRes + 1)].y;
				break;
			default:
				AssertMsg(0 , "Unknown Boundary Type");
				break;
		}		
	}

	if( m_Boundary == phvfbBounce )
	{
		pField[IDX(0, 0)]							= 0.5f * (pField[IDX(1, 0)]							+ pField[IDX(0, 1)]);
		pField[IDX(0, m_nGridRes + 1)]				= 0.5f * (pField[IDX(1, m_nGridRes + 1)]			+ pField[IDX(0, m_nGridRes)]);
		pField[IDX(m_nGridRes + 1, 0)]				= 0.5f * (pField[IDX(m_nGridRes, 0)]				+ pField[IDX(m_nGridRes + 1, 1)]);
		pField[IDX(m_nGridRes + 1, m_nGridRes + 1)]	= 0.5f * (pField[IDX(m_nGridRes, m_nGridRes + 1)]	+ pField[IDX(m_nGridRes + 1, m_nGridRes)]);
	}
}

void phVelocityField::Project(Vector3* pField0)
{
	float* pTemp1 = Alloca(float, m_nGridSize);
	float* pTemp2 = Alloca(float, m_nGridSize);
	for( int x = 1; x <= m_nGridRes; x++ )
	{
		for( int y = 1; y <= m_nGridRes; y++ )
		{
			pTemp1[IDX(x, y)]= -0.5f * (pField0[IDX(x + 1, y)].x - pField0[IDX(x - 1, y)].x + pField0[IDX(x, y + 1)].y - pField0[IDX(x, y - 1)].y) / m_nGridRes;
			pTemp2[IDX(x, y)] = 0.0f;
		}
	}
	SetBound(0, pTemp1);
	SetBound(0, pTemp2);

	LinSolve(0, pTemp2, pTemp1, 1, 4);

	for( int x = 1; x <= m_nGridRes; x++ )
	{
		for( int y = 1; y <= m_nGridRes; y++ )
		{
			pField0[IDX(x, y)] -= 0.5f * m_nGridRes * (Vector3(pTemp2[IDX(x + 1, y)] - pTemp2[IDX(x - 1, y)], pTemp2[IDX(x, y + 1)] - pTemp2[IDX(x, y - 1)], 0.0f));
		}
	}
	SetBounds(pField0);
}

void phVelocityField::Advect(Vector3* pField0, Vector3* pField1, float fTimeStep)
{
	float dt0 = fTimeStep * m_nGridRes;
	for( int x = 1; x <= m_nGridRes; x++ )
	{
		for( int y = 1; y <= m_nGridRes; y++ )
		{
			Vector3 vXY = Vector3((float)x, (float)y, 0.0f) - dt0 * pField1[IDX(x, y)];
			vXY.Max(vXY, Vector3(0.5f, 0.5f, 0.0f));
			vXY.Min(vXY, Vector3((float)m_nGridRes + 0.5f, (float)m_nGridRes + 0.5f, 0.0f));

			int x0 = (int)vXY.x;
			int x1 = x0 + 1;
			int y0 = (int)vXY.y;
			int y1 = y0 + 1;

			float s1 = vXY.x - x0;
			float s0 = 1.0f - s1;
			float t1 = vXY.y - y0;
			float t0 = 1.0f - t1;

			Vector3 temp;
			temp.x =	s0 * (t0 * pField1[IDX(x0, y0)].x + t1 * pField1[IDX(x0, y1)].x) + 
						s1 * (t0 * pField1[IDX(x1, y0)].x + t1 * pField1[IDX(x1, y1)].x);
			temp.y =	s0 * (t0 * pField1[IDX(x0, y0)].y + t1 * pField1[IDX(x0, y1)].y) + 
						s1 * (t0 * pField1[IDX(x1, y0)].y + t1 * pField1[IDX(x1, y1)].y);
			temp.z =	0.f;

			pField0[IDX(x, y)] = temp;
		}
	}
	SetBounds(pField0);
}

} // namespace rage
