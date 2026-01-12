//
// pharticulated/bodyinertia.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "bodyinertia.h"

#include "math/simplemath.h"
#include "mathext/lcp.h"
#include "phcore/config.h"
#include "vectormath/legacyconvert.h"


namespace rage {


// **********************************************************************
// Member functions for ArticulatedBodyInertia
// **********************************************************************


// Computes the inverse (which is also symmetric)

void phArticulatedBodyInertia::Inverse ( phArticulatedBodyInertia& ret ) const
{
	Mat33V v_retInvInertia;
	Mat33V v_retMass;
	Mat33V v_retCrossInertia;

	Mat33V Iinverse;
	InverseSym ( m_InvInertia, Iinverse );

	// I^{-1}
	Mat33V zzz = m_CrossInertia;
	Transpose(zzz, zzz);
	Mat33V HT_Iinv;
	Multiply( HT_Iinv, Iinverse, zzz );
	//	HT_Iinv.LeftMultiplyByTranspose(H);	// H^T * I^{-1}

	v_retInvInertia = HT_Iinv;
	Multiply( v_retInvInertia, m_CrossInertia, v_retInvInertia );	//-- *=
	v_retInvInertia = m_Mass - v_retInvInertia;
	//--ret.I.SubtractFrom(M);
	//InvertSym(ret.I);
	float* matrixArray = (float*)(&v_retInvInertia);
	VALIDATE_PHYSICS_ASSERTF(matrixArray[0]+matrixArray[5]+matrixArray[10]>=0.0f,"Contact force solver inverting a bad matrix, diagonals = %f, %f, %f",
		matrixArray[0],matrixArray[5],matrixArray[10]);
	
	LCPSolver::InvertPositiveDefinite(matrixArray, VEC3_NUM_STORED_FLOATS);

	Multiply( v_retCrossInertia, HT_Iinv, v_retInvInertia );
	v_retMass = m_CrossInertia;
	Multiply( v_retMass, v_retCrossInertia, v_retMass );
	v_retMass += Mat33V(V_IDENTITY);

	//zzz.Set3x3(ret.M);
	// Had to replace this with an equivalent set of lower-level operations to work around
	// some Xenon optimizer bug.
	zzz = v_retMass;

	Multiply( v_retMass, zzz, Iinverse );
	v_retCrossInertia = -v_retCrossInertia;

	ret.m_CrossInertia = v_retCrossInertia;
	ret.m_InvInertia = v_retInvInertia;
	ret.m_Mass = v_retMass;
}

void phArticulatedBodyInertia::SetOuterProductScaled( const phPhaseSpaceVector& u, const float& scale )
{
	Vec3V v_uOmega( u.omega );
	Vec3V v_uTrans( u.trans );
	Vec3V tempOmega( u.omega );
	Vec3V v_scale = Vec3VFromF32( scale );

	tempOmega = Scale(tempOmega, v_scale);
	OuterProduct( m_Mass, tempOmega, v_uOmega );
	OuterProduct( m_CrossInertia, tempOmega, v_uTrans );
	tempOmega = Scale(v_uTrans, v_scale);
	OuterProduct( m_InvInertia, tempOmega, v_uTrans );
}


void phArticulatedBodyInertia::InverseSym_Imp( MAT33V_DECL(original), Mat33V_InOut inverse )
{
	//////////////////////////
	// "SYMMETRIC" INVERSE
	//////////////////////////

	// The reason this implementation was needed was it seems that a full, general vectorized 3x3 matrix inverse
	// was producing slightly-off unstable results that eventually blew up the system after many calls to this function.
	//
	// Reverting to the original scalar solution fixed the problem...
	//
	// Matrix33 mtx = MAT33V_TO_MATRIX33(MAT33V_ARG_GET(original));
	//
	// // Compute the six distinct subdeterminants
	// float sd11 = mtx.b.y*mtx.c.z-square(mtx.c.y);
	// float sd12 = mtx.c.x*mtx.c.y-mtx.b.x*mtx.c.z;
	// float sd13 = mtx.b.x*mtx.c.y-mtx.c.x*mtx.b.y;
	// float sd22 = mtx.a.x*mtx.c.z-square(mtx.c.x);
	// float sd23 = mtx.c.x*mtx.b.x-mtx.a.x*mtx.c.y;
	// float sd33 = mtx.a.x*mtx.b.y-square(mtx.b.x);
	//
	// float detInv = InvertSafe(mtx.a.x*sd11 + mtx.b.x*sd12 + mtx.c.x*sd13,LARGE_FLOAT);
	//
	// mtx.a.x = sd11*detInv;
	// mtx.a.y = sd12*detInv;
	// mtx.a.z = sd13*detInv;
	//
	// mtx.b.x = sd12*detInv;
	// mtx.b.y = sd22*detInv;
	// mtx.b.z = sd23*detInv;
	//
	// mtx.c.x = sd13*detInv;
	// mtx.c.y = sd23*detInv;
	// mtx.c.z = sd33*detInv;
	//
	// inverse = RCC_MAT33V(mtx);
	//
	// ...so I produced a vectorized version that does that exact same, only faster. /wpfeil@r*sd

	Mat33V inMatOrig = MAT33V_ARG_GET(original);

	// Splat some matrix elements that we'll need.
	ScalarV _ax = SplatX( inMatOrig.GetCol0() );
	ScalarV _bx = SplatX( inMatOrig.GetCol1() );
	ScalarV _by = SplatY( inMatOrig.GetCol1() );
	ScalarV _cx = SplatX( inMatOrig.GetCol2() );
	ScalarV _cy = SplatY( inMatOrig.GetCol2() );
	ScalarV _cz = SplatZ( inMatOrig.GetCol2() );

	// Construct some columns of the calculation which are fast (1 instruction) to permute into position on all
	// platforms (i.e. either PS3's vec_merge[l|h]() or vec_sld()).
	Vec4V _01_cy = Vec4V( _cz, Vec3V(_cy) );
	Vec4V _02_bz = Vec4V( _cy, inMatOrig.GetCol1() );
	Vec4V _04_ax = MergeXY( Vec4V(_cx), Vec4V(_ax) );
	Vec4V _08_cx = MergeXY( Vec4V(inMatOrig.GetCol1()), Vec4V(_cx) );

	// Construct the rest from the original matrix elements and from the four we just created (1 instruction each, also).
	Vec4V _07_ax = MergeXY( _01_cy, _04_ax );
	Vec4V _00_bx = GetFromTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( _08_cx, Vec4V(_bx) );
	Vec4V _05_cy = MergeXY( _02_bz, _01_cy );
	Vec4V _09_bx = MergeZW( _07_ax, Vec4V(_bx) );

	// These will need more than one instruction it seems (two each), but at least are four all independent of each other...
	// they only depend on the initial splats, so should be scheduled nicely.
	Vec4V _03_cz = Vec4V( Vec3V( _cy, _cz, _cx ) );
	Vec4V _06_cx = Vec4V( Vec3V( _bx, _cx, _ax ) );
	Vec4V _10_ax = Vec4V( Vec3V( _cx, _ax, _bx ) );
	Vec4V _11_cy = Vec4V( Vec3V( _by, _cy, _bx ) );

	// Now we just manipulate the vectors above like so:
	//
	// col0 = _00 * _01 - _02 _ _03
	// col1 = _00 * _01 - _02 _ _03
	// col2 = _00 * _01 - _02 _ _03
	//
	// to create the following matrix.
	//
	// +---+---+---+
	// | C | C | C |
	// |   |   |   |
	// |---+---+---|
	// | O | O | O |
	// |   |   |   |
	// |---+---+---|
	// | L | L | L |
	// |   |   |   |
	// | 0 | 1 | 2 |
	// +---+---+---+

	Vec4V _col0 = SubtractScaled( _00_bx * _01_cy, _02_bz, _03_cz );
	Vec4V _col1 = SubtractScaled( _04_ax * _05_cy, _06_cx, _07_ax );
	Vec4V _col2 = SubtractScaled( _08_cx * _09_bx, _10_ax, _11_cy );

	// Now we just need the inv determinant to scale that whole matrix by.
	// detInv = 1.0f / ( _ax*col0.x + _bx*col1.x + _cx*col2.x )
	ScalarV _determinantInv = InvertSafe( AddScaled( _ax * SplatX(_col0) + _bx * SplatX(_col1), _cx, SplatX(_col2) ), ScalarV(V_ZERO) );
	Vec3V _v3_determinantInv = Vec3V(_determinantInv);

	// Scale the matrix.
	inverse = Mat33V(
			_col0.GetXYZ() * _v3_determinantInv,
			_col1.GetXYZ() * _v3_determinantInv,
			_col2.GetXYZ() * _v3_determinantInv
					);

	// By the way, below is the full, general vectorized inverse that was causing instability.

	// Mat33V inMatOrig = MAT33V_ARG_GET(original);
	// // This is a full 3x3 matrix inverse (doesn't take the symmetric property into account), but it's still faster than the scalar float version.
	// // TODO: Maybe make an even faster version that takes advantage of symmetry.
	// Invert( inverse, inMatOrig );
}

} // namespace rage
