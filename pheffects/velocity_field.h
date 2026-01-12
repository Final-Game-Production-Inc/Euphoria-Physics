// 
// pheffects/velocity_field.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHEFFECTS_VELOCITY_FIELD_H
#define PHEFFECTS_VELOCITY_FIELD_H

namespace rage
{

class Vector3;

class phVelocityField
{
public:
	enum phvfBoundary
	{
		phvfbZero,
		phvfbBounce,
	};

	phVelocityField();
	~phVelocityField();

	void Init(int gridResolution, float viscosity, float timeStep, phvfBoundary boundary = phvfbBounce);

	void AddForce(int x, int y, const Vector3& force);

	void Update();

	const Vector3& GetVelocity(int x, int y) const;
	int GetResolution() const;

private:
	void Diffuse(Vector3* pOut, Vector3* pIn, float fViscosity, float fTimeStep);
	void LinSolve(int nBound, float* pOut, float* pIn, float fA, float fC);
	void LinSolve(Vector3* pOut, Vector3* pIn, float fA, float fC);
	void SetBound(int nBound, float* pField);
	void SetBounds(Vector3* pField);
	void Project(Vector3* pField0);
	void Advect(Vector3* pField0, Vector3* pField1, float fTimeStep);

protected:
	Vector3*		m_pGrid;
	Vector3*		m_pForceField;
	int				m_nGridRes;
	int				m_nGridSize;
	float			m_fTimeStep;
	float			m_fViscosity;
	phvfBoundary	m_Boundary;
};


} // namespace rage

#endif // PHEFFECTS_VELOCITY_FIELD_H
