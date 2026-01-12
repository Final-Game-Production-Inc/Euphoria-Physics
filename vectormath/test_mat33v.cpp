#include "classes.h"
#include "legacyconvert.h"
#include "qa/qa.h"
#include "vector/matrix33.h"



#if __QA

// Macro for testing a snippet of code.
//#define MAT34V_QA_ITEM_BEGIN(Name)			
//class qa##Name : public qaItem				
//{ public:									
//	void Init() {};							
//	void Update(qaResult& result)
//#define MAT34V_QA_ITEM_END		};

#define MAT33V_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define MAT33V_QA_ALL_END		};

#define MAT33V_QA_ITEM_BEGIN(Name)	currentTestName = #Name;
#define MAT33V_QA_ITEM_END			currentTestName = NULL;


// Some custom check macros.// Some custom check macros.
#define CHECK(X)						\
	{									\
	bool result = (X);				\
	if (!result) {					\
	mthErrorf("Test %s failed: %s", currentTestName, #X);	\
	}								\
	bRESULT = bRESULT && result;	\
	}
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON 0.001f




using namespace rage;
using namespace Vec;

MAT33V_QA_ALL_START(MAT33V)

{	// beginning of .Update()

	bool bRESULT = true;
	const char* currentTestName = NULL;	

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM00f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM00f();
	CHECK_EQUAL( x, 1.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM01f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM01f();
	CHECK_EQUAL( x, 4.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM02f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM02f();
	CHECK_EQUAL( x, 7.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM10f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM10f();
	CHECK_EQUAL( x, 2.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM11f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM11f();
	CHECK_EQUAL( x, 5.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM12f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM12f();
	CHECK_EQUAL( x, 8.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM20f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM20f();
	CHECK_EQUAL( x, 3.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM21f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM21f();
	CHECK_EQUAL( x, 6.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetM22f )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	float x = testMat.GetM22f();
	CHECK_EQUAL( x, 9.0f );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetCol0 )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	Vec3V col0 = testMat.GetCol0();
	Vec3V col0compare(1.0f, 2.0f, 3.0f);
	CHECK( 0 != IsEqualAll(col0, col0compare) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetCol1 )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	Vec3V col1 = testMat.GetCol1();
	Vec3V col1compare(4.0f, 5.0f, 6.0f);
	CHECK( 0 != IsEqualAll(col1, col1compare) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_GetCol2 )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	Vec3V col2 = testMat.GetCol2();
	Vec3V col2compare(7.0f, 8.0f, 9.0f);
	CHECK( 0 != IsEqualAll(col2, col2compare) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_operator_access )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	bool bResult = true;
	Vec::Vector_4V col0compare = VECTOR4V_LITERAL(1.0f, 2.0f, 3.0f, 0.0f);
	Vec::Vector_4V col1compare = VECTOR4V_LITERAL(4.0f, 5.0f, 6.0f, 0.0f);
	Vec::Vector_4V col2compare = VECTOR4V_LITERAL(7.0f, 8.0f, 9.0f, 0.0f);
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[0], col0compare ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[1], col1compare ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[2], col2compare ) );

	CHECK( bResult );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_operator_access_ref )
{
	Mat33V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f );
	bool bResult = true;
	Vec::Vector_4V col0new = VECTOR4V_LITERAL(10.0f, 11.0f, 12.0f, 0.0f);
	Vec::Vector_4V col1new = VECTOR4V_LITERAL(13.0f, 14.0f, 15.0f, 0.0f);
	Vec::Vector_4V col2new = VECTOR4V_LITERAL(16.0f, 17.0f, 18.0f, 0.0f);
	testMat[0] = col0new;
	testMat[1] = col1new;
	testMat[2] = col2new;
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[0], col0new ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[1], col1new ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[2], col2new ) );

	CHECK( bResult );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromAxisAngle )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat33V rotMatrix;
	Mat33VFromAxisAngle( rotMatrix, normAxis, angle );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s 
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s 
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c 
	// Where:
	// c = cos(angle), s = sin(angle), t = 1-c
	// x = normAxis.x, y = normAxis.y, z = normAxis.z

	float anglef = angle.Getf();
	float c = cosf( anglef );
	float s = sinf( anglef );
	float t = 1-c;
	float x = normAxis.GetXf();
	float y = normAxis.GetXf();
	float z = normAxis.GetXf();

	Mat33V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c	);
	
	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromXAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat33V rotMatrix;
	Mat33VFromXAxisAngle( rotMatrix, angle );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s 
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s 
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c 
	// Where:
	// c = cos(angle), s = sin(angle), t = 1-c
	// x = 1.0f, y = 0.0f, z = 0.0f

	float anglef = angle.Getf();
	float c = cosf( anglef );
	float s = sinf( anglef );
	float t = 1-c;
	float x = 1.0f;
	float y = 0.0f;
	float z = 0.0f;

	Mat33V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromYAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat33V rotMatrix;
	Mat33VFromYAxisAngle( rotMatrix, angle );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s 
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s 
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c 
	// Where:
	// c = cos(angle), s = sin(angle), t = 1-c
	// x = 0.0f, y = 1.0f, z = 0.0f

	float anglef = angle.Getf();
	float c = cosf( anglef );
	float s = sinf( anglef );
	float t = 1-c;
	float x = 0.0f;
	float y = 1.0f;
	float z = 0.0f;

	Mat33V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromZAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat33V rotMatrix;
	Mat33VFromZAxisAngle( rotMatrix, angle );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s 
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s 
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c 
	// Where:
	// c = cos(angle), s = sin(angle), t = 1-c
	// x = 0.0f, y = 0.0f, z = 1.0f

	float anglef = angle.Getf();
	float c = cosf( anglef );
	float s = sinf( anglef );
	float t = 1-c;
	float x = 0.0f;
	float y = 0.0f;
	float z = 1.0f;

	Mat33V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromQuat )
{
	QuatV quat( 1.0f, 2.0f, 3.0f, 4.0f );
	quat = Normalize( quat );
	Mat33V rotMatrix;
	Mat33VFromQuatV( rotMatrix, quat );

	// Should be:
	// 1 - 2*qy^2 - 2*qz^2		2*qx*qy - 2*qz*qw		2*qx*qz + 2*qy*qw 
	// 2*qx*qy + 2*qz*qw		1 - 2*qx^2 - 2*qz^2		2*qy*qz - 2*qx*qw 
	// 2*qx*qz - 2*qy*qw		2*qy*qz + 2*qx*qw		1 - 2*qx^2 - 2*qy^2 

	float qx = quat.GetXf();
	float qy = quat.GetYf();
	float qz = quat.GetZf();
	float qw = quat.GetWf();	

	Mat33V rotMatrixCompare(	1 - 2*qy*qy - 2*qz*qz,		2*qx*qy + 2*qz*qw,			2*qx*qz - 2*qy*qw, 
								2*qx*qy - 2*qz*qw,			1 - 2*qx*qx - 2*qz*qz,		2*qy*qz + 2*qx*qw, 
								2*qx*qz + 2*qy*qw,			2*qy*qz - 2*qx*qw,			1 - 2*qx*qx - 2*qy*qy 	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromEulersXYZ )
{
	// (R,P,Y) = (pi/4, pi, pi/2)
	Vec3V rpyVect( 0.78539816339744830961566084581988f, 3.1415926535897932384626433832795f, 1.5707963267948966192313216916398f );
	Mat33V rotMatrix;
	Mat33VFromEulersXYZ( rotMatrix, rpyVect );

	// Should be:
	// cos(Y)*cos(P)		cos(Y)*sin(P)*sin(R)-sin(Y)*cos(R)		cos(Y)*sin(P)*cos(R)+sin(Y)*sin(R)
	// sin(Y)*cos(P)		sin(Y)*sin(P)*sin(R)+cos(Y)*cos(R)		sin(Y)*sin(P)*cos(R)-cos(Y)*sin(R)
	// -sin(P)				cos(P)*sin(R)							cos(P)*cos(R)
	//
	// (http://planning.cs.uiuc.edu/node102.html)

	float R = rpyVect.GetXf(); // Maybe GetZf()?
	float P = rpyVect.GetYf();
	float Y = rpyVect.GetZf(); // Maybe GetXf()?

	Mat33V rotMatrixCompare(	cosf(Y)*cosf(P),							sinf(Y)*cosf(P),							-sinf(P), 
								cosf(Y)*sinf(P)*sinf(R)-sinf(Y)*cosf(R),	sinf(Y)*sinf(P)*sinf(R)+cosf(Y)*cosf(R),	cosf(P)*sinf(R), 
								cosf(Y)*sinf(P)*cosf(R)+sinf(Y)*sinf(R),	sinf(Y)*sinf(P)*cosf(R)-cosf(Y)*sinf(R),	cosf(P)*cosf(R) 	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromScale )
{
	Vec3V scaleVect( 2.0f, 3.0f, 4.0f );
	Mat33V scaleMatrix;
	Mat33VFromScale( scaleMatrix, scaleVect );

	Mat33V scaleMatrixCompare(	2.0f,	0.0f,	0.0f, 
								0.0f,	3.0f,	0.0f, 
								0.0f,	0.0f,	4.0f 	);

	CHECK( 0 != IsEqualAll( scaleMatrix, scaleMatrixCompare ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mat33FromScale_V1 )
{
	Mat33V scaleMatrix;
	Mat33VFromScale( scaleMatrix, ScalarVFromF32(2.0f), ScalarVFromF32(3.0f), ScalarVFromF32(4.0f) );

	Mat33V scaleMatrixCompare(	2.0f,	0.0f,	0.0f, 
								0.0f,	3.0f,	0.0f, 
								0.0f,	0.0f,	4.0f 	);

	CHECK( 0 != IsEqualAll( scaleMatrix, scaleMatrixCompare ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_DeterminantV )
{
	Mat33V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f	);

	ScalarV det = Determinant( inputMat );
	ScalarV detCompare = ScalarVFromF32( 46.0f );

	CHECK( 0 != IsCloseAll( det, detCompare, ScalarVFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Transpose )
{
	Mat33V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f	);

	Mat33V transpose;
	Transpose( transpose, inputMat );
	Mat33V transposeCompare(	1.0f, 2.0f, 3.0f,
								-5.0f, 1.0f, 4.0f,
								2.0f, 1.0f, 5.0f	);

	CHECK( 0 != IsEqualAll( transpose, transposeCompare ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Invert )
{
	Mat33V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f	);

	Mat33V invertMat;
	InvertFull( invertMat, inputMat );

	Mat33V invertMatCompare(	0.021739130f, 0.717391304f, -0.152173913f,
								-0.152173913f, -0.021739130f, 0.065217391f,
								0.108695652f, -0.413043478f, 0.239130435f	);

	CHECK( 0 != IsCloseAll( invertMat, invertMatCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_InvertOrtho )
{
	Mat33V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f	);

	Mat33V invertMat;
	InvertOrtho( invertMat, inputMat );
	Mat33V transposeMat;
	Transpose( transposeMat, inputMat );

	CHECK( 0 != IsEqualAll( invertMat, transposeMat ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mul_M33_M33 )
{
	Mat33V inputMatA(	-2.0f, 1.0f, 7.0f,
						4.0f, 5.0f, -3.0f,
						9.0f, 7.0f, 2.0f	);
	Mat33V inputMatB(	1.0f, 2.0f, 1.0f,
						-3.0f, 2.0f, 0.0f,
						1.0f, 1.0f, 2.0f	);

	Mat33V prodMat;
	Multiply( prodMat, inputMatA, inputMatB );
	Mat33V prodMatCompare(	15.0f, 18.0f, 3.0f,
							14.0f, 7.0f, -27.0f,
							20.0f, 20.0f, 8.0f	);

	CHECK( 0 != IsCloseAll( prodMat, prodMatCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mul_M33_V3 )
{
	Mat33V inputMatA(	-2.0f, 1.0f, 7.0f,
						4.0f, 5.0f, -3.0f,
						9.0f, 7.0f, 2.0f	);
	Vec3V inputVecB(	1.0f, 2.0f, 1.0f	);

	Vec3V prodVec = Multiply( inputMatA, inputVecB );
	Vec3V prodVecCompare( 15.0f, 18.0f, 3.0f );

	CHECK( 0 != IsCloseAll( prodVec, prodVecCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Mul_V3_M33 )
{
	Mat33V inputMatB(	-2.0f, 1.0f, 7.0f,
						4.0f, 5.0f, -3.0f,
						9.0f, 7.0f, 2.0f	);
	Vec3V inputVecA(	1.0f, 2.0f, 1.0f	);

	Vec3V prodVec = Multiply( inputVecA, inputMatB );
	Vec3V prodVecCompare( 7.0f, 11.0f, 25.0f );

	CHECK( 0 != IsCloseAll( prodVec, prodVecCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_UnTransform_M33 )
{
	Mat33V transformMat(	-2.0f, 1.0f, 7.0f,
							4.0f, 5.0f, -3.0f,
							9.0f, 7.0f, 2.0f	);
	Mat33V matToTransform(	1.0f, 2.0f, 1.0f,
							-3.0f, 2.0f, 0.0f,
							1.0f, 1.0f, 2.0f	);

	Mat33V transformedMat;
	Multiply( transformedMat, transformMat, matToTransform );
	Mat33V untransformedMat;
	UnTransformFull( untransformedMat, transformMat, transformedMat );

	CHECK( 0 != IsCloseAll( untransformedMat, matToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_UnTransformOrtho_M33 )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	Mat33V orthoTransformMat;
	Mat33VFromAxisAngle( orthoTransformMat, normAxis, angle );

	Mat33V matToTransform(	1.0f, 2.0f, 1.0f,
							-3.0f, 2.0f, 0.0f,
							1.0f, 1.0f, 2.0f	);

	Mat33V transformedMat;
	Multiply( transformedMat, orthoTransformMat, matToTransform );
	Mat33V untransformedMat;
	UnTransformOrtho( untransformedMat, orthoTransformMat, transformedMat );

	CHECK( 0 != IsCloseAll( untransformedMat, matToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_UnTransform_V3 )
{
	Mat33V transformMat(	-2.0f, 1.0f, 7.0f,
							4.0f, 5.0f, -3.0f,
							9.0f, 7.0f, 2.0f	);
	Vec3V vecToTransform(	1.0f, 2.0f, 5.0f	);

	Vec3V transformedVec = Multiply( transformMat, vecToTransform );
	Vec3V untransformedVec = UnTransformFull( transformMat, transformedVec );

	CHECK( 0 != IsCloseAll( untransformedVec, vecToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_UnTransformOrtho_V3 )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat33V orthoTransformMat;
	Mat33VFromAxisAngle( orthoTransformMat, normAxis, angle );

	Vec3V vecToTransform(	1.0f, 2.0f, 5.0f	);

	Vec3V transformedVec = Multiply( orthoTransformMat, vecToTransform );
	Vec3V untransformedVec = UnTransformOrtho( orthoTransformMat, transformedVec );

	CHECK( 0 != IsCloseAll( untransformedVec, vecToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_ReOrthonormalize )
{
	bool bResult = true;

	ScalarV angle = ScalarVFromF32( 0.78539816339744830961566084581988f ); // pi/4
	Mat33V orthoTransformMat;
	Mat33VFromYAxisAngle( orthoTransformMat, angle );

	// Should be orthonormal right now.
	bResult = bResult && orthoTransformMat.IsOrthonormal( ScalarVFromF32(0.01f) );

	// Perturb the x-axis transform a bit.
	orthoTransformMat.SetM00f( orthoTransformMat.GetM00f() + 0.5f );

	// Should no longer be orthonormal.
	bResult = bResult && !orthoTransformMat.IsOrthonormal( ScalarVFromF32(0.01f) );

	// Re-orthonormalize.
	ReOrthonormalize( orthoTransformMat, orthoTransformMat );

	// Should be orthonormal again.
	bResult = bResult && orthoTransformMat.IsOrthonormal( ScalarVFromF32(0.01f) );

	CHECK( bResult );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Add )
{
	Mat33V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f	);

	Mat33V matB(	-1.0f, -2.0f, -3.0f,
					-5.0f, -6.0f, -7.0f,
					-9.0f, -10.0f, -11.0f	);

	Mat33V matSum;
	Add( matSum, matA, matB );
	Mat33V matSumCompare(V_ZERO);
	
	CHECK( 0 != IsCloseAll( matSum, matSumCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Subtract )
{
	Mat33V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f	);

	Mat33V matB(	-1.0f, -2.0f, -3.0f,
					-5.0f, -6.0f, -7.0f,
					-9.0f, -10.0f, -11.0f	);

	Mat33V matDiff;
	Subtract( matDiff, matA, matB );

	Mat33V matDiffCompare(	2*1.0f, 2*2.0f, 2*3.0f,
							2*5.0f, 2*6.0f, 2*7.0f,
							2*9.0f, 2*10.0f, 2*11.0f	);
	
	CHECK( 0 != IsCloseAll( matDiff, matDiffCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Abs )
{
	Mat33V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f	);

	Mat33V matB(	-1.0f, -2.0f, -3.0f,
					-5.0f, -6.0f, -7.0f,
					-9.0f, -10.0f, -11.0f	);

	Mat33V matAbs;
	Abs( matAbs, matB );
	
	CHECK( 0 != IsCloseAll( matAbs, matA, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Scale )
{
	Mat33V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f	);

	Vec3V scaleVect( -1.0f, -2.0f, -3.0f );

	Mat33V matScaledVersion1;
	Scale( matScaledVersion1, matA, scaleVect );
	Mat33V matScaledVersion2;
	Scale( matScaledVersion2, scaleVect, matA );

	Mat33V matScaledCompare(	-1.0f, -4.0f, -9.0f,
								-5.0f, -12.0f, -21.0f,
								-9.0f, -20.0f, -33.0f	);

	CHECK(	0 != IsCloseAll( matScaledVersion1, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) &&
			0 != IsCloseAll( matScaledVersion2, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_InvScale )
{
	Mat33V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f	);

	Vec3V scaleVect( -1.0f, -2.0f, -3.0f );

	Mat33V matScaled;
	InvScale( matScaled, matA, scaleVect );

	Mat33V matScaledCompare(	-1.0f, -1.0f, -1.0f,
								-5.0f, -3.0f, -7.0f/3.0f,
								-9.0f, -5.0f, -11.0f/3.0f	);
	
	CHECK( 0 != IsCloseAll( matScaled, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_InvScaleSafe )
{
	Mat33V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f	);

	Vec3V scaleVect( -1.0f, -2.0f, 0.0f );

	Mat33V matScaled;
	InvScaleSafe( matScaled, matA, scaleVect, Vec3V(V_ZERO) );

	Mat33V matScaledCompare(	-1.0f, -1.0f, 0.0f,
								-5.0f, -3.0f, 0.0f,
								-9.0f, -5.0f, 0.0f	);
	
	CHECK( 0 != IsCloseAll( matScaled, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) );
}

MAT33V_QA_ITEM_BEGIN( Mat44V_InverseCompatibilityCheck )
{
	Matrix33 matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 1.0f, 11.0f 	);
	Matrix33 invMatA;
	invMatA.Inverse( matA );

	Mat33V _matA = RC_MAT33V( matA );
	Mat33V _invMatA;
	InvertFull( _invMatA, _matA );
	Matrix33 invMatA_compare = RC_MATRIX33( _invMatA );

	CHECK( invMatA.IsClose( invMatA_compare, TEST_EPSILON ) );
}
MAT33V_QA_ITEM_END


MAT33V_QA_ITEM_BEGIN( Mat33V_OuterProduct )
{
	Mat33V outerProduct;
	OuterProduct(outerProduct, Vec3V(1.0f, 2.0f, 3.0f), Vec3V(3.0f, 4.0f, 5.0f));

	Mat33V outerProductCompare(Mat33V::ROW_MAJOR,
		3.0f, 4.0f, 5.0f,
		6.0f, 8.0f, 10.0f,
		9.0f, 12.0f, 15.0f
		);
	CHECK (0 != IsCloseAll(outerProduct, outerProductCompare, Vec3VFromF32(TEST_EPSILON)));
}
MAT33V_QA_ITEM_END

MAT33V_QA_ITEM_BEGIN( Mat33V_Eulers )
{
	const int count = 10;
	const float base = 2.f*PI;
	const float step = 4.f*PI / float(count);

	for(int w=0; w < 6; ++w)
	for(int i=0; i < count; ++i)
	for(int j=0; j < count; ++j)
	for(int k=0; k < count; ++k)
	{
		const Vector3 e0(-base+i*step, -base+j*step, -base+k*step);
		const Vec3V e1 = RCC_VEC3V(e0);

		Matrix34 m0(Matrix34::ZeroType);
		Mat33V m1(Mat33V::ZERO);
		Vector3 v0;
		Vec3V v1;
		switch(EulerAngleOrder(w))
		{
		case EULER_XYZ: 
			m0.FromEulersXYZ(e0); 
			m0.ToEulersFastXYZ(v0); 
			Mat33VFromEulersXYZ(m1,e1); 
			v1=Mat33VToEulersXYZ(m1); break;
		case EULER_XZY: 
			m0.FromEulersXZY(e0);
			m0.ToEulersFastXZY(v0);
			Mat33VFromEulersXZY(m1,e1); 
			v1=Mat33VToEulersXZY(m1); break;
		case EULER_YXZ: 
			m0.FromEulersYXZ(e0); 
			m0.ToEulersFastYXZ(v0); 
			Mat33VFromEulersYXZ(m1,e1); 
			v1=Mat33VToEulersYXZ(m1); break;
		case EULER_YZX: 
			m0.FromEulersYZX(e0); 
			m0.ToEulersFastYZX(v0); 
			Mat33VFromEulersYZX(m1,e1); 
			v1=Mat33VToEulersYZX(m1); break;
		case EULER_ZXY: 
			m0.FromEulersZXY(e0);
			m0.ToEulersFastZXY(v0); 
			Mat33VFromEulersZXY(m1,e1); 
			v1=Mat33VToEulersZXY(m1); break;
		case EULER_ZYX: 
			m0.FromEulersZYX(e0); 
			m0.ToEulersFastZYX(v0); 
			Mat33VFromEulersZYX(m1,e1); 
			v1=Mat33VToEulersZYX(m1); break;
		default: FastAssert(false);
		}

		Vec3V v = CanonicalizeAngle(RCC_VEC3V(v0) - v1);
		CHECK(0 != IsCloseAll(v, Vec3V(V_ZERO), Vec3VFromF32(TEST_EPSILON)));
		CHECK(0 != IsCloseAll(RCC_MAT34V(m0).GetMat33(), m1, Vec3VFromF32(TEST_EPSILON)));
	}
}
MAT33V_QA_ITEM_END

	if( bRESULT )
	{
		TST_PASS;
	}
	else
	{
		TST_FAIL;
	}
} // end of .Update()

MAT33V_QA_ALL_END


QA_ITEM_FAMILY( qaMAT33V, (), () );
QA_ITEM_FAST( qaMAT33V, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// MEMBER FUNCTIONS
//================================================
/*
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM00f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM00f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM01f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM01f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM02f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM02f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM10f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM10f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM11f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM11f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM12f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM12f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM20f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM20f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM21f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM21f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetM22f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetM22f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetCol0, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetCol0, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetCol1, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetCol1, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_GetCol2, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_GetCol2, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_operator_access, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_operator_access, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_operator_access_ref, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_operator_access_ref, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// FREE FUNCTIONS
//================================================

Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromXAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromXAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromYAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromYAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromZAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromZAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromQuat, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromQuat, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromScale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromScale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mat33FromScale_V1, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mat33FromScale_V1, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_DeterminantV, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_DeterminantV, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Transpose, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Transpose, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Invert, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Invert, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_InvertOrtho, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_InvertOrtho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mul_M33_M33, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mul_M33_M33, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mul_M33_V3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mul_M33_V3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Mul_V3_M33, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Mul_V3_M33, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_UnTransform_M33, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_UnTransform_M33, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_UnTransformOrtho_M33, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_UnTransformOrtho_M33, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_UnTransform_V3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_UnTransform_V3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_UnTransformOrtho_V3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_UnTransformOrtho_V3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_ReOrthonormalize, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_ReOrthonormalize, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Add, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Add, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Subtract, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Subtract, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Abs, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Abs, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_Scale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_Scale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_InvScale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_InvScale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat33V_InvScaleSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat33V_InvScaleSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
*/






#endif // __QA
