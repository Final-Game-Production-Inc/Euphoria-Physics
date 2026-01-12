#ifndef VECTORMATH_VECTORUTILITY_H
#define VECTORMATH_VECTORUTILITY_H



//================================================
// Set of macros for taking advantage of SelectFT()
// branching. Keep in mind that these are
// component-wise assignments. Also keep in mind
// that these are only advantageous on
// architectures that support operations like
// "__vsel()".
//================================================

// (a == b ? c : d)
// NOTE: -0.0f != 0.0f here!
#define IF_EQ_INT_THEN_ELSE(a, b, c, d)		( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsEqualIntV( (a), (b) ), (d), (c) ) )

// (a != b ? c : d)
// NOTE: -0.0f != 0.0f here!
#define IF_NEQ_INT_THEN_ELSE(a, b, c, d)	( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsEqualIntV( (a), (b) ), (c), (d) ) )

// (a == b ? c : d)
// NOTE: This is a floating point comparison, and WON'T WORK for two matching masks or NaNs
#define IF_EQ_THEN_ELSE(a, b, c, d)			( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsEqualV( (a), (b) ), (d), (c) ) )

// (a != b ? c : d)
// NOTE: This is a floating point comparison, and WON'T WORK for two matching masks or NaNs
#define IF_NEQ_THEN_ELSE(a, b, c, d)		( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsEqualV( (a), (b) ), (c), (d) ) )

// (a < b ? c : d)
// NOTE: This is a floating point comparison, and WON'T WORK for two matching masks or NaNs
#define IF_LT_THEN_ELSE(a, b, c, d)			( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsLessThanV( (a), (b) ), (d), (c) ) )

// (a > b ? c : d)
// NOTE: This is a floating point comparison, and WON'T WORK for two matching masks or NaNs
#define IF_GT_THEN_ELSE(a, b, c, d)			( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsGreaterThanV( (a), (b) ), (d), (c) ) )

// (a <= b ? c : d)
// NOTE: This is a floating point comparison, and WON'T WORK for two matching masks or NaNs
#define IF_LTE_THEN_ELSE(a, b, c, d)		( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsLessThanOrEqualV( (a), (b) ), (d), (c) ) )

// (a >= b ? c : d)
// NOTE: This is a floating point comparison, and WON'T WORK for two matching masks or NaNs
#define IF_GTE_THEN_ELSE(a, b, c, d)		( ::rage::Vec::V4SelectFT( ::rage::Vec::V4IsGreaterThanOrEqualV( (a), (b) ), (d), (c) ) )

// (a.w == 0.0f)
#define W_IS_ZERO(a)						( ::rage::Vec::GetW(a) == 0.0f )

//================================================
// Macro for generating the immediate values
// needed for swizzling.
//================================================

// Converts from x=0,1,2,3 to the immed value required.
#define SWIZZLE_VAL_WIN32( x, y, z, w )		((((x) & 3)) | (((y) & 3) << 2) | (((z) & 3) << 4) | (((w) & 3) << 6))

// Converts from x=0,1,2,3 to the immed value required.
#define SWIZZLE_VAL_XBOX360( x, y, z, w )	((((x) & 3) << 6) | (((y) & 3) << 4) | (((z) & 3) << 2) | ((w) & 3))


//================================================
// Other functions.
//================================================

namespace rage
{
	float GetAllF();
	float GetNaN();
	float GetInf();
	float GetNegInf();
	float Get0x7FFFFFFF();
	float Get0x80000000();
	float GetAsFloat(unsigned int num);
	unsigned int GetAsUint(float num);

#if __WIN32PC
	//
	// Processor vendor.
	//
	enum eProcVendor { VENDOR_INTEL=0, VENDOR_AMD, VENDOR_OTHER };
	eProcVendor GetProcVendor();

	//
	// Cache line size in bytes. (Data cache)
	//
	u32 GetDataCacheLineSize();
#endif

namespace Vec
{
	// Disable denormals, which can hurt performance in rare cases.
	void DisableDenormals();

	// Make sure denormals are still disabled.
	void AssertDenormalsDisabled();

#if __WIN32PC
	//
	// SSE.
	//
	enum eSSESupport { VEC_SCALAR=0, VEC_MMX, VEC_SSE, VEC_SSE2, VEC_SSE3, VEC_SSSE3, VEC_SSE4_1, VEC_SSE4_2 };
	eSSESupport GetSSESupportLevel();
	void PrintSSESupportLevelInfo(eSSESupport supp);
#endif
}

} // namespace rage


#endif // VECTORMATH_VECTORUTILITY_H
