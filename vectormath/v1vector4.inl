
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{


#ifndef V1SET_1F__4
	__forceinline void V1Set( Vector_4_InOut inoutVector, float x0 )
	{
		inoutVector.x = x0;
	}
#endif // V1SET_1F__4

#ifndef V1SET_V__4
	__forceinline void V1Set( Vector_4_InOut inoutVector, Vector_4_In inVector )
	{
		inoutVector.x = inVector.x;
	}
#endif // V1SET_V__4

	//============================================================================
	// Standard Algebra

#ifndef V1SCALE__4
	__forceinline Vector_4_Out V1Scale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * floatVal;
		return outVect;
	}
#endif // V1SCALE__4

#ifndef V1SCALE_V__4
	__forceinline Vector_4_Out V1Scale( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x * inVector2.x;
		return outVect;
	}
#endif // V1SCALE_V__4

#ifndef V1INVSCALE__4
	__forceinline Vector_4_Out V1InvScale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x / floatVal;
		return outVect;
	}
#endif // V1INVSCALE__4

#ifndef V1INVSCALE_V__4
	__forceinline Vector_4_Out V1InvScale( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x / floatVal.x;
		return outVect;
	}
#endif // V1INVSCALE_V__4

#ifndef V1INVSCALESAFE__4
	__forceinline Vector_4_Out V1InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPIfEqThenElse(floatVal, 0.0f, errVal, inVector.x / floatVal);
		return outVect;
	}
#endif // V1INVSCALESAFE__4

#ifndef V1INVSCALESAFE_V__4
	__forceinline Vector_4_Out V1InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal  )
	{
		Vector_4 outVect;
		outVect.x = FPIfEqThenElse(floatVal, 0.0f, errVal, inVector.x / floatVal.x);
		return outVect;
	}
#endif // V1INVSCALESAFE_V__4

#ifndef V1INVSCALEFAST__4
	__forceinline Vector_4_Out V1InvScaleFast( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal);
		return outVect;
	}
#endif // V1INVSCALEFAST__4

#ifndef V1INVSCALEFAST_V__4
	__forceinline Vector_4_Out V1InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal.x);
		return outVect;
	}
#endif // V1INVSCALEFAST_V__4

#ifndef V1INVSCALEFASTSAFE__4
	__forceinline Vector_4_Out V1InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPIfEqThenElse(floatVal, 0.0f, errVal, inVector.x*FPInvertFast(floatVal));
		return outVect;
	}
#endif // V1INVSCALEFASTSAFE__4

#ifndef V1INVSCALEFASTSAFE_V__4
	__forceinline Vector_4_Out V1InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPIfEqThenElse(floatVal, 0.0f, errVal, inVector.x*FPInvertFast(floatVal.x));
		return outVect;
	}
#endif // V1INVSCALEFASTSAFE_V__4

#ifndef V1ADD_1F__4
	__forceinline Vector_4_Out V1Add( Vector_4_In inVector, float sx )
	{
		Vector_4 outVect = inVector;
		outVect.x = inVector.x + sx;
		return outVect;
	}
#endif // V1ADD_1F__4

#ifndef V1ADD_V__4
	__forceinline Vector_4_Out V1Add( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x;
		return outVect;
	}
#endif // V1ADD_V__4

#ifndef V1ADDSCALED__4
	__forceinline Vector_4_Out V1AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect = inVector1;
		outVect.x = inVector1.x + inVector2.x * floatValue;
		return outVect;
	}
#endif // V1ADDSCALED__4

#ifndef V1ADDSCALED_V__4
	__forceinline Vector_4_Out V1AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue.x;
		return outVect;
	}
#endif // V1ADDSCALED_V__4

#ifndef V1SUBTRACT_1F__4
	__forceinline Vector_4_Out V1Subtract( Vector_4_In inVector, float sx )
	{
		Vector_4 outVect;
		outVect.x = inVector.x - sx;
		return outVect;
	}
#endif // V1SUBTRACT_1F__4

#ifndef V1SUBTRACT_V__4
	__forceinline Vector_4_Out V1Subtract( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x;
		return outVect;
	}
#endif // V1SUBTRACT_V__4

#ifndef V1SUBTRACTSCALED__4
	__forceinline Vector_4_Out V1SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue;
		return outVect;
	}
#endif // V1SUBTRACTSCALED__4

#ifndef V1SUBTRACTSCALED_V__4
	__forceinline Vector_4_Out V1SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue.x;
		return outVect;
	}
#endif // V1SUBTRACTSCALED_V__4

