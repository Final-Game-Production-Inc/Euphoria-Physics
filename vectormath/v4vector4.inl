
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

#ifndef V4ISFINITEV
	inline Vector_4_Out V4IsFiniteV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = (FPIsFinite(inVector.x) ? allBitsF : 0.0f);
		outVect.y = (FPIsFinite(inVector.y) ? allBitsF : 0.0f);
		outVect.z = (FPIsFinite(inVector.z) ? allBitsF : 0.0f);
		outVect.w = (FPIsFinite(inVector.w) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISFINITEV

#ifndef V4ISNOTNANV
	inline Vector_4_Out V4IsNotNanV( Vector_4_In inVector )
	{
#	if __PPU
		// This is a slow-as-hell method, but at least the vector V4IsNotNanV() works, so we'll use it.
		// (To make the #else version work on __PPU, encourage RAGE to turn off -ffast-math for gcc.)
#if UNIQUE_VECTORIZED_TYPE
		Vector_4V inVec, resultV;
#else
		Vector_4V_Persistent inVec, resultV;		
#endif
		Vector_4 result;

		*(Vector_4*)(&inVec) = inVector; // Since we don't know the alignment of the scalar input vector.

		// We have to inline V4IsNotNanV(Vec::Vector_4V_Persistent) here, since it doesn't exist when UNIQUE_VECTORIZED_TYPE == 0.
#if UNIQUE_VECTORIZED_TYPE
		resultV = (Vector_4V)vec_cmpeq( inVec, inVec );
#else
		resultV = (Vector_4V_Persistent)vec_cmpeq( inVec, inVec );
#endif

		result = *(Vector_4*)(&resultV); // Since we don't know the alignment of the scalar destination vector.
		return result;
#	else
		Vector_4 outVect;
		outVect.x = (inVector.x == inVector.x ? allBitsF : 0.0f);
		outVect.y = (inVector.y == inVector.y ? allBitsF : 0.0f);
		outVect.z = (inVector.z == inVector.z ? allBitsF : 0.0f);
		outVect.w = (inVector.w == inVector.w ? allBitsF : 0.0f);
		return outVect;
#	endif
	}
#endif // V4ISNOTNANV

#ifndef V4GETELEM
	__forceinline float GetElem( Vector_4_In inVector, unsigned int elem )
	{
		mthAssertf( elem <= 3 , "Invalid element index." );
		return ((float*)(&inVector))[elem];
	}
#endif // V4GETELEM

#ifndef V4GETELEMREF
	__forceinline float& GetElemRef( Vector_4_Ptr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 3 , "Invalid element index." );
		return ((float*)(pInVector))[elem];
	}
#endif // V4GETELEMREF

#ifndef V4GETELEMREF_CONST
	__forceinline const float& GetElemRef( Vector_4_ConstPtr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 3 , "Invalid element index." );
		return ((const float*)(pInVector))[elem];
	}
#endif // V4GETELEMREF_CONST

#ifndef V4GETX
	__forceinline float GetX( Vector_4_In inVector )
	{
		return inVector.x;
	}
#endif // V4GETX

#ifndef V4GETY
	__forceinline float GetY( Vector_4_In inVector )
	{
		return inVector.y;
	}
#endif // V4GETY

#ifndef V4GETZ
	__forceinline float GetZ( Vector_4_In inVector )
	{
		return inVector.z;
	}
#endif // V4GETZ

#ifndef V4GETW
	__forceinline float GetW( Vector_4_In inVector )
	{
		return inVector.w;
	}
#endif // V4GETW

#ifndef V4GETXV
	__forceinline Vector_4_Out GetXV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float x = inVector.x;
		outVect.x = x;
		outVect.y = x;
		outVect.z = x;
		outVect.w = x;
		return outVect;
	}
#endif // V4GETXV

#ifndef V4GETYV
	__forceinline Vector_4_Out GetYV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float y = inVector.y;
		outVect.x = y;
		outVect.y = y;
		outVect.z = y;
		outVect.w = y;
		return outVect;
	}
#endif // V4GETYV

#ifndef V4GETZV
	__forceinline Vector_4_Out GetZV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float z = inVector.z;
		outVect.x = z;
		outVect.y = z;
		outVect.z = z;
		outVect.w = z;
		return outVect;
	}
#endif // V4GETZV

#ifndef V4GETWV
	__forceinline Vector_4_Out GetWV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float w = inVector.w;
		outVect.x = w;
		outVect.y = w;
		outVect.z = w;
		outVect.w = w;
		return outVect;
	}
#endif // V4GETWV

#ifndef V4GETXI
	// Same as above, but uses int pipeline.
	__forceinline int GetXi( Vector_4_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[0];
	}
#endif // V4GETXI

#ifndef V4GETYI
	// Same as above, but uses int pipeline.
	__forceinline int GetYi( Vector_4_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[1];
	}
#endif // V4GETYI

#ifndef V4GETZI
	// Same as above, but uses int pipeline.
	__forceinline int GetZi( Vector_4_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[2];
	}
#endif // V4GETZI

#ifndef V4GETWI
	// Same as above, but uses int pipeline.
	__forceinline int GetWi( Vector_4_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[3];
	}
#endif // V4GETWI

#ifndef V4SETX
	__forceinline void SetX( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.x = floatVal;
	}
#endif // V4SETX

#ifndef V4SETY
	__forceinline void SetY( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.y = floatVal;
	}
#endif // V4SETY

#ifndef V4SETZ
	__forceinline void SetZ( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.z = floatVal;
	}
#endif // V4SETZ

#ifndef V4SETW
	__forceinline void SetW( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.w = floatVal;
	}
#endif // V4SETW

#ifndef V4SETXINMEMORY_I
	__forceinline void SetXInMemory( Vector_4_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[0] = intVal;
	}
#endif // V4SETXINMEMORY_I

#ifndef V4SETYINMEMORY_I
	__forceinline void SetYInMemory( Vector_4_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[1] = intVal;
	}
#endif // V4SETYINMEMORY_I

#ifndef V4SETZINMEMORY_I
	__forceinline void SetZInMemory( Vector_4_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[2] = intVal;
	}
#endif // V4SETZINMEMORY_I

#ifndef V4SETWINMEMORY_I
	__forceinline void SetWInMemory( Vector_4_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[3] = intVal;
	}
#endif // V4SETWINMEMORY_I

#if !UNIQUE_VECTORIZED_TYPE

#ifndef V4SETXINMEMORY
	__forceinline void SetXInMemory( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.x = floatVal;
	}
#endif // V4SETXINMEMORY

#ifndef V4SETYINMEMORY
	__forceinline void SetYInMemory( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.y = floatVal;
	}
#endif // V4SETYINMEMORY

#ifndef V4SETZINMEMORY
	__forceinline void SetZInMemory( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.z = floatVal;
	}
#endif // V4SETZINMEMORY

#ifndef V4SETWINMEMORY
	__forceinline void SetWInMemory( Vector_4_InOut inoutVector, float floatVal )
	{
		inoutVector.w = floatVal;
	}
#endif // V4SETWINMEMORY

	__forceinline void SetXYZWInMemory( Vector_4V_InOut inoutVector, float x, float y, float z, float w )
	{
		float *asFloats = reinterpret_cast<float*>(&inoutVector);
		asFloats[0] = x;
		asFloats[1] = y;
		asFloats[2] = z;
		asFloats[3] = w;
	}

#ifndef V4LOADSCALAR32INTOSPLATTED_F32
	__forceinline Vector_4_Out V4LoadScalar32IntoSplatted( const float& scalar )
	{
		Vector_4 retval;
		retval.x = retval.y = retval.z = retval.w = scalar;
		return retval;
	}
#endif // V4LOADSCALAR32INTOSPLATTED

#ifndef V4LOADSCALAR32INTOSPLATTED_U32
	__forceinline Vector_4_Out V4LoadScalar32IntoSplatted( const u32& scalar )
	{
		union
		{
			float f;
			u32 u;
		} Temp;
		Temp.u = scalar;
		Vector_4 retval;
		retval.x = retval.y = retval.z = retval.w = Temp.f;
		return retval;
	}
#endif // V4LOADSCALAR32INTOSPLATTED_U32

#ifndef V4LOADSCALAR32INTOSPLATTED_I32
	__forceinline Vector_4_Out V4LoadScalar32IntoSplatted( const int& scalar )
	{
		union
		{
			float f;
			int i;
		} Temp;
		Temp.i = scalar;
		Vector_4 retval;
		retval.x = retval.y = retval.z = retval.w = Temp.f;
		return retval;
	}
#endif // V4LOADSCALAR32INTOSPLATTED_I32

#ifndef V4STORESCALAR32FROMSPLATTED_F32
	__forceinline void V4StoreScalar32FromSplatted( float& fLoc, Vector_4_In splattedVec )
	{
		fLoc = splattedVec.x;
	}
#endif // V4STORESCALAR32FROMSPLATTED_F32

#ifndef V4STORESCALAR32FROMSPLATTED_U32
	__forceinline void V4StoreScalar32FromSplatted( u32& fLoc, Vector_4_In splattedVec )
	{
		*reinterpret_cast<float*>(&fLoc) = splattedVec.x;
	}
#endif // V4STORESCALAR32FROMSPLATTED_u32

#ifndef V4STORESCALAR32FROMSPLATTED_I32
	__forceinline void V4StoreScalar32FromSplatted( int& fLoc, Vector_4_In splattedVec )
	{
		*reinterpret_cast<float*>(&fLoc) = splattedVec.x;
	}
#endif // V4STORESCALAR32FROMSPLATTED_I32

#ifndef V4LOADUNALIGNED
	__forceinline Vector_4_Out V4LoadUnaligned( const void* ptr )
	{
		Vector_4 v;
		memcpy(&v, ptr, 16);
		return v;
	}
#endif // V4LOADUNALIGNED

#endif // !UNIQUE_VECTORIZED_TYPE

#ifndef V4SPLATX
	__forceinline Vector_4_Out V4SplatX( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float x = inVector.x;
		outVect.x = x;
		outVect.y = x;
		outVect.z = x;
		outVect.w = x;
		return outVect;
	}
#endif // V4SPLATX

#ifndef V4SPLATY
	__forceinline Vector_4_Out V4SplatY( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float y = inVector.y;
		outVect.x = y;
		outVect.y = y;
		outVect.z = y;
		outVect.w = y;
		return outVect;
	}
#endif // V4SPLATY

#ifndef V4SPLATZ
	__forceinline Vector_4_Out V4SplatZ( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float z = inVector.z;
		outVect.x = z;
		outVect.y = z;
		outVect.z = z;
		outVect.w = z;
		return outVect;
	}
#endif // V4SPLATZ

#ifndef V4SPLATW
	__forceinline Vector_4_Out V4SplatW( Vector_4_In inVector )
	{
		Vector_4 outVect;
		float w = inVector.w;
		outVect.x = w;
		outVect.y = w;
		outVect.z = w;
		outVect.w = w;
		return outVect;
	}
#endif // V4SPLATW

#ifndef V4SET_4F
	__forceinline void V4Set( Vector_4_InOut inoutVector, const float& x0, const float& y0, const float& z0, const float& w0 )
	{
		inoutVector.x = x0;
		inoutVector.y = y0;
		inoutVector.z = z0;
		inoutVector.w = w0;
	}
#endif // V4SET_4F

#ifndef V4SET_V
	__forceinline void V4Set( Vector_4_InOut inoutVector, Vector_4_In inVector )
	{
		inoutVector = inVector;
	}
#endif // V4SET_V

#ifndef V4SET
	__forceinline void V4Set( Vector_4_InOut inoutVector, const float& s )
	{
		inoutVector.x = s;
		inoutVector.y = s;
		inoutVector.z = s;
		inoutVector.w = s;
	}
#endif // V4SET

#ifndef V4SET_U32
	__forceinline void V4Set( Vector_4_InOut inoutVector, const u32& s )
	{
		union
		{
			float f;
			u32 u;
		} Temp;
		Temp.u = s;
		inoutVector.x = Temp.f;
		inoutVector.y = Temp.f;
		inoutVector.z = Temp.f;
		inoutVector.w = Temp.f;
	}
#endif // V4SET_U32

#ifndef V4SET_I32
	__forceinline void V4Set( Vector_4_InOut inoutVector, const int& s )
	{
		union
		{
			float f;
			int i;
		} Temp;
		Temp.i = s;
		inoutVector.x = Temp.f;
		inoutVector.y = Temp.f;
		inoutVector.z = Temp.f;
		inoutVector.w = Temp.f;
	}
#endif // V4SET_I32

#ifndef V4ZEROCOMPONENTS
	__forceinline void V4ZeroComponents( Vector_4_InOut inoutVector )
	{
		inoutVector.x = inoutVector.y = inoutVector.z = inoutVector.w = 0.0f;
	}
#endif // V4ZEROCOMPONENTS

#ifndef V4CLEARW
	__forceinline void V4SetWZero( Vector_4_InOut inoutVector )
	{
		inoutVector.w = 0.0f;
	}
#endif // V4CLEARW

	/********** Standard Algebra **********/

#ifndef V4SCALE
	__forceinline Vector_4_Out V4Scale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * floatVal;
		outVect.y = inVector.y * floatVal;
		outVect.z = inVector.z * floatVal;
		outVect.w = inVector.w * floatVal;
		return outVect;
	}
#endif // V4SCALE

#ifndef V4INVSCALE
	__forceinline Vector_4_Out V4InvScale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		float invVal = 1.0f/floatVal;
		outVect.x = inVector.x * invVal;
		outVect.y = inVector.y * invVal;
		outVect.z = inVector.z * invVal;
		outVect.w = inVector.w * invVal;
		return outVect;
	}
#endif // V4INVSCALE

#ifndef V4INVSCALE_V
	__forceinline Vector_4_Out V4InvScale( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x / floatVal.x;
		outVect.y = inVector.y / floatVal.y;
		outVect.z = inVector.z / floatVal.z;
		outVect.w = inVector.w / floatVal.w;
		return outVect;
	}
#endif // V4INVSCALE_V

