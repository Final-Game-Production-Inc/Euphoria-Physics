

#include "classes.h"
#include "legacyconvert.h"
#include "math/random.h"
#include "qa/qa.h"
#include "vector/quaternion.h"


#if __QA

//// Macro for testing a snippet of code.
//#define QUATV_QA_ITEM_BEGIN(Name)		
//class qa##Name : public qaItem			
//{ public:								
//	void Init() {};						
//	void Update(qaResult& result)
//#define QUATV_QA_ITEM_END		};

#define QUATV_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define QUATV_QA_ALL_END		};

#define QUATV_QA_ITEM_BEGIN(Name)	;
#define QUATV_QA_ITEM_END			;

// Some custom check macros.
#define CHECK(X)				( bRESULT = bRESULT && (X) )
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON						0.001f
#define TEST_EPSILON_LENIENT				0.05f
#define TEST_EPSILON_REALLY_LENIENT			0.10f
#define TEST_EPSILON_REALLY_REALLY_LENIENT	0.30f



using namespace rage;
using namespace Vec;

QUATV_QA_ALL_START(QUATV)

{	// beginning of .Update()

	bool bRESULT = true;

QUATV_QA_ITEM_BEGIN( QuatV_GetXf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	float x = testQuat.GetXf();
	CHECK_EQUAL( x, 1.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetYf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	float x = testQuat.GetYf();
	CHECK_EQUAL( x, 2.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetZf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	float x = testQuat.GetZf();
	CHECK_EQUAL( x, 3.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetWf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	float x = testQuat.GetWf();
	CHECK_EQUAL( x, 4.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetX )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = testQuat.GetX();
	CHECK_EQUAL( testQuat.GetXf(), testVect2.Getf() );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetY )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = testQuat.GetY();
	CHECK_EQUAL( testQuat.GetYf(), testVect2.Getf() );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetZ )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = testQuat.GetZ();
	CHECK_EQUAL( testQuat.GetZf(), testVect2.Getf() );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetW )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = testQuat.GetW();
	CHECK_EQUAL( testQuat.GetWf(), testVect2.Getf() );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_GetXYZ )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	Vec3V testVect2 = testQuat.GetXYZ();
	CHECK(	testQuat.GetXf() == testVect2.GetXf() &&
			testQuat.GetYf() == testVect2.GetYf() &&
			testQuat.GetZf() == testVect2.GetZf() );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetXf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	testQuat.SetXf( 5.0f );
	CHECK_EQUAL( testQuat.GetXf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetYf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	testQuat.SetYf( 5.0f );
	CHECK_EQUAL( testQuat.GetYf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetZf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	testQuat.SetZf( 5.0f );
	CHECK_EQUAL( testQuat.GetZf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetWf )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	testQuat.SetWf( 5.0f );
	CHECK_EQUAL( testQuat.GetWf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetX )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = ScalarVFromF32( 5.0f );
	testQuat.SetX( testVect2 );
	CHECK_EQUAL( testQuat.GetXf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetY )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = ScalarVFromF32( 5.0f );
	testQuat.SetY( testVect2 );
	CHECK_EQUAL( testQuat.GetYf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetZ )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = ScalarVFromF32( 5.0f );
	testQuat.SetZ( testVect2 );
	CHECK_EQUAL( testQuat.GetZf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetW )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	ScalarV testVect2 = ScalarVFromF32( 5.0f );
	testQuat.SetW( testVect2 );
	CHECK_EQUAL( testQuat.GetWf(), 5.0f );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_ZeroComponents )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	testQuat.ZeroComponents();
	QuatV quatZero = QuatV(V_ZERO);
	CHECK( 0 != IsEqualAll( testQuat, quatZero ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SetWZero )
{
	QuatV testQuat( 1.0f, 2.0f, 3.0f, 4.0f );
	testQuat.SetWZero();
	QuatV testQuat2( 1.0f, 2.0f, 3.0f, 0.0f );
	CHECK( 0 != IsEqualAll( testQuat, testQuat2 ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_QuatFromAxisAngle )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	QuatV outQuat = QuatVFromAxisAngle( normAxis, angle );

	float ax = normAxis.GetXf();
	float ay = normAxis.GetYf();
	float az = normAxis.GetZf();
	float s = sinf(angle.Getf() / 2.0f);
	float c = cosf(angle.Getf() / 2.0f);

	QuatV outQuatCompare( ax*s, ay*s, az*s, c );

	CHECK( 0 != IsCloseAll( outQuat, outQuatCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_QuatFromXAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	QuatV outQuat = QuatVFromXAxisAngle( angle );

	float ax = 1.0f;
	float ay = 0.0f;
	float az = 0.0f;
	float s = sinf(angle.Getf() / 2.0f);
	float c = cosf(angle.Getf() / 2.0f);

	QuatV outQuatCompare( ax*s, ay*s, az*s, c );

	CHECK( 0 != IsCloseAll( outQuat, outQuatCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_QuatFromYAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	QuatV outQuat = QuatVFromYAxisAngle( angle );

	float ax = 0.0f;
	float ay = 1.0f;
	float az = 0.0f;
	float s = sinf(angle.Getf() / 2.0f);
	float c = cosf(angle.Getf() / 2.0f);

	QuatV outQuatCompare( ax*s, ay*s, az*s, c );

	CHECK( 0 != IsCloseAll( outQuat, outQuatCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_QuatFromZAxisAngle )
{
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	QuatV outQuat = QuatVFromZAxisAngle( angle );

	float ax = 0.0f;
	float ay = 0.0f;
	float az = 1.0f;
	float s = sinf(angle.Getf() / 2.0f);
	float c = cosf(angle.Getf() / 2.0f);

	QuatV outQuatCompare( ax*s, ay*s, az*s, c );

	CHECK( 0 != IsCloseAll( outQuat, outQuatCompare, Vec4VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_QuatFromMat33 )
{
	bool bResult = true;

	// Rotation matrix should not have to be orthogonal for this function.

	Mat33V rotMatrixCase0(	1.0f, 0.0f, 0.2f,
							0.3f, 1.0f, 0.4f,
							0.2f, 1.0f, 1.0f	);
	Mat33V rotMatrixCase1(	1.0f, 0.4f, 0.0f,
							0.3f, -5.0f, 0.0f,
							0.2f, 1.0f, 0.5f	);
	Mat33V rotMatrixCase2(	-5.0f, 0.4f, 0.0f,
							0.3f, 1.0f, 0.0f,
							0.2f, 1.0f, 0.5f	);
	Mat33V rotMatrixCase3(	-5.0f, 0.4f, 0.0f,
							0.3f, 0.5f, 0.0f,
							0.2f, 1.0f, 1.0f	);

	QuatV outQuat0 = QuatVFromMat33V( rotMatrixCase0 );
	QuatV outQuat1 = QuatVFromMat33V( rotMatrixCase1 );
	QuatV outQuat2 = QuatVFromMat33V( rotMatrixCase2 );
	QuatV outQuat3 = QuatVFromMat33V( rotMatrixCase3 );

	// CASE 0: trace > 0
	{
		float trace = rotMatrixCase0.GetM00f() + rotMatrixCase0.GetM11f() + rotMatrixCase0.GetM22f() + 1.0f;
		float s = 0.5f / sqrtf( trace );
		QuatV outQuat0Compare(	(rotMatrixCase0.GetM21f()-rotMatrixCase0.GetM12f())*s,
								(rotMatrixCase0.GetM02f()-rotMatrixCase0.GetM20f())*s,
								(rotMatrixCase0.GetM10f()-rotMatrixCase0.GetM01f())*s,
								0.25f / s	);
		QuatV outQuat0CompareAlt = QuatV( outQuat0Compare.GetXf(), outQuat0Compare.GetYf(), outQuat0Compare.GetZf(), -outQuat0Compare.GetWf() );

		// This function may actually compute the shortest-rotation quaternion.
		// TODO: Verify this! For now, as long as |w| is correct, the rotation is a correct one.
		bResult = bResult && (	0 != IsCloseAll( outQuat0, outQuat0Compare, Vec4VFromF32(TEST_EPSILON) ) ||
								0 != IsCloseAll( outQuat0, outQuat0CompareAlt, Vec4VFromF32(TEST_EPSILON) ) );
	}

	// CASE 1: else, m00 > m11 && m00 > m22
	{
		float s = sqrtf( 1.0f + rotMatrixCase1.GetM00f() - rotMatrixCase1.GetM11f() - rotMatrixCase1.GetM22f() ) * 2.0f;
		float qx = 0.25f * s;
		float qy = (rotMatrixCase1.GetM01f() + rotMatrixCase1.GetM10f()) / s;
		float qz = (rotMatrixCase1.GetM02f() + rotMatrixCase1.GetM20f()) / s;
		float qw = (rotMatrixCase1.GetM12f() - rotMatrixCase1.GetM21f()) / s;
		QuatV outQuat1Compare( qx, qy, qz, qw );
		QuatV outQuat1CompareAlt = QuatV( outQuat1Compare.GetXf(), outQuat1Compare.GetYf(), outQuat1Compare.GetZf(), -outQuat1Compare.GetWf() );

		// This function may actually compute the shortest-rotation quaternion.
		// TODO: Verify this! For now, as long as |w| is correct, the rotation is a correct one.
		bResult = bResult && (	0 != IsCloseAll( outQuat1, outQuat1Compare, Vec4VFromF32(TEST_EPSILON) ) ||
								0 != IsCloseAll( outQuat1, outQuat1CompareAlt, Vec4VFromF32(TEST_EPSILON) ) );
	}

	// CASE 2: else, m11 > m00 && m11 > m22
	{
		float s = sqrtf( 1.0f + rotMatrixCase2.GetM11f() - rotMatrixCase2.GetM00f() - rotMatrixCase2.GetM22f() ) * 2.0f;
		float qx = (rotMatrixCase2.GetM01f() + rotMatrixCase2.GetM10f()) / s;
		float qy = 0.25f * s;
		float qz = (rotMatrixCase2.GetM12f() + rotMatrixCase2.GetM21f()) / s;
		float qw = (rotMatrixCase2.GetM02f() + rotMatrixCase2.GetM20f()) / s;
		QuatV outQuat2Compare( qx, qy, qz, qw );
		QuatV outQuat2CompareAlt = QuatV( outQuat2Compare.GetXf(), outQuat2Compare.GetYf(), outQuat2Compare.GetZf(), -outQuat2Compare.GetWf() );

		// This function may actually compute the shortest-rotation quaternion.
		// TODO: Verify this! For now, as long as |w| is correct, the rotation is a correct one.
		bResult = bResult && (	0 != IsCloseAll( outQuat2, outQuat2Compare, Vec4VFromF32(TEST_EPSILON) ) ||
								0 != IsCloseAll( outQuat2, outQuat2CompareAlt, Vec4VFromF32(TEST_EPSILON) ) );
	}

	// CASE 3: else, m22 > m00 && m22 > m11
	{
		float s = sqrtf( 1.0f + rotMatrixCase3.GetM22f() - rotMatrixCase3.GetM00f() - rotMatrixCase3.GetM11f() ) * 2.0f;
		float qx = (rotMatrixCase3.GetM02f() + rotMatrixCase3.GetM20f()) / s;
		float qy = (rotMatrixCase3.GetM12f() + rotMatrixCase3.GetM21f()) / s;
		float qz = 0.25f * s;
		float qw = (rotMatrixCase3.GetM01f() - rotMatrixCase3.GetM10f()) / s;
		QuatV outQuat3Compare( qx, qy, qz, qw );
		QuatV outQuat3CompareAlt = QuatV( outQuat3Compare.GetXf(), outQuat3Compare.GetYf(), outQuat3Compare.GetZf(), -outQuat3Compare.GetWf() );

		// This function may actually compute the shortest-rotation quaternion.
		// TODO: Verify this! For now, as long as |w| is correct, the rotation is a correct one.
		bResult = bResult && (	0 != IsCloseAll( outQuat3, outQuat3Compare, Vec4VFromF32(TEST_EPSILON) ) ||
								0 != IsCloseAll( outQuat3, outQuat3CompareAlt, Vec4VFromF32(TEST_EPSILON) ) );
	}

	CHECK( bResult );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_QuatFromMat33Ortho )
{
	//// This rotation matrix has trace > 0.0f, so we're ok to use QuatFromMat33Ortho() on it.
	//Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	//ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4
	//Mat33V rotMat;
	//Mat33VFromAxisAngle( rotMat, normAxis, angle );

	//QuatV outQuat = QuatVFromMat33VOrtho( rotMat );

	//float qw = sqrtf( 1.0f + rotMat.GetM00f() + rotMat.GetM11f() + rotMat.GetM22f() ) / 2.0f;
	//float qx = (rotMat.GetM21f() - rotMat.GetM12f()) / (4.0f * qw);
	//float qy = (rotMat.GetM02f() - rotMat.GetM20f()) / (4.0f * qw);
	//float qz = (rotMat.GetM10f() - rotMat.GetM01f()) / (4.0f * qw);
	//QuatV outQuatCompare( qx, qy, qz, qw );

	//CHECK( 0 != IsCloseAll( outQuat, outQuatCompare, Vec4VFromF32(TEST_EPSILON) )  );
	CHECK(true);
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_Conjugate )
{
	QuatV q( 1.0f, 2.0f, 3.0f, 4.0f );
	QuatV conj = Conjugate( q );

	QuatV conjCompare( -1.0f, -2.0f, -3.0f, 4.0f );

	CHECK( 0 != IsCloseAll( conj, conjCompare, Vec4VFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_Normalize )
{
	QuatV q( 1.0f, 3.0f, 3.0f, 9.0f );
	QuatV normQuat = Normalize( q );

	QuatV normQuatCompare( 0.1f, 0.3f, 0.3f, 0.9f );

	CHECK( 0 != IsCloseAll( normQuat, normQuatCompare, Vec4VFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_NormalizeSafe )
{
	QuatV q( 0.0f, 0.0f, 0.0f, 0.0f );
	QuatV normQuat = NormalizeSafe( q );

	QuatV normQuatCompare = QuatV(V_FLT_LARGE_8);

	CHECK( 0 != IsEqualAll( normQuat, normQuatCompare )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_NormalizeFast )
{
	QuatV q( 1.0f, 3.0f, 3.0f, 9.0f );
	QuatV normQuat = NormalizeFast( q );

	QuatV normQuatCompare( 0.1f, 0.3f, 0.3f, 0.9f );

	CHECK( 0 != IsCloseAll( normQuat, normQuatCompare, Vec4VFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_NormalizeFastSafe )
{
	QuatV q( 0.0f, 0.0f, 0.0f, 0.0f );
	QuatV normQuat = NormalizeFastSafe( q );

	QuatV normQuatCompare = QuatV(V_FLT_LARGE_8);

	CHECK( 0 != IsEqualAll( normQuat, normQuatCompare )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_Invert )
{
	QuatV q( 1.0f, 3.0f, 3.0f, 9.0f );
	QuatV invQuat = Invert( q );

	QuatV invQuatCompare( -1.0f/100.0f, -3.0f/100.0f, -3.0f/100.0f, 9.0f/100.0f );

	CHECK( 0 != IsCloseAll(invQuat, invQuatCompare, Vec4VFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_InvertSafe )
{
	QuatV q( 0.0f, 0.0f, 0.0f, 0.0f );
	QuatV invQuat = InvertSafe( q );

	QuatV invQuatCompare = QuatV(V_FLT_LARGE_8);

	CHECK( 0 != IsEqualAll(invQuat, invQuatCompare )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_InvertFast )
{
	QuatV q( 1.0f, 3.0f, 3.0f, 9.0f );
	QuatV invQuat = InvertFast( q );

	QuatV invQuatCompare( -1.0f/100.0f, -3.0f/100.0f, -3.0f/100.0f, 9.0f/100.0f );

	CHECK( 0 != IsCloseAll(invQuat, invQuatCompare, Vec4VFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_InvertFastSafe )
{
	QuatV q( 0.0f, 0.0f, 0.0f, 0.0f );
	QuatV invQuat = InvertFastSafe( q );

	QuatV invQuatCompare = QuatV(V_FLT_LARGE_8);

	CHECK( 0 != IsEqualAll(invQuat, invQuatCompare )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_DotV )
{
	QuatV q1( 1.0f, 2.0f, 3.0f, 4.0f );
	QuatV q2( 5.0f, 6.0f, 7.0f, 8.0f );

	ScalarV dotV = Dot( q1, q2 );
	ScalarV dotVCompare = ScalarVFromF32( 70.0f );

	CHECK( 0 != IsCloseAll(dotV, dotVCompare, ScalarVFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_Mul )
{
	QuatV q1( 1.0f, 2.0f, 3.0f, 4.0f );
	QuatV q2( 5.0f, 6.0f, 7.0f, 8.0f );

	QuatV quatProd = Multiply( q1, q2 );

	float qx = q1.GetWf()*q2.GetXf() + q1.GetXf()*q2.GetWf() + q1.GetYf()*q2.GetZf() - q1.GetZf()*q2.GetYf();
	float qy = q1.GetWf()*q2.GetYf() - q1.GetXf()*q2.GetZf() + q1.GetYf()*q2.GetWf() + q1.GetZf()*q2.GetXf();
	float qz = q1.GetWf()*q2.GetZf() + q1.GetXf()*q2.GetYf() - q1.GetYf()*q2.GetXf() + q1.GetZf()*q2.GetWf();
	float qw = q1.GetWf()*q2.GetWf() - q1.GetXf()*q2.GetXf() - q1.GetYf()*q2.GetYf() - q1.GetZf()*q2.GetZf();
	QuatV quatProdCompare( qx, qy, qz, qw );

	CHECK( 0 != IsCloseAll(quatProd, quatProdCompare, Vec4VFromF32(TEST_EPSILON) )  );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_Transform )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	// We'll test the equivalency of these two transforms.
	Mat33V rotMat;
	Mat33VFromAxisAngle( rotMat, normAxis, angle );
	QuatV rotQuat = QuatVFromAxisAngle( normAxis, angle );

	// Rotate the same vector by the two representations.
	Vec3V vecToTransform( 1.0f, 2.0f, 3.0f );
	Vec3V transformedByMat = Multiply( rotMat, vecToTransform );
	Vec3V transformedByQuat = Transform( rotQuat, vecToTransform );

	CHECK( 0 != IsCloseAll(transformedByQuat, transformedByMat, Vec3VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_UnTransform )
{
	Vec3V normAxis( 0.577350259f, 0.577350259f, 0.577350259f );
	ScalarV angle = ScalarVFromF32( -0.78539816339744830961566084581988f ); // -pi/4

	QuatV rotQuat = QuatVFromAxisAngle( normAxis, angle );

	Vec3V vecToTransform( 1.0f, 2.0f, 3.0f );
	Vec3V transformedVec = Transform( rotQuat, vecToTransform );
	Vec3V unTransformedVec = UnTransform( rotQuat, transformedVec );

	CHECK( 0 != IsCloseAll( vecToTransform, unTransformedVec, Vec3VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_MultiplyCompatibilityCheck )
{
	QuatV q1( 1.0f, 2.0f, 3.0f, 4.0f );
	QuatV q2( 5.0f, 6.0f, 7.0f, 8.0f );
	QuatV quatProd = Multiply( q1, q2 );

	Quaternion q1_old = RC_QUATERNION(q1);
	Quaternion q2_old = RC_QUATERNION(q2);
	Quaternion quatProd_old;
	quatProd_old.Multiply(q1_old, q2_old);

	QuatV quatProd_compare = RCC_QUATV(quatProd_old);

	CHECK( 0 != IsCloseAll( quatProd, quatProd_compare, Vec4VFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_UnTransformCompatibilityCheck )
{
	// Rotation of increasing amounts around a random axis.
	// Testing for equivalent behavior with the old vector library.
	for( float angle = 0.0f; angle < 2.0f*3.14159f; angle += 0.01f )
	{
		// Create the axis of rotation, and from that plus an angle, a quaternion.
		Vec3V rotAxis( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
		rotAxis = Normalize( rotAxis );
		QuatV quat = QuatVFromAxisAngle( rotAxis, ScalarVFromF32(angle) );

		// Use the new library.
		Vec3V v1( 3.9f, 4.9f, 5.9f ); // Point to transform.
		QuatV q1 = quat;
		q1 = Normalize(q1);
		Vec3V v2 = UnTransform( q1, v1 );

		// Use the old library.
		Vector3 v1_old( 3.9f, 4.9f, 5.9f ); // Point to transform.
		Quaternion q1_old = RCC_QUATERNION(quat);
		q1_old.Normalize();
		Vector3 v2_old;
		q1_old.UnTransform(v1_old, v2_old);
		Vec3V v2_compare = RCC_VEC3V( v2_old );

		// Compare.
		CHECK( 0 != IsCloseAll( v2, v2_compare, Vec3VFromF32(TEST_EPSILON) ) );
	}	
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_SlerpNearCompatibilityCheck )
{
	// Rotation of increasing amounts around a random axis.
	// Testing for equivalent behavior with the old vector library.
	for( float angle = 0.0f; angle < 2.0f*3.14159f; angle += 0.01f )
	{
		// Create the axis of rotation, and from that plus an angle, a quaternion.
		Vec3V rotAxis( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
		rotAxis = Normalize( rotAxis );
		QuatV quat = QuatVFromAxisAngle( rotAxis, ScalarVFromF32(angle) );

		// Create another axis of rotation, and from that plus an angle, a quaternion.
		Vec3V rotAxis2( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
		rotAxis2 = Normalize( rotAxis2 );
		QuatV quat2 = QuatVFromAxisAngle( rotAxis2, ScalarVFromF32(3.14159f - angle) );

		// Go through 10 't' values.
		for( float t = 0.0f; t < 1.0f; t += 0.1f )
		{
			// Use the new library.
			QuatV q1 = quat;
			QuatV q2 = quat2;
			q1 = Normalize(q1);
			q2 = Normalize(q2);
			QuatV q3 = SlerpNear( ScalarVFromF32(t), q1, q2 );

			// Use the old library.
			Quaternion q1_old = RCC_QUATERNION(quat);
			Quaternion q2_old = RCC_QUATERNION(quat2);
			q1_old.Normalize();
			q2_old.Normalize();
			Quaternion q3_old;
			q3_old.SlerpNear( t, q1_old, q2_old );
			QuatV q3_compare = RCC_QUATV( q3_old );

			// Compare.
			CHECK( 0 != IsCloseAll( q3, q3_compare, ScalarVFromF32(TEST_EPSILON_LENIENT) ) );
		}
	}
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_PrepareSlerpAndSlerpCompatibilityCheck )
{
	// Rotation of increasing amounts around a random axis.
	// Testing for equivalent behavior with the old vector library.
	for( float angle = 0.0f; angle < 2.0f*3.14159f; angle += 0.01f )
	{
		// Create the axis of rotation, and from that plus an angle, a quaternion.
		Vec3V rotAxis( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
		rotAxis = Normalize( rotAxis );
		QuatV quat = QuatVFromAxisAngle( rotAxis, ScalarVFromF32(angle) );

		// Create another axis of rotation, and from that plus an angle, a quaternion.
		Vec3V rotAxis2( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
		rotAxis2 = Normalize( rotAxis2 );
		QuatV quat2 = QuatVFromAxisAngle( rotAxis2, ScalarVFromF32(3.14159f - angle) );

		// Go through 10 't' values.
		for( float t = 0.0f; t < 1.0f; t += 0.1f )
		{
			// Use the new library.
			QuatV q1 = quat;
			QuatV q2 = quat2;
			QuatV q2Prepped;
			q1 = Normalize(q1);
			q2 = Normalize(q2);
			q2Prepped = PrepareSlerp(q1, q2);
			QuatV q3 = Slerp( ScalarVFromF32(t), q1, q2Prepped );

			// Use the old library.
			Quaternion q1_old = RCC_QUATERNION(quat);
			Quaternion q2_old = RCC_QUATERNION(quat2);
			q1_old.Normalize();
			q2_old.Normalize();
			Quaternion q3_old;
			q3_old.SlerpNear( t, q1_old, q2_old );
			QuatV q3_compare = RCC_QUATV( q3_old );

			// Compare.
			CHECK( 0 != IsCloseAll( q3, q3_compare, ScalarVFromF32(TEST_EPSILON_LENIENT) ) );
		}
	}
}
QUATV_QA_ITEM_END

//QUATV_QA_ITEM_BEGIN( QuatV_SlerpCompatibilityCheck )
//{
//	// Rotation of increasing amounts around a random axis.
//	// Testing for equivalent behavior with the old vector library.
//	for( float angle = 0.0f; angle < 2.0f*3.14159f; angle += 0.01f )
//	{
//		// Create the axis of rotation, and from that plus an angle, a quaternion.
//		Vec3V rotAxis( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
//		rotAxis = Normalize( rotAxis );
//		QuatV quat = QuatFromAxisAngle( rotAxis, ScalarV(angle) );
//
//		// Create another axis of rotation, and from that plus an angle, a quaternion.
//		Vec3V rotAxis2( g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f, g_ReplayRand.GetFloat()+1.0f );
//		rotAxis2 = Normalize( rotAxis2 );
//		QuatV quat2 = QuatFromAxisAngle( rotAxis2, ScalarV(3.14159f - angle) );
//
//		// Go through 10 't' values.
//		for( float t = 0.0f; t < 1.0f; t += 0.1f )
//		{
//			// Use the new library.
//			QuatV q1 = quat;
//			QuatV q2 = quat2;
//			QuatV q2Prepped;
//			q1 = Normalize(q1);
//			q2 = Normalize(q2);
//			QuatV q3 = Slerp( ScalarV(t), q1, q2 );
//
//			// Use the old library.
//			Quaternion q1_old = RCC_QUATERNION(quat);
//			Quaternion q2_old = RCC_QUATERNION(quat2);
//			q1_old.Normalize();
//			q2_old.Normalize();
//			Quaternion q3_old;
//			q3_old.Slerp( t, q1_old, q2_old );
//			QuatV q3_compare = RCC_QUATV( q3_old );
//
//			// Tried to debug this...
//			if( 0 == IsCloseAll( q3, q3_compare, ScalarV(TEST_EPSILON_LENIENT) ) )
//				__debugbreak();
//
//			// Compare.
//			CHECK( 0 != IsCloseAll( q3, q3_compare, ScalarV(TEST_EPSILON_LENIENT) ) );
//		}
//	}
//}
//QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_FromRotationCompatibilityCheck )
{
	// Testing for equivalent behavior with the old vector library.

	Vec3V axis(-1.0f, -1.0f, -1.0f);
	axis = Normalize(axis);

	QuatV q;
	q = QuatVFromAxisAngle( axis, ScalarVFromF32(0.78f) );

	Quaternion q_old;
	q_old.FromRotation( RCC_VECTOR3(axis), 0.78f );
	QuatV q_compare = RCC_QUATV(q_old);

	// Compare.
	CHECK( 0 != IsCloseAll( q, q_compare, ScalarVFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END

QUATV_QA_ITEM_BEGIN( QuatV_FromMatrixCompatibilityCheck )
{
	// Testing for equivalent behavior with the old vector library.
	Vec3V axis(-1.0f, -1.0f, -1.0f);

	Mat34V rotMat;
	Mat34VFromAxisAngle( rotMat, axis, ScalarVFromF32(0.78f) );
	QuatV q;
	q = QuatVFromMat33V( rotMat.GetMat33() );
	//q = Normalize(q); // (normalize seems not to be needed)

	Matrix34 rotMat_old = RCC_MATRIX34(rotMat);
	Quaternion q_old;
	q_old.FromMatrix34( rotMat_old );
	QuatV q_compare = RCC_QUATV(q_old);
	//q_compare = Normalize(q_compare); // (normalize seems not to be needed)

	// Compare.
	CHECK( 0 != IsCloseAll( q, q_compare, ScalarVFromF32(TEST_EPSILON) ) );
}
QUATV_QA_ITEM_END



	if( bRESULT )
	{
		TST_PASS;
	}
	else
	{
		TST_FAIL;
	}
} // end of .Update()

QUATV_QA_ALL_END


QA_ITEM_FAMILY( qaQUATV, (), () );
QA_ITEM_FAST( qaQUATV, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// MEMBER FUNCTIONS
//================================================
/*
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetXf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetXf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetYf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetYf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetZf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetZf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetWf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetWf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetX, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetX, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetY, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetY, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetZ, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetZ, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetW, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetW, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_GetXYZ, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_GetXYZ, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetXf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetXf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetYf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetYf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetZf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetZf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetWf, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetWf, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetX, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetX, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetY, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetY, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetZ, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetZ, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetW, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetW, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_ZeroComponents, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_ZeroComponents, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_SetWZero, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_SetWZero, (), qaResult::FAIL_OR_TOTAL_TIME );

//================================================
// FREE FUNCTIONS
//================================================

Qdont_parse_meA_ITEM_FAMILY( qaQuatV_QuatFromAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_QuatFromAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_QuatFromXAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_QuatFromXAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_QuatFromYAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_QuatFromYAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_QuatFromZAxisAngle, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_QuatFromZAxisAngle, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_QuatFromMat33, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_QuatFromMat33, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_QuatFromMat33Ortho, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_QuatFromMat33Ortho, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_Conjugate, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_Conjugate, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_Normalize, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_Normalize, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_NormalizeSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_NormalizeSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_NormalizeFast, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_NormalizeFast, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_NormalizeFastSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_NormalizeFastSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_Invert, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_Invert, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_InvertSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_InvertSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_InvertFast, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_InvertFast, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_InvertFastSafe, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_InvertFastSafe, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_DotV, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_DotV, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_Mul, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_Mul, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_Transform, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_Transform, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaQuatV_UnTransform, (), () );
Qdont_parse_meA_ITEM_FAST( qaQuatV_UnTransform, (), qaResult::FAIL_OR_TOTAL_TIME );
*/





#endif // __QA