#ifndef V1NEGATE__4
	__forceinline Vector_4_Out V1Negate(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = -inVector.x;
		return outVect;
	}
#endif // V1NEGATE__4

#ifndef V1ABS__4
	__forceinline Vector_4_Out V1Abs(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPAbs(inVector.x);
		return outVect;
	}
#endif // V1ABS__4

#ifndef V1INVERT__4
	__forceinline Vector_4_Out V1Invert(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = 1.0f/inVector.x;
		return outVect;
	}
#endif // V1INVERT__4

#ifndef V1INVERTSAFE__4
	__forceinline Vector_4_Out V1InvertSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = FPIfEqThenElse( inVector.x, 0.0f, errVal, 1.0f/inVector.x );
		return outVect;
	}
#endif // V1INVERTSAFE__4

#ifndef V1INVERTFAST__4
	__forceinline Vector_4_Out V1InvertFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPInvertFast(inVector.x);
		return outVect;
	}
#endif // V1INVERTFAST__4

#ifndef V1INVERTFASTSAFE__4
	__forceinline Vector_4_Out V1InvertFastSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = FPIfEqThenElse( inVector.x, 0.0f, errVal, FPInvertFast(inVector.x) );
		return outVect;
	}
#endif // V1INVERTFASTSAFE__4

#ifndef V1AVERAGE__4
	__forceinline Vector_4_Out V1Average(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = 0.5f * (a.x + b.x);
		outVect.y = 0.5f * (a.y + b.y);
		return outVect;
	}
#endif // V1AVERAGE__4

#ifndef V1LERP__4
	__forceinline Vector_4_Out V1Lerp( float t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t, a.x, b.x);
		return outVect;
	}
#endif // V1LERP__4

#ifndef V1LERP_V__4
	__forceinline Vector_4_Out V1Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t.x, a.x, b.x);
		return outVect;
	}
#endif // V1LERP_V__4

#ifndef V1POW__4
	__forceinline Vector_4_Out V1Pow( Vector_4_In x, Vector_4_In y )
	{
		Vector_4 outVect;
		outVect.x = FPPow(x.x, y.x);
		return outVect;
	}
#endif // V1POW__4

#ifndef V1EXPT__4
	__forceinline Vector_4_Out V1Expt( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPExpt(x.x);
		return outVect;
	}
#endif // V1EXPT__4

#ifndef V1LOG2__4
	__forceinline Vector_4_Out V1Log2( Vector_4_In x )
	{
		// log2(f) = ln(f)/ln(2)
		Vector_4 outVect;
		outVect.x = FPLog2(x.x);
		return outVect;
	}
#endif // V1LOG2__4

#ifndef V1LOG10__4
	__forceinline Vector_4_Out V1Log10( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPLog10(x.x);
		return outVect;
	}
#endif // V1LOG10__4

	//============================================================================
	// Magnitude and distance

#ifndef V1SQRT__4
	__forceinline Vector_4_Out V1Sqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPSqrt(v.x);
		return outVect;
	}
#endif // V1SQRT__4

#ifndef V1SQRTSAFE__4
	__forceinline Vector_4_Out V1SqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPSqrtSafe( v.x, errVal );
		return outVect;
	}
#endif // V1SQRTSAFE__4

#ifndef V1SQRTFAST__4
	__forceinline Vector_4_Out V1SqrtFast( Vector_4_In v )
	{
		return V1Sqrt( v );
	}
#endif // V1SQRTFAST__4

#ifndef V1SQRTFASTSAFE__4
	__forceinline Vector_4_Out V1SqrtFastSafe( Vector_4_In v, float errVal )
	{
		return V1SqrtSafe( v, errVal );
	}
#endif // V1SQRTFASTSAFE__4

#ifndef V1INVSQRT__4
	__forceinline Vector_4_Out V1InvSqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrt(v.x);
		return outVect;
	}
#endif // V1INVSQRT__4

#ifndef V1INVSQRTSAFE__4
	__forceinline Vector_4_Out V1InvSqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtSafe( v.x, errVal );
		return outVect;
	}
#endif // V1INVSQRTSAFE__4

#ifndef V1INVSQRTFAST__4
	__forceinline Vector_4_Out V1InvSqrtFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFast( v.x );
		return outVect;
	}
#endif // V1INVSQRTFAST__4

#ifndef V1INVSQRTFASTSAFE__4
	__forceinline Vector_4_Out V1InvSqrtFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFastSafe( v.x, errVal );
		return outVect;
	}
#endif // V1INVSQRTFASTSAFE__4


	//============================================================================
	// Conversion functions