#ifndef V4INVSCALESAFE
	__forceinline Vector_4_Out V4InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		if( floatVal != 0.0f )
		{
			float invVal = 1.0f/floatVal;
			outVect.x = inVector.x * invVal;
			outVect.y = inVector.y * invVal;
			outVect.z = inVector.z * invVal;
			outVect.w = inVector.w * invVal;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = outVect.w = errVal;
		}
		return outVect;
	}
#endif // V4INVSCALESAFE

#ifndef V4INVSCALESAFE_V
	__forceinline Vector_4_Out V4InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x/floatVal.x : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y/floatVal.y : errVal;
		outVect.z = (floatVal.z != 0.0f) ? inVector.z/floatVal.z : errVal;
		outVect.w = (floatVal.w != 0.0f) ? inVector.w/floatVal.w : errVal;
		return outVect;
	}
#endif // V4INVSCALESAFE_V

#ifndef V4INVSCALEFAST
	__forceinline Vector_4_Out V4InvScaleFast( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		float inv = FPInvertFast(floatVal);
		outVect.x = inVector.x * inv;
		outVect.y = inVector.y * inv;
		outVect.z = inVector.z * inv;
		outVect.w = inVector.w * inv;
		return outVect;
	}
#endif // V4INVSCALEFAST

#ifndef V4INVSCALEFAST_V
	__forceinline Vector_4_Out V4InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal.x);
		outVect.y = inVector.y * FPInvertFast(floatVal.y);
		outVect.z = inVector.z * FPInvertFast(floatVal.z);
		outVect.w = inVector.w * FPInvertFast(floatVal.w);
		return outVect;
	}
#endif // V4INVSCALEFAST_V

#ifndef V4INVSCALEFASTSAFE
	__forceinline Vector_4_Out V4InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		if( floatVal != 0.0f )
		{
			float inv = FPInvertFast(floatVal);
			outVect.x = inVector.x*inv;
			outVect.y = inVector.y*inv;
			outVect.z = inVector.z*inv;
			outVect.w = inVector.w*inv;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = outVect.w = errVal;
		}
		return outVect;
	}
#endif // V4INVSCALEFASTSAFE

#ifndef V4INVSCALEFASTSAFE_V
	__forceinline Vector_4_Out V4InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x*FPInvertFast(floatVal.x) : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y*FPInvertFast(floatVal.y) : errVal;
		outVect.z = (floatVal.z != 0.0f) ? inVector.z*FPInvertFast(floatVal.z) : errVal;
		outVect.w = (floatVal.w != 0.0f) ? inVector.w*FPInvertFast(floatVal.w) : errVal;
		return outVect;
	}
#endif // V4INVSCALEFASTSAFE_V

#ifndef V4ADD_4F
	__forceinline Vector_4_Out V4Add( Vector_4_In inVector, float sx, float sy, float sz, float sw )
	{
		Vector_4 outVect;
		outVect.x = inVector.x + sx;
		outVect.y = inVector.y + sy;
		outVect.z = inVector.z + sz;
		outVect.w = inVector.w + sw;
		return outVect;
	}
#endif // V4ADD_4F

#ifndef V4ADD_V
	__forceinline Vector_4_Out V4Add( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x;
		outVect.y = inVector1.y + inVector2.y;
		outVect.z = inVector1.z + inVector2.z;
		outVect.w = inVector1.w + inVector2.w;
		return outVect;
	}
#endif // V4ADD_V

#ifndef V4ADDINT_V
	__forceinline Vector_4_Out V4AddInt( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} Temp1, Temp2, TempResult;

		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.x = TempResult.f;

		Temp1.f = inVector1.y;
		Temp2.f = inVector2.y;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.y = TempResult.f;

		Temp1.f = inVector1.z;
		Temp2.f = inVector2.z;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.z = TempResult.f;

		Temp1.f = inVector1.w;
		Temp2.f = inVector2.w;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.w = TempResult.f;

		return outVect;
	}
#endif // V4ADDINT_V

#ifndef V4SUBTRACTINT_V
	__forceinline Vector_4_Out V4SubtractInt( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} Temp1, Temp2, TempResult;

		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.x = TempResult.f;

		Temp1.f = inVector1.y;
		Temp2.f = inVector2.y;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.y = TempResult.f;

		Temp1.f = inVector1.z;
		Temp2.f = inVector2.z;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.z = TempResult.f;

		Temp1.f = inVector1.w;
		Temp2.f = inVector2.w;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.w = TempResult.f;

		return outVect;
	}
#endif

#ifndef V4ADDSCALED
	__forceinline Vector_4_Out V4AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue;
		outVect.y = inVector1.y + inVector2.y * floatValue;
		outVect.z = inVector1.z + inVector2.z * floatValue;
		outVect.w = inVector1.w + inVector2.w * floatValue;
		return outVect;
	}
#endif // V4ADDSCALED

#ifndef V4ADDSCALED_V
	__forceinline Vector_4_Out V4AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue.x;
		outVect.y = inVector1.y + inVector2.y * floatValue.y;
		outVect.z = inVector1.z + inVector2.z * floatValue.z;
		outVect.w = inVector1.w + inVector2.w * floatValue.w;
		return outVect;
	}
#endif // V4ADDSCALED_V

#ifndef V4SUBTRACT_4F
	__forceinline Vector_4_Out V4Subtract( Vector_4_In inVector, float sx, float sy, float sz, float sw)
	{
		Vector_4 outVect;
		outVect.x = inVector.x - sx;
		outVect.y = inVector.y - sy;
		outVect.z = inVector.z - sz;
		outVect.w = inVector.w - sw;
		return outVect;
	}
#endif // V4SUBTRACT_4F

#ifndef V4SUBTRACT_V
	__forceinline Vector_4_Out V4Subtract( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect = inVector1;
		outVect.x -= inVector2.x;
		outVect.y -= inVector2.y;
		outVect.z -= inVector2.z;
		outVect.w -= inVector2.w;
		return outVect;
	}
#endif // V4SUBTRACT_V

#ifndef V4SUBTRACTSCALED
	__forceinline Vector_4_Out V4SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue;
		outVect.y = inVector1.y - inVector2.y * floatValue;
		outVect.z = inVector1.z - inVector2.z * floatValue;
		outVect.w = inVector1.w - inVector2.w * floatValue;
		return outVect;
	}
#endif // V4SUBTRACTSCALED

#ifndef V4SUBTRACTSCALED_V
	__forceinline Vector_4_Out V4SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue.x;
		outVect.y = inVector1.y - inVector2.y * floatValue.y;
		outVect.z = inVector1.z - inVector2.z * floatValue.z;
		outVect.w = inVector1.w - inVector2.w * floatValue.w;
		return outVect;
	}
#endif // V4SUBTRACTSCALED_V

#ifndef V4SCALE_V
	__forceinline Vector_4_Out V4Scale( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x * inVector2.x;
		outVect.y = inVector1.y * inVector2.y;
		outVect.z = inVector1.z * inVector2.z;
		outVect.w = inVector1.w * inVector2.w;
		return outVect;
	}

	__forceinline void V4Scale( Vector_4_InOut c, Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x * inVector2.x;
		outVect.y = inVector1.y * inVector2.y;
		outVect.z = inVector1.z * inVector2.z;
		outVect.w = inVector1.w * inVector2.w;
		c = outVect;
	}
#endif // V4SCALE_V

#ifndef V4NEGATE
	__forceinline Vector_4_Out V4Negate(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = -inVector.x;
		outVect.y = -inVector.y;
		outVect.z = -inVector.z;
		outVect.w = -inVector.w;
		return outVect;
	}
#endif // V4NEGATE

#ifndef V4ABS
	__forceinline Vector_4_Out V4Abs(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPAbs(inVector.x);
		outVect.y = FPAbs(inVector.y);
		outVect.z = FPAbs(inVector.z);
		outVect.w = FPAbs(inVector.w);
		return outVect;
	}
#endif // V4ABS

#ifndef V4INVERTBITS
	__forceinline Vector_4_Out V4InvertBits(Vector_4_In inVector)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} Temp1, TempResult;

		Temp1.f = inVector.x;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.x = TempResult.f;

		Temp1.f = inVector.y;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.y = TempResult.f;

		Temp1.f = inVector.z;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.z = TempResult.f;

		Temp1.f = inVector.w;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.w = TempResult.f;

		return outVect;
	}
#endif // V4INVERTBITS

	__forceinline Vector_4_Out V4Invert_NewtonRaphsonRefine(Vector_4_In x, Vector_4_In y0)
	{
		// y1 = y0 + y0 * (1.0 - x * y0)

		const Vector_4 one = V4Constant(V_ONE);
		return V4AddScaled(y0, y0, V4SubtractScaled(one, x, y0));
	}

	__forceinline Vector_4_Out V4InvSqrt_NewtonRaphsonRefine(Vector_4_In x, Vector_4_In y0)
	{
		// y1 = y0 + 0.5 * y0 * (1.0 - x * y0 * y0)
		// .. = y0 + y0 * (0.5 - 0.5 * x * y0 * y0)

		const Vector_4 half = V4Constant(V_HALF);
		return V4AddScaled(y0, y0, V4SubtractScaled(half, half, V4Scale(x, V4Scale(y0, y0))));
	}

#ifndef V4INVERT
	__forceinline Vector_4_Out V4Invert(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = 1.0f/inVector.x;
		outVect.y = 1.0f/inVector.y;
		outVect.z = 1.0f/inVector.z;
		outVect.w = 1.0f/inVector.w;
		return outVect;
	}
#endif // V4INVERT

#ifndef V4INVERTSAFE
	__forceinline Vector_4_Out V4InvertSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : 1.0f/inVector.x );
		outVect.y = ( inVector.y == 0.0f ? errVal : 1.0f/inVector.y );
		outVect.z = ( inVector.z == 0.0f ? errVal : 1.0f/inVector.z );
		outVect.w = ( inVector.w == 0.0f ? errVal : 1.0f/inVector.w );
		return outVect;
	}
#endif // V4INVERTSAFE

#ifndef V4INVERTFAST
	__forceinline Vector_4_Out V4InvertFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPInvertFast(inVector.x);
		outVect.y = FPInvertFast(inVector.y);
		outVect.z = FPInvertFast(inVector.z);
		outVect.w = FPInvertFast(inVector.w);
		return outVect;
	}
#endif // V4INVERTFAST

#ifndef V4INVERTFASTSAFE
	__forceinline Vector_4_Out V4InvertFastSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : FPInvertFast(inVector.x) );
		outVect.y = ( inVector.y == 0.0f ? errVal : FPInvertFast(inVector.y) );
		outVect.z = ( inVector.z == 0.0f ? errVal : FPInvertFast(inVector.z) );
		outVect.w = ( inVector.w == 0.0f ? errVal : FPInvertFast(inVector.w) );
		return outVect;
	}
#endif // V4INVERTFASTSAFE

	// NOTE -- 'Precise' functions do an additional Newton-Raphson iteration, but typically this only improves accuracy by 1-2 bits
	__forceinline Vector_4_Out V4InvertPrecise      (Vector_4_In a) { return V4Invert_NewtonRaphsonRefine (a, V4Invert     (a)); }
	__forceinline Vector_4_Out V4InvSqrtPrecise     (Vector_4_In a) { return V4InvSqrt_NewtonRaphsonRefine(a, V4InvSqrt    (a)); }


#ifndef V4NORMALIZE
	__forceinline Vector_4_Out V4Normalize(Vector_4_In inVector)
	{
		Vector_4 outVect;
		float invMag = V4InvMag(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		outVect.z = inVector.z * invMag;
		outVect.w = inVector.w * invMag;
		return outVect;
	}
#endif // V4NORMALIZE

#ifndef V4NORMALIZESAFE
	__forceinline Vector_4_Out V4NormalizeSafe(Vector_4_In inVector, float errVal, float magSqThreshold)
	{
		Vector_4 outVect;
		float mag2 = V4MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrt(mag2);
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
			outVect.z = inVector.z*invMag;
			outVect.w = inVector.w*invMag;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = outVect.w = errVal;
		}
		return outVect;
	}
#endif // V4NORMALIZESAFE

#ifndef V4NORMALIZEFAST
	__forceinline Vector_4_Out V4NormalizeFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		float invMag = V4InvMagFast(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		outVect.z = inVector.z * invMag;
		outVect.w = inVector.w * invMag;
		return outVect;
	}
#endif // V4NORMALIZEFAST

#ifndef V4NORMALIZEFASTSAFE
	__forceinline Vector_4_Out V4NormalizeFastSafe(Vector_4_In inVector, float errVal, float magSqThreshold)
	{
		Vector_4 outVect;
		float mag2 = V4MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrtFast(mag2);
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
			outVect.z = inVector.z*invMag;
			outVect.w = inVector.w*invMag;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = outVect.w = errVal;
		}
		return outVect;
	}
#endif // V4NORMALIZEFASTSAFE

#ifndef V4DOT
	__forceinline float V4Dot(Vector_4_In a, Vector_4_In b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
	}
#endif // V4DOT

#ifndef V4DOTV
	__forceinline Vector_4_Out V4DotV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4Dot(a, b);
		return outVect;
	}
#endif // V4DOTV

#ifndef V4AVERAGE
	__forceinline Vector_4_Out V4Average(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = 0.5f * (a.x + b.x);
		outVect.y = 0.5f * (a.y + b.y);
		outVect.z = 0.5f * (a.z + b.z);
		outVect.w = 0.5f * (a.w + b.w);
		return outVect;
	}
#endif // V4AVERAGE

#ifndef V4LERP
	__forceinline Vector_4_Out V4Lerp( float t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t, a.x, b.x);
		outVect.y = FPLerp(t, a.y, b.y);
		outVect.z = FPLerp(t, a.z, b.z);
		outVect.w = FPLerp(t, a.w, b.w);
		return outVect;
	}
#endif // V4LERP

#ifndef V4LERP_V
	__forceinline Vector_4_Out V4Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t.x, a.x, b.x);
		outVect.y = FPLerp(t.y, a.y, b.y);
		outVect.z = FPLerp(t.z, a.z, b.z);
		outVect.w = FPLerp(t.w, a.w, b.w);
		return outVect;
	}
#endif // V4LERP_V

