#include "classes_soa.h"
#include "qa/qa.h"


#if __QA

#define VEC4V_SOA_QA_ALL_START(Name)			\
class qa##Name : public qaItem				\
{ public:									\
	void Init() {};							\
	void Update(qaResult& result)
#define VEC4V_SOA_QA_ALL_END		};

#define VEC4V_SOA_QA_ITEM_BEGIN(Name)	;
#define VEC4V_SOA_QA_ITEM_END			;

// Some custom check macros.
#define CHECK(X)				( bRESULT = bRESULT && (X) )
#define CHECK_EQUAL(X,Y)		CHECK(((X)==(Y)))
#define CHECK_CLOSE(X,Y,E)		CHECK( Abs( ((X)-(Y)) ) <= (E) )

// Account for some floating point error.
#define TEST_EPSILON 0.001f


using namespace rage;
using namespace Vec;


VEC4V_SOA_QA_ALL_START(VEC4V_SOA)

{	// beginning of .Update()

	bool bRESULT = true;

	//=========================================================================
	// TESTS START HERE
	//=========================================================================

	VEC4V_SOA_QA_ITEM_BEGIN( Vec4V_soa_ZeroComponents )
	{
		Vector_4V _x = VECTOR4V_LITERAL( 1.0f, 2.0f, 3.0f, 4.0f );
		Vector_4V _y = VECTOR4V_LITERAL( 5.0f, 6.0f, 7.0f, 8.0f );
		Vector_4V _z = VECTOR4V_LITERAL( 9.0f, 10.0f, 11.0f, 12.0f );
		Vector_4V _w = VECTOR4V_LITERAL( 13.0f, 14.0f, 15.0f, 16.0f );
		SoA_Vec4V vTemp( _x, _y, _z, _w );

		vTemp.ZeroComponents();
		SoA_Vec4V vCompare( V4VConstant(V_ZERO) );

		SoA_VecBool1V result = IsEqualIntAll( vTemp, vCompare );
		CHECK( 0 != V4IsEqualIntAll( result.GetIntrin128(), V4VConstant(V_MASKXYZW) ) );
	}
	VEC4V_SOA_QA_ITEM_END






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

VEC4V_SOA_QA_ALL_END

QA_ITEM_FAMILY( qaVEC4V_SOA, (), () );
QA_ITEM_FAST( qaVEC4V_SOA, (), qaResult::FAIL_OR_TOTAL_TIME );

#endif // __QA
