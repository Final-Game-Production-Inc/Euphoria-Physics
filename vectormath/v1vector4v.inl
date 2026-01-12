
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{


	//============================================================================
	// Comparison functions

	__forceinline unsigned int V1IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector )
	{
		Vector_4V test = V4SplatX( testVector );
		Vector_4V bounds = V4SplatX( boundsVector );
		return V4IsBetweenNegAndPosBounds( test, bounds );
	}

	__forceinline unsigned int V1IsZero(Vector_4V_In inVector)
	{
		return V1IsEqual( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V1IsClose(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V1IsEqualInt( bothVect, V4VConstant(V_MASKXYZW) );
	}


	
} // namespace Vec
} // namespace rage