#ifndef V4POW
	__forceinline Vector_4_Out V4Pow( Vector_4_In x, Vector_4_In y )
	{
		Vector_4 outVect;
		outVect.x = FPPow(x.x, y.x);
		outVect.y = FPPow(x.y, y.y);
		outVect.z = FPPow(x.z, y.z);
		outVect.w = FPPow(x.w, y.w);
		return outVect;
	}
#endif // V4POW

#ifndef V4EXPT
	__forceinline Vector_4_Out V4Expt( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPExpt(x.x);
		outVect.y = FPExpt(x.y);
		outVect.z = FPExpt(x.z);
		outVect.w = FPExpt(x.w);
		return outVect;
	}
#endif // V4EXPT

#ifndef V4LOG2
	__forceinline Vector_4_Out V4Log2( Vector_4_In x )
	{
		// log2(f) = ln(f)/ln(2)
		Vector_4 outVect;
		outVect.x = FPLog2(x.x);
		outVect.y = FPLog2(x.y);
		outVect.z = FPLog2(x.z);
		outVect.w = FPLog2(x.w);
		return outVect;
	}
#endif

#ifndef V4LOG10
	__forceinline Vector_4_Out V4Log10( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPLog10(x.x);
		outVect.y = FPLog10(x.y);
		outVect.z = FPLog10(x.z);
		outVect.w = FPLog10(x.w);
		return outVect;
	}
#endif // V4LOG10

#ifndef V4MODULUS
	__forceinline Vector_4_Out V4Modulus( Vector_4_In inVector, Vector_4_In inMod )
	{
		Vector_4 outVect;
		outVect.x = inVector.x - (inMod.x * static_cast<int>( inVector.x / inMod.x ));
		outVect.y = inVector.y - (inMod.y * static_cast<int>( inVector.y / inMod.y ));
		outVect.z = inVector.z - (inMod.z * static_cast<int>( inVector.z / inMod.z ));
		outVect.w = inVector.w - (inMod.w * static_cast<int>( inVector.w / inMod.w ));
		return outVect;
	}
#endif // V4MODULUS

	//============================================================================
	// Magnitude and distance

#ifndef V4SQRT
	__forceinline Vector_4_Out V4Sqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPSqrt(v.x);
		outVect.y = FPSqrt(v.y);
		outVect.z = FPSqrt(v.z);
		outVect.w = FPSqrt(v.w);
		return outVect;
	}
#endif // V4SQRT

#ifndef V4SQRTSAFE
	__forceinline Vector_4_Out V4SqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPSqrtSafe( v.x, errVal );
		outVect.y = FPSqrtSafe( v.y, errVal );
		outVect.z = FPSqrtSafe( v.z, errVal );
		outVect.w = FPSqrtSafe( v.w, errVal );
		return outVect;
	}
#endif // V4SQRTSAFE

#ifndef V4SQRTFAST
	__forceinline Vector_4_Out V4SqrtFast( Vector_4_In v )
	{
		return V4Sqrt( v );
	}
#endif // V4SQRTFAST

#ifndef V4SQRTFASTSAFE
	__forceinline Vector_4_Out V4SqrtFastSafe( Vector_4_In v, Vector_4_In errValVect )
	{
		return V4SqrtSafe( v, errValVect );
	}
#endif // V4SQRTFASTSAFE

#ifndef V4INVSQRT
	__forceinline Vector_4_Out V4InvSqrt(Vector_4_In v)
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrt(v.x);
		outVect.y = FPInvSqrt(v.y);
		outVect.z = FPInvSqrt(v.z);
		outVect.w = FPInvSqrt(v.w);
		return outVect;
	}
#endif // V4INVSQRT

#ifndef V4INVSQRTSAFE
	__forceinline Vector_4_Out V4InvSqrtSafe(Vector_4_In v, float errVal)
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtSafe(v.x, errVal);
		outVect.y = FPInvSqrtSafe(v.y, errVal);
		outVect.z = FPInvSqrtSafe(v.z, errVal);
		outVect.w = FPInvSqrtSafe(v.w, errVal);
		return outVect;
	}
#endif // V4INVSQRTSAFE

#ifndef V4INVSQRTFAST
	__forceinline Vector_4_Out V4InvSqrtFast(Vector_4_In v)
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFast(v.x);
		outVect.y = FPInvSqrtFast(v.y);
		outVect.z = FPInvSqrtFast(v.z);
		outVect.w = FPInvSqrtFast(v.w);
		return outVect;
	}
#endif // V4INVSQRTFAST

#ifndef V4INVSQRTFASTSAFE
	__forceinline Vector_4_Out V4InvSqrtFastSafe(Vector_4_In v, float errVal)
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFastSafe(v.x, errVal);
		outVect.y = FPInvSqrtFastSafe(v.y, errVal);
		outVect.z = FPInvSqrtFastSafe(v.z, errVal);
		outVect.w = FPInvSqrtFastSafe(v.w, errVal);
		return outVect;
	}
#endif // V4INVSQRTFASTSAFE

#ifndef V4MAG
	__forceinline float V4Mag( Vector_4_In v )
	{
		return FPSqrt( V4Dot( v, v ) );
	}
#endif // V4MAG

#ifndef V4MAGV
	__forceinline Vector_4_Out V4MagV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4Mag( v );
		return outVect;
	}
#endif // V4MAGV

#ifndef V4MAGFAST
	__forceinline float V4MagFast( Vector_4_In v )
	{
		return V4Mag( v );
	}
#endif // V4MAGFAST

#ifndef V4MAGVFAST
	__forceinline Vector_4_Out V4MagVFast( Vector_4_In v )
	{
		return V4MagV( v );
	}
#endif // V4MAGVFAST

#ifndef V4MAGSQUARED
	__forceinline float V4MagSquared( Vector_4_In v )
	{
		return V4Dot( v, v );
	}
#endif // V4MAGSQUARED

#ifndef V4MAGSQUAREDV
	__forceinline Vector_4_Out V4MagSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4MagSquared( v );
		return outVect;
	}
#endif // V4MAGSQUAREDV

#ifndef V4INVMAG
	__forceinline float V4InvMag( Vector_4_In v )
	{
		return FPInvSqrt( V4Dot( v, v ) );
	}
#endif // V4INVMAG

#ifndef V4INVMAGSAFE
	__forceinline float V4InvMagSafe( Vector_4_In v, float errVal )
	{
		float dot = V4Dot( v, v );
		return FPInvSqrtSafe( dot, errVal );
	}
#endif // V4INVMAGSAFE

#ifndef V4INVMAGFAST
	__forceinline float V4InvMagFast( Vector_4_In v )
	{
		return FPInvSqrtFast( V4Dot( v, v ) );
	}
#endif // V4INVMAGFAST

#ifndef V4INVMAGFASTSAFE
	__forceinline float V4InvMagFastSafe( Vector_4_In v, float errVal )
	{
		float dot = V4Dot( v, v );
		return FPInvSqrtFastSafe( dot, errVal );
	}
#endif // V4INVMAGFASTSAFE

#ifndef V4INVMAGV
	__forceinline Vector_4_Out V4InvMagV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMag( v );
		return outVect;
	}
#endif // V4INVMAGV

#ifndef V4INVMAGVSAFE
	__forceinline Vector_4_Out V4InvMagVSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagSafe( v, errVal );
		return outVect;
	}
#endif // V4INVMAGVSAFE

#ifndef V4INVMAGVFAST
	__forceinline Vector_4_Out V4InvMagVFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagFast( v );
		return outVect;
	}
#endif // V4INVMAGVFAST

#ifndef V4INVMAGVFASTSAFE
	__forceinline Vector_4_Out V4InvMagVFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagFastSafe( v, errVal );
		return outVect;
	}
#endif // V4INVMAGVFASTSAFE

#ifndef V4INVMAGSQUARED
	__forceinline float V4InvMagSquared( Vector_4_In v )
	{
		return 1.0f/V4MagSquared( v );
	}
#endif // V4INVMAGSQUARED

#ifndef V4INVMAGSQUAREDSAFE
	__forceinline float V4InvMagSquaredSafe( Vector_4_In v, float errVal )
	{
		float magSq = V4MagSquared( v );
		return FPIfGtZeroThenElse( magSq, 1.0f/magSq, errVal );
	}
#endif // V4INVMAGSQUAREDSAFE

#ifndef V4INVMAGSQUAREDFAST
	__forceinline float V4InvMagSquaredFast( Vector_4_In v )
	{
		return FPInvertFast( V4MagSquared( v ) );
	}
#endif // V4INVMAGSQUAREDFAST

#ifndef V4INVMAGSQUAREDFASTSAFE
	__forceinline float V4InvMagSquaredFastSafe( Vector_4_In v, float errVal )
	{
		return FPInvertFastSafe( V4MagSquared( v ), errVal );
	}
#endif // V4INVMAGSQUAREDFASTSAFE

#ifndef V4INVMAGSQUAREDV
	__forceinline Vector_4_Out V4InvMagSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagSquared( v );
		return outVect;
	}
#endif // V4INVMAGSQUAREDV

#ifndef V4INVMAGSQUAREDVSAFE
	__forceinline Vector_4_Out V4InvMagSquaredVSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagSquaredSafe( v, errVal );
		return outVect;
	}
#endif // V4INVMAGSQUAREDVSAFE

#ifndef V4INVMAGSQUAREDVFAST
	__forceinline Vector_4_Out V4InvMagSquaredVFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagSquaredFast( v );
		return outVect;
	}
#endif // V4INVMAGSQUAREDVFAST

#ifndef V4INVMAGSQUAREDVFASTSAFE
	__forceinline Vector_4_Out V4InvMagSquaredVFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvMagSquaredFastSafe( v, errVal );
		return outVect;
	}
#endif // V4INVMAGSQUAREDVFASTSAFE

#ifndef V4DIST
	__forceinline float V4Dist(Vector_4_In a, Vector_4_In b)
	{
		return V4Mag( V4Subtract( a, b ) );
	}
#endif // V4DIST

#ifndef V4DISTFAST
	__forceinline float V4DistFast(Vector_4_In a, Vector_4_In b)
	{
		return V4Dist( a, b );
	}
#endif // V4DISTFAST

#ifndef V4DISTV
	__forceinline Vector_4_Out V4DistV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4Dist( a, b );
		return outVect;
	}
#endif // V4DISTV

#ifndef V4DISTVFAST
	__forceinline Vector_4_Out V4DistVFast(Vector_4_In a, Vector_4_In b)
	{
		return V4DistV( a, b );
	}
#endif // V4DISTVFAST

#ifndef V4INVDIST
	__forceinline float V4InvDist(Vector_4_In a, Vector_4_In b)
	{
		return V4InvMag( V4Subtract( a, b ) );
	}
#endif // V4INVDIST

#ifndef V4INVDISTSAFE
	__forceinline float V4InvDistSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V4InvMagSafe( V4Subtract( a, b ), errVal );
	}
#endif // V4INVDISTSAFE

#ifndef V4INVDISTFAST
	__forceinline float V4InvDistFast(Vector_4_In a, Vector_4_In b)
	{
		return V4InvMagFast( V4Subtract( a, b ) );
	}
#endif // V4INVDISTFAST

#ifndef V4INVDISTFASTSAFE
	__forceinline float V4InvDistFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V4InvMagFastSafe( V4Subtract( a, b ), errVal );
	}
#endif // V4INVDISTFASTSAFE

#ifndef V4INVDISTV
	__forceinline Vector_4_Out V4InvDistV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDist( a, b );
		return outVect;
	}
#endif // V4INVDISTV

#ifndef V4INVDISTVSAFE
	__forceinline Vector_4_Out V4InvDistVSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistSafe( a, b, errVal );
		return outVect;
	}
#endif // V4INVDISTVSAFE

#ifndef V4INVDISTVFAST
	__forceinline Vector_4_Out V4InvDistVFast(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistFast( a, b );
		return outVect;
	}
#endif // V4INVDISTVFAST

#ifndef V4INVDISTVFASTSAFE
	__forceinline Vector_4_Out V4InvDistVFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V4INVDISTVFASTSAFE

#ifndef V4DISTSQUARED
	__forceinline float V4DistSquared(Vector_4_In a, Vector_4_In b)
	{
		return V4MagSquared( V4Subtract( a, b ) );
	}
#endif // V4DISTSQUARED

#ifndef V4DISTSQUAREDV
	__forceinline Vector_4_Out V4DistSquaredV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4DistSquared( a, b );
		return outVect;
	}
#endif // V4DISTSQUAREDV

#ifndef V4INVDISTSQUARED
	__forceinline float V4InvDistSquared(Vector_4_In a, Vector_4_In b)
	{
		return V4InvMagSquared( V4Subtract( a, b ) );
	}
#endif // V4INVDISTSQUARED

#ifndef V4INVDISTSQUAREDSAFE
	__forceinline float V4InvDistSquaredSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V4InvMagSquaredSafe( V4Subtract( a, b ), errVal );
	}
#endif // V4INVDISTSQUAREDSAFE

#ifndef V4INVDISTSQUAREDFAST
	__forceinline float V4InvDistSquaredFast(Vector_4_In a, Vector_4_In b)
	{
		return V4InvMagSquaredFast( V4Subtract( a, b ) );
	}
#endif // V4INVDISTSQUAREDFAST

#ifndef V4INVDISTSQUAREDFASTSAFE
	__forceinline float V4InvDistSquaredFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V4InvMagSquaredFastSafe( V4Subtract( a, b ), errVal );
	}
#endif // V4INVDISTSQUAREDFASTSAFE

#ifndef V4INVDISTSQUAREDV
	__forceinline Vector_4_Out V4InvDistSquaredV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistSquared( a, b );
		return outVect;
	}
#endif // V4INVDISTSQUAREDV

#ifndef V4INVDISTSQUAREDVSAFE
	__forceinline Vector_4_Out V4InvDistSquaredVSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistSquaredSafe( a, b, errVal );
		return outVect;
	}
#endif // V4INVDISTSQUAREDVSAFE

#ifndef V4INVDISTSQUAREDVFAST
	__forceinline Vector_4_Out V4InvDistSquaredVFast(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistSquaredFast( a, b );
		return outVect;
	}
#endif // V4INVDISTSQUAREDVFAST

