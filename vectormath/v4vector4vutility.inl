#if UNIQUE_VECTORIZED_TYPE

namespace rage
{
namespace Vec
{
namespace Util
{

	// Forceinlined since it's never called directly.. just a helper.
	__forceinline Vector_4V_Out V4SinHelper( Vector_4V_In inVector, Vector_4V_In numPi )
	{
		// Subtract 'numPi' PI's, to get the input in the range [-PI/2, PI/2]. This is where the Taylor series is most accurate.
		// "inVectModded = inVect + (-PI * #PI)"
		Vector_4V inVectModded = V4AddScaled( inVector, V4VConstant(V_NEG_PI), numPi );

		// A little more -PI accuracy.
		//inVectModded = V4AddScaled( inVectModded, V4VConstant<0xB4222168,0xB4222168,0xB4222168,0xB4222168>(), numPi );
		// Even more -PI accuracy.
		//inVectModded = V4AddScaled( inVectModded, V4VConstant<0xA84234C4,0xA84234C4,0xA84234C4,0xA84234C4>(), numPi );

		// Do a sine Taylor polynomial approximation (degree 9). Sine is faster since it allows more AddScaled()'s.
		Vector_4V vectSq = V4Scale( inVectModded, inVectModded );
		Vector_4V sineVect = V4AddScaled( V4VConstant<0xB9500D01,0xB9500D01,0xB9500D01,0xB9500D01>(), vectSq, V4VConstant<0x3638EF1D,0x3638EF1D,0x3638EF1D,0x3638EF1D>() );
		sineVect = V4AddScaled( V4VConstant<0x3C088889,0x3C088889,0x3C088889,0x3C088889>(), vectSq, sineVect );
		sineVect = V4AddScaled( V4VConstant<0xBE2AAAAB,0xBE2AAAAB,0xBE2AAAAB,0xBE2AAAAB>(), vectSq, sineVect );
		sineVect = V4AddScaled( V4VConstant(V_ONE), vectSq, sineVect );
		return V4Scale( inVectModded, sineVect );
	}

} // namespace Util
} // namespace Vec
} // namespace rage

#endif // UNIQUE_VECTORIZED_TYPE
