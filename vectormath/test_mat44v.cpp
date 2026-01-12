
#include "channel.h"
#include "classes.h"
#include "qa/qa.h"


#if __QA

//// Macro for testing a snippet of code.
//#define MAT44V_QA_ITEM_BEGIN(Name)			
//class qa##Name : public qaItem				
//{ public:									
//	void Init() {};							
//	void Update(qaResult& result)
//#define MAT44V_QA_ITEM_END		};

#define MAT44V_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)			
#define MAT44V_QA_ALL_END		};

#define MAT44V_QA_ITEM_BEGIN(Name)	currentTestName = #Name;
#define MAT44V_QA_ITEM_END			currentTestName = NULL;

// Some custom check macros.
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

MAT44V_QA_ALL_START(MAT44V)

{	// beginning of .Update()

	bool bRESULT = true;
	const char* currentTestName = NULL;	

MAT44V_QA_ITEM_BEGIN( Mat44V_GetMat34 )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	
	Mat34V mat34 = testMat.GetMat34();
	Mat34V mat34Compare = Mat34V( testMat.GetCol0Intrin128(), testMat.GetCol1Intrin128(), testMat.GetCol2Intrin128(), testMat.GetCol3Intrin128() );

	CHECK( 0 != IsEqualAll( mat34, mat34Compare ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM00f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM00f();
	CHECK_EQUAL( x, 1.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM01f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM01f();
	CHECK_EQUAL( x, 5.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM02f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM02f();
	CHECK_EQUAL( x, 9.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM03f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM03f();
	CHECK_EQUAL( x, 13.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM10f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM10f();
	CHECK_EQUAL( x, 2.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM11f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM11f();
	CHECK_EQUAL( x, 6.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM12f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM12f();
	CHECK_EQUAL( x, 10.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM13f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM13f();
	CHECK_EQUAL( x, 14.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM20f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM20f();
	CHECK_EQUAL( x, 3.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM21f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM21f();
	CHECK_EQUAL( x, 7.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM22f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM22f();
	CHECK_EQUAL( x, 11.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM23f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM23f();
	CHECK_EQUAL( x, 15.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM30f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM30f();
	CHECK_EQUAL( x, 4.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM31f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM31f();
	CHECK_EQUAL( x, 8.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM32f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM32f();
	CHECK_EQUAL( x, 12.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetM33f )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	float x = testMat.GetM33f();
	CHECK_EQUAL( x, 16.0f );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetCol0 )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	Vec4V col0 = testMat.GetCol0();
	Vec4V col0compare(1.0f, 2.0f, 3.0f, 4.0f);
	CHECK( 0 != IsEqualAll(col0, col0compare) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetCol1 )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	Vec4V col1 = testMat.GetCol1();
	Vec4V col1compare(5.0f, 6.0f, 7.0f, 8.0f);
	CHECK( 0 != IsEqualAll(col1, col1compare) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetCol2 )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	Vec4V col2 = testMat.GetCol2();
	Vec4V col2compare(9.0f, 10.0f, 11.0f, 12.0f);
	CHECK( 0 != IsEqualAll(col2, col2compare) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_GetCol3 )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	Vec4V col2 = testMat.GetCol3();
	Vec4V col2compare(13.0f, 14.0f, 15.0f, 16.0f);
	CHECK( 0 != IsEqualAll(col2, col2compare) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_operator_access )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	bool bResult = true;
	Vec::Vector_4V col0compare = VECTOR4V_LITERAL(1.0f, 2.0f, 3.0f, 4.0f);
	Vec::Vector_4V col1compare = VECTOR4V_LITERAL(5.0f, 6.0f, 7.0f, 8.0f);
	Vec::Vector_4V col2compare = VECTOR4V_LITERAL(9.0f, 10.0f, 11.0f, 12.0f);
	Vec::Vector_4V col3compare = VECTOR4V_LITERAL(13.0f, 14.0f, 15.0f, 16.0f);
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[0], col0compare ) );
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[1], col1compare ) );
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[2], col2compare ) );
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[3], col3compare ) );

	CHECK( bResult );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_operator_access_ref )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );
	bool bResult = true;
	Vec::Vector_4V col0new = VECTOR4V_LITERAL(17.0f, 18.0f, 19.0f, 20.0f);
	Vec::Vector_4V col1new = VECTOR4V_LITERAL(21.0f, 22.0f, 23.0f, 24.0f);
	Vec::Vector_4V col2new = VECTOR4V_LITERAL(25.0f, 26.0f, 27.0f, 28.0f);
	Vec::Vector_4V col3new = VECTOR4V_LITERAL(29.0f, 30.0f, 31.0f, 32.0f);
	testMat[0] = col0new;
	testMat[1] = col1new;
	testMat[2] = col2new;
	testMat[3] = col3new;
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[0], col0new ) );
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[1], col1new ) );
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[2], col2new ) );
	bResult = bResult && ( 0 != Vec::V4IsEqualAll( testMat[3], col3new ) );

	CHECK( bResult );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromAxisAngle )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat44V rotMatrix;
	Mat44VFromAxisAngle( rotMatrix, normAxis, angle, Vec4V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	// 0.0f				0.0f			0.0f			transW
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

	Mat44V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s, 0.0f,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s, 0.0f,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c, 0.0f,
								1.0f, 1.0f, 1.0f, 1.0f	);
	
	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromXAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat44V rotMatrix;
	Mat44VFromXAxisAngle( rotMatrix, angle, Vec4V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX						
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	// 0.0f				0.0f			0.0f			transW
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

	Mat44V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s, 0.0f,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s, 0.0f,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c, 0.0f,
								1.0f, 1.0f, 1.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromYAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat44V rotMatrix;
	Mat44VFromYAxisAngle( rotMatrix, angle, Vec4V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX						
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	// 0.0f				0.0f			0.0f			transW
	
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

	Mat44V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s, 0.0f,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s, 0.0f,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c, 0.0f,
								1.0f, 1.0f, 1.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromZAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat44V rotMatrix;
	Mat44VFromZAxisAngle( rotMatrix, angle, Vec4V(V_ONE) );

	// Should be:
	// t*x*x + c		t*x*y - z*s		t*x*z + y*s		transX
	// t*x*y + z*s		t*y*y + c		t*y*z - x*s		transY
	// t*x*z - y*s		t*y*z + x*s		t*z*z + c		transZ
	// 0.0f				0.0f			0.0f			transW
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

	Mat44V rotMatrixCompare(	t*x*x + c, t*x*y + z*s, t*x*z - y*s, 0.0f,
								t*x*y - z*s, t*y*y + c, t*y*z + x*s, 0.0f,
								t*x*z + y*s, t*y*z - x*s, t*z*z + c, 0.0f,
								1.0f, 1.0f, 1.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromQuat )
{
	QuatV quat( 1.0f, 2.0f, 3.0f, 4.0f );
	quat = Normalize( quat );
	Mat44V rotMatrix;
	Mat44VFromQuatV( rotMatrix, quat, Vec4V(V_ONE) );

	// Should be:
	// 1 - 2*qy^2 - 2*qz^2		2*qx*qy - 2*qz*qw		2*qx*qz + 2*qy*qw		transX
	// 2*qx*qy + 2*qz*qw		1 - 2*qx^2 - 2*qz^2		2*qy*qz - 2*qx*qw		transY
	// 2*qx*qz - 2*qy*qw		2*qy*qz + 2*qx*qw		1 - 2*qx^2 - 2*qy^2		transZ
	// 0.0f						0.0f					0.0f					transW
	// 

	float qx = quat.GetXf();
	float qy = quat.GetYf();
	float qz = quat.GetZf();
	float qw = quat.GetWf();	

	Mat44V rotMatrixCompare(	1 - 2*qy*qy - 2*qz*qz,		2*qx*qy + 2*qz*qw,			2*qx*qz - 2*qy*qw,		0.0f,
								2*qx*qy - 2*qz*qw,			1 - 2*qx*qx - 2*qz*qz,		2*qy*qz + 2*qx*qw,		0.0f,
								2*qx*qz + 2*qy*qw,			2*qy*qz - 2*qx*qw,			1 - 2*qx*qx - 2*qy*qy,	0.0f,
								1.0f,						1.0f,						1.0f,					1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromEulersXYZ )
{
	// (R,P,Y) = (pi/4, pi, pi/2)
	Vec3V rpyVect( 0.78539816339744830961566084581988f, 3.1415926535897932384626433832795f, 1.5707963267948966192313216916398f );
	Mat44V rotMatrix;
	Mat44VFromEulersXYZ( rotMatrix, rpyVect, Vec4V(V_ONE) );

	// Should be:
	// cos(Y)*cos(P)		cos(Y)*sin(P)*sin(R)-sin(Y)*cos(R)		cos(Y)*sin(P)*cos(R)+sin(Y)*sin(R)		transX
	// sin(Y)*cos(P)		sin(Y)*sin(P)*sin(R)+cos(Y)*cos(R)		sin(Y)*sin(P)*cos(R)-cos(Y)*sin(R)		transY
	// -sin(P)				cos(P)*sin(R)							cos(P)*cos(R)							transZ
	// 0.0f					0.0f									0.0f									transW
	// 
	//
	// (http://planning.cs.uiuc.edu/node102.html)

	float R = rpyVect.GetXf();
	float P = rpyVect.GetYf();
	float Y = rpyVect.GetZf();

	Mat44V rotMatrixCompare(	cosf(Y)*cosf(P),							sinf(Y)*cosf(P),							-sinf(P),			0.0f,
								cosf(Y)*sinf(P)*sinf(R)-sinf(Y)*cosf(R),	sinf(Y)*sinf(P)*sinf(R)+cosf(Y)*cosf(R),	cosf(P)*sinf(R),	0.0f,
								cosf(Y)*sinf(P)*cosf(R)+sinf(Y)*sinf(R),	sinf(Y)*sinf(P)*cosf(R)-cosf(Y)*sinf(R),	cosf(P)*cosf(R),	0.0f,
								1.0f,										1.0f,										1.0f,				1.0f	);

	CHECK( 0 != IsCloseAll( rotMatrix, rotMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromScale )
{
	Vec3V scaleVect( 2.0f, 3.0f, 4.0f );
	Mat44V scaleMatrix;
	Mat44VFromScale( scaleMatrix, scaleVect, Vec4V(V_ONE) );

	Mat44V scaleMatrixCompare(	2.0f,	0.0f,	0.0f,	0.0f, 
								0.0f,	3.0f,	0.0f, 	0.0f,
								0.0f,	0.0f,	4.0f,	0.0f,
								1.0f,	1.0f,	1.0f,	1.0f	);

	CHECK( 0 != IsEqualAll( scaleMatrix, scaleMatrixCompare ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromScale_V1 )
{
	Mat44V scaleMatrix;
	Mat44VFromScale( scaleMatrix, ScalarVFromF32(2.0f), ScalarVFromF32(3.0f), ScalarVFromF32(4.0f), Vec4V(V_ONE) );

	Mat44V scaleMatrixCompare(	2.0f,	0.0f,	0.0f,	0.0f, 
								0.0f,	3.0f,	0.0f, 	0.0f,
								0.0f,	0.0f,	4.0f,	0.0f,
								1.0f,	1.0f,	1.0f,	1.0f	);

	CHECK( 0 != IsEqualAll( scaleMatrix, scaleMatrixCompare ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_DeterminantV )
{
	Mat44V inputMat(	1.0f, -5.0f, 2.0f, -3.0f,
						2.0f, 1.0f, 1.0f, 2.0f,
						3.0f, 4.0f, 5.0f, 2.0f,
						1.0f, 1.0f, 1.0f, 2.0f	);

	ScalarV det = Determinant( inputMat );
	ScalarV detCompare = ScalarVFromF32( 49.0f );

	CHECK( 0 != IsCloseAll( det, detCompare, ScalarVFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mat44FromTranslation )
{
	Vec4V translationVect( 2.0f, 3.0f, 4.0f, 1.0f );
	Mat44V translationMatrix;
	Mat44VFromTranslation( translationMatrix, translationVect );
	Mat44V translationMatrixCompare(	1.0f, 0.0f, 0.0f, 0.0f,
										0.0f, 1.0f, 0.0f, 0.0f,
										0.0f, 0.0f, 1.0f, 0.0f,
										2.0f, 3.0f, 4.0f, 1.0f	);

	CHECK( 0 != IsCloseAll( translationMatrix, translationMatrixCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Transpose )
{
	Mat44V testMat(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f );

	Mat44V transpose;
	Transpose( transpose, testMat );
	Mat44V transposeCompare(	1.0f, 5.0f, 9.0f, 13.0f,
								2.0f, 6.0f, 10.0f, 14.0f,
								3.0f, 7.0f, 11.0f, 15.0f,
								4.0f, 8.0f, 12.0f, 16.0f );

	CHECK( 0 != IsEqualAll( transpose, transposeCompare ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Invert )
{
	Mat44V inputMat(	1.0f, -5.0f, 2.0f, -3.0f,
						2.0f, 1.0f, 1.0f, 2.0f,
						3.0f, 4.0f, 5.0f, 2.0f,
						1.0f, 1.0f, 1.0f, 2.0f	);

	Mat44V invertMat;
	InvertFull( invertMat, inputMat );

	Mat44V invertMatCompare(	0.0f,			1.0f,			0.0f,			-1.0f,
								-0.163265306f,	0.122448980f,	0.142857143f,	-0.510204082f,
								0.122448980f,	-0.591836735f,	0.142857143f,	0.632653061f,
								0.020408163f,	-0.265306122f,	-0.142857143f,	0.938775510f	);

	CHECK( 0 != IsCloseAll( invertMat, invertMatCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Invert3x4 )
{
	Mat44V inputMat(	1.0f, -5.0f, 2.0f, -30.0f,
						2.0f, 1.0f, 1.0f, 20.0f,
						3.0f, 4.0f, 5.0f, 200.0f,
						1.0f, 1.0f, 1.0f, -20.0f	);

	Mat34V inputMat3x4(	1.0f, -5.0f, 2.0f,
						2.0f, 1.0f, 1.0f,
						3.0f, 4.0f, 5.0f,
						1.0f, 1.0f, 1.0f	);

	// These two should be equivalent, except with the 4x4 having the last row = 0,0,0,1
	Mat44V invMat4x4;
	Invert3x4Full( invMat4x4, inputMat );
	Mat34V invMat3x4;
	InvertTransformFull( invMat3x4, inputMat3x4 );

	Mat44V invMat3x4CompareMat = Mat44V( invMat3x4 );
	invMat3x4CompareMat.SetM30M31M32Zero();
	invMat3x4CompareMat.SetM33f( 1.0f );

	CHECK( 0 != IsCloseAll( invMat4x4, invMat3x4CompareMat, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Invert3x4Ortho )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	Mat44V rotMatrix;
	Mat44VFromAxisAngle( rotMatrix, normAxis, angle, Vec4V(V_ONE) );

	Mat44V inputMat;
	Mat44VFromAxisAngle( inputMat, normAxis, angle, Vec4V(V_ONE) );
	Mat34V inputMat3x4;
	Mat34VFromAxisAngle( inputMat3x4, normAxis, angle, Vec3V(V_ONE) );

	// These two should be equivalent, except with the 4x4 having the last row = 0,0,0,1
	Mat44V invMat4x4;
	Invert3x4Ortho( invMat4x4, inputMat );
	Mat34V invMat3x4;
	InvertTransformOrtho( invMat3x4, inputMat3x4 );

	Mat44V invMat3x4CompareMat = Mat44V( invMat3x4 );
	invMat3x4CompareMat.SetM30M31M32Zero();
	invMat3x4CompareMat.SetM33f( 1.0f );

	CHECK( 0 != IsCloseAll( invMat4x4, invMat3x4CompareMat, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mul_M44_M44 )
{
	Mat44V inputMatA(	1.0f, -5.0f, 2.0f, -3.0f,
						2.0f, 1.0f, 1.0f, 2.0f,
						3.0f, 4.0f, 5.0f, 2.0f,
						1.0f, 1.0f, 1.0f, 2.0f	);

	Mat44V inputMatB(	1.0f, -5.0f, 8.0f, -3.0f,
						2.0f, 0.5f, 2.0f, -1.0f,
						3.0f, 4.0f, 5.0f, 2.0f,
						3.0f, 1.0f, 6.0f, 1.0f	);

	Mat44V prod;
	Multiply( prod, inputMatA, inputMatB );
	
	Mat44V prodCompare(		12.0f, 19.0f, 34.0f, -3.0f,
							8.0f, -2.5f, 13.5f, -3.0f,
							28.0f, 11.0f, 37.0f, 13.0f,
							24.0f, 11.0f, 38.0f, 7.0f	);

	CHECK( 0 != IsCloseAll( prod, prodCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mul_M44_V4 )
{
	Mat44V inputMatA(	1.0f, -5.0f, 2.0f, -3.0f,
						2.0f, 1.0f, 1.0f, 2.0f,
						3.0f, 4.0f, 5.0f, 2.0f,
						1.0f, 1.0f, 1.0f, 2.0f	);

	Vec4V inputVecB(	1.0f, 2.0f, 3.0f, 4.0f	);

	Vec4V prodVec = Multiply( inputMatA, inputVecB );
	Vec4V prodVecCompare( 18.0f, 13.0f, 23.0f, 15.0f );

	CHECK( 0 != IsCloseAll( prodVec, prodVecCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Mul_V4_M44 )
{
	Mat44V inputMatB(	1.0f, -5.0f, 2.0f, -3.0f,
						2.0f, 1.0f, 1.0f, 2.0f,
						3.0f, 4.0f, 5.0f, 2.0f,
						1.0f, 1.0f, 1.0f, 2.0f	);

	Vec4V inputVecA(	1.0f, 2.0f, 3.0f, 4.0f	);

	Vec4V prodVec = Multiply( inputVecA, inputMatB );
	Vec4V prodVecCompare( -15.0f, 15.0f, 34.0f, 14.0f );

	CHECK( 0 != IsCloseAll( prodVec, prodVecCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_UnTransform_M44 )
{
	Mat44V transformMat(	1.0f, -5.0f, 2.0f, -3.0f,
							2.0f, 1.0f, 1.0f, 2.0f,
							3.0f, 4.0f, 5.0f, 2.0f,
							1.0f, 1.0f, 1.0f, 2.0f	);
	Mat44V matToTransform(	1.0f, 2.0f, 1.0f, 1.0f,
							-3.0f, 2.0f, 0.0f, 1.0f,
							1.0f, 1.0f, 2.0f, 1.0f,
							5.0f, 5.0f, 5.0f, 1.0f	);

	Mat44V transformedMat;
	Multiply( transformedMat, transformMat, matToTransform );
	Mat44V untransformedMat;
	UnTransformFull( untransformedMat, transformMat, transformedMat );

	CHECK( 0 != IsCloseAll( untransformedMat, matToTransform, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_UnTransformOrtho_M44 )
{
	Mat44V orthoTransformMat(	0.0f, 0.0f, 1.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f,
								0.0f, 1.0f, 0.0f, 0.0f,
								1.0f, 0.0f, 0.0f, 0.0f	);

	Mat44V matToTransform(	1.0f, 2.0f, 1.0f, 1.0f,
							-3.0f, 2.0f, 0.0f, 1.0f,
							1.0f, 1.0f, 2.0f, 1.0f,
							5.0f, 5.0f, 5.0f, 1.0f	);

	Mat44V transformedMat;
	Multiply( transformedMat, orthoTransformMat, matToTransform );
	Mat44V untransformedMat;
	UnTransformOrtho( untransformedMat, orthoTransformMat, transformedMat );

	CHECK( 0 != IsCloseAll( untransformedMat, matToTransform, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_UnTransform_V4 )
{
	Mat44V transformMat(	1.0f, -5.0f, 2.0f, -3.0f,
							2.0f, 1.0f, 1.0f, 2.0f,
							3.0f, 4.0f, 5.0f, 2.0f,
							1.0f, 1.0f, 1.0f, 2.0f	);

	Vec4V vecToTransform(	1.0f, 2.0f, 3.0f, 4.0f	);

	Vec4V transformedVec = Multiply( transformMat, vecToTransform );
	Vec4V untransformedVec = UnTransformFull( transformMat, transformedVec );

	CHECK( 0 != IsCloseAll( untransformedVec, vecToTransform, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_UnTransformOrtho_V4 )
{
	Mat44V orthoTransformMat(	0.0f, 0.0f, 1.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f,
								0.0f, 1.0f, 0.0f, 0.0f,
								1.0f, 0.0f, 0.0f, 0.0f	);

	Vec4V vecToTransform(	1.0f, 2.0f, 3.0f, 4.0f	);

	Vec4V transformedVec = Multiply( orthoTransformMat, vecToTransform );
	Vec4V untransformedVec = UnTransformOrtho( orthoTransformMat, transformedVec );

	CHECK( 0 != IsCloseAll( untransformedVec, vecToTransform, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_ReOrthonormalize3x3 )
{
	bool bResult = true;

	ScalarV angle = ScalarVFromF32( 0.78539816339744830961566084581988f ); // pi/4
	Mat44V orthoTransformMat;
	Mat44VFromYAxisAngle( orthoTransformMat, angle );

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
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Add )
{
	Mat44V matA(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f	);

	Mat44V matB(	-1.0f, -2.0f, -3.0f, -4.0f,
					-5.0f, -6.0f, -7.0f, -8.0f,
					-9.0f, -10.0f, -11.0f, -12.0f,
					-13.0f, -14.0f, -15.0f, -16.0f	);

	Mat44V matSum;
	Add( matSum, matA, matB );
	Mat44V matSumCompare(V_ZERO);
	
	CHECK( 0 != IsCloseAll( matSum, matSumCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Subtract )
{
	Mat44V matA(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f	);

	Mat44V matB(	-1.0f, -2.0f, -3.0f, -4.0f,
					-5.0f, -6.0f, -7.0f, -8.0f,
					-9.0f, -10.0f, -11.0f, -12.0f,
					-13.0f, -14.0f, -15.0f, -16.0f	);

	Mat44V matDiff;
	Subtract( matDiff, matA, matB );

	Mat44V matDiffCompare(	2*1.0f, 2*2.0f, 2*3.0f, 2*4.0f,
							2*5.0f, 2*6.0f, 2*7.0f, 2*8.0f,
							2*9.0f, 2*10.0f, 2*11.0f, 2*12.0f,
							2*13.0f, 2*14.0f, 2*15.0f, 2*16.0f	);
	
	CHECK( 0 != IsCloseAll( matDiff, matDiffCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Abs )
{
	Mat44V matA(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f	);

	Mat44V matB(	-1.0f, -2.0f, -3.0f, -4.0f,
					-5.0f, -6.0f, -7.0f, -8.0f,
					-9.0f, -10.0f, -11.0f, -12.0f,
					-13.0f, -14.0f, -15.0f, -16.0f	);

	Mat44V matAbs;
	Abs( matAbs, matB );
	
	CHECK( 0 != IsCloseAll( matAbs, matA, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_Scale )
{
	Mat44V matA(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f	);

	Vec4V scaleVect( -1.0f, -2.0f, -3.0f, -4.0f );

	Mat44V matScaledVersion1;
	Scale( matScaledVersion1, matA, scaleVect );
	Mat44V matScaledVersion2;
	Scale( matScaledVersion2, scaleVect, matA );

	Mat44V matScaledCompare(	-1.0f, -4.0f, -9.0f, -16.0f,
								-5.0f, -12.0f, -21.0f, -32.0f,
								-9.0f, -20.0f, -33.0f, -48.0f,
								-13.0f, -28.0f, -45.0f, -64.0f	);

	CHECK(	0 != IsCloseAll( matScaledVersion1, matScaledCompare, Vec4VFromF32(TEST_EPSILON) ) &&
			0 != IsCloseAll( matScaledVersion2, matScaledCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_InvScale )
{
	Mat44V matA(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f	);

	Vec4V scaleVect( -1.0f, -2.0f, -3.0f, -4.0f );

	Mat44V matScaled;
	InvScale( matScaled, matA, scaleVect );

	Mat44V matScaledCompare(	-1.0f, -1.0f, -1.0f, -1.0f,
								-5.0f, -3.0f, -7.0f/3.0f, -2.0f,
								-9.0f, -5.0f, -11.0f/3.0f, -3.0f,
								-13.0f, -7.0f, -5.0f, -4.0f	);
	
	CHECK( 0 != IsCloseAll( matScaled, matScaledCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_InvScaleSafe )
{
	Mat44V matA(	1.0f, 2.0f, 3.0f, 4.0f,
					5.0f, 6.0f, 7.0f, 8.0f,
					9.0f, 10.0f, 11.0f, 12.0f,
					13.0f, 14.0f, 15.0f, 16.0f	);

	Vec4V scaleVect( -1.0f, -2.0f, -3.0f, 0.0f );

	Mat44V matScaled;
	InvScaleSafe( matScaled, matA, scaleVect, Vec4V(V_ZERO) );

	Mat44V matScaledCompare(	-1.0f, -1.0f, -1.0f, 0.0f,
								-5.0f, -3.0f, -7.0f/3.0f, 0.0f,
								-9.0f, -5.0f, -11.0f/3.0f, 0.0f,
								-13.0f, -7.0f, -5.0f, 0.0f	);
	
	CHECK( 0 != IsCloseAll( matScaled, matScaledCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
MAT44V_QA_ITEM_END

MAT44V_QA_ITEM_BEGIN( Mat44V_OuterProduct )
{
	Mat44V outerProduct;
	OuterProduct(outerProduct, Vec4V(1.0f, 2.0f, 3.0f, 4.0f), Vec4V(3.0f, 4.0f, 5.0f, 6.0f));

	Mat44V outerProductCompare(Mat44V::ROW_MAJOR,
		3.0f, 4.0f, 5.0f, 6.0f,
		6.0f, 8.0f, 10.0f, 12.0f,
		9.0f, 12.0f, 15.0f, 18.0f,
		12.0f, 16.0f, 20.0f, 24.0f
		);
	CHECK (0 != IsCloseAll(outerProduct, outerProductCompare, Vec4VFromF32(TEST_EPSILON)));
}
MAT44V_QA_ITEM_END

	if( bRESULT )
	{
		TST_PASS;
	}
	else
	{
		TST_FAIL;
	}
} // end of .Update()

MAT44V_QA_ALL_END


QA_ITEM_FAMILY( qaMAT44V, (), () );
QA_ITEM_FAST( qaMAT44V, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// MEMBER FUNCTIONS
//================================================
/*
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetMat34, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetMat34, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM00f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM00f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM01f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM01f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM02f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM02f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM03f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM03f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM10f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM10f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM11f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM11f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM12f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM12f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM13f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM13f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM20f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM20f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM21f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM21f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM22f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM22f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM23f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM23f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM30f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM30f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM31f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM31f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM32f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM32f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetM33f, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetM33f, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetCol0, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetCol0, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetCol1, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetCol1, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetCol2, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetCol2, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_GetCol3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_GetCol3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_operator_access, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_operator_access, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_operator_access_ref, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_operator_access_ref, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// FREE FUNCTIONS
//================================================

Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromXAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromXAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromYAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromYAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromZAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromZAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromQuat, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromQuat, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromScale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromScale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromScale_V1, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromScale_V1, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_DeterminantV, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_DeterminantV, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mat44FromTranslation, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mat44FromTranslation, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Transpose, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Transpose, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Invert, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Invert, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Invert3x4, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Invert3x4, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Invert3x4Ortho, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Invert3x4Ortho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mul_M44_M44, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mul_M44_M44, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mul_M44_V4, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mul_M44_V4, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Mul_V4_M44, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Mul_V4_M44, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_UnTransform_M44, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_UnTransform_M44, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_UnTransformOrtho_M44, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_UnTransformOrtho_M44, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_UnTransform_V4, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_UnTransform_V4, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_UnTransformOrtho_V4, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_UnTransformOrtho_V4, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_ReOrthonormalize3x3, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_ReOrthonormalize3x3, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Add, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Add, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Subtract, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Subtract, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Abs, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Abs, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_Scale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_Scale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_InvScale, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_InvScale, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaMat44V_InvScaleSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaMat44V_InvScaleSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
*/



#endif // __QA
