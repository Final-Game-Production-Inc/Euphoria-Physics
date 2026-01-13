// 
// grcore/matrix43.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_MATRIX43_H 
#define GRCORE_MATRIX43_H 

#include "data/safestruct.h"
#include "math/float16.h"
#include "vectormath/vectormath.h"
#include "vectormath/legacyconvert.h"

namespace rage {

 // was in float16.h ..

// Our nomenclature is backwards from the rest of the planet!
// This is a transposed Matrix34 that takes only three constant registers.
struct Matrix43 {
	Vec::Vector_4V x, y, z;
	
	Matrix43() {}
	Matrix43(class datResource& /*rsc*/) {}
	DECLARE_PLACE(Matrix43);

	void Zero()
	{
		x = y = z = Vec::V4VConstant(V_ZERO);
	}

	void Identity()
	{
		x = Vec::V4VConstant(V_X_AXIS_WZERO);
		y = Vec::V4VConstant(V_Y_AXIS_WZERO);
		z = Vec::V4VConstant(V_Z_AXIS_WZERO);
	}

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s)
	{
		SSTRUCT_BEGIN( Matrix43 )
			SSTRUCT_FIELD( Matrix43, x )
			SSTRUCT_FIELD( Matrix43, y )
			SSTRUCT_FIELD( Matrix43, z )
		SSTRUCT_END( Matrix43 )
	}
#endif

	void FromMatrix34(Mat34V_In mtx) {
		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( mtx.GetCol0().GetIntrin128(), mtx.GetCol2().GetIntrin128() );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( mtx.GetCol1().GetIntrin128(), mtx.GetCol3().GetIntrin128() );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( mtx.GetCol0().GetIntrin128(), mtx.GetCol2().GetIntrin128() );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( mtx.GetCol1().GetIntrin128(), mtx.GetCol3().GetIntrin128() );

		x = Vec::V4MergeXY( tempVect0, tempVect1 );
		y = Vec::V4MergeZW( tempVect0, tempVect1 );
		z = Vec::V4MergeXY( tempVect2, tempVect3 );
	}

	void ToMatrix34(Mat34V_InOut mtx) const {
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( x, z );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( y, _zero );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( x, z );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( y, _zero );

		mtx.SetCol0(Vec3V(Vec::V4MergeXY( tempVect0, tempVect1 )));
		mtx.SetCol1(Vec3V(Vec::V4MergeZW( tempVect0, tempVect1 )));
		mtx.SetCol2(Vec3V(Vec::V4MergeXY( tempVect2, tempVect3 )));
		mtx.SetCol3(Vec3V(Vec::V4MergeZW( tempVect2, tempVect3 )));
	}
};

struct Matrix43_Float16 {	
	Float16 ax, bx, cx, dx;
	Float16 ay, by, cy, dy;
	Float16 az, bz, cz, dz;
};


} // namespace rage

#endif // GRCORE_MATRIX43_H 