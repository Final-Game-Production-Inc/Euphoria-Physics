#include "classes_soa.h"
#include "qa/qa.h"


#if __QA

#define MAT34V_SOA_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define MAT34V_SOA_QA_ALL_END		};

#define MAT34V_SOA_QA_ITEM_BEGIN(Name)	;
#define MAT34V_SOA_QA_ITEM_END			;

// Some custom check macros.
#define CHECK(X)				( bRESULT = bRESULT && (X) )
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON 0.001f


using namespace rage;


MAT34V_SOA_QA_ALL_START(MAT34V_SOA)

{	// beginning of .Update()

	bool bRESULT = true;

	//=========================================================================
	// TESTS START HERE
	//=========================================================================

	MAT34V_SOA_QA_ITEM_BEGIN( Mat34V_soa_InvertTransform )
	{
		SoA_Vec3V initialPoint( 4.0f, 5.0f, 6.0f );
		SoA_Mat34V inputMat(	1.0f, -5.0f, 2.0f,
							2.0f, 1.0f, 1.0f,
							3.0f, 4.0f, 5.0f,
							0.0f, 0.0f, 0.0f	);

		SoA_Mat34V invertMat;
		InvertTransformFull( invertMat, inputMat );

		// Transform & untransform a point.
		SoA_Vec3V transformedPoint;
		Transform( transformedPoint, inputMat, initialPoint );
		SoA_Vec3V unTransformedPoint;
		Transform( unTransformedPoint, invertMat, transformedPoint );

		// Transform & untransform a vector.
		SoA_Vec3V transformedVec;
		Transform3x3( transformedVec, inputMat, initialPoint );
		SoA_Vec3V unTransformedVec;
		Transform3x3( unTransformedVec, invertMat, transformedVec );

		SoA_VecBool1V b1 = IsCloseAll( initialPoint, unTransformedPoint, SoA_ScalarVFromF32(TEST_EPSILON) );
		SoA_VecBool1V b2 = IsCloseAll( initialPoint, unTransformedVec, SoA_ScalarVFromF32(TEST_EPSILON) );
		SoA_VecBool1V b1and2 = And( b1, b2 );

		CHECK( 0 != Vec::V4IsEqualIntAll( b1and2.GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) );
	}
	MAT34V_SOA_QA_ITEM_END





	//=========================================================================
	// TESTS END HERE
	//=========================================================================

	if( bRESULT )
	{
		TST_PASS;
	}
	else
	{
		TST_FAIL;
	}
} // end of .Update()

MAT34V_SOA_QA_ALL_END

QA_ITEM_FAMILY( qaMAT34V_SOA, (), () );
QA_ITEM_FAST( qaMAT34V_SOA, (), qaResult::FAIL_OR_TOTAL_TIME );

#endif // __QA