#ifndef V4INVDISTSQUAREDVFASTSAFE
	__forceinline Vector_4_Out V4InvDistSquaredVFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4InvDistSquaredFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V4INVDISTSQUAREDVFASTSAFE


	//============================================================================
	// Conversion functions

#ifndef V4FLOATTOINTRAW
	template <int exponent>
	__forceinline Vector_4_Out V4FloatToIntRaw(Vector_4_In inVector)
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

		TempResult.i = static_cast<int>(inVector.y * multiplier);
		outVect.y = TempResult.f;

		TempResult.i = static_cast<int>(inVector.z * multiplier);
		outVect.z = TempResult.f;

		TempResult.i = static_cast<int>(inVector.w * multiplier);
		outVect.w = TempResult.f;

		return outVect;
	}
#endif // V4FLOATTOINTRAW

#ifndef V4INTTOFLOATRAW
	template <int exponent>
	__forceinline Vector_4_Out V4IntToFloatRaw(Vector_4_In inVector)
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
		Temp1.f = inVector.y;
		outVect.y = static_cast<float>(Temp1.i) * invDiv;
		Temp1.f = inVector.z;
		outVect.z = static_cast<float>(Temp1.i) * invDiv;
		Temp1.f = inVector.w;
		outVect.w = static_cast<float>(Temp1.i) * invDiv;
		return outVect;
	}
#endif // V4INTTOFLOATRAW

#ifndef V4ROUNDTONEARESTINT
	__forceinline Vector_4_Out V4RoundToNearestInt(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x+(FPIfGteZeroThenElse(inVector.x, 0.5f, -0.5f))) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y+(FPIfGteZeroThenElse(inVector.y, 0.5f, -0.5f))) );
		outVect.z = static_cast<float>( static_cast<int>(inVector.z+(FPIfGteZeroThenElse(inVector.z, 0.5f, -0.5f))) );
		outVect.w = static_cast<float>( static_cast<int>(inVector.w+(FPIfGteZeroThenElse(inVector.w, 0.5f, -0.5f))) );
		return outVect;
	}
#endif // V4ROUNDTONEARESTINT

#ifndef V4ROUNDTONEARESTINTZERO
	__forceinline Vector_4_Out V4RoundToNearestIntZero(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y) );
		outVect.z = static_cast<float>( static_cast<int>(inVector.z) );
		outVect.w = static_cast<float>( static_cast<int>(inVector.w) );
		return outVect;
	}
#endif // V4ROUNDTONEARESTINTZERO

#ifndef V4ROUNDTONEARESTINTNEGINF
	__forceinline Vector_4_Out V4RoundToNearestIntNegInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPFloor( inVector.x );
		outVect.y = FPFloor( inVector.y );
		outVect.z = FPFloor( inVector.z );
		outVect.w = FPFloor( inVector.w );
		return outVect;
	}
#endif // V4ROUNDTONEARESTINTNEGINF

#ifndef V4ROUNDTONEARESTINTPOSINF
	__forceinline Vector_4_Out V4RoundToNearestIntPosInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPCeil( inVector.x );
		outVect.y = FPCeil( inVector.y );
		outVect.z = FPCeil( inVector.z );
		outVect.w = FPCeil( inVector.w );
		return outVect;
	}
#endif // V4ROUNDTONEARESTINTPOSINF

	//============================================================================
	// Trigonometry

#ifndef V4CANONICALIZEANGLE
	__forceinline Vector_4_Out V4CanonicalizeAngle( Vector_4_In inVector )
	{
		// Not optimal, just here as a backup for vectorized version.

		Vector_4 _zero = V4Constant(V_ZERO);
		Vector_4 _one = V4Constant(V_ONE);
		Vector_4 _negone = V4Constant(V_NEGONE);
		Vector_4 _half = V4Constant(V_HALF);
		Vector_4 piConstants = V4Constant<U32_ONE_OVER_PI,U32_PI,U32_TWO_PI,U32_NEG_PI>();
		Vector_4 _1_over_pi = V4SplatX(piConstants);
		Vector_4 _pi = V4SplatY(piConstants);
		Vector_4 _2pi = V4SplatZ(piConstants);
		Vector_4 _negPi = V4SplatW(piConstants);
		Vector_4 isNegative = V4IsLessThanV( inVector, _zero );
		Vector_4 negOrPosOne = V4SelectFT( isNegative, _one, _negone );
		Vector_4 inputMinusPi = V4Subtract( inVector, V4SelectFT(isNegative, _pi, _negPi ) );
		Vector_4 numPiPastBoundary = V4Scale( inputMinusPi, _1_over_pi );
		Vector_4 num2PiPastBoundary = V4AddScaled( negOrPosOne, numPiPastBoundary, _half );
		Vector_4 subtractFactor = V4RoundToNearestIntZero( num2PiPastBoundary );
		Vector_4 canonAngle = V4SubtractScaled( inVector, _2pi, subtractFactor );
		return canonAngle;
	}
#endif // V4CANONICALIZEANGLE

#ifndef V4SINANDCOS
	__forceinline void V4SinAndCos( Vector_4_InOut inOutSine, Vector_4_InOut inOutCosine, Vector_4_In inVector )
	{
		FPSinAndCos(inOutSine.x, inOutCosine.x, inVector.x);
		FPSinAndCos(inOutSine.y, inOutCosine.y, inVector.y);
		FPSinAndCos(inOutSine.z, inOutCosine.z, inVector.z);
		FPSinAndCos(inOutSine.w, inOutCosine.w, inVector.w);
	}
#endif // V4SINANDCOS

#ifndef V4SIN
	__forceinline Vector_4_Out V4Sin( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPSin( inVector.x );
		outVect.y = FPSin( inVector.y );
		outVect.z = FPSin( inVector.z );
		outVect.w = FPSin( inVector.w );
		return outVect;
	}
#endif // V4SIN

#ifndef V4COS
	__forceinline Vector_4_Out V4Cos( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPCos( inVector.x );
		outVect.y = FPCos( inVector.y );
		outVect.z = FPCos( inVector.z );
		outVect.w = FPCos( inVector.w );
		return outVect;
	}
#endif // V4COS

#ifndef V4TAN
	__forceinline Vector_4_Out V4Tan( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPTan( inVector.x );
		outVect.y = FPTan( inVector.y );
		outVect.z = FPTan( inVector.z );
		outVect.w = FPTan( inVector.w );
		return outVect;
	}
#endif // V4TAN

#ifndef V4ARCSIN
	__forceinline Vector_4_Out V4Arcsin( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPASin( inVector.x );
		outVect.y = FPASin( inVector.y );
		outVect.z = FPASin( inVector.z );
		outVect.w = FPASin( inVector.w );
		return outVect;
	}
#endif // V4ARCSIN

#ifndef V4ARCCOS
	__forceinline Vector_4_Out V4Arccos( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPACos( inVector.x );
		outVect.y = FPACos( inVector.y );
		outVect.z = FPACos( inVector.z );
		outVect.w = FPACos( inVector.w );
		return outVect;
	}
#endif // V4ARCCOS

#ifndef V4ARCTAN
	__forceinline Vector_4_Out V4Arctan( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPATan( inVector.x );
		outVect.y = FPATan( inVector.y );
		outVect.z = FPATan( inVector.z );
		outVect.w = FPATan( inVector.w );
		return outVect;
	}
#endif // V4ARCTAN

#ifndef V4ARCTAN2
	__forceinline Vector_4_Out V4Arctan2( Vector_4_In inVectorY, Vector_4_In inVectorX )
	{
		Vector_4 outVect;
		outVect.x = FPATan2( inVectorY.x, inVectorX.x );
		outVect.y = FPATan2( inVectorY.y, inVectorX.y );
		outVect.z = FPATan2( inVectorY.z, inVectorX.z );
		outVect.w = FPATan2( inVectorY.w, inVectorX.w );
		return outVect;
	}
#endif // V4ARCTAN2

#ifndef V4SINANDCOSFAST
	__forceinline void V4SinAndCosFast( Vector_4_InOut inOutSine, Vector_4_InOut inOutCosine, Vector_4_In inVector )
	{
		V4SinAndCos( inOutSine, inOutCosine, inVector );
	}
#endif // V4SINANDCOSFAST

#ifndef V4SINFAST
	__forceinline Vector_4_Out V4SinFast( Vector_4_In inVector )
	{
		return V4Sin( inVector );
	}
#endif // V4SINFAST

#ifndef V4COSFAST
	__forceinline Vector_4_Out V4CosFast( Vector_4_In inVector )
	{
		return V4Cos( inVector );
	}
#endif // V4COSFAST

#ifndef V4TANFAST
	__forceinline Vector_4_Out V4TanFast( Vector_4_In inVector )
	{
		return V4Tan( inVector );
	}
#endif // V4TANFAST

#ifndef V4ARCSINFAST
	__forceinline Vector_4_Out V4ArcsinFast( Vector_4_In inVector )
	{
		return V4Arcsin( inVector );
	}
#endif // V4ARCSINFAST

#ifndef V4ARCCOSFAST
	__forceinline Vector_4_Out V4ArccosFast( Vector_4_In inVector )
	{
		return V4Arccos( inVector );
	}
#endif // V4ARCCOSFAST

#ifndef V4ARCTANFAST
	__forceinline Vector_4_Out V4ArctanFast( Vector_4_In inVector )
	{
		return V4Arctan( inVector );
	}
#endif // V4ARCTANFAST

#ifndef V4ARCTAN2FAST
	__forceinline Vector_4_Out V4Arctan2Fast( Vector_4_In inVectorY, Vector_4_In inVectorX )
	{
		return V4Arctan2( inVectorY, inVectorX );
	}
#endif // V4ARCTAN2FAST

#ifndef V4SAMESIGNV
	__forceinline Vector_4_Out V4SameSignV(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = (*(int*)( &inVector1.x ) & 0x80000000) == (*(int*)( &inVector2.x ) & 0x80000000) ? allBitsF : 0;
		outVect.y = (*(int*)( &inVector1.y ) & 0x80000000) == (*(int*)( &inVector2.y ) & 0x80000000) ? allBitsF : 0;
		outVect.z = (*(int*)( &inVector1.z ) & 0x80000000) == (*(int*)( &inVector2.z ) & 0x80000000) ? allBitsF : 0;
		outVect.w = (*(int*)( &inVector1.w ) & 0x80000000) == (*(int*)( &inVector2.w ) & 0x80000000) ? allBitsF : 0;
		return outVect;
	}
#endif // V4SAMESIGNV

#ifndef V4SAMESIGNALL
	__forceinline unsigned int V4SameSignAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return (
			(*(int*)( &inVector1.x ) & 0x80000000) == (*(int*)( &inVector2.x ) & 0x80000000) &&
			(*(int*)( &inVector1.y ) & 0x80000000) == (*(int*)( &inVector2.y ) & 0x80000000) &&
			(*(int*)( &inVector1.z ) & 0x80000000) == (*(int*)( &inVector2.z ) & 0x80000000) &&
			(*(int*)( &inVector1.w ) & 0x80000000) == (*(int*)( &inVector2.w ) & 0x80000000)
			) ? 1u : 0u;
	}
#endif // V4SAMESIGNALL

#ifndef V4ISEVENV
	__forceinline Vector_4_Out V4IsEvenV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = ((int)( inVector.x ) & 1) == 0 ? allBitsF : 0;
		outVect.y = ((int)( inVector.y ) & 1) == 0 ? allBitsF : 0;
		outVect.z = ((int)( inVector.z ) & 1) == 0 ? allBitsF : 0;
		outVect.w = ((int)( inVector.w ) & 1) == 0 ? allBitsF : 0;
		return outVect;
	}
#endif // V4ISEVENV

#ifndef V4ISODDV
	__forceinline Vector_4_Out V4IsOddV( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = ((int)( inVector.x ) & 1) != 0 ? allBitsF : 0;
		outVect.y = ((int)( inVector.y ) & 1) != 0 ? allBitsF : 0;
		outVect.z = ((int)( inVector.z ) & 1) != 0 ? allBitsF : 0;
		outVect.w = ((int)( inVector.w ) & 1) != 0 ? allBitsF : 0;
		return outVect;
	}
#endif // V4ISODDV

#ifndef V4SLOWINOUT
	__forceinline Vector_4_Out V4SlowInOut( Vector_4_In t )
	{
		Vector_4 outVect;
		outVect.x = FPSlowInOut( t.x );
		outVect.y = FPSlowInOut( t.y );
		outVect.z = FPSlowInOut( t.z );
		outVect.w = FPSlowInOut( t.w );
		return outVect;
	}
#endif // V4SLOWINOUT

#ifndef V4SLOWIN
	__forceinline Vector_4_Out V4SlowIn( Vector_4_In t )
	{
		Vector_4 outVect;
		outVect.x = FPSlowIn( t.x );
		outVect.y = FPSlowIn( t.y );
		outVect.z = FPSlowIn( t.z );
		outVect.w = FPSlowIn( t.w );
		return outVect;
	}
#endif // V4SLOWIN

#ifndef V4SLOWOUT
	__forceinline Vector_4_Out V4SlowOut( Vector_4_In t )
	{
		Vector_4 outVect;
		outVect.x = FPSlowOut( t.x );
		outVect.y = FPSlowOut( t.y );
		outVect.z = FPSlowOut( t.z );
		outVect.w = FPSlowOut( t.w );
		return outVect;
	}
#endif // V4SLOWOUT

#ifndef V4BELLINOUT
	__forceinline Vector_4_Out V4BellInOut( Vector_4_In t )
	{
		Vector_4 outVect;
		outVect.x = FPBellInOut( t.x );
		outVect.y = FPBellInOut( t.y );
		outVect.z = FPBellInOut( t.z );
		outVect.w = FPBellInOut( t.w );
		return outVect;
	}
#endif // V4BELLINOUT

#ifndef V4RANGE
	__forceinline Vector_4_Out V4Range( Vector_4_In t, Vector_4_In lower, Vector_4_In upper )
	{
		Vector_4 outVect;
		outVect.x = FPRange( t.x, lower.x, upper.x );
		outVect.y = FPRange( t.y, lower.y, upper.y );
		outVect.z = FPRange( t.z, lower.z, upper.z );
		outVect.w = FPRange( t.w, lower.w, upper.w );
		return outVect;
	}
