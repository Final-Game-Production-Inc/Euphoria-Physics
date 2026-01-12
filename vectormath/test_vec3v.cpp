
#include "classes.h"
#include "qa/qa.h"

#if __QA

//// Macro for testing a snippet of code.
//#define VEC3V_QA_ITEM_BEGIN(Name)		
//class qa##Name : public qaItem			
//{ public:									
//	void Init() {};							
//	void Update(qaResult& result)
//#define VEC3V_QA_ITEM_END		};

#define VEC3V_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define VEC3V_QA_ALL_END		};

#define VEC3V_QA_ITEM_BEGIN(Name)	;
#define VEC3V_QA_ITEM_END			;

// Some custom check macros.
#define CHECK(X)				( bRESULT = bRESULT && (X) )
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON 0.001f

// Some ops just ain't as precise. :/
#define TEST_EPSILON_LENIENT 0.01f
#define TEST_EPSILON_REALLY_LENIENT 0.05f

// Trig epsilons.
#define TEST_EPSILON_TRIG 0.004f



using namespace rage;
using namespace Vec;

VEC3V_QA_ALL_START(VEC3V)

{	// beginning of .Update()

	bool bRESULT = true;

VEC3V_QA_ITEM_BEGIN( Vec3V_Extend )
{
	// Current magnitude is sqrt(3)
	Vec3V testVect( 1.0f, 1.0f, 1.0f );
	Vec3V amtToExtend( 1.0f, 1.0f, 1.0f );
	// Future magnitude should be sqrt(3)+1.
	// So each component should be (1 + sqrt(3)/3 = 1.577350269189625764509148780502f).
	testVect = Extend( testVect, amtToExtend );
	Vec3V testVect2(	1.577350269189625764509148780502f,
						1.577350269189625764509148780502f,
						1.577350269189625764509148780502f );
	CHECK( 0 != IsCloseAll( testVect, testVect2, ScalarVFromF32(TEST_EPSILON) ) );
}
VEC3V_QA_ITEM_END

VEC3V_QA_ITEM_BEGIN( Vec3V_RotateAboutXAxis )
{
	Vec3V testVect( 1.0f, 1.0f, 0.0f );
	Vec3V amtToRotate( V4VConstant(V_PI_OVER_TWO) );
	testVect = RotateAboutXAxis( testVect, amtToRotate );
	Vec3V testVect2( 1.0f, 0.0f, 1.0f );
	CHECK( 0 != IsCloseAll( testVect, testVect2, ScalarVFromF32(TEST_EPSILON) ) );
}
VEC3V_QA_ITEM_END

VEC3V_QA_ITEM_BEGIN( Vec3V_RotateAboutYAxis )
{
	Vec3V testVect( 1.0f, 1.0f, 0.0f );
	Vec3V amtToRotate( V4VConstant(V_PI_OVER_TWO) );
	testVect = RotateAboutYAxis( testVect, amtToRotate );
	Vec3V testVect2( 0.0f, 1.0f, -1.0f );
	CHECK( 0 != IsCloseAll( testVect, testVect2, ScalarVFromF32(TEST_EPSILON) ) );
}
VEC3V_QA_ITEM_END

VEC3V_QA_ITEM_BEGIN( Vec3V_RotateAboutZAxis )
{
	Vec3V testVect( 1.0f, 1.0f, 0.0f );
	Vec3V amtToRotate( V4VConstant(V_PI_OVER_TWO) );
	testVect = RotateAboutZAxis( testVect, amtToRotate );
	Vec3V testVect2( -1.0f, 1.0f, 0.0f );
	CHECK( 0 != IsCloseAll( testVect, testVect2, ScalarVFromF32(TEST_EPSILON) ) );
}
VEC3V_QA_ITEM_END

VEC3V_QA_ITEM_BEGIN( Vec3V_Reflect )
{
	Vec3V testVect( 1.0f, 0.5f, 1.0f );
	testVect = Normalize( testVect );
	Vec3V planeNormal( 0.0f, 1.0f, 0.0f );
	testVect = Reflect( testVect, planeNormal );
	Vec3V testVect2( 1.0f, -0.5f, 1.0f );
	testVect2 = Normalize( testVect2 );
	CHECK( 0 != IsCloseAll( testVect, testVect2, ScalarVFromF32(TEST_EPSILON) ) );
}
VEC3V_QA_ITEM_END

VEC3V_QA_ITEM_BEGIN( Vec3V_ApproachStraight )
{
	Vec3V startVec( 0.0f, 0.0f, 0.0f );
	Vec3V goalVec( -2.0f, -2.0f, -2.0f );
	int numIterations = 0;
	bool bResult = true;

	// Simulate a rate of 0.25 units/sec, 1.0 sec at a time.
	u32 approachTest = 0;
	while( approachTest == 0 )
	{
		// Make sure that startVec is on its way...
		Vec3V unitDirectionVect = (goalVec - startVec);
		unitDirectionVect = Normalize( unitDirectionVect );
		
		// Should be < -1.0/sqrt(3), -1.0/sqrt(3), -1.0/sqrt(3) >
		bResult = bResult && ( 0 != IsCloseAll( unitDirectionVect, Vec3VFromF32(-0.57735026919f), ScalarVFromF32(TEST_EPSILON) ) );

		// Prevent an infinite loop.
		numIterations++;
		if( numIterations > 40 )
		{
			CHECK( false );
			return;
		}

		// Approach a bit.
		startVec = ApproachStraight( startVec, goalVec, Vec3VFromF32(0.25f), Vec3VFromF32(1.0f), approachTest );
	}
	CHECK( bResult );
}
VEC3V_QA_ITEM_END

	if( bRESULT )
	{
		TST_PASS;
	}
	else
	{
		TST_FAIL;
	}
} // end of .Update()

VEC3V_QA_ALL_END


QA_ITEM_FAMILY( qaVEC3V, (), () );
QA_ITEM_FAST( qaVEC3V, (), qaResult::FAIL_OR_TOTAL_TIME );

/*
Qdont_parse_meA_ITEM_FAMILY( qaVec3V_Extend, (), () );
Qdont_parse_meA_ITEM_FAST( qaVec3V_Extend, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaVec3V_RotateAboutXAxis, (), () );
Qdont_parse_meA_ITEM_FAST( qaVec3V_RotateAboutXAxis, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaVec3V_RotateAboutYAxis, (), () );
Qdont_parse_meA_ITEM_FAST( qaVec3V_RotateAboutYAxis, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaVec3V_RotateAboutZAxis, (), () );
Qdont_parse_meA_ITEM_FAST( qaVec3V_RotateAboutZAxis, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaVec3V_Reflect, (), () );
Qdont_parse_meA_ITEM_FAST( qaVec3V_Reflect, (), qaResult::FAIL_OR_TOTAL_TIME );
Qdont_parse_meA_ITEM_FAMILY( qaVec3V_ApproachStraight, (), () );
Qdont_parse_meA_ITEM_FAST( qaVec3V_ApproachStraight, (), qaResult::FAIL_OR_TOTAL_TIME );
*/

#endif // __QA
