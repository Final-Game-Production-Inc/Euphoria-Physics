
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
#define DEF_Vec2VConstantInitializer(constant) \
	__forceinline Vec2V::Vec2V(e##constant##Initializer) \
	{ \
		Vec::V4Set( v, Vec::V4VConstant(V_##constant) ); \
	}

	// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !

	DEF_Vec2VConstantInitializer(ZERO                 )
	DEF_Vec2VConstantInitializer(ONE                  )
	DEF_Vec2VConstantInitializer(TWO                  )
	DEF_Vec2VConstantInitializer(THREE                )
	DEF_Vec2VConstantInitializer(FOUR                 )
	DEF_Vec2VConstantInitializer(FIVE                 )
	DEF_Vec2VConstantInitializer(SIX                  )
	DEF_Vec2VConstantInitializer(SEVEN                )
	DEF_Vec2VConstantInitializer(EIGHT                )
	DEF_Vec2VConstantInitializer(NINE                 )
	DEF_Vec2VConstantInitializer(TEN                  )
	DEF_Vec2VConstantInitializer(ELEVEN               )
	DEF_Vec2VConstantInitializer(TWELVE               )
	DEF_Vec2VConstantInitializer(THIRTEEN             )
	DEF_Vec2VConstantInitializer(FOURTEEN             )
	DEF_Vec2VConstantInitializer(FIFTEEN              )
	DEF_Vec2VConstantInitializer(NEGONE               )
	DEF_Vec2VConstantInitializer(NEGTWO               )
	DEF_Vec2VConstantInitializer(NEGTHREE             )
	DEF_Vec2VConstantInitializer(NEGFOUR              )
	DEF_Vec2VConstantInitializer(NEGFIVE              )
	DEF_Vec2VConstantInitializer(NEGSIX               )
	DEF_Vec2VConstantInitializer(NEGSEVEN             )
	DEF_Vec2VConstantInitializer(NEGEIGHT             )
	DEF_Vec2VConstantInitializer(NEGNINE              )
	DEF_Vec2VConstantInitializer(NEGTEN               )
	DEF_Vec2VConstantInitializer(NEGELEVEN            )
	DEF_Vec2VConstantInitializer(NEGTWELVE            )
	DEF_Vec2VConstantInitializer(NEGTHIRTEEN          )
	DEF_Vec2VConstantInitializer(NEGFOURTEEN          )
	DEF_Vec2VConstantInitializer(NEGFIFTEEN           )
	DEF_Vec2VConstantInitializer(NEGSIXTEEN           )

	DEF_Vec2VConstantInitializer(NEG_FLT_MAX          )
	DEF_Vec2VConstantInitializer(FLT_MAX              )
	DEF_Vec2VConstantInitializer(FLT_MIN              )
	DEF_Vec2VConstantInitializer(FLT_LARGE_2          )
	DEF_Vec2VConstantInitializer(FLT_LARGE_4          )
	DEF_Vec2VConstantInitializer(FLT_LARGE_6          )
	DEF_Vec2VConstantInitializer(FLT_LARGE_8          )
	DEF_Vec2VConstantInitializer(FLT_EPSILON          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_6          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_5          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_4          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_3          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_2          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_1          )
	DEF_Vec2VConstantInitializer(FLT_SMALL_12         )
	DEF_Vec2VConstantInitializer(ONE_PLUS_EPSILON     )
	DEF_Vec2VConstantInitializer(ONE_MINUS_FLT_SMALL_3)

	DEF_Vec2VConstantInitializer(ZERO_WONE            )
	DEF_Vec2VConstantInitializer(ONE_WZERO            )

	DEF_Vec2VConstantInitializer(X_AXIS_WONE          )
	DEF_Vec2VConstantInitializer(Y_AXIS_WONE          )
	DEF_Vec2VConstantInitializer(Z_AXIS_WONE          )

	DEF_Vec2VConstantInitializer(X_AXIS_WZERO         )
	DEF_Vec2VConstantInitializer(Y_AXIS_WZERO         )
	DEF_Vec2VConstantInitializer(Z_AXIS_WZERO         )

	DEF_Vec2VConstantInitializer(MASKX                )
	DEF_Vec2VConstantInitializer(MASKY                )
	DEF_Vec2VConstantInitializer(MASKZ                )
	DEF_Vec2VConstantInitializer(MASKW                )
	DEF_Vec2VConstantInitializer(MASKXY               )
	DEF_Vec2VConstantInitializer(MASKXZ               )
	DEF_Vec2VConstantInitializer(MASKXW               )
	DEF_Vec2VConstantInitializer(MASKYZ               )
	DEF_Vec2VConstantInitializer(MASKYW               )
	DEF_Vec2VConstantInitializer(MASKZW               )
	DEF_Vec2VConstantInitializer(MASKYZW              )
	DEF_Vec2VConstantInitializer(MASKXZW              )
	DEF_Vec2VConstantInitializer(MASKXYW              )
	DEF_Vec2VConstantInitializer(MASKXYZ              )
	DEF_Vec2VConstantInitializer(MASKXYZW             )

	DEF_Vec2VConstantInitializer(QUARTER              )
	DEF_Vec2VConstantInitializer(THIRD                )
	DEF_Vec2VConstantInitializer(HALF                 )
	DEF_Vec2VConstantInitializer(NEGHALF              )
	DEF_Vec2VConstantInitializer(INF                  )
	DEF_Vec2VConstantInitializer(NEGINF               )
	DEF_Vec2VConstantInitializer(NAN                  )
	DEF_Vec2VConstantInitializer(LOG2_TO_LOG10        )

	DEF_Vec2VConstantInitializer(ONE_OVER_1024        )
	DEF_Vec2VConstantInitializer(ONE_OVER_PI          )
	DEF_Vec2VConstantInitializer(TWO_OVER_PI          )
	DEF_Vec2VConstantInitializer(PI                   )
	DEF_Vec2VConstantInitializer(TWO_PI               )
	DEF_Vec2VConstantInitializer(PI_OVER_TWO          )
	DEF_Vec2VConstantInitializer(NEG_PI               )
	DEF_Vec2VConstantInitializer(NEG_PI_OVER_TWO      )
	DEF_Vec2VConstantInitializer(TO_DEGREES           )
	DEF_Vec2VConstantInitializer(TO_RADIANS           )
	DEF_Vec2VConstantInitializer(SQRT_TWO		      )
	DEF_Vec2VConstantInitializer(ONE_OVER_SQRT_TWO	  )
	DEF_Vec2VConstantInitializer(SQRT_THREE		      )
	DEF_Vec2VConstantInitializer(E					  )

	DEF_Vec2VConstantInitializer(INT_1                )
	DEF_Vec2VConstantInitializer(INT_2                )
	DEF_Vec2VConstantInitializer(INT_3                )
	DEF_Vec2VConstantInitializer(INT_4                )
	DEF_Vec2VConstantInitializer(INT_5                )
	DEF_Vec2VConstantInitializer(INT_6                )
	DEF_Vec2VConstantInitializer(INT_7                )
	DEF_Vec2VConstantInitializer(INT_8                )
	DEF_Vec2VConstantInitializer(INT_9                )
	DEF_Vec2VConstantInitializer(INT_10               )
	DEF_Vec2VConstantInitializer(INT_11               )
	DEF_Vec2VConstantInitializer(INT_12               )
	DEF_Vec2VConstantInitializer(INT_13               )
	DEF_Vec2VConstantInitializer(INT_14               )
	DEF_Vec2VConstantInitializer(INT_15               )

	DEF_Vec2VConstantInitializer(7FFFFFFF             )
	DEF_Vec2VConstantInitializer(80000000             )

#undef DEF_Vec2VConstantInitializer

	__forceinline Vec2V::Vec2V()
	{
	}

#if __WIN32PC
	__forceinline Vec2V::Vec2V(Vec2V_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline Vec2V_ConstRef Vec2V::operator=(Vec2V_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline Vec2V::Vec2V(const float& s1, const float& s2)
	{
		Vec::V2Set( v, s1, s2 );
	}

	__forceinline Vec2V::Vec2V(ScalarV_In a, ScalarV_In b)
		: v( Vec::V4MergeXY( a.v, b.v ) )
	{
		/*v = Vec::V4MergeXY( a.v, b.v );*/
	}

	__forceinline Vec2V::Vec2V(Vec::Vector_4V_In a)
		: v( a )
	{
	}

	__forceinline Vec2V::Vec2V(ScalarV_In a)
		: v( a.v )
	{
	}

	__forceinline Vec2V::Vec2V(VecBoolV_In a)
		: v( a.v )
	{
	}

	__forceinline Vec2V::Vec2V(BoolV_In a)
		: v( a.v )
	{
	}

	__forceinline void Vec2V::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V_Val Vec2V::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref Vec2V::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef Vec2V::GetIntrin128ConstRef() const
	{
		return v;
	}

	__forceinline VecBoolV_Out Vec2V::AsVecBoolV() const
	{
		return VecBoolV( v );
	}

	__forceinline ScalarV_Out Vec2V::GetX() const
	{
		return ScalarV( Vec::V4SplatX(v) );
	}

	__forceinline ScalarV_Out Vec2V::GetY() const
	{
		return ScalarV( Vec::V4SplatY(v) );
	}

	__forceinline void Vec2V::SetX( ScalarV_In newX )
	{
		v = Vec::V4PermuteTwo<Vec::X1,Vec::Y2,Vec::Y1,Vec::X1>( newX.GetIntrin128(), v );
	}

	__forceinline void Vec2V::SetY( ScalarV_In newY )
	{
		v = Vec::V4PermuteTwo<Vec::X1,Vec::Y2,Vec::Y1,Vec::X1>( v, newY.GetIntrin128() );
	}

	__forceinline void Vec2V::SetXInMemory( ScalarV_In newX )
	{
		Vec::SetXInMemory( v, newX.GetIntrin128() );
	}

	__forceinline void Vec2V::SetYInMemory( ScalarV_In newY )
	{
		Vec::SetYInMemory( v, newY.GetIntrin128() );
	}

	__forceinline float Vec2V::GetXf() const
	{
		return Vec::GetX( v );
	}

	__forceinline float Vec2V::GetYf() const
	{
		return Vec::GetY( v );
	}

	__forceinline int Vec2V::GetXi() const
	{
		return Vec::GetXi( v );
	}

	__forceinline int Vec2V::GetYi() const
	{
		return Vec::GetYi( v );
	}

	__forceinline void Vec2V::SetElemf( unsigned elem, float fVal )
	{
		VecAssertMsg( elem <= 1 , "Invalid elem!" );
		(*this)[elem] = fVal;
	}

	__forceinline float Vec2V::GetElemf( unsigned elem ) const
	{
		VecAssertMsg( elem <= 1 , "Invalid elem!" );
		return (*this)[elem];
	}

	__forceinline void Vec2V::SetElemi( unsigned elem, int intVal )
	{
		VecAssertMsg( elem <= 1 , "Invalid elem!" );
		((int*)(this))[elem] = intVal;
	}

	__forceinline int Vec2V::GetElemi( unsigned elem ) const
	{
		VecAssertMsg( elem <= 1 , "Invalid elem!" );
		return ((const int*)(this))[elem];
	}

	__forceinline void Vec2V::SetXf( float floatVal )
	{
		Vec::SetXInMemory( v, floatVal );
	}

	__forceinline void Vec2V::SetYf( float floatVal )
	{
		Vec::SetYInMemory( v, floatVal );
	}

	__forceinline void Vec2V::SetXi( int intVal )
	{
		Vec::SetXInMemory( v, intVal );
	}

	__forceinline void Vec2V::SetYi( int intVal )
	{
		Vec::SetYInMemory( v, intVal );
	}

	__forceinline void Vec2V::SetX( const float& floatVal )
	{
		Vec::SetX( v, floatVal );
	}

	__forceinline void Vec2V::SetY( const float& floatVal )
	{
		Vec::SetY( v, floatVal );
	}

	__forceinline void Vec2V::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	//============================================================================
	// Operators

	__forceinline VecBoolV_Out Vec2V::operator== (Vec2V_In b) const
	{
		return VecBoolV( Vec::V4IsEqualV( v, b.v ) );
	}

	__forceinline VecBoolV_Out Vec2V::operator!= (Vec2V_In b) const
	{
		return VecBoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline VecBoolV_Out	Vec2V::operator< (Vec2V_In bigVector) const
	{
		return VecBoolV( Vec::V4IsLessThanV( v, bigVector.v ) );
	}

	__forceinline VecBoolV_Out	Vec2V::operator<= (Vec2V_In bigVector) const
	{
		return VecBoolV( Vec::V4IsLessThanOrEqualV( v, bigVector.v ) );
	}

	__forceinline VecBoolV_Out	Vec2V::operator> (Vec2V_In smallVector) const
	{
		return VecBoolV( Vec::V4IsGreaterThanV( v, smallVector.v ) );
	}

	__forceinline VecBoolV_Out	Vec2V::operator>= (Vec2V_In smallVector) const
	{
		return VecBoolV( Vec::V4IsGreaterThanOrEqualV( v, smallVector.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator* (Vec2V_In b) const
	{
		return Vec2V( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec2V_Out operator* (ScalarV_In a, Vec2V_In b)
	{
		return Vec2V( Vec::V4Scale( a.GetIntrin128(), b.GetIntrin128() ) );
	}

	__forceinline Vec2V_Out Vec2V::operator* (ScalarV_In b) const
	{
		return Vec2V( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator/ (Vec2V_In b) const
	{
		return Vec2V( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator/ (ScalarV_In b) const
	{
		return Vec2V( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator+ (Vec2V_In b) const
	{
		return Vec2V( Vec::V4Add( v, b.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator- (Vec2V_In b) const
	{
		return Vec2V( Vec::V4Subtract( v, b.v ) );
	}

	__forceinline void Vec2V::operator*= (Vec2V_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec2V::operator*= (ScalarV_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec2V::operator/= (Vec2V_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec2V::operator/= (ScalarV_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec2V::operator+= (Vec2V_In b)
	{
		v = Vec::V4Add( v, b.v );
	}

	__forceinline void Vec2V::operator-= (Vec2V_In b)
	{
		v = Vec::V4Subtract( v, b.v );
	}

	__forceinline Vec2V_Out Vec2V::operator+ () const
	{
		return Vec2V( v );
	}

	__forceinline Vec2V_Out Vec2V::operator- () const
	{
		return Vec2V( Vec::V4Negate( v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator| (Vec2V_In b) const
	{
		return Vec2V( Vec::V4Or( v, b.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator& (Vec2V_In b) const
	{
		return Vec2V( Vec::V4And( v, b.v ) );
	}

	__forceinline Vec2V_Out Vec2V::operator^ (Vec2V_In b) const
	{
		return Vec2V( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void Vec2V::operator|= (Vec2V_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void Vec2V::operator&= (Vec2V_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void Vec2V::operator^= (Vec2V_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline Vec2V_Out Vec2V::operator~ () const
	{
		return Vec2V( Vec::V4InvertBits( v ) );
	}

	__forceinline const float& Vec2V::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 1 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Vec2V::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 1 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	template <u32 permX, u32 permY>
	__forceinline Vec2V_Out Vec2V::Get() const
	{
		CompileTimeAssert( !(permX == Vec::X && permY == Vec::Y) );
		// Useless permute!

		if(permX == Vec::X && permY == Vec::X)
		{
			return Vec2V( Vec::V4SplatX( v ) );
		}
		else if(permX == Vec::Y && permY == Vec::X)
		{
			return Vec2V( Vec::V4Permute<Vec::Y, Vec::X, Vec::W, Vec::W>( v ) );
		}
		else if(permX == Vec::Y && permY == Vec::Y)
		{
			return Vec2V( Vec::V4SplatY( v ) );
		}
		else // permX == Vec::X && permY == Vec::Y
		{
			VecAssertMsg( 0, "This is a useless permute!" );
			return *this;
		}
	}

	namespace sysEndian
	{
		template<> inline void SwapMe(Vec2V& v) { return SwapMe(v.GetIntrin128Ref()); }
	} // namespace sysEndian


} // namespace rage
