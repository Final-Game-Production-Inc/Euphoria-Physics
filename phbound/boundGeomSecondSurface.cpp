#include "phCore/constants.h"

#include "boundgeom.h"
#include "boundGeomSecondSurface.h"

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA

namespace rage {

void phBoundGeometrySecondSurfacePolygonCalculator::ComputeSecondSurfacePolyVertsAndNormal
(const phBoundGeometry& geomBound, const phPolygon& polygon, 
 Vector3* pavSecondSurfacePolyVertices, Vector3& vSecondSurfacePolyNormal, float& fSecondSurfacePolyArea) const
{
	//Make sure the polygon is in the geomBound.
	Assert(&geomBound.GetPolygon((int)(&polygon-geomBound.GetPolygonPointer()))==&polygon);

	if(!geomBound.GetHasSecondSurface())
	{
		//Second surface verts will be the same as first surface verts.
		for(int i=0;i<POLY_MAX_VERTICES;i++)
		{
			const int iVertIndex=polygon.GetVertexIndex(i);
			pavSecondSurfacePolyVertices[i]=VEC3V_TO_VECTOR3(geomBound.GetVertex(iVertIndex));
		}
		//Second surface will be same as first surface normal.
		const int iPolyIndex=(int)(&polygon-geomBound.GetPolygonPointer());
		Assert(&geomBound.GetPolygon(iPolyIndex)==&polygon);
		Assert(iPolyIndex<geomBound.GetNumPolygons());
		Assert(iPolyIndex>=0);
		Assert(geomBound.GetPolygon(iPolyIndex).GetVertexIndex(0)<(u32)geomBound.GetNumVertices());
		Assert(geomBound.GetPolygon(iPolyIndex).GetVertexIndex(1)<(u32)geomBound.GetNumVertices());
		Assert(geomBound.GetPolygon(iPolyIndex).GetVertexIndex(2)<(u32)geomBound.GetNumVertices());
		vSecondSurfacePolyNormal=VEC3V_TO_VECTOR3(geomBound.GetPolygonUnitNormal(iPolyIndex));
		fSecondSurfacePolyArea=polygon.GetArea();
	}
	else
	{
		Assert(m_fSecondSurfaceInterp>=0);
		Assert(m_fSecondSurfaceInterp<=1);

		//Compute the second surface polygon verts.
		for(int i=0;i<POLY_MAX_VERTICES;i++)
		{
			geomBound.ComputeSecondSurfacePosition(polygon.GetVertexIndex(i),m_fSecondSurfaceInterp,pavSecondSurfacePolyVertices[i]);
		}

		//Compute the normal of the polygon formed by the displaced vertices.
		//The unit normal calculation uses the area of the polygon as the normalising
		//factor but we can't rely on this any longer because the area of the polygon formed
		//by the disiplaced verts differs from the original polygon's area.
		//To solve this problem calculate the non-unit normal then unitise.
		vSecondSurfacePolyNormal= VEC3V_TO_VECTOR3(polygon.ComputeNonUnitNormal(RCC_VEC3V(pavSecondSurfacePolyVertices[0]), RCC_VEC3V(pavSecondSurfacePolyVertices[1]), RCC_VEC3V(pavSecondSurfacePolyVertices[2])));
		fSecondSurfacePolyArea=vSecondSurfacePolyNormal.Mag();
		vSecondSurfacePolyNormal*=(1.0f/fSecondSurfacePolyArea);
		fSecondSurfacePolyArea*=0.5f;

#if __ASSERT
		float fPolySurfaceNormalMag2 = vSecondSurfacePolyNormal.Mag2();
		Assert(fPolySurfaceNormalMag2>0.99f*0.99f && fPolySurfaceNormalMag2<1.01f*1.01f);
#endif

		Assert(&polygon-geomBound.GetPolygonPointer()<geomBound.GetNumPolygons());
		Assert(&polygon-geomBound.GetPolygonPointer()>=0);
		Assert(geomBound.GetPolygon((int)(&polygon-geomBound.GetPolygonPointer())).GetVertexIndex(0)<(u32)geomBound.GetNumVertices());
		Assert(geomBound.GetPolygon((int)(&polygon-geomBound.GetPolygonPointer())).GetVertexIndex(1)<(u32)geomBound.GetNumVertices());
		Assert(geomBound.GetPolygon((int)(&polygon-geomBound.GetPolygonPointer())).GetVertexIndex(2)<(u32)geomBound.GetNumVertices());
		Assert(VEC3V_TO_VECTOR3(geomBound.GetPolygonUnitNormal((int)(&polygon-geomBound.GetPolygonPointer()))).Dot(vSecondSurfacePolyNormal)>=0);
	}
}

void phBoundGeometrySecondSurfacePolygonCalculator::ComputeSecondSurfacePolyVertsAndNormal
(const phBoundGeometry& ASSERT_ONLY(geomBound), 
 const Vector3* RESTRICT const pavGeomBoundVertices, const float* const pafGeomBoundSecondSurfaceVertexDisplacements, 
 const phPolygon& polygon, const int UNUSED_PARAM(iPolygonBoundIndex),
 Vector3* RESTRICT pavSecondSurfacePolyVertices, Vector3& vSecondSurfacePolyNormal, float& fSecondSurfacePolyArea) const
{
	Assert(pavGeomBoundVertices);
	// Checking that the RESTRICT above is okay.  Strictly speaking the memory of the arrays shouldn't overlap and this isn't a strong enough check.
	Assert(pavGeomBoundVertices != pavSecondSurfacePolyVertices);

	if(!pafGeomBoundSecondSurfaceVertexDisplacements)
	{
		Assert(!geomBound.GetHasSecondSurface());

		//Second surface verts will be the same as first surface verts.
		for(int i=0;i<POLY_MAX_VERTICES;i++)
		{
			const int iVertIndex=polygon.GetVertexIndex(i);
			pavSecondSurfacePolyVertices[i]=pavGeomBoundVertices[iVertIndex];
		}
		//Second surface normal will be computed using first surface verts.
		vSecondSurfacePolyNormal=VEC3V_TO_VECTOR3(polygon.ComputeUnitNormal(
			RCC_VEC3V(pavGeomBoundVertices[polygon.GetVertexIndex(0)]),
			RCC_VEC3V(pavGeomBoundVertices[polygon.GetVertexIndex(1)]),
			RCC_VEC3V(pavGeomBoundVertices[polygon.GetVertexIndex(2)])));
		fSecondSurfacePolyArea=polygon.GetArea();
	}
	else
	{
		Assert(m_fSecondSurfaceInterp>=0);
		Assert(m_fSecondSurfaceInterp<=1);
		const ScalarV vsSecondSurfaceInterp=ScalarVFromF32(m_fSecondSurfaceInterp);

		//Compute the second surface polygon verts.
		for(int i=0;i<POLY_MAX_VERTICES;i++)
		{
			const int iVertIndex=polygon.GetVertexIndex(i);
			Vec3V vDisplacement(V_ZERO);
			vDisplacement.SetZ(ScalarVFromF32(pafGeomBoundSecondSurfaceVertexDisplacements[iVertIndex]));
			vDisplacement=vDisplacement*vsSecondSurfaceInterp;
			pavSecondSurfacePolyVertices[i]=pavGeomBoundVertices[iVertIndex]-RCC_VECTOR3(vDisplacement);
		}

		//Compute the normal of the polygon formed by the displaced vertices.
		//The unit normal calculation uses the area of the polygon as the normalising
		//factor but we can't rely on this any longer because the area of the polygon formed
		//by the disiplaced verts differs from the original polygon's area.
		//To solve this problem calculate the non-unit normal then unitise.
		Vec3V vTempSecondSurfacePolyNormal=polygon.ComputeNonUnitNormal(RCC_VEC3V(pavSecondSurfacePolyVertices[0]),
		                                                                RCC_VEC3V(pavSecondSurfacePolyVertices[1]),
		                                                                RCC_VEC3V(pavSecondSurfacePolyVertices[2]));
		fSecondSurfacePolyArea = (Mag(vTempSecondSurfacePolyNormal) * ScalarV(V_HALF)).Getf();
		vTempSecondSurfacePolyNormal = NormalizeFast(vTempSecondSurfacePolyNormal);
		vSecondSurfacePolyNormal = RCC_VECTOR3(vTempSecondSurfacePolyNormal);


#if __ASSERT
		float fPolySurfaceNormalMag2 = vSecondSurfacePolyNormal.Mag2();
		Assert(fPolySurfaceNormalMag2>0.99f*0.99f && fPolySurfaceNormalMag2<1.01f*1.01f);
#endif
	}
}

}//namespace rage

#endif //HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