#ifndef V1FLOATTOINTRAW__4
	template <int exponent>
	__forceinline Vector_4_Out V1FloatToIntRaw(Vector_4_In inVector)
	{
		float multiplier = static_cast<float>(1 << exponent);

		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult;

		TempResult.i = static_cast<int>(inVector.x * multiplier);
		outVect.x = TempResult.f;

		return outVect;
	}
#endif // V1FLOATTOINTRAW__4

#ifndef V1INTTOFLOATRAW__4
	template <int exponent>
	__forceinline Vector_4_Out V1IntToFloatRaw(Vector_4_In inVector)
	{
		float divider = static_cast<float>(1 << exponent);
		float invDiv = 1.0f/divider;

		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} Temp1;

		Temp1.f = inVector.x;
		outVect.x = static_cast<float>(Temp1.i) * invDiv;
		return outVect;
	}
#endif // V1INTTOFLOATRAW__4

#ifndef V1ROUNDTONEARESTINT__4
	__forceinline Vector_4_Out V1RoundToNearestInt(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x+(FPIfGteZeroThenElse(inVector.x, 0.5f, -0.5f))) );
		return outVect;
	}
#endif // V1ROUNDTONEARESTINT__4

#ifndef V1ROUNDTONEARESTINTZERO__4
	__forceinline Vector_4_Out V1RoundToNearestIntZero(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x) );
		return outVect;
	}
#endif // V1ROUNDTONEARESTINTZERO__4

#ifndef V1ROUNDTONEARESTINTNEGINF__4
	__forceinline Vector_4_Out V1RoundToNearestIntNegInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPFloor( inVector.x );
		return outVect;
	}
#endif // V1ROUNDTONEARESTINTNEGINF__4

#ifndef V1ROUNDTONEARESTINTPOSINF__4
	__forceinline Vector_4_Out V1RoundToNearestIntPosInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPCeil( inVector.x );
		return outVect;
	}
#endif // V1ROUNDTONEARESTINTPOSINF__4

	//============================================================================
	// Trigonometry

#ifndef V1SINANDCOS__4
	__forceinline void V1SinAndCos( Vector_4_InOut inOutSine, Vector_4_InOut inOutCosine, Vector_4_In inVector )
	{
		FPSinAndCos( inOutSine.x, inOutCosine.x, inVector.x );
	}
#endif // V1SINANDCOS__4

#ifndef V1SIN__4
	__forceinline Vector_4_Out V1Sin( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPSin( inVector.x );
		return outVect;
	}
#endif // V1SIN__4

#ifndef V1COS__4
	__forceinline Vector_4_Out V1Cos( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPCos( inVector.x );
		return outVect;
	}
#endif // V1COS__4

#ifndef V1TAN__4
	__forceinline Vector_4_Out V1Tan( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPTan( inVector.x );
		return outVect;
	}
#endif // V1TAN__4

#ifndef V1ARCSIN__4
	__forceinline Vector_4_Out V1Arcsin( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPASin( inVector.x );
		return outVect;
	}
#endif // V1ARCSIN__4

#ifndef V1ARCCOS__4
	__forceinline Vector_4_Out V1Arccos( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPACos( inVector.x );
		return outVect;
	}
#endif // V1ARCCOS__4

#ifndef V1ARCTAN__4
	__forceinline Vector_4_Out V1Arctan( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPATan( inVector.x );
		return outVect;
	}
#endif // V1ARCTAN__4

	//============================================================================
	// Comparison functions

#ifndef V1ISBETWEENNEGANDPOSBOUNDS__4
	__forceinline unsigned int V1IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector )
	{
		return ( testVector.x <= boundsVector.x && testVector.x >= -boundsVector.x ? 1u : 0u );
		//float fResult = FPIfGteThenElse( boundsVector.x, testVector.x, FPIfGteThenElse(testVector.x,-boundsVector.x,1.0f,0.0f), 0.0f );
		//return static_cast<u32>(fResult); // (too slow!)
	}
#endif // V1ISBETWEENNEGANDPOSBOUNDS__4

#ifndef V1ISZERO__4
	__forceinline unsigned int V1IsZero(Vector_4_In inVector)
	{
		return ( inVector.x == 0.0f ? 1u : 0u );
		//float fResult = FPIfEqThenElse( inVector.x, 0.0f, 1.0f, 0.0f );
		//return static_cast<u32>(fResult); // (too slow!)
	}
#endif // V1ISZERO__4

#ifndef V1ISEQUAL__4
	__forceinline unsigned int V1IsEqual(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x == inVector2.x ? 1u : 0u );
	}
#endif // V1ISEQUAL__4