#endif // V4RANGE

#ifndef V4RANGESAFE
	__forceinline Vector_4_Out V4RangeSafe( Vector_4_In t, Vector_4_In lower, Vector_4_In upper, Vector_4_In errValVect )
	{
		Vector_4 outVect;
		outVect.x = FPRangeSafe( t.x, lower.x, upper.x, errValVect.x );
		outVect.y = FPRangeSafe( t.y, lower.y, upper.y, errValVect.y );
		outVect.z = FPRangeSafe( t.z, lower.z, upper.z, errValVect.z );
		outVect.w = FPRangeSafe( t.w, lower.w, upper.w, errValVect.w );
		return outVect;
	}
#endif // V4RANGESAFE

#ifndef V4RANGEFAST
	__forceinline Vector_4_Out V4RangeFast( Vector_4_In t, Vector_4_In lower, Vector_4_In upper )
	{
		Vector_4 outVect;
		outVect.x = FPRangeFast( t.x, lower.x, upper.x );
		outVect.y = FPRangeFast( t.y, lower.y, upper.y );
		outVect.z = FPRangeFast( t.z, lower.z, upper.z );
		outVect.w = FPRangeFast( t.w, lower.w, upper.w );
		return outVect;
	}
#endif // V4RANGEFAST

#ifndef V4RANGECLAMP
	__forceinline Vector_4_Out V4RangeClamp( Vector_4_In t, Vector_4_In lower, Vector_4_In upper )
	{
		Vector_4 outVect;
		outVect.x = FPRangeClamp( t.x, lower.x, upper.x );
		outVect.y = FPRangeClamp( t.y, lower.y, upper.y );
		outVect.z = FPRangeClamp( t.z, lower.z, upper.z );
		outVect.w = FPRangeClamp( t.w, lower.w, upper.w );
		return outVect;
	}
#endif // V4RANGECLAMP

#ifndef V4RANGECLAMPFAST
	__forceinline Vector_4_Out V4RangeClampFast( Vector_4_In t, Vector_4_In lower, Vector_4_In upper )
	{
		Vector_4 outVect;
		outVect.x = FPRangeClampFast( t.x, lower.x, upper.x );
		outVect.y = FPRangeClampFast( t.y, lower.y, upper.y );
		outVect.z = FPRangeClampFast( t.z, lower.z, upper.z );
		outVect.w = FPRangeClampFast( t.w, lower.w, upper.w );
		return outVect;
	}
#endif // V4RANGECLAMPFAST

#ifndef V4RAMP
	__forceinline Vector_4_Out V4Ramp( Vector_4_In x, Vector_4_In funcInA, Vector_4_In funcInB, Vector_4_In funcOutA, Vector_4_In funcOutB )
	{
		Vector_4 outVect;
		outVect.x = FPRamp( x.x, funcInA.x, funcInB.x, funcOutA.x, funcOutB.x );
		outVect.y = FPRamp( x.y, funcInA.y, funcInB.y, funcOutA.y, funcOutB.y );
		outVect.z = FPRamp( x.z, funcInA.z, funcInB.z, funcOutA.z, funcOutB.z );
		outVect.w = FPRamp( x.w, funcInA.w, funcInB.w, funcOutA.w, funcOutB.w );
		return outVect;
	}
#endif // V4RAMP

#ifndef V4RAMPFAST
	__forceinline Vector_4_Out V4RampFast( Vector_4_In x, Vector_4_In funcInA, Vector_4_In funcInB, Vector_4_In funcOutA, Vector_4_In funcOutB )
	{
		Vector_4 outVect;
		outVect.x = FPRampFast( x.x, funcInA.x, funcInB.x, funcOutA.x, funcOutB.x );
		outVect.y = FPRampFast( x.y, funcInA.y, funcInB.y, funcOutA.y, funcOutB.y );
		outVect.z = FPRampFast( x.z, funcInA.z, funcInB.z, funcOutA.z, funcOutB.z );
		outVect.w = FPRampFast( x.w, funcInA.w, funcInB.w, funcOutA.w, funcOutB.w );
		return outVect;
	}
#endif // V4RAMPFAST

	//============================================================================
	// Comparison functions

#ifndef V4ISBETWEENNEGANDPOSBOUNDS
	__forceinline unsigned int V4IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector )
	{
		return (	testVector.x <= boundsVector.x && testVector.x >= -boundsVector.x &&
					testVector.y <= boundsVector.y && testVector.y >= -boundsVector.y &&
					testVector.z <= boundsVector.z && testVector.z >= -boundsVector.z &&
					testVector.w <= boundsVector.w && testVector.w >= -boundsVector.w ? 1u : 0u );
	}
#endif // V4ISBETWEENNEGANDPOSBOUNDS

#ifndef V4ISZEROALL
	__forceinline unsigned int V4IsZeroAll(Vector_4_In inVector)
	{
		return ( inVector.x == 0.0f && inVector.y == 0.0f && inVector.z == 0.0f && inVector.w == 0.0f ? 1u : 0u );
	}
#endif // V4ISZEROALL

#ifndef V4ISZERONONE
	__forceinline unsigned int V4IsZeroNone(Vector_4_In inVector)
	{
		return ( inVector.x != 0.0f && inVector.y != 0.0f && inVector.z != 0.0f && inVector.w != 0.0f ? 1u : 0u );
	}
#endif // V4ISZERONONE

#ifndef V4ISEQUALALL
	__forceinline unsigned int V4IsEqualAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x == inVector2.x &&
				 inVector1.y == inVector2.y &&
				 inVector1.z == inVector2.z &&
				 inVector1.w == inVector2.w ? 1u : 0u );
	}
#endif // V4ISEQUALALL

#ifndef V4ISEQUALNONE
	__forceinline unsigned int V4IsEqualNone(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x != inVector2.x &&
				 inVector1.y != inVector2.y &&
				 inVector1.z != inVector2.z &&
				 inVector1.w != inVector2.w ? 1u : 0u );
	}
#endif // V4ISEQUALNONE

