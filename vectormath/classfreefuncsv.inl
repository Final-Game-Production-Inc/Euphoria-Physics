
// Note: We only give ONE chance for a no-inline for a function with matrix-return-values. No more! i.e.:
//	If a function is only 'inline' (not 'forceinline'), make sure it makes no calls to other functions (which return a matrix as a result)
//	that are also declared 'inline'.

namespace rage
{

	//============================================================================
	// Assertion functions

	inline bool IsFiniteAll( Vec2V_In v )
	{
		return !!Vec::V2IsFiniteAll(v.GetIntrin128());
	}

	inline bool IsFiniteAll( Vec3V_In v )
	{
		return !!Vec::V3IsFiniteAll(v.GetIntrin128());
	}

	inline bool IsFiniteAll( Vec4V_In v )
	{
		return !!Vec::V4IsFiniteAll(v.GetIntrin128());
	}

	inline bool IsFiniteAll( QuatV_In v )
	{
		return !!Vec::V4IsFiniteAll(v.GetIntrin128());
	}

	inline bool IsFiniteAll( ScalarV_In v )
	{
		return !!Vec::V4IsFiniteAll(v.GetIntrin128());
	}

	inline bool IsFiniteAll( Mat33V_In m )
	{
		return IsFiniteAll(m.GetCol0()) && IsFiniteAll(m.GetCol1()) && IsFiniteAll(m.GetCol2());
	}

	inline bool IsFiniteAll( Mat34V_In m )
	{
		return IsFiniteAll(m.GetCol0()) && IsFiniteAll(m.GetCol1()) && IsFiniteAll(m.GetCol2()) && IsFiniteAll(m.GetCol3());
	}

	inline bool IsFiniteStable( Vec2V_In v )
	{
		return FPIsFinite(v.GetXf()) && FPIsFinite(v.GetYf());
	}

	inline bool IsFiniteStable( Vec3V_In v )
	{
		return FPIsFinite(v.GetXf()) && FPIsFinite(v.GetYf()) && FPIsFinite(v.GetZf());
	}

	inline bool IsFiniteStable( Vec4V_In v )
	{
		return FPIsFinite(v.GetXf()) && FPIsFinite(v.GetYf()) && FPIsFinite(v.GetZf()) && FPIsFinite(v.GetWf());
	}

	//============================================================================
	// Utility functions

	__forceinline FASTRETURNCHECK(ScalarV_Out) LoadScalar32IntoScalarV( const float& scalar )
	{
		return ScalarV( Vec::V4LoadScalar32IntoSplatted( scalar ) );
	}

	__forceinline void StoreScalar32FromScalarV( float& fLoc, ScalarV_In splattedVec )
	{
		Vec::V4StoreScalar32FromSplatted( fLoc, splattedVec.GetIntrin128() );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) LoadScalar32IntoScalarV( const u32& scalar )
	{
		return ScalarV( Vec::V4LoadScalar32IntoSplatted( scalar ) );
	}

	__forceinline void StoreScalar32FromScalarV( u32& loc, ScalarV_In splattedVec )
	{
		Vec::V4StoreScalar32FromSplatted( loc, splattedVec.GetIntrin128() );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) LoadScalar32IntoScalarV( const s32& scalar )
	{
		return ScalarV( Vec::V4LoadScalar32IntoSplatted( scalar ) );
	}

	__forceinline void StoreScalar32FromScalarV( s32& loc, ScalarV_In splattedVec )
	{
		Vec::V4StoreScalar32FromSplatted( loc, splattedVec.GetIntrin128() );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromAxisAngle( Vec3V_In normAxis, ScalarV_In radians )
	{
		return QuatV( Vec::V4QuatFromAxisAngle( normAxis.GetIntrin128(), radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVScaleAngle( QuatV_In _quat, ScalarV_In scale )
	{
		return QuatV( Vec::V4QuatScaleAngle( _quat.GetIntrin128(), scale.GetIntrin128() ) );
	}

#if !SCALAR_TYPES_ONLY
	__forceinline FASTRETURNCHECK(ScalarV_Out) QuatVTwistAngle( QuatV_In _quat, Vec4V_In vec )
	{
		return ScalarV( Vec::V4QuatTwistAngle( _quat.GetIntrin128(), vec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) QuatVTwistAngle( QuatV_In _quat, Vec3V_In vec )
	{
		return ScalarV( Vec::V4QuatTwistAngle( _quat.GetIntrin128(), vec.GetIntrin128() ) );
	}
#endif

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromXAxisAngle( ScalarV_In radians )
	{
		return QuatV( Vec::V4QuatFromXAxisAngle( radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromYAxisAngle( ScalarV_In radians )
	{
		return QuatV( Vec::V4QuatFromYAxisAngle( radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromZAxisAngle( ScalarV_In radians )
	{
		return QuatV( Vec::V4QuatFromZAxisAngle( radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromMat33V( Mat33V_In mat )
	{
		return QuatV( Imp::QuatFromMat33V_Imp33( MAT33V_ARG(mat) ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromMat34V( Mat34V_In mat )
	{
		return QuatV( Imp::QuatFromMat33V_Imp33( MAT33V_ARG(mat) ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromMat33VOrtho( Mat33V_In mat )
	{
		return QuatV( Imp::QuatFromMat33VOrtho_Imp33( MAT33V_ARG(mat) ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromMat33VSafe( Mat33V_In mat, QuatV_In errVal )
	{
		Mat33V tMat, inMat = mat;
		Transpose(tMat, inMat);

		// remove scale from input matrix
		Vec::Vector_4V xx = Vec::V4Scale( tMat.GetCol0Intrin128(), tMat.GetCol0Intrin128() );
		Vec::Vector_4V xx_yy = Vec::V4AddScaled( xx, tMat.GetCol1Intrin128(), tMat.GetCol1Intrin128()  );
		Vec::Vector_4V xx_yy_zz = Vec::V4AddScaled( xx_yy, tMat.GetCol2Intrin128(), tMat.GetCol2Intrin128() );
		Vec::Vector_4V invMag = Vec::V4InvSqrt(xx_yy_zz);
		ScaleTranspose(tMat, Vec3V(invMag), inMat);

		// compute normalized quaternion
		Vec::Vector_4V q = Imp::QuatFromMat33V_Imp33(tMat.GetCol0Intrin128(), tMat.GetCol1Intrin128(), tMat.GetCol2Intrin128());
		Vec::Vector_4V normQ = Vec::V4Normalize(q);

		// if magnitude is zero, return error value
		Vec::Vector_4V err = Vec::V4IsEqualV(Vec::V3DotV(xx_yy_zz, xx_yy_zz), Vec::V4VConstant(V_ZERO));

		return QuatV( Vec::V4SelectFT(err, normQ, errVal.GetIntrin128()) );
	}

	namespace QuatHelpers 
	{
		static const u32 P = U32_ZERO;
		static const u32 N = U32_NEGZERO;
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulersXYZ( Vec3V_In radianAngles )
	{
		using namespace QuatHelpers;
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, Vec::V4VConstant<N,P,P,P>(), Vec::V4VConstant<P,P,N,P>()) );
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulersXZY( Vec3V_In radianAngles )
	{
		using namespace QuatHelpers;
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, Vec::V4VConstant<P,P,P,P>(), Vec::V4VConstant<P,P,N,N>()));
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulersYXZ( Vec3V_In radianAngles )
	{
		using namespace QuatHelpers;
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, Vec::V4VConstant<N,P,P,P>(), Vec::V4VConstant<P,P,P,N>()));
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulersYZX( Vec3V_In radianAngles )
	{
		using namespace QuatHelpers;
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, Vec::V4VConstant<N,P,P,P>(), Vec::V4VConstant<P,N,P,P>()));
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulersZXY( Vec3V_In radianAngles )
	{
		using namespace QuatHelpers;
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, Vec::V4VConstant<P,P,P,P>(), Vec::V4VConstant<P,N,N,P>()));
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulersZYX( Vec3V_In radianAngles )
	{
		using namespace QuatHelpers;
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, Vec::V4VConstant<P,P,P,P>(), Vec::V4VConstant<P,N,P,N>()));
	}
	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromEulers( Vec3V_In radianAngles, EulerAngleOrder order )
	{
		return QuatV( Imp::QuatVFromEulers_Imp(radianAngles, order));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulersXYZ( QuatV_In quatIn )
	{
		using namespace Vec;
		return Vec3V(Imp::QuatVToEulers_Imp<Z,Y,X,1>(quatIn));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulersXZY( QuatV_In quatIn )
	{
		using namespace Vec;
		return Vec3V(Imp::QuatVToEulers_Imp<X,Z,Y,0>(quatIn));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulersYXZ( QuatV_In quatIn )
	{
		using namespace Vec;
		return Vec3V(Imp::QuatVToEulers_Imp<Y,X,Z,0>(quatIn));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulersYZX( QuatV_In quatIn )
	{
		using namespace Vec;
		return Vec3V(Imp::QuatVToEulers_Imp<X,Z,Y,1>(quatIn));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulersZXY( QuatV_In quatIn )
	{
		using namespace Vec;
		return Vec3V(Imp::QuatVToEulers_Imp<Y,X,Z,1>(quatIn));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulersZYX( QuatV_In quatIn )
	{
		using namespace Vec;
		return Vec3V(Imp::QuatVToEulers_Imp<Z,Y,X,0>(quatIn));
	}
	__forceinline FASTRETURNCHECK(Vec3V_Out) QuatVToEulers( QuatV_In quatIn, EulerAngleOrder order )
	{
		return Vec3V(Imp::QuatVToEulers_Imp(quatIn, order));
	}

	// TODO: In most cases, these matrix creators are doing minimal processing. They should probably be __forceinlined.
	// Then again, the user may not care about the performance of setup functions, and may care more about code size.
	// (In any case, they are currently not __forceinlined, since there are no matrix ARGUMENTS.)

	inline void Mat33VFromAxisAngle( Mat33V_InOut inoutMat, Vec3V_In normAxis, ScalarV_In radians )
	{
		Vec::Vector_4V sinVect;
		Vec::Vector_4V cosVect;
		Vec::Vector_4V tempVect0;
		Vec::Vector_4V tempVect1;
		Vec::Vector_4V tempVect2;
		Vec::Vector_4V sinTimesAxis;
		Vec::Vector_4V negSinTimesAxis;
		Vec::Vector_4V outCol0;
		Vec::Vector_4V outCol1;
		Vec::Vector_4V outCol2;

		Vec::Vector_4V _normAxis = normAxis.GetIntrin128();
		Vec::Vector_4V _xxxx = Vec::V4SplatX( _normAxis );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( _normAxis );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( _normAxis );

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		Vec::Vector_4V oneMinusCos = Vec::V4Subtract( Vec::V4VConstant(V_ONE), cosVect );

		sinTimesAxis = Vec::V4Scale( _normAxis, sinVect );
		negSinTimesAxis = Vec::V4Negate( sinTimesAxis );

		tempVect0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z1,Vec::Y2,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect1 = Vec::V4PermuteTwo<Vec::Z2,Vec::X1,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect2 = Vec::V4PermuteTwo<Vec::Y1,Vec::X2,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );

		tempVect0 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKX), tempVect0, cosVect );
		tempVect1 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKY), tempVect1, cosVect );
		tempVect2 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKZ), tempVect2, cosVect );

		outCol0 = Vec::V4AddScaled( tempVect0, Vec::V4Scale( _normAxis, _xxxx ), oneMinusCos );
		outCol1 = Vec::V4AddScaled( tempVect1, Vec::V4Scale( _normAxis, _yyyy ), oneMinusCos );
		outCol2 = Vec::V4AddScaled( tempVect2, Vec::V4Scale( _normAxis, _zzzz ), oneMinusCos );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat33VFromXAxisAngle( Mat33V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V _zero;
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _1000, _0_sin_0_sin, _0_cos_0_cos;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_zero = Vec::V4VConstant(V_ZERO);
		_1000 = Vec::V4VConstant(V_X_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );
		
		_0_sin_0_sin = Vec::V4MergeXY( _zero, sinVect );
		_0_cos_0_cos = Vec::V4MergeXY( _zero, cosVect );

		outCol0 = _1000;
		outCol1 = Vec::V4MergeXY( _0_sin_0_sin, cosVect );
		outCol2 = Vec::V4MergeXY( _0_cos_0_cos, negSinVect );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat33VFromYAxisAngle( Mat33V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _0000, _0100, _cos_negSin_cos_negSin, _sin_cos_sin_cos;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_0000 = Vec::V4VConstant(V_ZERO);
		_0100 = Vec::V4VConstant(V_Y_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );

		_cos_negSin_cos_negSin = Vec::V4MergeXY( cosVect, negSinVect );
		_sin_cos_sin_cos = Vec::V4MergeXY( sinVect, cosVect );

		outCol0 = Vec::V4MergeXY( _cos_negSin_cos_negSin, _0000 );
		outCol1 = _0100;
		outCol2 = Vec::V4MergeXY( _sin_cos_sin_cos, _0000 );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat33VFromZAxisAngle( Mat33V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _0011, _cos_0_cos_0, _negSin_0_negSin_0;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_0011 = Vec::V4VConstant(V_Z_AXIS_WONE);
		negSinVect = Vec::V4Negate( sinVect );

		_cos_0_cos_0 = Vec::V4MergeXY( cosVect, _0011 );
		_negSin_0_negSin_0 = Vec::V4MergeXY( negSinVect, _0011 );

		outCol0 = Vec::V4MergeXY( _cos_0_cos_0, sinVect );
		outCol1 = Vec::V4MergeXY( _negSin_0_negSin_0, cosVect );
		outCol2 = _0011;

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat33VFromQuatV( Mat33V_InOut inoutMat, QuatV_In inQuat )
	{
		Vec::Vector_4V q = inQuat.GetIntrin128();
		Vec::Vector_4V maskX = Vec::V4VConstant(V_MASKX);
		Vec::Vector_4V maskZ = Vec::V4VConstant(V_MASKZW);

		Vec::Vector_4V outCol0, outCol1, outCol2;
		Vec::Vector_4V _wwww, _yzxw, _zxyw;
		Vec::Vector_4V _xyzwSq, _yzxwSq, _zxywSq;
		Vec::Vector_4V tempVect0, tempVect1, tempVect2, tempVect3, tempVect4, tempVect5;

		_xyzwSq = Vec::V4Add( q, q );
		_wwww = Vec::V4SplatW( q );
		_yzxw = Vec::V4Permute<Vec::Y,Vec::Z,Vec::X,Vec::W>( q );
		_zxyw = Vec::V4Permute<Vec::Z,Vec::X,Vec::Y,Vec::W>( q );
		_yzxwSq = Vec::V4Permute<Vec::Y,Vec::Z,Vec::X,Vec::W>( _xyzwSq );
		_zxywSq = Vec::V4Permute<Vec::Z,Vec::X,Vec::Y,Vec::W>( _xyzwSq );

		tempVect0 = Vec::V4Scale( _yzxwSq, _wwww );
		tempVect1 = Vec::V4SubtractScaled( Vec::V4VConstant(V_ONE), _yzxwSq, _yzxw );
		tempVect2 = Vec::V4Scale( _yzxw, _xyzwSq );
		tempVect0 = Vec::V4AddScaled( tempVect0, _zxyw, _xyzwSq );
		tempVect1 = Vec::V4SubtractScaled( tempVect1, _zxywSq, _zxyw );
		tempVect2 = Vec::V4SubtractScaled( tempVect2, _wwww, _zxywSq );
		tempVect3 = Vec::V4SelectFT( maskX, tempVect0, tempVect1 );
		tempVect4 = Vec::V4SelectFT( maskX, tempVect1, tempVect2 );
		tempVect5 = Vec::V4SelectFT( maskX, tempVect2, tempVect0 );

		outCol0 = Vec::V4SelectFT( maskZ, tempVect3, tempVect2 );
		outCol1 = Vec::V4SelectFT( maskZ, tempVect4, tempVect0 );
		outCol2 = Vec::V4SelectFT( maskZ, tempVect5, tempVect1 );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat33VFromEulersXYZ( Mat33V_InOut inoutMat, Vec3V_In radianAngles )
	{
		Vec::Vector_4V angles = radianAngles.GetIntrin128();
		Vec::Vector_4V outCol0, outCol1, outCol2;
		Vec::Vector_4V sinVect, cosVect, negSinVect;

		Vec::Vector_4V tempVect0, tempVect1, tempVec2, tempVec3, tempVec4, tempVec5, tempVec6;

		Vec::V4SetWZero( angles );
		
		Vec::V4SinAndCos( sinVect, cosVect, angles );
		negSinVect = Vec::V4Negate( sinVect );
		tempVec4 = Vec::V4MergeZW( cosVect, sinVect );
		tempVec5 = Vec::V4MergeZW( negSinVect, cosVect );
		tempVec5 = Vec::V4Andc( tempVec5, Vec::V4VConstant(V_MASKW) );
		tempVec2 = Vec::V4PermuteTwo<Vec::Y2,Vec::Y2,Vec::Y1,Vec::X1>( negSinVect, cosVect );
		tempVec3 = Vec::V4PermuteTwo<Vec::Y2,Vec::Y2,Vec::Y1,Vec::X1>( cosVect, sinVect );
		tempVect0 = Vec::V4SplatX( sinVect );
		tempVect1 = Vec::V4SplatX( cosVect );
		tempVec6 = Vec::V4Scale( tempVec4, tempVec3 );

		outCol0 = Vec::V4Scale( tempVec4, tempVec2 );
		outCol1 = Vec::V4AddScaled( Vec::V4Scale( tempVec6, tempVect0 ), tempVec5, tempVect1 );
		outCol2 = Vec::V4SubtractScaled( Vec::V4Scale( tempVec6, tempVect1 ), tempVect0, tempVec5 );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat33VFromEulersXZY(Mat33V_InOut inoutMat, Vec3V_In radianAngles)
	{
		// (1,1,-1)	 (cz,sz,cz)	(cy,1,sy)
		// (-1,1,1)	 (cx,cx,cx)	(sz,cz,sz)	(cy,1,sy) + (sx,*,sx) (sy,0,cy)
		// (1,-1,-1) (sx,sx,sx)	(sz,cz,sz)	(cy,1,sy) + (cx,*,cx) (sy,0,cy)

		Vec::Vector_4V sinVect, cosVect;
		Vec::V4SinAndCos( sinVect, cosVect, radianAngles.GetIntrin128() );

		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V _one = Vec::V4VConstant(V_ONE);
		Vec::Vector_4V _negone = Vec::V4VConstant(V_NEGONE);
		Vec::Vector_4V _pnpn = Vec::V4MergeXY(_one,_negone);
		Vec::Vector_4V _ppnn = Vec::V4MergeXY(_pnpn,_pnpn);
		Vec::Vector_4V _pnnn = Vec::V4MergeXY(_pnpn,_negone);

		Vec::Vector_4V sx = Vec::V4SplatX(sinVect);
		Vec::Vector_4V cx = Vec::V4SplatX(cosVect);
		Vec::Vector_4V sz = Vec::V4SplatZ(sinVect);
		Vec::Vector_4V cz = Vec::V4SplatZ(cosVect);

		Vec::Vector_4V cy_sy_sy_cy = Vec::V4PermuteTwo<Vec::Y2,Vec::Y1,Vec::Y1,Vec::Y2>(sinVect, cosVect);
		Vec::Vector_4V cz_sz_cz_sz = Vec::V4MergeXY(cz,sz);
		Vec::Vector_4V cy_p_sy_p = Vec::V4MergeXY(cy_sy_sy_cy, _one);
		Vec::Vector_4V sy_z_cy_z = Vec::V4MergeZW(cy_sy_sy_cy, _zero);
		Vec::Vector_4V sz_cz_sz_cz = Vec::V4MergeXY(sz,cz);
		Vec::Vector_4V szcy_cz_szsy_cz = Vec::V4Scale(sz_cz_sz_cz, cy_p_sy_p);

		Vec::Vector_4V col0 = Vec::V4Scale(Vec::V4Scale(cz_sz_cz_sz, cy_p_sy_p), _ppnn);
		Vec::Vector_4V col1 = Vec::V4SubtractScaled(Vec::V4Scale(sx, sy_z_cy_z), szcy_cz_szsy_cz, Vec::V4Scale(_pnnn, cx));
		Vec::Vector_4V col2 = Vec::V4AddScaled(Vec::V4Scale(cx,sy_z_cy_z), szcy_cz_szsy_cz, Vec::V4Scale(_pnnn, sx));

		inoutMat.SetColsIntrin128( col0, col1, col2 );
	}

	inline void Mat33VFromEulersYXZ( Mat33V_InOut inoutMat, Vec3V_In radianAngles )
	{
		// (cy,cy,0) (cz,sz,*)	+	(-1,1,-1) (sy,sy,sy) (sx,sx,cx) (sz,cz,1)
		// (-1,1,1)	 (cx,cx,sx) (sz,cz,1)
		// (sy,sy,0) (cz,sz,*)	+	(1,-1,1)  (cy,cy,cy) (sx,sx,cx) (sz,cz,1)

		Vec::Vector_4V sinVect, cosVect;
		Vec::V4SinAndCos( sinVect, cosVect, radianAngles.GetIntrin128() );

		Vec::Vector_4V _maskXY = Vec::V4VConstant(V_MASKXY);
		Vec::Vector_4V _one = Vec::V4VConstant(V_ONE);
		Vec::Vector_4V _negone = Vec::V4VConstant(V_NEGONE);
		Vec::Vector_4V _npnp = Vec::V4MergeXY(_negone, _one);
		Vec::Vector_4V _nppp = Vec::V4MergeXY(_npnp, _one);

		Vec::Vector_4V sy = Vec::V4SplatY(sinVect);
		Vec::Vector_4V cy = Vec::V4SplatY(cosVect);

		Vec::Vector_4V sx_cx_sy_cy = Vec::V4MergeXY(sinVect, cosVect);
		Vec::Vector_4V sx_sx_cx_cx = Vec::V4MergeXY(sx_cx_sy_cy, sx_cx_sy_cy);
		Vec::Vector_4V cx_sx_cy_sy = Vec::V4MergeXY(cosVect, sinVect);
		Vec::Vector_4V cx_cx_sx_sx = Vec::V4MergeXY(cx_sx_cy_sy, cx_sx_cy_sy);
		Vec::Vector_4V cz_sz_cw_sw = Vec::V4MergeZW(cosVect, sinVect);
		Vec::Vector_4V sz_cz_p_p = Vec::V4PermuteTwo<Vec::Y1,Vec::X1,Vec::X2,Vec::X2>(cz_sz_cw_sw, _one);
		Vec::Vector_4V sxsz_sxcz_cx = Vec::V4Scale(sx_sx_cx_cx, sz_cz_p_p);

		Vec::Vector_4V col0 = Vec::V4AddScaled(Vec::V4And( Vec::V4Scale(cy, cz_sz_cw_sw), _maskXY), sxsz_sxcz_cx, Vec::V4Scale(_npnp, sy));
		Vec::Vector_4V col1 = Vec::V4Scale(sz_cz_p_p, Vec::V4Scale(_nppp, cx_cx_sx_sx));
		Vec::Vector_4V col2 = Vec::V4SubtractScaled(Vec::V4And(Vec::V4Scale(sy, cz_sz_cw_sw), _maskXY), sxsz_sxcz_cx, Vec::V4Scale(_npnp, cy));

		inoutMat.SetColsIntrin128( col0, col1, col2 );
	}

	inline void Mat33VFromEulersYZX( Mat33V_InOut inoutMat, Vec3V_In radianAngles )
	{
		// (cy,cy,cy) (cz,sz,sz) (1,cx,sx) + (1,1,-1) (0,sy,sy) (*,sx,cx)
		// (-1,1,1)   (sz,cz,cz) (1,cx,sx)
		// (sy,sy,sy) (cz,sz,sz) (1,cx,sx) + (1,-1,1) (0,cy,cy) (*,sx,cx)

		Vec::Vector_4V sinVect, cosVect;
		Vec::V4SinAndCos( sinVect, cosVect, radianAngles.GetIntrin128() );

		Vec::Vector_4V _maskYZ = Vec::V4VConstant(V_MASKYZ);
		Vec::Vector_4V _one = Vec::V4VConstant(V_ONE);
		Vec::Vector_4V _negone = Vec::V4VConstant(V_NEGONE);
		Vec::Vector_4V _npnp = Vec::V4MergeXY(_negone, _one);
		Vec::Vector_4V _nppp = Vec::V4MergeXY(_npnp, _one);
		Vec::Vector_4V _nnpp = Vec::V4MergeXY(_npnp, _npnp);

		Vec::Vector_4V sy = Vec::V4SplatY(sinVect);
		Vec::Vector_4V cy = Vec::V4SplatY(cosVect);
		Vec::Vector_4V sz = Vec::V4SplatZ(sinVect);
		Vec::Vector_4V cz = Vec::V4SplatZ(cosVect);

		Vec::Vector_4V z_sy_sy_z = Vec::V4And(sy, _maskYZ);
		Vec::Vector_4V z_cy_cy_z = Vec::V4And(cy, _maskYZ);

		Vec::Vector_4V cz_sz_sz_cz = Vec::V4SelectFT(_maskYZ, cz, sz);
		Vec::Vector_4V sz_cz_cz_sz = Vec::V4SelectFT(_maskYZ, sz, cz);
		Vec::Vector_4V sx_cx_sy_cy = Vec::V4MergeXY(sinVect, cosVect);
		Vec::Vector_4V sx_sx_cx_cx = Vec::V4MergeXY(sx_cx_sy_cy, sx_cx_sy_cy);
		Vec::Vector_4V p_sx_p_sy = Vec::V4MergeXY(_one, sinVect);
		Vec::Vector_4V cx_p_cy_p = Vec::V4MergeXY(cosVect, _one);
		Vec::Vector_4V p_cx_sx_p = Vec::V4MergeXY(p_sx_p_sy, cx_p_cy_p);
		Vec::Vector_4V cz_szcx_szsx = Vec::V4Scale(cz_sz_sz_cz, p_cx_sx_p);

		Vec::Vector_4V col0 = Vec::V4SubtractScaled(Vec::V4Scale(cy, cz_szcx_szsx), sx_sx_cx_cx, Vec::V4Scale(_nnpp, z_sy_sy_z));
		Vec::Vector_4V col1 = Vec::V4Scale(p_cx_sx_p, Vec::V4Scale(_nppp, sz_cz_cz_sz));
		Vec::Vector_4V col2 = Vec::V4SubtractScaled(Vec::V4Scale(sy, cz_szcx_szsx), sx_sx_cx_cx, Vec::V4Scale(_npnp, z_cy_cy_z));

		inoutMat.SetColsIntrin128( col0, col1, col2 );
	}

	inline void Mat33VFromEulersZXY( Mat33V_InOut inoutMat, Vec3V_In radianAngles )
	{
		// (sz,sz,sz) (sx,cx,sx) (sy,1,cy) + (1,1,-1) (cz,0,cz) (cy,0,sy)
		// (cz,cz,cz) (sx,cx,sx) (sy,1,cy) + (-1,1,1) (sz,0,sz) (cy,0,sy)
		// (1,-1,1)   (cx,sx,cx) (sy,1,cy)

		Vec::Vector_4V sinVect, cosVect;
		Vec::V4SinAndCos( sinVect, cosVect, radianAngles.GetIntrin128() );

		Vec::Vector_4V _maskXZ = Vec::V4VConstant(V_MASKXZ);
		Vec::Vector_4V _one = Vec::V4VConstant(V_ONE);
		Vec::Vector_4V _negone = Vec::V4VConstant(V_NEGONE);
		Vec::Vector_4V _pnpn = Vec::V4MergeXY(_one,_negone);
		Vec::Vector_4V _ppnn = Vec::V4MergeXY(_pnpn,_pnpn);
		Vec::Vector_4V _pnnn = Vec::V4MergeXY(_pnpn,_negone);

		Vec::Vector_4V sx = Vec::V4SplatX(sinVect);
		Vec::Vector_4V cx = Vec::V4SplatX(cosVect);
		Vec::Vector_4V sz = Vec::V4SplatZ(sinVect);
		Vec::Vector_4V cz = Vec::V4SplatZ(cosVect);

		Vec::Vector_4V sz_z_sz_z = Vec::V4And(sz, _maskXZ);
		Vec::Vector_4V cz_z_cz_z = Vec::V4And(cz, _maskXZ);

		Vec::Vector_4V cx_sx_cx_sx = Vec::V4MergeXY(cx, sx);
		Vec::Vector_4V sx_cx_sx_cx = Vec::V4MergeXY(sx, cx);

		Vec::Vector_4V sx_cx_sy_cy = Vec::V4MergeXY(sinVect, cosVect);
		Vec::Vector_4V cx_sx_cy_sy = Vec::V4MergeXY(cosVect, sinVect);
		Vec::Vector_4V sy_p_cy_p = Vec::V4MergeZW(sx_cx_sy_cy, _one);
		Vec::Vector_4V cy_sy_sy_cy = Vec::V4MergeZW(cx_sx_cy_sy, sx_cx_sy_cy);
		Vec::Vector_4V cy_z_sy_z = Vec::V4And(cy_sy_sy_cy, _maskXZ);
		Vec::Vector_4V sxsy_cx_sxcy = Vec::V4Scale(sx_cx_sx_cx, sy_p_cy_p);

		Vec::Vector_4V col0 = Vec::V4AddScaled(Vec::V4Scale(sz, sxsy_cx_sxcy), cz_z_cz_z, Vec::V4Scale(_ppnn, cy_z_sy_z));
		Vec::Vector_4V col1 = Vec::V4SubtractScaled(Vec::V4Scale(cz, sxsy_cx_sxcy), sz_z_sz_z, Vec::V4Scale(_pnnn, cy_z_sy_z));
		Vec::Vector_4V col2 = Vec::V4Scale(cx_sx_cx_sx, Vec::V4Scale(_pnpn, sy_p_cy_p));

		inoutMat.SetColsIntrin128( col0, col1, col2 );
	}

	inline void Mat33VFromEulersZYX( Mat33V_InOut inoutMat, Vec3V_In radianAngles )
	{
		Vec::Vector_4V angles = radianAngles.GetIntrin128();
		Vec::Vector_4V outCol0, outCol1, outCol2;
		Vec::Vector_4V sinVect, cosVect;
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V _one = Vec::V4VConstant(V_ONE);
		Vec::Vector_4V _negone = Vec::V4VConstant(V_NEGONE);

		// Split it into a left half matrix and a right half matrix, find some common components, etc.
		// We're constructing the rows shown here, but those are columns in this new vector library.

		// /--------------------------------\   /-------------------\ //
		// | cy*cz  | sx*sy*cz  | -cx*sy*cz |   | 0 | sz*cx | sx*sz | //
		// |--------+-----------+-----------|   |---+-------+-------| //
		// | -sz*cy | -sz*sx*sy | sz*sy*cx  | + | 0 | cx*cz | cz*sx | //
		// |--------+-----------+-----------|   |---+-------+-------| //
		// | sy     | -sx*cy    | cx*cy     |   | 0 | 0     | 0     | //
		// \--------------------------------/   \-------------------/ //

		// Some splatted components.
		Vec::V4SinAndCos( sinVect, cosVect, angles );
		Vec::Vector_4V _sz = Vec::V4SplatZ( sinVect );
		Vec::Vector_4V _cz = Vec::V4SplatZ( cosVect );
		Vec::Vector_4V _sy = Vec::V4SplatY( sinVect );
		Vec::Vector_4V _cy = Vec::V4SplatY( cosVect );

		// Right half.
		Vec::Vector_4V _0_sx_0_sy = Vec::V4MergeXY( _zero, sinVect );
		Vec::Vector_4V _0_cx_sx_cy = Vec::V4MergeXY( _0_sx_0_sy, cosVect );
		Vec::Vector_4V _0_szcx_szsx_szcy = Vec::V4Scale( _sz, _0_cx_sx_cy );
		Vec::Vector_4V _0_czcx_czsx_czcy = Vec::V4Scale( _cz, _0_cx_sx_cy );

		// Left half, top two rows.
		Vec::Vector_4V _sxsy_sysy_szsy = Vec::V4Scale( sinVect, _sy );
		Vec::Vector_4V _cxsy_cysy_czsy = Vec::V4Scale( cosVect, _sy );
		Vec::Vector_4V _cy_cxsy_cy_cysy = Vec::V4MergeXY( _cy, _cxsy_cysy_czsy );
		Vec::Vector_4V _cy_sxsy_cxsy_sysy = Vec::V4MergeXY( _cy_cxsy_cy_cysy, _sxsy_sysy_szsy );

		// Left half, bottom-row.
		Vec::Vector_4V _sx_cx_sy_cy = Vec::V4MergeXY( sinVect, cosVect );
		Vec::Vector_4V _sxcy_cxcy_sycy_cycy = Vec::V4Scale( _sx_cx_sy_cy, _cy );
		Vec::Vector_4V _sy_sxcy_cxcy_sycy = Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>( _sy, _sxcy_cxcy_sycy_cycy );

		// Negators.
		Vec::Vector_4V _pnpn = Vec::V4MergeXY( _one, _negone );
		Vec::Vector_4V _ppnn = Vec::V4MergeXY( _pnpn, _pnpn );

		// Add both halves.
		outCol0 = Vec::V4AddScaled( _0_szcx_szsx_szcy, _cy_sxsy_cxsy_sysy, Vec::V4Scale(_cz, _ppnn) );
		outCol1 = Vec::V4SubtractScaled( _0_czcx_czsx_czcy, _cy_sxsy_cxsy_sysy, Vec::V4Scale(_sz, _ppnn) );
		outCol2 = Vec::V4AddScaled( _zero, _sy_sxcy_cxcy_sycy, _pnpn );

		// Set the output matrix.
		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline void Mat34VFromEulersXYZ( Mat34V_InOut inoutMat, Vec3V_In radianAngles, Vec3V_In translation )
	{
		Mat33VFromEulersXYZ( inoutMat.GetMat33Ref(), radianAngles );
		inoutMat.SetCol3Intrin128( translation.GetIntrin128() );
	}

	inline void Mat34VFromEulersXZY( Mat34V_InOut inoutMat, Vec3V_In radianAngles, Vec3V_In translation )
	{
		Mat33VFromEulersXZY( inoutMat.GetMat33Ref(), radianAngles );
		inoutMat.SetCol3Intrin128( translation.GetIntrin128() );
	}

	inline void Mat34VFromEulersYXZ( Mat34V_InOut inoutMat, Vec3V_In radianAngles, Vec3V_In translation )
	{
		Mat33VFromEulersYXZ( inoutMat.GetMat33Ref(), radianAngles );
		inoutMat.SetCol3Intrin128( translation.GetIntrin128() );
	}

	inline void Mat34VFromEulersYZX( Mat34V_InOut inoutMat, Vec3V_In radianAngles, Vec3V_In translation )
	{
		Mat33VFromEulersYZX( inoutMat.GetMat33Ref(), radianAngles );
		inoutMat.SetCol3Intrin128( translation.GetIntrin128() );
	}

	inline void Mat34VFromEulersZXY( Mat34V_InOut inoutMat, Vec3V_In radianAngles, Vec3V_In translation )
	{
		Mat33VFromEulersZXY( inoutMat.GetMat33Ref(), radianAngles );
		inoutMat.SetCol3Intrin128( translation.GetIntrin128() );
	}

	inline void Mat34VFromEulersZYX( Mat34V_InOut inoutMat, Vec3V_In radianAngles, Vec3V_In translation )
	{
		Mat33VFromEulersZYX( inoutMat.GetMat33Ref(), radianAngles );
		inoutMat.SetCol3Intrin128( translation.GetIntrin128() );
	}

	__forceinline void Mat33VFromScale( Mat33V_InOut inoutMat, Vec3V_In scaleAmounts )
	{
		Vec::Vector_4V _xyz = scaleAmounts.GetIntrin128();
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		Vec::Vector_4V _x0y0 = Vec::V4MergeXY( _xyz, _zero );
		Vec::Vector_4V _0z0 = Vec::V4MergeZW( _zero, _xyz );
		Vec::Vector_4V _x000 = Vec::V4MergeXY( _x0y0, _zero );
		Vec::Vector_4V _0y00 = Vec::V4MergeZW( _zero, _x0y0 );
		Vec::Vector_4V _00z0 = Vec::V4MergeXY( _0z0, _zero );

		inoutMat.SetColsIntrin128( _x000, _0y00, _00z0 );
	}

	__forceinline void Mat33VFromScale( Mat33V_InOut inoutMat, ScalarV_In scaleX, ScalarV_In scaleY, ScalarV_In scaleZ )
	{
		Vec::Vector_4V _x = scaleX.GetIntrin128();
		Vec::Vector_4V _y = scaleY.GetIntrin128();
		Vec::Vector_4V _z = scaleZ.GetIntrin128();
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		Vec::Vector_4V _x000 = Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>( _x, _zero );
		Vec::Vector_4V _0y0y = Vec::V4MergeXY( _zero, _y );
		Vec::Vector_4V _00zz = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( _zero, _z  );

		inoutMat.SetColsIntrin128( _x000, _0y0y, _00zz );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) ScaleFromMat33VTranspose( Mat33V_In inMat )
	{
		Mat33V tMat;
		Transpose(tMat, inMat);
		Vec::Vector_4V sqX = Vec::V4Scale( tMat.GetCol0Intrin128(), tMat.GetCol0Intrin128() );
		Vec::Vector_4V sqY = Vec::V4Scale( tMat.GetCol1Intrin128(), tMat.GetCol1Intrin128()  );
		Vec::Vector_4V sqZ = Vec::V4Scale( tMat.GetCol2Intrin128(), tMat.GetCol2Intrin128() );
		Vec::Vector_4V sqMag = Vec::V4Add(sqX, Vec::V4Add(sqY, sqZ));
		return Vec3V(Vec::V4Sqrt(sqMag));
	}

	__forceinline void ScaleTranspose( Mat33V_InOut outMat, Vec3V_In inScale, Mat33V_In inMat )
	{
		Vec::Vector_4V scale = inScale.GetIntrin128();
		Vec::Vector_4V scaleX = Vec::V4SplatX(scale);
		Vec::Vector_4V scaleY = Vec::V4SplatY(scale);
		Vec::Vector_4V scaleZ = Vec::V4SplatZ(scale);
		outMat.SetColsIntrin128(Vec::V4Scale(scaleX, inMat.GetCol0Intrin128()), Vec::V4Scale(scaleY, inMat.GetCol1Intrin128()), Vec::V4Scale(scaleZ, inMat.GetCol2Intrin128()));
	}

	inline void Mat44VFromAxisAngle( Mat44V_InOut inoutMat, Vec3V_In normAxis, ScalarV_In radians, Vec4V_In translation )
	{
		Vec::Vector_4V sinVect;
		Vec::Vector_4V cosVect;
		Vec::Vector_4V tempVect0;
		Vec::Vector_4V tempVect1;
		Vec::Vector_4V tempVect2;
		Vec::Vector_4V sinTimesAxis;
		Vec::Vector_4V negSinTimesAxis;
		Vec::Vector_4V outCol0;
		Vec::Vector_4V outCol1;
		Vec::Vector_4V outCol2;
		Vec::Vector_4V maskxyz = Vec::V4VConstant(V_MASKXYZ);

		Vec::Vector_4V _normAxis = normAxis.GetIntrin128();
		Vec::Vector_4V _xxxx = Vec::V4SplatX( _normAxis );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( _normAxis );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( _normAxis );

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		Vec::Vector_4V oneMinusCos = Vec::V4Subtract( Vec::V4VConstant(V_ONE), cosVect );

		sinTimesAxis = Vec::V4Scale( _normAxis, sinVect );
		negSinTimesAxis = Vec::V4Negate( sinTimesAxis );

		tempVect0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z1,Vec::Y2,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect1 = Vec::V4PermuteTwo<Vec::Z2,Vec::X1,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect2 = Vec::V4PermuteTwo<Vec::Y1,Vec::X2,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );

		tempVect0 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKX), tempVect0, cosVect );
		tempVect1 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKY), tempVect1, cosVect );
		tempVect2 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKZ), tempVect2, cosVect );

		outCol0 = Vec::V4AddScaled( tempVect0, Vec::V4Scale( _normAxis, _xxxx ), oneMinusCos );
		outCol1 = Vec::V4AddScaled( tempVect1, Vec::V4Scale( _normAxis, _yyyy ), oneMinusCos );
		outCol2 = Vec::V4AddScaled( tempVect2, Vec::V4Scale( _normAxis, _zzzz ), oneMinusCos );
		outCol0 = Vec::V4And( outCol0, maskxyz );
		outCol1 = Vec::V4And( outCol1, maskxyz );
		outCol2 = Vec::V4And( outCol2, maskxyz );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat44VFromXAxisAngle( Mat44V_InOut inoutMat, ScalarV_In radians, Vec4V_In translation )
	{
		Vec::Vector_4V _zero;
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _1000, _0_sin_0_sin, _0_cos_0_cos, _cos_0_cos_0, _negSin_0_negSin_0;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_zero = Vec::V4VConstant(V_ZERO);
		_1000 = Vec::V4VConstant(V_X_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );
		
		_0_sin_0_sin = Vec::V4MergeXY( _zero, sinVect );
		_0_cos_0_cos = Vec::V4MergeXY( _zero, cosVect );
		
		_cos_0_cos_0 = Vec::V4MergeXY( cosVect, _zero );
		_negSin_0_negSin_0 = Vec::V4MergeXY( negSinVect, _zero );

		outCol0 = _1000;
		outCol1 = Vec::V4MergeXY( _0_sin_0_sin, _cos_0_cos_0 );
		outCol2 = Vec::V4MergeXY( _0_cos_0_cos, _negSin_0_negSin_0 );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat44VFromYAxisAngle( Mat44V_InOut inoutMat, ScalarV_In radians, Vec4V_In translation )
	{
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _zero, _0100, _cos_negSin_cos_negSin, _sin_cos_sin_cos;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_zero = Vec::V4VConstant(V_ZERO);
		_0100 = Vec::V4VConstant(V_Y_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );

		_cos_negSin_cos_negSin = Vec::V4MergeXY( cosVect, negSinVect );
		_sin_cos_sin_cos = Vec::V4MergeXY( sinVect, cosVect );

		outCol0 = Vec::V4MergeXY( _cos_negSin_cos_negSin, _zero );
		outCol1 = _0100;
		outCol2 = Vec::V4MergeXY( _sin_cos_sin_cos, _zero );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat44VFromZAxisAngle( Mat44V_InOut inoutMat, ScalarV_In radians, Vec4V_In translation )
	{
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _zero, _0010, _cos_0_cos_0, _negSin_0_negSin_0, _sin_0_sin_0;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_zero = Vec::V4VConstant(V_ZERO);
		_0010 = Vec::V4VConstant(V_Z_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );

		_cos_0_cos_0 = Vec::V4MergeXY( cosVect, _0010 );
		_negSin_0_negSin_0 = Vec::V4MergeXY( negSinVect, _0010 );
		_sin_0_sin_0 = Vec::V4MergeXY( sinVect, _zero );

		outCol0 = Vec::V4MergeXY( _cos_0_cos_0, _sin_0_sin_0 );
		outCol1 = Vec::V4MergeXY( _negSin_0_negSin_0, _cos_0_cos_0 );
		outCol2 = _0010;

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat44VFromQuatV( Mat44V_InOut inoutMat, QuatV_In inQuat, Vec4V_In translation )
	{
		Vec::Vector_4V q = inQuat.GetIntrin128();
		Vec::Vector_4V maskX = Vec::V4VConstant(V_MASKX);
		Vec::Vector_4V maskZ = Vec::V4VConstant(V_MASKZW);
		Vec::Vector_4V maskxyz = Vec::V4VConstant(V_MASKXYZ);

		Vec::Vector_4V outCol0, outCol1, outCol2;
		Vec::Vector_4V _wwww, _yzxw, _zxyw;
		Vec::Vector_4V _xyzwSq, _yzxwSq, _zxywSq;
		Vec::Vector_4V tempVect0, tempVect1, tempVect2, tempVect3, tempVect4, tempVect5;

		_xyzwSq = Vec::V4Add( q, q );
		_wwww = Vec::V4SplatW( q );
		_yzxw = Vec::V4Permute<Vec::Y,Vec::Z,Vec::X,Vec::W>( q );
		_zxyw = Vec::V4Permute<Vec::Z,Vec::X,Vec::Y,Vec::W>( q );
		_yzxwSq = Vec::V4Permute<Vec::Y,Vec::Z,Vec::X,Vec::W>( _xyzwSq );
		_zxywSq = Vec::V4Permute<Vec::Z,Vec::X,Vec::Y,Vec::W>( _xyzwSq );

		tempVect0 = Vec::V4Scale( _yzxwSq, _wwww );
		tempVect1 = Vec::V4SubtractScaled( Vec::V4VConstant(V_ONE), _yzxwSq, _yzxw );
		tempVect2 = Vec::V4Scale( _yzxw, _xyzwSq );
		tempVect0 = Vec::V4AddScaled( tempVect0, _zxyw, _xyzwSq );
		tempVect1 = Vec::V4SubtractScaled( tempVect1, _zxywSq, _zxyw );
		tempVect2 = Vec::V4SubtractScaled( tempVect2, _wwww, _zxywSq );
		tempVect3 = Vec::V4SelectFT( maskX, tempVect0, tempVect1 );
		tempVect4 = Vec::V4SelectFT( maskX, tempVect1, tempVect2 );
		tempVect5 = Vec::V4SelectFT( maskX, tempVect2, tempVect0 );

		outCol0 = Vec::V4SelectFT( maskZ, tempVect3, tempVect2 );
		outCol1 = Vec::V4SelectFT( maskZ, tempVect4, tempVect0 );
		outCol2 = Vec::V4SelectFT( maskZ, tempVect5, tempVect1 );
		outCol0 = Vec::V4And( outCol0, maskxyz );
		outCol1 = Vec::V4And( outCol1, maskxyz );
		outCol2 = Vec::V4And( outCol2, maskxyz );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	__forceinline void Mat44VFromScale( Mat44V_InOut inoutMat, Vec3V_In scaleAmounts, Vec4V_In translation )
	{
		Vec::Vector_4V _xyz = scaleAmounts.GetIntrin128();
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		Vec::Vector_4V _x0y0 = Vec::V4MergeXY( _xyz, _zero );
		Vec::Vector_4V _0z0 = Vec::V4MergeZW( _zero, _xyz );
		Vec::Vector_4V _x000 = Vec::V4MergeXY( _x0y0, _zero );
		Vec::Vector_4V _0y00 = Vec::V4MergeZW( _zero, _x0y0 );
		Vec::Vector_4V _00z0 = Vec::V4MergeXY( _0z0, _zero );

		inoutMat.SetColsIntrin128( _x000, _0y00, _00z0, translation.GetIntrin128() );
	}

	__forceinline void Mat44VFromScale( Mat44V_InOut inoutMat, ScalarV_In scaleX, ScalarV_In scaleY, ScalarV_In scaleZ, Vec4V_In translation )
	{
		Vec::Vector_4V _x = scaleX.GetIntrin128();
		Vec::Vector_4V _y = scaleY.GetIntrin128();
		Vec::Vector_4V _z = scaleZ.GetIntrin128();
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		Vec::Vector_4V _x000 = Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>( _x, _zero );
		Vec::Vector_4V _0y00 = Vec::V4MergeXY( _zero, Vec::V4MergeXY( _y, _zero ) );
		Vec::Vector_4V _0z0 = Vec::V4MergeXY( _zero, _z );
		Vec::Vector_4V _00z0 = Vec::V4MergeXY( _0z0, _zero );

		inoutMat.SetColsIntrin128( _x000, _0y00, _00z0, translation.GetIntrin128() );
	}

	__forceinline void Mat44VFromTranslation( Mat44V_InOut inoutMat, Vec4V_In translation )
	{
		Vec::Vector_4V outCol0, outCol1, outCol2;
#if __XENON && UNIQUE_VECTORIZED_TYPE // take advantage of __vupkd3d
		Vec::Vector_4V zeroInZ_oneInW = __vupkd3d(Vec::V4VConstant(V_ZERO), VPACK_NORMSHORT2);
		outCol0 = Vec::V4Permute<Vec::W,Vec::Z,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		outCol1 = Vec::V4Permute<Vec::Z,Vec::W,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		outCol2 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::W,Vec::Z>( zeroInZ_oneInW );
#else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		outCol0 = Vec::V4VConstant(V_X_AXIS_WZERO);
		outCol1 = Vec::V4MergeXY( _zero, outCol0 );
		outCol2 = Vec::V4MergeXY( outCol1, _zero );
#endif
		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersXYZ( Mat33V_In inMat )
	{
		// atan2(b.z,c.z), asin(-a.z), atan2(a.y,a.x)
		Vec::Vector_4V col0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V col1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V col2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V scale = Vec::V4VConstant<U32_ONE,U32_NEGONE,U32_ONE,U32_ZERO>();
		Vec::Vector_4V sinAngle0 = Vec::V4PermuteTwo<Vec::Z2,Vec::Z1,Vec::Y1,Vec::X1>(col0, col1);
		Vec::Vector_4V sinAngle1 = Vec::V4Scale(scale, sinAngle0);

		Vec::Vector_4V cosDerived = Vec::V4Sqrt(Vec::V4Subtract(Vec::V4VConstant(V_ONE), Vec::V4Scale(col0,col0)));
		Vec::Vector_4V cosAngle0 = Vec::V4PermuteTwo<Vec::Z2,Vec::X1,Vec::X1,Vec::X1>(col0, col2);
		Vec::Vector_4V cosAngle1 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Z1,Vec::X1>(cosAngle0, cosDerived);

		return Vec3V(Vec::V4Arctan2(sinAngle1, cosAngle1));
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersXZY( Mat33V_In inMat )
	{
		// atan2(-c.y,b.y), atan2(-a.z,a.x), asin(a.y)
		Vec::Vector_4V col0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V col1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V col2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V scale = Vec::V4VConstant<U32_NEGONE,U32_NEGONE,U32_ONE,U32_ZERO>();
		Vec::Vector_4V sinAngle0 = Vec::V4PermuteTwo<Vec::Y2,Vec::Z1,Vec::Y1,Vec::X1>(col0, col2);
		Vec::Vector_4V sinAngle1 = Vec::V4Scale(scale, sinAngle0);

		Vec::Vector_4V cosDerived = Vec::V4Sqrt(Vec::V4Subtract(Vec::V4VConstant(V_ONE), Vec::V4Scale(col0,col0)));
		Vec::Vector_4V cosAngle0 = Vec::V4PermuteTwo<Vec::Y2,Vec::X1,Vec::X1,Vec::X1>(col0, col1);
		Vec::Vector_4V cosAngle1 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::Y2,Vec::X1>(cosAngle0, cosDerived);

		return Vec3V(Vec::V4Arctan2(sinAngle1, cosAngle1));
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersYXZ( Mat33V_In inMat )
	{
		// asin(b.z), atan2(-a.z,c.z), atan2(-b.x,b.y)
		Vec::Vector_4V col0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V col1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V col2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V scale = Vec::V4VConstant<U32_ONE,U32_NEGONE,U32_NEGONE,U32_ZERO>();
		Vec::Vector_4V sinAngle0 = Vec::V4PermuteTwo<Vec::Z2,Vec::Z1,Vec::X2,Vec::X1>(col0, col1);
		Vec::Vector_4V sinAngle1 = Vec::V4Scale(scale, sinAngle0);

		Vec::Vector_4V cosDerived = Vec::V4Sqrt(Vec::V4Subtract(Vec::V4VConstant(V_ONE), Vec::V4Scale(col1,col1)));
		Vec::Vector_4V cosAngle0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>(col1, col2);
		Vec::Vector_4V cosAngle1 = Vec::V4PermuteTwo<Vec::Z2,Vec::Y1,Vec::Z1,Vec::X1>(cosAngle0, cosDerived);

		return Vec3V(Vec::V4Arctan2(sinAngle1, cosAngle1));
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersYZX( Mat33V_In inMat )
	{
		// atan2(b.z,b.y), atan2(c.x,a.x), asin(-b.x)
		Vec::Vector_4V col0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V col1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V col2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V scale = Vec::V4VConstant<U32_ONE,U32_ONE,U32_NEGONE,U32_ZERO>();
		Vec::Vector_4V sinAngle0 = Vec::V4PermuteTwo<Vec::Z1,Vec::X2,Vec::X1,Vec::X1>(col1, col2);
		Vec::Vector_4V sinAngle1 = Vec::V4Scale(scale, sinAngle0);

		Vec::Vector_4V cosDerived = Vec::V4Sqrt(Vec::V4Subtract(Vec::V4VConstant(V_ONE), Vec::V4Scale(col1,col1)));
		Vec::Vector_4V cosAngle0 = Vec::V4PermuteTwo<Vec::Y2,Vec::X1,Vec::X1,Vec::X1>(col0, col1);
		Vec::Vector_4V cosAngle1 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::X1>(cosAngle0, cosDerived);

		return Vec3V(Vec::V4Arctan2(sinAngle1, cosAngle1));
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersZXY( Mat33V_In inMat )
	{
		// asin(-c.y), atan2(c.x,c.z), atan2(a.y,b.y)
		Vec::Vector_4V col0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V col1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V col2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V scale = Vec::V4VConstant<U32_NEGONE,U32_ONE,U32_ONE,U32_ZERO>();
		Vec::Vector_4V sinAngle0 = Vec::V4PermuteTwo<Vec::Y2,Vec::X2,Vec::Y1,Vec::X1>(col0, col2);
		Vec::Vector_4V sinAngle1 = Vec::V4Scale(scale, sinAngle0);

		Vec::Vector_4V cosDerived = Vec::V4Sqrt(Vec::V4Subtract(Vec::V4VConstant(V_ONE), Vec::V4Scale(col2,col2)));
		Vec::Vector_4V cosAngle0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>(col1, col2);
		Vec::Vector_4V cosAngle1 = Vec::V4PermuteTwo<Vec::Y2,Vec::Y1,Vec::Z1,Vec::X1>(cosAngle0, cosDerived);

		return Vec3V(Vec::V4Arctan2(sinAngle1, cosAngle1));
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersZYX( Mat33V_In inMat )
	{
		// atan2(-c.y,c.z), asin(c.x), atan2(-b.x,a.x)
		Vec::Vector_4V col0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V col1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V col2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V scale = Vec::V4VConstant<U32_NEGONE,U32_ONE,U32_NEGONE,U32_ZERO>();
		Vec::Vector_4V sinAngle0 = Vec::V4PermuteTwo<Vec::Y2,Vec::X2,Vec::X1,Vec::X1>(col1, col2);
		Vec::Vector_4V sinAngle1 = Vec::V4Scale(scale, sinAngle0);

		Vec::Vector_4V cosDerived = Vec::V4Sqrt(Vec::V4Subtract(Vec::V4VConstant(V_ONE), Vec::V4Scale(col2,col2)));
		Vec::Vector_4V cosAngle0 = Vec::V4PermuteTwo<Vec::Z2,Vec::X1,Vec::X1,Vec::X1>(col0, col2);
		Vec::Vector_4V cosAngle1 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::X1>(cosAngle0, cosDerived);

		return Vec3V(Vec::V4Arctan2(sinAngle1, cosAngle1));
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersXYZ( Mat34V_In inMat )
	{
		return Mat33VToEulersXYZ(inMat.GetMat33ConstRef());
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersXZY( Mat34V_In inMat )
	{
		return Mat33VToEulersXZY(inMat.GetMat33ConstRef());
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersYXZ( Mat34V_In inMat )
	{
		return Mat33VToEulersYXZ(inMat.GetMat33ConstRef());
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersYZX( Mat34V_In inMat )
	{
		return Mat33VToEulersYZX(inMat.GetMat33ConstRef());
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersZXY( Mat34V_In inMat )
	{
		return Mat33VToEulersZXY(inMat.GetMat33ConstRef());
	}

	inline FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersZYX( Mat34V_In inMat )
	{
		return Mat33VToEulersZYX(inMat.GetMat33ConstRef());
	}

	inline void Mat34VFromAxisAngle( Mat34V_InOut inoutMat, Vec3V_In normAxis, ScalarV_In radians, Vec3V_In translation )
	{
		Vec::Vector_4V sinVect;
		Vec::Vector_4V cosVect;
		Vec::Vector_4V tempVect0;
		Vec::Vector_4V tempVect1;
		Vec::Vector_4V tempVect2;
		Vec::Vector_4V sinTimesAxis;
		Vec::Vector_4V negSinTimesAxis;
		Vec::Vector_4V outCol0;
		Vec::Vector_4V outCol1;
		Vec::Vector_4V outCol2;

		Vec::Vector_4V _normAxis = normAxis.GetIntrin128();
		Vec::Vector_4V _xxxx = Vec::V4SplatX( _normAxis );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( _normAxis );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( _normAxis );

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		Vec::Vector_4V oneMinusCos = Vec::V4Subtract( Vec::V4VConstant(V_ONE), cosVect );

		sinTimesAxis = Vec::V4Scale( _normAxis, sinVect );
		negSinTimesAxis = Vec::V4Negate( sinTimesAxis );

		tempVect0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z1,Vec::Y2,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect1 = Vec::V4PermuteTwo<Vec::Z2,Vec::X1,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );
		tempVect2 = Vec::V4PermuteTwo<Vec::Y1,Vec::X2,Vec::X1,Vec::X1>( sinTimesAxis, negSinTimesAxis );

		tempVect0 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKX), tempVect0, cosVect );
		tempVect1 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKY), tempVect1, cosVect );
		tempVect2 = Vec::V4SelectFT( Vec::V4VConstant(V_MASKZ), tempVect2, cosVect );

		outCol0 = Vec::V4AddScaled( tempVect0, Vec::V4Scale( _normAxis, _xxxx ), oneMinusCos );
		outCol1 = Vec::V4AddScaled( tempVect1, Vec::V4Scale( _normAxis, _yyyy ), oneMinusCos );
		outCol2 = Vec::V4AddScaled( tempVect2, Vec::V4Scale( _normAxis, _zzzz ), oneMinusCos );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat34VFromXAxisAngle( Mat34V_InOut inoutMat, ScalarV_In radians, Vec3V_In translation )
	{
		Vec::Vector_4V _zero;
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _1000, _0_sin_0_sin, _0_cos_0_cos;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_zero = Vec::V4VConstant(V_ZERO);
		_1000 = Vec::V4VConstant(V_X_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );
		
		_0_sin_0_sin = Vec::V4MergeXY( _zero, sinVect );
		_0_cos_0_cos = Vec::V4MergeXY( _zero, cosVect );

		outCol0 = _1000;
		outCol1 = Vec::V4MergeXY( _0_sin_0_sin, cosVect );
		outCol2 = Vec::V4MergeXY( _0_cos_0_cos, negSinVect );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat34VFromYAxisAngle( Mat34V_InOut inoutMat, ScalarV_In radians, Vec3V_In translation )
	{
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _0000, _0100, _cos_negSin_cos_negSin, _sin_cos_sin_cos;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_0000 = Vec::V4VConstant(V_ZERO);
		_0100 = Vec::V4VConstant(V_Y_AXIS_WZERO);
		negSinVect = Vec::V4Negate( sinVect );

		_cos_negSin_cos_negSin = Vec::V4MergeXY( cosVect, negSinVect );
		_sin_cos_sin_cos = Vec::V4MergeXY( sinVect, cosVect );

		outCol0 = Vec::V4MergeXY( _cos_negSin_cos_negSin, _0000 );
		outCol1 = _0100;
		outCol2 = Vec::V4MergeXY( _sin_cos_sin_cos, _0000 );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	inline void Mat34VFromZAxisAngle( Mat34V_InOut inoutMat, ScalarV_In radians, Vec3V_In translation )
	{
		Vec::Vector_4V sinVect, cosVect, negSinVect;
		Vec::Vector_4V _0011, _cos_0_cos_0, _negSin_0_negSin_0;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		Vec::V4SinAndCos( sinVect, cosVect, radians.GetIntrin128() );

		_0011 = Vec::V4VConstant(V_Z_AXIS_WONE);
		negSinVect = Vec::V4Negate( sinVect );

		_cos_0_cos_0 = Vec::V4MergeXY( cosVect, _0011 );
		_negSin_0_negSin_0 = Vec::V4MergeXY( negSinVect, _0011 );

		outCol0 = Vec::V4MergeXY( _cos_0_cos_0, sinVect );
		outCol1 = Vec::V4MergeXY( _negSin_0_negSin_0, cosVect );
		outCol2 = _0011;

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}


	inline void Mat34VRotateLocalX( Mat34V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V cosine,sine;
		Vec::V4SinAndCos( sine, cosine, radians.GetIntrin128() );

		Vec::Vector_4V b = inoutMat.GetCol1Intrin128();
		Vec::Vector_4V c = inoutMat.GetCol2Intrin128();

		inoutMat.SetCol1Intrin128( Vec::V4AddScaled( Vec::V4Scale( b, cosine ), c, sine) );
		inoutMat.SetCol2Intrin128( Vec::V4SubtractScaled( Vec::V4Scale( c, cosine), b, sine ) );		
	}


	inline void Mat34VRotateLocalY( Mat34V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V cosine,sine;
		Vec::V4SinAndCos( sine, cosine, radians.GetIntrin128() );

		Vec::Vector_4V a = inoutMat.GetCol0Intrin128();
		Vec::Vector_4V c = inoutMat.GetCol2Intrin128();

		inoutMat.SetCol0Intrin128( Vec::V4SubtractScaled( Vec::V4Scale( a, cosine ), c, sine) );
		inoutMat.SetCol2Intrin128( Vec::V4AddScaled( Vec::V4Scale( c, cosine), a, sine ) );		
	}


	inline void Mat34VRotateLocalZ( Mat34V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V cosine,sine;
		Vec::V4SinAndCos( sine, cosine, radians.GetIntrin128() );

		Vec::Vector_4V a = inoutMat.GetCol0Intrin128();
		Vec::Vector_4V b = inoutMat.GetCol1Intrin128();
		
		inoutMat.SetCol0Intrin128( Vec::V4AddScaled( Vec::V4Scale( a, cosine ), b, sine) );
		inoutMat.SetCol1Intrin128( Vec::V4SubtractScaled( Vec::V4Scale( b, cosine), a, sine ) );		
	}


	inline void Mat34VRotateGlobalX( Mat34V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V sine, cosine;
		Vec::V4SinAndCos( sine, cosine, radians.GetIntrin128() );
		inoutMat.SetCol0Intrin128( Vec::V3RotateAboutXAxis( inoutMat.GetCol0Intrin128(), sine, cosine ) );
		inoutMat.SetCol1Intrin128( Vec::V3RotateAboutXAxis( inoutMat.GetCol1Intrin128(), sine, cosine ) );
		inoutMat.SetCol2Intrin128( Vec::V3RotateAboutXAxis( inoutMat.GetCol2Intrin128(), sine, cosine ) );
	}


	inline void Mat34VRotateGlobalY( Mat34V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V sine, cosine;
		Vec::V4SinAndCos( sine, cosine, radians.GetIntrin128() );
		inoutMat.SetCol0Intrin128( Vec::V3RotateAboutYAxis( inoutMat.GetCol0Intrin128(), sine, cosine ) );
		inoutMat.SetCol1Intrin128( Vec::V3RotateAboutYAxis( inoutMat.GetCol1Intrin128(), sine, cosine ) );
		inoutMat.SetCol2Intrin128( Vec::V3RotateAboutYAxis( inoutMat.GetCol2Intrin128(), sine, cosine ) );
	}


	inline void Mat34VRotateGlobalZ( Mat34V_InOut inoutMat, ScalarV_In radians )
	{
		Vec::Vector_4V sine, cosine;
		Vec::V4SinAndCos( sine, cosine, radians.GetIntrin128() );
		inoutMat.SetCol0Intrin128( Vec::V3RotateAboutZAxis( inoutMat.GetCol0Intrin128(), sine, cosine ) );
		inoutMat.SetCol1Intrin128( Vec::V3RotateAboutZAxis( inoutMat.GetCol1Intrin128(), sine, cosine ) );
		inoutMat.SetCol2Intrin128( Vec::V3RotateAboutZAxis( inoutMat.GetCol2Intrin128(), sine, cosine ) );
	}


	inline void Mat34VFromQuatV( Mat34V_InOut inoutMat, QuatV_In inQuat, Vec3V_In translation )
	{
		Vec::Vector_4V q = inQuat.GetIntrin128();
		Vec::Vector_4V maskX = Vec::V4VConstant(V_MASKX);
		Vec::Vector_4V maskZ = Vec::V4VConstant(V_MASKZW);

		Vec::Vector_4V outCol0, outCol1, outCol2;
		Vec::Vector_4V _wwww, _yzxw, _zxyw;
		Vec::Vector_4V _xyzwSq, _yzxwSq, _zxywSq;
		Vec::Vector_4V tempVect0, tempVect1, tempVect2, tempVect3, tempVect4, tempVect5;

		_xyzwSq = Vec::V4Add( q, q );
		_wwww = Vec::V4SplatW( q );
		_yzxw = Vec::V4Permute<Vec::Y,Vec::Z,Vec::X,Vec::W>( q );
		_zxyw = Vec::V4Permute<Vec::Z,Vec::X,Vec::Y,Vec::W>( q );
		_yzxwSq = Vec::V4Permute<Vec::Y,Vec::Z,Vec::X,Vec::W>( _xyzwSq );
		_zxywSq = Vec::V4Permute<Vec::Z,Vec::X,Vec::Y,Vec::W>( _xyzwSq );

		tempVect0 = Vec::V4Scale( _yzxwSq, _wwww );
		tempVect1 = Vec::V4SubtractScaled( Vec::V4VConstant(V_ONE), _yzxwSq, _yzxw );
		tempVect2 = Vec::V4Scale( _yzxw, _xyzwSq );
		tempVect0 = Vec::V4AddScaled( tempVect0, _zxyw, _xyzwSq );
		tempVect1 = Vec::V4SubtractScaled( tempVect1, _zxywSq, _zxyw );
		tempVect2 = Vec::V4SubtractScaled( tempVect2, _wwww, _zxywSq );
		tempVect3 = Vec::V4SelectFT( maskX, tempVect0, tempVect1 );
		tempVect4 = Vec::V4SelectFT( maskX, tempVect1, tempVect2 );
		tempVect5 = Vec::V4SelectFT( maskX, tempVect2, tempVect0 );

		outCol0 = Vec::V4SelectFT( maskZ, tempVect3, tempVect2 );
		outCol1 = Vec::V4SelectFT( maskZ, tempVect4, tempVect0 );
		outCol2 = Vec::V4SelectFT( maskZ, tempVect5, tempVect1 );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, translation.GetIntrin128() );
	}

	__forceinline void Mat34VFromScale( Mat34V_InOut inoutMat, Vec3V_In scaleAmounts, Vec3V_In translation )
	{
		Vec::Vector_4V _xyz = scaleAmounts.GetIntrin128();
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		Vec::Vector_4V _x0y0 = Vec::V4MergeXY( _xyz, _zero );
		Vec::Vector_4V _0z0 = Vec::V4MergeZW( _zero, _xyz );
		Vec::Vector_4V _x000 = Vec::V4MergeXY( _x0y0, _zero );
		Vec::Vector_4V _0y00 = Vec::V4MergeZW( _zero, _x0y0 );
		Vec::Vector_4V _00z0 = Vec::V4MergeXY( _0z0, _zero );

		inoutMat.SetColsIntrin128( _x000, _0y00, _00z0, translation.GetIntrin128() );
	}

	__forceinline void Mat34VFromScale( Mat34V_InOut inoutMat, ScalarV_In scaleX, ScalarV_In scaleY, ScalarV_In scaleZ, Vec3V_In translation )
	{
		Vec::Vector_4V _x = scaleX.GetIntrin128();
		Vec::Vector_4V _y = scaleY.GetIntrin128();
		Vec::Vector_4V _z = scaleZ.GetIntrin128();
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		Vec::Vector_4V _x000 = Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>( _x, _zero );
		Vec::Vector_4V _0y0y = Vec::V4MergeXY( _zero, _y );
		Vec::Vector_4V _00zz = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( _zero, _z  );

		inoutMat.SetColsIntrin128( _x000, _0y0y, _00zz, translation.GetIntrin128() );
	}

	__forceinline void Mat34VFromTranslation( Mat34V_InOut inoutMat, Vec3V_In translation )
	{
		inoutMat = Mat34V( Mat33V(V_IDENTITY), translation );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) ScalarVFromF32( const float& f )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, f );
		return ScalarV( out );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) ScalarVFromU32( const u32& u )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, u );
		return ScalarV( out );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) ScalarVFromS32( const s32& s )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, s );
		return ScalarV( out );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Vec2VFromF32( const float& f )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, f );
		return Vec2V( out );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Vec2VFromU32( const u32& u )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, u );
		return Vec2V( out );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Vec2VFromS32( const s32& s )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, s );
		return Vec2V( out );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Vec3VFromF32( const float& f )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, f );
		return Vec3V( out );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Vec3VFromU32( const u32& u )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, u );
		return Vec3V( out );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Vec3VFromS32( const s32& s )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, s );
		return Vec3V( out );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Vec4VFromF32( const float& f )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, f );
		return Vec4V( out );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Vec4VFromU32( const u32& u )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, u );
		return Vec4V( out );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Vec4VFromS32( const s32& s )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, s );
		return Vec4V( out );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromF32( const float& f )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, f );
		return QuatV( out );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromU32( const u32& u )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, u );
		return QuatV( out );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromS32( const s32& s )
	{
		Vec::Vector_4V out;
		Vec::V4Set( out, s );
		return QuatV( out );
	}

	template <u32 floatAsInt>
	__forceinline const FASTRETURNCHECK(ScalarV_Out) ScalarVConstant()
	{
		return ScalarV( Vec::V4VConstantSplat<floatAsInt>() );
	}

	template <u32 floatAsIntX, u32 floatAsIntY>
	__forceinline const FASTRETURNCHECK(Vec2V_Out) Vec2VConstant()
	{
		return Vec2V( Vec::V4VConstant<floatAsIntX,floatAsIntY,floatAsIntX,floatAsIntY>() );
	}

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ>
	__forceinline const FASTRETURNCHECK(Vec3V_Out) Vec3VConstant()
	{
		return Vec3V( Vec::V4VConstant<floatAsIntX,floatAsIntY,floatAsIntZ,floatAsIntX>() );
	}

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	__forceinline const FASTRETURNCHECK(Vec4V_Out) Vec4VConstant()
	{
		return Vec4V( Vec::V4VConstant<floatAsIntX,floatAsIntY,floatAsIntZ,floatAsIntW>() );
	}

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	__forceinline const FASTRETURNCHECK(QuatV_Out) QuatVConstant()
	{
		return QuatV( Vec::V4VConstant<floatAsIntX,floatAsIntY,floatAsIntZ,floatAsIntW>() );
	}

	template <	u8 byte0, u8 byte1, u8 byte2, u8 byte3,
				u8 byte4, u8 byte5, u8 byte6, u8 byte7,
				u8 byte8, u8 byte9, u8 byte10, u8 byte11,
				u8 byte12, u8 byte13, u8 byte14, u8 byte15	>
	__forceinline const FASTRETURNCHECK(Vec4V_Out) Vec4VConstant()
	{
		return Vec4V( Vec::V4VConstant<	byte0,byte1,byte2,byte3,
										byte4,byte5,byte6,byte7,
										byte8,byte9,byte10,byte11,
										byte12,byte13,byte14,byte15	>() );
	}

	__forceinline void CrossProduct( Mat33V_InOut inoutMat, Vec3V_In r )
	{
		Mat33V ident(V_IDENTITY);
		Vec3V col0 = Cross( ident.GetCol0(), r );
		Vec3V col1 = Cross( ident.GetCol1(), r );
		Vec3V col2 = Cross( ident.GetCol2(), r );
		inoutMat.SetCols(
			col0,
			col1,
			col2
			);

		FastAssert( inoutMat.GetM00f() == 0.0f && inoutMat.GetM10f() == -r.GetZf() && inoutMat.GetM20f() == r.GetYf() );
	}

	__forceinline void CrossProduct( Mat34V_InOut inoutMat, Mat34V_In m, Vec3V_In r )
	{
		Mat33V ident(V_IDENTITY);
		Vec3V col0 = Cross( ident.GetCol0(), r );
		Vec3V col1 = Cross( ident.GetCol1(), r );
		Vec3V col2 = Cross( ident.GetCol2(), r );
		inoutMat.SetCols(
			col0,
			col1,
			col2,
			m.GetCol3()
			);

		FastAssert( inoutMat.GetM00f() == 0.0f && inoutMat.GetM10f() == -r.GetZf() && inoutMat.GetM20f() == r.GetYf() );
	}

	__forceinline void DotCrossProduct( Mat33V_InOut inoutMat, Mat33V_In m, Vec3V_In r )
	{
		Vec3V col0 = Cross( m.GetCol0(), r );
		Vec3V col1 = Cross( m.GetCol1(), r );
		Vec3V col2 = Cross( m.GetCol2(), r );
		inoutMat.SetCols(
			col0,
			col1,
			col2
			);
	}

	__forceinline void Dot3x3CrossProduct( Mat34V_InOut inoutMat, Mat34V_In m, Vec3V_In r )
	{
		Vec3V col0 = Cross( m.GetCol0(), r );
		Vec3V col1 = Cross( m.GetCol1(), r );
		Vec3V col2 = Cross( m.GetCol2(), r );
		inoutMat.SetCols(
			col0,
			col1,
			col2,
			m.GetCol3()
			);
	}

	__forceinline void Dot3x3CrossProductTranspose( Mat34V_InOut inoutMat, Mat34V_In m, Vec3V_In r )
	{
		Vec3V col0 = Cross( r, m.GetCol0() );
		Vec3V col1 = Cross( r, m.GetCol1() );
		Vec3V col2 = Cross( r, m.GetCol2() );
		inoutMat.SetCols(
			col0,
			col1,
			col2,
			m.GetCol3()
			);
	}

	//============================================================================
	// Comparison functions.

	__forceinline bool IsTrue( BoolV_In b )
	{
		return Vec::V4IsEqualIntAll( b.GetIntrin128(), Vec::V4VConstant(V_ZERO) ) == 0;
	}

	__forceinline bool IsFalse( BoolV_In b )
	{
		return Vec::V4IsEqualIntAll( b.GetIntrin128(), Vec::V4VConstant(V_ZERO) ) != 0;
	}

	__forceinline bool IsTrueAll( VecBoolV_In v )
	{
		return Vec::V4IsEqualIntAll( v.GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline bool IsFalseAll( VecBoolV_In v )
	{
		return Vec::V4IsEqualIntAll( v.GetIntrin128(), Vec::V4VConstant(V_ZERO) ) != 0;
	}
	__forceinline bool IsTrueXYZ( VecBoolV_In v )
	{
		return IsTrueAll( v | VecBoolV(V_F_F_F_T));		
	}
	__forceinline bool IsFalseXYZ( VecBoolV_In v )
	{
		return IsFalseAll(  v & VecBoolV(V_T_T_T_F));
	}
	__forceinline BoolV_Out SameSign( ScalarV_In v1, ScalarV_In v2 )
	{
		return BoolV( Vec::V4SameSignV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out SameSign( Vec2V_In v1, Vec2V_In v2 )
	{
		return VecBoolV( Vec::V4SameSignV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out SameSign( Vec3V_In v1, Vec3V_In v2 )
	{
		return VecBoolV( Vec::V4SameSignV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out SameSign( Vec4V_In v1, Vec4V_In v2 )
	{
		return VecBoolV( Vec::V4SameSignV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline unsigned int SameSignAll( ScalarV_In v1, ScalarV_In v2 )
	{
		return Vec::V4SameSignAll( v1.GetIntrin128(), v2.GetIntrin128() );
	}

	__forceinline unsigned int SameSignAll( Vec2V_In v1, Vec2V_In v2 )
	{
		return Vec::V2SameSignAll( v1.GetIntrin128(), v2.GetIntrin128() );
	}

	__forceinline unsigned int SameSignAll( Vec3V_In v1, Vec3V_In v2 )
	{
		return Vec::V3SameSignAll( v1.GetIntrin128(), v2.GetIntrin128() );
	}

	__forceinline unsigned int SameSignAll( Vec4V_In v1, Vec4V_In v2 )
	{
		return Vec::V4SameSignAll( v1.GetIntrin128(), v2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsFinite( Vec4V_In v )
	{
		return VecBoolV( Vec::V4IsFiniteV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsFinite( Vec3V_In v )
	{
		return VecBoolV( Vec::V4IsFiniteV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsFinite( Vec2V_In v )
	{
		return VecBoolV( Vec::V4IsFiniteV( v.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsFinite( ScalarV_In v )
	{
		return BoolV( Vec::V4IsFiniteV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsFinite( QuatV_In v )
	{
		return VecBoolV( Vec::V4IsFiniteV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsNotNan( Vec4V_In v )
	{
		return VecBoolV( Vec::V4IsNotNanV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsNotNan( Vec3V_In v )
	{
		return VecBoolV( Vec::V4IsNotNanV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsNotNan( Vec2V_In v )
	{
		return VecBoolV( Vec::V4IsNotNanV( v.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsNotNan( ScalarV_In v )
	{
		return BoolV( Vec::V4IsNotNanV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsNotNan( QuatV_In v )
	{
		return VecBoolV( Vec::V4IsNotNanV( v.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsEqualInt(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualIntAll(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return Vec::V4IsEqualIntAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return Vec::V4IsEqualIntNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline BoolV_Out IsEqualInt(BoolV_In inVector1, BoolV_In inVector2)
	{
		return BoolV( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualIntAll(BoolV_In inVector1, BoolV_In inVector2)
	{
		return Vec::V4IsEqualIntAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(BoolV_In inVector1, BoolV_In inVector2)
	{
		return Vec::V4IsEqualIntNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEven(Vec4V_In inVector)
	{
		return VecBoolV( Vec::V4IsEvenV( inVector.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsOdd(Vec4V_In inVector)
	{
		return VecBoolV( Vec::V4IsOddV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsZeroAll(Vec4V_In inVector)
	{
		return Vec::V4IsZeroAll( inVector.GetIntrin128() );
	}

	__forceinline unsigned int IsZeroNone(Vec4V_In inVector)
	{
		return Vec::V4IsZeroNone( inVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsZero(Vec4V_In inVector)
	{
		return VecBoolV( Vec::V4IsZeroV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( Vec4V_In inVector1, Vec4V_In boundsVector )
	{
		return Vec::V4IsBetweenNegAndPosBounds( inVector1.GetIntrin128(), boundsVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqual(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualAll(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec::V4IsEqualAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualNone(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec::V4IsEqualNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqualInt(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualIntAll(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec::V4IsEqualIntAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec::V4IsEqualIntNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsClose(Vec4V_In inVector1, Vec4V_In inVector2, ScalarV_In epsValue)
	{
		return VecBoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsClose(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In epsValues)
	{
		return VecBoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() ) );
	}

	__forceinline unsigned int IsCloseAll(Vec4V_In inVector1, Vec4V_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V4IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseAll(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In epsValues)
	{
		return Vec::V4IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(Vec4V_In inVector1, Vec4V_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V4IsCloseNone( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In epsValues)
	{
		return Vec::V4IsCloseNone( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsGreaterThanAll(Vec4V_In bigVector, Vec4V_In smallVector)
	{
		return Vec::V4IsGreaterThanAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThan(Vec4V_In bigVector, Vec4V_In smallVector)
	{
		return VecBoolV( Vec::V4IsGreaterThanV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Vec4V_In bigVector, Vec4V_In smallVector)
	{
		return Vec::V4IsGreaterThanOrEqualAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThanOrEqual(Vec4V_In bigVector, Vec4V_In smallVector)
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanAll(Vec4V_In smallVector, Vec4V_In bigVector)
	{
		return Vec::V4IsLessThanAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThan(Vec4V_In smallVector, Vec4V_In bigVector)
	{
		return VecBoolV( Vec::V4IsLessThanV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Vec4V_In smallVector, Vec4V_In bigVector)
	{
		return Vec::V4IsLessThanOrEqualAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThanOrEqual(Vec4V_In smallVector, Vec4V_In bigVector)
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsEven(Vec3V_In inVector)
	{
		return VecBoolV( Vec::V4IsEvenV( inVector.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsOdd(Vec3V_In inVector)
	{
		return VecBoolV( Vec::V4IsOddV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsZeroAll(Vec3V_In inVector)
	{
		return Vec::V3IsZeroAll( inVector.GetIntrin128() );
	}

	__forceinline unsigned int IsZeroNone(Vec3V_In inVector)
	{
		return Vec::V3IsZeroNone( inVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsZero(Vec3V_In inVector)
	{
		return VecBoolV( Vec::V4IsZeroV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( Vec3V_In inVector, Vec3V_In boundsVector )
	{
		return Vec::V3IsBetweenNegAndPosBounds( inVector.GetIntrin128(), boundsVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqual(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualAll(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec::V3IsEqualAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualNone(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec::V3IsEqualNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqualInt(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualIntAll(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec::V3IsEqualIntAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec::V3IsEqualIntNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsClose(Vec3V_In inVector1, Vec3V_In inVector2, ScalarV_In epsValue)
	{
		return VecBoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsClose(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In epsValues)
	{
		return VecBoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() ) );
	}

	__forceinline unsigned int IsCloseAll(Vec3V_In inVector1, Vec3V_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V3IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseAll(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In epsValues)
	{
		return Vec::V3IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(Vec3V_In inVector1, Vec3V_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V3IsCloseNone( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In epsValues)
	{
		return Vec::V3IsCloseNone( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsGreaterThanAll(Vec3V_In bigVector, Vec3V_In smallVector)
	{
		return Vec::V3IsGreaterThanAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThan(Vec3V_In bigVector, Vec3V_In smallVector)
	{
		return VecBoolV( Vec::V4IsGreaterThanV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Vec3V_In bigVector, Vec3V_In smallVector)
	{
		return Vec::V3IsGreaterThanOrEqualAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThanOrEqual(Vec3V_In bigVector, Vec3V_In smallVector)
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanAll(Vec3V_In smallVector, Vec3V_In bigVector)
	{
		return Vec::V3IsLessThanAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThan(Vec3V_In smallVector, Vec3V_In bigVector)
	{
		return VecBoolV( Vec::V4IsLessThanV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Vec3V_In smallVector, Vec3V_In bigVector)
	{
		return Vec::V3IsLessThanOrEqualAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThanOrEqual(Vec3V_In smallVector, Vec3V_In bigVector)
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsEven(Vec2V_In inVector)
	{
		return VecBoolV( Vec::V4IsEvenV( inVector.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsOdd(Vec2V_In inVector)
	{
		return VecBoolV( Vec::V4IsOddV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsZeroAll(Vec2V_In inVector)
	{
		return Vec::V2IsZeroAll( inVector.GetIntrin128() );
	}

	__forceinline unsigned int IsZeroNone(Vec2V_In inVector)
	{
		return Vec::V2IsZeroNone( inVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsZero(Vec2V_In inVector)
	{
		return VecBoolV( Vec::V4IsZeroV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( Vec2V_In inVector, Vec2V_In boundsVector )
	{
		return Vec::V2IsBetweenNegAndPosBounds( inVector.GetIntrin128(), boundsVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqual(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualAll(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec::V2IsEqualAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualNone(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec::V2IsEqualNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqualInt(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return VecBoolV( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualIntAll(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec::V2IsEqualIntAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec::V2IsEqualIntNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsClose(Vec2V_In inVector1, Vec2V_In inVector2, ScalarV_In epsValue)
	{
		return VecBoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsClose(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In epsValues)
	{
		return VecBoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() ) );
	}

	__forceinline unsigned int IsCloseAll(Vec2V_In inVector1, Vec2V_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V2IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseAll(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In epsValues)
	{
		return Vec::V2IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(Vec2V_In inVector1, Vec2V_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V2IsCloseNone( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In epsValues)
	{
		return Vec::V2IsCloseNone( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsGreaterThanAll(Vec2V_In bigVector, Vec2V_In smallVector)
	{
		return Vec::V2IsGreaterThanAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThan(Vec2V_In bigVector, Vec2V_In smallVector)
	{
		return VecBoolV( Vec::V4IsGreaterThanV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Vec2V_In bigVector, Vec2V_In smallVector)
	{
		return Vec::V2IsGreaterThanOrEqualAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThanOrEqual(Vec2V_In bigVector, Vec2V_In smallVector)
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanAll(Vec2V_In smallVector, Vec2V_In bigVector)
	{
		return Vec::V2IsLessThanAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThan(Vec2V_In smallVector, Vec2V_In bigVector)
	{
		return VecBoolV( Vec::V4IsLessThanV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Vec2V_In smallVector, Vec2V_In bigVector)
	{
		return Vec::V2IsLessThanOrEqualAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThanOrEqual(Vec2V_In smallVector, Vec2V_In bigVector)
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsEven(ScalarV_In inVector)
	{
		return BoolV( Vec::V4IsEvenV( inVector.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsOdd(ScalarV_In inVector)
	{
		return BoolV( Vec::V4IsOddV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsZeroAll(ScalarV_In inVector)
	{
		return Vec::V4IsZeroAll( inVector.GetIntrin128() );
	}

	__forceinline BoolV_Out IsZero(ScalarV_In inVector)
	{
		return BoolV( Vec::V4IsZeroV( inVector.GetIntrin128() ) );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( ScalarV_In inVector, ScalarV_In boundsVector )
	{
		return Vec::V4IsBetweenNegAndPosBounds( inVector.GetIntrin128(), boundsVector.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualAll(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return Vec::V4IsEqualAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualNone(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return Vec::V4IsEqualNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntAll(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return Vec::V4IsEqualIntAll( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return Vec::V4IsEqualIntNone( inVector1.GetIntrin128(), inVector2.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseAll(ScalarV_In inVector1, ScalarV_In inVector2, ScalarV_In epsValue)
	{
		return Vec::V4IsCloseAll( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsGreaterThanAll(ScalarV_In bigVector, ScalarV_In smallVector)
	{
		return Vec::V4IsGreaterThanAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(ScalarV_In bigVector, ScalarV_In smallVector)
	{
		return Vec::V4IsGreaterThanOrEqualAll( bigVector.GetIntrin128(), smallVector.GetIntrin128() );
	}

	__forceinline unsigned int IsLessThanAll(ScalarV_In smallVector, ScalarV_In bigVector)
	{
		return Vec::V4IsLessThanAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(ScalarV_In smallVector, ScalarV_In bigVector)
	{
		return Vec::V4IsLessThanOrEqualAll( smallVector.GetIntrin128(), bigVector.GetIntrin128() );
	}

	__forceinline BoolV_Out IsEqual(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return BoolV( Vec::V4IsEqualV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsEqualInt(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return BoolV( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsClose(ScalarV_In inVector1, ScalarV_In inVector2, ScalarV_In epsValue)
	{
		return BoolV( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValue.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsGreaterThan(ScalarV_In bigVector, ScalarV_In smallVector)
	{
		return BoolV( Vec::V4IsGreaterThanV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsGreaterThanOrEqual(ScalarV_In bigVector, ScalarV_In smallVector)
	{
		return BoolV( Vec::V4IsGreaterThanOrEqualV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsLessThan(ScalarV_In smallVector, ScalarV_In bigVector)
	{
		return BoolV( Vec::V4IsLessThanV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline BoolV_Out IsLessThanOrEqual(ScalarV_In smallVector, ScalarV_In bigVector)
	{
		return BoolV( Vec::V4IsLessThanOrEqualV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}










	__forceinline void ResultToIndexZYX( u32& outInt, VecBoolV_In resultVec )
	{
		Vec::V3ResultToIndexZYX( outInt, resultVec.GetIntrin128() );
	}

	__forceinline void ResultToIndexXYZ( u32& outInt, VecBoolV_In resultVec )
	{
		Vec::V3ResultToIndexXYZ( outInt, resultVec.GetIntrin128() );
	}

	__forceinline unsigned int IsZeroAll(QuatV_In inQuat)
	{
		return Vec::V4IsZeroAll( inQuat.GetIntrin128() );
	}

	__forceinline unsigned int IsZeroNone(QuatV_In inQuat)
	{
		return Vec::V4IsZeroNone( inQuat.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsZero(QuatV_In inQuat)
	{
		return VecBoolV( Vec::V4IsZeroV( inQuat.GetIntrin128() ) );
	}

 	__forceinline BoolV_Out IsU32NonZero(u32 b)
 	{
		return BoolV(Vec::V4IsNonZeroV(b));
 	}

	__forceinline VecBoolV_Out IsEqual(QuatV_In inQuat1, QuatV_In inQuat2)
	{
		return VecBoolV( Vec::V4IsEqualV( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualAll(QuatV_In inQuat1, QuatV_In inQuat2)
	{
		return Vec::V4IsEqualAll( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualNone(QuatV_In inQuat1, QuatV_In inQuat2)
	{
		return Vec::V4IsEqualNone( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsEqualInt(QuatV_In inQuat1, QuatV_In inQuat2)
	{
		return VecBoolV( Vec::V4IsEqualIntV( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualIntAll(QuatV_In inQuat1, QuatV_In inQuat2)
	{
		return Vec::V4IsEqualIntAll( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() );
	}

	__forceinline unsigned int IsEqualIntNone(QuatV_In inQuat1, QuatV_In inQuat2)
	{
		return Vec::V4IsEqualIntNone( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsClose(QuatV_In inQuat1, QuatV_In inQuat2, ScalarV_In epsValue)
	{
		return VecBoolV( Vec::V4IsCloseV( inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), epsValue.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out IsClose(QuatV_In inQuat1, QuatV_In inQuat2, Vec4V_In epsValues)
	{
		return VecBoolV( Vec::V4IsCloseV( inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), epsValues.GetIntrin128() ) );
	}

	__forceinline unsigned int IsCloseAll(QuatV_In inQuat1, QuatV_In inQuat2, ScalarV_In epsValue)
	{
		return Vec::V4IsCloseAll( inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseAll(QuatV_In inQuat1, QuatV_In inQuat2, Vec4V_In epsValues)
	{
		return Vec::V4IsCloseAll( inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(QuatV_In inQuat1, QuatV_In inQuat2, ScalarV_In epsValue)
	{
		return Vec::V4IsCloseNone( inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), epsValue.GetIntrin128() );
	}

	__forceinline unsigned int IsCloseNone(QuatV_In inQuat1, QuatV_In inQuat2, Vec4V_In epsValues)
	{
		return Vec::V4IsCloseNone( inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), epsValues.GetIntrin128() );
	}

	__forceinline unsigned int IsGreaterThanAll(QuatV_In bigQuat, QuatV_In smallQuat)
	{
		return Vec::V4IsGreaterThanAll( bigQuat.GetIntrin128(), smallQuat.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThan(QuatV_In bigQuat, QuatV_In smallQuat)
	{
		return VecBoolV( Vec::V4IsGreaterThanV( bigQuat.GetIntrin128(), smallQuat.GetIntrin128() ) );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(QuatV_In bigQuat, QuatV_In smallQuat)
	{
		return Vec::V4IsGreaterThanOrEqualAll( bigQuat.GetIntrin128(), smallQuat.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsGreaterThanOrEqual(QuatV_In bigQuat, QuatV_In smallQuat)
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( bigQuat.GetIntrin128(), smallQuat.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanAll(QuatV_In smallQuat, QuatV_In bigQuat)
	{
		return Vec::V4IsLessThanAll( smallQuat.GetIntrin128(), bigQuat.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThan(QuatV_In smallQuat, QuatV_In bigQuat)
	{
		return VecBoolV( Vec::V4IsLessThanV( smallQuat.GetIntrin128(), bigQuat.GetIntrin128() ) );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(QuatV_In smallQuat, QuatV_In bigQuat)
	{
		return Vec::V4IsLessThanOrEqualAll( smallQuat.GetIntrin128(), bigQuat.GetIntrin128() );
	}

	__forceinline VecBoolV_Out IsLessThanOrEqual(QuatV_In smallQuat, QuatV_In bigQuat)
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( smallQuat.GetIntrin128(), bigQuat.GetIntrin128() ) );
	}

	__forceinline unsigned int IsEqualAll(Mat44V_In inMat1, Mat44V_In inMat2)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsEqualV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsEqualV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsEqualNone(Mat44V_In inMat1, Mat44V_In inMat2)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsEqualV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4Or( Vec::V4IsEqualV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsEqualIntAll(Mat44V_In inMat1, Mat44V_In inMat2)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsEqualIntV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsEqualIntV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsEqualIntNone(Mat44V_In inMat1, Mat44V_In inMat2)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsEqualIntV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4Or( Vec::V4IsEqualIntV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsGreaterThanAll(Mat44V_In bigMat, Mat44V_In smallMat)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsGreaterThanV( bigMat.GetCol0Intrin128(), smallMat.GetCol0Intrin128() ), Vec::V4IsGreaterThanV( bigMat.GetCol1Intrin128(), smallMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsGreaterThanV( bigMat.GetCol2Intrin128(), smallMat.GetCol2Intrin128() ), Vec::V4IsGreaterThanV( bigMat.GetCol3Intrin128(), smallMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Mat44V_In bigMat, Mat44V_In smallMat)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol0Intrin128(), smallMat.GetCol0Intrin128() ), Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol1Intrin128(), smallMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol2Intrin128(), smallMat.GetCol2Intrin128() ), Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol3Intrin128(), smallMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsLessThanAll(Mat44V_In smallMat, Mat44V_In bigMat)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsLessThanV( smallMat.GetCol0Intrin128(), bigMat.GetCol0Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol1Intrin128(), bigMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsLessThanV( smallMat.GetCol2Intrin128(), bigMat.GetCol2Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol3Intrin128(), bigMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Mat44V_In smallMat, Mat44V_In bigMat)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsLessThanOrEqualV( smallMat.GetCol0Intrin128(), bigMat.GetCol0Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol1Intrin128(), bigMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsLessThanOrEqualV( smallMat.GetCol2Intrin128(), bigMat.GetCol2Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol3Intrin128(), bigMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseAll(Mat44V_In inMat1, Mat44V_In inMat2, ScalarV_In epsValue)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValue.GetIntrin128() ) ),
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValue.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseAll(Mat44V_In inMat1, Mat44V_In inMat2, Vec4V_In epsValues)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValues.GetIntrin128() ) ),
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValues.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseNone(Mat44V_In inMat1, Mat44V_In inMat2, ScalarV_In epsValue)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValue.GetIntrin128() ) ),
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValue.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsCloseNone(Mat44V_In inMat1, Mat44V_In inMat2, Vec4V_In epsValues)
	{
		return Vec::V4IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValues.GetIntrin128() ) ),
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValues.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsEqualAll(Mat34V_In inMat1, Mat34V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsEqualV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsEqualV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsEqualNone(Mat34V_In inMat1, Mat34V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsEqualV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4Or( Vec::V4IsEqualV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsEqualIntAll(Mat34V_In inMat1, Mat34V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsEqualIntV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsEqualIntV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsEqualIntNone(Mat34V_In inMat1, Mat34V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsEqualIntV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4Or( Vec::V4IsEqualIntV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsGreaterThanAll(Mat34V_In bigMat, Mat34V_In smallMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsGreaterThanV( bigMat.GetCol0Intrin128(), smallMat.GetCol0Intrin128() ), Vec::V4IsGreaterThanV( bigMat.GetCol1Intrin128(), smallMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsGreaterThanV( bigMat.GetCol2Intrin128(), smallMat.GetCol2Intrin128() ), Vec::V4IsGreaterThanV( bigMat.GetCol3Intrin128(), smallMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Mat34V_In bigMat, Mat34V_In smallMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol0Intrin128(), smallMat.GetCol0Intrin128() ), Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol1Intrin128(), smallMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol2Intrin128(), smallMat.GetCol2Intrin128() ), Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol3Intrin128(), smallMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsLessThanAll(Mat34V_In smallMat, Mat34V_In bigMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsLessThanV( smallMat.GetCol0Intrin128(), bigMat.GetCol0Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol1Intrin128(), bigMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsLessThanV( smallMat.GetCol2Intrin128(), bigMat.GetCol2Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol3Intrin128(), bigMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Mat34V_In smallMat, Mat34V_In bigMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsLessThanOrEqualV( smallMat.GetCol0Intrin128(), bigMat.GetCol0Intrin128() ), Vec::V4IsLessThanOrEqualV( smallMat.GetCol1Intrin128(), bigMat.GetCol1Intrin128() ) ),
				Vec::V4And( Vec::V4IsLessThanOrEqualV( smallMat.GetCol2Intrin128(), bigMat.GetCol2Intrin128() ), Vec::V4IsLessThanOrEqualV( smallMat.GetCol3Intrin128(), bigMat.GetCol3Intrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseAll(Mat34V_In inMat1, Mat34V_In inMat2, ScalarV_In epsValue)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValue.GetIntrin128() ) ),
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValue.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseAll(Mat34V_In inMat1, Mat34V_In inMat2, Vec3V_In epsValues)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValues.GetIntrin128() ) ),
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValues.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseNone(Mat34V_In inMat1, Mat34V_In inMat2, ScalarV_In epsValue)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValue.GetIntrin128() ) ),
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValue.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsCloseNone(Mat34V_In inMat1, Mat34V_In inMat2, Vec3V_In epsValues)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValues.GetIntrin128() ) ),
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), epsValues.GetIntrin128() ) )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}













	__forceinline unsigned int IsEqualAll(Mat33V_In inMat1, Mat33V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsEqualV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4IsEqualV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsEqualNone(Mat33V_In inMat1, Mat33V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsEqualV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4IsEqualV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsEqualIntAll(Mat33V_In inMat1, Mat33V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsEqualIntV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4IsEqualIntV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsEqualIntNone(Mat33V_In inMat1, Mat33V_In inMat2)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsEqualIntV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128() ), Vec::V4IsEqualIntV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128() ) ),
				Vec::V4IsEqualIntV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsGreaterThanAll(Mat33V_In bigMat, Mat33V_In smallMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsGreaterThanV( bigMat.GetCol0Intrin128(), smallMat.GetCol0Intrin128() ), Vec::V4IsGreaterThanV( bigMat.GetCol1Intrin128(), smallMat.GetCol1Intrin128() ) ),
				Vec::V4IsGreaterThanV( bigMat.GetCol2Intrin128(), smallMat.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Mat33V_In bigMat, Mat33V_In smallMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol0Intrin128(), smallMat.GetCol0Intrin128() ), Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol1Intrin128(), smallMat.GetCol1Intrin128() ) ),
				Vec::V4IsGreaterThanOrEqualV( bigMat.GetCol2Intrin128(), smallMat.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsLessThanAll(Mat33V_In smallMat, Mat33V_In bigMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsLessThanV( smallMat.GetCol0Intrin128(), bigMat.GetCol0Intrin128() ), Vec::V4IsLessThanV( smallMat.GetCol1Intrin128(), bigMat.GetCol1Intrin128() ) ),
				Vec::V4IsLessThanV( smallMat.GetCol2Intrin128(), bigMat.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Mat33V_In smallMat, Mat33V_In bigMat)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsLessThanOrEqualV( smallMat.GetCol0Intrin128(), bigMat.GetCol0Intrin128() ), Vec::V4IsLessThanOrEqualV( smallMat.GetCol1Intrin128(), bigMat.GetCol1Intrin128() ) ),
				Vec::V4IsLessThanOrEqualV( smallMat.GetCol2Intrin128(), bigMat.GetCol2Intrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseAll(Mat33V_In inMat1, Mat33V_In inMat2, ScalarV_In epsValue)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValue.GetIntrin128() ) ),
				Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValue.GetIntrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseAll(Mat33V_In inMat1, Mat33V_In inMat2, Vec3V_In epsValues)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4And(
				Vec::V4And( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValues.GetIntrin128() ) ),
				Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValues.GetIntrin128() )
			)
			,
			Vec::V4VConstant(V_MASKXYZW)
			);
	}

	__forceinline unsigned int IsCloseNone(Mat33V_In inMat1, Mat33V_In inMat2, ScalarV_In epsValue)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValue.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValue.GetIntrin128() ) ),
				Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValue.GetIntrin128() )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	__forceinline unsigned int IsCloseNone(Mat33V_In inMat1, Mat33V_In inMat2, Vec3V_In epsValues)
	{
		return Vec::V3IsEqualIntAll(
			Vec::V4Or(
				Vec::V4Or( Vec::V4IsCloseV( inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), epsValues.GetIntrin128() ), Vec::V4IsCloseV( inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), epsValues.GetIntrin128() ) ),
				Vec::V4IsCloseV( inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), epsValues.GetIntrin128() )
			)
			,
			Vec::V4VConstant(V_ZERO)
			);
	}

	//============================================================================
	// Conversion functions

	template <int exponent>
	__forceinline FASTRETURNCHECK(Vec4V_Out) FloatToIntRaw(Vec4V_In inVec)
	{
		return Vec4V( Vec::V4FloatToIntRaw<exponent>( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(Vec4V_Out) IntToFloatRaw(Vec4V_In inVec)
	{
		return Vec4V( Vec::V4IntToFloatRaw<exponent>( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RoundToNearestInt(Vec4V_In inVec)
	{
		return Vec4V( Vec::V4RoundToNearestInt( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RoundToNearestIntZero(Vec4V_In inVec)
	{
		return Vec4V( Vec::V4RoundToNearestIntZero( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RoundToNearestIntNegInf(Vec4V_In inVec)
	{
		return Vec4V( Vec::V4RoundToNearestIntNegInf( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RoundToNearestIntPosInf(Vec4V_In inVec)
	{
		return Vec4V( Vec::V4RoundToNearestIntPosInf( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(Vec3V_Out) FloatToIntRaw(Vec3V_In inVec)
	{
		return Vec3V( Vec::V4FloatToIntRaw<exponent>( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(Vec3V_Out) IntToFloatRaw(Vec3V_In inVec)
	{
		return Vec3V( Vec::V4IntToFloatRaw<exponent>( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RoundToNearestInt(Vec3V_In inVec)
	{
		return Vec3V( Vec::V4RoundToNearestInt( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RoundToNearestIntZero(Vec3V_In inVec)
	{
		return Vec3V( Vec::V4RoundToNearestIntZero( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RoundToNearestIntNegInf(Vec3V_In inVec)
	{
		return Vec3V( Vec::V4RoundToNearestIntNegInf( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RoundToNearestIntPosInf(Vec3V_In inVec)
	{
		return Vec3V( Vec::V4RoundToNearestIntPosInf( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(Vec2V_Out) FloatToIntRaw(Vec2V_In inVec)
	{
		return Vec2V( Vec::V4FloatToIntRaw<exponent>( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(Vec2V_Out) IntToFloatRaw(Vec2V_In inVec)
	{
		return Vec2V( Vec::V4IntToFloatRaw<exponent>( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RoundToNearestInt(Vec2V_In inVec)
	{
		return Vec2V( Vec::V4RoundToNearestInt( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RoundToNearestIntZero(Vec2V_In inVec)
	{
		return Vec2V( Vec::V4RoundToNearestIntZero( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RoundToNearestIntNegInf(Vec2V_In inVec)
	{
		return Vec2V( Vec::V4RoundToNearestIntNegInf( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RoundToNearestIntPosInf(Vec2V_In inVec)
	{
		return Vec2V( Vec::V4RoundToNearestIntPosInf( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(ScalarV_Out) FloatToIntRaw(ScalarV_In inVec)
	{
		return ScalarV( Vec::V4FloatToIntRaw<exponent>( inVec.GetIntrin128() ) );
	}

	template <int exponent>
	__forceinline FASTRETURNCHECK(ScalarV_Out) IntToFloatRaw(ScalarV_In inVec)
	{
		return ScalarV( Vec::V4IntToFloatRaw<exponent>( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RoundToNearestInt(ScalarV_In inVec)
	{
		return ScalarV( Vec::V4RoundToNearestInt( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RoundToNearestIntZero(ScalarV_In inVec)
	{
		return ScalarV( Vec::V4RoundToNearestIntZero( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RoundToNearestIntNegInf(ScalarV_In inVec)
	{
		return ScalarV( Vec::V4RoundToNearestIntNegInf( inVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RoundToNearestIntPosInf(ScalarV_In inVec)
	{
		return ScalarV( Vec::V4RoundToNearestIntPosInf( inVec.GetIntrin128() ) );
	}

	//============================================================================
	// Standard quaternion math

	__forceinline FASTRETURNCHECK(QuatV_Out) Conjugate(QuatV_In inQuat)
	{
		return QuatV( Vec::V4QuatConjugate( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Normalize(QuatV_In inQuat)
	{
		return QuatV( Vec::V4QuatNormalize( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) NormalizeSafe(QuatV_In inQuat, QuatV_In errValVect, QuatV_In magSqThreshold )
	{
		return QuatV( Vec::V4QuatNormalizeSafe( inQuat.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) NormalizeFast(QuatV_In inQuat)
	{
		return QuatV( Vec::V4QuatNormalizeFast( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) NormalizeFastSafe(QuatV_In inQuat, QuatV_In errValVect, QuatV_In magSqThreshold)
	{
		return QuatV( Vec::V4QuatNormalizeFastSafe( inQuat.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Invert(QuatV_In inQuat)
	{
		return QuatV( Vec::V4QuatInvert( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) InvertSafe(QuatV_In inQuat, QuatV_In errValVect)
	{
		return QuatV( Vec::V4QuatInvertSafe( inQuat.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) InvertFast(QuatV_In inQuat)
	{
		return QuatV( Vec::V4QuatInvertFast( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) InvertFastSafe(QuatV_In inQuat, QuatV_In errValVect)
	{
		return QuatV( Vec::V4QuatInvertFastSafe( inQuat.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) InvertNormInput(QuatV_In inQuat)
	{
		return QuatV( Vec::V4QuatInvertNormInput( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) GetUnitDirection( QuatV_In inQuat )
	{
		return Vec3V( Vec::V4QuatGetUnitDirection( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) GetUnitDirectionFast( QuatV_In inQuat )
	{
		return Vec3V( Vec::V4QuatGetUnitDirectionFast( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) GetUnitDirectionSafe( QuatV_In inQuat, Vec3V_In errValVec )
	{
		return Vec3V( Vec::V4QuatGetUnitDirectionSafe( inQuat.GetIntrin128(), errValVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) GetUnitDirectionFastSafe( QuatV_In inQuat, Vec3V_In errValVec )
	{
		return Vec3V( Vec::V4QuatGetUnitDirectionFastSafe( inQuat.GetIntrin128(), errValVec.GetIntrin128() ) );
	}

	__forceinline void QuatVToAxisAngle( Vec3V_InOut Axis, ScalarV_InOut radians, QuatV_In inQuat )
	{
		Vec::V4QuatToAxisAngle( Axis.GetIntrin128Ref(), radians.GetIntrin128Ref(), inQuat.GetIntrin128() );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) GetAngle( QuatV_In inQuat )
	{
		return ScalarV( Vec::V4QuatGetAngle( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RelAngle( QuatV_In inQuat1, QuatV_In inQuat2  )
	{
		return ScalarV( Vec::V4QuatRelAngle( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromVectors( Vec3V_In inVec1, Vec3V_In inVec2, Vec3V_In inVec3 )
	{
		return QuatV( Vec::V4QuatFromVectors( inVec1.GetIntrin128(), inVec2.GetIntrin128(), inVec3.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) QuatVFromVectors( Vec3V_In inVec1, Vec3V_In inVec2 )
	{
		return QuatV( Vec::V4QuatFromVectors( inVec1.GetIntrin128(), inVec2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) PrepareSlerp( QuatV_In quat1, QuatV_In quatToNegate )
	{
		return QuatV( Vec::V4QuatPrepareSlerp( quat1.GetIntrin128(), quatToNegate.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Clamp( ScalarV_In inVect, ScalarV_In lowBound, ScalarV_In highBound )
	{
		return ScalarV( Vec::V4Clamp( inVect.GetIntrin128(), lowBound.GetIntrin128(), highBound.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Saturate( ScalarV_In inVect )
	{
		return ScalarV( Vec::V4Saturate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Negate(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4Negate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvertBits(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Invert(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4Invert( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvertSafe(ScalarV_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvertSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvertFast(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4InvertFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvertFastSafe(ScalarV_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvertFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Abs(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4Abs( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Sqrt(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4Sqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SqrtSafe(ScalarV_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4SqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SqrtFast(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4SqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SqrtFastSafe(ScalarV_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4SqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Clamp( Vec2V_In inVect, Vec2V_In lowBound, Vec2V_In highBound )
	{
		return Vec2V( Vec::V4Clamp( inVect.GetIntrin128(), lowBound.GetIntrin128(), highBound.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Saturate( Vec2V_In inVect )
	{
		return Vec2V( Vec::V4Saturate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Negate(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4Negate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvertBits(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Invert(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4Invert( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvertSafe(Vec2V_In inVect, Vec2V_In errValVect)
	{
		return Vec2V( Vec::V4InvertSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvertFast(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4InvertFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvertFastSafe(Vec2V_In inVect, Vec2V_In errValVect)
	{
		return Vec2V( Vec::V4InvertFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Normalize(Vec2V_In inVect)
	{
		return Vec2V( Vec::V2Normalize( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) NormalizeSafe(Vec2V_In inVect, Vec2V_In errValVect, Vec2V_In magSqThreshold)
	{
		return Vec2V( Vec::V2NormalizeSafe( inVect.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) NormalizeFast(Vec2V_In inVect)
	{
		return Vec2V( Vec::V2NormalizeFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) NormalizeFastSafe(Vec2V_In inVect, Vec2V_In errValVect, Vec2V_In magSqThreshold)
	{
		return Vec2V( Vec::V2NormalizeFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Extend( Vec2V_In inVect, Vec2V_In amount )
	{
		return Vec2V( Vec::V2Extend( inVect.GetIntrin128(), amount.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Rotate( Vec2V_In inVect, ScalarV_In radians )
	{
		return Vec2V( Vec::V2Rotate( inVect.GetIntrin128(), radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Reflect( Vec2V_In inVect, Vec2V_In wall2DNormal )
	{
		return Vec2V( Vec::V2Reflect( inVect.GetIntrin128(), wall2DNormal.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) ApproachStraight( Vec2V_In inVect, Vec2V_In goal, Vec2V_In rate, Vec2V_In time, unsigned int& rResult )
	{
		return Vec2V( Vec::V2ApproachStraight( inVect.GetIntrin128(), goal.GetIntrin128(), rate.GetIntrin128(), time.GetIntrin128(), rResult ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Abs(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4Abs( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Sqrt(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4Sqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SqrtSafe(Vec2V_In inVect, Vec2V_In errValVect)
	{
		return Vec2V( Vec::V4SqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SqrtFast(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4SqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SqrtFastSafe(Vec2V_In inVect, Vec2V_In errValVect)
	{
		return Vec2V( Vec::V4SqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Mag(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2MagV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagFast(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2MagVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagSquared(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2MagSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMag(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2InvMagV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSafe(Vec2V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V2InvMagVSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagFast(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2InvMagVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagFastSafe(Vec2V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V2InvMagVFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquared(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2InvMagSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredSafe(Vec2V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V2InvMagSquaredVSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFast(Vec2V_In inVect)
	{
		return ScalarV( Vec::V2InvMagSquaredVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFastSafe(Vec2V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V2InvMagSquaredVFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Clamp( Vec3V_In inVect, Vec3V_In lowBound, Vec3V_In highBound )
	{
		return Vec3V( Vec::V4Clamp( inVect.GetIntrin128(), lowBound.GetIntrin128(), highBound.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Saturate( Vec3V_In inVect )
	{
		return Vec3V( Vec::V4Saturate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Negate(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4Negate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvertBits(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Invert(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4Invert( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvertSafe(Vec3V_In inVect, Vec3V_In errValVect)
	{
		return Vec3V( Vec::V4InvertSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvertFast(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4InvertFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvertFastSafe(Vec3V_In inVect, Vec3V_In errValVect)
	{
		return Vec3V( Vec::V4InvertFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Normalize(Vec3V_In inVect)
	{
		return Vec3V( Vec::V3Normalize( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) NormalizeSafe(Vec3V_In inVect, Vec3V_In errValVect, Vec3V_In magSqThreshold)
	{
		return Vec3V( Vec::V3NormalizeSafe( inVect.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) NormalizeFast(Vec3V_In inVect)
	{
		return Vec3V( Vec::V3NormalizeFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) NormalizeFastSafe(Vec3V_In inVect, Vec3V_In errValVect, Vec3V_In magSqThreshold)
	{
		return Vec3V( Vec::V3NormalizeFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Extend( Vec3V_In inVect, Vec3V_In amount )
	{
		return Vec3V( Vec::V3Extend( inVect.GetIntrin128(), amount.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RotateAboutXAxis( Vec3V_In inVect, ScalarV_In radians )
	{
		return Vec3V( Vec::V3RotateAboutXAxis( inVect.GetIntrin128(), radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RotateAboutYAxis( Vec3V_In inVect, ScalarV_In radians )
	{
		return Vec3V( Vec::V3RotateAboutYAxis( inVect.GetIntrin128(), radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RotateAboutZAxis( Vec3V_In inVect, ScalarV_In radians )
	{
		return Vec3V( Vec::V3RotateAboutZAxis( inVect.GetIntrin128(), radians.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Reflect( Vec3V_In inVect, Vec3V_In planeNormal )
	{
		return Vec3V( Vec::V3Reflect( inVect.GetIntrin128(), planeNormal.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Abs(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4Abs( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Sqrt(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4Sqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SqrtSafe(Vec3V_In inVect, Vec3V_In errValVect)
	{
		return Vec3V( Vec::V4SqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SqrtFast(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4SqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SqrtFastSafe(Vec3V_In inVect, Vec3V_In errValVect)
	{
		return Vec3V( Vec::V4SqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagXY( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagXYV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagXZ( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagXZV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagYZ( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagYZV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagXYSquared( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagXYSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagXZSquared( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagXZSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagYZSquared( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagYZSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagXYFast( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagXYVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagXZFast( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagXZVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagYZFast( Vec3V_In inVect )
	{
		return ScalarV( Vec::V3MagYZVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistXY( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DistXYV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistXZ( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DistXZV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistYZ( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DistYZV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistXYFast( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DistXYVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistXZFast( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DistXZVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistYZFast( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DistYZVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Mag(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3MagV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagFast(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3MagVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagSquared(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3MagSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMag(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3InvMagV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSafe(Vec3V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V3InvMagVSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagFast(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3InvMagVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagFastSafe(Vec3V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V3InvMagVFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquared(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3InvMagSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredSafe(Vec3V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V3InvMagSquaredVSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFast(Vec3V_In inVect)
	{
		return ScalarV( Vec::V3InvMagSquaredVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFastSafe(Vec3V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V3InvMagSquaredVFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Clamp( Vec4V_In inVect, Vec4V_In lowBound, Vec4V_In highBound )
	{
		return Vec4V( Vec::V4Clamp( inVect.GetIntrin128(), lowBound.GetIntrin128(), highBound.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Saturate( Vec4V_In inVect )
	{
		return Vec4V( Vec::V4Saturate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) ClampMag( Vec3V_In inVect, ScalarV_In minMag, ScalarV_In maxMag )
	{
		return Vec3V( Vec::V3ClampMag( inVect.GetIntrin128(), minMag.GetIntrin128(), maxMag.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Negate(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4Negate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Negate(QuatV_In inVect)
	{
		return QuatV( Vec::V4Negate( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvertBits(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out InvertBits(VecBoolV_In inVect)
	{
		return VecBoolV( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline BoolV_Out InvertBits(BoolV_In inVect)
	{
		return BoolV( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Invert(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4Invert( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvertSafe(Vec4V_In inVect, Vec4V_In errValVect)
	{
		return Vec4V( Vec::V4InvertSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvertFast(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4InvertFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvertFastSafe(Vec4V_In inVect, Vec4V_In errValVect)
	{
		return Vec4V( Vec::V4InvertFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Normalize(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4Normalize( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) NormalizeSafe(Vec4V_In inVect, Vec4V_In errValVect, Vec4V_In magSqThreshold)
	{
		return Vec4V( Vec::V4NormalizeSafe( inVect.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) NormalizeFast(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4NormalizeFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) NormalizeFastSafe(Vec4V_In inVect, Vec4V_In errValVect, Vec4V_In magSqThreshold)
	{
		return Vec4V( Vec::V4NormalizeFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128(), magSqThreshold.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Abs(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4Abs( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Sqrt(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4Sqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SqrtSafe(Vec4V_In inVect, Vec4V_In errValVect)
	{
		return Vec4V( Vec::V4SqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SqrtFast(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4SqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SqrtFastSafe(Vec4V_In inVect, Vec4V_In errValVect)
	{
		return Vec4V( Vec::V4SqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvSqrt(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4InvSqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvSqrt(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4InvSqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvSqrt(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4InvSqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvSqrt(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4InvSqrt( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvSqrtSafe(ScalarV_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvSqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvSqrtSafe(Vec2V_In inVect, Vec2V_In errValVect)
	{
		return Vec2V( Vec::V4InvSqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvSqrtSafe(Vec3V_In inVect, Vec3V_In errValVect)
	{
		return Vec3V( Vec::V4InvSqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvSqrtSafe(Vec4V_In inVect, Vec4V_In errValVect)
	{
		return Vec4V( Vec::V4InvSqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvSqrtFast(ScalarV_In inVect)
	{
		return ScalarV( Vec::V4InvSqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvSqrtFast(Vec2V_In inVect)
	{
		return Vec2V( Vec::V4InvSqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvSqrtFast(Vec3V_In inVect)
	{
		return Vec3V( Vec::V4InvSqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvSqrtFast(Vec4V_In inVect)
	{
		return Vec4V( Vec::V4InvSqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvSqrtFastSafe(ScalarV_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvSqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvSqrtFastSafe(Vec2V_In inVect, Vec2V_In errValVect)
	{
		return Vec2V( Vec::V4InvSqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvSqrtFastSafe(Vec3V_In inVect, Vec3V_In errValVect)
	{
		return Vec3V( Vec::V4InvSqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvSqrtFastSafe(Vec4V_In inVect, Vec4V_In errValVect)
	{
		return Vec4V( Vec::V4InvSqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Mag(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4MagV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagFast(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4MagVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagSquared(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4MagSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMag(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4InvMagV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSafe(Vec4V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvMagVSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagFast(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4InvMagVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagFastSafe(Vec4V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvMagVFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquared(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4InvMagSquaredV( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredSafe(Vec4V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvMagSquaredVSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFast(Vec4V_In inVect)
	{
		return ScalarV( Vec::V4InvMagSquaredVFast( inVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFastSafe(Vec4V_In inVect, ScalarV_In errValVect)
	{
		return ScalarV( Vec::V4InvMagSquaredVFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Mag(QuatV_In inQuat)
	{
		return ScalarV( Vec::V4MagV( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagFast(QuatV_In inQuat)
	{
		return ScalarV( Vec::V4MagVFast( inQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MagSquared(QuatV_In inQuat)
	{
		return ScalarV( Vec::V4MagSquaredV( inQuat.GetIntrin128() ) );
	}

















































	__forceinline FASTRETURNCHECK(Vec4V_Out) CanonicalizeAngle( Vec4V_In angle )
	{
		return Vec4V( Vec::V4CanonicalizeAngle( angle.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) CanonicalizeAngle( Vec3V_In angle )
	{
		return Vec3V( Vec::V4CanonicalizeAngle( angle.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) CanonicalizeAngle( Vec2V_In angle )
	{
		return Vec2V( Vec::V4CanonicalizeAngle( angle.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) CanonicalizeAngle( ScalarV_In angle )
	{
		return ScalarV( Vec::V4CanonicalizeAngle( angle.GetIntrin128() ) );
	}

	__forceinline void SinAndCos( Vec4V_InOut inOutSin, Vec4V_InOut inOutCos, Vec4V_In x )
	{
		Vec::V4SinAndCos( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline void SinAndCos( Vec3V_InOut inOutSin, Vec3V_InOut inOutCos, Vec3V_In x )
	{
		Vec::V4SinAndCos( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline void SinAndCos( Vec2V_InOut inOutSin, Vec2V_InOut inOutCos, Vec2V_In x )
	{
		Vec::V4SinAndCos( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline void SinAndCos( ScalarV_InOut inOutSin, ScalarV_InOut inOutCos, ScalarV_In x )
	{
		Vec::V4SinAndCos( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Sin( Vec4V_In a )
	{
		return Vec4V( Vec::V4Sin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Sin( Vec3V_In a )
	{
		return Vec3V( Vec::V4Sin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Sin( Vec2V_In a )
	{
		return Vec2V( Vec::V4Sin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Sin( ScalarV_In a )
	{
		return ScalarV( Vec::V4Sin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Cos( Vec4V_In a )
	{
		return Vec4V( Vec::V4Cos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Cos( Vec3V_In a )
	{
		return Vec3V( Vec::V4Cos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Cos( Vec2V_In a )
	{
		return Vec2V( Vec::V4Cos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Cos( ScalarV_In a )
	{
		return ScalarV( Vec::V4Cos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Tan( Vec4V_In a )
	{
		return Vec4V( Vec::V4Tan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Tan( Vec3V_In a )
	{
		return Vec3V( Vec::V4Tan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Tan( Vec2V_In a )
	{
		return Vec2V( Vec::V4Tan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Tan( ScalarV_In a )
	{
		return ScalarV( Vec::V4Tan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Arcsin( Vec4V_In a )
	{
		return Vec4V( Vec::V4Arcsin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Arcsin( Vec3V_In a )
	{
		return Vec3V( Vec::V4Arcsin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Arcsin( Vec2V_In a )
	{
		return Vec2V( Vec::V4Arcsin( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Arcsin( ScalarV_In a )
	{
		return ScalarV( Vec::V4Arcsin( a.GetIntrin128() ) );
	}

	// NOTE - I disabled the 'isNotNan' checks for Arccos since they were causing problems on SPU ..
	// not sure what the issue is. More generally, why do we need range checks and and NaN checks
	// for Arccos but not for Arcsin, Arctan etc.?
	__forceinline FASTRETURNCHECK(Vec4V_Out) Arccos( Vec4V_In a )
	{
#if __ASSERT
		const u32 isInRange = IsGreaterThanOrEqualAll(a, Vec4V(V_NEGONE)) & IsLessThanOrEqualAll(a, Vec4V(V_ONE));
		const u32 isNotNan  = 1;//IsEqualIntAll(Vec4V(IsNotNan(a).GetIntrin128()), Vec4V(V_T_T_T_T));

		if ((isInRange & isNotNan) == 0)
		{
			mthWarningf( "Arccos has an invalid input angle %f %f %f %f, should be [-1..1]", a.GetXf(), a.GetYf(), a.GetZf(), a.GetWf());
		}
#endif // __ASSERT

		return Vec4V( Vec::V4Arccos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Arccos( Vec3V_In a )
	{
#if __ASSERT
		const u32 isInRange = IsGreaterThanOrEqualAll(a, Vec3V(V_NEGONE)) & IsLessThanOrEqualAll(a, Vec3V(V_ONE));
		const u32 isNotNan  = 1;//IsEqualIntAll(Vec3V(IsNotNan(a).GetIntrin128()), Vec3V(V_T_T_T_T));

		if ((isInRange & isNotNan) == 0)
		{
			mthWarningf( "Arccos has an invalid input angle %f %f %f, should be [-1..1]", a.GetXf(), a.GetYf(), a.GetZf());
		}
#endif // __ASSERT

		return Vec3V( Vec::V4Arccos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Arccos( Vec2V_In a )
	{
#if __ASSERT
		const u32 isInRange = IsGreaterThanOrEqualAll(a, Vec2V(V_NEGONE)) & IsLessThanOrEqualAll(a, Vec2V(V_ONE));
		const u32 isNotNan  = 1;//IsEqualIntAll(Vec2V(IsNotNan(a).GetIntrin128()), Vec2V(V_T_T_T_T));

		if ((isInRange & isNotNan) == 0)
		{
			mthWarningf( "Arccos has an invalid input angle %f %f, should be [-1..1]", a.GetXf(), a.GetYf());
		}
#endif // __ASSERT

		return Vec2V( Vec::V4Arccos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Arccos( ScalarV_In a )
	{
#if __ASSERT
		const u32 isInRange = IsGreaterThanOrEqualAll(a, ScalarV(V_NEGONE)) & IsLessThanOrEqualAll(a, ScalarV(V_ONE));
		const u32 isNotNan  = 1;//IsEqualIntAll(ScalarV(IsNotNan(a).GetIntrin128()), ScalarV(V_T_T_T_T));

		if ((isInRange & isNotNan) == 0)
		{
			//mthWarningf( "Arccos has an invalid input angle %f, should be [-1..1]", a.Getf());

			// temporarily displaying a more verbose warning for the ScalarV case, because something is causing this to fail in a weird way ..
			mthWarningf(
				"Arccos has an invalid input angle %f %f %f %f, should be [-1..1] (isInRange=%d,isNotNan=%d)",
				Vec::GetX(a.GetIntrin128()),
				Vec::GetY(a.GetIntrin128()),
				Vec::GetZ(a.GetIntrin128()),
				Vec::GetW(a.GetIntrin128()),
				isInRange,
				isNotNan
			);
		}
#endif // __ASSERT

		return ScalarV( Vec::V4Arccos( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Arctan( Vec4V_In a )
	{
		return Vec4V( Vec::V4Arctan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Arctan( Vec3V_In a )
	{
		return Vec3V( Vec::V4Arctan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Arctan( Vec2V_In a )
	{
		return Vec2V( Vec::V4Arctan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Arctan( ScalarV_In a )
	{
		return ScalarV( Vec::V4Arctan( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Arctan2( Vec4V_In y, Vec4V_In x )
	{
		return Vec4V( Vec::V4Arctan2(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Arctan2( Vec3V_In y, Vec3V_In x )
	{
		return Vec3V( Vec::V4Arctan2(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Arctan2( Vec2V_In y, Vec2V_In x )
	{
		return Vec2V( Vec::V4Arctan2(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Arctan2( ScalarV_In y, ScalarV_In x )
	{
		return ScalarV( Vec::V4Arctan2(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline void SinAndCosFast( Vec4V_InOut inOutSin, Vec4V_InOut inOutCos, Vec4V_In x )
	{
		Vec::V4SinAndCosFast( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline void SinAndCosFast( Vec3V_InOut inOutSin, Vec3V_InOut inOutCos, Vec3V_In x )
	{
		Vec::V4SinAndCosFast( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline void SinAndCosFast( Vec2V_InOut inOutSin, Vec2V_InOut inOutCos, Vec2V_In x )
	{
		Vec::V4SinAndCosFast( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline void SinAndCosFast( ScalarV_InOut inOutSin, ScalarV_InOut inOutCos, ScalarV_In x )
	{
		Vec::V4SinAndCosFast( inOutSin.GetIntrin128Ref(), inOutCos.GetIntrin128Ref(), x.GetIntrin128() );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SinFast( Vec4V_In a )
	{
		return Vec4V( Vec::V4SinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SinFast( Vec3V_In a )
	{
		return Vec3V( Vec::V4SinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SinFast( Vec2V_In a )
	{
		return Vec2V( Vec::V4SinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SinFast( ScalarV_In a )
	{
		return ScalarV( Vec::V4SinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) CosFast( Vec4V_In a )
	{
		return Vec4V( Vec::V4CosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) CosFast( Vec3V_In a )
	{
		return Vec3V( Vec::V4CosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) CosFast( Vec2V_In a )
	{
		return Vec2V( Vec::V4CosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) CosFast( ScalarV_In a )
	{
		return ScalarV( Vec::V4CosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) TanFast( Vec4V_In a )
	{
		return Vec4V( Vec::V4TanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) TanFast( Vec3V_In a )
	{
		return Vec3V( Vec::V4TanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) TanFast( Vec2V_In a )
	{
		return Vec2V( Vec::V4TanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) TanFast( ScalarV_In a )
	{
		return ScalarV( Vec::V4TanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) ArcsinFast( Vec4V_In a )
	{
		return Vec4V( Vec::V4ArcsinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) ArcsinFast( Vec3V_In a )
	{
		return Vec3V( Vec::V4ArcsinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) ArcsinFast( Vec2V_In a )
	{
		return Vec2V( Vec::V4ArcsinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) ArcsinFast( ScalarV_In a )
	{
		return ScalarV( Vec::V4ArcsinFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) ArccosFast( Vec4V_In a )
	{
		return Vec4V( Vec::V4ArccosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) ArccosFast( Vec3V_In a )
	{
		return Vec3V( Vec::V4ArccosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) ArccosFast( Vec2V_In a )
	{
		return Vec2V( Vec::V4ArccosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) ArccosFast( ScalarV_In a )
	{
		return ScalarV( Vec::V4ArccosFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) ArctanFast( Vec4V_In a )
	{
		return Vec4V( Vec::V4ArctanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) ArctanFast( Vec3V_In a )
	{
		return Vec3V( Vec::V4ArctanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) ArctanFast( Vec2V_In a )
	{
		return Vec2V( Vec::V4ArctanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) ArctanFast( ScalarV_In a )
	{
		return ScalarV( Vec::V4ArctanFast( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Arctan2Fast( Vec4V_In y, Vec4V_In x )
	{
		return Vec4V( Vec::V4Arctan2Fast(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Arctan2Fast( Vec3V_In y, Vec3V_In x )
	{
		return Vec3V( Vec::V4Arctan2Fast(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Arctan2Fast( Vec2V_In y, Vec2V_In x )
	{
		return Vec2V( Vec::V4Arctan2Fast(y.GetIntrin128(), x.GetIntrin128()) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Arctan2Fast( ScalarV_In y, ScalarV_In x )
	{
		return ScalarV( Vec::V4Arctan2Fast(y.GetIntrin128(), x.GetIntrin128()) );
	}



	__forceinline FASTRETURNCHECK(Vec4V_Out) SlowInOut( Vec4V_In t )
	{
		return Vec4V( Vec::V4SlowInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SlowInOut( Vec3V_In t )
	{
		return Vec3V( Vec::V4SlowInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SlowInOut( Vec2V_In t )
	{
		return Vec2V( Vec::V4SlowInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SlowInOut( ScalarV_In t )
	{
		return ScalarV( Vec::V4SlowInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SlowIn( Vec4V_In t )
	{
		return Vec4V( Vec::V4SlowIn( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SlowIn( Vec3V_In t )
	{
		return Vec3V( Vec::V4SlowIn( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SlowIn( Vec2V_In t )
	{
		return Vec2V( Vec::V4SlowIn( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SlowIn( ScalarV_In t )
	{
		return ScalarV( Vec::V4SlowIn( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SlowOut( Vec4V_In t )
	{
		return Vec4V( Vec::V4SlowOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SlowOut( Vec3V_In t )
	{
		return Vec3V( Vec::V4SlowOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SlowOut( Vec2V_In t )
	{
		return Vec2V( Vec::V4SlowOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SlowOut( ScalarV_In t )
	{
		return ScalarV( Vec::V4SlowOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) BellInOut( Vec4V_In t )
	{
		return Vec4V( Vec::V4BellInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) BellInOut( Vec3V_In t )
	{
		return Vec3V( Vec::V4BellInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) BellInOut( Vec2V_In t )
	{
		return Vec2V( Vec::V4BellInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) BellInOut( ScalarV_In t )
	{
		return ScalarV( Vec::V4BellInOut( t.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Range( Vec4V_In t, Vec4V_In lower, Vec4V_In upper )
	{
		return Vec4V( Vec::V4Range( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Range( Vec3V_In t, Vec3V_In lower, Vec3V_In upper )
	{
		return Vec3V( Vec::V4Range( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Range( Vec2V_In t, Vec2V_In lower, Vec2V_In upper )
	{
		return Vec2V( Vec::V4Range( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Range( ScalarV_In t, ScalarV_In lower, ScalarV_In upper )
	{
		return ScalarV( Vec::V4Range( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RangeSafe( Vec4V_In t, Vec4V_In lower, Vec4V_In upper, Vec4V_In errValVect )
	{
		return Vec4V( Vec::V4RangeSafe( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RangeSafe( Vec3V_In t, Vec3V_In lower, Vec3V_In upper, Vec3V_In errValVect )
	{
		return Vec3V( Vec::V4RangeSafe( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RangeSafe( Vec2V_In t, Vec2V_In lower, Vec2V_In upper, Vec2V_In errValVect )
	{
		return Vec2V( Vec::V4RangeSafe( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RangeSafe( ScalarV_In t, ScalarV_In lower, ScalarV_In upper, ScalarV_In errValVect )
	{
		return ScalarV( Vec::V4RangeSafe( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RangeFast( Vec4V_In t, Vec4V_In lower, Vec4V_In upper )
	{
		return Vec4V( Vec::V4RangeFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RangeFast( Vec3V_In t, Vec3V_In lower, Vec3V_In upper )
	{
		return Vec3V( Vec::V4RangeFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RangeFast( Vec2V_In t, Vec2V_In lower, Vec2V_In upper )
	{
		return Vec2V( Vec::V4RangeFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RangeFast( ScalarV_In t, ScalarV_In lower, ScalarV_In upper )
	{
		return ScalarV( Vec::V4RangeFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RangeClamp( Vec4V_In t, Vec4V_In lower, Vec4V_In upper )
	{
		return Vec4V( Vec::V4RangeClamp( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RangeClamp( Vec3V_In t, Vec3V_In lower, Vec3V_In upper )
	{
		return Vec3V( Vec::V4RangeClamp( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RangeClamp( Vec2V_In t, Vec2V_In lower, Vec2V_In upper )
	{
		return Vec2V( Vec::V4RangeClamp( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RangeClamp( ScalarV_In t, ScalarV_In lower, ScalarV_In upper )
	{
		return ScalarV( Vec::V4RangeClamp( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RangeClampFast( Vec4V_In t, Vec4V_In lower, Vec4V_In upper )
	{
		return Vec4V( Vec::V4RangeClampFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RangeClampFast( Vec3V_In t, Vec3V_In lower, Vec3V_In upper )
	{
		return Vec3V( Vec::V4RangeClampFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RangeClampFast( Vec2V_In t, Vec2V_In lower, Vec2V_In upper )
	{
		return Vec2V( Vec::V4RangeClampFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RangeClampFast( ScalarV_In t, ScalarV_In lower, ScalarV_In upper )
	{
		return ScalarV( Vec::V4RangeClampFast( t.GetIntrin128(), lower.GetIntrin128(), upper.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Ramp( Vec4V_In x, Vec4V_In funcInA, Vec4V_In funcInB, Vec4V_In funcOutA, Vec4V_In funcOutB )
	{
		return Vec4V( Vec::V4Ramp( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Ramp( Vec3V_In x, Vec3V_In funcInA, Vec3V_In funcInB, Vec3V_In funcOutA, Vec3V_In funcOutB )
	{
		return Vec3V( Vec::V4Ramp( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Ramp( Vec2V_In x, Vec2V_In funcInA, Vec2V_In funcInB, Vec2V_In funcOutA, Vec2V_In funcOutB )
	{
		return Vec2V( Vec::V4Ramp( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Ramp( ScalarV_In x, ScalarV_In funcInA, ScalarV_In funcInB, ScalarV_In funcOutA, ScalarV_In funcOutB )
	{
		return ScalarV( Vec::V4Ramp( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) RampFast( Vec4V_In x, Vec4V_In funcInA, Vec4V_In funcInB, Vec4V_In funcOutA, Vec4V_In funcOutB )
	{
		return Vec4V( Vec::V4RampFast( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) RampFast( Vec3V_In x, Vec3V_In funcInA, Vec3V_In funcInB, Vec3V_In funcOutA, Vec3V_In funcOutB )
	{
		return Vec3V( Vec::V4RampFast( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) RampFast( Vec2V_In x, Vec2V_In funcInA, Vec2V_In funcInB, Vec2V_In funcOutA, Vec2V_In funcOutB )
	{
		return Vec2V( Vec::V4RampFast( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) RampFast( ScalarV_In x, ScalarV_In funcInA, ScalarV_In funcInB, ScalarV_In funcOutA, ScalarV_In funcOutB )
	{
		return ScalarV( Vec::V4RampFast( x.GetIntrin128(), funcInA.GetIntrin128(), funcInB.GetIntrin128(), funcOutA.GetIntrin128(), funcOutB.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) AddNet( Vec3V_In inVector, Vec3V_In toAdd )
	{
		return Vec3V( Vec::V3AddNet( inVector.GetIntrin128(), toAdd.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Angle(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleNormInput(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleVNormInput( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleX(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleXV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleY(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleYV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleZ(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleZV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleXNormInput(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleXVNormInput( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleYNormInput(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleYVNormInput( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleZNormInput(Vec3V_In v1, Vec3V_In v2)
	{
		return ScalarV( Vec::V3AngleZVNormInput( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline void MakeOrthonormals(Vec3V_In inVector, Vec3V_InOut ortho1, Vec3V_InOut ortho2)
	{
		Vec::V3MakeOrthonormals( inVector.GetIntrin128(), ortho1.GetIntrin128Ref(), ortho2.GetIntrin128Ref() );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) ApproachStraight(Vec3V_In inVect, Vec3V_In goal, Vec3V_In rate, Vec3V_In time, unsigned int& rResult)
	{
		return Vec3V( Vec::V3ApproachStraight( inVect.GetIntrin128(), goal.GetIntrin128(), rate.GetIntrin128(), time.GetIntrin128(), rResult ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) AddNet( Vec2V_In inVector, Vec2V_In toAdd )
	{
		return Vec2V( Vec::V2AddNet( inVector.GetIntrin128(), toAdd.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Angle(Vec2V_In v1, Vec2V_In v2)
	{
		return ScalarV( Vec::V2AngleV( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AngleNormInput(Vec2V_In v1, Vec2V_In v2)
	{
		return ScalarV( Vec::V2AngleVNormInput( v1.GetIntrin128(), v2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) WhichSideOfLineV(Vec2V_In point, Vec2V_In lineP1, Vec2V_In lineP2)
	{
		return ScalarV( Vec::V2WhichSideOfLineV( point.GetIntrin128(), lineP1.GetIntrin128(), lineP2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatX( Vec4V_In a )
	{
		return ScalarV( Vec::V4SplatX( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatY( Vec4V_In a )
	{
		return ScalarV( Vec::V4SplatY( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatZ( Vec4V_In a )
	{
		return ScalarV( Vec::V4SplatZ( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatW( Vec4V_In a )
	{
		return ScalarV( Vec::V4SplatW( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatX( Vec3V_In a )
	{
		return ScalarV( Vec::V4SplatX( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatY( Vec3V_In a )
	{
		return ScalarV( Vec::V4SplatY( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatZ( Vec3V_In a )
	{
		return ScalarV( Vec::V4SplatZ( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatX( Vec2V_In a )
	{
		return ScalarV( Vec::V4SplatX( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SplatY( Vec2V_In a )
	{
		return ScalarV( Vec::V4SplatY( a.GetIntrin128() ) );
	}

	__forceinline BoolV_Out SplatX( VecBoolV_In a )
	{
		return BoolV( Vec::V4SplatX( a.GetIntrin128() ) );
	}

	__forceinline BoolV_Out SplatY( VecBoolV_In a )
	{
		return BoolV( Vec::V4SplatY( a.GetIntrin128() ) );
	}

	__forceinline BoolV_Out SplatZ( VecBoolV_In a )
	{
		return BoolV( Vec::V4SplatZ( a.GetIntrin128() ) );
	}

	__forceinline BoolV_Out SplatW( VecBoolV_In a )
	{
		return BoolV( Vec::V4SplatW( a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Scale( QuatV_In a, ScalarV_In b )
	{
		return QuatV( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Scale( QuatV_In a, Vec4V_In b )
	{
		return QuatV( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Scale( Vec4V_In a, ScalarV_In b )
	{
		return Vec4V( Vec::V4Scale( a.GetIntrin128(),  b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Scale( ScalarV_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4Scale( b.GetIntrin128(), a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Scale( Vec3V_In a, ScalarV_In b )
	{
		return Vec3V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Scale( ScalarV_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4Scale( b.GetIntrin128(), a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Scale( Vec2V_In a, ScalarV_In b )
	{
		return Vec2V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Scale( ScalarV_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4Scale( b.GetIntrin128(), a.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Scale( ScalarV_In a, ScalarV_In b )
	{
		return ScalarV( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Scale( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Scale( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Scale( Vec2V_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Cross( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V3Cross( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Cross3( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V3Cross( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Cross( Vec2V_In a, Vec2V_In b )
	{
		return ScalarV( Vec::V2CrossV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) AddCrossed( Vec3V_In toAddTo, Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V3AddCrossed( toAddTo.GetIntrin128(), a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) AddCrossed3( Vec4V_In toAddTo, Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V3AddCrossed( toAddTo.GetIntrin128(), a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SubtractCrossed( Vec3V_In toSubtractFrom, Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V3SubtractCrossed( toSubtractFrom.GetIntrin128(), a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SubtractCrossed3( Vec4V_In toSubtractFrom, Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V3SubtractCrossed( toSubtractFrom.GetIntrin128(), a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dot( Vec4V_In a, Vec4V_In b )
	{
		return ScalarV( Vec::V4DotV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dot( Vec3V_In a, Vec3V_In b )
	{
		return ScalarV( Vec::V3DotV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dot( Vec2V_In a, Vec2V_In b )
	{
		return ScalarV( Vec::V2DotV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Add( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4Add( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Add( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4Add( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Add( Vec2V_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4Add( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Add( ScalarV_In a, ScalarV_In b )
	{
		return ScalarV( Vec::V4Add( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Subtract( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4Subtract( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Subtract( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4Subtract( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Subtract( Vec2V_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4Subtract( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Subtract( ScalarV_In a, ScalarV_In b )
	{
		return ScalarV( Vec::V4Subtract( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) AddInt( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4AddInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) AddInt( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4AddInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) AddInt( Vec2V_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4AddInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AddInt( ScalarV_In a, ScalarV_In b )
	{
		return ScalarV( Vec::V4AddInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SubtractInt( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4SubtractInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SubtractInt( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4SubtractInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SubtractInt( Vec2V_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4SubtractInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SubtractInt( ScalarV_In a, ScalarV_In b )
	{
		return ScalarV( Vec::V4SubtractInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Average( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4Average( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Average( Vec3V_In a, Vec3V_In b )
	{
		return Vec3V( Vec::V4Average( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Average( Vec2V_In a, Vec2V_In b )
	{
		return Vec2V( Vec::V4Average( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Average( ScalarV_In a, ScalarV_In b )
	{
		return ScalarV( Vec::V4Average( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) AddScaled( Vec4V_In toAdd, Vec4V_In toScaleThenAdd, ScalarV_In scaleValue )
	{
		return Vec4V( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) AddScaled( Vec4V_In toAdd, Vec4V_In toScaleThenAdd, Vec4V_In scaleValues )
	{
		return Vec4V( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) AddScaled( Vec3V_In toAdd, Vec3V_In toScaleThenAdd, ScalarV_In scaleValue )
	{
		return Vec3V( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) AddScaled( Vec3V_In toAdd, Vec3V_In toScaleThenAdd, Vec3V_In scaleValues )
	{
		return Vec3V( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) AddScaled( Vec2V_In toAdd, Vec2V_In toScaleThenAdd, ScalarV_In scaleValue )
	{
		return Vec2V( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) AddScaled( Vec2V_In toAdd, Vec2V_In toScaleThenAdd, Vec2V_In scaleValues )
	{
		return Vec2V( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) AddScaled( ScalarV_In toAdd, ScalarV_In toScaleThenAdd, ScalarV_In scaleValue )
	{
		return ScalarV( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SubtractScaled( Vec4V_In toSubtractFrom, Vec4V_In toScaleThenSubtract, ScalarV_In scaleValue )
	{
		return Vec4V( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SubtractScaled( Vec4V_In toSubtractFrom, Vec4V_In toScaleThenSubtract, Vec4V_In scaleValues )
	{
		return Vec4V( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SubtractScaled( Vec3V_In toSubtractFrom, Vec3V_In toScaleThenSubtract, ScalarV_In scaleValue )
	{
		return Vec3V( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SubtractScaled( Vec3V_In toSubtractFrom, Vec3V_In toScaleThenSubtract, Vec3V_In scaleValues )
	{
		return Vec3V( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SubtractScaled( Vec2V_In toSubtractFrom, Vec2V_In toScaleThenSubtract, ScalarV_In scaleValue )
	{
		return Vec2V( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SubtractScaled( Vec2V_In toSubtractFrom, Vec2V_In toScaleThenSubtract, Vec2V_In scaleValues )
	{
		return Vec2V( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SubtractScaled( ScalarV_In toSubtractFrom, ScalarV_In toScaleThenSubtract, ScalarV_In scaleValue )
	{
		return ScalarV( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScale( Vec4V_In toScale, ScalarV_In scaleValue )
	{
		return Vec4V( Vec::V4InvScale( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScale( Vec4V_In toScale, Vec4V_In scaleValues )
	{
		return Vec4V( Vec::V4InvScale( toScale.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScaleSafe( Vec4V_In toScale, ScalarV_In scaleValue, Vec4V_In errValVect )
	{
		return Vec4V( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScaleSafe( Vec4V_In toScale, Vec4V_In scaleValues, Vec4V_In errValVect )
	{
		return Vec4V( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValues.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScaleFast( Vec4V_In toScale, ScalarV_In scaleValue )
	{
		return Vec4V( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScaleFast( Vec4V_In toScale, Vec4V_In scaleValues )
	{
		return Vec4V( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScaleFastSafe( Vec4V_In toScale, ScalarV_In scaleValue, Vec4V_In errValVect  )
	{
		return Vec4V( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) InvScaleFastSafe( Vec4V_In toScale, Vec4V_In scaleValues, Vec4V_In errValVect )
	{
		return Vec4V( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValues.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScale( Vec3V_In toScale, ScalarV_In scaleValue )
	{
		return Vec3V( Vec::V4InvScale( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScale( Vec3V_In toScale, Vec3V_In scaleValues )
	{
		return Vec3V( Vec::V4InvScale( toScale.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScaleSafe( Vec3V_In toScale, ScalarV_In scaleValue, Vec3V_In errValVect )
	{
		return Vec3V( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScaleSafe( Vec3V_In toScale, Vec3V_In scaleValues, Vec3V_In errValVect )
	{
		return Vec3V( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValues.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScaleFast( Vec3V_In toScale, ScalarV_In scaleValue )
	{
		return Vec3V( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScaleFast( Vec3V_In toScale, Vec3V_In scaleValues )
	{
		return Vec3V( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScaleFastSafe( Vec3V_In toScale, ScalarV_In scaleValue, Vec3V_In errValVect )
	{
		return Vec3V( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) InvScaleFastSafe( Vec3V_In toScale, Vec3V_In scaleValues, Vec3V_In errValVect )
	{
		return Vec3V( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValues.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScale( Vec2V_In toScale, ScalarV_In scaleValue )
	{
		return Vec2V( Vec::V4InvScale( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScale( Vec2V_In toScale, Vec2V_In scaleValues )
	{
		return Vec2V( Vec::V4InvScale( toScale.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScaleSafe( Vec2V_In toScale, ScalarV_In scaleValue, Vec2V_In errValVect )
	{
		return Vec2V( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScaleSafe( Vec2V_In toScale, Vec2V_In scaleValues, Vec2V_In errValVect )
	{
		return Vec2V( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValues.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScaleFast( Vec2V_In toScale, ScalarV_In scaleValue )
	{
		return Vec2V( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScaleFast( Vec2V_In toScale, Vec2V_In scaleValues )
	{
		return Vec2V( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScaleFastSafe( Vec2V_In toScale, ScalarV_In scaleValue, Vec2V_In errValVect )
	{
		return Vec2V( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) InvScaleFastSafe( Vec2V_In toScale, Vec2V_In scaleValues, Vec2V_In errValVect )
	{
		return Vec2V( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValues.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvScale( ScalarV_In toScale, ScalarV_In scaleValue )
	{
		return ScalarV( Vec::V4InvScale( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvScaleSafe( ScalarV_In toScale, ScalarV_In scaleValue, ScalarV_In errValVect )
	{
		return ScalarV( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvScaleFast( ScalarV_In toScale, ScalarV_In scaleValue )
	{
		return ScalarV( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvScaleFastSafe( ScalarV_In toScale, ScalarV_In scaleValue, ScalarV_In errValVect )
	{
		return ScalarV( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Lerp( ScalarV_In tValue, Vec4V_In vectA, Vec4V_In vectB )
	{
		return Vec4V( Vec::V4Lerp( tValue.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Lerp( Vec4V_In tValues, Vec4V_In vectA, Vec4V_In vectB )
	{
		return Vec4V( Vec::V4Lerp( tValues.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Lerp( ScalarV_In tValue, Vec3V_In vectA, Vec3V_In vectB )
	{
		return Vec3V( Vec::V4Lerp( tValue.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Lerp( Vec3V_In tValues, Vec3V_In vectA, Vec3V_In vectB )
	{
		return Vec3V( Vec::V4Lerp( tValues.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Lerp( ScalarV_In tValue, Vec2V_In vectA, Vec2V_In vectB )
	{
		return Vec2V( Vec::V4Lerp( tValue.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Lerp( Vec2V_In tValues, Vec2V_In vectA, Vec2V_In vectB )
	{
		return Vec2V( Vec::V4Lerp( tValues.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Lerp( ScalarV_In tValue, ScalarV_In vectA, ScalarV_In vectB )
	{
		return ScalarV( Vec::V4Lerp( tValue.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXY( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4MergeXY( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXY( Vec3V_In a, Vec3V_In b )
	{
		return Vec4V( Vec::V4MergeXY( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXY( Vec2V_In a, Vec2V_In b )
	{
		return Vec4V( Vec::V4MergeXY( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(VecBoolV_Out) MergeXY( VecBoolV_In a, VecBoolV_In b )
	{
		return VecBoolV( Vec::V4MergeXY( a.GetIntrin128(), b.GetIntrin128() ) );
	}

#if !SCALAR_TYPES_ONLY
	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXYShort( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4MergeXYShort( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXYShort( Vec3V_In a, Vec3V_In b )
	{
		return Vec4V( Vec::V4MergeXYShort( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXYShort( Vec2V_In a, Vec2V_In b )
	{
		return Vec4V( Vec::V4MergeXYShort( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXYByte( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4MergeXYByte( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXYByte( Vec3V_In a, Vec3V_In b )
	{
		return Vec4V( Vec::V4MergeXYByte( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeXYByte( Vec2V_In a, Vec2V_In b )
	{
		return Vec4V( Vec::V4MergeXYByte( a.GetIntrin128(), b.GetIntrin128() ) );
	}
#endif

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeZW( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4MergeZW( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(VecBoolV_Out) MergeZW( VecBoolV_In a, VecBoolV_In b )
	{
		return VecBoolV( Vec::V4MergeZW( a.GetIntrin128(), b.GetIntrin128() ) );
	}

#if !SCALAR_TYPES_ONLY
	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeZWShort( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4MergeZWShort( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) MergeZWByte( Vec4V_In a, Vec4V_In b )
	{
		return Vec4V( Vec::V4MergeZWByte( a.GetIntrin128(), b.GetIntrin128() ) );
	}
#endif

	__forceinline FASTRETURNCHECK(Vec4V_Out) Pow( Vec4V_In x, Vec4V_In y )
	{
		return Vec4V( Vec::V4Pow( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Pow( Vec3V_In x, Vec3V_In y )
	{
		return Vec3V( Vec::V4Pow( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Pow( Vec2V_In x, Vec2V_In y )
	{
		return Vec2V( Vec::V4Pow( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Pow( ScalarV_In x, ScalarV_In y )
	{
		return ScalarV( Vec::V4Pow( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) PowPrecise( Vec4V_In x, Vec4V_In y )
	{
		return Vec4V( Vec::V4PowPrecise( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) PowPrecise( Vec3V_In x, Vec3V_In y )
	{
		return Vec3V( Vec::V4PowPrecise( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) PowPrecise( Vec2V_In x, Vec2V_In y )
	{
		return Vec2V( Vec::V4PowPrecise( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) PowPrecise( ScalarV_In x, ScalarV_In y )
	{
		return ScalarV( Vec::V4PowPrecise( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Expt( Vec4V_In x )
	{
		return Vec4V( Vec::V4Expt( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Expt( Vec3V_In x )
	{
		return Vec3V( Vec::V4Expt( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Expt( Vec2V_In x )
	{
		return Vec2V( Vec::V4Expt( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Expt( ScalarV_In x )
	{
		return ScalarV( Vec::V4Expt( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Log2( Vec4V_In x )
	{
		return Vec4V( Vec::V4Log2( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Log2( Vec3V_In x )
	{
		return Vec3V( Vec::V4Log2( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Log2( Vec2V_In x )
	{
		return Vec2V( Vec::V4Log2( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Log2( ScalarV_In x )
	{
		return ScalarV( Vec::V4Log2( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Log10( Vec4V_In x )
	{
		return Vec4V( Vec::V4Log10( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Log10( Vec3V_In x )
	{
		return Vec3V( Vec::V4Log10( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Log10( Vec2V_In x )
	{
		return Vec2V( Vec::V4Log10( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Log10( ScalarV_In x )
	{
		return ScalarV( Vec::V4Log10( x.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Modulus(Vec4V_In inVec, ScalarV_In mod)
	{
		return Vec4V( Vec::V4Modulus( inVec.GetIntrin128(), mod.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Modulus(Vec3V_In inVec, ScalarV_In mod)
	{
		return Vec3V( Vec::V4Modulus( inVec.GetIntrin128(), mod.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Modulus(Vec2V_In inVec, ScalarV_In mod)
	{
		return Vec2V( Vec::V4Modulus( inVec.GetIntrin128(), mod.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Modulus(ScalarV_In inVec, ScalarV_In mod)
	{
		return ScalarV( Vec::V4Modulus( inVec.GetIntrin128(), mod.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dist(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4DistV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistFast(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4DistVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dist(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3DistV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistFast(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3DistVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dist(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2DistV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistFast(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2DistVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDist(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4InvDistV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSafe(Vec4V_In a, Vec4V_In b, Vec4V_In errValVect)
	{
		return ScalarV( Vec::V4InvDistVSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistFast(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4InvDistVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistFastSafe(Vec4V_In a, Vec4V_In b, Vec4V_In errValVect)
	{
		return ScalarV( Vec::V4InvDistVFastSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDist(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3InvDistV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSafe(Vec3V_In a, Vec3V_In b, Vec3V_In errValVect)
	{
		return ScalarV( Vec::V3InvDistVSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistFast(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3InvDistVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistFastSafe(Vec3V_In a, Vec3V_In b, Vec3V_In errValVect)
	{
		return ScalarV( Vec::V3InvDistVFastSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDist(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2InvDistV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSafe(Vec2V_In a, Vec2V_In b, Vec2V_In errValVect)
	{
		return ScalarV( Vec::V2InvDistVSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistFast(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2InvDistVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistFastSafe(Vec2V_In a, Vec2V_In b, Vec2V_In errValVect)
	{
		return ScalarV( Vec::V2InvDistVFastSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistSquared(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4DistSquaredV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistSquared(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3DistSquaredV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) DistSquared(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2DistSquaredV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquared(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4InvDistSquaredV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredSafe(Vec4V_In a, Vec4V_In b, Vec4V_In errValVect)
	{
		return ScalarV( Vec::V4InvDistSquaredVSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFast(Vec4V_In a, Vec4V_In b)
	{
		return ScalarV( Vec::V4InvDistSquaredVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFastSafe(Vec4V_In a, Vec4V_In b, Vec4V_In errValVect)
	{
		return ScalarV( Vec::V4InvDistSquaredVFastSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquared(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3InvDistSquaredV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredSafe(Vec3V_In a, Vec3V_In b, Vec3V_In errValVect)
	{
		return ScalarV( Vec::V3InvDistSquaredVSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFast(Vec3V_In a, Vec3V_In b)
	{
		return ScalarV( Vec::V3InvDistSquaredVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFastSafe(Vec3V_In a, Vec3V_In b, Vec3V_In errValVect)
	{
		return ScalarV( Vec::V3InvDistSquaredVFastSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquared(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2InvDistSquaredV( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredSafe(Vec2V_In a, Vec2V_In b, Vec2V_In errValVect)
	{
		return ScalarV( Vec::V2InvDistSquaredVSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFast(Vec2V_In a, Vec2V_In b)
	{
		return ScalarV( Vec::V2InvDistSquaredVFast( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFastSafe(Vec2V_In a, Vec2V_In b, Vec2V_In errValVect)
	{
		return ScalarV( Vec::V2InvDistSquaredVFastSafe( a.GetIntrin128(), b.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SelectFT(VecBoolV_In choiceVector, Vec4V_In zero, Vec4V_In nonZero)
	{
		return Vec4V( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SelectFT(VecBoolV_In choiceVector, Vec3V_In zero, Vec3V_In nonZero)
	{
		return Vec3V( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SelectFT(VecBoolV_In choiceVector, Vec2V_In zero, Vec2V_In nonZero)
	{
		return Vec2V( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SelectFT(VecBoolV_In choiceVector, ScalarV_In zero, ScalarV_In nonZero)
	{
		return ScalarV( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) SelectFT(VecBoolV_In choiceVector, QuatV_In zero, QuatV_In nonZero)
	{
		return QuatV( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Mat33V_Out) SelectFT(VecBoolV_In choiceVector, Mat33V_In zero, Mat33V_In nonZero)
	{
		return Mat33V(	Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetCol0().GetIntrin128(), nonZero.GetCol0().GetIntrin128() ),
			Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetCol1().GetIntrin128(), nonZero.GetCol1().GetIntrin128() ),
			Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetCol2().GetIntrin128(), nonZero.GetCol2().GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) SelectFT(BoolV_In choiceVector, Vec4V_In zero, Vec4V_In nonZero)
	{
		return Vec4V( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) SelectFT(BoolV_In choiceVector, Vec3V_In zero, Vec3V_In nonZero)
	{
		return Vec3V( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) SelectFT(BoolV_In choiceVector, Vec2V_In zero, Vec2V_In nonZero)
	{
		return Vec2V( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) SelectFT(BoolV_In choiceVector, ScalarV_In zero, ScalarV_In nonZero)
	{
		return ScalarV( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) SelectFT(BoolV_In choiceVector, QuatV_In zero, QuatV_In nonZero)
	{
		return QuatV( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Max(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec4V( Vec::V4Max( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Mat33V_Out) SelectFT(BoolV_In choiceVector, Mat33V_In zero, Mat33V_In nonZero)
	{
		return Mat33V(	Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetCol0().GetIntrin128(), nonZero.GetCol0().GetIntrin128() ),
			Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetCol1().GetIntrin128(), nonZero.GetCol1().GetIntrin128() ),
			Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetCol2().GetIntrin128(), nonZero.GetCol2().GetIntrin128() ));
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Max(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec3V( Vec::V4Max( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Max(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec2V( Vec::V4Max( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Max(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return ScalarV( Vec::V4Max( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

#if __WIN32PC
	__forceinline FASTRETURNCHECK(ScalarV_Out) Max(ScalarV_In inVector1, ScalarV_In inVector2, ScalarV_In inVector3)
	{
		return Max( Max( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Max(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In inVector3)
	{
		return Max( Max( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Max(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In inVector3)
	{
		return Max( Max( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Max(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In inVector3)
	{
		return Max( Max( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Max(ScalarV_In inVect1, ScalarV_In inVect2, ScalarV_In inVect3, ScalarV_In inVect4)
	{
		return Max( Max( inVect1, inVect2 ), Max( inVect3, inVect4 ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Max(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In inVect3, Vec2V_In inVect4)
	{
		return Max( Max( inVect1, inVect2 ), Max( inVect3, inVect4 ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Max(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In inVect3, Vec3V_In inVect4)
	{
		return Max( Max( inVect1, inVect2 ), Max( inVect3, inVect4 ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Max(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In inVect3, Vec4V_In inVect4)
	{
		return Max( Max( inVect1, inVect2 ), Max( inVect3, inVect4 ) );
	}
#endif // __WIN32PC

	__forceinline FASTRETURNCHECK(Vec4V_Out) Min(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec4V( Vec::V4Min( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Min(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec3V( Vec::V4Min( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Min(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec2V( Vec::V4Min( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Min(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return ScalarV( Vec::V4Min( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

#if __WIN32PC
	__forceinline FASTRETURNCHECK(ScalarV_Out) Min(ScalarV_In inVector1, ScalarV_In inVector2, ScalarV_In inVector3)
	{
		return Min( Min( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Min(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In inVector3)
	{
		return Min( Min( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Min(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In inVector3)
	{
		return Min( Min( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Min(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In inVector3)
	{
		return Min( Min( inVector1, inVector2 ), inVector3 );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Min(ScalarV_In inVect1, ScalarV_In inVect2, ScalarV_In inVect3, ScalarV_In inVect4)
	{
		return Min( Min( inVect1, inVect2 ), Min( inVect3, inVect4 ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Min(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In inVect3, Vec2V_In inVect4)
	{
		return Min( Min( inVect1, inVect2 ), Min( inVect3, inVect4 ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Min(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In inVect3, Vec3V_In inVect4)
	{
		return Min( Min( inVect1, inVect2 ), Min( inVect3, inVect4 ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Min(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In inVect3, Vec4V_In inVect4)
	{
		return Min( Min( inVect1, inVect2 ), Min( inVect3, inVect4 ) );
	}
#endif // __WIN32PC

	__forceinline FASTRETURNCHECK(ScalarV_Out) MinElement(Vec2V_In inVector)
	{
		return ScalarV( Vec::V2MinElement( inVector.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MaxElement(Vec2V_In inVector)
	{
		return ScalarV( Vec::V2MaxElement( inVector.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MinElement(Vec3V_In inVector)
	{
		return ScalarV( Vec::V3MinElement( inVector.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MaxElement(Vec3V_In inVector)
	{
		return ScalarV( Vec::V3MaxElement( inVector.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MinElement(Vec4V_In inVector)
	{
		return ScalarV( Vec::V4MinElement( inVector.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) MaxElement(Vec4V_In inVector)
	{
		return ScalarV( Vec::V4MaxElement( inVector.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) And(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec4V( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) And(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec3V( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) And(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec2V( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) And(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return ScalarV( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out And(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return VecBoolV( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline BoolV_Out And(BoolV_In inVector1, BoolV_In inVector2)
	{
		return BoolV( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Or(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec4V( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Or(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec3V( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Or(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec2V( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Or(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return ScalarV( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out Or(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return VecBoolV( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline BoolV_Out Or(BoolV_In inVector1, BoolV_In inVector2)
	{
		return BoolV( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Xor(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec4V( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Xor(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec3V( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Xor(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec2V( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Xor(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return ScalarV( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out Xor(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return VecBoolV( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline BoolV_Out Xor(BoolV_In inVector1, BoolV_In inVector2)
	{
		return BoolV( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Andc(Vec4V_In inVector1, Vec4V_In inVector2)
	{
		return Vec4V( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Andc(Vec3V_In inVector1, Vec3V_In inVector2)
	{
		return Vec3V( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec2V_Out) Andc(Vec2V_In inVector1, Vec2V_In inVector2)
	{
		return Vec2V( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Andc(ScalarV_In inVector1, ScalarV_In inVector2)
	{
		return ScalarV( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline VecBoolV_Out Andc(VecBoolV_In inVector1, VecBoolV_In inVector2)
	{
		return VecBoolV( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline BoolV_Out Andc(BoolV_In inVector1, BoolV_In inVector2)
	{
		return BoolV( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 )
	{
		// Allowing w in this assertion for now, since the users might want to grab a component stuffed into W.
		CompileTimeAssert(	(permX==Vec::X1 || permX==Vec::Y1 || permX==Vec::Z1 || permX==Vec::W1 ||
							permX==Vec::X2 || permX==Vec::Y2 || permX==Vec::Z2 || permX==Vec::W2) &&
							(permY==Vec::X1 || permY==Vec::Y1 || permY==Vec::Z1 || permY==Vec::W1 ||
							permY==Vec::X2 || permY==Vec::Y2 || permY==Vec::Z2 || permY==Vec::W2) &&
							(permZ==Vec::X1 || permZ==Vec::Y1 || permZ==Vec::Z1 || permZ==Vec::W1 ||
							permZ==Vec::X2 || permZ==Vec::Y2 || permZ==Vec::Z2 || permZ==Vec::W2) &&
							(permW==Vec::X1 || permW==Vec::Y1 || permW==Vec::Z1 || permW==Vec::W1 ||
							permW==Vec::X2 || permW==Vec::Y2 || permW==Vec::Z2 || permW==Vec::W2) );
							// Invalid permute args!

		CompileTimeAssert(	!(permX <= Vec::W1 && permY <= Vec::W1 && permZ <= Vec::W1 && permW <= Vec::W1) &&
							!(permX >= Vec::X2 && permY >= Vec::X2 && permZ >= Vec::X2 && permW >= Vec::X2)	);
							// Use the single-vector permute, Vec4V::Get<...>()!

		return Vec4V( Vec::V4PermuteTwo<permX, permY, permZ, permW>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2 )
	{
		// Allowing w in this assertion for now, since the users might want to grab a component stuffed into W.
		CompileTimeAssert(	(permX==Vec::X1 || permX==Vec::Y1 || permX==Vec::Z1 || permX==Vec::W1 ||
							permX==Vec::X2 || permX==Vec::Y2 || permX==Vec::Z2 || permX==Vec::W2) &&
							(permY==Vec::X1 || permY==Vec::Y1 || permY==Vec::Z1 || permY==Vec::W1 ||
							permY==Vec::X2 || permY==Vec::Y2 || permY==Vec::Z2 || permY==Vec::W2) &&
							(permZ==Vec::X1 || permZ==Vec::Y1 || permZ==Vec::Z1 || permZ==Vec::W1 ||
							permZ==Vec::X2 || permZ==Vec::Y2 || permZ==Vec::Z2 || permZ==Vec::W2) &&
							(permW==Vec::X1 || permW==Vec::Y1 || permW==Vec::Z1 || permW==Vec::W1 ||
							permW==Vec::X2 || permW==Vec::Y2 || permW==Vec::Z2 || permW==Vec::W2) );
							// Invalid permute args!

		CompileTimeAssert(	!(permX <= Vec::W1 && permY <= Vec::W1 && permZ <= Vec::W1 && permW <= Vec::W1) &&
							!(permX >= Vec::X2 && permY >= Vec::X2 && permZ >= Vec::X2 && permW >= Vec::X2)	);
							// Use the single-vector permute, Vec4V::Get<...>()!

		return Vec4V( Vec::V4PermuteTwo<permX, permY, permZ, permW>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	
	template <u32 permX, u32 permY, u32 permZ>
	__forceinline FASTRETURNCHECK(Vec3V_Out) GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2 )
	{
		// Allowing w to be grabbed for now, since the users might want to grab a component stuffed into W.
		return GetFromTwo<permX, permY, permZ>( Vec4V(inVector1), Vec4V(inVector2) );
	}

	template <u32 permX, u32 permY, u32 permZ>
	__forceinline FASTRETURNCHECK(Vec3V_Out) GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 )
	{
		CompileTimeAssert(	(permX==Vec::X1 || permX==Vec::Y1 || permX==Vec::Z1 || permX==Vec::W1 ||
							permX==Vec::X2 || permX==Vec::Y2 || permX==Vec::Z2 || permX==Vec::W2) &&
							(permY==Vec::X1 || permY==Vec::Y1 || permY==Vec::Z1 || permY==Vec::W1 ||
							permY==Vec::X2 || permY==Vec::Y2 || permY==Vec::Z2 || permY==Vec::W2) &&
							(permZ==Vec::X1 || permZ==Vec::Y1 || permZ==Vec::Z1 || permZ==Vec::W1 ||
							permZ==Vec::X2 || permZ==Vec::Y2 || permZ==Vec::Z2 || permZ==Vec::W2) );
							// Invalid permute args!

		CompileTimeAssert(	!(permX==Vec::X1 && permY==Vec::Y1 && permZ==Vec::Z1) &&
							!(permX==Vec::X2 && permY==Vec::Y2 && permZ==Vec::Z2) );
							// This permute does nothing meaningful!

		CompileTimeAssert(	!(permX <= Vec::W1 && permY <= Vec::W1 && permZ <= Vec::W1) &&
							!(permX >= Vec::X2 && permY >= Vec::X2 && permZ >= Vec::X2)	);
							// Use the single-vector permute, Vec4V::Get<...>()!

		//================================================
		// 1-instruction PS3 PPU and XBox360 permute
		// specializations.
		// (see v4permtwo1instruction_ps3.inl and
		// v4permtwo1instruction_xbox360.inl)
		// (Covers V4Splat*,V4Merge*,vsldoi,vrlimi)
		//================================================

		if(	(permX == Vec::Y1 && permY == Vec::Z1 && permZ == Vec::Z2) ||
			(permX == Vec::Y1 && permY == Vec::Y2 && permZ == Vec::W1) ||
			(permX == Vec::Y1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Z1 && permZ == Vec::W1) ||
			(permX == Vec::X2 && permY == Vec::Z1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::W1) ) // Good when permW == Vec::X1
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::X1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::X2 && permY == Vec::X1 && permZ == Vec::Y2) ||
			(permX == Vec::Z2 && permY == Vec::W2 && permZ == Vec::X1) ||
			(permX == Vec::Z1 && permY == Vec::W1 && permZ == Vec::Z2) ||
			(permX == Vec::Z1 && permY == Vec::Y2 && permZ == Vec::X1) ||
			(permX == Vec::Z1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::W1 && permZ == Vec::X1) ||
			(permX == Vec::X2 && permY == Vec::W1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::X1) ) // Good when permW == Vec::Y1
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::Y1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::W2 && permY == Vec::X1 && permZ == Vec::Y1) ||
			(permX == Vec::W1 && permY == Vec::X1 && permZ == Vec::Z2) ||
			(permX == Vec::W1 && permY == Vec::Y2 && permZ == Vec::Y1) ||
			(permX == Vec::W1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::X1 && permZ == Vec::Y1) ||
			(permX == Vec::X2 && permY == Vec::X1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::Y1) ) // Good when permW == Vec::Z1
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::Z1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::Z2 && permY == Vec::Z1 && permZ == Vec::W2) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::X2) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::Y2) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::Z2) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::W2) ||
			(permX == Vec::X1 && permY == Vec::X2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::X2 && permZ == Vec::Y2) ||
			(permX == Vec::X1 && permY == Vec::Y2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::X1 && permY == Vec::Z2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::Z2 && permZ == Vec::W2) ||
			(permX == Vec::X1 && permY == Vec::W2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::W2 && permZ == Vec::X2) ||
			(permX == Vec::W1 && permY == Vec::X1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::X2 && permY == Vec::Y1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::Z1) ||
			(permX == Vec::Y2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::Y2 && permY == Vec::Y1 && permZ == Vec::W2) ||
			(permX == Vec::Y2 && permY == Vec::Z2 && permZ == Vec::Z1) ||
			(permX == Vec::Z2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::Z2 && permY == Vec::Y1 && permZ == Vec::X2) ||
			(permX == Vec::Z2 && permY == Vec::W2 && permZ == Vec::Z1) ||
			(permX == Vec::W2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::W2 && permY == Vec::Y1 && permZ == Vec::Y2) ||
			(permX == Vec::W2 && permY == Vec::X2 && permZ == Vec::Z1) ) // Good when permW == Vec::W1
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::W1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::W2) ||
			(permX == Vec::X1 && permY == Vec::Z2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::Z2 && permZ == Vec::W2) ||
			(permX == Vec::Y2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::Y2 && permY == Vec::Y1 && permZ == Vec::W2) ||
			(permX == Vec::Y2 && permY == Vec::Z2 && permZ == Vec::Z1) ) // Good when permW == Vec::X2
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::X2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::X1 && permY == Vec::X2 && permZ == Vec::Y1) ||
			(permX == Vec::Z1 && permY == Vec::W1 && permZ == Vec::X2) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::X2) ||
			(permX == Vec::X1 && permY == Vec::W2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::W2 && permZ == Vec::X2) ||
			(permX == Vec::Z2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::Z2 && permY == Vec::Y1 && permZ == Vec::X2) ||
			(permX == Vec::Z2 && permY == Vec::W2 && permZ == Vec::Z1) ) // Good when permW == Vec::Y2
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::Y2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::W1 && permY == Vec::X2 && permZ == Vec::Y2) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::Y2) ||
			(permX == Vec::X1 && permY == Vec::X2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::X2 && permZ == Vec::Y2) ||
			(permX == Vec::Z2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::W2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::W2 && permY == Vec::Y1 && permZ == Vec::Y2) ||
			(permX == Vec::W2 && permY == Vec::X2 && permZ == Vec::Z1) ) // Good when permW == Vec::Z2
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::Z2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::Z1 && permY == Vec::Z2 && permZ == Vec::W1) ||
			(permX == Vec::X1 && permY == Vec::Y1 && permZ == Vec::Z2) ||
			(permX == Vec::X1 && permY == Vec::Y2 && permZ == Vec::Z1) ||
			(permX == Vec::X1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::Y1 && permY == Vec::Z1 && permZ == Vec::Z2) ||
			(permX == Vec::Y1 && permY == Vec::Y2 && permZ == Vec::W1) ||
			(permX == Vec::Y1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::Z1 && permY == Vec::W1 && permZ == Vec::X1) ||
			(permX == Vec::Z1 && permY == Vec::W1 && permZ == Vec::Z2) ||
			(permX == Vec::Z1 && permY == Vec::Y2 && permZ == Vec::X1) ||
			(permX == Vec::Z1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::W1 && permY == Vec::Y2 && permZ == Vec::Y1) ||
			(permX == Vec::W1 && permY == Vec::Y2 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::X1 && permZ == Vec::Y1) ||
			(permX == Vec::X2 && permY == Vec::X1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y1 && permZ == Vec::Z1) ||
			(permX == Vec::X2 && permY == Vec::Y1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Z1 && permZ == Vec::W1) ||
			(permX == Vec::X2 && permY == Vec::Z1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::W1 && permZ == Vec::X1) ||
			(permX == Vec::X2 && permY == Vec::W1 && permZ == Vec::Z2) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::X1) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::Y1) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::Z1) ||
			(permX == Vec::X2 && permY == Vec::Y2 && permZ == Vec::W1) ) // Good when permW == Vec::W2
		{
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::W2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}

		// Default case...
		else
		{
			// Vec::X2 chosen arbitrarily!
			return Vec3V( Vec::V4PermuteTwo<permX, permY, permZ, Vec::X2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
	}

	template <u32 permX, u32 permY>
	__forceinline FASTRETURNCHECK(Vec2V_Out) GetFromTwo( Vec2V_In inVector1, Vec2V_In inVector2 )
	{
		return GetFromTwo<permX, permY>( Vec4V(inVector1), Vec4V(inVector2) );
	}

	template <u32 permX, u32 permY>
	__forceinline FASTRETURNCHECK(Vec2V_Out) GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2 )
	{
		return GetFromTwo<permX, permY>( Vec4V(inVector1), Vec4V(inVector2) );
	}

	template <u32 permX, u32 permY>
	__forceinline FASTRETURNCHECK(Vec2V_Out) GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 )
	{
		CompileTimeAssert(	(permX==Vec::X1 || permX==Vec::Y1 || permX==Vec::Z1 || permX==Vec::W1 ||
							permX==Vec::X2 || permX==Vec::Y2 || permX==Vec::Z2 || permX==Vec::W2) &&
							(permY==Vec::X1 || permY==Vec::Y1 || permY==Vec::Z1 || permY==Vec::W1 ||
							permY==Vec::X2 || permY==Vec::Y2 || permY==Vec::Z2 || permY==Vec::W2) );
							// Invalid permute args!

		CompileTimeAssert(	!(permX==Vec::X1 && permY==Vec::Y1) &&
							!(permX==Vec::X2 && permY==Vec::Y2) );
							// This permute does nothing meaningful!

		CompileTimeAssert(	!(permX <= Vec::W1 && permY <= Vec::W1) &&
							!(permX >= Vec::X2 && permY >= Vec::X2)	);
							// Use the single-vector permute, Vec4V::Get<...>()!

		//================================================
		// 1-instruction PS3 PPU and XBox360 permute
		// specializations.
		// (see v4permtwo1instruction_ps3.inl and
		// v4permtwo1instruction_xbox360.inl)
		// (Covers V4Splat*,V4Merge*,vsldoi,vrlimi)
		//================================================

		// __vsldoi() / MergeXY() specializations.

		if(	(permX == Vec::X1 && permY == Vec::X2) ) // Good when permZ == Vec::Y1, permW == Vec::Y2
		{
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::Y1, Vec::Y2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::Z1 && permY == Vec::Z2) ) // Good when permZ == Vec::W1, permW == Vec::W2
		{
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::W1, Vec::W2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::W1 && permY == Vec::X2) ) // Good when permZ == Vec::Y2, permW == Vec::Z2
		{
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::Y2, Vec::Z2>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::X2 && permY == Vec::X1) ) // Good when permZ == Vec::Y2, permW == Vec::Y1
		{
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::Y2, Vec::Y1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::Z2 && permY == Vec::Z1) ) // Good when permZ == Vec::W2, permW == Vec::W1
		{
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::W2, Vec::W1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
		else if(
			(permX == Vec::W2 && permY == Vec::X1) ) // Good when permZ == Vec::Y1, permW == Vec::Z1
		{
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::Y1, Vec::Z1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}

		// Default case...
		else
		{
			// Vec::X1, Vec::Y1 chosen arbitrarily!
			return Vec2V( Vec::V4PermuteTwo<permX, permY, Vec::X1, Vec::Y1>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
		}
	}

#if __XENON || __PS3
	template <u8 byte0,u8 byte1,u8 byte2,u8 byte3,u8 byte4,u8 byte5,u8 byte6,u8 byte7,u8 byte8,u8 byte9,u8 byte10,u8 byte11,u8 byte12,u8 byte13,u8 byte14,u8 byte15>
	__forceinline FASTRETURNCHECK(Vec4V_Out) ByteGetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 )
	{
		return Vec4V( Vec::V4BytePermuteTwo<byte0,byte1,byte2,byte3,byte4,byte5,byte6,byte7,byte8,byte9,byte10,byte11,byte12,byte13,byte14,byte15>( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2, Vec4V_In controlVec )
	{
		return Vec4V( Vec::V4PermuteTwo( inVector1.GetIntrin128(), inVector2.GetIntrin128(), controlVec.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In controlVec )
	{
		return Vec4V( Vec::V4PermuteTwo( inVector1.GetIntrin128(), inVector2.GetIntrin128(), controlVec.GetIntrin128() ) );
	}
#endif // __XENON || __PS3

	__forceinline FASTRETURNCHECK(ScalarV_Out) Dot( QuatV_In inQuat1, QuatV_In inQuat2 )
	{
		return ScalarV( Vec::V4QuatDotV( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Multiply( QuatV_In inQuat1, QuatV_In inQuat2 )
	{
		return QuatV( Vec::V4QuatMultiply( inQuat1.GetIntrin128(), inQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) SlerpNear( ScalarV_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 )
	{
		return QuatV( Vec::V4QuatSlerpNear( t.GetIntrin128(), inNormQuat1.GetIntrin128(), inNormQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) SlerpNear( Vec4V_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 )
	{
		return QuatV( Vec::V4QuatSlerpNear( t.GetIntrin128(), inNormQuat1.GetIntrin128(), inNormQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Slerp( ScalarV_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 )
	{
		return QuatV( Vec::V4QuatSlerp( t.GetIntrin128(), inNormQuat1.GetIntrin128(), inNormQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Slerp( Vec4V_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 )
	{
		return QuatV( Vec::V4QuatSlerp( t.GetIntrin128(), inNormQuat1.GetIntrin128(), inNormQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Nlerp( ScalarV_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 )
	{
		return QuatV( Vec::V4QuatNlerp( t.GetIntrin128(), inNormQuat1.GetIntrin128(), inNormQuat2.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Nlerp( Vec4V_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 )
	{
		return QuatV( Vec::V4QuatNlerp( t.GetIntrin128(), inNormQuat1.GetIntrin128(), inNormQuat2.GetIntrin128() ) );
	}


















	__forceinline void OuterProduct( Mat33V_InOut inoutMat, Vec3V_In a, Vec3V_In b )
	{
		Vec::Vector_4V _bx = Vec::V4SplatX( b.GetIntrin128() );
		Vec::Vector_4V _by = Vec::V4SplatY( b.GetIntrin128() );
		Vec::Vector_4V _bz = Vec::V4SplatZ( b.GetIntrin128() );
		Vec::Vector_4V col0 = Vec::V4Scale( _bx, a.GetIntrin128() );
		Vec::Vector_4V col1 = Vec::V4Scale( _by, a.GetIntrin128() );
		Vec::Vector_4V col2 = Vec::V4Scale( _bz, a.GetIntrin128() );
		inoutMat.SetColsIntrin128( col0, col1, col2 );
	}

	__forceinline void OuterProduct( Mat44V_InOut inoutMat, Vec4V_In a, Vec4V_In b)
	{
		Vec::Vector_4V _bx = Vec::V4SplatX( b.GetIntrin128() );
		Vec::Vector_4V _by = Vec::V4SplatY( b.GetIntrin128() );
		Vec::Vector_4V _bz = Vec::V4SplatZ( b.GetIntrin128() );
		Vec::Vector_4V _bw = Vec::V4SplatW( b.GetIntrin128() );
		Vec::Vector_4V col0 = Vec::V4Scale( _bx, a.GetIntrin128() );
		Vec::Vector_4V col1 = Vec::V4Scale( _by, a.GetIntrin128() );
		Vec::Vector_4V col2 = Vec::V4Scale( _bz, a.GetIntrin128() );
		Vec::Vector_4V col3 = Vec::V4Scale( _bw, a.GetIntrin128() );
		inoutMat.SetColsIntrin128( col0, col1, col2, col3 );
	}

	__forceinline void UnTransformFull( Mat44V_InOut inoutMat, Mat44V_In origTransformMat, Mat44V_In concatMat )
	{
		Imp::UnTransformFull_Imp44( inoutMat, MAT44V_ARG(origTransformMat), MAT44V_ARG(concatMat) );
	}

	__forceinline void UnTransformOrtho( Mat44V_InOut inoutMat, Mat44V_In origOrthoTransformMat, Mat44V_In concatMat )
	{
		Imp::UnTransformOrtho_Imp44( inoutMat, MAT44V_ARG(origOrthoTransformMat), MAT44V_ARG(concatMat) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) UnTransformFull( Mat44V_In origTransformMat, Vec4V_In transformedVect )
	{
		return Vec4V( Imp::UnTransformFull_Imp44( MAT44V_ARG(origTransformMat), transformedVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) UnTransformOrtho( Mat44V_In origOrthoTransformMat, Vec4V_In transformedVect )
	{
		return Vec4V( Imp::Mul_Imp_4_44( transformedVect.GetIntrin128(), MAT44V_ARG(origOrthoTransformMat) ) );
	}

	__forceinline void UnTransformFull( Mat33V_InOut inoutMat, Mat33V_In origTransformMat, Mat33V_In concatMat )
	{
		Imp::UnTransformFull_Imp33( inoutMat, MAT33V_ARG(origTransformMat), MAT33V_ARG(concatMat) );
	}

	__forceinline void UnTransformOrtho( Mat33V_InOut inoutMat, Mat33V_In origOrthoTransformMat, Mat33V_In concatMat )
	{
		Imp::UnTransformOrtho_Imp33( inoutMat, MAT33V_ARG(origOrthoTransformMat), MAT33V_ARG(concatMat) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransformFull( Mat33V_In origTransformMat, Vec3V_In transformedVect )
	{
		return Vec3V( Imp::UnTransformFull_Imp33( MAT33V_ARG(origTransformMat), transformedVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransformOrtho( Mat33V_In origOrthoTransformMat, Vec3V_In transformedVect )
	{
		return Vec3V( Imp::Mul_Imp_3_33( transformedVect.GetIntrin128(), MAT33V_ARG(origOrthoTransformMat) ) );
	}

	__forceinline void Translate( Mat34V_InOut inoutMat, Mat34V_In inMat, Vec3V_In translateAmt )
	{
		inoutMat.SetColsIntrin128(
			inMat.GetCol0Intrin128(),
			inMat.GetCol1Intrin128(),
			inMat.GetCol2Intrin128(),
			Vec::V4Add( inMat.GetCol3Intrin128(), translateAmt.GetIntrin128() )
			);
	}

	__forceinline void Transpose( Mat44V_InOut inoutMat, Mat44V_In mat )
	{
		Imp::Transpose_Imp44( inoutMat, MAT44V_ARG(mat) );
	}

	__forceinline void Transpose( Mat33V_InOut inoutMat, Mat33V_In mat )
	{
		Imp::Transpose_Imp33( inoutMat, MAT33V_ARG(mat) );
	}

	__forceinline void Transpose3x3( Vec3V_InOut dst0, Vec3V_InOut dst1, Vec3V_InOut dst2, Vec3V_In src0, Vec3V_In src1, Vec3V_In src2 )
	{
		Mat33V temp;
		Imp::Transpose_Imp33( temp, src0.GetIntrin128(), src1.GetIntrin128(), src2.GetIntrin128() );
		dst0 = temp.GetCol0();
		dst1 = temp.GetCol1();
		dst2 = temp.GetCol2();
	}

	__forceinline void Transpose3x4to4x3( Vec4V_InOut dst0, Vec4V_InOut dst1, Vec4V_InOut dst2, Vec3V_In src0, Vec3V_In src1, Vec3V_In src2, Vec3V_In src3 )
	{
		Mat44V temp;
		Imp::Transpose_Imp44( temp, src0.GetIntrin128(), src1.GetIntrin128(), src2.GetIntrin128(), src3.GetIntrin128() );
		dst0 = temp.GetCol0();
		dst1 = temp.GetCol1();
		dst2 = temp.GetCol2();
	}

	__forceinline void Transpose4x3to3x4( Vec3V_InOut dst0, Vec3V_InOut dst1, Vec3V_InOut dst2, Vec3V_InOut dst3, Vec4V_In src0, Vec4V_In src1, Vec4V_In src2 )
	{
		Mat44V temp;
		Imp::Transpose_Imp44( temp, src0.GetIntrin128(), src1.GetIntrin128(), src2.GetIntrin128(), src2.GetIntrin128() );
		dst0 = temp.GetCol0().GetXYZ();
		dst1 = temp.GetCol1().GetXYZ();
		dst2 = temp.GetCol2().GetXYZ();
		dst3 = temp.GetCol3().GetXYZ();
	}

	__forceinline void Transpose4x4( Vec4V_InOut dst0, Vec4V_InOut dst1, Vec4V_InOut dst2, Vec4V_InOut dst3, Vec4V_In src0, Vec4V_In src1, Vec4V_In src2, Vec4V_In src3 )
	{
		Mat44V temp;
		Imp::Transpose_Imp44( temp, src0.GetIntrin128(), src1.GetIntrin128(), src2.GetIntrin128(), src3.GetIntrin128() );
		dst0 = temp.GetCol0();
		dst1 = temp.GetCol1();
		dst2 = temp.GetCol2();
		dst3 = temp.GetCol3();
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Determinant( Mat33V_In mat )
	{
		Vec::Vector_4V crossVect = Vec::V3Cross( mat.GetCol0Intrin128(), mat.GetCol1Intrin128() );
		return ScalarV( Vec::V3DotV( crossVect, mat.GetCol2Intrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Determinant3x3( Mat34V_In mat )
	{
		Vec::Vector_4V crossVect = Vec::V3Cross( mat.GetCol0Intrin128(), mat.GetCol1Intrin128() );
		return ScalarV( Vec::V3DotV( crossVect, mat.GetCol2Intrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Determinant3x3( Mat44V_In mat )
	{
		Vec::Vector_4V crossVect = Vec::V3Cross( mat.GetCol0Intrin128(), mat.GetCol1Intrin128() );
		return ScalarV( Vec::V3DotV( crossVect, mat.GetCol2Intrin128() ) );
	}

	__forceinline FASTRETURNCHECK(ScalarV_Out) Determinant( Mat44V_In mat )
	{
		return ScalarV( Imp::DeterminantV_Imp44( MAT44V_ARG(mat) ) );
	}

	__forceinline void AddScaled( Mat33V_InOut inoutMat, Mat33V_In toAddTo, Mat33V_In toScaleThenAdd, Mat33V_In toScaleBy )
	{
		Imp::AddScaled_Imp33( inoutMat, MAT33V_ARG(toAddTo), MAT33V_ARG(toScaleThenAdd), MAT33V_ARG(toScaleBy) );
	}

	__forceinline void AddScaled( Mat33V_InOut inoutMat, Mat33V_In toAddTo, Mat33V_In toScaleThenAdd, Vec3V_In toScaleBy )
	{
		Imp::AddScaled_Imp33( inoutMat, MAT33V_ARG(toAddTo), MAT33V_ARG(toScaleThenAdd), toScaleBy.GetIntrin128() );
	}

	__forceinline void Add( Mat33V_InOut inoutMat, Mat33V_In a, Mat33V_In b )
	{
		Imp::Add_Imp33( inoutMat, MAT33V_ARG(a), MAT33V_ARG(b) );
	}

	__forceinline void Subtract( Mat33V_InOut inoutMat, Mat33V_In a, Mat33V_In b )
	{
		Imp::Subtract_Imp33( inoutMat, MAT33V_ARG(a), MAT33V_ARG(b) );
	}
	
	__forceinline void Abs( Mat33V_InOut inoutMat )
	{
		Imp::Abs_Imp33( inoutMat, MAT33V_ARG(inoutMat) );
	}

	__forceinline void Abs( Mat33V_InOut inoutMat, Mat33V_In a )
	{
		Imp::Abs_Imp33( inoutMat, MAT33V_ARG(a) );
	}

	__forceinline void Scale( Mat33V_InOut inoutMat, Vec3V_In a, Mat33V_In b )
	{
		Imp::Scale_Imp33( inoutMat, MAT33V_ARG(b), a.GetIntrin128() );
	}

	__forceinline void Scale( Mat33V_InOut inoutMat, Mat33V_In a, Vec3V_In b )
	{
		Imp::Scale_Imp33( inoutMat, MAT33V_ARG(a), b.GetIntrin128() );
	}

	__forceinline void InvScale( Mat33V_InOut inoutMat, Mat33V_In toScale, Vec3V_In scaleValue )
	{
		Imp::InvScale_Imp33( inoutMat, MAT33V_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleSafe( Mat33V_InOut inoutMat, Mat33V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect )
	{
		Imp::InvScaleSafe_Imp33( inoutMat, MAT33V_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFast( Mat33V_InOut inoutMat, Mat33V_In toScale, Vec3V_In scaleValue )
	{
		Imp::InvScaleFast_Imp33( inoutMat, MAT33V_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleFastSafe( Mat33V_InOut inoutMat, Mat33V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp33( inoutMat, MAT33V_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void AddScaled( Mat34V_InOut inoutMat, Mat34V_In toAddTo, Mat34V_In toScaleThenAdd, Mat34V_In toScaleBy )
	{
		Mat44V temp;
		Imp::AddScaled_Imp44( temp, MAT44V_ARG(toAddTo), MAT44V_ARG(toScaleThenAdd), MAT44V_ARG(toScaleBy) );
		inoutMat = temp.GetMat34();
	}

	__forceinline void AddScaled( Mat34V_InOut inoutMat, Mat34V_In toAddTo, Mat34V_In toScaleThenAdd, Vec3V_In toScaleBy )
	{
		Mat44V temp;
		Imp::AddScaled_Imp44( temp, MAT44V_ARG(toAddTo), MAT44V_ARG(toScaleThenAdd), toScaleBy.GetIntrin128() );
		inoutMat = temp.GetMat34();
	}

	__forceinline void Add( Mat34V_InOut inoutMat, Mat34V_In a, Mat34V_In b )
	{
		Imp::Add_Imp34( inoutMat, MAT34V_ARG(a), MAT34V_ARG(b) );
	}

	__forceinline void Add3x3( Mat34V_InOut inoutMat, Mat34V_In a, Mat34V_In b )
	{
		Mat33V temp;
		Imp::Add_Imp33( temp, MAT33V_ARG(a), MAT33V_ARG(b) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			a.GetCol3Intrin128()
			);
	}

	__forceinline void AddScaled3x3( Mat34V_InOut inoutMat, Mat34V_In toAddTo, Mat34V_In toScaleThenAdd, Mat34V_In toScaleBy )
	{
		Mat33V temp;
		Imp::AddScaled_Imp33( temp, MAT33V_ARG(toAddTo), MAT33V_ARG(toScaleThenAdd), MAT33V_ARG(toScaleBy) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			toAddTo.GetCol3Intrin128()
			);
	}

	__forceinline void AddScaled3x3( Mat34V_InOut inoutMat, Mat34V_In toAddTo, Mat34V_In toScaleThenAdd, Vec3V_In toScaleBy )
	{
		Mat33V temp;
		Imp::AddScaled_Imp33( temp, MAT33V_ARG(toAddTo), MAT33V_ARG(toScaleThenAdd), toScaleBy.GetIntrin128() );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			toAddTo.GetCol3Intrin128()
			);
	}

	__forceinline void Subtract( Mat34V_InOut inoutMat, Mat34V_In a, Mat34V_In b )
	{
		Imp::Subtract_Imp34( inoutMat, MAT34V_ARG(a), MAT34V_ARG(b) );
	}

	__forceinline void Abs( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Imp::Abs_Imp34( inoutMat, MAT34V_ARG(a) );
	}

	__forceinline void Abs3x3( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Mat33V temp;
		Imp::Abs_Imp33( temp, MAT33V_ARG(a) );
		inoutMat = Mat34V( temp, a.GetCol3() );
	}

	__forceinline void Scale( Mat34V_InOut inoutMat, Vec3V_In a, Mat34V_In b )
	{
		Imp::Scale_Imp34( inoutMat, MAT34V_ARG(b), a.GetIntrin128() );
	}

	__forceinline void Scale( Mat34V_InOut inoutMat, Mat34V_In a, Vec3V_In b )
	{
		Imp::Scale_Imp34( inoutMat, MAT34V_ARG(a), b.GetIntrin128() );
	}

	__forceinline void InvScale( Mat34V_InOut inoutMat, Mat34V_In toScale, Vec3V_In scaleValue )
	{
		Imp::InvScale_Imp34( inoutMat, MAT34V_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleSafe( Mat34V_InOut inoutMat, Mat34V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect )
	{
		Imp::InvScaleSafe_Imp34( inoutMat, MAT34V_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFast( Mat34V_InOut inoutMat, Mat34V_In toScale, Vec3V_In scaleValue )
	{
		Imp::InvScaleFast_Imp34( inoutMat, MAT34V_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleFastSafe( Mat34V_InOut inoutMat, Mat34V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp34( inoutMat, MAT34V_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void AddScaled( Mat44V_InOut inoutMat, Mat44V_In toAddTo, Mat44V_In toScaleThenAdd, Mat44V_In toScaleBy )
	{
		Imp::AddScaled_Imp44( inoutMat, MAT44V_ARG(toAddTo), MAT44V_ARG(toScaleThenAdd), MAT44V_ARG(toScaleBy) );
	}

	__forceinline void AddScaled( Mat44V_InOut inoutMat, Mat44V_In toAddTo, Mat44V_In toScaleThenAdd, Vec4V_In toScaleBy )
	{
		Imp::AddScaled_Imp44( inoutMat, MAT44V_ARG(toAddTo), MAT44V_ARG(toScaleThenAdd), toScaleBy.GetIntrin128() );
	}

	__forceinline void Add( Mat44V_InOut inoutMat, Mat44V_In a, Mat44V_In b )
	{
		Imp::Add_Imp44( inoutMat, MAT44V_ARG(a), MAT44V_ARG(b) );
	}

	__forceinline void AddScaled4x3( Mat44V_InOut inoutMat, Mat44V_In toAddTo, Mat44V_In toScaleThenAdd, Mat44V_In toScaleBy )
	{
		Mat33V temp;
		Imp::AddScaled_Imp33( temp, MAT33V_ARG(toAddTo), MAT33V_ARG(toScaleThenAdd), MAT33V_ARG(toScaleBy) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			toAddTo.GetCol3Intrin128()
			);
	}

	__forceinline void AddScaled4x3( Mat44V_InOut inoutMat, Mat44V_In toAddTo, Mat44V_In toScaleThenAdd, Vec4V_In toScaleBy )
	{
		Mat33V temp;
		Imp::AddScaled_Imp33( temp, MAT33V_ARG(toAddTo), MAT33V_ARG(toScaleThenAdd), toScaleBy.GetIntrin128() );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			toAddTo.GetCol3Intrin128()
			);
	}

	__forceinline void Add4x3( Mat44V_InOut inoutMat, Mat44V_In a, Mat44V_In b )
	{
		Mat33V temp;
		Imp::Add_Imp33( temp, MAT33V_ARG(a), MAT33V_ARG(b) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			a.GetCol3Intrin128()
			);
	}

	__forceinline void Subtract( Mat44V_InOut inoutMat, Mat44V_In a, Mat44V_In b )
	{
		Imp::Subtract_Imp44( inoutMat, MAT44V_ARG(a), MAT44V_ARG(b) );
	}

	__forceinline void Abs( Mat44V_InOut inoutMat, Mat44V_In a )
	{
		Imp::Abs_Imp44( inoutMat, MAT44V_ARG(a) );
	}

	__forceinline void Scale( Mat44V_InOut inoutMat, Vec4V_In a, Mat44V_In b )
	{
		Imp::Scale_Imp44( inoutMat, MAT44V_ARG(b), a.GetIntrin128() );
	}

	__forceinline void Scale( Mat44V_InOut inoutMat, Mat44V_In a, Vec4V_In b )
	{
		Imp::Scale_Imp44( inoutMat, MAT44V_ARG(a), b.GetIntrin128() );
	}

	__forceinline void Scale3x3( Mat34V_InOut inoutMat, Vec3V_In a, Mat34V_In b )
	{
		Imp::Scale_Imp33( inoutMat, MAT34V_ARG(b), a.GetIntrin128() );
	}

	__forceinline void Scale3x3( Mat34V_InOut inoutMat, Mat34V_In a, Vec3V_In b )
	{
		Imp::Scale_Imp33( inoutMat, MAT34V_ARG(a), b.GetIntrin128() );
	}

	__forceinline void Multiply( Mat44V_InOut c, Mat44V_In a, Mat44V_In b )
	{
		Imp::Mul_Imp_44_44( c, MAT44V_ARG(a), MAT44V_ARG(b) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Multiply( Mat44V_In a, Vec4V_In b )
	{
		return Vec4V( Imp::Mul_Imp_44_4( MAT44V_ARG(a), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Multiply( Vec4V_In a, Mat44V_In b )
	{
		return Vec4V( Imp::Mul_Imp_4_44( a.GetIntrin128(), MAT44V_ARG(b) ) );
	}

	__forceinline void Multiply( Mat33V_InOut inoutMat, Mat33V_In a, Mat33V_In b )
	{
		Imp::Mul_Imp_33_33( inoutMat, MAT33V_ARG(a), MAT33V_ARG(b) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Multiply( Mat33V_In a, Vec3V_In b )
	{
		return Vec3V( Imp::Mul_Imp_33_3( MAT33V_ARG(a), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Multiply( Vec3V_In a, Mat33V_In b )
	{
		return Vec3V( Imp::Mul_Imp_3_33( a.GetIntrin128(), MAT33V_ARG(b) ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Multiply( Mat34V_In a, Vec4V_In b )
	{
		return Vec3V( Imp::Mul_Imp_34_4( MAT34V_ARG(a), b.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec4V_Out) Multiply( Vec3V_In a, Mat34V_In b )
	{
		return Vec4V( Imp::Mul_Imp_3_34( a.GetIntrin128(), MAT34V_ARG(b) ) );
	}

	__forceinline unsigned int IsEqualIntAll( TransformV_In inT1, TransformV_In inT2 )
	{
		return IsEqualIntAll( inT1.GetRotation(), inT2.GetRotation()) && IsEqualIntAll( inT1.GetPosition(), inT2.GetPosition() );
	}

	__forceinline void TransformVFromMat34V( TransformV_InOut outT, Mat34V_In inMat )
	{
		outT = TransformV( QuatVFromMat33V( inMat.GetMat33() ), inMat.GetCol3() );
	}

	__forceinline void Mat34VFromTransformV( Mat34V_InOut outMat, TransformV_In inT )
	{
		Mat34VFromQuatV( outMat, inT.GetRotation(), inT.GetPosition() );
	}

	__forceinline void InvertTransform(TransformV_InOut outT, TransformV_In inT )
	{
		QuatV invRot = InvertNormInput( inT.GetRotation() );
		outT = TransformV( invRot, Negate( Transform(invRot, inT.GetPosition() ) ) );
	}

	__forceinline void Transform(TransformV_InOut outT, TransformV_In inT1, TransformV_In inT2 )
	{
		outT = TransformV(	Multiply( inT1.GetRotation(), inT2.GetRotation() ), 
							Transform( inT1.GetRotation(), inT2.GetPosition() ) + inT1.GetPosition() );
	}

	__forceinline void UnTransform(TransformV_InOut outT, TransformV_In a, TransformV_In b )
	{
		QuatV invRot = InvertNormInput( a.GetRotation() );
		outT = TransformV(	Multiply( invRot, b.GetRotation() ), 
							Transform( invRot, b.GetPosition() - a.GetPosition() ) );
	}

	__forceinline void Transform( QuatV_InOut outQ, Vec3V_InOut outT, QuatV_In inQ1, Vec3V_In inT1, QuatV_In inQ2, Vec3V_In inT2)
	{
		outT = Add(inT1, Transform(inQ1, inT2));
		outQ = Multiply(inQ1, inQ2);
	}

	__forceinline void UnTransform( QuatV_InOut outQ, Vec3V_InOut outT, QuatV_In inQ1, Vec3V_In inT1, QuatV_In inQ2, Vec3V_In inT2)
	{
		QuatV invQ1 = InvertNormInput(inQ1);
		outT = Transform(invQ1, Subtract(inT2, inT1));
		outQ = Multiply(invQ1, inQ2);
	}

	__forceinline FASTRETURNCHECK(TransformV_Out) Lerp(ScalarV_In tValue, TransformV_In inT1, TransformV_In inT2 )
	{
		return TransformV(	Nlerp( tValue, inT1.GetRotation(), PrepareSlerp( inT1.GetRotation(), inT2.GetRotation() ) ),
							Lerp( tValue, inT1.GetPosition(), inT2.GetPosition() ) );
	}

	__forceinline FASTRETURNCHECK(TransformV_Out) SelectFT( VecBoolV_In choiceVector, TransformV_In ifFalse, TransformV_In ifTrue )
	{
		return TransformV(	SelectFT( choiceVector, ifFalse.GetRotation(), ifTrue.GetRotation() ),
							SelectFT( choiceVector, ifFalse.GetPosition(), ifTrue.GetPosition() ) );
	}

	__forceinline FASTRETURNCHECK(TransformV_Out) SelectFT( BoolV_In choiceVector, TransformV_In ifFalse, TransformV_In ifTrue )
	{
		return TransformV(	SelectFT( choiceVector, ifFalse.GetRotation(), ifTrue.GetRotation() ), 
							SelectFT( choiceVector, ifFalse.GetPosition(), ifTrue.GetPosition() ) );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) Transform( TransformV_In inT, QuatV_In inQ )
	{
		return Multiply( inT.GetRotation(), inQ );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) UnTransform( TransformV_In inT, QuatV_In inQ )
	{
		return Multiply( InvertNormInput( inT.GetRotation() ), inQ );
	}

	__forceinline FASTRETURNCHECK(QuatV_Out) UnTransform( QuatV_In inQToInvert, QuatV_In inQ )
	{
		return Multiply( InvertNormInput( inQToInvert ), inQ );
	}
	
	__forceinline FASTRETURNCHECK(Vec3V_Out) Transform( TransformV_In inT, Vec3V_In inV )
	{
		return Transform( inT.GetRotation(), inV ) + inT.GetPosition();
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransform( TransformV_In inT, Vec3V_In inV )
	{
		return Transform( InvertNormInput( inT.GetRotation() ), inV - inT.GetPosition() );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Transform3x3( TransformV_In inT, Vec3V_In inV )
	{
		return Transform( inT.GetRotation(), inV );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransform3x3( TransformV_In inT, Vec3V_In inV )
	{
		return Transform( InvertNormInput( inT.GetRotation() ), inV );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Transform( QuatV_In inQ, Vec3V_In inT, Vec3V_In inV )
	{
		return Add(Transform(inQ, inV), inT);
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransform( QuatV_In inQ, Vec3V_In inT, Vec3V_In inV )
	{
		return Transform(InvertNormInput(inQ), Subtract(inV, inT));
	}

	__forceinline void InvScale( Mat44V_InOut inoutMat, Mat44V_In toScale, Vec4V_In scaleValue )
	{
		Imp::InvScale_Imp44( inoutMat, MAT44V_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleSafe( Mat44V_InOut inoutMat, Mat44V_In toScale, Vec4V_In scaleValue, Vec4V_In errValVect )
	{
		Imp::InvScaleSafe_Imp44( inoutMat, MAT44V_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFast( Mat44V_InOut inoutMat, Mat44V_In toScale, Vec4V_In scaleValue )
	{
		Imp::InvScaleFast_Imp44( inoutMat, MAT44V_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleFastSafe( Mat44V_InOut inoutMat, Mat44V_In toScale, Vec4V_In scaleValue, Vec4V_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp44( inoutMat, MAT44V_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvertFull( Mat44V_InOut inoutMat, Mat44V_In a )
	{
		Imp::InvertFull_Imp44( inoutMat, MAT44V_ARG(a) );
	}

	__forceinline void Transpose3x3( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Mat33V temp;
		Imp::Transpose_Imp33( temp, MAT33V_ARG(a) );
		inoutMat.SetColsIntrin128(	temp.GetCol0Intrin128(),
									temp.GetCol1Intrin128(),
									temp.GetCol2Intrin128(),
									a.GetCol3Intrin128()	);
	}

	__forceinline void Invert3x3Full( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Mat33V temp;
		Imp::InvertFull_Imp33( temp, MAT33V_ARG(a) );
		inoutMat.SetColsIntrin128(	temp.GetCol0Intrin128(),
									temp.GetCol1Intrin128(),
									temp.GetCol2Intrin128(),
									a.GetCol3Intrin128()	);
	}

	__forceinline void Invert3x3Ortho( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Transpose3x3( inoutMat, a );
	}

	__forceinline void InvertTransformFull( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Imp::InvertTransformFull_Imp34( inoutMat, MAT34V_ARG(a) );
	}

	__forceinline void InvertTransformOrtho( Mat34V_InOut inoutMat, Mat34V_In a )
	{
		Imp::InvertTransformOrtho_Imp34( inoutMat, MAT34V_ARG(a) );
	}

	__forceinline void Invert3x4Full( Mat44V_InOut inoutMat, Mat44V_In a )
	{
		Mat34V temp;
		Imp::InvertTransformFull_Imp34( temp, MAT34V_ARG(a) );

		Vec::Vector_4V outCol0 = temp.GetCol0Intrin128();
		Vec::Vector_4V outCol1 = temp.GetCol1Intrin128();
		Vec::Vector_4V outCol2 = temp.GetCol2Intrin128();
		Vec::Vector_4V outCol3 = temp.GetCol3Intrin128();

		// Last row = (0,0,0,1.0f)
		Vec::Vector_4V maskxyz = Vec::V4VConstant(V_MASKXYZ);
		outCol0 = Vec::V4And( outCol0, maskxyz );
		outCol1 = Vec::V4And( outCol1, maskxyz );
		outCol2 = Vec::V4And( outCol2, maskxyz );
		outCol3 = Vec::V4And( outCol3, maskxyz );
		outCol3 = Vec::V4Or( outCol3, Vec::V4VConstant(V_ZERO_WONE) );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2, outCol3 );
	}

	__forceinline void Invert3x4Ortho( Mat44V_InOut inoutMat, Mat44V_In a )
	{
		Mat34V temp;
		Imp::InvertTransformOrtho_Imp34( temp, MAT34V_ARG(a) );
		Vec::Vector_4V outVect0 = temp.GetCol0Intrin128();
		Vec::Vector_4V outVect1 = temp.GetCol1Intrin128();
		Vec::Vector_4V outVect2 = temp.GetCol2Intrin128();
		Vec::Vector_4V outVect3 = temp.GetCol3Intrin128();

		// Last row = (0,0,0,1.0f)
		Vec::Vector_4V maskxyz = Vec::V4VConstant(V_MASKXYZ);
		outVect0 = Vec::V4And( outVect0, maskxyz );
		outVect1 = Vec::V4And( outVect1, maskxyz );
		outVect2 = Vec::V4And( outVect2, maskxyz );
		outVect3 = Vec::V4And( outVect3, maskxyz );
		outVect3 = Vec::V4Or( outVect3, Vec::V4VConstant(V_ZERO_WONE) );
		
		inoutMat.SetColsIntrin128( outVect0, outVect1, outVect2, outVect3 );
	}

	__forceinline void InvertFull( Mat33V_InOut inoutMat, Mat33V_In a )
	{
		Imp::InvertFull_Imp33( inoutMat, MAT33V_ARG(a) );
	}

	__forceinline void InvertOrtho( Mat33V_InOut inoutMat, Mat33V_In a )
	{
		Imp::Transpose_Imp33( inoutMat, MAT33V_ARG(a) );
	}

	__forceinline void Transform( Mat34V_InOut inoutMat1, Mat34V_In transformMat2 )
	{
		Imp::Transform_Imp34( inoutMat1, MAT34V_ARG(inoutMat1), MAT34V_ARG(transformMat2) );
	}

	__forceinline void Transform( Mat34V_InOut inoutMat, Mat34V_In transformMat1, Mat34V_In transformMat2 )
	{
		Imp::Transform_Imp34( inoutMat, MAT34V_ARG(transformMat1), MAT34V_ARG(transformMat2) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Transform3x3( Mat34V_In transformMat, Vec3V_In vect )
	{
		return Vec3V( Imp::Mul_Imp_33_3( MAT33V_ARG(transformMat), vect.GetIntrin128() ) );
	}

	__forceinline void Transform3x3( Mat34V_InOut inoutMat, Mat34V_In prependMat, Mat34V_In transformMat )
	{
		Mat33V temp;
		Imp::Mul_Imp_33_33( temp, MAT33V_ARG(prependMat), MAT33V_ARG(transformMat) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			transformMat.GetCol3Intrin128()
			);
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Transform( Mat34V_In transformMat, Vec3V_In point )
	{
		return Vec3V( Imp::Transform_Imp34( MAT34V_ARG(transformMat), point.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransform3x3Full( Mat34V_In origTransformMat, Vec3V_In transformedVect )
	{
		return Vec3V( Imp::UnTransformFull_Imp33( MAT33V_ARG(origTransformMat), transformedVect.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransformFull( Mat34V_In origTransformMat, Vec3V_In transformedPoint )
	{
		return Vec3V( Imp::UnTransformFull_Imp34( MAT34V_ARG(origTransformMat), transformedPoint.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransform3x3Ortho( Mat34V_In origOrthoTransformMat, Vec3V_In transformedVect )
	{
		return Vec3V( Imp::Mul_Imp_3_33( transformedVect.GetIntrin128(), MAT33V_ARG(origOrthoTransformMat) ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransformOrtho( Mat34V_In origOrthoTransformMat, Vec3V_In transformedPoint )
	{
		return Vec3V( Imp::UnTransformOrtho_Imp34( MAT34V_ARG(origOrthoTransformMat), transformedPoint.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) Transform( QuatV_In unitQuat, Vec3V_In inVect )
	{
		return Vec3V( Vec::V3QuatRotate( inVect.GetIntrin128(), unitQuat.GetIntrin128() ) );
	}

	__forceinline FASTRETURNCHECK(Vec3V_Out) UnTransform( QuatV_In unitQuat, Vec3V_In inVect )
	{
		return Vec3V( Vec::V3QuatRotateReverse( inVect.GetIntrin128(), unitQuat.GetIntrin128() ) );
	}

	__forceinline void UnTransform3x3Full( Mat34V_InOut inoutMat, Mat34V_In origTransformMat, Mat34V_In concatMat )
	{
		Mat33V temp;
		Imp::UnTransformFull_Imp33( temp, MAT33V_ARG(origTransformMat), MAT33V_ARG(concatMat) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			concatMat.GetCol3Intrin128()
			);
	}

	__forceinline void UnTransform3x3Ortho( Mat34V_InOut inoutMat, Mat34V_In origOrthoTransformMat, Mat34V_In concatMat )
	{
		Mat33V temp;
		Imp::UnTransformOrtho_Imp33( temp, MAT33V_ARG(origOrthoTransformMat), MAT33V_ARG(concatMat) );
		inoutMat.SetColsIntrin128(
			temp.GetCol0Intrin128(),
			temp.GetCol1Intrin128(),
			temp.GetCol2Intrin128(),
			concatMat.GetCol3Intrin128()
			);
	}

	__forceinline void UnTransformFull( Mat34V_InOut inoutMat, Mat34V_In origTransformMat, Mat34V_In concatMat )
	{
		Imp::UnTransformFull_Imp34( inoutMat, MAT34V_ARG(origTransformMat), MAT34V_ARG(concatMat) );
	}

	__forceinline void UnTransformOrtho( Mat34V_InOut inoutMat, Mat34V_In origOrthoTransformMat, Mat34V_In concatMat )
	{
		Imp::UnTransformOrtho_Imp34( inoutMat, MAT34V_ARG(origOrthoTransformMat), MAT34V_ARG(concatMat) );
	}

	__forceinline void ReOrthonormalize( Mat33V_InOut inoutMat, Mat33V_In inMat )
	{
		Imp::ReOrthonormalize_Imp33( inoutMat, MAT33V_ARG( inMat ) );
	}

	__forceinline void ReOrthonormalize3x3( Mat34V_InOut inoutMat, Mat34V_In inMat )
	{
		Mat33V tempMat;
		Imp::ReOrthonormalize_Imp33( tempMat, MAT33V_ARG( inMat ) );
		inoutMat = Mat34V(
				tempMat.GetCol0Intrin128(),
				tempMat.GetCol1Intrin128(),
				tempMat.GetCol2Intrin128(),
				inMat.GetCol3Intrin128()
			);
	}

	__forceinline void ReOrthonormalize3x3( Mat44V_InOut inoutMat, Mat44V_In inMat )
	{
		Mat33V tempMat;
		Imp::ReOrthonormalize_Imp33( tempMat, MAT33V_ARG( inMat ) );
		inoutMat = Mat44V(
				tempMat.GetCol0Intrin128(),
				tempMat.GetCol1Intrin128(),
				tempMat.GetCol2Intrin128(),
				inMat.GetCol3Intrin128()
			);
	}

	// Updates everything but the position.
	__forceinline void LookDown(Mat34V_InOut mtx,Vec3V_In dir, Vec3V_In up)
	{
		Vec3V c = Normalize(dir);
		Vec3V a = Normalize(Cross(up, c));
		Vec3V b = Cross(c, a);
		mtx.SetCol0(a);
		mtx.SetCol1(b);
		mtx.SetCol2(c);
	}

	__forceinline void LookAt(Mat34V_InOut mtx,Vec3V_In from,Vec3V_In to,Vec3V_In up)
	{
		mtx.SetCol3(from);
		LookDown(mtx,to - from,up);
	}




















namespace Imp
{
	// The _Imp() functions are provided to hide the ugly syntax (macro surrounding a Mat*V argument) that is necessary to help pass via vector registers.
	// The non-_Imp() functions that call the _Imp() functions are very short and are __forceinline'd so that there is no param passing on the stack at all,
	// EVER! (except when the # of arguments exceeds the platform's register passing limits... see README.txt)

	//================================================
	// For private use only... implementations
	//================================================

	inline Vec::Vector_4V_Out QuatFromMat33V_Imp33( MAT33V_DECL(mat) )
	{
		// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm

		Mat33V inMat = MAT33V_ARG_GET(mat);

		Vec::Vector_4V inCol0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V maskX = Vec::V4VConstant(V_MASKX);
		Vec::Vector_4V maskY = Vec::V4VConstant(V_MASKY);
		Vec::Vector_4V maskZ = Vec::V4VConstant(V_MASKZ);
		Vec::Vector_4V maskW = Vec::V4VConstant(V_MASKW);

		Vec::Vector_4V quatResult;
		Vec::Vector_4V diagonalSum;
		Vec::Vector_4V diagonalDifference;

		Vec::Vector_4V _xxxx, _yyyy, _zzzz;
		Vec::Vector_4V xx_yy;
		Vec::Vector_4V zy_xz_yx;
		Vec::Vector_4V yz_zx_xy;
		Vec::Vector_4V xx_yy_zz_xx;
		Vec::Vector_4V yy_zz_xx_yy;
		Vec::Vector_4V zz_xx_yy_zz;
		Vec::Vector_4V rad, invSq, prod;
		Vec::Vector_4V tempSum, tempDiff;		
		Vec::Vector_4V tempResult0, tempResult1, tempResult2, tempResult3;

		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		xx_yy = Vec::V4SelectFT( maskY, inCol0, inCol1 );
		xx_yy_zz_xx = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::Z2,Vec::X1>( xx_yy, inCol2 );
		yy_zz_xx_yy = Vec::V4PermuteTwo<Vec::Y1,Vec::Z2,Vec::X1,Vec::Y1>( xx_yy, inCol2 );
		zz_xx_yy_zz = Vec::V4PermuteTwo<Vec::Z2,Vec::X1,Vec::Y1,Vec::Z2>( xx_yy, inCol2 );

		diagonalSum = Vec::V4Add( Vec::V4Add( xx_yy_zz_xx, yy_zz_xx_yy ), zz_xx_yy_zz );
		diagonalDifference = Vec::V4Subtract( Vec::V4Subtract( xx_yy_zz_xx, yy_zz_xx_yy ), zz_xx_yy_zz );
		rad = Vec::V4Add( Vec::V4SelectFT( maskW, diagonalDifference, diagonalSum ), Vec::V4VConstant(V_ONE) );
		invSq = Vec::V4InvSqrt( rad );

		zy_xz_yx = Vec::V4SelectFT( maskZ, inCol0, inCol1 );
		zy_xz_yx = Vec::V4PermuteTwo<Vec::Z1,Vec::X2,Vec::Y1,Vec::X1>( zy_xz_yx, inCol2 );
		yz_zx_xy = Vec::V4SelectFT( maskX, inCol0, inCol1 );
		yz_zx_xy = Vec::V4PermuteTwo<Vec::Y2,Vec::Z1,Vec::X1,Vec::X1>( yz_zx_xy, inCol2 );

		tempSum = Vec::V4Add( zy_xz_yx, yz_zx_xy );
		tempDiff = Vec::V4Subtract( zy_xz_yx, yz_zx_xy );

		prod = Vec::V4Scale( invSq, Vec::V4VConstant(V_HALF) );

		tempResult0 = Vec::V4PermuteTwo<Vec::X1,Vec::Z1,Vec::Y1,Vec::X2>( tempSum, tempDiff );
		tempResult1 = Vec::V4PermuteTwo<Vec::Z1,Vec::X1,Vec::X1,Vec::Y2>( tempSum, tempDiff );
		tempResult2 = Vec::V4PermuteTwo<Vec::Y1,Vec::X1,Vec::X1,Vec::Z2>( tempSum, tempDiff );
		tempResult3 = tempDiff;

		tempResult0 = Vec::V4SelectFT( maskX, tempResult0, rad );
		tempResult1 = Vec::V4SelectFT( maskY, tempResult1, rad );
		tempResult2 = Vec::V4SelectFT( maskZ, tempResult2, rad );
		tempResult3 = Vec::V4SelectFT( maskW, tempResult3, rad );

		tempResult0 = Vec::V4Scale( tempResult0, Vec::V4SplatX( prod ) );
		tempResult1 = Vec::V4Scale( tempResult1, Vec::V4SplatY( prod ) );
		tempResult2 = Vec::V4Scale( tempResult2, Vec::V4SplatZ( prod ) );
		tempResult3 = Vec::V4Scale( tempResult3, Vec::V4SplatW( prod ) );

		_xxxx = Vec::V4SplatX( inCol0 );
		_yyyy = Vec::V4SplatY( inCol1 );
		_zzzz = Vec::V4SplatZ( inCol2 );

		quatResult = Vec::V4SelectFT( Vec::V4IsGreaterThanV( _yyyy, _xxxx ), tempResult0, tempResult1 );
		quatResult = Vec::V4SelectFT( Vec::V4And( Vec::V4IsGreaterThanV( _zzzz, _xxxx ), Vec::V4IsGreaterThanV( _zzzz, _yyyy ) ), quatResult, tempResult2 );
		quatResult = Vec::V4SelectFT( Vec::V4IsGreaterThanV( Vec::V4SplatX( diagonalSum ), _zero ), quatResult, tempResult3 );

		return quatResult;
	}

	inline Vec::Vector_4V_Out QuatFromMat33VOrtho_Imp33( MAT33V_DECL(mat) )
	{
		// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm

		Mat33V inMat = MAT33V_ARG_GET(mat);

		Vec::Vector_4V inCol0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMat.GetCol2Intrin128();

		Vec::Vector_4V outQuat;
		Vec::Vector_4V _m00, _m11, _m22;
		Vec::Vector_4V temp;
		Vec::Vector_4V qw, fourTimesQW;
		Vec::Vector_4V _m01_m02_m11_m12, _m02_m11_m12_m01, _m12_m20_m01, _m10_m10_m10_m10, _m21_m10, _m21_m02_m10;

		_m00 = Vec::V4SplatX( inCol0 );
		_m11 = Vec::V4SplatY( inCol1 );
		_m22 = Vec::V4SplatZ( inCol2 );
		temp = Vec::V4Add( _m00, Vec::V4Add( _m11, Vec::V4Add( _m22, Vec::V4VConstant(V_ONE) ) ) );
		temp = Vec::V4Sqrt( temp );

		qw = Vec::V4Scale( temp, Vec::V4VConstant(V_HALF) );
		fourTimesQW = Vec::V4Scale( temp, Vec::V4VConstant(V_TWO) );

		_m01_m02_m11_m12 = Vec::V4MergeXY( inCol1, inCol2 );
		_m02_m11_m12_m01 = Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( _m01_m02_m11_m12 ); // 1 instruction on PS3/XBox360
		_m12_m20_m01 = Vec::V4MergeZW( _m02_m11_m12_m01, inCol0 );

		_m10_m10_m10_m10 = Vec::V4SplatY( inCol0 );
		_m21_m10 = Vec::V4MergeZW( inCol1, _m10_m10_m10_m10 );
		_m21_m02_m10 = Vec::V4MergeXY( _m21_m10, inCol2 );

		outQuat = Vec::V4Subtract( _m21_m02_m10, _m12_m20_m01 );
		outQuat = Vec::V4InvScale( outQuat, fourTimesQW );
		outQuat = Vec::V4PermuteTwo<Vec::W2,Vec::X1,Vec::Y1,Vec::Z1>( outQuat, qw ); // 1 instruction on PS3/XBox360
		outQuat = Vec::V4Permute<Vec::Y,Vec::Z,Vec::W,Vec::X>( outQuat ); // 1 instruction on PS3/XBox360

		return outQuat;		
	}

	__forceinline void Transpose_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(mat) )
	{
		Mat44V inMat = MAT44V_ARG_GET(mat);

		Vec::Vector_4V inCol0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMat.GetCol2Intrin128();
		Vec::Vector_4V inCol3 = inMat.GetCol3Intrin128();

		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( inCol0, inCol2 );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( inCol1, inCol3 );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( inCol0, inCol2 );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( inCol1, inCol3 );

		inoutMat.SetColsIntrin128(	Vec::V4MergeXY( tempVect0, tempVect1 ),
									Vec::V4MergeZW( tempVect0, tempVect1 ),
									Vec::V4MergeXY( tempVect2, tempVect3 ),
									Vec::V4MergeZW( tempVect2, tempVect3 )	);
	}

	inline void Transpose_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(mat) )
	{
		Mat33V inMat = MAT33V_ARG_GET(mat);

		Vec::Vector_4V inCol0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMat.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		inoutMat.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
		inoutMat.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE
	}

	inline Vec::Vector_4V_Out DeterminantV_Imp44( MAT44V_DECL(mat) )
	{
		Mat44V inMat = MAT44V_ARG_GET(mat);

#if __XENON && UNIQUE_VECTORIZED_TYPE
		// This is basically XMVECTOR's, with a transpose at the beginning... can possibly do better. (But the #else is NOT better for Xenon.)
		
		Vec::Vector_4V tempVec0, tempVec1, tempVec2, tempVec3, tempVec4, tempVec5;
		Vec::Vector_4V tempProd0, tempProd1, tempProd2, d2, d1;
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO); 
		Vec::Vector_4V _mask = Vec::V4VConstant(V_MASKXYZW);
		Mat44V inMatTranspose;

		// Find transpose.
		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( inMat.GetCol0Intrin128(), inMat.GetCol2Intrin128() );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( inMat.GetCol1Intrin128(), inMat.GetCol3Intrin128() );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( inMat.GetCol0Intrin128(), inMat.GetCol2Intrin128() );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( inMat.GetCol1Intrin128(), inMat.GetCol3Intrin128() );
		inMatTranspose.SetColsIntrin128(	Vec::V4MergeXY( tempVect0, tempVect1 ),
											Vec::V4MergeZW( tempVect0, tempVect1 ),
											Vec::V4MergeXY( tempVect2, tempVect3 ),
											Vec::V4MergeZW( tempVect2, tempVect3 )	);

		// Find determinant.
		tempVec0 = Vec::V4Permute<Vec::Y,Vec::X,Vec::X,Vec::X>(inMatTranspose.GetCol2Intrin128());
		tempVec1 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::Y,Vec::Y>(inMatTranspose.GetCol3Intrin128());
		tempVec2 = Vec::V4Permute<Vec::Y,Vec::X,Vec::X,Vec::X>(inMatTranspose.GetCol2Intrin128());
		tempVec3 = Vec::V4Permute<Vec::W,Vec::W,Vec::W,Vec::Z>(inMatTranspose.GetCol3Intrin128());
		tempVec4 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::Y,Vec::Y>(inMatTranspose.GetCol2Intrin128());
		tempVec5 = Vec::V4Permute<Vec::W,Vec::W,Vec::W,Vec::Z>(inMatTranspose.GetCol3Intrin128());

		tempProd0 = Vec::V4Scale(tempVec0, tempVec1);
		tempProd1 = Vec::V4Scale(tempVec2, tempVec3);
		tempProd2 = Vec::V4Scale(tempVec4, tempVec5);

		tempVec0 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::Y,Vec::Y>(inMatTranspose.GetCol2Intrin128());
		tempVec1 = Vec::V4Permute<Vec::Y,Vec::X,Vec::X,Vec::X>(inMatTranspose.GetCol3Intrin128());
		tempVec2 = Vec::V4Permute<Vec::W,Vec::W,Vec::W,Vec::Z>(inMatTranspose.GetCol2Intrin128());
		tempVec3 = Vec::V4Permute<Vec::Y,Vec::X,Vec::X,Vec::X>(inMatTranspose.GetCol3Intrin128());
		tempVec4 = Vec::V4Permute<Vec::W,Vec::W,Vec::W,Vec::Z>(inMatTranspose.GetCol2Intrin128());
		tempVec5 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::Y,Vec::Y>(inMatTranspose.GetCol3Intrin128());

		tempProd0 = Vec::V4SubtractScaled(tempProd0, tempVec1, tempVec0);
		tempProd1 = Vec::V4SubtractScaled(tempProd1, tempVec3, tempVec2);
		tempProd2 = Vec::V4SubtractScaled(tempProd2, tempVec5, tempVec4);

		tempVec0 = Vec::V4Permute<Vec::W,Vec::W,Vec::W,Vec::Z>(inMatTranspose.GetCol1Intrin128());
		tempVec1 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::Y,Vec::Y>(inMatTranspose.GetCol1Intrin128());
		tempVec2 = Vec::V4Permute<Vec::Y,Vec::X,Vec::X,Vec::X>(inMatTranspose.GetCol1Intrin128());

		d2			= Vec::V4Scale(tempVec0, tempProd0);
		d2			= Vec::V4SubtractScaled(d2, tempProd1, tempVec1);
		_mask		= __vslw(_mask, _mask);
		_mask		= Vec::V4MergeXY(_zero, _mask);
		d2			= Vec::V4AddScaled(d2, tempVec2, tempProd2);
		d1			= Vec::V4Xor(inMatTranspose.GetCol0Intrin128(), _mask);

		return Vec::V4DotV(d1, d2);

#else
		Vec::Vector_4V inVect0, inVect1, inVect2, inVect3;
		Vec::Vector_4V permutedVect0, permutedVect1, permutedVect2, permutedVect3;
		Vec::Vector_4V cofactorVect;
		Vec::Vector_4V psuedoTrans0, psuedoTrans1, psuedoTrans2, psuedoTrans3;
		Vec::Vector_4V _12, _23;
		Vec::Vector_4V _1rotated, _2rotated;
		Vec::Vector_4V _12rotated, _23rotated;
		Vec::Vector_4V _1rotated3, _1rotated3rotated;
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V tempVect;

		inVect0 = inMat.GetCol0Intrin128();
		inVect1 = inMat.GetCol1Intrin128();
		inVect2 = inMat.GetCol2Intrin128();
		inVect3 = inMat.GetCol3Intrin128();

		permutedVect0		= Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inVect0, inVect1);
		permutedVect1		= Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inVect2, inVect3);
		permutedVect2		= Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inVect0, inVect1);
		permutedVect3		= Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inVect2, inVect3);
		psuedoTrans0		= Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVect0, permutedVect1);
		psuedoTrans1		= Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVect3, permutedVect2);
		psuedoTrans2		= Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVect0, permutedVect1);
		psuedoTrans3		= Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVect3, permutedVect2);

		_23					= Vec::V4Scale(psuedoTrans2, psuedoTrans3);
		_23					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_23);
		cofactorVect		= Vec::V4SubtractScaled(_zero, _23, psuedoTrans1);
		_23rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_23);
		cofactorVect		= Vec::V4AddScaled(cofactorVect, psuedoTrans1, _23rotated);
		_12					= Vec::V4Scale(psuedoTrans1, psuedoTrans2);
		_12					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_12);
		cofactorVect		= Vec::V4AddScaled(cofactorVect, psuedoTrans3, _12);
		_12rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_12);
		cofactorVect		= Vec::V4SubtractScaled(cofactorVect, _12rotated, psuedoTrans3);
		_1rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(psuedoTrans1);
		_2rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(psuedoTrans2);
		_1rotated3			= Vec::V4Scale(_1rotated, psuedoTrans3);
		_1rotated3			= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_1rotated3);
		cofactorVect		= Vec::V4AddScaled(cofactorVect, _2rotated, _1rotated3);
		_1rotated3rotated	= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1rotated3);
		cofactorVect		= Vec::V4SubtractScaled(cofactorVect, _1rotated3rotated, _2rotated);
		tempVect			= Vec::V4DotV(psuedoTrans0, cofactorVect);

		return tempVect;
#endif
	}

	inline void Add_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(a), MAT44V_DECL2(b) )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);
		Mat44V inMatB = MAT44V_ARG_GET(b);

		inoutMat = inMatA + inMatB;
	}

	inline void Subtract_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(a), MAT44V_DECL2(b) )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);
		Mat44V inMatB = MAT44V_ARG_GET(b);

		inoutMat = inMatA - inMatB;
	}

	inline void Abs_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(a) )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);
		Vec::Vector_4V invAbsMask = Vec::V4VConstant(V_80000000);

		inoutMat.SetColsIntrin128(	Vec::V4Andc(inMatA.GetCol0Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol1Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol2Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol3Intrin128(), invAbsMask)	);
	}

	inline void Scale_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);

		inoutMat.SetColsIntrin128(	Vec::V4Scale(b, inMatA.GetCol0Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol1Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol2Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol3Intrin128())	);
	}

	inline void Mul_Imp_44_44( Mat44V_InOut c, MAT44V_DECL(a), MAT44V_DECL2(b) )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);
		Mat44V inMatB = MAT44V_ARG_GET(b);

#if __XENON
		Mat44V transposedA;
		Vec::Vector_4V inCol0 = inMatA.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatA.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatA.GetCol2Intrin128();
		Vec::Vector_4V inCol3 = inMatA.GetCol3Intrin128();
		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( inCol0, inCol2 );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( inCol1, inCol3 );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( inCol0, inCol2 );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( inCol1, inCol3 );
		transposedA.SetColsIntrin128(	Vec::V4MergeXY( tempVect0, tempVect1 ),
										Vec::V4MergeZW( tempVect0, tempVect1 ),
										Vec::V4MergeXY( tempVect2, tempVect3 ),
										Vec::V4MergeZW( tempVect2, tempVect3 )	);

		Vec::Vector_4V m00 = Vec::V4DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol0Intrin128() );
		Vec::Vector_4V m10 = Vec::V4DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol0Intrin128() );
		Vec::Vector_4V m20 = Vec::V4DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol0Intrin128() );
		Vec::Vector_4V m30 = Vec::V4DotV( transposedA.GetCol3Intrin128(), inMatB.GetCol0Intrin128() );
		Vec4V firstcol = Vec4V( ScalarV(m00), ScalarV(m10), ScalarV(m20), ScalarV(m30) );

		Vec::Vector_4V m01 = Vec::V4DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol1Intrin128() );
		Vec::Vector_4V m11 = Vec::V4DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol1Intrin128() );
		Vec::Vector_4V m21 = Vec::V4DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol1Intrin128() );
		Vec::Vector_4V m31 = Vec::V4DotV( transposedA.GetCol3Intrin128(), inMatB.GetCol1Intrin128() );
		Vec4V secondcol = Vec4V( ScalarV(m01), ScalarV(m11), ScalarV(m21), ScalarV(m31) );

		Vec::Vector_4V m02 = Vec::V4DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol2Intrin128() );
		Vec::Vector_4V m12 = Vec::V4DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol2Intrin128() );
		Vec::Vector_4V m22 = Vec::V4DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol2Intrin128() );
		Vec::Vector_4V m32 = Vec::V4DotV( transposedA.GetCol3Intrin128(), inMatB.GetCol2Intrin128() );
		Vec4V thirdcol = Vec4V( ScalarV(m02), ScalarV(m12), ScalarV(m22), ScalarV(m32) );

		Vec::Vector_4V m03 = Vec::V4DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol3Intrin128() );
		Vec::Vector_4V m13 = Vec::V4DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol3Intrin128() );
		Vec::Vector_4V m23 = Vec::V4DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol3Intrin128() );
		Vec::Vector_4V m33 = Vec::V4DotV( transposedA.GetCol3Intrin128(), inMatB.GetCol3Intrin128() );
		Vec4V fourthcol = Vec4V( ScalarV(m03), ScalarV(m13), ScalarV(m23), ScalarV(m33) );

		c = Mat44V( firstcol, secondcol, thirdcol, fourthcol );
#else
		//c = Mat44V(	Multiply( inMatA, inMatB.GetCol0() ),
		//				Multiply( inMatA, inMatB.GetCol1() ),
		//				Multiply( inMatA, inMatB.GetCol2() ),
		//				Multiply( inMatA, inMatB.GetCol3() )	);

		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _wwww0 = Vec::V4SplatW( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx0, inMatA.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _yyyy0, inMatA.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz0, inMatA.GetCol2Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _wwww0, inMatA.GetCol3Intrin128() );
		Vec::Vector_4V result0 = Vec::V4Add( sumTerm0, sumTerm1 );

		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _wwww1 = Vec::V4SplatW( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V sumTerm2 = Vec::V4Scale( _xxxx1, inMatA.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm3 = Vec::V4Scale( _yyyy1, inMatA.GetCol1Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _zzzz1, inMatA.GetCol2Intrin128() );
		sumTerm3 = Vec::V4AddScaled( sumTerm3, _wwww1, inMatA.GetCol3Intrin128() );
		Vec::Vector_4V result1 = Vec::V4Add( sumTerm2, sumTerm3 );

		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V _wwww2 = Vec::V4SplatW( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V sumTerm4 = Vec::V4Scale( _xxxx2, inMatA.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm5 = Vec::V4Scale( _yyyy2, inMatA.GetCol1Intrin128() );
		sumTerm4 = Vec::V4AddScaled( sumTerm4, _zzzz2, inMatA.GetCol2Intrin128() );
		sumTerm5 = Vec::V4AddScaled( sumTerm5, _wwww2, inMatA.GetCol3Intrin128() );
		Vec::Vector_4V result2 = Vec::V4Add( sumTerm4, sumTerm5 );

		Vec::Vector_4V _xxxx3 = Vec::V4SplatX( inMatB.GetCol3Intrin128() );
		Vec::Vector_4V _yyyy3 = Vec::V4SplatY( inMatB.GetCol3Intrin128() );
		Vec::Vector_4V _zzzz3 = Vec::V4SplatZ( inMatB.GetCol3Intrin128() );
		Vec::Vector_4V _wwww3 = Vec::V4SplatW( inMatB.GetCol3Intrin128() );
		Vec::Vector_4V sumTerm6 = Vec::V4Scale( _xxxx3, inMatA.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm7 = Vec::V4Scale( _yyyy3, inMatA.GetCol1Intrin128() );
		sumTerm6 = Vec::V4AddScaled( sumTerm6, _zzzz3, inMatA.GetCol2Intrin128() );
		sumTerm7 = Vec::V4AddScaled( sumTerm7, _wwww3, inMatA.GetCol3Intrin128() );
		Vec::Vector_4V result3 = Vec::V4Add( sumTerm6, sumTerm7 );

		c.SetColsIntrin128( result0, result1, result2, result3 );
#endif
	}

	inline Vec::Vector_4V_Out Mul_Imp_44_4( MAT44V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);

#if __XENON
		Mat44V transposedA;
		Vec::Vector_4V inCol0 = inMatA.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatA.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatA.GetCol2Intrin128();
		Vec::Vector_4V inCol3 = inMatA.GetCol3Intrin128();
		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( inCol0, inCol2 );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( inCol1, inCol3 );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( inCol0, inCol2 );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( inCol1, inCol3 );
		transposedA.SetColsIntrin128(	Vec::V4MergeXY( tempVect0, tempVect1 ),
										Vec::V4MergeZW( tempVect0, tempVect1 ),
										Vec::V4MergeXY( tempVect2, tempVect3 ),
										Vec::V4MergeZW( tempVect2, tempVect3 )	);

		Vec::Vector_4V _xxxx = Vec::V4DotV( transposedA.GetCol0Intrin128(), b );
		Vec::Vector_4V _yyyy = Vec::V4DotV( transposedA.GetCol1Intrin128(), b );
		Vec::Vector_4V _zzzz = Vec::V4DotV( transposedA.GetCol2Intrin128(), b );
		Vec::Vector_4V _wwww = Vec::V4DotV( transposedA.GetCol3Intrin128(), b );
		return Vec4V( ScalarV(_xxxx), ScalarV(_yyyy), ScalarV(_zzzz), ScalarV(_wwww) ).GetIntrin128();
#else
		Vec::Vector_4V _xxxx = Vec::V4SplatX( b );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( b );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( b );
		Vec::Vector_4V _wwww = Vec::V4SplatW( b );

		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx, inMatA.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _yyyy, inMatA.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz, inMatA.GetCol2Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _wwww, inMatA.GetCol3Intrin128() );
		Vec::Vector_4V outVec = Vec::V4Add( sumTerm0, sumTerm1 );
		//Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, inMatA.GetCol0Intrin128() );
		//sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, inMatA.GetCol1Intrin128() );
		//sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, inMatA.GetCol2Intrin128() );
		//sumTerm = Vec::V4AddScaled( sumTerm, _wwww, inMatA.GetCol3Intrin128() );

		return outVec;
#endif
	}

	inline Vec::Vector_4V_Out Mul_Imp_4_44( Vec::Vector_4V_In a, MAT44V_DECL3(b) )
	{
		Mat44V inMatB = MAT44V_ARG_GET(b);

#if __XENON // go the dot product route
		Vec::Vector_4V _x = Vec::V4DotV( a, inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _y = Vec::V4DotV( a, inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _z = Vec::V4DotV( a, inMatB.GetCol2Intrin128() );
		Vec::Vector_4V _w = Vec::V4DotV( a, inMatB.GetCol3Intrin128() );
		return Vec4V( ScalarV(_x), ScalarV(_y), ScalarV(_z), ScalarV(_w) ).GetIntrin128();
#else
		Vec::Vector_4V inCol0 = inMatB.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatB.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatB.GetCol2Intrin128();
		Vec::Vector_4V inCol3 = inMatB.GetCol3Intrin128();

		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( inCol0, inCol2 );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( inCol1, inCol3 );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( inCol0, inCol2 );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( inCol1, inCol3 );
		Mat44V transB;
		transB.SetColsIntrin128(	Vec::V4MergeXY( tempVect0, tempVect1 ),
									Vec::V4MergeZW( tempVect0, tempVect1 ),
									Vec::V4MergeXY( tempVect2, tempVect3 ),
									Vec::V4MergeZW( tempVect2, tempVect3 )	);

		Vec::Vector_4V _xxxx = Vec::V4SplatX( a );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( a );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( a );
		Vec::Vector_4V _wwww = Vec::V4SplatW( a );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx, transB.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _yyyy, transB.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz, transB.GetCol2Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _wwww, transB.GetCol3Intrin128() );
		Vec::Vector_4V outVec = Vec::V4Add( sumTerm0, sumTerm1 );

		return outVec;

		//Transpose( transB, inMatB );
		//return Multiply( transB, a );
#endif
	}

	inline void Mul_Imp_33_33( Mat33V_InOut inoutMat, MAT33V_DECL(a), MAT33V_DECL2(b) )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);
		Mat33V inMatB = MAT33V_ARG_GET(b);

		// The #else is actually faster on Xenon.
#if 0//__XENON
		/*
		// Find transpose.
		Mat33V transposedA;
		Vec::Vector_4V inCol0 = inMatA.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatA.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatA.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Do dot products.
		Vec::Vector_4V m00 = Vec::V3DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol0Intrin128() );
		Vec::Vector_4V m10 = Vec::V3DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol0Intrin128() );
		Vec::Vector_4V m20 = Vec::V3DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol0Intrin128() );
		Vec3V firstcol = Vec3V( ScalarV(m00), ScalarV(m10), ScalarV(m20) );

		Vec::Vector_4V m01 = Vec::V3DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol1Intrin128() );
		Vec::Vector_4V m11 = Vec::V3DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol1Intrin128() );
		Vec::Vector_4V m21 = Vec::V3DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol1Intrin128() );
		Vec3V secondcol = Vec3V( ScalarV(m01), ScalarV(m11), ScalarV(m21) );

		Vec::Vector_4V m02 = Vec::V3DotV( transposedA.GetCol0Intrin128(), inMatB.GetCol2Intrin128() );
		Vec::Vector_4V m12 = Vec::V3DotV( transposedA.GetCol1Intrin128(), inMatB.GetCol2Intrin128() );
		Vec::Vector_4V m22 = Vec::V3DotV( transposedA.GetCol2Intrin128(), inMatB.GetCol2Intrin128() );
		Vec3V thirdcol = Vec3V( ScalarV(m02), ScalarV(m12), ScalarV(m22) );

		inoutMat = Mat33V( firstcol, secondcol, thirdcol );
		*/
#else
		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( inMatB.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx0, inMatA.GetCol0Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _yyyy0, inMatA.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz0, inMatA.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( inMatB.GetCol1Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _xxxx1, inMatA.GetCol0Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _yyyy1, inMatA.GetCol1Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _zzzz1, inMatA.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( inMatB.GetCol2Intrin128() );
		Vec::Vector_4V sumTerm2 = Vec::V4Scale( _xxxx2, inMatA.GetCol0Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _yyyy2, inMatA.GetCol1Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _zzzz2, inMatA.GetCol2Intrin128() );

		inoutMat.SetColsIntrin128( sumTerm0, sumTerm1, sumTerm2 );
#endif

		//inoutMat.SetCol0(Multiply( inMatA, inMatB.GetCol0() ));
		//inoutMat.SetCol1(Multiply( inMatA, inMatB.GetCol1() ));
		//inoutMat.SetCol2(Multiply( inMatA, inMatB.GetCol2() ));
	}

	inline Vec::Vector_4V_Out Mul_Imp_33_3( MAT33V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);

#if __XENON
		//Mat33V transA;
		//Transpose( transA, inMatA );
		//return Multiply( b, transA );

		// Find transpose.
		Mat33V transposedA;
		Vec::Vector_4V inCol0 = inMatA.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatA.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatA.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;
#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Do dot products.
		Vec::Vector_4V x = Vec::V3DotV( transposedA.GetCol0Intrin128(), b );
		Vec::Vector_4V y = Vec::V3DotV( transposedA.GetCol1Intrin128(), b );
		Vec::Vector_4V z = Vec::V3DotV( transposedA.GetCol2Intrin128(), b );
		return Vec3V( ScalarV(x), ScalarV(y), ScalarV(z) ).GetIntrin128();

#else
		Vec::Vector_4V _xxxx = Vec::V4SplatX( b );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( b );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( b );

		Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, inMatA.GetCol0Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, inMatA.GetCol1Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, inMatA.GetCol2Intrin128() );
		return sumTerm;
#endif
	}

	inline Vec::Vector_4V_Out Mul_Imp_3_33( Vec::Vector_4V_In a, MAT33V_DECL3(b) )
	{
		Mat33V inMatB = MAT33V_ARG_GET(b);

#if __XENON // go the dot product route
		Vec::Vector_4V _x = Vec::V3DotV( a, inMatB.GetCol0Intrin128() );
		Vec::Vector_4V _y = Vec::V3DotV( a, inMatB.GetCol1Intrin128() );
		Vec::Vector_4V _z = Vec::V3DotV( a, inMatB.GetCol2Intrin128() );
		return Vec3V( ScalarV(_x), ScalarV(_y), ScalarV(_z) ).GetIntrin128();
#else
		Vec::Vector_4V inCol0 = inMatB.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatB.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatB.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;
#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		Vec::Vector_4V _xxxx = Vec::V4SplatX( a );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( a );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( a );
		Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, outVec0 );
		sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, outVec1 );
		sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, outVec2 );
		return sumTerm;

		//Mat33V transB;
		//Transpose( transB, inMatB );
		//return Multiply( transB, a );
#endif
	}

	inline Vec::Vector_4V_Out Mul_Imp_34_4( MAT34V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);

		// On Xenon, this dot product route is not actually quite as good as the other.
		//Mat44V tempM44 = Mat44V(	inMatA.GetCol0Intrin128(),
		//							inMatA.GetCol1Intrin128(),
		//							inMatA.GetCol2Intrin128(),
		//							inMatA.GetCol3Intrin128()	);
		//Mat44V transA;
		//Transpose( transA, tempM44 );
		//Vec::Vector_4V _x = Vec::V4DotV( b.GetIntrin128(), transA.GetCol0Intrin128() );
		//Vec::Vector_4V _y = Vec::V4DotV( b.GetIntrin128(), transA.GetCol1Intrin128() );
		//Vec::Vector_4V _z = Vec::V4DotV( b.GetIntrin128(), transA.GetCol2Intrin128() );
		//return Vec3V( ScalarV(_x), ScalarV(_y), ScalarV(_z) );

		Vec::Vector_4V _xxxx = Vec::V4SplatX( b );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( b );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( b );
		Vec::Vector_4V _wwww = Vec::V4SplatW( b );

		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx, inMatA.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _yyyy, inMatA.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz, inMatA.GetCol2Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _wwww, inMatA.GetCol3Intrin128() );
		Vec::Vector_4V outVec = Vec::V4Add( sumTerm0, sumTerm1 );
		return outVec;
	}

	inline Vec::Vector_4V_Out Mul_Imp_3_34( Vec::Vector_4V_In a, MAT34V_DECL3(b) )
	{
		Mat34V inMatB = MAT34V_ARG_GET(b);

#if __XENON // go the dot product route
		return Vec4V(	ScalarV(Vec::V3DotV( a, inMatB.GetCol0Intrin128() )),
						ScalarV(Vec::V3DotV( a, inMatB.GetCol1Intrin128() )),
						ScalarV(Vec::V3DotV( a, inMatB.GetCol2Intrin128() )),
						ScalarV(Vec::V3DotV( a, inMatB.GetCol3Intrin128() ))	).GetIntrin128();
#else
		Vec::Vector_4V inCol0 = inMatB.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMatB.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMatB.GetCol2Intrin128();
		Vec::Vector_4V inCol3 = inMatB.GetCol3Intrin128();

		Vec::Vector_4V tempVect0 = Vec::V4MergeXY( inCol0, inCol2 );
		Vec::Vector_4V tempVect1 = Vec::V4MergeXY( inCol1, inCol3 );
		Vec::Vector_4V tempVect2 = Vec::V4MergeZW( inCol0, inCol2 );
		Vec::Vector_4V tempVect3 = Vec::V4MergeZW( inCol1, inCol3 );

		Mat44V transB;
		transB.SetColsIntrin128(	Vec::V4MergeXY( tempVect0, tempVect1 ),
									Vec::V4MergeZW( tempVect0, tempVect1 ),
									Vec::V4MergeXY( tempVect2, tempVect3 ),
									Vec::V4MergeZW( tempVect2, tempVect3 )	);

		//Mat44V tempM44 = Mat44V(	inMatB.GetCol0Intrin128(),
		//							inMatB.GetCol1Intrin128(),
		//							inMatB.GetCol2Intrin128(),
		//							inMatB.GetCol3Intrin128()	);
		//Mat44V transB;
		//Transpose( transB, tempM44 );

		Vec::Vector_4V _xxxx = Vec::V4SplatX( a );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( a );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( a );

		Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, transB.GetCol0Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, transB.GetCol1Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, transB.GetCol2Intrin128() );
		return sumTerm;
#endif
	}

	inline void InvScale_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		Mat44V inMatA = MAT44V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScale(inMatA.GetCol0Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol1Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol2Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol3Intrin128(), scaleValue)	);
	}

	inline void InvScaleSafe_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect )
	{
		Mat44V inMatA = MAT44V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleSafe(inMatA.GetCol0Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol1Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol2Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol3Intrin128(), scaleValue, errValVect)	);
	}

	inline void InvScaleFast_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		Mat44V inMatA = MAT44V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleFast(inMatA.GetCol0Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol1Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol2Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol3Intrin128(), scaleValue)	);
	}

	inline void InvScaleFastSafe_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect )
	{
		Mat44V inMatA = MAT44V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleFastSafe(inMatA.GetCol0Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol1Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol2Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol3Intrin128(), scaleValue, errValVect)	);
	}

	inline void AddScaled_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(toAddTo), MAT33V_DECL2(toScaleThenAdd), MAT33V_DECL2(toScaleBy) )
	{
		Mat33V inMatA = MAT33V_ARG_GET(toAddTo);
		Mat33V inMatB = MAT33V_ARG_GET(toScaleThenAdd);
		Mat33V inMatC = MAT33V_ARG_GET(toScaleBy);

		inoutMat.SetColsIntrin128(
			Vec::V4AddScaled( inMatA.GetCol0Intrin128(), inMatB.GetCol0Intrin128(), inMatC.GetCol0Intrin128() ),
			Vec::V4AddScaled( inMatA.GetCol1Intrin128(), inMatB.GetCol1Intrin128(), inMatC.GetCol1Intrin128() ),
			Vec::V4AddScaled( inMatA.GetCol2Intrin128(), inMatB.GetCol2Intrin128(), inMatC.GetCol2Intrin128() )
			);
	}

	inline void AddScaled_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(toAddTo), MAT33V_DECL2(toScaleThenAdd), Vec::Vector_4V_In_After3Args toScaleBy )
	{
		Mat33V inMatA = MAT33V_ARG_GET(toAddTo);
		Mat33V inMatB = MAT33V_ARG_GET(toScaleThenAdd);

		inoutMat.SetColsIntrin128(
			Vec::V4AddScaled( inMatA.GetCol0Intrin128(), inMatB.GetCol0Intrin128(), toScaleBy ),
			Vec::V4AddScaled( inMatA.GetCol1Intrin128(), inMatB.GetCol1Intrin128(), toScaleBy ),
			Vec::V4AddScaled( inMatA.GetCol2Intrin128(), inMatB.GetCol2Intrin128(), toScaleBy )
			);
	}

	inline void AddScaled_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(toAddTo), MAT44V_DECL2(toScaleThenAdd), MAT44V_DECL2(toScaleBy) )
	{
		Mat44V inMatA = MAT44V_ARG_GET(toAddTo);
		Mat44V inMatB = MAT44V_ARG_GET(toScaleThenAdd);
		Mat44V inMatC = MAT44V_ARG_GET(toScaleBy);

		inoutMat.SetColsIntrin128(
			Vec::V4AddScaled( inMatA.GetCol0Intrin128(), inMatB.GetCol0Intrin128(), inMatC.GetCol0Intrin128() ),
			Vec::V4AddScaled( inMatA.GetCol1Intrin128(), inMatB.GetCol1Intrin128(), inMatC.GetCol1Intrin128() ),
			Vec::V4AddScaled( inMatA.GetCol2Intrin128(), inMatB.GetCol2Intrin128(), inMatC.GetCol2Intrin128() ),
			Vec::V4AddScaled( inMatA.GetCol3Intrin128(), inMatB.GetCol3Intrin128(), inMatC.GetCol3Intrin128() )
			);
	}

	inline void AddScaled_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(toAddTo), MAT44V_DECL2(toScaleThenAdd), Vec::Vector_4V_In_After3Args toScaleBy )
	{
		Mat44V inMatA = MAT44V_ARG_GET(toAddTo);
		Mat44V inMatB = MAT44V_ARG_GET(toScaleThenAdd);

		inoutMat.SetColsIntrin128(
			Vec::V4AddScaled( inMatA.GetCol0Intrin128(), inMatB.GetCol0Intrin128(), toScaleBy ),
			Vec::V4AddScaled( inMatA.GetCol1Intrin128(), inMatB.GetCol1Intrin128(), toScaleBy ),
			Vec::V4AddScaled( inMatA.GetCol2Intrin128(), inMatB.GetCol2Intrin128(), toScaleBy ),
			Vec::V4AddScaled( inMatA.GetCol3Intrin128(), inMatB.GetCol3Intrin128(), toScaleBy )
			);
	}

	inline void Add_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(a), MAT34V_DECL2(b) )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);
		Mat34V inMatB = MAT34V_ARG_GET(b);

		inoutMat = inMatA + inMatB;
	}

	inline void Subtract_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(a), MAT34V_DECL2(b) )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);
		Mat34V inMatB = MAT34V_ARG_GET(b);

		inoutMat = inMatA - inMatB;
	}

	inline void Abs_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(a) )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);
		Vec::Vector_4V invAbsMask = Vec::V4VConstant(V_80000000);

		inoutMat.SetColsIntrin128(	Vec::V4Andc(inMatA.GetCol0Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol1Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol2Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol3Intrin128(), invAbsMask)	);
	}

	inline void Scale_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);

		inoutMat.SetColsIntrin128(	Vec::V4Scale(b, inMatA.GetCol0Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol1Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol2Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol3Intrin128())	);
	}

	inline void Scale_Imp33( Mat34V_InOut inoutMat, MAT34V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);

		inoutMat.SetColsIntrin128(	Vec::V4Scale(b, inMatA.GetCol0Intrin128()),
			Vec::V4Scale(b, inMatA.GetCol1Intrin128()),
			Vec::V4Scale(b, inMatA.GetCol2Intrin128()),
			inMatA.GetCol3Intrin128());
	}

	inline void InvScale_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		Mat34V inMatA = MAT34V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScale(inMatA.GetCol0Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol1Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol2Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol3Intrin128(), scaleValue)	);
	}

	inline void InvScaleSafe_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect )
	{
		Mat34V inMatA = MAT34V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleSafe(inMatA.GetCol0Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol1Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol2Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol3Intrin128(), scaleValue, errValVect)	);
	}

	inline void InvScaleFast_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		Mat34V inMatA = MAT34V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleFast(inMatA.GetCol0Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol1Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol2Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol3Intrin128(), scaleValue)	);
	}

	inline void InvScaleFastSafe_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect )
	{
		Mat34V inMatA = MAT34V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleFastSafe(inMatA.GetCol0Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol1Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol2Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol3Intrin128(), scaleValue, errValVect)	);
	}

	inline void Add_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(a), MAT33V_DECL2(b) )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);
		Mat33V inMatB = MAT33V_ARG_GET(b);

		inoutMat = inMatA + inMatB;
	}

	inline void Subtract_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(a), MAT33V_DECL2(b) )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);
		Mat33V inMatB = MAT33V_ARG_GET(b);

		inoutMat = inMatA - inMatB;
	}

	inline void Abs_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(a) )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);
		Vec::Vector_4V invAbsMask = Vec::V4VConstant(V_80000000);

		inoutMat.SetColsIntrin128(	Vec::V4Andc(inMatA.GetCol0Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol1Intrin128(), invAbsMask),
									Vec::V4Andc(inMatA.GetCol2Intrin128(), invAbsMask)	);
	}

	inline void Scale_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(a), Vec::Vector_4V_In_After3Args b )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);

		inoutMat.SetColsIntrin128(	Vec::V4Scale(b, inMatA.GetCol0Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol1Intrin128()),
									Vec::V4Scale(b, inMatA.GetCol2Intrin128())	);
	}

	inline void InvScale_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		Mat33V inMatA = MAT33V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScale(inMatA.GetCol0Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol1Intrin128(), scaleValue),
									Vec::V4InvScale(inMatA.GetCol2Intrin128(), scaleValue)	);
	}

	inline void InvScaleSafe_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect )
	{
		Mat33V inMatA = MAT33V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleSafe(inMatA.GetCol0Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol1Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleSafe(inMatA.GetCol2Intrin128(), scaleValue, errValVect)	);
	}

	inline void InvScaleFast_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		Mat33V inMatA = MAT33V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleFast(inMatA.GetCol0Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol1Intrin128(), scaleValue),
									Vec::V4InvScaleFast(inMatA.GetCol2Intrin128(), scaleValue)	);
	}

	inline void InvScaleFastSafe_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect )
	{
		Mat33V inMatA = MAT33V_ARG_GET(toScale);

		inoutMat.SetColsIntrin128(	Vec::V4InvScaleFastSafe(inMatA.GetCol0Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol1Intrin128(), scaleValue, errValVect),
									Vec::V4InvScaleFastSafe(inMatA.GetCol2Intrin128(), scaleValue, errValVect)	);
	}

	inline void InvertFull_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(a) )
	{
		Mat44V inMatA = MAT44V_ARG_GET(a);

		Vec::Vector_4V inMatCol0, inMatCol1, inMatCol2, inMatCol3;
		Vec::Vector_4V permutedVec0, permutedVec1, permutedVec2, permutedVec3;
		Vec::Vector_4V cofactorVect0, cofactorVect1, cofactorVect2, cofactorVect3;
		Vec::Vector_4V _0, _1, _2, _3;
		Vec::Vector_4V _01, _02, _03, _12, _23;
		Vec::Vector_4V _1rotated, _2rotated;
		Vec::Vector_4V _01rotated, _02rotated, _03rotated, _12rotated, _23rotated;
		Vec::Vector_4V _1rotated3, _1rotated3rotated;
		Vec::Vector_4V detVect, detVect0, detVect1, detVect2, detVect3, invDetVect;
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		inMatCol0 = inMatA.GetCol0Intrin128();
		inMatCol1 = inMatA.GetCol1Intrin128();
		inMatCol2 = inMatA.GetCol2Intrin128();
		inMatCol3 = inMatA.GetCol3Intrin128();

		// The next two permutes are one instruction on XBox360.
		permutedVec0 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inMatCol0, inMatCol1);
		permutedVec1 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inMatCol2, inMatCol3);

		permutedVec2 = Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inMatCol0, inMatCol1);
		permutedVec3 = Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inMatCol2, inMatCol3);

		// The next two permutes are one instruction on XBox360.
		_0 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVec0, permutedVec1);
		_1 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVec3, permutedVec2);

		_2 = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVec0, permutedVec1);
		_3 = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVec3, permutedVec2);

		// V4Permute<Z,W,X,Y>() resolves to one instruction on PS3 PPU.
		// (All V4Permute<>()'s resolve to one instruction on XBox360.)
		_23					= Vec::V4Scale(_2, _3);
		_23					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_23);
		cofactorVect0		= Vec::V4SubtractScaled(_zero, _23, _1);
		cofactorVect1		= Vec::V4SubtractScaled(_zero, _23, _0);
		_23rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( _23 );
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _1, _23rotated);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _0, _23rotated);
		cofactorVect1		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect1);
		_12					= Vec::V4Scale(_1, _2);
		_12					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_12);
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _3, _12);
		cofactorVect3		= Vec::V4Scale(_0, _12);
		_12rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_12);
		cofactorVect0		= Vec::V4SubtractScaled(cofactorVect0, _12rotated, _3);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _12rotated, _0);
		cofactorVect3		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect3);
		_1rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1);
		_2rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_2);
		_1rotated3			= Vec::V4Scale(_1rotated, _3);
		_1rotated3			= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_1rotated3);
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _2rotated, _1rotated3);
		cofactorVect2		= Vec::V4Scale(_0, _1rotated3);
		_1rotated3rotated	= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1rotated3);
		cofactorVect0		= Vec::V4SubtractScaled(cofactorVect0, _1rotated3rotated, _2rotated);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _1rotated3rotated, _0);
		cofactorVect2		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect2);
		_01					= Vec::V4Scale(_0, _1);
		_01					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_01);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _01, _3);
		cofactorVect3		= Vec::V4AddScaled(cofactorVect3, _2rotated, _01);
		_01rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_01);
		cofactorVect2		= Vec::V4AddScaled(cofactorVect2, _3, _01rotated);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _01rotated, _2rotated);
		_03					= Vec::V4Scale(_0, _3);
		_03					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_03);
		cofactorVect1		= Vec::V4SubtractScaled(cofactorVect1, _03, _2rotated);
		cofactorVect2		= Vec::V4AddScaled(cofactorVect2, _03, _1);
		_03rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_03);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _03rotated, _2rotated);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _03rotated, _1);
		_02					= Vec::V4Scale(_0, _2rotated);
		_02					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_02);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _3, _02);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _02, _1);
		_02rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_02);
		cofactorVect1		= Vec::V4SubtractScaled(cofactorVect1, _02rotated, _3);
		cofactorVect3		= Vec::V4AddScaled(cofactorVect3, _1, _02rotated);

		detVect				= Vec::V4Scale(_0, cofactorVect0);
		detVect0			= Vec::V4SplatX(detVect);
		detVect1			= Vec::V4SplatY(detVect);
		detVect2			= Vec::V4SplatZ(detVect);
		detVect3			= Vec::V4SplatW(detVect);
		detVect				= Vec::V4Add(detVect0, detVect1);
		detVect2			= Vec::V4Add(detVect2, detVect3);
		detVect				= Vec::V4Add(detVect, detVect2);

		invDetVect			= Vec::V4Invert(detVect);

		inoutMat.SetColsIntrin128(	Vec::V4Scale(cofactorVect0, invDetVect),
									Vec::V4Scale(cofactorVect1, invDetVect),
									Vec::V4Scale(cofactorVect2, invDetVect),
									Vec::V4Scale(cofactorVect3, invDetVect)	);
	}

	inline void InvertTransformFull_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(a) )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);

		Vec::Vector_4V inMatCol0 = inMatA.GetCol0Intrin128();
		Vec::Vector_4V inMatCol1 = inMatA.GetCol1Intrin128();
		Vec::Vector_4V inMatCol2 = inMatA.GetCol2Intrin128();
		Vec::Vector_4V inMatCol3 = inMatA.GetCol3Intrin128();

		Vec::Vector_4V outVec0, outVec1, outVec2, outVec3;
		Vec::Vector_4V crossedVec0, crossedVec1, crossedVec2;
		Vec::Vector_4V dotVect, oneOverDeterminant;
		Vec::Vector_4V _xxxx, _yyyy, _zzzz;

		crossedVec2 = Vec::V3Cross( inMatCol0, inMatCol1 );
		crossedVec0 = Vec::V3Cross( inMatCol1, inMatCol2 );
		crossedVec1 = Vec::V3Cross( inMatCol2, inMatCol0 );
		outVec3 = Vec::V4Negate( inMatCol3 );
		dotVect = Vec::V3DotV( crossedVec2, inMatCol2 );
		oneOverDeterminant = Vec::V4Invert( dotVect );
		
#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V mergedVec0, mergedVec1;
		mergedVec0 = Vec::V4MergeXY( crossedVec0, crossedVec2 );
		mergedVec1 = Vec::V4MergeZW( crossedVec0, crossedVec2 );
		outVec0 = Vec::V4MergeXY( mergedVec0, crossedVec1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( mergedVec0, crossedVec1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( mergedVec1, crossedVec1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(crossedVec0, crossedVec2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(crossedVec1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(crossedVec0, crossedVec2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(crossedVec1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		_xxxx = Vec::V4SplatX( outVec3 );
		_yyyy = Vec::V4SplatY( outVec3 );
		_zzzz = Vec::V4SplatZ( outVec3 );
		outVec3 = Vec::V4Scale( outVec0, _xxxx );
		outVec3 = Vec::V4AddScaled( outVec3, outVec1, _yyyy );
		outVec3 = Vec::V4AddScaled( outVec3, outVec2, _zzzz );
		outVec0 = Vec::V4Scale( outVec0, oneOverDeterminant );
		outVec1 = Vec::V4Scale( outVec1, oneOverDeterminant );
		outVec2 = Vec::V4Scale( outVec2, oneOverDeterminant );
		outVec3 = Vec::V4Scale( outVec3, oneOverDeterminant );

		inoutMat.SetColsIntrin128( outVec0, outVec1, outVec2, outVec3 );
	}

	inline void InvertTransformOrtho_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(a) )
	{
		Mat34V inMatA = MAT34V_ARG_GET(a);

		Vec::Vector_4V inMatCol0 = inMatA.GetCol0Intrin128();
		Vec::Vector_4V inMatCol1 = inMatA.GetCol1Intrin128();
		Vec::Vector_4V inMatCol2 = inMatA.GetCol2Intrin128();
		Vec::Vector_4V inMatCol3 = inMatA.GetCol3Intrin128();

#if __XENON

		// Negate col3.
		inMatCol3 = Vec::V4Negate( inMatCol3 );

		// Find 3x3 transpose.
		Vec::Vector_4V transposed0, transposed1, transposed2;
#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inMatCol0, inMatCol2 );
		merged1 = Vec::V4MergeZW( inMatCol0, inMatCol2 );
		transposed0 = Vec::V4MergeXY( merged0, inMatCol1 );
		transposed1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inMatCol1 );
		transposed2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inMatCol1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inMatCol0, inMatCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inMatCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inMatCol0, inMatCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inMatCol1, _zero);
		transposed0 = Vec::V4MergeXY(temp0, temp1);
		transposed1 = Vec::V4MergeZW(temp0, temp1);
		transposed2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Do dot3 products.
		Vec::Vector_4V outElem0 = Vec::V3DotV( inMatCol0, inMatCol3 );
		Vec::Vector_4V outElem1 = Vec::V3DotV( inMatCol1, inMatCol3 );
		Vec::Vector_4V outElem2 = Vec::V3DotV( inMatCol2, inMatCol3 );
		Vec3V outCol3 = Vec3V( ScalarV(outElem0), ScalarV(outElem1), ScalarV(outElem2) );

		inoutMat.SetColsIntrin128( transposed0, transposed1, transposed2, outCol3.GetIntrin128() );
#else
		// Load from instructions (on PS3).
		Vec::Vector_4V signBitVect = Vec::V4VConstant(V_80000000);

		// Negate.
		Vec::Vector_4V outVect3 = Vec::V4Xor( inMatCol3, signBitVect );

		// Or, use the instructions-only negate.
		//Vec::Vector_4V outVect3 = Vec::V4Negate( inMatCol3 );

		// Transpose for the 3x3 part, then untransform the translation.
		Vec::Vector_4V outVect0, outVect1, outVect2;
#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V mergedVect0, mergedVect1;
		mergedVect0 = Vec::V4MergeXY( inMatCol0, inMatCol2 );
		mergedVect1 = Vec::V4MergeZW( inMatCol0, inMatCol2 );
		outVect0 = Vec::V4MergeXY( mergedVect0, inMatCol1 );
		outVect1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( mergedVect0, inMatCol1 );
		outVect2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( mergedVect1, inMatCol1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inMatCol0, inMatCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inMatCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inMatCol0, inMatCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inMatCol1, _zero);
		outVect0 = Vec::V4MergeXY(temp0, temp1);
		outVect1 = Vec::V4MergeZW(temp0, temp1);
		outVect2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		Vec::Vector_4V _xxxx, _yyyy, _zzzz;
		_xxxx = Vec::V4SplatX( outVect3 );
		_yyyy = Vec::V4SplatY( outVect3 );
		_zzzz = Vec::V4SplatZ( outVect3 );
		outVect3 = Vec::V4Scale( outVect0, _xxxx );
		outVect3 = Vec::V4AddScaled( outVect3, outVect1, _yyyy );
		outVect3 = Vec::V4AddScaled( outVect3, outVect2, _zzzz );

		inoutMat.SetColsIntrin128( outVect0, outVect1, outVect2, outVect3 );
#endif
	}

	inline void InvertFull_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(a) )
	{
		Mat33V inMatA = MAT33V_ARG_GET(a);

		Vec::Vector_4V cross0, cross1, cross2;
		Vec::Vector_4V dotVect;
		Vec::Vector_4V oneOverDeterminant;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		cross2 = Vec::V3Cross( inMatA.GetCol0Intrin128(), inMatA.GetCol1Intrin128() );
		cross0 = Vec::V3Cross( inMatA.GetCol1Intrin128(), inMatA.GetCol2Intrin128() );
		cross1 = Vec::V3Cross( inMatA.GetCol2Intrin128(), inMatA.GetCol0Intrin128() );
		dotVect = Vec::V3DotV( cross2, inMatA.GetCol2Intrin128() );
		oneOverDeterminant = Vec::V4Invert( dotVect );

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V tempVect0, tempVect1;
		tempVect0 = Vec::V4MergeXY( cross0, cross2 );
		tempVect1 = Vec::V4MergeZW( cross0, cross2 );
		outCol0 = Vec::V4MergeXY( tempVect0, cross1 );
		outCol1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( tempVect0, cross1 );
		outCol2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( tempVect1, cross1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(cross0, cross2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(cross1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(cross0, cross2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(cross1, _zero);
		outCol0 = Vec::V4MergeXY(temp0, temp1);
		outCol1 = Vec::V4MergeZW(temp0, temp1);
		outCol2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		outCol0 = Vec::V4Scale( outCol0, oneOverDeterminant );
		outCol1 = Vec::V4Scale( outCol1, oneOverDeterminant );
		outCol2 = Vec::V4Scale( outCol2, oneOverDeterminant );

		inoutMat.SetColsIntrin128( outCol0, outCol1, outCol2 );
	}

	inline Vec::Vector_4V_Out Transform_Imp34( MAT34V_DECL(transformMat), Vec::Vector_4V_In_After3Args inPoint )
	{
		Mat34V inMat = MAT34V_ARG_GET(transformMat);

#if __XENON // The splat/madd version actually beats the dotprod version, on __XENON. Except when inlined, as constant generation gets optimized out of the loop.

		// Find 3x3 transpose (but also get translation into w components).
		Mat33V transposedA;
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inMat.GetCol0Intrin128(), inMat.GetCol2Intrin128());
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inMat.GetCol1Intrin128(), inMat.GetCol3Intrin128());
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inMat.GetCol0Intrin128(), inMat.GetCol2Intrin128());
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inMat.GetCol1Intrin128(), inMat.GetCol3Intrin128());
		Vec::Vector_4V transposed0 = Vec::V4MergeXY(temp0, temp1);
		Vec::Vector_4V transposed1 = Vec::V4MergeZW(temp0, temp1);
		Vec::Vector_4V transposed2 = Vec::V4MergeXY(temp2, temp3);
		transposedA.SetColsIntrin128( transposed0, transposed1, transposed2 );

		// Do dot4 products.
		Vec::Vector_4V inPointOneW = Vec::V4Or( Vec::V4And( inPoint, Vec::V4VConstant(V_MASKXYZ) ), Vec::V4VConstant(V_ZERO_WONE) );
		Vec::Vector_4V outElem0 = Vec::V4DotV( transposedA.GetCol0Intrin128(), inPointOneW );
		Vec::Vector_4V outElem1 = Vec::V4DotV( transposedA.GetCol1Intrin128(), inPointOneW );
		Vec::Vector_4V outElem2 = Vec::V4DotV( transposedA.GetCol2Intrin128(), inPointOneW );
		Vec3V outVec = Vec3V( ScalarV(outElem0), ScalarV(outElem1), ScalarV(outElem2) );
		return outVec.GetIntrin128();

		//
		// Alternate method (no constant fetching required, but extra dependent __vaddfp() at the end)
		//

		// Find transpose.
		//Mat33V transposedA;
		//Vec::Vector_4V inCol0 = inMat.GetCol0Intrin128();
		//Vec::Vector_4V inCol1 = inMat.GetCol1Intrin128();
		//Vec::Vector_4V inCol2 = inMat.GetCol2Intrin128();
		//Vec::Vector_4V merged0, merged1, outVec0, outVec1, outVec2;
		//merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		//merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		//outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		//outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		//outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		//transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );

		// Do dot3 products.
		//Vec::Vector_4V outElem0 = Vec::V3DotV( transposedA.GetCol0Intrin128(), inPoint );
		//Vec::Vector_4V outElem1 = Vec::V3DotV( transposedA.GetCol1Intrin128(), inPoint );
		//Vec::Vector_4V outElem2 = Vec::V3DotV( transposedA.GetCol2Intrin128(), inPoint );
		//Vec3V outVec = Vec3V( ScalarV(outElem0), ScalarV(outElem1), ScalarV(outElem2) );
		//return Add( outVec, inMat.GetCol3() );
#else
		Vec::Vector_4V resultVec;
		Vec::Vector_4V _xxxx = Vec::V4SplatX( inPoint );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( inPoint );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( inPoint );
		resultVec = Vec::V4AddScaled( inMat.GetCol3Intrin128(), _xxxx, inMat.GetCol0Intrin128() );
		resultVec = Vec::V4AddScaled( resultVec, _yyyy, inMat.GetCol1Intrin128() );
		resultVec = Vec::V4AddScaled( resultVec, _zzzz, inMat.GetCol2Intrin128() );
		return resultVec;
#endif
	}

	inline Vec::Vector_4V_Out UnTransformFull_Imp34( MAT34V_DECL(origTransformMat), Vec::Vector_4V_In_After3Args transformedPoint )
	{
		return UnTransformFull_Imp33( origTransformMat_col0, origTransformMat_col1, origTransformMat_col2, Vec::V4Subtract( transformedPoint, origTransformMat_col3 ) );

		//Mat33V invMat33;
		//Invert( invMat33, Mat33V( inMat.GetCol0Intrin128(), inMat.GetCol1Intrin128(), inMat.GetCol2Intrin128() ) );
		//return Vec3V( Multiply( invMat33, tempPoint ) );
	}

	inline Vec::Vector_4V_Out UnTransformOrtho_Imp34( MAT34V_DECL(origOrthoTransformMat), Vec::Vector_4V_In_After3Args transformedPoint )
	{
		Mat34V inMat = MAT34V_ARG_GET(origOrthoTransformMat);

		// Subtract the translation.
		Vec::Vector_4V tempPoint = Vec::V4Subtract( transformedPoint, inMat.GetCol3Intrin128() );

#if __XENON
		// Pre-multiply by inverse (which is the transpose in this case).
		// (Pre-multiplying by the transpose is most efficient if we don't actually take the transpose.)
		Vec::Vector_4V _x = Vec::V3DotV( inMat.GetCol0Intrin128(), tempPoint );
		Vec::Vector_4V _y = Vec::V3DotV( inMat.GetCol1Intrin128(), tempPoint );
		Vec::Vector_4V _z = Vec::V3DotV( inMat.GetCol2Intrin128(), tempPoint );

		return Vec3V( ScalarV(_x), ScalarV(_y), ScalarV(_z) ).GetIntrin128();
#else
		// Find 3x3 transpose.
		Mat33V transInputMat;
		Vec::Vector_4V inCol0 = inMat.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = inMat.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = inMat.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;
#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		transInputMat.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
		transInputMat.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Transform vector3.
		Vec::Vector_4V _xxxx = Vec::V4SplatX( tempPoint );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( tempPoint );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( tempPoint );
		Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, transInputMat.GetCol0Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, transInputMat.GetCol1Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, transInputMat.GetCol2Intrin128() );
		return sumTerm;
#endif
	}

	__forceinline void ReOrthonormalize_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(inMat) )
	{
		// This routine uses the X and Y to reproduce the Z, then uses the Y and Z to reproduce the X. Thus Y is preserved.

		// Note that this routine uses 3 normalizes, when only 2 are needed (see old vec lib's Matrix34::Normalize()). But this
		// version has much less dependencies. Haven't benchmarked which is faster.

		Mat33V inMat = MAT33V_ARG_GET(inMat);

		Vec::Vector_4V _x = inMat.GetCol0Intrin128();
		Vec::Vector_4V _y = inMat.GetCol1Intrin128();

		// X' = Y cross Z'
		// Y' = Y
		// Z' = X cross Y
		Vec::Vector_4V newZ = Vec::V3Cross( _x, _y );
		Vec::Vector_4V newX = Vec::V3Cross( _y, newZ );

		// Normalize.
		Vec::Vector_4V normalizedY = Vec::V3Normalize( _y );
		Vec::Vector_4V normalizedZ = Vec::V3Normalize( newZ );
		Vec::Vector_4V normalizedX = Vec::V3Normalize( newX );
		
		inoutMat.SetColsIntrin128( normalizedX, normalizedY, normalizedZ );
	}


	inline void UnTransformOrtho_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(origOrthoTransform), MAT44V_DECL2(concat) )
	{
		Mat44V origOrthoTransformMat = MAT44V_ARG_GET(origOrthoTransform);
		Mat44V concatMat = MAT44V_ARG_GET(concat);

		// Pre-multiply by inverse (which is the transpose in this case).
		// (Pre-multiplying by the transpose is most efficient if we don't actually take the transpose.)
		Vec::Vector_4V _col0x = Vec::V4DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _col0y = Vec::V4DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _col0z = Vec::V4DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _col0w = Vec::V4DotV( origOrthoTransformMat.GetCol3Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _col1x = Vec::V4DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _col1y = Vec::V4DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _col1z = Vec::V4DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _col1w = Vec::V4DotV( origOrthoTransformMat.GetCol3Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _col2x = Vec::V4DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _col2y = Vec::V4DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _col2z = Vec::V4DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _col2w = Vec::V4DotV( origOrthoTransformMat.GetCol3Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _col3x = Vec::V4DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol3Intrin128() );
		Vec::Vector_4V _col3y = Vec::V4DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol3Intrin128() );
		Vec::Vector_4V _col3z = Vec::V4DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol3Intrin128() );
		Vec::Vector_4V _col3w = Vec::V4DotV( origOrthoTransformMat.GetCol3Intrin128(), concatMat.GetCol3Intrin128() );

		inoutMat.SetCol0( Vec4V( ScalarV(_col0x), ScalarV(_col0y), ScalarV(_col0z), ScalarV(_col0w) ) );
		inoutMat.SetCol1( Vec4V( ScalarV(_col1x), ScalarV(_col1y), ScalarV(_col1z), ScalarV(_col1w) ) );
		inoutMat.SetCol2( Vec4V( ScalarV(_col2x), ScalarV(_col2y), ScalarV(_col2z), ScalarV(_col2w) ) );
		inoutMat.SetCol3( Vec4V( ScalarV(_col3x), ScalarV(_col3y), ScalarV(_col3z), ScalarV(_col3w) ) );
	}

	inline void UnTransformFull_Imp44( Mat44V_InOut inoutMat, MAT44V_DECL(origTransform), MAT44V_DECL2(concat) )
	{
		Mat44V origTransformMat = MAT44V_ARG_GET(origTransform);
		Mat44V concatMat = MAT44V_ARG_GET(concat);

		// Get inverse.
		Mat44V invMat44;
		Vec::Vector_4V inMatCol0, inMatCol1, inMatCol2, inMatCol3;
		Vec::Vector_4V permutedVec0, permutedVec1, permutedVec2, permutedVec3;
		Vec::Vector_4V cofactorVect0, cofactorVect1, cofactorVect2, cofactorVect3;
		Vec::Vector_4V _0, _1, _2, _3;
		Vec::Vector_4V _01, _02, _03, _12, _23;
		Vec::Vector_4V _1rotated, _2rotated;
		Vec::Vector_4V _01rotated, _02rotated, _03rotated, _12rotated, _23rotated;
		Vec::Vector_4V _1rotated3, _1rotated3rotated;
		Vec::Vector_4V detVect, detVect0, detVect1, detVect2, detVect3, invDetVect;
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		inMatCol0 = origTransformMat.GetCol0Intrin128();
		inMatCol1 = origTransformMat.GetCol1Intrin128();
		inMatCol2 = origTransformMat.GetCol2Intrin128();
		inMatCol3 = origTransformMat.GetCol3Intrin128();

		// The next two permutes are one instruction on XBox360.
		permutedVec0 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inMatCol0, inMatCol1);
		permutedVec1 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inMatCol2, inMatCol3);

		permutedVec2 = Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inMatCol0, inMatCol1);
		permutedVec3 = Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inMatCol2, inMatCol3);

		// The next two permutes are one instruction on XBox360.
		_0 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVec0, permutedVec1);
		_1 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVec3, permutedVec2);

		_2 = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVec0, permutedVec1);
		_3 = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVec3, permutedVec2);

		// V4Permute<Z,W,X,Y>() resolves to one instruction on PS3 PPU.
		// (All V4Permute<>()'s resolve to one instruction on XBox360.)
		_23					= Vec::V4Scale(_2, _3);
		_23					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_23);
		cofactorVect0		= Vec::V4SubtractScaled(_zero, _23, _1);
		cofactorVect1		= Vec::V4SubtractScaled(_zero, _23, _0);
		_23rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( _23 );
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _1, _23rotated);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _0, _23rotated);
		cofactorVect1		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect1);
		_12					= Vec::V4Scale(_1, _2);
		_12					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_12);
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _3, _12);
		cofactorVect3		= Vec::V4Scale(_0, _12);
		_12rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_12);
		cofactorVect0		= Vec::V4SubtractScaled(cofactorVect0, _12rotated, _3);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _12rotated, _0);
		cofactorVect3		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect3);
		_1rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1);
		_2rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_2);
		_1rotated3			= Vec::V4Scale(_1rotated, _3);
		_1rotated3			= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_1rotated3);
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _2rotated, _1rotated3);
		cofactorVect2		= Vec::V4Scale(_0, _1rotated3);
		_1rotated3rotated	= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1rotated3);
		cofactorVect0		= Vec::V4SubtractScaled(cofactorVect0, _1rotated3rotated, _2rotated);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _1rotated3rotated, _0);
		cofactorVect2		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect2);
		_01					= Vec::V4Scale(_0, _1);
		_01					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_01);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _01, _3);
		cofactorVect3		= Vec::V4AddScaled(cofactorVect3, _2rotated, _01);
		_01rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_01);
		cofactorVect2		= Vec::V4AddScaled(cofactorVect2, _3, _01rotated);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _01rotated, _2rotated);
		_03					= Vec::V4Scale(_0, _3);
		_03					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_03);
		cofactorVect1		= Vec::V4SubtractScaled(cofactorVect1, _03, _2rotated);
		cofactorVect2		= Vec::V4AddScaled(cofactorVect2, _03, _1);
		_03rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_03);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _03rotated, _2rotated);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _03rotated, _1);
		_02					= Vec::V4Scale(_0, _2rotated);
		_02					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_02);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _3, _02);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _02, _1);
		_02rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_02);
		cofactorVect1		= Vec::V4SubtractScaled(cofactorVect1, _02rotated, _3);
		cofactorVect3		= Vec::V4AddScaled(cofactorVect3, _1, _02rotated);

		detVect				= Vec::V4Scale(_0, cofactorVect0);
		detVect0			= Vec::V4SplatX(detVect);
		detVect1			= Vec::V4SplatY(detVect);
		detVect2			= Vec::V4SplatZ(detVect);
		detVect3			= Vec::V4SplatW(detVect);
		detVect				= Vec::V4Add(detVect0, detVect1);
		detVect2			= Vec::V4Add(detVect2, detVect3);
		detVect				= Vec::V4Add(detVect, detVect2);

		invDetVect			= Vec::V4Invert(detVect);

		invMat44.SetColsIntrin128(	Vec::V4Scale(cofactorVect0, invDetVect),
									Vec::V4Scale(cofactorVect1, invDetVect),
									Vec::V4Scale(cofactorVect2, invDetVect),
									Vec::V4Scale(cofactorVect3, invDetVect)	);

		// Pre-multiply with other matrix, 'concatMat'.
		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _wwww0 = Vec::V4SplatW( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx0, invMat44.GetCol0Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _yyyy0, invMat44.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz0, invMat44.GetCol2Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _wwww0, invMat44.GetCol3Intrin128() );

		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _wwww1 = Vec::V4SplatW( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _xxxx1, invMat44.GetCol0Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _yyyy1, invMat44.GetCol1Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _zzzz1, invMat44.GetCol2Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _wwww1, invMat44.GetCol3Intrin128() );

		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _wwww2 = Vec::V4SplatW( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V sumTerm2 = Vec::V4Scale( _xxxx2, invMat44.GetCol0Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _yyyy2, invMat44.GetCol1Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _zzzz2, invMat44.GetCol2Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _wwww2, invMat44.GetCol3Intrin128() );

		Vec::Vector_4V _xxxx3 = Vec::V4SplatX( concatMat.GetCol3Intrin128() );
		Vec::Vector_4V _yyyy3 = Vec::V4SplatY( concatMat.GetCol3Intrin128() );
		Vec::Vector_4V _zzzz3 = Vec::V4SplatZ( concatMat.GetCol3Intrin128() );
		Vec::Vector_4V _wwww3 = Vec::V4SplatW( concatMat.GetCol3Intrin128() );
		Vec::Vector_4V sumTerm3 = Vec::V4Scale( _xxxx3, invMat44.GetCol0Intrin128() );
		sumTerm3 = Vec::V4AddScaled( sumTerm3, _yyyy3, invMat44.GetCol1Intrin128() );
		sumTerm3 = Vec::V4AddScaled( sumTerm3, _zzzz3, invMat44.GetCol2Intrin128() );
		sumTerm3 = Vec::V4AddScaled( sumTerm3, _wwww3, invMat44.GetCol3Intrin128() );

		inoutMat.SetColsIntrin128( sumTerm0, sumTerm1, sumTerm2, sumTerm3 );

		//Mat44V invMat44;
		//Invert( invMat44, origTransformMat );
		//Multiply( inoutMat, invMat44, concatMat );
	}

	inline Vec::Vector_4V_Out UnTransformFull_Imp44( MAT44V_DECL(origTransform), Vec::Vector_4V_In_After3Args transformedVect )
	{
		Mat44V origTransformMat = MAT44V_ARG_GET(origTransform);

		// Invert the transform mat.
		Mat44V invMat44;
		Vec::Vector_4V inMatCol0, inMatCol1, inMatCol2, inMatCol3;
		Vec::Vector_4V permutedVec0, permutedVec1, permutedVec2, permutedVec3;
		Vec::Vector_4V cofactorVect0, cofactorVect1, cofactorVect2, cofactorVect3;
		Vec::Vector_4V _0, _1, _2, _3;
		Vec::Vector_4V _01, _02, _03, _12, _23;
		Vec::Vector_4V _1rotated, _2rotated;
		Vec::Vector_4V _01rotated, _02rotated, _03rotated, _12rotated, _23rotated;
		Vec::Vector_4V _1rotated3, _1rotated3rotated;
		Vec::Vector_4V detVect, detVect0, detVect1, detVect2, detVect3, invDetVect;
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);

		inMatCol0 = origTransformMat.GetCol0Intrin128();
		inMatCol1 = origTransformMat.GetCol1Intrin128();
		inMatCol2 = origTransformMat.GetCol2Intrin128();
		inMatCol3 = origTransformMat.GetCol3Intrin128();

		// The next two permutes are one instruction on XBox360.
		permutedVec0 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inMatCol0, inMatCol1);
		permutedVec1 = Vec::V4PermuteTwo<Vec::X1,Vec::X2,Vec::Z1,Vec::Z2>(inMatCol2, inMatCol3);

		permutedVec2 = Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inMatCol0, inMatCol1);
		permutedVec3 = Vec::V4PermuteTwo<Vec::Y1,Vec::Y2,Vec::W1,Vec::W2>(inMatCol2, inMatCol3);

		// The next two permutes are one instruction on XBox360.
		_0 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVec0, permutedVec1);
		_1 = Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::X2,Vec::Y2>(permutedVec3, permutedVec2);

		_2 = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVec0, permutedVec1);
		_3 = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::Z2,Vec::W2>(permutedVec3, permutedVec2);

		// V4Permute<Z,W,X,Y>() resolves to one instruction on PS3 PPU.
		// (All V4Permute<>()'s resolve to one instruction on XBox360.)
		_23					= Vec::V4Scale(_2, _3);
		_23					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_23);
		cofactorVect0		= Vec::V4SubtractScaled(_zero, _23, _1);
		cofactorVect1		= Vec::V4SubtractScaled(_zero, _23, _0);
		_23rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( _23 );
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _1, _23rotated);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _0, _23rotated);
		cofactorVect1		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect1);
		_12					= Vec::V4Scale(_1, _2);
		_12					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_12);
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _3, _12);
		cofactorVect3		= Vec::V4Scale(_0, _12);
		_12rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_12);
		cofactorVect0		= Vec::V4SubtractScaled(cofactorVect0, _12rotated, _3);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _12rotated, _0);
		cofactorVect3		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect3);
		_1rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1);
		_2rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_2);
		_1rotated3			= Vec::V4Scale(_1rotated, _3);
		_1rotated3			= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_1rotated3);
		cofactorVect0		= Vec::V4AddScaled(cofactorVect0, _2rotated, _1rotated3);
		cofactorVect2		= Vec::V4Scale(_0, _1rotated3);
		_1rotated3rotated	= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_1rotated3);
		cofactorVect0		= Vec::V4SubtractScaled(cofactorVect0, _1rotated3rotated, _2rotated);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _1rotated3rotated, _0);
		cofactorVect2		= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(cofactorVect2);
		_01					= Vec::V4Scale(_0, _1);
		_01					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_01);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _01, _3);
		cofactorVect3		= Vec::V4AddScaled(cofactorVect3, _2rotated, _01);
		_01rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_01);
		cofactorVect2		= Vec::V4AddScaled(cofactorVect2, _3, _01rotated);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _01rotated, _2rotated);
		_03					= Vec::V4Scale(_0, _3);
		_03					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_03);
		cofactorVect1		= Vec::V4SubtractScaled(cofactorVect1, _03, _2rotated);
		cofactorVect2		= Vec::V4AddScaled(cofactorVect2, _03, _1);
		_03rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_03);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _03rotated, _2rotated);
		cofactorVect2		= Vec::V4SubtractScaled(cofactorVect2, _03rotated, _1);
		_02					= Vec::V4Scale(_0, _2rotated);
		_02					= Vec::V4Permute<Vec::Y,Vec::X,Vec::W,Vec::Z>(_02);
		cofactorVect1		= Vec::V4AddScaled(cofactorVect1, _3, _02);
		cofactorVect3		= Vec::V4SubtractScaled(cofactorVect3, _02, _1);
		_02rotated			= Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>(_02);
		cofactorVect1		= Vec::V4SubtractScaled(cofactorVect1, _02rotated, _3);
		cofactorVect3		= Vec::V4AddScaled(cofactorVect3, _1, _02rotated);

		detVect				= Vec::V4Scale(_0, cofactorVect0);
		detVect0			= Vec::V4SplatX(detVect);
		detVect1			= Vec::V4SplatY(detVect);
		detVect2			= Vec::V4SplatZ(detVect);
		detVect3			= Vec::V4SplatW(detVect);
		detVect				= Vec::V4Add(detVect0, detVect1);
		detVect2			= Vec::V4Add(detVect2, detVect3);
		detVect				= Vec::V4Add(detVect, detVect2);

		invDetVect			= Vec::V4Invert(detVect);

		invMat44.SetColsIntrin128(	Vec::V4Scale(cofactorVect0, invDetVect),
									Vec::V4Scale(cofactorVect1, invDetVect),
									Vec::V4Scale(cofactorVect2, invDetVect),
									Vec::V4Scale(cofactorVect3, invDetVect)	);

		// Premultiply by the vector, 'transformedVect'.
		Vec::Vector_4V _xxxx = Vec::V4SplatX( transformedVect );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( transformedVect );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( transformedVect );
		Vec::Vector_4V _wwww = Vec::V4SplatW( transformedVect );

		Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, invMat44.GetCol0Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, invMat44.GetCol1Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, invMat44.GetCol2Intrin128() );
		sumTerm = Vec::V4AddScaled( sumTerm, _wwww, invMat44.GetCol3Intrin128() );

		return sumTerm;

		//Mat44V invMat44;
		//Invert( invMat44, origTransformMat );
		//return Multiply( invMat44, transformedVect );
	}

	inline void UnTransformFull_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(origTransform), MAT33V_DECL2(concat) )
	{
		Mat33V origTransformMat = MAT33V_ARG_GET(origTransform);
		Mat33V concatMat = MAT33V_ARG_GET(concat);

		// Compute the inverse.
		Mat33V invMat33;
		Vec::Vector_4V cross0, cross1, cross2;
		Vec::Vector_4V dotVect;
		Vec::Vector_4V oneOverDeterminant;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		cross2 = Vec::V3Cross( origTransformMat.GetCol0Intrin128(), origTransformMat.GetCol1Intrin128() );
		cross0 = Vec::V3Cross( origTransformMat.GetCol1Intrin128(), origTransformMat.GetCol2Intrin128() );
		cross1 = Vec::V3Cross( origTransformMat.GetCol2Intrin128(), origTransformMat.GetCol0Intrin128() );
		dotVect = Vec::V3DotV( cross2, origTransformMat.GetCol2Intrin128() );
		oneOverDeterminant = Vec::V4Invert( dotVect );

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V tempVect0, tempVect1;
		tempVect0 = Vec::V4MergeXY( cross0, cross2 );
		tempVect1 = Vec::V4MergeZW( cross0, cross2 );
		outCol0 = Vec::V4MergeXY( tempVect0, cross1 );
		outCol1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( tempVect0, cross1 );
		outCol2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( tempVect1, cross1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(cross0, cross2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(cross1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(cross0, cross2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(cross1, _zero);
		outCol0 = Vec::V4MergeXY(temp0, temp1);
		outCol1 = Vec::V4MergeZW(temp0, temp1);
		outCol2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		outCol0 = Vec::V4Scale( outCol0, oneOverDeterminant );
		outCol1 = Vec::V4Scale( outCol1, oneOverDeterminant );
		outCol2 = Vec::V4Scale( outCol2, oneOverDeterminant );
		invMat33.SetColsIntrin128( outCol0, outCol1, outCol2 );

		// Pre-multiply the inverse by the matrix, 'concatMat'.
		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx0, invMat33.GetCol0Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _yyyy0, invMat33.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz0, invMat33.GetCol2Intrin128() );
		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _xxxx1, invMat33.GetCol0Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _yyyy1, invMat33.GetCol1Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _zzzz1, invMat33.GetCol2Intrin128() );
		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V sumTerm2 = Vec::V4Scale( _xxxx2, invMat33.GetCol0Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _yyyy2, invMat33.GetCol1Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _zzzz2, invMat33.GetCol2Intrin128() );

		inoutMat.SetColsIntrin128( sumTerm0, sumTerm1, sumTerm2 );

		//Mat33V invMat33;
		//Invert( invMat33, origTransformMat );
		//Multiply( inoutMat, invMat33, concatMat );
	}

	inline void UnTransformOrtho_Imp33( Mat33V_InOut inoutMat, MAT33V_DECL(origOrthoTransform), MAT33V_DECL2(concat) )
	{
		Mat33V origOrthoTransformMat = MAT33V_ARG_GET(origOrthoTransform);
		Mat33V concatMat = MAT33V_ARG_GET(concat);

#if __XENON
		// Pre-multiply by inverse (which is the transpose in this case).
		// (Pre-multiplying by the transpose is most efficient if we don't actually take the transpose.)
		Vec::Vector_4V _x0 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _y0 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _z0 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _x1 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _y1 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _z1 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _x2 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _y2 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _z2 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol2Intrin128() );
		inoutMat.SetCol0( Vec3V( ScalarV(_x0), ScalarV(_y0), ScalarV(_z0) ) );
		inoutMat.SetCol1( Vec3V( ScalarV(_x1), ScalarV(_y1), ScalarV(_z1) ) );
		inoutMat.SetCol2( Vec3V( ScalarV(_x2), ScalarV(_y2), ScalarV(_z2) ) );
#else
		// Find 3x3 transpose.
		Mat33V transposedA;
		Vec::Vector_4V inCol0 = origOrthoTransformMat.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = origOrthoTransformMat.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = origOrthoTransformMat.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Multiply.
		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx0, transposedA.GetCol0Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _yyyy0, transposedA.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz0, transposedA.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _xxxx1, transposedA.GetCol0Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _yyyy1, transposedA.GetCol1Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _zzzz1, transposedA.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V sumTerm2 = Vec::V4Scale( _xxxx2, transposedA.GetCol0Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _yyyy2, transposedA.GetCol1Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _zzzz2, transposedA.GetCol2Intrin128() );

		inoutMat.SetColsIntrin128( sumTerm0, sumTerm1, sumTerm2 );
#endif
		//inoutMat.SetCol0(UnTransformOrtho( origOrthoTransformMat, concatMat.GetCol0() ));
		//inoutMat.SetCol1(UnTransformOrtho( origOrthoTransformMat, concatMat.GetCol1() ));
		//inoutMat.SetCol2(UnTransformOrtho( origOrthoTransformMat, concatMat.GetCol2() ));
	}

	inline Vec::Vector_4V_Out UnTransformFull_Imp33( MAT33V_DECL(origTransform), Vec::Vector_4V_In_After3Args transformedVect )
	{
		Mat33V origTransformMat = MAT33V_ARG_GET(origTransform);

		// Find the inverse.
		Vec::Vector_4V cross0, cross1, cross2;
		Vec::Vector_4V dotVect;
		Vec::Vector_4V oneOverDeterminant;
		Vec::Vector_4V scaledVec;

		cross2 = Vec::V3Cross( origTransformMat.GetCol0Intrin128(), origTransformMat.GetCol1Intrin128() );
		cross0 = Vec::V3Cross( origTransformMat.GetCol1Intrin128(), origTransformMat.GetCol2Intrin128() );
		cross1 = Vec::V3Cross( origTransformMat.GetCol2Intrin128(), origTransformMat.GetCol0Intrin128() );
		dotVect = Vec::V3DotV( cross2, origTransformMat.GetCol2Intrin128() );
		oneOverDeterminant = Vec::V4Invert( dotVect );
		scaledVec = Vec::V4Scale( transformedVect, oneOverDeterminant );

#if __XENON

		Vec::Vector_4V x = Vec::V3DotV( cross0, scaledVec );
		Vec::Vector_4V y = Vec::V3DotV( cross1, scaledVec );
		Vec::Vector_4V z = Vec::V3DotV( cross2, scaledVec );
		return Vec3V( ScalarV(x), ScalarV(y), ScalarV(z) ).GetIntrin128();

#else

		Vec::Vector_4V outCol0, outCol1, outCol2;

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V tempVect0, tempVect1;
		tempVect0 = Vec::V4MergeXY( cross0, cross2 );
		tempVect1 = Vec::V4MergeZW( cross0, cross2 );
		outCol0 = Vec::V4MergeXY( tempVect0, cross1 );
		outCol1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( tempVect0, cross1 );
		outCol2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( tempVect1, cross1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(cross0, cross2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(cross1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(cross0, cross2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(cross1, _zero);
		outCol0 = Vec::V4MergeXY(temp0, temp1);
		outCol1 = Vec::V4MergeZW(temp0, temp1);
		outCol2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Pre-multiply by the vector, 'transformedVect'*oneOverDeterminant.
		Vec::Vector_4V _xxxx = Vec::V4SplatX( scaledVec );
		Vec::Vector_4V _yyyy = Vec::V4SplatY( scaledVec );
		Vec::Vector_4V _zzzz = Vec::V4SplatZ( scaledVec );

		Vec::Vector_4V sumTerm = Vec::V4Scale( _xxxx, outCol0 );
		sumTerm = Vec::V4AddScaled( sumTerm, _yyyy, outCol1 );
		sumTerm = Vec::V4AddScaled( sumTerm, _zzzz, outCol2 );
		return sumTerm;
#endif
		//Mat33V invMat33;
		//Invert( invMat33, origTransformMat );
		//return Multiply( invMat33, transformedVect );
	}

	inline void Transform_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(transform1), MAT34V_DECL2(transform2) )
	{
		Mat34V transformMat1 = MAT34V_ARG_GET(transform1);
		Mat34V transformMat2 = MAT34V_ARG_GET(transform2);


#if 0//__XENON
		/*
		// Find 3x3 transpose.
		Mat33V transposedA;
		Vec::Vector_4V inCol0 = transformMat1.GetCol0Intrin128();
		Vec::Vector_4V inCol1 = transformMat1.GetCol1Intrin128();
		Vec::Vector_4V inCol2 = transformMat1.GetCol2Intrin128();
		Vec::Vector_4V outVec0, outVec1, outVec2;

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V merged0, merged1;
		merged0 = Vec::V4MergeXY( inCol0, inCol2 );
		merged1 = Vec::V4MergeZW( inCol0, inCol2 );
		outVec0 = Vec::V4MergeXY( merged0, inCol1 );
		outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 );
		outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 );
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
		outVec0 = Vec::V4MergeXY(temp0, temp1);
		outVec1 = Vec::V4MergeZW(temp0, temp1);
		outVec2 = Vec::V4MergeXY(temp2, temp3);
		transposedA.SetColsIntrin128( outVec0, outVec1, outVec2 );
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		// Do dot products.
		Vec::Vector_4V outElem0_0 = Vec::V3DotV( transposedA.GetCol0Intrin128(), transformMat2.GetCol0Intrin128() );
		Vec::Vector_4V outElem1_0 = Vec::V3DotV( transposedA.GetCol1Intrin128(), transformMat2.GetCol0Intrin128() );
		Vec::Vector_4V outElem2_0 = Vec::V3DotV( transposedA.GetCol2Intrin128(), transformMat2.GetCol0Intrin128() );
		Vec3V outCol0 = Vec3V( ScalarV(outElem0_0), ScalarV(outElem1_0), ScalarV(outElem2_0) );

		Vec::Vector_4V outElem0_1 = Vec::V3DotV( transposedA.GetCol0Intrin128(), transformMat2.GetCol1Intrin128() );
		Vec::Vector_4V outElem1_1 = Vec::V3DotV( transposedA.GetCol1Intrin128(), transformMat2.GetCol1Intrin128() );
		Vec::Vector_4V outElem2_1 = Vec::V3DotV( transposedA.GetCol2Intrin128(), transformMat2.GetCol1Intrin128() );
		Vec3V outCol1 = Vec3V( ScalarV(outElem0_1), ScalarV(outElem1_1), ScalarV(outElem2_1) );

		Vec::Vector_4V outElem0_2 = Vec::V3DotV( transposedA.GetCol0Intrin128(), transformMat2.GetCol2Intrin128() );
		Vec::Vector_4V outElem1_2 = Vec::V3DotV( transposedA.GetCol1Intrin128(), transformMat2.GetCol2Intrin128() );
		Vec::Vector_4V outElem2_2 = Vec::V3DotV( transposedA.GetCol2Intrin128(), transformMat2.GetCol2Intrin128() );
		Vec3V outCol2 = Vec3V( ScalarV(outElem0_2), ScalarV(outElem1_2), ScalarV(outElem2_2) );

		Vec::Vector_4V outElem0_3 = Vec::V3DotV( transposedA.GetCol0Intrin128(), transformMat2.GetCol3Intrin128() );
		Vec::Vector_4V outElem1_3 = Vec::V3DotV( transposedA.GetCol1Intrin128(), transformMat2.GetCol3Intrin128() );
		Vec::Vector_4V outElem2_3 = Vec::V3DotV( transposedA.GetCol2Intrin128(), transformMat2.GetCol3Intrin128() );
		Vec3V outCol3 = Vec3V( ScalarV(outElem0_3), ScalarV(outElem1_3), ScalarV(outElem2_3) );
		outCol3 = Add( outCol3, transformMat1.GetCol3() );

		inoutMat = Mat34V( outCol0, outCol1, outCol2, outCol3 );
		*/
#else
		Vec::Vector_4V resultVec0;
		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( transformMat2.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( transformMat2.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( transformMat2.GetCol0Intrin128() );
		resultVec0 = Vec::V4Scale( _xxxx0, transformMat1.GetCol0Intrin128() );
		resultVec0 = Vec::V4AddScaled( resultVec0, _yyyy0, transformMat1.GetCol1Intrin128() );
		resultVec0 = Vec::V4AddScaled( resultVec0, _zzzz0, transformMat1.GetCol2Intrin128() );

		Vec::Vector_4V resultVec1;
		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( transformMat2.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( transformMat2.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( transformMat2.GetCol1Intrin128() );
		resultVec1 = Vec::V4Scale( _xxxx1, transformMat1.GetCol0Intrin128() );
		resultVec1 = Vec::V4AddScaled( resultVec1, _yyyy1, transformMat1.GetCol1Intrin128() );
		resultVec1 = Vec::V4AddScaled( resultVec1, _zzzz1, transformMat1.GetCol2Intrin128() );

		Vec::Vector_4V resultVec2;
		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( transformMat2.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( transformMat2.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( transformMat2.GetCol2Intrin128() );
		resultVec2 = Vec::V4Scale( _xxxx2, transformMat1.GetCol0Intrin128() );
		resultVec2 = Vec::V4AddScaled( resultVec2, _yyyy2, transformMat1.GetCol1Intrin128() );
		resultVec2 = Vec::V4AddScaled( resultVec2, _zzzz2, transformMat1.GetCol2Intrin128() );

		Vec::Vector_4V resultVec3;
		Vec::Vector_4V _xxxx3 = Vec::V4SplatX( transformMat2.GetCol3Intrin128() );
		Vec::Vector_4V _yyyy3 = Vec::V4SplatY( transformMat2.GetCol3Intrin128() );
		Vec::Vector_4V _zzzz3 = Vec::V4SplatZ( transformMat2.GetCol3Intrin128() );
		resultVec3 = Vec::V4AddScaled( transformMat1.GetCol3Intrin128(), _xxxx3, transformMat1.GetCol0Intrin128() );
		resultVec3 = Vec::V4AddScaled( resultVec3, _yyyy3, transformMat1.GetCol1Intrin128() );
		resultVec3 = Vec::V4AddScaled( resultVec3, _zzzz3, transformMat1.GetCol2Intrin128() );

		//Vec3V outCol0 = Transform3x3( transformMat1, transformMat2.GetCol0() );
		//Vec3V outCol1 = Transform3x3( transformMat1, transformMat2.GetCol1() );
		//Vec3V outCol2 = Transform3x3( transformMat1, transformMat2.GetCol2() );
		//Vec3V outCol3 = Transform( transformMat1, transformMat2.GetCol3() );

		inoutMat.SetColsIntrin128( resultVec0, resultVec1, resultVec2, resultVec3 );

#endif
	}

	inline void UnTransformFull_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(origTransform), MAT34V_DECL2(concat) )
	{
		Mat34V origTransformMat = MAT34V_ARG_GET(origTransform);
		Mat34V concatMat = MAT34V_ARG_GET(concat);

		// Subtract the translation.
		Vec::Vector_4V tempCol3 = Vec::V4Subtract( concatMat.GetCol3Intrin128(), origTransformMat.GetCol3Intrin128() );

		// Calculate 3x3 inverse.
		Mat33V invMat33;
		Vec::Vector_4V cross0, cross1, cross2;
		Vec::Vector_4V dotVect;
		Vec::Vector_4V oneOverDeterminant;
		Vec::Vector_4V outCol0, outCol1, outCol2;

		cross2 = Vec::V3Cross( origTransformMat.GetCol0Intrin128(), origTransformMat.GetCol1Intrin128() );
		cross0 = Vec::V3Cross( origTransformMat.GetCol1Intrin128(), origTransformMat.GetCol2Intrin128() );
		cross1 = Vec::V3Cross( origTransformMat.GetCol2Intrin128(), origTransformMat.GetCol0Intrin128() );
		dotVect = Vec::V3DotV( cross2, origTransformMat.GetCol2Intrin128() );
		oneOverDeterminant = Vec::V4Invert( dotVect );

#	if !USE_ALTERNATE_3X3_TRANSPOSE
		Vec::Vector_4V tempVect0, tempVect1;
		tempVect0 = Vec::V4MergeXY( cross0, cross2 );
		tempVect1 = Vec::V4MergeZW( cross0, cross2 );
		outCol0 = Vec::V4MergeXY( tempVect0, cross1 );
		outCol1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( tempVect0, cross1 );
		outCol2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( tempVect1, cross1 );
#	else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V temp0 = Vec::V4MergeXY(cross0, cross2);
		Vec::Vector_4V temp1 = Vec::V4MergeXY(cross1, _zero);
		Vec::Vector_4V temp2 = Vec::V4MergeZW(cross0, cross2);
		Vec::Vector_4V temp3 = Vec::V4MergeZW(cross1, _zero);
		outCol0 = Vec::V4MergeXY(temp0, temp1);
		outCol1 = Vec::V4MergeZW(temp0, temp1);
		outCol2 = Vec::V4MergeXY(temp2, temp3);
#	endif // !USE_ALTERNATE_3X3_TRANSPOSE

		outCol0 = Vec::V4Scale( outCol0, oneOverDeterminant );
		outCol1 = Vec::V4Scale( outCol1, oneOverDeterminant );
		outCol2 = Vec::V4Scale( outCol2, oneOverDeterminant );
		invMat33.SetColsIntrin128( outCol0, outCol1, outCol2 );

		// Pre-multiply by inverse.
		Vec::Vector_4V _xxxx0 = Vec::V4SplatX( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _yyyy0 = Vec::V4SplatY( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _zzzz0 = Vec::V4SplatZ( concatMat.GetCol0Intrin128() );
		Vec::Vector_4V sumTerm0 = Vec::V4Scale( _xxxx0, invMat33.GetCol0Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _yyyy0, invMat33.GetCol1Intrin128() );
		sumTerm0 = Vec::V4AddScaled( sumTerm0, _zzzz0, invMat33.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx1 = Vec::V4SplatX( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _yyyy1 = Vec::V4SplatY( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _zzzz1 = Vec::V4SplatZ( concatMat.GetCol1Intrin128() );
		Vec::Vector_4V sumTerm1 = Vec::V4Scale( _xxxx1, invMat33.GetCol0Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _yyyy1, invMat33.GetCol1Intrin128() );
		sumTerm1 = Vec::V4AddScaled( sumTerm1, _zzzz1, invMat33.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx2 = Vec::V4SplatX( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _yyyy2 = Vec::V4SplatY( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _zzzz2 = Vec::V4SplatZ( concatMat.GetCol2Intrin128() );
		Vec::Vector_4V sumTerm2 = Vec::V4Scale( _xxxx2, invMat33.GetCol0Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _yyyy2, invMat33.GetCol1Intrin128() );
		sumTerm2 = Vec::V4AddScaled( sumTerm2, _zzzz2, invMat33.GetCol2Intrin128() );

		Vec::Vector_4V _xxxx3 = Vec::V4SplatX( tempCol3 );
		Vec::Vector_4V _yyyy3 = Vec::V4SplatY( tempCol3 );
		Vec::Vector_4V _zzzz3 = Vec::V4SplatZ( tempCol3 );
		Vec::Vector_4V sumTerm3 = Vec::V4Scale( _xxxx3, invMat33.GetCol0Intrin128() );
		sumTerm3 = Vec::V4AddScaled( sumTerm3, _yyyy3, invMat33.GetCol1Intrin128() );
		sumTerm3 = Vec::V4AddScaled( sumTerm3, _zzzz3, invMat33.GetCol2Intrin128() );

		//Mat33V invMat33;
		//Invert( invMat33, Mat33V( origTransformMat.GetCol0Intrin128(), origTransformMat.GetCol1Intrin128(), origTransformMat.GetCol2Intrin128() ) );
		//Vec3V outCol0 = Multiply( invMat33, concatMat.GetCol0() );
		//Vec3V outCol1 = Multiply( invMat33, concatMat.GetCol1() );
		//Vec3V outCol2 = Multiply( invMat33, concatMat.GetCol2() );
		//Vec3V outCol3 = Multiply( invMat33, Vec3V(tempCol3) );

		inoutMat.SetColsIntrin128( sumTerm0, sumTerm1, sumTerm2, sumTerm3 );
	}

	inline void UnTransformOrtho_Imp34( Mat34V_InOut inoutMat, MAT34V_DECL(origOrthoTransform), MAT34V_DECL2(concat) )
	{
		Mat34V origOrthoTransformMat = MAT34V_ARG_GET(origOrthoTransform);
		Mat34V concatMat = MAT34V_ARG_GET(concat);

		// Pre-multiply by inverse (which is the transpose in this case).
		// (Pre-multiplying by the transpose is most efficient if we don't actually take the transpose.)

		// Subtract the translation.
		Vec::Vector_4V tempPoint = Vec::V4Subtract( concatMat.GetCol3Intrin128(), origOrthoTransformMat.GetCol3Intrin128() );

		Vec::Vector_4V _x0 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _y0 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _z0 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol0Intrin128() );
		Vec::Vector_4V _x1 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _y1 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _z1 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol1Intrin128() );
		Vec::Vector_4V _x2 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _y2 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _z2 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), concatMat.GetCol2Intrin128() );
		Vec::Vector_4V _x3 = Vec::V3DotV( origOrthoTransformMat.GetCol0Intrin128(), tempPoint );
		Vec::Vector_4V _y3 = Vec::V3DotV( origOrthoTransformMat.GetCol1Intrin128(), tempPoint );
		Vec::Vector_4V _z3 = Vec::V3DotV( origOrthoTransformMat.GetCol2Intrin128(), tempPoint );

		inoutMat.SetCol0( Vec3V( ScalarV(_x0), ScalarV(_y0), ScalarV(_z0) ) );
		inoutMat.SetCol1( Vec3V( ScalarV(_x1), ScalarV(_y1), ScalarV(_z1) ) );
		inoutMat.SetCol2( Vec3V( ScalarV(_x2), ScalarV(_y2), ScalarV(_z2) ) );
		inoutMat.SetCol3( Vec3V( ScalarV(_x3), ScalarV(_y3), ScalarV(_z3) ) );

		//Vec3V outCol0 = UnTransform3x3Ortho( origOrthoTransformMat, concatMat.GetCol0() );
		//Vec3V outCol1 = UnTransform3x3Ortho( origOrthoTransformMat, concatMat.GetCol1() );
		//Vec3V outCol2 = UnTransform3x3Ortho( origOrthoTransformMat, concatMat.GetCol2() );
		//Vec3V outCol3 = UnTransformOrtho( origOrthoTransformMat, concatMat.GetCol3() );
	}

	inline Vec::Vector_4V_Out QuatVFromEulers_Imp(Vec3V_In radianAngles, Vec::Vector_4V_In sign0, Vec::Vector_4V_In sign1 )
	{
		using namespace Vec;
		Vector_4V angles = V4Scale(radianAngles.GetIntrin128(), V4VConstant(V_HALF));
		Vector_4V sinVect, cosVect;
		V4SinAndCos(sinVect, cosVect, angles);
		Vector_4V cxcxcxcx = V4SplatX(cosVect);
		Vector_4V sxsxsxsx = V4SplatX(sinVect);
		Vector_4V sysycycy = V4PermuteTwo<Y1,Y1,Y2,Y2>(sinVect, cosVect);
		Vector_4V cycysysy = V4PermuteTwo<Y1,Y1,Y2,Y2>(cosVect, sinVect);
		Vector_4V szczszcz = V4PermuteTwo<Z1,Z2,Z1,Z2>(sinVect, cosVect);
		Vector_4V czszczsz = V4PermuteTwo<Z1,Z2,Z1,Z2>(cosVect, sinVect);
		Vector_4V lhs = V4Xor(sign0, V4Scale(V4Scale(cxcxcxcx, sysycycy), szczszcz));
		Vector_4V rhs = V4Xor(sign1, V4Scale(V4Scale(sxsxsxsx, cycysysy), czszczsz));
		return V4Add(lhs, rhs);
	}
	inline Vec::Vector_4V_Out QuatVFromEulers_Imp( Vec3V_In radianAngles, EulerAngleOrder order )
	{
		using namespace Vec;
		const u32 P = U32_ZERO;	const u32 N = U32_NEGZERO;
		static const ALIGNAS(16) u32 constants[6][2][4]  = {
			{{N,P,P,P},{P,P,N,P}}, {{P,P,P,P},{P,P,N,N}}, {{N,P,P,P},{P,P,P,N}},
			{{N,P,P,P},{P,N,P,P}}, {{P,P,P,P},{P,N,N,P}}, {{P,P,P,P},{P,N,P,N}}};		
		const Vector_4V* c = reinterpret_cast<const Vector_4V*>(constants[order]);
		return QuatVFromEulers_Imp(radianAngles, c[0], c[1]);
	}
	inline Vec::Vector_4V_Out QuatVToEulers_Imp( Vec4V_In quatIn, ScalarV_In sign )
	{
		using namespace Vec;
		Vector_4V sqrt2 = V4VConstant(V_SQRT_TWO);
		Vector_4V qin = quatIn.GetIntrin128();
		Vector_4V q = V4Scale(qin, sqrt2);
		Vector_4V qp = V4Xor(sign.GetIntrin128(), q);
		Vector_4V qp2 = V4Scale(qp, qp);
		Vector_4V qw = V4Scale(qp, V4SplatW(q)); 
		Vector_4V qy = V4Scale(qp, V4SplatY(qp)); 
		Vector_4V y = V4Xor(sign.GetIntrin128(), V4Subtract(qw, V4Permute<Z,W,X,Y>(qy)));
		Vector_4V x = V4Subtract(V4VConstant(V_ONE), V4Add(V4SplatY(qp2), qp2));
		Vector_4V z = V4Xor(sign.GetIntrin128(), V4AddScaled(qw, V4SplatX(qp), V4SplatZ(qp)));
		Vector_4V axy = V4Arctan2(y,x);
		Vector_4V az = V4Arcsin(z);
		return V4PermuteTwo<X1,Y2,Z1,W1>(axy, az);		
	}
	template<u32 x, u32 y, u32 z, u32 sign>
	__forceinline Vec::Vector_4V_Out QuatVToEulers_Imp( QuatV_In quatIn )
	{
		using namespace Vec;
		Vec4V in(Vec4V(quatIn).Get<x,y,z,Vec::W>());
		Vector_4V out = QuatVToEulers_Imp(in, sign ? ScalarV(V_80000000) : ScalarV(V_ZERO));
		return V4Permute<x,y,z,Vec::W>(out);
	}
	inline Vec::Vector_4V_Out QuatVToEulers_Imp(QuatV_In quatIn, EulerAngleOrder order )
	{
		using namespace Vec;
#if __PS3 || __XENON
		// use a table lookup for the permute & sign vectors on ps3/360
		const u32 P = U32_ZERO;	const u32 N = U32_NEGZERO;
		static const ALIGNAS(16) u32 constants[6][2][4]  = {
			{{Z,Y,X,W}, {N,N,N,N}},	{{X,Z,Y,W}, {P,P,P,P}},	{{Y,X,Z,W}, {P,P,P,P}},
			{{X,Z,Y,W}, {N,N,N,N}},	{{Y,X,Z,W}, {N,N,N,N}},	{{Z,Y,X,W}, {P,P,P,P}}};
		const Vector_4V* c = reinterpret_cast<const Vector_4V*>(constants[order]);
		Vector_4V permute = c[0];
		Vector_4V sign = c[1];
		Vec4V in(V4Permute(quatIn.GetIntrin128(), permute));
		Vector_4V out = QuatVToEulers_Imp(in, ScalarV(sign));
		return V4Permute(out, permute);
#else
		switch (order) {
		case EULER_XYZ: return QuatVToEulers_Imp<Z,Y,X,1>(quatIn);
		case EULER_ZYX: return QuatVToEulers_Imp<Z,Y,X,0>(quatIn);
		case EULER_YZX: return QuatVToEulers_Imp<X,Z,Y,1>(quatIn);
		case EULER_XZY: return QuatVToEulers_Imp<X,Z,Y,0>(quatIn);
		case EULER_ZXY: return QuatVToEulers_Imp<Y,X,Z,1>(quatIn);
		case EULER_YXZ: return QuatVToEulers_Imp<Y,X,Z,0>(quatIn);
		default:		return QuatVToEulers_Imp<Z,Y,X,1>(quatIn); // default is XYZ - but should never get here.
		}
#endif
	}
} // namespace Imp
	

} // namespace rage
