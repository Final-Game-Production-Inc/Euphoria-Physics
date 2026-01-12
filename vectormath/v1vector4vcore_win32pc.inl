namespace rage
{
namespace Vec
{

	__forceinline void V1Set( Vector_4V_InOut inoutVector, float x0 )
	{
		inoutVector = _mm_setr_ps( x0, x0, x0, x0 );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V1IsEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result & 0x1);
	}

	__forceinline unsigned int V1IsNotEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result ^ 0x1);
	}

	__forceinline unsigned int V1IsEqual(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result & 0x1);
	}

	__forceinline unsigned int V1IsNotEqual(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result ^ 0x1);
	}

	__forceinline unsigned int V1IsGreaterThan(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result & 0x1);
	}

	__forceinline unsigned int V1IsGreaterThanOrEqual(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result & 0x1);
	}

	__forceinline unsigned int V1IsLessThan(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result & 0x1);
	}

	__forceinline unsigned int V1IsLessThanOrEqual(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result & 0x1);
	}

	
} // namespace Vec
} // namespace rage