#ifndef V4ISEQUALV
	__forceinline Vector_4_Out V4IsEqualV(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = (inVector1.x == inVector2.x ? allBitsF : 0.0f);
		outVect.y = (inVector1.y == inVector2.y ? allBitsF : 0.0f);
		outVect.z = (inVector1.z == inVector2.z ? allBitsF : 0.0f);
		outVect.w = (inVector1.w == inVector2.w ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISEQUALV

#ifndef V4ISEQUALINTALL
	__forceinline unsigned int V4IsEqualIntAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;
		Temp5.f = inVector1.z;
		Temp6.f = inVector2.z;
		Temp7.f = inVector1.w;
		Temp8.f = inVector2.w;

		return (Temp1.i == Temp2.i &&
				Temp3.i == Temp4.i &&
				Temp5.i == Temp6.i &&
				Temp7.i == Temp8.i ? 1u : 0u );
	}
#endif // V4ISEQUALINTALL

#ifndef V4ISEQUALINTNONE
	__forceinline unsigned int V4IsEqualIntNone(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;
		Temp5.f = inVector1.z;
		Temp6.f = inVector2.z;
		Temp7.f = inVector1.w;
		Temp8.f = inVector2.w;

		return (Temp1.i != Temp2.i &&
				Temp3.i != Temp4.i &&
				Temp5.i != Temp6.i &&
				Temp7.i != Temp8.i ? 1u : 0u );
	}
#endif // V4ISEQUALINTNONE

#ifndef V4ISEQUALINTV
	__forceinline Vector_4_Out V4IsEqualIntV(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;
		Temp5.f = inVector1.z;
		Temp6.f = inVector2.z;
		Temp7.f = inVector1.w;
		Temp8.f = inVector2.w;

		Vector_4 outVect;
		outVect.x = (Temp1.i == Temp2.i ? allBitsF : 0.0f);
		outVect.y = (Temp3.i == Temp4.i ? allBitsF : 0.0f);
		outVect.z = (Temp5.i == Temp6.i ? allBitsF : 0.0f);
		outVect.w = (Temp7.i == Temp8.i ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISEQUALINTV

#ifndef V4ISCLOSEV
	__forceinline Vector_4_Out V4IsCloseV(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		Vector_4 outVect;
		outVect.x = (( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) ? allBitsF : 0.0f);
		outVect.z = (( inVector1.z >= inVector2.z - eps && inVector1.z <= inVector2.z + eps ) ? allBitsF : 0.0f);
		outVect.w = (( inVector1.w >= inVector2.w - eps && inVector1.w <= inVector2.w + eps ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISCLOSEV

#ifndef V4ISCLOSEV_V
	__forceinline Vector_4_Out V4IsCloseV(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		Vector_4 outVect;
		outVect.x = (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) ? allBitsF : 0.0f);
		outVect.z = (( inVector1.z >= inVector2.z - eps.z && inVector1.z <= inVector2.z + eps.z ) ? allBitsF : 0.0f);
		outVect.w = (( inVector1.w >= inVector2.w - eps.w && inVector1.w <= inVector2.w + eps.w ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISCLOSEV_V

#ifndef V4ISNOTCLOSEV
	__forceinline Vector_4_Out V4IsNotCloseV(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		Vector_4 outVect;
		outVect.x = (( inVector1.x < inVector2.x - eps || inVector1.x > inVector2.x + eps ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y < inVector2.y - eps || inVector1.y > inVector2.y + eps ) ? allBitsF : 0.0f);
		outVect.z = (( inVector1.z < inVector2.z - eps || inVector1.z > inVector2.z + eps ) ? allBitsF : 0.0f);
		outVect.w = (( inVector1.w < inVector2.w - eps || inVector1.w > inVector2.w + eps ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISNOTCLOSEV

#ifndef V4ISNOTCLOSEV_V
	__forceinline Vector_4_Out V4IsNotCloseV(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		Vector_4 outVect;
		outVect.x = (( inVector1.x < inVector2.x - eps.x || inVector1.x > inVector2.x + eps.x ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y < inVector2.y - eps.y || inVector1.y > inVector2.y + eps.y ) ? allBitsF : 0.0f);
		outVect.z = (( inVector1.z < inVector2.z - eps.z || inVector1.z > inVector2.z + eps.z ) ? allBitsF : 0.0f);
		outVect.w = (( inVector1.w < inVector2.w - eps.w || inVector1.w > inVector2.w + eps.w ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISNOTCLOSEV_V

#ifndef V4ISCLOSEALL
	__forceinline unsigned int V4IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return ( ( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) &&
				 ( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) &&
				 ( inVector1.z >= inVector2.z - eps && inVector1.z <= inVector2.z + eps ) &&
				 ( inVector1.w >= inVector2.w - eps && inVector1.w <= inVector2.w + eps ) ? 1u : 0u);
	}
#endif // V4ISCLOSEALL

#ifndef V4ISCLOSENONE
	__forceinline unsigned int V4IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return (( inVector1.x < inVector2.x - eps || inVector1.x > inVector2.x + eps ) &&
				( inVector1.y < inVector2.y - eps || inVector1.y > inVector2.y + eps ) &&
				( inVector1.z < inVector2.z - eps || inVector1.z > inVector2.z + eps ) &&
				( inVector1.w < inVector2.w - eps || inVector1.w > inVector2.w + eps ) ? 1u : 0u);
	}
#endif // V4ISCLOSENONE

#ifndef V4ISCLOSEALL_V
	__forceinline unsigned int V4IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) &&
				( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) &&
				( inVector1.z >= inVector2.z - eps.z && inVector1.z <= inVector2.z + eps.z ) &&
				( inVector1.w >= inVector2.w - eps.w && inVector1.w <= inVector2.w + eps.w ) ? 1u : 0u);
	}
#endif // V4ISCLOSEALL_V

#ifndef V4ISCLOSENONE_V
	__forceinline unsigned int V4IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x < inVector2.x - eps.x || inVector1.x > inVector2.x + eps.x ) &&
				( inVector1.y < inVector2.y - eps.y || inVector1.y > inVector2.y + eps.y ) &&
				( inVector1.z < inVector2.z - eps.z || inVector1.z > inVector2.z + eps.z ) &&
				( inVector1.w < inVector2.w - eps.w || inVector1.w > inVector2.w + eps.w ) ? 1u : 0u);
	}
#endif // V4ISCLOSENONE_V

#ifndef V4ISGREATERTHANALL
	__forceinline unsigned int V4IsGreaterThanAll(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) &&
				( bigVector.z > smallVector.z ) &&
				( bigVector.w > smallVector.w ) ? 1u : 0u );
	}
#endif // V4ISGREATERTHANALL

#ifndef V4ISGREATERTHANV
	__forceinline Vector_4_Out V4IsGreaterThanV(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		Vector_4 outVect;
		outVect.x = (bigVector.x > smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y > smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z > smallVector.z ? allBitsF : 0.0f);
		outVect.w = (bigVector.w > smallVector.w ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISGREATERTHANV

#ifndef V4ISGREATERTHANOREQUALALL
	__forceinline unsigned int V4IsGreaterThanOrEqualAll(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) &&
				( bigVector.z >= smallVector.z ) &&
				( bigVector.w >= smallVector.w ) ? 1u : 0u);
	}
#endif // V4ISGREATERTHANOREQUALALL

#ifndef V4ISGREATERTHANOREQUALV
	__forceinline Vector_4_Out V4IsGreaterThanOrEqualV(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		Vector_4 outVect;
		outVect.x = (bigVector.x >= smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y >= smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z >= smallVector.z ? allBitsF : 0.0f);
		outVect.w = (bigVector.w >= smallVector.w ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISGREATERTHANOREQUALV

#ifndef V4ISLESSTHANALL
	__forceinline unsigned int V4IsLessThanAll(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) &&
				( bigVector.z > smallVector.z ) &&
				( bigVector.w > smallVector.w ) ? 1u : 0u);
	}
#endif // V4ISLESSTHANALL

#ifndef V4ISLESSTHANV
	__forceinline Vector_4_Out V4IsLessThanV(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		Vector_4 outVect;
		outVect.x = (bigVector.x > smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y > smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z > smallVector.z ? allBitsF : 0.0f);
		outVect.w = (bigVector.w > smallVector.w ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISLESSTHANV

#ifndef V4ISLESSTHANOREQUALALL
	__forceinline unsigned int V4IsLessThanOrEqualAll(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) &&
				( bigVector.z >= smallVector.z ) &&
				( bigVector.w >= smallVector.w ) ? 1u : 0u);
	}
#endif // V4ISLESSTHANOREQUALALL

#ifndef V4ISLESSTHANOREQUALV
	__forceinline Vector_4_Out V4IsLessThanOrEqualV(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		Vector_4 outVect;
		outVect.x = (bigVector.x >= smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y >= smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z >= smallVector.z ? allBitsF : 0.0f);
		outVect.w = (bigVector.w >= smallVector.w ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V4ISLESSTHANOREQUALV

#ifndef V4SELECT
	__forceinline Vector_4_Out V4SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero)
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

		Temp1.f = choiceVector.y;
		Temp2.f = zero.y;
		Temp3.f = nonZero.y;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.y = TempResult.f;

		Temp1.f = choiceVector.z;
		Temp2.f = zero.z;
		Temp3.f = nonZero.z;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.z = TempResult.f;

		Temp1.f = choiceVector.w;
		Temp2.f = zero.w;
		Temp3.f = nonZero.w;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.w = TempResult.f;

		return outVect;
	}
#endif // V4SELECT

#ifndef V4SELECTVECT
	__forceinline Vector_4_Out V4SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero)
	{
		return (GetX(choiceVectorX) == 0.0f ? zero : nonZero );
	}
#endif // V4SELECTVECT

#ifndef V4MAX
	__forceinline Vector_4_Out V4Max(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMax(inVector1.x, inVector2.x);
		outVect.y = FPMax(inVector1.y, inVector2.y);
		outVect.z = FPMax(inVector1.z, inVector2.z);
		outVect.w = FPMax(inVector1.w, inVector2.w);
		return outVect;
	}
#endif // V4MAX

#ifndef V4MIN
	__forceinline Vector_4_Out V4Min(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMin(inVector1.x, inVector2.x);
		outVect.y = FPMin(inVector1.y, inVector2.y);
		outVect.z = FPMin(inVector1.z, inVector2.z);
		outVect.w = FPMin(inVector1.w, inVector2.w);
		return outVect;
	}
#endif // V4MIN

#ifndef V4MINELEMENT
	__forceinline Vector_4_Out V4MinElement(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPMin( FPMin( inVector.x, inVector.y ), FPMin( inVector.z, inVector.w ) );
		return outVect;
	}
#endif // V4MINELEMENT

#ifndef V4MAXELEMENT
	__forceinline Vector_4_Out V4MaxElement(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPMax( FPMax( inVector.x, inVector.y ), FPMax( inVector.z, inVector.w ) );
		return outVect;
	}
#endif // V4MAXELEMENT

#ifndef V4CLAMP
	__forceinline Vector_4_Out V4Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound )
	{
		Vector_4 outVect;
		outVect.x = FPClamp(inVector.x, lowBound.x, highBound.x);
		outVect.y = FPClamp(inVector.y, lowBound.y, highBound.y);
		outVect.z = FPClamp(inVector.z, lowBound.z, highBound.z);
		outVect.w = FPClamp(inVector.w, lowBound.w, highBound.w);
		return outVect;
	}
#endif // V4CLAMP

#ifndef V4SATURATE
	__forceinline Vector_4_Out V4Saturate( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPClamp(inVector.x, 0.0f, 1.0f);
		outVect.y = FPClamp(inVector.y, 0.0f, 1.0f);
		outVect.z = FPClamp(inVector.z, 0.0f, 1.0f);
		outVect.w = FPClamp(inVector.w, 0.0f, 1.0f);
		return outVect;
	}
#endif // V4SATURATE

	//============================================================================
	// Quaternions

#ifndef V4QUATDOTV
	__forceinline Vector_4_Out V4QuatDotV( Vector_4_In inQuat1, Vector_4_In inQuat2 )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V4QuatDot( inQuat1, inQuat2 );
		return outVect;
	}
#endif // V4QUATDOTV

#ifndef V4QUATDOT
	__forceinline float V4QuatDot( Vector_4_In inQuat1, Vector_4_In inQuat2 )
	{
		return V4Dot( inQuat1, inQuat2 );
	}
#endif // V4QUATDOT

#ifndef V4QUATMULTIPLY
	__forceinline Vector_4_Out V4QuatMultiply( Vector_4_In inQuat1, Vector_4_In inQuat2 )
	{
		Vector_4 outVect;
		outVect.x = inQuat1.w*inQuat2.x + inQuat2.w*inQuat1.x + inQuat1.y*inQuat2.z - inQuat1.z*inQuat2.y;
		outVect.y = inQuat1.w*inQuat2.y + inQuat2.w*inQuat1.y + inQuat1.z*inQuat2.x - inQuat1.x*inQuat2.z;
		outVect.z = inQuat1.w*inQuat2.z + inQuat2.w*inQuat1.z + inQuat1.x*inQuat2.y - inQuat1.y*inQuat2.x;
		outVect.w = inQuat1.w*inQuat2.w - inQuat1.x*inQuat2.x - inQuat1.y*inQuat2.y - inQuat1.z*inQuat2.z;
		return outVect;
	}
#endif // V4QUATMULTIPLY

#ifndef V4QUATCONJUGATE
	__forceinline Vector_4_Out V4QuatConjugate( Vector_4_In inQuat )
	{
		Vector_4 outVect;
		outVect.x = -inQuat.x;
		outVect.y = -inQuat.y;
		outVect.z = -inQuat.z;
		outVect.w = inQuat.w;
		return outVect;
	}
#endif // V4QUATCONJUGATE

#ifndef V4QUATNORMALIZE
	__forceinline Vector_4_Out V4QuatNormalize( Vector_4_In inQuat )
	{
		return V4Normalize( inQuat );
	}
#endif // V4QUATNORMALIZE

#ifndef V4QUATNORMALIZESAFE
	__forceinline Vector_4_Out V4QuatNormalizeSafe( Vector_4_In inQuat, float errVal, float magSqThreshold )
	{
		return V4NormalizeSafe( inQuat, errVal, magSqThreshold );
	}
#endif // V4QUATNORMALIZESAFE

#ifndef V4QUATNORMALIZEFAST
	__forceinline Vector_4_Out V4QuatNormalizeFast( Vector_4_In inQuat )
	{
		return V4NormalizeFast( inQuat );
	}
#endif // V4QUATNORMALIZEFAST

#ifndef V4QUATNORMALIZEFASTSAFE
	__forceinline Vector_4_Out V4QuatNormalizeFastSafe( Vector_4_In inQuat, float errVal, float magSqThreshold )
	{
		return V4NormalizeFastSafe( inQuat, errVal, magSqThreshold );
	}
#endif // V4QUATNORMALIZEFASTSAFE

#ifndef V4QUATINVERSE
	__forceinline Vector_4_Out V4QuatInvert( Vector_4_In inQuat )
	{
		Vector_4 numerator = V4QuatConjugate( inQuat );
		float denominator = V4QuatDot( inQuat, inQuat );
		return V4InvScale( numerator, denominator );
	}
#endif // V4QUATINVERSE

#ifndef V4QUATINVERSESAFE
	__forceinline Vector_4_Out V4QuatInvertSafe( Vector_4_In inQuat, float errVal )
	{
		Vector_4 numerator = V4QuatConjugate( inQuat );
		float denominator = V4QuatDot( inQuat, inQuat );
		return V4InvScaleSafe( numerator, denominator, errVal );
	}
#endif // V4QUATINVERSESAFE

#ifndef V4QUATINVERSEFAST
	__forceinline Vector_4_Out V4QuatInvertFast( Vector_4_In inQuat )
	{
		Vector_4 numerator = V4QuatConjugate( inQuat );
		float denominator = V4QuatDot( inQuat, inQuat );
		return V4InvScaleFast( numerator, denominator );
	}
#endif // V4QUATINVERSEFAST

#ifndef V4QUATINVERSEFASTSAFE
	__forceinline Vector_4_Out V4QuatInvertFastSafe( Vector_4_In inQuat, float errVal )
	{
		Vector_4 numerator = V4QuatConjugate( inQuat );
		float denominator = V4QuatDot( inQuat, inQuat );
		return V4InvScaleFastSafe( numerator, denominator, errVal );
	}
#endif // V4QUATINVERSEFASTSAFE

#ifndef V4QUATINVERSENORMINPUT
	__forceinline Vector_4_Out V4QuatInvertNormInput( Vector_4_In inNormQuat )
	{
		return V4QuatConjugate( inNormQuat );
	}
#endif // V4QUATINVERSENORMINPUT

#ifndef V4QUATSLERPNEAR
	__forceinline Vector_4_Out V4QuatSlerpNear( float t, Vector_4_In inNormQuat1, Vector_4_In inNormQuat2 )
	{
		//              inNormQuat1*sin((1-t)(theta)) + inNormQuat2*sin((t)(theta))
		// quatResult = -----------------------------------------------------------
		//                                       sin(theta)

		float theta;
		float sinTheta;
		float cosTheta = V4QuatDot( inNormQuat1, inNormQuat2 );

		// Save the original.
		float cosThetaOrig = cosTheta;

		//if( cosThetaOrig < 0.0f )
		//{
		//	// We could invert one of the quaternions here. But that will
		//	// just invert the dot product result. So let's just invert cos(theta).
		//	cosTheta = -cosTheta;
		//}

		// <= instead of < since it's a little faster where fsel is available.
		cosTheta = FPIfLteZeroThenElse( cosThetaOrig, -cosThetaOrig, cosThetaOrig );

		// Compute sin(theta) from cos(theta).
		sinTheta = FPSqrt( 1.0f-cosTheta );

		// Compute theta so that we can compute sin((1-t)(theta)) and sin((t)(theta)).
		// [ arctan( sin(theta)/cos(theta) ) = theta ]
		theta = FPATan2( sinTheta, cosTheta );

		float sinOneMinusTTheta = FPSin( (1.0f-t)*theta );
		float sinTTheta = FPSin( t*theta );

		// Invert this sign if need be. (TODO: Not sure if this is necessary?)
		sinTTheta = FPIfLteZeroThenElse( cosThetaOrig, -sinTTheta, sinTTheta );

		// Compute the fraction.
		Vector_4 numerator = V4Scale( inNormQuat1, sinOneMinusTTheta );
		numerator = V4Add( numerator, V4Scale( inNormQuat2, sinTTheta ) );
		return V4QuatNormalize( V4InvScale( numerator, sinTheta ) );
	}
#endif // V4QUATSLERPNEAR

#ifndef V4QUATSLERP
	__forceinline Vector_4_Out V4QuatSlerp( float t, Vector_4_In inNormQuat1, Vector_4_In inNormQuat2 )
	{
		//              inNormQuat1*sin((1-t)(theta)) + inNormQuat2*sin((t)(theta))
		// quatResult = -----------------------------------------------------------
		//                                       sin(theta)

		float theta;
		float sinTheta;
		float cosTheta = V4QuatDot( inNormQuat1, inNormQuat2 );

		// Save the original.
		//float cosThetaOrig = cosTheta;
		//
		//if( cosTheta < 0.0f )
		//{
		//	// We could invert one of the quaternions here. But that will
		//	// just invert the dot product result. So let's just invert cos(theta).
		//	cosTheta = -cosTheta;
		//}

		//// <= instead of < since it's a little faster where fsel is available.
		//cosTheta = FPIfLteZeroThenElse( cosThetaOrig, -cosThetaOrig, cosThetaOrig );

		// Compute sin(theta) from cos(theta).
		sinTheta = FPSqrt( 1.0f-cosTheta );

		// Compute theta so that we can compute sin((1-t)(theta)) and sin((t)(theta)).
		// [ arctan( sin(theta)/cos(theta) ) = theta ]
		theta = FPATan2( sinTheta, cosTheta );

		float sinOneMinusTTheta = FPSin( (1.0f-t)*theta );
		float sinTTheta = FPSin( t*theta );

		//// Invert this sign if need be. (TODO: Not sure if this is necessary?)
		//sinTTheta = FPIfLteZeroThenElse( cosThetaOrig, -sinTTheta, sinTTheta );

		// Compute the fraction.
		Vector_4 numerator = V4Scale( inNormQuat1, sinOneMinusTTheta );
		numerator = V4Add( numerator, V4Scale( inNormQuat2, sinTTheta ) );
		return V4QuatNormalize( V4InvScale( numerator, sinTheta ) );
	}
#endif // V4QUATSLERP

#ifndef V4QUATNLERP
	__forceinline Vector_4_Out V4QuatNlerp( float t, Vector_4_In inNormQuat1, Vector_4_In inNormQuat2 )
	{
		Vector_4 lerpedQuat = V4Lerp( t, inNormQuat1, inNormQuat2 );
		return V4QuatNormalize( lerpedQuat );
	}
#endif // V4QUATNLERP


#ifndef V4QUATTWISTANGLE
	__forceinline float V4QuatTwistAnglef( Vector_4_In inQuat, Vector_4_In v )
	{
		Vector_4 tmpV = V3QuatRotate( v, inQuat );

		Vector_4 tmpQ = V4QuatFromVectors( tmpV, v );
		tmpQ = V4QuatMultiply( tmpQ, inQuat );

		float dot = v.x * tmpQ.x + v.y * tmpQ.y + v.z * tmpQ.z;
		float angle = 2 * FPATan2(dot, tmpQ.w);

		float _PI = V4Constant(V_PI).x;
		if (angle > _PI)
		{
			angle -= 2.0f * _PI;
		}
		else if (angle < -_PI)
		{
			angle += 2.0f * _PI;
		}

		return angle;
	}
#endif // V4QUATTWISTANGLE


#ifndef V4QUATSCALEANGLE
	__forceinline Vector_4_Out V4QuatScaleAnglef( Vector_4_In inQuat, float scale )
	{
		Vector_4 outQuat = inQuat;
		Vector_4 axis;
		float angle;
		V4QuatToAxisAnglef(axis, angle, inQuat);
		if( (V4Dot( axis, axis ) > 0.999f) && (angle*angle > 0.f) )
		{
			outQuat = V4QuatFromAxisAngle(axis, angle * scale);
		}
		return outQuat;
	}
#endif // V4QUATSCALEANGLE

#ifndef V4QUATFROMAXISANGLE
	__forceinline Vector_4_Out V4QuatFromAxisAngle( Vector_4_In normAxis, float radians )
	{
		float halfAngle = 0.5f * radians;
		float theSin, theCos;
		FPSinAndCos(theSin, theCos, halfAngle);

		Vector_4 outVect;
		V4Set(	outVect,
				normAxis.x*theSin,
				normAxis.y*theSin,
				normAxis.z*theSin,
				theCos	);
		return outVect;
	}
#endif // V4QUATFROMAXISANGLE

#ifndef V4QUATFROMAXISANGLE__3
	__forceinline Vector_4_Out V4QuatFromAxisAngle( Vector_3_In normAxis, float radians )
	{
		float halfAngle = 0.5f * radians;
		float theSin, theCos;
		FPSinAndCos(theSin, theCos, halfAngle);

		Vector_4 outVect;
		V4Set(	outVect,
				normAxis.x*theSin,
				normAxis.y*theSin,
				normAxis.z*theSin,
				theCos	);
		return outVect;
	}
#endif // V4QUATFROMAXISANGLE__3

#ifndef V4QUATFROMXAXISANGLE
	__forceinline Vector_4_Out V4QuatFromXAxisAngle( float radians )
	{
		float halfAngle = 0.5f * radians;
		float theSin, theCos;
		FPSinAndCos(theSin, theCos, halfAngle);

		Vector_4 outVect;
		V4Set(	outVect,
				theSin,
				0.0f,
				0.0f,
				theCos	);
		return outVect;
	}
#endif // V4QUATFROMXAXISANGLE

#ifndef V4QUATFROMYAXISANGLE
	__forceinline Vector_4_Out V4QuatFromYAxisAngle( float radians )
	{
		float halfAngle = 0.5f * radians;
		float theSin, theCos;
		FPSinAndCos(theSin, theCos, halfAngle);

		Vector_4 outVect;
		V4Set(	outVect,
				0.0f,
				theSin,
				0.0f,
				theCos	);
		return outVect;
	}
#endif // V4QUATFROMYAXISANGLE

#ifndef V4QUATFROMZAXISANGLE
	__forceinline Vector_4_Out V4QuatFromZAxisAngle( float radians )
	{
		float halfAngle = 0.5f * radians;
		float theSin, theCos;
		FPSinAndCos(theSin, theCos, halfAngle);

		Vector_4 outVect;
		V4Set(	outVect,
				0.0f,
				0.0f,
				theSin,
				theCos	);
		return outVect;
	}
#endif // V4QUATFROMZAXISANGLE

#ifndef V4QUATTOAXISANGLEF
	__forceinline void V4QuatToAxisAnglef( Vector_3_InOut axis, float& radians, Vector_4_In inQuat )
	{
		mthAssertf( inQuat.w > -1.0f && inQuat.w < 1.0f , "These conditions are necessary in old vec lib's Quaternion::GetAngle()" );

		axis.x = inQuat.x;
		axis.y = inQuat.y;
		axis.z = inQuat.z;
		radians = 2.0f*FPACos( inQuat.w );
	}
#endif // V4QUATTOAXISANGLEF


#ifndef V4QUATTOAXISANGLEF_4
	__forceinline void V4QuatToAxisAnglef( Vector_4_InOut axis, float& radians, Vector_4_In inQuat )
	{
		AssertMsg( inQuat.w > -1.0f && inQuat.w < 1.0f , "These conditions are necessary in old vec lib's Quaternion::GetAngle()" );

		axis.x = inQuat.x;
		axis.y = inQuat.y;
		axis.z = inQuat.z;
		radians = 2.0f*FPACos( inQuat.w );
	}
#endif // V4QUATTOAXISANGLEF_4


#ifndef V4QUATGETANGLEF
	__forceinline float V4QuatGetAnglef( Vector_4_In inQuat )
	{
		mthAssertf( inQuat.w > -1.0f && inQuat.w < 1.0f , "These conditions are necessary in old vec lib's Quaternion::GetAngle()" );

		return 2.0f*FPACos( inQuat.w );
	}
#endif // V4QUATGETANGLEF

#ifndef V4QUATTOAXISANGLE
	__forceinline void V4QuatToAxisAngle( Vector_4_InOut axis, Vector_4_InOut radians, Vector_4_In inQuat )
	{
		mthAssertf( inQuat.w > -1.0f && inQuat.w < 1.0f , "These conditions are necessary in old vec lib's Quaternion::GetAngle()" );

		axis = inQuat;
		radians.x = radians.y = radians.z = radians.w = 2.0f*FPACos( inQuat.w );
	}
#endif // V4QUATTOAXISANGLE

#ifndef V4QUATGETANGLE
	__forceinline Vector_4_Out V4QuatGetAngle( Vector_4_In inQuat )
	{
		mthAssertf( inQuat.w > -1.0f && inQuat.w < 1.0f , "These conditions are necessary in old vec lib's Quaternion::GetAngle()" );

		Vector_4 out;
		out.x = out.y = out.z = out.w = 2.0f*FPACos( inQuat.w );
		return out;
	}
#endif // V4QUATGETANGLE


#ifndef V4QUATFROMVECTORS
	__forceinline Vector_4_Out V4QuatFromVectors( Vector_4_In from, Vector_4_In to, Vector_4_In axis )
	{		
		float scaleFrom	= V4Dot( from, axis );
		float scaleTo	= V4Dot( to, axis );
		Vector_4 fromN		= V4SubtractScaled( from, axis, scaleFrom ),
				 toN		= V4SubtractScaled( to, axis, scaleTo );

		fromN	= V4Normalize( fromN );
		toN		= V4Normalize( toN );

		Vector_4 out;
		float dot = V4Dot( fromN, toN );

		if (FPAbs(dot - 1.0f) < SMALL_FLOAT)
		{
			// Co-linear in same direction, no rotation.
			// set quaternion to identity
			out.x = out.y = out.z = 0.0f;
			out.w = 1.0f;
		}
		else if (FPAbs(dot + 1.0f) < SMALL_FLOAT)
		{
			// Co-linear in opposite direction,
			out = V4QuatFromAxisAngle( axis, 3.141592654f );
		}
		else
		{
			Vector_4 cross;
			cross.x = fromN.y * toN.z - fromN.z * toN.y;
			cross.y = fromN.z * toN.x - fromN.x * toN.z;
			cross.z = fromN.x * toN.y - fromN.y * toN.x;

			float __dot = V4Dot( cross, axis );
			Vector_4 __axis = __dot	>= 0.0f ? axis : V4Negate(axis) ;
			out = V4QuatFromAxisAngle( __axis, FPACos(dot) );
		}

		return out;
	}

	__forceinline Vector_4_Out V4QuatFromVectors( Vector_4_In from, Vector_4_In to )
	{		
		Vector_4 out;
		Vector_4 fromN	= V4Normalize( from ),
				 toN	= V4Normalize( to );

		float dot = V4Dot( fromN, toN );

		if (FPAbs(dot - 1.0f) < SMALL_FLOAT)
		{
			// Co-linear in same direction, no rotation.
			// set quaternion to identity
			out.x = out.y = out.z = 0.0f;
			out.w = 1.0f;
			return out;
		}
		
		if (FPAbs(dot + 1.0f) < SMALL_FLOAT)
		{
			// Co-linear in opposite direction, find a good axis of rotation.
			int maxIndex = (from.x >= from.y) ? ((from.x >= from.z) ? 0 : 2) : ((from.y >= from.z) ? 1 : 2);
			switch(maxIndex)
			{
			case 0:
				toN.x = toN.y = 0.0f;
				toN.z = 1.0f;
				break;
			case 1:
				toN.x = toN.y = 0.0f;
				toN.z = 1.0f;
				break;
			case 2:
				toN.x = toN.z = 0.0f;
				toN.y = 1.0f;
				break;
			default:
				Assert(0);	//lint !e506 constant value boolean
			}
		}

		out.x = fromN.y * toN.z - fromN.z * toN.y;
		out.y = fromN.z * toN.x - fromN.x * toN.z;
		out.z = fromN.x * toN.y - fromN.y * toN.x;
		out.w = dot + 1.0f;

		return V4Normalize( out );
	}

#endif // V4QUATFROMVECTORS


#ifndef V4QUATRELANGLE
	__forceinline Vector_4_Out V4QuatRelAngle( Vector_4_In inQuat1, Vector_4_In inQuat2 )
	{
		Vector_4 out;
		Vector_4 c = V4DotV( inQuat1, inQuat2 );
		if(c.x <= -1.0f || c.x >= 1.0f )
		{
			out.x = out.y = out.z = out.w = 0.0f;
		}
		else
		{
			out.x = out.y = out.z = out.w = 2.0f*FPACos( FPAbs( c.x ) );
		}
		return out;
	}
#endif // V4QUATRELANGLE


#ifndef V4QUATGETUNITDIRECTION
	__forceinline Vector_4_Out V4QuatGetUnitDirection( Vector_4_In inQuat )
	{
		Vector_4 outVect = inQuat;
		return V3Normalize( outVect );
	}
#endif // V4QUATGETUNITDIRECTION

#ifndef V4QUATGETUNITDIRECTIONFAST
	__forceinline Vector_4_Out V4QuatGetUnitDirectionFast( Vector_4_In inQuat )
	{
		Vector_4 outVect = inQuat;
		return V3NormalizeFast( outVect );
	}
#endif // V4QUATGETUNITDIRECTIONFAST

#ifndef V4QUATGETUNITDIRECTIONSAFE
	__forceinline Vector_4_Out V4QuatGetUnitDirectionSafe( Vector_4_In inQuat, Vector_4_In errValVect )
	{
		Vector_4 outVect = inQuat;
		float magSq = V3MagSquared( outVect );
		if ( magSq > VERY_SMALL_FLOAT )
		{
			outVect = V3Scale( outVect, FPInvSqrt(magSq) );
		}
		else
		{
			outVect = errValVect;
		}
		return outVect;
	}
#endif // V4QUATGETUNITDIRECTIONSAFE

#ifndef V4QUATGETUNITDIRECTIONFASTSAFE
	__forceinline Vector_4_Out V4QuatGetUnitDirectionFastSafe( Vector_4_In inQuat, Vector_4_In errValVect )
	{
		Vector_4 outVect = inQuat;
		float magSq = V3MagSquared( outVect );
		if ( magSq > VERY_SMALL_FLOAT )
		{
			outVect = V3Scale( outVect, FPInvSqrtFast(magSq) );
		}
		else
		{
			outVect = errValVect;
		}
		return outVect;
	}
#endif // V4QUATGETUNITDIRECTIONFASTSAFE

#ifndef V4QUATPREPARESLERP
	__forceinline Vector_4_Out V4QuatPrepareSlerp( Vector_4_In quat1, Vector_4_In quatToNegate )
	{
		Vector_4 outVect;
		float dot = V3Dot( quat1, quatToNegate );
		if ( dot < 0.0f )
		{
			outVect = V4Negate( quatToNegate );
		}
		else
		{
			outVect = quatToNegate;
		}
		return outVect;
	}
#endif // V4QUATPREPARESLERP

	//============================================================================
	// Bitwise operations

#ifndef V4SHIFTLEFT
	template <int shift>
	__forceinline Vector_4_Out V4ShiftLeft( Vector_4_In inVector )
	{
		Vector_4 outVect;

		union
		{
			float f;
			u32 i;
		} TempResult1, TempResult2, TempResult3, TempResult4;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult3.f = inVector.z;
		TempResult4.f = inVector.w;
		TempResult1.i <<= shift;
		TempResult2.i <<= shift;
		TempResult3.i <<= shift;
		TempResult4.i <<= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4SHIFTLEFT

#ifndef V4SHIFTRIGHT
	template <int shift>
	__forceinline Vector_4_Out V4ShiftRight( Vector_4_In inVector )
	{
		Vector_4 outVect;

		union
		{
			float f;
			u32 i;
		} TempResult1, TempResult2, TempResult3, TempResult4;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult3.f = inVector.z;
		TempResult4.f = inVector.w;
		TempResult1.i >>= shift;
		TempResult2.i >>= shift;
		TempResult3.i >>= shift;
		TempResult4.i >>= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4SHIFTRIGHT

#ifndef V4SHIFTRIGHTALGEBRAIC
	template <int shift>
	__forceinline Vector_4_Out V4ShiftRightAlgebraic( Vector_4_In inVector )
	{
		Vector_4 outVect;

		union
		{
			float f;
			s32 i;
		} TempResult1, TempResult2, TempResult3, TempResult4;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult3.f = inVector.z;
		TempResult4.f = inVector.w;
		TempResult1.i >>= shift;
		TempResult2.i >>= shift;
		TempResult3.i >>= shift;
		TempResult4.i >>= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4SHIFTRIGHTALGEBRAIC

#ifndef V4AND
	__forceinline Vector_4_Out V4And(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, TempResult4, Temp1, Temp2, Temp3, Temp4;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		TempResult4.f = inVector1.w;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		Temp4.f = inVector2.w;
		TempResult1.i &= Temp1.i;
		TempResult2.i &= Temp2.i;
		TempResult3.i &= Temp3.i;
		TempResult4.i &= Temp4.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4AND

#ifndef V4OR
	__forceinline Vector_4_Out V4Or(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, TempResult4, Temp1, Temp2, Temp3, Temp4;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		TempResult4.f = inVector1.w;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		Temp4.f = inVector2.w;
		TempResult1.i |= Temp1.i;
		TempResult2.i |= Temp2.i;
		TempResult3.i |= Temp3.i;
		TempResult4.i |= Temp4.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4OR

#ifndef V4XOR
	__forceinline Vector_4_Out V4Xor(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, TempResult4, Temp1, Temp2, Temp3, Temp4;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		TempResult4.f = inVector1.w;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		Temp4.f = inVector2.w;
		TempResult1.i ^= Temp1.i;
		TempResult2.i ^= Temp2.i;
		TempResult3.i ^= Temp3.i;
		TempResult4.i ^= Temp4.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4XOR

#ifndef V4ANDC
	__forceinline Vector_4_Out V4Andc(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, TempResult4, Temp1, Temp2, Temp3, Temp4;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		TempResult4.f = inVector1.w;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		Temp4.f = inVector2.w;
		TempResult1.i &= ~Temp1.i;
		TempResult2.i &= ~Temp2.i;
		TempResult3.i &= ~Temp3.i;
		TempResult4.i &= ~Temp4.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		outVect.w = TempResult4.f;
		return outVect;
	}
#endif // V4ANDC

#ifndef V4MERGEXY
	__forceinline Vector_4_Out V4MergeXY(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = inVector1.x;
		outVect.y = inVector2.x;
		outVect.z = inVector1.y;
		outVect.w = inVector2.y;
		return outVect;
	}
#endif // V4MERGEXY

#ifndef V4MERGEZW
	__forceinline Vector_4_Out V4MergeZW(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = inVector1.z;
		outVect.y = inVector2.z;
		outVect.z = inVector1.w;
		outVect.w = inVector2.w;
		return outVect;
	}
#endif // V4MERGEZW

#ifndef V4PACK1010102
	__forceinline void V4Pack1010102( u32& inoutInteger, Vector_4_In inVector )
	{
		u32 wPack = inVector.w > 0.5f ? 1 << 30 : inVector.w < -0.5f ? -1 << 30 : 0;
		inoutInteger = FPPack(inVector.x,10,0) | FPPack(inVector.y,10,10) | FPPack(inVector.z,10,20) | wPack;
	}
#endif // V4PACK1010102

#ifndef V4UNPACK1010102
	__forceinline void V4Unpack1010102( Vector_4_InOut inoutVector, u32 packed )
	{
		int ux = (packed & 0x3FF) << 22;
		int uy = ((packed >> 10) & 0x3FF) << 22;
		int uz = ((packed >> 20) & 0x3FF) << 22;
		int uw = (int)packed;
		float fx = (float)(ux >> 22);
		float fy = (float)(uy >> 22);
		float fz = (float)(uz >> 22);
		float fw = (float)(uw >> 30);

		const float scale = 0.0019569471624266144814090019569472f;	 // 1/511
		V4Set( inoutVector, fx * scale, fy * scale, fz * scale, fw );
	}
#endif // V4UNPACK1010102

#ifndef V4PERMUTE
	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline Vector_4_Out V4Permute( Vector_4_In v )
	{
		CompileTimeAssert(	(permX==X || permX==Y || permX==Z || permX==W) &&
							(permY==X || permY==Y || permY==Z || permY==W) &&
							(permZ==X || permZ==Y || permZ==Z || permZ==W) &&
							(permW==X || permW==Y || permW==Z || permW==W)  ); // Invalid permute args!

		CompileTimeAssert( !(permX==X && permY==Y && permZ==Z && permW==W) ); // This permute does nothing meaningful!

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )

		float* pFloat = (float*)&v;
		Vector_4 outVect = VEC4_LITERAL( pFloat[MASK_IT(permX)], pFloat[MASK_IT(permY)], pFloat[MASK_IT(permZ)], pFloat[MASK_IT(permW)] );
		return outVect;

#undef MASK_IT

	}
#endif // V4PERMUTE

#ifndef V4PERMUTETWO
	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline Vector_4_Out V4PermuteTwo( Vector_4_In v1, Vector_4_In v2 )
	{
		CompileTimeAssert(	(permX==X1 || permX==Y1 || permX==Z1 || permX==W1	||
							permX==X2 || permX==Y2 || permX==Z2 || permX==W2)	&&
							(permY==X1 || permY==Y1 || permY==Z1 || permY==W1	||
							permY==X2 || permY==Y2 || permY==Z2 || permY==W2)	&&
							(permZ==X1 || permZ==Y1 || permZ==Z1 || permZ==W1	||
							permZ==X2 || permZ==Y2 || permZ==Z2 || permZ==W2)	&&
							(permW==X1 || permW==Y1 || permW==Z1 || permW==W1	||
							permW==X2 || permW==Y2 || permW==Z2 || permW==W2)	 );
							// Invalid permute args!

		CompileTimeAssert(	!(permX==X1 && permY==Y1 && permZ==Z1 && permW==W1) &&
							!(permX==X2 && permY==Y2 && permZ==Z2 && permW==W2) );
							// This permute does nothing meaningful!

		CompileTimeAssert(	!(((permX)&(0xF0F0F0F0)) != 0 && ((permY)&(0xF0F0F0F0)) != 0 && ((permZ)&(0xF0F0F0F0)) != 0 && ((permW)&(0xF0F0F0F0)) != 0)	&&
							!(((permX)&(0xF0F0F0F0)) == 0 && ((permY)&(0xF0F0F0F0)) == 0 && ((permZ)&(0xF0F0F0F0)) == 0 && ((permW)&(0xF0F0F0F0)) == 0) );
							// You should be using V4Permute<>()! It's faster on some platforms!

		float x, y, z, w;
		float* ptrV1 = (float*)&v1;
		float* ptrV2 = (float*)&v2;

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )

		switch( permX )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			x = ptrV1[MASK_IT(permX)];
			break;
		default:
			x = ptrV2[MASK_IT(permX)];
			break;
		};
		switch( permY )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			y = ptrV1[MASK_IT(permY)];
			break;
		default:
			y = ptrV2[MASK_IT(permY)];
			break;
		};
		switch( permZ )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			z = ptrV1[MASK_IT(permZ)];
			break;
		default:
			z = ptrV2[MASK_IT(permZ)];
			break;
		};
		switch( permW )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			w = ptrV1[MASK_IT(permW)];
			break;
		default:
			w = ptrV2[MASK_IT(permW)];
			break;
		};

#undef MASK_IT

		Vector_4_Out outVect = VEC4_LITERAL(x, y, z, w);
		return outVect;
	}
#endif // V4PERMUTETWO


#ifndef V4PERMUTE__CONTROL
	__forceinline Vector_4_Out V4Permute( Vector_4_In v, Vector_4_In controlVect )
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4;
		Temp1.f = controlVect.x;
		Temp2.f = controlVect.y;
		Temp3.f = controlVect.z;
		Temp4.f = controlVect.w;

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )

		float* pFloat = (float*)&v;
		Vector_4 outVect = VEC4_LITERAL( pFloat[MASK_IT(Temp1.i)], pFloat[MASK_IT(Temp2.i)], pFloat[MASK_IT(Temp3.i)], pFloat[MASK_IT(Temp4.i)] );
		return outVect;

