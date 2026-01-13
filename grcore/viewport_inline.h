// 
// grcore/viewport_inline.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef GRCORE_VIEWPORT_INLINE_H
#define GRCORE_VIEWPORT_INLINE_H

#ifndef GRCORE_VIEWPORT_H
#error "Must be included after viewport.h"
#endif

#include "math/intrinsics.h"
#include "vectormath/classes.h"

namespace rage {

inline int grcViewport::IsSphereVisibleInline(Vec4V_In sphere) const 
{
	Vec4V localSph( sphere );
	Vec4V xxxx = Vec4V(SplatX(localSph));	// replicate .GetXf() through a vector
	Vec4V yyyy = Vec4V(SplatY(localSph));	// ...and y, etc
	Vec4V zzzz = Vec4V(SplatZ(localSph));
	Vec4V neg_wwww = Vec4V( -SplatW(localSph) );	// compute 0-w through a vector

	Vec4V sum0, sum1;
	sum0 = AddScaled( m_FrustumLRTB.d(), zzzz, m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, m_FrustumLRTB.a() );
	sum1 = AddScaled( m_FrustumNFNF.d(), zzzz, m_FrustumNFNF.c() );
	sum1 = AddScaled( sum1, yyyy, m_FrustumNFNF.b() );
	sum1 = AddScaled( sum1, xxxx, m_FrustumNFNF.a() );

	VecBoolV cmp0 = IsGreaterThanOrEqual(sum0, neg_wwww);		// Planes that pass visibility will have 0xFFFFFFFF in their column.
	VecBoolV cmp1 = IsGreaterThanOrEqual(sum1, neg_wwww);
	VecBoolV final = And(cmp0,cmp1);				// Intersect the two sets of frustum clip planes.

	// Copy appropriate bit out of condition code which is nonzero if all four columns matched the test vector.
	return IsEqualIntAll( final, VecBoolV(V_T_T_T_T) );
}

inline int grcViewport::IsSphereVisibleInline(Vec4V_In sphere,float &outZ) const 
{
	Vec4V localSph( sphere );
	Vec4V xxxx = Vec4V(SplatX(localSph));	// replicate .GetXf() through a vector
	Vec4V yyyy = Vec4V(SplatY(localSph));	// ...and y, etc
	Vec4V zzzz = Vec4V(SplatZ(localSph));
	Vec4V neg_wwww = Vec4V( -SplatW(localSph) );	// compute 0-w through a vector

	Vec4V sum0, sum1;
	sum0 = AddScaled( m_FrustumLRTB.d(), zzzz, m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, m_FrustumLRTB.a() );
	sum1 = AddScaled( m_FrustumNFNF.d(), zzzz, m_FrustumNFNF.c() );
	sum1 = AddScaled( sum1, yyyy, m_FrustumNFNF.b() );
	sum1 = AddScaled( sum1, xxxx, m_FrustumNFNF.a() );

	VecBoolV cmp0 = IsGreaterThanOrEqual(sum0, neg_wwww);		// Planes that pass visibility will have 0xFFFFFFFF in their column.
	VecBoolV cmp1 = IsGreaterThanOrEqual(sum1, neg_wwww);
	VecBoolV final = And(cmp0,cmp1);				// Intersect the two sets of frustum clip planes.

	Vec::V4StoreScalar32FromSplatted( outZ, Vec4V(SplatX(sum1)).GetIntrin128() );

	// Copy appropriate bit out of condition code which is nonzero if all four columns matched the test vector.
	return IsEqualIntAll( final, VecBoolV( V_TRUE ) );
}

inline grcCullStatus grcViewport::GetSphereCullStatusInline(Vec4V_In sphere NV_SUPPORT_ONLY(, bool useStereoFrustum)) const 
{
	Vec4V localSph( sphere );
	Vec4V xxxx = Vec4V(SplatX(localSph));	// replicate .GetXf() through a vector
	Vec4V yyyy = Vec4V(SplatY(localSph));	// ...and y, etc
	Vec4V zzzz = Vec4V(SplatZ(localSph));
	Vec4V wwww = Vec4V(SplatW(localSph));
	Vec4V neg_wwww = -wwww;	// compute 0-w through a vector

	Vec4V sum0, sum1;
#if NV_SUPPORT
	sum0 = AddScaled( useStereoFrustum ? m_CullFrustumLRTB.d() : m_FrustumLRTB.d(), zzzz, useStereoFrustum ? m_CullFrustumLRTB.c() : m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, useStereoFrustum ? m_CullFrustumLRTB.b() : m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, useStereoFrustum ? m_CullFrustumLRTB.a() : m_FrustumLRTB.a() );
#else
	sum0 = AddScaled( m_FrustumLRTB.d(), zzzz, m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, m_FrustumLRTB.a() );
#endif
	sum1 = AddScaled( m_FrustumNFNF.d(), zzzz, m_FrustumNFNF.c() );
	sum1 = AddScaled( sum1, yyyy, m_FrustumNFNF.b() );
	sum1 = AddScaled( sum1, xxxx, m_FrustumNFNF.a() );

	VecBoolV cmp0 = IsGreaterThanOrEqual(sum0, neg_wwww);	// > -w means we're at least touching
	VecBoolV cmp1 = IsGreaterThanOrEqual(sum1, neg_wwww);
	VecBoolV cmpClip0 = IsGreaterThanOrEqual(sum0, wwww);	// > w mean we're completely inside
	VecBoolV cmpClip1 = IsGreaterThanOrEqual(sum1, wwww);
	VecBoolV final = And(cmp0,cmp1);				// Intersect the two sets of frustum clip planes.
	VecBoolV finalClip = And(cmpClip0,cmpClip1);	// Intersect the two sets of frustum clip planes.

	// __vspltisw sign-extends, giving us 0xFFFFFFFF in all four columns
	// Copy appropriate bit out of condition code which is nonzero if all four columns matched the test vector.
	Vec4V _FFFFFFFF(V_MASKXYZW);
	return (grcCullStatus)( IsEqualIntAll(Vec4V(final), _FFFFFFFF) + IsEqualIntAll(Vec4V(finalClip), _FFFFFFFF) );
}

inline grcCullStatus grcViewport::GetSphereCullStatusInline(Vec4V_In sphere,float &outZ NV_SUPPORT_ONLY(, bool useStereoFrustum)) const 
{
	Vec4V localSph( sphere );
	Vec4V xxxx = Vec4V(SplatX(localSph));	// replicate .GetXf() through a vector
	Vec4V yyyy = Vec4V(SplatY(localSph));	// ...and y, etc
	Vec4V zzzz = Vec4V(SplatZ(localSph));
	Vec4V wwww = Vec4V(SplatW(localSph));
	Vec4V neg_wwww = -wwww;	// compute 0-w through a vector

	Vec4V sum0, sum1;
#if NV_SUPPORT
	sum0 = AddScaled( useStereoFrustum ? m_CullFrustumLRTB.d() : m_FrustumLRTB.d(), zzzz, useStereoFrustum ? m_CullFrustumLRTB.c() : m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, useStereoFrustum ? m_CullFrustumLRTB.b() : m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, useStereoFrustum ? m_CullFrustumLRTB.a() : m_FrustumLRTB.a() );
#else
	sum0 = AddScaled( m_FrustumLRTB.d(), zzzz, m_FrustumLRTB.c() );
	sum0 = AddScaled( sum0, yyyy, m_FrustumLRTB.b() );
	sum0 = AddScaled( sum0, xxxx, m_FrustumLRTB.a() );
#endif
	sum1 = AddScaled( m_FrustumNFNF.d(), zzzz, m_FrustumNFNF.c() );
	sum1 = AddScaled( sum1, yyyy, m_FrustumNFNF.b() );
	sum1 = AddScaled( sum1, xxxx, m_FrustumNFNF.a() );

	VecBoolV cmp0 = IsGreaterThanOrEqual(sum0, neg_wwww);	// > -w means we're at least touching
	VecBoolV cmp1 = IsGreaterThanOrEqual(sum1, neg_wwww);
	VecBoolV cmpClip0 = IsGreaterThanOrEqual(sum0, wwww);	// > w mean we're completely inside
	VecBoolV cmpClip1 = IsGreaterThanOrEqual(sum1, wwww);
	VecBoolV final = And(cmp0,cmp1);				// Intersect the two sets of frustum clip planes.
	VecBoolV finalClip = And(cmpClip0,cmpClip1);	// Intersect the two sets of frustum clip planes.

	Vec::V4StoreScalar32FromSplatted( outZ, Vec4V(SplatX(sum1)).GetIntrin128() );
	// __vspltisw sign-extends, giving us 0xFFFFFFFF in all four columns
	// Copy appropriate bit out of condition code which is nonzero if all four columns matched the test vector.
	Vec4V _FFFFFFFF( V_MASKXYZW );
	return (grcCullStatus)( IsEqualIntAll(Vec4V(final), _FFFFFFFF) + IsEqualIntAll(Vec4V(finalClip), _FFFFFFFF) );
}



/*
http://zach.in.tu-clausthal.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html

If the p-vertex is on the wrong side of the plane, the box can be immediately rejected, as it falls 
completely outside the frustum. On the other hand, if the p-vertex is on the right side of the plane, 
then testing the whereabouts of the n-vertex tells if the box is totally on the right side of the 
plane, or if the box intersects the plane.

Assume a AAB that has its components x,y,and z varying between xmin and xmax; ymin and ymax; and 
zmin and zmax. The components of the positive vertex p are selected as follows:


	p = (xmin,ymin,zmin)
	if (normal.GetXf() >= 0)
		p.GetXf() = xmax;
	if (normal.GetYf() >=0))
		p.GetYf() = ymax;
	if (normal.GetZf() >= 0)
		p.GetZf() = zmax:

The negative vertex n follows the opposite rule:


	n = (xmax,ymax,zmax)
	if (normal.GetXf() >= 0)
		n.GetXf() = xmin;
	if (normal.GetYf() >=0))
		n.GetYf() = ymin;
	if (normal.GetZf() >= 0)
		n.GetZf() = zmin:

Given both p-vertex and n-vertex, the code to find the position of an axis aligned box in a frustum 
is as follows:


int FrustumG::boxInFrustum(AABox &b) {

	int result = INSIDE;
	//for each plane do ...
	for(int i=0; i < 6; i++) {

		// is the positive vertex outside?
		if (pl[i].distance(b.getVertexP(pl[i].normal)) < 0)
			return OUTSIDE;
		// is the negative vertex outside?	
		else if (pl[i].distance(b.getVertexN(pl[i].normal)) < 0)
			result =  INTERSECT;
	}
	return(result);

 }
 */

#ifdef AABB_TEST_IMPL
inline Vec3V_Out getVertexP(Vec3V_In normal,Vec3V_In boxmin,Vec3V_In boxmax)
{
	return Vec3V(
		normal.GetXf() >= 0? boxmax.GetXf() : boxmin.GetXf(),
		normal.GetYf() >= 0? boxmax.GetYf() : boxmin.GetYf(),		
		normal.GetZf() >= 0? boxmax.GetZf() : boxmin.GetZf());
}

inline Vec3V_Out getVertexN(Vec3V_In normal,Vec3V_In boxmin,Vec3V_In boxmax)
{
	return Vec3V(
		normal.GetXf() >= 0? boxmin.GetXf() : boxmax.GetXf(),
		normal.GetYf() >= 0? boxmin.GetYf() : boxmax.GetYf(),		
		normal.GetZf() >= 0? boxmin.GetZf() : boxmax.GetZf());
}

inline float planeDistance(Vec4V_In plane,Vec3V_In pt)
{
	return pt.GetXf() * plane.GetXf() + pt.GetYf() * plane.GetYf() + pt.GetZf() * plane.GetZf() + plane.GetWf();
}

inline grcCullStatus grcViewport::GetAABBCullStatusInline_Test(Vec3V_In vmin,Vec3V_In vmax) const
{
	grcCullStatus result = cullInside;
	const Vec4V *plane = &m_FrustumClipPlanes[0];
	for (int i=0; i<4 /*6*/; i++)
	{
		if (planeDistance(plane[i],getVertexP(plane[i].GetXYZ(),vmin,vmax)) < 0)
			return cullOutside;
		else if (planeDistance(plane[i],getVertexN(plane[i].GetXYZ(),vmin,vmax)) < 0)
			result = cullClipped;
	}
	return result;
}

inline grcCullStatus grcViewport::GetAABBCullStatusInline_Test2(Vec3V_In vmin,Vec3V_In vmax) const
{
	int anyOutside = 0;
	// Matrix44 temp;
	//temp.Transpose(m_LocalLRTB);
	const Vec4V *plane = &m_FrustumClipPlanes[0];

	// Really naive version, generate all eight points.
	Vec3V points[8];
	points[0] = Vec3V(vmin.GetXf(),vmin.GetYf(),vmin.GetZf());
	points[1] = Vec3V(vmin.GetXf(),vmin.GetYf(),vmax.GetZf());
	points[2] = Vec3V(vmin.GetXf(),vmax.GetYf(),vmin.GetZf());
	points[3] = Vec3V(vmin.GetXf(),vmax.GetYf(),vmax.GetZf());
	points[4] = Vec3V(vmax.GetXf(),vmin.GetYf(),vmin.GetZf());
	points[5] = Vec3V(vmax.GetXf(),vmin.GetYf(),vmax.GetZf());
	points[6] = Vec3V(vmax.GetXf(),vmax.GetYf(),vmin.GetZf());
	points[7] = Vec3V(vmax.GetXf(),vmax.GetYf(),vmax.GetZf());

	for (int i=0; i<4 /*6*/; i++)
	{
		int allOutside = 1;
		for (int j=0; j<8; j++)
		{
			float dist = planeDistance(plane[i],points[j]);
			int outside = (dist < 0);
			anyOutside |= outside;
			allOutside &= outside;
		}
		if (allOutside)
			return cullOutside;
	}
	return anyOutside? cullClipped : cullInside;
}
#endif

inline grcCullStatus grcViewport::GetAABBCullStatusInline(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes)
{
	Vec4V lvmin(vmin), lvmax(vmax);
	Vec4V vmin_xxxx = Vec4V(SplatX(lvmin));	// replicate .GetXf() through a vector
	Vec4V vmin_yyyy = Vec4V(SplatY(lvmin));	// ...and y, etc
	Vec4V vmin_zzzz = Vec4V(SplatZ(lvmin));
	Vec4V vmax_xxxx = Vec4V(SplatX(lvmax));	// replicate .GetXf() through a vector
	Vec4V vmax_yyyy = Vec4V(SplatY(lvmax));	// ...and y, etc
	Vec4V vmax_zzzz = Vec4V(SplatZ(lvmax));
	Vec4V lrtb_xxxx = cullPlanes.a();
	Vec4V lrtb_yyyy = cullPlanes.b();
	Vec4V lrtb_zzzz = cullPlanes.c();
	Vec4V lrtb_wwww = cullPlanes.d();

	// Compute p-vertex and n-vertex for all four planes.
	// We want normal.GetXf()|y|z >= 0, so we do 0 > normal.GetXf()|y|z
	// and swap the parameters to SelectFT.
	Vec4V zero(V_ZERO);
	VecBoolV x_lt_0 = IsGreaterThan(zero,lrtb_xxxx);
	VecBoolV y_lt_0 = IsGreaterThan(zero,lrtb_yyyy);
	VecBoolV z_lt_0 = IsGreaterThan(zero,lrtb_zzzz);
	// SelectFT(boolV,zero,nonZero)
	// For positive vector, pick max if normal >= 0
	Vec4V px0123 = SelectFT(x_lt_0,vmax_xxxx,vmin_xxxx); 
	Vec4V py0123 = SelectFT(y_lt_0,vmax_yyyy,vmin_yyyy);
	Vec4V pz0123 = SelectFT(z_lt_0,vmax_zzzz,vmin_zzzz);
	// For negative vector, pick min if normal >= 0
	Vec4V nx0123 = SelectFT(x_lt_0,vmin_xxxx,vmax_xxxx);
	Vec4V ny0123 = SelectFT(y_lt_0,vmin_yyyy,vmax_yyyy);
	Vec4V nz0123 = SelectFT(z_lt_0,vmin_zzzz,vmax_zzzz);

	// compute all four distances of p-vertex and n-vertex from all four planes
	// AddScaled(a,b,c) = a + b * c
	// Ax + By + Cz + w = AddScaled(AddScaled(AddScaled(w,C,z),B,y),A,x);
	Vec4V dist_p0123 = AddScaled(AddScaled(AddScaled(lrtb_wwww,pz0123,lrtb_zzzz),py0123,lrtb_yyyy),px0123,lrtb_xxxx);
	Vec4V dist_n0123 = AddScaled(AddScaled(AddScaled(lrtb_wwww,nz0123,lrtb_zzzz),ny0123,lrtb_yyyy),nx0123,lrtb_xxxx);
	// if any p-vertex distances are less than zero, OUTSIDE
	// else if any n-vertex distances are less than zero, CLIPPED
	// else INSIDE.
	VecBoolV p_tests = IsGreaterThan(zero,dist_p0123);
	VecBoolV n_tests = IsGreaterThan(zero,dist_n0123);
	int p_test = IsEqualIntAll( p_tests, VecBoolV(V_F_F_F_F) );
	int n_test = IsEqualIntAll( n_tests, VecBoolV(V_F_F_F_F) );
	return (grcCullStatus)(p_test + n_test);
}


inline int grcViewport::IsAABBVisibleInline(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes)
{
	Vec4V lvmin(vmin), lvmax(vmax);
	Vec4V vmin_xxxx = Vec4V(SplatX(lvmin));	// replicate .GetXf() through a vector
	Vec4V vmin_yyyy = Vec4V(SplatY(lvmin));	// ...and y, etc
	Vec4V vmin_zzzz = Vec4V(SplatZ(lvmin));
	Vec4V vmax_xxxx = Vec4V(SplatX(lvmax));	// replicate .GetXf() through a vector
	Vec4V vmax_yyyy = Vec4V(SplatY(lvmax));	// ...and y, etc
	Vec4V vmax_zzzz = Vec4V(SplatZ(lvmax));
	Vec4V lrtb_xxxx = cullPlanes.a();
	Vec4V lrtb_yyyy = cullPlanes.b();
	Vec4V lrtb_zzzz = cullPlanes.c();
	Vec4V lrtb_wwww = cullPlanes.d();

	// Compute p-vertex and n-vertex for all four planes.
	// We want normal.GetXf()|y|z >= 0, so we do 0 > normal.GetXf()|y|z
	// and swap the parameters to SelectFT.
	Vec4V zero(V_ZERO);
	VecBoolV x_lt_0 = IsGreaterThan(zero,lrtb_xxxx);
	VecBoolV y_lt_0 = IsGreaterThan(zero,lrtb_yyyy);
	VecBoolV z_lt_0 = IsGreaterThan(zero,lrtb_zzzz);
	// SelectFT(boolV,zero,nonZero)
	// For positive vector, pick max if normal >= 0
	Vec4V px0123 = SelectFT(x_lt_0,vmax_xxxx,vmin_xxxx); 
	Vec4V py0123 = SelectFT(y_lt_0,vmax_yyyy,vmin_yyyy);
	Vec4V pz0123 = SelectFT(z_lt_0,vmax_zzzz,vmin_zzzz);

	// compute all four distances of p-vertex from all four planes
	// AddScaled(a,b,c) = a + b * c
	// Ax + By + Cz + w = AddScaled(AddScaled(AddScaled(w,C,z),B,y),A,x);
	Vec4V dist_p0123 = AddScaled(AddScaled(AddScaled(lrtb_wwww,pz0123,lrtb_zzzz),py0123,lrtb_yyyy),px0123,lrtb_xxxx);
	// if all p-vertex distances are greater than zero, VISIBLE
	return IsGreaterThanOrEqualAll(dist_p0123,zero);
}


inline grcCullStatus grcViewport::GetAABBCullStatusInline(Vec::V4Param128 vmin,Vec::V4Param128 vmax,Mat44V_In cullPlanes1,Mat44V_In cullPlanes2)
{
	// lrtb = "left right top bottom" and nfnf = "near far near far"
	// Note the names don't really mean anything, any eight planes will be tested.
	Vec4V lvmin(vmin), lvmax(vmax);
	Vec4V vmin_xxxx = Vec4V(SplatX(lvmin));	// replicate .GetXf() through a vector
	Vec4V vmin_yyyy = Vec4V(SplatY(lvmin));	// ...and y, etc
	Vec4V vmin_zzzz = Vec4V(SplatZ(lvmin));
	Vec4V vmax_xxxx = Vec4V(SplatX(lvmax));	// replicate .GetXf() through a vector
	Vec4V vmax_yyyy = Vec4V(SplatY(lvmax));	// ...and y, etc
	Vec4V vmax_zzzz = Vec4V(SplatZ(lvmax));
	Vec4V lrtb_xxxx = cullPlanes1.a();
	Vec4V lrtb_yyyy = cullPlanes1.b();
	Vec4V lrtb_zzzz = cullPlanes1.c();
	Vec4V lrtb_wwww = cullPlanes1.d();
	Vec4V nfnf_xxxx = cullPlanes2.a();
	Vec4V nfnf_yyyy = cullPlanes2.b();
	Vec4V nfnf_zzzz = cullPlanes2.c();
	Vec4V nfnf_wwww = cullPlanes2.d();

	// Compute p-vertex and n-vertex for all four planes.
	// We want normal.GetXf()|y|z >= 0, so we do 0 > normal.GetXf()|y|z
	// and swap the parameters to SelectFT.
	Vec4V zero(V_ZERO);
	VecBoolV lrtb_x_lt_0 = IsGreaterThan(zero,lrtb_xxxx);
	VecBoolV lrtb_y_lt_0 = IsGreaterThan(zero,lrtb_yyyy);
	VecBoolV lrtb_z_lt_0 = IsGreaterThan(zero,lrtb_zzzz);
	VecBoolV nfnf_x_lt_0 = IsGreaterThan(zero,nfnf_xxxx);
	VecBoolV nfnf_y_lt_0 = IsGreaterThan(zero,nfnf_yyyy);
	VecBoolV nfnf_z_lt_0 = IsGreaterThan(zero,nfnf_zzzz);
	// SelectFT(boolV,zero,nonZero)
	// For positive vector, pick max if normal >= 0
	Vec4V lrtb_px0123 = SelectFT(lrtb_x_lt_0,vmax_xxxx,vmin_xxxx); 
	Vec4V lrtb_py0123 = SelectFT(lrtb_y_lt_0,vmax_yyyy,vmin_yyyy);
	Vec4V lrtb_pz0123 = SelectFT(lrtb_z_lt_0,vmax_zzzz,vmin_zzzz);
	Vec4V nfnf_px0123 = SelectFT(nfnf_x_lt_0,vmax_xxxx,vmin_xxxx); 
	Vec4V nfnf_py0123 = SelectFT(nfnf_y_lt_0,vmax_yyyy,vmin_yyyy);
	Vec4V nfnf_pz0123 = SelectFT(nfnf_z_lt_0,vmax_zzzz,vmin_zzzz);
	// For negative vector, pick min if normal >= 0
	Vec4V lrtb_nx0123 = SelectFT(lrtb_x_lt_0,vmin_xxxx,vmax_xxxx);
	Vec4V lrtb_ny0123 = SelectFT(lrtb_y_lt_0,vmin_yyyy,vmax_yyyy);
	Vec4V lrtb_nz0123 = SelectFT(lrtb_z_lt_0,vmin_zzzz,vmax_zzzz);
	Vec4V nfnf_nx0123 = SelectFT(nfnf_x_lt_0,vmin_xxxx,vmax_xxxx);
	Vec4V nfnf_ny0123 = SelectFT(nfnf_y_lt_0,vmin_yyyy,vmax_yyyy);
	Vec4V nfnf_nz0123 = SelectFT(nfnf_z_lt_0,vmin_zzzz,vmax_zzzz);

	// compute all four distances of p-vertex and n-vertex from all four planes
	// AddScaled(a,b,c) = a + b * c
	// Ax + By + Cz + w = AddScaled(AddScaled(AddScaled(w,C,z),B,y),A,x);
	Vec4V lrtb_dist_p0123 = AddScaled(AddScaled(AddScaled(lrtb_wwww,lrtb_pz0123,lrtb_zzzz),lrtb_py0123,lrtb_yyyy),lrtb_px0123,lrtb_xxxx);
	Vec4V lrtb_dist_n0123 = AddScaled(AddScaled(AddScaled(lrtb_wwww,lrtb_nz0123,lrtb_zzzz),lrtb_ny0123,lrtb_yyyy),lrtb_nx0123,lrtb_xxxx);
	Vec4V nfnf_dist_p0123 = AddScaled(AddScaled(AddScaled(nfnf_wwww,nfnf_pz0123,nfnf_zzzz),nfnf_py0123,nfnf_yyyy),nfnf_px0123,nfnf_xxxx);
	Vec4V nfnf_dist_n0123 = AddScaled(AddScaled(AddScaled(nfnf_wwww,nfnf_nz0123,nfnf_zzzz),nfnf_ny0123,nfnf_yyyy),nfnf_nx0123,nfnf_xxxx);
	// if any p-vertex distances are less than zero, OUTSIDE
	// else if any n-vertex distances are less than zero, CLIPPED
	// else INSIDE.
	VecBoolV p_tests = Or(IsGreaterThan(zero,lrtb_dist_p0123),IsGreaterThan(zero,nfnf_dist_p0123));
	VecBoolV n_tests = Or(IsGreaterThan(zero,lrtb_dist_n0123),IsGreaterThan(zero,nfnf_dist_n0123));
	int p_test = IsEqualIntAll( p_tests, VecBoolV(V_F_F_F_F) );
	int n_test = IsEqualIntAll( n_tests, VecBoolV(V_F_F_F_F) );
	return (grcCullStatus)(p_test + n_test);
}


}	// namespace rage

#endif	// GRCORE_VIEWPORT_INLINE_H
