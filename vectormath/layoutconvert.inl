namespace rage
{
	// SoA: 1-tuple x 4 --> four vecsplattedv's
	__forceinline
	void ToAoS( ScalarV_InOut outVec0, ScalarV_InOut outVec1, ScalarV_InOut outVec2, ScalarV_InOut outVec3, SoA_ScalarV_In inVec )
	{
		outVec0 = ScalarV( Vec::V4SplatX( inVec.GetIntrin128() ) );
		outVec1 = ScalarV( Vec::V4SplatY( inVec.GetIntrin128() ) );
		outVec2 = ScalarV( Vec::V4SplatZ( inVec.GetIntrin128() ) );
		outVec3 = ScalarV( Vec::V4SplatW( inVec.GetIntrin128() ) );
	}

	// SoA: 1-tuple x 4 --> one vec4v
	__forceinline
	void ToAoS( Vec4V_InOut outVec, SoA_ScalarV_In inVec )
	{
		outVec.SetIntrin128( inVec.GetIntrin128() );
	}

	// SoA: 2-tuple x 4 --> four vec2v's
	__forceinline
	void ToAoS( Vec2V_InOut outVec0, Vec2V_InOut outVec1, Vec2V_InOut outVec2, Vec2V_InOut outVec3, SoA_Vec2V_In inVec )
	{
		Vec::Vector_4V _x0_y0_x1_y1 = Vec::V4MergeXY( inVec.GetXIntrin128(), inVec.GetYIntrin128() );
		Vec::Vector_4V _x1_y1_x0_y0 = Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( _x0_y0_x1_y1 );
		Vec::Vector_4V _x2_y2_x3_y3 = Vec::V4MergeZW( inVec.GetXIntrin128(), inVec.GetYIntrin128() );
		Vec::Vector_4V _x3_y3_x2_y2 = Vec::V4Permute<Vec::Z,Vec::W,Vec::X,Vec::Y>( _x2_y2_x3_y3 );
		outVec0 = Vec2V( _x0_y0_x1_y1 );
		outVec1 = Vec2V( _x1_y1_x0_y0 );
		outVec2 = Vec2V( _x2_y2_x3_y3 );
		outVec3 = Vec2V( _x3_y3_x2_y2 );
	}

	// SoA: 3-tuple x 4 --> four vec3v's
	__forceinline
	void ToAoS( Vec3V_InOut outVec0, Vec3V_InOut outVec1, Vec3V_InOut outVec2, Vec3V_InOut outVec3, SoA_Vec3V_In inVec )
	{
		// Just do a 4x4 matrix transpose.
		Mat44V mat( inVec.GetXIntrin128(), inVec.GetYIntrin128(), inVec.GetZIntrin128(), Vec::V4VConstant(V_ZERO) );
		Transpose( mat, mat );
		outVec0 = Vec3V( mat.GetCol0Intrin128() );
		outVec1 = Vec3V( mat.GetCol1Intrin128() );
		outVec2 = Vec3V( mat.GetCol2Intrin128() );
		outVec3 = Vec3V( mat.GetCol3Intrin128() );
	}

	// SoA: 4-tuple x 4 --> four vec4v's
	__forceinline
	void ToAoS( Vec4V_InOut outVec0, Vec4V_InOut outVec1, Vec4V_InOut outVec2, Vec4V_InOut outVec3, SoA_Vec4V_In inVec )
	{
		// Just do a 4x4 matrix transpose.
		Mat44V mat( inVec.GetXIntrin128(), inVec.GetYIntrin128(), inVec.GetZIntrin128(), inVec.GetWIntrin128() );
		Transpose( mat, mat );
		outVec0 = Vec4V( mat.GetCol0Intrin128() );
		outVec1 = Vec4V( mat.GetCol1Intrin128() );
		outVec2 = Vec4V( mat.GetCol2Intrin128() );
		outVec3 = Vec4V( mat.GetCol3Intrin128() );
	}

	// SoA: 4-tuple x 4 --> four quatv's
	__forceinline
	void ToAoS( QuatV_InOut outQuat0, QuatV_InOut outQuat1, QuatV_InOut outQuat2, QuatV_InOut outQuat3, SoA_QuatV_In inQuat )
	{
		// Just do a 4x4 matrix transpose.
		Mat44V mat( inQuat.GetXIntrin128(), inQuat.GetYIntrin128(), inQuat.GetZIntrin128(), inQuat.GetWIntrin128() );
		Transpose( mat, mat );
		outQuat0 = QuatV( mat.GetCol0Intrin128() );
		outQuat1 = QuatV( mat.GetCol1Intrin128() );
		outQuat2 = QuatV( mat.GetCol2Intrin128() );
		outQuat3 = QuatV( mat.GetCol3Intrin128() );
	}

	// SoA: 9-tuple x 4 --> four mat33v's
	__forceinline
	void ToAoS( Mat33V_InOut outMat0, Mat33V_InOut outMat1, Mat33V_InOut outMat2, Mat33V_InOut outMat3, SoA_Mat33V_In inMat )
	{
		// Just do three 4x4 matrix transposes.
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Mat44V matCol0s( inMat.GetM00Intrin128(), inMat.GetM10Intrin128(), inMat.GetM20Intrin128(), _zero );
		Mat44V matCol1s( inMat.GetM01Intrin128(), inMat.GetM11Intrin128(), inMat.GetM21Intrin128(), _zero );
		Mat44V matCol2s( inMat.GetM02Intrin128(), inMat.GetM12Intrin128(), inMat.GetM22Intrin128(), _zero );
		Transpose( matCol0s, matCol0s );		
		Transpose( matCol1s, matCol1s );
		Transpose( matCol2s, matCol2s );

		outMat0 = Mat33V( matCol0s.GetCol0Intrin128(), matCol1s.GetCol0Intrin128(), matCol2s.GetCol0Intrin128() );
		outMat1 = Mat33V( matCol0s.GetCol1Intrin128(), matCol1s.GetCol1Intrin128(), matCol2s.GetCol1Intrin128() );
		outMat2 = Mat33V( matCol0s.GetCol2Intrin128(), matCol1s.GetCol2Intrin128(), matCol2s.GetCol2Intrin128() );
		outMat3 = Mat33V( matCol0s.GetCol3Intrin128(), matCol1s.GetCol3Intrin128(), matCol2s.GetCol3Intrin128() );
	}

	// SoA: 12-tuple x 4 --> four mat34v's
	__forceinline
	void ToAoS( Mat34V_InOut outMat0, Mat34V_InOut outMat1, Mat34V_InOut outMat2, Mat34V_InOut outMat3, SoA_Mat34V_In inMat )
	{
		// Just do four 4x4 matrix transposes.
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Mat44V matCol0s( inMat.GetM00Intrin128(), inMat.GetM10Intrin128(), inMat.GetM20Intrin128(), _zero );
		Mat44V matCol1s( inMat.GetM01Intrin128(), inMat.GetM11Intrin128(), inMat.GetM21Intrin128(), _zero );
		Mat44V matCol2s( inMat.GetM02Intrin128(), inMat.GetM12Intrin128(), inMat.GetM22Intrin128(), _zero );
		Mat44V matCol3s( inMat.GetM03Intrin128(), inMat.GetM13Intrin128(), inMat.GetM23Intrin128(), _zero );
		Transpose( matCol0s, matCol0s );		
		Transpose( matCol1s, matCol1s );
		Transpose( matCol2s, matCol2s );
		Transpose( matCol3s, matCol3s );

		outMat0 = Mat34V( matCol0s.GetCol0Intrin128(), matCol1s.GetCol0Intrin128(), matCol2s.GetCol0Intrin128(), matCol3s.GetCol0Intrin128() );
		outMat1 = Mat34V( matCol0s.GetCol1Intrin128(), matCol1s.GetCol1Intrin128(), matCol2s.GetCol1Intrin128(), matCol3s.GetCol1Intrin128() );
		outMat2 = Mat34V( matCol0s.GetCol2Intrin128(), matCol1s.GetCol2Intrin128(), matCol2s.GetCol2Intrin128(), matCol3s.GetCol2Intrin128() );
		outMat3 = Mat34V( matCol0s.GetCol3Intrin128(), matCol1s.GetCol3Intrin128(), matCol2s.GetCol3Intrin128(), matCol3s.GetCol3Intrin128() );
	}

	// SoA: 16-tuple x 4 --> four mat44v's
	__forceinline
	void ToAoS( Mat44V_InOut outMat0, Mat44V_InOut outMat1, Mat44V_InOut outMat2, Mat44V_InOut outMat3, SoA_Mat44V_In inMat )
	{
		// Just do four 4x4 matrix transposes.
		Mat44V matCol0s( inMat.GetM00Intrin128(), inMat.GetM10Intrin128(), inMat.GetM20Intrin128(), inMat.GetM30Intrin128() );
		Mat44V matCol1s( inMat.GetM01Intrin128(), inMat.GetM11Intrin128(), inMat.GetM21Intrin128(), inMat.GetM31Intrin128() );
		Mat44V matCol2s( inMat.GetM02Intrin128(), inMat.GetM12Intrin128(), inMat.GetM22Intrin128(), inMat.GetM32Intrin128() );
		Mat44V matCol3s( inMat.GetM03Intrin128(), inMat.GetM13Intrin128(), inMat.GetM23Intrin128(), inMat.GetM33Intrin128() );
		Transpose( matCol0s, matCol0s );		
		Transpose( matCol1s, matCol1s );
		Transpose( matCol2s, matCol2s );
		Transpose( matCol3s, matCol3s );

		outMat0 = Mat44V( matCol0s.GetCol0Intrin128(), matCol1s.GetCol0Intrin128(), matCol2s.GetCol0Intrin128(), matCol3s.GetCol0Intrin128() );
		outMat1 = Mat44V( matCol0s.GetCol1Intrin128(), matCol1s.GetCol1Intrin128(), matCol2s.GetCol1Intrin128(), matCol3s.GetCol1Intrin128() );
		outMat2 = Mat44V( matCol0s.GetCol2Intrin128(), matCol1s.GetCol2Intrin128(), matCol2s.GetCol2Intrin128(), matCol3s.GetCol2Intrin128() );
		outMat3 = Mat44V( matCol0s.GetCol3Intrin128(), matCol1s.GetCol3Intrin128(), matCol2s.GetCol3Intrin128(), matCol3s.GetCol3Intrin128() );
	}

	// AoS: four vecsplattedv's --> 1-tuple x 4
	__forceinline
	void ToSoA( SoA_ScalarV_InOut outVec, ScalarV_In inVec0, ScalarV_In inVec1, ScalarV_In inVec2, ScalarV_In inVec3 )
	{
		// Use a Vec4V constructor to combine 4 ScalarV's into one 4-component vec in the most optimal way possible.
		Vec4V temp = Vec4V( inVec0, inVec1, inVec2, inVec3 );
		outVec = SoA_ScalarV( temp.GetIntrin128() );
	}

	// AoS: one vec4v's --> 1-tuple x 4
	__forceinline
	void ToSoA( SoA_ScalarV_InOut outVec, Vec4V_In inVec )
	{
		outVec.SetIntrin128( inVec.GetIntrin128() );
	}

	// AoS: four vec2v's --> 2-tuple x 4
	__forceinline
	void ToSoA( SoA_Vec2V_InOut outVec, Vec2V_In inVec0, Vec2V_In inVec1, Vec2V_In inVec2, Vec2V_In inVec3 )
	{
		Vec::Vector_4V _x0_x2_y0_y2 = Vec::V4MergeXY( inVec0.GetIntrin128(), inVec2.GetIntrin128() );
		Vec::Vector_4V _x1_x3_y1_y3 = Vec::V4MergeXY( inVec1.GetIntrin128(), inVec3.GetIntrin128() );
		Vec::Vector_4V _x0_x1_x2_x3 = Vec::V4MergeXY( _x0_x2_y0_y2, _x1_x3_y1_y3 );
		Vec::Vector_4V _y0_y1_y2_y3 = Vec::V4MergeZW( _x0_x2_y0_y2, _x1_x3_y1_y3 );
		outVec = SoA_Vec2V( _x0_x1_x2_x3, _y0_y1_y2_y3 );
	}

	// AoS: four vec3v's --> 3-tuple x 4
	__forceinline
	void ToSoA( SoA_Vec3V_InOut outVec, Vec3V_In inVec0, Vec3V_In inVec1, Vec3V_In inVec2, Vec3V_In inVec3 )
	{
		Vec::Vector_4V _x0_x2_y0_y2 = Vec::V4MergeXY( inVec0.GetIntrin128(), inVec2.GetIntrin128() );
		Vec::Vector_4V _x1_x3_y1_y3 = Vec::V4MergeXY( inVec1.GetIntrin128(), inVec3.GetIntrin128() );
		Vec::Vector_4V _z0_z2_w0_w2 = Vec::V4MergeZW( inVec0.GetIntrin128(), inVec2.GetIntrin128() );
		Vec::Vector_4V _z1_z3_w1_w3 = Vec::V4MergeZW( inVec1.GetIntrin128(), inVec3.GetIntrin128() );
		Vec::Vector_4V _x0_x1_x2_x3 = Vec::V4MergeXY( _x0_x2_y0_y2, _x1_x3_y1_y3 );
		Vec::Vector_4V _y0_y1_y2_y3 = Vec::V4MergeZW( _x0_x2_y0_y2, _x1_x3_y1_y3 );		
		Vec::Vector_4V _z0_z1_z2_z3 = Vec::V4MergeXY( _z0_z2_w0_w2, _z1_z3_w1_w3 );
		outVec = SoA_Vec3V( _x0_x1_x2_x3, _y0_y1_y2_y3, _z0_z1_z2_z3 );
	}

	// AoS: four vec4v's --> 4-tuple x 4
	__forceinline
	void ToSoA( SoA_Vec4V_InOut outVec, Vec4V_In inVec0, Vec4V_In inVec1, Vec4V_In inVec2, Vec4V_In inVec3 )
	{
		Vec::Vector_4V _x0_x2_y0_y2 = Vec::V4MergeXY( inVec0.GetIntrin128(), inVec2.GetIntrin128() );
		Vec::Vector_4V _x1_x3_y1_y3 = Vec::V4MergeXY( inVec1.GetIntrin128(), inVec3.GetIntrin128() );
		Vec::Vector_4V _z0_z2_w0_w2 = Vec::V4MergeZW( inVec0.GetIntrin128(), inVec2.GetIntrin128() );
		Vec::Vector_4V _z1_z3_w1_w3 = Vec::V4MergeZW( inVec1.GetIntrin128(), inVec3.GetIntrin128() );
		Vec::Vector_4V _x0_x1_x2_x3 = Vec::V4MergeXY( _x0_x2_y0_y2, _x1_x3_y1_y3 );
		Vec::Vector_4V _y0_y1_y2_y3 = Vec::V4MergeZW( _x0_x2_y0_y2, _x1_x3_y1_y3 );		
		Vec::Vector_4V _z0_z1_z2_z3 = Vec::V4MergeXY( _z0_z2_w0_w2, _z1_z3_w1_w3 );
		Vec::Vector_4V _w0_w1_w2_w3 = Vec::V4MergeZW( _z0_z2_w0_w2, _z1_z3_w1_w3 );
		outVec = SoA_Vec4V( _x0_x1_x2_x3, _y0_y1_y2_y3, _z0_z1_z2_z3, _w0_w1_w2_w3 );
	}

	// AoS: four quatv's --> 4-tuple x 4
	__forceinline
	void ToSoA( SoA_QuatV_InOut outQuat, QuatV_In inQuat0, QuatV_In inQuat1, QuatV_In inQuat2, QuatV_In inQuat3 )
	{
		// Just do a 4x4 matrix transpose.
		Mat44V tempMat = Mat44V( inQuat0.GetIntrin128(), inQuat1.GetIntrin128(), inQuat2.GetIntrin128(), inQuat3.GetIntrin128() );
		Transpose( tempMat, tempMat );
		outQuat = SoA_QuatV( tempMat.GetCol0Intrin128(), tempMat.GetCol1Intrin128(), tempMat.GetCol2Intrin128(), tempMat.GetCol3Intrin128() );
	}

	// AoS: four mat33v's --> 9-tuple x 4
	__forceinline
	void ToSoA( SoA_Mat33V_InOut outMat, Mat33V_In inMat0, Mat33V_In inMat1, Mat33V_In inMat2, Mat33V_In inMat3 )
	{
		// Just do three 4x4 matrix transposes.
		Mat44V matCol0 = Mat44V( inMat0.GetCol0Intrin128(), inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), inMat3.GetCol0Intrin128() );
		Mat44V matCol1 = Mat44V( inMat0.GetCol1Intrin128(), inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), inMat3.GetCol1Intrin128() );
		Mat44V matCol2 = Mat44V( inMat0.GetCol2Intrin128(), inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), inMat3.GetCol2Intrin128() );
		Transpose( matCol0, matCol0 );
		Transpose( matCol1, matCol1 );
		Transpose( matCol2, matCol2 );
		outMat =  SoA_Mat33V(	matCol0.GetCol0Intrin128(), matCol0.GetCol1Intrin128(), matCol0.GetCol2Intrin128(),
								matCol1.GetCol0Intrin128(), matCol1.GetCol1Intrin128(), matCol1.GetCol2Intrin128(),
								matCol2.GetCol0Intrin128(), matCol2.GetCol1Intrin128(), matCol2.GetCol2Intrin128()	);
	}

	// AoS: four mat34v's --> 12-tuple x 4
	__forceinline
	void ToSoA( SoA_Mat34V_InOut outMat, Mat34V_In inMat0, Mat34V_In inMat1, Mat34V_In inMat2, Mat34V_In inMat3 )
	{
		// Just do four 4x4 matrix transposes.
		Mat44V matCol0 = Mat44V( inMat0.GetCol0Intrin128(), inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), inMat3.GetCol0Intrin128() );
		Mat44V matCol1 = Mat44V( inMat0.GetCol1Intrin128(), inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), inMat3.GetCol1Intrin128() );
		Mat44V matCol2 = Mat44V( inMat0.GetCol2Intrin128(), inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), inMat3.GetCol2Intrin128() );
		Mat44V matCol3 = Mat44V( inMat0.GetCol3Intrin128(), inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), inMat3.GetCol3Intrin128() );
		Transpose( matCol0, matCol0 );
		Transpose( matCol1, matCol1 );
		Transpose( matCol2, matCol2 );
		Transpose( matCol3, matCol3 );
		outMat =  SoA_Mat34V(	matCol0.GetCol0Intrin128(), matCol0.GetCol1Intrin128(), matCol0.GetCol2Intrin128(),
								matCol1.GetCol0Intrin128(), matCol1.GetCol1Intrin128(), matCol1.GetCol2Intrin128(),
								matCol2.GetCol0Intrin128(), matCol2.GetCol1Intrin128(), matCol2.GetCol2Intrin128(),
								matCol3.GetCol0Intrin128(), matCol3.GetCol1Intrin128(), matCol3.GetCol2Intrin128()	);
	}

	// AoS: one mat34v --> 12-tuple x 4
	__forceinline
	void ToSoA( SoA_Mat34V_InOut outMat, Mat34V_In inMat )
	{
		outMat = SoA_Mat34V( inMat.GetCol0().GetX().GetIntrin128(), inMat.GetCol0().GetY().GetIntrin128(), inMat.GetCol0().GetZ().GetIntrin128(), 
							  inMat.GetCol1().GetX().GetIntrin128(), inMat.GetCol1().GetY().GetIntrin128(), inMat.GetCol1().GetZ().GetIntrin128(), 
							  inMat.GetCol2().GetX().GetIntrin128(), inMat.GetCol2().GetY().GetIntrin128(), inMat.GetCol2().GetZ().GetIntrin128(), 
							  inMat.GetCol3().GetX().GetIntrin128(), inMat.GetCol3().GetY().GetIntrin128(), inMat.GetCol3().GetZ().GetIntrin128() );
	}

	// AoS: four mat44v's --> 16-tuple x 4
	__forceinline
	void ToSoA( SoA_Mat44V_InOut outMat, Mat44V_In inMat0, Mat44V_In inMat1, Mat44V_In inMat2, Mat44V_In inMat3 )
	{
		// Just do four 4x4 matrix transposes.
		Mat44V matCol0 = Mat44V( inMat0.GetCol0Intrin128(), inMat1.GetCol0Intrin128(), inMat2.GetCol0Intrin128(), inMat3.GetCol0Intrin128() );
		Mat44V matCol1 = Mat44V( inMat0.GetCol1Intrin128(), inMat1.GetCol1Intrin128(), inMat2.GetCol1Intrin128(), inMat3.GetCol1Intrin128() );
		Mat44V matCol2 = Mat44V( inMat0.GetCol2Intrin128(), inMat1.GetCol2Intrin128(), inMat2.GetCol2Intrin128(), inMat3.GetCol2Intrin128() );
		Mat44V matCol3 = Mat44V( inMat0.GetCol3Intrin128(), inMat1.GetCol3Intrin128(), inMat2.GetCol3Intrin128(), inMat3.GetCol3Intrin128() );
		Transpose( matCol0, matCol0 );
		Transpose( matCol1, matCol1 );
		Transpose( matCol2, matCol2 );
		Transpose( matCol3, matCol3 );
		outMat =  SoA_Mat44V(	matCol0.GetCol0Intrin128(), matCol0.GetCol1Intrin128(), matCol0.GetCol2Intrin128(), matCol0.GetCol3Intrin128(),
								matCol1.GetCol0Intrin128(), matCol1.GetCol1Intrin128(), matCol1.GetCol2Intrin128(), matCol1.GetCol3Intrin128(),
								matCol2.GetCol0Intrin128(), matCol2.GetCol1Intrin128(), matCol2.GetCol2Intrin128(), matCol2.GetCol3Intrin128(),
								matCol3.GetCol0Intrin128(), matCol3.GetCol1Intrin128(), matCol3.GetCol2Intrin128(), matCol3.GetCol3Intrin128()	);
	}
}