#undef MASK_IT
	}
#endif // V4PERMUTE__CONTROL

#ifndef V4PERMUTETWO__CONTROL
	__forceinline Vector_4_Out V4PermuteTwo( Vector_4_In v1, Vector_4_In v2, Vector_4_In controlVect )
	{
		float x, y, z, w;
		float* ptrV1 = (float*)&v1;
		float* ptrV2 = (float*)&v2;

		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4;
		Temp1.f = controlVect.x;
		Temp2.f = controlVect.y;
		Temp3.f = controlVect.z;
		Temp4.f = controlVect.w;

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )

		switch( Temp1.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			x = ptrV1[MASK_IT(Temp1.i)];
			break;
		default:
			x = ptrV2[MASK_IT(Temp1.i)];
			break;
		};
		switch( Temp2.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			y = ptrV1[MASK_IT(Temp2.i)];
			break;
		default:
			y = ptrV2[MASK_IT(Temp2.i)];
			break;
		};
		switch( Temp3.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			z = ptrV1[MASK_IT(Temp3.i)];
			break;
		default:
			z = ptrV2[MASK_IT(Temp3.i)];
			break;
		};
		switch( Temp4.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			w = ptrV1[MASK_IT(Temp4.i)];
			break;
		default:
			w = ptrV2[MASK_IT(Temp4.i)];
			break;
		};

