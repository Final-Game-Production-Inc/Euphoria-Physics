namespace rage
{

	//============================================================================
	// Utility functions

	__forceinline Quatf_Out QuatFromAxisAngle( Vec3f_In normAxis, float radians )
	{
		return Quatf( Vec::V4QuatFromAxisAngle( normAxis.GetIntrin(), radians ) );
	}

	__forceinline Quatf_Out QuatFromXAxisAngle( float radians )
	{
		return Quatf( Vec::V4QuatFromXAxisAngle( radians ) );
	}

	__forceinline Quatf_Out QuatFromYAxisAngle( float radians )
	{
		return Quatf( Vec::V4QuatFromYAxisAngle( radians ) );
	}

	__forceinline Quatf_Out QuatFromZAxisAngle( float radians )
	{
		return Quatf( Vec::V4QuatFromZAxisAngle( radians ) );
	}

	__forceinline Vec2f_Out Vec2FromF32( float f )
	{
		return Vec2f( f, f );
	}

	__forceinline Vec3f_Out Vec3FromF32( float f )
	{
		return Vec3f( f, f, f );
	}

	__forceinline Vec4f_Out Vec4FromF32( float f )
	{
		return Vec4f( f, f, f, f );
	}

	__forceinline Quatf_Out QuatFromF32( float f )
	{
		return Quatf( f, f, f, f );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int IsZeroAll(Quatf_In inQuat)
	{
		return Vec::V4IsZeroAll( inQuat.GetIntrin() );
	}

	__forceinline unsigned int IsZeroNone(Quatf_In inQuat)
	{
		return Vec::V4IsZeroNone( inQuat.GetIntrin() );
	}

	__forceinline unsigned int IsEqualAll(Quatf_In inVector1, Quatf_In inVector2)
	{
		return Vec::V4IsEqualAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualNone(Quatf_In inVector1, Quatf_In inVector2)
	{
		return Vec::V4IsEqualNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntAll(Quatf_In inVector1, Quatf_In inVector2)
	{
		return Vec::V4IsEqualIntAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntNone(Quatf_In inVector1, Quatf_In inVector2)
	{
		return Vec::V4IsEqualIntNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsCloseAll(Quatf_In inVector1, Quatf_In inVector2, float epsValue)
	{
		return Vec::V4IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseAll(Quatf_In inVector1, Quatf_In inVector2, Vec4f_In epsValues)
	{
		return Vec::V4IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsCloseNone(Quatf_In inVector1, Quatf_In inVector2, float epsValue)
	{
		return Vec::V4IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseNone(Quatf_In inVector1, Quatf_In inVector2, Vec4f_In epsValues)
	{
		return Vec::V4IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanAll(Quatf_In bigVector, Quatf_In smallVector)
	{
		return Vec::V4IsGreaterThanAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Quatf_In bigVector, Quatf_In smallVector)
	{
		return Vec::V4IsGreaterThanOrEqualAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanAll(Quatf_In smallVector, Quatf_In bigVector)
	{
		return Vec::V4IsLessThanAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Quatf_In smallVector, Quatf_In bigVector)
	{
		return Vec::V4IsLessThanOrEqualAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsZeroAll(Vec2f_In inVector)
	{
		return Vec::V2IsZeroAll( inVector.GetIntrin() );
	}

	__forceinline unsigned int IsZeroNone(Vec2f_In inVector)
	{
		return Vec::V2IsZeroNone( inVector.GetIntrin() );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( Vec2f_In inVector, Vec2f_In boundsVector )
	{
		return Vec::V2IsBetweenNegAndPosBounds( inVector.GetIntrin(), boundsVector.GetIntrin() );
	}

	__forceinline unsigned int IsEqualAll(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec::V2IsEqualAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualNone(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec::V2IsEqualNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntAll(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec::V2IsEqualIntAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntNone(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec::V2IsEqualIntNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsCloseAll(Vec2f_In inVector1, Vec2f_In inVector2, float epsValue)
	{
		return Vec::V2IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseAll(Vec2f_In inVector1, Vec2f_In inVector2, Vec2f_In epsValues)
	{
		return Vec::V2IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsCloseNone(Vec2f_In inVector1, Vec2f_In inVector2, float epsValue)
	{
		return Vec::V2IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseNone(Vec2f_In inVector1, Vec2f_In inVector2, Vec2f_In epsValues)
	{
		return Vec::V2IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanAll(Vec2f_In bigVector, Vec2f_In smallVector)
	{
		return Vec::V2IsGreaterThanAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Vec2f_In bigVector, Vec2f_In smallVector)
	{
		return Vec::V2IsGreaterThanOrEqualAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanAll(Vec2f_In smallVector, Vec2f_In bigVector)
	{
		return Vec::V2IsLessThanAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Vec2f_In smallVector, Vec2f_In bigVector)
	{
		return Vec::V2IsLessThanOrEqualAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsZeroAll(Vec3f_In inVector)
	{
		return Vec::V3IsZeroAll( inVector.GetIntrin() );
	}

	__forceinline unsigned int IsZeroNone(Vec3f_In inVector)
	{
		return Vec::V3IsZeroNone( inVector.GetIntrin() );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( Vec3f_In inVector, Vec3f_In boundsVector )
	{
		return Vec::V3IsBetweenNegAndPosBounds( inVector.GetIntrin(), boundsVector.GetIntrin() );
	}

	__forceinline unsigned int IsEqualAll(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec::V3IsEqualAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualNone(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec::V3IsEqualNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntAll(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec::V3IsEqualIntAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntNone(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec::V3IsEqualIntNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsCloseAll(Vec3f_In inVector1, Vec3f_In inVector2, float epsValue)
	{
		return Vec::V3IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseAll(Vec3f_In inVector1, Vec3f_In inVector2, Vec3f_In epsValues)
	{
		return Vec::V3IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsCloseNone(Vec3f_In inVector1, Vec3f_In inVector2, float epsValue)
	{
		return Vec::V3IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseNone(Vec3f_In inVector1, Vec3f_In inVector2, Vec3f_In epsValues)
	{
		return Vec::V3IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanAll(Vec3f_In bigVector, Vec3f_In smallVector)
	{
		return Vec::V3IsGreaterThanAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Vec3f_In bigVector, Vec3f_In smallVector)
	{
		return Vec::V3IsGreaterThanOrEqualAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanAll(Vec3f_In smallVector, Vec3f_In bigVector)
	{
		return Vec::V3IsLessThanAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Vec3f_In smallVector, Vec3f_In bigVector)
	{
		return Vec::V3IsLessThanOrEqualAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsZeroAll(Vec4f_In inVector)
	{
		return Vec::V4IsZeroAll( inVector.GetIntrin() );
	}

	__forceinline unsigned int IsZeroNone(Vec4f_In inVector)
	{
		return Vec::V4IsZeroNone( inVector.GetIntrin() );
	}

	__forceinline unsigned int IsBetweenNegAndPosBounds( Vec4f_In inVector, Vec4f_In boundsVector )
	{
		return Vec::V4IsBetweenNegAndPosBounds( inVector.GetIntrin(), boundsVector.GetIntrin() );
	}

	__forceinline unsigned int IsEqualAll(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec::V4IsEqualAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualNone(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec::V4IsEqualNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntAll(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec::V4IsEqualIntAll( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsEqualIntNone(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec::V4IsEqualIntNone( inVector1.GetIntrin(), inVector2.GetIntrin() );
	}

	__forceinline unsigned int IsCloseAll(Vec4f_In inVector1, Vec4f_In inVector2, float epsValue)
	{
		return Vec::V4IsCloseAll(inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseAll(Vec4f_In inVector1, Vec4f_In inVector2, Vec4f_In epsValues)
	{
		return Vec::V4IsCloseAll( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsCloseNone(Vec4f_In inVector1, Vec4f_In inVector2, float epsValue)
	{
		return Vec::V4IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValue );
	}

	__forceinline unsigned int IsCloseNone(Vec4f_In inVector1, Vec4f_In inVector2, Vec4f_In epsValues)
	{
		return Vec::V4IsCloseNone( inVector1.GetIntrin(), inVector2.GetIntrin(), epsValues.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanAll(Vec4f_In bigVector, Vec4f_In smallVector)
	{
		return Vec::V4IsGreaterThanAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsGreaterThanOrEqualAll(Vec4f_In bigVector, Vec4f_In smallVector)
	{
		return Vec::V4IsGreaterThanOrEqualAll( bigVector.GetIntrin(), smallVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanAll(Vec4f_In smallVector, Vec4f_In bigVector)
	{
		return Vec::V4IsLessThanAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	__forceinline unsigned int IsLessThanOrEqualAll(Vec4f_In smallVector, Vec4f_In bigVector)
	{
		return Vec::V4IsLessThanOrEqualAll( smallVector.GetIntrin(), bigVector.GetIntrin() );
	}

	//============================================================================
	// Standard quaternion math

	__forceinline Quatf_Out Conjugate(Quatf_In inQuat)
	{
		return Quatf( Vec::V4QuatConjugate( inQuat.GetIntrin() ) );
	}

	__forceinline Quatf_Out Normalize(Quatf_In inQuat)
	{
		return Quatf( Vec::V4QuatNormalize( inQuat.GetIntrin() ) );
	}

	__forceinline Quatf_Out NormalizeSafe(Quatf_In inQuat, float errVal)
	{
		return Quatf( Vec::V4QuatNormalizeSafe( inQuat.GetIntrin(), errVal ) );
	}

	__forceinline Quatf_Out NormalizeFast(Quatf_In inQuat)
	{
		return Quatf( Vec::V4QuatNormalizeFast( inQuat.GetIntrin() ) );
	}

	__forceinline Quatf_Out NormalizeFastSafe(Quatf_In inQuat, float errVal )
	{
		return Quatf( Vec::V4QuatNormalizeFastSafe( inQuat.GetIntrin(), errVal ) );
	}

	__forceinline Quatf_Out Invert(Quatf_In inQuat)
	{
		return Quatf( Vec::V4QuatInvert( inQuat.GetIntrin() ) );
	}

	__forceinline Quatf_Out InvertSafe(Quatf_In inQuat, float errVal)
	{
		return Quatf( Vec::V4QuatInvertSafe( inQuat.GetIntrin(), errVal ) );
	}

	__forceinline Quatf_Out InvertFast(Quatf_In inQuat)
	{
		return Quatf( Vec::V4QuatInvertFast( inQuat.GetIntrin() ) );
	}

	__forceinline Quatf_Out InvertFastSafe(Quatf_In inQuat, float errVal)
	{
		return Quatf( Vec::V4QuatInvertFastSafe( inQuat.GetIntrin(), errVal ) );
	}

	__forceinline Quatf_Out InvertNormInput(Quatf_In inQuat)
	{
		return Quatf( Vec::V4QuatInvertNormInput( inQuat.GetIntrin() ) );
	}

	__forceinline float Dot( Quatf_In inQuat1, Quatf_In inQuat2 )
	{
		return Vec::V4QuatDot( inQuat1.GetIntrin(), inQuat2.GetIntrin() );
	}

	__forceinline Quatf_Out Multiply( Quatf_In inQuat1, Quatf_In inQuat2 )
	{
		return Quatf( Vec::V4QuatMultiply( inQuat1.GetIntrin(), inQuat2.GetIntrin() ) );
	}

	__forceinline Quatf_Out SlerpNear( float t, Quatf_In inNormQuat1, Quatf_In inNormQuat2 )
	{
		return Quatf( Vec::V4QuatSlerpNear(t, inNormQuat1.GetIntrin(), inNormQuat2.GetIntrin()) );
	}

	__forceinline Quatf_Out Slerp( float t, Quatf_In inNormQuat1, Quatf_In inNormQuat2 )
	{
		return Quatf( Vec::V4QuatSlerp(t, inNormQuat1.GetIntrin(), inNormQuat2.GetIntrin()) );
	}

	__forceinline Quatf_Out Nlerp( float t, Quatf_In inNormQuat1, Quatf_In inNormQuat2 )
	{
		return Quatf( Vec::V4QuatNlerp( t, inNormQuat1.GetIntrin(), inNormQuat2.GetIntrin() ) );
	}

	__forceinline Quatf_Out PrepareSlerp( Quatf_In quat1, Quatf_In quatToNegate )
	{
		return Quatf( Vec::V4QuatPrepareSlerp( quat1.GetIntrin(), quatToNegate.GetIntrin() ) );
	}

	__forceinline void ToAxisAngle( Vec3f_InOut Axis, float& radians, Quatf_In inQuat )
	{
		Vec::V4QuatToAxisAnglef( Axis.GetIntrinRef(), radians, inQuat.GetIntrin() );
	}

	__forceinline float GetAngle( Quatf_In inQuat )
	{
		return Vec::V4QuatGetAnglef( inQuat.GetIntrin() );
	}

	//============================================================================
	// Conversion functions

	template <int exponent>
	__forceinline Vec2f_Out FloatToIntRaw(Vec2f_In inVec)
	{
		return Vec2f( Vec::V2FloatToIntRaw<exponent>( inVec.GetIntrin() ) );
	}

	template <int exponent>
	__forceinline Vec2f_Out IntToFloatRaw(Vec2f_In inVec)
	{
		return Vec2f( Vec::V2IntToFloatRaw<exponent>( inVec.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RoundToNearestInt(Vec2f_In inVec)
	{
		return Vec2f( Vec::V2RoundToNearestInt( inVec.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RoundToNearestIntZero(Vec2f_In inVec)
	{
		return Vec2f( Vec::V2RoundToNearestIntZero( inVec.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RoundToNearestIntNegInf(Vec2f_In inVec)
	{
		return Vec2f( Vec::V2RoundToNearestIntNegInf( inVec.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RoundToNearestIntPosInf(Vec2f_In inVec)
	{
		return Vec2f( Vec::V2RoundToNearestIntPosInf( inVec.GetIntrin() ) );
	}

	template <int exponent>
	__forceinline Vec3f_Out FloatToIntRaw(Vec3f_In inVec)
	{
		return Vec3f( Vec::V3FloatToIntRaw<exponent>( inVec.GetIntrin() ) );
	}

	template <int exponent>
	__forceinline Vec3f_Out IntToFloatRaw(Vec3f_In inVec)
	{
		return Vec3f( Vec::V3IntToFloatRaw<exponent>( inVec.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RoundToNearestInt(Vec3f_In inVec)
	{
		return Vec3f( Vec::V3RoundToNearestInt( inVec.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RoundToNearestIntZero(Vec3f_In inVec)
	{
		return Vec3f( Vec::V3RoundToNearestIntZero( inVec.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RoundToNearestIntNegInf(Vec3f_In inVec)
	{
		return Vec3f( Vec::V3RoundToNearestIntNegInf( inVec.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RoundToNearestIntPosInf(Vec3f_In inVec)
	{
		return Vec3f( Vec::V3RoundToNearestIntPosInf( inVec.GetIntrin() ) );
	}

	template <int exponent>
	__forceinline Vec4f_Out FloatToIntRaw(Vec4f_In inVec)
	{
		return Vec4f( Vec::V4FloatToIntRaw<exponent>( inVec.GetIntrin() ) );
	}

	template <int exponent>
	__forceinline Vec4f_Out IntToFloatRaw(Vec4f_In inVec)
	{
		return Vec4f( Vec::V4IntToFloatRaw<exponent>( inVec.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RoundToNearestInt(Vec4f_In inVec)
	{
		return Vec4f( Vec::V4RoundToNearestInt( inVec.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RoundToNearestIntZero(Vec4f_In inVec)
	{
		return Vec4f( Vec::V4RoundToNearestIntZero( inVec.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RoundToNearestIntNegInf(Vec4f_In inVec)
	{
		return Vec4f( Vec::V4RoundToNearestIntNegInf( inVec.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RoundToNearestIntPosInf(Vec4f_In inVec)
	{
		return Vec4f( Vec::V4RoundToNearestIntPosInf( inVec.GetIntrin() ) );
	}

	//============================================================================
	// Standard algebra

	__forceinline Vec2f_Out Clamp( Vec2f_In inVect, Vec2f_In lowBound, Vec2f_In highBound )
	{
		return Vec2f( Vec::V2Clamp( inVect.GetIntrin(), lowBound.GetIntrin(), highBound.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Clamp( Vec3f_In inVect, Vec3f_In lowBound, Vec3f_In highBound )
	{
		return Vec3f( Vec::V3Clamp( inVect.GetIntrin(), lowBound.GetIntrin(), highBound.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Clamp( Vec4f_In inVect, Vec4f_In lowBound, Vec4f_In highBound )
	{
		return Vec4f( Vec::V4Clamp( inVect.GetIntrin(), lowBound.GetIntrin(), highBound.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Saturate( Vec2f_In inVect )
	{
		return Vec2f( Vec::V2Saturate( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Saturate( Vec3f_In inVect )
	{
		return Vec3f( Vec::V3Saturate( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Saturate( Vec4f_In inVect )
	{
		return Vec4f( Vec::V4Saturate( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out ClampMag( Vec3f_In inVect, float minMag, float maxMag )
	{
		return Vec3f( Vec::V3ClampMag( inVect.GetIntrin(), minMag, maxMag ) );
	}

	__forceinline Vec2f_Out Negate(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2Negate( inVect.GetIntrin() ) );
	}
	__forceinline Vec3f_Out Negate(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3Negate( inVect.GetIntrin() ) );
	}
	__forceinline Vec4f_Out Negate(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4Negate( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvertBits(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2InvertBits( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out InvertBits(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3InvertBits( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvertBits(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4InvertBits( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Invert(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2Invert( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Invert(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3Invert( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Invert(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4Invert( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvertSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2InvertSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out InvertSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3InvertSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out InvertSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4InvertSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out InvertFast(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2InvertFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out InvertFast(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3InvertFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvertFast(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4InvertFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvertFastSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2InvertFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out InvertFastSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3InvertFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out InvertFastSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4InvertFastSafe( inVect.GetIntrin(), errVal ) );
	}

	//============================================================================
	// Magnitude
	
	__forceinline Vec2f_Out Abs(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2Abs( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Abs(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3Abs( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Abs(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4Abs( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Sqrt(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2Sqrt( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Sqrt(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3Sqrt( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Sqrt(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4Sqrt( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out SqrtSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2SqrtSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out SqrtSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3SqrtSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out SqrtSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4SqrtSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out SqrtFast(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2SqrtFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SqrtFast(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3SqrtFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SqrtFast(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4SqrtFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out SqrtFastSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2SqrtFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out SqrtFastSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3SqrtFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out SqrtFastSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4SqrtFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out InvSqrt(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2InvSqrt( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out InvSqrt(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3InvSqrt( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvSqrt(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4InvSqrt( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvSqrtSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2InvSqrtSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out InvSqrtSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3InvSqrtSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out InvSqrtSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4InvSqrtSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out InvSqrtFast(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2InvSqrtFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out InvSqrtFast(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3InvSqrtFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvSqrtFast(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4InvSqrtFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvSqrtFastSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2InvSqrtFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out InvSqrtFastSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3InvSqrtFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out InvSqrtFastSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4InvSqrtFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline float Mag(Vec2f_In inVect)
	{
		return Vec::V2Mag( inVect.GetIntrin() );
	}

	__forceinline float Mag(Vec3f_In inVect)
	{
		return Vec::V3Mag( inVect.GetIntrin() );
	}

	__forceinline float Mag(Vec4f_In inVect)
	{
		return Vec::V4Mag( inVect.GetIntrin() );
	}

	__forceinline float MagFast(Vec2f_In inVect)
	{
		return Vec::V2MagFast( inVect.GetIntrin() );
	}

	__forceinline float MagFast(Vec3f_In inVect)
	{
		return Vec::V3MagFast( inVect.GetIntrin() );
	}

	__forceinline float MagFast(Vec4f_In inVect)
	{
		return Vec::V4MagFast( inVect.GetIntrin() );
	}

	__forceinline float MagSquared(Vec2f_In inVect)
	{
		return Vec::V2MagSquared( inVect.GetIntrin() );
	}

	__forceinline float MagSquared(Vec3f_In inVect)
	{
		return Vec::V3MagSquared( inVect.GetIntrin() );
	}

	__forceinline float MagSquared(Vec4f_In inVect)
	{
		return Vec::V4MagSquared( inVect.GetIntrin() );
	}

	__forceinline float InvMag(Vec2f_In inVect)
	{
		return Vec::V2InvMag( inVect.GetIntrin() );
	}

	__forceinline float InvMag(Vec3f_In inVect)
	{
		return Vec::V3InvMag( inVect.GetIntrin() );
	}

	__forceinline float InvMag(Vec4f_In inVect)
	{
		return Vec::V4InvMag( inVect.GetIntrin() );
	}

	__forceinline float InvMagSafe(Vec2f_In inVect, float errVal)
	{
		return Vec::V2InvMagSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSafe(Vec3f_In inVect, float errVal)
	{
		return Vec::V3InvMagSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSafe(Vec4f_In inVect, float errVal)
	{
		return Vec::V4InvMagSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagFast(Vec2f_In inVect)
	{
		return Vec::V2InvMagFast( inVect.GetIntrin() );
	}

	__forceinline float InvMagFast(Vec3f_In inVect)
	{
		return Vec::V3InvMagFast( inVect.GetIntrin() );
	}

	__forceinline float InvMagFast(Vec4f_In inVect)
	{
		return Vec::V4InvMagFast( inVect.GetIntrin() );
	}

	__forceinline float InvMagFastSafe(Vec2f_In inVect, float errVal)
	{
		return Vec::V2InvMagFastSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagFastSafe(Vec3f_In inVect, float errVal)
	{
		return Vec::V3InvMagFastSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagFastSafe(Vec4f_In inVect, float errVal)
	{
		return Vec::V4InvMagFastSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSquared(Vec2f_In inVect)
	{
		return Vec::V2InvMagSquared( inVect.GetIntrin() );
	}

	__forceinline float InvMagSquared(Vec3f_In inVect)
	{
		return Vec::V3InvMagSquared( inVect.GetIntrin() );
	}

	__forceinline float InvMagSquared(Vec4f_In inVect)
	{
		return Vec::V4InvMagSquared( inVect.GetIntrin() );
	}

	__forceinline float InvMagSquaredSafe(Vec2f_In inVect, float errVal)
	{
		return Vec::V2InvMagSquaredSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSquaredSafe(Vec3f_In inVect, float errVal)
	{
		return Vec::V3InvMagSquaredSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSquaredSafe(Vec4f_In inVect, float errVal)
	{
		return Vec::V4InvMagSquaredSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSquaredFast(Vec2f_In inVect)
	{
		return Vec::V2InvMagSquaredFast( inVect.GetIntrin() );
	}

	__forceinline float InvMagSquaredFast(Vec3f_In inVect)
	{
		return Vec::V3InvMagSquaredFast( inVect.GetIntrin() );
	}

	__forceinline float InvMagSquaredFast(Vec4f_In inVect)
	{
		return Vec::V4InvMagSquaredFast( inVect.GetIntrin() );
	}

	__forceinline float InvMagSquaredFastSafe(Vec2f_In inVect, float errVal)
	{
		return Vec::V2InvMagSquaredFastSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSquaredFastSafe(Vec3f_In inVect, float errVal)
	{
		return Vec::V3InvMagSquaredFastSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline float InvMagSquaredFastSafe(Vec4f_In inVect, float errVal)
	{
		return Vec::V4InvMagSquaredFastSafe( inVect.GetIntrin(), errVal );
	}

	__forceinline Vec2f_Out Normalize(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2Normalize( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Normalize(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3Normalize( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Normalize(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4Normalize( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out NormalizeSafe(Vec2f_In inVect, float errVal)
	{
		return Vec2f( Vec::V2NormalizeSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out NormalizeSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3NormalizeSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out NormalizeSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4NormalizeSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out NormalizeFast(Vec2f_In inVect)
	{
		return Vec2f( Vec::V2NormalizeFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec3f_Out NormalizeFast(Vec3f_In inVect)
	{
		return Vec3f( Vec::V3NormalizeFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec4f_Out NormalizeFast(Vec4f_In inVect)
	{
		return Vec4f( Vec::V4NormalizeFast( inVect.GetIntrin() ) );
	}

	__forceinline Vec2f_Out NormalizeFastSafe(Vec2f_In inVect, float errVal )
	{
		return Vec2f( Vec::V2NormalizeFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out NormalizeFastSafe(Vec3f_In inVect, float errVal)
	{
		return Vec3f( Vec::V3NormalizeFastSafe( inVect.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out NormalizeFastSafe(Vec4f_In inVect, float errVal)
	{
		return Vec4f( Vec::V4NormalizeFastSafe( inVect.GetIntrin(), errVal ) );
	}

	//============================================================================
	// Angular

	__forceinline Vec2f_Out Extend( Vec2f_In inVect, Vec2f_In amount )
	{
		return Vec2f( Vec::V2Extend( inVect.GetIntrin(), amount.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Rotate( Vec2f_In inVect, float radians )
	{
		return Vec2f( Vec::V2Rotate( inVect.GetIntrin(), radians ) );
	}

	__forceinline Vec2f_Out Reflect( Vec2f_In inVect, Vec2f_In wall2DNormal )
	{
		return Vec2f( Vec::V2Reflect( inVect.GetIntrin(), wall2DNormal.GetIntrin() ) );
	}

	__forceinline Vec2f_Out ApproachStraight(Vec2f_In inVect, Vec2f_In goal, float rate, float time, unsigned int& rResult)
	{
		return Vec2f( Vec::V2ApproachStraight( inVect.GetIntrin(), goal.GetIntrin(), rate, time, rResult ) );
	}

	__forceinline Vec3f_Out Extend( Vec3f_In inVect, Vec3f_In amount )
	{
		return Vec3f( Vec::V3Extend( inVect.GetIntrin(), amount.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RotateAboutXAxis( Vec3f_In inVect, float radians )
	{
		return Vec3f( Vec::V3RotateAboutXAxis( inVect.GetIntrin(), radians ) );
	}

	__forceinline Vec3f_Out RotateAboutYAxis( Vec3f_In inVect, float radians )
	{
		return Vec3f( Vec::V3RotateAboutYAxis( inVect.GetIntrin(), radians ) );
	}

	__forceinline Vec3f_Out RotateAboutZAxis( Vec3f_In inVect, float radians )
	{
		return Vec3f( Vec::V3RotateAboutZAxis( inVect.GetIntrin(), radians ) );
	}

	__forceinline Vec3f_Out Reflect( Vec3f_In inVect, Vec3f_In planeNormal )
	{
		return Vec3f( Vec::V3Reflect( inVect.GetIntrin(), planeNormal.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SlowInOut( Vec4f_In t )
	{
		return Vec4f( Vec::V4SlowInOut( t.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SlowInOut( Vec3f_In t )
	{
		return Vec3f( Vec::V3SlowInOut( t.GetIntrin() ) );
	}
	__forceinline Vec2f_Out SlowInOut( Vec2f_In t )
	{
		return Vec2f( Vec::V2SlowInOut( t.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SlowIn( Vec4f_In t )
	{
		return Vec4f( Vec::V4SlowIn( t.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SlowIn( Vec3f_In t )
	{
		return Vec3f( Vec::V3SlowIn( t.GetIntrin() ) );
	}

	__forceinline Vec2f_Out SlowIn( Vec2f_In t )
	{
		return Vec2f( Vec::V2SlowIn( t.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SlowOut( Vec4f_In t )
	{
		return Vec4f( Vec::V4SlowOut( t.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SlowOut( Vec3f_In t )
	{
		return Vec3f( Vec::V3SlowOut( t.GetIntrin() ) );
	}

	__forceinline Vec2f_Out SlowOut( Vec2f_In t )
	{
		return Vec2f( Vec::V2SlowOut( t.GetIntrin() ) );
	}

	__forceinline Vec4f_Out BellInOut( Vec4f_In t )
	{
		return Vec4f( Vec::V4BellInOut( t.GetIntrin() ) );
	}

	__forceinline Vec3f_Out BellInOut( Vec3f_In t )
	{
		return Vec3f( Vec::V3BellInOut( t.GetIntrin() ) );
	}

	__forceinline Vec2f_Out BellInOut( Vec2f_In t )
	{
		return Vec2f( Vec::V2BellInOut( t.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Range( Vec4f_In t, Vec4f_In lower, Vec4f_In upper )
	{
		return Vec4f( Vec::V4Range( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Range( Vec3f_In t, Vec3f_In lower, Vec3f_In upper )
	{
		return Vec3f( Vec::V3Range( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Range( Vec2f_In t, Vec2f_In lower, Vec2f_In upper )
	{
		return Vec2f( Vec::V2Range( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RangeFast( Vec4f_In t, Vec4f_In lower, Vec4f_In upper )
	{
		return Vec4f( Vec::V4RangeFast( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RangeFast( Vec3f_In t, Vec3f_In lower, Vec3f_In upper )
	{
		return Vec3f( Vec::V3RangeFast( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RangeFast( Vec2f_In t, Vec2f_In lower, Vec2f_In upper )
	{
		return Vec2f( Vec::V2RangeFast( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RangeClamp( Vec4f_In t, Vec4f_In lower, Vec4f_In upper )
	{
		return Vec4f( Vec::V4RangeClamp( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RangeClamp( Vec3f_In t, Vec3f_In lower, Vec3f_In upper )
	{
		return Vec3f( Vec::V3RangeClamp( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RangeClamp( Vec2f_In t, Vec2f_In lower, Vec2f_In upper )
	{
		return Vec2f( Vec::V2RangeClamp( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RangeClampFast( Vec4f_In t, Vec4f_In lower, Vec4f_In upper )
	{
		return Vec4f( Vec::V4RangeClampFast( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RangeClampFast( Vec3f_In t, Vec3f_In lower, Vec3f_In upper )
	{
		return Vec3f( Vec::V3RangeClampFast( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RangeClampFast( Vec2f_In t, Vec2f_In lower, Vec2f_In upper )
	{
		return Vec2f( Vec::V2RangeClampFast( t.GetIntrin(), lower.GetIntrin(), upper.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Ramp( Vec4f_In x, Vec4f_In funcInA, Vec4f_In funcInB, Vec4f_In funcOutA, Vec4f_In funcOutB )
	{
		return Vec4f( Vec::V4Ramp( x.GetIntrin(), funcInA.GetIntrin(), funcInB.GetIntrin(), funcOutA.GetIntrin(), funcOutB.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Ramp( Vec3f_In x, Vec3f_In funcInA, Vec3f_In funcInB, Vec3f_In funcOutA, Vec3f_In funcOutB )
	{
		return Vec3f( Vec::V3Ramp( x.GetIntrin(), funcInA.GetIntrin(), funcInB.GetIntrin(), funcOutA.GetIntrin(), funcOutB.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Ramp( Vec2f_In x, Vec2f_In funcInA, Vec2f_In funcInB, Vec2f_In funcOutA, Vec2f_In funcOutB )
	{
		return Vec2f( Vec::V2Ramp( x.GetIntrin(), funcInA.GetIntrin(), funcInB.GetIntrin(), funcOutA.GetIntrin(), funcOutB.GetIntrin() ) );
	}

	__forceinline Vec4f_Out RampFast( Vec4f_In x, Vec4f_In funcInA, Vec4f_In funcInB, Vec4f_In funcOutA, Vec4f_In funcOutB )
	{
		return Vec4f( Vec::V4Ramp( x.GetIntrin(), funcInA.GetIntrin(), funcInB.GetIntrin(), funcOutA.GetIntrin(), funcOutB.GetIntrin() ) );
	}

	__forceinline Vec3f_Out RampFast( Vec3f_In x, Vec3f_In funcInA, Vec3f_In funcInB, Vec3f_In funcOutA, Vec3f_In funcOutB )
	{
		return Vec3f( Vec::V3Ramp( x.GetIntrin(), funcInA.GetIntrin(), funcInB.GetIntrin(), funcOutA.GetIntrin(), funcOutB.GetIntrin() ) );
	}

	__forceinline Vec2f_Out RampFast( Vec2f_In x, Vec2f_In funcInA, Vec2f_In funcInB, Vec2f_In funcOutA, Vec2f_In funcOutB )
	{
		return Vec2f( Vec::V2Ramp( x.GetIntrin(), funcInA.GetIntrin(), funcInB.GetIntrin(), funcOutA.GetIntrin(), funcOutB.GetIntrin() ) );
	}

	__forceinline Vec3f_Out AddNet( Vec3f_In inVector, Vec3f_In toAdd )
	{
		return Vec3f( Vec::V3AddNet( inVector.GetIntrin(), toAdd.GetIntrin() ) );
	}

	__forceinline float Angle(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3Angle( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleNormInput(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleNormInput( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleX(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleX( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleY(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleY( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleZ(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleZ( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleXNormInput(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleXNormInput( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleYNormInput(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleYNormInput( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline float AngleZNormInput(Vec3f_In v1, Vec3f_In v2)
	{
		return Vec::V3AngleZNormInput( v1.GetIntrin(), v2.GetIntrin() );
	}

	__forceinline void MakeOrthonormals(Vec3f_In inVector, Vec3f_InOut ortho1, Vec3f_InOut ortho2)
	{
		Vec::V3MakeOrthonormals( inVector.GetIntrin(), ortho1.GetIntrinRef(), ortho2.GetIntrinRef() );
	}

	__forceinline Vec3f_Out ApproachStraight(Vec3f_In inVect, Vec3f_In goal, float rate, float time, unsigned int& rResult)
	{
		return Vec3f( Vec::V3ApproachStraight( inVect.GetIntrin(), goal.GetIntrin(), rate, time, rResult ) );
	}

	__forceinline Vec2f_Out AddNet( Vec2f_In inVector, Vec2f_In toAdd )
	{
		return Vec2f( Vec::V2AddNet( inVector.GetIntrin(), toAdd.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Angle(Vec2f_In v1, Vec2f_In v2)
	{
		return Vec2FromF32( Vec::V2Angle( v1.GetIntrin(), v2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out AngleNormInput(Vec2f_In v1, Vec2f_In v2)
	{
		return Vec2FromF32( Vec::V2AngleNormInput( v1.GetIntrin(), v2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out WhichSideOfLineV(Vec2f_In point, Vec2f_In lineP1, Vec2f_In lineP2)
	{
		return Vec2FromF32( Vec::V2WhichSideOfLine( point.GetIntrin(), lineP1.GetIntrin(), lineP2.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Splat( float a )
	{
		return Vec4FromF32( a );
	}

	__forceinline Vec4f_Out Scale( Vec4f_In a, float b )
	{
		return Vec4f( Vec::V4Scale( a.GetIntrin(), b ) );
	}

	__forceinline Vec4f_Out Scale( float a, Vec4f_In b )
	{
		return Vec4f( Vec::V4Scale( b.GetIntrin(), a ) );
	}

	__forceinline Vec3f_Out Scale( Vec3f_In a, float b )
	{
		return Vec3f( Vec::V3Scale( a.GetIntrin(), b ) );
	}

	__forceinline Vec3f_Out Scale( float a, Vec3f_In b )
	{
		return Vec3f( Vec::V3Scale( b.GetIntrin(), a ) );
	}

	__forceinline Vec2f_Out Scale( Vec2f_In a, float b )
	{
		return Vec2f( Vec::V2Scale( a.GetIntrin(), b ) );
	}

	__forceinline Vec2f_Out Scale( float a, Vec2f_In b )
	{
		return Vec2f( Vec::V2Scale( b.GetIntrin(), a ) );
	}

	__forceinline Vec4f_Out Scale( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V4Scale( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Scale( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3Scale( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Scale( Vec2f_In a, Vec2f_In b )
	{
		return Vec2f( Vec::V2Scale( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Cross3( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V3Cross( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Cross( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3Cross( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline float Cross( Vec2f_In a, Vec2f_In b )
	{
		return Vec::V2Cross( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline Vec3f_Out AddCrossed( Vec3f_In toAddTo, Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3AddCrossed( toAddTo.GetIntrin(), a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec4f_Out AddCrossed3( Vec4f_In toAddTo, Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V3AddCrossed( toAddTo.GetIntrin(), a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SubtractCrossed( Vec3f_In toSubtractFrom, Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3SubtractCrossed( toSubtractFrom.GetIntrin(), a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SubtractCrossed3( Vec4f_In toSubtractFrom, Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V3SubtractCrossed( toSubtractFrom.GetIntrin(), a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline float Dot( Vec4f_In a, Vec4f_In b )
	{
		return Vec::V4Dot( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float Dot( Vec3f_In a, Vec3f_In b )
	{
		return Vec::V3Dot( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float Dot( Vec2f_In a, Vec2f_In b )
	{
		return Vec::V2Dot( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline Vec4f_Out AddInt( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V4AddInt( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec3f_Out AddInt( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3AddInt( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec2f_Out AddInt( Vec2f_In a, Vec2f_In b )
	{
		return Vec2f( Vec::V2AddInt( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SubtractInt( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V4SubtractInt( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SubtractInt( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3SubtractInt( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec2f_Out SubtractInt( Vec2f_In a, Vec2f_In b )
	{
		return Vec2f( Vec::V2SubtractInt( a.GetIntrin(), b.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Add( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V4Add(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec3f_Out Add( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3Add(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec2f_Out Add( Vec2f_In a, Vec2f_In b )
	{
		return Vec2f( Vec::V2Add(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec4f_Out Subtract( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V4Subtract(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec3f_Out Subtract( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3Subtract(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec2f_Out Subtract( Vec2f_In a, Vec2f_In b )
	{
		return Vec2f( Vec::V2Subtract(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec4f_Out Average( Vec4f_In a, Vec4f_In b )
	{
		return Vec4f( Vec::V4Average(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec3f_Out Average( Vec3f_In a, Vec3f_In b )
	{
		return Vec3f( Vec::V3Average(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec2f_Out Average( Vec2f_In a, Vec2f_In b )
	{
		return Vec2f( Vec::V2Average(a.GetIntrin(), b.GetIntrin()) );
	}

	__forceinline Vec4f_Out AddScaled( Vec4f_In toAdd, Vec4f_In toScaleThenAdd, float scaleValue )
	{
		return Vec4f( Vec::V4AddScaled( toAdd.GetIntrin(), toScaleThenAdd.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec4f_Out AddScaled( Vec4f_In toAdd, Vec4f_In toScaleThenAdd, Vec4f_In scaleValues )
	{
		return Vec4f( Vec::V4AddScaled( toAdd.GetIntrin(), toScaleThenAdd.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec3f_Out AddScaled( Vec3f_In toAdd, Vec3f_In toScaleThenAdd, float scaleValue )
	{
		return Vec3f( Vec::V3AddScaled( toAdd.GetIntrin(), toScaleThenAdd.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec3f_Out AddScaled( Vec3f_In toAdd, Vec3f_In toScaleThenAdd, Vec3f_In scaleValues )
	{
		return Vec3f( Vec::V3AddScaled( toAdd.GetIntrin(), toScaleThenAdd.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec2f_Out AddScaled( Vec2f_In toAdd, Vec2f_In toScaleThenAdd, float scaleValue )
	{
		return Vec2f( Vec::V2AddScaled( toAdd.GetIntrin(), toScaleThenAdd.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec2f_Out AddScaled( Vec2f_In toAdd, Vec2f_In toScaleThenAdd, Vec2f_In scaleValues )
	{
		return Vec2f( Vec::V2AddScaled( toAdd.GetIntrin(), toScaleThenAdd.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec4f_Out SubtractScaled( Vec4f_In toSubtractFrom, Vec4f_In toScaleThenSubtract, float scaleValue )
	{
		return Vec4f( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin(), toScaleThenSubtract.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec4f_Out SubtractScaled( Vec4f_In toSubtractFrom, Vec4f_In toScaleThenSubtract, Vec4f_In scaleValues )
	{
		return Vec4f( Vec::V4SubtractScaled( toSubtractFrom.GetIntrin(), toScaleThenSubtract.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec3f_Out SubtractScaled( Vec3f_In toSubtractFrom, Vec3f_In toScaleThenSubtract, float scaleValue )
	{
		return Vec3f( Vec::V3SubtractScaled( toSubtractFrom.GetIntrin(), toScaleThenSubtract.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec3f_Out SubtractScaled( Vec3f_In toSubtractFrom, Vec3f_In toScaleThenSubtract, Vec3f_In scaleValues )
	{
		return Vec3f( Vec::V3SubtractScaled( toSubtractFrom.GetIntrin(), toScaleThenSubtract.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec2f_Out SubtractScaled( Vec2f_In toSubtractFrom, Vec2f_In toScaleThenSubtract, float scaleValue )
	{
		return Vec2f( Vec::V2SubtractScaled( toSubtractFrom.GetIntrin(), toScaleThenSubtract.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec2f_Out SubtractScaled( Vec2f_In toSubtractFrom, Vec2f_In toScaleThenSubtract, Vec2f_In scaleValues )
	{
		return Vec2f( Vec::V2SubtractScaled( toSubtractFrom.GetIntrin(), toScaleThenSubtract.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvScale( Vec4f_In toScale, float scaleValue )
	{
		return Vec4f( Vec::V4InvScale( toScale.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec4f_Out InvScale( Vec4f_In toScale, Vec4f_In scaleValues )
	{
		return Vec4f( Vec::V4InvScale( toScale.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvScaleSafe( Vec4f_In toScale, float scaleValue, float errVal )
	{
		return Vec4f( Vec::V4InvScaleSafe( toScale.GetIntrin(), scaleValue, errVal ) );
	}

	__forceinline Vec4f_Out InvScaleSafe( Vec4f_In toScale, Vec4f_In scaleValues, float errVal )
	{
		return Vec4f( Vec::V4InvScaleSafe( toScale.GetIntrin(), scaleValues.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out InvScaleFast( Vec4f_In toScale, float scaleValue )
	{
		return Vec4f( Vec::V4InvScaleFast( toScale.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec4f_Out InvScaleFast( Vec4f_In toScale, Vec4f_In scaleValues )
	{
		return Vec4f( Vec::V4InvScaleFast( toScale.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec4f_Out InvScaleFastSafe( Vec4f_In toScale, float scaleValue, float errVal )
	{
		return Vec4f( Vec::V4InvScaleFastSafe( toScale.GetIntrin(), scaleValue, errVal ) );
	}

	__forceinline Vec4f_Out InvScaleFastSafe( Vec4f_In toScale, Vec4f_In scaleValues, float errVal )
	{
		return Vec4f( Vec::V4InvScaleFastSafe( toScale.GetIntrin(), scaleValues.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out InvScale( Vec3f_In toScale, float scaleValue )
	{
		return Vec3f( Vec::V3InvScale( toScale.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec3f_Out InvScale( Vec3f_In toScale, Vec3f_In scaleValues )
	{
		return Vec3f( Vec::V3InvScale( toScale.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec3f_Out InvScaleSafe( Vec3f_In toScale, float scaleValue, float errVal )
	{
		return Vec3f( Vec::V3InvScaleSafe( toScale.GetIntrin(), scaleValue, errVal ) );
	}

	__forceinline Vec3f_Out InvScaleSafe( Vec3f_In toScale, Vec3f_In scaleValues, float errVal )
	{
		return Vec3f( Vec::V3InvScaleSafe( toScale.GetIntrin(), scaleValues.GetIntrin(), errVal ) );
	}

	__forceinline Vec3f_Out InvScaleFast( Vec3f_In toScale, float scaleValue )
	{
		return Vec3f( Vec::V3InvScaleFast( toScale.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec3f_Out InvScaleFast( Vec3f_In toScale, Vec3f_In scaleValues )
	{
		return Vec3f( Vec::V3InvScaleFast( toScale.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec3f_Out InvScaleFastSafe( Vec3f_In toScale, float scaleValue, float errVal )
	{
		return Vec3f( Vec::V3InvScaleFastSafe( toScale.GetIntrin(), scaleValue, errVal ) );
	}

	__forceinline Vec3f_Out InvScaleFastSafe( Vec3f_In toScale, Vec3f_In scaleValues, float errVal )
	{
		return Vec3f( Vec::V3InvScaleFastSafe( toScale.GetIntrin(), scaleValues.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out InvScale( Vec2f_In toScale, float scaleValue )
	{
		return Vec2f( Vec::V2InvScale( toScale.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec2f_Out InvScale( Vec2f_In toScale, Vec2f_In scaleValues )
	{
		return Vec2f( Vec::V2InvScale( toScale.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvScaleSafe( Vec2f_In toScale, float scaleValue, float errVal )
	{
		return Vec2f( Vec::V2InvScaleSafe( toScale.GetIntrin(), scaleValue, errVal ) );
	}

	__forceinline Vec2f_Out InvScaleSafe( Vec2f_In toScale, Vec2f_In scaleValues, float errVal )
	{
		return Vec2f( Vec::V2InvScaleSafe( toScale.GetIntrin(), scaleValues.GetIntrin(), errVal ) );
	}

	__forceinline Vec2f_Out InvScaleFast( Vec2f_In toScale, float scaleValue )
	{
		return Vec2f( Vec::V2InvScaleFast( toScale.GetIntrin(), scaleValue ) );
	}

	__forceinline Vec2f_Out InvScaleFast( Vec2f_In toScale, Vec2f_In scaleValues )
	{
		return Vec2f( Vec::V2InvScaleFast( toScale.GetIntrin(), scaleValues.GetIntrin() ) );
	}

	__forceinline Vec2f_Out InvScaleFastSafe( Vec2f_In toScale, float scaleValue, float errVal )
	{
		return Vec2f( Vec::V2InvScaleFastSafe( toScale.GetIntrin(), scaleValue, errVal ) );
	}

	__forceinline Vec2f_Out InvScaleFastSafe( Vec2f_In toScale, Vec2f_In scaleValues, float errVal )
	{
		return Vec2f( Vec::V2InvScaleFastSafe( toScale.GetIntrin(), scaleValues.GetIntrin(), errVal ) );
	}

	__forceinline Vec4f_Out Lerp( float tValue, Vec4f_In vectA, Vec4f_In vectB )
	{
		return Vec4f( Vec::V4Lerp( tValue, vectA.GetIntrin(), vectB.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Lerp( Vec4f_In tValues, Vec4f_In vectA, Vec4f_In vectB )
	{
		return Vec4f( Vec::V4Lerp( tValues.GetIntrin(), vectA.GetIntrin(), vectB.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Lerp( float tValue, Vec3f_In vectA, Vec3f_In vectB )
	{
		return Vec3f( Vec::V3Lerp( tValue, vectA.GetIntrin(), vectB.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Lerp( Vec3f_In tValues, Vec3f_In vectA, Vec3f_In vectB )
	{
		return Vec3f( Vec::V3Lerp( tValues.GetIntrin(), vectA.GetIntrin(), vectB.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Lerp( float tValue, Vec2f_In vectA, Vec2f_In vectB )
	{
		return Vec2f( Vec::V2Lerp( tValue, vectA.GetIntrin(), vectB.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Lerp( Vec2f_In tValues, Vec2f_In vectA, Vec2f_In vectB )
	{
		return Vec2f( Vec::V2Lerp( tValues.GetIntrin(), vectA.GetIntrin(), vectB.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Pow( Vec4f_In x, Vec4f_In y )
	{
		return Vec4f( Vec::V4Pow( x.GetIntrin(), y.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Pow( Vec3f_In x, Vec3f_In y )
	{
		return Vec3f( Vec::V3Pow( x.GetIntrin(), y.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Pow( Vec2f_In x, Vec2f_In y )
	{
		return Vec2f( Vec::V2Pow( x.GetIntrin(), y.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Expt( Vec4f_In x )
	{
		return Vec4f( Vec::V4Expt( x.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Expt( Vec3f_In x )
	{
		return Vec3f( Vec::V3Expt( x.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Expt( Vec2f_In x )
	{
		return Vec2f( Vec::V2Expt( x.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Log2( Vec4f_In x )
	{
		return Vec4f( Vec::V4Log2( x.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Log2( Vec3f_In x )
	{
		return Vec3f( Vec::V3Log2( x.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Log2( Vec2f_In x )
	{
		return Vec2f( Vec::V2Log2( x.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Log10( Vec4f_In x )
	{
		return Vec4f( Vec::V4Log10( x.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Log10( Vec3f_In x )
	{
		return Vec3f( Vec::V3Log10( x.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Log10( Vec2f_In x )
	{
		return Vec2f( Vec::V2Log10( x.GetIntrin() ) );
	}

	__forceinline float Dist(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4Dist( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float DistFast(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4DistFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float Dist(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3Dist( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float DistFast(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3DistFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float Dist(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2Dist( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float DistFast(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2DistFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDist(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4InvDist( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSafe(Vec4f_In a, Vec4f_In b, float errVal)
	{
		return Vec::V4InvDistSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistFast(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4InvDistFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistFastSafe(Vec4f_In a, Vec4f_In b, float errVal)
	{
		return Vec::V4InvDistFastSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDist(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3InvDist( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSafe(Vec3f_In a, Vec3f_In b, float errVal)
	{
		return Vec::V3InvDistSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistFast(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3InvDistFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistFastSafe(Vec3f_In a, Vec3f_In b, float errVal)
	{
		return Vec::V3InvDistFastSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDist(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2InvDist( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSafe(Vec2f_In a, Vec2f_In b, float errVal)
	{
		return Vec::V2InvDistSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistFast(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2InvDistFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistFastSafe(Vec2f_In a, Vec2f_In b, float errVal)
	{
		return Vec::V2InvDistFastSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float DistSquared(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4DistSquared( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float DistSquared(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3DistSquared( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float DistSquared(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2DistSquared( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquared(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4InvDistSquared( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquaredSafe(Vec4f_In a, Vec4f_In b, float errVal)
	{
		return Vec::V4InvDistSquaredSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistSquaredFast(Vec4f_In a, Vec4f_In b)
	{
		return Vec::V4InvDistSquaredFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquaredFastSafe(Vec4f_In a, Vec4f_In b, float errVal)
	{
		return Vec::V4InvDistSquaredFastSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistSquared(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3InvDistSquared( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquaredSafe(Vec3f_In a, Vec3f_In b, float errVal)
	{
		return Vec::V3InvDistSquaredSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistSquaredFast(Vec3f_In a, Vec3f_In b)
	{
		return Vec::V3InvDistSquaredFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquaredFastSafe(Vec3f_In a, Vec3f_In b, float errVal)
	{
		return Vec::V3InvDistSquaredFastSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistSquared(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2InvDistSquared( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquaredSafe(Vec2f_In a, Vec2f_In b, float errVal)
	{
		return Vec::V2InvDistSquaredSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline float InvDistSquaredFast(Vec2f_In a, Vec2f_In b)
	{
		return Vec::V2InvDistSquaredFast( a.GetIntrin(), b.GetIntrin() );
	}

	__forceinline float InvDistSquaredFastSafe(Vec2f_In a, Vec2f_In b, float errVal)
	{
		return Vec::V2InvDistSquaredFastSafe( a.GetIntrin(), b.GetIntrin(), errVal );
	}

	__forceinline Vec4f_Out Max(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec4f( Vec::V4Max( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Max(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec3f( Vec::V3Max( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Max(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec2f( Vec::V2Max( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Min(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec4f( Vec::V4Min( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Min(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec3f( Vec::V3Min( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Min(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec2f( Vec::V2Min( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec4f_Out And(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec4f( Vec::V4And( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out And(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec3f( Vec::V3And( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out And(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec2f( Vec::V2And( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Or(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec4f( Vec::V4Or( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Or(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec3f( Vec::V3Or( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Or(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec2f( Vec::V2Or( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Xor(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec4f( Vec::V4Xor( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Xor(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec3f( Vec::V3Xor( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Xor(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec2f( Vec::V2Xor( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec4f_Out Andc(Vec4f_In inVector1, Vec4f_In inVector2)
	{
		return Vec4f( Vec::V4Andc( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Andc(Vec3f_In inVector1, Vec3f_In inVector2)
	{
		return Vec3f( Vec::V3Andc( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec2f_Out Andc(Vec2f_In inVector1, Vec2f_In inVector2)
	{
		return Vec2f( Vec::V2Andc( inVector1.GetIntrin(), inVector2.GetIntrin() ) );
	}

	__forceinline Vec3f_Out Transform( Quatf_In unitQuat, Vec3f_In inVect )
	{
		return Vec3f( Vec::V3QuatRotate( inVect.GetIntrin(), unitQuat.GetIntrin() ) );
	}

	__forceinline Vec3f_Out UnTransform( Quatf_In unitQuat, Vec3f_In inVect )
	{
		return Vec3f( Vec::V3QuatRotateReverse( inVect.GetIntrin(), unitQuat.GetIntrin() ) );
	}

} // namespace rage
