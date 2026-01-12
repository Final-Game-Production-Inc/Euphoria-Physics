namespace rage
{
namespace Vec
{


	__forceinline void V1Set( Vector_4V_InOut inoutVector, float x0 )
	{
		SetX( inoutVector, x0 );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V1IsEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		unsigned int cr;
		__vcmpequwR( inV1, inV2, &cr );
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V1IsNotEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		unsigned int cr;
		__vcmpequwR( inV1, inV2, &cr );
		return (cr & 0x20) >> 5;
	}


	__forceinline unsigned int V1IsEqual(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		unsigned int cr;
		__vcmpeqfpR(inV1, inV2, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V1IsNotEqual(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4SplatX( inVector1 );
		Vector_4V inV2 = V4SplatX( inVector2 );

		unsigned int cr;
		__vcmpeqfpR( inV1, inV2, &cr );
		return (cr & 0x20) >> 5;
	}

	__forceinline unsigned int V1IsGreaterThan(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		unsigned int cr;
		__vcmpgtfpR(bigV, smallV, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V1IsGreaterThanOrEqual(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		unsigned int cr;
		__vcmpgefpR(bigV, smallV, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V1IsLessThan(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		unsigned int cr;
		__vcmpgtfpR(bigV, smallV, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V1IsLessThanOrEqual(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V bigV = V4SplatX( bigVector );
		Vector_4V smallV = V4SplatX( smallVector );

		unsigned int cr;
		__vcmpgefpR(bigV, smallV, &cr);
		return (cr & 0x80) >> 7;
	}

	
} // namespace Vec
} // namespace rage