#undef MASK_IT

		Vector_4_Out outVect = VEC4_LITERAL(x, y, z, w);
		return outVect;
	}
#endif // V4PERMUTETWO__CONTROL

#ifndef V4BYTEPERMUTE
	template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
	__forceinline Vector_4_Out V4BytePermute( Vector_4_In v )
	{
		CompileTimeAssert(	(perm0 >= 0 && perm0 <= 15) && (perm1 >= 0 && perm1 <= 15) &&
							(perm2 >= 0 && perm2 <= 15) && (perm3 >= 0 && perm3 <= 15) &&
							(perm4 >= 0 && perm4 <= 15) && (perm5 >= 0 && perm5 <= 15) &&
							(perm6 >= 0 && perm6 <= 15) && (perm7 >= 0 && perm7 <= 15) &&
							(perm8 >= 0 && perm8 <= 15) && (perm9 >= 0 && perm9 <= 15) &&
							(perm10 >= 0 && perm10 <= 15) && (perm11 >= 0 && perm11 <= 15) &&
							(perm12 >= 0 && perm12 <= 15) && (perm13 >= 0 && perm13 <= 15) &&
							(perm14 >= 0 && perm14 <= 15) && (perm15 >= 0 && perm15 <= 15)	);
							// Invalid permute args!

		CompileTimeAssert(	!(	perm0==0 && perm1==1 && perm2==2 && perm3==3 && perm4==4 && perm5==5 && perm6==6 && perm7==7 &&
								perm8==8 && perm9==9 && perm10==10 && perm11==11 && perm12==12 && perm13==13 && perm14==14 && perm15==16) );
							// This permute does nothing meaningful!

		Vector_4 out;
		u8* srcptr = (u8*)(&v);
		u8* destptr = (u8*)(&out);
		destptr[0] = srcptr[perm0];
		destptr[1] = srcptr[perm1];
		destptr[2] = srcptr[perm2];
		destptr[3] = srcptr[perm3];
		destptr[4] = srcptr[perm4];
		destptr[5] = srcptr[perm5];
		destptr[6] = srcptr[perm6];
		destptr[7] = srcptr[perm7];
		destptr[8] = srcptr[perm8];
		destptr[9] = srcptr[perm9];
		destptr[10] = srcptr[perm10];
		destptr[11] = srcptr[perm11];
		destptr[12] = srcptr[perm12];
		destptr[13] = srcptr[perm13];
		destptr[14] = srcptr[perm14];
		destptr[15] = srcptr[perm15];
		return out;
	}
#endif // V4BYTEPERMUTE

#ifndef V4BYTEPERMUTETWO
	template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
	__forceinline Vector_4_Out V4BytePermuteTwo( Vector_4_In v1, Vector_4_In v2 )
	{
		CompileTimeAssert(	(perm0 >= 0 && perm0 <= 31) && (perm1 >= 0 && perm1 <= 31) &&
							(perm2 >= 0 && perm2 <= 31) && (perm3 >= 0 && perm3 <= 31) &&
							(perm4 >= 0 && perm4 <= 31) && (perm5 >= 0 && perm5 <= 31) &&
							(perm6 >= 0 && perm6 <= 31) && (perm7 >= 0 && perm7 <= 31) &&
							(perm8 >= 0 && perm8 <= 31) && (perm9 >= 0 && perm9 <= 31) &&
							(perm10 >= 0 && perm10 <= 31) && (perm11 >= 0 && perm11 <= 31) &&
							(perm12 >= 0 && perm12 <= 31) && (perm13 >= 0 && perm13 <= 31) &&
							(perm14 >= 0 && perm14 <= 31) && (perm15 >= 0 && perm15 <= 31)	);
							// Invalid permute args!

		CompileTimeAssert(	!(	perm0==0 && perm1==1 && perm2==2 && perm3==3 && perm4==4 && perm5==5 && perm6==6 && perm7==7 &&
								perm8==8 && perm9==9 && perm10==10 && perm11==11 && perm12==12 && perm13==13 && perm14==14 && perm15==16) &&
							!(	perm0==15 && perm1==16 && perm2==17 && perm3==18 && perm4==19 && perm5==20 && perm6==21 && perm7==22 &&
								perm8==23 && perm9==24 && perm10==25 && perm11==26 && perm12==27 && perm13==28 && perm14==29 && perm15==30) );
							// This permute does nothing meaningful!

		Vector_4 out;
		u8* srcptr1 = (u8*)(&v1);
		u8* srcptr2 = (u8*)(&v2);
		u8* destptr = (u8*)(&out);
		destptr[0] = (perm0 <= 15) ? srcptr1[perm0] : srcptr2[perm0-16];
		destptr[1] = (perm1 <= 15) ? srcptr1[perm1] : srcptr2[perm1-16];
		destptr[2] = (perm2 <= 15) ? srcptr1[perm2] : srcptr2[perm2-16];
		destptr[3] = (perm3 <= 15) ? srcptr1[perm3] : srcptr2[perm3-16];
		destptr[4] = (perm4 <= 15) ? srcptr1[perm4] : srcptr2[perm4-16];
		destptr[5] = (perm5 <= 15) ? srcptr1[perm5] : srcptr2[perm5-16];
		destptr[6] = (perm6 <= 15) ? srcptr1[perm6] : srcptr2[perm6-16];
		destptr[7] = (perm7 <= 15) ? srcptr1[perm7] : srcptr2[perm7-16];
		destptr[8] = (perm8 <= 15) ? srcptr1[perm8] : srcptr2[perm8-16];
		destptr[9] = (perm9 <= 15) ? srcptr1[perm9] : srcptr2[perm9-16];
		destptr[10] = (perm10 <= 15) ? srcptr1[perm10] : srcptr2[perm10-16];
		destptr[11] = (perm11 <= 15) ? srcptr1[perm11] : srcptr2[perm11-16];
		destptr[12] = (perm12 <= 15) ? srcptr1[perm12] : srcptr2[perm12-16];
		destptr[13] = (perm13 <= 15) ? srcptr1[perm13] : srcptr2[perm13-16];
		destptr[14] = (perm14 <= 15) ? srcptr1[perm14] : srcptr2[perm14-16];
		destptr[15] = (perm15 <= 15) ? srcptr1[perm15] : srcptr2[perm15-16];
		return out;
	}
#endif // V4BYTEPERMUTETWO
	
} // namespace Vec
} // namespace rage

