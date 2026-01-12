#ifndef PHBOUND_BOUNDGEOM_SECOND_SURFACE_H
#define PHBOUND_BOUNDGEOM_SECOND_SURFACE_H

#include "phCore/constants.h"
#include "physics/inst.h"

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA

#include "boundGeom.h"
#include "primitives.h"
#include "vector/vector3.h"

namespace rage {

class phBoundGeometrySecondSurfacePolygonCalculator
{
public:

	phBoundGeometrySecondSurfacePolygonCalculator()
		: m_fSecondSurfaceInterp(-1)
	{
	}
	~phBoundGeometrySecondSurfacePolygonCalculator()
	{
	}

	enum
	{
		MAX_NUM_NEIGHBOURING_POLYS=64
	};

	//Set the interpolation factor before computing any second surface polgyons.
	//This is forced because the default value of the interpolation value is -1 and
	//we assert the value when the calculation is performed.
	void SetSecondSurfaceInterp(const float fSecondSurfaceInterp)
	{
		Assert(fSecondSurfaceInterp>=0);
		Assert(fSecondSurfaceInterp<=1);
		m_fSecondSurfaceInterp=fSecondSurfaceInterp;
	}

	//Given a polygon in a geometry bound compute the polygon of the second surface of the geometry bound.
	void ComputeSecondSurfacePolyVertsAndNormal
		(const phBoundGeometry& geomBound, const phPolygon& polygon,
		Vector3* pavSecondSurfacePolyVertices, Vector3& vSecondSurfacePolyNormal, float& fSecondSurfacePolyArea) const;

	//Given a polygon in a geometry bound compute the polygon of the second surface of the geometry bound.
	void ComputeSecondSurfacePolyVertsAndNormal
		(const phBoundGeometry& geomBound, const Vector3* const geomBoundVertices, const float* const geomBoundSecondSurfaceVertexDisplacements, const phPolygon& polygon, const int iPolygonBoundIndex,
		Vector3* pavSecondSurfacePolyVertices, Vector3& vSecondSurfacePolyNormal, float& fSecondSurfacePolyArea) const;

private:

	//Interpolate between principal and second surface of phBoundGeom.
	float m_fSecondSurfaceInterp;
};

}//namespace rage

#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
#endif //PHBOUND_BOUNDGEOM_SECOND_SURFACE_H