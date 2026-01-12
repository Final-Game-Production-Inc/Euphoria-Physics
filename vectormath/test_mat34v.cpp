#include "classes.h"
#include "legacyconvert.h"
#include "qa/qa.h"
#include "vector/matrix34.h"



#if __QA

// Macro for testing a snippet of code.
//#define MAT34V_QA_ITEM_BEGIN(Name)			
//class qa##Name : public qaItem				
//{ public:									
//	void Init() {};							
//	void Update(qaResult& result)
//#define MAT34V_QA_ITEM_END		};

#define MAT34V_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define MAT34V_QA_ALL_END		};

#define MAT34V_QA_ITEM_BEGIN(Name)	;
#define MAT34V_QA_ITEM_END			;

// Some custom check macros.
#define CHECK(X)				( bRESULT = bRESULT && (X) )
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON 0.001f




using namespace rage;
using namespace Vec;


MAT34V_QA_ALL_START(MAT34V)

{	// beginning of .Update()

	bool bRESULT = true;

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM00f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM00f();
	CHECK_EQUAL( x, 1.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM01f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM01f();
	CHECK_EQUAL( x, 4.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM02f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM02f();
	CHECK_EQUAL( x, 7.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM03f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM03f();
	CHECK_EQUAL( x, 10.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM10f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM10f();
	CHECK_EQUAL( x, 2.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM11f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM11f();
	CHECK_EQUAL( x, 5.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM12f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM12f();
	CHECK_EQUAL( x, 8.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM13f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM13f();
	CHECK_EQUAL( x, 11.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM20f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM20f();
	CHECK_EQUAL( x, 3.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM21f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM21f();
	CHECK_EQUAL( x, 6.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM22f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM22f();
	CHECK_EQUAL( x, 9.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetM23f )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	float x = testMat.GetM23f();
	CHECK_EQUAL( x, 12.0f );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetCol0 )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	Vec3V col0 = testMat.GetCol0();
	Vec3V col0compare(1.0f, 2.0f, 3.0f);
	CHECK( 0 != IsEqualAll(col0, col0compare) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetCol1 )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	Vec3V col1 = testMat.GetCol1();
	Vec3V col1compare(4.0f, 5.0f, 6.0f);
	CHECK( 0 != IsEqualAll(col1, col1compare) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetCol2 )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	Vec3V col2 = testMat.GetCol2();
	Vec3V col2compare(7.0f, 8.0f, 9.0f);
	CHECK( 0 != IsEqualAll(col2, col2compare) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_GetCol3 )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	Vec3V col2 = testMat.GetCol3();
	Vec3V col2compare(10.0f, 11.0f, 12.0f);
	CHECK( 0 != IsEqualAll(col2, col2compare) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_operator_access )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	bool bResult = true;
	Vec::Vector_4V col0compare = VECTOR4V_LITERAL(1.0f, 2.0f, 3.0f, 0.0f);
	Vec::Vector_4V col1compare = VECTOR4V_LITERAL(4.0f, 5.0f, 6.0f, 0.0f);
	Vec::Vector_4V col2compare = VECTOR4V_LITERAL(7.0f, 8.0f, 9.0f, 0.0f);
	Vec::Vector_4V col3compare = VECTOR4V_LITERAL(10.0f, 11.0f, 12.0f, 0.0f);
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[0], col0compare ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[1], col1compare ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[2], col2compare ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[3], col3compare ) );

	CHECK( bResult );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_operator_access_ref )
{
	Mat34V testMat(	1.0f, 2.0f, 3.0f,
					4.0f, 5.0f, 6.0f,
					7.0f, 8.0f, 9.0f,
					10.0f, 11.0f, 12.0f );
	bool bResult = true;
	Vec::Vector_4V col0new = VECTOR4V_LITERAL(10.0f, 11.0f, 12.0f, 0.0f);
	Vec::Vector_4V col1new = VECTOR4V_LITERAL(13.0f, 14.0f, 15.0f, 0.0f);
	Vec::Vector_4V col2new = VECTOR4V_LITERAL(16.0f, 17.0f, 18.0f, 0.0f);
	Vec::Vector_4V col3new = VECTOR4V_LITERAL(19.0f, 20.0f, 21.0f, 0.0f);
	testMat[0] = col0new;
	testMat[1] = col1new;
	testMat[2] = col2new;
	testMat[3] = col3new;
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[0], col0new ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[1], col1new ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[2], col2new ) );
	bResult = bResult && ( 0 != Vec::V3IsEqualAll( testMat[3], col3new ) );

	CHECK( bResult );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromAxisAngle )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V rotMatrix;
	Mat34VFromAxisAngle( rotMatrix, normAxis, angle, Vec3V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	//
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

	Mat34V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c,
								1.0f, 1.0f, 1.0f	);
	
	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromXAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V rotMatrix;
	Mat34VFromXAxisAngle( rotMatrix, angle, Vec3V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	//
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

	Mat34V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c,
								1.0f, 1.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromYAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V rotMatrix;
	Mat34VFromYAxisAngle( rotMatrix, angle, Vec3V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX						
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	// 
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

	Mat34V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c,
								1.0f, 1.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromZAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V rotMatrix;
	Mat34VFromZAxisAngle( rotMatrix, angle, Vec3V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX						
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	// 
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

	Mat34V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c,
								1.0f, 1.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromQuat )
{
	QuatV quat( 1.0f, 2.0f, 3.0f, 4.0f );
	quat = Normalize( quat );
	Mat34V rotMatrix;
	Mat34VFromQuatV( rotMatrix, quat, Vec3V(V_ONE) );

	// Should be:
	// 1 - 2*qy^2 - 2*qz^2		2*qx*qy - 2*qz*qw		2*qx*qz + 2*qy*qw		transX
	// 2*qx*qy + 2*qz*qw		1 - 2*qx^2 - 2*qz^2		2*qy*qz - 2*qx*qw		transY
	// 2*qx*qz - 2*qy*qw		2*qy*qz + 2*qx*qw		1 - 2*qx^2 - 2*qy^2		transZ
	// 

	float qx = quat.GetXf();
	float qy = quat.GetYf();
	float qz = quat.GetZf();
	float qw = quat.GetWf();	

	Mat34V rotMatrixCompare(	1 - 2*qy*qy - 2*qz*qz,		2*qx*qy + 2*qz*qw,			2*qx*qz - 2*qy*qw, 
								2*qx*qy - 2*qz*qw,			1 - 2*qx*qx - 2*qz*qz,		2*qy*qz + 2*qx*qw, 
								2*qx*qz + 2*qy*qw,			2*qy*qz - 2*qx*qw,			1 - 2*qx*qx - 2*qy*qy,
								1.0f,						1.0f,						1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromEulersXYZ )
{
	// (R,P,Y) = (pi/4, pi, pi/2)
	Vec3V rpyVect( 0.78539816339744830961566084581988f, 3.1415926535897932384626433832795f, 1.5707963267948966192313216916398f );
	Mat34V rotMatrix;
	Mat34VFromEulersXYZ( rotMatrix, rpyVect, Vec3V(V_ONE) );

	// Should be:
	// cos(Y)*cos(P)		cos(Y)*sin(P)*sin(R)-sin(Y)*cos(R)		cos(Y)*sin(P)*cos(R)+sin(Y)*sin(R)		transX
	// sin(Y)*cos(P)		sin(Y)*sin(P)*sin(R)+cos(Y)*cos(R)		sin(Y)*sin(P)*cos(R)-cos(Y)*sin(R)		transY
	// -sin(P)				cos(P)*sin(R)							cos(P)*cos(R)							transZ
	//
	// (http://planning.cs.uiuc.edu/node102.html)

	float R = rpyVect.GetXf();
	float P = rpyVect.GetYf();
	float Y = rpyVect.GetZf();

	Mat34V rotMatrixCompare(	cosf(Y)*cosf(P),							sinf(Y)*cosf(P),							-sinf(P), 
								cosf(Y)*sinf(P)*sinf(R)-sinf(Y)*cosf(R),	sinf(Y)*sinf(P)*sinf(R)+cosf(Y)*cosf(R),	cosf(P)*sinf(R), 
								cosf(Y)*sinf(P)*cosf(R)+sinf(Y)*sinf(R),	sinf(Y)*sinf(P)*cosf(R)-cosf(Y)*sinf(R),	cosf(P)*cosf(R),
								1.0f,										1.0f,										1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromScale )
{
	Vec3V scaleVect( 2.0f, 3.0f, 4.0f );
	Mat34V scaleMatrix;
	Mat34VFromScale( scaleMatrix, scaleVect, Vec3V(V_ONE) );

	Mat34V scaleMatrixCompare(	2.0f,	0.0f,	0.0f, 
								0.0f,	3.0f,	0.0f, 
								0.0f,	0.0f,	4.0f,
								1.0f,	1.0f,	1.0f	);

	CHECK( 0 != IsEqualAll( scaleMatrix, scaleMatrixCompare ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromScale_V1 )
{
	Mat34V scaleMatrix;
	Mat34VFromScale( scaleMatrix, ScalarVFromF32(2.0f), ScalarVFromF32(3.0f), ScalarVFromF32(4.0f), Vec3V(V_ONE) );

	Mat34V scaleMatrixCompare(	2.0f,	0.0f,	0.0f, 
								0.0f,	3.0f,	0.0f, 
								0.0f,	0.0f,	4.0f,
								1.0f,	1.0f,	1.0f	);

	CHECK( 0 != IsEqualAll( scaleMatrix, scaleMatrixCompare ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Determinant3x3V )
{
	Mat34V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f,
						1.0f, 1.0f, 1.0f	);

	ScalarV det = Determinant3x3( inputMat );
	ScalarV detCompare = ScalarVFromF32( 46.0f );

	CHECK( 0 != IsCloseAll( det, detCompare, ScalarVFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mat34FromTranslation )
{
	Vec3V translationVect( 2.0f, 3.0f, 4.0f );
	Mat34V translationMatrix;
	Mat34VFromTranslation( translationMatrix, translationVect );
	Mat34V translationMatrixCompare(	1.0f, 0.0f, 0.0f,
										0.0f, 1.0f, 0.0f,
										0.0f, 0.0f, 1.0f,
										2.0f, 3.0f, 4.0f	);

	CHECK( 0 != IsCloseAll( translationMatrix, translationMatrixCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Transpose3x3 )
{
	Mat34V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f,
						2.0f, 3.0f, 4.0f	);

	Mat34V transpose(	0.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f,
						70.0f, 80.0f, 90.0f	);
	Transpose3x3( transpose, inputMat );
	Mat34V transposeCompare(	1.0f, 2.0f, 3.0f,
								-5.0f, 1.0f, 4.0f,
								2.0f, 1.0f, 5.0f,
								2.0f, 3.0f, 4.0f	);

	CHECK( 0 != IsEqualAll( transpose, transposeCompare ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Invert3x3 )
{
	Mat34V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f,
						2.0f, 3.0f, 4.0f	);

	Mat34V invertMat(	0.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f,
						70.0f, 80.0f, 90.0f	);
	Invert3x3Full( invertMat, inputMat );

	Mat34V invertMatCompare(	0.021739130f,	0.717391304f,	-0.152173913f,
								-0.152173913f,	-0.021739130f,	0.065217391f,
								0.108695652f,	-0.413043478f,	0.239130435f,
								2.0f,			3.0f,			4.0f	);

	CHECK( 0 != IsCloseAll( invertMat, invertMatCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Invert3x3Ortho )
{
	Mat34V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f,
						2.0f, 3.0f, 4.0f	);

	Mat34V invertMat;
	Invert3x3Ortho( invertMat, inputMat );
	Mat34V transposeMat;
	Transpose3x3( transposeMat, inputMat );

	CHECK( 0 != IsEqualAll( invertMat, transposeMat ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_InvertTransform )
{
	Vec3V initialPoint( 4.0f, 5.0f, 6.0f );
	Mat34V inputMat(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f,
						2.0f, 3.0f, 4.0f	);

	Mat34V invertMat;
	InvertTransformFull( invertMat, inputMat );

	Vec3V transformedPoint = Transform( inputMat, initialPoint );
	Vec3V unTransformedPoint = Transform( invertMat, transformedPoint );

	Vec3V transformedVec = Transform3x3( inputMat, initialPoint );
	Vec3V unTransformedVec = Transform3x3( invertMat, transformedVec );

	CHECK(	0 != IsCloseAll( initialPoint, unTransformedPoint, Vec3VFromF32(TEST_EPSILON) ) &&
			0 != IsCloseAll( initialPoint, unTransformedVec, Vec3VFromF32(TEST_EPSILON) )	);
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_InvertTransformOrtho )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V orthoTransformMat;
	Mat34VFromAxisAngle( orthoTransformMat, normAxis, angle );

	Vec3V initialPoint( 4.0f, 5.0f, 6.0f );

	Mat34V invertMat;
	InvertTransformOrtho( invertMat, orthoTransformMat );

	Vec3V transformedPoint = Transform( orthoTransformMat, initialPoint );
	Vec3V unTransformedPoint = Transform( invertMat, transformedPoint );

	Vec3V transformedVec = Transform3x3( orthoTransformMat, initialPoint );
	Vec3V unTransformedVec = Transform3x3( invertMat, transformedPoint );

	CHECK(	0 != IsCloseAll( initialPoint, unTransformedPoint, Vec3VFromF32(TEST_EPSILON) )	&&
			0 != IsCloseAll( initialPoint, unTransformedVec, Vec3VFromF32(TEST_EPSILON) )	);
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mul_M34_V4 )
{
	Mat34V inputMatA(	-2.0f, 1.0f, 7.0f,
						4.0f, 5.0f, -3.0f,
						9.0f, 7.0f, 2.0f,
						2.0f, 3.0f, 4.0f	);
	Vec4V inputVecB( 1.0f, 2.0f, 3.0f, 4.0f );
	Vec3V prod = Multiply( inputMatA, inputVecB );
	Vec3V prodCompare( 41.0f, 44.0f, 23.0f );

	CHECK( 0 != IsCloseAll( prod, prodCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Mul_V3_M34 )
{
	Mat34V inputMatB(	-2.0f, 1.0f, 7.0f,
						4.0f, 5.0f, -3.0f,
						9.0f, 7.0f, 2.0f,
						2.0f, 3.0f, 4.0f	);
	Vec3V inputVecA(	1.0f, 2.0f, 3.0f	);

	Vec4V prodVec = Multiply( inputVecA, inputMatB );
	Vec4V prodVecCompare( 21.0f, 5.0f, 29.0f, 20.0f );

	CHECK( 0 != IsCloseAll( prodVec, prodVecCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_UnTransform_M34 )
{
	Mat34V transformMat(	-2.0f, 1.0f, 7.0f,
							4.0f, 5.0f, -3.0f,
							9.0f, 7.0f, 2.0f,
							2.0f, 3.0f, 4.0f	);
	Mat34V matToTransform(	1.0f, 2.0f, 1.0f,
							-3.0f, 2.0f, 0.0f,
							1.0f, 1.0f, 2.0f,
							5.0f, 5.0f, 5.0f	);

	Mat34V transformedMat;
	Transform( transformedMat, transformMat, matToTransform );
	Mat34V untransformedMat;
	UnTransformFull( untransformedMat, transformMat, transformedMat );

	CHECK( 0 != IsCloseAll( untransformedMat, matToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_UnTransformOrtho_M34 )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	Mat34V orthoTransformMat;
	Mat34VFromAxisAngle( orthoTransformMat, normAxis, angle );

	Mat34V matToTransform(	1.0f, 2.0f, 1.0f,
							-3.0f, 2.0f, 0.0f,
							1.0f, 1.0f, 2.0f,
							2.0f, 3.0f, 4.0f	);

	Mat34V transformedMat;
	Transform( transformedMat, orthoTransformMat, matToTransform );
	Mat34V untransformedMat;
	UnTransformOrtho( untransformedMat, orthoTransformMat, transformedMat );

	CHECK( 0 != IsCloseAll( untransformedMat, matToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_UnTransform3x3 )
{
	Mat34V transformMat(	-2.0f, 1.0f, 7.0f,
							4.0f, 5.0f, -3.0f,
							9.0f, 7.0f, 2.0f,
							1.0f, 1.0f, 2.0f	);
	Vec3V vecToTransform(	1.0f, 2.0f, 5.0f	);

	Vec3V transformedVec = Transform3x3( transformMat, vecToTransform );
	Vec3V untransformedVec = UnTransform3x3Full( transformMat, transformedVec );

	CHECK( 0 != IsCloseAll( untransformedVec, vecToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_UnTransform3x3Ortho )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V orthoTransformMat;
	Mat34VFromAxisAngle( orthoTransformMat, normAxis, angle );

	Vec3V vecToTransform(	1.0f, 2.0f, 5.0f	);

	Vec3V transformedVec = Transform3x3( orthoTransformMat, vecToTransform );
	Vec3V untransformedVec = UnTransform3x3Ortho( orthoTransformMat, transformedVec );

	CHECK( 0 != IsCloseAll( untransformedVec, vecToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_UnTransform )
{
	Mat34V transformMat(	-2.0f, 1.0f, 7.0f,
							4.0f, 5.0f, -3.0f,
							9.0f, 7.0f, 2.0f,
							1.0f, 1.0f, 2.0f	);
	Vec3V pointToTransform( 1.0f, 2.0f, 5.0f );

	Vec3V transformedPoint = Transform( transformMat, pointToTransform );
	Vec3V untransformedPoint = UnTransformFull( transformMat, transformedPoint );

	CHECK( 0 != IsCloseAll( untransformedPoint, pointToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_UnTransformOrtho )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat34V orthoTransformMat;
	Mat34VFromAxisAngle( orthoTransformMat, normAxis, angle );

	Vec3V pointToTransform(	1.0f, 2.0f, 5.0f	);

	Vec3V transformedPoint = Transform( orthoTransformMat, pointToTransform );
	Vec3V untransformedPoint = UnTransformOrtho( orthoTransformMat, transformedPoint );

	CHECK( 0 != IsCloseAll( untransformedPoint, pointToTransform, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_ReOrthonormalize3x3 )
{
	bool bResult = true;

	ScalarV angle = ScalarVFromF32( 0.78539816339744830961566084581988f ); // pi/4
	Mat34V orthoTransformMat;
	Mat34VFromYAxisAngle( orthoTransformMat, angle );

	// Should be orthonormal right now.
	bResult = bResult && orthoTransformMat.IsOrthonormal3x3( ScalarVFromF32(0.01f) );

	// Perturb the x-axis transform a bit.
	orthoTransformMat.SetM00f( orthoTransformMat.GetM00f() + 0.5f );

	// Should no longer be orthonormal.
	bResult = bResult && !orthoTransformMat.IsOrthonormal3x3( ScalarVFromF32(0.01f) );

	// Re-orthonormalize.
	ReOrthonormalize3x3( orthoTransformMat, orthoTransformMat );

	// Should be orthonormal again.
	bResult = bResult && orthoTransformMat.IsOrthonormal3x3( ScalarVFromF32(0.01f) );

	CHECK( bResult );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Add )
{
	Mat34V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f,
					13.0f, 14.0f, 15.0f	);

	Mat34V matB(	-1.0f, -2.0f, -3.0f,
					-5.0f, -6.0f, -7.0f,
					-9.0f, -10.0f, -11.0f,
					-13.0f, -14.0f, -15.0f	);

	Mat34V matSum;
	Add( matSum, matA, matB );
	Mat34V matSumCompare(V_ZERO);
	
	CHECK( 0 != IsCloseAll( matSum, matSumCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Subtract )
{
	Mat34V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f,
					13.0f, 14.0f, 15.0f	);

	Mat34V matB(	-1.0f, -2.0f, -3.0f,
					-5.0f, -6.0f, -7.0f,
					-9.0f, -10.0f, -11.0f,
					-13.0f, -14.0f, -15.0f	);

	Mat34V matDiff;
	Subtract( matDiff, matA, matB );

	Mat34V matDiffCompare(	2*1.0f, 2*2.0f, 2*3.0f,
							2*5.0f, 2*6.0f, 2*7.0f,
							2*9.0f, 2*10.0f, 2*11.0f,
							2*13.0f, 2*14.0f, 2*15.0f	);
	
	CHECK( 0 != IsCloseAll( matDiff, matDiffCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Abs )
{
	Mat34V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f,
					13.0f, 14.0f, 15.0f	);

	Mat34V matB(	-1.0f, -2.0f, -3.0f,
					-5.0f, -6.0f, -7.0f,
					-9.0f, -10.0f, -11.0f,
					-13.0f, -14.0f, -15.0f	);

	Mat34V matAbs;
	Abs( matAbs, matB );
	
	CHECK( 0 != IsCloseAll( matAbs, matA, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_Scale )
{
	Mat34V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f,
					13.0f, 14.0f, 15.0f	);

	Vec3V scaleVect( -1.0f, -2.0f, -3.0f );

	Mat34V matScaledVersion1;
	Scale( matScaledVersion1, matA, scaleVect );
	Mat34V matScaledVersion2;
	Scale( matScaledVersion2, scaleVect, matA );

	Mat34V matScaledCompare(	-1.0f, -4.0f, -9.0f,
								-5.0f, -12.0f, -21.0f,
								-9.0f, -20.0f, -33.0f,
								-13.0f, -28.0f, -45.0f	);

	CHECK(	0 != IsCloseAll( matScaledVersion1, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) &&
			0 != IsCloseAll( matScaledVersion2, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_InvScale )
{
	Mat34V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f,
					13.0f, 14.0f, 15.0f	);

	Vec3V scaleVect( -1.0f, -2.0f, -3.0f );

	Mat34V matScaled;
	InvScale( matScaled, matA, scaleVect );

	Mat34V matScaledCompare(	-1.0f, -1.0f, -1.0f,
								-5.0f, -3.0f, -7.0f/3.0f,
								-9.0f, -5.0f, -11.0f/3.0f,
								-13.0f, -7.0f, -5.0f	);
	
	CHECK( 0 != IsCloseAll( matScaled, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) );
}
MAT34V_QA_ITEM_END

MAT34V_QA_ITEM_BEGIN( Mat34V_InvScaleSafe )
{
	Mat34V matA(	1.0f, 2.0f, 3.0f,
					5.0f, 6.0f, 7.0f,
					9.0f, 10.0f, 11.0f,
					13.0f, 14.0f, 15.0f	);

	Vec3V scaleVect( -1.0f, -2.0f, 0.0f );

	Mat34V matScaled;
	InvScaleSafe( matScaled, matA, scaleVect, Vec3V(V_ZERO) );

	Mat34V matScaledCompare(	-1.0f, -1.0f, 0.0f,
								-5.0f, -3.0f, 0.0f,
								-9.0f, -5.0f, 0.0f,
								-13.0f, -7.0f, 0.0f	);
	
	CHECK( 0 != IsCloseAll( matScaled, matScaledCompare, Vec3VFromF32(TEST_EPSILON) ) );
}

MAT34V_QA_ITEM_BEGIN( Mat34V_OldNewTransform_Compatibility )
{
	// This unit test is to make sure we can reinterpret_cast Matrix34's into Mat34V's, do an equivalent transform,
	// then reinterpret_cast the result back into a Matrix34, and have the correct result.

	Matrix34 _A, _B, _C;
	_A.Set( 1.0f, 2.0f, 1.0f, 3.0f, 2.0f, 1.5f, 6.0f, 2.3f, 1.0f, 1.0f, 1.0f, 2.5f );
	_B.Set( 3.0f, 1.0f, 2.0f, 2.0f, 1.0f, 5.5f, 2.0f, 3.3f, 4.0f, 2.0f, 2.0f, 3.5f );
	_C.Dot( _A, _B );

	Mat34V A, B, C;
	Matrix34 _C_compare;
	A = RC_MAT34V( _A );
	B = RC_MAT34V( _B );
	Transform( C, B, A );
	_C_compare = RC_MATRIX34( C );

	CHECK( _C_compare.IsEqual( _C ) );
}

MAT34V_QA_ITEM_BEGIN( Mat34V_Transform3x3_Compatibility )
{
	// These should be equivalent 3x3 transforms of 3x4 matrices.
	Mat34V a_1;
	Mat34V b_1;
	Mat34V c_1;
	Mat34VFromEulersXYZ( a_1, Vec3V(1.0f, 0.7f, 0.5f), Vec3V(1.0f, 2.0f, 3.0f) );
	Mat34VFromEulersXYZ( b_1, Vec3V(0.5f, 0.2f, 0.1f), Vec3V(4.0f, 1.0f, 2.0f) );
	Transform3x3( c_1, a_1, b_1 );

	Matrix34 a_2 = RCC_MATRIX34( a_1 );
	Matrix34 b_2 = RCC_MATRIX34( b_1 );
	b_2.Dot3x3( a_2 );
	Mat34V b_2_result = RCC_MAT34V(b_2);

	CHECK( IsCloseAll( c_1, b_2_result, Vec3VFromF32(TEST_EPSILON) ) );
	
	// These should be equivalent 3x3 untransforms of orthonormal 3x4 matrices.
	Mat34V a_3;
	Mat34V b_3;
	Mat34V c_3;
	Mat34VFromEulersXYZ( a_3, Vec3V(1.0f, 0.7f, 0.5f), Vec3V(1.0f, 2.0f, 3.0f) );
	Mat34VFromEulersXYZ( b_3, Vec3V(0.5f, 0.2f, 0.1f), Vec3V(4.0f, 1.0f, 2.0f) );
	UnTransform3x3Ortho( c_3, a_3, b_3 );

	Matrix34 a_4 = RCC_MATRIX34( a_3 );
	Matrix34 b_4 = RCC_MATRIX34( b_3 );
	b_4.Dot3x3Transpose( a_4 );
	Mat34V b_4_result = RCC_MAT34V(b_4);

	CHECK( IsCloseAll( c_3, b_4_result, Vec3VFromF32(TEST_EPSILON) ) );
}

MAT34V_QA_ITEM_BEGIN( Mat34V_CrossProduct_Compatibility )
{
	Matrix34 a(1.0f, 2.0f, 3.0f, 1.0f, 4.0f, 1.0f, 2.0f, 5.0f, 6.0f, 2.0f, 1.0f, 1.0f);
	Vector3 v(1.0f, 2.0f, 3.0f);
	a.CrossProduct(v);

	Mat34V a1(1.0f, 2.0f, 3.0f, 1.0f, 4.0f, 1.0f, 2.0f, 5.0f, 6.0f, 2.0f, 1.0f, 1.0f);
	Vec3V v1(1.0f, 2.0f, 3.0f);
	Mat34V b1;
	CrossProduct(b1, a1, v1);

	CHECK( IsCloseAll( RCC_MAT34V(a), b1, Vec3VFromF32(TEST_EPSILON) ) != 0 );
}

MAT34V_QA_ITEM_BEGIN( Mat34V_Invert_Compatibility )
{
	Matrix34 a(	1.0f, 2.0f, 3.0f,
				4.0f, 1.0f, 2.0f,
				1.0f, 1.0f, 2.0f,
				4.0f, 5.0f, 6.0f );
	Matrix34 a_result = a;
	a_result.Inverse();

	Mat34V b = RCC_MAT34V(a);
	InvertTransformFull( b, b );
	Matrix34 b_result = RCC_MATRIX34(b);

	CHECK( IsCloseAll( RCC_MAT34V(a_result), RCC_MAT34V(b_result), Vec3VFromF32(TEST_EPSILON) ) != 0 );
}




MAT34V_QA_ITEM_END

	if( bRESULT )
	{
		TST_PASS;
	}
	else
	{
		TST_FAIL;
	}
} // end of .Update()

MAT34V_QA_ALL_END


QA_ITEM_FAMILY( qaMAT34V, (), () );
QA_ITEM_FAST( qaMAT34V, (), qaResult::FAIL_OR_TOTAL_TIME );


//================================================
// MEMBER FUNCTIONS
//================================================
/*
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM00f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM00f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM01f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM01f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM02f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM02f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM03f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM03f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM10f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM10f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM11f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM11f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM12f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM12f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM13f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM13f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM20f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM20f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM21f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM21f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM22f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM22f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetM23f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetM23f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetCol0, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetCol0, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetCol1, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetCol1, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetCol2, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetCol2, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_GetCol3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_GetCol3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_operator_access, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_operator_access, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_operator_access_ref, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_operator_access_ref, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// FREE FUNCTIONS
//================================================

Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromXAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromXAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromYAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromYAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromZAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromZAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromQuat, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromQuat, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromScale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromScale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromScale_V1, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromScale_V1, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Determinant3x3V, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Determinant3x3V, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mat34FromTranslation, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mat34FromTranslation, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Transpose3x3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Transpose3x3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Invert3x3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Invert3x3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Invert3x3Ortho, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Invert3x3Ortho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_InvertTransform, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_InvertTransform, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_InvertTransformOrtho, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_InvertTransformOrtho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mul_M34_V4, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mul_M34_V4, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Mul_V3_M34, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Mul_V3_M34, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_UnTransform_M34, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_UnTransform_M34, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_UnTransformOrtho_M34, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_UnTransformOrtho_M34, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_UnTransform3x3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_UnTransform3x3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_UnTransform3x3Ortho, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_UnTransform3x3Ortho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_UnTransform, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_UnTransform, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_UnTransformOrtho, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_UnTransformOrtho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_ReOrthonormalize3x3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_ReOrthonormalize3x3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Add, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Add, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Subtract, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Subtract, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Abs, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Abs, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_Scale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_Scale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_InvScale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_InvScale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat34V_InvScaleSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat34V_InvScaleSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
*/





#endif // __QA
