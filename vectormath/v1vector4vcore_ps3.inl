namespace rage
{
namespace Vec
{

	__forceinline void V1Set( Vector_4V_InOut inoutVector, float x0 )
	{
		if (__builtin_constant_p(x0))
		{
			inoutVector = VECTOR4V_LITERAL(x0, x0, x0, x0);
		}
		else
		{
			f32 *pVect = (f32*)&inoutVector;
			pVect[0] = x0;
		}
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V1IsEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		return vec_all_eq( (Vector_4V_uint)inV1, (Vector_4V_uint)inV2 );
	}

	__forceinline unsigned int V1IsNotEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		return vec_all_eq( (Vector_4V_uint)inV1, (Vector_4V_uint)inV2 ) ^ 0x1;
	}

	__forceinline unsigned int V1IsEqual(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		return vec_all_eq( inV1, inV2 );
	}

	__forceinline unsigned int V1IsNotEqual(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		return vec_all_eq( inV1, inV2 ) ^ 0x1;
	}

	__forceinline unsigned int V1IsGreaterThan(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		return vec_all_gt( bigV, smallV );
	}

	__forceinline unsigned int V1IsGreaterThanOrEqual(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		return vec_all_ge( bigV, smallV );		
	}

	__forceinline unsigned int V1IsLessThan(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		return vec_all_lt( smallV, bigV );
	}

	__forceinline unsigned int V1IsLessThanOrEqual(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		return vec_all_le( smallV, bigV );
	}

	
} // namespace Vec
} // namespace rage
