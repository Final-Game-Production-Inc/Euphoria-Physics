
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
#define DEF_ScalarVConstantInitializer(constant) \
	__forceinline ScalarV::ScalarV(e##constant##Initializer) \
	{ \
		Vec::V4Set( v, Vec::V4VConstant(V_##constant) ); \
	}

	// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !

	DEF_ScalarVConstantInitializer(ZERO                 )
	DEF_ScalarVConstantInitializer(ONE                  )
	DEF_ScalarVConstantInitializer(TWO                  )
	DEF_ScalarVConstantInitializer(THREE                )
	DEF_ScalarVConstantInitializer(FOUR                 )
	DEF_ScalarVConstantInitializer(FIVE                 )
	DEF_ScalarVConstantInitializer(SIX                  )
	DEF_ScalarVConstantInitializer(SEVEN                )
	DEF_ScalarVConstantInitializer(EIGHT                )
	DEF_ScalarVConstantInitializer(NINE                 )
	DEF_ScalarVConstantInitializer(TEN                  )
	DEF_ScalarVConstantInitializer(ELEVEN               )
	DEF_ScalarVConstantInitializer(TWELVE               )
	DEF_ScalarVConstantInitializer(THIRTEEN             )
	DEF_ScalarVConstantInitializer(FOURTEEN             )
	DEF_ScalarVConstantInitializer(FIFTEEN              )
	DEF_ScalarVConstantInitializer(NEGONE               )
	DEF_ScalarVConstantInitializer(NEGTWO               )
	DEF_ScalarVConstantInitializer(NEGTHREE             )
	DEF_ScalarVConstantInitializer(NEGFOUR              )
	DEF_ScalarVConstantInitializer(NEGFIVE              )
	DEF_ScalarVConstantInitializer(NEGSIX               )
	DEF_ScalarVConstantInitializer(NEGSEVEN             )
	DEF_ScalarVConstantInitializer(NEGEIGHT             )
	DEF_ScalarVConstantInitializer(NEGNINE              )
	DEF_ScalarVConstantInitializer(NEGTEN               )
	DEF_ScalarVConstantInitializer(NEGELEVEN            )
	DEF_ScalarVConstantInitializer(NEGTWELVE            )
	DEF_ScalarVConstantInitializer(NEGTHIRTEEN          )
	DEF_ScalarVConstantInitializer(NEGFOURTEEN          )
	DEF_ScalarVConstantInitializer(NEGFIFTEEN           )
	DEF_ScalarVConstantInitializer(NEGSIXTEEN           )

	DEF_ScalarVConstantInitializer(NEG_FLT_MAX          )
	DEF_ScalarVConstantInitializer(FLT_MAX              )
	DEF_ScalarVConstantInitializer(FLT_MIN              )
	DEF_ScalarVConstantInitializer(FLT_LARGE_2          )
	DEF_ScalarVConstantInitializer(FLT_LARGE_4          )
	DEF_ScalarVConstantInitializer(FLT_LARGE_6          )
	DEF_ScalarVConstantInitializer(FLT_LARGE_8          )
	DEF_ScalarVConstantInitializer(FLT_EPSILON          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_6          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_5          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_4          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_3          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_2          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_1          )
	DEF_ScalarVConstantInitializer(FLT_SMALL_12         )
	DEF_ScalarVConstantInitializer(ONE_PLUS_EPSILON     )
	DEF_ScalarVConstantInitializer(ONE_MINUS_FLT_SMALL_3)

	//DEF_ScalarVConstantInitializer(ZERO_WONE            )
	//DEF_ScalarVConstantInitializer(ONE_WZERO            )

	//DEF_ScalarVConstantInitializer(X_AXIS_WONE          )
	//DEF_ScalarVConstantInitializer(Y_AXIS_WONE          )
	//DEF_ScalarVConstantInitializer(Z_AXIS_WONE          )

	//DEF_ScalarVConstantInitializer(X_AXIS_WZERO         )
	//DEF_ScalarVConstantInitializer(Y_AXIS_WZERO         )
	//DEF_ScalarVConstantInitializer(Z_AXIS_WZERO         )

	//DEF_ScalarVConstantInitializer(MASKX                )
	//DEF_ScalarVConstantInitializer(MASKY                )
	//DEF_ScalarVConstantInitializer(MASKZ                )
	//DEF_ScalarVConstantInitializer(MASKW                )
	//DEF_ScalarVConstantInitializer(MASKXY               )
	//DEF_ScalarVConstantInitializer(MASKXZ               )
	//DEF_ScalarVConstantInitializer(MASKXW               )
	//DEF_ScalarVConstantInitializer(MASKYZ               )
	//DEF_ScalarVConstantInitializer(MASKYW               )
	//DEF_ScalarVConstantInitializer(MASKZW               )
	//DEF_ScalarVConstantInitializer(MASKYZW              )
	//DEF_ScalarVConstantInitializer(MASKXZW              )
	//DEF_ScalarVConstantInitializer(MASKXYW              )
	//DEF_ScalarVConstantInitializer(MASKXYZ              )
	//DEF_ScalarVConstantInitializer(MASKXYZW             )

	DEF_ScalarVConstantInitializer(QUARTER              )
	DEF_ScalarVConstantInitializer(THIRD                )
	DEF_ScalarVConstantInitializer(HALF                 )
	DEF_ScalarVConstantInitializer(NEGHALF              )
	DEF_ScalarVConstantInitializer(INF                  )
	DEF_ScalarVConstantInitializer(NEGINF               )
	DEF_ScalarVConstantInitializer(NAN                  )
	DEF_ScalarVConstantInitializer(LOG2_TO_LOG10        )

	DEF_ScalarVConstantInitializer(ONE_OVER_1024        )
	DEF_ScalarVConstantInitializer(ONE_OVER_PI          )
	DEF_ScalarVConstantInitializer(TWO_OVER_PI          )
	DEF_ScalarVConstantInitializer(PI                   )
	DEF_ScalarVConstantInitializer(TWO_PI               )
	DEF_ScalarVConstantInitializer(PI_OVER_TWO          )
	DEF_ScalarVConstantInitializer(NEG_PI               )
	DEF_ScalarVConstantInitializer(NEG_PI_OVER_TWO      )
	DEF_ScalarVConstantInitializer(TO_DEGREES           )
	DEF_ScalarVConstantInitializer(TO_RADIANS           )
	DEF_ScalarVConstantInitializer(SQRT_TWO				)
	DEF_ScalarVConstantInitializer(ONE_OVER_SQRT_TWO	)
	DEF_ScalarVConstantInitializer(SQRT_THREE		    )
	DEF_ScalarVConstantInitializer(E					)

	DEF_ScalarVConstantInitializer(INT_1                )
	DEF_ScalarVConstantInitializer(INT_2                )
	DEF_ScalarVConstantInitializer(INT_3                )
	DEF_ScalarVConstantInitializer(INT_4                )
	DEF_ScalarVConstantInitializer(INT_5                )
	DEF_ScalarVConstantInitializer(INT_6                )
	DEF_ScalarVConstantInitializer(INT_7                )
	DEF_ScalarVConstantInitializer(INT_8                )
	DEF_ScalarVConstantInitializer(INT_9                )
	DEF_ScalarVConstantInitializer(INT_10               )
	DEF_ScalarVConstantInitializer(INT_11               )
	DEF_ScalarVConstantInitializer(INT_12               )
	DEF_ScalarVConstantInitializer(INT_13               )
	DEF_ScalarVConstantInitializer(INT_14               )
	DEF_ScalarVConstantInitializer(INT_15               )

	DEF_ScalarVConstantInitializer(7FFFFFFF             )
	DEF_ScalarVConstantInitializer(80000000             )

#undef DEF_ScalarVConstantInitializer

	__forceinline ScalarV::ScalarV()
	{
	}

	__forceinline ScalarV::ScalarV(const float& s1)
	{
		Vec::V4Set( v, s1 );
	}

#if __WIN32PC
	__forceinline ScalarV::ScalarV(ScalarV_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline ScalarV_ConstRef ScalarV::operator=(ScalarV_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline ScalarV::ScalarV(Vec::Vector_4V_In a)
		: v( a )
	{
#if !__OPTIMIZED // Only Debug gets this mthAssert. To speed up Beta builds.
		mthAssertf( IsValid(), "Scalar is invalid" );
#endif
	}

	__forceinline void ScalarV::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V_Val ScalarV::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref ScalarV::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef ScalarV::GetIntrin128ConstRef() const
	{
		return v;
	}

	__forceinline BoolV_Out ScalarV::AsBoolV() const
	{
		return BoolV( v );
	}

	__forceinline float ScalarV::Getf() const
	{
		return Vec::GetX( v );
	}

	__forceinline void ScalarV::Setf( float floatVal )
	{
		Vec::V4Set( v, floatVal );
	}

	__forceinline void ScalarV::Set( const float& rFloatVal )
	{
		Vec::V4Set( v, rFloatVal );
	}

	__forceinline int ScalarV::Geti() const
	{
		return Vec::GetXi( v );
	}

	__forceinline void ScalarV::Seti( int intVal ) 
	{
		return Vec::V4Set( v, intVal );
	}

	__forceinline void ScalarV::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	//============================================================================
	// Operators

	__forceinline BoolV_Out ScalarV::operator== (ScalarV_In b) const
	{
		return BoolV( Vec::V4IsEqualV( v, b.v ) );
	}

	__forceinline BoolV_Out ScalarV::operator!= (ScalarV_In b) const
	{
		return BoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline BoolV_Out ScalarV::operator< (ScalarV_In bigVector) const
	{
		return BoolV( Vec::V4IsLessThanV( v, bigVector.v ) );
	}

	__forceinline BoolV_Out ScalarV::operator<= (ScalarV_In bigVector) const
	{
		return BoolV( Vec::V4IsLessThanOrEqualV( v, bigVector.v ) );
	}

	__forceinline BoolV_Out ScalarV::operator> (ScalarV_In smallVector) const
	{
		return BoolV( Vec::V4IsGreaterThanV( v, smallVector.v ) );
	}

	__forceinline BoolV_Out ScalarV::operator>= (ScalarV_In smallVector) const
	{
		return BoolV( Vec::V4IsGreaterThanOrEqualV( v, smallVector.v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator* (ScalarV_In b) const
	{
		return ScalarV( Vec::V4Scale( v, b.v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator/ (ScalarV_In b) const
	{
		return ScalarV( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator+ (ScalarV_In b) const
	{
		return ScalarV( Vec::V4Add( v, b.v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator- (ScalarV_In b) const
	{
		return ScalarV( Vec::V4Subtract( v, b.v ) );
	}

	__forceinline void ScalarV::operator*= (ScalarV_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void ScalarV::operator/= (ScalarV_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void ScalarV::operator+= (ScalarV_In b)
	{
		v = Vec::V4Add( v, b.v );
	}

	__forceinline void ScalarV::operator-= (ScalarV_In b)
	{
		v = Vec::V4Subtract( v, b.v );
	}

	__forceinline ScalarV_Out ScalarV::operator+ () const
	{
		return ScalarV( v );
	}

	__forceinline ScalarV_Out ScalarV::operator- () const
	{
		return ScalarV( Vec::V4Negate( v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator| (ScalarV_In b) const
	{
		return ScalarV( Vec::V4Or( v, b.v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator& (ScalarV_In b) const
	{
		return ScalarV( Vec::V4And( v, b.v ) );
	}

	__forceinline ScalarV_Out ScalarV::operator^ (ScalarV_In b) const
	{
		return ScalarV( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void ScalarV::operator|= (ScalarV_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void ScalarV::operator&= (ScalarV_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void ScalarV::operator^= (ScalarV_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline ScalarV_Out ScalarV::operator~ () const
	{
		return ScalarV( Vec::V4InvertBits( v ) );
	}

	namespace sysEndian
	{
		template<> inline void SwapMe(ScalarV& v) { return SwapMe(v.GetIntrin128Ref()); }
	} // namespace sysEndian

} // namespace rage
