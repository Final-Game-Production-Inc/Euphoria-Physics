#include "classes_soa.h"
#include "qa/qa.h"


#if __QA

#define MAT33V_SOA_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define MAT33V_SOA_QA_ALL_END		};

#define MAT33V_SOA_QA_ITEM_BEGIN(Name)	;
#define MAT33V_SOA_QA_ITEM_END			;

// Some custom check macros.
#define CHECK(X)				( bRESULT = bRESULT && (X) )
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON 0.001f


using namespace rage;


MAT33V_SOA_QA_ALL_START(MAT33V_SOA)

{	// beginning of .Update()

	bool bRESULT = true;

	//=========================================================================
	// TESTS START HERE
	//=========================================================================

	MAT33V_SOA_QA_ITEM_BEGIN( Mat33V_soa_Determinant )
	{
		SoA_Mat33V mat = SoA_Mat33V( 1.0f, 4.0f, 2.0f, 4.0f, 3.0f, 1.0f, 2.0f, 2.0f, 1.0f );
		SoA_ScalarV det = Determinant( mat );
		SoA_ScalarV detTest = SoA_ScalarVFromF32( -3.0f );

		CHECK( 0 != Vec::V4IsEqualIntAll( det.GetIntrin128(), detTest.GetIntrin128() ) );
	}
	MAT33V_SOA_QA_ITEM_END

	MAT33V_SOA_QA_ITEM_BEGIN( Mat33V_soa_Invert )
	{
		SoA_Mat33V mat = SoA_Mat33V( 1.0f, 4.0f, 2.0f, 4.0f, 3.0f, 1.0f, 2.0f, 2.0f, 1.0f );
		SoA_Mat33V inv;
		InvertFull( inv, mat );
		SoA_Mat33V invTest = SoA_Mat33V(
							-1.0f/3.0f, 0.0f, 2.0f/3.0f,
							2.0f/3.0f, 1.0f, -7.0f/3.0f,
							-2.0f/3.0f, -2.0f, 13.0f/3.0f
								);

		SoA_VecBool1V testVec = IsCloseAll( inv, invTest, SoA_ScalarVFromF32(TEST_EPSILON) );
		CHECK( 0 != Vec::V4IsEqualIntAll( testVec.GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) );
	}
	MAT33V_SOA_QA_ITEM_END

	MAT33V_SOA_QA_ITEM_BEGIN( Mat33V_soa_Transpose )
	{
		SoA_Mat33V mat = SoA_Mat33V( 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f );
		SoA_Mat33V trans;
		Transpose( trans, mat );
		SoA_Mat33V transTest = SoA_Mat33V( 1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f );

		SoA_VecBool1V testVec = IsEqualIntAll( trans, transTest );
		CHECK( 0 != Vec::V4IsEqualIntAll( testVec.GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) );
	}
	MAT33V_SOA_QA_ITEM_END

	MAT33V_SOA_QA_ITEM_BEGIN( Mat33V_soa_Multiply )
	{
		SoA_Mat33V mat1 = SoA_Mat33V( 1.0f, 4.0f, 2.0f, 4.0f, 3.0f, 2.0f, 2.0f, 1.0f, 1.0f );
		SoA_Mat33V mat2 = SoA_Mat33V( 1.0f, 1.0f, 3.0f, 2.0f, 1.0f, 1.0f, 3.0f, 2.0f, 1.0f );
		SoA_Mat33V prod;
		Multiply( prod, mat1, mat2 );
		SoA_Mat33V prodTest = SoA_Mat33V( 11.0f, 10.0f, 7.0f, 8.0f, 12.0f, 7.0f, 13.0f, 19.0f, 11.0f );

		SoA_VecBool1V testVec = IsCloseAll( prod, prodTest, SoA_ScalarVFromF32(TEST_EPSILON) );
		CHECK( 0 != Vec::V4IsEqualIntAll( testVec.GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) );
	}
	MAT33V_SOA_QA_ITEM_END

	MAT33V_SOA_QA_ITEM_BEGIN( Mat33V_soa_UnTransform )
	{
		SoA_Mat33V transformMat(	-2.0f, 1.0f, 7.0f,
								4.0f, 5.0f, -3.0f,
								9.0f, 7.0f, 2.0f	);
		SoA_Mat33V matToTransform(	1.0f, 2.0f, 1.0f,
								-3.0f, 2.0f, 0.0f,
								1.0f, 1.0f, 2.0f	);

		SoA_Mat33V transformedMat;
		Multiply( transformedMat, transformMat, matToTransform );
		SoA_Mat33V untransformedMat;
		UnTransformFull( untransformedMat, transformMat, transformedMat );

		SoA_VecBool1V testVec = IsCloseAll( untransformedMat, matToTransform, SoA_ScalarVFromF32(TEST_EPSILON) );
		CHECK( 0 != Vec::V4IsEqualIntAll( testVec.GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) );
	}
	MAT33V_SOA_QA_ITEM_END




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

MAT33V_SOA_QA_ALL_END

QA_ITEM_FAMILY( qaMAT33V_SOA, (), () );
QA_ITEM_FAST( qaMAT33V_SOA, (), qaResult::FAIL_OR_TOTAL_TIME );

#endif // __QA
