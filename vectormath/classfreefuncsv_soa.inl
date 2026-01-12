
namespace rage
{
	//============================================================================
	// Utility functions

	__forceinline SoA_ScalarV_Out SoA_ScalarVFromF32( const float& f )
	{
		Vec::Vector_4V x;
		Vec::V4Set( x, f );
		return SoA_ScalarV( x );
	}

	//============================================================================
	// Magnitude

	__forceinline SoA_ScalarV_Out Abs(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4Abs( inVect.GetIntrin128() ) );
	}

	__forceinline void Abs(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect)
	{
		Imp::Abs_Imp( inoutVect, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void Abs(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect)
	{
		Imp::Abs_Imp( inoutVect, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void Abs(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect)
	{
		Imp::Abs_Imp( inoutVect, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out Sqrt(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4Sqrt(inVect.GetIntrin128()) ); 
	}

	__forceinline void Sqrt(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect)
	{
		Imp::Sqrt_Imp( inoutVect, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void Sqrt(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect)
	{
		Imp::Sqrt_Imp( inoutVect, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void Sqrt(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect)
	{
		Imp::Sqrt_Imp( inoutVect, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out SqrtSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Vec::V4SqrtSafe(inVect.GetIntrin128(), errValVect.GetIntrin128()) ); 
	}

	__forceinline void SqrtSafe(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::SqrtSafe_Imp( inoutVect, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void SqrtSafe(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::SqrtSafe_Imp( inoutVect, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void SqrtSafe(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::SqrtSafe_Imp( inoutVect, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out SqrtFast(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4SqrtFast(inVect.GetIntrin128()) ); 
	}

	__forceinline void SqrtFast(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect)
	{
		Imp::SqrtFast_Imp( inoutVect, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void SqrtFast(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect)
	{
		Imp::SqrtFast_Imp( inoutVect, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void SqrtFast(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect)
	{
		Imp::SqrtFast_Imp( inoutVect, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out SqrtFastSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Vec::V4SqrtFastSafe(inVect.GetIntrin128(), errValVect.GetIntrin128()) );
	}

	__forceinline void SqrtFastSafe(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::SqrtFastSafe_Imp( inoutVect, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void SqrtFastSafe(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::SqrtFastSafe_Imp( inoutVect, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void SqrtFastSafe(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::SqrtFastSafe_Imp( inoutVect, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out InvSqrt(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4InvSqrt( inVect.GetIntrin128() ) );
	}

	__forceinline void InvSqrt(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect)
	{
		Imp::InvSqrt_Imp( inoutVect, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void InvSqrt(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect)
	{
		Imp::InvSqrt_Imp( inoutVect, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void InvSqrt(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect)
	{
		Imp::InvSqrt_Imp( inoutVect, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out InvSqrtSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Vec::V4InvSqrtSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline void InvSqrtSafe(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvSqrtSafe_Imp( inoutVect, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvSqrtSafe(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvSqrtSafe_Imp( inoutVect, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvSqrtSafe(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvSqrtSafe_Imp( inoutVect, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out InvSqrtFast(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4InvSqrtFast( inVect.GetIntrin128() ) );
	}

	__forceinline void InvSqrtFast(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect)
	{
		Imp::InvSqrtFast_Imp( inoutVect, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void InvSqrtFast(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect)
	{
		Imp::InvSqrtFast_Imp( inoutVect, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void InvSqrtFast(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect)
	{
		Imp::InvSqrtFast_Imp( inoutVect, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out InvSqrtFastSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Vec::V4InvSqrtFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline void InvSqrtFastSafe(SoA_Vec2V_InOut inoutVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvSqrtFastSafe_Imp( inoutVect, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvSqrtFastSafe(SoA_Vec3V_InOut inoutVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvSqrtFastSafe_Imp( inoutVect, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvSqrtFastSafe(SoA_Vec4V_InOut inoutVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvSqrtFastSafe_Imp( inoutVect, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out Mag(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::Mag_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out Mag(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::Mag_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out Mag(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::Mag_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out MagFast(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::MagFast_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out MagFast(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::MagFast_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out MagFast(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::MagFast_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out MagSquared(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::MagSquared_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out MagSquared(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::MagSquared_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out MagSquared(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::MagSquared_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMag(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMag_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMag(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMag_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMag(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMag_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSafe_Imp( VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSafe_Imp( VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSafe_Imp( VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagFast(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagFast_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagFast(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagFast_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagFast(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagFast_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagFastSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagFastSafe_Imp( VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagFastSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagFastSafe_Imp( VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagFastSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagFastSafe_Imp( VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquared(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagSquared_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquared(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagSquared_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquared(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagSquared_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredSafe_Imp( VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredSafe_Imp( VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredSafe_Imp( VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredFast(SoA_Vec2V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredFast_Imp( VEC2V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredFast(SoA_Vec3V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredFast_Imp( VEC3V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredFast(SoA_Vec4V_In inVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredFast_Imp( VEC4V_SOA_ARG(inVect) ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredFastSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredFastSafe_Imp( VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredFastSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredFastSafe_Imp( VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvMagSquaredFastSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvMagSquaredFastSafe_Imp( VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() ) );
	}

	//============================================================================
	// Comparison functions.

	__forceinline SoA_VecBool1V_Out IsEqualInt(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqual(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4IsEqualV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsClose(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Vec::V4IsCloseV( inVector1.GetIntrin128(), inVector2.GetIntrin128(), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThan(SoA_ScalarV_In bigVector, SoA_ScalarV_In smallVector)
	{
		return SoA_VecBool1V( Vec::V4IsGreaterThanV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqual(SoA_ScalarV_In bigVector, SoA_ScalarV_In smallVector)
	{
		return SoA_VecBool1V( Vec::V4IsGreaterThanOrEqualV( bigVector.GetIntrin128(), smallVector.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThan(SoA_ScalarV_In smallVector, SoA_ScalarV_In bigVector)
	{
		return SoA_VecBool1V( Vec::V4IsLessThanV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqual(SoA_ScalarV_In smallVector, SoA_ScalarV_In bigVector)
	{
		return SoA_VecBool1V( Vec::V4IsLessThanOrEqualV( smallVector.GetIntrin128(), bigVector.GetIntrin128() ) );
	}

	__forceinline void IsEqualInt(SoA_VecBool2V_InOut inoutVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) ) );
	}

	__forceinline void IsEqualInt(SoA_VecBool3V_InOut inoutVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) ) );
	}

	__forceinline void IsEqualInt(SoA_VecBool4V_InOut inoutVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroAll(SoA_Vec4V_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroAll_Imp( VEC4V_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroAll(SoA_Vec3V_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroAll_Imp( VEC3V_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroAll(SoA_Vec2V_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroAll_Imp( VEC2V_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroAll(SoA_QuatV_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroAll_Imp( QUATV_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroNone(SoA_Vec4V_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroNone_Imp( VEC4V_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroNone(SoA_Vec3V_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroNone_Imp( VEC3V_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroNone(SoA_Vec2V_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroNone_Imp( VEC2V_SOA_ARG(inVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsZeroNone(SoA_QuatV_In inVector)
	{
		return SoA_VecBool1V( Imp::IsZeroNone_Imp( QUATV_SOA_ARG(inVector) ) );
	}

	__forceinline void IsEqual(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::IsEqualV_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void IsEqual(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::IsEqualV_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void IsEqual(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::IsEqualV_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline void IsEqual(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In inVector1, SoA_QuatV_In inVector2)
	{
		Imp::IsEqualV_Imp( inoutVec, QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp( VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp( VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp( VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_QuatV_In inVector1, SoA_QuatV_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp( QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp( VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp( VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp( VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_QuatV_In inVector1, SoA_QuatV_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp( QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualInt(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4IsEqualIntV( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void IsEqualInt(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void IsEqualInt(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void IsEqualInt(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline void IsEqualInt(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In inVector1, SoA_QuatV_In inVector2)
	{
		Imp::IsEqualIntV_Imp( inoutVec, QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_QuatV_In inVector1, SoA_QuatV_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp( QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntNone(SoA_QuatV_In inVector1, SoA_QuatV_In inVector2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntNone_Imp( QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2) ) );
	}

	__forceinline void IsClose(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2, SoA_ScalarV_In epsValues)
	{
		Imp::IsCloseV_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2), epsValues.GetIntrin128() );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp( VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp( VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline void IsClose(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In inVector1, SoA_QuatV_In inVector2, SoA_ScalarV_In epsValues)
	{
		Imp::IsCloseV_Imp( inoutVec, QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2), epsValues.GetIntrin128() );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_QuatV_In inVector1, SoA_QuatV_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp( QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_QuatV_In inVector1, SoA_QuatV_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp( QUATV_SOA_ARG(inVector1), QUATV_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline void IsClose(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2, SoA_ScalarV_In epsValues)
	{
		Imp::IsCloseV_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2), epsValues.GetIntrin128() );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp( VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp( VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline void IsClose(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2, SoA_ScalarV_In epsValues)
	{
		Imp::IsCloseV_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2), epsValues.GetIntrin128() );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp( VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp( VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp( VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThan(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector)
	{
		Imp::IsGreaterThanV_Imp( inoutVec, VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp( VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThan(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector)
	{
		Imp::IsGreaterThanV_Imp( inoutVec, VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) );
	}

	__forceinline void IsGreaterThan(SoA_VecBool3V_InOut inoutVec, SoA_ScalarV_In bigVector, SoA_Vec3V_In smallVector)
	{
		Imp::IsGreaterThanV_Imp( inoutVec, bigVector.GetIntrin128(), VEC3V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp( VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThan(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector)
	{
		Imp::IsGreaterThanV_Imp( inoutVec, VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) );
	}

	__forceinline void IsGreaterThan(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In bigVector, SoA_QuatV_In smallVector)
	{
		Imp::IsGreaterThanV_Imp( inoutVec, QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_QuatV_In bigVector, SoA_QuatV_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp( QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp( VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThanOrEqual(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector)
	{
		Imp::IsGreaterThanOrEqualV_Imp( inoutVec, VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_QuatV_In bigVector, SoA_QuatV_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp( QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThanOrEqual(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In bigVector, SoA_QuatV_In smallVector)
	{
		Imp::IsGreaterThanOrEqualV_Imp( inoutVec, QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp( VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThanOrEqual(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector)
	{
		Imp::IsGreaterThanOrEqualV_Imp( inoutVec, VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp( VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsGreaterThanOrEqual(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector)
	{
		Imp::IsGreaterThanOrEqualV_Imp( inoutVec, VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp( VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThan(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector)
	{
		Imp::IsLessThanV_Imp( inoutVec, VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_QuatV_In smallVector, SoA_QuatV_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp( QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThan(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In smallVector, SoA_QuatV_In bigVector)
	{
		Imp::IsLessThanV_Imp( inoutVec, QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp( VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThan(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector)
	{
		Imp::IsLessThanV_Imp( inoutVec, VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp( VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThan(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector)
	{
		Imp::IsLessThanV_Imp( inoutVec, VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp( VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThanOrEqual(SoA_VecBool4V_InOut inoutVec, SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector)
	{
		Imp::IsLessThanOrEqualV_Imp( inoutVec, VEC4V_SOA_ARG(bigVector), VEC4V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_QuatV_In smallVector, SoA_QuatV_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp( QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThanOrEqual(SoA_VecBool4V_InOut inoutVec, SoA_QuatV_In smallVector, SoA_QuatV_In bigVector)
	{
		Imp::IsLessThanOrEqualV_Imp( inoutVec, QUATV_SOA_ARG(bigVector), QUATV_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp( VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThanOrEqual(SoA_VecBool3V_InOut inoutVec, SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector)
	{
		Imp::IsLessThanOrEqualV_Imp( inoutVec, VEC3V_SOA_ARG(bigVector), VEC3V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp( VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) ) );
	}

	__forceinline void IsLessThanOrEqual(SoA_VecBool2V_InOut inoutVec, SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector)
	{
		Imp::IsLessThanOrEqualV_Imp( inoutVec, VEC2V_SOA_ARG(bigVector), VEC2V_SOA_ARG(smallVector) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp_44_44( MAT44V_SOA_ARG(inMat1), MAT44V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp_34_34( MAT34V_SOA_ARG(inMat1), MAT34V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualAll(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualAll_Imp_33_33( MAT33V_SOA_ARG(inMat1), MAT33V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp_44_44( MAT44V_SOA_ARG(inMat1), MAT44V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp_34_34( MAT34V_SOA_ARG(inMat1), MAT34V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualNone(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualNone_Imp_33_33( MAT33V_SOA_ARG(inMat1), MAT33V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp_44_44( MAT44V_SOA_ARG(inMat1), MAT44V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp_34_34( MAT34V_SOA_ARG(inMat1), MAT34V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsEqualIntAll(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2)
	{
		return SoA_VecBool1V( Imp::IsEqualIntAll_Imp_33_33( MAT33V_SOA_ARG(inMat1), MAT33V_SOA_ARG(inMat2) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_Mat44V_In bigMat, SoA_Mat44V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp_44_44( MAT44V_SOA_ARG(bigMat), MAT44V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_Mat34V_In bigMat, SoA_Mat34V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp_34_34( MAT34V_SOA_ARG(bigMat), MAT34V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanAll(SoA_Mat33V_In bigMat, SoA_Mat33V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanAll_Imp_33_33( MAT33V_SOA_ARG(bigMat), MAT33V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Mat44V_In bigMat, SoA_Mat44V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp_44_44( MAT44V_SOA_ARG(bigMat), MAT44V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Mat34V_In bigMat, SoA_Mat34V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp_34_34( MAT34V_SOA_ARG(bigMat), MAT34V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Mat33V_In bigMat, SoA_Mat33V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsGreaterThanOrEqualAll_Imp_33_33( MAT33V_SOA_ARG(bigMat), MAT33V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_Mat44V_In bigMat, SoA_Mat44V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp_44_44( MAT44V_SOA_ARG(bigMat), MAT44V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_Mat34V_In bigMat, SoA_Mat34V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp_34_34( MAT34V_SOA_ARG(bigMat), MAT34V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanAll(SoA_Mat33V_In bigMat, SoA_Mat33V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsLessThanAll_Imp_33_33( MAT33V_SOA_ARG(bigMat), MAT33V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Mat44V_In bigMat, SoA_Mat44V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp_44_44( MAT44V_SOA_ARG(bigMat), MAT44V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Mat34V_In bigMat, SoA_Mat34V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp_34_34( MAT34V_SOA_ARG(bigMat), MAT34V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Mat33V_In bigMat, SoA_Mat33V_In smallMat)
	{
		return SoA_VecBool1V( Imp::IsLessThanOrEqualAll_Imp_33_33( MAT33V_SOA_ARG(bigMat), MAT33V_SOA_ARG(smallMat) ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp_44_44( MAT44V_SOA_ARG(inMat1), MAT44V_SOA_ARG(inMat2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp_34_34( MAT34V_SOA_ARG(inMat1), MAT34V_SOA_ARG(inMat2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseAll(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseAll_Imp_33_33( MAT33V_SOA_ARG(inMat1), MAT33V_SOA_ARG(inMat2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp_44_44( MAT44V_SOA_ARG(inMat1), MAT44V_SOA_ARG(inMat2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp_34_34( MAT34V_SOA_ARG(inMat1), MAT34V_SOA_ARG(inMat2), epsValues.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out IsCloseNone(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2, SoA_ScalarV_In epsValues)
	{
		return SoA_VecBool1V( Imp::IsCloseNone_Imp_33_33( MAT33V_SOA_ARG(inMat1), MAT33V_SOA_ARG(inMat2), epsValues.GetIntrin128() ) );
	}

	__forceinline void AddInt( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		Imp::AddInt_Imp( inoutVec, VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void AddInt( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::AddInt_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void AddInt( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		Imp::AddInt_Imp( inoutVec, VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out AddInt( SoA_ScalarV_In a, SoA_ScalarV_In b )
	{
		return SoA_ScalarV( Vec::V4AddInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline void SubtractInt( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		Imp::SubtractInt_Imp( inoutVec, VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void SubtractInt( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::SubtractInt_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void SubtractInt( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		Imp::SubtractInt_Imp( inoutVec, VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out SubtractInt( SoA_ScalarV_In a, SoA_ScalarV_In b )
	{
		return SoA_ScalarV( Vec::V4SubtractInt( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline void AddScaled( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toAdd, SoA_Vec4V_In toScaleThenAdd, SoA_Vec4V_In scaleValues )
	{
		Imp::AddScaled_Imp( inoutVec, VEC4V_SOA_ARG(toAdd), VEC4V_SOA_ARG(toScaleThenAdd), VEC4V_SOA_ARG(scaleValues) );
	}

	__forceinline void AddScaled( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toAdd, SoA_Vec3V_In toScaleThenAdd, SoA_Vec3V_In scaleValues )
	{
		Imp::AddScaled_Imp( inoutVec, VEC3V_SOA_ARG(toAdd), VEC3V_SOA_ARG(toScaleThenAdd), VEC3V_SOA_ARG(scaleValues) );
	}

	__forceinline void AddScaled( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toAdd, SoA_Vec3V_In toScaleThenAdd, SoA_ScalarV_In scaleValue )
	{
		Imp::AddScaled_Imp( inoutVec, VEC3V_SOA_ARG(toAdd), VEC3V_SOA_ARG(toScaleThenAdd), scaleValue.GetIntrin128() );
	}

	__forceinline void AddScaled( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toAdd, SoA_Vec2V_In toScaleThenAdd, SoA_Vec2V_In scaleValues )
	{
		Imp::AddScaled_Imp( inoutVec, VEC2V_SOA_ARG(toAdd), VEC2V_SOA_ARG(toScaleThenAdd), VEC2V_SOA_ARG(scaleValues) );
	}

	__forceinline SoA_ScalarV_Out AddScaled( SoA_ScalarV_In toAdd, SoA_ScalarV_In toScaleThenAdd, SoA_ScalarV_In scaleValues )
	{
		return SoA_ScalarV( Vec::V4AddScaled( toAdd.GetIntrin128(), toScaleThenAdd.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline void SubtractScaled( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toSubtractFrom, SoA_Vec4V_In toScaleThenSubtract, SoA_Vec4V_In scaleValues )
	{
		Imp::SubtractScaled_Imp( inoutVec, VEC4V_SOA_ARG(toSubtractFrom), VEC4V_SOA_ARG(toScaleThenSubtract), VEC4V_SOA_ARG(scaleValues) );
	}

	__forceinline void SubtractScaled( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toSubtractFrom, SoA_Vec3V_In toScaleThenSubtract, SoA_Vec3V_In scaleValues )
	{
		Imp::SubtractScaled_Imp( inoutVec, VEC3V_SOA_ARG(toSubtractFrom), VEC3V_SOA_ARG(toScaleThenSubtract), VEC3V_SOA_ARG(scaleValues) );
	}

	__forceinline void SubtractScaled( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toSubtractFrom, SoA_Vec2V_In toScaleThenSubtract, SoA_Vec2V_In scaleValues )
	{
		Imp::SubtractScaled_Imp( inoutVec, VEC2V_SOA_ARG(toSubtractFrom), VEC2V_SOA_ARG(toScaleThenSubtract), VEC2V_SOA_ARG(scaleValues) );
	}

	__forceinline SoA_ScalarV_Out SubtractScaled( SoA_ScalarV_In toSubtractFrom, SoA_ScalarV_In toScaleThenSubtract, SoA_ScalarV_In scaleValues )
	{
		return SoA_ScalarV( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin128(), toScaleThenSubtract.GetIntrin128(), scaleValues.GetIntrin128() ) );
	}

	__forceinline void Scale( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		Imp::Scale_Imp( inoutVec, VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::Scale_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		Imp::Scale_Imp( inoutVec, VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Dot( SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		return Imp::Dot_Imp_4_4( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Dot( SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		return Imp::Dot_Imp_3_3( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Dot( SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		return Imp::Dot_Imp_2_2( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Scale( SoA_ScalarV_In a, SoA_ScalarV_In b )
	{
		return SoA_ScalarV( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline void Scale( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_ScalarV_In b )
	{
		Imp::Scale_Imp_1_4( inoutVec, b.GetIntrin128(), VEC4V_SOA_ARG(a) );
	}

	__forceinline void Scale( SoA_Vec4V_InOut inoutVec, SoA_ScalarV_In a, SoA_Vec4V_In b )
	{
		Imp::Scale_Imp_1_4( inoutVec, a.GetIntrin128(), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_ScalarV_In b )
	{
		Imp::Scale_Imp_1_3( inoutVec, b.GetIntrin128(), VEC3V_SOA_ARG(a) );
	}

	__forceinline void Scale( SoA_Vec3V_InOut inoutVec, SoA_ScalarV_In a, SoA_Vec3V_In b )
	{
		Imp::Scale_Imp_1_3( inoutVec, a.GetIntrin128(), VEC3V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_ScalarV_In b )
	{
		Imp::Scale_Imp_1_2( inoutVec, b.GetIntrin128(), VEC2V_SOA_ARG(a) );
	}

	__forceinline void Scale( SoA_Vec2V_InOut inoutVec, SoA_ScalarV_In a, SoA_Vec2V_In b )
	{
		Imp::Scale_Imp_1_2( inoutVec, a.GetIntrin128(), VEC2V_SOA_ARG(b) );
	}

	__forceinline void InvScale( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue )
	{
		Imp::InvScale_Imp( inoutVec, VEC4V_SOA_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScale( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues )
	{
		Imp::InvScale_Imp( inoutVec, VEC4V_SOA_ARG(toScale), VEC4V_SOA_ARG(scaleValues) );
	}

	__forceinline void InvScaleSafe( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleSafe_Imp( inoutVec, VEC4V_SOA_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleSafe( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleSafe_Imp( inoutVec, VEC4V_SOA_ARG(toScale), VEC4V_SOA_ARG(scaleValues), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFast( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue )
	{
		Imp::InvScaleFast_Imp( inoutVec, VEC4V_SOA_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleFast( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues )
	{
		Imp::InvScaleFast_Imp( inoutVec, VEC4V_SOA_ARG(toScale), VEC4V_SOA_ARG(scaleValues) );
	}

	__forceinline void InvScaleFastSafe( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp( inoutVec, VEC4V_SOA_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFastSafe( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp( inoutVec, VEC4V_SOA_ARG(toScale), VEC4V_SOA_ARG(scaleValues), errValVect.GetIntrin128() );
	}

	__forceinline void InvScale( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue )
	{
		Imp::InvScale_Imp( inoutVec, VEC3V_SOA_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScale( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues )
	{
		Imp::InvScale_Imp( inoutVec, VEC3V_SOA_ARG(toScale), VEC3V_SOA_ARG(scaleValues) );
	}

	__forceinline void InvScaleSafe( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleSafe_Imp( inoutVec, VEC3V_SOA_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleSafe( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleSafe_Imp( inoutVec, VEC3V_SOA_ARG(toScale), VEC3V_SOA_ARG(scaleValues), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFast( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue )
	{
		Imp::InvScaleFast_Imp( inoutVec, VEC3V_SOA_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleFast( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues )
	{
		Imp::InvScaleFast_Imp( inoutVec, VEC3V_SOA_ARG(toScale), VEC3V_SOA_ARG(scaleValues) );
	}

	__forceinline void InvScaleFastSafe( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp( inoutVec, VEC3V_SOA_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFastSafe( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp( inoutVec, VEC3V_SOA_ARG(toScale), VEC3V_SOA_ARG(scaleValues), errValVect.GetIntrin128() );
	}















	__forceinline void InvScale( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue )
	{
		Imp::InvScale_Imp( inoutVec, VEC2V_SOA_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScale( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues )
	{
		Imp::InvScale_Imp( inoutVec, VEC2V_SOA_ARG(toScale), VEC2V_SOA_ARG(scaleValues) );
	}

	__forceinline void InvScaleSafe( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleSafe_Imp( inoutVec, VEC2V_SOA_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleSafe( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleSafe_Imp( inoutVec, VEC2V_SOA_ARG(toScale), VEC2V_SOA_ARG(scaleValues), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFast( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue )
	{
		Imp::InvScaleFast_Imp( inoutVec, VEC2V_SOA_ARG(toScale), scaleValue.GetIntrin128() );
	}

	__forceinline void InvScaleFast( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues )
	{
		Imp::InvScaleFast_Imp( inoutVec, VEC2V_SOA_ARG(toScale), VEC2V_SOA_ARG(scaleValues) );
	}

	__forceinline void InvScaleFastSafe( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp( inoutVec, VEC2V_SOA_ARG(toScale), scaleValue.GetIntrin128(), errValVect.GetIntrin128() );
	}

	__forceinline void InvScaleFastSafe( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues, SoA_ScalarV_In errValVect )
	{
		Imp::InvScaleFastSafe_Imp( inoutVec, VEC2V_SOA_ARG(toScale), VEC2V_SOA_ARG(scaleValues), errValVect.GetIntrin128() );
	}
	
	__forceinline SoA_ScalarV_Out InvScale( SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue )
	{
		return SoA_ScalarV( Vec::V4InvScale( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvScaleSafe( SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		return SoA_ScalarV( Vec::V4InvScaleSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvScaleFast( SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue )
	{
		return SoA_ScalarV( Vec::V4InvScaleFast( toScale.GetIntrin128(), scaleValue.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvScaleFastSafe( SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect )
	{
		return SoA_ScalarV( Vec::V4InvScaleFastSafe( toScale.GetIntrin128(), scaleValue.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline void Lerp( SoA_Vec4V_InOut inoutVec, SoA_ScalarV_In tValue, SoA_Vec4V_In vectA, SoA_Vec4V_In vectB )
	{
		Imp::Lerp_Imp( inoutVec, tValue.GetIntrin128(), VEC4V_SOA_ARG(vectA), VEC4V_SOA_ARG(vectB) );
	}

	__forceinline void Lerp( SoA_Vec3V_InOut inoutVec, SoA_ScalarV_In tValue, SoA_Vec3V_In vectA, SoA_Vec3V_In vectB )
	{
		Imp::Lerp_Imp( inoutVec, tValue.GetIntrin128(), VEC3V_SOA_ARG(vectA), VEC3V_SOA_ARG(vectB) );
	}

	__forceinline void Lerp( SoA_Vec2V_InOut inoutVec, SoA_ScalarV_In tValue, SoA_Vec2V_In vectA, SoA_Vec2V_In vectB )
	{
		Imp::Lerp_Imp( inoutVec, tValue.GetIntrin128(), VEC2V_SOA_ARG(vectA), VEC2V_SOA_ARG(vectB) );
	}

	__forceinline SoA_ScalarV_Out Lerp( SoA_ScalarV_In tValue, SoA_ScalarV_In vectA, SoA_ScalarV_In vectB )
	{
		return SoA_ScalarV( Vec::V4Lerp( tValue.GetIntrin128(), vectA.GetIntrin128(), vectB.GetIntrin128() ) );
	}

	//============================================================================
	// Conversion functions

	template <int exponent>
	__forceinline void FloatToIntRaw(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVec)
	{
		Imp::FloatToIntRaw_Imp<exponent>( inoutVec, VEC4V_SOA_ARG(inVec) );
	}

	template <int exponent>
	__forceinline void IntToFloatRaw(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVec)
	{
		Imp::IntToFloatRaw_Imp<exponent>( inoutVec, VEC4V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestInt(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVec)
	{
		Imp::RoundToNearestInt_Imp( inoutVec, VEC4V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntZero(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVec)
	{
		Imp::RoundToNearestIntZero_Imp( inoutVec, VEC4V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntNegInf(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVec)
	{
		Imp::RoundToNearestIntNegInf_Imp( inoutVec, VEC4V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntPosInf(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVec)
	{
		Imp::RoundToNearestIntPosInf_Imp( inoutVec, VEC4V_SOA_ARG(inVec) );
	}

	template <int exponent>
	__forceinline void FloatToIntRaw(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVec)
	{
		Imp::FloatToIntRaw_Imp<exponent>( inoutVec, VEC3V_SOA_ARG(inVec) );
	}

	template <int exponent>
	__forceinline void IntToFloatRaw(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVec)
	{
		Imp::IntToFloatRaw_Imp<exponent>( inoutVec, VEC3V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestInt(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVec)
	{
		Imp::RoundToNearestInt_Imp( inoutVec, VEC3V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntZero(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVec)
	{
		Imp::RoundToNearestIntZero_Imp( inoutVec, VEC3V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntNegInf(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVec)
	{
		Imp::RoundToNearestIntNegInf_Imp( inoutVec, VEC3V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntPosInf(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVec)
	{
		Imp::RoundToNearestIntPosInf_Imp( inoutVec, VEC3V_SOA_ARG(inVec) );
	}

	template <int exponent>
	__forceinline void FloatToIntRaw(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVec)
	{
		Imp::FloatToIntRaw_Imp<exponent>( inoutVec, VEC2V_SOA_ARG(inVec) );
	}

	template <int exponent>
	__forceinline void IntToFloatRaw(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVec)
	{
		Imp::IntToFloatRaw_Imp<exponent>( inoutVec, VEC2V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestInt(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVec)
	{
		Imp::RoundToNearestInt_Imp( inoutVec, VEC2V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntZero(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVec)
	{
		Imp::RoundToNearestIntZero_Imp( inoutVec, VEC2V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntNegInf(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVec)
	{
		Imp::RoundToNearestIntNegInf_Imp( inoutVec, VEC2V_SOA_ARG(inVec) );
	}

	__forceinline void RoundToNearestIntPosInf(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVec)
	{
		Imp::RoundToNearestIntPosInf_Imp( inoutVec, VEC2V_SOA_ARG(inVec) );
	}

	__forceinline void Normalize(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect)
	{
		Imp::Normalize_Imp( inoutVec, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void Normalize(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect)
	{
		Imp::Normalize_Imp( inoutVec, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void Normalize(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect)
	{
		Imp::Normalize_Imp( inoutVec, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline void NormalizeSafe(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeSafe_Imp( inoutVec, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void NormalizeSafe(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeSafe_Imp( inoutVec, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void NormalizeSafe(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeSafe_Imp( inoutVec, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void NormalizeFast(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect)
	{
		Imp::NormalizeFast_Imp( inoutVec, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void NormalizeFast(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect)
	{
		Imp::NormalizeFast_Imp( inoutVec, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void NormalizeFast(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect)
	{
		Imp::NormalizeFast_Imp( inoutVec, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline void NormalizeFastSafe(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeFastSafe_Imp( inoutVec, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void NormalizeFastSafe(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeFastSafe_Imp( inoutVec, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void NormalizeFastSafe(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeFastSafe_Imp( inoutVec, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void Conjugate(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat)
	{
		Imp::Conjugate_Imp_Q( inoutQuat, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void Normalize(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat)
	{
		Imp::Normalize_Imp( inoutQuat, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void NormalizeSafe(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeSafe_Imp( inoutQuat, QUATV_SOA_ARG(inQuat), errValVect.GetIntrin128() );
	}

	__forceinline void NormalizeFast(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat)
	{
		Imp::NormalizeFast_Imp( inoutQuat, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void NormalizeFastSafe(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In errValVect)
	{
		Imp::NormalizeFastSafe_Imp( inoutQuat, QUATV_SOA_ARG(inQuat), errValVect.GetIntrin128() );
	}

	__forceinline void Invert(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat)
	{
		Imp::Invert_Imp_Q( inoutQuat, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void InvertSafe(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In errValVect)
	{
		Imp::InvertSafe_Imp_Q( inoutQuat, QUATV_SOA_ARG(inQuat), errValVect.GetIntrin128() );
	}

	__forceinline void InvertFast(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat)
	{
		Imp::InvertFast_Imp_Q( inoutQuat, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void InvertFastSafe(SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In errValVect)
	{
		Imp::InvertFastSafe_Imp_Q( inoutQuat, QUATV_SOA_ARG(inQuat), errValVect.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out Dot( SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 )
	{
		return Imp::Dot_Imp_4_4( QUATV_SOA_ARG(inQuat1), QUATV_SOA_ARG(inQuat2) );
	}

	__forceinline void Multiply( SoA_QuatV_InOut inoutQuat, SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 )
	{
		Imp::Multiply_Imp_Q( inoutQuat, QUATV_SOA_ARG(inQuat1), QUATV_SOA_ARG(inQuat2) );
	}

	// TODO: Implement when needed.
	//__forceinline void Slerp( SoA_QuatV_InOut inoutQuat, SoA_ScalarV_In t, SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 )
	//{
	//	Imp::Slerp_Imp_Q( inoutQuat, t.GetIntrin128(), QUATV_SOA_ARG(inQuat1), QUATV_SOA_ARG(inQuat2) );
	//}

	__forceinline void PrepareSlerp( SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 )
	{
		Imp::PrepareSlerp_Imp_Q( outQuat, QUATV_SOA_ARG(inQuat1), QUATV_SOA_ARG(inQuat2) );
	}

	__forceinline void Nlerp( SoA_QuatV_InOut outQuat, SoA_ScalarV_In t, SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 )
	{
		Imp::Nlerp_Imp_Q( outQuat, t.GetIntrin128(), QUATV_SOA_ARG(inQuat1), QUATV_SOA_ARG(inQuat2) );
	}

	__forceinline void ToAxisAngle( SoA_Vec3V_InOut outAxis, SoA_ScalarV_InOut outRadians, SoA_QuatV_In inQuat )
	{
		Imp::ToAxisAngle_Imp_Q( outAxis, outRadians, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void FromAxisAngle( SoA_QuatV_InOut outQuat, SoA_Vec3V_In inNormAxis, SoA_ScalarV_In inRadians )
	{
		Imp::FromAxisAngle_Imp_Q( outQuat, VEC3V_SOA_ARG(inNormAxis), inRadians.GetIntrin128() );
	}

	__forceinline void ScaleAngle( SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In inRadians )
	{
		Imp::ScaleAngle_Imp_Q( outQuat, QUATV_SOA_ARG(inQuat), inRadians.GetIntrin128() );
	}

	__forceinline void Mat33VFromQuatV( SoA_Mat33V_InOut outMat, SoA_QuatV_In inQuat )
	{
		Imp::Mat33VFromQuatV_Imp_Q( outMat, QUATV_SOA_ARG(inQuat) );
	}

	__forceinline void QuatVFromEulersXYZ( SoA_QuatV_InOut outQuat, SoA_Vec3V_In radianAngles )
	{
		Imp::QuatVFromEulersXYZ_Imp_Q( outQuat, VEC3V_SOA_ARG(radianAngles) );
	}

	__forceinline SoA_ScalarV_Out Clamp( SoA_ScalarV_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound )
	{
		return SoA_ScalarV( Vec::V4Clamp( inVect.GetIntrin128(), lowBound.GetIntrin128(), highBound.GetIntrin128() ) );
	}

	__forceinline void Clamp( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound )
	{
		Imp::Clamp_Imp( inoutVec, VEC2V_SOA_ARG(inVect), lowBound.GetIntrin128(), highBound.GetIntrin128() );
	}

	__forceinline void Clamp( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound )
	{
		Imp::Clamp_Imp( inoutVec, VEC3V_SOA_ARG(inVect), lowBound.GetIntrin128(), highBound.GetIntrin128() );
	}

	__forceinline void Clamp( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound )
	{
		Imp::Clamp_Imp( inoutVec, VEC4V_SOA_ARG(inVect), lowBound.GetIntrin128(), highBound.GetIntrin128() );
	}

	__forceinline void ClampMag( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect, SoA_ScalarV_In minMag, SoA_ScalarV_In maxMag )
	{
		Imp::ClampMag_Imp( inoutVec, VEC3V_SOA_ARG(inVect), minMag.GetIntrin128(), maxMag.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out Negate(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4Negate( inVect.GetIntrin128() ) );
	}

	__forceinline void Negate(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect)
	{
		Imp::Negate_Imp( inoutVec, VEC2V_SOA_ARG( inVect ) );
	}

	__forceinline void Negate(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect)
	{
		Imp::Negate_Imp( inoutVec, VEC3V_SOA_ARG( inVect ) );
	}

	__forceinline void Negate(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect)
	{
		Imp::Negate_Imp( inoutVec, VEC4V_SOA_ARG( inVect ) );
	}

	__forceinline SoA_ScalarV_Out InvertBits(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline SoA_VecBool1V_Out InvertBits(SoA_VecBool1V_In inVect)
	{
		return SoA_VecBool1V( Vec::V4InvertBits( inVect.GetIntrin128() ) );
	}

	__forceinline void InvertBits(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect)
	{
		Imp::InvertBits_Imp( inoutVec, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void InvertBits(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect)
	{
		Imp::InvertBits_Imp( inoutVec, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void InvertBits(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect)
	{
		Imp::InvertBits_Imp( inoutVec, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline void InvertBits(SoA_VecBool2V_InOut inoutVec, SoA_VecBool2V_In inVect)
	{
		Imp::InvertBits_Imp( inoutVec, VECBOOL2V_SOA_ARG(inVect) );
	}

	__forceinline void InvertBits(SoA_VecBool3V_InOut inoutVec, SoA_VecBool3V_In inVect)
	{
		Imp::InvertBits_Imp( inoutVec, VECBOOL3V_SOA_ARG(inVect) );
	}

	__forceinline void InvertBits(SoA_VecBool4V_InOut inoutVec, SoA_VecBool4V_In inVect)
	{
		Imp::InvertBits_Imp( inoutVec, VECBOOL4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out Invert(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4Invert( inVect.GetIntrin128() ) );
	}

	__forceinline void Invert(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect)
	{
		Imp::Invert_Imp( inoutVec, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void Invert(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect)
	{
		Imp::Invert_Imp( inoutVec, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void Invert(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect)
	{
		Imp::Invert_Imp( inoutVec, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out InvertSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Vec::V4InvertSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline void InvertSafe(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvertSafe_Imp( inoutVec, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvertSafe(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvertSafe_Imp( inoutVec, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvertSafe(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvertSafe_Imp( inoutVec, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline SoA_ScalarV_Out InvertFast(SoA_ScalarV_In inVect)
	{
		return SoA_ScalarV( Vec::V4InvertFast( inVect.GetIntrin128() ) );
	}

	__forceinline void InvertFast(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect)
	{
		Imp::InvertFast_Imp( inoutVec, VEC2V_SOA_ARG(inVect) );
	}

	__forceinline void InvertFast(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect)
	{
		Imp::InvertFast_Imp( inoutVec, VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void InvertFast(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect)
	{
		Imp::InvertFast_Imp( inoutVec, VEC4V_SOA_ARG(inVect) );
	}

	__forceinline SoA_ScalarV_Out InvertFastSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Vec::V4InvertFastSafe( inVect.GetIntrin128(), errValVect.GetIntrin128() ) );
	}

	__forceinline void InvertFastSafe(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvertFastSafe_Imp( inoutVec, VEC2V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvertFastSafe(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvertFastSafe_Imp( inoutVec, VEC3V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline void InvertFastSafe(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect)
	{
		Imp::InvertFastSafe_Imp( inoutVec, VEC4V_SOA_ARG(inVect), errValVect.GetIntrin128() );
	}

	__forceinline SoA_Vec4V_Out SplatX( SoA_Vec4V_In a )
	{
		return SoA_Vec4V( a.GetXIntrin128() );
	}

	__forceinline SoA_Vec4V_Out SplatY( SoA_Vec4V_In a )
	{
		return SoA_Vec4V( a.GetYIntrin128() );
	}

	__forceinline SoA_Vec4V_Out SplatZ( SoA_Vec4V_In a )
	{
		return SoA_Vec4V( a.GetZIntrin128() );
	}

	__forceinline SoA_Vec4V_Out SplatW( SoA_Vec4V_In a )
	{
		return SoA_Vec4V( a.GetWIntrin128() );
	}

	__forceinline SoA_Vec3V_Out SplatX( SoA_Vec3V_In a )
	{
		return SoA_Vec3V( a.GetXIntrin128() );
	}

	__forceinline SoA_Vec3V_Out SplatY( SoA_Vec3V_In a )
	{
		return SoA_Vec3V( a.GetYIntrin128() );
	}

	__forceinline SoA_Vec3V_Out SplatZ( SoA_Vec3V_In a )
	{
		return SoA_Vec3V( a.GetZIntrin128() );
	}

	__forceinline SoA_Vec2V_Out SplatX( SoA_Vec2V_In a )
	{
		return SoA_Vec2V( a.GetXIntrin128() );
	}

	__forceinline SoA_Vec2V_Out SplatY( SoA_Vec2V_In a )
	{
		return SoA_Vec2V( a.GetYIntrin128() );
	}

	__forceinline SoA_VecBool4V_Out SplatX( SoA_VecBool4V_In a )
	{
		return SoA_VecBool4V( a.GetXIntrin128() );
	}

	__forceinline SoA_VecBool4V_Out SplatY( SoA_VecBool4V_In a )
	{
		return SoA_VecBool4V( a.GetYIntrin128() );
	}

	__forceinline SoA_VecBool4V_Out SplatZ( SoA_VecBool4V_In a )
	{
		return SoA_VecBool4V( a.GetZIntrin128() );
	}

	__forceinline SoA_VecBool4V_Out SplatW( SoA_VecBool4V_In a )
	{
		return SoA_VecBool4V( a.GetWIntrin128() );
	}

	__forceinline SoA_VecBool3V_Out SplatX( SoA_VecBool3V_In a )
	{
		return SoA_VecBool3V( a.GetXIntrin128() );
	}

	__forceinline SoA_VecBool3V_Out SplatY( SoA_VecBool3V_In a )
	{
		return SoA_VecBool3V( a.GetYIntrin128() );
	}

	__forceinline SoA_VecBool3V_Out SplatZ( SoA_VecBool3V_In a )
	{
		return SoA_VecBool3V( a.GetZIntrin128() );
	}

	__forceinline SoA_VecBool2V_Out SplatX( SoA_VecBool2V_In a )
	{
		return SoA_VecBool2V( a.GetXIntrin128() );
	}

	__forceinline SoA_VecBool2V_Out SplatY( SoA_VecBool2V_In a )
	{
		return SoA_VecBool2V( a.GetYIntrin128() );
	}

	__forceinline void SelectFT(SoA_QuatV_InOut inoutVec, SoA_VecBool1V_In choiceVector, SoA_QuatV_In zero, SoA_QuatV_In nonZero)
	{
		Imp::Select_Imp( inoutVec, choiceVector.GetIntrin128(), QUATV_SOA_ARG(zero), QUATV_SOA_ARG(nonZero) );
	}

	__forceinline void SelectFT(SoA_Vec4V_InOut inoutVec, SoA_VecBool4V_In choiceVector, SoA_Vec4V_In zero, SoA_Vec4V_In nonZero)
	{
		Imp::Select_Imp( inoutVec, VECBOOL4V_SOA_ARG(choiceVector), VEC4V_SOA_ARG(zero), VEC4V_SOA_ARG(nonZero) );
	}

	__forceinline void SelectFT(SoA_Vec3V_InOut inoutVec, SoA_VecBool3V_In choiceVector, SoA_Vec3V_In zero, SoA_Vec3V_In nonZero)
	{
		Imp::Select_Imp( inoutVec, VECBOOL3V_SOA_ARG(choiceVector), VEC3V_SOA_ARG(zero), VEC3V_SOA_ARG(nonZero) );
	}

	__forceinline void SelectFT(SoA_Vec2V_InOut inoutVec, SoA_VecBool2V_In choiceVector, SoA_Vec2V_In zero, SoA_Vec2V_In nonZero)
	{
		Imp::Select_Imp( inoutVec, VECBOOL2V_SOA_ARG(choiceVector), VEC2V_SOA_ARG(zero), VEC2V_SOA_ARG(nonZero) );
	}

	__forceinline SoA_ScalarV_Out SelectFT(SoA_VecBool1V_In choiceVector, SoA_ScalarV_In zero, SoA_ScalarV_In nonZero)
	{
		return SoA_ScalarV( Vec::V4SelectFT( choiceVector.GetIntrin128(), zero.GetIntrin128(), nonZero.GetIntrin128() ) );
	}

	__forceinline void SelectFT(SoA_Vec4V_InOut inoutVec, SoA_VecBool1V_In choiceVector, SoA_Vec4V_In zero, SoA_Vec4V_In nonZero)
	{
		Imp::Select_Imp( inoutVec, choiceVector.GetIntrin128(), VEC4V_SOA_ARG(zero), VEC4V_SOA_ARG(nonZero) );
	}

	__forceinline void SelectFT(SoA_Vec3V_InOut inoutVec, SoA_VecBool1V_In choiceVector, SoA_Vec3V_In zero, SoA_Vec3V_In nonZero)
	{
		Imp::Select_Imp( inoutVec, choiceVector.GetIntrin128(), VEC3V_SOA_ARG(zero), VEC3V_SOA_ARG(nonZero) );
	}

	__forceinline void SelectFT(SoA_Vec2V_InOut inoutVec, SoA_VecBool1V_In choiceVector, SoA_Vec2V_In zero, SoA_Vec2V_In nonZero)
	{
		Imp::Select_Imp( inoutVec, choiceVector.GetIntrin128(), VEC2V_SOA_ARG(zero), VEC2V_SOA_ARG(nonZero) );
	}

	__forceinline void Add( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		Imp::Add_Imp( inoutVec, VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Add( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::Add_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void Add( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		Imp::Add_Imp( inoutVec, VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Add( SoA_ScalarV_In a, SoA_ScalarV_In b )
	{
		return SoA_ScalarV( Vec::V4Add( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline void Subtract( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		Imp::Subtract_Imp( inoutVec, VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Subtract( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::Subtract_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void Subtract( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		Imp::Subtract_Imp( inoutVec, VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Subtract( SoA_ScalarV_In a, SoA_ScalarV_In b )
	{
		return SoA_ScalarV( Vec::V4Subtract( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline void Average( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Vec4V_In b )
	{
		Imp::Average_Imp( inoutVec, VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Average( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::Average_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void Average( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		Imp::Average_Imp( inoutVec, VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Average( SoA_ScalarV_In a, SoA_ScalarV_In b )
	{
		return SoA_ScalarV( Vec::V4Average( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline void Pow( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In x, SoA_Vec4V_In y )
	{
		Imp::Pow_Imp( inoutVec, VEC4V_SOA_ARG(x), VEC4V_SOA_ARG(y) );
	}

	__forceinline void Pow( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In x, SoA_Vec3V_In y )
	{
		Imp::Pow_Imp( inoutVec, VEC3V_SOA_ARG(x), VEC3V_SOA_ARG(y) );
	}

	__forceinline void Pow( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In x, SoA_Vec2V_In y )
	{
		Imp::Pow_Imp( inoutVec, VEC2V_SOA_ARG(x), VEC2V_SOA_ARG(y) );
	}

	__forceinline SoA_ScalarV_Out Pow( SoA_ScalarV_In x, SoA_ScalarV_In y )
	{
		return SoA_ScalarV( Vec::V4Pow( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline void PowPrecise( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In x, SoA_Vec4V_In y )
	{
		Imp::PowPrecise_Imp( inoutVec, VEC4V_SOA_ARG(x), VEC4V_SOA_ARG(y) );
	}

	__forceinline void PowPrecise( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In x, SoA_Vec3V_In y )
	{
		Imp::PowPrecise_Imp( inoutVec, VEC3V_SOA_ARG(x), VEC3V_SOA_ARG(y) );
	}

	__forceinline void PowPrecise( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In x, SoA_Vec2V_In y )
	{
		Imp::PowPrecise_Imp( inoutVec, VEC2V_SOA_ARG(x), VEC2V_SOA_ARG(y) );
	}

	__forceinline SoA_ScalarV_Out PowPrecise( SoA_ScalarV_In x, SoA_ScalarV_In y )
	{
		return SoA_ScalarV( Vec::V4PowPrecise( x.GetIntrin128(), y.GetIntrin128() ) );
	}

	__forceinline void Expt( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In x )
	{
		Imp::Expt_Imp( inoutVec, VEC4V_SOA_ARG(x) );
	}

	__forceinline void Expt( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In x )
	{
		Imp::Expt_Imp( inoutVec, VEC3V_SOA_ARG(x) );
	}

	__forceinline void Expt( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In x )
	{
		Imp::Expt_Imp( inoutVec, VEC2V_SOA_ARG(x) );
	}

	__forceinline SoA_ScalarV_Out Expt( SoA_ScalarV_In x )
	{
		return SoA_ScalarV( Vec::V4Expt( x.GetIntrin128() ) );
	}

	__forceinline void Log2( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In x )
	{
		Imp::Log2_Imp( inoutVec, VEC4V_SOA_ARG(x) );
	}

	__forceinline void Log2( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In x )
	{
		Imp::Log2_Imp( inoutVec, VEC3V_SOA_ARG(x) );
	}

	__forceinline void Log2( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In x )
	{
		Imp::Log2_Imp( inoutVec, VEC2V_SOA_ARG(x) );
	}

	__forceinline SoA_ScalarV_Out Log2( SoA_ScalarV_In x )
	{
		return SoA_ScalarV( Vec::V4Log2( x.GetIntrin128() ) );
	}

	__forceinline void Log10( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In x )
	{
		Imp::Log10_Imp( inoutVec, VEC4V_SOA_ARG(x) );
	}

	__forceinline void Log10( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In x )
	{
		Imp::Log10_Imp( inoutVec, VEC3V_SOA_ARG(x) );
	}

	__forceinline void Log10( SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In x )
	{
		Imp::Log10_Imp( inoutVec, VEC2V_SOA_ARG(x) );
	}

	__forceinline SoA_ScalarV_Out Log10( SoA_ScalarV_In x )
	{
		return SoA_ScalarV( Vec::V4Log10( x.GetIntrin128() ) );
	}

	__forceinline void Max(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::Max_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void Max(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::Max_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void Max(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::Max_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_ScalarV_Out Max(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_ScalarV( Vec::V4Max( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Min(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::Min_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void Min(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::Min_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void Min(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::Min_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_ScalarV_Out Min(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_ScalarV( Vec::V4Min( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void And(SoA_VecBool4V_InOut inoutVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		Imp::And_Imp( inoutVec, VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) );
	}

	__forceinline void And(SoA_VecBool3V_InOut inoutVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		Imp::And_Imp( inoutVec, VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) );
	}

	__forceinline void And(SoA_VecBool2V_InOut inoutVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		Imp::And_Imp( inoutVec, VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out And(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void And(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::And_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void And(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::And_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void And(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::And_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_ScalarV_Out And(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_ScalarV( Vec::V4And( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Or(SoA_VecBool4V_InOut inoutVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		Imp::Or_Imp( inoutVec, VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) );
	}

	__forceinline void Or(SoA_VecBool3V_InOut inoutVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		Imp::Or_Imp( inoutVec, VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) );
	}

	__forceinline void Or(SoA_VecBool2V_InOut inoutVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		Imp::Or_Imp( inoutVec, VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out Or(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Or(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::Or_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void Or(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::Or_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void Or(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::Or_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_ScalarV_Out Or(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_ScalarV( Vec::V4Or( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Xor(SoA_VecBool4V_InOut inoutVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		Imp::Xor_Imp( inoutVec, VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) );
	}

	__forceinline void Xor(SoA_VecBool3V_InOut inoutVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		Imp::Xor_Imp( inoutVec, VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) );
	}

	__forceinline void Xor(SoA_VecBool2V_InOut inoutVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		Imp::Xor_Imp( inoutVec, VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out Xor(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Xor(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::Xor_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void Xor(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::Xor_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void Xor(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::Xor_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_ScalarV_Out Xor(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_ScalarV( Vec::V4Xor( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Andc(SoA_VecBool4V_InOut inoutVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2)
	{
		Imp::Andc_Imp( inoutVec, VECBOOL4V_SOA_ARG(inVector1), VECBOOL4V_SOA_ARG(inVector2) );
	}

	__forceinline void Andc(SoA_VecBool3V_InOut inoutVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2)
	{
		Imp::Andc_Imp( inoutVec, VECBOOL3V_SOA_ARG(inVector1), VECBOOL3V_SOA_ARG(inVector2) );
	}

	__forceinline void Andc(SoA_VecBool2V_InOut inoutVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2)
	{
		Imp::Andc_Imp( inoutVec, VECBOOL2V_SOA_ARG(inVector1), VECBOOL2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_VecBool1V_Out Andc(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2)
	{
		return SoA_VecBool1V( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline void Andc(SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2)
	{
		Imp::Andc_Imp( inoutVec, VEC4V_SOA_ARG(inVector1), VEC4V_SOA_ARG(inVector2) );
	}

	__forceinline void Andc(SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2)
	{
		Imp::Andc_Imp( inoutVec, VEC3V_SOA_ARG(inVector1), VEC3V_SOA_ARG(inVector2) );
	}

	__forceinline void Andc(SoA_Vec2V_InOut inoutVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2)
	{
		Imp::Andc_Imp( inoutVec, VEC2V_SOA_ARG(inVector1), VEC2V_SOA_ARG(inVector2) );
	}

	__forceinline SoA_ScalarV_Out Andc(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2)
	{
		return SoA_ScalarV( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out Andc(SoA_ScalarV_In inVector1, SoA_VecBool1V_In inVector2)
	{
		return SoA_ScalarV( Vec::V4Andc( inVector1.GetIntrin128(), inVector2.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out Dist(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::Dist_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out DistFast(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::DistFast_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out Dist(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::Dist_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out DistFast(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::DistFast_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out Dist(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::Dist_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out DistFast(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::DistFast_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDist(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::InvDist_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSafe_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistFast(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::InvDistFast_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistFastSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistFastSafe_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDist(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::InvDist_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSafe_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistFast(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::InvDistFast_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistFastSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistFastSafe_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDist(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::InvDist_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSafe_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistFast(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::InvDistFast_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistFastSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistFastSafe_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out DistSquared(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::DistSquared_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out DistSquared(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::DistSquared_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out DistSquared(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::DistSquared_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquared(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::InvDistSquared_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSquaredSafe_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredFast(SoA_Vec4V_In a, SoA_Vec4V_In b)
	{
		return SoA_ScalarV( Imp::InvDistSquaredFast_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredFastSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSquaredFastSafe_Imp( VEC4V_SOA_ARG(a), VEC4V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquared(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::InvDistSquared_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSquaredSafe_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredFast(SoA_Vec3V_In a, SoA_Vec3V_In b)
	{
		return SoA_ScalarV( Imp::InvDistSquaredFast_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredFastSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSquaredFastSafe_Imp( VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquared(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::InvDistSquared_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSquaredSafe_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredFast(SoA_Vec2V_In a, SoA_Vec2V_In b)
	{
		return SoA_ScalarV( Imp::InvDistSquaredFast_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	__forceinline SoA_ScalarV_Out InvDistSquaredFastSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect)
	{
		return SoA_ScalarV( Imp::InvDistSquaredFastSafe_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b), errValVect.GetIntrin128() ) );
	}

	__forceinline void Cross( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Vec3V_In b )
	{
		Imp::Cross_Imp( inoutVec, VEC3V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline SoA_ScalarV_Out Cross( SoA_Vec2V_In a, SoA_Vec2V_In b )
	{
		return SoA_ScalarV( Imp::Cross_Imp( VEC2V_SOA_ARG(a), VEC2V_SOA_ARG(b) ) );
	}

	//============================================================================
	// Matrix functions

	__forceinline SoA_ScalarV_Out Determinant( SoA_Mat33V_In mat )
	{
		return SoA_ScalarV( Imp::Determinant_Imp33( MAT33V_SOA_ARG(mat) ) );
	}

	__forceinline SoA_ScalarV_Out Determinant( SoA_Mat44V_In mat )
	{
		return SoA_ScalarV( Imp::Determinant_Imp44( MAT44V_SOA_ARG(mat) ) );
	}

	__forceinline void Add( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In a, SoA_Mat33V_In b )
	{
		Imp::Add_Imp33( inoutMat, MAT33V_SOA_ARG(a), MAT33V_SOA_ARG(b) );
	}

	__forceinline void Subtract( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In a, SoA_Mat33V_In b )
	{
		Imp::Subtract_Imp33( inoutMat, MAT33V_SOA_ARG(a), MAT33V_SOA_ARG(b) );
	}

	__forceinline void Abs( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In a )
	{
		Imp::Abs_Imp33( inoutMat, MAT33V_SOA_ARG(a) );
	}

	__forceinline void Scale( SoA_Mat33V_InOut inoutMat, SoA_ScalarV_In a, SoA_Mat33V_In b )
	{
		Imp::Scale_Imp33( inoutMat, a.GetIntrin128(), MAT33V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In a, SoA_ScalarV_In b )
	{
		Imp::Scale_Imp33( inoutMat, b.GetIntrin128(), MAT33V_SOA_ARG(a) );
	}

	__forceinline void Add( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In a, SoA_Mat34V_In b )
	{
		Imp::Add_Imp34( inoutMat, MAT34V_SOA_ARG(a), MAT34V_SOA_ARG(b) );
	}

	__forceinline void Subtract( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In a, SoA_Mat34V_In b )
	{
		Imp::Subtract_Imp34( inoutMat, MAT34V_SOA_ARG(a), MAT34V_SOA_ARG(b) );
	}

	__forceinline void Abs( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In a )
	{
		Imp::Abs_Imp34( inoutMat, MAT34V_SOA_ARG(a) );
	}

	__forceinline void Scale( SoA_Mat34V_InOut inoutMat, SoA_ScalarV_In a, SoA_Mat34V_In b )
	{
		Imp::Scale_Imp34( inoutMat, a.GetIntrin128(), MAT34V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In a, SoA_ScalarV_In b )
	{
		Imp::Scale_Imp34( inoutMat, b.GetIntrin128(), MAT34V_SOA_ARG(a) );
	}

	__forceinline void Add( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In a, SoA_Mat44V_In b )
	{
		Imp::Add_Imp44( inoutMat, MAT44V_SOA_ARG(a), MAT44V_SOA_ARG(b) );
	}

	__forceinline void Subtract( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In a, SoA_Mat44V_In b )
	{
		Imp::Subtract_Imp44( inoutMat, MAT44V_SOA_ARG(a), MAT44V_SOA_ARG(b) );
	}

	__forceinline void Abs( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In a )
	{
		Imp::Abs_Imp44( inoutMat, MAT44V_SOA_ARG(a) );
	}

	__forceinline void Scale( SoA_Mat44V_InOut inoutMat, SoA_ScalarV_In a, SoA_Mat44V_In b )
	{
		Imp::Scale_Imp44( inoutMat, a.GetIntrin128(), MAT44V_SOA_ARG(b) );
	}

	__forceinline void Scale( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In a, SoA_ScalarV_In b )
	{
		Imp::Scale_Imp44( inoutMat, b.GetIntrin128(), MAT44V_SOA_ARG(a) );
	}

	__forceinline void Transpose( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In mat )
	{
		Imp::Transpose_Imp44( inoutMat, MAT44V_SOA_ARG(mat) );
	}

	__forceinline void InvertFull( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In mat )
	{
		Imp::InvertFull_Imp44( inoutMat, MAT44V_SOA_ARG(mat) );
	}

	__forceinline void InvertTransformFull( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In mat )
	{
		Imp::InvertTransformFull_Imp34( inoutMat, MAT34V_SOA_ARG(mat) );
	}

	__forceinline void InvertTransformOrtho( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In mat )
	{
		Imp::InvertTransformOrtho_Imp34( inoutMat, MAT34V_SOA_ARG(mat) );
	}

	__forceinline void Transpose( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In mat )
	{
		Imp::Transpose_Imp33( inoutMat, MAT33V_SOA_ARG(mat) );
	}

	__forceinline void InvertFull( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In mat )
	{
		Imp::InvertFull_Imp33( inoutMat, MAT33V_SOA_ARG(mat) );
	}

	__forceinline void InvertOrtho( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In mat )
	{
		Imp::Transpose_Imp33( inoutMat, MAT33V_SOA_ARG(mat) );
	}

	__forceinline void Multiply( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In a, SoA_Mat44V_In b )
	{
		Imp::Mul_Imp_44_44( inoutMat, MAT44V_SOA_ARG(a), MAT44V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In a, SoA_Mat33V_In b )
	{
		Imp::Mul_Imp_33_33( inoutMat, MAT33V_SOA_ARG(a), MAT33V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Vec4V_InOut inoutVec, SoA_Mat44V_In a, SoA_Vec4V_In b )
	{
		Imp::Mul_Imp_44_4( inoutVec, MAT44V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Vec4V_InOut inoutVec, SoA_Vec4V_In a, SoA_Mat44V_In b )
	{
		Imp::Mul_Imp_4_44( inoutVec, VEC4V_SOA_ARG(a), MAT44V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Vec3V_InOut inoutVec, SoA_Mat33V_In a, SoA_Vec3V_In b )
	{
		Imp::Mul_Imp_33_3( inoutVec, MAT33V_SOA_ARG(a), VEC3V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Vec3V_InOut inoutVec, SoA_Vec3V_In a, SoA_Mat33V_In b )
	{
		Imp::Mul_Imp_3_33( inoutVec, VEC3V_SOA_ARG(a), MAT33V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Vec3V_InOut inoutVec, SoA_Mat34V_In a, SoA_Vec4V_In b )
	{
		Imp::Mul_Imp_34_4( inoutVec, MAT34V_SOA_ARG(a), VEC4V_SOA_ARG(b) );
	}

	__forceinline void Multiply( SoA_Vec4V_InOut inoutVec, SoA_Vec3V_In a, SoA_Mat34V_In b )
	{
		Imp::Mul_Imp_3_34( inoutVec, VEC3V_SOA_ARG(a), MAT34V_SOA_ARG(b) );
	}

	__forceinline void UnTransformFull( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In origTransformMat, SoA_Mat44V_In concatMat )
	{
		Imp::UnTransformFull_Imp_44_44( inoutMat, MAT44V_SOA_ARG(origTransformMat), MAT44V_SOA_ARG(concatMat) );
	}

	__forceinline void UnTransformOrtho( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In origOrthoTransformMat, SoA_Mat44V_In concatMat )
	{
		Imp::UnTransformOrtho_Imp_44_44( inoutMat, MAT44V_SOA_ARG(origOrthoTransformMat), MAT44V_SOA_ARG(concatMat) );
	}

	__forceinline void UnTransformFull( SoA_Vec4V_InOut inoutVec, SoA_Mat44V_In origTransformMat, SoA_Vec4V_In transformedVect )
	{
		Imp::UnTransformFull_Imp_44_4( inoutVec, MAT44V_SOA_ARG(origTransformMat), VEC4V_SOA_ARG(transformedVect) );
	}

	__forceinline void UnTransformOrtho( SoA_Vec4V_InOut inoutVec, SoA_Mat44V_In origOrthoTransformMat, SoA_Vec4V_In transformedVect )
	{
		Imp::UnTransformOrtho_Imp_44_4( inoutVec, MAT44V_SOA_ARG(origOrthoTransformMat), VEC4V_SOA_ARG(transformedVect) );
	}

	__forceinline void UnTransformFull( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In origTransformMat, SoA_Mat33V_In concatMat )
	{
		Imp::UnTransformFull_Imp_33_33( inoutMat, MAT33V_SOA_ARG(origTransformMat), MAT33V_SOA_ARG(concatMat) );
	}

	__forceinline void UnTransformOrtho( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In origOrthoTransformMat, SoA_Mat33V_In concatMat )
	{
		Imp::UnTransformOrtho_Imp_33_33( inoutMat, MAT33V_SOA_ARG(origOrthoTransformMat), MAT33V_SOA_ARG(concatMat) );
	}

	__forceinline void UnTransformFull( SoA_Vec3V_InOut inoutVec, SoA_Mat33V_In origTransformMat, SoA_Vec3V_In transformedVect )
	{
		Imp::UnTransformFull_Imp_33_3( inoutVec, MAT33V_SOA_ARG(origTransformMat), VEC3V_SOA_ARG(transformedVect) );
	}

	__forceinline void UnTransformOrtho( SoA_Vec3V_InOut inoutVec, SoA_Mat33V_In origOrthoTransformMat, SoA_Vec3V_In transformedVect )
	{
		Imp::UnTransformOrtho_Imp_33_3( inoutVec, MAT33V_SOA_ARG(origOrthoTransformMat), VEC3V_SOA_ARG(transformedVect) );
	}

	__forceinline void Transform( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In transformMat1, SoA_Mat34V_In transformMat2 )
	{
		Imp::Transform_Imp_34_34( inoutMat, MAT34V_SOA_ARG(transformMat1), MAT34V_SOA_ARG(transformMat2) );
	}

	__forceinline void Transform3x3( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In prependMat, SoA_Mat34V_In transformMat )
	{
		SoA_Mat33V temp;
		Imp::Mul_Imp_33_33( temp, MAT33V_SOA_ARG(prependMat), MAT33V_SOA_ARG(transformMat) );
		inoutMat = SoA_Mat34V(
			temp.GetM00Intrin128(),
			temp.GetM10Intrin128(),
			temp.GetM20Intrin128(),
			temp.GetM01Intrin128(),
			temp.GetM11Intrin128(),
			temp.GetM21Intrin128(),
			temp.GetM02Intrin128(),
			temp.GetM12Intrin128(),
			temp.GetM22Intrin128(),
			transformMat.GetM03Intrin128(),
			transformMat.GetM13Intrin128(),
			transformMat.GetM23Intrin128()
			);
	}

	__forceinline void Transform3x3( SoA_Vec3V_InOut inoutVec, SoA_Mat34V_In transformMat, SoA_Vec3V_In inVect )
	{
		// In this one case, we don't need to pass down the 4th (translation) column. So pass a SoA_Mat33V instead.
		Imp::Transform3x3_Imp_34_3( inoutVec, MAT33V_SOA_ARG(transformMat), VEC3V_SOA_ARG(inVect) );
	}

	__forceinline void Transform( SoA_Vec3V_InOut inoutVec, SoA_Mat34V_In transformMat, SoA_Vec3V_In inPoint )
	{
		Imp::Transform_Imp_34_3( inoutVec, MAT34V_SOA_ARG(transformMat), VEC3V_SOA_ARG(inPoint) );
	}

	__forceinline void UnTransform3x3Full( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In origTransformMat, SoA_Mat34V_In concatMat )
	{
		SoA_Mat33V temp;
		Imp::UnTransformFull_Imp_33_33( temp, MAT33V_SOA_ARG(origTransformMat), MAT33V_SOA_ARG(concatMat) );
		inoutMat = SoA_Mat34V(
			temp.GetM00Intrin128(),
			temp.GetM10Intrin128(),
			temp.GetM20Intrin128(),
			temp.GetM01Intrin128(),
			temp.GetM11Intrin128(),
			temp.GetM21Intrin128(),
			temp.GetM02Intrin128(),
			temp.GetM12Intrin128(),
			temp.GetM22Intrin128(),
			concatMat.GetM03Intrin128(),
			concatMat.GetM13Intrin128(),
			concatMat.GetM23Intrin128()
			);
	}

	__forceinline void UnTransform3x3FullOrtho( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In origOrthoTransformMat, SoA_Mat34V_In concatMat )
	{
		SoA_Mat33V temp;
		Imp::UnTransformOrtho_Imp_33_33( temp, MAT33V_SOA_ARG(origOrthoTransformMat), MAT33V_SOA_ARG(concatMat) );
		inoutMat = SoA_Mat34V(
			temp.GetM00Intrin128(),
			temp.GetM10Intrin128(),
			temp.GetM20Intrin128(),
			temp.GetM01Intrin128(),
			temp.GetM11Intrin128(),
			temp.GetM21Intrin128(),
			temp.GetM02Intrin128(),
			temp.GetM12Intrin128(),
			temp.GetM22Intrin128(),
			concatMat.GetM03Intrin128(),
			concatMat.GetM13Intrin128(),
			concatMat.GetM23Intrin128()
			);
	}

	__forceinline void UnTransformFull( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In origTransformMat, SoA_Mat34V_In concatMat )
	{
		Imp::UnTransformFull_Imp_34_34( inoutMat, MAT34V_SOA_ARG(origTransformMat), MAT34V_SOA_ARG(concatMat) );
	}

	__forceinline void UnTransformOrtho( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In origOrthoTransformMat, SoA_Mat34V_In concatMat )
	{
		Imp::UnTransformOrtho_Imp_34_34( inoutMat, MAT34V_SOA_ARG(origOrthoTransformMat), MAT34V_SOA_ARG(concatMat) );
	}

	__forceinline void UnTransform3x3Full( SoA_Vec3V_InOut inoutVec, SoA_Mat34V_In origTransformMat, SoA_Vec3V_In transformedVect )
	{
		Imp::UnTransform3x3Full_Imp_34_3( inoutVec, MAT33V_SOA_ARG(origTransformMat), VEC3V_SOA_ARG(transformedVect) );
	}

	__forceinline void UnTransformFull( SoA_Vec3V_InOut inoutPoint, SoA_Mat34V_In origTransformMat, SoA_Vec3V_In transformedPoint )
	{
		Imp::UnTransformFull_Imp_34_3( inoutPoint, MAT34V_SOA_ARG(origTransformMat), VEC3V_SOA_ARG(transformedPoint) );
	}

	__forceinline void UnTransform3x3Ortho( SoA_Vec3V_InOut inoutVec, SoA_Mat34V_In origOrthoTransformMat, SoA_Vec3V_In transformedVect )
	{
		Imp::UnTransform3x3Ortho_Imp_34_3( inoutVec, MAT33V_SOA_ARG(origOrthoTransformMat), VEC3V_SOA_ARG(transformedVect) );
	}

	__forceinline void UnTransformOrtho( SoA_Vec3V_InOut inoutPoint, SoA_Mat34V_In origOrthoTransformMat, SoA_Vec3V_In transformedPoint )
	{
		Imp::UnTransformOrtho_Imp_34_3( inoutPoint, MAT34V_SOA_ARG(origOrthoTransformMat), VEC3V_SOA_ARG(transformedPoint) );
	}

	__forceinline void ReOrthonormalize( SoA_Mat33V_InOut inoutMat, SoA_Mat33V_In inMat )
	{
		Imp::ReOrthonormalize_Imp33( inoutMat, MAT33V_SOA_ARG(inMat) );
	}

	__forceinline void ReOrthonormalize3x3( SoA_Mat34V_InOut inoutMat, SoA_Mat34V_In inMat )
	{
		SoA_Mat33V temp;
		Imp::ReOrthonormalize_Imp33( temp, MAT33V_SOA_ARG(inMat) );
		inoutMat = SoA_Mat34V(
			temp.GetM00Intrin128(), temp.GetM10Intrin128(), temp.GetM20Intrin128(),
			temp.GetM01Intrin128(), temp.GetM11Intrin128(), temp.GetM21Intrin128(),
			temp.GetM02Intrin128(), temp.GetM12Intrin128(), temp.GetM22Intrin128(),
			inMat.GetM03Intrin128(), inMat.GetM13Intrin128(), inMat.GetM23Intrin128()
			);
	}

	__forceinline void ReOrthonormalize3x3( SoA_Mat44V_InOut inoutMat, SoA_Mat44V_In inMat )
	{
		SoA_Mat33V temp;
		Imp::ReOrthonormalize_Imp33( temp, MAT33V_SOA_ARG(inMat) );
		inoutMat = SoA_Mat44V(
			temp.GetM00Intrin128(), temp.GetM10Intrin128(), temp.GetM20Intrin128(), inMat.GetM30Intrin128(),
			temp.GetM01Intrin128(), temp.GetM11Intrin128(), temp.GetM21Intrin128(), inMat.GetM31Intrin128(),
			temp.GetM02Intrin128(), temp.GetM12Intrin128(), temp.GetM22Intrin128(), inMat.GetM32Intrin128(),
			inMat.GetM03Intrin128(), inMat.GetM13Intrin128(), inMat.GetM23Intrin128(), inMat.GetM33Intrin128()
			);
	}






















































namespace Imp
{

	// The _Imp() functions are provided to hide the ugly syntax (macro surrounding a Mat*V argument) that is necessary to help pass via vector registers.
	// The non-_Imp() functions that call the _Imp() functions are very short and are __forceinline'd so that there is no param passing on the stack at all,
	// EVER! (except when the # of arguments exceeds the platform's register passing limits... see README.txt)

	//================================================
	// For private use only...
	//================================================

	inline void Abs_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V invAbsMask = Vec::V4VConstant(V_80000000);
		inoutVect = SoA_Vec2V(
			Vec::V4Andc( localInput.GetXIntrin128(), invAbsMask ),
			Vec::V4Andc( localInput.GetYIntrin128(), invAbsMask )
			);
	}

	inline void Abs_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V invAbsMask = Vec::V4VConstant(V_80000000);
		inoutVect = SoA_Vec3V(
			Vec::V4Andc( localInput.GetXIntrin128(), invAbsMask ),
			Vec::V4Andc( localInput.GetYIntrin128(), invAbsMask ),
			Vec::V4Andc( localInput.GetZIntrin128(), invAbsMask )
			);
	}

	inline void Abs_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect))
	{
		// 100000 no-inline loop:
		// WAY #1: 7417.042480 us
		// WAY #2: 6938.157715 us
		// That's unexpected, even after looking at disasm..

		// WAY #1
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V invAbsMask = Vec::V4VConstant(V_80000000);
		inoutVect = SoA_Vec4V(
			Vec::V4Andc( localInput.GetXIntrin128(), invAbsMask ),
			Vec::V4Andc( localInput.GetYIntrin128(), invAbsMask ),
			Vec::V4Andc( localInput.GetZIntrin128(), invAbsMask ),
			Vec::V4Andc( localInput.GetWIntrin128(), invAbsMask )
			);

		// WAY #2
		//SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		//inoutVect = SoA_Vec4V(
		//	Vec::V4Abs( localInput.GetXIntrin128() ),
		//	Vec::V4Abs( localInput.GetYIntrin128() ),
		//	Vec::V4Abs( localInput.GetZIntrin128() ),
		//	Vec::V4Abs( localInput.GetWIntrin128() )
		//	);
	}

	inline void Sqrt_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4Sqrt( localInput.GetXIntrin128() ),
			Vec::V4Sqrt( localInput.GetYIntrin128() )
			);
	}

	inline void Sqrt_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4Sqrt( localInput.GetXIntrin128() ),
			Vec::V4Sqrt( localInput.GetYIntrin128() ),
			Vec::V4Sqrt( localInput.GetZIntrin128() )
			);
	}

	inline void Sqrt_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4Sqrt( localInput.GetXIntrin128() ),
			Vec::V4Sqrt( localInput.GetYIntrin128() ),
			Vec::V4Sqrt( localInput.GetZIntrin128() ),
			Vec::V4Sqrt( localInput.GetWIntrin128() )
			);
	}

	inline void SqrtSafe_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect)
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4SqrtSafe( localInput.GetXIntrin128(), errValVect ),
			Vec::V4SqrtSafe( localInput.GetYIntrin128(), errValVect )
			);
	}

	inline void SqrtSafe_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4SqrtSafe( localInput.GetXIntrin128(), errValVect ),
			Vec::V4SqrtSafe( localInput.GetYIntrin128(), errValVect ),
			Vec::V4SqrtSafe( localInput.GetZIntrin128(), errValVect )
			);
	}

	inline void SqrtSafe_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4SqrtSafe( localInput.GetXIntrin128(), errValVect ),
			Vec::V4SqrtSafe( localInput.GetYIntrin128(), errValVect ),
			Vec::V4SqrtSafe( localInput.GetZIntrin128(), errValVect ),
			Vec::V4SqrtSafe( localInput.GetWIntrin128(), errValVect )
			);
	}

	inline void SqrtFast_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4SqrtFast( localInput.GetXIntrin128() ),
			Vec::V4SqrtFast( localInput.GetYIntrin128() )
			);
	}

	inline void SqrtFast_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4SqrtFast( localInput.GetXIntrin128() ),
			Vec::V4SqrtFast( localInput.GetYIntrin128() ),
			Vec::V4SqrtFast( localInput.GetZIntrin128() )
			);
	}

	inline void SqrtFast_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4SqrtFast( localInput.GetXIntrin128() ),
			Vec::V4SqrtFast( localInput.GetYIntrin128() ),
			Vec::V4SqrtFast( localInput.GetZIntrin128() ),
			Vec::V4SqrtFast( localInput.GetWIntrin128() )
			);
	}

	inline void SqrtFastSafe_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect)
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4SqrtFastSafe( localInput.GetXIntrin128(), errValVect ),
			Vec::V4SqrtFastSafe( localInput.GetYIntrin128(), errValVect )
			);
	}

	inline void SqrtFastSafe_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4SqrtFastSafe( localInput.GetXIntrin128(), errValVect ),
			Vec::V4SqrtFastSafe( localInput.GetYIntrin128(), errValVect ),
			Vec::V4SqrtFastSafe( localInput.GetZIntrin128(), errValVect )
			);
	}

	inline void SqrtFastSafe_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4SqrtFastSafe( localInput.GetXIntrin128(), errValVect ),
			Vec::V4SqrtFastSafe( localInput.GetYIntrin128(), errValVect ),
			Vec::V4SqrtFastSafe( localInput.GetZIntrin128(), errValVect ),
			Vec::V4SqrtFastSafe( localInput.GetWIntrin128(), errValVect )
			);
	}

	inline void InvSqrt_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4InvSqrt( localInput.GetXIntrin128() ),
			Vec::V4InvSqrt( localInput.GetYIntrin128() )
			);
	}

	inline void InvSqrt_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4InvSqrt( localInput.GetXIntrin128() ),
			Vec::V4InvSqrt( localInput.GetYIntrin128() ),
			Vec::V4InvSqrt( localInput.GetZIntrin128() )
			);
	}

	inline void InvSqrt_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4InvSqrt( localInput.GetXIntrin128() ),
			Vec::V4InvSqrt( localInput.GetYIntrin128() ),
			Vec::V4InvSqrt( localInput.GetZIntrin128() ),
			Vec::V4InvSqrt( localInput.GetWIntrin128() )
			);
	}

	inline void InvSqrtSafe_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec)
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4InvSqrtSafe( localInput.GetXIntrin128(), errValVec ),
			Vec::V4InvSqrtSafe( localInput.GetYIntrin128(), errValVec )
			);
	}

	inline void InvSqrtSafe_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec)
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4InvSqrtSafe( localInput.GetXIntrin128(), errValVec ),
			Vec::V4InvSqrtSafe( localInput.GetYIntrin128(), errValVec ),
			Vec::V4InvSqrtSafe( localInput.GetZIntrin128(), errValVec )
			);
	}

	inline void InvSqrtSafe_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec)
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4InvSqrtSafe( localInput.GetXIntrin128(), errValVec ),
			Vec::V4InvSqrtSafe( localInput.GetYIntrin128(), errValVec ),
			Vec::V4InvSqrtSafe( localInput.GetZIntrin128(), errValVec ),
			Vec::V4InvSqrtSafe( localInput.GetWIntrin128(), errValVec )
			);
	}

	inline void InvSqrtFast_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4InvSqrtFast( localInput.GetXIntrin128() ),
			Vec::V4InvSqrtFast( localInput.GetYIntrin128() )
			);
	}

	inline void InvSqrtFast_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4InvSqrtFast( localInput.GetXIntrin128() ),
			Vec::V4InvSqrtFast( localInput.GetYIntrin128() ),
			Vec::V4InvSqrtFast( localInput.GetZIntrin128() )
			);
	}

	inline void InvSqrtFast_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4InvSqrtFast( localInput.GetXIntrin128() ),
			Vec::V4InvSqrtFast( localInput.GetYIntrin128() ),
			Vec::V4InvSqrtFast( localInput.GetZIntrin128() ),
			Vec::V4InvSqrtFast( localInput.GetWIntrin128() )
			);
	}

	inline void InvSqrtFastSafe_Imp(SoA_Vec2V_InOut inoutVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec)
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec2V(
			Vec::V4InvSqrtFastSafe( localInput.GetXIntrin128(), errValVec ),
			Vec::V4InvSqrtFastSafe( localInput.GetYIntrin128(), errValVec )
			);
	}

	inline void InvSqrtFastSafe_Imp(SoA_Vec3V_InOut inoutVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec)
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec3V(
			Vec::V4InvSqrtFastSafe( localInput.GetXIntrin128(), errValVec ),
			Vec::V4InvSqrtFastSafe( localInput.GetYIntrin128(), errValVec ),
			Vec::V4InvSqrtFastSafe( localInput.GetZIntrin128(), errValVec )
			);
	}

	inline void InvSqrtFastSafe_Imp(SoA_Vec4V_InOut inoutVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec)
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		inoutVect = SoA_Vec4V(
			Vec::V4InvSqrtFastSafe( localInput.GetXIntrin128(), errValVec ),
			Vec::V4InvSqrtFastSafe( localInput.GetYIntrin128(), errValVec ),
			Vec::V4InvSqrtFastSafe( localInput.GetZIntrin128(), errValVec ),
			Vec::V4InvSqrtFastSafe( localInput.GetWIntrin128(), errValVec )
			);
	}

	inline SoA_ScalarV_Out Dot_Imp_4_4( VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		Vec::Vector_4V xx = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInputA.GetWIntrin128(), localInputB.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		return SoA_ScalarV( xx_plus_yy_plus_zz_plus_ww );
	}

	inline SoA_ScalarV_Out Dot_Imp_3_3( VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		Vec::Vector_4V xx = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() );
		return SoA_ScalarV( xx_plus_yy_plus_zz );
	}

	inline SoA_ScalarV_Out Dot_Imp_2_2( VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		Vec::Vector_4V xx = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() );
		return SoA_ScalarV( xx_plus_yy );
	}

	inline void Scale_Imp_1_4( SoA_Vec4V_InOut inoutVec, Vec::Vector_4V_In a, VEC4V_SOA_DECL3(b) )
	{
		Vec::Vector_4V_In localInputA = a;
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);

		inoutVec = SoA_Vec4V(
			Vec::V4Scale( localInputA, localInputB.GetXIntrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetYIntrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetZIntrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetWIntrin128() )
			);
	}

	inline void Scale_Imp_1_3( SoA_Vec3V_InOut inoutVec, Vec::Vector_4V_In a, VEC3V_SOA_DECL3(b) )
	{
		Vec::Vector_4V_In localInputA = a;
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);

		inoutVec = SoA_Vec3V(
			Vec::V4Scale( localInputA, localInputB.GetXIntrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetYIntrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetZIntrin128() )
			);
	}

	inline void Scale_Imp_1_2( SoA_Vec2V_InOut inoutVec, Vec::Vector_4V_In a, VEC2V_SOA_DECL3(b) )
	{
		Vec::Vector_4V_In localInputA = a;
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);

		inoutVec = SoA_Vec2V(
			Vec::V4Scale( localInputA, localInputB.GetXIntrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetYIntrin128() )
			);
	}

	inline void AddInt_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec4V(
			Vec::V4AddInt( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4AddInt( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4AddInt( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4AddInt( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				);
	}

	inline void AddInt_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec3V(
			Vec::V4AddInt( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4AddInt( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4AddInt( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
				);
	}

	inline void AddInt_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec2V(
			Vec::V4AddInt( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4AddInt( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				);
	}

	inline void SubtractInt_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec4V(
			Vec::V4SubtractInt( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4SubtractInt( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4SubtractInt( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4SubtractInt( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				);
	}

	inline void SubtractInt_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec3V(
			Vec::V4SubtractInt( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4SubtractInt( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4SubtractInt( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
				);
	}

	inline void SubtractInt_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec2V(
			Vec::V4SubtractInt( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4SubtractInt( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				);
	}

	inline void AddScaled_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toAdd), VEC4V_SOA_DECL2(toScaleThenAdd), VEC4V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toAdd);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(toScaleThenAdd);
		SoA_Vec4V localInputC = VEC4V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec4V(
			Vec::V4AddScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4AddScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4AddScaled( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC.GetZIntrin128() ),
			Vec::V4AddScaled( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), localInputC.GetWIntrin128() )
				);
	}

	inline void AddScaled_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toAdd), VEC3V_SOA_DECL2(toScaleThenAdd), VEC3V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toAdd);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(toScaleThenAdd);
		SoA_Vec3V localInputC = VEC3V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec3V(
			Vec::V4AddScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4AddScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4AddScaled( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC.GetZIntrin128() )
				);
	}

	inline void AddScaled_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toAdd), VEC3V_SOA_DECL2(toScaleThenAdd), Vec::Vector_4V_In_After3Args scaleValue )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toAdd);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(toScaleThenAdd);
		inoutVec = SoA_Vec3V(
			Vec::V4AddScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), scaleValue ),
			Vec::V4AddScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), scaleValue ),
			Vec::V4AddScaled( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), scaleValue )
			);
	}

	inline void AddScaled_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toAdd), VEC2V_SOA_DECL2(toScaleThenAdd), VEC2V_SOA_DECL3(scaleValues) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toAdd);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(toScaleThenAdd);
		SoA_Vec2V localInputC = VEC2V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec2V(
			Vec::V4AddScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4AddScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() )
				);
	}

	inline void SubtractScaled_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toSubtractFrom), VEC4V_SOA_DECL2(toScaleThenSubtract), VEC4V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toSubtractFrom);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(toScaleThenSubtract);
		SoA_Vec4V localInputC = VEC4V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec4V(
			Vec::V4SubtractScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SubtractScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4SubtractScaled( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC.GetZIntrin128() ),
			Vec::V4SubtractScaled( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), localInputC.GetWIntrin128() )
				);
	}

	inline void SubtractScaled_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toSubtractFrom), VEC3V_SOA_DECL2(toScaleThenSubtract), VEC3V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toSubtractFrom);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(toScaleThenSubtract);
		SoA_Vec3V localInputC = VEC3V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec3V(
			Vec::V4SubtractScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SubtractScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4SubtractScaled( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC.GetZIntrin128() )
				);
	}

	inline void SubtractScaled_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toSubtractFrom), VEC2V_SOA_DECL2(toScaleThenSubtract), VEC2V_SOA_DECL3(scaleValues) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toSubtractFrom);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(toScaleThenSubtract);
		SoA_Vec2V localInputC = VEC2V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec2V(
			Vec::V4SubtractScaled( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SubtractScaled( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() )
				);
	}

	inline void Scale_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec4V(
			Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Scale_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec3V(
			Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Scale_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec2V(
			Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void InvScale_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScale( localInputA.GetXIntrin128(), scaleValue ),
			Vec::V4InvScale( localInputA.GetYIntrin128(), scaleValue ),
			Vec::V4InvScale( localInputA.GetZIntrin128(), scaleValue ),
			Vec::V4InvScale( localInputA.GetWIntrin128(), scaleValue )
			);
	}

	inline void InvScale_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4InvScale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4InvScale( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4InvScale( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void InvScaleSafe_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScaleSafe( localInputA.GetXIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetYIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetZIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetWIntrin128(), scaleValue, errValVect )
			);
	}

	inline void InvScaleSafe_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScaleSafe( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), errValVect )
			);
	}

	inline void InvScaleFast_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScaleFast( localInputA.GetXIntrin128(), scaleValue ),
			Vec::V4InvScaleFast( localInputA.GetYIntrin128(), scaleValue ),
			Vec::V4InvScaleFast( localInputA.GetZIntrin128(), scaleValue ),
			Vec::V4InvScaleFast( localInputA.GetWIntrin128(), scaleValue )
			);
	}

	inline void InvScaleFast_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScaleFast( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4InvScaleFast( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4InvScaleFast( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4InvScaleFast( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void InvScaleFastSafe_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScaleFastSafe( localInputA.GetXIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetYIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetZIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetWIntrin128(), scaleValue, errValVect )
			);
	}

	inline void InvScaleFastSafe_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(toScale);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec4V(
			Vec::V4InvScaleFastSafe( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), errValVect )
			);
	}

	inline void InvScale_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScale( localInputA.GetXIntrin128(), scaleValue ),
			Vec::V4InvScale( localInputA.GetYIntrin128(), scaleValue ),
			Vec::V4InvScale( localInputA.GetZIntrin128(), scaleValue )
			);
	}

	inline void InvScale_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4InvScale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4InvScale( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void InvScaleSafe_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScaleSafe( localInputA.GetXIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetYIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetZIntrin128(), scaleValue, errValVect )
			);
	}

	inline void InvScaleSafe_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScaleSafe( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), errValVect )
			);
	}

	inline void InvScaleFast_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL2(toScale), Vec::Vector_4V_In_After3Args scaleValue )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScaleFast( localInputA.GetXIntrin128(), scaleValue ),
			Vec::V4InvScaleFast( localInputA.GetYIntrin128(), scaleValue ),
			Vec::V4InvScaleFast( localInputA.GetZIntrin128(), scaleValue )
			);
	}

	inline void InvScaleFast_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScaleFast( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4InvScaleFast( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4InvScaleFast( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void InvScaleFastSafe_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScaleFastSafe( localInputA.GetXIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetYIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetZIntrin128(), scaleValue, errValVect )
			);
	}

	inline void InvScaleFastSafe_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(toScale);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec3V(
			Vec::V4InvScaleFastSafe( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), errValVect )
			);
	}

	inline void InvScale_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScale( localInputA.GetXIntrin128(), scaleValue ),
			Vec::V4InvScale( localInputA.GetYIntrin128(), scaleValue )
			);
	}

	inline void InvScale_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4InvScale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void InvScaleSafe_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue, Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScaleSafe( localInputA.GetXIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetYIntrin128(), scaleValue, errValVect )
			);
	}

	inline void InvScaleSafe_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScaleSafe( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), errValVect ),
			Vec::V4InvScaleSafe( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), errValVect )
			);
	}

	inline void InvScaleFast_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScaleFast( localInputA.GetXIntrin128(), scaleValue ),
			Vec::V4InvScaleFast( localInputA.GetYIntrin128(), scaleValue )
			);
	}

	inline void InvScaleFast_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScaleFast( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4InvScaleFast( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void InvScaleFastSafe_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue, Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScaleFastSafe( localInputA.GetXIntrin128(), scaleValue, errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetYIntrin128(), scaleValue, errValVect )
			);
	}

	inline void InvScaleFastSafe_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(toScale);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(scaleValues);
		inoutVec = SoA_Vec2V(
			Vec::V4InvScaleFastSafe( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), errValVect ),
			Vec::V4InvScaleFastSafe( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), errValVect )
			);
	}

	inline Vec::Vector_4V_Out Mag_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V sqrt_xx_plus_yy = Vec::V4Sqrt( xx_plus_yy );
		return sqrt_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out Mag_Imp( VEC3V_SOA_DECL(inVect) )
	{
#if 1 // Less instructions (best)
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz = Vec::V4Sqrt( xx_plus_yy_plus_zz );
		return sqrt_xx_plus_yy_plus_zz;
#else // Less dependencies 
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V zz = Vec::V4Scale( localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4Add( xx, yy );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4Add( xx_plus_yy, zz );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz = Vec::V4Sqrt( xx_plus_yy_plus_zz );
		return sqrt_xx_plus_yy_plus_zz;
#endif
	}

	inline Vec::Vector_4V_Out Mag_Imp( VEC4V_SOA_DECL(inVect) )
	{
#if 0 // Less dependencies
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V zz = Vec::V4Scale( localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V ww = Vec::V4Scale( localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4Add( xx, yy );
		Vec::Vector_4V zz_plus_ww = Vec::V4Add( zz, ww );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_yy, zz_plus_ww );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4Sqrt( xx_plus_yy_plus_zz_plus_ww );
		return SoA_ScalarV( sqrt_xx_plus_yy_plus_zz_plus_ww );
#elif 0 // Less instructions
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4AddScaled( xx_plus_yy_plus_zz, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4Sqrt( xx_plus_yy_plus_zz_plus_ww );
		return SoA_ScalarV( sqrt_xx_plus_yy_plus_zz_plus_ww );
#else // Hybrid (best)
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4Sqrt( xx_plus_yy_plus_zz_plus_ww );
		return sqrt_xx_plus_yy_plus_zz_plus_ww;
#endif
	}

	inline Vec::Vector_4V_Out MagFast_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V sqrt_xx_plus_yy = Vec::V4SqrtFast( xx_plus_yy );
		return sqrt_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out MagFast_Imp( VEC3V_SOA_DECL(inVect) )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz = Vec::V4SqrtFast( xx_plus_yy_plus_zz );
		return sqrt_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out MagFast_Imp( VEC4V_SOA_DECL(inVect) )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V sqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4SqrtFast( xx_plus_yy_plus_zz_plus_ww );
		return sqrt_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out MagSquared_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		return xx_plus_yy;
	}

	inline Vec::Vector_4V_Out MagSquared_Imp( VEC3V_SOA_DECL(inVect) )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		return xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out MagSquared_Imp( VEC4V_SOA_DECL(inVect) )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		return xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMag_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy = Vec::V4InvSqrt( xx_plus_yy );
		return invsqrt_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMag_Imp( VEC3V_SOA_DECL(inVect) )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz = Vec::V4InvSqrt( xx_plus_yy_plus_zz );
		return invsqrt_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMag_Imp( VEC4V_SOA_DECL(inVect) )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrt( xx_plus_yy_plus_zz_plus_ww );
		return invsqrt_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMagSafe_Imp( VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect )
	{
		using namespace Vec;
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V xx_plus_yy = V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V invsqrt_xx_plus_yy = V4InvSqrt( xx_plus_yy );
		return IF_EQ_THEN_ELSE( xx_plus_yy, V4VConstant(V_ZERO), errValVect, invsqrt_xx_plus_yy );
	}

	inline Vec::Vector_4V_Out InvMagSafe_Imp( VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect )
	{
		using namespace Vec;
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V xx_plus_yy = V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_yy_plus_zz = V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V invsqrt_xx_plus_yy_plus_zz = V4InvSqrt( xx_plus_yy_plus_zz );
		return IF_EQ_THEN_ELSE( xx_plus_yy_plus_zz, V4VConstant(V_ZERO), errValVect, invsqrt_xx_plus_yy_plus_zz );
	}

	inline Vec::Vector_4V_Out InvMagSafe_Imp( VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect )
	{
		using namespace Vec;
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V yy = V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_zz = V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V yy_plus_ww = V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vector_4V xx_plus_yy_plus_zz_plus_ww = V4Add( xx_plus_zz, yy_plus_ww );
		Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = V4InvSqrt( xx_plus_yy_plus_zz_plus_ww );
		return IF_EQ_THEN_ELSE( xx_plus_yy_plus_zz_plus_ww, V4VConstant(V_ZERO), errValVect, invsqrt_xx_plus_yy_plus_zz_plus_ww );
	}

	inline Vec::Vector_4V_Out InvMagFast_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy = Vec::V4InvSqrtFast( xx_plus_yy );
		return invsqrt_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMagFast_Imp( VEC3V_SOA_DECL(inVect) )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz = Vec::V4InvSqrtFast( xx_plus_yy_plus_zz );
		return invsqrt_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMagFast_Imp( VEC4V_SOA_DECL(inVect) )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrtFast( xx_plus_yy_plus_zz_plus_ww );
		return invsqrt_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMagFastSafe_Imp( VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy = Vec::V4InvSqrtFastSafe( xx_plus_yy, errValVect );
		return invsqrt_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMagFastSafe_Imp( VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz = Vec::V4InvSqrtFastSafe( xx_plus_yy_plus_zz, errValVect );
		return invsqrt_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMagFastSafe_Imp( VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrtFastSafe( xx_plus_yy_plus_zz_plus_ww, errValVect );
		return invsqrt_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMagSquared_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy = Vec::V4Invert( xx_plus_yy );
		return inv_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMagSquared_Imp( VEC3V_SOA_DECL(inVect) )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz = Vec::V4Invert( xx_plus_yy_plus_zz );
		return inv_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMagSquared_Imp( VEC4V_SOA_DECL(inVect) )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz_plus_ww = Vec::V4Invert( xx_plus_yy_plus_zz_plus_ww );
		return inv_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMagSquaredSafe_Imp( VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy = Vec::V4InvertSafe( xx_plus_yy, errValVec );
		return inv_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMagSquaredSafe_Imp( VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz = Vec::V4InvertSafe( xx_plus_yy_plus_zz, errValVec );
		return inv_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMagSquaredSafe_Imp( VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvertSafe( xx_plus_yy_plus_zz_plus_ww, errValVec );
		return inv_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMagSquaredFast_Imp( VEC2V_SOA_DECL(inVect) )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy = Vec::V4InvertFast( xx_plus_yy );
		return inv_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMagSquaredFast_Imp( VEC3V_SOA_DECL(inVect) )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz = Vec::V4InvertFast( xx_plus_yy_plus_zz );
		return inv_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMagSquaredFast_Imp( VEC4V_SOA_DECL(inVect) )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvertFast( xx_plus_yy_plus_zz_plus_ww );
		return inv_xx_plus_yy_plus_zz_plus_ww;
	}

	inline Vec::Vector_4V_Out InvMagSquaredFastSafe_Imp( VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec )
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy = Vec::V4InvertFastSafe( xx_plus_yy, errValVec );
		return inv_xx_plus_yy;
	}

	inline Vec::Vector_4V_Out InvMagSquaredFastSafe_Imp( VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec )
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz = Vec::V4InvertFastSafe( xx_plus_yy_plus_zz, errValVec );
		return inv_xx_plus_yy_plus_zz;
	}

	inline Vec::Vector_4V_Out InvMagSquaredFastSafe_Imp( VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec )
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V inv_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvertFastSafe( xx_plus_yy_plus_zz_plus_ww, errValVec );
		return inv_xx_plus_yy_plus_zz_plus_ww;
	}

	inline void Normalize_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy = Vec::V4InvSqrt( xx_plus_yy );
		inoutVec = SoA_Vec2V(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy )
			);
	}

	inline void Normalize_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz = Vec::V4InvSqrt( xx_plus_yy_plus_zz );
		inoutVec = SoA_Vec3V(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz ),
			Vec::V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz )
			);
	}

	inline void Normalize_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrt( xx_plus_yy_plus_zz_plus_ww );
		inoutVec = SoA_Vec4V(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww )
			);
	}

	inline void NormalizeSafe_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect)
	{
		using namespace Vec;
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V xx_plus_yy = V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V invsqrt_xx_plus_yy = V4InvSqrt( xx_plus_yy );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy, V4VConstant(V_ZERO) );
		inoutVec = SoA_Vec2V(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy ), errValVect )
			);
	}

	inline void NormalizeSafe_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		using namespace Vec;
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V xx_plus_yy = V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_yy_plus_zz = V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V invsqrt_xx_plus_yy_plus_zz = V4InvSqrt( xx_plus_yy_plus_zz );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy_plus_zz, V4VConstant(V_ZERO) );
		inoutVec = SoA_Vec3V(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz ), errValVect )
			);
	}

	inline void NormalizeSafe_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		using namespace Vec;
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V yy = V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_zz = V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V yy_plus_ww = V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vector_4V xx_plus_yy_plus_zz_plus_ww = V4Add( xx_plus_zz, yy_plus_ww );
		Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = V4InvSqrt( xx_plus_yy_plus_zz_plus_ww );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy_plus_zz_plus_ww, V4VConstant(V_ZERO) );
		inoutVec = SoA_Vec4V(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect )
			);
	}

	inline void NormalizeFast_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy = Vec::V4InvSqrtFast( xx_plus_yy );
		inoutVec = SoA_Vec2V(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy )
			);
	}

	inline void NormalizeFast_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V xx_plus_yy = Vec::V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz = Vec::V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz = Vec::V4InvSqrtFast( xx_plus_yy_plus_zz );
		inoutVec = SoA_Vec3V(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz ),
			Vec::V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz )
			);
	}

	inline void NormalizeFast_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrtFast( xx_plus_yy_plus_zz_plus_ww );
		inoutVec = SoA_Vec4V(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww )
			);
	}

	inline void NormalizeFastSafe_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect)
	{
		using namespace Vec;
		SoA_Vec2V localInput = VEC2V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V xx_plus_yy = V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V invsqrt_xx_plus_yy = V4InvSqrtFast( xx_plus_yy );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy, V4VConstant(V_ZERO) );
		inoutVec = SoA_Vec2V(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy ), errValVect )
			);
	}

	inline void NormalizeFastSafe_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		using namespace Vec;
		SoA_Vec3V localInput = VEC3V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V xx_plus_yy = V4AddScaled( xx, localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_yy_plus_zz = V4AddScaled( xx_plus_yy, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V invsqrt_xx_plus_yy_plus_zz = V4InvSqrtFast( xx_plus_yy_plus_zz );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy_plus_zz, V4VConstant(V_ZERO) );
		inoutVec = SoA_Vec3V(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz ), errValVect )
			);
	}

	inline void NormalizeFastSafe_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		using namespace Vec;
		SoA_Vec4V localInput = VEC4V_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V yy = V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_zz = V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V yy_plus_ww = V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vector_4V xx_plus_yy_plus_zz_plus_ww = V4Add( xx_plus_zz, yy_plus_ww );
		Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = V4InvSqrtFast( xx_plus_yy_plus_zz_plus_ww );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy_plus_zz_plus_ww, V4VConstant(V_ZERO) );
		inoutVec = SoA_Vec4V(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect )
			);
	}

	inline void IsEqualIntV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool2V(
			Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsEqualIntAll_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4And(
				Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
				Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntNone_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		return
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				)
			);
	}

	inline void IsEqualIntV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsEqualIntAll_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4And(
				Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
				Vec::V4And(
					Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
					Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntNone_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4Or(
						Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
					)
				)
			);
	}

	inline void IsEqualIntV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool4V(
			Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4IsEqualIntV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsEqualIntAll_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4And(
					Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
					Vec::V4IsEqualIntV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntNone_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
					),
					Vec::V4Or(
						Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsZeroAll_Imp(VEC4V_SOA_DECL(inVector))
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), _zero ),
					Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), _zero )
				),
				Vec::V4And(
					Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), _zero ),
					Vec::V4IsEqualIntV( localInputA.GetWIntrin128(), _zero )
				)
			);
	}

	inline Vec::Vector_4V_Out IsZeroAll_Imp(VEC3V_SOA_DECL(inVector))
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector);
		return 
			Vec::V4And(
				Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), _zero ),
				Vec::V4And(
					Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), _zero ),
					Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), _zero )
				)
			);
	}

	inline Vec::Vector_4V_Out IsZeroAll_Imp(VEC2V_SOA_DECL(inVector))
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector);
		return 
			Vec::V4And(
				Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), _zero ),
				Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), _zero )
			);
	}

	inline Vec::Vector_4V_Out IsZeroNone_Imp(VEC4V_SOA_DECL(inVector))
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), _zero ),
						Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), _zero )
					),
					Vec::V4Or(
						Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), _zero ),
						Vec::V4IsEqualIntV( localInputA.GetWIntrin128(), _zero )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsZeroNone_Imp(VEC3V_SOA_DECL(inVector))
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), _zero ),
					Vec::V4Or(
						Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), _zero ),
						Vec::V4IsEqualIntV( localInputA.GetZIntrin128(), _zero )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsZeroNone_Imp(VEC2V_SOA_DECL(inVector))
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4IsEqualIntV( localInputA.GetXIntrin128(), _zero ),
					Vec::V4IsEqualIntV( localInputA.GetYIntrin128(), _zero )
				)
			);
	}

	inline void IsEqualV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool4V(
			Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4IsEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
		);
	}

	inline void IsEqualV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
		);
	}

	inline void IsEqualV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool2V(
			Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsEqualAll_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4And(
					Vec::V4IsEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
					Vec::V4IsEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualAll_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4IsEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out IsEqualAll_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4And(
				Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
				Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out IsEqualNone_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
						Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
					),
					Vec::V4Or(
						Vec::V4IsEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
						Vec::V4IsEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualNone_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
						Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
					),
					Vec::V4IsEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualNone_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4IsEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				)
			);
	}

	inline void IsCloseV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		inoutVec = SoA_VecBool4V(
			Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
			Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC ),
			Vec::V4IsCloseV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC ),
			Vec::V4IsCloseV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), localInputC )
		);
	}

	inline void IsCloseV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		inoutVec = SoA_VecBool3V(
			Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
			Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC ),
			Vec::V4IsCloseV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC )
		);
	}

	inline void IsCloseV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		inoutVec = SoA_VecBool2V(
			Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
			Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
		);
	}

	inline Vec::Vector_4V_Out IsCloseAll_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
					Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
				),
				Vec::V4And(
					Vec::V4IsCloseV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC ),
					Vec::V4IsCloseV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), localInputC )
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseAll_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
					Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
				),
				Vec::V4IsCloseV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC )
			);
	}

	inline Vec::Vector_4V_Out IsCloseAll_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		return 
			Vec::V4And(
				Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
				Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
			);
	}

	inline Vec::Vector_4V_Out IsCloseNone_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
					),
					Vec::V4Or(
						Vec::V4IsCloseV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), localInputC )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseNone_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
					),
					Vec::V4IsCloseV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC )
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseNone_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		Vec::Vector_4V localInputC = eps;
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4IsCloseV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC ),
					Vec::V4IsCloseV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC )
				)
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanAll_Imp(VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(bigVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(smallVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsGreaterThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsGreaterThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4And(
					Vec::V4IsGreaterThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
					Vec::V4IsGreaterThanV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanAll_Imp(VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(bigVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(smallVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsGreaterThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsGreaterThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4IsGreaterThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanAll_Imp(VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(bigVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(smallVector);
		return 
			Vec::V4And(
				Vec::V4IsGreaterThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
				Vec::V4IsGreaterThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void IsGreaterThanV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(bigVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool4V(
			Vec::V4IsGreaterThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsGreaterThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4IsGreaterThanV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
		);
	}

	inline void IsGreaterThanV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(bigVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsGreaterThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsGreaterThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
		);
	}

	inline void IsGreaterThanV_Imp(SoA_VecBool3V_InOut inoutVec, Vec::Vector_4V_In_After3Args bigVector, VEC3V_SOA_DECL(smallVector))
	{
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsGreaterThanV( bigVector, localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanV( bigVector, localInputB.GetYIntrin128() ),
			Vec::V4IsGreaterThanV( bigVector, localInputB.GetZIntrin128() )
			);
	}

	inline void IsGreaterThanV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(bigVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool2V(
			Vec::V4IsGreaterThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp(VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(bigVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(smallVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4And(
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp(VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(bigVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(smallVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4IsGreaterThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp(VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(bigVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(smallVector);
		return 
			Vec::V4And(
				Vec::V4IsGreaterThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
				Vec::V4IsGreaterThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void IsGreaterThanOrEqualV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(bigVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool4V(
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
		);
	}

	inline void IsGreaterThanOrEqualV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(bigVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
		);
	}

	inline void IsGreaterThanOrEqualV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(bigVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(smallVector);
		inoutVec = SoA_VecBool2V(
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsGreaterThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsLessThanAll_Imp(VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(smallVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(bigVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsLessThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsLessThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4And(
					Vec::V4IsLessThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
					Vec::V4IsLessThanV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanAll_Imp(VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(smallVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(bigVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsLessThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsLessThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4IsLessThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out IsLessThanAll_Imp(VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(smallVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(bigVector);
		return 
			Vec::V4And(
				Vec::V4IsLessThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
				Vec::V4IsLessThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void IsLessThanV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(smallVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(bigVector);
		inoutVec = SoA_VecBool4V(
			Vec::V4IsLessThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsLessThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsLessThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4IsLessThanV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
		);
	}

	inline void IsLessThanV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(smallVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(bigVector);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsLessThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsLessThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsLessThanV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
		);
	}

	inline void IsLessThanV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(smallVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(bigVector);
		inoutVec = SoA_VecBool2V(
			Vec::V4IsLessThanV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsLessThanV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp(VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(smallVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(bigVector);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4IsLessThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
					Vec::V4IsLessThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
				),
				Vec::V4And(
					Vec::V4IsLessThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
					Vec::V4IsLessThanOrEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
				)
			);
	}

	inline void IsLessThanOrEqualV_Imp(SoA_VecBool4V_InOut inoutVec, VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(smallVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(bigVector);
		inoutVec = SoA_VecBool4V(
			Vec::V4IsLessThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsLessThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsLessThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4IsLessThanOrEqualV( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
		);
	}

	inline void IsLessThanOrEqualV_Imp(SoA_VecBool3V_InOut inoutVec, VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(smallVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(bigVector);
		inoutVec = SoA_VecBool3V(
			Vec::V4IsLessThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsLessThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4IsLessThanOrEqualV( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
		);
	}

	inline void IsLessThanOrEqualV_Imp(SoA_VecBool2V_InOut inoutVec, VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(smallVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(bigVector);
		inoutVec = SoA_VecBool2V(
			Vec::V4IsLessThanOrEqualV( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4IsLessThanOrEqualV( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
		);
	}

	inline Vec::Vector_4V_Out IsEqualAll_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(inMat1);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualAll_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(inMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
						Vec::V4IsEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
						Vec::V4IsEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualAll_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat1);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
						)
					),
					Vec::V4IsEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
						Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
					),
					Vec::V4And(
						Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
						Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualNone_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(inMat1);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
							)
						)
					),
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
							)
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualNone_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(inMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4IsEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						)
					),
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualNone_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat1);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
								Vec::V4IsEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
							)
						),
						Vec::V4IsEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					),
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4IsEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
						),
						Vec::V4Or(
							Vec::V4IsEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
							Vec::V4IsEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntNone_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(inMat1);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
							)
						)
					),
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
							)
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntNone_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(inMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4IsEqualIntV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						)
					),
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
							)
						),
						Vec::V4Or(
							Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntNone_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat1);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4InvertBits(
				Vec::V4Or(
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
							),
							Vec::V4Or(
								Vec::V4IsEqualIntV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
								Vec::V4IsEqualIntV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
							)
						),
						Vec::V4IsEqualIntV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					),
					Vec::V4Or(
						Vec::V4Or(
							Vec::V4IsEqualIntV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
						),
						Vec::V4Or(
							Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntAll_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(inMat1);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntAll_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(inMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsEqualIntV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsEqualIntAll_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat1);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(inMat2);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsEqualIntV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsEqualIntV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
						)
					),
					Vec::V4IsEqualIntV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsEqualIntV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
					),
					Vec::V4And(
						Vec::V4IsEqualIntV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
						Vec::V4IsEqualIntV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp_44_44(MAT44V_SOA_DECL(bigMat), MAT44V_SOA_DECL2(smallMat))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(bigMat);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(smallMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp_34_34(MAT34V_SOA_DECL(bigMat), MAT34V_SOA_DECL2(smallMat))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(bigMat);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(smallMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp_33_33(MAT33V_SOA_DECL(bigMat), MAT33V_SOA_DECL2(smallMat))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(bigMat);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(smallMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsGreaterThanOrEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
						)
					),
					Vec::V4IsGreaterThanOrEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
					),
					Vec::V4And(
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
						Vec::V4IsGreaterThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanAll_Imp_44_44(MAT44V_SOA_DECL(smallMat), MAT44V_SOA_DECL2(bigMat))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(smallMat);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(bigMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanAll_Imp_34_34(MAT34V_SOA_DECL(smallMat), MAT34V_SOA_DECL2(bigMat))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(smallMat);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(bigMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsLessThanV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
						Vec::V4IsLessThanV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsLessThanV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
						Vec::V4IsLessThanV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanAll_Imp_33_33(MAT33V_SOA_DECL(smallMat), MAT33V_SOA_DECL2(bigMat))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(smallMat);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(bigMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsLessThanV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
						)
					),
					Vec::V4IsLessThanV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsLessThanV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
						Vec::V4IsLessThanV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
					),
					Vec::V4And(
						Vec::V4IsLessThanV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
						Vec::V4IsLessThanV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp_44_44(MAT44V_SOA_DECL(smallMat), MAT44V_SOA_DECL2(bigMat))
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(smallMat);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(bigMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp_34_34(MAT34V_SOA_DECL(smallMat), MAT34V_SOA_DECL2(bigMat))
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(smallMat);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(bigMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsLessThanOrEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
						Vec::V4IsLessThanOrEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() )
						)
					),
					Vec::V4And(
						Vec::V4IsLessThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
						Vec::V4IsLessThanOrEqualV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp_33_33(MAT33V_SOA_DECL(smallMat), MAT33V_SOA_DECL2(bigMat))
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(smallMat);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(bigMat);
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() )
						),
						Vec::V4And(
							Vec::V4IsLessThanOrEqualV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
							Vec::V4IsLessThanOrEqualV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() )
						)
					),
					Vec::V4IsLessThanOrEqualV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsLessThanOrEqualV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
						Vec::V4IsLessThanOrEqualV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() )
					),
					Vec::V4And(
						Vec::V4IsLessThanOrEqualV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
						Vec::V4IsLessThanOrEqualV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseAll_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues)
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(inMat1);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(inMat2);
		Vec::Vector_4V localInputC = epsValues;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128(), localInputC )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128(), localInputC )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseAll_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues)
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(inMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(inMat2);
		Vec::Vector_4V localInputC = epsValues;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4IsCloseV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128(), localInputC )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4IsCloseV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128(), localInputC )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseAll_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues)
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat1);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(inMat2);
		Vec::Vector_4V localInputC = epsValues;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsCloseV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128(), localInputC ),
							Vec::V4IsCloseV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128(), localInputC )
						)
					),
					Vec::V4IsCloseV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128(), localInputC )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsCloseV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128(), localInputC )
					),
					Vec::V4And(
						Vec::V4IsCloseV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128(), localInputC ),
						Vec::V4IsCloseV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128(), localInputC )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseNone_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues)
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(inMat1);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(inMat2);
		Vec::Vector_4V localInputC = epsValues;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128(), localInputC )
						)
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM20Intrin128(), localInputB.GetM30Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM21Intrin128(), localInputB.GetM31Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM22Intrin128(), localInputB.GetM32Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128(), localInputC )
						)
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseNone_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues)
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(inMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(inMat2);
		Vec::Vector_4V localInputC = epsValues;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4IsNotCloseV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128(), localInputC ),
						Vec::V4IsNotCloseV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128(), localInputC )
					)
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128(), localInputC )
						)
					),
					Vec::V4And(
						Vec::V4IsNotCloseV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128(), localInputC ),
						Vec::V4IsNotCloseV( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128(), localInputC )
					)
				)
			);
	}

	inline Vec::Vector_4V_Out IsCloseNone_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues)
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat1);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(inMat2);
		Vec::Vector_4V localInputC = epsValues;
		return 
			Vec::V4And(
				Vec::V4And(
					Vec::V4And(
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128(), localInputC )
						),
						Vec::V4And(
							Vec::V4IsNotCloseV( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128(), localInputC ),
							Vec::V4IsNotCloseV( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128(), localInputC )
						)
					),
					Vec::V4IsNotCloseV( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128(), localInputC )
				),
				Vec::V4And(
					Vec::V4And(
						Vec::V4IsNotCloseV( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128(), localInputC ),
						Vec::V4IsNotCloseV( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128(), localInputC )
					),
					Vec::V4And(
						Vec::V4IsNotCloseV( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128(), localInputC ),
						Vec::V4IsNotCloseV( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128(), localInputC )
					)
				)
			);
	}

	template <int exponent>
	inline void FloatToIntRaw_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVec))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec4V(
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetXIntrin128() ),
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetYIntrin128() ),
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetZIntrin128() ),
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetWIntrin128() )
				);
	}

	template <int exponent>
	inline void IntToFloatRaw_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVec))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec4V(
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetXIntrin128() ),
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetYIntrin128() ),
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetZIntrin128() ),
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetWIntrin128() )
				);
	}

	inline void RoundToNearestInt_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVec))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec4V(
			Vec::V4RoundToNearestInt( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestInt( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestInt( localInputA.GetZIntrin128() ),
			Vec::V4RoundToNearestInt( localInputA.GetWIntrin128() )
				);
	}

	inline void RoundToNearestIntZero_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVec))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec4V(
			Vec::V4RoundToNearestIntZero( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntZero( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestIntZero( localInputA.GetZIntrin128() ),
			Vec::V4RoundToNearestIntZero( localInputA.GetWIntrin128() )
				);
	}

	inline void RoundToNearestIntNegInf_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVec))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec4V(
			Vec::V4RoundToNearestIntNegInf( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntNegInf( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestIntNegInf( localInputA.GetZIntrin128() ),
			Vec::V4RoundToNearestIntNegInf( localInputA.GetWIntrin128() )
				);
	}

	inline void RoundToNearestIntPosInf_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVec))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec4V(
			Vec::V4RoundToNearestIntPosInf( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntPosInf( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestIntPosInf( localInputA.GetZIntrin128() ),
			Vec::V4RoundToNearestIntPosInf( localInputA.GetWIntrin128() )
				);
	}

	template <int exponent>
	inline void FloatToIntRaw_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVec))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec3V(
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetXIntrin128() ),
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetYIntrin128() ),
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetZIntrin128() )
				);
	}

	template <int exponent>
	inline void IntToFloatRaw_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVec))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec3V(
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetXIntrin128() ),
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetYIntrin128() ),
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetZIntrin128() )
				);
	}

	inline void RoundToNearestInt_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVec))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec3V(
			Vec::V4RoundToNearestInt( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestInt( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestInt( localInputA.GetZIntrin128() )
				);
	}

	inline void RoundToNearestIntZero_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVec))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec3V(
			Vec::V4RoundToNearestIntZero( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntZero( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestIntZero( localInputA.GetZIntrin128() )
				);
	}

	inline void RoundToNearestIntNegInf_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVec))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec3V(
			Vec::V4RoundToNearestIntNegInf( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntNegInf( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestIntNegInf( localInputA.GetZIntrin128() )
				);
	}

	inline void RoundToNearestIntPosInf_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVec))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec3V(
			Vec::V4RoundToNearestIntPosInf( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntPosInf( localInputA.GetYIntrin128() ),
			Vec::V4RoundToNearestIntPosInf( localInputA.GetZIntrin128() )
				);
	}

	template <int exponent>
	inline void FloatToIntRaw_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVec))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec2V(
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetXIntrin128() ),
			Vec::V4FloatToIntRaw<exponent>( localInputA.GetYIntrin128() )
				);
	}

	template <int exponent>
	inline void IntToFloatRaw_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVec))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec2V(
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetXIntrin128() ),
			Vec::V4IntToFloatRaw<exponent>( localInputA.GetYIntrin128() )
				);
	}

	inline void RoundToNearestInt_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVec))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec2V(
			Vec::V4RoundToNearestInt( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestInt( localInputA.GetYIntrin128() )
				);
	}

	inline void RoundToNearestIntZero_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVec))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec2V(
			Vec::V4RoundToNearestIntZero( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntZero( localInputA.GetYIntrin128() )
				);
	}

	inline void RoundToNearestIntNegInf_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVec))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec2V(
			Vec::V4RoundToNearestIntNegInf( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntNegInf( localInputA.GetYIntrin128() )
				);
	}

	inline void RoundToNearestIntPosInf_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVec))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVec);
		inoutVec = SoA_Vec2V(
			Vec::V4RoundToNearestIntPosInf( localInputA.GetXIntrin128() ),
			Vec::V4RoundToNearestIntPosInf( localInputA.GetYIntrin128() )
			);
	}











































	inline void Conjugate_Imp_Q(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inQuat))
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat);
		Vec::Vector_4V invMask = Vec::V4VConstant(V_80000000);
		inoutQuat = SoA_QuatV(
			Vec::V4Xor( localInputA.GetXIntrin128(), invMask ),
			Vec::V4Xor( localInputA.GetYIntrin128(), invMask ),
			Vec::V4Xor( localInputA.GetZIntrin128(), invMask ),
			localInputA.GetWIntrin128()
			);
	}

	inline void Normalize_Imp(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inVect))
	{
		SoA_QuatV localInput = QUATV_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrt( xx_plus_yy_plus_zz_plus_ww );
		inoutQuat = SoA_QuatV(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww )
			);
	}

	inline void NormalizeSafe_Imp(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		using namespace Vec;
		SoA_QuatV localInput = QUATV_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V yy = V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_zz = V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V yy_plus_ww = V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vector_4V xx_plus_yy_plus_zz_plus_ww = V4Add( xx_plus_zz, yy_plus_ww );
		Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = V4InvSqrt( xx_plus_yy_plus_zz_plus_ww );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy_plus_zz_plus_ww, V4VConstant(V_ZERO) );
		inoutQuat = SoA_QuatV(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect )
			);
	}

	inline void NormalizeFast_Imp(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inVect))
	{
		SoA_QuatV localInput = QUATV_SOA_ARG_GET(inVect);
		Vec::Vector_4V xx = Vec::V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vec::Vector_4V xx_plus_yy_plus_zz_plus_ww = Vec::V4Add( xx_plus_zz, yy_plus_ww );
		Vec::Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = Vec::V4InvSqrtFast( xx_plus_yy_plus_zz_plus_ww );
		inoutQuat = SoA_QuatV(
			Vec::V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ),
			Vec::V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww )
			);
	}

	inline void NormalizeFastSafe_Imp(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		using namespace Vec;
		SoA_QuatV localInput = QUATV_SOA_ARG_GET(inVect);
		Vector_4V xx = V4Scale( localInput.GetXIntrin128(), localInput.GetXIntrin128() );
		Vector_4V yy = V4Scale( localInput.GetYIntrin128(), localInput.GetYIntrin128() );
		Vector_4V xx_plus_zz = V4AddScaled( xx, localInput.GetZIntrin128(), localInput.GetZIntrin128() );
		Vector_4V yy_plus_ww = V4AddScaled( yy, localInput.GetWIntrin128(), localInput.GetWIntrin128() );
		Vector_4V xx_plus_yy_plus_zz_plus_ww = V4Add( xx_plus_zz, yy_plus_ww );
		Vector_4V invsqrt_xx_plus_yy_plus_zz_plus_ww = V4InvSqrtFast( xx_plus_yy_plus_zz_plus_ww );
		Vector_4V isZeroInput = V4IsEqualV( xx_plus_yy_plus_zz_plus_ww, V4VConstant(V_ZERO) );
		inoutQuat = SoA_QuatV(
			V4SelectFT( isZeroInput, V4Scale( localInput.GetXIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetYIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetZIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect ),
			V4SelectFT( isZeroInput, V4Scale( localInput.GetWIntrin128(), invsqrt_xx_plus_yy_plus_zz_plus_ww ), errValVect )
			);
	}

	inline void Invert_Imp_Q(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inQuat))
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat);
		SoA_Vec4V v_localInputA;
		SoA_Vec4V numerator;
		SoA_QuatV conj;
		SoA_ScalarV denominator;
		SoA_Vec4V result;

		Cast( v_localInputA, localInputA );
		Conjugate( conj, localInputA );
		denominator = MagSquared( v_localInputA );
		Cast( numerator, conj );
		InvScale( result, numerator, denominator );
		Cast( inoutQuat, result );
	}

	inline void InvertSafe_Imp_Q(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inQuat), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat);
		SoA_Vec4V v_localInputA;
		SoA_Vec4V numerator;
		SoA_QuatV conj;
		SoA_ScalarV denominator;
		SoA_Vec4V result;

		Cast( v_localInputA, localInputA );
		Conjugate( conj, localInputA );
		denominator = MagSquared( v_localInputA );
		Cast( numerator, conj );
		InvScaleSafe( result, numerator, denominator, SoA_ScalarV(errValVect) );
		Cast( inoutQuat, result );
	}

	inline void InvertFast_Imp_Q(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inQuat))
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat);
		SoA_Vec4V v_localInputA;
		SoA_Vec4V numerator;
		SoA_QuatV conj;
		SoA_ScalarV denominator;
		SoA_Vec4V result;

		Cast( v_localInputA, localInputA );
		Conjugate( conj, localInputA );
		denominator = MagSquared( v_localInputA );
		Cast( numerator, conj );
		InvScaleFast( result, numerator, denominator );
		Cast( inoutQuat, result );
	}

	inline void InvertFastSafe_Imp_Q(SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inQuat), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat);
		SoA_Vec4V v_localInputA;
		SoA_Vec4V numerator;
		SoA_QuatV conj;
		SoA_ScalarV denominator;
		SoA_Vec4V result;

		Cast( v_localInputA, localInputA );
		Conjugate( conj, localInputA );
		denominator = MagSquared( v_localInputA );
		Cast( numerator, conj );
		InvScaleFastSafe( result, numerator, denominator, SoA_ScalarV(errValVect) );
		Cast( inoutQuat, result );
	}

	inline void Multiply_Imp_Q( SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL(inQuat1), QUATV_SOA_DECL2(inQuat2) )
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat1);
		SoA_QuatV localInputB = QUATV_SOA_ARG_GET(inQuat2);
		
		// Scalar version
		//outVect.f.x = inQuat1.f.w*inQuat2.f.x + inQuat2.f.w*inQuat1.f.x + inQuat1.f.y*inQuat2.f.z - inQuat1.f.z*inQuat2.f.y; // Wx_wX_yZ_Zy
		//outVect.f.y = inQuat1.f.w*inQuat2.f.y + inQuat2.f.w*inQuat1.f.y + inQuat1.f.z*inQuat2.f.x - inQuat1.f.x*inQuat2.f.z; // Wy_wY_Zx_Xz
		//outVect.f.z = inQuat1.f.w*inQuat2.f.z + inQuat2.f.w*inQuat1.f.z + inQuat1.f.x*inQuat2.f.y - inQuat1.f.y*inQuat2.f.x; // Wz_wZ_xY_Yx
		//outVect.f.w = inQuat1.f.w*inQuat2.f.w - inQuat1.f.x*inQuat2.f.x - inQuat1.f.y*inQuat2.f.y - inQuat1.f.z*inQuat2.f.z; // Ww_Xx_Yy_Zz

		Vec::Vector_4V Wx = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V Wx_wX = Vec::V4AddScaled( Wx, localInputB.GetWIntrin128(), localInputA.GetXIntrin128() );
		Vec::Vector_4V Wx_wX_yZ = Vec::V4AddScaled( Wx_wX, localInputA.GetYIntrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V Wx_wX_yZ_Zy = Vec::V4SubtractScaled( Wx_wX_yZ, localInputA.GetZIntrin128(), localInputB.GetYIntrin128() );

		Vec::Vector_4V Wy = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V Wy_wY = Vec::V4AddScaled( Wy, localInputB.GetWIntrin128(), localInputA.GetYIntrin128() );
		Vec::Vector_4V Wy_wY_Zx = Vec::V4AddScaled( Wy_wY, localInputA.GetZIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V Wy_wY_Zx_Xz = Vec::V4SubtractScaled( Wy_wY_Zx, localInputA.GetXIntrin128(), localInputB.GetZIntrin128() );

		Vec::Vector_4V Wz = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V Wz_wZ = Vec::V4AddScaled( Wz, localInputB.GetWIntrin128(), localInputA.GetZIntrin128() );
		Vec::Vector_4V Wz_wZ_xY = Vec::V4AddScaled( Wz_wZ, localInputA.GetXIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V Wz_wZ_xY_Yx = Vec::V4SubtractScaled( Wz_wZ_xY, localInputA.GetYIntrin128(), localInputB.GetXIntrin128() );

		Vec::Vector_4V Ww = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() );
		Vec::Vector_4V Ww_Xx = Vec::V4SubtractScaled( Ww, localInputA.GetXIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V Ww_Xx_Yy = Vec::V4SubtractScaled( Ww_Xx, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V Ww_Xx_Yy_Zz = Vec::V4SubtractScaled( Ww_Xx_Yy, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() );

		inoutQuat = SoA_QuatV( Wx_wX_yZ_Zy, Wy_wY_Zx_Xz, Wz_wZ_xY_Yx, Ww_Xx_Yy_Zz );
	}

	// TODO: Implement when needed.
//	inline void Slerp_Imp_Q( SoA_QuatV_InOut inoutQuat, Vec::Vector_4V_In t, QUATV_SOA_DECL(inQuat1), QUATV_SOA_DECL(inQuat2) )
//	{
//		//              inNormQuat1*sin((1-t)(theta)) + inNormQuat2*sin((t)(theta))
//		// quatResult = -----------------------------------------------------------
//		//                                       sin(theta)
//
//		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat1);
//		SoA_QuatV localInputB = QUATV_SOA_ARG_GET(inQuat2);
//		Vec::Vector_4V localInputC = t;
//
//		Vector_4V quatResult;
//		Vector_4V negT;
//		Vector_4V sinArg; // <(1-t)(theta), (t)(theta), ..., ...>
//		Vector_4V theSin;
//		Vector_4V theSin1;
//		Vector_4V theSin2;
//
//		Vector_4V theta;
//		Vector_4V sinTheta;
//		SoA_ScalarV cosTheta;
//		SoA_VecBool4V cosThetaLTZero;
//
//		Vector_4V _signMask = V4VConstant(V_80000000);
//		Vector_4V _zero = V4VConstant(V_ZERO);
//		Vector_4V _one = V4VConstant(V_ONE);
//
//		SoA_Vec4V v_zero( _zero );
//		SoA_Vec4V v_signMask( _signMask );
//		SoA_Vec4V v_one( _one );
//
//
//
//#define NEGATE( V ) Xor( (V), v_signMask )
//		// Find cos(theta) the quick way.
//		cosTheta = Dot( localInputA, localInputB );
//
//		// Correct for a long rotation.
//		cosThetaLTZero = IsLessThan( cosTheta, v_zero );
//		cosTheta = SelectFT( cosThetaLTZero, cosTheta, NEGATE(cosTheta) );
//
//		// Compute sin(theta) from cos(theta). (sin = sqrt(1-cos^2)).
//		sinTheta = Sqrt( SubtractScaled( v_one, cosTheta, cosTheta ) );
//
//		// Compute theta so that we can compute sin((1-t)(theta)) and sin((t)(theta)).
//		// [ arctan( sin(theta)/cos(theta) ) = theta ]
//		theta = V4Arctan2( sinTheta, cosTheta );
//
//		// Compute the two sines at once.
//		negT = NEGATE( t );
//		sinArg = Add( negT, v_one );
//		// LEFT OFF HERE!!
//		sinArg = V4MergeXY( sinArg, t );
//		sinArg = V4Scale( sinArg, theta );
//		theSin = V4Sin( sinArg );
//		theSin = V4InvScale( theSin, sinTheta ); // include the denominator
//
//		// Extract the components.
//		theSin1 = V4SplatX( theSin );
//		theSin2 = V4SplatY( theSin );
//
//		// InvertFull this sign if need be. (TODO: Not sure if this is necessary?)
//		theSin2 = V4SelectFT( cosThetaLTZero, theSin2, NEGATE(theSin2) );
//
//		// Now just scale by the input quats and add.
//		quatResult = V4Scale( inNormQuat1, theSin1 );
//		quatResult = V4AddScaled( quatResult, inNormQuat2, theSin2 );
//		return quatResult;
//#undef NEGATE
//
//	}

	inline void PrepareSlerp_Imp_Q( SoA_QuatV_InOut outQuat, QUATV_SOA_DECL2(inQuat1), QUATV_SOA_DECL2(inQuat2) )
	{
		// dot
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat1);
		SoA_QuatV localInputB = QUATV_SOA_ARG_GET(inQuat2);
		Vec::Vector_4V xx = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V yy = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V xx_plus_zz = Vec::V4AddScaled( xx, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V yy_plus_ww = Vec::V4AddScaled( yy, localInputA.GetWIntrin128(), localInputB.GetWIntrin128() );
		Vec::Vector_4V dotP = Vec::V4Add( xx_plus_zz, yy_plus_ww );

		// select sign
		Vec::Vector_4V dotPLTZero = Vec::V4IsLessThanV( dotP, Vec::V4VConstant(V_ZERO) );
		Vec::Vector_4V sign = Vec::V4And( dotPLTZero, Vec::V4VConstant(V_80000000) );
		outQuat = SoA_QuatV(
			Vec::V4Xor( localInputB.GetXIntrin128(), sign ),
			Vec::V4Xor( localInputB.GetYIntrin128(), sign ),
			Vec::V4Xor( localInputB.GetZIntrin128(), sign ),
			Vec::V4Xor( localInputB.GetWIntrin128(), sign )
			);
	}

	inline void Nlerp_Imp_Q( SoA_QuatV_InOut outQuat, Vec::Vector_4V_In tValue, QUATV_SOA_DECL2(inQuat1), QUATV_SOA_DECL2(inQuat2) )
	{
		SoA_QuatV localInputA = QUATV_SOA_ARG_GET(inQuat1);
		SoA_QuatV localInputB = QUATV_SOA_ARG_GET(inQuat2);
		SoA_QuatV lerpQuat = SoA_QuatV(
			Vec::V4Lerp( tValue, localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);

		Normalize_Imp(outQuat, QUATV_SOA_ARG(lerpQuat));
	}

	__forceinline void ToAxisAngle_Imp_Q( SoA_Vec3V_InOut outAxis, SoA_ScalarV_InOut outRadians, QUATV_SOA_DECL(inQuat) )
	{
		SoA_QuatV localQuat = QUATV_SOA_ARG_GET(inQuat);
		SoA_Vec3V axis;
		NormalizeSafe_Imp(axis, localQuat.GetXIntrin128(), localQuat.GetYIntrin128(), localQuat.GetZIntrin128(), Vec::V4VConstant(V_ZERO));

		Vec::Vector_4V quatW = Vec::V4Clamp(localQuat.GetWIntrin128(), Vec::V4VConstant(V_NEGONE), Vec::V4VConstant(V_ONE));
		Vec::Vector_4V halfAngle = Vec::V4Arccos(quatW);
		outRadians = SoA_ScalarV(Vec::V4Scale( Vec::V4VConstant(V_TWO), halfAngle));
		outAxis = axis;
	}

	__forceinline void FromAxisAngle_Imp_Q( SoA_QuatV_InOut outQuat, VEC3V_SOA_DECL3(inNormAxis), Vec::Vector_4V_In_After3Args inRadians )
	{
		SoA_Vec3V localAxis = VEC3V_SOA_ARG_GET(inNormAxis);

		Vec::Vector_4V theCos, theSin;
		Vec::V4SinAndCos( theSin, theCos, Vec::V4Scale( inRadians, Vec::V4VConstant(V_HALF) ) );

		SoA_Vec3V scaledAxis;
		Scale( scaledAxis, localAxis, SoA_ScalarV(theSin) );

		outQuat = SoA_QuatV(scaledAxis.GetXIntrin128(), scaledAxis.GetYIntrin128(), scaledAxis.GetZIntrin128(), theCos );
	}

	__forceinline void ScaleAngle_Imp_Q( SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat), Vec::Vector_4V_In_After3Args inRadians )
	{
		SoA_QuatV localQuat = QUATV_SOA_ARG_GET(inQuat);
		SoA_Vec3V axis;
		SoA_ScalarV angle;
		ToAxisAngle(axis, angle, localQuat);

		SoA_QuatV scaledQuat;
		FromAxisAngle( scaledQuat, axis, Scale( angle, SoA_ScalarV(inRadians) ) );

		SoA_ScalarV dot = Dot(axis, axis);
		Vec::Vector_4V normalizedAxis = Vec::V4IsGreaterThanV( dot.GetIntrin128(), Vec::V4VConstant(V_ONE_MINUS_FLT_SMALL_3) );
		Vec::Vector_4V positiveAngle = Vec::V4IsGreaterThanV( Vec::V3DotV( angle.GetIntrin128(), angle.GetIntrin128() ), Vec::V4VConstant(V_ZERO) );
		Vec::Vector_4V useScaled = Vec::V4And(normalizedAxis, positiveAngle);

		SelectFT(outQuat, SoA_VecBool1V(useScaled), localQuat, scaledQuat);
	}

	__forceinline void Mat33VFromQuatV_Imp_Q( SoA_Mat33V_InOut outMat, QUATV_SOA_DECL(inQuat) )
	{
		//	1 - 2*qy2 - 2*qz2	2*qx*qy - 2*qz*qw	2*qx*qz + 2*qy*qw
		//	2*qx*qy + 2*qz*qw	1 - 2*qx2 - 2*qz2	2*qy*qz - 2*qx*qw
		//	2*qx*qz - 2*qy*qw	2*qy*qz + 2*qx*qw	1 - 2*qx2 - 2*qy2

		using namespace Vec;
		SoA_QuatV localQuat = QUATV_SOA_ARG_GET(inQuat);
		Vector_4V qx = localQuat.GetX().GetIntrin128();
		Vector_4V qy = localQuat.GetY().GetIntrin128();
		Vector_4V qz = localQuat.GetZ().GetIntrin128();
		Vector_4V qw = localQuat.GetW().GetIntrin128();

		Vector_4V one = Vec::V4VConstant(V_ONE);
		Vector_4V two = Vec::V4VConstant(V_TWO);
		Vector_4V qx_qx = V4Scale(qx, qx);
		Vector_4V qy_qy = V4Scale(qy, qy);
		Vector_4V qz_qz = V4Scale(qz, qz);
		Vector_4V qz_qw = V4Scale(qz, qw);
		Vector_4V qy_qw = V4Scale(qy, qw);
		Vector_4V qx_qw = V4Scale(qx, qw);
		Vector_4V two_qx_qy = V4Scale(V4Scale(qx, qy), two);
		Vector_4V two_qx_qz = V4Scale(V4Scale(qx, qz), two);
		Vector_4V two_qy_qz = V4Scale(V4Scale(qy, qz), two);
		Vector_4V one_minus_two_qy_qy = V4SubtractScaled(one, two, qy_qy);
		Vector_4V one_minus_two_qx_qx = V4SubtractScaled(one, two, qx_qx);

		outMat = SoA_Mat33V(
			V4SubtractScaled(one_minus_two_qy_qy, two, qz_qz),
			V4AddScaled(two_qx_qy, two, qz_qw),
			V4SubtractScaled(two_qx_qz, two, qy_qw),
			V4SubtractScaled(two_qx_qy, two, qz_qw),
			V4SubtractScaled(one_minus_two_qx_qx, two, qz_qz),
			V4AddScaled(two_qy_qz, two, qx_qw),
			V4AddScaled(two_qx_qz, two, qy_qw),
			V4SubtractScaled(two_qy_qz, two, qx_qw),
			V4SubtractScaled(one_minus_two_qx_qx, two, qy_qy)
			);
	}

	__forceinline void QuatVFromEulersXYZ_Imp_Q(SoA_QuatV_InOut outQuat, VEC3V_SOA_DECL(radianAngles) )
	{
		using namespace Vec;

		// x, y, z half angles
		// qx = -cos(x)*sin(y)*sin(z) + sin(x)*cos(y)*cos(z)
		// qy =  cos(x)*sin(y)*cos(z) + sin(x)*cos(y)*sin(z)
		// qz =  cos(x)*cos(y)*sin(z) - sin(x)*sin(y)*cos(z)
		// qw =  cos(x)*cos(y)*cos(z) + sin(x)*sin(y)*sin(z)

		Vector_4V half = V4VConstant(V_HALF);
		SoA_Vec3V localAngles = VEC3V_SOA_ARG_GET(radianAngles);
		Vector_4V x = V4Scale(half, localAngles.GetX().GetIntrin128());
		Vector_4V y = V4Scale(half, localAngles.GetY().GetIntrin128());
		Vector_4V z = V4Scale(half, localAngles.GetZ().GetIntrin128());

		Vector_4V sx, sy, sz, cx, cy, cz;
		V4SinAndCos(sx, cx, x);
		V4SinAndCos(sy, cy, y);
		V4SinAndCos(sz, cz, z);

		Vector_4V cxsy = V4Scale(cx, sy);
		Vector_4V cxcy = V4Scale(cx, cy);
		Vector_4V sxcy = V4Scale(sx, cy);
		Vector_4V sxsy = V4Scale(sx, sy);

		Vector_4V qx = V4SubtractScaled(V4Scale(sxcy, cz), cxsy, sz);
		Vector_4V qy = V4AddScaled(V4Scale(cxsy, cz), sxcy, sz);
		Vector_4V qz = V4SubtractScaled(V4Scale(cxcy, sz), sxsy, cz);
		Vector_4V qw = V4AddScaled(V4Scale(cxcy, cz), sxsy, sz);

		outQuat = SoA_QuatV(qx, qy, qz, qw);
	}

	inline void Lerp_Imp( SoA_Vec4V_InOut inoutVec, Vec::Vector_4V_In tValue, VEC4V_SOA_DECL3(vectA), VEC4V_SOA_DECL2(vectB) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(vectA);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(vectB);

		inoutVec = SoA_Vec4V(
			Vec::V4Lerp( tValue, localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Lerp_Imp( SoA_Vec3V_InOut inoutVec, Vec::Vector_4V_In tValue, VEC3V_SOA_DECL3(vectA), VEC3V_SOA_DECL2(vectB) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(vectA);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(vectB);

		inoutVec = SoA_Vec3V(
			Vec::V4Lerp( tValue, localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Lerp_Imp( SoA_Vec2V_InOut inoutVec, Vec::Vector_4V_In tValue, VEC2V_SOA_DECL(vectA), VEC2V_SOA_DECL3(vectB) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(vectA);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(vectB);

		inoutVec = SoA_Vec2V(
			Vec::V4Lerp( tValue, localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Lerp( tValue, localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Clamp_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In lowBound, Vec::Vector_4V_In_After3Args highBound )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V localInputB = lowBound;
		Vec::Vector_4V localInputC = highBound;

		inoutVec = SoA_Vec2V(
			Vec::V4Clamp( localInputA.GetXIntrin128(), localInputB, localInputC ),
			Vec::V4Clamp( localInputA.GetYIntrin128(), localInputB, localInputC )
			);
	}

	inline void Clamp_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args lowBound, Vec::Vector_4V_In_After3Args highBound )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V localInputB = lowBound;
		Vec::Vector_4V localInputC = highBound;

		inoutVec = SoA_Vec3V(
			Vec::V4Clamp( localInputA.GetXIntrin128(), localInputB, localInputC ),
			Vec::V4Clamp( localInputA.GetYIntrin128(), localInputB, localInputC ),
			Vec::V4Clamp( localInputA.GetZIntrin128(), localInputB, localInputC )
			);
	}

	inline void Clamp_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args lowBound, Vec::Vector_4V_In_After3Args highBound )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V localInputB = lowBound;
		Vec::Vector_4V localInputC = highBound;

		inoutVec = SoA_Vec4V(
			Vec::V4Clamp( localInputA.GetXIntrin128(), localInputB, localInputC ),
			Vec::V4Clamp( localInputA.GetYIntrin128(), localInputB, localInputC ),
			Vec::V4Clamp( localInputA.GetZIntrin128(), localInputB, localInputC ),
			Vec::V4Clamp( localInputA.GetWIntrin128(), localInputB, localInputC )
			);
	}

	inline void ClampMag_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args minMag, Vec::Vector_4V_In_After3Args maxMag )
	{
		SoA_Vec3V result = VEC3V_SOA_ARG_GET(inVect);
		SoA_ScalarV localInputB = SoA_ScalarV(minMag);
		SoA_ScalarV localInputC = SoA_ScalarV(maxMag);

		SoA_ScalarV mag2 = MagSquared( result );
		SoA_ScalarV invMag = InvSqrt( mag2 );
		SoA_ScalarV maxMag2 = Scale( localInputC, localInputC );
		SoA_ScalarV minMag2 = Scale( localInputB, localInputB );
		SoA_VecBool1V isGT = (mag2 > maxMag2);
		SoA_VecBool1V isLT = (mag2 < minMag2);

		SoA_ScalarV multiplier = SelectFT( isGT, SelectFT(isLT, SoA_ScalarV(SoA_ScalarV::ONE), Scale(localInputB,invMag)), Scale(localInputC,invMag) );
		Scale( result, result, SoA_Vec3V(multiplier) );
		inoutVec = result;
	}

	inline void Negate_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		Vec::Vector_4V signMask = Vec::V4VConstant(V_80000000);
		inoutVec = SoA_Vec2V(
			Vec::V4Xor( localInputA.GetXIntrin128(), signMask ),
			Vec::V4Xor( localInputA.GetYIntrin128(), signMask )
			);
	}

	inline void Negate_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		Vec::Vector_4V signMask = Vec::V4VConstant(V_80000000);
		inoutVec = SoA_Vec3V(
			Vec::V4Xor( localInputA.GetXIntrin128(), signMask ),
			Vec::V4Xor( localInputA.GetYIntrin128(), signMask ),
			Vec::V4Xor( localInputA.GetZIntrin128(), signMask )
			);
	}

	inline void Negate_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		Vec::Vector_4V signMask = Vec::V4VConstant(V_80000000);
		inoutVec = SoA_Vec4V(
			Vec::V4Xor( localInputA.GetXIntrin128(), signMask ),
			Vec::V4Xor( localInputA.GetYIntrin128(), signMask ),
			Vec::V4Xor( localInputA.GetZIntrin128(), signMask ),
			Vec::V4Xor( localInputA.GetWIntrin128(), signMask )
			);
	}

	inline void InvertBits_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec2V(
			Vec::V4InvertBits(localInputA.GetXIntrin128()),
			Vec::V4InvertBits(localInputA.GetYIntrin128())
			);
	}

	inline void InvertBits_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec3V(
			Vec::V4InvertBits(localInputA.GetXIntrin128()),
			Vec::V4InvertBits(localInputA.GetYIntrin128()),
			Vec::V4InvertBits(localInputA.GetZIntrin128())
			);
	}

	inline void InvertBits_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec4V(
			Vec::V4InvertBits(localInputA.GetXIntrin128()),
			Vec::V4InvertBits(localInputA.GetYIntrin128()),
			Vec::V4InvertBits(localInputA.GetZIntrin128()),
			Vec::V4InvertBits(localInputA.GetWIntrin128())
			);
	}

	inline void InvertBits_Imp(SoA_VecBool2V_InOut inoutVec, VECBOOL2V_SOA_DECL(inVect))
	{
		SoA_VecBool2V localInputA = VECBOOL2V_SOA_ARG_GET(inVect);
		inoutVec = SoA_VecBool2V(
			Vec::V4InvertBits(localInputA.GetXIntrin128()),
			Vec::V4InvertBits(localInputA.GetYIntrin128())
			);
	}

	inline void InvertBits_Imp(SoA_VecBool3V_InOut inoutVec, VECBOOL3V_SOA_DECL(inVect))
	{
		SoA_VecBool3V localInputA = VECBOOL3V_SOA_ARG_GET(inVect);
		inoutVec = SoA_VecBool3V(
			Vec::V4InvertBits(localInputA.GetXIntrin128()),
			Vec::V4InvertBits(localInputA.GetYIntrin128()),
			Vec::V4InvertBits(localInputA.GetZIntrin128())
			);
	}

	inline void InvertBits_Imp(SoA_VecBool4V_InOut inoutVec, VECBOOL4V_SOA_DECL(inVect))
	{
		SoA_VecBool4V localInputA = VECBOOL4V_SOA_ARG_GET(inVect);
		inoutVec = SoA_VecBool4V(
			Vec::V4InvertBits(localInputA.GetXIntrin128()),
			Vec::V4InvertBits(localInputA.GetYIntrin128()),
			Vec::V4InvertBits(localInputA.GetZIntrin128()),
			Vec::V4InvertBits(localInputA.GetWIntrin128())
			);
	}

	inline void Invert_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec2V(
			Vec::V4Invert(localInputA.GetXIntrin128()),
			Vec::V4Invert(localInputA.GetYIntrin128())
			);
	}

	inline void Invert_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec3V(
			Vec::V4Invert(localInputA.GetXIntrin128()),
			Vec::V4Invert(localInputA.GetYIntrin128()),
			Vec::V4Invert(localInputA.GetZIntrin128())
			);
	}

	inline void Invert_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec4V(
			Vec::V4Invert(localInputA.GetXIntrin128()),
			Vec::V4Invert(localInputA.GetYIntrin128()),
			Vec::V4Invert(localInputA.GetZIntrin128()),
			Vec::V4Invert(localInputA.GetWIntrin128())
			);
	}

	inline void InvertSafe_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec2V(
			Vec::V4InvertSafe(localInputA.GetXIntrin128(), errValVect),
			Vec::V4InvertSafe(localInputA.GetYIntrin128(), errValVect)
			);
	}

	inline void InvertSafe_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec3V(
			Vec::V4InvertSafe(localInputA.GetXIntrin128(), errValVect),
			Vec::V4InvertSafe(localInputA.GetYIntrin128(), errValVect),
			Vec::V4InvertSafe(localInputA.GetZIntrin128(), errValVect)
			);
	}

	inline void InvertSafe_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec4V(
			Vec::V4InvertSafe(localInputA.GetXIntrin128(), errValVect),
			Vec::V4InvertSafe(localInputA.GetYIntrin128(), errValVect),
			Vec::V4InvertSafe(localInputA.GetZIntrin128(), errValVect),
			Vec::V4InvertSafe(localInputA.GetWIntrin128(), errValVect)
			);
	}

	inline void InvertFast_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec2V(
			Vec::V4InvertFast(localInputA.GetXIntrin128()),
			Vec::V4InvertFast(localInputA.GetYIntrin128())
			);
	}

	inline void InvertFast_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec3V(
			Vec::V4InvertFast(localInputA.GetXIntrin128()),
			Vec::V4InvertFast(localInputA.GetYIntrin128()),
			Vec::V4InvertFast(localInputA.GetZIntrin128())
			);
	}

	inline void InvertFast_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec4V(
			Vec::V4InvertFast(localInputA.GetXIntrin128()),
			Vec::V4InvertFast(localInputA.GetYIntrin128()),
			Vec::V4InvertFast(localInputA.GetZIntrin128()),
			Vec::V4InvertFast(localInputA.GetWIntrin128())
			);
	}

	inline void InvertFastSafe_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec2V(
			Vec::V4InvertFastSafe(localInputA.GetXIntrin128(), errValVect),
			Vec::V4InvertFastSafe(localInputA.GetYIntrin128(), errValVect)
			);
	}

	inline void InvertFastSafe_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec3V(
			Vec::V4InvertFastSafe(localInputA.GetXIntrin128(), errValVect),
			Vec::V4InvertFastSafe(localInputA.GetYIntrin128(), errValVect),
			Vec::V4InvertFastSafe(localInputA.GetZIntrin128(), errValVect)
			);
	}

	inline void InvertFastSafe_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVect);
		inoutVec = SoA_Vec4V(
			Vec::V4InvertFastSafe(localInputA.GetXIntrin128(), errValVect),
			Vec::V4InvertFastSafe(localInputA.GetYIntrin128(), errValVect),
			Vec::V4InvertFastSafe(localInputA.GetZIntrin128(), errValVect),
			Vec::V4InvertFastSafe(localInputA.GetWIntrin128(), errValVect)
			);
	}

	inline void Select_Imp(SoA_Vec4V_InOut inoutVec, VECBOOL4V_SOA_DECL(choiceVector), VEC4V_SOA_DECL2(zero), VEC4V_SOA_DECL2(nonZero))
	{
		SoA_VecBool4V localInputA = VECBOOL4V_SOA_ARG_GET(choiceVector);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(zero);
		SoA_Vec4V localInputC = VEC4V_SOA_ARG_GET(nonZero);
		inoutVec = SoA_Vec4V(
			Vec::V4SelectFT( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SelectFT( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4SelectFT( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC.GetZIntrin128() ),
			Vec::V4SelectFT( localInputA.GetWIntrin128(), localInputB.GetWIntrin128(), localInputC.GetWIntrin128() )
			);
	}

	inline void Select_Imp(SoA_Vec3V_InOut inoutVec, VECBOOL3V_SOA_DECL(choiceVector), VEC3V_SOA_DECL2(zero), VEC3V_SOA_DECL2(nonZero))
	{
		SoA_VecBool3V localInputA = VECBOOL3V_SOA_ARG_GET(choiceVector);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(zero);
		SoA_Vec3V localInputC = VEC3V_SOA_ARG_GET(nonZero);
		inoutVec = SoA_Vec3V(
			Vec::V4SelectFT( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SelectFT( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4SelectFT( localInputA.GetZIntrin128(), localInputB.GetZIntrin128(), localInputC.GetZIntrin128() )
			);
	}

	inline void Select_Imp(SoA_Vec2V_InOut inoutVec, VECBOOL2V_SOA_DECL(choiceVector), VEC2V_SOA_DECL2(zero), VEC2V_SOA_DECL3(nonZero))
	{
		SoA_VecBool2V localInputA = VECBOOL2V_SOA_ARG_GET(choiceVector);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(zero);
		SoA_Vec2V localInputC = VEC2V_SOA_ARG_GET(nonZero);
		inoutVec = SoA_Vec2V(
			Vec::V4SelectFT( localInputA.GetXIntrin128(), localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SelectFT( localInputA.GetYIntrin128(), localInputB.GetYIntrin128(), localInputC.GetYIntrin128() )
			);
	}

	inline void Select_Imp(SoA_Vec4V_InOut inoutVec, Vec::Vector_4V_In choiceVector, VEC4V_SOA_DECL3(zero), VEC4V_SOA_DECL2(nonZero))
	{
		Vec::Vector_4V localInputA = choiceVector;
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(zero);
		SoA_Vec4V localInputC = VEC4V_SOA_ARG_GET(nonZero);
		inoutVec = SoA_Vec4V(
			Vec::V4SelectFT( localInputA, localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SelectFT( localInputA, localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4SelectFT( localInputA, localInputB.GetZIntrin128(), localInputC.GetZIntrin128() ),
			Vec::V4SelectFT( localInputA, localInputB.GetWIntrin128(), localInputC.GetWIntrin128() )
			);
	}

	inline void Select_Imp(SoA_Vec3V_InOut inoutVec, Vec::Vector_4V_In choiceVector, VEC3V_SOA_DECL3(zero), VEC3V_SOA_DECL2(nonZero))
	{
		Vec::Vector_4V localInputA = choiceVector;
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(zero);
		SoA_Vec3V localInputC = VEC3V_SOA_ARG_GET(nonZero);
		inoutVec = SoA_Vec3V(
			Vec::V4SelectFT( localInputA, localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SelectFT( localInputA, localInputB.GetYIntrin128(), localInputC.GetYIntrin128() ),
			Vec::V4SelectFT( localInputA, localInputB.GetZIntrin128(), localInputC.GetZIntrin128() )
			);
	}

	inline void Select_Imp(SoA_Vec2V_InOut inoutVec, Vec::Vector_4V_In choiceVector, VEC2V_SOA_DECL(zero), VEC2V_SOA_DECL3(nonZero))
	{
		Vec::Vector_4V localInputA = choiceVector;
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(zero);
		SoA_Vec2V localInputC = VEC2V_SOA_ARG_GET(nonZero);
		inoutVec = SoA_Vec2V(
			Vec::V4SelectFT( localInputA, localInputB.GetXIntrin128(), localInputC.GetXIntrin128() ),
			Vec::V4SelectFT( localInputA, localInputB.GetYIntrin128(), localInputC.GetYIntrin128() )
			);
	}

	inline void Add_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec4V(
			Vec::V4Add( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Add( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Add( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Add( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Add_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec3V(
			Vec::V4Add( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Add( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Add( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Add_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec2V(
			Vec::V4Add( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Add( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Subtract_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec4V(
			Vec::V4Subtract( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Subtract( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Subtract( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Subtract( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Subtract_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec3V(
			Vec::V4Subtract( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Subtract( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Subtract( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Subtract_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec2V(
			Vec::V4Subtract( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Subtract( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Average_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec4V(
			Vec::V4Average( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Average( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Average( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Average( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Average_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec3V(
			Vec::V4Average( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Average( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Average( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Average_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		inoutVec = SoA_Vec2V(
			Vec::V4Average( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Average( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Pow_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(x), VEC4V_SOA_DECL2(y) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(x);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(y);
		inoutVec = SoA_Vec4V(
			Vec::V4Pow( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Pow( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Pow( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Pow( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Pow_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(x), VEC3V_SOA_DECL2(y) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(x);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(y);
		inoutVec = SoA_Vec3V(
			Vec::V4Pow( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Pow( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Pow( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Pow_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(x), VEC2V_SOA_DECL2(y) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(x);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(y);
		inoutVec = SoA_Vec2V(
			Vec::V4Pow( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Pow( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void PowPrecise_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(x), VEC4V_SOA_DECL2(y) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(x);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(y);
		inoutVec = SoA_Vec4V(
			Vec::V4PowPrecise( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4PowPrecise( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4PowPrecise( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4PowPrecise( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void PowPrecise_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(x), VEC3V_SOA_DECL2(y) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(x);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(y);
		inoutVec = SoA_Vec3V(
			Vec::V4PowPrecise( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4PowPrecise( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4PowPrecise( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void PowPrecise_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(x), VEC2V_SOA_DECL2(y) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(x);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(y);
		inoutVec = SoA_Vec2V(
			Vec::V4PowPrecise( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4PowPrecise( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Expt_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(x) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec4V(
			Vec::V4Expt( localInputA.GetXIntrin128() ),
			Vec::V4Expt( localInputA.GetYIntrin128() ),
			Vec::V4Expt( localInputA.GetZIntrin128() ),
			Vec::V4Expt( localInputA.GetWIntrin128() )
			);
	}

	inline void Expt_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(x) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec3V(
			Vec::V4Expt( localInputA.GetXIntrin128() ),
			Vec::V4Expt( localInputA.GetYIntrin128() ),
			Vec::V4Expt( localInputA.GetZIntrin128() )
			);
	}

	inline void Expt_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(x) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec2V(
			Vec::V4Expt( localInputA.GetXIntrin128() ),
			Vec::V4Expt( localInputA.GetYIntrin128() )
			);
	}

	inline void Log2_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(x) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec4V(
			Vec::V4Log2( localInputA.GetXIntrin128() ),
			Vec::V4Log2( localInputA.GetYIntrin128() ),
			Vec::V4Log2( localInputA.GetZIntrin128() ),
			Vec::V4Log2( localInputA.GetWIntrin128() )
			);
	}

	inline void Log2_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(x) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec3V(
			Vec::V4Log2( localInputA.GetXIntrin128() ),
			Vec::V4Log2( localInputA.GetYIntrin128() ),
			Vec::V4Log2( localInputA.GetZIntrin128() )
			);
	}

	inline void Log2_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(x) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec2V(
			Vec::V4Log2( localInputA.GetXIntrin128() ),
			Vec::V4Log2( localInputA.GetYIntrin128() )
			);
	}

	inline void Log10_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(x) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec4V(
			Vec::V4Log10( localInputA.GetXIntrin128() ),
			Vec::V4Log10( localInputA.GetYIntrin128() ),
			Vec::V4Log10( localInputA.GetZIntrin128() ),
			Vec::V4Log10( localInputA.GetWIntrin128() )
			);
	}

	inline void Log10_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(x) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec3V(
			Vec::V4Log10( localInputA.GetXIntrin128() ),
			Vec::V4Log10( localInputA.GetYIntrin128() ),
			Vec::V4Log10( localInputA.GetZIntrin128() )
			);
	}

	inline void Log10_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(x) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(x);
		inoutVec = SoA_Vec2V(
			Vec::V4Log10( localInputA.GetXIntrin128() ),
			Vec::V4Log10( localInputA.GetYIntrin128() )
			);
	}

	inline void Max_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec4V(
			Vec::V4Max( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Max( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Max( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Max( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Max_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec3V(
			Vec::V4Max( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Max( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Max( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Max_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec2V(
			Vec::V4Max( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Max( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Min_Imp( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec4V(
			Vec::V4Min( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Min( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Min( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Min( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Min_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec3V(
			Vec::V4Min( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Min( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Min( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Min_Imp( SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2) )
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec2V(
			Vec::V4Min( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Min( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void And_Imp(SoA_VecBool4V_InOut inoutVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2))
	{
		SoA_VecBool4V localInputA = VECBOOL4V_SOA_ARG_GET(inVector1);
		SoA_VecBool4V localInputB = VECBOOL4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool4V(
			Vec::V4And( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4And( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4And( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4And( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void And_Imp(SoA_VecBool3V_InOut inoutVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2))
	{
		SoA_VecBool3V localInputA = VECBOOL3V_SOA_ARG_GET(inVector1);
		SoA_VecBool3V localInputB = VECBOOL3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool3V(
			Vec::V4And( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4And( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4And( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void And_Imp(SoA_VecBool2V_InOut inoutVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2))
	{
		SoA_VecBool2V localInputA = VECBOOL2V_SOA_ARG_GET(inVector1);
		SoA_VecBool2V localInputB = VECBOOL2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool2V(
			Vec::V4And( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4And( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void And_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec4V(
			Vec::V4And( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4And( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4And( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4And( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void And_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec3V(
			Vec::V4And( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4And( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4And( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void And_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec2V(
			Vec::V4And( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4And( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Or_Imp(SoA_VecBool4V_InOut inoutVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2))
	{
		SoA_VecBool4V localInputA = VECBOOL4V_SOA_ARG_GET(inVector1);
		SoA_VecBool4V localInputB = VECBOOL4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool4V(
			Vec::V4Or( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Or( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Or( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Or( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Or_Imp(SoA_VecBool3V_InOut inoutVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2))
	{
		SoA_VecBool3V localInputA = VECBOOL3V_SOA_ARG_GET(inVector1);
		SoA_VecBool3V localInputB = VECBOOL3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool3V(
			Vec::V4Or( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Or( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Or( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Or_Imp(SoA_VecBool2V_InOut inoutVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2))
	{
		SoA_VecBool2V localInputA = VECBOOL2V_SOA_ARG_GET(inVector1);
		SoA_VecBool2V localInputB = VECBOOL2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool2V(
			Vec::V4Or( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Or( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Or_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec4V(
			Vec::V4Or( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Or( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Or( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Or( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Or_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec3V(
			Vec::V4Or( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Or( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Or( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Or_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec2V(
			Vec::V4Or( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Or( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Xor_Imp(SoA_VecBool4V_InOut inoutVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2))
	{
		SoA_VecBool4V localInputA = VECBOOL4V_SOA_ARG_GET(inVector1);
		SoA_VecBool4V localInputB = VECBOOL4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool4V(
			Vec::V4Xor( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Xor( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Xor( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Xor( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Xor_Imp(SoA_VecBool3V_InOut inoutVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2))
	{
		SoA_VecBool3V localInputA = VECBOOL3V_SOA_ARG_GET(inVector1);
		SoA_VecBool3V localInputB = VECBOOL3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool3V(
			Vec::V4Xor( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Xor( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Xor( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Xor_Imp(SoA_VecBool2V_InOut inoutVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2))
	{
		SoA_VecBool2V localInputA = VECBOOL2V_SOA_ARG_GET(inVector1);
		SoA_VecBool2V localInputB = VECBOOL2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool2V(
			Vec::V4Xor( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Xor( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Xor_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec4V(
			Vec::V4Xor( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Xor( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Xor( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Xor( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Xor_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec3V(
			Vec::V4Xor( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Xor( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Xor( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Xor_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec2V(
			Vec::V4Xor( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Xor( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Andc_Imp(SoA_VecBool4V_InOut inoutVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2))
	{
		SoA_VecBool4V localInputA = VECBOOL4V_SOA_ARG_GET(inVector1);
		SoA_VecBool4V localInputB = VECBOOL4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool4V(
			Vec::V4Andc( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Andc( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Andc( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Andc( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Andc_Imp(SoA_VecBool3V_InOut inoutVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2))
	{
		SoA_VecBool3V localInputA = VECBOOL3V_SOA_ARG_GET(inVector1);
		SoA_VecBool3V localInputB = VECBOOL3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool3V(
			Vec::V4Andc( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Andc( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Andc( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Andc_Imp(SoA_VecBool2V_InOut inoutVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2))
	{
		SoA_VecBool2V localInputA = VECBOOL2V_SOA_ARG_GET(inVector1);
		SoA_VecBool2V localInputB = VECBOOL2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_VecBool2V(
			Vec::V4Andc( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Andc( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline void Andc_Imp(SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(inVector1);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec4V(
			Vec::V4Andc( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Andc( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Andc( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() ),
			Vec::V4Andc( localInputA.GetWIntrin128(), localInputB.GetWIntrin128() )
			);
	}

	inline void Andc_Imp(SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(inVector1);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec3V(
			Vec::V4Andc( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Andc( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() ),
			Vec::V4Andc( localInputA.GetZIntrin128(), localInputB.GetZIntrin128() )
			);
	}

	inline void Andc_Imp(SoA_Vec2V_InOut inoutVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(inVector1);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(inVector2);
		inoutVec = SoA_Vec2V(
			Vec::V4Andc( localInputA.GetXIntrin128(), localInputB.GetXIntrin128() ),
			Vec::V4Andc( localInputA.GetYIntrin128(), localInputB.GetYIntrin128() )
			);
	}

	inline Vec::Vector_4V_Out Dist_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return Mag( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out DistFast_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return MagFast( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out Dist_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return Mag( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out DistFast_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return MagFast( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out Dist_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return Mag( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out DistFast_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return MagFast( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDist_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMag( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagSafe( diff, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistFast_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagFast( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistFastSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagFastSafe( diff, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDist_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMag( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagSafe( diff, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistFast_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagFast( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistFastSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagFastSafe( diff, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDist_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMag( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagSafe( diff, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistFast_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagFast( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistFastSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return InvMagFastSafe( diff, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out DistSquared_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		Subtract( diff, localInputA, localInputB );
		return MagSquared( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out DistSquared_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		Subtract( diff, localInputA, localInputB );
		return MagSquared( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out DistSquared_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		Subtract( diff, localInputA, localInputB );
		return MagSquared( diff ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquared_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return Invert( magSq ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertSafe( magSq, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredFast_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b))
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertFast( magSq ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredFastSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);
		SoA_Vec4V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertFastSafe( magSq, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquared_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return Invert( magSq ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertSafe( magSq, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredFast_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b))
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertFast( magSq ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredFastSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		SoA_Vec3V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertFastSafe( magSq, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquared_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return Invert( magSq ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertSafe( magSq, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredFast_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b))
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertFast( magSq ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out InvDistSquaredFastSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect)
	{
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		SoA_Vec2V diff;
		SoA_ScalarV magSq;
		Subtract( diff, localInputA, localInputB );
		magSq = MagSquared( diff );
		return InvertFastSafe( magSq, SoA_ScalarV(errValVect) ).GetIntrin128();
	}

	inline void Cross_Imp( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		// Scalar code
		//outVect.f.x = a.f.y * b.f.z - a.f.z * b.f.y; // p0-p1
		//outVect.f.y = a.f.z * b.f.x - a.f.x * b.f.z; // p2-p3
		//outVect.f.z = a.f.x * b.f.y - a.f.y * b.f.x; // p4-p5
		
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);
		Vec::Vector_4V p0 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V p1 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V p2 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V p3 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V p4 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V p5 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetXIntrin128() );
		inoutVec = SoA_Vec3V(
			Vec::V4Subtract( p0, p1 ),
			Vec::V4Subtract( p2, p3 ),
			Vec::V4Subtract( p4, p5 )
			);
	}

	inline Vec::Vector_4V_Out Cross_Imp( VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) )
	{
		// Scalar code
		// (a.f.x * b.f.y - a.f.y * b.f.x)
		SoA_Vec2V localInputA = VEC2V_SOA_ARG_GET(a);
		SoA_Vec2V localInputB = VEC2V_SOA_ARG_GET(b);
		return 
			Vec::V4Subtract(
				Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetYIntrin128() ),
				Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetXIntrin128() )
				);
	}

	//============================================================================
	// Matrix functions

	inline Vec::Vector_4V_Out Determinant_Imp33( MAT33V_SOA_DECL(mat) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(mat);
		SoA_Vec3V col0( localInputA.GetM00Intrin128(), localInputA.GetM10Intrin128(), localInputA.GetM20Intrin128() );
		SoA_Vec3V col1( localInputA.GetM01Intrin128(), localInputA.GetM11Intrin128(), localInputA.GetM21Intrin128() );
		SoA_Vec3V col2( localInputA.GetM02Intrin128(), localInputA.GetM12Intrin128(), localInputA.GetM22Intrin128() );
		SoA_Vec3V crossProd;
		Cross( crossProd, col0, col1 );
		return Dot( col2, crossProd ).GetIntrin128();
	}

	inline Vec::Vector_4V_Out Determinant_Imp44( MAT44V_SOA_DECL(mat) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(mat);
		Vec::Vector_4V _temp0, _temp1, _temp2, _temp3, _temp4, _temp5, _temp6, _temp7, _temp8, _temp9;
		Vec::Vector_4V _m00, _m10, _m20, _m30, _m01, _m11, _m21, _m31, _m02, _m12, _m22, _m32, _m03, _m13, _m23, _m33;
		_m00 = localInputA.GetM00Intrin128();
		_m10 = localInputA.GetM10Intrin128();
		_m20 = localInputA.GetM20Intrin128();
		_m30 = localInputA.GetM30Intrin128();
		_m01 = localInputA.GetM01Intrin128();
		_m11 = localInputA.GetM11Intrin128();
		_m21 = localInputA.GetM21Intrin128();
		_m31 = localInputA.GetM31Intrin128();
		_m02 = localInputA.GetM02Intrin128();
		_m12 = localInputA.GetM12Intrin128();
		_m22 = localInputA.GetM22Intrin128();
		_m32 = localInputA.GetM32Intrin128();
		_m03 = localInputA.GetM03Intrin128();
		_m13 = localInputA.GetM13Intrin128();
		_m23 = localInputA.GetM23Intrin128();
		_m33 = localInputA.GetM33Intrin128();
		_temp4 = Vec::V4Subtract( Vec::V4Scale( _m22, _m30 ), Vec::V4Scale( _m20, _m32 ) );
		_temp5 = Vec::V4Subtract( Vec::V4Scale( _m23, _m31 ), Vec::V4Scale( _m21, _m33 ) );
		_temp6 = Vec::V4Subtract( Vec::V4Scale( _m10, _m22 ), Vec::V4Scale( _m12, _m20 ) );
		_temp7 = Vec::V4Subtract( Vec::V4Scale( _m11, _m23 ), Vec::V4Scale( _m13, _m21 ) );
		_temp8 = Vec::V4Subtract( Vec::V4Scale( _m12, _m30 ), Vec::V4Scale( _m10, _m32 ) );
		_temp9 = Vec::V4Subtract( Vec::V4Scale( _m13, _m31 ), Vec::V4Scale( _m11, _m33 ) );
		_temp0 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m12, _temp5 ), Vec::V4Scale( _m32, _temp7 ) ), Vec::V4Scale( _m22, _temp9 ) );
		_temp1 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m13, _temp4 ), Vec::V4Scale( _m33, _temp6 ) ), Vec::V4Scale( _m23, _temp8 ) );
		_temp2 = Vec::V4Subtract( Vec::V4Add( Vec::V4Scale( _m30, _temp7 ), Vec::V4Scale( _m20, _temp9 ) ), Vec::V4Scale( _m10, _temp5 ) );
		_temp3 = Vec::V4Subtract( Vec::V4Add( Vec::V4Scale( _m31, _temp6 ), Vec::V4Scale( _m21, _temp8 ) ), Vec::V4Scale( _m11, _temp4 ) );
		return Vec::V4Add( Vec::V4Add( Vec::V4Add( Vec::V4Scale( _m00, _temp0 ), Vec::V4Scale( _m01, _temp1 ) ), Vec::V4Scale( _m02, _temp2 ) ), Vec::V4Scale( _m03, _temp3 ) );
	}

	inline void Add_Imp33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(a), MAT33V_SOA_DECL2(b) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(a);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat33V(
			Vec::V4Add( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
			Vec::V4Add( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
			Vec::V4Add( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
			Vec::V4Add( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() ),
			Vec::V4Add( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() ),
			Vec::V4Add( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
			Vec::V4Add( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
			Vec::V4Add( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
			Vec::V4Add( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
			);
	}

	inline void Subtract_Imp33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(a), MAT33V_SOA_DECL2(b) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(a);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat33V(
			Vec::V4Subtract( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
			Vec::V4Subtract( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
			Vec::V4Subtract( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
			Vec::V4Subtract( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() ),
			Vec::V4Subtract( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() ),
			Vec::V4Subtract( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
			Vec::V4Subtract( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
			Vec::V4Subtract( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
			Vec::V4Subtract( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() )
			);
	}

	inline void Abs_Imp33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(a) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(a);
		Vec::Vector_4V signMask = Vec::V4VConstant(V_80000000);
		inoutMat = SoA_Mat33V(
			Vec::V4Andc( localInputA.GetM00Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM10Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM20Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM01Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM11Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM21Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM02Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM12Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM22Intrin128(), signMask )
			);
	}

	inline void Scale_Imp33( SoA_Mat33V_InOut inoutMat, Vec::Vector_4V_In a, MAT33V_SOA_DECL3(b) )
	{
		Vec::Vector_4V localInputA = a;
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat33V(
			Vec::V4Scale( localInputA, localInputB.GetM00Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM10Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM20Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM01Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM11Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM21Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM02Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM12Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM22Intrin128() )
			);
	}

	inline void Add_Imp34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(a), MAT34V_SOA_DECL2(b) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(a);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat34V(
			Vec::V4Add( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
			Vec::V4Add( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
			Vec::V4Add( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
			Vec::V4Add( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() ),
			Vec::V4Add( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() ),
			Vec::V4Add( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
			Vec::V4Add( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
			Vec::V4Add( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
			Vec::V4Add( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
			Vec::V4Add( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() ),
			Vec::V4Add( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() ),
			Vec::V4Add( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
			);
	}

	inline void Subtract_Imp34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(a), MAT34V_SOA_DECL2(b) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(a);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat34V(
			Vec::V4Subtract( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
			Vec::V4Subtract( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
			Vec::V4Subtract( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
			Vec::V4Subtract( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() ),
			Vec::V4Subtract( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() ),
			Vec::V4Subtract( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
			Vec::V4Subtract( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
			Vec::V4Subtract( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
			Vec::V4Subtract( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
			Vec::V4Subtract( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() ),
			Vec::V4Subtract( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() ),
			Vec::V4Subtract( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() )
			);
	}

	inline void Abs_Imp34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(a) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(a);
		Vec::Vector_4V signMask = Vec::V4VConstant(V_80000000);
		inoutMat = SoA_Mat34V(
			Vec::V4Andc( localInputA.GetM00Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM10Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM20Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM01Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM11Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM21Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM02Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM12Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM22Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM03Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM13Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM23Intrin128(), signMask )
			);
	}

	inline void Scale_Imp34( SoA_Mat34V_InOut inoutMat, Vec::Vector_4V_In a, MAT34V_SOA_DECL3(b) )
	{
		Vec::Vector_4V localInputA = a;
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat34V(
			Vec::V4Scale( localInputA, localInputB.GetM00Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM10Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM20Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM01Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM11Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM21Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM02Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM12Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM22Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM03Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM13Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM23Intrin128() )
			);
	}











	inline void Add_Imp44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(a), MAT44V_SOA_DECL2(b) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(a);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat44V(
			Vec::V4Add( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
			Vec::V4Add( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
			Vec::V4Add( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
			Vec::V4Add( localInputA.GetM30Intrin128(), localInputB.GetM30Intrin128() ),
			Vec::V4Add( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() ),
			Vec::V4Add( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() ),
			Vec::V4Add( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
			Vec::V4Add( localInputA.GetM31Intrin128(), localInputB.GetM31Intrin128() ),
			Vec::V4Add( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
			Vec::V4Add( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
			Vec::V4Add( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
			Vec::V4Add( localInputA.GetM32Intrin128(), localInputB.GetM32Intrin128() ),
			Vec::V4Add( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() ),
			Vec::V4Add( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() ),
			Vec::V4Add( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() ),
			Vec::V4Add( localInputA.GetM33Intrin128(), localInputB.GetM33Intrin128() )
			);
	}

	inline void Subtract_Imp44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(a), MAT44V_SOA_DECL2(b) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(a);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat44V(
			Vec::V4Subtract( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() ),
			Vec::V4Subtract( localInputA.GetM10Intrin128(), localInputB.GetM10Intrin128() ),
			Vec::V4Subtract( localInputA.GetM20Intrin128(), localInputB.GetM20Intrin128() ),
			Vec::V4Subtract( localInputA.GetM30Intrin128(), localInputB.GetM30Intrin128() ),
			Vec::V4Subtract( localInputA.GetM01Intrin128(), localInputB.GetM01Intrin128() ),
			Vec::V4Subtract( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() ),
			Vec::V4Subtract( localInputA.GetM21Intrin128(), localInputB.GetM21Intrin128() ),
			Vec::V4Subtract( localInputA.GetM31Intrin128(), localInputB.GetM31Intrin128() ),
			Vec::V4Subtract( localInputA.GetM02Intrin128(), localInputB.GetM02Intrin128() ),
			Vec::V4Subtract( localInputA.GetM12Intrin128(), localInputB.GetM12Intrin128() ),
			Vec::V4Subtract( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() ),
			Vec::V4Subtract( localInputA.GetM32Intrin128(), localInputB.GetM32Intrin128() ),
			Vec::V4Subtract( localInputA.GetM03Intrin128(), localInputB.GetM03Intrin128() ),
			Vec::V4Subtract( localInputA.GetM13Intrin128(), localInputB.GetM13Intrin128() ),
			Vec::V4Subtract( localInputA.GetM23Intrin128(), localInputB.GetM23Intrin128() ),
			Vec::V4Subtract( localInputA.GetM33Intrin128(), localInputB.GetM33Intrin128() )
			);
	}

	inline void Abs_Imp44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(a) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(a);
		Vec::Vector_4V signMask = Vec::V4VConstant(V_80000000);
		inoutMat = SoA_Mat44V(
			Vec::V4Andc( localInputA.GetM00Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM10Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM20Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM30Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM01Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM11Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM21Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM31Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM02Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM12Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM22Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM32Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM03Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM13Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM23Intrin128(), signMask ),
			Vec::V4Andc( localInputA.GetM33Intrin128(), signMask )
			);
	}

	inline void Scale_Imp44( SoA_Mat44V_InOut inoutMat, Vec::Vector_4V_In a, MAT44V_SOA_DECL3(b) )
	{
		Vec::Vector_4V localInputA = a;
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(b);
		inoutMat = SoA_Mat44V(
			Vec::V4Scale( localInputA, localInputB.GetM00Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM10Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM20Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM30Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM01Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM11Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM21Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM31Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM02Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM12Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM22Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM32Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM03Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM13Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM23Intrin128() ),
			Vec::V4Scale( localInputA, localInputB.GetM33Intrin128() )
			);
	}

	// __forceinlined since it does nothing but change around components
	__forceinline void Transpose_Imp44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(mat) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(mat);
		inoutMat = SoA_Mat44V(
			localInputA.GetM00Intrin128(),
			localInputA.GetM01Intrin128(),
			localInputA.GetM02Intrin128(),
			localInputA.GetM03Intrin128(),
			localInputA.GetM10Intrin128(),
			localInputA.GetM11Intrin128(),
			localInputA.GetM12Intrin128(),
			localInputA.GetM13Intrin128(),
			localInputA.GetM20Intrin128(),
			localInputA.GetM21Intrin128(),
			localInputA.GetM22Intrin128(),
			localInputA.GetM23Intrin128(),
			localInputA.GetM30Intrin128(),
			localInputA.GetM31Intrin128(),
			localInputA.GetM32Intrin128(),
			localInputA.GetM33Intrin128()
			);
	}

	inline void InvertFull_Imp44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(mat) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(mat);

		Vec::Vector_4V _m00, _m10, _m20, _m30, _m01, _m11, _m21, _m31, _m02, _m12, _m22, _m32, _m03, _m13, _m23, _m33;
		Vec::Vector_4V _temp0, _temp1, _temp2, _temp3, _temp4, _temp5;
		Vec::Vector_4V _oneOverDeterminant;

		Vec::Vector_4V result00, result01, result02, result03;
		Vec::Vector_4V result10, result11, result12, result13;
		Vec::Vector_4V result20, result21, result22, result23;
		Vec::Vector_4V result30, result31, result32, result33;

		_m00 = localInputA.GetM00Intrin128();
		_m10 = localInputA.GetM10Intrin128();
		_m20 = localInputA.GetM20Intrin128();
		_m30 = localInputA.GetM30Intrin128();
		_m01 = localInputA.GetM01Intrin128();
		_m11 = localInputA.GetM11Intrin128();
		_m21 = localInputA.GetM21Intrin128();
		_m31 = localInputA.GetM31Intrin128();
		_m02 = localInputA.GetM02Intrin128();
		_m12 = localInputA.GetM12Intrin128();
		_m22 = localInputA.GetM22Intrin128();
		_m32 = localInputA.GetM32Intrin128();
		_m03 = localInputA.GetM03Intrin128();
		_m13 = localInputA.GetM13Intrin128();
		_m23 = localInputA.GetM23Intrin128();
		_m33 = localInputA.GetM33Intrin128();

		_temp0 = Vec::V4Subtract( Vec::V4Scale( _m22, _m30 ), Vec::V4Scale( _m20, _m32 ) );
		_temp1 = Vec::V4Subtract( Vec::V4Scale( _m23, _m31 ), Vec::V4Scale( _m21, _m33 ) );
		_temp2 = Vec::V4Subtract( Vec::V4Scale( _m10, _m22 ), Vec::V4Scale( _m12, _m20 ) );
		_temp3 = Vec::V4Subtract( Vec::V4Scale( _m11, _m23 ), Vec::V4Scale( _m13, _m21 ) );
		_temp4 = Vec::V4Subtract( Vec::V4Scale( _m12, _m30 ), Vec::V4Scale( _m10, _m32 ) );
		_temp5 = Vec::V4Subtract( Vec::V4Scale( _m13, _m31 ), Vec::V4Scale( _m11, _m33 ) );
		result00 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m12, _temp1 ), Vec::V4Scale( _m32, _temp3 ) ), Vec::V4Scale( _m22, _temp5 ) );
		result10 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m13, _temp0 ), Vec::V4Scale( _m33, _temp2 ) ), Vec::V4Scale( _m23, _temp4 ) );
		result20 = Vec::V4Subtract( Vec::V4Add( Vec::V4Scale( _m30, _temp3 ), Vec::V4Scale( _m20, _temp5 ) ), Vec::V4Scale( _m10, _temp1 ) );
		result30 = Vec::V4Subtract( Vec::V4Add( Vec::V4Scale( _m31, _temp2 ), Vec::V4Scale( _m21, _temp4 ) ), Vec::V4Scale( _m11, _temp0 ) );
		_oneOverDeterminant = Vec::V4Invert( Vec::V4Add( Vec::V4Add( Vec::V4Add( Vec::V4Scale( _m00, result00 ), Vec::V4Scale( _m01, result10 ) ), Vec::V4Scale( _m02, result20 ) ), Vec::V4Scale( _m03, result30 ) ) );
		result01 = Vec::V4Scale( _m02, _temp1 );
		result11 = Vec::V4Scale( _m03, _temp0 );
		result21 = Vec::V4Scale( _m00, _temp1 );
		result31 = Vec::V4Scale( _m01, _temp0 );
		result03 = Vec::V4Scale( _m02, _temp3 );
		result13 = Vec::V4Scale( _m03, _temp2 );
		result23 = Vec::V4Scale( _m00, _temp3 );
		result33 = Vec::V4Scale( _m01, _temp2 );
		result02 = Vec::V4Scale( _m02, _temp5 );
		result12 = Vec::V4Scale( _m03, _temp4 );
		result22 = Vec::V4Scale( _m00, _temp5 );
		result32 = Vec::V4Scale( _m01, _temp4 );
		_temp0 = Vec::V4Subtract( Vec::V4Scale( _m02, _m10 ), Vec::V4Scale( _m00, _m12 ) );
		_temp1 = Vec::V4Subtract( Vec::V4Scale( _m03, _m11 ), Vec::V4Scale( _m01, _m13 ) );
		_temp2 = Vec::V4Subtract( Vec::V4Scale( _m02, _m30 ), Vec::V4Scale( _m00, _m32 ) );
		_temp3 = Vec::V4Subtract( Vec::V4Scale( _m03, _m31 ), Vec::V4Scale( _m01, _m33 ) );
		_temp4 = Vec::V4Subtract( Vec::V4Scale( _m02, _m20 ), Vec::V4Scale( _m00, _m22 ) );
		_temp5 = Vec::V4Subtract( Vec::V4Scale( _m03, _m21 ), Vec::V4Scale( _m01, _m23 ) );
		result02 = Vec::V4Add( Vec::V4Subtract( Vec::V4Scale( _m32, _temp1 ), Vec::V4Scale( _m12, _temp3 ) ), result02 );
		result12 = Vec::V4Add( Vec::V4Subtract( Vec::V4Scale( _m33, _temp0 ), Vec::V4Scale( _m13, _temp2 ) ), result12 );
		result22 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m10, _temp3 ), Vec::V4Scale( _m30, _temp1 ) ), result22 );
		result32 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m11, _temp2 ), Vec::V4Scale( _m31, _temp0 ) ), result32 );
		result03 = Vec::V4Add( Vec::V4Subtract( Vec::V4Scale( _m12, _temp5 ), Vec::V4Scale( _m22, _temp1 ) ), result03 );
		result13 = Vec::V4Add( Vec::V4Subtract( Vec::V4Scale( _m13, _temp4 ), Vec::V4Scale( _m23, _temp0 ) ), result13 );
		result23 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m20, _temp1 ), Vec::V4Scale( _m10, _temp5 ) ), result23 );
		result33 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m21, _temp0 ), Vec::V4Scale( _m11, _temp4 ) ), result33 );
		result01 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m22, _temp3 ), Vec::V4Scale( _m32, _temp5 ) ), result01 );
		result11 = Vec::V4Subtract( Vec::V4Subtract( Vec::V4Scale( _m23, _temp2 ), Vec::V4Scale( _m33, _temp4 ) ), result11 );
		result21 = Vec::V4Add( Vec::V4Subtract( Vec::V4Scale( _m30, _temp5 ), Vec::V4Scale( _m20, _temp3 ) ), result21 );
		result31 = Vec::V4Add( Vec::V4Subtract( Vec::V4Scale( _m31, _temp4 ), Vec::V4Scale( _m21, _temp2 ) ), result31 );
		inoutMat = SoA_Mat44V(
			Vec::V4Scale( result00, _oneOverDeterminant ),
			Vec::V4Scale( result10, _oneOverDeterminant ),
			Vec::V4Scale( result20, _oneOverDeterminant ),
			Vec::V4Scale( result30, _oneOverDeterminant ),
			Vec::V4Scale( result01, _oneOverDeterminant ),
			Vec::V4Scale( result11, _oneOverDeterminant ),
			Vec::V4Scale( result21, _oneOverDeterminant ),
			Vec::V4Scale( result31, _oneOverDeterminant ),
			Vec::V4Scale( result02, _oneOverDeterminant ),
			Vec::V4Scale( result12, _oneOverDeterminant ),
			Vec::V4Scale( result22, _oneOverDeterminant ),
			Vec::V4Scale( result32, _oneOverDeterminant ),
			Vec::V4Scale( result03, _oneOverDeterminant ),
			Vec::V4Scale( result13, _oneOverDeterminant ),
			Vec::V4Scale( result23, _oneOverDeterminant ),
			Vec::V4Scale( result33, _oneOverDeterminant )
			);
	}

	inline void InvertTransformFull_Imp34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(mat) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(mat);

		Vec::Vector_4V oneOverDeterminant;
		SoA_Vec3V negatedCol3;
		SoA_Vec3V tempVec0, tempVec1, tempVec2;
		SoA_Vec3V invVec0, invVec1, invVec2;
		SoA_Vec3V translation;
		SoA_Vec3V sumA, sumB, sumC;
		SoA_Vec3V tempSum;

		Negate( negatedCol3, localInputA.GetCol3() );

		Cross( tempVec0, localInputA.GetCol1(), localInputA.GetCol2() );
		Cross( tempVec1, localInputA.GetCol2(), localInputA.GetCol0() );
		Cross( tempVec2, localInputA.GetCol0(), localInputA.GetCol1() );
		oneOverDeterminant = Invert( Dot( localInputA.GetCol2(), tempVec2 ) ).GetIntrin128();
		invVec0 = SoA_Vec3V( Vec::V4Scale( tempVec0.GetXIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec1.GetXIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec2.GetXIntrin128(), oneOverDeterminant ) );
		invVec1 = SoA_Vec3V( Vec::V4Scale( tempVec0.GetYIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec1.GetYIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec2.GetYIntrin128(), oneOverDeterminant ) );
		invVec2 = SoA_Vec3V( Vec::V4Scale( tempVec0.GetZIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec1.GetZIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec2.GetZIntrin128(), oneOverDeterminant ) );

		Scale( sumA, invVec0, SoA_Vec3V(negatedCol3.GetXIntrin128()) );
		Scale( sumB, invVec1, SoA_Vec3V(negatedCol3.GetYIntrin128()) );
		Scale( sumC, invVec2, SoA_Vec3V(negatedCol3.GetZIntrin128()) );
		Add( tempSum, sumA, sumB );
		Add( translation, tempSum, sumC );

		inoutMat = SoA_Mat34V(
			invVec0,
			invVec1,
			invVec2,
			translation
			);
	}

	inline void InvertTransformOrtho_Imp34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(mat) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(mat);
		SoA_Vec3V invVec0, invVec1, invVec2;
		SoA_Vec3V translation;
		SoA_Vec3V sumA, sumB, sumC;
		SoA_Vec3V tempSum, tempSum2;

		invVec0 = SoA_Vec3V( localInputA.GetM00Intrin128(), localInputA.GetM01Intrin128(), localInputA.GetM02Intrin128() );
		invVec1 = SoA_Vec3V( localInputA.GetM10Intrin128(), localInputA.GetM11Intrin128(), localInputA.GetM12Intrin128() );
		invVec2 = SoA_Vec3V( localInputA.GetM20Intrin128(), localInputA.GetM21Intrin128(), localInputA.GetM22Intrin128() );

		Scale( sumA, invVec0, SoA_Vec3V(localInputA.GetM03Intrin128()) );
		Scale( sumB, invVec1, SoA_Vec3V(localInputA.GetM13Intrin128()) );
		Scale( sumC, invVec2, SoA_Vec3V(localInputA.GetM23Intrin128()) );
		Add( tempSum, sumA, sumB );
		Add( tempSum2, tempSum, sumC );
		Negate( translation, tempSum2 );

		inoutMat = SoA_Mat34V(
			invVec0,
			invVec1,
			invVec2,
			translation
			);
	}

	// __forceinlined since it does nothing but change around components
	__forceinline void Transpose_Imp33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(mat) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(mat);
		inoutMat = SoA_Mat33V(
			localInputA.GetM00Intrin128(),
			localInputA.GetM01Intrin128(),
			localInputA.GetM02Intrin128(),
			localInputA.GetM10Intrin128(),
			localInputA.GetM11Intrin128(),
			localInputA.GetM12Intrin128(),
			localInputA.GetM20Intrin128(),
			localInputA.GetM21Intrin128(),
			localInputA.GetM22Intrin128()
			);
	}

	inline void InvertFull_Imp33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(mat) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(mat);

		Vec::Vector_4V oneOverDeterminant;
		SoA_Vec3V tempVec0, tempVec1, tempVec2;
		SoA_Vec3V invVec0, invVec1, invVec2;

		Cross( tempVec0, localInputA.GetCol1(), localInputA.GetCol2() );
		Cross( tempVec1, localInputA.GetCol2(), localInputA.GetCol0() );
		Cross( tempVec2, localInputA.GetCol0(), localInputA.GetCol1() );
		oneOverDeterminant = Invert( Dot( localInputA.GetCol2(), tempVec2 ) ).GetIntrin128();
		invVec0 = SoA_Vec3V( Vec::V4Scale( tempVec0.GetXIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec1.GetXIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec2.GetXIntrin128(), oneOverDeterminant ) );
		invVec1 = SoA_Vec3V( Vec::V4Scale( tempVec0.GetYIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec1.GetYIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec2.GetYIntrin128(), oneOverDeterminant ) );
		invVec2 = SoA_Vec3V( Vec::V4Scale( tempVec0.GetZIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec1.GetZIntrin128(), oneOverDeterminant ), Vec::V4Scale( tempVec2.GetZIntrin128(), oneOverDeterminant ) );

		inoutMat = SoA_Mat33V( invVec0, invVec1, invVec2 );
	}

	inline void Mul_Imp_44_44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(a), MAT44V_SOA_DECL2(b) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(a);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(b);

		Vec::Vector_4V _a00_b00 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a00_b01 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a00_b02 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a00_b03 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a01_b10 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a01_b11 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a01_b12 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a01_b13 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a02_b20 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a02_b21 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a02_b22 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a02_b23 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM23Intrin128() );
		Vec::Vector_4V _a03_b30 = Vec::V4Scale( localInputA.GetM03Intrin128(), localInputB.GetM30Intrin128() );
		Vec::Vector_4V _a03_b31 = Vec::V4Scale( localInputA.GetM03Intrin128(), localInputB.GetM31Intrin128() );
		Vec::Vector_4V _a03_b32 = Vec::V4Scale( localInputA.GetM03Intrin128(), localInputB.GetM32Intrin128() );
		Vec::Vector_4V _a03_b33 = Vec::V4Scale( localInputA.GetM03Intrin128(), localInputB.GetM33Intrin128() );

		Vec::Vector_4V _a10_b00 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a10_b01 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a10_b02 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a10_b03 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a11_b10 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a11_b11 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a11_b12 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a11_b13 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a12_b20 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a12_b21 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a12_b22 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a12_b23 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM23Intrin128() );
		Vec::Vector_4V _a13_b30 = Vec::V4Scale( localInputA.GetM13Intrin128(), localInputB.GetM30Intrin128() );
		Vec::Vector_4V _a13_b31 = Vec::V4Scale( localInputA.GetM13Intrin128(), localInputB.GetM31Intrin128() );
		Vec::Vector_4V _a13_b32 = Vec::V4Scale( localInputA.GetM13Intrin128(), localInputB.GetM32Intrin128() );
		Vec::Vector_4V _a13_b33 = Vec::V4Scale( localInputA.GetM13Intrin128(), localInputB.GetM33Intrin128() );

		Vec::Vector_4V _a20_b00 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a20_b01 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a20_b02 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a20_b03 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a21_b10 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a21_b11 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a21_b12 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a21_b13 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a22_b20 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a22_b21 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a22_b22 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a22_b23 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM23Intrin128() );
		Vec::Vector_4V _a23_b30 = Vec::V4Scale( localInputA.GetM23Intrin128(), localInputB.GetM30Intrin128() );
		Vec::Vector_4V _a23_b31 = Vec::V4Scale( localInputA.GetM23Intrin128(), localInputB.GetM31Intrin128() );
		Vec::Vector_4V _a23_b32 = Vec::V4Scale( localInputA.GetM23Intrin128(), localInputB.GetM32Intrin128() );
		Vec::Vector_4V _a23_b33 = Vec::V4Scale( localInputA.GetM23Intrin128(), localInputB.GetM33Intrin128() );

		Vec::Vector_4V _a30_b00 = Vec::V4Scale( localInputA.GetM30Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a30_b01 = Vec::V4Scale( localInputA.GetM30Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a30_b02 = Vec::V4Scale( localInputA.GetM30Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a30_b03 = Vec::V4Scale( localInputA.GetM30Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a31_b10 = Vec::V4Scale( localInputA.GetM31Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a31_b11 = Vec::V4Scale( localInputA.GetM31Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a31_b12 = Vec::V4Scale( localInputA.GetM31Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a31_b13 = Vec::V4Scale( localInputA.GetM31Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a32_b20 = Vec::V4Scale( localInputA.GetM32Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a32_b21 = Vec::V4Scale( localInputA.GetM32Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a32_b22 = Vec::V4Scale( localInputA.GetM32Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a32_b23 = Vec::V4Scale( localInputA.GetM32Intrin128(), localInputB.GetM23Intrin128() );
		Vec::Vector_4V _a33_b30 = Vec::V4Scale( localInputA.GetM33Intrin128(), localInputB.GetM30Intrin128() );
		Vec::Vector_4V _a33_b31 = Vec::V4Scale( localInputA.GetM33Intrin128(), localInputB.GetM31Intrin128() );
		Vec::Vector_4V _a33_b32 = Vec::V4Scale( localInputA.GetM33Intrin128(), localInputB.GetM32Intrin128() );
		Vec::Vector_4V _a33_b33 = Vec::V4Scale( localInputA.GetM33Intrin128(), localInputB.GetM33Intrin128() );

		inoutMat = SoA_Mat44V(
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b00, _a01_b10), _a02_b20 ), _a03_b30 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b00, _a11_b10), _a12_b20 ), _a13_b30 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b00, _a21_b10), _a22_b20 ), _a23_b30 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a30_b00, _a31_b10), _a32_b20 ), _a33_b30 ),

			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b01, _a01_b11), _a02_b21 ), _a03_b31 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b01, _a11_b11), _a12_b21 ), _a13_b31 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b01, _a21_b11), _a22_b21 ), _a23_b31 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a30_b01, _a31_b11), _a32_b21 ), _a33_b31 ),

			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b02, _a01_b12), _a02_b22 ), _a03_b32 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b02, _a11_b12), _a12_b22 ), _a13_b32 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b02, _a21_b12), _a22_b22 ), _a23_b32 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a30_b02, _a31_b12), _a32_b22 ), _a33_b32 ),

			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b03, _a01_b13), _a02_b23 ), _a03_b33 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b03, _a11_b13), _a12_b23 ), _a13_b33 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b03, _a21_b13), _a22_b23 ), _a23_b33 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a30_b03, _a31_b13), _a32_b23 ), _a33_b33 )
			);
	}

	inline void Mul_Imp_33_33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(a), MAT33V_SOA_DECL2(b) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(a);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(b);

		Vec::Vector_4V _a00_b00 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a00_b01 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a00_b02 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a01_b10 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a01_b11 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a01_b12 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a02_b20 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a02_b21 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a02_b22 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM22Intrin128() );

		Vec::Vector_4V _a10_b00 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a10_b01 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a10_b02 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a11_b10 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a11_b11 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a11_b12 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a12_b20 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a12_b21 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a12_b22 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM22Intrin128() );

		Vec::Vector_4V _a20_b00 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a20_b01 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a20_b02 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a21_b10 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a21_b11 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a21_b12 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a22_b20 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a22_b21 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a22_b22 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() );

		inoutMat = SoA_Mat33V(
			Vec::V4Add( Vec::V4Add( _a00_b00, _a01_b10), _a02_b20 ),
			Vec::V4Add( Vec::V4Add( _a10_b00, _a11_b10), _a12_b20 ),
			Vec::V4Add( Vec::V4Add( _a20_b00, _a21_b10), _a22_b20 ),

			Vec::V4Add( Vec::V4Add( _a00_b01, _a01_b11), _a02_b21 ),
			Vec::V4Add( Vec::V4Add( _a10_b01, _a11_b11), _a12_b21 ),
			Vec::V4Add( Vec::V4Add( _a20_b01, _a21_b11), _a22_b21 ),

			Vec::V4Add( Vec::V4Add( _a00_b02, _a01_b12), _a02_b22 ),
			Vec::V4Add( Vec::V4Add( _a10_b02, _a11_b12), _a12_b22 ),
			Vec::V4Add( Vec::V4Add( _a20_b02, _a21_b12), _a22_b22 )
			);
	}

	inline void Mul_Imp_44_4( SoA_Vec4V_InOut inoutVec, MAT44V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);

		Vec::Vector_4V _a00_b0 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a01_b1 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a02_b2 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a03_b3 = Vec::V4Scale( localInputA.GetM03Intrin128(), localInputB.GetWIntrin128() );

		Vec::Vector_4V _a10_b0 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a11_b1 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a12_b2 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a13_b3 = Vec::V4Scale( localInputA.GetM13Intrin128(), localInputB.GetWIntrin128() );

		Vec::Vector_4V _a20_b0 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a21_b1 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a22_b2 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a23_b3 = Vec::V4Scale( localInputA.GetM23Intrin128(), localInputB.GetWIntrin128() );

		Vec::Vector_4V _a30_b0 = Vec::V4Scale( localInputA.GetM30Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a31_b1 = Vec::V4Scale( localInputA.GetM31Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a32_b2 = Vec::V4Scale( localInputA.GetM32Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a33_b3 = Vec::V4Scale( localInputA.GetM33Intrin128(), localInputB.GetWIntrin128() );

		inoutVec = SoA_Vec4V(
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b0, _a01_b1 ), _a02_b2 ), _a03_b3 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b0, _a11_b1 ), _a12_b2 ), _a13_b3 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b0, _a21_b1 ), _a22_b2 ), _a23_b3 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a30_b0, _a31_b1 ), _a32_b2 ), _a33_b3 )
			);
	}

	inline void Mul_Imp_4_44( SoA_Vec4V_InOut inoutVec, VEC4V_SOA_DECL(a), MAT44V_SOA_DECL2(b) )
	{
		SoA_Vec4V localInputA = VEC4V_SOA_ARG_GET(a);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(b);

		Vec::Vector_4V _a0_b00 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a1_b10 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a2_b20 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a3_b30 = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetM30Intrin128() );

		Vec::Vector_4V _a0_b01 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a1_b11 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a2_b21 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a3_b31 = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetM31Intrin128() );

		Vec::Vector_4V _a0_b02 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a1_b12 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a2_b22 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a3_b32 = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetM32Intrin128() );

		Vec::Vector_4V _a0_b03 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a1_b13 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a2_b23 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM23Intrin128() );
		Vec::Vector_4V _a3_b33 = Vec::V4Scale( localInputA.GetWIntrin128(), localInputB.GetM33Intrin128() );

		inoutVec = SoA_Vec4V(
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a0_b00, _a1_b10 ), _a2_b20 ), _a3_b30 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a0_b01, _a1_b11 ), _a2_b21 ), _a3_b31 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a0_b02, _a1_b12 ), _a2_b22 ), _a3_b32 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a0_b03, _a1_b13 ), _a2_b23 ), _a3_b33 )
			);
	}

	inline void Mul_Imp_33_3( SoA_Vec3V_InOut inoutVec, MAT33V_SOA_DECL(a), VEC3V_SOA_DECL2(b) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(a);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(b);

		Vec::Vector_4V _a00_b0 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a01_b1 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a02_b2 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetZIntrin128() );

		Vec::Vector_4V _a10_b0 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a11_b1 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a12_b2 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetZIntrin128() );

		Vec::Vector_4V _a20_b0 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a21_b1 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a22_b2 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetZIntrin128() );

		inoutVec = SoA_Vec3V(
			Vec::V4Add( Vec::V4Add( _a00_b0, _a01_b1 ), _a02_b2 ),
			Vec::V4Add( Vec::V4Add( _a10_b0, _a11_b1 ), _a12_b2 ),
			Vec::V4Add( Vec::V4Add( _a20_b0, _a21_b1 ), _a22_b2 )
			);
	}

	inline void Mul_Imp_3_33( SoA_Vec3V_InOut inoutVec, VEC3V_SOA_DECL(a), MAT33V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(b);

		Vec::Vector_4V _a0_b00 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a1_b10 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a2_b20 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM20Intrin128() );

		Vec::Vector_4V _a0_b01 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a1_b11 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a2_b21 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM21Intrin128() );

		Vec::Vector_4V _a0_b02 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a1_b12 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a2_b22 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM22Intrin128() );

		inoutVec = SoA_Vec3V(
			Vec::V4Add( Vec::V4Add( _a0_b00, _a1_b10 ), _a2_b20 ),
			Vec::V4Add( Vec::V4Add( _a0_b01, _a1_b11 ), _a2_b21 ),
			Vec::V4Add( Vec::V4Add( _a0_b02, _a1_b12 ), _a2_b22 )
			);
	}

	inline void Mul_Imp_34_4( SoA_Vec3V_InOut inoutVec, MAT34V_SOA_DECL(a), VEC4V_SOA_DECL2(b) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(a);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(b);

		Vec::Vector_4V _a00_b0 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a01_b1 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a02_b2 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a03_b3 = Vec::V4Scale( localInputA.GetM03Intrin128(), localInputB.GetWIntrin128() );

		Vec::Vector_4V _a10_b0 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a11_b1 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a12_b2 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a13_b3 = Vec::V4Scale( localInputA.GetM13Intrin128(), localInputB.GetWIntrin128() );

		Vec::Vector_4V _a20_b0 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a21_b1 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a22_b2 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetZIntrin128() );
		Vec::Vector_4V _a23_b3 = Vec::V4Scale( localInputA.GetM23Intrin128(), localInputB.GetWIntrin128() );

		inoutVec = SoA_Vec3V(
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b0, _a01_b1 ), _a02_b2 ), _a03_b3 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b0, _a11_b1 ), _a12_b2 ), _a13_b3 ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b0, _a21_b1 ), _a22_b2 ), _a23_b3 )
			);
	}

	inline void Mul_Imp_3_34( SoA_Vec4V_InOut inoutVec, VEC3V_SOA_DECL(a), MAT34V_SOA_DECL2(b) )
	{
		SoA_Vec3V localInputA = VEC3V_SOA_ARG_GET(a);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(b);

		Vec::Vector_4V _a0_b00 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a1_b10 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a2_b20 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM20Intrin128() );

		Vec::Vector_4V _a0_b01 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a1_b11 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a2_b21 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM21Intrin128() );

		Vec::Vector_4V _a0_b02 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a1_b12 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a2_b22 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM22Intrin128() );

		Vec::Vector_4V _a0_b03 = Vec::V4Scale( localInputA.GetXIntrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a1_b13 = Vec::V4Scale( localInputA.GetYIntrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a2_b23 = Vec::V4Scale( localInputA.GetZIntrin128(), localInputB.GetM23Intrin128() );

		inoutVec = SoA_Vec4V(
			Vec::V4Add( Vec::V4Add( _a0_b00, _a1_b10 ), _a2_b20 ),
			Vec::V4Add( Vec::V4Add( _a0_b01, _a1_b11 ), _a2_b21 ),
			Vec::V4Add( Vec::V4Add( _a0_b02, _a1_b12 ), _a2_b22 ),
			Vec::V4Add( Vec::V4Add( _a0_b03, _a1_b13 ), _a2_b23 )
			);
	}

	inline void UnTransformFull_Imp_44_44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(origTransformMat), MAT44V_SOA_DECL2(concatMat) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(origTransformMat);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(concatMat);

		SoA_Mat44V invMat;
		InvertFull( invMat, localInputA );
		Multiply( inoutMat, invMat, localInputB );
	}

	inline void UnTransformOrtho_Imp_44_44( SoA_Mat44V_InOut inoutMat, MAT44V_SOA_DECL(origOrthoTransformMat), MAT44V_SOA_DECL2(concatMat) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Mat44V localInputB = MAT44V_SOA_ARG_GET(concatMat);

		SoA_Mat44V invMat;
		Transpose( invMat, localInputA );
		Multiply( inoutMat, invMat, localInputB );
	}

	inline void UnTransformFull_Imp_44_4( SoA_Vec4V_InOut inoutVec, MAT44V_SOA_DECL(origTransformMat), VEC4V_SOA_DECL2(transformedVect) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(origTransformMat);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(transformedVect);

		SoA_Mat44V invMat;
		InvertFull( invMat, localInputA );
		Multiply( inoutVec, invMat, localInputB );
	}

	inline void UnTransformOrtho_Imp_44_4( SoA_Vec4V_InOut inoutVec, MAT44V_SOA_DECL(origOrthoTransformMat), VEC4V_SOA_DECL2(transformedVect) )
	{
		SoA_Mat44V localInputA = MAT44V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Vec4V localInputB = VEC4V_SOA_ARG_GET(transformedVect);

		SoA_Mat44V invMat;
		Transpose( invMat, localInputA );
		Multiply( inoutVec, invMat, localInputB );
	}

	inline void UnTransformFull_Imp_33_33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(origTransformMat), MAT33V_SOA_DECL2(concatMat) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(origTransformMat);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(concatMat);

		SoA_Mat33V invMat;
		InvertFull( invMat, localInputA );
		Multiply( inoutMat, invMat, localInputB );
	}

	inline void UnTransformOrtho_Imp_33_33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(origOrthoTransformMat), MAT33V_SOA_DECL2(concatMat) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Mat33V localInputB = MAT33V_SOA_ARG_GET(concatMat);

		SoA_Mat33V invMat;
		Transpose( invMat, localInputA );
		Multiply( inoutMat, invMat, localInputB );
	}

	inline void UnTransformFull_Imp_33_3( SoA_Vec3V_InOut inoutVec, MAT33V_SOA_DECL(origTransformMat), VEC3V_SOA_DECL2(transformedVect) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(origTransformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(transformedVect);

		SoA_Mat33V invMat;
		InvertFull( invMat, localInputA );
		Multiply( inoutVec, invMat, localInputB );
	}

	inline void UnTransformOrtho_Imp_33_3( SoA_Vec3V_InOut inoutVec, MAT33V_SOA_DECL(origOrthoTransformMat), VEC3V_SOA_DECL2(transformedVect) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(transformedVect);

		SoA_Mat33V invMat;
		Transpose( invMat, localInputA );
		Multiply( inoutVec, invMat, localInputB );
	}

	inline void Transform_Imp_34_34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(transformMat1), MAT34V_SOA_DECL2(transformMat2) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(transformMat1);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(transformMat2);

		Vec::Vector_4V _a00_b00 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a00_b01 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a00_b02 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a00_b03 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a01_b10 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a01_b11 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a01_b12 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a01_b13 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a02_b20 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a02_b21 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a02_b22 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a02_b23 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetM23Intrin128() );

		Vec::Vector_4V _a10_b00 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a10_b01 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a10_b02 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a10_b03 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a11_b10 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a11_b11 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a11_b12 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a11_b13 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a12_b20 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a12_b21 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a12_b22 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a12_b23 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetM23Intrin128() );

		Vec::Vector_4V _a20_b00 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM00Intrin128() );
		Vec::Vector_4V _a20_b01 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM01Intrin128() );
		Vec::Vector_4V _a20_b02 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM02Intrin128() );
		Vec::Vector_4V _a20_b03 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetM03Intrin128() );
		Vec::Vector_4V _a21_b10 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM10Intrin128() );
		Vec::Vector_4V _a21_b11 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM11Intrin128() );
		Vec::Vector_4V _a21_b12 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM12Intrin128() );
		Vec::Vector_4V _a21_b13 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetM13Intrin128() );
		Vec::Vector_4V _a22_b20 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM20Intrin128() );
		Vec::Vector_4V _a22_b21 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM21Intrin128() );
		Vec::Vector_4V _a22_b22 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM22Intrin128() );
		Vec::Vector_4V _a22_b23 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetM23Intrin128() );

		inoutMat = SoA_Mat34V(
			Vec::V4Add( Vec::V4Add( _a00_b00, _a01_b10), _a02_b20 ),
			Vec::V4Add( Vec::V4Add( _a10_b00, _a11_b10), _a12_b20 ),
			Vec::V4Add( Vec::V4Add( _a20_b00, _a21_b10), _a22_b20 ),

			Vec::V4Add( Vec::V4Add( _a00_b01, _a01_b11), _a02_b21 ),
			Vec::V4Add( Vec::V4Add( _a10_b01, _a11_b11), _a12_b21 ),
			Vec::V4Add( Vec::V4Add( _a20_b01, _a21_b11), _a22_b21 ),

			Vec::V4Add( Vec::V4Add( _a00_b02, _a01_b12), _a02_b22 ),
			Vec::V4Add( Vec::V4Add( _a10_b02, _a11_b12), _a12_b22 ),
			Vec::V4Add( Vec::V4Add( _a20_b02, _a21_b12), _a22_b22 ),

			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a00_b03, _a01_b13), _a02_b23 ), localInputA.GetM03Intrin128() ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a10_b03, _a11_b13), _a12_b23 ), localInputA.GetM13Intrin128() ),
			Vec::V4Add( Vec::V4Add( Vec::V4Add( _a20_b03, _a21_b13), _a22_b23 ), localInputA.GetM23Intrin128() )
			);
	}

	inline void Transform3x3_Imp_34_3( SoA_Vec3V_InOut inoutVec, MAT33V_SOA_DECL(transformMat), VEC3V_SOA_DECL2(inVec) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(transformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inVec);

		Vec::Vector_4V _a00_b0 = Vec::V4Scale( localInputA.GetM00Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a01_b1 = Vec::V4Scale( localInputA.GetM01Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a02_b2 = Vec::V4Scale( localInputA.GetM02Intrin128(), localInputB.GetZIntrin128() );

		Vec::Vector_4V _a10_b0 = Vec::V4Scale( localInputA.GetM10Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a11_b1 = Vec::V4Scale( localInputA.GetM11Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a12_b2 = Vec::V4Scale( localInputA.GetM12Intrin128(), localInputB.GetZIntrin128() );

		Vec::Vector_4V _a20_b0 = Vec::V4Scale( localInputA.GetM20Intrin128(), localInputB.GetXIntrin128() );
		Vec::Vector_4V _a21_b1 = Vec::V4Scale( localInputA.GetM21Intrin128(), localInputB.GetYIntrin128() );
		Vec::Vector_4V _a22_b2 = Vec::V4Scale( localInputA.GetM22Intrin128(), localInputB.GetZIntrin128() );

		inoutVec = SoA_Vec3V(
			Vec::V4Add( Vec::V4Add( _a00_b0, _a01_b1), _a02_b2 ),
			Vec::V4Add( Vec::V4Add( _a10_b0, _a11_b1), _a12_b2 ),
			Vec::V4Add( Vec::V4Add( _a20_b0, _a21_b1), _a22_b2 )
			);
	}

	inline void Transform_Imp_34_3( SoA_Vec3V_InOut inoutPoint, MAT34V_SOA_DECL(transformMat), VEC3V_SOA_DECL2(inPoint) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(transformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(inPoint);

		inoutPoint = SoA_Vec3V(
			Vec::V4AddScaled(Vec::V4AddScaled(Vec::V4AddScaled(localInputA.GetM03Intrin128(), 
									localInputA.GetM00Intrin128(), localInputB.GetXIntrin128()),
									localInputA.GetM01Intrin128(), localInputB.GetYIntrin128()),
									localInputA.GetM02Intrin128(), localInputB.GetZIntrin128()),
			Vec::V4AddScaled(Vec::V4AddScaled(Vec::V4AddScaled(localInputA.GetM13Intrin128(), 
									localInputA.GetM10Intrin128(), localInputB.GetXIntrin128()),
									localInputA.GetM11Intrin128(), localInputB.GetYIntrin128()),
									localInputA.GetM12Intrin128(), localInputB.GetZIntrin128()),
			Vec::V4AddScaled(Vec::V4AddScaled(Vec::V4AddScaled(localInputA.GetM23Intrin128(), 
									localInputA.GetM20Intrin128(), localInputB.GetXIntrin128()),
									localInputA.GetM21Intrin128(), localInputB.GetYIntrin128()),
									localInputA.GetM22Intrin128(), localInputB.GetZIntrin128()));
	}

	inline void UnTransformFull_Imp_34_34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(origTransformMat), MAT34V_SOA_DECL2(concatMat) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(origTransformMat);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(concatMat);

		// Subtract the translation.
		SoA_Vec3V tempCol3( localInputB.GetCol3() - localInputA.GetCol3() );

		// Find the 3x3 inverse.
		SoA_Mat33V localInputA_33 = localInputA.GetMat33();
		SoA_Mat33V invMat;
		InvertFull( invMat, localInputA_33 );

		// Premultiply by inverse.
		SoA_Vec3V resultCol0, resultCol1, resultCol2, resultCol3;
		SoA_Vec3V inCol0 = localInputB.GetCol0();
		SoA_Vec3V inCol1 = localInputB.GetCol1();
		SoA_Vec3V inCol2 = localInputB.GetCol2();
		SoA_Vec3V inCol3 = tempCol3;
		Multiply( resultCol0, invMat, inCol0 );
		Multiply( resultCol1, invMat, inCol1 );
		Multiply( resultCol2, invMat, inCol2 );
		Multiply( resultCol3, invMat, inCol3 );
		
		inoutMat = SoA_Mat34V(
			resultCol0,
			resultCol1,
			resultCol2,
			resultCol3
			);
	}

	inline void UnTransformOrtho_Imp_34_34( SoA_Mat34V_InOut inoutMat, MAT34V_SOA_DECL(origOrthoTransformMat), MAT34V_SOA_DECL2(concatMat) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Mat34V localInputB = MAT34V_SOA_ARG_GET(concatMat);

		// Subtract the translation.
		SoA_Vec3V tempCol3( localInputB.GetCol3() - localInputA.GetCol3() );

		// Find the 3x3 inverse.
		SoA_Mat33V localInputA_33 = localInputA.GetMat33();
		SoA_Mat33V invMat;
		Transpose( invMat, localInputA_33 );

		// Premultiply by inverse.
		SoA_Vec3V resultCol0, resultCol1, resultCol2, resultCol3;
		SoA_Vec3V inCol0 = localInputB.GetCol0();
		SoA_Vec3V inCol1 = localInputB.GetCol1();
		SoA_Vec3V inCol2 = localInputB.GetCol2();
		SoA_Vec3V inCol3 = tempCol3;
		Multiply( resultCol0, invMat, inCol0 );
		Multiply( resultCol1, invMat, inCol1 );
		Multiply( resultCol2, invMat, inCol2 );
		Multiply( resultCol3, invMat, inCol3 );
		
		inoutMat = SoA_Mat34V(
			resultCol0,
			resultCol1,
			resultCol2,
			resultCol3
			);
	}

	inline void UnTransform3x3Full_Imp_34_3( SoA_Vec3V_InOut inoutVec, MAT33V_SOA_DECL(origTransformMat), VEC3V_SOA_DECL2(transformedVect) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(origTransformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(transformedVect);

		// Find the 3x3 inverse.
		SoA_Mat33V invMat;
		InvertFull( invMat, localInputA );

		// Premultiply by inverse.
		Multiply( inoutVec, invMat, localInputB );
	}

	inline void UnTransformFull_Imp_34_3( SoA_Vec3V_InOut inoutPoint, MAT34V_SOA_DECL(origTransformMat), VEC3V_SOA_DECL2(transformedPoint) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(origTransformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(transformedPoint);

		// Subtract translation.
		SoA_Vec3V tempPoint = localInputB - localInputA.GetCol3();

		// Find the 3x3 inverse.
		SoA_Mat33V invMat;
		SoA_Mat33V localInputA_33 = localInputA.GetMat33();
		InvertFull( invMat, localInputA_33 );

		// Multiply.
		Multiply( inoutPoint, invMat, tempPoint );
	}

	inline void UnTransform3x3FullOrtho_Imp_34_3( SoA_Vec3V_InOut inoutVec, MAT33V_SOA_DECL(origOrthoTransformMat), VEC3V_SOA_DECL2(transformedVect) )
	{
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(transformedVect);

		// Find the 3x3 inverse.
		SoA_Mat33V invMat;
		Transpose( invMat, localInputA );

		// Premultiply by inverse.
		Multiply( inoutVec, invMat, localInputB );
	}

	inline void UnTransformOrtho_Imp_34_3( SoA_Vec3V_InOut inoutPoint, MAT34V_SOA_DECL(origOrthoTransformMat), VEC3V_SOA_DECL2(transformedPoint) )
	{
		SoA_Mat34V localInputA = MAT34V_SOA_ARG_GET(origOrthoTransformMat);
		SoA_Vec3V localInputB = VEC3V_SOA_ARG_GET(transformedPoint);

		// Subtract translation.
		SoA_Vec3V tempPoint = localInputB - localInputA.GetCol3();

		// Find the 3x3 inverse.
		SoA_Mat33V invMat;
		SoA_Mat33V localInputA_33 = localInputA.GetMat33();
		Transpose( invMat, localInputA_33 );

		// Multiply.
		Multiply( inoutPoint, invMat, tempPoint );
	}
	
	inline void ReOrthonormalize_Imp33( SoA_Mat33V_InOut inoutMat, MAT33V_SOA_DECL(inMat) )
	{
		// This routine uses the X and Y to reproduce the Z, then uses the Y and Z to reproduce the X. Thus Y is preserved.
		SoA_Mat33V localInputA = MAT33V_SOA_ARG_GET(inMat);

		SoA_Vec3V _x = localInputA.GetCol0();
		SoA_Vec3V _y = localInputA.GetCol1();

		// X' = Y cross Z'
		// Y' = Y
		// Z' = X cross Y
		SoA_Vec3V newX, newZ;
		Cross( newZ, _x, _y );
		Cross( newX, _y, newZ );

		// Normalize
		SoA_Vec3V normalizedX, normalizedY, normalizedZ;
		Normalize( normalizedX, newX );
		Normalize( normalizedY, _y );
		Normalize( normalizedZ, newZ );

		inoutMat = SoA_Mat33V( normalizedX, normalizedY, normalizedZ );
	}

} // namespace Imp


} // namespace rage