#ifndef V1ISEQUALINT__4
	__forceinline unsigned int V1IsEqualInt(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;

		return (Temp1.i == Temp2.i ? 1u : 0u );
	}
#endif // V1ISEQUALINT__4

#ifndef V1ISNOTEQUAL__4
	__forceinline unsigned int V1IsNotEqual(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return (inVector1.x != inVector2.x ? 1u : 0u );
	}
#endif // V1ISNOTEQUAL__4

#ifndef V1ISNOTEQUALINT__4
	__forceinline unsigned int V1IsNotEqualInt(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;

		return (Temp1.i != Temp2.i ? 1u : 0u );

	}
#endif // V1ISNOTEQUALINT__4

#ifndef V1ISCLOSE__4
	__forceinline unsigned int V1IsClose(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return ( ( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) ? 1u : 0u);
	}
#endif // V1ISCLOSE__4

#ifndef V1ISCLOSE_V__4
	__forceinline unsigned int V1IsClose(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) ? 1u : 0u);
	}
#endif // V1ISCLOSE_V__4

#ifndef V1ISGREATERTHAN__4
	__forceinline unsigned int V1IsGreaterThan(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x > smallVector.x ) ? 1u : 0u );
	}
#endif // V1ISGREATERTHAN__4

#ifndef V1ISGREATERTHANOREQUAL__4
	__forceinline unsigned int V1IsGreaterThanOrEqual(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x >= smallVector.x ) ? 1u : 0u);
	}
#endif // V1ISGREATERTHANOREQUAL__4

#ifndef V1ISLESSTHAN__4
	__forceinline unsigned int V1IsLessThan(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x > smallVector.x ) ? 1u : 0u);
	}
#endif // V1ISLESSTHAN__4

#ifndef V1ISLESSTHANOREQUAL__4
	__forceinline unsigned int V1IsLessThanOrEqual(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x >= smallVector.x ) ? 1u : 0u);
	}
#endif // V1ISLESSTHANOREQUAL__4

#ifndef V1SELECT__4
	__forceinline Vector_4_Out V1SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult, Temp1, Temp2, Temp3;

		Temp1.f = choiceVector.x;
		Temp2.f = zero.x;
		Temp3.f = nonZero.x;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.x = TempResult.f;

		return outVect;
	}
#endif // V1SELECT__4

#ifndef V1SELECTVECT__4
	__forceinline Vector_4_Out V1SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero)
	{
		return (GetX(choiceVectorX) == 0.0f ? zero : nonZero );
	}
#endif // V1SELECTVECT__4

#ifndef V1MAX__4
	__forceinline Vector_4_Out V1Max(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMax(inVector1.x, inVector2.x);
		return outVect;
	}
#endif // V1MAX__4

#ifndef V1MIN__4
	__forceinline Vector_4_Out V1Min(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMin(inVector1.x, inVector2.x);
		return outVect;
	}
#endif // V1MIN__4

#ifndef V1CLAMP__4
	__forceinline Vector_4_Out V1Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound )
	{
		Vector_4 outVect;
		outVect.x = FPClamp(inVector.x, lowBound.x, highBound.x);
		return outVect;
	}
#endif // V1CLAMP__4

	//============================================================================
	// Bitwise operations

#ifndef V1AND__4
	__forceinline Vector_4_Out V1And(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, Temp1;

		TempResult1.f = inVector1.x;
		Temp1.f = inVector2.x;
		TempResult1.i &= Temp1.i;

		outVect.x = TempResult1.f;
		return outVect;
	}
#endif // V1AND__4

#ifndef V1OR__4
	__forceinline Vector_4_Out V1Or(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, Temp1;

		TempResult1.f = inVector1.x;
		Temp1.f = inVector2.x;
		TempResult1.i |= Temp1.i;

		outVect.x = TempResult1.f;
		return outVect;
	}
#endif // V1OR__4

#ifndef V1XOR__4
	__forceinline Vector_4_Out V1Xor(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, Temp1;

		TempResult1.f = inVector1.x;
		Temp1.f = inVector2.x;
		TempResult1.i ^= Temp1.i;

		outVect.x = TempResult1.f;
		return outVect;
	}
#endif // V1XOR__4

#ifndef V1ANDC__4
	__forceinline Vector_4_Out V1Andc(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, Temp1;

		TempResult1.f = inVector1.x;
		Temp1.f = inVector2.x;
		TempResult1.i &= ~Temp1.i;

		outVect.x = TempResult1.f;
		return outVect;
	}
#endif // V1ANDC__4


} // namespace Vec
} // namespace rage

